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
#include <assert.h>
void fill(HPGL_Pt polygon[], int numpoints, HPGL_Pt point1,
	  HPGL_Pt point2, int scale_flag, int filltype, float spacing,
	  float hatchangle,float curwidth, int curdpi, int nzfill)
{
	typedef struct {
		double x, y;
	} HPGL_Pt2;
	double pxmin, pxmax, pymin, pymax;
	double spxmin, spxmax, spymin, spymax;
	double polyxmin, polyymin, polyxmax, polyymax;
	double scanx1=0., scanx2=0., scany1=0., scany2=0.;
	HPGL_Pt2 segment[MAXPOLY], tmp;
	double iy[MAXPOLY];
	int endp[MAXPOLY];
	double segx, segy, segiy;
	static int i;		/* to please valgrind when debugging memory accesses */
	int j, k, jj, kk;
	int ib,pati;
	int endpoint;
#if 0
	int ia,l;
#endif	
	int numlines;
	double penwidth;
	int patj=0;
	HPGL_Pt p;
	double rot_ang;
	double pxdiff = 0., pydiff = 0.;
	double avx, avy, bvx, bvy, ax, ay, bx, by, atx, aty, btx, bty, mu;
				int hit=0;
				int miss=0;
int winding=0,oldwinding=-1;
#if 0
	double my_eps=1.e-15;
#else
	double my_eps=1.e-12;
#endif
	PEN_W SafePenW = curwidth;
	LineEnds SafeLineEnd = CurrentLineEnd;
	if (numpoints <3) {
	fprintf(stderr,"polygon with %d vertices ???\n",numpoints);
	return;
	}

	(void)point2; /* currently unused, P2 reference may be needed for correct placement of patterns */
	CurrentLineEnd = LAE_butt;
/*	penwidth = 0.1*100; */
        penwidth =  curdpi / (2.*25.4);
	PlotCmd_to_tmpfile(DEF_PW);
	Pen_Width_to_tmpfile(1, penwidth/10.);

	PlotCmd_to_tmpfile(DEF_LA);
	Line_Attr_to_tmpfile(LineAttrEnd, LAE_round);

	if (filltype > 2 && filltype <5)
		penwidth = spacing;

if (filltype == 11) penwidth=1.69;
if (filltype==10) penwidth= curdpi*curwidth;

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
/*fprintf(stderr,"vertical fill\n");*/
		goto FILL_VERT;
	}

	pxmin = MIN(polyxmin,point1.x) - 0.5;
	if (filltype >2)
	  pymin = MIN(polyymin,point1.y) - 0.5;
	else 
          pymin=polyymin-1.;  
	pxmax = polyxmax+1.;
	pymax = polyymax+1.;
	if (polyxmin == polyxmax && polyymin == polyymax) {
		fprintf(stderr, "zero area polygon\n");
		return;
	}

/*	PlotCmd_to_tmpfile(DEF_LA);
	Line_Attr_to_tmpfile(LineAttrEnd,LAE_butt);
*/
	pydiff = pymax - pymin;
	pxdiff = pxmax - pxmin;
	if (hatchangle != 0.) {
		rot_ang = tan(M_PI * hatchangle / 180.);
		pymin = pymin - rot_ang * pxdiff;
		pymax = pymax + rot_ang * pxdiff;
	}
	spymax=pymax;
	spymin=pymin;
	if (scale_flag) {
	p.x=0.;
	p.y=spymax;
	User_to_Plotter_coord(&p,&p);
	spymax=p.y;
	p.x=0.;
	p.y=spymin;
	User_to_Plotter_coord(&p,&p);
	spymin=p.y;
	penwidth *= (pymax-pymin)/(spymax-spymin); 
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
/* debug code to show outline - always done for solid fills to catch special case of protruding lines*/
if (filltype <3 ) {
		for (j = 0; j <= numpoints; j = j + 2) {	/*for all polygon edges */
			p.x = polygon[j].x;
			p.y = polygon[j].y;
			Pen_action_to_tmpfile(MOVE_TO, &p, scale_flag);
			p.x = polygon[j + 1].x;
			p.y = polygon[j + 1].y;
			Pen_action_to_tmpfile(DRAW_TO, &p, scale_flag);
		}
}
#endif

/* start at lowest y , run scanlines parallel x across polygon */
/* looking for intersections with edges */

	pydiff = 0.;

	if (hatchangle != 0.)
		pydiff = tan(M_PI * hatchangle / 180.) * pxdiff;
	for (i = 0; i <= numlines; i++) {	/* for all scanlines ... */
		k = -1;
                winding=0;
		scany1 = pymin + (double) i *penwidth;
		scany2 = scany1 + pydiff;
		if (scany1 <= pymin) continue;
		if (scany1 >= pymax) continue;
		if (scany2 < polyymin) continue;
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


			segx=0.;
			segy=0.;
/*determine coordinates of intersection */
			if (mu >= 0.0 && mu <= 1.01) {
				segx = bx + mu * bvx;	/*x coordinate of intersection */
				segy = by + mu * bvy;	/*y coordinate of intersection */
			} else
				continue;

			if ((segy <
			     MIN((double) polygon[j].y,
				 (double) polygon[j + 1].y) - my_eps)
			    || (segy >
				MAX((double) polygon[j].y,
				    (double) polygon[j + 1].y) +
				my_eps)
			    || (segx <
				MIN((double) polygon[j].x,
				    (double) polygon[j + 1].x) -
				my_eps)
			    || (segx >
				MAX((double) polygon[j].x,
				    (double) polygon[j + 1].x)
				    +my_eps)) {
				/* ignore intersections beyond endpoint of line */
			} else {
				segiy=0;
				endpoint=0;

				if (fabs (segx- polygon[j].x)<1.e-3 
				    && fabs( segy-polygon[j].y)<1.e-3) {
				    endpoint=1;
				    segiy=polygon[j+1].y;
				} else if (fabs(segx-polygon[j+1].x)<1.e-3 
				           && fabs(segy-polygon[j+1].y)<1.e-3) {
				           endpoint=1;
				            segiy=polygon[j].y; 
				}

                            if (nzfill==1) {
                              oldwinding=winding;
                              if ( polygon[j].y<polygon[j+1].y) 
                                winding++;
                              else
                                winding--;
/*fprintf(stderr,"winding %d nach %d\n",winding,oldwinding);*/
                              if (winding != 0 &&  oldwinding!=0 ) goto SKIP;
/*fprintf(stderr,"punkt %d\n",k+1);*/
                            } else {
				for (kk = 0; kk <= k; kk++) {
		/* if two intersections are identical (at same x), check if they are on the
		   same or on opposite sides of the scanline :
		   if on the same side, the scanline must be tangential to a
		   vertex, so count both intersections to avoid filling the 
		   outside regions. if both lines terminate on opposite sides
		   of the scanline, it enters or leaves the polygon at this 
		   vertex, so this counts as one intersection only */
					if ((fabs(segment[kk].x - segx) < 1.e-4)) 
 if  (endpoint==0|| endp[kk]==0 || ((segiy-scany1)*(iy[kk]-scany1)<=0.)) 
						goto SKIP;
				}
                            }
				k++;

				segment[k].x = segx;
				segment[k].y = segy;
				iy[k] = segiy;
				endp[k]=endpoint;

		if (k > 0) {
					for (jj = 0; jj < k; jj++) {
						if (segment[k].x <
						    segment[jj].x) {
							tmp = segment[jj];
							segment[jj] =
							    segment[k];
							segment[k] = tmp;
							segiy = iy[jj];
							iy[jj] =
							    iy[k];
							iy[k] = segiy;
							endpoint = endp[jj];
							endp[jj] =
							    endp[k];
							endp[k] = endpoint;
						}
					}
				}
			}	/* if crossing withing range */
		      SKIP:
			continue;
		}		/*next edge */
		if (k > 0) {
			for (j = 0; j < k; j = j + 2) {
				p.x = segment[j].x;
				p.y = segment[j].y;
				Pen_action_to_tmpfile(MOVE_TO, &p,
						      scale_flag);
				switch (filltype){
				case 11:
#if 0
				for(ia=0;ia<ph[pat];ia++){
				for(ib=0;ib<pw[pat];ib++)if (pattern [pat][ia][ib]>0) printf("o"); 
								else
								printf(" ");
							printf("\n");
							 }	
#endif
						pati=0;
						patj=i-(i/ph[pat])*ph[pat];
						if (patj>ph[pat]) patj=0;
					do  {
					if (pattern[pat][pati][patj]> 0) { 
					Pen_action_to_tmpfile(MOVE_TO,&p,scale_flag);
					p.x+=0.001;
					p.y+=0.001;
					Pen_action_to_tmpfile(DRAW_TO,&p,scale_flag);
					}
					pati++;
					if (pati >pw[pat])pati=0;
					p.x+= 1.68;
					} while (p.x < segment[j+1].x);
				break;
				
				case 10:
				hit=miss=0;
				ib=0;
				do {
				ib = 1 + (int) (100.0 *rand()/(RAND_MAX+1.0));
				if ( ib <  (int)spacing ) {
					hit++;
					Pen_action_to_tmpfile(MOVE_TO,&p,scale_flag);
					p.x+=0.001;
					p.y+=0.001;
					Pen_action_to_tmpfile(DRAW_TO,&p,scale_flag);	
					p.x-=0.001;
					p.y-=0.001;
					}else{
					miss++;
					}
					p.x +=penwidth;
				} while (p.x < segment[j+1].x);			

/*		fprintf(stderr,"scanline hits %d percentage %f (%f)\n",hit,(float)hit/(float)(hit+miss),spacing);*/

				break;

				default: 
				p.x = segment[j + 1].x;
				p.y = segment[j + 1].y;
				Pen_action_to_tmpfile(DRAW_TO, &p,
						      scale_flag);
				}
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
	spxmax=pxmax;
	spxmin=pxmin;
	if (scale_flag) {
	p.x=spxmax;
	p.y=0;
	User_to_Plotter_coord(&p,&p);
	spxmax=p.x;
	p.x=spxmin;
	p.y=0.;
	User_to_Plotter_coord(&p,&p);
	spxmin=p.x;
	penwidth *= (pxmax-pxmin)/(spxmax-spxmin); 
#if 0	
	fprintf(stderr,"scaled penwidth: %f\n",penwidth);
#endif
	}

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
		winding=0;
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
			     MIN(polygon[j].y, polygon[j + 1].y) - my_eps)
			    || (segy >
				MAX(polygon[j].y, polygon[j + 1].y) + my_eps)
			    || (segx < MIN(polygon[j].x, polygon[j + 1].x))
			    || (segx >
				MAX(polygon[j].x, polygon[j + 1].x))) {
/*fprintf(stderr,"intersection  at %f %f is not within (%f,%f)-(%f,%f)\n",segx,segy,polygon[j].x,polygon[j].y,polygon[j+1].x,polygon[j+1].y ) ; */
			} else {
				segiy=0;
				endpoint=0;

				if (fabs (segx- polygon[j].x)<1.e-3 
				    && fabs( segy-polygon[j].y)<1.e-3) {
				    endpoint=1;
				    segiy=polygon[j+1].x;
				} else if (fabs(segx-polygon[j+1].x)<1.e-3 
				           && fabs(segy-polygon[j+1].y)<1.e-3) {
				           endpoint=1;
				            segiy=polygon[j].x; 
				}
                        if (nzfill==1) {
                          oldwinding=winding;
                          if ( polygon[j].x<polygon[j+1].x) 
                            winding++;
                          else
                            winding--;
/*fprintf(stderr,"winding %d nach %d\n",winding,oldwinding);*/
                          if (winding != 0 &&  oldwinding!=0 ) goto SKIP;
/*fprintf(stderr,"punkt %d\n",k+1);*/
                        } else {

				for (kk = 0; kk <= k; kk++) {
		/* if two intersections are identical (at same x), check if they are on the
		   same or on opposite sides of the scanline :
		   if on the same side, the scanline must be tangential to a
		   vertex, so count both intersections to avoid filling the 
		   outside regions. if both lines terminate on opposite sides
		   of the scanline, it enters or leaves the polygon at this 
		   vertex, so this counts as one intersection only */
					if ((fabs(segment[kk].y - segy) < 1.e-4))
 if (endpoint==0 || endp[kk]==0 || ((segiy-scanx1)*(iy[kk]-scanx1)<=0.)) 
						goto SKIP2;
				}
			}	
				k++;
				segment[k].x = segx;
				segment[k].y = segy;
				iy[k] = segiy;
                                endp[k]=endpoint;
/*fprintf(stderr,"fill: intersection %d with line %d at (%f %f)\n",k,j,segx,segy);*/
		if (k > 0) {
					for (jj = 0; jj < k; jj++) {
						if (segment[k].y <
						    segment[jj].y) {
							tmp = segment[jj];
							segment[jj] =
							    segment[k];
							segment[k] = tmp;
							segiy = iy[jj];
							iy[jj] =
							    iy[k];
							iy[k] = segiy;
							endpoint = endp[jj];
							endp[jj] =
							    endp[k];
							endp[k] = endpoint;
						}
					}
				}	
			}	/* if crossing withing range */
		      SKIP2:
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
