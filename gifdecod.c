/*
 * gifdecode.c
 *
 * Copyright (c) 1997,1998 by Hans Dinsen-Hansen
 * Copyright (c) 1995,1996 by Kevin Kadow
 * All rights reserved.
 *
 * This software may be freely copied, modified and redistributed
 * without fee provided that this copyright notice is preserved
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

#include "whirlgif.h"

extern unsigned int debugFlag, verbose;
extern int count;

GifTable table[MAXVALP];

UBYTE gifBuff[MAXVALP];

ULONG rootCodeSize, codeSize, CLEAR, EOI, INCSIZE, nextab, gifBlockSize,
      gifMask[16]={0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,0,0},
      gifPtwo[16]={1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,0,0};

int numBits, bits, picI, pass, step[5]={7,7,3,1,0}, start[5]= {0,4,2,1,0};
UBYTE *picture;

void GifDecode(fp, pix, gifimage)
	FILE  *fp;
	UBYTE *pix;
	GifImageHdr gifimage;
{
  ULONG code, old, k;
  picture=pix;
  picI = pass = bits = 0;
  numBits = 0;
  gifBlockSize = 0;
    /* starting code size of LZW */
  rootCodeSize = (Xgetc(fp) & 0xff);
  GifClearTable();           /* clear decoding symbol table */

  code = GifGetCode(fp);

  if (code == CLEAR) {
    GifClearTable();
    code = GifGetCode(fp);
  }
  /* put code(or what it currently stands for) to file */
  pix = GifSendData(code, pix, gifimage);
  old = code;
  code = GifGetCode(fp);
  do {
    if (table[code].valid == 1) {
       /* if known code, send it's associated string to file */
      pix = GifSendData(code, pix, gifimage);
      GifGetNextEntry(fp);     /* get next table entry (nextab) */
      GifAddToTable(old, code, nextab);  /* add old+code to table */
      old = code;
    }
    else {
      /* code doesn't exist */
      GifAddToTable(old, old, code);   /* add old+old to table */
      pix = GifSendData(code, pix, gifimage);
      old = code;
    }
    code = GifGetCode(fp);
    if (code == CLEAR) {
      GifClearTable();
      code = GifGetCode(fp);
      pix = GifSendData(code, pix, gifimage);
      old = code;
      code = GifGetCode(fp);
    }
  } while(code != EOI);
  if (debugFlag) fprintf(stderr, " picI = %d\n", picI);
  return;
}

void GifGetNextEntry(fp)
    FILE                *fp;
{
   /* table walk to empty spot */
  while( (table[nextab].valid == 1) &&(nextab < MAXVAL) ) nextab++;
  /*
   * Ran out of space?! Something's gone sour...
   */
  if (nextab >= MAXVAL) {
    fprintf(stderr, "Error: GetNext nextab=%d\n", nextab);
    fclose(fp);
    TheEnd();
  }
  if (nextab == INCSIZE) {
    /* go to next table size (and LZW code size ) */
    /* fprintf(stderr, "GetNext INCSIZE was %d ", nextab); */
    codeSize++;
    INCSIZE = (INCSIZE*2)+1;
    if (codeSize >= 12) codeSize = 12;
/*  fprintf(stderr, "<%d>", INCSIZE); */
  }

}

void GifAddToTable(body, next, index)
       ULONG body, next, index;
{
  if (body > MAXVAL || next > MAXVAL || index > MAXVAL ) fprintf(stderr, "Error body=%d, next=%d, index=%d, \n", index);
  else {
    table[index].valid = 1;
    table[index].data = table[next].first;
    table[index].first = table[body].first;
    table[index].last = body;
  }
}

UBYTE *GifSendData(index, pix, gifimage)
	int index;
	UBYTE *pix;
	GifImageHdr gifimage;
{
  int i, j, interlaced, imgwidth, imgheight;
  interlaced = gifimage.i;
  imgwidth = gifimage.width;
  imgheight = gifimage.height;
  i = 0;
  do {
    /* table walk to retrieve string associated with index */
    gifBuff[i] = table[index].data;
    i++;
    index = table[index].last;
    if (i > MAXVAL) {
      fprintf(stderr, "Error: Sending i=%d index=%d\n", i, index);
      TheEnd();
    }
  } while(index >= 0);

  /* now invert that string since we retreived it backwards */
  i--;
  for(j = i; j >= 0; j--) {
    picI++;
    *pix = gifBuff[j]; pix++;
    if (interlaced) {
      if ( picI % imgwidth == 0 )
      {
	picI += ( imgwidth * step[pass]);
	if (picI >= imgwidth * imgheight)
	{
	  picI = start[++pass] * imgwidth;
	}
	pix = &picture[picI];
      }
    }
  }
  return(pix);
}

/*
 * clear and initialize string table
 */
void GifClearTable()
{
  int maxi, i;
  if (debugFlag) fprintf(stderr, "Clearing Table...\n");
  for(i = 0; i < MAXVAL; i++) table[i].valid = 0;
  if (debugFlag) fprintf(stderr, "Initing Table...");
  maxi = gifPtwo[rootCodeSize];
  for(i = 0; i < maxi; i++) {
    table[i].data = i;
    table[i].first = i;
    table[i].valid = 1;
    table[i].last = -1;
  }
  CLEAR = maxi;
  nextab = maxi +2;
  EOI = maxi+1;
  INCSIZE = (2*maxi)-1;
  codeSize = rootCodeSize+1;
}

/*CODE*/
ULONG GifGetCode(fp) /* get code depending of current LZW code size */
	    FILE *fp;
{
  ULONG code;
  int tmp;

  while(numBits < codeSize) {
    /**** if at end of a block, start new block */
    if (gifBlockSize == 0) gifBlockSize = fgetc(fp);

    tmp = fgetc(fp);
    gifBlockSize--;
    bits |= ( ((ULONG)(tmp) & 0xff) << numBits );
    numBits += 8;
  }

  code = bits & gifMask[codeSize];
  bits >>= codeSize;
  numBits -= codeSize;

  if (code > MAXVAL) {
    fprintf(stderr, "\nError! in stream=%x \n", code);
    fprintf(stderr, "CLEAR=%x INCSIZE=%x EOI=%x codeSize=%x \n",
		 CLEAR, INCSIZE, EOI, codeSize);
    code = EOI;
  }
  if (code == INCSIZE) {
    if (codeSize < 12) {
      codeSize++;
      INCSIZE = (INCSIZE*2)+1;
    }
    else if (debugFlag) fprintf(stderr, "<13?>");
  }

  return(code);
}

