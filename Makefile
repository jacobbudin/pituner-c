CC = gcc
CFLAGS = -g -Wall

default: pituner

pituner.o: pituner.c
	$(CC) $(CFLAGS) -c pituner.c

pituner: pituner.o
	$(CC) $(CFLAGS) -o pituner -lbass -lwiringPi -lwiringPiDev pituner.o
