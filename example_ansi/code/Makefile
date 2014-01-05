CC = gcc
CFLAGS = -O2 -Wall
LFLAGS =

all:	16color 256color


16color:	16color.o
	$(CC) $(LFLAGS) -o 16color 16color.o

16color.o:	16color.c
	$(CC) $(CFLAGS) -c 16color.c
	

256color:	256color.o
	$(CC) $(LFLAGS) -o 256color 256color.o

256color.o:	256color.c
	$(CC) $(CFLAGS) -c 256color.c


clean:
	rm -f *~ *.o 16color 256color

