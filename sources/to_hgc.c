/*
   Copyright (c) 1993 Roland Emmerich & Heinz W. Werntges. All rights reserved.
   Distributed by Free Software Foundation, Inc.

This file is part of HP2xx.

HP2xx is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves any
particular purpose or works at all, unless he says so in writing.  Refer
to the GNU General Public License, Version 2 or later, for full details.

Everyone is granted permission to copy, modify and redistribute
HP2xx, but only under the conditions described in the GNU General Public
License.  A copy of this license is supposed to have been
given to you along with HP2xx so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/

/** to_hgc.c: HGC preview part of project "hp2xx" (PC only)
 **
 ** 93/06/16  V 1.00  RE   Originating
 ** 93/07/25  V 1.01  HWW  Minor adaptations to hp2xx standard
 **/

/**
 ** NOTE:
 **	For white lines on a black background, uncomment the next
 **	#define statement:
 **/
/* #define	HGC_INVERSE */

#define	HGC_XMAX	720
#define	HGC_YMAX	348


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dos.h>
#include <mem.h>
#include <graphics.h>
#include <conio.h>
#include "bresnham.h"
#include "hp2xx.h"

/**
 ** HGC registers
 **/

static unsigned int IndexReg  = 0x03B4;
static unsigned int DatenReg  = 0x03B5;
static unsigned int ModusReg  = 0x03B8;
static unsigned int KonfigReg = 0x03Bf;

/**
 ** Pointer to HGC video buffer (4 memory banks)
 **/

static unsigned int HGCBank[] = {0xB000, 0xB200, 0xB400, 0xB600};

/**
 ** set_pixel_HGC(x, y) set point at x, y
 **/

static void	set_pixel_HGC(int x, int y)
{
  unsigned int bank;
  unsigned int pkt_addr;
  unsigned char far *screen;
  unsigned char pixel[]  = {128, 64, 32, 16, 8, 4, 2, 1};

 /**
  ** The pixels are put directly into the screen buffer of the HGC,
  ** therefore it is necessary to check the address range
  **/

  if ((x >= 0) && (x < HGC_XMAX) && (y >= 0) && (y < HGC_YMAX))
  {
	bank = y % 4;
	pkt_addr = x >> 3;
	pkt_addr += 90 * ((y-bank) >> 2);

	screen = MK_FP (HGCBank[bank], pkt_addr);
#ifdef	HGC_INVERSE
	screen[0] |= pixel[x % 8];
#else
	screen[0] &= ~pixel[x % 8];
#endif
  }
}



/**
 ** switch to HGC text mode
 **/

static void	set_HGC_textmode(void)
{
unsigned char i;
unsigned int j;
unsigned int far *screen;
unsigned char TextModus[] = {
		0x61, 0x50, 0x52, 0x0f, 0x19, 0x06,
		0x19, 0x19, 0x02, 0x0d, 0x0b, 0x0c,
		0x00, 0x00, 0x00, 0x01
};

  outportb (KonfigReg, 0x00);   /** Partial access to HGC         **/
  outportb (ModusReg,  0x20);   /** Set text mode and turn off    **/
				/**    the display	          **/
  for (i=0 ; i < 16; i++)
  {
	outportb (IndexReg, i);	/** set registers to textmode     **/
	outportb (DatenReg, TextModus[i]);
  }
  screen = MK_FP (HGCBank[0], 0);	/** Erase text screen memory,	**/
  for (j = 0 ; j < 0x2000; j++)		/** i.e. fill with spaces 0x20	**/
	screen[j]   = 0x0720;		/** and attribut "white" 0x07.	**/

  outportb (ModusReg,  0x28);		/** Turn on the display		**/
}

/**
 ** Switch to HGC graphic mode
 **/

static void	set_HGC_graphmode(void)
{
unsigned char i;
unsigned int  j,k, bkgrnd;
unsigned int far *screen;
unsigned char GrafikModus[] = {
		0x35, 0x2d, 0x2e, 0x07, 0x5b, 0x02,
		0x57, 0x57, 0x02, 0x03, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x01
};

  outportb (KonfigReg, 0x03);	/** Full access to HGC		**/
  outportb (ModusReg,  0x02);	/** Switch to graphics mode     **/
				/**   and turn off the display	**/
  for (i = 0; i < 16; i++)
  {
	outportb (IndexReg, i);
	outportb (DatenReg, GrafikModus[i]);
  }


#ifdef	HGC_INVERSE
  bkgrnd = 0x0000;
#else
  bkgrnd = 0xFFFF;
#endif

  for (k=0; k < 4 ; k++)
    for (j = 0 ; j < 0x2000; j++)
    {
	screen = MK_FP (HGCBank[k], 0);	/** Erase graphics memory **/
	screen[j] = bkgrnd;
    }
  outportb (ModusReg,  0x0A); 		/** Turn on display	**/
}


/**
 ** Main entry point
 **/

void	PicBuf_to_HGC(PicBuf *picbuf, PAR *p)
{
int row_c, x, y, xoff, yoff, color_index;
RowBuf *row;

  if (!p->quiet)
  {
	fprintf(stderr, "\nHGCpreview follows.\n");
	fprintf(stderr, "Press <return> to start and end graphics mode\n");
	SilentWait();
  }

  xoff = p->xoff * p->dpi_x / 25.4;
  yoff = p->yoff * p->dpi_y / 25.4;

  if ((!p->quiet) &&
      (((picbuf->nb << 3) + xoff >= HGC_XMAX) || (picbuf->nr + yoff >= HGC_YMAX)) )
  {
	fprintf(stderr, "\n\007WARNING: Picture won't fit on a standard HGC!\n");
	fprintf(stderr, "Current range: (%d..%d) x (%d..%d) pels\n",
		xoff, (picbuf->nb << 3) + xoff, yoff, picbuf->nr + yoff);
	fprintf(stderr, "Continue anyway (y/n)?: ");
	if (toupper(getchar()) == 'N')
		return;
  }

  set_HGC_graphmode();

  for (row_c=0, y=picbuf->nr+yoff-1; row_c < picbuf->nr; row_c++, y--)
  {
	row = get_RowBuf (picbuf, row_c);
	for (x=0; x < picbuf->nc; x++)
	{
		color_index = index_from_RowBuf(row, x, picbuf);
		if (color_index != xxBackground)
			set_pixel_HGC(x+xoff, y);
	}
  }
  SilentWait();
  set_HGC_textmode();
}

