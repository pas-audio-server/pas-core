#include <iostream>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pulse/simple.h>
#include <pulse/error.h>

using namespace std;

int main(int argc, char * argv[])
{
	int rv = 0;
	int pulse_error;

	unsigned char * buffer = nullptr;
	size_t bytes_read;
	FILE * p = nullptr;
	// The div / mult by 6 is essential.
	int BUFFER_SIZE = ((1 << 20) * (1 << 3)) / 6 * 6;
	const int SAMPLE_RATE = 44100;
	string cmdline;

	if (argc < 2)
	{
		cerr << "usage: <audiofile>" << endl;
		exit(1);
	}

	buffer = (unsigned char *) malloc(BUFFER_SIZE);
	if (buffer == nullptr)
	{
		cerr << "buffer allocation failed" << endl;
		exit(1);
	}

	cmdline = string("ffmpeg -loglevel quiet -i ") + string(argv[1]) + string(" -f s24le -ac 2 -");
	cout << cmdline << endl;

	if ((p = popen(cmdline.c_str(), "r")) == nullptr)
	{
		cerr << "pipe failed to open" << endl;
		exit(1);
	}

	pa_sample_spec ss;
	pa_simple * pas = nullptr;

	ss.format = PA_SAMPLE_S24LE;
	ss.rate = SAMPLE_RATE;
	ss.channels = 2;

	cout << pa_frame_size(&ss) << endl;

	pa_channel_map cm;
	pa_channel_map_init_stereo(&cm);

	if ((pas = pa_simple_new(NULL, "PulseOut", PA_STREAM_PLAYBACK, "alsa_output.usb-Audioengine_Audioengine_D3_Audioengine-00.analog-stereo", "playback", &ss, &cm, NULL, &pulse_error)) == NULL)
	{
		cout << "pa_simple_new failed." << endl;
		cout << pa_strerror(pulse_error) << endl;
		exit(1);
	}

//	cout << ba.fragsize << endl << ba.maxlength << endl << ba.minreq << endl << ba.prebuf << endl << ba.tlength << endl;
	bytes_read = fread(buffer, 1, BUFFER_SIZE, p);
	while (bytes_read > 0)
	{
		pulse_error = 0;

		if (pa_simple_write(pas, (const void *) buffer, bytes_read, &pulse_error) < 0)
		{
			cout << "lost my pulse: " << pa_strerror(pulse_error) << endl;
			break;
		}	
		bytes_read = fread(buffer, 1, BUFFER_SIZE, p);
	}

bottom:

	if (pas != nullptr)
	{
		pa_simple_flush(pas, &pulse_error);
		pa_simple_free(pas);
	}

	if (p != nullptr)
		pclose(p);

	if (buffer != nullptr)
		free(buffer);

	return rv;
}
