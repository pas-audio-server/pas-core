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
#include <mutex>
#include <string>
#include <queue>
#include "audio_device.hpp"

struct AudioCommand
{
	AudioCommand()
	{
		cmd = 0;
		filler = 0;
	}

	AudioCommand(const AudioCommand & other)
	{
		argument = other.argument;
		cmd = other.cmd;
		filler = other.filler;
	}

	std::string argument;
	unsigned char cmd;
	unsigned char filler;
};

class AudioComponent
{
public:

	AudioComponent();
	~AudioComponent();
	bool Initialize(AudioDevice & ad);
	inline float GetSeconds() { return seconds; }
	void AddCommand(const AudioCommand & c);
	void Play(const std::string & path);
	std::string HumanName() { return ad.device_name; }

private:

	bool GetCommand(AudioCommand & ac);
	AudioDevice ad;
	std::mutex m;
	std::queue<AudioCommand> commands;
	
	float seconds;
	
	pa_simple * pas;

    unsigned char * buffer_1 = nullptr;
    unsigned char * buffer_2 = nullptr; 
	unsigned char * buffers[2];

    // The div / mult by 6 is essential. Furthermore, a very large size will
	// actually cause problems with ffmpeg keeping up. This value of 24K
	// works pretty well.
    int BUFFER_SIZE = (1 << 12) * 6; 

	const int SAMPLE_RATE = 44100;

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

void AudioThread(AudioComponent * ac);
