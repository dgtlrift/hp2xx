/*
   Copyright (c) 1991 - 1994 Heinz W. Werntges.  All rights reserved.
   Parts Copyright (c) 1999  Martin Kroeker  All rights reserved.
   
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

/** to_eps.c: 	Converter to Encapsulated PostScript format;
 **		(derived from PLPLOT driver  "postscript.c")
 **
 ** 91/06/29  V 1.00  HWW  Derived from postscript.c
 ** 91/10/15  V 1.01  HWW  ANSI_C
 ** 91/11/20  V 1.02  HWW  Changes due to "SPn;"
 ** 91/12/22  V 1.02a HWW  Many small changes
 ** 92/05/17  V 1.02b HWW  Output to stdout if outfile == '-'
 ** 92/05/24  V 1.10a HWW  Color supported (RGB)
 ** 92/10/20  V 1.10b HWW  Bug fix: Setting color implies a new path
 ** 92/12/12  V 1.10c HWW  Info line now interprets outfile=='-' as "stdout"
 ** 93/04/12  V 1.10d HWW  Explicit prototypes for time() and ctime() removed;
 **			   VMstatus reports only if !p->quiet;
 **			   BoundingBox calc.: roundinf included (floor, ceil)
 ** 93/04/25  V 1.10e HWW  BoundingBox corrected for (half) max. pen width
 ** 93/11/15  V 1.11a HWW  EPS syntax corrections (courtesy N. H. F. Beebe)
 ** 94/02/15  V 1.20a HWW  Adapted to changes in hp2xx.h
 ** 01/12/04          MK   Added missing colon to BeginProcSet (Bengt-Arne Fjellner)
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "bresnham.h"
#include "hp2xx.h"
#include "pendef.h"
#include "lindef.h"


/*#define	A4_height	297*//* in [mm]      */



static int linecount = 0;
static float xcoord2mm, ycoord2mm;
static float xmin, ymin;

/* globals for states since we have to init them for each file now */
static PEN_W lastwidth;
static int lastcap;
static int lastjoin;
static int lastlimit;
static BYTE lastred, lastgreen, lastblue;

void ps_set_linewidth(PEN_W, HPGL_Pt *, FILE *);
void ps_set_linecap(LineEnds type, PEN_W pensize, HPGL_Pt * ppt,
		    FILE * fd);
void ps_set_linejoin(LineJoins type, LineLimit limit, PEN_W pensize,
		     HPGL_Pt * ppt, FILE * fd);
void ps_set_color(PEN_C pencolor, HPGL_Pt * ppt, FILE * fd);
void ps_init(const GEN_PAR *, const OUT_PAR *, FILE *, PEN_W);
void ps_end(FILE *);
void ps_stroke_and_move_to(HPGL_Pt *, FILE *);
void ps_line_to(HPGL_Pt *, char, FILE *);
void ps_draw_dot(HPGL_Pt * ppt, double radius, FILE * fd);
char *Getdate(void);



/**
 ** Close graphics file
 **/
void ps_end(FILE * fd)
{
	fprintf(fd, " S\neop\n");
	fprintf(fd, "@end\n");
	fprintf(fd, "%%%%PageTrailer\n");
	fprintf(fd, "%%%%Trailer\n");
	fprintf(fd, "%%%%EOF\n");
	linecount = 0;
}



/**
 ** Flush old path and move
 **/
void ps_stroke_and_move_to(HPGL_Pt * ppt, FILE * fd)
{
	fprintf(fd, " S\n%6.2f %6.2f M",	/* S: Start a new path  */
		(ppt->x - xmin) * xcoord2mm, (ppt->y - ymin) * ycoord2mm);
	linecount = 0;
}

/**
 ** Set line width
 **/
void ps_set_linewidth(PEN_W width, HPGL_Pt * ppt, FILE * fd)
{
	if ((fabs(width - lastwidth) >= 0.01) && (width >= 0.05)) {
		ps_stroke_and_move_to(ppt, fd);	/* MUST start a new path!      */
		fprintf(fd, " %6.3f W\n", width);
		lastwidth = width;
	}
	return;
}

/**
 ** Set line ends
 **/
void ps_set_linecap(LineEnds type, PEN_W pensize, HPGL_Pt * ppt, FILE * fd)
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
		ps_stroke_and_move_to(ppt, fd);	/* MUST start a new path! */
		fprintf(fd, " %d setlinecap\n", newcap);
		lastcap = newcap;
	}

	return;
}

/**
 ** Set line Joins
 **/
void ps_set_linejoin(LineJoins type, LineLimit limit, PEN_W pensize,
		     HPGL_Pt * ppt, FILE * fd)
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
		ps_stroke_and_move_to(ppt, fd);	/* MUST start a new path! */
		fprintf(fd, " %d setlinejoin\n", newjoin);
		lastjoin = newjoin;
	}

	if (newlimit != lastlimit) {
		ps_stroke_and_move_to(ppt, fd);	/* MUST start a new path! */
		fprintf(fd, " %d setmiterlimit\n", newlimit);
		lastlimit = newlimit;
	}

}


/**
 ** Set RGB color
 **/
void ps_set_color(PEN_C pencolor, HPGL_Pt * ppt, FILE * fd)
{
	if ((pt.clut[pencolor][0] != lastred) ||
	    (pt.clut[pencolor][1] != lastgreen)
	    || (pt.clut[pencolor][2] != lastblue)) {


		ps_stroke_and_move_to(ppt, fd);	/* MUST start a new path!       */
		fprintf(fd, " %6.3f %6.3f %6.3f C\n",
			(double) pt.clut[pencolor][0] / 255.0,
			(double) pt.clut[pencolor][1] / 255.0,
			(double) pt.clut[pencolor][2] / 255.0);

		lastred = pt.clut[pencolor][0];
		lastgreen = pt.clut[pencolor][1];
		lastblue = pt.clut[pencolor][2];
	} else {
		return;
	}
}

void ps_line_to(HPGL_Pt * ppt, char mode, FILE * fd)
{
	if (linecount > 3) {
		putc('\n', fd);
		linecount = 0;
	} else
		putc(' ', fd);

	fprintf(fd, "%6.2f %6.2f %c",
		(ppt->x - xmin) * xcoord2mm, (ppt->y - ymin) * ycoord2mm,
		mode);
	linecount++;
}

void ps_draw_dot(HPGL_Pt * ppt, double radius, FILE * fd)
{
	fprintf(fd, " currentpoint newpath %0.2f 0 360 arc fill\n",
		radius);
}


/**
 ** Get the date and time: This is optional, since its result only
 ** appeares in the PS header.
 **/

char *Getdate(void)
{
	int len;
	time_t t;
	char *p;

	t = time((time_t *) 0);
	p = ctime(&t);
	len = (int) strlen(p);
							/*@-modobserver@ */ *(p + len - 1) = '\0';
							/* zap the newline character */
	return p;
}



/**
 ** PostScript definitions
 **/

void ps_init(const GEN_PAR * pg, const OUT_PAR * po, FILE * fd,
	     PEN_W pensize)
{
	long left, right, low, high;
	double hmxpenw;

	lastwidth = -1.0;
	lastcap = lastjoin = lastlimit = -1;
	lastred = lastgreen = lastblue = -1;

	hmxpenw = pg->maxpensize / 2.0;	/* Half max. pen width, in mm   */

/**
 ** Header comments into PostScript file
 **/

	fprintf(fd, "%%!PS-Adobe-2.0 EPSF-2.0\n");
	fprintf(fd, "%%%%Title: %s\n", po->outfile);
	fprintf(fd,
		"%%%%Creator: hp2xx %s (c) 1991-1994 by H. Werntges, 1999-2003 by M. Kroeker\n",VERS_NO);
	fprintf(fd, "%%%%CreationDate: %s\n", Getdate());
	fprintf(fd, "%%%%Pages: 1\n");

/**
 ** Bounding Box limits: Conversion factor: 2.834646 * 1/72" = 1 mm
 **
 ** (hmxpenw & floor/ceil corrections suggested by Eric Norum)
 **/
	left = (long) floor(fabs(po->xoff - hmxpenw) * MM_TO_PS_POINT);
	low = (long) floor(fabs(po->yoff - hmxpenw) * MM_TO_PS_POINT);
	right =
	    (long) ceil((po->xoff + po->width + hmxpenw) * MM_TO_PS_POINT);
	high =
	    (long) ceil((po->yoff + po->height + hmxpenw) *
			MM_TO_PS_POINT);
	fprintf(fd, "%%%%BoundingBox: %ld %ld %ld %ld\n", left, low, right,
		high);
	if (!pg->quiet)
		Eprintf("Bounding Box: [%ld %ld %ld %ld]\n", left, low,
			right, high);

	fprintf(fd, "%%%%EndComments\n\n");

/**
 ** Definitions
 **/

	fprintf(fd, "%%%%BeginProcSet:\n");
	fprintf(fd, "/PSSave save def\n");	/* save VM state */
	fprintf(fd, "/PSDict 200 dict def\n");	/* define a dictionary */
	fprintf(fd, "PSDict begin\n");	/* start using it */
	fprintf(fd, "/@restore /restore load def\n");
	fprintf(fd, "/restore\n");
	fprintf(fd, "   {vmstatus pop\n");
	fprintf(fd, "    dup @VMused lt {pop @VMused} if\n");
	fprintf(fd, "    exch pop exch @restore /@VMused exch def\n");
	fprintf(fd, "   } def\n");

	fprintf(fd, "/@pri\n");
	fprintf(fd, "   {\n");
	fprintf(fd, "    ( ) print\n");
	fprintf(fd,
		"    (                                       ) cvs print\n");
	fprintf(fd, "   } def\n");

	fprintf(fd, "/@start\n");	/* - @start -  -- start everything */
	fprintf(fd, "   {\n");
	fprintf(fd, "    vmstatus pop /@VMused exch def pop\n");
	fprintf(fd, "   } def\n");

	fprintf(fd, "/@end\n");	/* - @end -  -- finished */
	fprintf(fd, "   {");
	if (!pg->quiet) {
		fprintf(fd, "(VM Used: ) print @VMused @pri\n");
		fprintf(fd,
			"    (. Unused: ) print vmstatus @VMused sub @pri pop pop\n");
		fprintf(fd, "    (\\n) print flush\n");

	}
	fprintf(fd, "    end\n");
	fprintf(fd, "    PSSave restore\n");
	fprintf(fd, "   } def\n");

	fprintf(fd, "/bop\n");	/* bop -  -- begin a new page */
	fprintf(fd, "   {\n");
	fprintf(fd, "    /SaveImage save def\n");
	fprintf(fd, "   } def\n");

	fprintf(fd, "/eop\n");	/* - eop -  -- end a page */
	fprintf(fd, "   {\n");
	fprintf(fd, "    showpage\n");
	fprintf(fd, "    SaveImage restore\n");
	fprintf(fd, "   } def\n");

	fprintf(fd, "/@line\n");	/* set line parameters */
	fprintf(fd, "   {\n");
	fprintf(fd,
		"%%%%    1 setlinejoin      %%%% now set from LA command\n");
	fprintf(fd,
		"%%%%    1 setmiterlimit    %%%% now set from LA command\n");
	fprintf(fd, "   } def\n");

	fprintf(fd, "/@SetPlot\n");
	fprintf(fd, "   {\n");
	fprintf(fd, "    %f %f scale\n", MM_TO_PS_POINT, MM_TO_PS_POINT);	/* 1/72"--> mm */
	fprintf(fd, "    %7.3f %7.3f translate\n", po->xoff + hmxpenw,
		po->yoff + hmxpenw);
	fprintf(fd, "    %6.3f setlinewidth\n", pensize);
	fprintf(fd, "   } def\n");
	fprintf(fd, "/C {setrgbcolor} def\n");
	fprintf(fd, "/D {lineto} def\n");
	fprintf(fd, "/M {moveto} def\n");
	fprintf(fd, "/S {stroke} def\n");
	fprintf(fd, "/W {setlinewidth} def\n");
	fprintf(fd, "/Z {stroke newpath} def\n");
	fprintf(fd, "end\n");	/* end of dictionary definition */
	fprintf(fd, "%%%%EndProcSet\n\n");

/**
 ** Set up the plots
 **/

	fprintf(fd, "%%%%BeginSetup\n");
	fprintf(fd, "/#copies 1 def\n");
	fprintf(fd, "%%%%EndSetup\n");
	fprintf(fd, "%%%%Page: 1 1\n");
	fprintf(fd, "%%%%BeginPageSetup\n");
	fprintf(fd, "PSDict begin\n");
	fprintf(fd, "@start\n");
	fprintf(fd, "@line\n");
	fprintf(fd, "@SetPlot\n\n");
	fprintf(fd, "bop\n");
	fprintf(fd, "%%%%EndPageSetup\n");
}

/**
 ** Higher-level interface: Output Encapsulated PostScript format
 **/

int to_eps(const GEN_PAR * pg, const OUT_PAR * po)
{
	PlotCmd cmd;
	HPGL_Pt pt1 = { 0, 0 };
	FILE *md;
	int pen_no = 0, err;
	PEN_W pensize;

	err = 0;
	if (!pg->quiet)
		Eprintf("\n\n- Writing EPS code to \"%s\"\n",
			*po->outfile == '-' ? "stdout" : po->outfile);

	/* Init. of PostScript file: */
	if (*po->outfile != '-') {
		if ((md = fopen(po->outfile, "w")) == NULL) {
			PError("hp2xx (eps)");
			return ERROR;
		}
	} else
		md = stdout;

	/* PS header */

	pensize = pt.width[DEFAULT_PEN_NO];	/* Default pen    */
	ps_init(pg, po, md, pensize);

	if (pensize > 0.05)
		fprintf(md, " %6.3f W\n", pensize);

	/* Factor for transformation of HP coordinates to mm  */

	xcoord2mm = po->width / (po->xmax - po->xmin);
	ycoord2mm = po->height / (po->ymax - po->ymin);
	xmin = po->xmin;
	ymin = po->ymin;

/**
 ** Command loop: While temporary file not empty: process command.
 **/

	while ((cmd = PlotCmd_from_tmpfile()) != CMD_EOF) {
		switch (cmd) {
		case NOP:
			break;
		case SET_PEN:
			if ((pen_no = fgetc(pg->td)) == EOF) {
				PError("Unexpected end of temp. file: ");
				err = ERROR;
				goto EPS_exit;
			}
			pensize = pt.width[pen_no];
			break;
		case DEF_PW:
			if (!load_pen_width_table(pg->td)) {
				PError("Unexpected end of temp. file");
				err = ERROR;
				goto EPS_exit;
			}
			pensize = pt.width[pen_no];
			break;
		case DEF_PC:
			err = load_pen_color_table(pg->td);
			if (err < 0) {
				PError("Unexpected end of temp. file");
				err = ERROR;
				goto EPS_exit;
			}
			break;
		case DEF_LA:
			if (load_line_attr(pg->td) < 0) {
				PError("Unexpected end of temp. file");
				err = ERROR;
				goto EPS_exit;
			}
			break;
		case MOVE_TO:
			ps_set_linewidth(pensize, &pt1, md);
			ps_set_linecap(CurrentLineAttr.End, pensize, &pt1,
				       md);
			ps_set_linejoin(CurrentLineAttr.Join,
					CurrentLineAttr.Limit, pensize,
					&pt1, md);
			ps_set_color(pt.color[pen_no], &pt1, md);

			HPGL_Pt_from_tmpfile(&pt1);
			if (pensize > 0.05)
				ps_stroke_and_move_to(&pt1, md);
			break;
		case DRAW_TO:
			ps_set_linewidth(pensize, &pt1, md);
			ps_set_linecap(CurrentLineAttr.End, pensize, &pt1,
				       md);
			ps_set_linejoin(CurrentLineAttr.Join,
					CurrentLineAttr.Limit, pensize,
					&pt1, md);
			ps_set_color(pt.color[pen_no], &pt1, md);

			HPGL_Pt_from_tmpfile(&pt1);
			if (pensize > 0.05)
				ps_line_to(&pt1, 'D', md);
			break;
		case PLOT_AT:
			ps_set_linewidth(pensize, &pt1, md);
			ps_set_linecap(CurrentLineAttr.End, pensize, &pt1,
				       md);
			ps_set_linejoin(CurrentLineAttr.Join,
					CurrentLineAttr.Limit, pensize,
					&pt1, md);
			ps_set_color(pt.color[pen_no], &pt1, md);

			HPGL_Pt_from_tmpfile(&pt1);
			if (pensize > 0.05) {
				ps_line_to(&pt1, 'M', md);
				ps_line_to(&pt1, 'D', md);	/* not sure whether this is needed */
				ps_draw_dot(&pt1, pensize / 2, md);
			}
			break;
		default:
			Eprintf("Illegal cmd in temp. file!");
			err = ERROR;
			goto EPS_exit;
		}
	}

	/* Finish up */

	ps_end(md);

      EPS_exit:
	if (md != stdout)
		fclose(md);

	if (!pg->quiet)
		Eprintf("\n");
	return err;
}
