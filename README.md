# ACard Studio

ACard Studio is a visual block editor for programming the M5Stack Cardputer ADV.

## Editor
- Cardputer-specific block categories: Display, Keyboard, Speaker, Microphone, Wi-Fi, Bluetooth LE, Motion Sensors, Battery, microSD Files, Infrared, Device & Time, Program Flow, Logic & Math, and Variables.
- Drag blocks from the block library into the program workspace; drag existing blocks to rearrange them.
- Click a palette block to add it without dragging.
- Edit each block's setting, move it with the arrow controls, or delete it.
- Cardputer screen preview for a small set of display, sound, date/time and timer actions.
- Import/export version 3 .airproj projects. Older project files containing a blocks array can be imported.

## Native ACard OS firmware
A separate ESP-IDF/C++ firmware project now lives in `firmware/` and targets the M5Stack Cardputer ADV (ESP32-S3FN8). It includes initial display bring-up, ADV peripheral detection, optional microSD mounting and an on-device performance benchmark suite. See [firmware/README.md](firmware/README.md) for build instructions and current implementation status.

## Use the editor
Open index.html in a modern browser or host the folder on a static web server.

## Hardware compatibility
The browser preview remains a simulator. Physical hardware operations require the native firmware runtime. The native OS is under active development; not every editor block is connected to a hardware implementation yet.
