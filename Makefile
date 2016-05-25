TARGET: main

CC	= g++
CFLAGS	= -g -std=c++11 -Wall -pedantic
LFLAGS	= -std=c++11 -Wall

#main: main.o err.o
#	$(CC) $(CFLAGS) $^ -o $@

main: main.o err.o
	$(CC) $(CFLAGS) -o main main.o err.o

main.o : main.cpp err.h
	$(CC) $(CFLAGS) -c main.cpp

err.o : err.cpp err.h
	$(CC) $(CFLAGS) -c err.cpp

.PHONY: clean TARGET

clean:
	rm -f main *.o *~ *.bak