##############################################################
#  Makefile for ansi2gif 0.9.9 -- by Vince Weaver            #
#                                                            #
#  To modify for your configuration, add or remove the #     #
#                                                            #
##############################################################

#Your compiler.  If gcc doesn't work, try CC
CC = gcc

#You need the "gd" gif library, available from 
# http://www.boutell.com/gd/
#
# Either install it, or else point the options below to point to the location
# of the library and include files.
#GD_L_OPTS = -L/usr/lib 
#GD_C_OPTS = -I/usr/include

GD_L_OPTS = ../gd1.2/libgd.a
GD_C_OPTS = -I../gd1.2/

#Standard compiler and library options

C_OPTS = $(GD_C_OPTS) -O2 -Wall

 
L_OPTS = $(GD_L_OPTS) -lm 


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

whirlgif.o:	whirlgif.c
	$(CC) $(C_OPTS) -c whirlgif.c
	
gifdecod.o:	gifdecod.c
	$(CC) $(C_OPTS) -c gifdecod.c

gifencod.o:	gifencod.c
	$(CC) $(C_OPTS) -c gifencod.c

ansi2gif.o:	ansi2gif.c
	$(CC) $(C_OPTS) -c ansi2gif.c 
