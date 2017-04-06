#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

const int BS = 2048;

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

bool SetUpConnection(int server_socket, sockaddr_in * server_sockaddr, hostent * server_hostent, int port)
{
	bool rv = true;

	memset(server_sockaddr, 0, sizeof(sockaddr_in));
	server_sockaddr->sin_family = AF_INET;
	memmove(&server_sockaddr->sin_addr.s_addr, server_hostent->h_addr, server_hostent->h_length);
	server_sockaddr->sin_port = htons(port);

	if (connect(server_socket, (struct sockaddr*) server_sockaddr, sizeof(sockaddr_in)) == -1)
	{
		perror("Client connection failure:");
		rv = false;
	}
	return rv;
}

bool SimpleCase(int server_socket, string command, string legend)
{
	bool rv = true;
	char buffer[BS];
	assert(server_socket >= 0);
	memset(buffer, 0, BS);
	size_t bytes_sent = send(server_socket, (const void *) command.c_str(), command.size(), 0);
	if (bytes_sent != command.size())
	{
		rv = false;
	}
	else
	{
		size_t bytes_read = recv(server_socket, (void *) buffer, BS, 0);
		if (bytes_read > 0)
		{
			string s(buffer);
			cout << legend << ": "  << s;
		}
	}
	return rv;
}

int main(int argc, char * argv[])
{
	int port = 5077;
    char * ip = (char *) ("127.0.0.1");

	int server_socket;
	hostent * server_hostent;
	sockaddr_in server_sockaddr;

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)
	{
		perror("Client failed to open socket");
		exit(1);
	}

	server_hostent = gethostbyname(ip);
	if (server_hostent == nullptr)
	{
		cerr << "Client failed in gethostbyname()" << endl;
		close(server_socket);
		exit(1);
	}

	if (!SetUpConnection(server_socket, &server_sockaddr, server_hostent, port))
	{
		close(server_socket);
		exit(1);
	}

	bool connected = true;
	char buffer[BS];
	ssize_t bytes_sent;
	ssize_t bytes_read;
	string l;
	cout << "Command: ";
	getline(cin, l);
	while (l != "quit")
	{
		memset(buffer, 0, BS);

		if (l == "cm")
		{
			char cmd;
			cout << "Which command (p, z, r or c): ";
			cin >> cmd;
			if (cmd == 'p' || cmd == 'c' || cmd == 'z' || cmd== 'r')
			{
				char unit;
				cout << "Which device (0 - 9): ";
				cin >> unit;
				if (unit >= '0' && unit <= '9')
				{
					string id_number;
					if (cmd == 'p')
					{
						cout << "ID number: ";
						cin >> id_number;
						// No need to vet this. It will be done later.
					}
					l = unit + string(" ") + cmd + string(" ") + id_number; 
					bytes_sent = send(server_socket, (const void *) l.c_str(), l.size(), 0);
					if (bytes_sent != (int) l.size())
						break;
				}
			} 
		}
		else if (l == "se")
		{
			string column, pattern;
			cout << "Column: ";
			cin >> column;
			cout << "Pattern (no spaces): ";
			cin >> pattern;
			l = "se " + column + " " + pattern;
			bytes_sent = send(server_socket, (const void *) l.c_str(), l.size(), 0);
			if (bytes_sent != (int) l.size())
				break;

			string s;
			while (s.find("****") == string::npos)
			{
				memset(buffer, 0, BS);
				bytes_read = recv(server_socket, (void *) buffer, BS, 0);
				if (bytes_read > 0) ;
				s = string(buffer);
				cout << s;
			}
			cout << endl;
		}
		else if (l == "ac")
		{
			SimpleCase(server_socket, l, "Artist count");
		}
		else if (l == "tc")
		{
			SimpleCase(server_socket, l, "Track count");
		}
		else if (l == "ti")
		{
			char unit;
			cout << "Which device (0 - 9): ";
			cin >> unit;
			if (unit >= '0' && unit <= '9')
			{
				l = unit + string(" ") + l; 
				SimpleCase(server_socket, l, "Time code");
				cout << endl;
			}
		}
		else if (l == "rc")
		{
			if (connected)
				cout << "Already connected" << endl;
			else
			{
				server_socket = socket(AF_INET, SOCK_STREAM, 0);
				if (server_socket < 0)
				{
					perror("Client failed to open socket for reconnection.");
					break;
				}
				if (!SetUpConnection(server_socket, &server_sockaddr, server_hostent, port))
				{
					close(server_socket);
					server_socket = -1;
					break;
				}
				cout << "Reconnected" << endl;
			}
		}
		else if (l == "sq")
		{
			cout << "Disconnecting" << endl;
			bytes_sent = send(server_socket, (const void *) l.c_str(), l.size(), 0);
			if (bytes_sent != (int) l.size())
				break;
			close(server_socket);
			server_socket = -1;
			connected = false;
		}
		cin.ignore(99, '\n');
		cout << "Command: ";
		getline(cin, l);
	}
	if (server_socket >= 0)
		close(server_socket);
	return 0;
}
