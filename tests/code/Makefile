CC = gcc
CFLAGS = -O2 -Wall
LFLAGS =

all:	16color 256color 256color_oflo 24bit


16color:	16color.o
	$(CC) $(LFLAGS) -o 16color 16color.o

16color.o:	16color.c
	$(CC) $(CFLAGS) -c 16color.c
	

24bit:	24bit.o
	$(CC) $(LFLAGS) -o 24bit 24bit.o

24bit.o:	24bit.c
	$(CC) $(CFLAGS) -c 24bit.c



256color:	256color.o
	$(CC) $(LFLAGS) -o 256color 256color.o

256color.o:	256color.c
	$(CC) $(CFLAGS) -c 256color.c


256color_oflo:	256color_oflo.o
	$(CC) $(LFLAGS) -o 256color_oflo 256color_oflo.o

256color_oflo.o:	256color_oflo.c
	$(CC) $(CFLAGS) -c 256color_oflo.c


clean:
	rm -f *~ *.o 16color 256color 256color_oflo 24bit

