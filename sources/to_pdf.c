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
#ifdef PDFLIB
#include <pdflib.h>
#else
#include <libharu.h>
#endif
#include "bresnham.h"
#include "hp2xx.h"
#include "pendef.h"
#include "lindef.h"


static int linecount = 0;
static float xcoord2mm, ycoord2mm;
static float xmin, ymin;

/* globals for states since we have to init them for each file now */
static PEN_W lastwidth;
static int lastcap;
static int lastjoin;
static int lastlimit;
static Byte lastred, lastgreen, lastblue;

#ifndef PDFLIB
typedef pdf_doc PDF;
pdf_page page;
pdf_contents canvas;
#endif

int to_pdf(const GEN_PAR *, const OUT_PAR *);
void pdf_init(const GEN_PAR *, const OUT_PAR *, PDF *, PEN_W);
void pdf_set_linewidth(double, PDF *);
void pdf_set_linecap(LineEnds type, double pensize, PDF * fd);
void pdf_set_linejoin(LineJoins type, LineLimit limit, double pensize,
		      PDF * fd);
void pdf_set_color(PEN_C pencolor, PDF * fd);
int pdf_end(PDF *,const OUT_PAR *);

#ifndef PDFLIB
#define PAGEMODE if (openpath==1) { pdf_contents_stroke(canvas); openpath=0; }
#else
#define PAGEMODE if (openpath==1) { PDF_stroke(md); openpath=0; }
#endif

/**
 ** Close graphics file
 **/
int pdf_end(PDF * fd, const OUT_PAR *po)
{
#ifdef PDFLIB
	PDF_end_page(fd);
	PDF_close(fd);
	PDF_delete(fd);
	PDF_shutdown();
#else
	if (pdf_doc_write_to_file(fd, po->outfile) !=PDF_SUCCESS) {
		PError("hp2xx (pdf)");
		pdf_doc_free(fd);
		return ERROR;
	}
	pdf_doc_free(fd);
#endif
	linecount = 0;
	return 0;
}


/**
 ** Set line width
 **/
void pdf_set_linewidth(double width, PDF * fd)
{
	double newwidth;

	if (width == 0.0) {
		newwidth = 0.0001;
	} else {
		newwidth = width;
	}

	if (fabs(newwidth - lastwidth) >= 0.01) {
#ifdef PDFLIB
		PDF_setlinewidth(fd, newwidth * MM_TO_PS_POINT);
#else
		pdf_contents_set_line_width(canvas, newwidth * MM_TO_PS_POINT);
#endif
		lastwidth = newwidth;
	}
}


/**
 ** Set line Ends
 **/
void pdf_set_linecap(LineEnds type, double pensize, PDF * fd)
{
	int newcap;

	if (pensize > 0.35) {
		switch (type) {
		case LAE_butt:
			newcap = 0;
			break;
		case LAE_triangular:	/* triangular not implemented in PS/PDF */
			newcap = 1;
			break;
		case LAE_round:
			newcap = 1;
			break;
		case LAE_square:
			newcap = 2;
			break;
		default:
			newcap = 0;
			break;
		}
	} else {
		newcap = 1;
	}

	if (newcap != lastcap) {
#ifdef PDFLIB
		PDF_setlinecap(fd, newcap);
#else		
		pdf_contents_set_line_cap(canvas, newcap);
#endif
		lastcap = newcap;
	}
}


/**
 ** Set line Joins
 **/
void pdf_set_linejoin(LineJoins type, LineLimit limit, double pensize,
		      PDF * fd)
{
	int newjoin;
	int newlimit = lastlimit;


	if (pensize > 0.35) {
		switch (type) {
		case LAJ_plain_miter:
			newjoin = 0;
			newlimit = 5;	/* arbitrary value */
			break;
		case LAJ_bevel_miter:	/* not available */
			newjoin = 0;
			newlimit = limit;
			break;
		case LAJ_triangular:	/* not available */
			newjoin = 1;
			break;
		case LAJ_round:
			newjoin = 1;
			break;
		case LAJ_bevelled:
			newjoin = 2;
			break;
		case LAJ_nojoin:	/* not available */
			newjoin = 1;
			break;
		default:
			newjoin = 0;
			newlimit = 5;	/* arbitrary value */
			break;
		}
	} else {
		newjoin = 1;
	}

	if (newjoin != lastjoin) {
#ifdef PDFLIB
		PDF_setlinejoin(fd, newjoin);
#else
		pdf_contents_set_line_join(canvas, newjoin);
#endif
		lastjoin = newjoin;
	}

	if (newlimit != lastlimit) {
#ifdef PDFLIB
		PDF_setmiterlimit(fd, newlimit * MM_TO_PS_POINT);
#else
		pdf_contents_set_miter_limit(canvas, newlimit * MM_TO_PS_POINT);
#endif
		lastlimit = newlimit;
	}
}


/**
 ** Set RGB color
 **/
void pdf_set_color(PEN_C pencolor, PDF * fd)
{

	if ((pt.clut[pencolor][0] != lastred) ||
	    (pt.clut[pencolor][1] != lastgreen)
	    || (pt.clut[pencolor][2] != lastblue)) {
#ifdef PDFLIB
			PDF_setrgbcolor(fd,
				(double) pt.clut[pencolor][0] / 255.0,
				(double) pt.clut[pencolor][1] / 255.0,
				(double) pt.clut[pencolor][2] / 255.0);
#else				    
			pdf_contents_set_rgb_stroke(canvas,PdfRGBColor(
				(double) pt.clut[pencolor][0] / 255.0,
				(double) pt.clut[pencolor][1] / 255.0,
				(double) pt.clut[pencolor][2] / 255.0));
#endif


		lastred = pt.clut[pencolor][0];
		lastgreen = pt.clut[pencolor][1];
		lastblue = pt.clut[pencolor][2];
	} else {
		return;
	}
}


/**
 ** basic PDF definitions
 **/

void pdf_init(const GEN_PAR * pg, const OUT_PAR * po, PDF * fd,
	      PEN_W pensize)
{
	long left, right, low, high;
	double hmxpenw;

	lastwidth = -1.0;
	lastcap = lastjoin = lastlimit = -1;
	lastred = lastgreen = lastblue = -1;

	hmxpenw = pg->maxpensize / 20.0;	/* Half max. pen width, in mm   */

	left = (long) floor(fabs(po->xoff - hmxpenw) * MM_TO_PS_POINT);
	low = (long) floor(fabs(po->yoff - hmxpenw) * MM_TO_PS_POINT);
	right =
	    (long) ceil((po->xoff + po->width + hmxpenw) * MM_TO_PS_POINT);
	high =
	    (long) ceil((po->yoff + po->height + hmxpenw) *
			MM_TO_PS_POINT);

#ifdef PDFLIB
	PDF_begin_page(fd, (float) right, (float) high);
	pdf_set_linewidth(pensize, fd);
	pdf_set_linecap(CurrentLineAttr.End, pensize, fd);
	pdf_set_linejoin(CurrentLineAttr.Join, CurrentLineAttr.Limit,
			 pensize, fd);
#else	
	page=pdf_doc_add_page(fd);
	pdf_page_set_size(page, (float) right, (float) high);
	canvas=pdf_page_get_canvas(page);
	pdf_stream_add_filter(canvas,PDF_FILTER_DEFLATE);
	pdf_set_linewidth(pensize, canvas);
	pdf_set_linecap(CurrentLineAttr.End, pensize, canvas);
	pdf_set_linejoin(CurrentLineAttr.Join, CurrentLineAttr.Limit,
			 pensize, canvas);
#endif
}



/**
 ** Higher-level interface: Output Portable Document Format
 **/

int to_pdf(const GEN_PAR * pg, const OUT_PAR * po)
{
	PlotCmd cmd;
	PDF *md;
	HPGL_Pt pt1 = { 0, 0 };
	int pen_no = 0, err;
	int openpath;
	PEN_W pensize;

#ifdef PDFLIB
	PDF_boot();
#endif	

	err = 0;
	if (!pg->quiet)
		Eprintf("\n\n- Writing PDF code to \"%s\"\n",
			*po->outfile == '-' ? "stdout" : po->outfile);

	/* Init. of PDF file: */

#ifdef PDFLIB
	md = PDF_new();
	if (PDF_open_file(md, po->outfile) == -1) {
		PError("hp2xx (pdf)");
		return ERROR;
	}	
#else	
	md = pdf_doc_new();
	if (md == NULL) {
		PError("hp2xx (pdf)");
		return ERROR;
	}
	
	err=pdf_doc_new_doc(md);
	if (err != PDF_SUCCESS) fprintf(stderr,"failed to create pdf context\n");
#endif
	/* header */

	pensize = pt.width[DEFAULT_PEN_NO];	/* Default pen    */
	pdf_init(pg, po, md, pensize);


	/* Factor for transformation of HP coordinates to mm  */

	xcoord2mm = po->width / (po->xmax - po->xmin) * MM_TO_PS_POINT;
	ycoord2mm = po->height / (po->ymax - po->ymin) * MM_TO_PS_POINT;
	xmin = po->xmin;
	ymin = po->ymin;

/**
 ** Command loop: While temporary file not empty: process command.
 **/
	openpath = 0;
	while ((cmd = PlotCmd_from_tmpfile()) != CMD_EOF) {
		switch (cmd) {
		case NOP:
			break;

		case SET_PEN:
			if ((pen_no = fgetc(pg->td)) == EOF) {
				PError("Unexpected end of temp. file: ");
				err = ERROR;
				goto PDF_exit;
			}
			pensize = pt.width[pen_no];
			break;

		case DEF_PW:
			if (!load_pen_width_table(pg->td,0)) {
				PError("Unexpected end of temp. file");
				err = ERROR;
				goto PDF_exit;
			}
			pensize = pt.width[pen_no];
			break;

		case DEF_PC:
			err = load_pen_color_table(pg->td,0);
			if (err < 0) {
				PError("Unexpected end of temp. file");
				err = ERROR;
				goto PDF_exit;
			}
			break;

		case DEF_LA:
			if (load_line_attr(pg->td,0) < 0) {
				PError("Unexpected end of temp. file");
				err = ERROR;
				goto PDF_exit;
			}
			break;

		case MOVE_TO:
			pensize = pt.width[pen_no];
			PAGEMODE;
			pdf_set_linewidth((double) pensize, md);
			pdf_set_linecap(CurrentLineAttr.End,
					(double) pensize, md);
			pdf_set_linejoin(CurrentLineAttr.Join,
					 CurrentLineAttr.Limit,
					 (double) pensize, md);
			pdf_set_color(pt.color[pen_no], md);

			HPGL_Pt_from_tmpfile(&pt1);
#ifdef PDFLIB
			PDF_moveto(md,
#else			
			pdf_contents_moveto(canvas,
#endif			
			   		(pt1.x - xmin) * xcoord2mm,
			   		(pt1.y - ymin) * ycoord2mm);
			openpath = 1;
			break;

		case DRAW_TO:
			pensize = pt.width[pen_no];
			pdf_set_linewidth((double) pensize, md);
			pdf_set_linecap(CurrentLineAttr.End,
					(double) pensize, md);
			pdf_set_linejoin(CurrentLineAttr.Join,
					 CurrentLineAttr.Limit,
					 (double) pensize, md);

			pdf_set_color(pt.color[pen_no], md);

			HPGL_Pt_from_tmpfile(&pt1);
#ifdef PDFLIB
			PDF_lineto(md,
#else			
			pdf_contents_lineto(canvas,
#endif			
					(pt1.x - xmin) * xcoord2mm,
				   	(pt1.y - ymin) * ycoord2mm);
			openpath = 1;
			break;

		case PLOT_AT:
			pensize = pt.width[pen_no];

			pdf_set_color(pt.color[pen_no], md);

			HPGL_Pt_from_tmpfile(&pt1);
			PAGEMODE;
#ifdef PDFLIB
			PDF_save(md);
			PDF_setlinewidth(md, 0.00001);
			PDF_circle(md,(pt1.x - xmin) * xcoord2mm,
				   (pt1.y - ymin) * ycoord2mm,
				   pensize / 2 * MM_TO_PS_POINT);
			PDF_fill(md);
			PDF_restore(md);
#else				   
			pdf_contents_gsave(canvas);
			pdf_contents_moveto(canvas,(pt1.x-xmin)*xcoord2mm,
				  (pt1.y-ymin)*ycoord2mm); 
			pdf_contents_lineto(canvas,(pt1.x-xmin+0.00001)*xcoord2mm,
				  (pt1.y-ymin+0.00001)*ycoord2mm); 
			pdf_contents_stroke(canvas);	  
			pdf_contents_grestore(canvas);
#endif
			break;

		default:
			Eprintf("Illegal cmd in temp. file!");
			err = ERROR;
			goto PDF_exit;
		}
	}

	/* Finish up */
	PAGEMODE;
	return pdf_end(md,po);

PDF_exit:
	if (!pg->quiet)
		Eprintf("\n");
	return err;
}
