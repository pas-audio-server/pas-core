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

#include <iostream>
#include <string>
#include <vector>
#include "audio_component.hpp"
#include "utility.hpp"

using namespace std;

AudioComponent::AudioComponent()
{
	pas = nullptr;
	// There are no commands waiting for the player.
	sem_init(&sem, 0, 0);
}

AudioComponent::~AudioComponent()
{
//
// QUESTION: This is ripping the pas out from under the player thread. What are
// the implicationsof this. Do I need:
//
// http://stackoverflow.com/questions/9094422/how-to-check-if-a-stdthread-is-still-running
//
// I could SIGNAL the thread or send it a command 
// in either case then join it.
//
// This would fail if the thread were hung.

	int pulse_error = 0;
	if (pas != nullptr)
	{
		pa_simple_flush(pas, &pulse_error);
		pa_simple_free(pas);
		pas = nullptr;
	}

	if (buffer_1 != nullptr)
		free(buffer_1);

	if (buffer_2 != nullptr)
		free(buffer_2);

	sem_destroy(&sem);
}
                                                        
/*	Initialize is happening at the time pas starts. This means all the DACS are ready to
	go and can be used by all clients. Here ONE SPECIFIC DAC is being readied. Initialize()
	is being called from the main thread. This class wraps all communication with the 
	thread that is actually managing the hardware.

	ONLY Initialize and the destructor can mess with the pas (PulseAudioSimple)!!!!
*/
bool AudioComponent::Initialize(AudioDevice & ad)
{
	bool rv = true;
	assert(this->ad.device_spec.size() == 0);
	
	this->ad = ad;

	pa_sample_spec ss;
	pa_channel_map cm;
	int pulse_error;

	pa_channel_map_init_stereo(&cm);

	ss.format = PA_SAMPLE_S24LE;
	ss.rate = SAMPLE_RATE;
	ss.channels = 2;

	if ((pas = pa_simple_new(NULL, "pas_out", PA_STREAM_PLAYBACK, this->ad.device_spec.c_str(), "playback", &ss, &cm, NULL, &pulse_error)) == NULL)
	{
		cerr << "pa_simple_new failed." << endl;
		cerr << pa_strerror(pulse_error) << endl;
		rv = false;
	}

	buffer_1 = (unsigned char *) malloc(BUFFER_SIZE);
    buffer_2 = (unsigned char *) malloc(BUFFER_SIZE);   
                                                        
    if (buffer_1 == nullptr || buffer_2 == nullptr)     
    {                                                   
        cerr << "buffer allocation failed" << endl;     
        exit(1);                                        
    }                                                   
                                                        
    buffers[0] = buffer_1;
	buffers[1] = buffer_2;

	return rv;
}

/*	Called by the audio thread only.
*/
bool AudioComponent::GetCommand(AudioCommand & ac, bool was_idle)
{
	bool rv = false;
	if (m.try_lock())
	{
		if (commands.size() > 0)
		{
			// If we were idle, then we have alread done the sem_wait.
			// If we were not idle, then the sem_wait should return immediately but
			// must be called for balance. Note, because the sem_wait should return
			// immediately, deadlock ought not to happen (even if we are holding the
			// m lock).
			// The audio player main loop won't exit when idle, instead t will sem_wait
			// for something to do.
			if (!was_idle)
				sem_wait(&sem);
			ac = commands.front();
			commands.pop();
			rv = true;
		}
		m.unlock();
	}
	return rv;
}

/*	Called by the connection manager only.
*/
void AudioComponent::AddCommand(const AudioCommand & cmd)
{
	m.lock();
	commands.push(cmd);
	sem_post(&sem);
	m.unlock();
}

/*	Called by the connection manager only.
*/
void AudioComponent::Play(const string & path)
{
	AudioCommand ac;
	ac.argument = path;
	ac.cmd = 'p';
	AddCommand(ac);
}

void AudioComponent::Play(unsigned int id)
{
	DB * db = new DB();
	
	try
	{
		if (db == nullptr)
			throw LOG("allocating db failed");

		if (!db->Initialize())
			throw LOG("db->Initialize() failed");

		string path = db->PathFromID(id);
		if (path.size() > 0)
			Play(path);
	}

	catch (string s)
	{
		if (s.size() > 0)
			cerr << s << endl;
	}

	if (db != nullptr)
		delete db;
}















// Make it easier to code in bed`
