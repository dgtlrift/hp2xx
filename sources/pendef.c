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


#include <stdio.h>
#include <stdlib.h>
#include "hpgl.h"
#include "pendef.h"

PEN pt;

void Pen_Width_to_tmpfile(int pen, PEN_W width)
{
	int i;
	PEN_N tp;
	PEN_W tw;

	tp = (PEN_N) pen;
	tw = width;

	if (record_off)		/* Wrong page!  */
		return;
	if (pen < 0)
		return;		/* Might happen when "current pen" is still
				   undefined */
	if (tp == 0) {		/* set all pens */
		for (i = 1; i < NUMPENS; ++i)
			pt.width[i] = tw;
	} else {
		pt.width[tp] = tw;	/* set just the specified one */
	}

	if (fwrite(&tp, sizeof(tp), 1, td) != 1) {
		PError("Pen_Width_to_tmpfile - pen");
		Eprintf("Error @ Cmd %ld\n", vec_cntr_w);
		exit(ERROR);
	}
	if (fwrite(&tw, sizeof(tw), 1, td) != 1) {
		PError("Pen_Width_to_tmpfile - width");
		Eprintf("Error @ Cmd %ld\n", vec_cntr_w);
		exit(ERROR);
	}
}

void Pen_Color_to_tmpfile(int pen, int red, int green, int blue)
{
	PEN_N tp;
	PEN_C r, g, b;

	tp = (PEN_N) pen;
	r = (PEN_C) red;
	g = (PEN_C) green;
	b = (PEN_C) blue;

	if (record_off)		/* Wrong page!  */
		return;

	if (fwrite(&tp, sizeof(tp), 1, td) != 1) {
		PError("Pen_Color_to_tmpfile - pen");
		Eprintf("Error @ Cmd %ld\n", vec_cntr_w);
		exit(ERROR);
	}
	if (fwrite(&r, sizeof(r), 1, td) != 1) {
		PError("Pen_Color_to_tmpfile - red component");
		Eprintf("Error @ Cmd %ld\n", vec_cntr_w);
		exit(ERROR);
	}
	if (fwrite(&g, sizeof(g), 1, td) != 1) {
		PError("Pen_Color_to_tmpfile - green component");
		Eprintf("Error @ Cmd %ld\n", vec_cntr_w);
		exit(ERROR);
	}
	if (fwrite(&b, sizeof(b), 1, td) != 1) {
		PError("Pen_Color_to_tmpfile - blue component");
		Eprintf("Error @ Cmd %ld\n", vec_cntr_w);
		exit(ERROR);
	}
}

int load_pen_width_table(FILE * td)
{
	PEN_N tp;
	PEN_W tw;
	int i;

	if (fread((void *) &tp, sizeof(tp), 1, td) != 1) {
		return (0);
	}

	if (fread((void *) &tw, sizeof(tw), 1, td) != 1) {
		return (0);
	}

	if (tp >= NUMPENS) {	/* don't check < 0 - unsigned */
		return (1);
	}

	if (tp == 0) {		/* set all pens */
		for (i = 1; i < NUMPENS; ++i)
			pt.width[i] = tw;
	} else {
		pt.width[tp] = tw;	/* set just the specified one */
	}

	return (1);
}

int load_pen_color_table(FILE * td)
{
	PEN_N tp;
	PEN_C r, g, b;

	if (fread((void *) &tp, sizeof(tp), 1, td) != 1) {
		return (-1);
	}

	if (fread((void *) &r, sizeof(r), 1, td) != 1) {
		return (-1);
	}

	if (fread((void *) &g, sizeof(g), 1, td) != 1) {
		return (-1);
	}

	if (fread((void *) &b, sizeof(b), 1, td) != 1) {
		return (-1);
	}

	if (tp >= NUMPENS) {	/* don't check < 0 - unsigned */
		return (1);
	}

	set_color_rgb(tp, (BYTE) r, (BYTE) g, (BYTE) b);
	return ((int) tp);
}

void set_color_rgb(PEN_N index, BYTE r, BYTE g, BYTE b)
{
	pt.clut[index][0] = r;
	pt.clut[index][1] = g;
	pt.clut[index][2] = b;
	pt.color[index] = index;
}
