# pas
Perry's audio server

## What is pas?

pas is an audio server capable of sending multiple **concurrent analog** stereo feeds to some (as yet undetermined maximum) number of outboard DACs. Each DAC will drive a separate audio zone in a multi-zone or multi-room installation. 

A key feature of pas is that it is quite light weight, capable of running multiple concurrent streams from an ARM-based development board. See directly below for information about the sbc pas being being developed on.

**To repeat: pas does not stream digital data. It (via DACs) emits analog audio for injection into a means of analog audio distribution.**

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

## Who is Perry?

This is <a href="https://en.wikipedia.org/wiki/Perry_Kivolowitz">me</a>.

I contribute to <a href="https://en.wikipedia.org/wiki/SilhouetteFX">this</a>.

I used to teach CS <a href="http://www.cs.wisc.edu/">here</a> but now I teach CS <a href="https://www.carthage.edu/">here</a>.

## Why did I start pas when \*.\* is available?

Because

## Is pas MPD-client compatible?

I hope this is added someday. As of this writing, pas is not yet ready for any UI beyond a text-based debugging tool.

## What additional information is available about pas?

The <a href="https://github.com/pkivolowitz/pas/wiki">wiki</a> is maintained.

## Is there a style guide?

Yes. It is in the wiki.

## Are other contributors welcomed?

Not yet but I am very hopeful pas becomes worthy of other developer's interest. It would be so cool if my kid or one of my former students were among those future contributors.

## How do computer programmers kill zombies?

Read <a href="https://www.amazon.com/Get-Off-My-Zombie-Novel-ebook/dp/B00DQ26J8G/">Get off my L@wn</a> and find out. And don't steal it like everybody else, please? It's just $2.99 for crap's sake.

## Is pas shitty code?

I take pride in my work. I strive not to write shitty code. If you believe I have written shitty code, please let me know in a non-shitty way.

## Does pas contain much *cool* code?

No. When I wrote the <a href="https://en.wikipedia.org/wiki/Keystroke_logging">first documented keylogger</a> back in the day I believed in cool code.

Alas, I am now a gray-beard. 

I no longer believe in cool code. 

I believe in *working* code.

I believe in *maintainable* code. 

I believe cool code is for much younger people who are much smarter than me.

Seriously, one might expect me to say something like "I've forgotten more than you'll ever know." The truth is, I've forgetten what I've forgotten so there is a *reasonable chance* you are smarter than me. Maybe you are better looking too. You bastard. I hate you.

## When will pas be done?

I am a busy person and easily lose interest in things. So ¯\_(ツ)_/¯.

## What's the best computer ever made?

The Amiga.

\<mic drop>
