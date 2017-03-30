/*
   Copyright (c) 1991 - 1994 Heinz W. Werntges.  All rights reserved.
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

/** bresnham.c: Implementation of Bresenham's algorithm
 **
 ** 1991/01/04  V 1.00  HWW  Due to pseudocode in D.F. Rogers (1986) McGraw Hill
 ** 1991/10/15  V 1.01  HWW  ANSI_C
 ** 2002/04/28	V 1.02  AJB  Move static vars into struct
 **/

#define	TEST	0



#include <stdio.h>
#include <stdlib.h>
#include "bresnham.h"

static struct {
	DevPt p_act;
	int dx, dy, s1, s2, swapdir, err, count;
} bres;

DevPt *bresenham_init(DevPt * pp1, DevPt * pp2)
/**
 ** Init. generation of a straight line between *pp1 & *pp2
 **
 ** Returns pointer to internally generated actual point of line.
 ** Use this ptr for further reference! Example of use:
 **
 **	..
 **	#include <share/bresnham.h>
 **	...
 **	DevPt	p1, p2, *pp;
 **	...
 **	pp = bresenham_init (&p1, &p2);
 **	do {
 **		plot (pp);
 **	} while (bresenham_next() != BRESENHAM_ERR);
 **/
{
	bres.p_act = *pp1;

	if ((bres.dx = pp2->x - pp1->x) != 0) {
		if (bres.dx < 0) {
			bres.dx = -bres.dx;
			bres.s1 = -1;
		} else
			bres.s1 = 1;
	} else
		bres.s1 = 0;	/* dx = abs(x2-x1), s1 = sign(x2-x1)    */

	if ((bres.dy = pp2->y - pp1->y) != 0) {
		if (bres.dy < 0) {
			bres.dy = -bres.dy;
			bres.s2 = -1;
		} else
			bres.s2 = 1;
	} else
		bres.s2 = 0;	/* dy = abs(y2-y1), s2 = sign(y2-y1)    */

	if (bres.dy > bres.dx) {
		bres.swapdir = bres.dx;	/* use swapdir as temp. var.    */
		bres.dx = bres.dy;
		bres.dy = bres.swapdir;
		bres.swapdir = 1;
	} else
		bres.swapdir = 0;

	bres.count = bres.dx;	/* Init. of loop cnt    */
	bres.dy <<= 1;
	bres.err = bres.dy - bres.dx;	/* Init. of error term  */
	bres.dx <<= 1;

	return &bres.p_act;
}



int bresenham_next(void)
/**
 ** Move actual point to next position (if possible)
 **
 ** Returns 0		 if ok,
 **	   BRESENHAM_EOL if last point reached (p_act == *pp2),
 **	   BRESENHAM_ERR else (e.g. if moving past EOL attempted)
 **/
{
	if (bres.count <= 0)
		return (BRESENHAM_ERR);	/* Beyond last point! */

	while (bres.err >= 0) {
		if (bres.swapdir)
			bres.p_act.x += bres.s1;
		else
			bres.p_act.y += bres.s2;
		bres.err -= bres.dx;
	}
	if (bres.swapdir)
		bres.p_act.y += bres.s2;
	else
		bres.p_act.x += bres.s1;
	bres.err += bres.dy;

	bres.count--;		/* i==0 indicates "last point reached"  */
	return ((bres.count) ? 0 : BRESENHAM_EOL);
}

	/* Test module */
#if TEST
#ifdef __TURBOC__ && __MSDOS__


#include <graphics.h>


void b_line(DevPt * pp1, DevPt * pp2, int col)
{
	DevPt *pp;

	pp = bresenham_init(pp1, pp2);
	do {
		putpixel(pp->x, pp->y, col);
	} while (bresenham_next() != BRESENHAM_ERR);
}


void main(void)
{
	int gdriver = DETECT, gmode, MaxX, MaxY;
	DevPt pa, pc;

	initgraph(&gdriver, &gmode, "");

	MaxX = getmaxx();
	MaxY = getmaxy();

	pc.x = (MaxX + 1) >> 1;
	pc.y = (MaxY + 1) >> 1;

	for (pa.x = 0, pa.y = 0; pa.x < MaxX; pa.x += 4)
		b_line(&pc, &pa, RED);
	for (pa.x = MaxX, pa.y = 0; pa.y < MaxY; pa.y += 3)
		b_line(&pc, &pa, GREEN);

	for (pa.x = MaxX, pa.y = MaxY; pa.x >= 0; pa.x -= 4)
		b_line(&pc, &pa, LIGHTRED);
	for (pa.x = 0, pa.y = MaxY; pa.y >= 0; pa.y -= 3)
		b_line(&pc, &pa, LIGHTGREEN);

	getchar();
}

#endif
#endif
