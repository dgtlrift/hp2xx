/*
   Copyright (c) 2001  Martin Kroeker  All rights reserved.
   
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

/** to_pdf.c:   Converter to Adobe Portable Document (PDF) Format using pdflib
 ** 
 ** 01/06/19  V 1.00 MK   derived from to_eps.c
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pdflib.h>
#include "bresnham.h"
#include "hp2xx.h"
#include "pendef.h"
#include "lindef.h"


static	int	linecount = 0;
static	float	xcoord2mm, ycoord2mm;
static	float	xmin, ymin;


int 	to_pdf (const GEN_PAR *, const OUT_PAR *);
void	pdf_init (const GEN_PAR *, const OUT_PAR *, PDF *, PEN_W);
void	pdf_set_linewidth (double, HPGL_Pt *, PDF *);
void pdf_set_linecap( LineEnds type,double pensize, PDF *fd);
void pdf_set_linejoin( LineJoins type, LineLimit limit, double pensize, PDF *fd);
void	pdf_set_color (double, double, double, HPGL_Pt *, PDF *);
void 	pdf_end(PDF *);



/**
 ** Close graphics file
 **/
void	pdf_end (PDF *fd)
{
  PDF_end_page(fd);
  PDF_close(fd);
  PDF_delete(fd);
  PDF_shutdown();
  linecount = 0;
}


/**
 ** Set line width
 **/
void	pdf_set_linewidth (double width, HPGL_Pt *ppt, PDF *fd)
{
  PDF_setlinewidth(fd, width);
}


/**
 ** Set line Ends
 **/
void pdf_set_linecap( LineEnds type,double pensize, PDF *fd) {

   static int lastcap = -1;
   int newcap;

   if ( pensize > 0.35 ) {
      switch (type) {
         case LAE_butt:
            newcap=0;
            break;
         case LAE_triangular:                                /* triangular not implemented in PS/PDF */
            newcap=1;
            break;
         case LAE_round:
            newcap=1;
            break;
         case LAE_square:
            newcap=2;
            break;
         default:
            newcap=0;
            break;
      }
   } else {
      newcap=1;
   }

   if(newcap != lastcap) {
      PDF_setlinecap(fd,newcap);
      lastcap=newcap;
   }
}


/**
 ** Set line Joins
 **/
void pdf_set_linejoin( LineJoins type, LineLimit limit, double pensize, PDF *fd) {

   static int lastcap = -1;
   static int lastlimit = -1;
   int newcap;
   int newlimit = lastlimit;
   

   if ( pensize > 0.35 ) {
      switch (type) {
         case LAJ_plain_miter:
            newcap=0;
	    newlimit = 100; /* arbitrary value */
            break;
         case LAJ_bevel_miter: /* not available */
            newcap=0;
	    newlimit = limit;
            break;
         case LAJ_triangular: /* not available */
            newcap=1;
            break;
         case LAJ_round:
            newcap=1;
            break;
	 case LAJ_bevelled:
            newcap=2;
            break;
	 case LAJ_nojoin: /* not available */
            newcap=1;
            break;
         default:
            newcap=0;
	    newlimit = 100; /* arbitrary value */
            break;
      }
   } else {
      newcap=1;
   }

   if(newcap != lastcap) {
      PDF_setlinejoin(fd,newcap);
      lastcap=newcap;
   }
   if(newlimit != lastlimit) {
      PDF_setmiterlimit(fd,newlimit);
      lastlimit=newlimit;
   }
}


/**
 ** Set RGB color
 **/
void	pdf_set_color (double red, double green, double blue,
			HPGL_Pt *ppt, PDF *fd)
{
PDF_setrgbcolor(fd,red,green,blue);
}


/**
 ** basic PDF definitions
 **/

void	pdf_init (const GEN_PAR *pg, const OUT_PAR *po, PDF *fd,
		 PEN_W pensize)
{
long	left, right, low, high;
double	hmxpenw;

  hmxpenw = pg->maxpensize / 20.0;	/* Half max. pen width, in mm	*/

  left  = (long) floor(abs(po->xoff-hmxpenw)		    * 2.834646);
  low   = (long) floor(abs(po->yoff-hmxpenw)* 2.834646);
  right = (long) ceil ((po->xoff   + po->width+hmxpenw)	    * 2.834646);
  high  = (long) ceil ((po->yoff+po->height+hmxpenw)	    * 2.834646);

  PDF_begin_page(fd, right,high);
  PDF_setlinewidth(fd, pensize);
  pdf_set_linecap(CurrentLineAttr.End,pensize,fd);
  pdf_set_linejoin(CurrentLineAttr.Join,CurrentLineAttr.Limit,pensize,fd);
}



/**
 ** Higher-level interface: Output Portable Document Format
 **/

int
to_pdf (const GEN_PAR *pg, const OUT_PAR *po)
{
PlotCmd	cmd;
PDF	*md;
HPGL_Pt	pt1 = {0};
int	pen_no=0, pencolor=0, err;
int openpath;
PEN_W pensize;

   PDF_boot();
   
  err = 0;
  if (!pg->quiet)
	Eprintf ("\n\n- Writing PDF code to \"%s\"\n",
		*po->outfile == '-' ? "stdout" : po->outfile);

  /* Init. of PDF file: */
  
  md=PDF_new();
  if (PDF_open_file(md, po->outfile) == -1) {
     PError("hp2xx (pdf)");
     return ERROR;
  }

  /* header */

  pensize = pt.width[DEFAULT_PEN_NO]; /* Default pen	*/
  pdf_init (pg, po, md, pensize);


  /* Factor for transformation of HP coordinates to mm	*/

  xcoord2mm = po->width  / (po->xmax - po->xmin) * 2.834646;
  ycoord2mm = po->height / (po->ymax - po->ymin) * 2.834646;
  xmin	    = po->xmin;
  ymin	    = po->ymin;

/**
 ** Command loop: While temporary file not empty: process command.
 **/
  openpath=0;
  while ((cmd = PlotCmd_from_tmpfile()) != CMD_EOF)
  {
	switch (cmd)
	{
	  case NOP:
		break;
	  case SET_PEN:
		if ((pen_no = fgetc(pg->td)) == EOF)
		{
			PError("Unexpected end of temp. file: ");
			err = ERROR;
			goto PDF_exit;
		}
		pensize = pt.width[pen_no];
		pencolor = pt.color[pen_no];
		 if(openpath==1){PDF_stroke(md);
		 	openpath=0;
		 	}
                    if (pensize != 0)
		pdf_set_linewidth ((double) pensize, &pt1, md);
                pdf_set_linecap(CurrentLineAttr.End,(double) pensize,md);
                pdf_set_linejoin(CurrentLineAttr.Join,CurrentLineAttr.Limit,(double) pensize,md);
		pdf_set_color (  pt.clut[pencolor][0]/255.0,
				pt.clut[pencolor][1]/255.0,
				pt.clut[pencolor][2]/255.0,
				&pt1, md);
		break;
          case DEF_PW:
                if(!load_pen_width_table(pg->td)) {
                    PError("Unexpected end of temp. file");
		    err = ERROR;
		    goto PDF_exit;
                }
                break;
          case DEF_PC:
                err=load_pen_color_table(pg->td);
                if (err<0) {
                    PError("Unexpected end of temp. file");
		    err = ERROR;
		    goto PDF_exit;
                }
                if (err==pencolor) pencolor *=-1; /*current pen changed*/
                break;
          case DEF_LA:
                if(load_line_attr(pg->td) <0) {
                    PError("Unexpected end of temp. file");
		    err = ERROR;
		    goto PDF_exit;
                }
                break;
	  case MOVE_TO:
		if (openpath==1) {
			PDF_stroke(md);
			openpath=0;
			}
                if(fabs(pensize-pt.width[pen_no]) >= 0.01) {
                    pensize=pt.width[pen_no];
                    if (pensize != 0){
                       pdf_set_linewidth ((double) pensize, &pt1, md);
                       pdf_set_linecap(CurrentLineAttr.End,(double) pensize,md);
                       pdf_set_linejoin(CurrentLineAttr.Join,CurrentLineAttr.Limit,(double) pensize,md);
                    }
                }
                if(pencolor <0) {
                   pencolor=pt.color[pen_no];
                   pdf_set_color(pt.clut[pencolor][0]/255.0,
                                 pt.clut[pencolor][1]/255.0,
                                 pt.clut[pencolor][2]/255.0,
                                 &pt1, md);
                }

                HPGL_Pt_from_tmpfile (&pt1);
                if(pensize != 0){
                   PDF_moveto(md,(pt1.x-xmin)*xcoord2mm,(pt1.y-ymin)*ycoord2mm);
                }
		break;
	  case DRAW_TO:
                if(fabs(pensize-pt.width[pen_no]) >= 0.01) {
                   pensize=pt.width[pen_no];
                   if(pensize != 0){
                      pdf_set_linewidth ((double) pensize, &pt1, md);
                      pdf_set_linecap(CurrentLineAttr.End,(double) pensize,md);
                      pdf_set_linejoin(CurrentLineAttr.Join,CurrentLineAttr.Limit,(double) pensize,md);
                   }
                }
                if(pencolor <0) {
                   pencolor=pt.color[pen_no];
                   pdf_set_color(pt.clut[pencolor][0]/255.0,
                                 pt.clut[pencolor][1]/255.0,
                                 pt.clut[pencolor][2]/255.0,
                                 &pt1, md);
                }
                HPGL_Pt_from_tmpfile (&pt1);
                if(pensize != 0){
                   PDF_lineto(md,(pt1.x-xmin)*xcoord2mm,(pt1.y-ymin)*ycoord2mm);
                   openpath=1;
                }
                break;
	  case PLOT_AT:
/*		if (openpath==1){
			PDF_stroke(md);
			openpath=0;
			}*/
                if(fabs(pensize-pt.width[pen_no]) >= 0.01) {
                   pensize=pt.width[pen_no];
                   if(pensize != 0){
                      pdf_set_linewidth ((double) pensize, &pt1, md);
                      pdf_set_linecap(CurrentLineAttr.End,(double) pensize,md);
                      pdf_set_linejoin(CurrentLineAttr.Join,CurrentLineAttr.Limit,(double) pensize,md);
                   }
                }
                if(pencolor<0) {
                   pencolor=pt.color[pen_no];
		   pdf_set_color(pt.clut[pencolor][0]/255.0,
                                 pt.clut[pencolor][1]/255.0,
                                 pt.clut[pencolor][2]/255.0,
                                 &pt1, md);
                }
		HPGL_Pt_from_tmpfile (&pt1);
                if(pensize != 0) {
                   PDF_moveto(md,(pt1.x-xmin)*xcoord2mm,(pt1.y-ymin)*ycoord2mm);
                   PDF_lineto(md,(pt1.x-xmin)*xcoord2mm+1,(pt1.y-ymin)*ycoord2mm+1);
/*		   PDF_stroke(md);*/
		openpath=1;
                }
                break;
	  default:
		Eprintf ("Illegal cmd in temp. file!");
		err = ERROR;
		goto PDF_exit;
	}
  }

  /* Finish up */
if (openpath==1) PDF_stroke(md);
  pdf_end (md);

PDF_exit:
/*  if (md != stdout)
	fclose (md);
*/
  if (!pg->quiet)
	Eprintf ("\n");
  return err;
}

