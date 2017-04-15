CC		= g++
CFLAGS	= -O3 -Wall -std=c++11 -pthread -fopenmp 
LDFLAGS	= -lmysqlcppconn -fopenmp
SRC		= fsmain.cpp

%.o			: %.cpp
			$(CC) -c $(CFLAGS) $< -o $@

all			: clear_screen fsmain

fsmain		:	fsmain.o
			$(CC) -o $@ fsmain.o $(LDFLAGS)

clean		:
			rm -f fsmain *.o core

.PHONY					: clear_screen

clear_screen			:
						  clear

