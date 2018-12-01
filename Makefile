CC=gcc
CFLAGS=  -g -c -Wall

all:    myport \
	vessel

myport:   myport.c
	$(CC)  $(CFLAGS) myport.c
	$(CC)  myport.o -o myport -lpthread

vessel:   vessel.c
	$(CC)  $(CFLAGS) vessel.c
	$(CC)  vessel.o -o vessel

clean:
	rm -f   \
		myport.o myport	\
		vessel.o vessel
