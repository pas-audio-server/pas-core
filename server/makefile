CC	= g++
#CFLAGS	= -O3 -Wall -std=c++11 -fopenmp
CFLAGS	= -g -Wall -std=c++11 -fopenmp -D _DEBUG_
LDFLAGS	= -lpthread -lavcodec -lavformat -lmysqlcppconn -fopenmp -lpulse-simple -lpulse -lrt
SRC		= $(wildcard *.cpp)
OBJS	= $(SRC:.cpp=.o)

all						: clear_screen server.out

server.out				: server.o network_component.o file_system_component.o db_component.o track.o connection_manager.o audio_component.o
						$(CC) $^ -o $@ $(LDFLAGS)

%.o						: %.cpp
						$(CC) -c $(CFLAGS) $< -o $@

.PHONY					: clear_screen

clear_screen			:
						clear

server.o				: server.cpp network_component.hpp file_system_component.hpp utility.hpp audio_component.hpp audio_device.hpp

connection_manager.o	: connection_manager.cpp connection_manager.hpp

network_component.o		: network_component.cpp network_component.hpp utility.hpp connection_manager.hpp

db_component.o			: db_component.cpp db_component.hpp utility.hpp track.hpp

file_system_component.o	: file_system_component.cpp file_system_component.hpp db_component.hpp utility.hpp track.hpp

track.o					: track.cpp track.hpp

audio_component.o		: audio_component.cpp audio_component.hpp

clean					:
						rm -f server $(OBJS)
