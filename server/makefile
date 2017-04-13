# handy command: git ls-files -d | xargs git checkout --

CC	= g++
CFLAGS	= -O3 -Wall -std=c++11 -fopenmp -pthread
#CFLAGS	= -g -Wall -std=c++11 -fopenmp -D _DEBUG_

LDFLAGS	= -lavcodec -lavformat -lmysqlcppconn -fopenmp -lpulse-simple -lpulse -lrt -lprotobuf

SRC		= server.cpp logger.cpp connection_manager.cpp network_component.cpp \
		  db_component.cpp file_system_component.cpp track.cpp audio_component.cpp \
		  ../protos/cpp/commands.pb.cc
 

all						: clear_screen server.out

server.out				: server.o network_component.o file_system_component.o db_component.o \
						  track.o connection_manager.o audio_component.o logger.o commands.pb.o
						  $(CC) $^ -o $@ $(LDFLAGS)

%.o						: %.cpp 
						  $(CC) -c $(CFLAGS) $< -o $@

.PHONY					: clear_screen

clear_screen			:
						  clear

server.o				: server.cpp 

logger.o				: logger.cpp 

connection_manager.o	: connection_manager.cpp 

network_component.o		: network_component.cpp 

db_component.o			: db_component.cpp 

file_system_component.o	: file_system_component.cpp 

track.o					: track.cpp 

audio_component.o		: audio_component.cpp

commands.pb.o			: ../protos/cpp/commands.pb.cc
						  $(CC) -c $(CFLAGS) $< -o $@

depend					:
						  makedepend -Y -- $(CFLAGS) -- $(SRC) 2> /dev/null

clean					:
						  rm -f server core *.o

# DO NOT DELETE

server.o: network_component.hpp file_system_component.hpp utility.hpp
server.o: db_component.hpp track.hpp audio_component.hpp audio_device.hpp
server.o: logger.hpp
logger.o: logger.hpp
connection_manager.o: connection_manager.hpp utility.hpp audio_component.hpp
connection_manager.o: audio_device.hpp db_component.hpp track.hpp logger.hpp
connection_manager.o: ../protos/cpp/commands.pb.h
network_component.o: connection_manager.hpp utility.hpp network_component.hpp
network_component.o: audio_component.hpp audio_device.hpp db_component.hpp
network_component.o: track.hpp logger.hpp
db_component.o: db_component.hpp track.hpp utility.hpp logger.hpp
file_system_component.o: file_system_component.hpp utility.hpp
file_system_component.o: db_component.hpp track.hpp logger.hpp
track.o: track.hpp logger.hpp
audio_component.o: audio_component.hpp audio_device.hpp db_component.hpp
audio_component.o: track.hpp utility.hpp logger.hpp
