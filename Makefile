##############################################################
#  Makefile for ansi2gif -- by Vince Weaver                  #
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
#GD_LFLAGS = -L/usr/lib
#GD_CFLAGS = -I/usr/include

#Standard compiler and library options

CFLAGS = $(GD_CFLAGS) -Wall -O2 -g

LFLAGS = $(GD_LFLAGS) -lgd -lm


# DO NOT EDIT BELOW THIS LINE

all:	ansi2gif

clean:
	rm -f *.o
	rm -f ansi2gif ansi2png ansi2eps
	rm -f *~
	cd tests && make clean
	cd sample_fonts && make clean
	cd sample_ansi_art && make clean

install:	ansi2gif
	cp ansi2gif /usr/local/bin
	cp ansi2png /usr/local/bin
	cp ansi2eps /usr/local/bin

ansi2gif:	ansi2gif.o pcfont.o
	$(CC) -o ansi2gif ansi2gif.o pcfont.o $(LFLAGS)
	ln -f -s ansi2gif ansi2png
	ln -f -s ansi2gif ansi2eps

pcfont.o:     pcfont.c
	      $(CC) $(CFLAGS) -c pcfont.c

ansi2gif.o:	ansi2gif.c
	$(CC) $(CFLAGS) -c ansi2gif.c
