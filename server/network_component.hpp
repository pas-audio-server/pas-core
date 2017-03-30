#pragma once
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

class NetworkComponent
{
public:
	NetworkComponent();
	void AcceptConnections(void (*f)(sockaddr_in, int, int));

private:
	static bool keep_going;
	static void SIGINTHandler(int);
	int port;
	const int max_connections = 4;
	int listening_socket;
	std::vector<std::thread *> threads;
};

