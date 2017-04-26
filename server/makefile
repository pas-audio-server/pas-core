# handy command: git ls-files -d | xargs git checkout --

CC	= g++
CFLAGS	= -O3 -Wall -std=c++11 -pthread -D _DEBUG_
LDFLAGS	= -lavcodec -lavformat -lmysqlcppconn -lpulse-simple -lpulse -lrt -lprotobuf

SRC		= audio_component.cpp \
		  connection_manager.cpp \
		  db_component.cpp \
		  discover_dacs.cpp \
		  logger.cpp \
		  network_component.cpp \
		  server.cpp \
		  track.cpp \
		  commands.pb.cc

# Now, add on platform-dependent flags.
# Taken from http://stackoverflow.com/a/12099167
ifeq ($(OS),Windows_NT)
	# Nothing Windows-specific yet.
    # ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
    # else
    #     ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
    #     endif
    #     ifeq ($(PROCESSOR_ARCHITECTURE),x86)
    #     endif
    # endif
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
		CFLAGS += -fopenmp
		LDFLAGS += -fopenmp
    endif
    ifeq ($(UNAME_S),Darwin)
		# -fopenmp flag is on by default with clang.
    endif
    # UNAME_P := $(shell uname -p)
    # ifeq ($(UNAME_P),x86_64)
    # endif
    # ifneq ($(filter %86,$(UNAME_P)),)
    # endif
    # ifneq ($(filter arm%,$(UNAME_P)),)
    # endif
endif

all						: clear_screen server.out

server.out				: server.o network_component.o db_component.o discover_dacs.o \
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

track.o					: track.cpp 

audio_component.o		: audio_component.cpp

commands.pb.o			: ../protos/cpp/commands.pb.cc
						  $(CC) -c $(CFLAGS) $< -o $@

discover_dacs.o 		: discover_dacs.cpp

depend					:
						  makedepend -Y -- $(CFLAGS) -- $(SRC) 2> /dev/null

clean					:
						  rm -f server core *.o

# DO NOT DELETE

server.o: network_component.hpp utility.hpp discover_dacs.hpp
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
track.o: track.hpp logger.hpp
audio_component.o: audio_component.hpp audio_device.hpp db_component.hpp
audio_component.o: track.hpp utility.hpp logger.hpp ../protos/cpp/commands.pb.h
discover_dacs.o: discover_dacs.hpp audio_device.hpp
