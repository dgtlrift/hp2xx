/*
   Copyright (c) 2002 Andrew Bird  All rights reserved.
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
#include <math.h>

#include "bresnham.h"
#include "murphy.h"
#include "picbuf.h"
#include "hp2xx.h"
#include "hpgl.h"

static struct {
	PEN_C color;
	PicBuf *pb;
	int u, v;		/* delta x , delta y */
	int ku, kt, kv, kd;	/* loop constants */
	int oct2;
	int quad4;
	DevPt last1, last2, first1, first2, temp;
} murphy;


/*#i fdef NORINT
#de fine lrint(a) ((long)(a+0.5))
#en dif
*/

#define my_lrint(a) ((long)(a+0.5))

static void do_miter(int, DevPt, DevPt, DevPt, DevPt);

void murphy_init(PicBuf * pb, PEN_C color)
{
	murphy.color = color;	/* setup buffer / color info */
	murphy.pb = pb;
}

void murphy_paraline(DevPt pt, int d1)
{				/* implements Figure 5B */
	int p;			/* pel counter, p=along line */
	d1 = -d1;

	for (p = 0; p <= murphy.u; p++) {	/* test for end of parallel line */

		plot_PicBuf(murphy.pb, &pt, murphy.color);

		if (d1 <= murphy.kt) {	/* square move */
			if (murphy.oct2 == 0) {
				pt.x++;
			} else {
				if (murphy.quad4 == 0) {
					pt.y++;
				} else {
					pt.y--;
				}
			}
			d1 += murphy.kv;
		} else {	/* diagonal move */
			pt.x++;
			if (murphy.quad4 == 0) {
				pt.y++;
			} else {
				pt.y--;
			}
			d1 += murphy.kd;
		}
	}
	murphy.temp = pt;
}

void murphy_wideline(DevPt p0, DevPt p1, int width, int miter)
{				/* implements figure 5A - draws lines parallel to ideal line */

	float offset = width / 2.;

	DevPt pt, ptx, ml1, ml2, ml1b, ml2b;

	int d0, d1;		/* difference terms d0=perpendicular to line, d1=along line */

	int q;			/* pel counter,q=perpendicular to line */
	int tmp;

	int dd;			/* distance along line */
	int tk;			/* thickness threshold */
	double ang;		/* angle for initial point calculation */
	/* Initialisation */
	murphy.u = p1.x - p0.x;	/* delta x */
	murphy.v = p1.y - p0.y;	/* delta y */

	if (murphy.u < 0) {	/* swap to make sure we are in quadrants 1 or 4 */
		pt = p0;
		p0 = p1;
		p1 = pt;
		murphy.u *= -1;
		murphy.v *= -1;
	}

	if (murphy.v < 0) {	/* swap to 1st quadrant and flag */
		murphy.v *= -1;
		murphy.quad4 = 1;
	} else {
		murphy.quad4 = 0;
	}

	if (murphy.v > murphy.u) {	/* swap things if in 2 octant */
		tmp = murphy.u;
		murphy.u = murphy.v;
		murphy.v = tmp;
		murphy.oct2 = 1;
	} else {
		murphy.oct2 = 0;
	}

	murphy.ku = murphy.u + murphy.u;	/* change in l for square shift */
	murphy.kv = murphy.v + murphy.v;	/* change in d for square shift */
	murphy.kd = murphy.kv - murphy.ku;	/* change in d for diagonal shift */
	murphy.kt = murphy.u - murphy.kv;	/* diag/square decision threshold */

	d0 = 0;
	d1 = 0;
	dd = 0;

	ang = atan((double) murphy.v / (double) murphy.u);	/* calc new initial point - offset both sides of ideal */

	if (murphy.oct2 == 0) {
		pt.x = p0.x + my_lrint(offset * sin(ang));
		if (murphy.quad4 == 0) {
			pt.y = p0.y - my_lrint(offset * cos(ang));
		} else {
			pt.y = p0.y + my_lrint(offset * cos(ang));
		}
	} else {
		pt.x = p0.x - my_lrint(offset * cos(ang));
		if (murphy.quad4 == 0) {
			pt.y = p0.y + my_lrint(offset * sin(ang));
		} else {
			pt.y = p0.y - my_lrint(offset * sin(ang));
		}
	}

	tk = (int) (4. * HYPOT(pt.x - p0.x, pt.y - p0.y) * HYPOT(murphy.u, murphy.v));	/* used here for constant thickness line */

	if (miter == 0) {
		murphy.first1.x = -10000000;
		murphy.first1.y = -10000000;
		murphy.first2.x = -10000000;
		murphy.first2.y = -10000000;
		murphy.last1.x = -10000000;
		murphy.last1.y = -10000000;
		murphy.last2.x = -10000000;
		murphy.last2.y = -10000000;
	}
	ptx = pt;

	for (q = 0; dd <= tk; q++) {	/* outer loop, stepping perpendicular to line */

		murphy_paraline(pt, d1);	/* call to inner loop - right edge */
		if (q == 0) {
			ml1 = pt;
			ml1b = murphy.temp;
		} else {
			ml2 = pt;
			ml2b = murphy.temp;
		}
		if (d0 < murphy.kt) {	/* square move  - M2 */
			if (murphy.oct2 == 0) {
				if (murphy.quad4 == 0) {
					pt.y++;
				} else {
					pt.y--;
				}
			} else {
				pt.x++;
			}
		} else {	/* diagonal move */
			dd += murphy.kv;
			d0 -= murphy.ku;
			if (d1 < murphy.kt) {	/* normal diagonal - M3 */
				if (murphy.oct2 == 0) {
					pt.x--;
					if (murphy.quad4 == 0) {
						pt.y++;
					} else {
						pt.y--;
					}
				} else {
					pt.x++;
					if (murphy.quad4 == 0) {
						pt.y--;
					} else {
						pt.y++;
					}
				}
				d1 += murphy.kv;
			} else {	/* double square move, extra parallel line */
				if (murphy.oct2 == 0) {
					pt.x--;
				} else {
					if (murphy.quad4 == 0) {
						pt.y--;
					} else {
						pt.y++;
					}
				}
				d1 += murphy.kd;
				if (dd > tk) {
					do_miter(miter, ml1b, ml2b, ml1,
						 ml2);
					return;	/* breakout on the extra line */
				}
				murphy_paraline(pt, d1);
				if (murphy.oct2 == 0) {
					if (murphy.quad4 == 0) {
						pt.y++;
					} else {

						pt.y--;
					}
				} else {
					pt.x++;
				}
			}
		}
		dd += murphy.ku;
		d0 += murphy.kv;
	}

	do_miter(miter, ml1b, ml2b, ml1, ml2);

}

static void do_miter(miter, ml1b, ml2b, ml1, ml2)
int miter;
DevPt ml1b, ml2b, ml1, ml2;

{
	int ftmp1, ftmp2;
	DevPt m1, m2, *p_act;
	DevPt fi, la, cur;

	if (miter > 1) {
		if (murphy.first1.x != -10000000) {
			fi.x = (murphy.first1.x + murphy.first2.x) / 2;
			fi.y = (murphy.first1.y + murphy.first2.y) / 2;
			la.x = (murphy.last1.x + murphy.last2.x) / 2;
			la.y = (murphy.last1.y + murphy.last2.y) / 2;
			cur.x = (ml1.x + ml2.x) / 2;
			cur.y = (ml1.y + ml2.y) / 2;
			ftmp1 =
			    (fi.x - cur.x) * (fi.x - cur.x) + (fi.y -
							       cur.y) *
			    (fi.y - cur.y);
			ftmp2 =
			    (la.x - cur.x) * (la.x - cur.x) + (la.y -
							       cur.y) *
			    (la.y - cur.y);
			if (ftmp1 <= ftmp2) {
				m1 = murphy.first1;
				m2 = murphy.first2;
			} else {
				m1 = murphy.last1;
				m2 = murphy.last2;
			}
			ftmp2 =
			    (m2.x - ml2b.x) * (m2.x - ml2b.x) + (m2.y -
								 ml2b.y) *
			    (m2.y - ml2b.y);
			ftmp1 =
			    (m2.x - ml2.x) * (m2.x - ml2.x) + (m2.y -
							       ml2.y) *
			    (m2.y - ml2.y);

			if (abs(ftmp2) >= abs(ftmp1)) {
				ftmp1 = ml2b.x;
				ftmp2 = ml2b.y;
				ml2b.x = ml2.x;
				ml2b.y = ml2.y;
				ml2.x = ftmp1;
				ml2.y = ftmp2;
				ftmp1 = ml1b.x;
				ftmp2 = ml1b.y;
				ml1b.x = ml1.x;
				ml1b.y = ml1.y;
				ml1.x = ftmp1;
				ml1.y = ftmp2;
			}

/*draw outline of miter segment */
			p_act = bresenham_init(&m2, &m1);
			do {
				plot_PicBuf(murphy.pb, p_act,
					    murphy.color);
			} while (bresenham_next() != BRESENHAM_ERR);

			p_act = bresenham_init(&m1, &ml1b);
			do {
				plot_PicBuf(murphy.pb, p_act,
					    murphy.color);
			} while (bresenham_next() != BRESENHAM_ERR);

			p_act = bresenham_init(&ml1b, &ml2b);
			do {
				plot_PicBuf(murphy.pb, p_act,
					    murphy.color);
			} while (bresenham_next() != BRESENHAM_ERR);

			p_act = bresenham_init(&ml2b, &m2);
			do {
				plot_PicBuf(murphy.pb, p_act,
					    murphy.color);
			} while (bresenham_next() != BRESENHAM_ERR);
			polygon_PicBuf(m1, m2, ml1b, ml2b, murphy.color,
				       murphy.pb);
		}
	}
	murphy.last2 = ml2;
	murphy.last1 = ml1;
	murphy.first1 = ml1b;
	murphy.first2 = ml2b;
}
