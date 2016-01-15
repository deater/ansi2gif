/*
 * gifencode.c
 *
 * Copyright (c) 1997,1998 by Hans Dinsen-Hansen
 * The algorithms are inspired by those of gifcode.c
 * Copyright (c) 1995,1996 Michael A. Mayer
 * All rights reserved.
 *
 * gifdecode.c
 *
 * Copyright (c) 1997,1998 by Hans Dinsen-Hansen
 * Copyright (c) 1995,1996 by Kevin Kadow
 * All rights reserved.
 *
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

/*           Implements GIF encoding by means of a tree search.
 *           --------------------------------------------------
 *
 *  - The string table may be thought of being stored in a "b-tree of
 * steroids," or more specifically, a {256,128,...,4}-tree, depending on
 * the size of the color map.
 *  - Each (non-NULL) node contains the string table index (or code) and
 * {256,128,...,4} pointers to other nodes.
 *  - For example, the index associated with the string 0-3-173-25 would be
 * stored in:
 *       first->node[0]->node[3]->node[173]->node[25]->code
 *
 *  - Speed and effectivity considerations, however, have made this
 * implementation somewhat obscure, because it is costly to initialize
 * a node-array where most elements will never be used.
 *  - Initially new nodes will be marked as terminating, TERMIN.
 * If this node is used at a later stage, its mark will be changed.
 *  - Only nodes with several used nodes will be associated with a
 * node-array.  Such nodes are marked LOOKUP.
 *  - The remaining nodes are marked SEARCH.  They are linked together
 * in a search-list, where a field, NODE->alt, points at an alternative
 * following color.
 *  - It is hardly feasible exactly to predict which nodes will have most
 * used node pointers.  The theory here is that the very first node as
 * well as the first couple of nodes which need at least one alternative
 * color, will be among the ones with many nodes ("... whatever that
 * means", as my tutor in Num. Analysis and programming used to say).
 *  - The number of possible LOOKUP nodes depends on the size of the color 
 * map.  Large color maps will have many SEARCH nodes; small color maps
 * will probably have many LOOKUP nodes.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


/* define constants and defaults */
/* If set to 1, Netscape 'loop' code will be added by default */
#define DEFAULT_LOOP 0
/* If set to 1, use the colormaps from all images, not just the first */
#define DEFAULT_USE_COLORMAP 0

/* Used in calculating the transparent color */
#define TRANS_NONE 1
#define TRANS_RGB 2
#define TRANS_MAP 3

#define DISP_NONE 0
#define DISP_NOT  1
#define DISP_BACK 2
#define DISP_PREV 3
#define DEFAULT_DISPOSAL DISP_NONE
    /* set default disposal method here to any of the DISP_XXXX values */


/* ? */
#define BIGSTRING 256
#define MAXVAL  4100        /* maxval of lzw coding size */
#define MAXVALP 4200
#define TERMIN 'T'
#define LOOKUP 'L'
#define SEARCH 'S'

#define noOfArrays 20
/* defines the amount of memory set aside in the encoding for the
 * LOOKUP type nodes; for a 256 color GIF, the number of LOOKUP
 * nodes will be <= noOfArrays, for a 128 color GIF the number of
 * LOOKUP nodes will be <= 2 * noOfArrays, etc.  */

typedef struct GifTree {
	char typ;	/* terminating, lookup, or search */
	int code;	/* the code to be output */
	uint8_t ix;	/* the color map index */
	struct GifTree **node, *nxt, *alt;
} GifTree;

/* definition of various structures */
typedef struct Transparency {
	int type;
	uint8_t valid;
	uint8_t map;
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} Transparency;

typedef struct Global {
	Transparency trans;
	int left;
	int top;
	unsigned int time;
	unsigned short disposal;
} Global;

typedef struct GifScreenHdr {
	int width;
	int height;
	uint8_t m;
	uint8_t cres;
	uint8_t pixbits;
	uint8_t bc;
	uint8_t aspect;
} GifScreenHdr;

typedef union GifColor {
	struct cmap {
		uint8_t red;
		uint8_t green;
		uint8_t blue;
		uint8_t pad;
	} cmap;
	uint32_t pixel;
} GifColor;

typedef struct GifImageHdr {
	int left;
	int top;
	int width;
	int height;
	uint8_t m;
	uint8_t i;
	uint8_t pixbits;
	uint8_t reserved;
} GifImageHdr;

typedef struct GifTable {
	uint8_t valid;
	uint8_t data;
	uint8_t first;
	uint8_t res;
	int last;
} GifTable;

static unsigned int debugFlag=0;
static int chainlen = 0, maxchainlen = 0, nodecount = 0, lookuptypes = 0;
static short need = 8;
static GifTree *empty[256], GifRoot = {LOOKUP, 0, 0, empty, NULL, NULL},
	*topNode, *baseNode, **nodeArray, **lastArray;

static GifTable table[MAXVALP];
static uint8_t gifBuff[MAXVALP];

static uint32_t rootCodeSize, codeSize, CLEAR, EOI, INCSIZE, nextab, gifBlockSize,
      gifMask[16]={0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,0,0},
      gifPtwo[16]={1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,0,0};

static int numBits, bits, picI, pass, step[5]={7,7,3,1,0}, start[5]= {0,4,2,1,0};
static uint8_t *picture;


/*
 * Set some defaults, these can be changed on the command line
 */
static unsigned int loop=DEFAULT_LOOP, loopcount=0,
	 globmap=0, minimize=0;

static int imagex = 0, imagey = 0, imagec = 0, GifBgcolor=0, count=0;

/* global settings for offset, transparency */

static Global global;

static GifColor gifGmap[256], gifCmap[256];
static GifScreenHdr globscrn, gifscrn;

static GifImageHdr gifimage, gifimageold;

static uint8_t *pixold=NULL;


#define BLOKLEN 255
#define BUFLEN 1000

static uint8_t Xgetc(FILE *fin) {

	int i;
	if ( ( i = fgetc(fin) ) == EOF ) {
		fprintf(stderr, "Unexpected EOF in input file %d\n", count);
		exit(0);
	}
	return(i & 0xff);
}



static void GifPutShort(int i, FILE *fout) {
	fputc(i&0xff, fout);
	fputc(i>>8, fout);
}

static unsigned short GifGetShort(FILE *fin) {
	return (Xgetc(fin) | Xgetc(fin)<<8);
}



static void ClearTree(int cc, GifTree *root) {

	int i;
	GifTree *newNode, **xx;

	if (debugFlag) {
		fprintf(stderr, "Clear Tree  cc= %d\n", cc);
	}
	if (debugFlag) {
		fprintf(stderr, "nodeCount = %d lookup nodes = %d\n", nodecount, lookuptypes);
	}

	maxchainlen=0; lookuptypes = 1;
	nodecount = 0;
	nodeArray = root->node;
	xx= nodeArray;
	for (i = 0; i < noOfArrays; i++ ) {
		memmove (xx, empty, 256*sizeof(GifTree **));
		xx += 256;
	}
	topNode = baseNode;
	for(i=0; i<cc; i++) {
		root->node[i] = newNode = ++topNode;
		newNode->nxt = NULL;
		newNode->alt = NULL;
		newNode->code = i;
		newNode->ix = i;
		newNode->typ = TERMIN;
		newNode->node = empty;
		nodecount++;
	}
}


/*
 * read Gif header
 */
static void GifScreenHeader(FILE *fp, FILE *fout, int firstTime) {

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
			if (( pixold = (uint8_t *)malloc(temp * sizeof(uint8_t)) ) == NULL ) {
				fprintf(stderr,"No memory for image\n");
				exit(1);
			}
			if (debugFlag) {
				fprintf(stderr, "BACKGROUND = %d\n", gifscrn.bc);
			}
			while (temp > 0) pixold[--temp] = 0; /* gifscrn.bc; */
			gifimageold.left = gifimageold.top = 0;
			gifimageold.width = gifscrn.width;
			gifimageold.height = gifscrn.height;
			gifimageold.pixbits = gifscrn.pixbits;
		}
	}
	imagec = gifPtwo[(1+gifscrn.pixbits)];

	if (debugFlag) {
		fprintf(stderr, "Screen #%d: %dx%dx%d m=%d cres=%d bkgnd=%d pix=%d\n",
			count, gifscrn.width, gifscrn.height, imagec, gifscrn.m, gifscrn.cres,
			gifscrn.bc, gifscrn.pixbits);
	}

	if (gifscrn.m) {
		for(i = 0; i < imagec; i++) {
			gifCmap[i].cmap.red   = temp = Xgetc(fp);
			if (firstTime) fputc(temp, fout);
			gifCmap[i].cmap.green = temp = Xgetc(fp);
			if (firstTime) fputc(temp, fout);
			gifCmap[i].cmap.blue  = temp = Xgetc(fp);
			if (firstTime) fputc(temp, fout);

			if(firstTime && (global.trans.type==TRANS_RGB && global.trans.valid==0) ) {
				if (global.trans.red == gifCmap[i].cmap.red &&
	  				global.trans.green == gifCmap[i].cmap.green &&
	  				global.trans.blue == gifCmap[i].cmap.blue) {
					if(debugFlag > 1) fprintf(stderr, " Transparent match at %d\n", i);
					global.trans.map = i;
					global.trans.valid = 1;
      				}
				else {
					if(debugFlag > 1) fprintf(stderr, "No transp. RGB=(%x,%x,%x)\n",
								gifCmap[i].cmap.red, gifCmap[i].cmap.green, gifCmap[i].cmap.blue);
				}
			}
		}
	}
}


static char *AddCodeToBuffer(int code, short n, char *buf) {

	int    mask;

	if (n<0) {
		if (need<8) {
			buf++;
			*buf = 0x0;
		}
		need = 8;
		return buf;
	}

	while(n>=need) {
		mask = (1<<need)-1;
		*buf += (mask&code)<<(8-need);
		buf++;
		*buf = 0x0;
		code = code>>need;
		n -= need;
		need = 8;
	}

	if (n) {
		mask = (1<<n)-1;
		*buf += (mask&code)<<(8-need);
		need -= n;
	}
	return buf;
}


static void GifEncode(FILE *fout, uint8_t *pixels, int depth, int siz) {


	GifTree *first = &GifRoot, *newNode, *curNode;
	uint8_t   *end;
	int     cc, eoi, next, tel=0;
	short   cLength;

	char    *pos, *buffer;

	empty[0] = NULL;
	need = 8;

	nodeArray = empty;
	memmove(++nodeArray, empty, 255*sizeof(GifTree **));
	if (( buffer = (char *)malloc((BUFLEN+1)*sizeof(char))) == NULL ) {
		fprintf(stderr,"No memory for writing");
		exit(0);
	}
	buffer++;

	pos = buffer;
	buffer[0] = 0x0;

	cc = (depth == 1) ? 0x4 : 1<<depth;
	fputc((depth == 1) ? 2 : depth, fout);
	eoi = cc+1;
	next = cc+2;

	cLength = (depth == 1) ? 3 : depth+1;

	if (( topNode = baseNode = (GifTree *)malloc(sizeof(GifTree)*4094)) == NULL ) {
		fprintf(stderr,"No memory for GIF-code tree");
		exit(0);
	}

	if (( nodeArray = first->node = (GifTree **)malloc(256*sizeof(GifTree *)*noOfArrays)) == NULL ) {
		fprintf(stderr,"No memory for search nodes");
		exit(0);
	}

	lastArray = nodeArray + ( 256*noOfArrays - cc);
	ClearTree(cc, first);

	pos = AddCodeToBuffer(cc, cLength, pos);

	end = pixels+siz;
	curNode = first;
	while(pixels < end) {

		if ( curNode->node[*pixels] != NULL ) {
			curNode = curNode->node[*pixels];
			tel++;
			pixels++;
			chainlen++;
			continue;
		} else if ( curNode->typ == SEARCH ) {
			newNode = curNode->nxt;
			while ( newNode->alt != NULL ) {
				if ( newNode->ix == *pixels ) break;
				newNode = newNode->alt;
			}
			if (newNode->ix == *pixels ) {
				tel++;
				pixels++;
				chainlen++;
				curNode = newNode;
				continue;
			}
		}

/* ******************************************************
 * If there is no more thread to follow, we create a new node.  If the
 * current node is terminating, it will become a SEARCH node.  If it is
 * a SEARCH node, and if we still have room, it will be converted to a
 * LOOKUP node.
*/
  newNode = ++topNode;
  switch (curNode->typ ) {
   case LOOKUP:
     newNode->nxt = NULL;
     newNode->alt = NULL,
     curNode->node[*pixels] = newNode;
   break;
   case SEARCH:
     if ( nodeArray != lastArray ) {
       nodeArray += cc;
       curNode->node = nodeArray;
       curNode->typ = LOOKUP;
       curNode->node[*pixels] = newNode;
       curNode->node[(curNode->nxt)->ix] = curNode->nxt;
       lookuptypes++;
       newNode->nxt = NULL;
       newNode->alt = NULL,
       curNode->nxt = NULL;
       break;
     }
/*   otherwise do as we do with a TERMIN node  */
   case TERMIN:
     newNode->alt = curNode->nxt;
     newNode->nxt = NULL,
     curNode->nxt = newNode;
     curNode->typ = SEARCH;
     break;
   default:
     fprintf(stderr, "Silly node type: %d\n", curNode->typ);
  }
  newNode->code = next;
  newNode->ix = *pixels;
  newNode->typ = TERMIN;
  newNode->node = empty;
  nodecount++;
/*
* End of node creation
* ******************************************************
*/
  if (debugFlag) {
    if (curNode == newNode) fprintf(stderr, "Wrong choice of node\n");
    if ( curNode->typ == LOOKUP && curNode->node[*pixels] != newNode ) fprintf(stderr, "Wrong pixel coding\n");
    if ( curNode->typ == TERMIN ) fprintf(stderr, "Wrong Type coding; frame no = %d; pixel# = %d; nodecount = %d\n", count, tel, nodecount);
  }
    pos = AddCodeToBuffer(curNode->code, cLength, pos);
    if ( chainlen > maxchainlen ) maxchainlen = chainlen;
    chainlen = 0;
    if(pos-buffer>BLOKLEN) {
      buffer[-1] = BLOKLEN;
      fwrite(buffer-1, 1, BLOKLEN+1, fout);
      buffer[0] = buffer[BLOKLEN];
      buffer[1] = buffer[BLOKLEN+1];
      buffer[2] = buffer[BLOKLEN+2];
      buffer[3] = buffer[BLOKLEN+3];
      pos -= BLOKLEN;
    }
    curNode = first;

    if(next == (1<<cLength)) cLength++;
    next++;

    if(next == 0xfff) {
      ClearTree(cc,first);
      pos = AddCodeToBuffer(cc, cLength, pos);
      if(pos-buffer>BLOKLEN) {
	buffer[-1] = BLOKLEN;
	fwrite(buffer-1, 1, BLOKLEN+1, fout);
	buffer[0] = buffer[BLOKLEN];
	buffer[1] = buffer[BLOKLEN+1];
	buffer[2] = buffer[BLOKLEN+2];
	buffer[3] = buffer[BLOKLEN+3];
	pos -= BLOKLEN;
      }
      next = cc+2;
      cLength = (depth == 1)?3:depth+1;
    }
  }

  pos = AddCodeToBuffer(curNode->code, cLength, pos);
  if(pos-buffer>BLOKLEN-3) {
    buffer[-1] = BLOKLEN-3;
    fwrite(buffer-1, 1, BLOKLEN-2, fout);
    buffer[0] = buffer[BLOKLEN-3];
    buffer[1] = buffer[BLOKLEN-2];
    buffer[2] = buffer[BLOKLEN-1];
    buffer[3] = buffer[BLOKLEN];
    buffer[4] = buffer[BLOKLEN+1];
    pos -= BLOKLEN-3;
  }
  pos = AddCodeToBuffer(eoi, cLength, pos);
  pos = AddCodeToBuffer(0x0, -1, pos);
  buffer[-1] = pos-buffer;

  fwrite(buffer-1, pos-buffer+1, 1, fout);
  free(buffer-1); free(first->node); free(baseNode);
  if (debugFlag) fprintf(stderr, "pixel count = %d; nodeCount = %d lookup nodes = %d\n", tel, nodecount, lookuptypes);
  return;

}

/*
 * Write a Netscape loop marker.
 */
static void GifLoop(FILE *fout, unsigned int repeats) {

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



static long sq(uint8_t i,uint8_t j) {
	return((i-j)*(i-j));
}

static void ReadImageHeader(FILE *fp) {
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

/*
 * clear and initialize string table
 */
static void GifClearTable(void) {
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

/* get code depending of current LZW code size */
static uint32_t GifGetCode(FILE *fp) {

  uint32_t code;
  int tmp;

  while(numBits < codeSize) {
    /**** if at end of a block, start new block */
    if (gifBlockSize == 0) gifBlockSize = fgetc(fp);

    tmp = fgetc(fp);
    gifBlockSize--;
    bits |= ( ((uint32_t)(tmp) & 0xff) << numBits );
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

static uint8_t *GifSendData(int index, uint8_t *pix, GifImageHdr gifimage) {
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
      exit(0);
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

static void GifGetNextEntry(FILE *fp) {

   /* table walk to empty spot */
  while( (table[nextab].valid == 1) &&(nextab < MAXVAL) ) nextab++;
  /*
   * Ran out of space?! Something's gone sour...
   */
  if (nextab >= MAXVAL) {
    fprintf(stderr, "Error: GetNext nextab=%d\n", nextab);
    fclose(fp);
    exit(0);
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

static void GifAddToTable(int32_t body, int32_t next, int32_t index) {

  if (body > MAXVAL || next > MAXVAL || index > MAXVAL ) fprintf(stderr, "Error body=%d, next=%d, index=%d, \n", index,index,index);
  else {
    table[index].valid = 1;
    table[index].data = table[next].first;
    table[index].first = table[body].first;
    table[index].last = body;
  }
}



static void GifDecode(FILE  *fp, uint8_t *pix, GifImageHdr gifimage) {
  uint32_t code, old;
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

static void WriteImageHeader(FILE *fout) {
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


/* time in 100th of seconds */
void animate_gif(FILE *fout,char *fname,int firstImage,int Xoff,int Yoff,
		 int delay_time,int loop_val) {

	FILE *fp;
	uint8_t *pix;
	int i;

	if ( (fp = fopen(fname, "rb")) == 0) {
		fprintf(stderr, "Can't open %s for reading.\n", fname);
		exit(0);
	}

	global.trans.type = TRANS_NONE;
	global.trans.valid = 0;
	global.time = delay_time;

	global.disposal = DEFAULT_DISPOSAL;
	global.left = Xoff;
	global.top = Yoff;

	loop=loop_val;

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
	if ( feof(fp) || i == ';' ) {
		fprintf(stderr,"GifReadHeader: Unexpected End of File\n");
		exit(0);
	}
	fprintf(stderr,"GifReadHeader: Unknown block type\n");
	exit(0);
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
    uint8_t translator[256], *p, *po;
    int left, right=0, top, bot=0, i, j, k, l, hi, wi;
    long dsquare, dsquare1;
    hi = gifimage.height;
    wi = gifimage.width;
    if (( pix = (uint8_t *)malloc(wi * hi * sizeof(uint8_t)) ) == NULL ) {
	 fprintf(stderr,"No memory for image\n");
	exit(0);
	}
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
      if (0) { 
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
    else {
	fprintf(stderr,"GifPassing: Unexpected End of File\n");
	exit(0);
	}
  }

  fclose(fp);
}
