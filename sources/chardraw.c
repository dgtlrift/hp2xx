/*
   Copyright (c) 1991 - 1994 Heinz W. Werntges.  All rights reserved.
   Parts Copyright (c) 1999 -2000  Martin Kroeker  All rights reserved.
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

/** CHARDRAW.c: Implementation of characters by draw/move commands
 **
 ** 92/02/19 HWW  V 1.00b Derived from pplib.cpp and sprite.cpp (V 1.2)
 ** 92/02/27 HWW  V 2.00b Many changes - last: bug fixed in adjust_text_par()
 ** 92/03/03 HWW  V 2.01a LB != PB: acknowledged by introducing LB_Mode
 ** 92/04/28 HWW  V 2.01b VAX seems to mix up tp & TP --> TEXTP instead
 ** 92/05/28 HWW  V 2.02a plot_symbol_char() added
 ** 92/10/15 HWW  V 2.02b Line types acknowledged
 ** 92/10/17 HWW  V 2.03  RS6000 bug fixed in code_to_ucoord()
 ** 93/02/09 HWW  V 2.03b Comments added; prepared for more fonts;
 **			  Font OV bug fixed (8-bit codes now checked!)
 ** 93/04/13 HWW  V 2.04a plot_user_char() added (code by A. Treindl)
 ** 94/01/02 HWW  V 2.05a Symbol plotting: improved centering (L. Lowe)
 ** 94/02/14 HWW  V 2.05b Adapted to changes in hp2xx.h
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bresnham.h"
#include "hp2xx.h"
#include "lindef.h"
#include "hpgl.h"
#include "chardraw.h"
#include "charset0.h"
#include "charset1.h"
#include "charset2.h"
#include "charset3.h"
#include "charset4.h"
#include "charset5.h"
#include "charset6.h"
#include "charset7.h"
#include "font205.h"
#include "font173.h"

#ifdef STROKED_FONTS

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

void ASCII_to_font(int);
int tt_stroke_moveto(FT_Vector *, void *);
int tt_stroke_lineto(FT_Vector *, void *);
int tt_bezier1(FT_Vector *, FT_Vector *, void *);
int tt_bezier2(FT_Vector *, FT_Vector *, FT_Vector *, void *);
HPGL_Pt polygon[MAXPOLY], oldp;
int numpoints;
int ttfont = 0;

FT_Library library;
FT_Face face;
FT_Vector tt_refpoint;

static const FT_Outline_Funcs my_tt_functions = {
	(FT_Outline_MoveTo_Func) tt_stroke_moveto,
	(FT_Outline_LineTo_Func) tt_stroke_lineto,
	(FT_Outline_ConicTo_Func) tt_bezier1,
	(FT_Outline_CubicTo_Func) tt_bezier2,
	0, 0
};

#endif

/**
 ** NOTE: There is code here masked off by symbol STROKED_FONTS
 ** In an earlier version it had some meaning but is now inactive.
 ** I left it here in the hope that some day it may resume its
 ** earlier purpose which was:
 **   Loading of external fonts (here: Hershey fonts as within the
 **   Borland BGI fonts.)
 **/

extern HPGL_Pt HP_pos, P1, P2;

extern int iwflag;
extern short scale_flag;
extern int mode_vert;
extern HPGL_Pt C1, C2;
extern HPGL_Pt S1, Q;
TEXTPAR TEXTP, *tp = &TEXTP;



static void code_to_ucoord(char c, HPGL_Pt * pp)
/**
 ** Converts internal one-byte code (in c) for a character vector
 ** into HP-GL coordinates (pointed to by pp)
 **/
{
	double x, y;

	/*
	 * RS6000 bug fix:
	 *    outer braces of casts removed, costing 2 double ops
	 * My guess: "char" is unsigned on RS6000
	 */
	x = (double) (c >> 4) - 1.0;	/* Bits 4,5,6 --> value 0..7 */
	y = (double) (c & 0x0f) - 4.0;	/* Bits 0-3   --> value 0..f */

	pp->x = tp->Txx * x + tp->Txy * y + tp->refpoint.x + tp->offset.x;
	pp->y = tp->Tyx * x + tp->Tyy * y + tp->refpoint.y + tp->offset.y;
}




static void ASCII_to_char(int c)
/**
 ** Main user interface: Convert ASCII code c into a sequence
 ** of move/draw vectors which draw a corresponding character
 **/
{
	HPGL_Pt p;
	char *ptr;
	int outside = 0;
	static int warnfont = 0;
	int SafeLineType = CurrentLineType;
	LineEnds SafeLineEnd = CurrentLineEnd;

	CurrentLineType = LT_solid;
	PlotCmd_to_tmpfile(DEF_LA);
	Line_Attr_to_tmpfile(LineAttrEnd, LAE_round);

	switch (tp->font) {
	case 0:		/* charset 0, limited to 7 bit ASCII - 8bit addressing maps to charset 7        */

		if (c & 0x80) {
			/*Eprintf ("8bit character mapped to charset 7\n"); */
			c += 128;
			ptr = &charset7[c][0];
		} else {
			ptr = &charset0[c][0];
		}
		break;

	case 1:		/* charset 1, 9825      */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		if (c == 95 || c == 96 || c == 126) {	/* backspacing for special characters  */
			tp->refpoint.x -= tp->chardiff.x;
			tp->refpoint.y -= tp->chardiff.y;
		}
		ptr = &charset1[c][0];
		break;

	case 2:		/* charset 2, French/German     */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		if (c == 39 || c == 94 || c == 95 || c == 96 || c == 123 || c == 124 || c == 125) {	/* backspacing for special characters  */
			tp->refpoint.x -= tp->chardiff.x;
			tp->refpoint.y -= tp->chardiff.y;
		}
		ptr = &charset2[c][0];
		break;

	case 3:		/* charset 3, Scandinavian      */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		if (c == 95 || c >= 123) {	/* backspacing for special characters  */
			tp->refpoint.x -= tp->chardiff.x;
			tp->refpoint.y -= tp->chardiff.y;
		}
		ptr = &charset3[c][0];
		break;

	case 4:		/* charset 4, Spanish/Latin American    */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		if (c == 39 || c == 94 || c == 95 || c >= 123) {	/* backspacing for special characters  */
			tp->refpoint.x -= tp->chardiff.x;
			tp->refpoint.y -= tp->chardiff.y;
		}
		ptr = &charset4[c][0];
		break;

	case 5:		/* charset 5, Special Symbols   */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		if (c == 101) {	/* backspacing for special characters  */
			tp->refpoint.x -= tp->chardiff.x;
			tp->refpoint.y -= tp->chardiff.y;
		}
		ptr = &charset5[c][0];
		break;

	case 6:		/* charset 6, JIS ASCII */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		ptr = &charset6[c][0];
		break;

	case 7:		/* charset 7, 'HP Roman 8', limited to 7 bit ASCII      */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		ptr = &charset7[c][0];
		break;

	case 9:		/* charset 9, 'ISO Reference Version' */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		switch (c) {
		case 36:
			c = 58;
			ptr = &charset7[c][0];
			break;
		case 126:
			ptr = &charset6[c][0];
			break;
		default:
			ptr = &charset0[c][0];
			break;
		}
		break;

	case 30:		/* charset 30, 'ISO 646 Swedish' */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		switch (c) {
		case 36:
			c = 58;
			ptr = &charset7[c][0];
			break;
		case 91:
			c = 88;
			ptr = &charset7[c][0];
			break;
		case 92:
			c = 90;
			ptr = &charset7[c][0];
			break;
		case 93:
			c = 80;
			ptr = &charset7[c][0];
			break;
		case 123:
			c = 76;
			ptr = &charset7[c][0];
			break;
		case 124:
			c = 78;
			ptr = &charset7[c][0];
			break;
		case 125:
			c = 84;
			ptr = &charset7[c][0];
			break;
		case 126:
			ptr = &charset6[c][0];
		default:
			ptr = &charset0[c][0];
			break;
		}
		break;

	case 31:		/* charset 31, 'ISO 646 Swedish for Names' */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		switch (c) {
		case 36:
			c = 58;
			ptr = &charset7[c][0];
			break;
		case 64:
			c = 92;
			ptr = &charset7[c][0];
			break;
		case 91:
			c = 88;
			ptr = &charset7[c][0];
			break;
		case 92:
			c = 90;
			ptr = &charset7[c][0];
			break;
		case 93:
			c = 80;
			ptr = &charset7[c][0];
			break;
		case 94:
			c = 91;
			ptr = &charset7[c][0];
			break;
		case 96:
			c = 69;
			ptr = &charset7[c][0];
			break;
		case 123:
			c = 76;
			ptr = &charset7[c][0];
			break;
		case 124:
			c = 78;
			ptr = &charset7[c][0];
			break;
		case 125:
			c = 84;
			ptr = &charset7[c][0];
			break;
		case 126:
			c = 79;
			ptr = &charset7[c][0];
		default:
			ptr = &charset0[c][0];
			break;
		}
		break;

	case 32:		/* charset 30, 'ISO 646 Norway, Version 1' */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		switch (c) {
		case 91:
			c = 83;
			ptr = &charset7[c][0];
			break;
		case 92:
			c = 82;
			ptr = &charset7[c][0];
			break;
		case 93:
			c = 80;
			ptr = &charset7[c][0];
			break;
		case 123:
			c = 87;
			ptr = &charset7[c][0];
			break;
		case 124:
			c = 86;
			ptr = &charset7[c][0];
			break;
		case 125:
			c = 84;
			ptr = &charset7[c][0];
			break;
		case 126:
			ptr = &charset6[c][0];
		default:
			ptr = &charset0[c][0];
			break;
		}
		break;

	case 33:		/* charset 33, 'ISO 646 German' */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		switch (c) {
		case 64:
			c = 61;
			ptr = &charset7[c][0];
			break;
		case 91:
			c = 88;
			ptr = &charset7[c][0];
			break;
		case 92:
			c = 90;
			ptr = &charset7[c][0];
			break;
		case 93:
			c = 91;
			ptr = &charset7[c][0];
			break;
		case 123:
			c = 76;
			ptr = &charset7[c][0];
			break;
		case 124:
			c = 78;
			ptr = &charset7[c][0];
			break;
		case 125:
			c = 79;
			ptr = &charset7[c][0];
			break;
		case 126:
			c = 94;
			ptr = &charset7[c][0];
		default:
			ptr = &charset0[c][0];
			break;
		}
		break;

	case 34:		/* charset 34, 'ISO 646 French' */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		switch (c) {
		case 35:
			c = 59;
			ptr = &charset7[c][0];
			break;
		case 64:
			c = 72;
			ptr = &charset7[c][0];
			break;
		case 91:
			c = 51;
			ptr = &charset7[c][0];
			break;
		case 92:
			c = 53;
			ptr = &charset7[c][0];
			break;
		case 93:
			c = 61;
			ptr = &charset7[c][0];
			break;
		case 123:
			c = 69;
			ptr = &charset7[c][0];
			break;
		case 124:
			c = 75;
			ptr = &charset7[c][0];
			break;
		case 125:
			c = 73;
			ptr = &charset7[c][0];
			break;
		case 126:
			c = 43;
			ptr = &charset7[c][0];
		default:
			ptr = &charset0[c][0];
			break;
		}
		break;

	case 35:		/* charset 35, 'ISO 646 United Kingdom' */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		switch (c) {
		case 35:
			c = 59;
			ptr = &charset7[c][0];
			break;
		case 126:
			ptr = &charset6[c][0];
		default:
			ptr = &charset0[c][0];
			break;
		}
		break;

	case 36:		/* charset 36, 'ISO 646 Italian' */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		switch (c) {
		case 35:
			c = 59;
			ptr = &charset7[c][0];
			break;
		case 64:
			c = 61;
			ptr = &charset7[c][0];
			break;
		case 91:
			c = 51;
			ptr = &charset7[c][0];
			break;
		case 92:
			c = 53;
			ptr = &charset7[c][0];
			break;
		case 93:
			c = 69;
			ptr = &charset7[c][0];
			break;
		case 96:
			c = 75;
			ptr = &charset7[c][0];
			break;
		case 123:
			c = 72;
			ptr = &charset7[c][0];
			break;
		case 124:
			c = 74;
			ptr = &charset7[c][0];
			break;
		case 125:
			c = 73;
			ptr = &charset7[c][0];
			break;
		case 126:
			c = 89;
			ptr = &charset7[c][0];
		default:
			ptr = &charset0[c][0];
			break;
		}
		break;


	case 37:		/* charset 37, 'ISO 646 Spanish' */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		switch (c) {
		case 35:
			c = 59;
			ptr = &charset7[c][0];
			break;
		case 64:
			c = 61;
			ptr = &charset7[c][0];
			break;
		case 91:
			c = 56;
			ptr = &charset7[c][0];
			break;
		case 92:
			c = 54;
			ptr = &charset7[c][0];
			break;
		case 93:
			c = 57;
			ptr = &charset7[c][0];
			break;
		case 123:
			c = 51;
			ptr = &charset7[c][0];
			break;
		case 124:
			c = 55;
			ptr = &charset7[c][0];
			break;
		case 125:
			c = 53;
			ptr = &charset7[c][0];
			break;
		default:
			ptr = &charset0[c][0];
			break;
		}
		break;


	case 38:		/* charset 38, 'ISO 646 Portuguese' */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		switch (c) {
		case 64:
			c = 61;
			ptr = &charset7[c][0];
			break;
		case 91:
			c = 97;
			ptr = &charset7[c][0];
			break;
		case 92:
			c = 52;
			ptr = &charset7[c][0];
			break;
		case 93:
			c = 105;
			ptr = &charset7[c][0];
			break;
		case 123:
			c = 98;
			ptr = &charset7[c][0];
			break;
		case 124:
			c = 53;
			ptr = &charset7[c][0];
			break;
		case 125:
			c = 106;
			ptr = &charset7[c][0];
			break;
		case 126:
			c = 51;
			ptr = &charset7[c][0];
			break;
		default:
			ptr = &charset0[c][0];
			break;
		}
		break;

	case 39:		/* charset 39, 'ISO 646 Norway Version 2' */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		switch (c) {
		case 35:
			c = 61;
			ptr = &charset7[c][0];
			break;
		case 91:
			c = 83;
			ptr = &charset7[c][0];
			break;
		case 92:
			c = 82;
			ptr = &charset7[c][0];
			break;
		case 93:
			c = 80;
			ptr = &charset7[c][0];
			break;
		case 123:
			c = 87;
			ptr = &charset7[c][0];
			break;
		case 124:
			c = 86;
			ptr = &charset7[c][0];
			break;
		case 125:
			c = 84;
			ptr = &charset7[c][0];
			break;
		case 126:
			c = 124;
			ptr = &charset0[c][0];
			break;
		default:
			ptr = &charset0[c][0];
			break;
		}
		break;
	case 173:		/* PS Math */
		if (c < 0)
			c += 256;
		ptr = &charset173[c][0];
		break;
	case 205:		/* Ventura Math */
		if (c < 0)
			c += 256;
		ptr = &charset205[c][0];
		break;
	default:		/* Currently, only charsets 0-7,30-39 are supported     */
		if (tp->font != warnfont) {	/* warn only once per font */
			warnfont = tp->font;
			Eprintf
			    ("Charset %d not supported -- replaced by charset 0!\n",
			     tp->font);
		}
		if (c & 0x80) {
			/*Eprintf ("8bit character mapped to charset 7\n"); */
			c += 128;
			ptr = &charset7[c][0];
		} else {
			ptr = &charset0[c][0];
		}
		break;
	}

	for (; *ptr; ptr++) {	/* Draw this char */
		code_to_ucoord(*ptr & 0x7f, &p);
		 /*MK*/ if (iwflag) {
			if (scale_flag) {
				if (P1.x + p.x > C2.x || P1.y + p.y > C2.y) {
					outside = 1;
				}
				if (P1.x + p.x < C1.x || P1.y + p.y < C1.y) {
					outside = 1;
				}
			} else {
				if (P1.x + (p.x - S1.x) * Q.x > C2.x
				    || P1.y + (p.y - S1.y) * Q.y > C2.y) {
					outside = 1;
				}
				if (P1.x + (p.x - S1.x) * Q.x < C1.x
				    || P1.y + (p.y - S1.y) * Q.y < C1.y) {
					outside = 1;
				}
			}
		}

		if ((*ptr & 0x80) && !outside)	/* High bit is draw flag */
			Pen_action_to_tmpfile(DRAW_TO, &p, FALSE);
		else
			Pen_action_to_tmpfile(MOVE_TO, &p, FALSE);

		outside = 0;
	}

	/* Update cursor: to next character origin!   */

	tp->refpoint.x += tp->chardiff.x;
	tp->refpoint.y += tp->chardiff.y;
	CurrentLineType = SafeLineType;

	/* Restore Line Ends */
	PlotCmd_to_tmpfile(DEF_LA);
	Line_Attr_to_tmpfile(LineAttrEnd, SafeLineEnd);

}


/**********************************************************************/
void init_text_par(void)
{
	tp->width = 0.005 * (P2.x - P1.x);
	tp->height = 0.0075 * (P2.y - P1.y);
	tp->espace = 0.0;
	tp->eline = 0.0;
	tp->dir = 0.0;
	tp->slant = 0.0;
	tp->font = 0;
	tp->orig = 1;		/* Font number: 0 = old */
	tp->refpoint = tp->CR_point = HP_pos;
	tp->offset.x = tp->offset.y = 0.0;
	adjust_text_par();
}




void adjust_text_par(void)
/**
 ** Width, height, space, line, dir, slant
 ** as given in structure declaration
 **/
{
	double cdir, sdir;

/**
 ** Here, we use space & line as basic data, since these parameters
 ** are affected by SI and SR commands, not width or height!
 **/
	tp->space = tp->width * 1.5;
	tp->line = tp->height * 2.0;

	cdir = cos(tp->dir);
	sdir = sin(tp->dir);
	tp->Txx = tp->width * cdir / 4.0;
	tp->Tyx = tp->width * sdir / 4.0;
	tp->Txy = tp->height * (tp->slant * cdir - sdir) / 6.0;
	tp->Tyy = tp->height * (tp->slant * sdir + cdir) / 6.0;

	tp->chardiff.x = tp->space * (1.0 + tp->espace) * cdir;
	tp->chardiff.y = tp->space * (1.0 + tp->espace) * sdir;
	tp->linediff.x = tp->line * (1.0 + tp->eline) * sdir;
	tp->linediff.y = -tp->line * (1.0 + tp->eline) * cdir;

#ifdef STROKED_FONTS
	if (init_font(tp->font)) {
		Eprintf("\007 init_font() failed for font #%d\n",
			tp->font);
		Eprintf("Font 0 used instead!\n");
		tp->font = 0;
		ttfont = 0;
	} else
		ttfont = 1;
#endif
}



#define	HEIGHT_FAC	0.666666666
#define	WIDTH_FAC	0.666666666


static void get_label_offset(char *txt, LB_Mode mode)
/**
 ** Depending on the current HP-GL label mode, a string may have different
 ** x and y offsets. These offsets are accounted for here:
 **/
{
	float dx = 0.0, dy = 0.0;
	static float nc, nl, nc_max;

	switch (mode) {
	case LB_direct:	/* Get number of printable plot cells   */
		nc = nl = nc_max = 0.0;
		for (; *txt && *txt != _CR; txt++)	/* to ETX or CR */
			if (*txt >= ' ' && ((*txt & '\x80') == 0))
				nc++;	/* Printable ?  */
			else if (*txt == _BS)	/* Backspace ?      */
				nc--;
			else if (*txt == _HT)
				nc -= 0.5;
		break;

	case LB_buffered:
		/* Max number of printable plot cells + lines   */
		nc = nl = nc_max = 0.0;
		for (; *txt; txt++)
			if (*txt >= ' ' && ((*txt & '\x80') == 0))
				nc++;
			else if (*txt == _BS)
				nc--;
			else if (*txt == _HT)
				nc -= 0.5;
			else if (*txt == _LF)
				nl++;
			else if (*txt == _VT)
				nl--;
			else if (*txt == _CR) {
				if (nc > nc_max)
					nc_max = nc;
				nc = 0.0;
			}
		if (nc > nc_max)
			nc_max = nc;
		nc = nc_max;	/* Use longest row for adjustment       */
		break;

	case LB_buffered_in_use:
		break;		/* Use data from earlier LB_buffered call       */
	}

	switch (tp->orig) {
	case 1:
	case 2:
	case 3:
		dx = 0.0;
		break;
	case 11:
	case 12:
	case 13:
		dx = 0.5 * WIDTH_FAC;
		break;
	case 4:
	case 5:
	case 6:
	case 14:
	case 15:
	case 16:
		dx = 0.5 * (1.0 - WIDTH_FAC);
		tp->refpoint.x -= tp->chardiff.x * nc / 2.0;
		tp->refpoint.y -= tp->chardiff.y * nc / 2.0;
		break;
	case 7:
	case 8:
	case 9:
		dx = 1.0 - WIDTH_FAC;
		tp->refpoint.x -= tp->chardiff.x * nc;
		tp->refpoint.y -= tp->chardiff.y * nc;
		break;
	case 17:
	case 18:
	case 19:
		dx = 1.0 - 1.5 * WIDTH_FAC;
		tp->refpoint.x -= tp->chardiff.x * nc;
		tp->refpoint.y -= tp->chardiff.y * nc;
		break;
	}

	switch (tp->orig) {
	case 1:
	case 4:
	case 7:
		dy = 0.0;
		if (mode == LB_buffered || mode == LB_buffered_in_use) {
			tp->refpoint.x -= tp->linediff.x * nl;
			tp->refpoint.y -= tp->linediff.y * nl;
		}
		break;
	case 11:
	case 14:
	case 17:
		dy = 0.5 * HEIGHT_FAC;
		if (mode == LB_buffered || mode == LB_buffered_in_use) {
			tp->refpoint.x -= tp->linediff.x * nl;
			tp->refpoint.y -= tp->linediff.y * nl;
		}
		break;
	case 2:
	case 5:
	case 8:
	case 12:
	case 15:
	case 18:
		dy = -0.5 * HEIGHT_FAC;
		if (mode == LB_buffered || mode == LB_buffered_in_use) {
			tp->refpoint.x -= tp->linediff.x * nl / 2.0;
			tp->refpoint.y -= tp->linediff.y * nl / 2.0;
		}
		break;
	case 3:
	case 6:
	case 9:
		dy = -HEIGHT_FAC;
		if (mode == LB_buffered || mode == LB_buffered_in_use) {
			tp->refpoint.x += tp->linediff.x * (nl - 1.0);
			tp->refpoint.y += tp->linediff.y * (nl - 1.0);
		}
		break;
	case 13:
	case 16:
	case 19:
		dy = -1.5 * HEIGHT_FAC;
		if (mode == LB_buffered || mode == LB_buffered_in_use) {
			tp->refpoint.x += tp->linediff.x * (nl - 1.0);
			tp->refpoint.y += tp->linediff.y * (nl - 1.0);
		}
		break;
	}

	tp->offset.x = tp->chardiff.x * dx - tp->linediff.x * dy;
	tp->offset.y = tp->chardiff.y * dx - tp->linediff.y * dy;
}




void plot_string(char *txt, LB_Mode mode, short current_pen)
/**
 ** String txt cannot simply be processed char-by-char. Depending on
 ** the current label mode, its origin must first be calculated properly.
 ** Then, there are some special control characters which affect cursor
 ** position but don't draw anything. Finally, characters can be drawn
 ** one-by-one.
 **/
{
	char *txt0;
	double savedwidth = 0.;

	txt0 = txt;
	tp->refpoint = HP_pos;
	get_label_offset(txt, mode);
	if (tp->strokewidth != 9999.) {
		savedwidth = pt.width[current_pen];
		PlotCmd_to_tmpfile(DEF_PW);
		Pen_Width_to_tmpfile(current_pen, tp->strokewidth);
	}
	while (*txt) {
		switch (*txt) {
		case ' ':
			tp->refpoint.x += tp->chardiff.x;
			tp->refpoint.y += tp->chardiff.y;
			break;
		case _CR:
			if (mode_vert)
				tp->CR_point.y -= tp->linediff.y;
			tp->refpoint = tp->CR_point;
			switch (mode) {
			case LB_direct:
				get_label_offset(txt + 1, mode);
				break;
			case LB_buffered:
				get_label_offset(txt0, LB_buffered_in_use);
				break;
			default:
				break;
			}
			break;
		case _LF:
			tp->CR_point.x += tp->linediff.x;
			tp->refpoint.x += tp->linediff.x;
			if (!mode_vert) {
				tp->CR_point.y += tp->linediff.y;
				tp->refpoint.y += tp->linediff.y;
			} else {
				tp->refpoint.y -= tp->linediff.y;
			}
			break;
		case _BS:
			tp->refpoint.x -= tp->chardiff.x;
			tp->refpoint.y -= tp->chardiff.y;
			break;
		case _HT:
			tp->refpoint.x -= 0.5 * tp->chardiff.x;
			tp->refpoint.y -= 0.5 * tp->chardiff.y;
			break;
		case _VT:
			tp->CR_point.x -= tp->linediff.x;
			tp->CR_point.y -= tp->linediff.y;
			tp->refpoint.x -= tp->linediff.x;
			tp->refpoint.y -= tp->linediff.y;
			break;
		case _SO:
			if (tp->altfont)
				tp->font = tp->altfont;
			break;
		case _SI:
			tp->font = tp->stdfont;
			break;
		default:
#ifdef STROKED_FONTS
			if (ttfont)
				ASCII_to_font((int) *txt);
			else
#endif
				ASCII_to_char((int) *txt);
			break;
		}
/**
 ** Move to next reference point, e. g. the next character origin
 **/
		if (mode_vert) {
			tp->refpoint.x -= tp->chardiff.x;
			tp->refpoint.y += tp->linediff.y;
		}
		Pen_action_to_tmpfile(MOVE_TO, &tp->refpoint, FALSE);
		txt++;
	}
	if (tp->strokewidth != 9999.) {
		PlotCmd_to_tmpfile(DEF_PW);
		Pen_Width_to_tmpfile(current_pen, savedwidth);
	}
}





static void ASCII_set_center(int c)
/**
 ** Convert ASCII code c into a sequence of move/draw vectors
 ** and determine their "center of gravity"
 **/
{
	HPGL_Pt p, center;
	int cnt;
	char *ptr;

	switch (tp->font) {

	case 0:		/* charset 0, limited to 7 bit ASCII - 8bit addressing maps to charset 7        */

		if (c & 0x80) {
			Eprintf("8bit character mapped to charset 7\n");
			c += 128;
			ptr = &charset7[c][0];
		} else {
			ptr = &charset0[c][0];
		}
		break;

	case 5:		/* charset 5, limited to 7 bit ASCII    */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		ptr = &charset5[c][0];
		break;

	case 7:		/* charset 7, 'HP Roman 8', limited to 7 bit ASCII      */

		if (c & 0x80) {
			Eprintf
			    ("Illegal char in string: %d = %c replaced by blank!\n",
			     c, c);
			c = ' ';
		}
		ptr = &charset7[c][0];
		break;

	default:		/* Currently, there is just one charset */
		Eprintf("Charset %d not supported -- replaced by blank!\n",
			tp->font);
		c = ' ';
		ptr = &charset0[c][0];
		break;
	}

	center.x = center.y = 0.0;
	for (cnt = 0; *ptr; ptr++, cnt++) {	/* Scan this char */
		code_to_ucoord(*ptr & 0x7f, &p);
		center.x += p.x;
		center.y += p.y;
	}
	if (cnt) {
		tp->offset.x = -center.x / cnt;
		tp->offset.y = -center.y / cnt;
	} else			/* Should never happen:     */
		tp->offset.x = tp->offset.y = 0.0;
}




static void set_symbol_center(char c)
/**
 ** Symbol plotting requires a special x and y offset for proper
 ** symbol-specific centering
 **/
{
	tp->refpoint.x = 0.0;
	tp->refpoint.y = 0.0;
	tp->offset.x = 0.0;
	tp->offset.y = 0.0;
	ASCII_set_center(c);
	tp->refpoint.x = HP_pos.x;	/*  - tp->chardiff.x / 2.0; */
	tp->refpoint.y = HP_pos.y;	/*  - tp->chardiff.y / 2.0; */
}




void plot_symbol_char(char c)
/**
 ** Special case: Symbol plotting. This requires a special
 ** x and y offset (for proper centering) but then simply amounts to
 ** drawing a single character.
 **/
{
	set_symbol_center(c);

#ifdef STROKED_FONTS
	if (tp->font)
		ASCII_to_font((int) c);
	else
#endif
		ASCII_to_char((int) c);

/**
 ** Move to next reference point, e. g. the next character origin
 **/
	Pen_action_to_tmpfile(MOVE_TO, &tp->refpoint, FALSE);
}




void plot_user_char(FILE * hd, short current_pen)
/**
 ** added by Alois Treindl 12-apr-93
 **/
{
	HPGL_Pt p;
	double x, y;
	float fx, fy;
	int pendown = FALSE;
	double savedwidth = 0.;

	LineType SafeLineType = CurrentLineType;	/* Save Current Line Type */
	CurrentLineType = LT_solid;

	tp->refpoint = HP_pos;
	p.x = tp->refpoint.x + tp->offset.x;
	p.y = tp->refpoint.y + tp->offset.y;

	if (tp->strokewidth != 9999.) {
		savedwidth = pt.width[current_pen];
		PlotCmd_to_tmpfile(DEF_PW);
		Pen_Width_to_tmpfile(current_pen, tp->strokewidth);
	}

	while (read_float(&fx, hd) == 0) {
		if (fx >= 99)
			pendown = TRUE;
		else if (fx <= -99)
			pendown = FALSE;
		else {
			if (read_float(&fy, hd) != 0)
				break;
			x = fx;
			y = fy * 2.0 / 3.0;
			p.x += tp->Txx * x + tp->Txy * y;
			p.y += tp->Tyx * x + tp->Tyy * y;
			if (pendown)
				Pen_action_to_tmpfile(DRAW_TO, &p, FALSE);
			else
				Pen_action_to_tmpfile(MOVE_TO, &p, FALSE);
		}
	}

	/* Update cursor: to next character origin!   */

	tp->refpoint.x += tp->chardiff.x;
	tp->refpoint.y += tp->chardiff.y;
	Pen_action_to_tmpfile(MOVE_TO, &tp->refpoint, FALSE);

	CurrentLineType = SafeLineType;	/* restore LineType */
	if (tp->strokewidth != 9999.) {
		PlotCmd_to_tmpfile(DEF_PW);
		Pen_Width_to_tmpfile(current_pen, savedwidth);
	}
}

#ifdef STROKED_FONTS

int init_font(int thefont)
{
	int error;

	if (face)
		return 0;	/* font already open */

	error = FT_Init_FreeType(&library);
	if (error) {
		fprintf(stderr, " ! FT_Init_FreeType\n");
		return -1;
	}
	error = FT_New_Face(library,
/*			    "/usr/X11R6/lib/X11/fonts/truetype/LucidaTypewriterRegular.ttf",*/
			    STROKED_FONTS, 0, &face);
	if (error) {
		fprintf(stderr, " ! FT_New_Face \n");
		return -1;
	}

/*error = FT_Set_Char_Size (
	face,
	0,
	12*64,
	100,
	100);
*/
	error = FT_Set_Pixel_Sizes(face, 20, 20);
	if (error) {
		fprintf(stderr, " ! FT_Set_Char_Size\n");
		return -1;
	}
#if 0
	fprintf(stderr, "init_font ok\n");
#endif
	return 0;
}


void ASCII_to_font(int c)
{
	int error;
	int dummy;
	HPGL_Pt boxmin, boxmax;
	FT_GlyphSlot slot = face->glyph;
	FT_Outline *theoutline = &slot->outline;
	int SafeLineType = CurrentLineType;
	LineEnds SafeLineEnd = CurrentLineEnd;

	CurrentLineType = LT_solid;
	PlotCmd_to_tmpfile(DEF_LA);
	Line_Attr_to_tmpfile(LineAttrEnd, LAE_round);
	if (c < 0)
		c += 256;
	if (tp->font == 0 || tp->font == 7)
		switch (c) {	/* HP Roman8 to iso8859 conversion table */
		case 179:
			c = 176;
			break;
		case 254:
			c = 177;
			break;
		case 243:
			c = 181;
			break;
		case 216:
			c = 196;
			break;
		case 218:
			c = 214;
			break;
		case 219:
			c = 220;
			break;
		case 222:
			c = 223;
			break;
		case 204:
			c = 228;
			break;
		case 206:
			c = 246;
			break;
		case 214:
			c = 248;
			break;
		case 207:
			c = 252;
			break;
		default:
			break;
		}
	error = FT_Load_Char(face, (FT_ULong) c, FT_LOAD_NO_SCALE);
	if (error) {
		fprintf(stderr, " ! FT_Load_Char %c\n", c);
		return;
	}
#if 0
	fprintf(stderr, "metrics : width %d, height %d advance %d\n",
		(int) (slot->metrics.width / 64.),
		(int) (slot->metrics.height / 64.),
		(int) (slot->metrics.horiAdvance / 64.));
	fprintf(stderr, "FT_Outline_Decompose...\n");
#endif

	numpoints = -1;

	error = FT_Outline_Decompose(theoutline, &my_tt_functions, &dummy);
	if (error)
		fprintf(stderr, " ! FT_Outline_Decompose\n");
#if 0
	fprintf(stderr, "refpoint %f %f + chardiff %f %f\n",
		tp->refpoint.x, tp->refpoint.y, tp->chardiff.x,
		tp->chardiff.y);
	fprintf(stderr, "numpoints %d\n", numpoints);
#endif
	boxmin.x = tp->refpoint.x - 5;
	boxmin.y = tp->refpoint.y - 150;
	boxmax.x = boxmin.x + tp->chardiff.x + 5;
	boxmax.y = boxmin.y + tp->chardiff.y + 5;
	fill(polygon, numpoints, boxmin, boxmax, 0, 2, 1, 0);
	tp->refpoint.x += tp->chardiff.x;
	tp->refpoint.y += tp->chardiff.y;
	tt_refpoint.x = 0;
	tt_refpoint.y = 0;
	/* Restore Line Ends */
	CurrentLineType = SafeLineType;
	PlotCmd_to_tmpfile(DEF_LA);
	Line_Attr_to_tmpfile(LineAttrEnd, SafeLineEnd);
}


int tt_stroke_moveto(FT_Vector * to, void *dummy)
{
	HPGL_Pt p;
#if 0
	fprintf(stderr, "TT move to %ld %ld (oder ists %d %d ??)\n", to->x,
		to->y, (int) (to->x / 64.), (int) (to->y / 64.));
#endif
	p.x = tp->Txx * to->x + tp->Txy * to->y;
	p.y = tp->Tyx * to->x + tp->Tyy * to->y;
	p.x = p.x / 200. + tp->refpoint.x + tp->offset.x;
	p.y = p.y / 200. + tp->refpoint.y + tp->offset.y;

	Pen_action_to_tmpfile(MOVE_TO, &p, FALSE);

	/* Update cursor: to next character origin!   */
	tt_refpoint.x = to->x;
	tt_refpoint.y = to->y;
	oldp = p;

	return 0;
}

int tt_stroke_lineto(FT_Vector * to, void *dummy)
{
	HPGL_Pt p;
	int outside = 0;
#if 0
	fprintf(stderr, "TT line to %ld %ld\n", to->x, to->y);
#endif

	p.x = tp->Txx * to->x + tp->Txy * to->y;
	p.y = tp->Tyx * to->x + tp->Tyy * to->y;
	p.x = p.x / 200. + tp->refpoint.x + tp->offset.x;
	p.y = p.y / 200. + tp->refpoint.y + tp->offset.y;
#if 0
	if (iwflag) {
		if (scale_flag) {
			if (P1.x + p.x > C2.x || P1.y + p.y > C2.y) {
				outside = 1;
			}
			if (P1.x + p.x < C1.x || P1.y + p.y < C1.y) {
				outside = 1;
			}
		} else {
			if (P1.x + (p.x - S1.x) * Q.x > C2.x
			    || P1.y + (p.y - S1.y) * Q.y > C2.y) {
				outside = 1;
			}
			if (P1.x + (p.x - S1.x) * Q.x < C1.x
			    || P1.y + (p.y - S1.y) * Q.y < C1.y) {
				outside = 1;
			}
		}
	}
#endif
	if (!outside) {
		/*      Pen_action_to_tmpfile (DRAW_TO, &p, FALSE); */
		polygon[++numpoints] = oldp;
		polygon[++numpoints] = p;
	} else
		Pen_action_to_tmpfile(MOVE_TO, &p, FALSE);

	outside = 0;

	/* Update cursor: to next character origin!   */
	tt_refpoint.x = to->x;
	tt_refpoint.y = to->y;
	oldp = p;
	return 0;
}

int tt_bezier1(FT_Vector * p1, FT_Vector * p3, void *dummy)
{
	HPGL_Pt p, pp;
	int i, outside;
	float t;
	FT_Vector p2;

	p2.x = p1->x;
	p2.y = p1->y;

/*    
p(t) = t^3*P3 + 3*t^2*(1-t)*P2 + 3*t*(1-t)^2* P1 + (1-t)^3 * P0
*/
#if 0
	fprintf(stderr, "TT refpoint %ld %ld\n", tt_refpoint.x,
		tt_refpoint.y);
#endif
	outside = 0;

	for (i = 0; i < 51; i++) {
		t = (float) i / 50.0;
		p.x =
		    t * t * t * p3->x + 3 * t * t * (1. - t) * p2.x
		    + 3 * t * (1. - t) * (1. - t) * p1->x
		    + (1. - t) * (1. - t) * (1. - t) * tt_refpoint.x;
		p.y =
		    t * t * t * p3->y + 3 * t * t * (1. - t) * p2.y
		    + 3 * t * (1. - t) * (1. - t) * p1->y
		    + (1. - t) * (1. - t) * (1. - t) * tt_refpoint.y;

		pp.x = tp->Txx * p.x + tp->Txy * p.y;
		pp.y = tp->Tyx * p.x + tp->Tyy * p.y;
		pp.x = pp.x / 200. + tp->refpoint.x + tp->offset.x;
		pp.y = pp.y / 200. + tp->refpoint.y + tp->offset.y;
/*fprintf(stderr,"bezier point %f %f\n",pp.x,pp.y);*/
#if 0
		if (iwflag) {
			if (P1.x + (pp.x - S1.x) * Q.x > C2.x
			    || P1.y + (pp.y - S1.y) * Q.y > C2.y) {
/*fprintf(stderr,"IW set:point %f %f >P2\n",pp.x,pp.y); */
				outside = 1;
			}
			if (P1.x + (pp.x - S1.x) * Q.x < C1.x
			    || P1.y + (pp.y - S1.y) * Q.y < C1.y) {
/*fprintf(stderr,"IW set:point  %f %f <P1\n",pp.x,pp.y); */
				outside = 1;
			}
		}
#endif
		if (!outside) {
/*	     Pen_action_to_tmpfile (DRAW_TO, &pp, FALSE);  */
			polygon[++numpoints] = oldp;
			polygon[++numpoints] = pp;
		} else
			Pen_action_to_tmpfile(MOVE_TO, &pp, FALSE);

		outside = 0;
		oldp = pp;
	}

	/* Update cursor: to next character origin!   */
	tt_refpoint.x = p3->x;
	tt_refpoint.y = p3->y;

	return 0;
}

int tt_bezier2(FT_Vector * p1, FT_Vector * p2, FT_Vector * p3, void *dummy)
{
	HPGL_Pt p, pp;
	int i, outside;
	float t;

/*    
p(t) = t^3*P3 + 3*t^2*(1-t)*P2 + 3*t*(1-t)^2* P1 + (1-t)^3 * P0
*/

	outside = 0;

	for (i = 0; i < 51; i++) {
		t = (float) i / 50.;
		p.x =
		    t * t * t * p3->x + 3 * t * t * (1. - t) * p2->x
		    + 3 * t * (1. - t) * (1. - t) * p1->x
		    + (1. - t) * (1. - t) * (1. - t) * tt_refpoint.x;
		p.y =
		    t * t * t * p3->y + 3 * t * t * (1. - t) * p2->y
		    + 3 * t * (1. - t) * (1. - t) * p1->y
		    + (1. - t) * (1. - t) * (1. - t) * tt_refpoint.y;

		pp.x = tp->Txx * p.x + tp->Txy * p.y;
		pp.y = tp->Tyx * p.x + tp->Tyy * p.y;
		pp.x = pp.x / 200. + tp->refpoint.x + tp->offset.x;
		pp.y = pp.y / 200. + tp->refpoint.y + tp->offset.y;
/*fprintf(stderr,"bezier point %f %f\n",pp.x,pp.y);*/
#if 0
		if (iwflag) {
			if (P1.x + (pp.x - S1.x) * Q.x > C2.x
			    || P1.y + (pp.y - S1.y) * Q.y > C2.y) {
/*fprintf(stderr,"IW set:point %f %f >P2\n",pp.x,pp.y); */
				outside = 1;
			}
			if (P1.x + (pp.x - S1.x) * Q.x < C1.x
			    || P1.y + (pp.y - S1.y) * Q.y < C1.y) {
/*fprintf(stderr,"IW set:point  %f %f <P1\n",pp.x,pp.y); */
				outside = 1;
			}
		}
#endif
		if (!outside) {
/*	     Pen_action_to_tmpfile (DRAW_TO, &pp, FALSE);  */
			polygon[++numpoints] = oldp;
			polygon[++numpoints] = pp;
		} else
			Pen_action_to_tmpfile(MOVE_TO, &pp, FALSE);

		outside = 0;
		oldp = pp;

	}

	/* Update cursor: to next character origin!   */
	tt_refpoint.x = p3->x;
	tt_refpoint.y = p3->y;
	return 0;
}

#endif
