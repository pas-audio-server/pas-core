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
#include <signal.h>
#include "audio_component.hpp"
#include "utility.hpp"

using namespace std;

AudioComponent::AudioComponent()
{
	pas = nullptr;
	t = nullptr;
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

	if (t != nullptr)
	{
		pthread_kill((pthread_t) t->native_handle(), SIGKILL);
		delete t;
	}

	sem_destroy(&sem);
}
                                                        
inline bool GoodCommand(unsigned char c)
{
	return (c == PLAY || c == STOP || c == PAUSE || c == RESUME || c == QUIT);
}

void AudioComponent::PlayerThread(AudioComponent * me)
{
	AudioCommand ac;

	pa_sample_spec ss;
	pa_channel_map cm;
	int pulse_error;

	pa_channel_map_init_stereo(&cm);

	ss.format = PA_SAMPLE_S24LE;
	ss.rate = me->SAMPLE_RATE;
	ss.channels = 2;

	cerr << LOG(me->ad.device_spec) << endl;
	if ((me->pas = pa_simple_new(NULL, "pas_out", PA_STREAM_PLAYBACK, me->ad.device_spec.c_str(), "playback", &ss, &cm, NULL, &pulse_error)) == NULL)
	{
		cerr << "pa_simple_new failed." << endl;
		cerr << pa_strerror(pulse_error) << endl;
		// MUST CHANGE THIS
		me->t = nullptr;
		return;
	}

	unsigned char * buffer_1 = nullptr;
	unsigned char * buffer_2 = nullptr;
	buffer_1 = (unsigned char *) malloc(me->BUFFER_SIZE);
    buffer_2 = (unsigned char *) malloc(me->BUFFER_SIZE);   
	unsigned char * buffers[2];

	cerr << LOG(to_string(me->BUFFER_SIZE)) << endl;
                                                        
    if (buffer_1 == nullptr || buffer_2 == nullptr)     
    {                                                   
        cerr << "buffer allocation failed" << endl;     
        // MUST CHANGE THIS
		me->t = nullptr;
		return;                                        
    }                                                   
                                                        
    buffers[0] = buffer_1;
	buffers[1] = buffer_2;

	while (true)
	{
		cout << LOG("") << endl;
		// IDLE STATE
		sem_wait(&me->sem);
		// If we get here, there is a command waiting.
		cout << "Awake" << endl;
		if (!me->GetCommand(ac, true))
		{
			cout << "No command!" << endl;
			continue;
		}
		if (!GoodCommand(ac.cmd))
		{
			cout << "Bad command" << endl;
			continue;
		}
		if (ac.cmd == QUIT)
			break;
		if (ac.cmd != PLAY)
			continue;
		cerr << LOG(ac.argument) << endl;

		// OK - It's playtime!
		FILE * p = nullptr;
		size_t bytes_read;
		me->seconds = 0.0f;
		try
		{
			// assemble the command line
			string player_command = string("ffmpeg -loglevel quiet -i \"") + ac.argument + string("\" -f s24le -ac 2 -");
			// Launch the decoder
			if ((p = popen(player_command.c_str(), "r")) == nullptr)
				throw LOG("pipe failed to open");
			cerr << LOG(player_command) << endl;
			bool stop_flag = false;
			int buffer_index = 0;
			off_t read_offset = 0;
			int fd = fileno(p);
			int error, pulse_error;
			aiocb cb;
			
			//cout << "block: 0 0" << endl;
			bytes_read = fread(buffers[0], 1, me->BUFFER_SIZE, p);
			read_offset += bytes_read;
			cerr << LOG("") << endl;

			// There needs to be one I/O already in flight by 
			// the time the loop starts. I found a gap happening
			// the first time through the loop.

			memset(&cb, 0, sizeof(aiocb));
			cb.aio_fildes = fd;
			cb.aio_offset = read_offset;
			cb.aio_buf = (void *) buffers[1];
			cb.aio_nbytes = me->BUFFER_SIZE;
			cb.aio_reqprio = 0;
			cb.aio_sigevent.sigev_notify = SIGEV_NONE;
			cb.aio_lio_opcode = LIO_NOP;
			//cout << "async: 1 " << read_offset << " * " << endl;
			error = aio_read(&cb);
			if (error != 0)
				throw LOG("aio_read() failed: " + to_string(error));

			//cerr << LOG("bytes_read: " + to_string(bytes_read)) << endl;
			bool first_loop = true;

			while (bytes_read > 0 && stop_flag == false)
			{
				//cerr << LOG("") << endl;
				if (!first_loop)
				{
					while ((error = aio_error(&cb)) == EINPROGRESS) 
					{
						usleep(100);
					}

					if (error > 0)
					{
						throw LOG("async i/o returned error");
						break;
					}
					assert(error == 0);
					int t = aio_return(&cb);
					assert(t >= 0);
					if (t == 0)
						stop_flag = true;
					read_offset += t;
					bytes_read = t;
					//cerr << LOG("bytes_read: " + to_string(bytes_read)) << endl;
				}
				//cerr << LOG("") << endl;

				if (bytes_read <= 0)
					break;

				me->BufferToggle(buffer_index);

				if (!first_loop)
				{
					memset(&cb, 0, sizeof(aiocb));
					cb.aio_fildes = fd;
					cb.aio_offset = read_offset;
					cb.aio_buf = (void *) buffers[me->BufferNext(buffer_index)];
					cb.aio_nbytes = me->BUFFER_SIZE;
					cb.aio_reqprio = 0;
					cb.aio_sigevent.sigev_notify = SIGEV_NONE;
					cb.aio_lio_opcode = LIO_NOP;
					//cout << "async: " << BufferNext(buffer_index) << " " << read_offset << endl;
					error = aio_read(&cb);
					if (error != 0)
						throw LOG("aio_read() failed: " + to_string(error));
				}

				first_loop = false;

				// Launch blocking write to pulse
				pulse_error = 0;
				bytes_read = bytes_read / 6 * 6;
				//cout << "rendr: " << me->BufferPrev(buffer_index) << " " << bytes_read << endl;
				if (pa_simple_write(me->pas, (const void *) buffers[me->BufferPrev(buffer_index)], bytes_read, &pulse_error) < 0)
				{
					throw LOG("lost my pulse: " + string(pa_strerror(pulse_error)));
				}
			}
		}
		catch (string s)
		{
			if (s.size() > 0)
				cerr << s << endl;
		}
		if (p != nullptr)
			pclose(p);
	}
	cout << "Player thread exiting" << endl;
	me->t = nullptr;

	if (buffer_1 != nullptr)
		free(buffer_1);

	if (buffer_2 != nullptr)
		free(buffer_2);

}

/*	Initialize is happening at the time pas starts. This means all the DACS are ready to
	go and can be used by all clients. Here ONE SPECIFIC DAC is being readied. Initialize()
	is being called from the main thread. This class wraps all communication with the 
	thread that is actually managing the hardware.

	ONLY Initialize and the destructor can mess with the pas (PulseAudioSimple)!!!!
*/
bool AudioComponent::Initialize(AudioDevice & ad)
{
	cout << LOG(ad.device_name) << endl;

	bool rv = true;
	assert(this->ad.device_spec.size() == 0);
	assert(t == nullptr);

	this->ad = ad;

	// Launch the thread associated with this DAC.
	t = new thread(PlayerThread, this);
	return rv;
}

/*	Called by the audio thread only.
*/
bool AudioComponent::GetCommand(AudioCommand & ac, bool was_idle)
{
	bool rv = false;
	cerr << LOG("") << endl;
	if (m.try_lock())
	{
		//cerr << LOG("") << endl;
		if (commands.size() > 0)
		{
			// If the commands queue is non-empty, someone must have done an
			// AddCommand. This means they did a sem_post. We must do a sem_wait
			// here to balance the forces in the universe. The universe should
			// respond immediately.
			// The audio player main loop won't exit when idle, instead t will sem_wait
			// for something to do.
			ac = commands.front();
			commands.pop();
			//cerr << LOG("") << endl;
			if (!was_idle)
				sem_wait(&sem);
			//cerr << LOG("") << endl;
			rv = true;
		}
		m.unlock();
		//cerr << LOG("") << endl;
	}
	cerr << LOG("") << endl;
	return rv;
}

/*	Called by the connection manager only. The sem_post will at most wake
	an idle player. At least it will cause no harm.
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
		delete db;
		db = nullptr;
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
