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

/** HPGL.c: HPGL parser & i/o part of HP2xx (based on D. Donath's "HPtoGF.c")
 **
 ** 91/01/13  V 1.00  HWW  Originating
 ** 91/01/19  V 1.01  HWW  reorganized
 ** 91/01/24  V 1.02  HWW  ESC.-Sequences acknowledged (preliminary!!)
 ** 91/01/29  V 1.03  HWW  Incl. SUN portation
 ** 91/01/31  V 1.04  HWW  Parser: ESC sequences should be skipped now
 ** 91/02/10  V 1.05  HWW  Parser renewed
 ** 91/02/15  V 1.06  HWW  stdlib.h supported
 ** 91/02/19  V 1.07a HWW  parser refined, bugs fixed
 ** 91/06/09  V 1.08  HWW  New options added; some restructuring
 ** 91/06/16  V 1.09  HWW  VGA mode added; some renaming; silent_mode!
 ** 91/06/20  V 1.10  HWW  Rotation added
 ** 91/10/15  V 1.11  HWW  ANSI_C; header files reorganized
 ** 91/10/20  V 1.11a HWW  VAX_C support
 ** 91/10/25  V 1.11b HWW  Support of LT; and LT0; (line type, partial)
 ** 91/11/20  V 1.12  HWW  SPn; support: many changes!
 ** 91/11/21  V 1.12b HWW  First comma in "PA,xxxx,yyyy..." accepted
 ** 91/12/22  V 1.13  HWW  Multiple MOVE compression; "plot_rel", "old_pen"
 ** 92/01/13  V 1.13c HWW  VAX problem with ungetc()/fscanf() fixed; bug fixed
 ** 92/01/15  V 1.13d HWW  "vga" --> "pre"
 ** 92/01/30  V 1.14c HWW  Parser: no need of ';', better portable
 ** 92/02/06  V 1.15a HWW  Parser: AR, AA, CI, read_float() added;
 **			   toupper() removed (MACH problems)
 ** 92/02/19  V 1.16c HWW  LB etc. supported
 ** 92/02/23  V 1.17b HWW  LB etc. improved, PG supported
 ** 92/02/25  V 1.17c HWW  Parser improved: SP, LT, multi-mv suppression
 ** 92/03/01  V 1.17d HWW  Char sizes: debugged
 ** 92/03/03  V 1.17e HWW  LB_Mode introduced
 ** 92/04/15  V 1.17f HWW  Width x Height limit assumed
 ** 92/05/21  V 1.18a HWW  Multiple-file usage
 ** 92/05/28  V 1.19a HWW  XT, YT, TL, SM added
 ** 92/10/20  V 1.20c HWW  More line types added (debugged)
 ** 92/11/08  V 1.20d HWW  Interval of active pages
 ** 92/12/13  V 1.20e HWW  truesize option added
 ** 93/02/10  V 1.21a HWW  Arcs & circles now properly closed;
 **			   Bug fixed: SC does not interfere with last move
 ** 93/03/10  V 1.21b HWW  Bug fixed in LT scanner part
 ** 93/03/22, V 1.21c HWW  HYPOT() workaround for a weird BCC behavior;
 ** 93/04/02		   Line_Generator(): Case *pb==*pa caught
 ** 93/04/13  V 1.22a HWW  UC supported (code by Alois Treindl)
 ** 93/04/25  V 1.22b HWW  LB/PR bug fix by E. Norum included
 ** 93/05/20  V 1.22c HWW  LT1 pattern adjustment (report by E. Frambach)
 ** 93/09/02  V 1.22d HWW  EA (rect) added (by Th. Hiller)
 ** 94/01/01  V 1.22e HWW  Misc. additions suggested by L. Lowe:
 **			    1) PlotCmd_from_tmpfile(): int --> PlotCmd
 **                         2) ES: 2nd parameter now optional
 **                         3) evaluate_HPGL(): center_mode introduced
 ** 94/02/14  V 1.30a HWW  Re-organized; many changes; min/max bug fixed
 ** 99/02/28          MK   IW,CA,CS,SA,SS commands added
 ** 99/04/24          MK   PC,PW commands added
 ** 99/05/10          MK   RO command added (code by rs@daveg.com) 
 ** 99/05/18          MK   partial PE support (by Eugene Doudine)
 ** 99/06/05          MK   PC improvements and fixes
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "bresnham.h"
#include "hp2xx.h"
#include "chardraw.h"
#include "clip.h"

#define	ETX		'\003'

#define P1X_default	603.0	/* DIN A4 defaults	*/
#define P1Y_default	521.0
#define P2X_default	10603.0
#define P2Y_default	7721.0


#ifdef __TURBOC__
#define	HYPOT(x,y)	sqrt((x)*(x)+(y)*(y))
#else
#define	HYPOT(x,y)	hypot(x,y)
#endif


/**
 ** Globals needed in chardraw.c:
 **/
				/* Line type selected by HP-GL code	*/
LineType GlobalLineType = LT_default;
				/* Currently effective line type	*/
LineType CurrentLineType= LT_default;
HPGL_Pt		HP_pos	= {0};		/* Actual plotter pen position 	*/
HPGL_Pt		P1	= {P1X_default, P1Y_default}; /* Scaling points	*/
HPGL_Pt		P2	= {P2X_default, P2Y_default};
int iwflag=0; /*MK*/
HPGL_Pt		C1	= {P1X_default, P1Y_default}; /* Clipping points	*/
HPGL_Pt		C2	= {P2X_default, P2Y_default};
HPGL_Pt	S1	= {P1X_default, P1Y_default};	/* Scaled 	*/
HPGL_Pt	S2	= {P2X_default, P2Y_default};	/* points	*/
HPGL_Pt	Q;	/* Delta-P/Delta-S: Initialized with first SC 	*/
HPGL_Pt M; /* maximum coordinates set by PS instruction */
/**
 ** Global from chardraw.c:
 **/
extern	TextPar	tp;

/**
 ** "Local" globals (I know this is messy...) :
 **/
static	float	xmin, xmax, ymin, ymax, neg_ticklen, pos_ticklen;
static	double	Diag_P1_P2, glb_pat_len, cur_pat_len, pat_pos;
static	HPGL_Pt	p_last	= {M_PI, M_PI};	/* Init. to "impossible" values */

static	float	rot_cos, rot_sin;

static	short rotate_flag	= FALSE;	/* Flags tec external to HP-GL	*/
static  short ps_flag		= FALSE;
static  double  rot_ang = 0.;
static	short scale_flag	= FALSE;
static	short mv_flag 	= FALSE;
#ifdef	ATARI
extern	short silent_mode	= FALSE;	/* Don't clobber ATARI preview!	*/
#else
static	short silent_mode	= FALSE;
#endif
static	short record_off	= FALSE;
static	short first_page	= 0;
static	int last_page	= 0;
static	int n_unexpected	= 0;
static	int n_unknown	= 0;
static	int page_number	= 1;
static	long vec_cntr_r	= 0L;
static	long vec_cntr_w	= 0L;
static	short	pen	= -1;
static	short pens_in_use[NUMPENS];
static	short pen_down	= FALSE;	/* Internal HP-GL book-keeping:	*/
static	short plot_rel	= FALSE;
static	char	StrTerm	= ETX;		/* String terminator char	*/
static	char	*strbuf = NULL;
static	int	strbufsize = MAX_LB_LEN+1;
static	char	symbol_char = '\0';	/* Char	in Symbol Mode (0=off)	*/

static	FILE	*td;


/* Known HPGL commands, ASCII-coded as High-byte/low-byte int's	*/

#define AA	0x4141
#define AF	0x4146
#define AH	0x4148
#define AR	0x4152
#define BL	0x424C
#define BP      0x4250
#define CA      0x4341 /*MK*/
#define CI	0x4349
#define CP	0x4350
#define CS      0x4353 /*MK*/
#define DF	0x4446
#define DI	0x4449
#define DR	0x4452
#define DT	0x4454
#define EA	0x4541
#define ES	0x4553
#define EW      0x4557 /*MK*/
#define IN	0x494E
#define IP	0x4950
#define IW      0x4957 /*MK*/
#define LB	0x4C42
#define LO	0x4C4F
#define LT	0x4C54
#define MG      0x4D47
#define NP      0x4E50
#define OP	0x4F50
#define PA	0x5041
#define PB	0x5042
#define PC      0x5043 /*MK*/
#define PD	0x5044
#define PE      0x5045
#define PG	0x5047
#define PR	0x5052
#define PS      0x5053
#define PU	0x5055
#define PW	0x5057 /*MK*/
#define RO	0x524F /*RS*/
#define SA      0x5341 /*MK*/
#define SC	0x5343
#define SI	0x5349
#define SL	0x534C
#define SM	0x534D
#define SP	0x5350
#define SR	0x5352
#define SS      0x5353 /*MK*/
#define TL	0x544C
#define UC	0x5543
#define WD	0x5744
#define XT	0x5854
#define YT	0x5954




static void
par_err_exit (short code, short cmd)
{
char	*msg;

  switch(code)
  {
    case 0:	msg = "Illegal parameters";		break;
    case 1:	msg = "Error in first parameter";	break;
    case 2:	msg = "No second parameter";		break;
    case 3:	msg = "No third parameter";		break;
    case 4:	msg = "No fourth parameter";		break;
    case 98:	msg = "sscanf error: corrupted file?";	break;
    case 99:
    default:	msg = "Internal error";			break;
  }
  Eprintf ("\nError in command %c%c: %s\n", cmd>>8, cmd&0xFF, msg);
  Eprintf (" @ Cmd %ld\n", vec_cntr_w);
  exit(ERROR);
}




static void
reset_HPGL (void)
{
  p_last.x	= p_last.y = M_PI;
  pen_down 	= FALSE;
  plot_rel	= FALSE;
  pen		= -1;
  n_unexpected	= 0;
  n_unknown	= 0;
  mv_flag 	= FALSE;

  GlobalLineType= CurrentLineType = LT_default;

  StrTerm	= ETX;
  if (strbuf == NULL)
  {
    strbuf = malloc (strbufsize);
    if (strbuf == NULL)
    {
      fprintf (stderr, "\nNo memory !\n");
      exit (ERROR);
    }
  }
  strbuf[0]	= '\0';

  P1.x		= P1X_default;
  P1.y		= P1Y_default;
  P2.x		= P2X_default;
  P2.y		= P2Y_default;
  Diag_P1_P2	= HYPOT (P2.x - P1.x, P2.y - P1.y);
  glb_pat_len	= cur_pat_len = 0.04 * Diag_P1_P2;
  pat_pos	= 0.0;
  scale_flag	= FALSE;
  S1		= P1;
  S2		= P2;
  Q.x		= Q.y = 1.0;
  HP_pos.x	= HP_pos.y = 0.0;
  neg_ticklen	= 0.005;	/* 0.5 %	*/
  pos_ticklen	= 0.005;
  symbol_char	= '\0';

  init_text_par();
}



static void
init_HPGL (const GEN_PAR *pg, const IN_PAR* pi)
{
/**
 ** Re-init. global var's for multiple-file applications
 **/

  td		= pg->td;
  silent_mode	= pg->quiet;
  xmin		= pi->x0;
  ymin		= pi->y0;
  xmax		= pi->x1;
  ymax		= pi->y1;

/*  pens_in_use	= 0;*/

  /**
   ** Record ON if no page selected (pg->page == 0)!
   **/
  first_page	= pi->first_page;	/* May be 0 	*/
  last_page	= pi->last_page;	/* May be 0 	*/
  page_number	= 1;
  record_off	=     (first_page > page_number)
		  || ((last_page  < page_number) && (last_page > 0));

  rot_ang = pi->rotation;
  rotate_flag	= (rot_ang != 0.0) ? TRUE : FALSE;
  if (rotate_flag)
  {
	rot_cos = cos (M_PI * rot_ang / 180.0);
	rot_sin = sin (M_PI * rot_ang / 180.0);
  }

  vec_cntr_r	= 0L;
  vec_cntr_w	= 0L;

  reset_HPGL();
}



static void
User_to_Plotter_coord (const HPGL_Pt *p_usr, HPGL_Pt *p_plot)
/**
 ** 	Utility: Transformation from (scaled) user coordinates
 **	to plotter coordinates
 **/
{
  p_plot->x = P1.x + (p_usr->x - S1.x) * Q.x;
  p_plot->y = P1.y + (p_usr->y - S1.y) * Q.y;
}



static void
Plotter_to_User_coord (const HPGL_Pt *p_plot, HPGL_Pt *p_usr)
/**
 ** 	Utility: Transformation from plotter coordinates
 **	to (scaled) user coordinates
 **/
{
  p_usr->x = S1.x + (p_plot->x - P1.x) / Q.x;
  p_usr->y = S1.y + (p_plot->y - P1.y) / Q.y;
}



static void
PlotCmd_to_tmpfile (PlotCmd cmd)
{
  if (record_off)	/* Wrong page!	*/
	return;

  if (!silent_mode) switch (vec_cntr_w++)
  {
    case 0:	Eprintf ("Writing Cmd: ");break;
    case 1:	Eprintf ("1 ");		break;
    case 2:	Eprintf ("2 ");		break;
    case 5:	Eprintf ("5 ");		break;
    case 10:	Eprintf ("10 ");	break;
    case 20:	Eprintf ("20 ");	break;
    case 50:	Eprintf ("50 ");	break;
    case 100:	Eprintf ("100 ");	break;
    case 200:	Eprintf ("200 ");	break;
    case 500:	Eprintf ("500 ");	break;
    case 1000:	Eprintf ("1k ");	break;
    case 2000:	Eprintf ("2k ");	break;
    case 5000:	Eprintf ("5k ");	break;
    case 10000:	Eprintf ("10k ");	break;
    case 20000:	Eprintf ("20k ");	break;
    case 50000L:Eprintf ("50k ");	break;
    case 100000L:Eprintf ("100k ");	break;
    case 200000L:Eprintf ("200k ");	break;
    case 500000L:Eprintf ("500k ");	break;
  }

  if (fputc ((int) cmd, td) == EOF)
  {
	PError("PlotCmd_to_tmpfile");
	Eprintf ("Error @ Cmd %ld\n", vec_cntr_w);
	exit (ERROR);
  }
}




static void
HPGL_Pt_to_tmpfile (const HPGL_Pt *pf)
{
  if (record_off)	/* Wrong page!	*/
	return;

  if (fwrite ((VOID *) pf, sizeof(*pf), 1, td) != 1)
  {
	PError("HPGL_Pt_to_tmpfile");
	Eprintf ("Error @ Cmd %ld\n", vec_cntr_w);
	exit (ERROR);
  }
  xmin = MIN (pf->x, xmin);
  ymin = MIN (pf->y, ymin);
  xmax = MAX (pf->x, xmax);
  ymax = MAX (pf->y, ymax);
}




/**
 **	Low-level vector generation & file I/O
 **/

static void
LPattern_Generator (HPGL_Pt	*pa,
		    double	dx, 		double	dy,
		    double	start_of_pat,	double	end_of_pat)
/**
 **	Generator of Line type patterns:
 **
 **	pa:		Start point (ptr) of current line segment
 **	dx, dy:		Components of c * (*pb - *pa), c holding
 **				dx^2 + dy^2 = pattern_length^2
 **	start_of_pat:	Fraction of start point within pattern
 **	end_of_pat:	Fraction of end   point within pattern
 **			Valid: 0 <= start_of_pat <= end_of_pat <= 1
 **
 **	A pattern consists of alternating "line"/"point" and "gap" elements,
 **	always starting with a line/point. A point is a line of zero length.
 **	The table below contains the relative lengths of the elements
 **	of all line types except LT0; and LT; (7), which are treated separately.
 **	These lengths always add up to 1. A negative value terminates a pattern.
 **/
{
double		length_of_ele, start_of_action, end_of_action;
static	double	*p_cur_pat,
		pattern[14][8] = {
	/* 	 Line  gap   Line  gap   Line   gap   Line  TERM	*/
	/*-6 */	{0.25, 0.10, 0.10, 0.10, 0.10,  0.10, 0.25, -1},
	/*-5 */	{0.35, 0.10, 0.10, 0.10, 0.35, -1,   -1     -1},
	/*-4 */	{0.40, 0.10, 0.00, 0.10, 0.40, -1,   -1,    -1},
	/*-3 */	{0.35, 0.30, 0.35,-1,   -1,    -1,   -1,    -1},
	/*-2 */	{0.25, 0.75, 1.00,-1,   -1,    -1,   -1,    -1},
	/*-1 */	{0.00, 1.00,-1,   -1,   -1,    -1,   -1,    -1},
	/* 0 */	{0.00, 1.00,-1,   -1,   -1,    -1,   -1,    -1},
		/* Dummy; should NOT be used	*/
	/* 1 */	{0.01, 0.99,-1,   -1,   -1,    -1,   -1,    -1},
	/* 2 */	{0.50, 0.50,-1,   -1,   -1,    -1,   -1,    -1},
	/* 3 */	{0.70, 0.30,-1,   -1,   -1,    -1,   -1,    -1},
	/* 4 */	{0.80, 0.10, 0.00, 0.10,-1,    -1,   -1,    -1},
	/* 5 */	{0.70, 0.10, 0.10, 0.10,-1,    -1,   -1,    -1},
	/* 6 */	{0.50, 0.10, 0.10, 0.10, 0.10,  0.10,-1,    -1},
	/* 7 */	{1.00,-1,   -1,   -1,   -1,    -1,   -1,    -1}
		/* Dummy; should NOT be used	*/
};

  p_cur_pat = pattern[(int)CurrentLineType + 6];

  if ((int) CurrentLineType < 0)
	for (;;)
	{
		length_of_ele = *p_cur_pat++;	/* Line or point	*/
		if (length_of_ele < 0)
			return;
		if (length_of_ele == 0.0)
			PlotCmd_to_tmpfile (PLOT_AT);
		else
			PlotCmd_to_tmpfile (DRAW_TO);

		pa->x += dx * length_of_ele;
		pa->y += dy * length_of_ele;
		HPGL_Pt_to_tmpfile (pa);

		length_of_ele = *p_cur_pat++;	/* Gap			*/
		if (length_of_ele < 0)
			return;
		pa->x += dx * length_of_ele;
		pa->y += dy * length_of_ele;
		PlotCmd_to_tmpfile (MOVE_TO);
		HPGL_Pt_to_tmpfile (pa);
	}
  else
	for (end_of_action=0.0; ; )
	{
	    /**
	     ** Line or point:
	     **/
	    start_of_action	= end_of_action;
	    length_of_ele	= *p_cur_pat++;
	    if (length_of_ele < 0)
		return;
	    end_of_action += length_of_ele;
	    if (end_of_action > start_of_pat)	/* If anything to do:	*/
	    {
		if (start_of_pat <= start_of_action)
		{				/* If start is valid	*/
		    if (end_of_action <= end_of_pat)
		    {				/* Draw full element	*/
			pa->x += dx * length_of_ele;
			pa->y += dy * length_of_ele;
			if (length_of_ele == 0.0)
				PlotCmd_to_tmpfile (PLOT_AT);
			else
				PlotCmd_to_tmpfile (DRAW_TO);
			HPGL_Pt_to_tmpfile (pa);
		    }
		    else	/* End_of_action beyond End_of_pattern:	*/
		    {		/* --> Draw only first part of element:	*/
			pa->x += dx * (end_of_pat - start_of_action);
			pa->y += dy * (end_of_pat - start_of_action);
			if (length_of_ele == 0.0)
				PlotCmd_to_tmpfile (MOVE_TO);
			else
				PlotCmd_to_tmpfile (DRAW_TO);
			HPGL_Pt_to_tmpfile (pa);
			return;
		    }
		}
		else	/* Start_of_action before Start_of_pattern: 	*/
		{
		    if (end_of_action <= end_of_pat)
		    {		/* Draw remainder of element		*/
			if (length_of_ele == 0.0)
				PlotCmd_to_tmpfile (PLOT_AT);
			else
				PlotCmd_to_tmpfile (DRAW_TO);
			pa->x += dx * (end_of_action - start_of_pat);
			pa->y += dy * (end_of_action - start_of_pat);
			HPGL_Pt_to_tmpfile (pa);
		    }
		    else	/* End_of_action beyond End_of_pattern:	*/
		    		/* Draw central part of element & leave	*/
		    {
			if (end_of_pat == start_of_pat)
				PlotCmd_to_tmpfile (PLOT_AT);
			else
				PlotCmd_to_tmpfile (DRAW_TO);
			pa->x += dx * (end_of_pat - start_of_pat);
			pa->y += dy * (end_of_pat - start_of_pat);
			HPGL_Pt_to_tmpfile (pa);
			return;
		    }
		}
	    }

	    /**
	     ** Gap (analogous to line/point):
	     **/
	    start_of_action =  end_of_action;
	    length_of_ele  =  *p_cur_pat++;
	    if (length_of_ele < 0)
		return;
	    end_of_action += length_of_ele;
	    if (end_of_action > start_of_pat)	/* If anything to do:	*/
	    {
		if (start_of_pat <= start_of_action)
		{				/* If start is valid	*/
		    if (end_of_action <= end_of_pat)
		    {				/* Full gap		*/
			pa->x += dx * length_of_ele;
			pa->y += dy * length_of_ele;
			PlotCmd_to_tmpfile (MOVE_TO);
			HPGL_Pt_to_tmpfile (pa);
		    }
		    else	/* End_of_action beyond End_of_pattern:	*/
		    {		/* --> Apply only first part of gap:	*/
			pa->x += dx * (end_of_pat - start_of_action);
			pa->y += dy * (end_of_pat - start_of_action);
			PlotCmd_to_tmpfile (MOVE_TO);
			HPGL_Pt_to_tmpfile (pa);
			return;
		    }
		}
		else	/* Start_of_action before Start_of_pattern: 	*/
		{
		    if (end_of_action <= end_of_pat)
		    {		/* Apply remainder of gap		*/
			pa->x += dx * (end_of_action - start_of_pat);
			pa->y += dy * (end_of_action - start_of_pat);
			PlotCmd_to_tmpfile (MOVE_TO);
			HPGL_Pt_to_tmpfile (pa);
		    }
		    else	/* End_of_action beyond End_of_pattern:	*/
		    		/* Apply central part of gap & leave	*/
		    {
			if (end_of_pat == start_of_pat)
				return;			/* A null move	*/
			pa->x += dx * (end_of_pat - start_of_pat);
			pa->y += dy * (end_of_pat - start_of_pat);
			PlotCmd_to_tmpfile (MOVE_TO);
			HPGL_Pt_to_tmpfile (pa);
			return;
		    }
		}
	    }
	}
}
/*
struct PE_flags{
    int abs;
    int up;
    int sbmode;
    int fract;
    int pen;
} ;
*/
int read_PE_flags(const GEN_PAR *pg, int c,FILE *hd,PE_flags *fl) {
	short old_pen;
        float ftmp;
	int ctmp;
    switch(c) {
    case 183:
    case '7':
	/* seven bit mode */
	fl->sbmode=1;
	break;
	
    case 186:
    case  ':':
	/* select pen */
	if (EOF==(fl->pen=getc(hd))) {
	   par_err_exit(98,PE); 
	}
          old_pen=pen;
    read_PE_coord(c,hd,fl,&ftmp);
            pen=ftmp;
	if (pen < 0 || pen > pg->maxpens)
	{
		Eprintf (
		    "\nIllegal pen number %d: replaced by %d\n", pen, pen % pg->maxpens);
		n_unexpected++;
		pen = pen % pg->maxpens;
	}
	if (old_pen != pen)
	{
		if ((fputc (SET_PEN, td) ==  EOF) ||
		    (fputc (    pen, td) ==  EOF))
		{
			PError("Writing to temporary file:");
			Eprintf ("Error @ Cmd %ld\n", vec_cntr_w);
			exit (ERROR);
		}
	}
	if (pen)    pens_in_use[pen]=1;
             /*		pens_in_use |= (1 << (pen-1)); */
/*MK*/
	break;
	
    case 190:
    case '>':
	/* fractional data */

	if (EOF==(ctmp=getc(hd))){ 
	    par_err_exit(98,PE); 
	}	
	fl->fract=decode_PE_char(ctmp,fl);
        fl->fract=((fl->fract>>1)*((fl->fract&0x01)?-1:1));
/*    	fprintf(stderr,"PE > flag, fract =%d (%d decimals) ",fl->fract, fl->fract/3);*/
	break;

    case 188:
    case '<':
	/* pen up*/
	fl->up=1;
	break;
	
    case 189:
    case '=':
	/* abs plotting*/

	fl->abs=1;
	break;

    default:
	return(0);
    }
    return(1);
}



void	PE_line (int abs,HPGL_Pt p)
{

  if (!abs)			/* Process coordinates	*/
  {
    p.x += p_last.x;
    p.y += p_last.y;
  }
  
  if (pen_down)
    Pen_action_to_tmpfile	(DRAW_TO, &p, scale_flag);
  else
    Pen_action_to_tmpfile	(MOVE_TO, &p, scale_flag);
  
  if (symbol_char)
    {
      plot_symbol_char	(symbol_char);
      Pen_action_to_tmpfile	(MOVE_TO, &p, scale_flag);
    }
  
  p_last = p;
}



int isPEterm(int c, PE_flags *fl) {
    if ((fl->sbmode) && ((c>94)||(c<63))) return 1;
    if ((! fl->sbmode) && ((c>190)||(c<63))) return 1;
    return (0);
}


int decode_PE_char(int c, PE_flags *fl) 
{
  if (fl->sbmode) {
    c&=0x7f;
    return((c>94)?(c-95):(c-63));
  } else {
    return((c>190)?(c-191):(c-63));
  }
}

int read_PE_coord(int c,FILE* hd, PE_flags *fl,float *fv) {
  long lv=0;
  int i=0;
  int shft=(fl->sbmode)?5:6;

  for (;;) {
    if (c<63) {
      ungetc(c,hd);
      break;
    }
    lv|=((long)decode_PE_char(c,fl))<<(i*shft);
    i++;
    if (isPEterm(c,fl)) {
       break;
    }
    if (EOF==(c=getc(hd))) {
	par_err_exit(98,PE);
    }
  } 
  *fv=(float)(((lv>>1)*((lv&0x01)?-1:1))<<fl->fract);
  return(1);
}


int read_PE_pair(int c,FILE *hd, PE_flags *fl,HPGL_Pt *p) 
{
    read_PE_coord(c,hd,fl,&(p->x));
    if (EOF==(c=getc(hd))) {
	par_err_exit(98,PE);
    }
    read_PE_coord(c,hd,fl,&(p->y));
    return(1);
}




void read_PE(const GEN_PAR *pg,FILE *hd) 
{
    int	c;

    HPGL_Pt p;
    PE_flags fl;
    
    fl.fract=0;
    fl.sbmode=0;
    fl.abs=0;
    fl.up=0;
    fl.pen=0;

    for (c = getc(hd); (c!=EOF) && (c!=';'); c = getc(hd)) {
	if (!read_PE_flags(pg,c,hd,&fl)) {
	    read_PE_pair(c,hd,&fl,&p);
	    pen_down=(fl.up)?FALSE:TRUE;
	    PE_line(fl.abs,p);
	    tp->CR_point = HP_pos;
	    fl.abs=0;
	    fl.up=0;
	}
    } 
}


static void
Line_Generator (HPGL_Pt *pa, const HPGL_Pt *pb, int mv_flag)
{
double	seg_len, dx, dy, quot;
int	n_pat, i;

  switch (CurrentLineType)
  {
    case LT_solid:
	PlotCmd_to_tmpfile (DRAW_TO);
	HPGL_Pt_to_tmpfile (pb);
	return;
    case LT_plot_at:
	PlotCmd_to_tmpfile (PLOT_AT);
	HPGL_Pt_to_tmpfile (pb);
	return;
    default:
	break;
  }

  dx = pb->x - pa->x;
  dy = pb->y - pa->y;
  seg_len = HYPOT (dx, dy);
  if (seg_len == 0.0)
  {
	if (!silent_mode)
		Eprintf ("Warning: Zero line segment length -- skipped\n");
	return;		/* No line to draw ??		*/
  }

  if ((int) CurrentLineType < 0)	/* Adaptive	*/
  {
	pat_pos = 0.0;	/* Reset to start-of-pattern	*/
	n_pat	= ceil  (seg_len / cur_pat_len);
	dx	/= n_pat;
	dy	/= n_pat;
			/* Now draw n_pat complete line patterns*/
	for (i=0; i < n_pat; i++)
		LPattern_Generator (pa, dx, dy, 0.0, 1.0);
  }
  else
  {
	if (mv_flag)	/* Last move ends old line pattern	*/
		pat_pos = 0.0;
	quot	= seg_len / cur_pat_len;
	dx	/= quot;
	dy	/= quot;
	while (quot >= 1.0)
	{
		LPattern_Generator (pa, dx, dy, pat_pos, 1.0);
		quot -= (1.0 - pat_pos);
		pat_pos = 0.0;
	}
	quot += pat_pos;
	if (quot >= 1.0)
	{
		LPattern_Generator (pa, dx, dy, pat_pos, 1.0);
		quot -= 1.0;
		pat_pos = 0.0;
	}
	LPattern_Generator (pa, dx, dy, pat_pos, quot);
	pat_pos = quot;
  }
}




void
Pen_action_to_tmpfile (PlotCmd cmd, const HPGL_Pt *p, int scaled)
{
static	HPGL_Pt	P_last;
HPGL_Pt		P;
double		tmp;

  if (record_off)	/* Wrong page!	*/
	return;

  if (scaled)		/* Rescaling	*/
	User_to_Plotter_coord (p, &P);
  else
	P = *p;		/* Local copy	*/


  HP_pos = P;		/* Actual plotter pos. in plotter coord	*/

  if (rotate_flag)	/* hp2xx-specific global rotation	*/
  {
	tmp = rot_cos * P.x - rot_sin * P.y;
	P.y = rot_sin * P.x + rot_cos * P.y;
	P.x = tmp;
  }


  /* Extreme values needed for later scaling:	 */

  switch (cmd)
  {
    case MOVE_TO:
	mv_flag = TRUE;
	break;

  /**
   ** Multiple-move suppression. In addition,
   ** a move only precedes a draw -- nothing else!
   **/

    case DRAW_TO:
	if (mv_flag)
	{
		PlotCmd_to_tmpfile (MOVE_TO);
		HPGL_Pt_to_tmpfile (&P_last);
	}
	/* drop through	*/
    case PLOT_AT:
	Line_Generator (&P_last, &P, mv_flag);
	mv_flag = FALSE;
	break;

    default:
	Eprintf ("Illegal Pen Action: %d\n", cmd);
	Eprintf ("Error @ Cmd %ld\n", vec_cntr_w);
	exit (ERROR);
  }
  P_last = P;
}





int
read_float (float *pnum, FILE *hd)
/**
 ** Main work-horse for parameter input:
 **
 ** Search for next number, skipping white space but return if mnemonic met.
 ** If found, read in number
 **	returns	0 if valid number
 **		1 if command ended
 **		2 if scanf failed (possibly corrupted file)
 **	      EOF if EOF met
 **/
{
int	c;
char	*ptr, numbuf[80];

  for ( c=getc(hd); (c!='.')&&(c!='+')&&(c!='-')&&((c<'0')||(c>'9')) ; c=getc(hd) )
  {
	if (c==EOF)		/* Wait for number	*/
		return EOF;	/* Should not happen	*/
	if (c==';')
		return  1;	/* Terminator reached	*/
	if (((c >= 'A') && (c <= 'Z')) ||
	    ((c >= 'a') && (c <= 'a')) || (c==ESC))
	{
		ungetc (c, hd);
		return  1;	/* Next Mnemonic reached*/
	}
  }
				/* Number found: Get it	*/
  ptr = numbuf;
  for (*ptr++ = c, c=getc(hd); ((c>='0')&&(c<='9'))||(c=='.'); c=getc(hd) )
	*ptr++ = c;		/* Read number		*/
  *ptr='\0';
  if (c!=EOF)
	ungetc (c, hd);

  if (sscanf(numbuf,"%f", pnum) != 1)
	return 11;		/* Should never happen	*/
  return 0;
}




static void
read_string (char *buf, FILE *hd)
{
int	c, n;

  for (n=0,c = getc(hd); (c!=EOF) && (c!=StrTerm); c = getc(hd))
  {
	if (n > strbufsize/2)
	{
		strbufsize *= 2;
		strbuf = realloc (strbuf, strbufsize);
		if (strbuf == NULL)
		{
			fprintf (stderr, "\nNo memory !\n");
			exit (ERROR);
		}
		buf = strbuf+n;
	}
	if (n++ <strbufsize)
		*buf++ = c;
  }
  if (c==StrTerm && c!=ETX)
	*buf++ = c;
  *buf = '\0';
}




static void
read_symbol_char (FILE *hd)
{
int	c;

  for (c = getc(hd); /* ended by switch{} */ ; c = getc(hd))
	switch (c)
	{
	  case ' ':
	  case _HT:
	  case _LF:
		break;		/* Skip white space		*/
	  case _CR:
	  case EOF:
	  case ';':		/* CR or "term" end symbol mode	*/
		symbol_char = '\0';
		return;
	  default:
		if (c < ' ' || c > '~')
			break;	/* Ignore unprintable chars	*/
		else
		{
			symbol_char = c;
			return;
		}
	}
}




static void
read_ESC_cmd (FILE *hd)
/*
 * Read & skip device control commands (ESC.-Commands)
 * Currently known: HP 7550A command set
 */
{
int	c;

  if (getc(hd) != '.')
  {
	n_unexpected++;
	return;
  }

  switch (getc(hd))
  {
    case EOF:
	n_unexpected++;
	Eprintf ("\nUnexpected EOF!\n");
	return;
    case 'A':
    case 'B':
    case 'E':
    case 'J':
    case 'K':
    case 'L':
    case 'O':
    case 'U':
    case 'Y':
    case '(':
    case ')':
	return;	/* Commands without parameters	*/
    case '@':
    case 'H':
    case 'I':
    case 'M':
    case 'N':
    case 'P':
    case 'Q':
    case 'S':
    case 'T':
	do {	/* Search for terminator ':'	*/
		c = getc(hd);
	} while ((c!=':')&&(c!=EOF));
	if (c==EOF)
	{
		n_unexpected++;
		Eprintf ("\nUnexpected EOF!\n");
	}
	return;
    default:
	n_unknown++;
	return;
  }
}




/****************************************************************************/



/**
 **	lines:	Process PA-, PR-, PU-, and  PD- commands
 **/


static void
lines (int relative, FILE *hd)
/**
 ** Examples of anticipated commands:
 **
 **	PA PD0,0,80,50,90,20PU140,30PD150,80;
 **	PU0,0;PD20.53,40.32,30.08,60.2,40,90,;PU100,300;PD120,340...
 **/
{
HPGL_Pt	p,pl,porig;
int outside=0;
double x1,y1,x2,y2;

  for ( ; ; )
  {
	if (read_float (&p.x, hd))	/* No number found	*/
		return ;

	if (read_float (&p.y, hd))	/* x without y invalid! */
		par_err_exit (2,PA);

	if (relative)			/* Process coordinates	*/
	{
		p.x += p_last.x;
		p.y += p_last.y;
	}

                porig.x=p.x;
                porig.y=p.y;
	
if (iwflag) 
{
		x1= P1.x+(p_last.x-S1.x)*Q.x;
		y1= P1.y+(p_last.y-S1.y)*Q.y;
		x2= P1.x+(p.x-S1.x)*Q.x;
		y2= P1.y+(p.y-S1.y)*Q.y;
	outside= DtClipLine(C1.x,C1.y,C2.x,C2.y,&x1,&y1,&x2,&y2);

	if (!outside){
		p.x=(x2-P1.x)/Q.x+S1.x;
		p.y=(y2-P1.y)/Q.y+S1.y;
		pl.x=(x1-P1.x)/Q.x+S1.x;
		pl.y=(y1-P1.y)/Q.y+S1.y;
		if (pl.x != p_last.x || pl.y != p_last.y)
		Pen_action_to_tmpfile	(MOVE_TO, &pl, scale_flag);
        
        }

}        

	if (pen_down && !outside)
		Pen_action_to_tmpfile	(DRAW_TO, &p, scale_flag);
	else
		Pen_action_to_tmpfile	(MOVE_TO, &p, scale_flag);
		
if (iwflag)	Pen_action_to_tmpfile	(MOVE_TO, &porig, scale_flag);

        
	if (symbol_char)
	{
		plot_symbol_char	(symbol_char);
		Pen_action_to_tmpfile	(MOVE_TO, &p, scale_flag);
	}
outside=0;
	p_last = porig;
  }
}



/**
 **	Arcs, circles and alike
 **/


static void
arc_increment (HPGL_Pt *pcenter, double r, double phi)
{
HPGL_Pt	p;
int outside=0;

  p.x = pcenter->x + r * cos(phi);
  p.y = pcenter->y + r * sin(phi);

if (iwflag) 
{
if ( P1.x+(p.x-S1.x)*Q.x > C2.x || P1.y+(p.y-S1.y)*Q.y > C2.y){
/*fprintf(stderr,"IW set:point %f %f >P2\n",p.x,p.y);*/
outside=1;
 }
if (P1.x+(p.x-S1.x)*Q.x  < C1.x  || P1.y+(p.y-S1.y)*Q.y < C1.y) {
/*fprintf(stderr,"IW set:point  %f %f <P1\n",p.x,p.y);*/
outside=1;
 }
}

  if (pen_down && !outside)
	Pen_action_to_tmpfile (DRAW_TO, &p, scale_flag);
  else
	if ((p.x != p_last.x) || (p.y != p_last.y))
		Pen_action_to_tmpfile (MOVE_TO, &p, scale_flag);
  p_last = p;
}



static void
arcs (int relative, FILE *hd)
{
HPGL_Pt	p, d, center;
float	alpha, eps;
double	phi, phi0, r;

  if (read_float (&p.x, hd))	/* No number found	*/
	return ;

  if (read_float (&p.y, hd))	/* x without y invalid! */
	par_err_exit (2,AA);

  if (read_float (&alpha, hd))	/* Invalid without angle*/
	par_err_exit (3,AA);
  else
	alpha *= M_PI / 180.0;	/* Deg-to-Rad		*/

  switch (read_float (&eps, hd))
  {
    case 0:
	break;
    case 1:			/* No resolution option	*/
	eps = 5.0;		/*    so use default!	*/
	break;
    case 2:    			/* Illegal state	*/
	par_err_exit (98,AA);
    case EOF:
	return;
    default:			/* Illegal state	*/
	par_err_exit (99,AA);
  }
  eps *= M_PI / 180.0;		/* Deg-to-Rad		*/


  if (relative)			/* Process coordinates	*/
  {
	d = p;			/* Difference vector	*/
	center.x = d.x + p_last.x;
	center.y = d.y + p_last.y;
  }
  else
  {
	d.x = p.x - p_last.x;
	d.y = p.y - p_last.y;
	center.x = p.x;
	center.y = p.y;
  }

  if ( ((r = sqrt(d.x*d.x + d.y*d.y)) == 0.0) || (alpha == 0.0) )
	return;	/* Zero radius or zero arc angle given	*/

  phi0 = atan2(-d.y, -d.x);

  if ((int) GlobalLineType < 0)	/* Adaptive patterns:	*/
  {
	p.x = r * cos(eps);	/* A chord segment	*/
	p.y = r * sin(eps);
	if (scale_flag)
		User_to_Plotter_coord (&p, &p);

	/* 	Pattern length = chord length		*/
	cur_pat_len = HYPOT (p.x, p.y);
  }

  if (alpha > 0.0)
  {
	for (phi=phi0+MIN(eps,alpha) ; phi < phi0 + alpha; phi += eps)
		arc_increment (&center, r, phi);
	arc_increment (&center, r, phi0 + alpha);	/* to endpoint */
  }
  else
  {
	for (phi=phi0-MIN(eps,-alpha) ; phi > phi0 + alpha; phi -= eps)
		arc_increment (&center, r, phi);
	arc_increment (&center, r, phi0 + alpha);	/* to endpoint */
  }

  cur_pat_len = glb_pat_len;	/* Restore, just in case*/
}




static void
circles (FILE *hd)
{
HPGL_Pt	p, center;
float	eps, r;
double	phi;
int outside=0;

  if (read_float (&r, hd))	/* No radius found	*/
	return ;

  switch (read_float (&eps, hd))
  {
    case 0:
	break;
    case 1:			/* No resolution option	*/
	eps = 5.0;		/*    so use default!	*/
	break;
    case 2:    			/* Illegal state	*/
	par_err_exit (98,CI);
    case EOF:
	return;
    default:			/* Illegal state	*/
	par_err_exit (99,CI);
  }
  eps *= M_PI / 180.0;		/* Deg-to-Rad		*/


  center = p_last;

  if (r == 0.0)			/* Zero radius given	*/
	return;

  p.x = center.x + r;
  p.y = center.y;
  Pen_action_to_tmpfile (MOVE_TO, &p, scale_flag);

  if ((int) GlobalLineType < 0)	/* Adaptive patterns	*/
  {
	p.x = r * cos(eps);	/* A chord segment	*/
	p.y = r * sin(eps);
	if (scale_flag)
		User_to_Plotter_coord (&p, &p);

	/* 	Pattern length = chord length		*/
	cur_pat_len = HYPOT (p.x, p.y);
  }

  for (phi=eps; phi < 2.0 * M_PI; phi += eps)
  {
	p.x = center.x + r * cos(phi);
	p.y = center.y + r * sin(phi);
if (iwflag) 
{
if ( P1.x+(p.x-S1.x)*Q.x > C2.x || P1.y+(p.y-S1.y)*Q.y > C2.y){
/*fprintf(stderr,"IW set:point %f %f >P2\n",p.x,p.y);*/
outside=1;
 }
if (P1.x+(p.x-S1.x)*Q.x  < C1.x  || P1.y+(p.y-S1.y)*Q.y < C1.y) {
/*fprintf(stderr,"IW set:point  %f %f <P1\n",p.x,p.y);*/
outside=1;
 }
}
         if (!outside)
	Pen_action_to_tmpfile (DRAW_TO, &p, scale_flag);
        else
	Pen_action_to_tmpfile (MOVE_TO, &p, scale_flag);
	outside=0;
  }
  p.x = center.x + r;	/* Close circle at r * (1, 0)	*/
  p.y = center.y;
  Pen_action_to_tmpfile (DRAW_TO, &p, scale_flag);

  Pen_action_to_tmpfile (MOVE_TO, &center, scale_flag);

  cur_pat_len = glb_pat_len;	/* Restore, just in case*/
}

static void
wedges (FILE *hd) /*derived from circles*/
{
HPGL_Pt	p, center;
float	eps, r, start, sweep;
double	phi;
int outside=0;

  if (read_float (&r, hd))	/* No radius found	*/
	return ;

  if (read_float (&start,hd)) /* No start angle found */
  return;

  if (read_float (&sweep,hd)) /* No sweep angle found */
  return;

  switch (read_float (&eps, hd)) /* chord angle */
  {
    case 0:
	break;
    case 1:			/* No resolution option	*/
	eps = 5.0;		/*    so use default!	*/
	break;
    case 2:    			/* Illegal state	*/
	par_err_exit (98,EW);
    case EOF:
	return;
    default:			/* Illegal state	*/
	par_err_exit (99,EW);
  }
  eps *= M_PI / 180.0;		/* Deg-to-Rad		*/
  start *= M_PI / 180.0;		/* Deg-to-Rad		*/
  sweep *= M_PI / 180.0;		/* Deg-to-Rad		*/


  center = p_last;              /* reference point is last position */

  if (r == 0.0)			/* Zero radius given	*/
	return;

  p.x = center.x  + r*cos(start) ;
  p.y = center.y + r *sin(start);
  Pen_action_to_tmpfile (DRAW_TO, &p, scale_flag);

  if ((int) GlobalLineType < 0)	/* Adaptive patterns	*/
  {
	p.x = r * cos(eps);	/* A chord segment	*/
	p.y = r * sin(eps);
	if (scale_flag)
		User_to_Plotter_coord (&p, &p);

	/* 	Pattern length = chord length		*/
	cur_pat_len = HYPOT (p.x, p.y);
  }

  for (phi=eps; phi <= sweep; phi += eps)
  {
	p.x = center.x + r * cos(start+phi);
	p.y = center.y + r * sin(start+phi);
if (iwflag) 
{
if ( P1.x+(p.x-S1.x)*Q.x > C2.x || P1.y+(p.y-S1.y)*Q.y > C2.y){
/*fprintf(stderr,"IW set:point %f %f >P2\n",p.x,p.y);*/
outside=1;
 }
if (P1.x+(p.x-S1.x)*Q.x  < C1.x  || P1.y+(p.y-S1.y)*Q.y < C1.y) {
/*fprintf(stderr,"IW set:point  %f %f <P1\n",p.x,p.y);*/
outside=1;
 }
}
         if (!outside)
	Pen_action_to_tmpfile (DRAW_TO, &p, scale_flag);
        else
	Pen_action_to_tmpfile (MOVE_TO, &p, scale_flag);
	outside=0;
  }

  Pen_action_to_tmpfile (DRAW_TO, &center, scale_flag);

  cur_pat_len = glb_pat_len;	/* Restore, just in case*/
}


/**
 ** Rectangles --  by Th. Hiller (hiller@tu-harburg.d400.de)
 **/

static void
rect (int relative, FILE *hd)
{
HPGL_Pt p;
HPGL_Pt p1;

  for ( ; ; )
  {
	if (read_float (&p.x, hd)) /* No number found */
		return ;

	if (read_float (&p.y, hd)) /* x without y invalid! */
		par_err_exit (2,EA);


	if (relative)   /* Process coordinates */
	{
		p.x += p_last.x;
		p.y += p_last.y;
	}

	p1.x = p_last.x; p1.y = p.y;
        Pen_action_to_tmpfile (DRAW_TO, &p1, scale_flag);
	p1.x = p.x; p1.y = p.y;
        Pen_action_to_tmpfile (DRAW_TO, &p1, scale_flag);
	p1.x = p.x; p1.y = p_last.y;
        Pen_action_to_tmpfile (DRAW_TO, &p1, scale_flag);
	p1.x = p_last.x; p1.y = p_last.y;
        Pen_action_to_tmpfile (DRAW_TO, &p1, scale_flag);

/*	Pen_action_to_tmpfile (MOVE_TO, &p, scale_flag);
	p_last = p;
*/
  }
}




static void
ax_ticks (int mode)
{
HPGL_Pt	p0, p1, p2;

  p0 = p1 = p2 = p_last;

/**
 ** According to the HP-GL manual,
 ** XT & YT are not affected by LT:
 **/
  CurrentLineType = LT_solid;

  if (mode == 0)	/* X tick	*/
  {
  	/* dividing by Q appears to be unnecessary - MK
	p1.y -= neg_ticklen * (P2.y - P1.y) / Q.y;
	p2.y += pos_ticklen * (P2.y - P1.y) / Q.y;
	*/
	p1.y -= neg_ticklen * (P2.y - P1.y) ;
	p2.y += pos_ticklen * (P2.y - P1.y) ;
  }
  else			/* Y tick	*/
  {
	/*
	p1.x -= neg_ticklen * (P2.x - P1.x) / Q.x;
	p2.x += pos_ticklen * (P2.x - P1.x) / Q.x;
	*/
	p1.x -= neg_ticklen * (P2.x - P1.x);
	p2.x += pos_ticklen * (P2.x - P1.x);
  }

  Pen_action_to_tmpfile (MOVE_TO, &p1, scale_flag);
  Pen_action_to_tmpfile (DRAW_TO, &p2, scale_flag);
  Pen_action_to_tmpfile (MOVE_TO, &p0, scale_flag);

  CurrentLineType = GlobalLineType;
}



/**
 **	Process a single HPGL command
 **/

static void
read_HPGL_cmd (GEN_PAR *pg,int c, FILE *hd)
{
short	cmd, old_pen;
HPGL_Pt	p1, p2;
float	ftmp;
float csfont;
        int mypen,myred,mygreen,myblue,i;
        float mywidth,myheight;
/**
 ** Each command consists of 2 characters. We unite them here to a single int
 ** to allow for easy processing within a big switch statement:
 **/
  cmd = c<<8;
  if ((c = getc(hd)) == EOF)
	return;
  cmd |= (c & 0xFF);

  switch (cmd & 0xDFDF)	/* & forces to upper case	*/
  {
  /**
   ** Commands appear in alphabetical order within each topic group
   ** except for command synonyms.
   **/
    case AA:		/* Arc Absolute			*/
	arcs (FALSE, hd);
	tp->CR_point = HP_pos;
	break;
    case AR:		/* Arc Relative			*/
	arcs (TRUE, hd);
	tp->CR_point = HP_pos;
	break;
    case CA:		/* Alternate character set      */
    	if (read_float (&csfont, hd))    /* just CA;    */
    		tp->altfont = 0;
    	else
    		tp->altfont =csfont;
    	break;
    case CI:		/* Circle			*/
	circles (hd);
	break;
    case CS:            /*character set selection       */
	if (read_float (&csfont, hd))	/* just CS;	*/
		tp->font = 0;
	else
	tp->font=csfont;
	tp->stdfont=csfont;	
        break;
   case EW:		/* Edge Wedge			*/
	wedges (hd);
	break;
     case NP:            /* Number of Pens                    */
    	if (read_float (&ftmp, hd) || ftmp >NUMPENS)  /* invalid or missing */
    	break;
    	 else   {
    	  pg->maxpens=ftmp;
         fprintf(stderr, "NP: %d pens requested\n",pg->maxpens);
        }
        break;
    case PA:		/* Plot Absolute		*/
	lines (plot_rel = FALSE,  hd);
	tp->CR_point = HP_pos;
	break;
    case PC:            /* Pen Color                    */
    	if (read_float (&ftmp, hd) || ftmp >pg->maxpens) { /* invalid or missing */
    	break;
    	} else  {  
    	  mypen=ftmp;
    	  if (read_float (&ftmp, hd)) /* no red component  */
    	  myred=0;
    	  else
    	  myred=ftmp;
    	  if (read_float (&ftmp, hd)) /* no green component  */
          mygreen=0;
          else
          mygreen=ftmp;
    	  if (read_float (&ftmp, hd)) /* no blue component  */
    	  myblue=0;
    	  else
    	  myblue=ftmp;
          pg->is_color  = TRUE;
          pg->Clut[mypen][0]=myred;
          pg->Clut[mypen][1]=mygreen;
          pg->Clut[mypen][2]=myblue;
	    pg->pencolor[mypen]=mypen;
	    break;
	  }
         break;
    case PD:		/* Pen  Down			*/
	pen_down = TRUE;
	lines (plot_rel,  hd);
	tp->CR_point = HP_pos;
	break;
    case PE:
	read_PE(pg,hd);
	tp->CR_point = HP_pos;
	break;
    case PR:		/* Plot Relative		*/
	lines (plot_rel = TRUE, hd);
	tp->CR_point = HP_pos;
	break;
    case PS:
    	if (read_float (&ftmp, hd) ){  /* no parameters */
        break;
        } else {
        myheight=ftmp;
        } 
    	if (read_float (&ftmp, hd) ){  /* no parameters */
        mywidth=xmax;
        } else {
        mywidth=ftmp;
        } 
        ps_flag=1;
        M.x=myheight;
        M.y=mywidth;
        p1.x=0;p1.y=0;
  if (scale_flag)		/* Rescaling	*/
	User_to_Plotter_coord (&p1, &p2);
  else
	p2 = p1;		/* Local copy	*/


  if (rotate_flag)	/* hp2xx-specific global rotation	*/
  {
	ftmp = rot_cos * p2.x - rot_sin * p2.y;
	p2.y = rot_sin * p2.x + rot_cos * p2.y;
	p2.x = ftmp;
  }
  xmin = MIN (p2.x, xmin);
  ymin = MIN (p2.y, ymin);
  xmax = MAX (p2.x, xmax);
  ymax = MAX (p2.y, ymax);
	p1.x=myheight;
	p1.y=mywidth;

  if (scale_flag)		/* Rescaling	*/
	User_to_Plotter_coord (&p1, &p2);
  else
	p2 = p1;		/* Local copy	*/


  if (rotate_flag)	/* hp2xx-specific global rotation	*/
  {
	ftmp = rot_cos * p2.x - rot_sin * p2.y;
	p2.y = rot_sin * p2.x + rot_cos * p2.y;
	p2.x = ftmp;
  }
  xmin = MIN (p2.x, xmin);
  ymin = MIN (p2.y, ymin);
  xmax = MAX (p2.x, xmax);
  ymax = MAX (p2.y, ymax);
	break;
    case PU:		/* Pen  Up			*/
	pen_down = FALSE;
	lines (plot_rel,  hd);
	tp->CR_point = HP_pos;
	break;
    case PW:            /* Pen Width                    */
    	if (read_float (&ftmp, hd) ){  /* no parameters -> set defaults */
        mywidth=0.35;
    	for (i =1;i<=pg->maxpens;++i) pg->pensize[i]=mywidth*10.;
/*fprintf(stderr,"PW: defaulting to 0.35 for all pens\n");*/
        break;
        } else {
        mywidth=ftmp; /* first or only parameter is width */
        if (mywidth <= 0.) mywidth=0.1;
        } 
        
    	if (read_float (&ftmp, hd) ) { /* width only, applies to all pens*/
    	for (i =1;i<=pg->maxpens;++i) pg->pensize[i]=mywidth*10.;
             if (pg->maxpensize < mywidth*10.) pg->maxpensize=mywidth*10.;
/*fprintf(stderr,"PW: defaulting to %d for all pens\n",mywidth*10.);*/
        } else { /* second parameter is pen */
    	  if  (ftmp <=pg->maxpens) {
    	     mypen=ftmp;
             pg->pensize[mypen]=mywidth*10.;
             if (pg->maxpensize < mywidth*10.) pg->maxpensize=mywidth*10.;
/*fprintf(stderr,"pen%d, size now %d\n",mypen,pg->pensize[mypen]);*/
          }
        }
        break;     
    case TL:		/* Tick Length			*/
	if (read_float (&ftmp, hd)) /* No number found	*/
	{
		neg_ticklen = pos_ticklen = 0.005;
		return;
	}
	else
		pos_ticklen = ftmp / 100.0;

	if (read_float (&ftmp, hd)) /* pos, but not neg	*/
	{
		neg_ticklen = 0.0;
		return;
	}
	else
		neg_ticklen = ftmp / 100.0;
	break;
    case XT:		/* X Tick			*/
	ax_ticks (0);
	break;
    case YT:		/* Y Tick			*/
	ax_ticks (1);
	break;


    case IP:		/* Input reference Points P1,P2	*/
	tp->width  /= (P2.x - P1.x);
	tp->height /= (P2.y - P1.y);
	if (read_float (&p1.x, hd)) /* No number found	*/
	{
		P1.x = P1X_default;
		P1.y = P1Y_default;
		P2.x = P2X_default;
		P2.y = P2Y_default;
		goto IP_Exit ;
	}
	if (read_float (&p1.y, hd))	/* x without y! */
		par_err_exit (2,cmd);

	if (read_float (&p2.x, hd)) /* No number found	*/
	{
		P2.x += p1.x - P1.x;
		P2.y += p1.y - P1.y;
		P1    = p1;
		goto IP_Exit ;
	}
	if (read_float (&p2.y, hd))	/* x without y! */
		par_err_exit (4,cmd);

	P1 = p1;
	P2 = p2;


    IP_Exit:
	Q.x = (P2.x - P1.x) / (S2.x - S1.x);
	Q.y = (P2.y - P1.y) / (S2.y - S1.y);
	Diag_P1_P2  = HYPOT (P2.x - P1.x, P2.y - P1.y);
	glb_pat_len = cur_pat_len = 0.04 * Diag_P1_P2;
	tp->width  *= (P2.x - P1.x);
	tp->height *= (P2.y - P1.y);
	adjust_text_par();
	return;

    case IW:
	iwflag=1;
	if (read_float (&C1.x, hd)) /* No number found	*/
	{
		C1.x = P1.x;
		C1.y = P1.y;
		C2.x = P2.x;
		C2.y = P2.y;
		break;
	}
	else {
	if (read_float (&C1.y, hd))	/* x without y! */
		par_err_exit (2,cmd);
	if (read_float (&C2.x, hd)) /* No number found	*/
                par_err_exit(3,cmd);
	if (read_float (&C2.y, hd))	/* x without y! */
		par_err_exit (4,cmd);
	}
    break;
    
    case OP:		/* Output reference Points P1,P2*/
	if (!silent_mode)
	{
		Eprintf ( "\nP1 = (%g, %g)\n", P1.x, P1.y);
		Eprintf (   "P2 = (%g, %g)\n", P2.x, P2.y);
	}
	break ;

    case AF:
    case AH:
    case PG:		/* new PaGe			*/
			/* record ON happens only once!	*/
	page_number++;
	record_off =     (first_page > page_number)
		     || ((last_page  < page_number) && (last_page > 0));
	break ;

    case EA:		/* Edge Rectangle absolute */
	rect (plot_rel = FALSE,  hd);
	tp->CR_point = HP_pos;
	break;

    case LT:		/* Line Type: 			*/
	if (read_float (&p1.x, hd))	/* just LT;	*/
		GlobalLineType = LT_solid;
	else
	{
		switch ((int) p1.x)
		{
		 case 0:	/* Starting dot (LT0;) 	*/
			GlobalLineType = LT_plot_at;	break;
		 case 1:
			GlobalLineType = LT_d_fix;	break;
		 case -1:
			GlobalLineType = LT_d_ada;	break;
		 case 2:
			GlobalLineType = LT_l_fix;	break;
		 case -2:
			GlobalLineType = LT_l_ada;	break;
		 case 3:
			GlobalLineType = LT_ls_fix;	break;
		 case -3:
			GlobalLineType = LT_ls_ada;	break;
		 case 4:
			GlobalLineType = LT_lgd_fix;	break;
		 case -4:
			GlobalLineType = LT_lgd_ada;	break;
		 case 5:
			GlobalLineType = LT_lgs_fix;	break;
		 case -5:
			GlobalLineType = LT_lgs_ada;	break;
		 case 6:
			GlobalLineType = LT_lgsgs_fix;	break;
		 case -6:
			GlobalLineType = LT_lgsgs_ada;	break;
		 default:
			Eprintf ("Illegal line type:\t%d\n", (int) p1.x);
		}

		if (!read_float (&p1.y, hd))	/* optional pattern length?	*/
		{
			if (p1.y < 0.0)
				Eprintf ("Illegal pattern length:\t%g\n", p1.y);
			else
				glb_pat_len = Diag_P1_P2 * p1.y / 100.0;
		}
	}
	CurrentLineType = GlobalLineType;
	cur_pat_len	= glb_pat_len;
	break;

    case SC:		/* Input Scale Points S1,S2	*/
	User_to_Plotter_coord (&p_last, &p_last);
	if (read_float (&S1.x, hd)) /* No number found	*/
	{
		S1.x = P1X_default;
		S1.y = P1Y_default;
		S2.x = P2X_default;
		S2.y = P2Y_default;
		scale_flag = FALSE;
		Q.x = Q.y = 1.0;
		return;
	}
	if (read_float (&S2.x, hd))	/* x without y! */
		par_err_exit (2,cmd);
	if (read_float (&S1.y, hd)) /* No number found	*/
		par_err_exit (3,cmd);
	if (read_float (&S2.y, hd))	/* x without y! */
		par_err_exit (4,cmd);

	Q.x = (P2.x - P1.x) / (S2.x - S1.x);
	Q.y = (P2.y - P1.y) / (S2.y - S1.y);
	scale_flag = TRUE;
	Plotter_to_User_coord (&p_last, &p_last);
	break;

    case SP:		/* Select pen: none/0, or 1...8	*/
	old_pen = pen;
	if (read_float (&p1.x, hd))	/* just SP;	*/
		pen = 0;
	else
		pen = (int) p1.x;

	if (pen < 0 || pen > pg->maxpens)
	{
		Eprintf (
		    "\nIllegal pen number %d: replaced by %d\n", pen, pen % pg->maxpens);
		n_unexpected++;
		pen = pen % pg->maxpens;
	}
	if (old_pen != pen)
	{
		if ((fputc (SET_PEN, td) ==  EOF) ||
		    (fputc (    pen, td) ==  EOF))
		{
			PError("Writing to temporary file:");
			Eprintf ("Error @ Cmd %ld\n", vec_cntr_w);
			exit (ERROR);
		}
	}
	if (pen) pens_in_use[pen]=1;
/*		pens_in_use |= (1 << (pen-1));*/
	break;

    case DF:		/* Set to default		*/
    case BP:   /* Begin Plot */
    case IN:   /* Initialize */
	reset_HPGL();
	tp->CR_point = HP_pos;
	break;
    case RO:
	if (read_float (&ftmp, hd)) /* No number found	*/
	{
		break;
	} else {
	fprintf(stderr,"RO encountered, rotating P1,P2 by %f\n",ftmp);

        rotate_flag=1;
rot_ang+=ftmp;

	switch ((int)ftmp) {
		case 90: 
		case 270:
			 ftmp=M.x;M.x=M.y;M.y=ftmp;
			 break;
		case 180: 
		default :
			break;  
		}
	fprintf(stderr,"cumulative rot_ang now %f\n",rot_ang);
	rot_cos =cos (M_PI * rot_ang / 180.0);
	rot_sin =sin (M_PI * rot_ang / 180.0);
        rotate_flag=1;

	if (ps_flag){ /* transform extents from previous PS statement */

  xmin		= 1e10;
  ymin		= 1e10;
  xmax		= 1e-10;
  ymax		= 1e-10;

	p1.x=0;p1.y=0;
	  if (scale_flag)		/* Rescaling	*/
	User_to_Plotter_coord (&p1, &p2);
  else
	p2 = p1;		/* Local copy	*/
  HP_pos = p2;		/* Actual plotter pos. in plotter coord	*/
	ftmp = rot_cos * p2.x - rot_sin * p2.y;
	p2.y = rot_sin * p2.x + rot_cos * p2.y;
	p2.x = ftmp;
	  xmin = MIN (p2.x, xmin);
  ymin = MIN (p2.y, ymin);
  xmax = MAX (p2.x, xmax);
  ymax = MAX (p2.y, ymax);
	p1.x=M.x;
	p1.y=M.y;
  if (scale_flag)		/* Rescaling	*/
	User_to_Plotter_coord (&p1, &p2);
  else
	p2 = p1;		/* Local copy	*/
  HP_pos = p2;		/* Actual plotter pos. in plotter coord	*/
	ftmp = rot_cos * p2.x - rot_sin * p2.y;
	p2.y = rot_sin * p2.x + rot_cos * p2.y;
	p2.x = ftmp;
  xmin = MIN (p2.x, xmin);
  ymin = MIN (p2.y, ymin);
  xmax = MAX (p2.x, xmax);
  ymax = MAX (p2.y, ymax);
	}
	}
	break;
    case BL:		/* Buffer label string		*/
	read_string (strbuf, hd);
	break;
    case CP:		/* Char Plot (rather: move)	*/
	if (read_float (&p1.x, hd)) /* No number found	*/
	{
		plot_string("\n\r", LB_direct);
		return;
	}
	else
		if (read_float (&p1.y, hd))
			par_err_exit (2,cmd);

	p2.x = p1.x * tp->chardiff.x - p1.y * tp->linediff.x + HP_pos.x;
	p2.y = p1.x * tp->chardiff.y - p1.y * tp->linediff.y + HP_pos.y;
	Pen_action_to_tmpfile (MOVE_TO, &p2, FALSE);
	break;
    case DI:		/* Char plot Dir (absolute)	*/
	if (read_float (&p1.x, hd)) /* No number found	*/
	{
		tp->dir = 0.0;
		tp->CR_point = HP_pos;
		adjust_text_par();
		break;
	}
	if (read_float (&p1.y, hd))	/* x, but not y	*/
		par_err_exit (2,cmd);
	if ((p1.x==0.0) && (p1.y==0.0))
		par_err_exit (0,cmd);
	tp->dir = atan2(p1.y, p1.x);
	tp->CR_point = HP_pos;
	adjust_text_par();
	break;
    case DR:		/* Char plot Dir (rel P1,P2)	*/
	if (read_float (&p1.x, hd)) /* No number found	*/
	{
		tp->dir = 0.0;
		tp->CR_point = HP_pos;
		adjust_text_par();
		break;
	}
	if (read_float (&p1.y, hd))
		par_err_exit (2,cmd);	/* x, but not y	*/
	if ((p1.x==0.0) && (p1.y==0.0))
		par_err_exit (0,cmd);
	tp->dir = atan2(p1.y*(P2.y - P1.y), p1.x*(P2.x - P1.x));
	tp->CR_point = HP_pos;
	adjust_text_par();
	break;
    case DT:		/* Define string terminator	*/
	StrTerm = getc(hd);
	break;
    case ES:		/* Extra Space			*/
	if (read_float (&tp->espace, hd))/* No number found*/
	{
		tp->espace = 0.0;
		tp->eline  = 0.0;
	}
	else
		if (read_float (&tp->eline, hd))
			tp->eline = 0.0;	/* Supply default	*/
	adjust_text_par();
	break;
    case LB:		/* Label string			*/
	read_string (strbuf, hd);
	plot_string (strbuf, LB_direct);
	/*
	 * Bug fix by W. Eric Norum:
	 * Update the position so that subsequent `PR's will work.
	 */
	if (scale_flag)
		Plotter_to_User_coord (&HP_pos, &p_last);
	else
		p_last = HP_pos;
	break;
    case LO:		/* Label Origin			*/
	if (read_float (&p1.x, hd))  /* No number found	*/
		tp->orig = 1;
	else
	{
		tp->orig = (int) p1.x;
		if (tp->orig < 1 || tp->orig == 10 || tp->orig > 19)
			tp->orig = 1;	/* Error	*/
	}
	adjust_text_par();
	break;
    case PB:		/* Plot Buffered label string	*/
	plot_string (strbuf, LB_buffered);
	break;
    case SI:		/* Char cell Sizes (absolute)	*/
	if (read_float (&tp->width, hd))/* No number found*/
	{
		tp->width  = 0.187;	/* [cm], A4	*/
		tp->height = 0.269;	/* [cm], A4	*/
	}
	else
	{
		if (read_float (&tp->height, hd))
			par_err_exit (2,cmd);
		if ((tp->width==0.0) || (tp->height==0.0))
			par_err_exit (0,cmd);
	}
	tp->width  *= 400.0; /* [cm] --> [plotter units]	*/
	tp->height *= 400.0; /* [cm] --> [plotter units]	*/
	adjust_text_par();
	break;
    case SL:		/* Char Slant			*/
	if (read_float (&tp->slant, hd)) /* No number found	*/
		tp->slant =  0.0;
	adjust_text_par();
	break;
    case SM:		/* Symbol Mode			*/
	read_symbol_char (hd);
	break;
    case SR:		/* Character  sizes (Rel P1,P2)	*/
	if (read_float (&tp->width, hd)) /* No number found*/
	{
		tp->width  = 0.75; /* % of (P2-P1)_x	*/
		tp->height = 1.5;  /* % of (P2-P1)_y	*/
	}
	else
	{
		if (read_float (&tp->height, hd))
			par_err_exit (2,cmd);
		if ((tp->width==0.0) || (tp->height==0.0))
			par_err_exit (0,cmd);
	}
	tp->width  *= (P2.x-P1.x)/100.0; /* --> [pl. units]	*/
	tp->height *= (P2.y-P1.y)/100.0;
	adjust_text_par();
	break;
    case SA:            /* Select designated alternate charset */
    	if (tp->altfont)
    		tp->font=tp->altfont;
    	else           /* Was never designated, default to 0 */
    		tp->font=0;
    	break;
    case SS:            /* Select designated standard character set */
    	if (tp->stdfont)
    		tp->font=tp->stdfont;
    	else           /* Was never designated, default to 0 */
    		tp->font=0;
    	break;
    case UC:            /* User defined character	*/
	plot_user_char (hd);
	break;
    case MG:
    case WD:		/* Write string	to display	*/
	read_string (strbuf, hd);
	if (!silent_mode)
		Eprintf ( "\nLABEL: %s\n", strbuf);
	break;

    default :		/* Skip unknown HPGL command: */
	n_unknown++;
	if (!silent_mode)
		Eprintf ("  %c%c: ignored  ",cmd>>8,cmd&0xFF);
	if (c==EOF)
	{
		n_unexpected++;
		if (!silent_mode)
			Eprintf ("\nUnexpected EOF!\t");
	}
	break;
  }
}


void read_HPGL ( GEN_PAR* pg, const IN_PAR* pi)
/**
 ** This routine is the high-level entry for HP-GL processing.
 ** It reads the input stream character-by-character, identifies
 ** ESC. commands (device controls) and HP-GL mnemonics, reads
 ** paramters (if expected), and initiates processing of these
 ** commands. It finally reports on this parsing process.
 **/
{
int	c;

  init_HPGL (pg, pi);

  if (!pg->quiet)
	Eprintf ( "\nReading HPGL file\n");

  /**
   ** MAIN parser LOOP!!
   **/
  while ((c = getc(pi->hd)) != EOF)
	if (c == ESC)
		read_ESC_cmd (pi->hd);	/* ESC sequence			*/
	else
	{
		if ((c<'A') || (c>'z') || ((c>'Z')&&(c<'a')))
			continue;	/* Wait for HPGL mnemonic...	*/
		read_HPGL_cmd (pg, c, pi->hd); /* ... Process it		*/
	}

  if (!pg->quiet)
  {
	Eprintf ("\nHPGL command(s) ignored: %d\n", n_unknown);
	Eprintf ("Unexpected event(s):  %d\n",  n_unexpected);
	Eprintf ("Internal command(s):  %ld\n", vec_cntr_w);
	Eprintf ("Pens used: ");
/*	for (c=0; c < NUMPENS; c++, pens_in_use >>= 1)
		if (pens_in_use & 1)
*/
	for (c=0; c < NUMPENS; c++)
		if (pens_in_use[c] == 1)	
			Eprintf ("%d ", c);
/*			Eprintf ("%d ", c+1);*/
	Eprintf ("\nMax. number of pages: %d\n",  page_number);
  }
}




void
adjust_input_transform (const GEN_PAR *pg, const IN_PAR *pi, OUT_PAR *po)
{
/**
 ** The temporary input data of the temp. file may be re-used multiple
 ** times by calling this function with varying parameters,
 ** mainly in pi.
 **
 ** Some conversion factors for transformation from HP-GL coordinates
 ** (as given in the temp. file) into mm or pel numbers are set here.
 ** There are both global parameters and elemts of po set here.
 ** DPI-related factors only apply if the current mode is a raster mode.
 **
 ** # points (dots) in any direction = range [mm] * 1in/25.4mm * #dots/in
 **/

double	dot_ratio, Dx, Dy, tmp_w, tmp_h;
char	*dir_str;

  Dx = xmax - xmin;
  Dy = ymax - ymin;
  dot_ratio = (double) po->dpi_y / (double) po->dpi_x;
  po->width = pi->width;
  po->height= pi->height;
  po->xoff  = pi->xoff;
  po->yoff  = pi->yoff;

  /* Width  assuming given height:	*/
  tmp_w     = pi->height * Dx / Dy * pi->aspectfactor;
  tmp_w     = pi->height * Dx / Dx / pi->aspectfactor;
  /* Height assuming given width:	*/
  tmp_h     = pi->width  * Dy / Dx / pi->aspectfactor;

  /**
   ** EITHER width OR height MUST be the correct limit. The other will
   ** be adapted. Adaptation of both is inconsistent, except in truesize mode.
   **/

  if (pi->truesize)
  {
    po->width	= Dx / 40.0;	/* Ignore -w, take natural HP-GL range	*/
    po->height	= Dy / 40.0;	/* Ignore -h, take natural HP-GL range	*/
    po->HP_to_xdots = (float) (po->dpi_x / 1016.0); /* dots per HP unit	*/
    po->HP_to_ydots = (float) (po->dpi_y / 1016.0); /*  (1/40 mm)	*/
    dir_str	= "true sizes";
if (pi->center_mode)  {
fprintf(stderr,"trying to center image\n");
fprintf(stderr,"po->width ?<? tmp_w: %f %f\n",po->width, tmp_w);
fprintf(stderr,"po->height ?<? tmp_h: %f %f\n",po->height, tmp_h);
if (po->width < tmp_w) po->xoff += (tmp_w - po->width )/2.0;
if (po->height < tmp_h) po->yoff += (tmp_h - po->height)/2.0;
}

  }
  else
  {
    if (po->width > tmp_w)
    {
	po->HP_to_ydots = (float) (po->dpi_y * po->height)/ Dy / 25.4;
	po->HP_to_xdots = po->HP_to_ydots * pi->aspectfactor  / dot_ratio;
	if (pi->center_mode)
		po->xoff += (po->width - tmp_w) / 2.0;	/* by L. Lowe	*/
	po->width   = tmp_w;
	dir_str	    = "width adapted";	/* Height fits, adjust width	*/
    }
    else
    {
	po->HP_to_xdots = (float) (po->dpi_x * po->width) / Dx / 25.4;
	po->HP_to_ydots = po->HP_to_xdots * dot_ratio / pi->aspectfactor;
	if (pi->center_mode)
		po->yoff += (po->height - tmp_h) / 2.0;	/* by L. Lowe	*/
	po->height  = tmp_h;
	dir_str	    = "height adapted";	/* Width  fits, adjust height	*/
    }
  }

  if (!pg->quiet)
  {
	Eprintf ("\nWidth  x  height: %5.2f x %5.2f mm, %s\n",
		po->width, po->height, dir_str);
	Eprintf ("Coordinate range: (%g, %g) ... (%g, %g)\n",
			xmin, ymin, xmax, ymax);
  }

  po->xmin = xmin;
  po->xmax = xmax;
  po->ymin = ymin;
  po->ymax = ymax;
}



PlotCmd
PlotCmd_from_tmpfile (void)
{
int	cmd;

  if (!silent_mode) switch (vec_cntr_r++)
  {
    case 0:	Eprintf ("\nProcessing Cmd: ");break;
    case 1:	Eprintf ("1 ");		break;
    case 2:	Eprintf ("2 ");		break;
    case 5:	Eprintf ("5 ");		break;
    case 10:	Eprintf ("10 ");	break;
    case 20:	Eprintf ("20 ");	break;
    case 50:	Eprintf ("50 ");	break;
    case 100:	Eprintf ("100 ");	break;
    case 200:	Eprintf ("200 ");	break;
    case 500:	Eprintf ("500 ");	break;
    case 1000:	Eprintf ("1k ");	break;
    case 2000:	Eprintf ("2k ");	break;
    case 5000:	Eprintf ("5k ");	break;
    case 10000:	Eprintf ("10k ");	break;
    case 20000:	Eprintf ("20k ");	break;
    case 50000L:Eprintf ("50k ");	break;
    case 100000L:Eprintf("100k ");	break;
    case 200000L:Eprintf("200k ");	break;
    case 500000L:Eprintf("500k ");	break;
  }

  switch (cmd = fgetc (td))
  {
    case NOP:
    case MOVE_TO:
    case DRAW_TO:
    case PLOT_AT:
    case SET_PEN:
	return cmd;
    case EOF:
    default:
	return CMD_EOF;
  }
}



void
HPGL_Pt_from_tmpfile (HPGL_Pt *pf)
{
  if (fread ((VOID *) pf, sizeof(*pf), 1,td) != 1)
  {
	PError("HPGL_Pt_from_tmpfile");
	Eprintf ("Error @ Cmd %ld\n", vec_cntr_r);
	exit (ERROR);
  }
  if (pf->x < xmin || pf->x > xmax)
	Eprintf("HPGL_Pt_from_tmpfile: x out of range (%g not in [%g, %g])\n",
		pf->x, xmin, xmax);
  if (pf->y < ymin || pf->y > ymax)
	Eprintf("HPGL_Pt_from_tmpfile: y out of range (%g not in [%g, %g])\n",
		pf->y, ymin, ymax);
}
