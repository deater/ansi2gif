all:	\
	linux_logo.gif \
	linux_logo.png \
	linux_logo.eps

linux_logo.eps:	linux_logo_output
	../ansi2gif linux_logo_output linux_logo.eps

linux_logo.gif:	linux_logo_output
	../ansi2gif linux_logo_output linux_logo.gif

linux_logo.png:	linux_logo_output
	../ansi2gif linux_logo_output linux_logo.png


clean:	
	rm -f *~ *.gif *.png *.eps

