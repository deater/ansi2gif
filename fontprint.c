/*            ----------- PROGRAM FONTRPINT ------------------ *\
\*   by Vince Weaver... Prints out text file in VGA fonts      */
/*   With improvements by Jiri Kuchta;   kuchta@fee.vutbr.cz   *\
\*   Translated to Linux and FPK-Pascal: 8 October 1996        */
/*   Translated to C and rewritten: 1 February 1998            *\
\*                                                             */
/* http://www.glue.umd.edu/~weave/vmwprod/vmwsoft.html         */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef MAKE_GIF
#include "gd.h"
#endif

#define VERSION "3.9.9"

/* Type of Outputs Possible */
#define PRINTER 0
#define GIF     1

typedef struct {
      char *font_data;
      int width;
      int height;
      int numchars;
} vga_font;



   /* Really roughly hacked together.  This is a rough translation
    * of 4 year old code. I developed it from a Panasonic KX-1123
    * Printer Manual I had.  It should Work... I don't have a printer
    * with me so its a bit difficult to test.... */
int output_printer(vga_font *font,FILE *in_f,FILE *out_f,int ExtraSpacing,
		   int ExtraWide)
{
    int arraysize,string_length,position,in_char,arraysize2=640;
    int tempbyte,tempbyteR,bb2,bb1,ExtraWideCh,yy,i,j;
    unsigned char tempst[81];
    int outputar[729][16];
   
    if (ExtraSpacing) arraysize=9;
       else arraysize=8;
   
   while(fgets(tempst,80,in_f)!=NULL) {
        string_length=strlen(tempst);
        for(j=0;j<720;j++) for(i=0;i<16;i++) outputar[j][i]=0;
        for (position=0;position<string_length;position++) { 
	    for(in_char=0;in_char<15;in_char++) {
	       tempbyte=(unsigned char)font->font_data[((tempst[position])*16)+in_char];
               if(tempst[position]>31) {
	       outputar[((position)*arraysize)][in_char]=(tempbyte/128);
               tempbyteR=tempbyte%128;
               outputar[((position)*arraysize)+1][in_char]=(tempbyteR/64);
               tempbyteR=tempbyteR%64;
               outputar[((position)*arraysize)+2][in_char]=(tempbyteR/32);
               tempbyteR=tempbyteR%32;
               outputar[((position)*arraysize)+3][in_char]=(tempbyteR/16);
               tempbyteR=tempbyteR%16;
               outputar[((position)*arraysize)+4][in_char]=(tempbyteR/8);
               tempbyteR=tempbyteR%8;
               outputar[((position)*arraysize)+5][in_char]=(tempbyteR/4);
               tempbyteR=tempbyteR%4;
               outputar[((position)*arraysize)+6][in_char]=(tempbyteR/2);
               outputar[((position)*arraysize)+7][in_char]=tempbyteR%2;
	       }
	       if (ExtraSpacing) outputar[((position)*arraysize)+8][in_char]=0;
	    }
	}
/* Debugging code */      
/*      for(j=0;j<16;j++){
	 printf("%d : ",(unsigned char)font->font_data[((tempst[0]*16)+j)]);
	 for(i=0;i<80;i++){
	    printf("%d",(outputar[i][j]==0));
	 }
	 printf("\n");
      }
      */
      
      
  if (ExtraSpacing) arraysize2=720;
                    else arraysize2=640;
      /* Convert these all to shifts? */
      /* I would optimize this all a lot more if I had a   *\
      \* printer with me ;)                                */
  for (i=0;i<arraysize2;i++) {
      outputar[i][0]*=128;
      outputar[i][1]*=64;
      outputar[i][2]*=32;
      outputar[i][3]*=16;
      outputar[i][4]*=8;
      outputar[i][5]*=4;
      outputar[i][6]*=2;

      outputar[i][8]*=128;
      outputar[i][9]*=64;
      outputar[i][10]*=32;
      outputar[i][11]*=16;
      outputar[i][12]*=8;
      outputar[i][13]*=4;
      outputar[i][14]*=2;
  }
      
      
    bb2=(string_length*arraysize)/256;
    bb1=(string_length*arraysize)%256;
    if (ExtraWide) ExtraWideCh='K';
       else ExtraWideCh='L';
      
      
    fprintf(out_f,"%c3%c",27,24);
    fprintf(out_f,"%c%c%c%c",27,ExtraWideCh,bb1,bb2);
                      
    for(yy=0;yy<(arraysize*(string_length));yy++) {
	
    /*for(yy=0;yy<(arraysize*(string_length-1));yy++) {*/
       
      fprintf(out_f,"%c",outputar[yy][0]+outputar[yy][1]+outputar[yy][2]+
                    outputar[yy][3]+outputar[yy][4]+outputar[yy][5]+
                    outputar[yy][6]+outputar[yy][7]);
    }
    fprintf(out_f,"%c%c",10,13);

    fprintf(out_f,"%c%c%c%c",27,ExtraWideCh,bb1,bb2);
            
    for(yy=0;yy<(arraysize*(string_length));yy++) {
	
    /*for(yy=0;yy<(arraysize*(string_length-1));yy++) {*/
       fprintf(out_f,"%c",outputar[yy][8]+outputar[yy][9]+outputar[yy][10]+
                    outputar[yy][11]+outputar[yy][12]+outputar[yy][13]+
                    outputar[yy][14]+outputar[yy][15]);
    }
      fprintf(out_f,"%c%c",10,13);
      
        
   }
   return 0;
}


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
	     return -1;
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
  xxxxxxxx
 * 
 blink r g b  i r g b
 * 
 * 

 * 
 * 
 * 
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

int parse_text(vga_font *font,FILE *in_f) {
   unsigned char temp_char;
   unsigned char screen[80][25];
   unsigned char attributes[80][25];

   unsigned char escape_code[BUFSIZ];
   
   int x_position,y_position,oldx_position,oldy_position;
   int color=0,x,y,emergency_exit=0;
   int escape_counter,n,n2,c,invisible=0,xx,yy;
   
   int colors[16];
   
   gdImagePtr im;
   int black,white;
   
   FILE *fff;
   
   int x_size=80,y_size=25;
   
   for(x=0;x<x_size;x++)
      for(y=0;y<y_size;y++) {
         screen[x][y]=' ';
	 attributes[x][y]=BLACK;
      }
       
    x_position=1; y_position=1;
    oldx_position=1; oldy_position=1;
    
   
   while (!feof(in_f) && (emergency_exit==0) ) {
      fread(&temp_char,sizeof(temp_char),1,in_f);
      
      if (temp_char==27) {
	 fread(&temp_char,sizeof(temp_char),1,in_f);
	 if (temp_char!='[') printf("False Escape\n");
	 else {
	    escape_counter=0;
	    while (!isalpha(temp_char)) {
	       fread(&temp_char,sizeof(temp_char),1,in_f);
	       escape_code[escape_counter]=temp_char;
	       escape_counter++;
	    }
	    escape_code[escape_counter]='\000';
	     /* printf("%s : ",escape_code); */
	    /*n=parse_numbers((char *)&escape_code,0);
	    printf("%d ",n);
	    for (i=0;i<n;i++) printf(" %d ",parse_numbers((char *)&escape_code,i+1));
	    */
	    switch(escape_code[escape_counter-1]) {
	     case 'H': 
	     case 'f': n=parse_numbers((char *)&escape_code,1);
	               n2=parse_numbers((char *)&escape_code,2);
	               printf("Move to %d %d\n",n,n2);
	               y_position=n;
	               x_position=n2;
	               break; 
	     case 'A': n=parse_numbers((char *)&escape_code,1);
	               printf("Move up %d\n",n);
	               y_position-=n;
	               break;
	     case 'B': n=parse_numbers((char *)&escape_code,1);
	               printf("Move down %d\n",n); 
	               y_position+=n;
	               break;
	     case 'C': n=parse_numbers((char *)&escape_code,1);
	               printf("Move forward %d\n",n); 
	               x_position+=n;
	               break;
	     case 'D': n=parse_numbers((char *)&escape_code,1); 
	               printf("Move backward %d\n",n); 
	               x_position-=n;
	               break;
	     case 'R': printf("Current Position: %d, %d\n",
			      x_position,y_position); break;
	     case 's': printf("Save Position\n"); 
	               oldx_position=x_position;
	               oldy_position=y_position;
	               break;
	     case 'u': printf("Return Position\n"); 
	               x_position=oldx_position;
	               y_position=oldy_position;
	               break;
	     case 'J': printf("Clear Screen and Home\n"); 
	               for (x=0;x<x_size;x++)
		           for (y=0;y<y_size;y++)
		               screen[x][y]=' ';
	               x_position=1;
	               y_position=1;
	               break;
	     case 'K': printf("Clear to end of line\n"); 
	               for(x=x_position;x<x_size;x++) 
		          screen[x][y_position]=' ';
	               x_position=x_size;
	                break;
	     case 'm': /* printf("Color\n"); */
	               n=parse_numbers((char *)&escape_code,0);
	               for(n2=1;n2<n+1;n2++) {
			  c=parse_numbers((char *)&escape_code,n2);
			  switch(c) {
			   case 0: color=DEFAULT; break; /* Normal */
			   case 1: color|=INTENSE; break; /* BOLD */
			   case 4: printf("Warning!  Underline not supported!\n");
			           break;
			   case 5: color|=BLINK; break; /* BLINK */
			   case 7: color=(color>>4)+(color<<4); /* REVERSE */
			           break;
			   case 8: invisible=1; printf("Warning! Invisible!\n"); break;
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
			   default: printf("Warning! Invalid Color %d!\n",c);
                         			     
			  }
			  
			  
			  
			  
			  
		       }
	       
	       
	       
	       
	               break;
	     case 'h': printf("Warning!  Screen Mode Setting not Supported.\n"); 
	               break;
	    /* note, look for [= code */   
 	     case 'p': printf("Warning! Keyboard Reassign not Supported.\n"); 
	               break;
	     default: printf("Warning! Unknown Escape Code\n"); 
	              break;
           	       
	    }

	 }
      }
      
    else {  
      if (temp_char!='\n') {
         screen[x_position-1][y_position-1]=temp_char;
        if (!invisible) attributes[x_position-1][y_position-1]=color;
        x_position++;
      }
      else {
         x_position=1;
	 y_position++;
      }
      if (x_position>80) {
	 x_position=x_position%80;
	 y_position++;
	 if (y_position>25) emergency_exit=1;
      }
      
      
   }
   }

       im = gdImageCreate(x_size*8,y_size*16);
                                       /* r    g     b   */
       colors[0] =gdImageColorAllocate(im,0x00,0x00,0x00);
       colors[1] =gdImageColorAllocate(im,0x00,0x00,0xAA);
       colors[2] =gdImageColorAllocate(im,0x00,0xAA,0x00);  
       colors[3] =gdImageColorAllocate(im,0x00,0xAA,0xAA);
       colors[4] =gdImageColorAllocate(im,0xAA,0x00,0x00);
       colors[5] =gdImageColorAllocate(im,0xAA,0x00,0xAA);
       colors[6] =gdImageColorAllocate(im,0xAA,0xAA,0x00);
       colors[7] =gdImageColorAllocate(im,0xAA,0xAA,0xAA);
       colors[8] =gdImageColorAllocate(im,0x44,0x44,0x44);
       colors[9] =gdImageColorAllocate(im,0x00,0x00,0xFF);
       colors[10]=gdImageColorAllocate(im,0x00,0xFF,0x00);
       colors[11]=gdImageColorAllocate(im,0x00,0xFF,0xFF);
       colors[12]=gdImageColorAllocate(im,0xFF,0x00,0x00);
       colors[13]=gdImageColorAllocate(im,0xFF,0x00,0xFF);
       colors[14]=gdImageColorAllocate(im,0xFF,0xFF,0x00);
       colors[15]=gdImageColorAllocate(im,0xFF,0xFF,0xFF);
   

       for(y=0;y<y_size;y++) {
	  for(x=0;x<x_size;x++) {
             for(xx=0;xx<8;xx++) {
		for(yy=0;yy<16;yy++) {
	            if ( ((unsigned char)
		       (font->font_data[(screen[x][y]*16)+yy])) &
	               (128>>xx) )
		       gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,colors[attributes[x][y]&0x0f]);
                  else gdImageSetPixel(im,(x*8)+xx,(y*16)+yy,colors[(attributes[x][y]&0xf0)>>4]);
    
		}
	     }
	  }
       }

   
   
   
       fff=fopen("out.gif","w+");
   
       gdImageGif(im, fff);
         /* Destroy the image in memory. */
       gdImageDestroy(im);
       return 0;
   
   
   
   for(y=0;y<25;y++)
     for(x=0;x<80;x++) {
	printf("%c",screen[x][y]);
     }
  
}


#ifdef MAKE_GIF

   /* Sorta hacked together.  Requires the GD library.  See README */
int output_gif(vga_font *font,FILE *in_f,FILE *out_f,int ExtraSpacing,
		   int ExtraWide)
{
    gdImagePtr im;
    int black,white;
    int xx=0,x=0,y=0,line,len,i;
    unsigned char tempst[81];	   

         /* BUG.. can't have more than 80 columns and like 35 rows */
    im = gdImageCreate(720, 600);
    black = gdImageColorAllocate(im, 0, 0, 0);
    white = gdImageColorAllocate(im, 255, 255, 255);
    gdImageFill(im,10,10,white);    
   
    while(fgets(tempst,80,in_f)!=NULL) {
       len=strlen(tempst);
       for(line=0;line<16;line++)
          for(i=0;i<len;i++) 
	     if (tempst[i]>31)
                for (xx=0;xx<8;xx++)
                    if ( ((unsigned char) 
		         (font->font_data[(tempst[i]*16)+line])) & 
		                                    (128>>xx) ) 
		       gdImageSetPixel(im,(x+(i*(8+ExtraSpacing))+xx),
					                  y+line,black);

       y+=16;
    }
    gdImageGif(im, out_f); 
      /* Destroy the image in memory. */
    gdImageDestroy(im);
    return 0;
}

#endif

int display_help(char *name_run_as,int just_version)
{
    printf("\nLinux FontPrint v %s by Vince Weaver (weave@eng.umd.edu)\n\n",
	   VERSION);
    if (!just_version) {
       printf(" %s font_file [text_file] [output_file] [-n] [-w] [-gif]"
	      " [-h] [-v]\n\n",name_run_as);
       printf("   font_file   : standard VGA font to use, usually 4096 bytes "
	                        "long\n");
       printf("   text_file   : The text file you wish to print\n");
       printf("   output_file : The file where the output is to be placed.\n");
       printf("   -n          : Eliminate extra spaces added between each "
	                        "character\n");
       printf("   -w          : Print in \"extra-wide\" mode\n");
       printf("   -gif        : Output gif file instead of raw printer "
	                        "output\n");
       printf("   -v          : Print version information\n");
       printf("   -h          : Show this help information\n\n");
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
       printf("ERROR loading font file %s.\n",namest);
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
   int i;
   int ExtraSpacing=1,ExtraWide=0,
       OutPutType=PRINTER;
   
   int FontNamePresent=0,InputNamePresent=0,OutputNamePresent=0;
   char font_name[255],input_name[255],output_name[255];
   vga_font *font_to_use;

   
       /*--  PARSE COMMAND LINE PARAMATERS --*/
   
    if (argc==1) display_help(argv[0],0);
    for(i=1;i<argc;i++) {
       if ((argv[i][0]=='-')) { 
	  switch(argv[i][1]) {
	   case 'v': case 'V': display_help(argv[0],1); break;
	   case 'h': case 'H': case '?': display_help(argv[0],0); break;
	   case 'n': ExtraSpacing=0; break;
	   case 'w': ExtraWide=1; break;
	   case 'g': OutPutType=GIF; 
#ifndef MAKE_GIF
                     printf("\nGif output not supported in this binary.  "
			    "\nPlease get the gd library and re-compile"
			    " as per the README file for this option.\n");
	             exit(1);
#endif	     
	     break;
	   default: printf("\nInvalid Option %s.  Please type \"%s -h\""
			   " for a list of valid options.\n\n",argv[i],
			   argv[0]); exit(1); break;
	  }
       }
       else {
	  if (!FontNamePresent) {
	     strncpy(font_name,argv[i],254);
	     FontNamePresent=1;
	  } else if(!InputNamePresent) {
	     strncpy(input_name,argv[i],254);
	     InputNamePresent=1;
	  }
	  else if(!OutputNamePresent) {
	     strncpy(output_name,argv[i],254);
	     OutputNamePresent=1;
	  }
	  else {
	    printf("\n\nExtraneous Extra Filename %s.  Please type \"%s -h\""
		   " to view proper inputs.\n\n",argv[i],argv[0]);
	     exit(1);
	  }
       }
    }
 
    font_to_use=load_vga_font(font_name,8,16,256);
    if (font_to_use==NULL) return 0;
   
    if (InputNamePresent) {
       if( (input_f=fopen(input_name,"r"))==NULL) {
	  printf("\nInvalid Input File: %s\n",input_name);
	  exit(1);
       }
    }
    else input_f=stdin;
   
    if (OutputNamePresent) {
       if( (output_f=fopen(output_name,"w"))==NULL){
	  printf("\nInvalid Output File: %s\n",output_name);
	  exit(1);
       }
    }
    else output_f=stdout;

   parse_text(font_to_use,input_f);
 
     return 0;
   
   if (OutPutType==PRINTER) {
       output_printer(font_to_use,input_f,output_f,ExtraSpacing,ExtraWide);
    }
#ifdef MAKE_GIF
    if (OutPutType==GIF) {
       output_gif(font_to_use,input_f,output_f,ExtraSpacing,ExtraWide);
    }
#endif
   
    fclose(input_f);
    fclose(output_f);
   

    return 0;   
}
