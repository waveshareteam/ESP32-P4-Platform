# xiaozhi-esp32 adaptations

This directory contains the small OGG demuxer imported from
78/xiaozhi-esp32 at commit
0ec696f64f5843ca0f5fcf700ae45977d1dcd2e8.

The matching Chinese activation prompt and digit recordings are stored in
../../assets/prompts/:

- activation.ogg
- 0.ogg through 9.ogg
- success.ogg

The recordings are 16 kHz mono Opus audio in OGG containers. The firmware
demuxes each file and feeds raw Opus packets to esp_audio_codec, avoiding a
dependency on the complete upstream audio framework.

The XiaozhiApp application also adapts the upstream WebSocket protocol,
wake-word Opus pre-roll, automatic listening-turn flow, and dynamic Opus
playback configuration from commit
1b48ebd7863695bf80d384c6c09af6299a6d7d0e. The local integration keeps the
Brookesia application lifecycle, board codec access, single-microphone AFE,
and UI instead of importing the complete upstream board and application
framework.

The imported code and recordings are distributed under the MIT license in
LICENSE.
