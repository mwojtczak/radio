TARGET: player

CC	= g++
CFLAGS	= -g -std=c++11 -lboost_regex -Wall -pedantic
LFLAGS	= -std=c++11 -Wall

#player: player.o err.o
#	$(CC) $(CFLAGS) $^ -o $@

player: player.o err.o
	$(CC) $(CFLAGS) -o player player.o err.o

player.o : player.cpp err.h
	$(CC) $(CFLAGS) -c player.cpp

err.o : err.cpp err.h
	$(CC) $(CFLAGS) -c err.cpp

.PHONY: clean TARGET

clean:
	rm -f player *.o *~ *.bak