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

/** picbuf.c: Part of hp2xx project dealing with the picture buffer
 **
 ** 91/01/19  V 1.00  HWW  Derived from hptopcl.c
 ** 91/01/29  V 1.01  HWW  Tested on SUN
 ** 91/02/15  V 1.02  HWW  stdlib.h supported
 ** 91/02/20  V 1.03a HWW  minor mod. in fread(); adaptation to
 **			   new HPGL_Pt structures in tmpfile_to_PicBuf()
 ** 91/06/09  V 1.04  HWW  new options acknowledged; minimal changes
 ** 91/10/15  V 1.05  HWW  ANSI_C
 ** 91/11/20  V 1.06  HWW  "SPn;" consequences
 ** 92/02/17  V 1.07b HWW  Preparations for font support
 ** 92/05/24  V 2.00c HWW  Color supported! Fonts ok now; "init" bug fixed
 ** 92/06/08  V 2.00d HWW  GIVE_BACK: 5 --> 8; free_PicBuf() debugged
 ** 92/12/24  V 2.00e HWW  plot_RowBuf() augmented to bit REsetting
 ** 93/04/02  V 2.01a HWW  Always use four bit planes in color mode!
 **			   Out-dated "DotBlock" concept replaced by "char".
 **/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bresnham.h"
#include "hp2xx.h"


static	RowBuf	*first_buf = NULL, *last_buf = NULL;




#ifndef SEEK_SET
#define SEEK_SET 0
#endif



void	swapout_RowBuf (RowBuf *row, PicBuf *picbuf)
{
  if (fseek (picbuf->sd, (long) row->index*picbuf->nb*picbuf->depth, SEEK_SET))
  {
	perror	("swapout_RowBuf (on seek)");
	exit	(ERROR);
  }

  if (fwrite((char *) row->buf, picbuf->nb, picbuf->depth, picbuf->sd)
	!= picbuf->depth)
  {
	perror	("swapout_RowBuf (on write)");
	exit	(ERROR);
  }
}



void	swapin_RowBuf (RowBuf *row, PicBuf *picbuf)
{
  if (fseek (picbuf->sd, (long) row->index*picbuf->nb*picbuf->depth, SEEK_SET))
  {
	perror	("swapin_RowBuf (on seek)");
	exit	(ERROR);
  }

  if (fread ((char *) row->buf, picbuf->nb, picbuf->depth, picbuf->sd)
	!= picbuf->depth)
  {
	perror	("swapin_RowBuf (on read)");
	exit	(ERROR);
  }
}





void	link_RowBuf (RowBuf *act, RowBuf *prev)
{
  if (prev == NULL)			/* Make act the new "first_buf"	*/
  {
	if (first_buf == NULL)
	{
		first_buf = act;
		act->next = act->prev = NULL;
		return;
	}
	act->next = first_buf;
	act->prev = NULL;
	first_buf->prev = act;
	first_buf = act;
  }
  else				/* Squeeze act between prev & prev->next */
  {
	if ((act->next = prev->next) != NULL)
		act->next->prev = act;
	act->prev = prev;
	prev->next = act;
  }
}





void	unlink_RowBuf (RowBuf *act)
{
  if ((act->prev==NULL) && (act->next==NULL))
	return;

  if (act->prev)
	act->prev->next = act->next;
  else
	first_buf = act->next;

  if (act->next)
	act->next->prev = act->prev;
  else
	last_buf = act->prev;

  act->next = act->prev = NULL;
}




RowBuf	*get_RowBuf (PicBuf *picbuf, int index)
{
RowBuf	*row;

  row = picbuf->row + index;
/**
 ** If swapped, load first. Put into first position, if not already there:
 **/
  if ((row->prev == NULL) && (row->next == NULL))
  {
	swapout_RowBuf	(last_buf, picbuf);
	row->buf = last_buf->buf;
	unlink_RowBuf	(last_buf);		/* Mark as swapped	 */
	swapin_RowBuf	(row, picbuf);
	link_RowBuf	(row, NULL);		/* Put in first position */
  }
  else
	if (row->prev != NULL)
	{
		unlink_RowBuf (row);
		link_RowBuf (row, NULL);	/* Put in first position */
	}
	/* else: Leave it in first position */

  return row;
}





void	plot_RowBuf (RowBuf *rowbuf, int x, PicBuf *pb, int color_index)
/**
 ** Write color index into pixel x of given row buffer
 **/
{
int	i, Mask;
Byte	*addr;

/**
 ** color_index is either the low bit (b/w) or the low nybble (color)
 ** rowbuf->buf is either a sequence of such bits or nybbles.
 ** High bits show left, low bits show right.
 **
 ** This is a time-critical step, so code here is compact,
 ** but not easily readable...
 **/

  if (pb->depth == 1)
  {
	Mask = 0x80;
	if ((i = x & 0x07) != 0)
	{
		Mask >>= i;
		if (i!=7)
			color_index <<= (7-i);
	}
	else
		color_index <<= 7;
	addr = rowbuf->buf + (x >> 3);
  }
  else
  {
	Mask = 0xF0;
	if ((x & 0x01) != 0)
		Mask >>= 4;
	else
		color_index <<= 4;
	addr = rowbuf->buf + (x >> 1);
  }

  *addr &= ~Mask;
  *addr |=  color_index;
}





int	index_from_RowBuf (RowBuf *rowbuf, int x, PicBuf *pb)
/**
 ** Return color index of pixel x in given row
 **/
{
int	i, Mask, color_index;
Byte	*addr;

  if (pb->depth == 1)
  {
	Mask = 0x80;
	if ((i = x & 0x07) != 0)
		Mask >>= i;
	addr = rowbuf->buf + (x >> 3);
	return (*addr & Mask) ? xxForeground : xxBackground;
  }
  else
  {
	Mask = 0xF0;
	if ((x & 0x01) != 0)
		Mask >>= 4;
	addr = rowbuf->buf + (x >> 1);
	color_index = *addr & Mask;
	if ((x & 0x01) == 0)
		color_index >>= 4;
	return color_index;
  }
}




PicBuf	*allocate_PicBuf (DevPt *maxdotcoord, PAR *p)
/**
 ** Here we allocated the picture buffer. This memory is used by all raster
 ** modes. It is organized in rows (scan lines). Rows which do not
 ** end on a byte boundary will be right-padded with "background" bits.
 **
 ** If colors are active, there will always be four bit layers per row,
 ** even if you need only three colors.
 ** These layers are implemented by allocating longer rows
 ** (regular length times number of bit planes per pel (depth)).
 **
 ** We try to allocate all row buffers from main memory first.
 ** If allocation fails, we first free a few lines (see constant GIVE_BACK)
 ** to avoid operation close to the dyn. memory limit,
 ** and then initiate swapping to a file.
 **/
{
PicBuf	*pb;
RowBuf	*prev, *act;
int	nr, not_allocated;
#define	GIVE_BACK 8

  if ((pb = (PicBuf *) malloc(sizeof(*pb))) == NULL)
  {
	fprintf(stderr,"cannot malloc() PicBuf structure\n");
	return NULL;
  }
  pb->nr = maxdotcoord->y + 1;	/* Number of rows    to buffer	*/
  pb->nc = maxdotcoord->x + 1;	/* Number of columns to buffer	*/
  pb->sd = NULL;

  first_buf	= NULL;		/* Re-init for multiple-file	*/
  last_buf	= NULL;		/* applications			*/

/**
 ** Number of buffer bytes per row:
 **
 ** Example:
 **
 ** dot range (horiz.): 0...2595 ==> 2596 dots per row, pb->nc=2096 ==>
 ** 	[2596 bits / 8 bits per byte]
 ** ==> 324 DotBlocks + 4 bits which require another whole byte (!)
 **/

  pb->nb = (pb->nc >> 3);
  if (pb->nc & 7)
	pb->nb++;

/**
 ** Auto-detection of depth (# bits per pel):
 **
 ** B/W mode      (1 bit per pel, Foreground & Background),
 ** or color mode (4 bits per pel)
 **/

  pb->depth = (p->is_color) ? 4 : 1;

/**
 ** Allocate a (large) array of RowBuf structures: One for each scan line.
 ** !!! The NULL initialization done implicitly by calloc() is crucial !!!
 **/

  if ((pb->row = (RowBuf *) calloc((unsigned) pb->nr, sizeof(RowBuf)))
		== NULL)
  {
	fprintf(stderr,"cannot calloc() %d RowBuf structures\n", pb->nr);
	return NULL;
  }

/**
 ** Now try to allocate as many buffers as possible. Double-link all RowBuf's
 ** which succeed in buffer allocation, leave the rest isolated (swapping
 ** candidates!)
 **/

  not_allocated = 0;
  prev = (RowBuf *) NULL;
  for (nr=0, act = pb->row; nr < pb->nr; nr++, act++)
  {
	act->prev = act->next = NULL;
	act->index= nr;
	if ((act->buf=
	    (Byte *) calloc((unsigned) (pb->nb * pb->depth),1)) == NULL)
		not_allocated++;
	else
	{
		link_RowBuf (act, prev);
		prev = act;
		last_buf = act;
	}
  }

/**
 ** Prepare swapping
 **/

  if (not_allocated)
  {
	if (last_buf->index > GIVE_BACK) for (nr = 0; nr < GIVE_BACK; nr++)
	{
			/* Return some memory for internal use */
		free ((char *) last_buf->buf);
		unlink_RowBuf (last_buf);
		not_allocated++;
	}
	else
	{
		fprintf(stderr,"\nNot enough memory for swapping -- sorry!\n");
		exit (ERROR);
	}

	fprintf(stderr,"\nCouldn't allocate %d row buffers: swapping to disk\n",
		not_allocated);
	if ((pb->sd = fopen (p->swapfile, WRITE_BIN)) == NULL)
	{
		fprintf(stderr,"Couldn't open swap file: %s\n", p->swapfile);
		perror ("hp2xx");
		return NULL;
	}

/**
 ** Init. swap file data to background color (0), using a shortcut by
 ** assuming that all data are stored without gaps. Thus, instead of
 ** row-by-row operation, we simply write a sufficient number of 0 rows
 ** into the swap file sequentially.
 **/

	for (nr=0; nr < pb->nr; nr++)
	    if (fwrite((char *) pb->row[0].buf, pb->nb, pb->depth, pb->sd)
		!= pb->depth)
	    {
			fprintf(stderr,"Couldn't clear swap file!\n");
			perror ("hp2xx");
			fclose (pb->sd);
			return NULL;
	    }
  }

  return pb;
}




void	free_PicBuf (PicBuf *picbuf, char *swapfilename)
/**
 ** De-allocate all row buffersand the picture puffer struct,
 ** remove the swap file (if any).
 **/
{
RowBuf	*row;
int	i;

  if (picbuf->sd)
  {
	fclose (picbuf->sd);
	picbuf->sd = NULL;
#ifdef VAX
	delete (swapfilename);
#else
	unlink (swapfilename);
#endif
  }
  for (i=0; i< picbuf->nr; i++)
  {
	row = &(picbuf->row[i]);
	if ((row->prev != NULL) || (row->next != NULL))
		free ((char *) row->buf);
  }
  free((char *) picbuf->row);
  free((char *) picbuf);
}





void	plot_PicBuf(PicBuf *picbuf, DevPt *pt, int color_index)
{
  plot_RowBuf(get_RowBuf(picbuf, pt->y), pt->x, picbuf, color_index);
}




int	index_from_PicBuf(PicBuf *picbuf, DevPt *pt)
{
  return index_from_RowBuf(get_RowBuf(picbuf, pt->y), pt->x, picbuf);
}




void	line_PicBuf (PicBuf *picbuf, DevPt *p0, DevPt *p1, PAR *p)
/**
 ** Rasterize a vector (draw a line in the picture buffer), using the
 ** Bresenham algorithm.
 **/
{
DevPt	pt, *p_act;
int	pensize, pencolor;

  pensize = p->pensize[p->pen];
  if (pensize == 0)		/* No pen selected!	*/
	return;

  pencolor = p->pencolor[p->pen];
  if (pencolor == xxBackground)	/* No drawable color!	*/
	return;

  p_act = bresenham_init (p0, p1);
  if (pensize == 1) do
  {
	plot_PicBuf (picbuf, p_act, pencolor);
  } while (bresenham_next() != BRESENHAM_ERR);
  else do
  {
	plot_PicBuf (picbuf, p_act, pencolor);

	pt = *p_act;
	pt.x++;	plot_PicBuf (picbuf, &pt, pencolor);
	pt.y++; plot_PicBuf (picbuf, &pt, pencolor);
	pt.x--; plot_PicBuf (picbuf, &pt, pencolor);

	if (pensize > 2)
	{
		pt = *p_act;
		pt.x += 2;	plot_PicBuf (picbuf, &pt, pencolor);
		pt.y++;		plot_PicBuf (picbuf, &pt, pencolor);
		pt.y++;		plot_PicBuf (picbuf, &pt, pencolor);
		pt.x--;		plot_PicBuf (picbuf, &pt, pencolor);
		pt.x--;		plot_PicBuf (picbuf, &pt, pencolor);

		if (pensize > 3)	/* expecting 4 ... 9	*/
		{
			pt = *p_act;
			pt.x += 3;	plot_PicBuf (picbuf, &pt, pencolor);
			pt.y++;		plot_PicBuf (picbuf, &pt, pencolor);
			pt.y++;		plot_PicBuf (picbuf, &pt, pencolor);
			pt.y++;		plot_PicBuf (picbuf, &pt, pencolor);
			pt.x--;		plot_PicBuf (picbuf, &pt, pencolor);
			pt.x--;		plot_PicBuf (picbuf, &pt, pencolor);
			pt.x--;		plot_PicBuf (picbuf, &pt, pencolor);
		}
	}
  } while (bresenham_next() != BRESENHAM_ERR);
}





void	tmpfile_to_PicBuf (PicBuf *picbuf, PAR *p, FILE *td)
/**
 ** Interface to higher-level routines:
 **   Assuming a valid picture buffer, read the drawing commands from
 **   the temporary file, transform HP_GL coordinates into dot coordinates,
 **   and draw (rasterize) vectors.
 **/
{
HPGL_Pt		pt1;
static	DevPt	ref = {0};
DevPt		next;
PlotCmd		cmd;

  if (!p->quiet)
	fprintf(stderr, "\nPlotting in buffer\n");
  rewind (td);
  while ((cmd = PlotCmd_from_tmpfile()) != EOF)
	switch (cmd)
	{
	  case NOP:
		break;
	  case SET_PEN:
		if ((p->pen = fgetc(td)) == EOF)
		{
			perror("Unexpected end of temp. file");
			exit (ERROR);
		}
		break;
	  case MOVE_TO:
		HPGL_Pt_from_tmpfile(&pt1);
		HPcoord_to_dotcoord (&pt1, &ref);
		break;
	  case DRAW_TO:
		HPGL_Pt_from_tmpfile(&pt1);
		HPcoord_to_dotcoord (&pt1, &next);
		line_PicBuf (picbuf, &ref, &next, p);
		memcpy (&ref, &next, sizeof(ref));
		break;
	  case PLOT_AT:
		HPGL_Pt_from_tmpfile(&pt1);
		HPcoord_to_dotcoord (&pt1, &ref);
		line_PicBuf (picbuf, &ref, &ref, p);
		break;

	  default:
		fprintf(stderr,"Illegal cmd in temp. file!\n");
		exit (ERROR);
	}
}
