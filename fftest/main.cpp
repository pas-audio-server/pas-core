// pacmd list-sinks will find full alsa names.
// it is given directly under the * index: n line with heading "name:"
// alsa force-reload
// good resource https://wiki.archlinux.org/index.php/PulseAudio/Examples
// 

// ffprobe loglevel quiet -show_entries stream_tags:format_tags ../../b.flac
/*
Bus 003 Device 004: ID 2972:0006		this is the FIIO
Bus 003 Device 003: ID 2912:120b		this is the D3
*/
#include <iostream>
#include <string>
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

using namespace std;

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

bool Play(string cmdline, unsigned char * buffers[], int buffer_size, const char * device)
{

	bool rv = true;
	int error;
	FILE * p = nullptr;
	const int SAMPLE_RATE = 44100;

	if ((p = popen(cmdline.c_str(), "r")) == nullptr)
	{
		cerr << "pipe failed to open" << endl;
		rv = false;
	}
	else
	{
		size_t bytes_read;
		int pulse_error;
		pa_sample_spec ss;
		pa_channel_map cm;
		pa_simple * pas = nullptr;

		pa_channel_map_init_stereo(&cm);

		ss.format = PA_SAMPLE_S24LE;
		ss.rate = SAMPLE_RATE;
		ss.channels = 2;

		if ((pas = pa_simple_new(NULL, "pas_out", PA_STREAM_PLAYBACK, "alsa_output.usb-Audioengine_Audioengine_D3_Audioengine-00.analog-stereo", "playback", &ss, &cm, NULL, &pulse_error)) == NULL)
		{
			cerr << "pa_simple_new failed." << endl;
			cerr << pa_strerror(pulse_error) << endl;
			rv = false;
		}
		else
		{
			bool stop_flag = false;
			int buffer_index = 0;
			off_t read_offset = 0;
			int fd = fileno(p);
			aiocb cb;
			
			//cout << "block: 0 0" << endl;
			bytes_read = fread(buffers[0], 1, buffer_size, p);
			read_offset += bytes_read;

			// There needs to be one I/O already in flight by 
			// the time the loop starts. I found a gap happening
			// the first time through the loop.

			memset(&cb, 0, sizeof(aiocb));
			cb.aio_fildes = fd;
			cb.aio_offset = read_offset;
			cb.aio_buf = (void *) buffers[1];
			cb.aio_nbytes = buffer_size;
			cb.aio_reqprio = 0;
			cb.aio_sigevent.sigev_notify = SIGEV_NONE;
			cb.aio_lio_opcode = LIO_NOP;
			//cout << "async: 1 " << read_offset << " * " << endl;
			error = aio_read(&cb);

			bool first_loop = true;

			while (bytes_read > 0 && stop_flag == false)
			{
				if (!first_loop)
				{
					while ((error = aio_error(&cb)) == EINPROGRESS) 
					{
						usleep(100);
					}

					if (error > 0)
					{
						perror("async i/o returned error");
						rv = false;
						break;
					}
					assert(error == 0);
					int t = aio_return(&cb);
					assert(t >= 0);
					if (t == 0)
						stop_flag = true;
					read_offset += t;
					bytes_read = t;
				}

				if (bytes_read <= 0)
					break;

				BufferToggle(buffer_index);

				if (!first_loop)
				{
					memset(&cb, 0, sizeof(aiocb));
					cb.aio_fildes = fd;
					cb.aio_offset = read_offset;
					cb.aio_buf = (void *) buffers[BufferNext(buffer_index)];
					cb.aio_nbytes = buffer_size;
					cb.aio_reqprio = 0;
					cb.aio_sigevent.sigev_notify = SIGEV_NONE;
					cb.aio_lio_opcode = LIO_NOP;
					//cout << "async: " << BufferNext(buffer_index) << " " << read_offset << endl;
					error = aio_read(&cb);
	
					assert(error >= 0);
					assert(bytes_read > 0);
				}

				first_loop = false;

				// Launch blocking write to pulse
				pulse_error = 0;
				bytes_read = bytes_read / 6 * 6;
				cout << "rendr: " << BufferPrev(buffer_index) << " " << bytes_read << endl;
				if (pa_simple_write(pas, (const void *) buffers[BufferPrev(buffer_index)], bytes_read, &pulse_error) < 0)
				{
					cerr << "lost my pulse: " << pa_strerror(pulse_error) << endl;
					break;
				}
				//cout << endl;
			}
		}
		if (pas != nullptr)
		{
			pa_simple_flush(pas, &pulse_error);
			pa_simple_free(pas);
		}
	}

	if (p != nullptr)
		pclose(p);

	return rv;
}

int main(int argc, char * argv[])
{
	int rv = 0;

	unsigned char * buffer_1 = nullptr;
	unsigned char * buffer_2 = nullptr;

	// The div / mult by 6 is essential.
	int BUFFER_SIZE = (1 << 12) * 6; //((1 << 20) * (1 << 0)) / 6 * 6;
	string cmdline;

	if (argc < 2)
	{
		cerr << "usage: <audiofile>" << endl;
		exit(1);
	}

	buffer_1 = (unsigned char *) malloc(BUFFER_SIZE);
	buffer_2 = (unsigned char *) malloc(BUFFER_SIZE);

	if (buffer_1 == nullptr || buffer_2 == nullptr)
	{
		cerr << "buffer allocation failed" << endl;
		exit(1);
	}

	unsigned char * buffers[] { buffer_1, buffer_2 };

	for (int i = 1; i < argc; i++)
	{
		cmdline = string("ffmpeg -loglevel quiet -i ") + string(argv[i]) + string(" -f s24le -ac 2 -");
		cout << argv[i] << endl;
		Play(cmdline, buffers, BUFFER_SIZE, "alsa_output.usb-Audioengine_Audioengine_D3_Audioengine-00.analog-stereo");
	}

	if (buffer_1 != nullptr)
		free(buffer_1);

	if (buffer_2 != nullptr)
		free(buffer_2);

	return rv;
}
