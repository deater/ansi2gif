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
C_OPTS = -O2 -Wall 

#If You want GIF support, and you have installed the gd library
# [see README] then uncomment the following.

EXTRA = $(C_OPTS) -DMAKE_GIF
L_OPTS = -lgd -lm

# DO NOT EDIT BELOW THIS LINE

all:	fontprint

clean:
	rm -f *.o
	rm -f fontprint
	rm -f *~

install:	fontprint
	cp fontprint /usr/local/bin
	
fontprint:	fontprint.o 
	$(CC) $(C_OPTS) -o fontprint fontprint.o $(L_OPTS)
	@strip fontprint

fontprint.o:	fontprint.c
	$(CC) $(C_OPTS) $(EXTRA) -c fontprint.c 
