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
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "bresnham.h"
#include "hp2xx.h"


#define	A4_height	297	/* in [mm]	*/



extern	float	xmin, xmax, ymin, ymax;
static		linecount = 0;
static	float	coord2mm;




/**
 ** Close graphics file
 **/
void	ps_end (FILE *fd)
{
  fprintf(fd," S\neop\n");
  fprintf(fd, "@end\n");
  fprintf(fd, "%%%%PageTrailer\n");
  fprintf(fd, "%%%%Trailer\n");
  fprintf(fd, "%%%%EOF\n");
  fclose (fd);
  linecount = 0;
}



/**
 ** Flush old path and move
 **/
void	ps_stroke_and_move_to (HPGL_Pt *ppt, FILE *fd)
{
  fprintf(fd, " S\n%6.2f %6.2f M",	/* S: Start a new path	*/
		(ppt->x-xmin) * coord2mm, (ppt->y-ymin) * coord2mm);
  linecount = 0;
}




/**
 ** Set line width
 **/
void	ps_set_linewidth (double width, HPGL_Pt *ppt, FILE *fd)
{
  ps_stroke_and_move_to (ppt, fd);	/* MUST start a new path!	*/
  fprintf  (fd," %6.3f W\n", width);
}



/**
 ** Set RGB color
 **/
void	ps_set_color (double red, double green, double blue,
			HPGL_Pt *ppt, FILE *fd)
{
  ps_stroke_and_move_to (ppt, fd);	/* MUST start a new path!	*/
  fprintf  (fd," %6.3f %6.3f %6.3f C\n", red, green, blue);
}





void	ps_line_to (HPGL_Pt *ppt, char mode, FILE *fd)
{
  if (linecount > 3)
  {
      putc('\n', fd);
      linecount = 0;
  }
  else
      putc(' ', fd);

  fprintf(fd, "%6.2f %6.2f %c",
		(ppt->x-xmin) * coord2mm, (ppt->y-ymin) * coord2mm, mode);
  linecount++;
}





/**
 ** Get the date and time: This is optional, since its result only
 ** appeares in the PS header.
 **/

char	*Getdate (void)
{
int len;
long t;
char *p;

  t = time((long *) 0);
  p = ctime(&t);
  len = strlen(p);
  *(p + len - 1) = '\0';  /* zap the newline character */
  return p;
}



/**
 ** PostScript definitions
 **/

void	ps_init (PAR *p, FILE *fd)
{
long	left, right, low, high;
int	pensize;
double	hmxpenw;

  pensize = p->pensize[p->pen];
  hmxpenw = p->maxpensize / 20.0;	/* Half max. pen width, in mm	*/

/**
 ** Header comments into PostScript file
 **/

  fprintf(fd,"%%!PS-Adobe-2.0 EPSF-2.0\n");
  fprintf(fd,"%%%%Title: %s\n", p->outfile);
  fprintf(fd,"%%%%Creator: hp2xx (c) 1991, 1992 by H. Werntges\n");
  fprintf(fd,"%%%%CreationDate: %s\n", Getdate());
  fprintf(fd,"%%%%Pages: 1\n");

/**
 ** Bounding Box limits: Conversion factor: 2.834646 * 1/72" = 1 mm
 **
 ** (hmxpenw & floor/ceil corrections suggested by Eric Norum)
 **/

  left  = (long) floor((p->xoff-hmxpenw)		    * 2.834646);
  low   = (long) floor((A4_height-p->yoff-p->height-hmxpenw)* 2.834646);
  right = (long) ceil ((p->xoff   + p->width+hmxpenw)	    * 2.834646);
  high  = (long) ceil ((A4_height - p->yoff+hmxpenw)	    * 2.834646);
  fprintf(fd,"%%%%BoundingBox: %ld %ld %ld %ld\n", left, low, right, high);
  if (!p->quiet)
	fprintf(stderr,"Bounding Box: [%ld %ld %ld %ld]\n",
			left, low, right, high);

  fprintf(fd,"%%%%EndComments\n\n");

/**
 ** Definitions
 **/

  fprintf(fd,"%%%%BeginProcSet\n");
  fprintf(fd,"/PSSave save def\n");      /* save VM state */
  fprintf(fd,"/PSDict 200 dict def\n");  /* define a dictionary */
  fprintf(fd,"PSDict begin\n");          /* start using it */
  fprintf(fd,"/@restore /restore load def\n");
  fprintf(fd,"/restore\n");
  fprintf(fd,"   {vmstatus pop\n");
  fprintf(fd,"    dup @VMused lt {pop @VMused} if\n");
  fprintf(fd,"    exch pop exch @restore /@VMused exch def\n");
  fprintf(fd,"   } def\n");

  fprintf(fd,"/@pri\n");
  fprintf(fd,"   {\n");
  fprintf(fd,"    ( ) print\n");
  fprintf(fd,"    (                                       ) cvs print\n");
  fprintf(fd,"   } def\n");

  fprintf(fd,"/@start\n");    /* - @start -  -- start everything */
  fprintf(fd,"   {\n");
  fprintf(fd,"    vmstatus pop /@VMused exch def pop\n");
  fprintf(fd,"   } def\n");

  fprintf(fd,"/@end\n");      /* - @end -  -- finished */
  fprintf(fd,"   {");
  if (!p->quiet)
  {
    fprintf(fd,    "(VM Used: ) print @VMused @pri\n");
    fprintf(fd,"    (. Unused: ) print vmstatus @VMused sub @pri pop pop\n");
    fprintf(fd,"    (\\n) print flush\n");

  }
  fprintf(fd,"    end\n");
  fprintf(fd,"    PSSave restore\n");
  fprintf(fd,"   } def\n");

  fprintf(fd,"/bop\n");       /* bop -  -- begin a new page */
  fprintf(fd,"   {\n");
  fprintf(fd,"    /SaveImage save def\n");
  fprintf(fd,"   } def\n");

  fprintf(fd,"/eop\n");       /* - eop -  -- end a page */
  fprintf(fd,"   {\n");
  fprintf(fd,"    showpage\n");
  fprintf(fd,"    SaveImage restore\n");
  fprintf(fd,"   } def\n");

  fprintf(fd,"/@line\n");     /* set line parameters */
  fprintf(fd,"   {1 setlinecap  %%%% Replace 1 by 0 for cut-off lines\n");
  fprintf(fd,"    1 setlinejoin %%%% Replace 1 by 0 for cut-off lines\n");
  fprintf(fd,"%%%%    1 setmiterlimit    %%%%  Uncomment this for cut-off lines\n");
  fprintf(fd,"   } def\n");

  fprintf(fd,"/@SetPlot\n");
  fprintf(fd,"   {\n");
  fprintf(fd,"    2.834646 2.834646 scale\n");	/* 1/72"--> mm */
  fprintf(fd,"    %7.3f %7.3f translate\n", p->xoff,
			A4_height - p->yoff - p->height);
  fprintf(fd,"    %6.3f setlinewidth\n", pensize/10.0);
  fprintf(fd,"   } def\n");
  fprintf(fd,"/C {setrgbcolor} def\n");
  fprintf(fd,"/D {lineto} def\n");
  fprintf(fd,"/M {moveto} def\n");
  fprintf(fd,"/S {stroke} def\n");
  fprintf(fd,"/W {setlinewidth} def\n");
  fprintf(fd,"/Z {stroke newpath} def\n");
  fprintf(fd,"end\n");      /* end of dictionary definition */
  fprintf(fd,"%%%%EndProcSet\n\n");

/**
 ** Set up the plots
 **/

  fprintf(fd,"%%%%BeginSetup\n");
  fprintf(fd,"/#copies 1 def\n");
  fprintf(fd,"%%%%EndSetup\n");
  fprintf(fd,"%%%%Page: 1 1\n");
  fprintf(fd,"%%%%BeginPageSetup\n");
  fprintf(fd,"PSDict begin\n");
  fprintf(fd,"@start\n");
  fprintf(fd,"@line\n");
  fprintf(fd,"@SetPlot\n\n");
  fprintf(fd,"bop\n");
  fprintf(fd,"%%%%EndPageSetup\n");
}



/**
 ** Higher-level interface: Output Encapsulated PostScript format
 **/

void	to_eps (PAR *p, FILE *td)
{
PlotCmd	cmd;
HPGL_Pt	pt1 = {0};
FILE	*md;
int	pensize, pencolor;

  if (!p->quiet)
	fprintf(stderr,"\n\n- Writing EPS code to \"%s\"\n",
		*p->outfile == '-' ? "stdout" : p->outfile);

  /* Init. of PostScript file: */
  if (*p->outfile != '-')
  {
	if ((md = fopen(p->outfile, "w")) == NULL)
	{
		perror("hp2xx (eps)");
		exit(ERROR);
	}
  }
  else
	md = stdout;

  /* PS header */
  ps_init (p, md);

  /* Factor for transformation of HP coordinates to mm	*/

  coord2mm = p->height / (ymax-ymin);


  pensize = p->pensize[p->pen];
  if (pensize != 0)
	fprintf(md," %6.3f W\n", pensize/10.0);

/**
 ** Command loop: While temporaty file not empty process command.
 **/

  while ((cmd = PlotCmd_from_tmpfile()) != EOF)
  {
	switch (cmd)
	{
	  case NOP:
		break;
	  case SET_PEN:
		if ((p->pen = fgetc(td)) == EOF)
		{
			perror("Unexpected end of temp. file: ");
			exit (ERROR);
		}
		pensize = p->pensize[p->pen];
		if (pensize != 0)
			ps_set_linewidth ((double) pensize/10.0, &pt1, md);
		pencolor = p->pencolor[p->pen];
		ps_set_color (  p->Clut[pencolor][0]/255.0,
				p->Clut[pencolor][1]/255.0,
				p->Clut[pencolor][2]/255.0,
				&pt1, md);
		break;
	  case MOVE_TO:
		HPGL_Pt_from_tmpfile (&pt1);
		if (pensize != 0)
			ps_stroke_and_move_to (&pt1, md);
		break;
	  case DRAW_TO:
		HPGL_Pt_from_tmpfile (&pt1);
		if (pensize != 0)
			ps_line_to (&pt1, 'D', md);
		break;
	  case PLOT_AT:
		HPGL_Pt_from_tmpfile (&pt1);
		if (pensize != 0)
		{
			ps_line_to (&pt1, 'M', md);
			ps_line_to (&pt1, 'D', md);
		}
		break;
	  default:
		fprintf(stderr,"Illegal cmd in temp. file!");
		exit (ERROR);
	}
  }

  /* Finish up */

  ps_end (md);
  if (md != stdout)
	fclose (md);

  if (!p->quiet)
	fputc ('\n', stderr);
}

