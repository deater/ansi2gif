/*
 * whirlgif.c
 *
 * Copyright (c) 1997,1998 by Hans Dinsen-Hansen (dino@danbbs.dk)
 * Copyright (c) 1995,1996 by Kevin Kadow (kadokev@msg.net)
 * Based on txtmerge.c
 * Copyright (c) 1990,1991,1992,1993 by Mark Podlipec.
 * All rights reserved.
 *
 * This software may be freely copied, modified and redistributed
 * without fee provided that above copyright notices are preserved
 * intact on all copies and modified copies.
 *
 * There is no warranty or other guarantee of fitness of this software.
 * It is provided solely "as is". The author(s) disclaim(s) all
 * responsibility and liability with respect to this software's usage
 * or its effect upon hardware or computer systems.
 *
 * The Graphics Interchange format (c) is the Copyright property of
 * Compuserve Incorporated.  Gif(sm) is a Service Mark property of
 * Compuserve Incorporated.
 *
 */
/*
 * Description:
 *
 * This program reads in a sequence of single-image Gif format files and
 * outputs a single multi-image Gif file, suitable for use as an animation.
 *
 * TODO:
 *
 * More options for dealing with the colormap
 *
 */

/*
 * Rev 3.02    ??Oct9? Hans Dinsen-Hansen
 * Loop. Verbose -> DEBUG. Further minimizing.
 * Rev 3.01      Oct98 Hans Dinsen-Hansen
 * Never published.  Various experiments with Windows versions.
 * Rev 3.00    29jul98 Hans Dinsen-Hansen
 * Gif-repacking; unification of color map; only output diff.
 * Rev 2.02    09Sep97 Hans Dinsen-Hansen
 * Gif89a input; use global colormap whenever possible; background index
 * Rev 2.01    31Aug96 Kevin Kadow
 * disposal
 * Rev 2.00    05Feb96 Kevin Kadow
 * transparency, gif comments,
 * Rev 1.10    29Jan96 Kevin Kadow
 * first release of whirlgif
 *
 * txtmerge:
 * Rev 1.01    08Jan92 Mark Podlipec
 * use all colormaps, not just 1st.
 * Rev 1.00    23Jul91 Mark Podlipec
 * creation
 *
 *
 */

#include "whirlgif.h"

/*
 * Set some defaults, these can be changed on the command line
 */
unsigned int loop=DEFAULT_LOOP, loopcount=0,
	 useColormap=DEFAULT_USE_COLORMAP, debugFlag=0,
	 globmap=0, minimize=0;

int imagex = 0, imagey = 0, imagec = 0, GifBgcolor=0, count=0;

/* global settings for offset, transparency */

Global global;

GifColor gifGmap[256], gifCmap[256];
GifScreenHdr globscrn, gifscrn;

GifImageHdr gifimage, gifimageold;

extern ULONG gifBlockSize, gifMask[], gifPtwo[];
UBYTE *pixold=NULL;

char gifFileName[BIGSTRING];
FILE *ff;

long sq(UBYTE i,UBYTE j)
{
  return((i-j)*(i-j));
}


  /* set global values */
/*  global.trans.type = TRANS_NONE;
  global.trans.valid = 0;
  global.time = DEFAULT_TIME;
  global.left = 0;
  global.top = 0;
  global.disposal = DEFAULT_DISPOSAL;
*/

	/* set Background color index */
	/*
	  GifBgcolor = atoi(argv[i++]) | 0x100; */
   
	/* Enable looping */
	/*   loop = TRUE; */
        /* loopcount = 0; */
   
	    /* Delay time in 1/100's of a second */
	    /* global.time = atoi(argv[i++]); */
	  
	   /* Output file - send output to a given filename */
	 
           /* offset */
	    /* SetOffset(argv[i]); */
	   
	    /* input file - file with a list of images */
	 
/*	      if(!count) GifReadFile(fout, gifFileName, 1);
	      else       GifReadFile(fout, gifFileName, 0);
	      count++;
   
              global.left = global.top = 0; */
  
/*
   fputc(';', fout); # End of Gif file #
   fclose(fout);
*/


/*
 * Read a Gif file.
 */
void animate_gif(FILE *fout,char *fname,int firstImage,int Xoff,int Yoff,
		 int delay_time,int loop_val) /* time in 100th of seconds */
{
  FILE *fp;
  UBYTE *pix;
  int i;
  if ( (fp = fopen(fname, REABIN)) == 0) {
    fprintf(stderr, "Can't open %s for reading.\n", fname);
    TheEnd();
  }
     global.trans.type = TRANS_NONE;
     global.trans.valid = 0;
     global.time = delay_time;

     global.disposal = DEFAULT_DISPOSAL;
    global.left = Xoff;
    global.top = Yoff;

    loop=loop_val;
     /*   loop = TRUE; */
           /* loopcount = 0; */
   
   
  /*global.left = global.top = 0;*/
   
   
   GifScreenHeader(fp, fout, firstImage);

   /* read until , separator */
  do {
    switch ( i = Xgetc(fp)) {
      case ',':
      case '\0':
	break;
      case '!':
	Xgetc(fp); /* the extension code */
	for ( i = Xgetc(fp); i > 0; i-- ) Xgetc(fp);
	while ( ( i = Xgetc(fp) ) > 0 ) {
	  for ( i = i ; i > 0; i-- ) Xgetc(fp);
	}
	break;
      default:
	fclose(fp);
	if ( feof(fp) || i == ';' )
	TheEnd1("GifReadHeader: Unexpected End of File\n");
	TheEnd1("GifReadHeader: Unknown block type\n");
     }
   } while(i != ',');

  if(firstImage) {
    globscrn.m = gifscrn.m;
    globscrn.pixbits = gifscrn.pixbits;
    globscrn.bc = gifscrn.bc;
    if ( globscrn.m ) {
      for (i = gifMask[1+globscrn.pixbits]; i >= 0; i--) {
	gifGmap[i].cmap.red   = gifCmap[i].cmap.red;
	gifGmap[i].cmap.green = gifCmap[i].cmap.green;
	gifGmap[i].cmap.blue  = gifCmap[i].cmap.blue;
      }
    }
    if(loop) GifLoop(fout, loopcount);
  }

  ReadImageHeader(fp);

 /*** ACTION for IMAGE */

  if ( ( gifimage.m != 0 && globmap !=0 ) || minimize !=0 ) {
    UBYTE translator[256], *p, *po;
    int left, right, top, bot, i, j, k, l, hi, wi;
    long dsquare, dsquare1;
    hi = gifimage.height;
    wi = gifimage.width;
    if (( pix = (UBYTE *)malloc(wi * hi * sizeof(UBYTE)) ) == NULL )
	 TheEnd1("No memory for image\n");
    if (debugFlag) fprintf(stderr, "  decoding picture no %d\n", count);
    GifDecode(fp, pix, gifimage);
    gifimage.i = 0;
    k = gifMask[1+globscrn.pixbits]; 
    l = gifMask[1+gifscrn.pixbits]; 
    for (j = 0; j <= l; j++) {
      dsquare = 256*256*3;
      for (i = 0; i <= k; i++) {
	dsquare1 = sq(gifGmap[i].cmap.red, gifCmap[j].cmap.red) +
		   sq(gifGmap[i].cmap.green, gifCmap[j].cmap.green) +
		   sq(gifGmap[i].cmap.blue,  gifCmap[j].cmap.blue);
	if ( dsquare1 < dsquare ) {
	  dsquare = dsquare1;
	  translator[j]=i;
	  if ( dsquare == 0 ) break;
	}
      }
    }
    gifimage.m = 0;
    gifscrn.pixbits = globscrn.pixbits;
    if (debugFlag) fprintf(stderr, "  translating picture no %d\n", count);
    for (i = wi * hi -1; i>=0; i--)
      pix[i]=translator[pix[i]];
    if ( minimize != 0 && pixold != NULL  && hi == gifimageold.height
	&& wi == gifimageold.width && gifimage.top == gifimageold.top
	&& gifimage.left == gifimageold.left ) {
      gifimageold = gifimage;
/* First test from left to right, top to bottom */
      p = pix; po = pixold;
      for (i = 0; i < hi; i++ ) {
	for (j = 0; j < wi; j++ ) {
	  if ( *p++ != *po++ ) {
	    left = j; top=i;
	    goto done;
	  }
	}
      }
      if (FALSE) { 
done: /* i.e. a preliminary left and top found */ ;
      }
      else goto alike; /* writes full image, should we drop it ? */
/* Then test from right to left, bottom to top */
      k=hi*wi-1; 
      p = &pix[k]; po = &pixold[k];
      for (i = hi-1; i >= top; i-- ) {
	for (j = wi -1; j >= 0; j-- ) {
	  if ( *p-- != *po-- ) {
	    right = j; bot=i;
	    goto botfound;
	  }
	}
      }
botfound:
/* The form of the differing area (not rectangle) may be slanted */
      if ( right < left ) {
	i = right; right = left; left = i;
      }
/* Now test between top and bottom at the left hand side */
      for (i = top+1; i <= bot; i++ ) {
	k= i * wi;
	p = &pix[k]; po = &pixold[k];
	for (j = 0; j < left; j++ ) {
	  if ( *p++ != *po++ ) {
	    left = j;
	    break;
	  }
	}
      }
/* Finally test between bottom and top at the right hand side */
      for (i = bot-1; i >= top; i-- ) {
	k= (i+1) * wi-1;
	p = &pix[k]; po = &pixold[k];
	for (j = wi-1; j > right; j-- ) {
	  if ( *p-- != *po-- ) {
	    right = j;
	    break;
	  }
	}
      }
      gifimage.left += left;
      gifimage.top += top;
      gifimage.width = right-left+1;
      gifimage.height = bot-top+1;
      WriteImageHeader(fout);
/* The rectangle containing diffs is transferred to the mem area of pixold */
      po = pixold;
      for (i = top; i <= bot; i++ ) {
	p = &pix[i * wi+left];
	for (j = left; j <= right; j++ ) {
	  *po++ = *p++;
	}
      }
      GifEncode(fout, pixold, gifscrn.pixbits+1, gifimage.height * gifimage.width);
      if (debugFlag)
	fprintf(stderr, "  encoded: width= %d, height = %d, left = %d, top = %d\n",
	   gifimage.width, gifimage.height, gifimage.left, gifimage.top);
    }
    else {
alike:
      WriteImageHeader(fout);
      gifimageold = gifimage;
      GifEncode(fout, pix, gifscrn.pixbits+1, gifimage.height * gifimage.width);
      if (debugFlag) fprintf(stderr, "  picture re-encoded\n");
    }
    free(pixold);
    pixold = pix;
    fputc(0, fout);    /* block count of zero */
  }
  else {
    WriteImageHeader(fout);
    i = Xgetc(fp); fputc(i, fout); /* the LZW code size */
    while ( ( gifBlockSize = Xgetc(fp) ) > 0 ) {
      fputc(gifBlockSize, fout);
      while ( gifBlockSize-- > 0 ) fputc(Xgetc(fp),fout);
    }
    if ( gifBlockSize == 0 ) fputc(gifBlockSize, fout);
    else TheEnd1("GifPassing: Unexpected End of File\n");
  }

  fclose(fp);
}



/*
 * read Gif header
 */
void GifScreenHeader(fp, fout, firstTime)
	    FILE *fp, *fout;
	    int firstTime;
{
  int temp, i;

  for(i = 0; i < 6; i++) {
    temp = Xgetc(fp);
    if(i == 4 && temp == '7') temp = '9';
    if (firstTime) fputc(temp, fout);
  }

  gifscrn.width = GifGetShort(fp);
  gifscrn.height = GifGetShort(fp);
  temp = Xgetc(fp);
  if (firstTime) {
    GifPutShort(gifscrn.width, fout);
    GifPutShort(gifscrn.height, fout);
    fputc(temp, fout);
  }
  gifscrn.m  =  temp & 0x80;
  gifscrn.cres   = (temp & 0x70) >> 4;
  gifscrn.pixbits =  temp & 0x07;

  gifscrn.bc  = Xgetc(fp);
  if (firstTime) {
    if (debugFlag) fprintf(stderr, "First Time ... ");
    if(GifBgcolor) gifscrn.bc = GifBgcolor & 0xff;
    fputc(gifscrn.bc, fout);
  }

  temp = Xgetc(fp);
  if (firstTime)  {
    fputc(temp, fout);
    if ( minimize && gifscrn.bc == 0 ) {
      /* Set a pseudo screen filled with the background color.
	 This is only done for background color index == 0 because
	 of Netscape and I.E.'s strange handling of backgrounds not
	 covered by an image.
      */
      temp = gifscrn.width * gifscrn.height;
      if (( pixold = (UBYTE *)malloc(temp * sizeof(UBYTE)) ) == NULL )
	    TheEnd1("No memory for image\n");
      if (debugFlag) fprintf(stderr, "BACKGROUND = %d\n", gifscrn.bc);
      while (temp > 0) pixold[--temp] = 0; /* gifscrn.bc; */
      gifimageold.left = gifimageold.top = 0;
      gifimageold.width = gifscrn.width;
      gifimageold.height = gifscrn.height;
      gifimageold.pixbits = gifscrn.pixbits;
    }
  }
  imagec = gifPtwo[(1+gifscrn.pixbits)];

  if (debugFlag)
    fprintf(stderr, "Screen #%d: %dx%dx%d m=%d cres=%d bkgnd=%d pix=%d\n",
      count, gifscrn.width, gifscrn.height, imagec, gifscrn.m, gifscrn.cres,
      gifscrn.bc, gifscrn.pixbits);

  if (gifscrn.m) {
    for(i = 0; i < imagec; i++) {
      gifCmap[i].cmap.red   = temp = Xgetc(fp);
      if (firstTime) fputc(temp, fout);
      gifCmap[i].cmap.green = temp = Xgetc(fp);
      if (firstTime) fputc(temp, fout);
      gifCmap[i].cmap.blue  = temp = Xgetc(fp);
      if (firstTime) fputc(temp, fout);

    if(firstTime && (global.trans.type==TRANS_RGB && global.trans.valid==0) )
      if (global.trans.red == gifCmap[i].cmap.red &&
	  global.trans.green == gifCmap[i].cmap.green &&
	  global.trans.blue == gifCmap[i].cmap.blue) {
	if(debugFlag > 1) fprintf(stderr, " Transparent match at %d\n", i);
	global.trans.map = i;
	global.trans.valid = 1;
      }
      else
	if(debugFlag > 1) fprintf(stderr, "No transp. RGB=(%x,%x,%x)\n",
	 gifCmap[i].cmap.red, gifCmap[i].cmap.green, gifCmap[i].cmap.blue);
    }
  }
}

void ReadImageHeader(fp)
	       FILE *fp;
{
  int tnum, i, flag;

  gifimage.left  = GifGetShort(fp);
  if(global.left) gifimage.left += global.left;

  gifimage.top   = GifGetShort(fp);
  if(global.top) gifimage.top += global.top;

  gifimage.width     = GifGetShort(fp);
  gifimage.height = GifGetShort(fp);
  flag = Xgetc(fp);

  gifimage.i       = flag & 0x40;
  gifimage.pixbits = flag & 0x07;
  gifimage.m       = flag & 0x80;

  imagex = gifimage.width;
  imagey = gifimage.height;
  tnum = gifPtwo[(1+gifimage.pixbits)];
  if (debugFlag > 1)
    fprintf(stderr, "Image: %dx%dx%d (%d,%d) m=%d i=%d pix=%d \n",
      imagex, imagey, tnum, gifimage.left, gifimage.top,
      gifimage.m, gifimage.i, gifimage.pixbits);

   /* if there is an image cmap then read it */
  if (gifimage.m) {
    if(debugFlag)
      fprintf(stderr, "DEBUG:Transferring colormap of %d colors\n",
	imagec);
  /*
   * note below assignment, it may make the subsequent code confusing
   */
    gifscrn.pixbits = gifimage.pixbits;

    for(i = 0; i < tnum; i++) {
      gifCmap[i].cmap.red   = Xgetc(fp);
      gifCmap[i].cmap.green = Xgetc(fp);
      gifCmap[i].cmap.blue  = Xgetc(fp);
    }
  }
  gifimage.m = 0;
  if ( globscrn.m && globscrn.pixbits == gifscrn.pixbits ) {
    for (i = gifMask[1+globscrn.pixbits]; i >= 0; i--) {
      if (gifGmap[i].cmap.red  != gifCmap[i].cmap.red ||
      gifGmap[i].cmap.green != gifCmap[i].cmap.green ||
      gifGmap[i].cmap.blue != gifCmap[i].cmap.blue ) {
	gifimage.m = 0x80;
	break;
      }
    }
  }
  else gifimage.m = 0x80;
  return;
}

void WriteImageHeader(fout)
		FILE *fout;
{
  int temp, i, flag;
  /* compute a Gif_GCL */

  fputc(0x21, fout);
  fputc(0xF9, fout);
  fputc(0x04, fout);

  flag = global.disposal <<2;
  if(global.time) flag |= 0x80;
  if(global.trans.type == TRANS_RGB && global.trans.valid == 0)
	gifimage.m = 0x80;

  temp = global.trans.map;
  if (gifimage.m != 0 && global.trans.type == TRANS_RGB ) {
    temp = 0; /* set a default value, in case nothing found */
    for (i = gifMask[1+gifscrn.pixbits]; i >= 0; i--) {
      if(global.trans.red == gifCmap[i].cmap.red &&
      global.trans.green == gifCmap[i].cmap.green &&
      global.trans.blue == gifCmap[i].cmap.blue) {
	if(debugFlag > 1) fprintf(stderr, " Transparent match at %d\n", i);
	temp = i;
	flag |= 0x01;
      }
    }
  }
  else if(global.trans.valid) flag |= 0x01;
  fputc(flag, fout);

  GifPutShort(global.time, fout); /* the delay speed - 0 is instantaneous */

  fputc(temp, fout); /* the transparency index */
  if(debugFlag > 1) {
    fprintf(stderr, "GCL: delay %d", global.time);
    if(flag && 0x1) fprintf(stderr, " Transparent: %d", temp);
    fputc('\n', stderr);
  }

  fputc(0, fout);
  /* end of Gif_GCL */

  fputc(',', fout); /* image separator */
  GifPutShort(gifimage.left  , fout);
  GifPutShort(gifimage.top   , fout);
  GifPutShort(gifimage.width , fout);
  GifPutShort(gifimage.height, fout);
  fputc(gifscrn.pixbits | gifimage.i | gifimage.m, fout);

  if ( gifimage.m ) {
    for(i = 0; i < imagec; i++) {
      fputc(gifCmap[i].cmap.red,   fout);
      fputc(gifCmap[i].cmap.green, fout);
      fputc(gifCmap[i].cmap.blue,  fout);
    }
    if(debugFlag) fprintf(stderr, "Local %d color map for picture #%d\n",
			   imagec, count);
  }
}


void GifComment(fout, string)
	   FILE *fout;
	   char *string;
{
  int len;

  if( (len = strlen(string)) > 254 ) fprintf(stderr,
		 "GifComment: String too long ; dropped\n");
  else if ( len > 0 ) {
    /* Undocumented feature:
       Empty comment string means no comment block in outfile */
    fputc(0x21, fout);
    fputc(0xFE, fout);
    fputc(len, fout);
    fputs(string, fout);
    fputc(0, fout);
  }
  return;
}

/*
 * Write a Netscape loop marker.
 */
void GifLoop(fout, repeats)
	FILE *fout;
	unsigned int repeats;
{

  fputc(0x21, fout);
  fputc(0xFF, fout);
  fputc(0x0B, fout);
  fputs("NETSCAPE2.0", fout);

  fputc(0x03, fout);
  fputc(0x01, fout);
  GifPutShort(repeats, fout); /* repeat count */

  fputc(0x00, fout); /* terminator */

  if(debugFlag) fprintf(stderr, "Wrote loop extension\n");
}


void CalcTrans(string)
	  char *string;
{
  if(string[0] != '#') {
    global.trans.type = TRANS_MAP;
    global.trans.map = atoi(string);
    global.trans.valid = 1;
  }
  else {
    /* it's an RGB value */
    int r, g, b;
    string++;
    if (debugFlag > 1) fprintf(stderr, "String is %s\n", string);
    if(3 == sscanf(string, "%2x%2x%2x", &r, &g, &b)) {
      global.trans.red = r;
      global.trans.green = g;
      global.trans.blue = b;
      global.trans.type = TRANS_RGB;
      global.trans.valid = 0;
      if(debugFlag > 1)
	fprintf(stderr, "Transparent RGB=(%x,%x,%x) = (%d,%d,%d)\n",
	    r, g, b, r, g, b);
    }
  }
  if(debugFlag > 1)
       fprintf(stderr, "DEBUG:CalcTrans is %d\n", global.trans.type);
}

void TheEnd()
{
 exit(0);
}

void TheEnd1(p)
char *p;
{
 fprintf(stderr, "%s", p);
 TheEnd();
}

UBYTE Xgetc(fin)
   FILE *fin;
{
  int i;
  if ( ( i = fgetc(fin) ) == EOF ) {
    char a[100];
    sprintf(a, "Unexpected EOF in input file %d\n", count); 
    TheEnd1(a);
  }
  return(i & 0xff);
}
