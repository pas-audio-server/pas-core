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
#include <iomanip>
#include <string>
#include <vector>
#include <signal.h>
#include "audio_component.hpp"
#include "utility.hpp"
#include "logger.hpp"

using namespace std;

extern Logger _log_;

AudioComponent::AudioComponent()
{
	pas = nullptr;
	t = nullptr;
	// There are no commands waiting for the player so set initial count to 0.
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
	//
	// This is no longer a problem. The threads will now cleanly exit by sending them a QUIT
	// command.

	int pulse_error = 0;
	if (pas != nullptr)
	{
		pa_simple_flush(pas, &pulse_error);
		pa_simple_free(pas);
		pas = nullptr;
	}

	if (t != nullptr)
	{
		// This rarely happens anymore.
		pthread_kill((pthread_t) t->native_handle(), SIGKILL);
		delete t;
	}

	sem_destroy(&sem);
}

inline bool GoodCommand(unsigned char c)
{
	return (c == PLAY || c == STOP || c == PAUSE || c == RESUME || c == QUIT);
}

// This is used twice, formerly as copy / pasted code. Copy / pasted code
// is evil. Code in one place, test in one place, fix in one place.
// This is a good, old fashioned Unix asynchronous I/O block. It was magical
// to me when I first encountered it. 
// 
// Here it is a necessity as with it, the latency of decoding audio is
// buried in the unavoidable latecy of playing audio.
void AudioComponent::LaunchAIO(aiocb & cb, int fd, AudioComponent * me, unsigned char * b)
{
	assert(b != nullptr);

	int error;
	cb.aio_fildes = fd;
	cb.aio_offset = me->read_offset;
	cb.aio_buf = (void *) b;
	cb.aio_nbytes = me->BUFFER_SIZE;
	cb.aio_reqprio = 0;
	cb.aio_sigevent.sigev_notify = SIGEV_NONE;
	cb.aio_lio_opcode = LIO_NOP;
	//cout << "async: 1 " << read_offset << " * " << endl;
	error = aio_read(&cb);
	if (error != 0)
		throw LOG(_log_, "aio_read() failed: " + to_string(error));
}

static bool HasEnding (string const & fullString, string const & ending)
{
	bool rv = false;

	if (fullString.length() >= ending.length())
	{
		rv = (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
	}
	return rv;
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

	LOG(_log_, to_string(me->ad.index) + " " + me->ad.device_spec);
	if ((me->pas = pa_simple_new(NULL, "pas_out", PA_STREAM_PLAYBACK, me->ad.device_spec.c_str(), "playback", &ss, &cm, NULL, &pulse_error)) == NULL)
	{
		LOG(_log_, "pa_simple_new failed: " + to_string(me->ad.index) + " " + me->ad.device_spec);
		LOG(_log_, pa_strerror(pulse_error));
		return;
	}
	LOG(_log_, nullptr);
	// Audio is double buffered. The two entry "buffers" allows for each buffer selection.
	// Inline helpers BufferNext, BufferPrev, BufferToggle help ensure the buffers are
	// kept straight. This strategy is often not used by younger programmers on projects
	// involing, for example, grid like data structures.
	unsigned char * buffer_1 = nullptr;
	unsigned char * buffer_2 = nullptr;
	buffer_1 = (unsigned char *) malloc(me->BUFFER_SIZE);
	buffer_2 = (unsigned char *) malloc(me->BUFFER_SIZE);   
	unsigned char * buffers[2];

	//LOG(_log_, to_string(me->BUFFER_SIZE));

	if (buffer_1 == nullptr || buffer_2 == nullptr)     
	{
		LOG(_log_, "buffer allocation failed");     
		return;                                        
	}                                                   

	buffers[0] = buffer_1;
	buffers[1] = buffer_2;

	// terminate_flag will trip when the thread is told to exit.
	//
	// restarting will trip when the thread was put to sleep by a PAUSE command.
	// and awakened specifically by a PLAY. The flag causes the initial sem_wait
	// to be skipped because the command that woke the thread up has already done
	// so.
	bool terminate_flag = false;
	bool restarting = false;

	while (!terminate_flag)
	{
		LOG(_log_, nullptr);
		if (!restarting)
		{
			// IDLE STATE
			me->idle = true;
			me->read_offset = 0;
			me->title = me->artist = string("");
			//LOG(_log_, nullptr);
			me->m_play_queue.lock();
			if (me->play_queue.empty())
			{
				me->m_play_queue.unlock();
				//LOG(_log_, nullptr);

				sem_wait(&me->sem);

				// If we get here, there is a command waiting.
				if (!me->GetCommand(ac, true))
				{
					LOG(_log_, "No command!");
					continue;
				}

				//LOG(_log_, string(1, ac.cmd));
				if (!GoodCommand(ac.cmd))
				{
					LOG(_log_, "Bad command");
					continue;
				}
				if (ac.cmd == QUIT)
				{
					LOG(_log_, "QUIT");
					terminate_flag = true;
					break;
				}
				if (ac.cmd != PLAY)
					continue;
			}
			me->m_play_queue.unlock();
		}
		restarting = false;

		// OK - It's playtime!
		FILE * p = nullptr;
		size_t bytes_read;
		try
		{
			// If a stop command is received, this flag will be set to true
			// then the main while loop is broken. terminate_flag on the other
			// hand means the thread should terminate.
			bool stop_flag = false;

			// Assemble the command line. The forcing of 44100 ensures that trackes sampled
			// at other rates (22500, for example) will be resampled. This has a bad side effect
			// documented a few lines down... the skipping of three buffers.
			//
			// NOTE: This can be avoided if the database has knowledge of the sample rate.
			//
			//LOG(_log_, nullptr);
			me->m_play_queue.lock();
			if (me->play_queue.empty())
			{
				me->m_play_queue.unlock();
				LOG(_log_, "error in queue");
				continue;
			}
			PlayStruct ps = me->play_queue.front();
			me->artist = ps.artist;
			me->title = ps.title;
			me->play_queue.pop();
			me->m_play_queue.unlock();
			//LOG(_log_, nullptr);
			string player_command = string("ffmpeg -loglevel quiet -i \"") + ps.path + string("\" -f s24le -ar 44100 -ac 2 -");
			bool is_mp3 = HasEnding(ps.path, "mp3");
			if ((p = popen(player_command.c_str(), "r")) == nullptr)
				throw LOG(_log_, "pipe failed to open");
			LOG(_log_, player_command);
			int buffer_index = 0;
			int fd = fileno(p);
			int error, pulse_error;
			aiocb cb;
			me->idle = false;

			// This unfortunate hack is needed to avoid a nasty glitch in ffmpeg output
			// if it has to resample audio. This will happen on the Moody Blues "On the
			// Threshold of a Dream" tracks which are sampled at 22500!!!
			// With 24K buffers, this is a loss of 0.84 seconds.
			bytes_read = fread(buffers[0], 1, me->BUFFER_SIZE, p);
			me->read_offset += bytes_read;
			if (is_mp3)
			{	
				// It turns out flac will die if these are done. So,
				// there are competing fixes. Without these, upsampled mp3
				// will pssst. With these, flac dies. So, up above I am
				// testing for file names ending in mp3. If a match,
				// add the skips.
				//
				// Note: watch for psssst in the future as there may
				// be other low resolution file formats out there.
				//
				bytes_read = fread(buffers[0], 1, me->BUFFER_SIZE, p);
				me->read_offset += bytes_read;
				bytes_read = fread(buffers[0], 1, me->BUFFER_SIZE, p);
				me->read_offset += bytes_read;
			}
			//LOG(_log_, nullptr);

			// There needs to be one I/O already in flight by 
			// the time the loop starts. I found a gap happening
			// the first time through the loop.

			memset(&cb, 0, sizeof(aiocb));
			me->LaunchAIO(cb, fd, me, buffers[1]);
			//LOG(_log_, "bytes_read: " + to_string(bytes_read));
			bool first_loop = true;

			// Ensure that the decoder has time to fill the pipe.
			//usleep(10000);
			pthread_yield();

			// The preceding has been foreplay, priming the pump for this
			// main loop of audio play. We'll always launch the NEXT buffer's
			// asynchonous I/O before writing the PREVIOUS buffer to pulse
			// in a blocking manner.
			while (bytes_read > 0 && !terminate_flag && !stop_flag)
			{
				//LOG(_log_, nullptr);
				if (!first_loop)
				{
					while ((error = aio_error(&cb)) == EINPROGRESS) 
					{
						usleep(1000);
					}

					if (error > 0)
						throw LOG(_log_, "async i/o returned error");

					assert(error == 0);
					int t = aio_return(&cb);
					assert(t >= 0);
					if (t == 0)
					{
						LOG(_log_, nullptr);
						break;
					}

					me->read_offset += t;
					bytes_read = t;
					//LOG(_log_, "bytes_read: " + to_string(bytes_read));
				}
				//LOG(_log_, nullptr);

				if (bytes_read <= 0)
					break;

				me->BufferToggle(buffer_index);

				if (!first_loop)
				{
					memset(&cb, 0, sizeof(aiocb));
					me->LaunchAIO(cb, fd, me, buffers[me->BufferNext(buffer_index)]);
				}

				first_loop = false;

				//LOG(_log_, nullptr);
				if (me->GetCommand(ac, false))
				{
retry_command:		if (GoodCommand(ac.cmd))
					{
						//LOG(_log_, nullptr);
						if (ac.cmd == QUIT)
						{
							// This will break the outermost loop causing the thread to terminate.
							terminate_flag = true;
							break;
						}
						if (ac.cmd == STOP)
						{
							LOG(_log_, "STOP");
							// This will break the playing loop. The thread will go to sleep again.
							stop_flag = true;
							break;
						}
						if (ac.cmd == PLAY)
						{
							// This will break the playing loop but the thread will not go to sleep.
							// Instead, it starts playing a new track. Recall, when the play loop
							// is broken, the pipe to ffmpeg is closed. This will cause ffmpeg
							// to exit (writing on closed pipe is fatal).
							LOG(_log_, nullptr);
							restarting = true;
							break;
						}
						/*	PAUSE:
							If you get a pause here, this thread should go to sleep. Here. It will be woken by
							another command. This new command should be retried here. But how to do this? A
							new local loop? THE BEST way of handling this is a goto. Plain and simple. Not
							using one makes the code much more complex.
						 */
						if (ac.cmd == PAUSE)
						{
							LOG(_log_, nullptr);
							sem_wait(&me->sem);
							me->GetCommand(ac, true);
							LOG(_log_, "PAUSE");
							goto retry_command;
						}
						if (ac.cmd == RESUME)
						{
							LOG(_log_, "RESUME");
						}
						//LOG(_log_, nullptr);
					}
				}

				// Launch blocking write to pulse. The buffer size has already accounted for
				// the requirement that it be a multiple of six. Enforcing this here will avoid
				// the case of the pipe filling with a non-compliant number of bytes. This
				// means there is the POTENTIAL for up to one sample to be lost.
				//
				// WAIT - IF THIS SHOULD HAPPEN, IT IS POSSIBLE I MIGHT SOMEDAY HIT A CASE
				// WHERE THE SAMPLESGET OUT OF FRAMING. WITH A SMALL BUFFER, THIS IS LESS
				// LIKELY TO HAPPEN.
				pulse_error = 0;
				bytes_read = bytes_read / 6 * 6;
				//cout << "rendr: " << me->BufferPrev(buffer_index) << " " << bytes_read << endl;
				if (pa_simple_write(me->pas, (const void *) buffers[me->BufferPrev(buffer_index)], bytes_read, &pulse_error) < 0)
				{
					throw LOG(_log_, "lost my pulse: " + string(pa_strerror(pulse_error)));
				}
			}
		}
		catch (LoggedException e)
		{
			LOG(_log_, e.Msg());
		}
		LOG(_log_, nullptr);
		if (p != nullptr)
			pclose(p);
	}
	LOG(_log_, "Player thread exiting");

	pa_simple_flush(me->pas, &pulse_error);
	pa_simple_free(me->pas);
	me->pas = nullptr;

	if (buffer_1 != nullptr)
		free(buffer_1);

	if (buffer_2 != nullptr)
		free(buffer_2);

	LOG(_log_, nullptr);
}

bool AudioComponent::Initialize(AudioDevice & ad)
{
	//LOG(_log_, ad.device_name);

	bool rv = true;
	assert(this->ad.device_spec.size() == 0);
	assert(t == nullptr);

	this->ad = ad;

	// Launch the thread associated with this DAC.
	t = new thread(PlayerThread, this);
	if (false)//t->joinable())
	{
		LOG(_log_, "Initialize() failed");
		rv = false;
	}
	LOG(_log_, nullptr);
	return rv;
}

/*	Called by the audio thread only.
 */
bool AudioComponent::GetCommand(AudioCommand & ac, bool was_idle)
{
	bool rv = false;
	//LOG(_log_, nullptr);
	if (m.try_lock())
	{
		//LOG(_log_, nullptr);
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
			//LOG(_log_, nullptr);
			if (!was_idle)
				sem_wait(&sem);
			//LOG(_log_, nullptr);
			rv = true;
		}
		m.unlock();
		//LOG(_log_, nullptr);
	}
	//LOG(_log_, nullptr);
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

void AudioComponent::Clear()
{
	LOG(_log_, nullptr);
	m_play_queue.lock();
	std::queue<PlayStruct>().swap(play_queue);
	m_play_queue.unlock();
}

/*	Called by the connection manager only.
 */
void AudioComponent::Play(const PlayStruct & ps)
{
	AudioCommand ac;
	LOG(_log_, nullptr);
	// I thought I could use the queue to tell me if the player thread was idle. But I cannot
	// since the queue is drained immiadtely. I need another flag.
	m_play_queue.lock();
	bool was_idle = IsIdle();
	play_queue.push(ps);
	if (was_idle)
	{
		//LOG(_log_, "sending PLAY");
		ac.cmd = PLAY;
		AddCommand(ac);
	}
	m_play_queue.unlock();
}

void AudioComponent::Play(unsigned int id)
{
	DB * db = new DB();

	try
	{
		if (db == nullptr)
			throw LOG(_log_, "allocating db failed");

		if (!db->Initialize())
			throw LOG(_log_, "db->Initialize() failed");

		PlayStruct ps;
		ps.path = db->PathFromID(id, &ps.title, &ps.artist);
		delete db;
		db = nullptr;
		if (ps.path.size() > 0)
			Play(ps);
	}
	catch (LoggedException e)
	{
	}

	if (db != nullptr)
		delete db;
}


string AudioComponent::TimeCode()
{
	stringstream ss;
	//cout << read_offset << endl;
	int ro = read_offset / (6 * SAMPLE_RATE);
	int hours, minutes, seconds;
	seconds = (int) ro % 60;
	minutes = ((int) ro / 60) % 60;
	hours = ((int) ro / 3600) % 100;
	ss << setw(2) << setfill('0') << hours << ":" << setw(2) << minutes << ":" << setw(2) << seconds;
	return ss.str();
}


