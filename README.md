# pas
Perry's audio server

## What is pas?

pas is an audio server capable of sending multiple stereo feeds to some (undetermined maximum) number of outboard DACs. Each DAC will drive a seperate audio zone. pas does **not** stream digital data. It (via DACs) emits analog audio.

Some information about pas:
- pas is Linux based.
- pas is heavily multithreaded and likes multicore machines.
- pas is written to be headless. UI will be provided via a web server.
- audio is emitted using pulseaudio via USB audio devices.
- audio is decoded using ffmpeg.
- data is maintained using sqlite.
- web interface is expected to be node.js with some salad framework on top.

- pas is being developed on an odroid XU4.
- pas is being developed using an audioengine D3 USB DAC, a 24 bits / sample device.

## Why did I start pas when * is available?

Because

## Is pas MPD-client compatible?

I hope this is added someday. As of this writing, pas is not yet ready for any UI beyond a text-based debugging tool.

## What additional information is available about pas?

The wiki is maintained.

## Is there a style guide?

Yes

## Are other contributors welcome?

Not yet but I am very hopeful pas becomes worthy of other developer's interest.

## Is pas shitty code?

I strive not to write shitty code. I take pride in my work. I welcome constructive criticism.

## Does pas contain cool code?

I am a gray-beard. I don't beleive in cool code. I beleive in working and maintainable code. Cool code is for young people smarter than I.

## When will pas be done?
I am a busy person and lose interest in non-paying projects easily. So ¯\_(ツ)_/¯.

