#Your compiler.  If gcc doesn't work, try CC
CC = gcc

#For Linux
PLATFORM = 'Linux'
C_OPTS = -O2 -Wall

# DO NOT EDIT BELOW THIS LINE

all:	font2include

clean:
	rm -f *.o
	rm -f font2include
	rm -f *~

font2include:	font2include.o
	$(CC) -o font2include font2include.o 
	
font2include.o:	font2include.c
	$(CC) $(C_OPTS) -c font2include.c
