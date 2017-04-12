#include <iostream>
#include <iomanip>

using namespace std;

void Decode(unsigned char * buffer, int * samples)
{
	short t;
	unsigned char * p = buffer;

	t = p[3] << 8 | p[0];
	samples[0] = t;

	t = p[4] << 8 | p[5];
	samples[1] = t;

	p += 6;

	t = p[3] << 8 | p[0];
	samples[2] = t;

	t = p[4] << 8 | p[5];
	samples[3] = t;
}

void Encode(int * samples, unsigned char * buffer)
{
}

/*
6400 00ff ff87 6400 00ff ff87 6600 00ff in s24le is
0000 ff64 0000 ff87 0000 ff64 0000 ff87 in s32le
*/
int main()
{
	unsigned char buffer[12] = 
	{ 0x64, 0x00, 0x00, 0xff, 0xff, 0x87, 0x64, 0x00, 0x00, 0xff, 0xff, 0x87};

	unsigned char output[12] = { 0 };

	int samples[4];

	Decode(buffer, samples);

	cout << hex << samples[0] << endl;
	cout << hex << samples[1] << endl;
	cout << hex << samples[2] << endl;
	cout << hex << samples[3] << endl;

	Encode(samples, output);

	for (int i = 0; i < 12; i++)
		cout << "0x" << hex << output[i] << " ";
	cout << endl;

	return 0;
}
