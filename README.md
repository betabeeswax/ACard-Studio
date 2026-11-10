# ACard Studio

ACard Studio is a Scratch-inspired block editor prototype for M5Stack Cardputer ADV and AIR OS.

## Current editor features
- Broad Scratch 3-style core block catalogue: Motion, Looks, Sound, Events, Control, Sensing, Operators, Variables, Lists and My Blocks.
- Scratch's built-in extension catalogues: Music, Pen, Video Sensing, Face Sensing, Text to Speech and Translate. Third-party hardware extensions have been removed.
- Cardputer-specific block catalogue for display drawing, keyboard, speaker, microphone, battery, IMU, Wi-Fi, BLE, microSD, IR and device information.
- Multiple sprites, built-in emoji costumes, sprite selection and costume selection.
- Ordered per-sprite scripts, editable block values, block reorder and remove.
- Keyboard-controlled on-screen pointer: arrow keys move; Enter selects; Shift + arrows move faster.
- Import/export version 2 .airproj files; old prototype files with a blocks array can be imported.
- Lightweight stage preview and a small set of executable preview actions (movement, visibility, size, speech/display and costumes).

## Use
Open `index.html` in a modern browser or host the folder on a static web server.

## Important compatibility notes
This is an expanding Scratch-style editor, not the official Scratch 3 runtime. Blocks are catalogued for editing, but not every block has an execution implementation yet. Camera/face sensing, Makey Makey, micro:bit, Go Direct and LEGO blocks require their matching external hardware and are not available on the Cardputer itself. Cardputer hardware blocks need a native AIR OS/Cardputer runtime before they can control the physical device. The web preview is a simulator, not a connection to the Cardputer.

The Cardputer has no touchscreen. Its editor interaction is designed around a keyboard cursor and select key. The current website preview uses arrow keys and Enter to demonstrate that control model.
