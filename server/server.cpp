#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <thread>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>

#include "network_component.hpp"
#include "file_system_component.hpp"

using namespace std;

bool keep_going = true;
int  port = 5077;

/*  ConnectionHandler is temporary.
*/
void ConnectionHandler(sockaddr_in client_info, int incoming_socket, int connection_number)
{
    size_t bytes_read;
    const int BS = 32;
    char buffer[BS];

    memset(buffer, 0, BS);
    cout << "ConnectionHandler(" << connection_number << ") servicing client at " << inet_ntoa(client_info.sin_addr) << endl;
    while ((bytes_read = recv(incoming_socket, (void *) buffer, BS, 0)) > 0)
    {
	if (bytes_read >= 2 && bytes_read < BS)
	{
	    if (buffer[0] == 's' && buffer[1] == 'q')
		break;
	}
	write(1, buffer, bytes_read);
	memset(buffer, 0, BS);
    }
    cout << "ConnectionHandler(" << connection_number << ") exiting." << endl;
}

int main()
{
	NetworkComponent nw;

	// Connect to database.
	// Initialize audio.
	nw.AcceptConnections(ConnectionHandler);
	exit(0);
}

