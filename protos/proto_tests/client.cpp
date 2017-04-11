#include <iostream>
#include <fstream>
#include <iomanip>
#include <cctype>
#include <map>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <memory.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unordered_set>
#include <time.h>
#include <assert.h>
#include <json/json.h>
#include "../cpp/commands.pb.h"

using namespace std;
using namespace pas;

int InitializeNetworkConnection(int argc, char * argv[])
{
	int server_socket = -1;
	int port = 5077;
	char * ip = (char *) ("127.0.0.1");
	int opt;

	while ((opt = getopt(argc, argv, "h:p:")) != -1) 
	{
		switch (opt) 
		{
			case 'h':
				ip = optarg;
				break;

			case 'p':
				port = atoi(optarg);
				break;
		}
	}

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)
	{
		perror("Failed to open socket");
		throw string("");
	}

	hostent * server_hostent = gethostbyname(ip);
	if (server_hostent == nullptr)
	{
		close(server_socket);
		throw  "Failed gethostbyname()";
	}

	sockaddr_in server_sockaddr;
	memset(&server_sockaddr, 0, sizeof(sockaddr_in));
	server_sockaddr.sin_family = AF_INET;
	memmove(&server_sockaddr.sin_addr.s_addr, server_hostent->h_addr, server_hostent->h_length);
	server_sockaddr.sin_port = htons(port);

	if (connect(server_socket, (struct sockaddr*) &server_sockaddr, sizeof(sockaddr_in)) == -1)
	{
		perror("Connection failed");
		throw string("");
	}

	return server_socket;
}

int main(int argc, char * argv[])
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	int server_socket = -1;

	try
	{
		server_socket = InitializeNetworkConnection(argc, argv);
		if (server_socket < 0)
		{
			perror("bad socket");
			throw string("bad socket");
		}
		
		SelectQuery sq;
		sq.set_type(SELECT_QUERY);
		sq.set_column("artist");
		sq.set_pattern("%joyous%");

		PlayTrackCommand ptc;
		ptc.set_type(Type::PLAY_TRACK_DEVICE);
		ptc.set_device_id(0);
		ptc.set_track_id(1000);
		cout << ptc.DebugString() << endl;

		string s;
		bool outcome = sq.SerializeToString(&s);
		if (!outcome)
			throw string("SerializeToString() failed");
		
		size_t length = s.size();
		cout << "size() of string s: " << length << endl;
		cout << "does this agree with: " << sq.ByteSize() << endl;

		size_t bytes_sent = send(server_socket, (const void *) &length, sizeof(length), 0);

		if (bytes_sent != sizeof(length))
		{
			perror("sending length");
			throw string("sent returns bad value for sending length: ") + to_string(sizeof(length)) + " value returned is: " + to_string(bytes_sent);
		}

		bytes_sent = send(server_socket, (const void *) s.data(), length, 0);
		if (bytes_sent != length)
		{
			perror("sending message");
			throw string("sent returns bad value for sending message") + to_string(length) + " value returned is: " + to_string(bytes_sent);
		}

		cout << "Hit enter to exit:";
		cin.get();
		cin.get();
	}
	catch (string s)
	{
		if (s.size() > 0)
			cerr << s << endl;
	}

	if (server_socket >= 0)
		close(server_socket);

	return 0;
}
