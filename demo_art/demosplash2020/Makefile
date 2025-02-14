ANSI2PNG = ~/research/ansi2gif/ansi2gif.git/ansi2png

all:	a2_myst.png

a2_myst.png:	a2_myst.ans
	$(ANSI2PNG) --ysize auto a2_myst.ans  > a2_myst.png

clean:	
	rm -f a2_myst.png *~
