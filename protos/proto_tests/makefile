CC	= g++
CFLAGS	= -g -Wall -std=c++11 -pthread
LDFLAGS	= -lprotobuf -lpthread

%.o			: %.cpp
			$(CC) -c $(CFLAGS) $< -o $@

all			:	client.out server.out

server.out	:	server.o commands.pb.o
			$(CC) server.o commands.pb.o $(LDFLAGS) -o $@

client.out	:	client.o commands.pb.o
			$(CC) client.o commands.pb.o $(LDFLAGS) -o $@

commands.pb.o	:	../cpp/commands.pb.cc
			$(CC) -c $(CFLAGS) $< -o $@
