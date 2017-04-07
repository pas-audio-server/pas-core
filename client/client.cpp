#include <iostream>
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

using namespace std;

#define	LOG(s)		(string(__FILE__) + string(":") + string(__FUNCTION__) + string("() line: ") + to_string(__LINE__) + string(" msg: ") + string(s))

const int BS = 2048;

bool keep_going = true;

map<string, string> simple_commands;
map<string, string> one_arg_commands;

void SIGHandler(int signal_number)
{
	keep_going = false;
	cout << endl;
	throw string("");	
}

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

inline bool BeginsWith(string const & fullString, string const & prefix)
{
	bool rv = false;

	if (fullString.length() >= prefix.length())
	{
		rv = (0 == fullString.compare(0, prefix.length(), prefix));
	}
	return rv;
}

bool HasEnding (string const & fullString, string const & ending)
{
	bool rv = false;

	if (fullString.length() >= ending.length())
	{
		rv = (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
	}
	return rv;
}

bool HandleArgCommand(int server_socket, string command, int argc = 1)
{
	assert(server_socket >= 0);

	bool rv = false;
	if (one_arg_commands.find(command) != one_arg_commands.end())
	{
		rv = true;

		string l1;
		cout << "Argument 1: ";
		getline(cin, l1);

		string l2;
		if (argc == 2)
		{
			cout << "Argument 2: ";
			getline(cin, l2);
		}

		char buffer[BS];
		memset(buffer, 0, BS);

		command = one_arg_commands[command] + l1 + ((argc > 1) ? (string(" ") + l2) : string(""));

		send(server_socket, (const void *) command.c_str(), command.size(), 0);
	}
	return rv;
}

bool HandleSimple(int server_socket, string command)
{
	assert(server_socket >= 0);

	bool rv = false;
	if (simple_commands.find(command) != simple_commands.end())
	{
		rv = true;

		char buffer[BS];
		memset(buffer, 0, BS);

		command = simple_commands[command];

		size_t bytes_sent = send(server_socket, (const void *) command.c_str(), command.size(), 0);
		// Note - commands that begin with a digit do not get anything returned.
		// rewrite this as 0 args, 0 args with return etc
		if (!isdigit(command.at(0)) && command != "sq" && bytes_sent == command.size())
		{
			size_t bytes_read = recv(server_socket, (void *) buffer, BS, 0);
			if (bytes_read > 0)
			{
				string s(buffer);
				cout << s;
				if (s.at(s.size() - 1) != '\n')
					cout << endl;
			}
		}
	}
	return rv;
}

void OrganizeCommands()
{
	simple_commands.insert(make_pair("sq", "sq"));
	simple_commands.insert(make_pair("tc", "tc"));
	simple_commands.insert(make_pair("ac", "ac"));
	simple_commands.insert(make_pair("ti 0", "ti 0"));
	simple_commands.insert(make_pair("ti 1", "ti 1"));
	simple_commands.insert(make_pair("ti 2", "ti 2"));
	simple_commands.insert(make_pair("ti 3", "ti 3"));

	simple_commands.insert(make_pair("s 0", "0 s"));
	simple_commands.insert(make_pair("s 1", "1 s"));
	simple_commands.insert(make_pair("s 2", "2 s"));
	simple_commands.insert(make_pair("s 3", "3 s"));

	simple_commands.insert(make_pair("z 0", "0 z"));
	simple_commands.insert(make_pair("z 1", "1 z"));
	simple_commands.insert(make_pair("z 2", "2 z"));
	simple_commands.insert(make_pair("z 3", "3 z"));
	
	simple_commands.insert(make_pair("r 0", "0 r"));
	simple_commands.insert(make_pair("r 1", "1 r"));
	simple_commands.insert(make_pair("r 2", "2 r"));
	simple_commands.insert(make_pair("r 3", "3 r"));

	one_arg_commands.insert(make_pair("p 0", "0 p "));
	one_arg_commands.insert(make_pair("p 1", "1 p "));
	one_arg_commands.insert(make_pair("p 2", "2 p "));
	one_arg_commands.insert(make_pair("p 3", "3 p "));


}

int main(int argc, char * argv[])
{
	int server_socket = -1;
	bool connected = false;
	char buffer[BS];
	ssize_t bytes_sent;
	ssize_t bytes_read;
	string l;

	OrganizeCommands();
	try
	{
		if (signal(SIGINT, SIGHandler) == SIG_ERR)
			throw LOG("");

		server_socket = InitializeNetworkConnection(argc, argv);
		connected = true;
		while (keep_going)
		{
			memset(buffer, 0, BS);
			cout << "Command: ";
			getline(cin, l);
			
			if (l == "quit")
				break;

			if (l == "rc")
			{
				if (!connected)
				{
					server_socket = InitializeNetworkConnection(argc, argv);
					if (server_socket < 0)
						break;
					connected = true;
					cout << "Connected" << endl;
				}
				continue;
			}

			if (!connected)
				continue;

			if (HandleSimple(server_socket, l))
			{
				if (l == "sq")
				{
					cout << "Disconnected" << endl;
					connected = false;
					server_socket = -1;
				}
				continue;
			}

			if (HandleArgCommand(server_socket, l))
				continue;
		}
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
