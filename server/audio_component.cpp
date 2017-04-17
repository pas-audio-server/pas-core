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
#include "../protos/cpp/commands.pb.h"

using namespace std;
using namespace pas;

extern Logger _log_;

/*	Note: pas means the puls_eaudio_simple (pasimple) pointer.

	quite_dead is used to let the party that spawned the Player
	thread know that the thread is quite definitely dead. It can
	be safely joined, therefore. This is used during startup only.

	t is the C++ thread for the actual playing (i.e. the
	Player thread).
*/
AudioComponent::AudioComponent()
{
	pas = nullptr;
	quite_dead = true;
	t = nullptr;
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
		LOG2(_log_, nullptr, CONVERSATIONAL);
		pthread_kill((pthread_t) t->native_handle(), SIGKILL);
		// This join should return immediately because of the 
		// SIGKILL above, or the thread has already exited due
		// to QUIT. Without this join, an error message will
		// be printed by the C++ threading system.
		t->join();
		delete t;
		LOG2(_log_, nullptr, CONVERSATIONAL);
	}
	LOG2(_log_, nullptr, REDICULOUS);
	sem_destroy(&sem);
}

inline bool GoodCommand(unsigned char c)
{
	return (c == PLAY || c == STOP || c == PAUSE || c == RESUME || c == QUIT || c == NEXT);
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
	error = aio_read(&cb);

	// NOTE: Not sure I want to keep this as a FATAL.
	if (error != 0)
		throw LOG2(_log_, "aio_read() failed: " + to_string(error), FATAL);
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

	LOG2(_log_, to_string(me->ad.index) + " " + me->ad.device_spec, CONVERSATIONAL);
	if ((me->pas = pa_simple_new(NULL, "pas_out", PA_STREAM_PLAYBACK, me->ad.device_spec.c_str(), "playback", &ss, &cm, NULL, &pulse_error)) == NULL)
	{
		LOG2(_log_, "pa_simple_new failed: " + to_string(me->ad.index) + " " + me->ad.device_spec, MINIMAL);
		LOG2(_log_, pa_strerror(pulse_error), MINIMAL);
		return;
	}

	LOG2(_log_, nullptr, REDICULOUS);
	// Audio is double buffered. The two entry "buffers" allows for each buffer selection.
	// Inline helpers BufferNext, BufferPrev, BufferToggle help ensure the buffers are
	// kept straight. This strategy is often not used by younger programmers on projects
	// involing, for example, grid like data structures.
	unsigned char * buffer_1 = nullptr;
	unsigned char * buffer_2 = nullptr;
	buffer_1 = (unsigned char *) malloc(me->BUFFER_SIZE);
	buffer_2 = (unsigned char *) malloc(me->BUFFER_SIZE);   
	unsigned char * buffers[2];

	LOG2(_log_, "audio buffer size set to: " + to_string(me->BUFFER_SIZE), VERBOSE);

	if (buffer_1 == nullptr || buffer_2 == nullptr)     
	{
		LOG2(_log_, "buffer allocation failed", MINIMAL);
		return;                                        
	}                                                   

	buffers[0] = buffer_1;
	buffers[1] = buffer_2;

	// terminate_flag will trip when the thread is told to exit.
	//
	// restarting will trip when the thread was put to sleep by a PAUSE command.
	// and awakened specifically by a PLAY or NEXT. The flag causes the initial sem_wait
	// to be skipped because the command that woke the thread up has already done
	// so.
	bool terminate_flag = false;
	bool restarting = false;

	// If a stop command is received, this flag will be set to true
	// then the main while loop is broken. terminate_flag on the other
	// hand means the thread should terminate.
	bool stop_flag = false;

	me->quite_dead = false;
	while (!terminate_flag)
	{
		LOG2(_log_, nullptr, REDICULOUS);
		if (!restarting)
		{
			// IDLE STATE
			me->idle = true;
			me->read_offset = 0;
			me->title = me->artist = string("");
			LOG2(_log_, nullptr, REDICULOUS);
			me->m_play_queue.lock();

			// stop_flag begins as false and will be set to false before playing something. 
			// If a stop command comes it, stop_flag becomes true and we loop around to here.
			// Getting here with it still true means we should sem_wait even if the queue
			// is not empty.
			if (me->play_queue.empty() || stop_flag)
			{
				LOG2(_log_, "waiting", REDICULOUS);
				me->m_play_queue.unlock();
				sem_wait(&me->sem);
				LOG2(_log_, "awakened", REDICULOUS);

				if (!me->GetCommand(ac, true))
				{
					// If there is no command, unlock and continue.
					LOG2(_log_, "no command", CONVERSATIONAL);
					continue;
				}
				else if (ac.cmd == QUIT)
				{
					// If the command is quit, unlock and terminate.
					LOG2(_log_, "QUIT", CONVERSATIONAL);
					terminate_flag = true;
					break;
				}
				else if (ac.cmd != NEXT && ac.cmd != PLAY)
				{
					// If the command is anything but NEXT or PLAY, unlock and continue.
					LOG2(_log_, nullptr, REDICULOUS);
					continue;
				}
			}
			else
			{
				me->m_play_queue.unlock();
			}
		}
		LOG2(_log_, nullptr, REDICULOUS);
		// If we just received a QUIT, take this short circuit.
		if (terminate_flag)
			continue;

		restarting = false;

		FILE * p = nullptr;
		size_t bytes_read;
		try
		{
			// Assemble the command line. The forcing of 44100 ensures that trackes sampled
			// at other rates (22500, for example) will be resampled. This has a bad side effect
			// documented a few lines down... the skipping of three buffers.
			//
			// NOTE: This can be avoided if the database has knowledge of the sample rate.
			//
			LOG2(_log_, nullptr, REDICULOUS);
			me->m_play_queue.lock();
			if (me->play_queue.empty())
			{
				me->m_play_queue.unlock();
				LOG2(_log_, "error in queue", CONVERSATIONAL);
				continue;
			}
			PlayStruct ps = me->play_queue.front();
			me->read_offset = 0;
			me->artist = ps.artist;
			me->title = ps.title;
			me->play_queue.pop();
			me->m_play_queue.unlock();

			LOG2(_log_, nullptr, REDICULOUS);
			
			string player_command = string("ffmpeg -loglevel quiet -i \"") + ps.path + string("\" -f s24le -ar 44100 -ac 2 -");
			bool is_mp3 = HasEnding(ps.path, "mp3");
			if ((p = popen(player_command.c_str(), "r")) == nullptr)
				throw LOG2(_log_, "pipe failed to open", MINIMAL);

			LOG2(_log_, player_command, CONVERSATIONAL);
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

			LOG2(_log_, nullptr, REDICULOUS);

			// There needs to be one I/O already in flight by 
			// the time the loop starts. I found a gap happening
			// the first time through the loop.

			memset(&cb, 0, sizeof(aiocb));
			me->LaunchAIO(cb, fd, me, buffers[1]);
			LOG2(_log_, "bytes_read: " + to_string(bytes_read), REDICULOUS);
			bool first_loop = true;

			// Ensure that the decoder has time to fill the pipe.
			//usleep(10000);
			pthread_yield();

			// The preceding has been foreplay, priming the pump for this
			// main loop of audio play. We'll always launch the NEXT buffer's
			// asynchonous I/O before writing the PREVIOUS buffer to pulse
			// in a blocking manner.
			stop_flag = false;
			while (bytes_read > 0 && !terminate_flag && !stop_flag)
			{
				LOG2(_log_, nullptr, REDICULOUS);
				if (!first_loop)
				{
					while ((error = aio_error(&cb)) == EINPROGRESS) 
					{
						usleep(1000);
					}

					if (error > 0)
						throw LOG2(_log_, "async i/o returned error", MINIMAL);

					assert(error == 0);
					int t = aio_return(&cb);
					assert(t >= 0);
					if (t == 0)
					{
						LOG2(_log_, nullptr, REDICULOUS);
						break;
					}

					me->read_offset += t;
					bytes_read = t;
					LOG2(_log_, "bytes_read: " + to_string(bytes_read), REDICULOUS);
				}
				LOG2(_log_, nullptr, REDICULOUS);

				if (bytes_read <= 0)
					break;

				me->BufferToggle(buffer_index);

				if (!first_loop)
				{
					memset(&cb, 0, sizeof(aiocb));
					me->LaunchAIO(cb, fd, me, buffers[me->BufferNext(buffer_index)]);
				}

				first_loop = false;

				LOG2(_log_, nullptr, REDICULOUS);
				if (me->GetCommand(ac, false))
				{
retry_command:		if (GoodCommand(ac.cmd))
					{
						LOG2(_log_, nullptr, REDICULOUS);
						if (ac.cmd == QUIT)
						{
							// This will end the outermost loop causing the thread to terminate.
							LOG2(_log_, "QUIT", MINIMAL);
							terminate_flag = true;
							break;
						}
						if (ac.cmd == STOP)
						{
							LOG2(_log_, "STOP", MINIMAL);
							// This will break the playing loop. The thread will go to sleep again.
							stop_flag = true;
							break;
						}
						if (ac.cmd == NEXT)
						{
							// This will end the playing loop but the thread will not go to sleep.
							// Instead, it starts playing a new track. Recall, when the play loop
							// is broken, the pipe to ffmpeg is closed. This will cause ffmpeg
							// to exit (writing on closed pipe is fatal).
							LOG2(_log_, "NEXT", MINIMAL);
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
							LOG2(_log_, "PAUSE+", MINIMAL);
							sem_wait(&me->sem);
							me->GetCommand(ac, true);
							LOG2(_log_, "PAUSE-", MINIMAL);
							goto retry_command;
						}
						if (ac.cmd == RESUME)
						{
							LOG2(_log_, "RESUME", MINIMAL);
						}
						LOG2(_log_, nullptr, REDICULOUS);
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
				LOG2(_log_, "rendr: " + to_string(me->BufferPrev(buffer_index)) + " " + to_string(bytes_read), VERBOSE);
				if (pa_simple_write(me->pas, (const void *) buffers[me->BufferPrev(buffer_index)], bytes_read, &pulse_error) < 0)
				{
					throw LOG2(_log_, "lost my pulse: " + string(pa_strerror(pulse_error)), MINIMAL);
				}
			}
		}
		catch (LoggedException e)
		{
			if (e.Level() == FATAL)
				throw e;
		}
		LOG2(_log_, nullptr, REDICULOUS);
		if (p != nullptr)
			pclose(p);
	}
	me->quite_dead = true;
	LOG2(_log_, "Player thread exiting", MINIMAL);

	pa_simple_flush(me->pas, &pulse_error);
	pa_simple_free(me->pas);
	me->pas = nullptr;

	if (buffer_1 != nullptr)
		free(buffer_1);

	if (buffer_2 != nullptr)
		free(buffer_2);

	LOG2(_log_, nullptr, MINIMAL);
}

bool AudioComponent::Initialize(AudioDevice & ad)
{
	LOG2(_log_, ad.device_name, CONVERSATIONAL);

	bool rv = true;
	assert(this->ad.device_spec.size() == 0);
	assert(t == nullptr);

	this->ad = ad;

	// Launch the thread associated with this DAC.
	t = new thread(PlayerThread, this);
	sleep(1);
	if (quite_dead)	{
		LOG2(_log_, "Initialize() failed", MINIMAL);
		rv = false;
	}
	LOG2(_log_, nullptr, REDICULOUS);
	return rv;
}

/*	Called by the audio thread only.
 */
bool AudioComponent::GetCommand(AudioCommand & ac, bool was_idle)
{
	bool rv = false;
	LOG2(_log_, nullptr, REDICULOUS);
	if (m.try_lock())
	{
		LOG2(_log_, nullptr, REDICULOUS);
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
			LOG2(_log_, nullptr, VERBOSE);
			if (!was_idle)
				sem_wait(&sem);
			LOG2(_log_, nullptr, REDICULOUS);
			rv = true;
		}
		m.unlock();
		LOG2(_log_, nullptr, REDICULOUS);
	}
	LOG2(_log_, nullptr, REDICULOUS);
	return rv;
}

/*	Called by the connection manager only. The sem_post will at most wake
	an idle player. At least it will cause no harm.
 */

void AudioComponent::AddCommand(const AudioCommand & cmd)
{
	LOG2(_log_, nullptr, CONVERSATIONAL);
	m.lock();
	commands.push(cmd);
	sem_post(&sem);
	m.unlock();
	LOG2(_log_, nullptr, CONVERSATIONAL);
}

void AudioComponent::ClearQueue()
{
	LOG2(_log_, nullptr, CONVERSATIONAL);
	m_play_queue.lock();
	std::queue<PlayStruct>().swap(play_queue);
	m_play_queue.unlock();
	LOG2(_log_, nullptr, CONVERSATIONAL);
}

void AudioComponent::AppendQueue(AudioComponent * other)
{
	if (other != this)
	{
		mutex * m1;
		mutex * m2;
		// By always locking in the same order, we avoid deadlocks.
		// The AudioComponent with the higher address will always be
		// locked first.
		if (other > this)
		{
			m1 = &other->m_play_queue;
			m2 = &this->m_play_queue;
		}
		else
		{
			m1 = &this->m_play_queue;
			m2 = &other->m_play_queue;
		}
		LOG2(_log_, nullptr, CONVERSATIONAL);
		m1->lock();
		m2->lock();
		queue<PlayStruct> temp = other->play_queue;
		while (!temp.empty())
		{
			PlayStruct ps = temp.front();
			this->play_queue.push(ps);
			temp.pop();
		}
		m2->unlock();
		m1->unlock();
		LOG2(_log_, nullptr, CONVERSATIONAL);
	}
}

/*	Called by the connection manager only.
 */
void AudioComponent::Play(const PlayStruct & ps)
{
	AudioCommand ac;
	LOG2(_log_, nullptr, CONVERSATIONAL);
	// I thought I could use the queue to tell me if the player thread was idle. But I cannot
	// since the queue is drained immiadtely. I need another flag.
	m_play_queue.lock();
	bool was_idle = IsIdle();
	play_queue.push(ps);
	if (was_idle)
	{
		LOG2(_log_, "sending PLAY", CONVERSATIONAL);
		ac.cmd = PLAY;
		AddCommand(ac);
	}
	m_play_queue.unlock();
	LOG2(_log_, nullptr, CONVERSATIONAL);
}

/*	FATAL errors can come from this. I can no longer swallow LoggedExceptions
	without looking at them. I have added the exception level to LoggedException
	for this purpose. This must be done for all the places where something severe
	can happen.
*/
void AudioComponent::Play(unsigned int id)
{
	DB * db = new DB();

	try
	{
		if (db == nullptr)
			throw LOG2(_log_, "allocating db failed", LogLevel::FATAL);

		if (!db->Initialize())
			throw LOG2(_log_, "db->Initialize() failed", LogLevel::FATAL);

		PlayStruct ps;
		ps.path = db->PathFromID(id, &ps.title, &ps.artist);
		delete db;
		db = nullptr;
		if (ps.path.size() > 0)
			Play(ps);
	}
	catch (LoggedException e)
	{
		if (e.Level() == LogLevel::FATAL) {
			throw e;
		}
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
