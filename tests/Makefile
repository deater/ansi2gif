all:	\
	linux_logo.gif \
	linux_logo.png \
	linux_logo.eps \
	utopia_blink.gif \
	utopia21.gif \
	utopia24.gif \
	ansi1.png \
	ansi1.gif \
	bl-mekka.png \
	bl-mekka.gif \
	16color.png \
	16color.gif \
	16color.eps \
	256color.gif \
	256color.png \
	256color.eps \
	256color_oflo.gif \
	256color_oflo.png \
	map12.gif \
	map12.png \
	24bit.gif \
	24bit.png \
	24bit.eps


map12.png:	map12.ans
	../ansi2gif map12.ans map12.png

map12.gif:	map12.ans
	../ansi2gif map12.ans map12.gif


16color.png:	16color.ans
	../ansi2gif 16color.ans 16color.png

16color.gif:	16color.ans
	../ansi2gif 16color.ans 16color.gif

16color.eps:	16color.ans
	../ansi2gif 16color.ans 16color.eps


24bit.png:	24bit.ans
	../ansi2gif 24bit.ans 24bit.png

24bit.gif:	24bit.ans
	../ansi2gif 24bit.ans 24bit.gif

24bit.eps:	24bit.ans
	../ansi2gif 24bit.ans 24bit.eps


256color.png:	256color.ans
	../ansi2gif 256color.ans 256color.png

256color.gif:	256color.ans
	../ansi2gif 256color.ans 256color.gif

256color.eps:	256color.ans
	../ansi2gif 256color.ans 256color.eps


256color_oflo.png:	256color_oflo.ans
	../ansi2gif 256color_oflo.ans 256color_oflo.png

256color_oflo.gif:	256color_oflo.ans
	../ansi2gif 256color_oflo.ans 256color_oflo.gif



ansi1.png:	ansi1.ans
	../ansi2gif --ysize auto ansi1.ans ansi1.png

ansi1.gif:	ansi1.ans
	../ansi2gif --ysize auto ansi1.ans ansi1.gif


bl-mekka.png:	bl-mekka.ans
	../ansi2gif --ysize auto bl-mekka.ans bl-mekka.png

bl-mekka.gif:	bl-mekka.ans
	../ansi2gif --ysize auto bl-mekka.ans bl-mekka.gif


linux_logo.eps:	linux_logo_output
	../ansi2gif linux_logo_output linux_logo.eps

linux_logo.gif:	linux_logo_output
	../ansi2gif linux_logo_output linux_logo.gif

linux_logo.png:	linux_logo_output
	../ansi2gif linux_logo_output linux_logo.png


utopia_blink.gif:	utopia_blink.ans
	../ansi2gif --blink utopia_blink.ans utopia_blink.gif


utopia21.gif:	utopia21.ans
	../ansi2gif --animate utopia21.ans utopia21.gif


utopia24.gif:	utopia24.ans
	../ansi2gif --animate utopia24.ans utopia24.gif

clean:	
	cd code && make clean
	rm -f *~ *.gif *.png *.eps

