#pragma once

/*  This file is part of pas.

    pas is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    pas is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with pas.  If not, see <http://www.gnu.org/licenses/>.
*/

/*  pas is Copyright 2017 by Perry Kivolowitz
*/

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <aio.h>
#include <cstring>
#include <assert.h>
#include <thread>
#include <mutex>
#include <string>
#include <queue>
#include "audio_device.hpp"
#include "db_component.hpp"
#include <semaphore.h>

/*	An audio component sits in someone elses thread. It manages the hardware
	for a specific DAC and the player thread that corresponds to it. Audio
	commands can be set to the player thread such as play, pause and stop.
	If the audio player thread was idle, it is sem_waiting. If the player is
	not idle, it will poll the command queue without blocking. Of course
	if it finds a command waiting, it must sem_wait to decrement the
	semaphore to balance the post made when the command was added.
*/

enum AUDIO_COMMANDS
{
	PLAY 	= 'P',
	STOP 	= 'S',
	PAUSE 	= 'Z',
	RESUME 	= 'R',
	NEXT	= 'N',
	QUIT 	= 'Q',
	NONE 	= 0
};

struct AudioCommand
{
	AudioCommand()
	{
		cmd = AUDIO_COMMANDS::NONE;
		filler = 0;
	}

	AudioCommand(const AudioCommand & other)
	{
		cmd = other.cmd;
		filler = other.filler;
	}

	unsigned char cmd;
	unsigned char filler;
};

/*	A PlayStruct() - eat it Paul Montgomery - is the thing that is
	passed to the Player thread. 

	It contains the full path of the track to be played. The work
	of deriving the full path (they are all relative now) is done
	by the caller - at this time only:

		AudioComponent::Play(unsigned int id)

	The artist and title are here so that the DAC can report
	precisely what and who it's playing right now. This information
	is passed back to clients in the DACINFO command.
*/
struct PlayStruct
{
	std::string path;
	std::string artist;
	std::string title;
};

class AudioComponent
{
public:

	AudioComponent();
	~AudioComponent();
	bool Initialize(AudioDevice & ad);
	void AddCommand(const AudioCommand & c);
	void Play(const PlayStruct & ps);
	void Play(unsigned int id);
	bool IsIdle() { return idle; }
	bool QuiteDead() { return quite_dead; }
	std::string HumanName() { return ad.device_name; }
	std::string TimeCode();
	std::string What() { return title; }
	std::string Who() { return artist; }
	void ClearQueue();
	void AppendQueue(AudioComponent * other);

private:
	std::string title;
	std::string artist;

	bool GetCommand(AudioCommand & ac, bool was_idle);
	AudioDevice ad;

	// play queue
	std::queue<PlayStruct> play_queue;
	std::mutex m_play_queue;

	// used when the audio thread is idle.
	bool idle;
	bool quite_dead;
	std::queue<AudioCommand> commands;
	sem_t sem;
	std::mutex m;

	std::thread * t;
	off_t read_offset;
	pa_simple * pas;


    // The div / mult by 6 is essential. Furthermore, a very large size will
	// actually cause problems with ffmpeg keeping up. This value of 24K
	// works pretty well. Let's try doubling it. This is 49152 bytes. Linux
	// pipes hold 64K. If we exceed this, we will sometimes wait for the pipe
	// to become non-empty. 49152 bytes is 0.1858 seconds of 24 bit stereo
	// at 44.1 Khz.

	// Heading back down to 24K. Hearing hiccup at beginning of song.
    int BUFFER_SIZE = (1 << 12) * 6; 

	const int SAMPLE_RATE = 44100;

	static void PlayerThread(AudioComponent * me);
	static void LaunchAIO(aiocb & cb, int fd, AudioComponent * me, unsigned char * b);

	inline int BufferNext(int & bi)
	{
		return bi;
	}

	inline int BufferPrev(int & bi)
	{
		return 1 - bi;
	}

	inline void BufferToggle(int & bi)
	{
		bi = 1 - bi;
	}

};

