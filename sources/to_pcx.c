/*
   Copyright (c) 1991 - 1993 Heinz W. Werntges.  All rights reserved.
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

/** to_pcx.c: PCX converter part of project "hp2xx"
 **
 ** 91/01/19  V 1.00  HWW  Originating: Format accepted by MS-Paintbrush,
 **			  but not by emTeX drivers
 **			  Use MS-Paintbrush "load/save" for conversion
 ** 91/02/15  V 1.01  HWW  VAX_C support added (not tested yet!)
 ** 91/02/18  V 1.02  HWW  PCX format: no zero run length allowed
 ** 91/02/20  V 1.03  HWW  Some VAX_C changes, debugged
 ** 91/06/09  V 1.04  HWW  New options added
 ** 91/06/16  V 1.05  HWW  Writing of PCX header now machine-independent
 ** 91/10/15  V 1.06  HWW  ANSI_C
 ** 91/10/25  V 1.07  HWW  VAX: fopen() augmentations used, open() removed
 ** 92/05/17  V 1.07b HWW  Output to stdout if outfile == '-'
 ** 92/05/19  V 1.07c HWW  Abort if color mode
 ** 92/06/08  V 1.08a HWW  First color version
 **
 **	      NOTE: According to my tests, setting of the
 **		    color lookup table is ignored by other programs,
 **		    so this code is *preliminary* when color is used.
 **		    Correct colors appeared only if the color setting corresponded to
 **		    PC conventions...
 **/


#include <stdio.h>
#include <stdlib.h>
#include "bresnham.h"
#include "hp2xx.h"


typedef	enum {PCX_INIT, PCX_NORMAL, PCX_EXIT}
	PCXmode;




void	RLEcode_to_file (int c, int repeat, FILE *fd)
{
  if ((repeat==1) && ((c & 0xC0) != 0xC0))
  {
	if (putc (c, fd) == EOF)
	{
		perror("RLEcode_to_file (1)");
		exit	(ERROR);
	}
  }
  else
  {
	if (putc (repeat | 0xC0, fd) == EOF)
	{
		perror("RLEcode_to_file (2)");
		exit	(ERROR);
	}
	if (putc (c, fd) == EOF)
	{
		perror("RLEcode_to_file (3)");
		exit	(ERROR);
	}
  }
}




void	byte_to_PCX (Byte b, PCXmode mode, FILE *fd)
{
static	last_b, rept;

  switch (mode)
  {
  case PCX_NORMAL:
	if (b == last_b)
	{
		if (++rept == 63)
		{
			RLEcode_to_file (last_b, rept, fd);
			rept = 0;
		}
	}
	else
	{
		if (rept)
			RLEcode_to_file (last_b, rept, fd);
		rept	= 1;
		last_b	= b;
	}
	break;

  case PCX_INIT:
	rept = 0;
	last_b = -2;	/* Init to impossible value	*/
	break;

  case PCX_EXIT:
	if (rept)
	{
		RLEcode_to_file (last_b, rept, fd);
		rept = 0;
	}
	break;
  }
}





typedef struct {
	char	creator, version, encoding, bits;
	short	xmin, ymin, xmax, ymax, hres, vres;
	char	palette[16][3], vmode, planes;
	short	byteperline, paletteinfo;
	char	dummy[58];
} PCXheader;



void	start_PCX (PAR *p, PicBuf *picbuf, FILE *fd)
{
PCXheader	h;
int		i, j;

  h.creator	= 0x0A;		/* ZSoft label			*/
  h.version	= '\003';	/* V 2.8/3.0, no palette info	*/
  h.encoding	= 1;		/* RLE				*/
  h.bits	= 1;		/* Bits per pixel		*/
  h.xmin	= 0;            /* Range of bitmap		*/
  h.ymin	= 0;
  h.xmax	= picbuf->nc - 1;
  h.ymax	= picbuf->nr - 1;
  h.hres	= p->dpi_x;	/* Resolution			*/
  h.vres	= p->dpi_y;

  for (i=0; i<8; i++)
     for (j=0; j<3; j++)
     {
	h.palette[i  ][j] = p->Clut[i][j];
	h.palette[i+8][j] = p->Clut[i][j];
     }

  h.vmode	= 0;		/* Reserved			   */
  h.planes	= picbuf->depth;/* Number of color planes	   */
  h.byteperline	= picbuf->nb;	/* Number of bytes per line	   */
  h.paletteinfo	= 1;		/* 1 = color & b/w, 2 = gray scale */
  for (i=0; i<58; )		/* Filler for a max. of 128 bytes  */
  {
	h.dummy[i++] = 'H';
	h.dummy[i++] = 'W';
  }

/**
 ** For complete machine independence, a bytewise writing of this header
 ** is mandatory. Else, fill bytes or HIGH/LOW-endian machines must be
 ** considered. A simple "fwrite(h,128,1,fd)" may not suffice!
 **/

  if (fputc (h.creator,	fd) == EOF)		goto ERROR_EXIT;
  if (fputc (h.version,	fd) == EOF)		goto ERROR_EXIT;
  if (fputc (h.encoding,fd) == EOF)		goto ERROR_EXIT;
  if (fputc (h.bits,	fd) == EOF)		goto ERROR_EXIT;
  if (fputc ((h.xmin & 0xff),	fd) == EOF)	goto ERROR_EXIT;
  if (fputc ((h.xmin >> 8),	fd) == EOF)	goto ERROR_EXIT;
  if (fputc ((h.ymin & 0xff),	fd) == EOF)	goto ERROR_EXIT;
  if (fputc ((h.ymin >> 8),	fd) == EOF)	goto ERROR_EXIT;
  if (fputc ((h.xmax & 0xff),	fd) == EOF)	goto ERROR_EXIT;
  if (fputc ((h.xmax >> 8),	fd) == EOF)	goto ERROR_EXIT;
  if (fputc ((h.ymax & 0xff),	fd) == EOF)	goto ERROR_EXIT;
  if (fputc ((h.ymax >> 8),	fd) == EOF)	goto ERROR_EXIT;
  if (fputc ((h.hres & 0xff),	fd) == EOF)	goto ERROR_EXIT;
  if (fputc ((h.hres >> 8),	fd) == EOF)	goto ERROR_EXIT;
  if (fputc ((h.vres & 0xff),	fd) == EOF)	goto ERROR_EXIT;
  if (fputc ((h.vres >> 8),	fd) == EOF)	goto ERROR_EXIT;
  if (fwrite((VOID *) h.palette,48,1,fd) != 1)	goto ERROR_EXIT;
  if (fputc (h.vmode,	fd) == EOF)		goto ERROR_EXIT;
  if (fputc (h.planes,	fd) == EOF)		goto ERROR_EXIT;
  if (fputc ((h.byteperline & 0xff),fd) == EOF)	goto ERROR_EXIT;
  if (fputc ((h.byteperline >> 8),  fd) == EOF)	goto ERROR_EXIT;
  if (fputc ((h.paletteinfo & 0xff),fd) == EOF)	goto ERROR_EXIT;
  if (fputc ((h.paletteinfo >> 8),  fd) == EOF)	goto ERROR_EXIT;
  if (fwrite((VOID *) h.dummy,58,1,fd) != 1)	goto ERROR_EXIT;
  return;

ERROR_EXIT:
  perror      ("start_PCX");
  free_PicBuf (picbuf, p->swapfile);
  exit	      (ERROR);
}



void	PicBuf_to_PCX (PicBuf *picbuf, PAR *p)
{
FILE	*fd;
RowBuf	*row;
int	row_c, np, x;
Byte	*pb;

  if (!p->quiet)
	fprintf(stderr, "\nWriting PCX output\n");
  if (*p->outfile != '-')
  {
#ifdef VAX
	if ((fd = fopen(p->outfile, WRITE_BIN, "rfm=var","mrs=512")) == NULL)
	{
#else
	if ((fd = fopen(p->outfile, WRITE_BIN)) == NULL)
	{
#endif
		perror ("hp2xx -- opening output file");
		free_PicBuf (picbuf, p->swapfile);
		exit (ERROR);
	}
  }
  else	fd = stdout;

  start_PCX (p, picbuf, fd);

  /* Backward since highest index is lowest line on screen! */
  for (row_c = picbuf->nr - 1; row_c >= 0; row_c--)
  {
	if ((!p->quiet) && (row_c % 10 == 0))
		  /* For the impatients among us ...	*/
		putc('.',stderr);
	row = get_RowBuf (picbuf, row_c);
	byte_to_PCX (0, PCX_INIT, fd);
	pb = row->buf;
	for (np=0; np < picbuf->depth; np++)
		for (x=0; x < picbuf->nb; x++)
			byte_to_PCX (~*pb++, PCX_NORMAL, fd);
	byte_to_PCX (0, PCX_EXIT, fd);	/* Flush	*/
  }
  if (p->is_color && !p->quiet)
	fprintf(stderr,"WARNING: PCX colors may (yet) be arbitrary!\n");

  if (!p->quiet)
	fputc ('\n', stderr);
  if (fd != stdout)
	fclose (fd);
}
