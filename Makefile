CC=gcc
CFLAGS=  -g -c -Wall

all:    myport \
	vessel \
	portmaster

myport:   myport.o myfunctions.o
	$(CC)  myport.o myfunctions.o -o myport -lpthread

vessel:   vessel.o myfunctions.o
	$(CC)  vessel.o myfunctions.o -o vessel -lpthread

portmaster:   portmaster.o myfunctions.o
	$(CC)  portmaster.o myfunctions.o -o portmaster -lpthread

myfunctions.o:   myfunctions.c myfunctions.h
	$(CC)  $(CFLAGS) myfunctions.c

myport.o:   myport.c
	$(CC)  $(CFLAGS) myport.c

vessel.o:   vessel.c
	$(CC)  $(CFLAGS) vessel.c

portmaster.o:   portmaster.c
	$(CC)  $(CFLAGS) portmaster.c

clean:
	rm -f   \
		myport.o myport	\
		vessel.o vessel	\
		portmaster.o portmaster \
		myfunctions.o
