#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "bresnham.h"
#include "hp2xx.h"
#include "hpgl.h"
#include "lindef.h"
#include "pendef.h"

void fill(HPGL_Pt polygon[], int numpoints, HPGL_Pt point1,
	  HPGL_Pt point2, int scale_flag, int filltype, float spacing,
	  float hatchangle)
{
	typedef struct {
		double x, y;
	} HPGL_Pt2;
	double pxmin, pxmax, pymin, pymax;
	double polyxmin, polyymin, polyxmax, polyymax;
	double scanx1, scanx2, scany1, scany2;
	HPGL_Pt2 segment[MAXPOLY], tmp;
	double segx, segy;
	static int i;		/* to please valgrind when debugging memory accesses */
	int j, k, jj, kk;
	int numlines;
	double penwidth;
	HPGL_Pt p;
	double rot_ang;
	double pxdiff = 0., pydiff = 0.;
	double avx, avy, bvx, bvy, ax, ay, bx, by, atx, aty, btx, bty, mu;
	PEN_W SafePenW = pt.width[1];
	LineEnds SafeLineEnd = CurrentLineEnd;
	CurrentLineEnd = LAE_butt;

	penwidth = 0.1;

	PlotCmd_to_tmpfile(DEF_PW);
	Pen_Width_to_tmpfile(1, penwidth);

	PlotCmd_to_tmpfile(DEF_LA);
	Line_Attr_to_tmpfile(LineAttrEnd, LAE_round);

	if (filltype > 2)
		penwidth = spacing;


	polyxmin = 100000.;
	polyymin = 100000.;
	polyxmax = -100000.;
	polyymax = -100000.;
	for (i = 0; i <= numpoints; i++) {

		polyxmin = MIN(polyxmin, polygon[i].x);
		polyymin = MIN(polyymin, polygon[i].y);

		polyxmax = MAX(polyxmax, polygon[i].x);
		polyymax = MAX(polyymax, polygon[i].y);
	}

	if (hatchangle > 89.9 && hatchangle < 180.) {
		hatchangle = hatchangle - 90.;
		goto FILL_VERT;
	}

	pxmin = point1.x - 0.5;
	pymin = point1.y - 0.5;
	pxmax = polyxmax;
	pymax = polyymax;
	
	if (polyxmin == polyxmax && polyymin == polyymax) {
		fprintf(stderr, "zero area polygon\n");
		return;
	}

	pydiff = pymax - pymin;
	pxdiff = pxmax - pxmin;
	if (hatchangle != 0.) {
		rot_ang = tan(M_PI * hatchangle / 180.);
		pymin = pymin - rot_ang * pxdiff;
		pymax = pymax + rot_ang * pxdiff;
	}
	numlines = (int) fabs(1. + (pymax - pymin + penwidth) / penwidth);
#if 0
/* debug code to show shade box */
	p.x = pxmin;
	p.y = pymin;
	Pen_action_to_tmpfile(MOVE_TO, &p, scale_flag);
	p.x = pxmin;
	p.y = pymax;
	Pen_action_to_tmpfile(DRAW_TO, &p, scale_flag);
	p.x = pxmax;
	p.y = pymax;
	Pen_action_to_tmpfile(DRAW_TO, &p, scale_flag);
	p.x = pxmax;
	p.y = pymin;
	Pen_action_to_tmpfile(DRAW_TO, &p, scale_flag);
	p.x = pxmin;
	p.y = pymin;
	Pen_action_to_tmpfile(DRAW_TO, &p, scale_flag);
#endif
#if 0
/* debug code to show outline */
		for (j=0;j<=numpoints;j=j+2){
			p.x = polygon[j].x;
			p.y = polygon[j].y;
			Pen_action_to_tmpfile(MOVE_TO, &p, scale_flag);
			p.x = polygon[j + 1].x;
			p.y = polygon[j + 1].y;
			Pen_action_to_tmpfile(DRAW_TO, &p, scale_flag);
		}
return;
#endif

/* start at lowest y , run scanlines parallel x across polygon */
/* looking for intersections with edges */

	pydiff = 0.;

	if (hatchangle != 0.)
		pydiff = tan(M_PI * hatchangle / 180.) * pxdiff;
	for (i = 0; i <= numlines; i++) {	/* for all scanlines ... */
		k = -1;
		scany1 = pymin + (double) i *penwidth;
		scany2 = scany1 + pydiff;
		if (scany1 >= pymax || scany1 <= pymin) {
/*fprintf(stderr,"zu weit\n");*/
			continue;
		}
		if (scany2 < polyymin)
			continue;
/* coefficients for current scan line */
		bx = pxmin;
		btx = pxmax;
		by = scany1;
		bty = scany2;
		bvx = btx - bx;
		bvy = bty - by;

		for (j = 0; j <= numpoints; j = j + 2) {	/*for all polygon edges */
			ax = polygon[j].x;
			ay = polygon[j].y;
			atx = polygon[j + 1].x;
			aty = polygon[j + 1].y;
			avx = atx - ax;
			avy = aty - ay;

			if (fabs(bvy * avx - avy * bvx) < 1.e-8)
				continue;
			mu = (avx * (ay - by) +
			      avy * (bx - ax)) / (bvy * avx - avy * bvx);



/*determine coordinates of intersection */
			if (mu >= 0. && mu <= 1.01) {
				segx = bx + mu * bvx;	/*x coordinate of intersection */
				segy = by + mu * bvy;	/*y coordinate of intersection */
			} else
				continue;

			if ((segy <
			     MIN((double) polygon[j].y,
				 (double) polygon[j + 1].y) - 0.0000001)
			    || (segy >
				MAX((double) polygon[j].y,
				    (double) polygon[j + 1].y) +
				0.000000001)
			    || (segx <
				MIN((double) polygon[j].x,
				    (double) polygon[j + 1].x) -
				0.000000001)
			    || (segx >
				MAX((double) polygon[j].x,
				    (double) polygon[j + 1].x))) {
/*fprintf(stderr,"intersection  at %f %f is not within (%f,%f)-(%f,%f)\n",segx,segy,polygon[j].x,polygon[j].y,polygon[j+1].x,polygon[j+1].y ) ; */
			} else {
				for (kk = 0; kk <= k; kk++) {
					if (fabs(segment[kk].x - segx) <
					    1.e-8)
						goto BARF;
				}
				k++;
				segment[k].x = segx;
				segment[k].y = segy;

/*fprintf(stderr,"fill: intersection %d with line %d at (%f %f)\n",k,j,segx,segy);*/
				if (k > 0) {
					for (jj = 0; jj < k; jj++) {
						if (segment[k].x <
						    segment[jj].x) {
							tmp = segment[jj];
							segment[jj] =
							    segment[k];
							segment[k] = tmp;
						}
					}
				}	/* if not the first intersection */
			}	/* if crossing withing range */
		      BARF:
			continue;
		}		/*next edge */

		if (k > 0) {
/*fprintf(stderr, "%d segments for scanline %d\n",k,i);*/
			for (j = 0; j < k; j = j + 2) {
/*fprintf(stderr, "segment (%f,%f)-(%f,%f)\n",segment[j].x,segment[j].y,segment[j+1].x,segment[j+1].y);*/
				p.x = segment[j].x;
				p.y = segment[j].y;
				Pen_action_to_tmpfile(MOVE_TO, &p,
						      scale_flag);
				p.x = segment[j + 1].x;
				p.y = segment[j + 1].y;
				Pen_action_to_tmpfile(DRAW_TO, &p,
						      scale_flag);
			}
		} else {
#if 0
/* debug code to show scanlines :*/
			p.x = pxmin;
			p.y = scany1;
			Pen_action_to_tmpfile(MOVE_TO, &p, scale_flag);
			p.x = pxmax;
			p.y = scany2;
			Pen_action_to_tmpfile(DRAW_TO, &p, scale_flag);
#endif
		}

	}			/* next scanline */

	if (filltype != 4) {
		CurrentLineEnd = SafeLineEnd;
		PlotCmd_to_tmpfile(DEF_PW);
		Pen_Width_to_tmpfile(1, SafePenW);
		PlotCmd_to_tmpfile(DEF_LA);
		Line_Attr_to_tmpfile(LineAttrEnd, SafeLineEnd);
		return;
	}

      FILL_VERT:

	pxmin = point1.x;
	pymin = point1.y;
	pxmax = polyxmax;
	pymax = polyymax;

	pydiff = pymax - pymin;
	if (hatchangle != 0.) {
		rot_ang = tan(M_PI * hatchangle / 180.);
		pxmin = pxmin - rot_ang * pydiff;
		pxmax = pxmax + rot_ang * pydiff;
	}

	pymin = pymin - 1.;
	pymax = pymax + 1.;

	PlotCmd_to_tmpfile(DEF_LA);
	Line_Attr_to_tmpfile(LineAttrEnd, LAE_butt);

	numlines = (int) fabs(1. + (pxmax - pxmin + penwidth) / penwidth);

/*fprintf(stderr,"numlines = %d\n",numlines);*/
#if 0
/* debug code to show shade box */
	p.x = pxmin;
	p.y = pymin;
	Pen_action_to_tmpfile(MOVE_TO, &p, scale_flag);
	p.x = pxmin;
	p.y = pymax;
	Pen_action_to_tmpfile(DRAW_TO, &p, scale_flag);
	p.x = pxmax;
	p.y = pymax;
	Pen_action_to_tmpfile(DRAW_TO, &p, scale_flag);
	p.x = pxmax;
	p.y = pymin;
	Pen_action_to_tmpfile(DRAW_TO, &p, scale_flag);
	p.x = pxmin;
	p.y = pymin;
	Pen_action_to_tmpfile(DRAW_TO, &p, scale_flag);
#endif

	pxdiff = 0.;
	if (hatchangle != 0.)
		pxdiff = tan(M_PI * hatchangle / 180.) * (pymax - pymin);
	for (i = 0; i <= numlines; ++i) {	/* for all scanlines ... */
		k = -1;
		scanx1 = pxmin + (double) i *penwidth;
		if (scanx1 >= pxmax || scanx1 <= pxmin)
			continue;
		scanx2 = scanx1 - pxdiff;
/*		if (scanx2 < polyxmin)
			continue;*/
/* coefficients for current scan line */
		bx = scanx1;
		btx = scanx2;
		by = pymin;
		bty = pymax;
		bvx = btx - bx;
		bvy = bty - by;

		for (j = 0; j <= numpoints; j = j + 2) {	/*for all polygon edges */
			ax = polygon[j].x;
			ay = polygon[j].y;
			atx = polygon[j + 1].x;
			aty = polygon[j + 1].y;
			avx = atx - ax;
			avy = aty - ay;

			if (fabs(bvy * avx - avy * bvx) < 1.e-8)
				continue;
			mu = (avx * (ay - by) +
			      avy * (bx - ax)) / (bvy * avx - avy * bvx);


#if 0
/* debug code to show outline */
			p.x = polygon[j].x;
			p.y = polygon[j].y;
			Pen_action_to_tmpfile(MOVE_TO, &p, scale_flag);
			p.x = polygon[j + 1].x;
			p.y = polygon[j + 1].y;
			Pen_action_to_tmpfile(DRAW_TO, &p, scale_flag);
#endif

/*determine coordinates of intersection */
			if (mu >= 0. && mu <= 1.01) {
				segx = bx + mu * bvx;	/*x coordinate of intersection */
				segy = by + mu * bvy;	/*y coordinate of intersection */
			} else
				continue;


			if ((segy <
			     MIN(polygon[j].y, polygon[j + 1].y) - 1.)
			    || (segy >
				MAX(polygon[j].y, polygon[j + 1].y) + 1.)
			    || (segx < MIN(polygon[j].x, polygon[j + 1].x))
			    || (segx >
				MAX(polygon[j].x, polygon[j + 1].x))) {
/*fprintf(stderr,"intersection  at %f %f is not within (%f,%f)-(%f,%f)\n",segx,segy,polygon[j].x,polygon[j].y,polygon[j+1].x,polygon[j+1].y ) ; */
			} else {
				for (kk = 0; kk <= k; kk++) {
					if (fabs(segment[kk].y - segy) <
					    1.e-8)
						goto BARF2;
				}
				k++;
				segment[k].x = segx;
				segment[k].y = segy;

/*fprintf(stderr,"fill: intersection %d with line %d at (%f %f)\n",k,j,segx,segy);*/
				if (k > 0) {
					for (jj = 0; jj < k; jj++) {
						if (segment[k].y <
						    segment[jj].y) {
							tmp = segment[jj];
							segment[jj] =
							    segment[k];
							segment[k] = tmp;
						}
					}
				}	/* if not the first intersection */
			}	/* if crossing withing range */
		      BARF2:
			continue;
		}		/*next edge */


		if (k > 0) {
/* fprintf(stderr, "%d segments for scanline %d\n",k,i);*/
			for (j = 0; j < k; j = j + 2) {
/*fprintf(stderr, "segment (%f,%f)-(%f,%f)\n",segment[j].x,segment[j].y,segment[j+1].x,segment[j+1].y);*/
				p.x = segment[j].x;
				p.y = segment[j].y;
				Pen_action_to_tmpfile(MOVE_TO, &p,
						      scale_flag);
				p.x = segment[j + 1].x;
				p.y = segment[j + 1].y;
				Pen_action_to_tmpfile(DRAW_TO, &p,
						      scale_flag);
			}
		} else {
#if 0
/* debug code to show scanlines :*/
			p.x = scanx1;
			p.y = pymin;
			Pen_action_to_tmpfile(MOVE_TO, &p, scale_flag);
			p.x = scanx2;
			p.y = pymax;
			Pen_action_to_tmpfile(DRAW_TO, &p, scale_flag);
#endif
		}

	}			/* next scanline */
	CurrentLineEnd = SafeLineEnd;
	PlotCmd_to_tmpfile(DEF_PW);
	Pen_Width_to_tmpfile(1, SafePenW);
	PlotCmd_to_tmpfile(DEF_LA);
	Line_Attr_to_tmpfile(LineAttrEnd, SafeLineEnd);

}
