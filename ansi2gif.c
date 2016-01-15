/*        ---------- ansi2gif.c ----------        *\
\*                 by Vince Weaver                */
/*             <vince _at_ deater.net>            *\
\*        Makes an image out of "ANSI" files      */
/*         Based on my "fontprint" program.       *\
\*  http://www.deater.net/weave/vmwprod/ansi2gif/ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>

#include <gd.h>

#include "default_font.h"
#include "pcfont.h"
#include "whirlgif.h"


#define DEFAULT_TIMEDELAY 1 /* 1/100 of a second */
#define DEFAULT_XSIZE    80
#define DEFAULT_YSIZE    25

/* For eps */
#define XFONTSIZE 7.1
#define YFONTSIZE 13

#define VERSION "0.10.0"

#define OUTPUT_PNG 0
#define OUTPUT_GIF 1
#define OUTPUT_EPS 2
#define OUTPUT_MPG 3

/* Attributes */
#define BLINK		1
#define BOLD		2
#define UNDERLINE	4

typedef struct {
	unsigned char *font_data;
	int width;
	int height;
	int numchars;
} vga_font;


	/* If index=0, return the number of numbers */
static int parse_numbers(char *string,int index) {

	int digit[3];  /* Assuming no 4 digit movements */
	int i=0,x=0,n=0,nums=0;

	if (index==0) {
		while (string[i]!='\000') {
			if (string[i]==';') nums++;
			i++;
		}
		return nums+1;
	}

	while (string[i]!='\000') {
		/* Spec says ';' but here !digit works */
		if (!isdigit(string[i])) {
			x++;
			if (index==x) {
				if (n==1) return digit[0];
				if (n==2) return (digit[0]*10)+digit[1];
				if (n==3) return (digit[0]*100)+(digit[1]*10)+digit[2];
				return 1;  /* If no number, default is 1 */
			}
			n=-1;
		}
		if (n>=0) digit[n]=string[i]-48;
		n++;
		i++;
	}
	return 0;
}


static int ansi_color[256];
static int colorC[256];

static gdImagePtr im,im2,im3;
static vga_font *font_to_use=NULL;
static unsigned char *screen;
static unsigned char *attributes;
static unsigned int *fore_colors;
static unsigned int *back_colors;

#define ANSI_BLACK		0
#define ANSI_RED		1
#define ANSI_GREEN		2
#define ANSI_BROWN		3
#define ANSI_BLUE		4
#define ANSI_PURPLE		5
#define ANSI_CYAN		6
#define ANSI_GREY		7
#define ANSI_DARKGREY		8
#define ANSI_BRIGHTRED		9
#define ANSI_BRIGHTGREEN	10
#define ANSI_YELLOW		11
#define ANSI_BRIGHTBLUE		12
#define ANSI_PINK		13
#define ANSI_BRIGHTCYAN		14
#define ANSI_WHITE		15

static void setup_gd_16colors(void) {

	ansi_color[ANSI_BLACK] = 	gdImageColorAllocate(im,0x00,0x00,0x00);
	ansi_color[ANSI_RED] =		gdImageColorAllocate(im,0xaa,0x00,0x00);
	ansi_color[ANSI_GREEN] =	gdImageColorAllocate(im,0x00,0xaa,0x00);
	ansi_color[ANSI_BROWN] =	gdImageColorAllocate(im,0xaa,0x55,0x22);
	ansi_color[ANSI_BLUE] =		gdImageColorAllocate(im,0x00,0x00,0xaa);
	ansi_color[ANSI_PURPLE] =	gdImageColorAllocate(im,0xaa,0x00,0xaa);
	ansi_color[ANSI_CYAN] =		gdImageColorAllocate(im,0x00,0xaa,0xaa);
	ansi_color[ANSI_GREY] =		gdImageColorAllocate(im,0xaa,0xaa,0xaa);
	ansi_color[ANSI_DARKGREY] =	gdImageColorAllocate(im,0x7d,0x7d,0x7d);
	ansi_color[ANSI_BRIGHTRED] =	gdImageColorAllocate(im,0xff,0x7d,0x7d);
	ansi_color[ANSI_BRIGHTGREEN] =	gdImageColorAllocate(im,0x00,0xff,0x00);
	ansi_color[ANSI_YELLOW] =	gdImageColorAllocate(im,0xff,0xff,0x00);
	ansi_color[ANSI_BRIGHTBLUE] =	gdImageColorAllocate(im,0x00,0x00,0xff);
	ansi_color[ANSI_PINK] =		gdImageColorAllocate(im,0xff,0x00,0xff);
	ansi_color[ANSI_BRIGHTCYAN] =	gdImageColorAllocate(im,0x00,0xff,0xff);
	ansi_color[ANSI_WHITE] =	gdImageColorAllocate(im,0xff,0xff,0xff);

	/* Setup the colors to use for character animation */
	/*   Can we just share the above?                  */

	colorC[ANSI_BLACK] = 		gdImageColorAllocate(im2,0x00,0x00,0x00);
	colorC[ANSI_RED] =		gdImageColorAllocate(im2,0xaa,0x00,0x00);
	colorC[ANSI_GREEN] =		gdImageColorAllocate(im2,0x00,0xaa,0x00);
	colorC[ANSI_BROWN] =		gdImageColorAllocate(im2,0xaa,0x55,0x22);
	colorC[ANSI_BLUE] =		gdImageColorAllocate(im2,0x00,0x00,0xaa);
	colorC[ANSI_PURPLE] =		gdImageColorAllocate(im2,0xaa,0x00,0xaa);
	colorC[ANSI_CYAN] =		gdImageColorAllocate(im2,0x00,0xaa,0xaa);
	colorC[ANSI_GREY] =		gdImageColorAllocate(im2,0xaa,0xaa,0xaa);
	colorC[ANSI_DARKGREY] =		gdImageColorAllocate(im2,0x7d,0x7d,0x7d);
	colorC[ANSI_BRIGHTRED] =	gdImageColorAllocate(im2,0xff,0x7d,0x7d);
	colorC[ANSI_BRIGHTGREEN] =	gdImageColorAllocate(im2,0x00,0xff,0x00);
	colorC[ANSI_YELLOW] =		gdImageColorAllocate(im2,0xff,0xff,0x00);
	colorC[ANSI_BRIGHTBLUE] =	gdImageColorAllocate(im2,0x00,0x00,0xff);
	colorC[ANSI_PINK] =		gdImageColorAllocate(im2,0xff,0x00,0xff);
	colorC[ANSI_BRIGHTCYAN] =	gdImageColorAllocate(im2,0x00,0xff,0xff);
	colorC[ANSI_WHITE] =		gdImageColorAllocate(im2,0xff,0xff,0xff);


}

static int allocated_256colors=0;

static void setup_gd_256colors(gdImagePtr *im) {

	int i,r,g,b;
	double grey;

	/* 6x6x6 color cube */

	/* This formula seems to be the one xfc4 term uses */
	/* when screen-capturing the result and using the  */
	/* gimp color picker				   */

	for(r=0;r<6;r++) {
		for(g=0;g<6;g++) {
			for(b=0;b<6;b++) {

				ansi_color[16+(36*r)+(6*g)+b] =
					gdImageColorAllocate(*im,
							r==0?0:55+r*40,
							g==0?0:55+g*40,
							b==0?0:55+b*40);
			}
		}
	}

	/* 24 steps of greyscale */
	for(i=0;i<24;i++) {
		grey=(256.0/24.0)*(double)i;
		ansi_color[0xe8+i] = gdImageColorAllocate(*im,grey,grey,grey);
	}

}

static void setup_eps_256colors(void) {

	int i,r,g,b;
	double grey;

	/* 6x6x6 color cube */

	/* This formula seems to be the one xfc4 term uses */
	/* when screen-capturing the result and using the  */
	/* gimp color picker				   */

	for(r=0;r<6;r++) {
		for(g=0;g<6;g++) {
			for(b=0;b<6;b++) {

				ansi_color[16+(36*r)+(6*g)+b] =
					((r==0?0:55+r*40)<<16)+
					((g==0?0:55+g*40)<<8)+
					(b==0?0:55+b*40);
			}
		}
	}

	/* 24 steps of greyscale */
	for(i=0;i<24;i++) {
		grey=(256.0/24.0)*(double)i;
		ansi_color[0xe8+i] = ((int)grey<<16)+((int)grey<<8)+((int)grey);
	}

}

static void setup_256colors(int output_type) {

	if (output_type==OUTPUT_EPS) {
		setup_eps_256colors();
	}
	else {
		setup_gd_256colors(&im);
		setup_gd_256colors(&im2);
	}


	allocated_256colors=1;
}

static int map_24bitcolor(int output_type, int r, int g, int b) {

	if (output_type==OUTPUT_EPS) {
		return (r<<16)+(g<<8)+b;
	}
	else {
		/* Not exact, matches to the 6x6x6 color */
		/* TODO: proper 24-bit color support     */
		return gdImageColorResolve(im,r,g,b);
	}
}

static void setup_gd(FILE *out_f,int x_size,int y_size) {

#if 0
	im = gdImageCreateTrueColor(x_size*8,y_size*16);  /* Full Screen */
	im2 = gdImageCreateTrueColor(8,16);               /* One Character */
#endif

	/* indexed is smaller */
	im = gdImageCreate(x_size*8,y_size*16);  /* Full Screen */
	im2 = gdImageCreate(8,16);               /* One Character */

	setup_gd_16colors();

}



static void setup_eps(FILE *out_f,int x_size, int y_size) {

	ansi_color[ANSI_BLACK] =(0x00<<16)+(0x00<<8)+0x00;
	ansi_color[ANSI_BLUE] =(0x00<<16)+(0x00<<8)+0xAA;
	ansi_color[ANSI_GREEN] =(0x00<<16)+(0xAA<<8)+0x00;
	ansi_color[ANSI_CYAN] =(0x00<<16)+(0xAA<<8)+0xAA;
	ansi_color[ANSI_RED] =(0xAA<<16)+(0x00<<8)+0x00;
	ansi_color[ANSI_PURPLE] =(0xAA<<16)+(0x00<<8)+0xAA;
	ansi_color[ANSI_BROWN] =(0xAA<<16)+(0x55<<8)+0x22;
	ansi_color[ANSI_GREY] =(0xAA<<16)+(0xAA<<8)+0xAA;
	ansi_color[ANSI_DARKGREY] =(0x7d<<16)+(0x7d<<8)+0x7d;
	ansi_color[ANSI_BRIGHTBLUE] =(0x00<<16)+(0x00<<8)+0xFF;
	ansi_color[ANSI_BRIGHTGREEN]=(0x00<<16)+(0xFF<<8)+0x00;
	ansi_color[ANSI_BRIGHTCYAN]=(0x00<<16)+(0xFF<<8)+0xFF;
	ansi_color[ANSI_BRIGHTRED]=(0xFF<<16)+(0x7d<<8)+0x7d;
	ansi_color[ANSI_PINK]=(0xFF<<16)+(0x00<<8)+0xff;
	ansi_color[ANSI_YELLOW]=(0xFF<<16)+(0xFF<<8)+0x00;
	ansi_color[ANSI_WHITE]=(0xFF<<16)+(0xFF<<8)+0xFF;

	fprintf(out_f,"%%!PS-Adobe-3.0 EPSF-3.0\n");
	fprintf(out_f,"%%%%Creator: ansi2eps\n");
	fprintf(out_f,"%%%%Title: Blargh\n");
	fprintf(out_f,"%%%%Origin: 0 0\n");
	fprintf(out_f,"%%%%BoundingBox: 0 0 %d %d\n",
		(int)((float)x_size*XFONTSIZE),y_size*YFONTSIZE); 
		//xmin ymin xmax ymax
	fprintf(out_f,"%%%%LanguageLevel: 2\n"); // [could be 1 2 or 3]
	//    fprintf(out_f,"%%%%EOF\n");
	make_pcfont(out_f);

	fprintf(out_f,"/PCFont findfont\n");
	fprintf(out_f,"%d scalefont\n",12);
	fprintf(out_f,"setfont\n");

	/* clear background to black */

	fprintf(out_f,"newpath\n");
	fprintf(out_f,"0 0 moveto\n");
	fprintf(out_f,"0 %d rlineto\n",y_size*YFONTSIZE);
	fprintf(out_f,"%d 0 rlineto\n",(int)((float)x_size*XFONTSIZE));
	fprintf(out_f,"0 -%d rlineto\n",y_size*YFONTSIZE);
	fprintf(out_f,"-%d 0 rlineto\n",(int)((float)x_size*XFONTSIZE));
	fprintf(out_f,"closepath\n");
	fprintf(out_f,"0.0 0.0 0.0 setrgbcolor\n");
	fprintf(out_f,"fill\n");
}

static void int_to_triple(int val,FILE *out_f) {

	float r,g,b;

	r=((float)((val>>16)&0xff))/255.0;
	g=((float)((val>>8)&0xff))/255.0;
	b=((float)((val)&0xff))/255.0;

	fprintf(out_f,"%.2f %.2f %.2f",r,g,b);
}


static void display_eps(FILE *out_f,int output_type,int x_size,int y_size) {

	int x,y,ch,fore,back,yloc,len,old_color,i,old;

	x=0;
	y=0;
	len=0;
	old_color=back_colors[0];

	yloc=(y_size*YFONTSIZE)-(YFONTSIZE*y)-YFONTSIZE;
	fprintf(out_f," %d %d moveto\n",0,yloc);
	while(y<y_size) {

		back=back_colors[x+(y*x_size)];
		if ((back!=old_color) || (x>=x_size)) {
			int_to_triple(old_color,out_f);
			fprintf(out_f," setrgbcolor\n");
			fprintf(out_f,"(");
			for(i=0;i<len;i++) fprintf(out_f,"\\333");
			fprintf(out_f,") show\n");
			old_color=back;
			len=0;
		}
		if (x>=x_size) {
			len=-1;
			x=0;
			y++;
			yloc=(y_size*YFONTSIZE)-(YFONTSIZE*y)-YFONTSIZE;
			fprintf(out_f," %d %d moveto\n",0,yloc);
		}
		else {
			x++;
		}
		len++;
	}
	fprintf(out_f,"%% DONE BACKGROUND\n");

	x=0;
	y=0;
	len=0;
	old=screen[0];
	old_color=fore_colors[0];
	yloc=(y_size*YFONTSIZE)-(YFONTSIZE*y)-YFONTSIZE;
	fprintf(out_f," %d %d moveto\n",0,yloc);

	while(y<y_size) {
		ch=screen[x+(y*x_size)];
		fore=fore_colors[x+(y*x_size)];
		if ((ch!=old) || (fore!=old_color) || (x>=x_size)) {

			int_to_triple(old_color,out_f);
			fprintf(out_f," setrgbcolor\n");
			fprintf(out_f,"(");
			for(i=0;i<len;i++) {
				if (old=='\\') fprintf(out_f,"\\\\");
				else if (old=='(') fprintf(out_f,"\\(");
				else if (old==')') fprintf(out_f,"\\)");
				else if (old<' ') fprintf(out_f,"\\%o",old);
				else if (old>127) fprintf(out_f,"\\%o",old);
				else fprintf(out_f,"%c",old);
			}
			fprintf(out_f,") show\n");
			old_color=fore;
			old=ch;
			len=0;
		}
		if (x>=x_size) {
			len=-1;
			x=0;
			y++;
			yloc=(y_size*YFONTSIZE)-(YFONTSIZE*y)-YFONTSIZE;
			fprintf(out_f," %d %d moveto\n",0,yloc);
		}
		else {
			x++;
		}
		len++;
	}
}


static void display_gd(FILE *out_f,int output_type,int x_size,int y_size) {

	int x,y,xx,yy;

	for(y=0;y<y_size;y++) {
		for(x=0;x<x_size;x++) {
			for(xx=0;xx<8;xx++) {
				for(yy=0;yy<16;yy++) {

	       if (
		    (font_to_use->font_data[(screen[x+(y*x_size)]*16)+yy]) &
	            (128>>xx) ) {
		    gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,
				    fore_colors[x+(y*x_size)]);
	       }
               else {
		    gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,
				    back_colors[x+(y*x_size)]);
	       }
				}
			}
		}
	}

	if (output_type==OUTPUT_PNG) {
		gdImagePng(im, out_f);
	}
	else {
		gdImageGif(im, out_f);
	}
}

static void finish_gd(FILE *out_f) {
	/* Destroy the image in memory. */
	gdImageDestroy(im);
	gdImageDestroy(im2);
	fclose(out_f);
}


static void finish_eps(FILE *out_f) {
	fclose(out_f);
}

static int use_blink=0,invisible=0;
static int fore_color=7,back_color=0;
static int intense=0,fore_color_blink=0;

static void parse_color(char *escape_code, int output_type) {

	int c,temp_color,r,g,b;
	static int already_print_color_warning=0;
	char *pointer;

	pointer=strtok(escape_code,";m");

	do {
		c=atoi(pointer);

		switch(c) {

			/* Normal */
		case 0:
			fore_color=ansi_color[ANSI_GREY];
			back_color=ansi_color[ANSI_BLACK];
			fore_color_blink=0;
			intense=0;
			break;

			/* Intense */
		case 1:
			intense=1;
			if (fore_color==ansi_color[ANSI_BLACK]) {
				fore_color=ansi_color[ANSI_DARKGREY];
			}
			if (fore_color==ansi_color[ANSI_BLUE]) {
				fore_color=ansi_color[ANSI_BRIGHTBLUE];
			}
			if (fore_color==ansi_color[ANSI_GREEN]) {
				fore_color=ansi_color[ANSI_BRIGHTGREEN];
			}
			if (fore_color==ansi_color[ANSI_CYAN]) {
				fore_color=ansi_color[ANSI_BRIGHTCYAN];
			}
			if (fore_color==ansi_color[ANSI_RED]) {
				fore_color=ansi_color[ANSI_BRIGHTRED];
			}
			if (fore_color==ansi_color[ANSI_PURPLE]) {
				fore_color=ansi_color[ANSI_PINK];
			}
			if (fore_color==ansi_color[ANSI_BROWN]) {
				fore_color=ansi_color[ANSI_YELLOW];
			}
			if (fore_color==ansi_color[ANSI_GREY]) {
				fore_color=ansi_color[ANSI_WHITE];
			}
			break;

			/* Underline */
		case 4:
			fprintf(stderr,"Warning!  Underline not supported!\n\n");
			break;

			/* Blink */
		case 5:
			fore_color_blink=1;
			use_blink++;
			break;

			/* Reverse */
		case 7:
			temp_color=fore_color;
			fore_color=back_color;
			back_color=temp_color;
			break;

			/* Invisible */
		case 8:
			invisible=1;
			fprintf(stderr,"Warning! Invisible!\n\n");
			break;

			/* Black Foreground */
		case 30:
			if (intense) fore_color=ansi_color[ANSI_DARKGREY];
			else fore_color=ansi_color[ANSI_BLACK];
			break;

			/* Red Foreground */
		case 31:
			if (intense) fore_color=ansi_color[ANSI_BRIGHTRED];
			else fore_color=ansi_color[ANSI_RED];
			break;

			/* Green Foreground */
		case 32:
			if (intense) fore_color=ansi_color[ANSI_BRIGHTGREEN];
			else fore_color=ansi_color[ANSI_GREEN];
			break;

			/* Yellow Foreground */
		case 33:
			if (intense) fore_color=ansi_color[ANSI_YELLOW];
			else fore_color=ansi_color[ANSI_BROWN];
			break;

			/* Blue Foreground */
		case 34:
			if (intense) fore_color=ansi_color[ANSI_BRIGHTBLUE];
			else fore_color=ansi_color[ANSI_BLUE];
			break;

			/* Purple Foreground */
		case 35:
			if (intense) fore_color=ansi_color[ANSI_PINK];
			else fore_color=ansi_color[ANSI_PURPLE];
			break;

			/* Cyan Foreground */
		case 36:
			if (intense) fore_color=ansi_color[ANSI_BRIGHTCYAN];
			else fore_color=ansi_color[ANSI_CYAN];
			break;

			/* "default" foreground */
			/* implementation defined */
		case 39:
			/* White Foreground */
		case 37:
			if (intense) fore_color=ansi_color[ANSI_WHITE];
			else fore_color=ansi_color[ANSI_GREY];
			break;

			/* "default" background */
			/* implementation defined */
		case 49:
			/* Black Background */
		case 40:
			back_color=ansi_color[ANSI_BLACK];
			break;

			/* Red Background */
		case 41:
			back_color=ansi_color[ANSI_RED];
			break;

			/* Green Background */
		case 42:
			back_color=ansi_color[ANSI_GREEN];
			break;

			/* Yellow Background */
		case 43:
			back_color=ansi_color[ANSI_BROWN];
			break;

			/* Blue Background */
		case 44:
			back_color=ansi_color[ANSI_BLUE];
			break;

			/* Purple Background */
		case 45:
			back_color=ansi_color[ANSI_PURPLE];
			break;

			/* Cyan Background */
		case 46:
			back_color=ansi_color[ANSI_CYAN];
			break;

			/* White Background */
		case 47:
			back_color=ansi_color[ANSI_GREY];
			break;

			/* Extended color support -- foreground */
		case 38:

			pointer=strtok(NULL,";m");
			if (pointer==NULL) break;
			c=atoi(pointer);

			/* 24-bit color */
			if (c==2) {
				/* FIXME: loop until get NULL? */
				pointer=strtok(NULL,";m");
				if (pointer==NULL) break;
				r=atoi(pointer);

				pointer=strtok(NULL,";m");
				if (pointer==NULL) break;
				g=atoi(pointer);

				pointer=strtok(NULL,";m");
				if (pointer==NULL) break;
				b=atoi(pointer);

				if (!allocated_256colors) {
					setup_256colors(output_type);
				}

				fore_color=map_24bitcolor(output_type,r,g,b);

				break;
			}
			else if (c==5) {
				/* FIXME: loop until get NULL? */
				pointer=strtok(NULL,";m");
				if (pointer==NULL) break;
				c=atoi(pointer);

				if ((c>=16) && (!allocated_256colors)) {
					setup_256colors(output_type);
				}

				if (c>255) {
					if (!already_print_color_warning) {
						fprintf(stderr,"Warning!  Color %d out of range!\n",c);
						already_print_color_warning=1;
					}
					break;
				}

				fore_color=ansi_color[c];
			}
			else {
				fprintf(stderr,"Warning!  Unknown extended color format!\n");
			}
			break;

			/* Extended color support -- background */
		case 48:

			pointer=strtok(NULL,";m");
			if (pointer==NULL) break;
			c=atoi(pointer);
			if (c==2) {
				/* FIXME: loop until get NULL? */
				pointer=strtok(NULL,";m");
				if (pointer==NULL) break;
				r=atoi(pointer);

				pointer=strtok(NULL,";m");
				if (pointer==NULL) break;
				g=atoi(pointer);

				pointer=strtok(NULL,";m");
				if (pointer==NULL) break;
				b=atoi(pointer);

				if (!allocated_256colors) {
					setup_256colors(output_type);
				}

				back_color=map_24bitcolor(output_type,r,g,b);

				break;
			}
			else if (c==5) {
				/* FIXME: loop until get NULL? */
				pointer=strtok(NULL,";m");
				if (pointer==NULL) break;
				c=atoi(pointer);

				if ((c>=16) && (!allocated_256colors)) {
					setup_256colors(output_type);
				}

				if (c>255) {
					if (!already_print_color_warning) {
						fprintf(stderr,"Warning!  Color %d out of range!\n",c);
						already_print_color_warning=1;
					}
					break;
				}

				back_color=ansi_color[c];
			}
			else {
				fprintf(stderr,"Warning!  Unknown extended color format!\n");
			}
			break;

		default:
			fprintf(stderr,"Warning! Invalid Color %d!\n\n",c);
		}

		pointer=strtok(NULL,";m");
	} while(pointer);
}


static void gif_the_text(int animate, int blink,
			int frame_per_refresh, int create_movie,
			FILE *in_f,FILE *out_f, int time_delay,int x_size,
			int y_size, int output_type) {

	FILE *animate_f=NULL;
	unsigned char temp_char;


	char escape_code[BUFSIZ];
	char temp_file_name[BUFSIZ];

	int x_position,y_position,oldx_position,oldy_position;
	int x,y,emergency_exit=0;
	int escape_counter,n,n2,xx,yy,i;

	int backtrack=0;

	int movie_frame=0;

	screen=calloc(x_size*y_size,sizeof(unsigned char));
	attributes=calloc(x_size*y_size,sizeof(unsigned char));
	fore_colors=calloc(x_size*y_size,sizeof(unsigned int));
	back_colors=calloc(x_size*y_size,sizeof(unsigned int));

	if (output_type==OUTPUT_EPS) {
		setup_eps(out_f,x_size,y_size);
	}
	else {
		setup_gd(out_f,x_size,y_size);
	}

	if ((animate||blink) && (output_type!=OUTPUT_GIF)) {
		fprintf(stderr,"Error!  Can only animate if output is gif!\n");
		exit(-1);
	}

	if (animate) {
		/* FIXME: use mkstemp()? */
		if (create_movie) {
			sprintf(temp_file_name,"/tmp/ansi2gif_%i_%08d.png",
					getpid(),movie_frame);
			movie_frame++;
		} else {
			sprintf(temp_file_name,"/tmp/ansi2gif_%i.gif",
					getpid());
		}

		if ( (animate_f=fopen(temp_file_name,"wb"))==NULL) {
			fprintf(stderr,"Error!  Cannot open file %s to store temporary "
				"animation info!\n\n",temp_file_name);
			exit(1);
		}

		/* Clear Screen */
		gdImageRectangle(im,0,0,x_size*8,y_size*16,ansi_color[0]);

		gdImageGif(im, animate_f);
		fclose(animate_f);

		animate_gif(out_f,temp_file_name,1,0,0,time_delay,0);
	}

	/* Clear the memory used to store the image */
	for(y=0;y<y_size;y++) {
		for(x=0;x<x_size;x++) {
			screen[x+(y*x_size)]=' ';
			fore_colors[x+(y*x_size)]=0;
			back_colors[x+(y*x_size)]=0;
		}
	}

	/* Initialize the Variables */
	x_position=1; y_position=1;
	oldx_position=1; oldy_position=1;

	/* FIXME: fgetc()? */
	while (  ((fread(&temp_char,sizeof(temp_char),1,in_f))>0)
		&& (!emergency_exit) ) {

		/* Did somebody say escape?? */
		if (temp_char==27) {
			fread(&temp_char,sizeof(temp_char),1,in_f);
			/* If after escape we have '[' we have an escape code */
			if (temp_char!='[') fprintf(stderr,"False Escape\n");
			else {
				escape_counter=0;

				/* Read in the command */
				while (!isalpha(temp_char)) {
					fread(&temp_char,sizeof(temp_char),1,in_f);
					escape_code[escape_counter]=temp_char;
					escape_counter++;
				}

				escape_code[escape_counter]='\000';
				/* Big 'ol switch statement to figure out what to do */
				switch(escape_code[escape_counter-1]) {

				/* Move to x,y */
				case 'H':
				case 'f':
					n=parse_numbers(escape_code,1);
					n2=parse_numbers(escape_code,2);
					if ( (y_position>n) ||
						((x_position>n2)&&(y_position>=n))) backtrack++;
					y_position=n;
					x_position=n2;
					break;

				/* Decrement Y by N */
				case 'A':
					n=parse_numbers(escape_code,1);
					backtrack++;
					y_position-=n;
					break;

				/* Increment Y by N */
				case 'B':
					n=parse_numbers(escape_code,1);
					y_position+=n;
					break;

				/* Increment X by N */
				case 'C':
					n=parse_numbers(escape_code,1);
					x_position+=n;
					break;

				/* Decrement X by N */
				case 'D':
					n=parse_numbers(escape_code,1);
					if (n!=255) {
						backtrack++;
						x_position-=n;
						if (x_position<0) x_position=0;
					}
					break;

				/* Report Current Position */
				case 'R':
					fprintf(stderr,"Current Position: %d, %d\n",
						x_position,y_position);
					break;

				/* Save Position */
				case 's':
					oldx_position=x_position;
					oldy_position=y_position;
					break;

				/* Restore Position */
				case 'u':
					x_position=oldx_position;
					y_position=oldy_position;
					break;

				/* Clear Screen and Home */
				case 'J':
					for (x=0;x<x_size;x++)
						for (y=0;y<y_size;y++)
							screen[x+(y*x_size)]=' ';
					x_position=1;
					y_position=1;
					if (animate) {
						if ( (animate_f=fopen(temp_file_name,"wb"))==NULL) {
							fprintf(stderr,"Error!  Cannot open file %s to store temporary animation info.\n",
								temp_file_name);
							exit(1);
						}
						gdImageGif(im, animate_f);
						fclose(animate_f);
						animate_gif(out_f,temp_file_name,0,0,0,time_delay,0);
					}
					break;

				/* Clear to end of line */
				case 'K':
					if (animate) {
						if ( (animate_f=fopen(temp_file_name,"wb"))==NULL) {
							fprintf(stderr,"Error!  Cannot open file %s to store temporary animation info.\n",
								temp_file_name);
							exit(1);
						}
						im3=gdImageCreateTrueColor((x_size-(x_position-1))*8,16);
						gdImageRectangle(im3,0,0,(x_size-(x_position-1))*8,16,gdImageColorAllocate(im3,0x00,0x00,0x00));

						gdImageGif(im3, animate_f);
						fclose(animate_f);
						gdImageDestroy(im3);
						animate_gif(out_f,temp_file_name,0,(x_position-1)*8,(y_position-1)*16,time_delay,0);  
					}
					if (y_position<y_size)
						for(x=x_position;x<x_size;x++)
							screen[x+(y_position*x_size)]=' ';
					x_position=x_size;
					break;

				/* Oh what fun, figuring out colors */
				case 'm':
					parse_color(escape_code,output_type);
					break;

				/* Set screen mode */
				case 'h':
					fprintf(stderr,"Warning!  Screen Mode Setting not Supported.\n\n"); 
					break;

				/* note, look for [= code */
 				case 'p':
					fprintf(stderr,"Warning! Keyboard Reassign not Supported.\n");
					break;

				default:
					fprintf(stderr,"Warning! Unknown Escape Code\n");
					break;
				}
			}
		}
		/* If it isn't an escape code, we do this */
		else {
			/* Line Feed */
			if (temp_char=='\n') {
				x_position=1;
				y_position++;
			}
			/* Tab */
			else if (temp_char=='\t') {
				x_position+=4;
			}
			/* Skip carriage returns, as most */
			/* ANSIs are from DOS            */
			else if (temp_char=='\r');
			else {
				/* Where is the best place to check for wrapping? */
				if (x_position>x_size) {
					x_position=x_position%x_size;
					y_position++;
				}

				if (animate && y_position<=y_size) {  /* Animate it if we have the right */
					for(xx=0;xx<8;xx++) {
						for(yy=0;yy<16;yy++) {
							if ( ((unsigned char) (font_to_use->font_data[(temp_char*16)+yy])) &(128>>xx) )
								if ((frame_per_refresh) || (create_movie)) 
									gdImageSetPixel(im,xx+(x_position-1)*8,yy+(y_position-1)*16,fore_color);
								else 
									gdImageSetPixel(im2,xx,yy,fore_color);
							else
								if ((frame_per_refresh) || (create_movie))
									gdImageSetPixel(im,xx+(x_position-1)*8,yy+(y_position-1)*16,back_color);
								else
									gdImageSetPixel(im2,xx,yy,back_color);
						}
					}
					if (!frame_per_refresh) {
						if (create_movie) {
							sprintf(temp_file_name,"/tmp/ansi2gif_%i_%08d.png",getpid(),movie_frame);
							movie_frame++;
							animate_f=fopen(temp_file_name,"wb");
							gdImagePng(im, animate_f);
							fclose(animate_f);
						}
						else {
							animate_f=fopen(temp_file_name,"wb");
							gdImageGif(im2, animate_f);
							fclose(animate_f);
							animate_gif(out_f,temp_file_name,0,(x_position-1)*8,
									(y_position-1)*16,time_delay,0);
						}
					}
				}
				if (y_position<=y_size) {
					screen[(x_position-1)+((y_position-1)*x_size)]=temp_char;
					if (!invisible) {
						fore_colors[(x_position-1)+((y_position-1)*x_size)]=fore_color;
						back_colors[(x_position-1)+((y_position-1)*x_size)]=back_color;
						if (fore_color_blink) {
							attributes[(x_position-1)+((y_position-1)*x_size)]|=BLINK;
						}
					}
					x_position++;
				}
			}

			/* See if the screen has wrapped */
			if (x_position>x_size) {
				x_position=x_position%x_size;
				y_position++;
			}
			if (y_position>y_size) {
				emergency_exit=1;
				fprintf(stderr,"Error!  Scrolled past maximum y_size of %i!\n\n",y_size);
			}
		}
	}

	/* If not animating, draw the final picture */
	if (!(animate||blink)) {

		if (output_type==OUTPUT_EPS) {
			display_eps(out_f,output_type,x_size,y_size);
		}
		else {
			display_gd(out_f,output_type,x_size,y_size);
		}
	}


	/* If blinking... */

	/* something needs to be done about this nesting */
	if ((!animate) && (blink)) {

		for(i=0;i<2;i++) {
			for(y=0;y<y_size;y++) {
				for(x=0;x<x_size;x++) {
					for(xx=0;xx<8;xx++) {
						for(yy=0;yy<16;yy++) {
							if ( ((unsigned char) (font_to_use->font_data[(screen[x+(y*x_size)]*16)+yy])) & (128>>xx) ) {
								if ((attributes[x+(y*x_size)]&BLINK)) {
									if (i) {
										gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,fore_colors[x+(y*x_size)]);
									}
									else {
										gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,back_colors[x+(y*x_size)]);
									}
								}
								else {
									gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,fore_colors[x+(y*x_size)]);
								}
							}
							else {
								gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,back_colors[x+(y*x_size)]);
							}
						}
					}
				}
			}

			if (create_movie) {
				sprintf(temp_file_name,
					"/tmp/ansi2gif_%i.png",getpid());
			} else {
				sprintf(temp_file_name,
					"/tmp/ansi2gif_%i_%08d.gif",
					getpid(),movie_frame);
				movie_frame++;
			}
			animate_f=fopen(temp_file_name,"wb");
			gdImageGif(im, animate_f);
			fclose(animate_f);
			animate_gif(out_f,temp_file_name,(1-i),0,0,time_delay,1);
		}
		fputc(';',out_f);

	}

	if (animate) {
		fputc(';', out_f); /* End of Gif file */
	}

	if ((backtrack) && !(animate)) {
		fprintf(stderr,"Warning!  The cursor moved backwards and animated output was not selected.\n"
			       "          For proper output, you might want to try again with --animate\n\n"); 
	}

	if ((use_blink)&&(!blink)) {
		fprintf(stderr,"Warning!  A blinking color code was used.  To display blinking ansis you\n"
			       "          to run with the --blink option to create an animated gif.\n\n");
	}

	/* Finish movie with 30 copies of the last frame */
	/* So the movie doesn't just end */
	if (create_movie) {
		for(i=0;i<30;i++) {
			sprintf(temp_file_name,
				"/tmp/ansi2gif_%i_%08d.png",
				getpid(),movie_frame);
			movie_frame++;
			animate_f=fopen(temp_file_name,"wb");
			gdImagePng(im, animate_f);
			fclose(animate_f);
		}
	}




	if (output_type==OUTPUT_EPS) {
		finish_eps(out_f);
	}
	else {
		finish_gd(out_f);
	}

	if (!create_movie) {
		unlink(temp_file_name);
	}
}


static int detect_max_y(int animate,int blink,
			FILE *in_f,FILE *out_f,
		        int time_delay,int x_size,int y_size,
			int output_type) {

	unsigned char temp_char;
	char escape_code[BUFSIZ];
	int x_position,y_position,oldx_position,oldy_position;
	int escape_counter,n,n2;
	int backtrack=0;
	int max_y=y_size;

	/* Initialize the Variables */
	x_position=1; y_position=1;
	oldx_position=1; oldy_position=1;

	while (  ((fread(&temp_char,sizeof(temp_char),1,in_f))>0) ) {

		/* Did somebody say escape?? */
		if (temp_char==27) {
			fread(&temp_char,sizeof(temp_char),1,in_f);
			/* If after escape we have '[' we have an escape code */
			if (temp_char!='[') fprintf(stderr,"False Escape\n");
			else {
				escape_counter=0;
				/* Read in the command */
				while (!isalpha(temp_char)) {
					fread(&temp_char,sizeof(temp_char),1,in_f);
					escape_code[escape_counter]=temp_char;
					escape_counter++;
				}
				escape_code[escape_counter]='\000';
				/* Big 'ol switch statement to figure out what to do */
				switch(escape_code[escape_counter-1]) {

				/* Move to x,y */
				case 'H':
				case 'f':
					n=parse_numbers(escape_code,1);
					n2=parse_numbers(escape_code,2);
					if ( (y_position>n) ||
						((x_position>n2)&&(y_position>=n))) backtrack++;
					y_position=n;
					x_position=n2;
					break;

				/* Decrement Y by N */
				case 'A':
					n=parse_numbers(escape_code,1);
					backtrack++;
					y_position-=n;
					break;

				/* Increment Y by N */
				case 'B':
					n=parse_numbers(escape_code,1);
					y_position+=n;
					break;

				/* Increment X by N */
				case 'C':
					n=parse_numbers(escape_code,1);
					x_position+=n;
					break;

				/* Decrement X by N */
				case 'D':
					n=parse_numbers(escape_code,1);
					if (n!=255) {
						backtrack++;
						x_position-=n;
						if (x_position<0) x_position=0;
					}
					break;

				/* Report Current Position */
				case 'R':
					fprintf(stderr,"Current Position: %d, %d\n",
						x_position,y_position);
					break;

				/* Save Position */
				case 's':
					oldx_position=x_position;
					oldy_position=y_position;
					break;

				/* Restore Position */
				case 'u':
					x_position=oldx_position;
					y_position=oldy_position;
					break;

				/* Clear Screen and Home */
				case 'J':
					x_position=1;
					y_position=1;
					break;

				/* Clear to end of line */
				case 'K':
					x_position=x_size;
					break;

				/* normal, we ignore some cases */
				default:
					break;
				}
			}
		}
		/* If it isn't an escape code, we do this */
		else {
			if (temp_char=='\n') { /* Line Feed */
				x_position=1;
				y_position++;
			}
			else if (temp_char=='\t') { /* Tab */
				x_position+=4;
			}
			else if (temp_char=='\r');	/* Skip carriage returns, as most */
							/* ansi's are from DOS            */
			else {
				/* Where is the best place to check for wrapping? */
				if (x_position>x_size) {
					x_position=x_position%x_size;
					y_position++;
				}

				// if (y_position<=y_size) {
				x_position++;
				// }
			}

			/* See if the screen has wrapped */
			if (x_position>x_size) {
				x_position=x_position%x_size;
				y_position++;
			}
			if (y_position>max_y) {
				max_y=y_position;
			}
		}
	}

	fprintf(stderr,"Found maximum y of %d\n",y_position);

	return max_y;
}


static void display_help(char *name_run_as, int just_version) {

	int i;

	printf("\nansi2gif v %s by Vince Weaver <vince _at_ deater.net>\n\n",
		VERSION);

	if (!just_version) {
		printf(" %s [--animate] [--blink] [--eps] [--font fontfile]\n",
			name_run_as);
		for(i=0;i<strlen(name_run_as);i++) putchar(' ');
		printf("  [--gif] [--help] [--png] [--version] [--timedelay T]\n");
		for(i=0;i<strlen(name_run_as);i++) putchar(' ');
		printf("  [--xsize X] [--ysize Y] input_file output_file\n\n");
		printf("   --animate          : Create an animated gif if an animated ansi\n");
		printf("   --frameperrefresh  : Create a animation frame every screen refresh instead for every character\n");
		printf("   --movie            : Make a series of frames, don't delete (for movies)\n");
		printf("   --blink            : Create an animated gif enabling blinking\n");
		printf("   --eps              : Output an Encapsulated Postscript\n");
		// printf("   --color X=0xRRGGBB : Set color \"X\" [0-16] to hex value RRGGBB [a number]\n");
		printf("   --font fontfile    : Use vgafont \"filename\" to create gif\n"); 
		printf("   --gif              : output a GIF file\n");
		printf("   --help             : show this help\n");
		printf("   --png              : output a PNG file\n");
		printf("   --timedelay T      : Delay T 100ths of seconds between each displayed\n"
		       "                        character in animate mode.\n");
		printf("   --version          : Print version information\n");
		printf("   --xsize X          : Make the output X characters wide\n");
		printf("   --ysize Y          : Make the output Y characters long.\n"
		       "                        use \"auto\" to figure out on the fly.\n");
		printf("\n");
		printf("Instead of the long option, a single dash and the first letter of the\n"
		       "option may be substituted.  That is, \"-a\" instead of \"--animate\"\n\n");
	}
	exit(0);
}

	/* Load VGA font... Used in my game TB1 */
	/* psf font support added by <bkbratko _at_ ardu.raaf.defence.gov.au> */
static vga_font *load_vga_font(char *namest,int xsize,int ysize,int numchars) {

	unsigned char buff[16];
	FILE *f;
	int i,fonty,numloop;
	vga_font *font;

	unsigned char *data;

	short int psf_id;
	char psf_mode;
	char psf_height;

	font=(vga_font *)malloc(sizeof(vga_font));
	data=calloc(numchars*ysize,(sizeof(unsigned char)));

	f=fopen(namest,"r");
	if (f==NULL) {
		fprintf(stderr,"\nERROR loading font file %s.\n\n",namest);
		return NULL;
	}

	fread(&psf_id,sizeof(psf_id),1,f);
	/* psf files contain a magic number 0x0436 in the first word */
	if ( 0==strncmp(".psf",namest+strlen(namest)-4,4) ) {
		if (psf_id!=0x436 ) {
			fprintf(stderr,"ERROR file %s is not a psf file \n",namest);
			return NULL;
		}
	}

	/* the next two bytes of psf file contain the mode and height
	 * mode 0 is for 256 character fonts, which can be used by fontprint
	 * only height = 16 is suitable for fontprint v3.0.x */
	fread(&psf_mode,sizeof(psf_mode),1,f);
	fread(&psf_height,sizeof(psf_height),1,f);
	if (psf_id==0x436 && (psf_mode!=0 || psf_height!=16 )) {
		fprintf(stderr,"ERROR unable to deal with this size of psf file \n");
		return NULL;
	}

	/* if control reaches this point and the font is not a psf file
	 * then we must rewind the file in order to recover the first
	 * four bytes */
	if (psf_id!=0x436) rewind(f);

	numloop=(numchars*ysize);
	font->width=xsize;
	font->height=ysize;
	font->numchars=numchars;
	font->font_data=data;
	fonty=0;
	while ( (!feof(f))&&(fonty<numloop)) {
		fread(buff,1,16,f);
		for(i=0;i<16;i++) font->font_data[fonty+i]=buff[i];
		fonty+=16;
	}
	fclose(f);
	return font;
}

	/****************/
	/* MAIN PROGRAM */
	/****************/

int main(int argc, char **argv) {

	FILE *input_f,*output_f;
	int c;

	int time_delay=DEFAULT_TIMEDELAY;
	int x_size=DEFAULT_XSIZE,y_size=DEFAULT_YSIZE;
	int animate=0,blink=0;
	int frame_per_refresh=0,create_movie=0;
	char *font_name=NULL,*input_name=NULL,*output_name=NULL;
	char *endptr;
	int option_index = 0;
	int output_type=OUTPUT_GIF;
	int font_supplied=0;
	int auto_ysize=0;

	static struct option long_options[] = {
		{"animate", 0, NULL, 'a'},
		{"blink", 0, NULL, 'b'},
		{"color", 1, NULL, 'c'},
		{"eps", 0, NULL, 'e'},
		{"font", 1, NULL, 'f'},
		{"frameperrefresh", 0, NULL, 'r'},
		{"gif", 0, NULL, 'g'},
		{"help", 0, NULL, 'h'},
		{"movie", 0, NULL, 'm'},
		{"output",1,NULL,'o'},
		{"png",0,NULL,'p'},
		{"timedelay",1,NULL,'t'},
		{"version",0,NULL,'v'},
		{"xsize",1,NULL,'x'},
		{"ysize",1,NULL,'y'},
		{0,0,0,0}
	};


//	fprintf(stderr,"Run as %s\n",argv[0]);

	/* Check to see how run. if we were ansi2png or ansi2eps set */
	/* default output appropriately                              */
	if (strstr(argv[0],"eps")) output_type=OUTPUT_EPS;
	if (strstr(argv[0],"png")) output_type=OUTPUT_PNG;

	/*--  PARSE COMMAND LINE PARAMATERS --*/
	opterr=0;
	while ((c = getopt_long (argc, argv,
			"abc:ref:ghpt:vx:y:",
			long_options,&option_index))!=-1) {
		switch (c) {
		case 'a':	animate=1;
				break;
		case 'b':	blink=1;
				break;
		case 'c':	fprintf (stderr,"\nWarning! Setting alternate "
					"colors not implemented yet.\n\n");
				break;
		case 'e':	output_type=OUTPUT_EPS;
				break;
		case 'f':	font_supplied=1;
				font_name=strdup(optarg);
				break;
		case 'g':	output_type=OUTPUT_GIF;
				break;
		case 'h':	display_help(argv[0],0);
				break;
		case 'm':	create_movie=1;
				output_type=OUTPUT_PNG;
				break;
		case 'p':	output_type=OUTPUT_PNG;
				break;
		case 'r':	frame_per_refresh=1;
				break;
		case 't':	time_delay=strtol(optarg,&endptr,10);
				if ( endptr == optarg ) {
					fprintf(stderr,"\nError! \"%s\" is an invalid time delay.\n"
							"\tPlease select a delay that is an integer "
							"number of 1/100 of seconds.\n\n",optarg);
					exit(9);
				}
				fprintf(stderr,"\nTime Delay in Animation %f seconds\n",
						((float)time_delay/100));
				break;
		case 'v':	display_help(argv[0],1);
				break;
		case 'x':	x_size=strtol(optarg,&endptr,10);
				if ( endptr == optarg ) {
					fprintf(stderr,"\nError!  \"%s\" is not a valid x size.\n\n",
						optarg);
					exit(9);
				}
				break;
		case 'y':	if (!strcmp(optarg,"auto")) {
					auto_ysize=1;
				}
				else {
					y_size=strtol(optarg,&endptr,10);
					if ( endptr == optarg ) {
						fprintf(stderr,"\nError!  \"%s\" is not a valid y size.\n\n",
							optarg);
						exit(9);
					}
				}
				break;
		default :	fprintf(stderr,"\nError! Bad command line option!\n\n"); 
				exit(5);
		}
	}

	if (animate && blink) {
		fprintf(stderr,"Error!  Cannot do blink and animate simultaneously!\n\n");
		exit(1);
	}

	if (blink) {
		time_delay=25;
	}

	if (optind<argc) {
		input_name=strdup(argv[optind]);
		if ( (input_f=fopen(input_name,"r"))==NULL) {
			fprintf(stderr,"\nInvalid Input File: %s\n",input_name);
			return 1;
		}
	}
	else {
		fprintf(stderr,"Using standard input...\n");
		input_f=stdin;

		if (auto_ysize) {
			fprintf(stderr,"Error!  auto ysize not supported when using stdin\n");
			return 1;
		}
	}

	if (optind<argc-1) {
		output_name=strdup(argv[optind+1]);
		if ( (output_f=fopen(output_name,"wb"))==NULL){
			fprintf(stderr,"\nInvalid Output File: %s\n",output_name);
			return 1;
		}

		/* Autodetect output filetype */
		if (strlen(output_name)>4) {
			if (!strcmp(output_name+(strlen(output_name)-4),".png")) {
				output_type=OUTPUT_PNG;
			}
			if (!strcmp(output_name+(strlen(output_name)-4),".eps")) {
				output_type=OUTPUT_EPS;
			}
			if (!strcmp(output_name+(strlen(output_name)-4),".gif")) {
				output_type=OUTPUT_GIF;
			}

		}
	}
	else {
		fprintf(stderr,"Using standard output...\n" );
		output_f=stdout;
	}

	if (font_supplied) {
		font_to_use=load_vga_font(font_name,8,16,256);
		if (font_to_use==NULL) exit(7);
	}
	else {
		font_to_use=(vga_font *)malloc(sizeof(vga_font));
		font_to_use->font_data=(unsigned char *)&default_font;
		font_to_use->width=8;
		font_to_use->height=16;
		font_to_use->numchars=256;
	}

	if (auto_ysize) {
		y_size=detect_max_y(animate,blink,input_f,output_f,
			time_delay,x_size,y_size,output_type);
		rewind(input_f);
	}

	gif_the_text(animate,blink,
			frame_per_refresh,create_movie,input_f,output_f,
			time_delay,x_size,y_size,output_type);

	fclose(input_f);

	return 0;
}
