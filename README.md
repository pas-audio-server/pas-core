# pas
Perry's audio server

## What is pas?

pas is an audio server capable of sending multiple **analog** stereo feeds to some (as yet undetermined maximum) number of outboard DACs. Each DAC will drive a seperate audio zone in a multi-zone or multi-room installation. 

pas does **not** stream digital data. It (via DACs) emits analog audio.

Some information about pas:
- pas is Linux based.
- pas is heavily multithreaded and likes multicore machines.
- pas is written to be headless. A UI will be provided via a web server.
- audio is emitted using pulseaudio via USB audio devices.
- audio is decoded using ffmpeg so pas supports those formats supported by ffmpeg.
- data is maintained using sqlite.
- pas *may* expose a MPD-compatible interface.

- pas is being developed on an odroid XU4.
- pas is being developed using an audioengine D3 USB DAC, a 24 bits / sample device.

## Why did I start pas when * is available?

Because

## Is pas MPD-client compatible?

I hope this is added someday. As of this writing, pas is not yet ready for any UI beyond a text-based debugging tool.

## What additional information is available about pas?

The <a href="https://github.com/pkivolowitz/pas/wiki">wiki</a> is maintained.

## Is there a style guide?

Yes. It is in the wiki.

## Are other contributors welcomed?

Not yet but I am very hopeful pas becomes worthy of other developer's interest. It would be so cool if my kid or one of my former students were among those future contributors.

## Is pas shitty code?

I take pride in my work. I strive not to write shitty code. I welcome constructive criticism.

## Does pas contain much *cool* code?

No. When I wrote the <a href="https://en.wikipedia.org/wiki/Keystroke_logging">first documented key-logger</a> back in the day I believed in cool code.

Alas, I am now a gray-beard. 

I don't believe in cool code. 

I believe in *working* code.

I believe in *maintainable* code. 

Cool code is for younger people smarter than me.

Seriously, one might expect me to say something like "I've forgotten more than you'll ever know." The truth is, I've forgetten what I've forgotten so there *is a good chance* you're smarter than me.

## When will pas be done?

I am a busy person and easily lose interest. So ¯\_(ツ)_/¯.

## What's the best computer ever made?

The Amiga.

\<mic drop>
