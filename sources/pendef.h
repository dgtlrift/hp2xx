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

#ifndef __PENDEF_H
#define __PENDEF_H

#define NUMPENS 256

/**
 ** Color codes used within hp2xx
 **/

#define xxBackground    0
#define xxForeground    1
#define xxRed           2
#define xxGreen         3
#define xxBlue          4
#define xxCyan          5
#define xxMagenta       6
#define xxYellow        7


typedef unsigned short int PEN_N;
typedef unsigned short int PEN_C;
typedef float PEN_W;
typedef unsigned char BYTE;
typedef struct {
   PEN_W width[NUMPENS+1];      /* -p xxxxxxxx / PW             */
   PEN_C color[NUMPENS+1];      /* -c xxxxxxxx / PC             */
   BYTE  clut[NUMPENS+9][3];    /* color lookup table           */
} PEN;

extern PEN pt;

void set_color_rgb(PEN_N index,BYTE r,BYTE g, BYTE b);
void Pen_Width_to_tmpfile (int pen, PEN_W width);
void Pen_Color_to_tmpfile (int pen, int red, int green, int blue);
int load_pen_width_table(FILE *td);
int load_pen_color_table(FILE *td);

#endif /* __PENDEF_H */
