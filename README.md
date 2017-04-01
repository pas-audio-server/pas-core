# pas
Perry's audio server

The house my wife and I just purchased has a number of rooms with in-room speakers. pas is an audio server capable of
sending multiple stereo feeds to some (undetermined maximum) number of outboard DACs. Each DAC, of course, will drive
an amplifier for each room.

pas does **not** stream digital data. It (via a DAC) emits analog audio.

Some information about pas:
- pas is Linux based.
- pas is heavily multithreaded and likes multicore machines.
- pas is written to be headless. UI will be provided via a web server.
- audio is emitted using portaudio using USB audio devices.
- audio is decoded using ffmpeg.
- data is maintained using sqlite.
- web interface is expected to be node.js with some salad framework on top.

- pas is being developed on an odroid XU4.
- pas is being developed using an audioengine D3 USB DAC.

This code is in its earliest stages and very far from complete.

Please see the wiki for style guide and other information.
