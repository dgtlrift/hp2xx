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

/** to_ps.c: 	Converter to PostScript format;
 **		(derived from "to_eps.c")
 **             This driver uses the high-level postscript facilities
 **             to fill and to edge polygons and rectangles
 **
 ** 03/03/03    ED  Initial version
 ** 07/04/03    ED  Fixed some bugs:
 **               extra colon after BeginProlog
 **               possible postscript nocurrentpoint exception after IW
 ** 08/04/03    ED  added PS L1 support for fill types 3 and 4
 ** 09/04/03    ED  added EPS support
 ** 09/04/03    ED  added media selection and resource inclusion support for PS files
 ** 11/04/03    ED  fixed incorrect dot size
 **/

/** Known bugs and things to do
 **  1. Some plots with IW and no PS may get incorrect (larger) bounding box
 **  2. RO command must rotate contents of the polygon bufer, it doesn't
 **  6. line patterns not supported for fill types 3 and 4
 **  8. only fill types 1-4 are implemented
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
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
static BYTE lastred, lastgreen,lastblue;

static short eps=FALSE;



void ps_set_linewidth(PEN_W, HPGL_Pt *, FILE *);
void ps_set_linecap(LineEnds type, PEN_W pensize, HPGL_Pt * ppt, FILE * fd);
void ps_set_linejoin(LineJoins type, LineLimit limit, PEN_W pensize, HPGL_Pt * ppt,
                     FILE * fd);
void ps_set_color(PEN_C pencolor, HPGL_Pt * ppt, FILE * fd);
void ps_init(const GEN_PAR *, const OUT_PAR *, FILE *, PEN_W);
void ps_end(FILE *);
void ps_stroke_and_move_to(HPGL_Pt *, FILE *);
void ps_line_to(HPGL_Pt *, char, FILE *);
char *Getdate(void);




/**
 ** Close graphics file
 **/
void ps_end(FILE * fd)
{

  if (eps) {
    fprintf(fd, " noclip\n");
  }
  fprintf(fd, " S\n");
  fprintf(fd, "showpage\n");
  fprintf(fd, "end\n");
  if (eps) {
    fprintf(fd, "hp2xxpagesave restore\n");
  } else {
    fprintf(fd, "%%%%PageTrailer\n\n");
    fprintf(fd, "%%%%Trailer\n\n");
  }
  fprintf(fd, "%%%%EOF\n");
  linecount = 0;
}

/**
 ** Flush old path and move
 **/
void ps_stroke_and_move_to(HPGL_Pt * ppt, FILE * fd)
{
  fprintf(fd, " S\n%6.2f %6.2f M",     /* S: Start a new path  */
	  (ppt->x - xmin) * xcoord2mm, (ppt->y - ymin) * ycoord2mm);
  linecount = 0;
}

/**
 ** Set line width
 **/
void ps_set_linewidth(PEN_W width, HPGL_Pt * ppt, FILE * fd)
{
  if ((fabs(width - lastwidth) >= 0.01) && (width >= 0.05)) {
    ps_stroke_and_move_to(ppt, fd);   /* MUST start a new path!      */
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
    case LAE_triangular:   /* triangular not implemented in PS/PDF */
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
    ps_stroke_and_move_to(ppt, fd);   /* MUST start a new path! */
    fprintf(fd, " %d setlinecap\n", newcap);
    lastcap = newcap;
  }

  return;
}

/**
 ** Set line Joins
 **/
void ps_set_linejoin(LineJoins type, LineLimit limit, PEN_W pensize, HPGL_Pt * ppt,
                     FILE * fd)
{
  int newjoin;
  int newlimit = lastlimit;

  if (pensize > 0.35) {
    switch (type) {
    case LAJ_plain_miter:
      newjoin = 0;
      newlimit = 5;      /* arbitrary value */
      break;
    case LAJ_bevel_miter:  /* not available */
      newjoin = 0;
      newlimit = limit;
      break;
    case LAJ_triangular:   /* not available */
      newjoin = 1;
      break;
    case LAJ_round:
      newjoin = 1;
      break;
    case LAJ_bevelled:
      newjoin = 2;
      break;
    case LAJ_nojoin:       /* not available */
      newjoin = 1;
      break;
    default:
      newjoin = 0;
      newlimit = 5;      /* arbitrary value */
      break;
    }
  } else {
    newjoin = 1;
  }

  if (newjoin != lastjoin) {
    ps_stroke_and_move_to(ppt, fd);   /* MUST start a new path! */
    fprintf(fd, " %d setlinejoin\n", newjoin);
    lastjoin = newjoin;
  }

  if (newlimit != lastlimit) {
    ps_stroke_and_move_to(ppt, fd);   /* MUST start a new path! */
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
      (pt.clut[pencolor][1] != lastgreen) || (pt.clut[pencolor][2] != lastblue)) {


    ps_stroke_and_move_to(ppt, fd);   /* MUST start a new path!       */
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
	  (ppt->x - xmin) * xcoord2mm, (ppt->y - ymin) * ycoord2mm, mode);
  linecount++;
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
  len = (int)strlen(p);
  /*@-modobserver@*/*(p + len - 1) = '\0';       /* zap the newline character */
  return p;
}



/**
 ** PostScript definitions
 **/

void ps_init(const GEN_PAR * pg, const OUT_PAR * po, FILE * fd, PEN_W pensize)
{
  long left, right, low, high;
  double hmxpenw;
  int c;
  FILE *incfd;

  lastwidth = -1.0;
  lastcap = lastjoin = lastlimit = -1;
  lastred = lastgreen = lastblue = -1;

  hmxpenw = pg->maxpensize / 2.0;      /* Half max. pen width, in mm   */

  /**
   ** Header comments into PostScript file
   **/

  fprintf(fd, "%%!PS-Adobe-3.0");
  if (eps) {
    fprintf(fd, " EPSF-3.0");
  }
  fprintf(fd, "\n");
  fprintf(fd, "%%%%Title: %s\n", po->outfile);
  fprintf(fd, "%%%%Creator: hp2xx %s (%s), %s\n",VERS_NO, VERS_DATE, VERS_COPYRIGHT);
  fprintf(fd, "%%%%CreationDate: %s\n", Getdate());
  if (!eps) {
    fprintf(fd, "%%%%Pages: 1\n");
  }

  /**
   ** Bounding Box limits: Conversion factor: 2.834646 * 1/72" = 1 mm
   **
   ** (hmxpenw & floor/ceil corrections suggested by Eric Norum)
   **/
  left = (long) floor(fabs(po->xoff - hmxpenw) * MM_TO_PS_POINT);
  low = (long) floor(fabs(po->yoff - hmxpenw) * MM_TO_PS_POINT);
  right = (long) ceil((po->xoff + po->width + hmxpenw) * MM_TO_PS_POINT);
  high = (long) ceil((po->yoff + po->height + hmxpenw) * MM_TO_PS_POINT);
   
  fprintf(fd, "%%%%BoundingBox: %ld %ld %ld %ld\n", left, low, right, high);
  if (!pg->quiet)
    Eprintf("Bounding Box: [%ld %ld %ld %ld]\n", left, low, right, high);
  fprintf(fd, "%%%%EndComments\n\n");

  fprintf(fd, "%%%%BeginProlog\n");
  fprintf(fd, "/PSDict 200 dict def\n");       /* define a dictionary */
  fprintf(fd, "PSDict begin\n");       /* start using it */
  fprintf(fd, "/@SetPlot\n");
  fprintf(fd, "   {\n");
  fprintf(fd, "    %f %f scale\n", MM_TO_PS_POINT, MM_TO_PS_POINT);    /* 1/72"--> mm */
  fprintf(fd, "    %7.3f %7.3f translate\n", po->xoff + hmxpenw,po->yoff + hmxpenw);
  fprintf(fd, "   } def\n");

  fprintf(fd, "/plotwidth %ld def /plotheight %ld def /defer %s def\n",right-left,high-low,po->ps_defer?"true":"false");
  fprintf(fd, "/C {setrgbcolor} def\n");
  fprintf(fd, "/D {lineto} def\n");
  fprintf(fd, "/M {moveto} def\n");
  fprintf(fd, "/S {stroke} def\n");
  fprintf(fd, "/W {setlinewidth} def\n");

  if (eps) {
    /* define variables before they are used by "bound" procedures, 
       this is recommended practice for EPS programs
    */
    fprintf(fd,"/xs 0 def /ys 0 def /xpos 0 def /ypos 0 def \n");
    fprintf(fd,"/angle 0 def /mlen 0 def /ff 0 def /fh 0 def /fw 0 def\n");
    fprintf(fd,"/bright 0 def /bleft 0 def /btop 0 def /bbottom 0 def \n"); 
    fprintf(fd,"/dw 0 def /dh 0 def /epsclip false def\n"); 
  }

  /* the pbuf variable stores polyline buffer */
  /* differences between HPGL/2 polyline buffers and postscript paths:                           */
  /* 1. The FP instruction fills between points defined with either the pen down or the pen up.  */
  /*    The fill and eofill operators do not fill between points defined with moveto             */
  /* 2. PM1 Closes the current polygon (or subpolygon) with a pen- up edge                       */
  /*    closepath closes current subpath, i.e. adds a line to the point of the most recent moveto*/
  /* so:                                                                                         */
  /* when we want to fill the polyline buffer we treat all the movetos as linetos, except the     */
  /* one between subpolygons                                                                     */
  fprintf(fd,"/polyfill false def\n");
  fprintf(fd,"/ssp {currentpoint dup /ys exch def /ypos exch def dup /xs exch def /xpos exch def } bind def\n");
  fprintf(fd,"/rsp {xs ys moveto} bind def\n");
  fprintf(fd,"/scp {currentpoint /ypos exch def /xpos exch def} bind def\n");
  fprintf(fd,"/rcp {xpos ypos moveto} bind def\n");
  fprintf(fd,"/CSP {rcp} def\n");
  fprintf(fd,"/clpbuf {currentpoint ypos eq exch xpos eq and not {xpos ypos M} if \n");
  fprintf(fd,"        /D {lineto} def  /M {moveto} def /CSP {rcp} def /polyfill false def\n");
  fprintf(fd,"} def\n");
  fprintf(fd,"/sfill {/M {lineto} def /polyfill true def /CSP {closepath} def} def\n");
  fprintf(fd,"/subpoly { CSP \n");
  fprintf(fd,"          /SM /M load def /SD /D load def\n");
  fprintf(fd,"          /D { moveto scp /D /SD load def /M /SM load def } def\n");
  fprintf(fd,"          /M { moveto scp /D /SD load def /M /SM load def } def\n");
  fprintf(fd,"} def\n");
  
  /* get language level of the  interpreter */
  fprintf(fd,"/languagelevel where {\n");
  fprintf(fd,"  pop\n");
  fprintf(fd,"  /pslevel languagelevel def \n");
  fprintf(fd,"} {\n");
  fprintf(fd,"  /pslevel 1 def\n");
  fprintf(fd,"} ifelse\n");
#if 1
  /* ToastScript is a shareware ps level 1 interpreter written in Java. I used it for testing */
  /* It returns languagelevel=2 and supports some L2 functionality (i.e. setpagedevice )      */
  /* but not pattern colorspaces      */
  fprintf(fd,"product (ToastScript) anchorsearch pslevel 2 eq and {\n");
  fprintf(fd,"  pop pop /pslevel 1 def");
  fprintf(fd,"} { pop } ifelse\n");
#endif

  /* define filling functions depending on the language level*/
  fprintf(fd,"pslevel 1 eq {\n");
  fprintf(fd,"  /setfill {\n");
  fprintf(fd,"    gsave\n");
  fprintf(fd,"      angle 90 gt {\n");
  fprintf(fd,"	bright btop translate\n");
  fprintf(fd,"      } {\n");
  fprintf(fd,"	bright bbottom translate\n");
  fprintf(fd,"      } ifelse\n");
  fprintf(fd,"      angle rotate\n");
  fprintf(fd,"      /mlen bright bleft sub dup mul btop bbottom sub dup mul add sqrt def\n");
  fprintf(fd,"      0 ff mlen {\n");
  fprintf(fd,"	dup\n");
  fprintf(fd,"	mlen neg  exch  moveto\n");
  fprintf(fd,"	mlen exch lineto\n");
  fprintf(fd,"      } for\n");
  fprintf(fd,"      stroke\n");
  fprintf(fd,"    grestore\n");
  fprintf(fd,"  } bind def\n");
  fprintf(fd,"  /hfill {\n");
  fprintf(fd,"    /filltype exch def \n");
  fprintf(fd,"    filltype 3 eq filltype 4 eq or filltype -3 eq or { \n");
  fprintf(fd,"      /angle exch def \n");
  fprintf(fd,"      /fw exch def \n");
  fprintf(fd,"      /fh exch def \n");
  fprintf(fd,"      /ff exch def \n");
  fprintf(fd,"      pathbbox\n");
  fprintf(fd,"      /btop exch def\n");
  fprintf(fd,"      /bright exch def\n");
  fprintf(fd,"      /bbottom exch def\n");
  fprintf(fd,"      /bleft exch def\n");
  fprintf(fd,"      gsave\n");
  fprintf(fd,"	dup /eo eq {eoclip} {clip} ifelse   \n");
  fprintf(fd,"	setfill\n");
  fprintf(fd,"	filltype 4 eq {\n");
  fprintf(fd,"	  /angle angle 90 add dup 180 ge { 180 sub} if def \n");
  fprintf(fd,"	  /eo eq {eoclip} {clip} ifelse   \n");
  fprintf(fd,"	  setfill\n");
  fprintf(fd,"	} { pop } ifelse\n");
  fprintf(fd,"      grestore\n");
  fprintf(fd,"    } {  \n");
  fprintf(fd,"      /eo eq {eofill} {fill} ifelse   \n");
  fprintf(fd,"    } ifelse\n");
  fprintf(fd,"    /M { \n");
  fprintf(fd,"      moveto \n");
  fprintf(fd,"    } def \n");
  fprintf(fd,"  } bind def\n");
  fprintf(fd,"} {\n");
  fprintf(fd,"  %% PS L2 fill\n");
  fprintf(fd,"  /setfill {    \n");
  fprintf(fd,"    10 dict	\n");
  fprintf(fd,"    dup /XStep fw put\n");
  fprintf(fd,"    dup /YStep fh put\n");
  fprintf(fd,"    dup /BBox [ dw neg dh neg  fw dw add fh dh add ] bind put\n");
  fprintf(fd,"    dup /PatternType 1 put\n");
  fprintf(fd,"    dup /PaintType 1 put\n");
  fprintf(fd,"    dup /TilingType 2  put\n");
  fprintf(fd,"    dup /PaintProc { \n");
  fprintf(fd,"      begin gsave \n");
  fprintf(fd,"      cr cg cb setrgbcolor clw setlinewidth 2 setlinecap \n");
  fprintf(fd,"	angle 90 gt { 0 fh moveto fw 0 lineto \n");
  fprintf(fd,"	} {  angle 0 eq { 0 0 moveto fw 0 lineto \n");
  fprintf(fd,"	} {  angle 90 eq {0 0 moveto 0 fh lineto }  \n");
  fprintf(fd,"	  {0 0 moveto fw fh lineto } ifelse \n");
  fprintf(fd,"	} ifelse \n");
  fprintf(fd,"	} ifelse \n");
  fprintf(fd,"	stroke  \n");
  fprintf(fd,"      grestore  \n");
  fprintf(fd,"    end \n");
  fprintf(fd,"    } put\n");
  fprintf(fd,"    matrix makepattern setcolor \n");
  fprintf(fd,"  } bind def\n");
  fprintf(fd,"\n");
  fprintf(fd,"  /hfill {  \n");
  fprintf(fd,"    /filltype exch def \n");
  fprintf(fd,"    filltype 3 eq filltype 4 eq or filltype -3 eq or { \n");
  fprintf(fd,"      /angle exch def \n");
  fprintf(fd,"      /fw exch def \n");
  fprintf(fd,"      /fh exch def \n");
  fprintf(fd,"      pop \n");
  fprintf(fd,"      currentrgbcolor /cb exch def /cg exch def /cr exch def \n");
  fprintf(fd,"      /clw currentlinewidth def \n");
  fprintf(fd,"      fh dup mul fw dup mul add sqrt clw mul 2 div dup fw div /dh exch def fh div /dw exch def \n");
  fprintf(fd,"      /Pattern setcolorspace \n");
  fprintf(fd,"      setfill \n");
  fprintf(fd,"      filltype 4 eq { \n");
  fprintf(fd,"	dup \n");
  fprintf(fd,"	gsave /eo eq {eofill} {fill} ifelse grestore \n");
  fprintf(fd,"	fw fh /fw exch def /fh exch def \n");
  fprintf(fd,"	dw dh /dw exch def /dh exch def \n");
  fprintf(fd,"	/angle angle 90 add dup 180 ge { 180 sub} if def \n");
  fprintf(fd,"	setfill \n");
  fprintf(fd,"      } if  \n");
  fprintf(fd,"    } if  \n");
  fprintf(fd,"    /eo eq {eofill} {fill} ifelse   \n");
  fprintf(fd,"    /M { \n");
  fprintf(fd,"      moveto \n");
  fprintf(fd,"    } def \n");
  fprintf(fd,"  } bind def\n");
  fprintf(fd,"} ifelse\n");

  /* define clipping function, initclip is musch faster then  gsave/grestore */
  /* but breaks EPS conformance                                              */
  fprintf(fd,"/setclip {\n");
  if (eps) {
    fprintf(fd," currentpoint  stroke\n");
    fprintf(fd," epsclip {\n");
    fprintf(fd,"   currentlinewidth currentrgbcolor\n");
    fprintf(fd,"   grestore\n");
    fprintf(fd,"   setrgbcolor setlinewidth\n");
    fprintf(fd," } {\n");
    fprintf(fd,"   /epsclip true def\n");
    fprintf(fd," } ifelse\n");
    fprintf(fd," gsave\n");
  } else {
    fprintf(fd," currentpoint stroke initclip\n");
  }
  fprintf(fd," 10 2 roll moveto lineto lineto lineto closepath clip newpath moveto\n } bind def\n");
  fprintf(fd,"/noclip {\n");
  if (eps) {
    fprintf(fd,"epsclip {\n");
    fprintf(fd,"  currentpoint\n");
    fprintf(fd,"  stroke\n");
    fprintf(fd,"  currentlinewidth currentrgbcolor\n");
    fprintf(fd,"  grestore\n");
    fprintf(fd,"  setrgbcolor setlinewidth moveto\n");
    fprintf(fd,"  /epsclip false def\n");
    fprintf(fd,"} if\n");
  } else {
    fprintf(fd," initclip\n");
  }
  fprintf(fd,"} bind def\n");

  /* 'currentpoint lineto' produces a nice dot when linecap==1 */
  fprintf(fd,"/dot {\n");
  fprintf(fd,"        currentlinecap 1 eq polyfill or {\n");
  fprintf(fd,"                currentpoint lineto\n");
  fprintf(fd,"        } {\n");
  fprintf(fd,"                currentpoint\n");
  fprintf(fd,"                gsave\n");
  fprintf(fd,"                1 setlinecap\n");
  fprintf(fd,"                newpath\n");
  fprintf(fd,"                moveto\n");
  fprintf(fd,"                currentpoint lineto\n");
  fprintf(fd,"                stroke\n");
  fprintf(fd,"                grestore\n");
  fprintf(fd,"        } ifelse\n");
  fprintf(fd,"} bind def\n");
  fprintf(fd, "end\n");        /* end of dictionary definition */
  fprintf(fd, "%%%%EndProlog\n\n");

  if (!eps) {
    fprintf(fd, "%%%%BeginSetup\n");
    fprintf(fd, "/#copies 1 def\n");
    fprintf(fd, "PSDict begin\n");

    /* some PS printers (HP) cannot select sane medium without this */
    if (po->ps_defer) {
      fprintf(fd,"mark {\n");
      fprintf(fd,"%%%%BeginFeature: *DeferredMediaSelection true\n");
      fprintf(fd," 1 dict \n"); 
      fprintf(fd," dup /DeferredMediaSelection true put\n");
      fprintf(fd," setpagedevice\n");
      fprintf(fd,"%%%%EndFeature\n");
      fprintf(fd,"} stopped cleartomark\n"); 
    }
    /* Media selection */
    if (po->ps_media) {
      fprintf(fd,"mark {\n");
      fprintf(fd,"%%%%BeginFeature: *PageSize %ld*%ld\n",right-left,high-low);
      fprintf(fd," 3 dict \n"); 
      fprintf(fd," dup /PageSize  [ plotwidth plotheight ] put\n");
      fprintf(fd," dup /ImagingBBox null put\n");
      /* 6 means select the next larger available medium but do not adjust the page */
      fprintf(fd," dup /Policies 1 dict dup /PageSize 6 put put\n");
      fprintf(fd," setpagedevice\n");
      fprintf(fd,"%%%%EndFeature\n");
      fprintf(fd,"} stopped cleartomark\n"); 
    }
    /* include user's postcript code */
    if (po->ps_incres) {
      incfd=fopen(po->ps_incres,"r");       
      if (NULL==incfd) {
	Eprintf("Warning: cannot open ps resource file %s: %s\n",po->ps_incres,strerror(errno));
      } else {
	fprintf(fd,"%%%%BeginResource: %s\n",po->ps_incres);
	while(EOF!=(c=fgetc(incfd))) {
	  fputc(c,fd);
	}
	fclose(incfd);
	fprintf(fd,"\n%%%%EndResource\n");
      }

    }
    fprintf(fd, "%%%%EndSetup\n");
    fprintf(fd, "%%%%Page: 1 1\n");
    fprintf(fd, "%%%%BeginPageSetup\n");
    fprintf(fd, "@SetPlot\n\n");
    fprintf(fd, "%%%%EndPageSetup\n");
  } else {
    fprintf(fd, "%%%%BeginSetup\n");
    fprintf(fd, "/hp2xxpagesave save def\n");
    fprintf(fd, "PSDict begin\n");
    fprintf(fd, "@SetPlot\n\n");
    fprintf(fd, "%%%%EndSetup\n");
  }
}



/**
 ** Higher-level interface: Output Encapsulated PostScript format
 **/

int to_ps(const GEN_PAR * pg, const OUT_PAR * po)
{
  PlotCmd cmd;
  HPGL_Pt pt1 = { 0, 0 },C1;
  FILE *md;
  PEN_N pen_no = 0;
  int err,itmp;
  PEN_W pensize;
  float hatch_space=0,hatch_angle=0; /* defaults are set in hpgl.c*/
  int fill_type=1;
  int polygon_mode=0;
  float f_tmp=0;

  err = 0;
  if (po->ps_eps) {
    eps=TRUE;
  }
  if (!pg->quiet)
    Eprintf("\n\n- Writing %spostscript to \"%s\"\n",
	    eps?"encapsulated ":"",
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

  pensize = pt.width[DEFAULT_PEN_NO];  /* Default pen    */
  ps_init(pg, po, md, pensize);
  fprintf(md, " %6.3f W\n", pensize);
  fprintf(md," 0 0 moveto\n");      

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
      if ((itmp = fgetc(pg->td)) == EOF) {
	PError("Unexpected end of temp. file: ");
	err = ERROR;
	goto EPS_exit;
      }
      if (!polygon_mode) {
	pen_no=itmp;
	pensize = pt.width[pen_no];
	ps_set_color(pt.color[pen_no], &pt1, md);
      } else {
	fprintf(stderr,"ignored SET_PEN in polygon mode!\n");		   
      }
      break;
    case DEF_PW:
      if (!load_pen_width_table(pg->td,polygon_mode)) {
	PError("Unexpected end of temp. file");
	err = ERROR;
	goto EPS_exit;
      }
      pensize = pt.width[pen_no];
      break;
    case DEF_PC:
      err = load_pen_color_table(pg->td,polygon_mode);
      if (err < 0) {
	PError("Unexpected end of temp. file");
	err = ERROR;
	goto EPS_exit;
      }
      break;
    case DEF_LA:
      if (load_line_attr(pg->td,polygon_mode) < 0) {
	PError("Unexpected end of temp. file");
	err = ERROR;
	goto EPS_exit;
      }
      break;
    case MOVE_TO:
    case DRAW_TO:
      if (!polygon_mode) {
	ps_set_linewidth(pensize, &pt1, md);
	ps_set_linecap(CurrentLineAttr.End, pensize, &pt1, md);
	ps_set_linejoin(CurrentLineAttr.Join, CurrentLineAttr.Limit, pensize,
			&pt1, md);
	ps_set_color(pt.color[pen_no], &pt1, md);
      }
      HPGL_Pt_from_tmpfile(&pt1);
      ps_line_to(&pt1,(cmd==DRAW_TO)?'D':'M',md);
      break;
    case PLOT_AT:
      if (!polygon_mode) {
	ps_set_linewidth(pensize, &pt1, md);
	ps_set_linecap(CurrentLineAttr.End, pensize, &pt1, md);
	ps_set_linejoin(CurrentLineAttr.Join, CurrentLineAttr.Limit, pensize,
			&pt1, md);
	ps_set_color(pt.color[pen_no], &pt1, md);
      }
      HPGL_Pt_from_tmpfile(&pt1);
      ps_line_to(&pt1, 'M', md);
      fprintf(md," dot ");
		   
      break;
    case OP_PBUF:
      if (polygon_mode) {
	fprintf(md,"\n } pop pop \n");
      }
      polygon_mode=TRUE;
      fprintf(md,"\nssp /pbuf { newpath rsp\n");
      break;  
    case CL_PBUF:
      if (!polygon_mode) {
	fprintf(stderr,"ignored CL_PBUF not in polygon mode!\n");
      } else {
	fprintf(md," clpbuf\n} def \n");
	polygon_mode=0;
      }
      break;
    case SUBPOLY:
      if (!polygon_mode) {
	fprintf(stderr,"ignored SUBPOLY not in polygon mode!\n");
      } else {
	fprintf(md,"\n subpoly\n");
      }
      break;

    case EDGE_POLY:
      ps_set_linewidth(pensize, &pt1, md);
      ps_set_linecap(CurrentLineAttr.End, pensize, &pt1, md);
      ps_set_linejoin(CurrentLineAttr.Join, CurrentLineAttr.Limit, pensize,
		      &pt1, md);
      ps_set_color(pt.color[pen_no], &pt1, md);
      fprintf(md,"\ngsave pbuf stroke grestore\n");
      break;
    case NZFILL_POLY:
    case EOFILL_POLY:
      ps_set_linewidth(pensize, &pt1, md);
      ps_set_linecap(CurrentLineAttr.End, pensize, &pt1, md);
      ps_set_linejoin(CurrentLineAttr.Join, CurrentLineAttr.Limit, pensize,
		      &pt1, md);
      ps_set_color(pt.color[pen_no], &pt1, md);

      fprintf(md,"\ngsave sfill pbuf\n  %s ",(cmd==EOFILL_POLY)?"/eo":"/nz");
      if ((fill_type = fgetc(pg->td)) == EOF) {
	PError("Unexpected end of temp. file: ");
	err = ERROR;
	goto EPS_exit;
      }
      if (3==fill_type || 4==fill_type) {
	if (fread(&hatch_space,sizeof(hatch_space),1,pg->td) != 1)  {
	  PError("Unexpected end of temp. file: ");
	  err = ERROR;
	  goto EPS_exit;
	}
	if (fread(&hatch_angle,sizeof(hatch_angle),1,pg->td) != 1)  {
	  PError("Unexpected end of temp. file: ");
	  err = ERROR;
	  goto EPS_exit;
	}
	fprintf(md,"%6.3f ",xcoord2mm * hatch_space);
	f_tmp=hatch_angle-floor(hatch_angle/180)*180;
	if (0==f_tmp || 90==f_tmp) {
	  fprintf(md,"%6.3f %6.3f %6.3f ",
		  fabs(ycoord2mm*hatch_space),
		  fabs(xcoord2mm*hatch_space),f_tmp); 
	} else {
	  fprintf(md,"%6.3f %6.3f %6.3f ",
		  fabs(ycoord2mm*hatch_space/cos(f_tmp*M_PI/180)),
		  fabs(xcoord2mm*hatch_space/sin(f_tmp*M_PI/180)),f_tmp); 
	}
      }
      fprintf(md,"%d hfill",((fill_type==3)&&(f_tmp>90))?-fill_type:fill_type);
      fprintf(md," grestore\n");
      break;
    case CLIP: 
      if (!polygon_mode) {
	HPGL_Pt_from_tmpfile(&C1);
	fprintf(md,"\n %6.3f %6.3f",xcoord2mm*(C1.x-xmin),ycoord2mm*(C1.y-ymin));
	HPGL_Pt_from_tmpfile(&C1);
	fprintf(md," %6.3f %6.3f",xcoord2mm*(C1.x-xmin),ycoord2mm*(C1.y-ymin));
	HPGL_Pt_from_tmpfile(&C1);
	fprintf(md," %6.3f %6.3f",xcoord2mm*(C1.x-xmin),ycoord2mm*(C1.y-ymin));
	HPGL_Pt_from_tmpfile(&C1);
	fprintf(md," %6.3f %6.3f setclip\n",xcoord2mm*(C1.x-xmin),ycoord2mm*(C1.y-ymin));
      } else {
	fprintf(stderr,"ignored CLIP in polygon mode!\n");
      }
      break;
    case NOCLIP:
      if (!polygon_mode) {
	fprintf(md," noclip \n");
      } else {
	fprintf(stderr,"ignored NOCLIP in polygon mode!\n");
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


