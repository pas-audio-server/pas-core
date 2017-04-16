# pas
Perry's audio server

## Latest Development Highlight

A curses-based client driving three DACs simultaneously.

![Curses Client](https://scontent-atl3-1.xx.fbcdn.net/v/t31.0-8/17917132_1994665244097111_4372928553100760128_o.jpg?oh=4399322525776bc5fe9cdd3345cef5ce&oe=594F65FC)

## Sponsors

**AUDIO DAC MAKERS - WE NEED MORE DACS TO FIND OUT HOW MANY CONCURRENT STREAMS CAN BE SUPPORTED ON A GIVEN CLASS OF MACHINE. I AM SEEKING SPONSORS TO PROVIDE ONE OR MORE OF THEIR USB DAC PRODUCTS. SPONSORS GET:**
1. **Promoted here.**
2. **Ceritification as being pas compatible (and pas will be YUGE!)**
3. **Our thanks!**

PLEASE CONTACT ME IF INTERESTED, USING MY <a href="https://www.linkedin.com/in/perrykivolowitz/">LINKEDIN</a> ACCOUNT.

## What is pas?

pas is an audio server capable of sending multiple **concurrent analog** stereo feeds to some (as yet undetermined maximum) number of outboard DACs. Each DAC drives a separate audio zone in a multi-zone or multi-room installation. 

A key feature of pas is that it is quite light weight, capable of running multiple concurrent streams even from a $50 ARM-based dev-board. See directly below for information about the sbc pas being being developed on.

**To repeat: pas does not stream digital data. It (via DACs) emits analog audio for injection into a means of analog audio distribution.**

Some information about pas:
- pas is Linux based.
- pas is heavily multithreaded and likes multicore machines.
- pas is written to be headless. UI's are provided via ssh or other means such as a web server.
- audio is emitted using <a href="https://www.freedesktop.org/wiki/Software/PulseAudio/">pulseaudio</a> via USB DACs.
- audio is decoded using <a href="https://ffmpeg.org/">ffmpeg</a> so pas supports those formats supported by ffmpeg.
- data is maintained using <a href="https://www.mysql.com/">MySQL</a>.
- pas *may* expose a MPD-compatible interface as the pas API is quite robust.

- pas is being developed on an odroid XU4.
- pas is being developed using an audioengine D3 USB DAC and a DragonFly Black from AudioQuest.

## Who is Perry?

This is <a href="https://en.wikipedia.org/wiki/Perry_Kivolowitz">me</a>.

You are already <a href="http://variety.com/1997/scene/vpage/acad-sci-tech-nods-1117433914/">familiar with my work</a>.

I contribute to <a href="https://en.wikipedia.org/wiki/SilhouetteFX">this</a>.

I used to teach CS <a href="http://www.cs.wisc.edu/">here</a> but now I teach CS <a href="https://www.carthage.edu/">here</a>.

## Why did I start pas when \*.\* is available?

Because. 'Murica.

pas is not <a href="https://www.hackerposse.com/~rozzin/journal/whole-home-pulseaudio.html">this</a> which sends audio as digital data via RTP connections.

pas is not <a href="https://www.hackerposse.com/~rozzin/journal/whole-home-pulseaudio.html">this</a> which, well, I have no idea what this does.

These and others send digital data to remote digital devices. pas sits next to a rack of amplifiers.

## Is pas MPD-client compatible?

I hope this is added someday. The pas API is likely a superset of what MPD is capable of, so it seems reasonable this may come to pass in pas.

## What additional information is available about pas?

The <a href="https://github.com/pkivolowitz/pas/wiki">wiki</a> is maintained.

## Is there a style guide?

Yes. It is in the wiki.

## Are other contributors welcomed?

*Yes*.

## How do computer programmers kill zombies?

Read <a href="https://www.amazon.com/Get-Off-My-Zombie-Novel-ebook/dp/B00DQ26J8G/">Get off my L@wn</a> and find out. And don't steal it like everybody else, please? It's just $2.99 for crap's sake.

## Is pas shitty code?

I take pride in my work. I strive not to write shitty code. If you believe we have written shitty code, please let us know in a non-shitty way.

## Does pas contain much *cool* code?

Yes, there is some cool code in pas. 

The web server is being written in go and it and the pas server use Google Protocol Buffers (version 3).

The threading models are nice.

## When will pas be done?

¯\_(ツ)_/¯.

## What's the best computer ever made?

The Amiga.

&lt;mic drop>
