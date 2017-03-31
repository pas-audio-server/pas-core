# pas
Perry's audio server

The house my wife and I just purchased has a number of rooms with in-room speakers. pas is a single audio server for multiple
streams supporting simultaneous playback of (either the same or different) audio to each room.

Some information about pas:
- pas is Linux based and is being developed on a odroid XU4 (ARM).
- pas is heavily multithreaded and likes multicore machines
- audio is emitted using portaudio.
- audio is decoded using ffmpeg.
- data is maintained using sqlite.
- web interface is expected to be node.js with some salad framework on top.

This code is in its earliest stages and very far from complete.
