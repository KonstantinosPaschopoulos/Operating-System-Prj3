CC=gcc
CFLAGS=  -g -c -Wall

all:    myport \
	vessel \
	portmaster

myport:   myport.c
	$(CC)  $(CFLAGS) myport.c
	$(CC)  myport.o -o myport -lpthread

vessel:   vessel.c
	$(CC)  $(CFLAGS) vessel.c
	$(CC)  vessel.o -o vessel -lpthread

portmaster:   portmaster.c
	$(CC)  $(CFLAGS) portmaster.c
	$(CC)  portmaster.o -o portmaster -lpthread

clean:
	rm -f   \
		myport.o myport	\
		vessel.o vessel	\
		portmaster.o portmaster
