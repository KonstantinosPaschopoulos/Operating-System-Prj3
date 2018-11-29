CC=gcc
CFLAGS=  -g -c -Wall

all:    myport

myport:   myport.c
	$(CC)  $(CFLAGS) myport.c
	$(CC)  myport.o -o myport

clean:
	rm -f   \
		myport.o myport
