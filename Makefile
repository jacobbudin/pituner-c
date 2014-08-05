CC = gcc
CFLAGS = -g -Wall

default: pituner

parson.o: parson.c
	$(CC) $(CFLAGS) -c parson.c

pituner.o: pituner.c
	$(CC) $(CFLAGS) -c pituner.c

pituner: pituner.o parson.o
	$(CC) $(CFLAGS) -o pituner -lbass -lwiringPi -lwiringPiDev parson.o pituner.o
