/*             ----------- ansi2gif.c ------------------       *\
\*   by Vince Weaver... Makes a gif out of "ANSI" files        */
/*   Based on my "fontprint" program.                          *\
\* http://www.glue.umd.edu/~weave/vmwprod/vmwsoft.html         */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>

#include <gd.h>

#include "default_font.h"

#define DEFAULT_TIMEDELAY 1 /* 1/100 of a second */
#define DEFAULT_XSIZE    80
#define DEFAULT_YSIZE    25

#define VERSION "0.9.9"

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
int parse_numbers(unsigned char *string,int index) {
   
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
       if (!isdigit(string[i])) {  /* Spec says ';' but here !digit works */
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

int gif_the_text(int animate,int blink,vga_font *font,FILE *in_f,char *outfile,
		 int time_delay,int x_size,int y_size) {
   
    FILE *out_f,*animate_f=NULL;
    unsigned char temp_char;
    unsigned char *screen;
    unsigned char *attributes;

    unsigned char escape_code[BUFSIZ];
    char temp_file_name[BUFSIZ];
      
    int x_position,y_position,oldx_position,oldy_position;
    int color=DEFAULT,x,y,emergency_exit=0;
    int escape_counter,n,n2,c,invisible=0,xx,yy;

    int colorF[16];
    int colorC[16];
    int backtrack=0,use_blink=0,i;
   
    gdImagePtr im,im2,im3;

    screen=(unsigned char *)calloc(x_size*y_size,sizeof(unsigned char));
    attributes=(unsigned char *)calloc(x_size*y_size,sizeof(unsigned char));
   
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
    
    if (animate) {
       sprintf(temp_file_name,"/tmp/ansi2gif_%i.gif",getpid());
         
       if ( (out_f=fopen(temp_file_name,"wb"))==NULL) {
	  printf("Error!  Cannot open file %s to store temporary animation info!\n\n",
		 temp_file_name);
	  exit(1);
       }
          /* Clear Screen */
       gdImageRectangle(im,0,0,x_size*8,y_size*16,colorF[0]);
			     
       gdImageGif(im, out_f);
       fclose(out_f);
       animate_f=fopen(outfile,"wb");
       animate_gif(animate_f,temp_file_name,1,0,0,time_delay,0); 
    }
    
       /* Clear the memory used to store the image */
    for(x=0;x<x_size;x++)
       for(y=0;y<y_size;y++) {
          screen[x+(y*x_size)]=' ';
	  attributes[x+(y*x_size)]=BLACK;
    }
   
       /* Initialize the Variables */
    x_position=1; y_position=1;
    oldx_position=1; oldy_position=1;
    
    while (  ((fread(&temp_char,sizeof(temp_char),1,in_f))>0) 
	     && (emergency_exit==0) ) {

          /* Did somebody say escape?? */
       if (temp_char==27) {
	  fread(&temp_char,sizeof(temp_char),1,in_f);
	     /* If after escape we have '[' we have an escape code */
	  if (temp_char!='[') printf("False Escape\n");
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
	     case 'f': n=parse_numbers((char *)&escape_code,1);
	               n2=parse_numbers((char *)&escape_code,2);
	               if ( (y_position>n) || 
			   ((x_position>n2)&&(y_position>=n))) backtrack++;
		       y_position=n;
	               x_position=n2;		       
	               break; 
		/* Decrement Y by N */
	     case 'A': n=parse_numbers((char *)&escape_code,1);
		       backtrack++;
	               y_position-=n;
	               break;
		/* Increment Y by N */
	     case 'B': n=parse_numbers((char *)&escape_code,1);
	               y_position+=n;
	               break;
	        /* Increment X by N */
	     case 'C': n=parse_numbers((char *)&escape_code,1); 
	               x_position+=n;
	               break;
		/* Decrement X by N */
	     case 'D': n=parse_numbers((char *)&escape_code,1); 
	               backtrack++;
		       x_position-=n;
		       if (x_position<0) x_position=0;
	               break;
		/* Report Current Position */
	     case 'R': printf("Current Position: %d, %d\n",
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
			  if ( (out_f=fopen(temp_file_name,"wb"))==NULL) {
			     printf("Error!  Cannot open file %s to store temporary animation info.\n",
				    temp_file_name);
			     exit(1);
			  }
			  gdImageGif(im, out_f);
			  fclose(out_f);
			  animate_gif(animate_f,temp_file_name,0,0,0,time_delay,0);  
		       }
	               break;
		/* Clear to end of line */
	     case 'K': if (animate) {
			  if ( (out_f=fopen(temp_file_name,"wb"))==NULL) {
			     printf("Error!  Cannot open file %s to store temporary animation info.\n",
				    temp_file_name);
			     exit(1);
			  }
			  im3=gdImageCreate((x_size-(x_position-1))*8,16);     
			  gdImageRectangle(im3,0,0,(x_size-(x_position-1))*8,16,gdImageColorAllocate(im3,0x00,0x00,0x00));
			  
			  gdImageGif(im3, out_f);
			  fclose(out_f);
			  gdImageDestroy(im3);
			  animate_gif(animate_f,temp_file_name,0,(x_position-1)*8,(y_position-1)*16,time_delay,0);  
		       }
		       for(x=x_position;x<x_size;x++) 
		          screen[x+(y_position*x_size)]=' ';
	               x_position=x_size; 
	               break;
		/* Oh what fun, figuring out colors */
	     case 'm': /* printf("Color\n"); */
	               n=parse_numbers((char *)&escape_code,0);
	               for(n2=1;n2<n+1;n2++) {
			  c=parse_numbers((char *)&escape_code,n2);
			  switch(c) {
			   case 0: color=DEFAULT; break; /* Normal */
			   case 1: color|=INTENSE; break; /* BOLD */
			   case 4: printf("Warning!  Underline not supported!\n\n");
			           break;
			   case 5: color|=BLINK; use_blink++; break; /* BLINK */
			   case 7: color=(color>>4)+(color<<4); /* REVERSE */
			           break;
			   case 8: invisible=1; printf("Warning! Invisible!\n\n"); break;
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
			   default: printf("Warning! Invalid Color %d!\n\n",c);
			  }  
		       }
	               break;
		/* Set screen mode */
	     case 'h': printf("Warning!  Screen Mode Setting not Supported.\n\n"); 
	               break;
	        /* note, look for [= code */   
 	     case 'p': printf("Warning! Keyboard Reassign not Supported.\n"); 
	               break;
	     default: printf("Warning! Unknown Escape Code\n"); 
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
          else if (temp_char=='\015'); /* Skip carriage returns, as most *\
				       \* ansi's are from DOS            */
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
	                 (font->font_data[(temp_char*16)+yy])) &(128>>xx) )
		         gdImageSetPixel(im2,xx,yy,colorC[color&0x0f]);
                         else gdImageSetPixel(im2,xx,yy,colorC[(color&0xf0)>>4]);
		   }
		}
	        out_f=fopen(temp_file_name,"wb");
	        gdImageGif(im2, out_f);
	        fclose(out_f);
	        animate_gif(animate_f,temp_file_name,0,(x_position-1)*8,
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
	       printf("Error!  Scrolled past maximum y_size of %i!\n\n",y_size);
	    }
       }
    }
    if ((!animate) && (!blink)) {  /* If not animating, draw the final picture */
       for(y=0;y<y_size;y++) {
	  for(x=0;x<x_size;x++) {
             for(xx=0;xx<8;xx++) {
		for(yy=0;yy<16;yy++) {
	            if ( ((unsigned char)
		       (font->font_data[(screen[x+(y*x_size)]*16)+yy])) &
	               (128>>xx) )
		       gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,colorF[attributes[x+(y*x_size)]&0x0f]);
                  else gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,colorF[(attributes[x+(y*x_size)]&0xf0)>>4]);
    
		}
	     }
	  }
       }
       out_f=fopen(outfile,"w");
       gdImageGif(im, out_f);
       fclose(out_f);
    }
    if ((!animate) && (blink)) {  /* If blinking... */
       animate_f=fopen(outfile,"w");
       
       for(i=0;i<2;i++) {
       for(y=0;y<y_size;y++) {
       for(x=0;x<x_size;x++) {
       for(xx=0;xx<8;xx++) {
       for(yy=0;yy<16;yy++) {
	  if ( ((unsigned char) (font->font_data[(screen[x+(y*x_size)]*16)+yy])) &
	       (128>>xx) ) {
	     if ((attributes[x+(y*x_size)]&0x80)) {
		if (i) {
                   gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,colorF[attributes[x+(y*x_size)]&0x0f]);
		}
		else gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,colorF[(attributes[x+(y*x_size)]&0x70)>>4]);
	     }
	     else gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,colorF[attributes[x+(y*x_size)]&0x0f]);
	  }
          else gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,colorF[(attributes[x+(y*x_size)]&0x70)>>4]);
			 
       }
       }
       }
       }
	  
       sprintf(temp_file_name,"/tmp/ansi2gif_%i.gif",getpid());
       out_f=fopen(temp_file_name,"wb");
       gdImageGif(im, out_f);
       fclose(out_f);
       animate_gif(animate_f,temp_file_name,(1-i),0,0,time_delay,1);
       }
       fputc(';',animate_f);
       fclose(animate_f);
    }
   
       /* Destroy the image in memory. */
    gdImageDestroy(im);
    gdImageDestroy(im2);
    if (animate) {
       fputc(';', animate_f); /* End of Gif file */
       fclose(animate_f);
    }
   if ((backtrack) && !(animate)) {
      printf("Warning!  The cursor moved backwards and animated output was not selected.\n"
	     "          For proper output, you might want to try again with --ansi\n\n"); 
   }
   if ((use_blink)&&(!blink)) {
      printf("Warning!  A blinking color code was used.  To display blinking ansis you\n"
	     "          to run with the --blink option to create an animated gif.\n\n");
   }
   unlink(temp_file_name);   
   return 0;
}

void print_spaces(int number_to_print) {
    int i;
    for(i=0;i<number_to_print;i++) printf(" ");
}

int display_help(char *name_run_as,int just_version)
{
    printf("\nansi2gif v %s by Vince Weaver (weave@eng.umd.edu)\n\n",
	   VERSION);
    if (!just_version) {
       printf(" %s [--animate] [--blink] [--color X=0xYYYYYY]\n",name_run_as); 
       print_spaces(strlen(name_run_as));
       printf("  [--font fontfile] [--help] [--version] [--timedelay T]\n");
       print_spaces(strlen(name_run_as));
       printf("  [--xsize X] [--ysize Y] input_file output_file\n\n");
       printf("   --animate          : Create an animated gif if an animated ansi\n");
       printf("   --blink            : Create an animated gif enabling blinking\n");
       printf("   --color X=0xRRGGBB : Set color \"X\" [0-16] to hex value RRGGBB [a number]\n");
       printf("   --font fontfile    : Use vgafont \"filename\" to create gif\n"); 
       printf("   --help             : show this help\n");
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
    return 0;
}

    /* Load VGA font... Used in my game TB1 */
    /* psf font support added by <bkbratko@ardu.raaf.defence.gov.au> */
vga_font *load_vga_font(char *namest,int xsize,int ysize,int numchars)
{
    unsigned char buff[16];
    FILE *f;
    int i,fonty,numloop;
    vga_font *font;
    char *data;
   
    short int psf_id;
    char psf_mode;
    char psf_height;
   
    font=(vga_font *)malloc(sizeof(vga_font));
    data=(char *)calloc(numchars*ysize,(sizeof(char)));
   
    f=fopen(namest,"r");
    if (f==NULL) {
       printf("\nERROR loading font file %s.\n\n",namest);
       return NULL;
    }
   
    fread(&psf_id,sizeof(psf_id),1,f);
    /* psf files contain a magic number 0x0436 in the first word */
    if ( 0==strncmp(".psf",namest+strlen(namest)-4,4) ) {
       if (psf_id!=0x436 ) {
	  printf("ERROR file %s is not a psf file \n",namest);
	  return NULL;
       }
    }
    /* the next two bytes of psf file contain the mode and height
     *      * mode 0 is for 256 character fonts, which can be used by fontprint
     *      * only height = 16 is suitable for fontprint v3.0.x */
    fread(&psf_mode,sizeof(psf_mode),1,f);
    fread(&psf_height,sizeof(psf_height),1,f);
    if (psf_id==0x436 && (psf_mode!=0 || psf_height!=16 )) {
       printf("ERROR unable to deal with this size of psf file \n");
       return NULL;
    }
   
    /* if control reaches this point and the font is not a psf file
     *      * then we must rewind the file in order to recover the first
     *      * four bytes */
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

    /* MAIN PROGRAM */
int main(int argc, char **argv) {

    FILE *input_f,*output_f;
    char c;
 
    int time_delay=DEFAULT_TIMEDELAY;
    int x_size=DEFAULT_XSIZE,y_size=DEFAULT_YSIZE;
    int animate=0,blink=0;
    char *font_name=NULL,*input_name=NULL,*output_name=NULL;
    vga_font *font_to_use=NULL;
    char *endptr;
    int option_index = 0;

    int font_supplied=0; 
   
    static struct option long_options[] =
    {
       {"animate", 0, NULL, 'a'},
       {"blink", 0, NULL, 'b'},
       {"color", 1, NULL, 'c'},
       {"font", 1, NULL, 'f'},
       {"help", 0, NULL, 'h'},
       {"output",1,NULL,'o'},
       {"timedelay",1,NULL,'t'},
       {"version",0,NULL,'v'},
       {"xsize",1,NULL,'x'},
       {"ysize",1,NULL,'y'},
       {0,0,0,0}
    };
   
       /*--  PARSE COMMAND LINE PARAMATERS --*/

    opterr=0;
    while ((c = getopt_long (argc, argv,
			     "abc:f:ht:vx:y:",
			     long_options,&option_index))!=-1) {
       switch (c) {
	  case 'a': animate=1; break;
	  case 'b': blink=1; break;
	  case 'c': printf ("\nWarning! Setting alternate colors not implemented yet.\n\n");break;
	  case 'f': font_supplied=1; 
	            font_name=strdup(optarg);
	            break;
	  case 'h': display_help(argv[0],0); break;
	  case 't': time_delay=strtol(optarg,&endptr,10);
	            if ( endptr == optarg ) {
		       printf("\nError! \"%s\" is an invalid time delay.\n"
			      "            Please select a delay that is an integer number of 1/100 of seconds.\n\n",optarg);
	               exit(9);
		    }
	            printf("\nTime Delay in Animation %f seconds\n",((float)time_delay/100));
	            break;
	  case 'v': display_help(argv[0],1); break;
	  case 'x': x_size=strtol(optarg,&endptr,10);
	            if ( endptr == optarg ) {
	               printf("\nError!  \"%s\" is not a valid x size.\n\n",optarg);
	               exit(9);
		    }
	            break;
	  case 'y': if (!strcmp(optarg,"auto")) {
	               printf("\nError! Automatic sizing is not implemented yet.  Sorry.\n\n");
	               exit(12);
	            } 
	            y_size=strtol(optarg,&endptr,10);
	            if ( endptr == optarg ) {
		       printf("\nError!  \"%s\" is not a valid y size.\n\n",optarg); 
		       exit(9);
		    }
	  break;
	  default : printf("\nError! Bad command line option!\n\n"); exit(5); break;
       }
    }
   
    if (animate && blink) {
       printf("Error!  Cannot do blink and animate simultaneously!\n\n");
       exit(1);
    }
    
    if (blink) time_delay=25;
   
    if (optind<argc)
       input_name=strdup(argv[optind]);
    else {
       printf("\nError!  You need to have both an input and output filename specified!\n\n");
       exit(3);
    }
    if (optind<argc-1)
       output_name=strdup(argv[optind+1]);
    else {
       printf( "\nError!  No output file was specified.\n\n" );
       exit( 3 );
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
   
    if ( (input_f=fopen(input_name,"r"))==NULL) {
	  printf("\nInvalid Input File: %s\n",input_name);
	  return 1;
    } 
   
    if ( (output_f=fopen(output_name,"w"))==NULL){
       printf("\nInvalid Output File: %s\n",output_name);
       return 1;    
    }
    fclose(output_f);
   
    gif_the_text(animate,blink,font_to_use,input_f,output_name,time_delay,
		 x_size,y_size);
 
    fclose(input_f);

    return 0;   
}
