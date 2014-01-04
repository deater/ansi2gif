/*             ----------- ansi2gif.c ------------------       *\
\*   by Vince Weaver... Makes a gif out of "ANSI" files        */
/*   Based on my "fontprint" program.                          *\
\* http://www.deater.net/weave/vmwprod/ansi2gif/               */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>

#include <gd.h>

#include "default_font.h"
#include "pcfont.h"

#define DEFAULT_TIMEDELAY 1 /* 1/100 of a second */
#define DEFAULT_XSIZE    80
#define DEFAULT_YSIZE    25

#define XFONTSIZE 7.1
#define YFONTSIZE 13

#define VERSION "0.9.15"

#define OUTPUT_PNG 0
#define OUTPUT_GIF 1
#define OUTPUT_EPS 2
#define OUTPUT_MPG 3

typedef struct {
	unsigned char *font_data;
	int width;
	int height;
	int numchars;
} vga_font;

	/* Located in the "whirlgif.c" file */
void animate_gif(FILE *fout,char *fname,int firstImage,int Xoff,int Yoff,
	int delay_time,int loop_val); /* time in 100th of seconds */


	/* If index=0, return the number of numbers */
static int parse_numbers(unsigned char *string,int index) {

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


/*
 blink r g b  i r g b
 */

#define BLINK     128
#define BACK_RED   64
#define BACK_GREEN 32
#define BACK_BLUE  16
#define INTENSE     8
#define FORE_RED    4
#define FORE_GREEN  2
#define FORE_BLUE   1
#define BLACK       0
#define DEFAULT     7

#define FORE_CLEAR 0xf8
#define BACK_CLEAR 0x8f

static int colorF[16];
static int colorC[16];

static gdImagePtr im,im2,im3;
static vga_font *font_to_use=NULL;
static unsigned char *screen;
static unsigned char *attributes;

static void setup_gd(FILE *out_f,int x_size,int y_size) {

	im = gdImageCreate(x_size*8,y_size*16);  /* Full Screen */
	im2 = gdImageCreate(8,16);               /* One Character */

	/* Setup the Colors to Use for fullscreen */
	colorF[0] =gdImageColorAllocate(im,0x00,0x00,0x00);
	colorF[1] =gdImageColorAllocate(im,0x00,0x00,0xAA);
	colorF[2] =gdImageColorAllocate(im,0x00,0xAA,0x00);
	colorF[3] =gdImageColorAllocate(im,0x00,0xAA,0xAA);
	colorF[4] =gdImageColorAllocate(im,0xAA,0x00,0x00);
	colorF[5] =gdImageColorAllocate(im,0xAA,0x00,0xAA);
	colorF[6] =gdImageColorAllocate(im,0xAA,0x55,0x22);
	colorF[7] =gdImageColorAllocate(im,0xAA,0xAA,0xAA);
	colorF[8] =gdImageColorAllocate(im,0x7d,0x7d,0x7d);
	colorF[9] =gdImageColorAllocate(im,0x00,0x00,0xFF);
	colorF[10]=gdImageColorAllocate(im,0x00,0xFF,0x00);
	colorF[11]=gdImageColorAllocate(im,0x00,0xFF,0xFF);
	colorF[12]=gdImageColorAllocate(im,0xFF,0x7d,0x7d);
	colorF[13]=gdImageColorAllocate(im,0xFF,0x00,0xff);
	colorF[14]=gdImageColorAllocate(im,0xFF,0xFF,0x00);
	colorF[15]=gdImageColorAllocate(im,0xFF,0xFF,0xFF);

	/* Setup the Colors to Use for character.  Is this needed? */
	colorC[0] =gdImageColorAllocate(im2,0x00,0x00,0x00);
	colorC[1] =gdImageColorAllocate(im2,0x00,0x00,0xAA);
	colorC[2] =gdImageColorAllocate(im2,0x00,0xAA,0x00);
	colorC[3] =gdImageColorAllocate(im2,0x00,0xAA,0xAA);
	colorC[4] =gdImageColorAllocate(im2,0xAA,0x00,0x00);
	colorC[5] =gdImageColorAllocate(im2,0xAA,0x00,0xAA);
	colorC[6] =gdImageColorAllocate(im2,0xAA,0x55,0x22);
	colorC[7] =gdImageColorAllocate(im2,0xAA,0xAA,0xAA);
	colorC[8] =gdImageColorAllocate(im2,0x7d,0x7d,0x7d);
	colorC[9] =gdImageColorAllocate(im2,0x00,0x00,0xFF);
	colorC[10]=gdImageColorAllocate(im2,0x00,0xFF,0x00);
	colorC[11]=gdImageColorAllocate(im2,0x00,0xFF,0xFF);
	colorC[12]=gdImageColorAllocate(im2,0xFF,0x7d,0x7d);
	colorC[13]=gdImageColorAllocate(im2,0xFF,0x00,0xFF);
	colorC[14]=gdImageColorAllocate(im2,0xFF,0xFF,0x00);
	colorC[15]=gdImageColorAllocate(im2,0xFF,0xFF,0xFF);
}



static void setup_eps(FILE *out_f,int x_size, int y_size) {

	colorF[0] =(0x00<<16)+(0x00<<8)+0x00;
	colorF[1] =(0x00<<16)+(0x00<<8)+0xAA;
	colorF[2] =(0x00<<16)+(0xAA<<8)+0x00;
	colorF[3] =(0x00<<16)+(0xAA<<8)+0xAA;
	colorF[4] =(0xAA<<16)+(0x00<<8)+0x00;
	colorF[5] =(0xAA<<16)+(0x00<<8)+0xAA;
	colorF[6] =(0xAA<<16)+(0x55<<8)+0x22;
	colorF[7] =(0xAA<<16)+(0xAA<<8)+0xAA;
	colorF[8] =(0x7d<<16)+(0x7d<<8)+0x7d;
	colorF[9] =(0x00<<16)+(0x00<<8)+0xFF;
	colorF[10]=(0x00<<16)+(0xFF<<8)+0x00;
	colorF[11]=(0x00<<16)+(0xFF<<8)+0xFF;
	colorF[12]=(0xFF<<16)+(0x7d<<8)+0x7d;
	colorF[13]=(0xFF<<16)+(0x00<<8)+0xff;
	colorF[14]=(0xFF<<16)+(0xFF<<8)+0x00;
	colorF[15]=(0xFF<<16)+(0xFF<<8)+0xFF;

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
	old_color=colorF[(attributes[0]&0xf0)>>4];

	yloc=(y_size*YFONTSIZE)-(YFONTSIZE*y)-YFONTSIZE;
	fprintf(out_f," %d %d moveto\n",0,yloc);
	while(y<y_size) {

		back=colorF[ (attributes[x+(y*x_size)]&0xf0)>>4];
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
	old_color=colorF[attributes[0]&0xf];
	yloc=(y_size*YFONTSIZE)-(YFONTSIZE*y)-YFONTSIZE;
	fprintf(out_f," %d %d moveto\n",0,yloc);   

	while(y<y_size) {
		ch=screen[x+(y*x_size)];
		fore=colorF[attributes[x+(y*x_size)]&0xf];
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
				    colorF[attributes[x+(y*x_size)]&0x0f]);
	       }
               else {
		    gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,
				    colorF[(attributes[x+(y*x_size)]&0xf0)>>4]);
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


static void gif_the_text(int animate,int blink,
			FILE *in_f,FILE *out_f,
		        int time_delay,int x_size,int y_size,
			int output_type) {
   
    FILE *animate_f=NULL;
    unsigned char temp_char;


    unsigned char escape_code[BUFSIZ];
    char temp_file_name[BUFSIZ];
      
    int x_position,y_position,oldx_position,oldy_position;
    int color=DEFAULT,x,y,emergency_exit=0,i;
    int escape_counter,n,n2,c,invisible=0,xx,yy;

    int backtrack=0,use_blink=0;
   
    screen=(unsigned char *)calloc(x_size*y_size,sizeof(unsigned char));
    attributes=(unsigned char *)calloc(x_size*y_size,sizeof(unsigned char));
   
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
       sprintf(temp_file_name,"/tmp/ansi2gif_%i.gif",getpid());
         
       if ( (animate_f=fopen(temp_file_name,"wb"))==NULL) {
	  fprintf(stderr,"Error!  Cannot open file %s to store temporary "
		 "animation info!\n\n",temp_file_name);
	  exit(1);
       }
          /* Clear Screen */
       gdImageRectangle(im,0,0,x_size*8,y_size*16,colorF[0]);
			     
       gdImageGif(im, animate_f);
       fclose(animate_f);
       
       animate_gif(out_f,temp_file_name,1,0,0,time_delay,0); 
    }
    
       /* Clear the memory used to store the image */
    for(y=0;y<y_size;y++) {   
       for(x=0;x<x_size;x++) {	
          screen[x+(y*x_size)]=' ';
	  attributes[x+(y*x_size)]=BLACK;
       }    
    }
   
   
       /* Initialize the Variables */
    x_position=1; y_position=1;
    oldx_position=1; oldy_position=1;
    
    while (  ((fread(&temp_char,sizeof(temp_char),1,in_f))>0) 
	     && (!emergency_exit) ) {

          /* Did somebody say escape?? */
       if (temp_char==27) {
	  fread(&temp_char,sizeof(temp_char),1,in_f);
	     /* If after escape we have '[' we have an escape code */
	  if (temp_char!='[') fprintf(stderr,"False Escape\n");
	  else {
	     escape_counter=0;
	     while (!isalpha(temp_char)) { /* Read in the command */
	        fread(&temp_char,sizeof(temp_char),1,in_f);
	        escape_code[escape_counter]=temp_char;
	        escape_counter++;
	     }
	     escape_code[escape_counter]='\000';
	        /* Big 'ol switch statement to figure out what to do */
	     switch(escape_code[escape_counter-1]) {
	        /* Move to x,y */
	     case 'H': 
	     case 'f': n=parse_numbers(escape_code,1);
	               n2=parse_numbers(escape_code,2);
	               if ( (y_position>n) || 
			   ((x_position>n2)&&(y_position>=n))) backtrack++;
		       y_position=n;
	               x_position=n2;		       
	               break; 
		/* Decrement Y by N */
	     case 'A': n=parse_numbers(escape_code,1);
		       backtrack++;
	               y_position-=n;
	               break;
		/* Increment Y by N */
	     case 'B': n=parse_numbers(escape_code,1);
	               y_position+=n;
	               break;
	        /* Increment X by N */
	     case 'C': n=parse_numbers(escape_code,1); 
	               x_position+=n;
	               break;
		/* Decrement X by N */
	     case 'D': n=parse_numbers(escape_code,1); 
		       if (n!=255) {
	                  backtrack++;
		          x_position-=n;
		          if (x_position<0) x_position=0;
		       }
		
	               break;
		/* Report Current Position */
	     case 'R': fprintf(stderr,"Current Position: %d, %d\n",
			      x_position,y_position); break;
		/* Save Position */
	     case 's': oldx_position=x_position;
	               oldy_position=y_position;
	               break;
		/* Restore Position */
	     case 'u': x_position=oldx_position;
	               y_position=oldy_position;
	               break;
		/* Clear Screen and Home */
	     case 'J': for (x=0;x<x_size;x++)
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
	     case 'K': if (animate) {
			  if ( (animate_f=fopen(temp_file_name,"wb"))==NULL) {
			     fprintf(stderr,"Error!  Cannot open file %s to store temporary animation info.\n",
				    temp_file_name);
			     exit(1);
			  }
			  im3=gdImageCreate((x_size-(x_position-1))*8,16);     
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
	     case 'm': /* printf("Color\n"); */
	               n=parse_numbers(escape_code,0);
	               for(n2=1;n2<n+1;n2++) {
			  c=parse_numbers(escape_code,n2);
			  switch(c) {
			   case 0: color=DEFAULT; break; /* Normal */
			   case 1: color|=INTENSE; break; /* BOLD */
			   case 4: fprintf(stderr,"Warning!  Underline not supported!\n\n");
			           break;
			   case 5: color|=BLINK; use_blink++; break; /* BLINK */
			   case 7: color=(color>>4)+(color<<4); /* REVERSE */
			           break;
			   case 8: invisible=1; fprintf(stderr,"Warning! Invisible!\n\n"); break;
			   case 30: color&=FORE_CLEAR; /* Black Fore */
			            break;
			   case 31: color&=FORE_CLEAR; /* Red Fore */
			            color|=FORE_RED;
			            break;
			   case 32: color&=FORE_CLEAR; /* Green Fore */
			            color|=FORE_GREEN;
			            break;
			   case 33: color&=FORE_CLEAR; /* Yellow Fore */
			            color|=FORE_RED;
			            color|=FORE_GREEN;
			            break;
			   case 34: color&=FORE_CLEAR; /* Blue Fore */
			            color|=FORE_BLUE;
			            break;
			   case 35: color&=FORE_CLEAR; /* Purple Fore */
			            color|=FORE_BLUE;
			            color|=FORE_RED;
			            break;
			   case 36: color&=FORE_CLEAR; /* Cyan Fore */
			           color|=FORE_BLUE;
			           color|=FORE_GREEN;
			           break;
			   case 37: color&=FORE_CLEAR; /* White Fore */
			           color|=FORE_RED;
			           color|=FORE_GREEN;
			           color|=FORE_BLUE;  break;
			   case 40: color&=BACK_CLEAR; /* Black Back */
			            break;
			   case 41: color&=BACK_CLEAR; /* Red Back */
			            color|=BACK_RED;
			            break;
			   case 42: color&=BACK_CLEAR; /* Green Back */
			            color|=BACK_GREEN;
			            break;
			   case 43: color&=BACK_CLEAR; /* Yellow Back */
			            color|=BACK_GREEN;
			            color|=BACK_RED;
			            break;
			   case 44: color&=BACK_CLEAR; /* Blue Back */
			            color|=BACK_BLUE;
			            break;
			   case 45: color&=BACK_CLEAR; /* Purple Back */
			            color|=BACK_BLUE;
			            color|=BACK_RED;
			            break;
			   case 46: color&=BACK_CLEAR; /* Cyan Back */
			           color|=BACK_BLUE;
			           color|=BACK_GREEN;
			           break;
			   case 47: color&=BACK_CLEAR; /* White Back */
			           color|=BACK_RED;
			           color|=BACK_GREEN;
			           color|=BACK_BLUE;  break;

				/* 24-bit color support */
				case 38:
				case 48:
					fprintf(stderr,"Warning!  Unsupported 256 or 24-bit color mode!\n");
					break;
			   default: fprintf(stderr,"Warning! Invalid Color %d!\n\n",c);
			  }
		       }
	               break;
		/* Set screen mode */
	     case 'h': fprintf(stderr,"Warning!  Screen Mode Setting not Supported.\n\n"); 
	               break;
	        /* note, look for [= code */
 	     case 'p': fprintf(stderr,"Warning! Keyboard Reassign not Supported.\n"); 
	               break;
	     default: fprintf(stderr,"Warning! Unknown Escape Code\n");
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
          else if (temp_char=='\r'); /* Skip carriage returns, as most */
				     /* ansi's are from DOS            */
          else {
	        /* Where is the best place to check for wrapping? */
	     if (x_position>x_size) {
	        x_position=x_position%x_size;
		y_position++;
	     }

	     if (animate && y_position<=y_size) {  /* Animate it if we have the right */
	        for(xx=0;xx<8;xx++) {
	           for(yy=0;yy<16;yy++) {
		      if ( ((unsigned char)
	                 (font_to_use->font_data[(temp_char*16)+yy])) &(128>>xx) )
		         gdImageSetPixel(im2,xx,yy,colorC[color&0x0f]);
                         else gdImageSetPixel(im2,xx,yy,colorC[(color&0xf0)>>4]);
		   }
		}
	        animate_f=fopen(temp_file_name,"wb");
	        gdImageGif(im2, animate_f);
	        fclose(animate_f);
	        animate_gif(out_f,temp_file_name,0,(x_position-1)*8,
			    (y_position-1)*16,time_delay,0);
	     }
	     if (y_position<=y_size) {
	        screen[(x_position-1)+((y_position-1)*x_size)]=temp_char;
                if (!invisible) attributes[(x_position-1)+((y_position-1)*x_size)]=color;
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
    if (!(animate||blink)) {  /* If not animating, draw the final picture */

       if (output_type==OUTPUT_EPS) {
	  display_eps(out_f,output_type,x_size,y_size);
       }
       else {
	  display_gd(out_f,output_type,x_size,y_size);
       }
    }


    if ((!animate) && (blink)) {  /* If blinking... */

       for(i=0;i<2;i++) {
          for(y=0;y<y_size;y++) {
             for(x=0;x<x_size;x++) {
                for(xx=0;xx<8;xx++) {
                   for(yy=0;yy<16;yy++) {
	              if ( ((unsigned char) (font_to_use->font_data[(screen[x+(y*x_size)]*16)+yy])) &
	                   (128>>xx) ) {
	                 if ((attributes[x+(y*x_size)]&0x80)) {
		            if (i) {
                               gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,colorF[attributes[x+(y*x_size)]&0x0f]);
		            }
		            else {
			       gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,colorF[(attributes[x+(y*x_size)]&0x70)>>4]);
			    }

			 }
	                 else {
			    gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,colorF[attributes[x+(y*x_size)]&0x0f]);
			 }
		      }
		      else {
			 gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,colorF[(attributes[x+(y*x_size)]&0x70)>>4]);
		      }
		   }
		}
	     }
	  }

          sprintf(temp_file_name,"/tmp/ansi2gif_%i.gif",getpid());
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
   if (output_type==OUTPUT_EPS) {
      finish_eps(out_f);
   }
   else {
      finish_gd(out_f);
   }
	
   unlink(temp_file_name);   
 
}


static int detect_max_y(int animate,int blink,
			FILE *in_f,FILE *out_f,
		        int time_delay,int x_size,int y_size,
			int output_type) {
   
    unsigned char temp_char;

    unsigned char escape_code[BUFSIZ];
      
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
	     while (!isalpha(temp_char)) { /* Read in the command */
	        fread(&temp_char,sizeof(temp_char),1,in_f);
	        escape_code[escape_counter]=temp_char;
	        escape_counter++;
	     }
	     escape_code[escape_counter]='\000';
	        /* Big 'ol switch statement to figure out what to do */
	     switch(escape_code[escape_counter-1]) {
	        /* Move to x,y */
	     case 'H': 
	     case 'f': n=parse_numbers(escape_code,1);
	               n2=parse_numbers(escape_code,2);
	               if ( (y_position>n) || 
			   ((x_position>n2)&&(y_position>=n))) backtrack++;
		       y_position=n;
	               x_position=n2;		       
	               break; 
		/* Decrement Y by N */
	     case 'A': n=parse_numbers(escape_code,1);
		       backtrack++;
	               y_position-=n;
	               break;
		/* Increment Y by N */
	     case 'B': n=parse_numbers(escape_code,1);
	               y_position+=n;
	               break;
	        /* Increment X by N */
	     case 'C': n=parse_numbers(escape_code,1); 
	               x_position+=n;
	               break;
		/* Decrement X by N */
	     case 'D': n=parse_numbers(escape_code,1); 
		       if (n!=255) {
	                  backtrack++;
		          x_position-=n;
		          if (x_position<0) x_position=0;
		       }
		
	               break;
		/* Report Current Position */
	     case 'R': fprintf(stderr,"Current Position: %d, %d\n",
			      x_position,y_position); break;
		/* Save Position */
	     case 's': oldx_position=x_position;
	               oldy_position=y_position;
	               break;
		/* Restore Position */
	     case 'u': x_position=oldx_position;
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
	     default: /* normal, we ignore some cases */
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
          else if (temp_char=='\r'); /* Skip carriage returns, as most */
				     /* ansi's are from DOS            */
          else {
	     
	        /* Where is the best place to check for wrapping? */
	     if (x_position>x_size) {
	        x_position=x_position%x_size;
		y_position++;
	     }

//	     if (y_position<=y_size) {
                x_position++;
//	     }	     	     

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
       printf("   --blink            : Create an animated gif enabling blinking\n");
       printf("   --eps              : Output an Encapsulated Postscript\n");
//       printf("   --color X=0xRRGGBB : Set color \"X\" [0-16] to hex value RRGGBB [a number]\n");
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
vga_font *load_vga_font(char *namest,int xsize,int ysize,int numchars) {
   
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
		{"gif", 0, NULL, 'g'},
		{"help", 0, NULL, 'h'},
		{"output",1,NULL,'o'},
		{"png",0,NULL,'p'},
		{"timedelay",1,NULL,'t'},
		{"version",0,NULL,'v'},
		{"xsize",1,NULL,'x'},
		{"ysize",1,NULL,'y'},
		{0,0,0,0}
	};

	fprintf(stderr,"Run as %s\n",argv[0]);

	/* Check to see how run. if we were ansi2png or ansi2eps set */
	/* default output appropriately                              */
	if (strstr(argv[0],"eps")) output_type=OUTPUT_EPS;
	if (strstr(argv[0],"png")) output_type=OUTPUT_PNG;

	/*--  PARSE COMMAND LINE PARAMATERS --*/
	opterr=0;
	while ((c = getopt_long (argc, argv,
			"abc:ef:ghpt:vx:y:",
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
		case 'p':	output_type=OUTPUT_PNG;
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

	gif_the_text(animate,blink,input_f,output_f,
			time_delay,x_size,y_size,output_type);

	fclose(input_f);

	return 0;
}
