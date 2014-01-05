CC = gcc
CFLAGS = -O2 -Wall
LFLAGS =

all:	16color

16color:	16color.o
	$(CC) $(LFLAGS) -o 16color 16color.o

16color.o:	16color.c
	$(CC) $(CFLAGS) -c 16color.c

clean:
	rm -f *~ *.o 16color

