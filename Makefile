##############################################################
#  Makefile for Fontprint 3.0.3 -- by Vince Weaver           #
#                                                            #
#  To modify for your configuration, add or remove the #     #
#                                                            #
##############################################################

#Your compiler.  If gcc doesn't work, try CC
CC = gcc

#For Linux
PLATFORM = 'Linux'
C_OPTS = -O2 -Wall  -DMAKE_GIF

#If You want GIF support, and you have installed the gd library
# [see README] then uncomment the following.

L_OPTS = -lgd -lm

# DO NOT EDIT BELOW THIS LINE

all:	ansi2gif

clean:
	rm -f *.o
	rm -f ansi2gif
	rm -f *~

install:	ansi2gif
	cp ansi2gif /usr/local/bin
	
ansi2gif:	ansi2gif.o whirlgif.o gifdecod.o gifencod.o gifdecod.o
	$(CC) -o ansi2gif ansi2gif.o whirlgif.o gifencod.o gifdecod.o $(L_OPTS)
	@strip ansi2gif

whirgif.o:	whirlgif.c
	$(CC) $(C_OPTS) -c whirlgif.c
	
gifdecod.o:	gifdecod.c
	$(CC) $(C_OPTS) -c gifdecod.c

gifencod.o:	gifencod.c
	$(CC) $(C_OPTS) -c gifencod.c

ansi2gif.o:	ansi2gif.c
	$(CC) $(C_OPTS) -c ansi2gif.c 

font2include:	font2include.o
	$(CC) -o font2include font2include.o 
	
font2include.o:	font2include.c
	$(CC) $(C_OPTS) -c font2include.c
