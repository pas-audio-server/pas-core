#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>
#include "../cpp/commands.pb.h"

using namespace std;
using namespace pas;

static bool keep_going = true;

void SIGINTHandler(int)
{
    keep_going = false;
}

void ConnectionHandler(int incoming_socket, int connection_number)
{
	size_t length;
	size_t bytes_received;

	cout << "handling a connection: " << connection_number << endl;
	bytes_received = recv(incoming_socket, &length, sizeof(length), 0);
	if (bytes_received != sizeof(length))
		throw string("received bad size when reading length");
	cout << "Message length: " << length << endl;

	string s;
	s.resize(length);
	bytes_received = recv(incoming_socket, &s[0], length, 0);
	if (bytes_received != length)
		throw string("received size when reading message");

	GenericPB generic;
	generic.ParseFromString(s);
	if (generic.type() == Type::PLAY_TRACK_DEVICE)
	{
		PlayTrackCommand ptc;
		ptc.ParseFromString(s);
		cout << "server device: " << ptc.device_id() << endl;
		cout << "server track id: " << ptc.track_id() << endl;
	}
	else if (generic.type() == Type::SELECT_QUERY)
	{
		SelectQuery sq;
		sq.ParseFromString(s);
		cout << "server column: " << sq.column() << endl;
		cout << "server pattern: " << sq.pattern() << endl;
	}
}

int main(int argc, char * argv[])
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	int listening_socket = -1;
	int port = 5077;
	int incoming_socket = -1;
	vector<thread *> threads;

	signal(SIGINT, SIGINTHandler);
	siginterrupt(SIGINT, 1);

	try
	{
		listening_socket = socket(AF_INET, SOCK_STREAM, 0);
		if (listening_socket < 0)
		{
			perror("Opening socket failed");
			throw string("");
		}

		int optval = 1;
		optval = setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
		if (optval < 0)
		{
			perror("sockopt failed");
			throw string("");
		}

		sockaddr_in listening_sockaddr;

		memset(&listening_sockaddr, 0, sizeof(sockaddr_in));
		listening_sockaddr.sin_family = AF_INET;
		listening_sockaddr.sin_port = htons(port);
		listening_sockaddr.sin_addr.s_addr = INADDR_ANY;

		if (bind(listening_socket, (sockaddr *) &listening_sockaddr, sizeof(sockaddr_in)) < 0)
		{
			perror("Bind failed");
			throw string("");
		}

		if (listen(listening_socket , 4) != 0)
		{
			perror("Error attempting to listen to socket:");
			throw string("");
		}

		sockaddr_in client_info;
		memset(&client_info, 0, sizeof(sockaddr_in));
		int c = sizeof(sockaddr_in);

		int connection_counter = 0;

		while ((incoming_socket = accept(listening_socket, (sockaddr *) &client_info, (socklen_t *) &c)) > 0)
		{
			thread * t = new thread(ConnectionHandler, incoming_socket, connection_counter++);
			threads.push_back(t);
			if (!keep_going)
				break;
		}
	}
	catch (string e)
	{
		if (incoming_socket >= 0)
			close(incoming_socket);

		if (listening_socket >= 0)
			close(listening_socket);

		while (threads.size() > 0)
		{
			threads.at(0)->join();
			threads.erase(threads.begin());
		}
	}
	return 0;
}

