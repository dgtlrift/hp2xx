/*
   Copyright (c) 2001-2002 Andrew Bird  All rights reserved.
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


#ifndef __LINDEF_H
#define __LINDEF_H

typedef enum { LT_solid, LT_adaptive, LT_plot_at, LT_fixed } LineType;   

extern double     CurrentLinePatLen;
extern LineType   CurrentLineType;
extern signed int CurrentLinePattern;

#define LT_MIN -8
#define LT_ZERO 0
#define LT_MAX  8

#define LT_PATTERNS  ((LT_MIN*-1) + 1 + LT_MAX)         /* -8 .. 0 ..  +8  */
#define LT_ELEMENTS   20

#define LT_PATTERN_TOL 0.005  /* 0.5% of pattern length */

typedef signed char SCHAR;    

typedef double LINESTYLE[LT_PATTERNS][LT_ELEMENTS+1];

extern  LINESTYLE lt;

void set_line_style_defaults(void);
void set_line_style(SCHAR index, ...);
void set_line_style_by_UL(FILE *hd);

void print_line_style(void);

/* line attributes */

typedef enum { LAE_butt          = 1,
               LAE_square        = 2,
               LAE_triangular    = 3,
               LAE_round         = 4 
} LineEnds;

typedef enum { LAJ_plain_miter   = 1,
               LAJ_bevel_miter   = 2,
               LAJ_triangular    = 3,
               LAJ_round         = 4,
               LAJ_bevelled      = 5,
               LAJ_nojoin        = 6
} LineJoins;

typedef int LineLimit;

typedef struct {
   LineEnds  End;
   LineJoins Join;
   LineLimit Limit;
} LineAttr;

typedef enum { LineAttrEnd,
               LineAttrJoin,
               LineAttrLimit 
} LineAttrKind;

extern LineAttr CurrentLineAttr;
extern LineEnds CurrentLineEnd;    /* used in pre tempfile context only */

void set_line_attr_defaults(void);
void set_line_attr(FILE *hd);
void Line_Attr_to_tmpfile ( LineAttrKind kind, int value);
int load_line_attr(FILE *td);


#endif /* __LINDEF_H */
