#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_chip_info.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c_master.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "nvs_flash.h"

static const char *TAG = "ACARD";
namespace board {
constexpr int LCD_BL=38, LCD_RST=33, LCD_DC=34, LCD_MOSI=35, LCD_SCLK=36, LCD_CS=37;
constexpr int I2C_SDA=8, I2C_SCL=9;
constexpr int SD_CS=12, SD_MOSI=14, SD_MISO=39, SD_SCLK=40;
constexpr int WIDTH=240, HEIGHT=135;
constexpr uint8_t TCA8418=0x34, ES8311=0x18, BMI270=0x69;
}

class Display {
 public:
  esp_err_t begin() {
    gpio_config_t out{}; out.mode=GPIO_MODE_OUTPUT; out.pin_bit_mask=(1ULL<<board::LCD_BL)|(1ULL<<board::LCD_RST)|(1ULL<<board::LCD_DC);
    ESP_RETURN_ON_ERROR(gpio_config(&out), TAG, "LCD GPIO config");
    gpio_set_level((gpio_num_t)board::LCD_BL,0); gpio_set_level((gpio_num_t)board::LCD_RST,0); vTaskDelay(pdMS_TO_TICKS(30)); gpio_set_level((gpio_num_t)board::LCD_RST,1); vTaskDelay(pdMS_TO_TICKS(120));
    spi_bus_config_t bus{}; bus.mosi_io_num=board::LCD_MOSI; bus.miso_io_num=-1; bus.sclk_io_num=board::LCD_SCLK; bus.quadwp_io_num=-1; bus.quadhd_io_num=-1; bus.max_transfer_sz=board::WIDTH*2+8;
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST,&bus,SPI_DMA_CH_AUTO),TAG,"LCD SPI bus");
    spi_device_interface_config_t dev{}; dev.clock_speed_hz=40*1000*1000; dev.mode=0; dev.spics_io_num=board::LCD_CS; dev.queue_size=1;
    ESP_RETURN_ON_ERROR(spi_bus_add_device(SPI2_HOST,&dev,&spi_),TAG,"LCD SPI device");
    command(0x01); vTaskDelay(pdMS_TO_TICKS(120));
    command(0x11); vTaskDelay(pdMS_TO_TICKS(120));
    uint8_t colmod=0x55; command(0x3A,&colmod,1);
    uint8_t madctl=0x00; command(0x36,&madctl,1);
    command(0x21); command(0x13); command(0x29); vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level((gpio_num_t)board::LCD_BL,1);
    fill(0x0000); return ESP_OK;
  }
  void fill(uint16_t color) { fillRect(0,0,board::WIDTH,board::HEIGHT,color); }
  void fillRect(int x,int y,int w,int h,uint16_t color) {
    x=std::max(0,x); y=std::max(0,y); w=std::min(w,board::WIDTH-x); h=std::min(h,board::HEIGHT-y); if(w<=0||h<=0||!spi_) return;
    setWindow(x,y,x+w-1,y+h-1);
    std::vector<uint8_t> row((size_t)w*2);
    for(int i=0;i<w;i++){ row[i*2]=(uint8_t)(color>>8); row[i*2+1]=(uint8_t)color; }
    for(int i=0;i<h;i++) data(row.data(),row.size());
  }
  void pixel(int x,int y,uint16_t c){ fillRect(x,y,1,1,c); }
  void text(int x,int y,const char *s,uint16_t fg,uint16_t bg=0x0000,int scale=1) {
    if(!s)return; for(;*s;s++){ char c=*s; if(c=='\n'){y+=8*scale;x=0;continue;} const uint8_t *g=glyph(c); for(int col=0;col<5;col++) for(int row=0;row<7;row++) if(g[col]&(1<<row)) fillRect(x+col*scale,y+row*scale,scale,scale,fg); else if(bg!=0xFFFF) fillRect(x+col*scale,y+row*scale,scale,scale,bg); x+=6*scale; if(x>board::WIDTH-6*scale){x=0;y+=8*scale;} }
  }
 private:
  spi_device_handle_t spi_=nullptr;
  void command(uint8_t c,const uint8_t *p=nullptr,size_t n=0){ gpio_set_level((gpio_num_t)board::LCD_DC,0); spi_transaction_t t{}; t.length=8;t.tx_data[0]=c;t.flags=SPI_TRANS_USE_TXDATA; spi_device_transmit(spi_,&t); if(n){gpio_set_level((gpio_num_t)board::LCD_DC,1);spi_transaction_t d{};d.length=n*8;d.tx_buffer=p;spi_device_transmit(spi_,&d);} }
  void data(const uint8_t *p,size_t n){gpio_set_level((gpio_num_t)board::LCD_DC,1);spi_transaction_t t{};t.length=n*8;t.tx_buffer=p;spi_device_transmit(spi_,&t);}
  void setWindow(int x0,int y0,int x1,int y1){uint8_t b[4];b[0]=x0>>8;b[1]=x0;b[2]=x1>>8;b[3]=x1;command(0x2A,b,4);b[0]=y0>>8;b[1]=y0;b[2]=y1>>8;b[3]=y1;command(0x2B,b,4);command(0x2C);}
  const uint8_t *glyph(char c) {
    static const uint8_t blank[5]={0,0,0,0,0};
    static const uint8_t font[36][5]={
      {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},{0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},{0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},{0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},{0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},{0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,0x08,0x14,0x63},{0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},
      {0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},{0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},{0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E}
    };
    if(c>='a'&&c<='z')c-=32; if(c>='A'&&c<='Z')return font[c-'A']; if(c>='0'&&c<='9')return font[26+c-'0'];
    static const uint8_t dash[5]={0x08,0x08,0x08,0x08,0x08}, colon[5]={0,0x36,0x36,0,0}, dot[5]={0,0x60,0x60,0,0};
    if(c=='-')return dash;if(c==':')return colon;if(c=='.')return dot;return blank;
  }
};

class Hardware {
 public:
  esp_err_t beginI2C(){
    i2c_master_bus_config_t cfg{};cfg.i2c_port=I2C_NUM_0;cfg.sda_io_num=(gpio_num_t)board::I2C_SDA;cfg.scl_io_num=(gpio_num_t)board::I2C_SCL;cfg.clk_source=I2C_CLK_SRC_DEFAULT;cfg.glitch_ignore_cnt=7;cfg.flags.enable_internal_pullup=true;
    return i2c_new_master_bus(&cfg,&bus_);
  }
  bool probe(uint8_t address){return bus_ && i2c_master_probe(bus_,address,100)==ESP_OK;}
  bool cardputerKeyboard(){return probe(board::TCA8418);}
  bool imu(){return probe(board::BMI270);}
  bool audioCodec(){return probe(board::ES8311);}
  esp_err_t mountSd(){
    sdmmc_host_t host=SDSPI_HOST_DEFAULT();host.slot=SPI3_HOST;
    spi_bus_config_t bus{};bus.mosi_io_num=board::SD_MOSI;bus.miso_io_num=board::SD_MISO;bus.sclk_io_num=board::SD_SCLK;bus.quadwp_io_num=-1;bus.quadhd_io_num=-1;bus.max_transfer_sz=4096;
    esp_err_t e=spi_bus_initialize((spi_host_device_t)host.slot,&bus,SPI_DMA_CH_AUTO);if(e!=ESP_OK&&e!=ESP_ERR_INVALID_STATE)return e;
    sdspi_device_config_t slot=SDSPI_DEVICE_CONFIG_DEFAULT();slot.gpio_cs=(gpio_num_t)board::SD_CS;slot.host_id=(spi_host_device_t)host.slot;
    esp_vfs_fat_sdmmc_mount_config_t mount{};mount.format_if_mount_failed=false;mount.max_files=3;mount.allocation_unit_size=16*1024;
    return esp_vfs_fat_sdspi_mount("/sdcard",&host,&slot,&mount,&card_);
  }
  bool sdReady()const{return card_!=nullptr;}
  bool sdBenchmark(double &kbps){
    if(!sdReady())return false;FILE *f=fopen("/sdcard/acard-benchmark.tmp","wb");if(!f)return false;
    static uint8_t buf[4096];for(size_t i=0;i<sizeof(buf);i++)buf[i]=(uint8_t)(i*37u);
    const size_t total=256*1024;int64_t start=esp_timer_get_time();size_t written=0;
    while(written<total){size_t n=fwrite(buf,1,sizeof(buf),f);if(n!=sizeof(buf))break;written+=n;}
    fflush(f);fclose(f);int64_t elapsed=esp_timer_get_time()-start;remove("/sdcard/acard-benchmark.tmp");
    if(written!=total||elapsed<=0)return false;kbps=(written/1024.0)/(elapsed/1000000.0);return true;
  }
 private:
  i2c_master_bus_handle_t bus_=nullptr;sdmmc_card_t *card_=nullptr;
};

struct BenchResult { double cpu_mops=0, display_mpix=0, sd_kbps=0; uint32_t free_heap=0, largest_heap=0; bool sd_ok=false; int64_t display_us=0; };
static Display display;
static Hardware hardware;
static BenchResult results;

static void runBenchmarks(){
  ESP_LOGI(TAG,"Starting native performance suite");
  volatile uint32_t x=0x12345678;constexpr uint32_t iterations=1200000;
  int64_t start=esp_timer_get_time();for(uint32_t i=0;i<iterations;i++){x^=x<<13;x^=x>>17;x^=x<<5;}int64_t elapsed=esp_timer_get_time()-start;
  results.cpu_mops=(iterations/(double)elapsed);
  results.free_heap=heap_caps_get_free_size(MALLOC_CAP_8BIT);results.largest_heap=heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  start=esp_timer_get_time();for(int i=0;i<8;i++)display.fillRect(0,0,board::WIDTH,board::HEIGHT,(i&1)?0x07E0:0x001F);results.display_us=esp_timer_get_time()-start;
  const double pixels=(double)board::WIDTH*board::HEIGHT*8.0;results.display_mpix=pixels/(results.display_us>0?results.display_us:1);
  results.sd_ok=hardware.sdBenchmark(results.sd_kbps);
  ESP_LOGI(TAG,"BENCH,cpu_mops=%.3f,display_mpix_s=%.3f,display_ms=%.2f,heap_free=%u,heap_largest=%u,sd_kB_s=%.1f,sd_ok=%d",results.cpu_mops,results.display_mpix,results.display_us/1000.0,results.free_heap,results.largest_heap,results.sd_kbps,results.sd_ok);
  (void)x;
}

static void renderHome(bool keyboard,bool imu,bool codec){
  display.fill(0x0000);
  display.fillRect(0,0,240,18,0x18E3);display.text(5,5,"ACARD OS",0xFFFF,0x18E3,1);
  display.text(5,26,"NATIVE ESP-IDF",0x07FF);
  display.text(5,40,"CPU: 240 MHZ",0xFFFF);
  display.text(5,54,keyboard?"KEYBOARD: FOUND":"KEYBOARD: CHECK",keyboard?0x07E0:0xF800);
  display.text(5,68,imu?"BMI270: FOUND":"BMI270: CHECK",imu?0x07E0:0xF800);
  display.text(5,82,codec?"ES8311: FOUND":"ES8311: CHECK",codec?0x07E0:0xF800);
  display.text(5,98,"BENCHMARK COMPLETE",0xFFE0);
  char line[48];snprintf(line,sizeof(line),"CPU %.2f MOPS",results.cpu_mops);display.text(5,110,line,0xFFFF);
  ESP_LOGI(TAG,"ACard OS ready. CPU benchmark: %.3f Mops/s",results.cpu_mops);
}

extern "C" void app_main(void){
  esp_err_t nvs=nvs_flash_init();if(nvs==ESP_ERR_NVS_NO_FREE_PAGES||nvs==ESP_ERR_NVS_NEW_VERSION_FOUND){ESP_ERROR_CHECK(nvs_flash_erase());nvs_flash_init();}
  ESP_LOGI(TAG,"ACard OS native boot");
  ESP_ERROR_CHECK(display.begin());
  bool i2cOk=hardware.beginI2C()==ESP_OK;
  bool keyboard=i2cOk&&hardware.cardputerKeyboard();
  bool imu=i2cOk&&hardware.imu();
  bool codec=i2cOk&&hardware.audioCodec();
  bool sd=false;if(hardware.mountSd()==ESP_OK)sd=true;
  ESP_LOGI(TAG,"Hardware: I2C=%d keyboard=%d BMI270=%d ES8311=%d microSD=%d",i2cOk,keyboard,imu,codec,sd);
  display.fill(0x0000);display.text(5,24,"ACARD OS",0x07FF,0x0000,2);display.text(5,52,"BOOTING NATIVE CORE",0xFFFF);
  display.text(5,70,"RUNNING BENCHMARKS...",0xFFE0);vTaskDelay(pdMS_TO_TICKS(350));
  runBenchmarks();renderHome(keyboard,imu,codec);
  while(true){vTaskDelay(pdMS_TO_TICKS(1000));}
}
