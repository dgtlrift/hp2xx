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
   int color;
   PicBuf *pb;
   int u,v;                       /* delta x , delta y */
   int ku,kt,kv,kd;               /* loop constants */
   int oct2;
   int quad4;
	DevPt last1,last2,first1,first2,temp;
} murphy;


#ifdef NORINT 
#define lrint(a) ((long)(a+0.5))
#endif

void do_miter(int,DevPt,DevPt,DevPt,DevPt);
void polygon_PicBuf(DevPt,DevPt,DevPt,DevPt);

void murphy_init(PicBuf *pb,int color) {
   murphy.color=color;            /* setup buffer / color info */
   murphy.pb=pb;
}

void murphy_paraline(DevPt pt,int d1) { /* implements Figure 5B */
   int p;                         /* pel counter, p=along line */
   d1 = -d1;

   for (p = 0; p <= murphy.u; p++) {   /* test for end of parallel line */
      plot_PicBuf(murphy.pb,&pt,murphy.color);
      if (d1 <= murphy.kt) {           /* square move */
         if(murphy.oct2 == 0) {
            pt.x++;
         } else {
            if(murphy.quad4 == 0) {
               pt.y++;
            } else {
               pt.y--;
            }
         }
         d1 += murphy.kv;
      } else {                         /* diagonal move */
         pt.x++;
         if(murphy.quad4 == 0) {
            pt.y++;
         } else {
            pt.y--;
         }
         d1 += murphy.kd;
      }
   }
   murphy.temp=pt;
}

void murphy_wideline(DevPt p0, DevPt p1, int width) {  /* implements figure 5A - draws lines parallel to ideal line */

   float offset = width/2;

   DevPt pt,ptx,ml1,ml2,ml1b,ml2b;
int dist1,dist2,miter;

   int d0, d1;                    /* difference terms d0=perpendicular to line, d1=along line */

   int q;                       /* pel counter,q=perpendicular to line */
   int tmp;

   int dd;                      /* distance along line */
   double tk;                   /* thickness threshold */
   double ang;                  /* angle for initial point calculation */
   /* Initialisation */
   murphy.u = p1.x - p0.x;             /* delta x */
   murphy.v = p1.y - p0.y;             /* delta y */

   if(murphy.u < 0) {                  /* swap to make sure we are in quadrants 1 or 4 */
      pt=p0;
      p0=p1;
      p1=pt;
      murphy.u *= -1;
      murphy.v *= -1;
   }

   if(murphy.v < 0) {                                /* swap to 1st quadrant and flag */
      murphy.v *= -1;
      murphy.quad4 = 1;
   } else {
      murphy.quad4 = 0;
   }

   if(murphy.v > murphy.u) {                         /* swap things if in 2 octant */
      tmp = murphy.u;
      murphy.u = murphy.v;
      murphy.v = tmp;
      murphy.oct2 = 1;
   } else {
      murphy.oct2 = 0;
   }
   
   murphy.ku = murphy.u  + murphy.u;                  /* change in l for square shift */
   murphy.kv = murphy.v  + murphy.v;                  /* change in d for square shift */
   murphy.kd = murphy.kv - murphy.ku;                 /* change in d for diagonal shift */
   murphy.kt = murphy.u  - murphy.kv;                 /* diag/square decision threshold */

   d0 = 0;
   d1 = 0;
   dd = 0;

   ang=atan((double) murphy.v / (double) murphy.u);         /* calc new initial point - offset both sides of ideal */

   if(murphy.oct2 == 0) {
      pt.x = p0.x + lrint(offset * sin(ang));
      if(murphy.quad4 == 0) {
         pt.y = p0.y - lrint(offset * cos(ang));
      } else {
         pt.y = p0.y + lrint(offset * cos(ang));
      }
   } else {
      pt.x = p0.x - lrint(offset * cos(ang));
      if(murphy.quad4 == 0) {
         pt.y = p0.y + lrint(offset * sin(ang));
      } else {
         pt.y = p0.y - lrint(offset * sin(ang));
      }
   }

   tk = 4 * HYPOT(pt.x-p0.x,pt.y-p0.y) * HYPOT(murphy.u, murphy.v); /* used here for constant thickness line */

	dist1=(pt.x-murphy.last1.x)*(pt.x-murphy.last1.x)+(pt.y-murphy.last1.y)*(pt.y-murphy.last1.y);
	dist2=(pt.x-murphy.last2.x)*(pt.x-murphy.last2.x)+(pt.y-murphy.last2.y)*(pt.y-murphy.last2.y);

	if(dist1>dist2){
	ptx=murphy.last1;
	murphy.last1=murphy.last2;
	murphy.last2=ptx;
	}

	miter=0;
	if (MAX(dist1,dist2)<= width*width*4) miter=1;
	ptx=pt;

   for (q = 0; dd <= tk; q++) {  /* outer loop, stepping perpendicular to line */

      murphy_paraline(pt,d1);         /* call to inner loop - right edge */
	if (q==0) {ml1=pt;ml1b=murphy.temp;}
		else
		  {ml2=pt;ml2b=murphy.temp;}
      if (d0 < murphy.kt) {            /* square move  - M2 */
         if(murphy.oct2 == 0) {
            if(murphy.quad4 == 0) {
               pt.y++;
            } else {
               pt.y--;
            }
         } else {
            pt.x++;
         }
      } else {                  /* diagonal move */
         dd += murphy.kv;
         d0 -= murphy.ku;
         if (d1 < murphy.kt) {         /* normal diagonal - M3 */
            if(murphy.oct2 == 0) {
               pt.x--;
               if(murphy.quad4 == 0) {
                  pt.y++;
               } else {
                  pt.y--;
               }
            } else {
               pt.x++;
               if(murphy.quad4 == 0) {
                  pt.y--;
               } else {
                  pt.y++;
               }
            }
            d1 += murphy.kv;
         } else {               /* double square move, extra parallel line */
            if(murphy.oct2 == 0) {
               pt.x--;
            } else {
               if(murphy.quad4 == 0) {
                  pt.y--;
               } else {
                  pt.y++;
               }
            }
            d1 += murphy.kd;
            if (dd > tk){
		 do_miter(miter,ml1b,ml2b,ml1,ml2);
               return;          /* breakout on the extra line */
            }
            murphy_paraline(pt,d1);
            if(murphy.oct2 == 0) {
               if(murphy.quad4 == 0) {
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
if ((murphy.last1.x+murphy.last2.x)/2 - p0.x > (murphy.first1.x+murphy.first2.x)/2-p0.x){
ml1b=ml1;
ml2b=ml2;
}
 
 do_miter(miter,ml1b,ml2b,ml1,ml2);
 
 
}

void do_miter(miter,ml1b,ml2b,ml1,ml2)
int miter;
DevPt ml1b,ml2b,ml1,ml2;

{
int ftmp1,ftmp2;
DevPt m1,m2,*p_act;


if (miter==1){
ftmp2=(murphy.last2.x-ml2b.x)*(murphy.last2.x-ml2b.x)+(murphy.last2.y-ml2b.y)*(murphy.last2.y-ml2b.y);
ftmp1=(murphy.first2.x-ml2b.x)*(murphy.first2.x-ml2b.x)+(murphy.first2.y-ml2b.y)*(murphy.first2.y-ml2b.y);
	if (fabs(ftmp2)<fabs(ftmp1))
	m2=murphy.last2;
	else 
	m2=murphy.first2;
ftmp2=(murphy.last1.x-ml1b.x)*(murphy.last1.x-ml1b.x)+(murphy.last1.y-ml1b.y)*(murphy.last1.y-ml1b.y);
ftmp1=(murphy.first1.x-ml1b.x)*(murphy.first1.x-ml1b.x)+(murphy.first1.y-ml1b.y)*(murphy.first1.y-ml1b.y);
	if (fabs(ftmp2)<fabs(ftmp1))
	m1=murphy.last1;
	else 
	m1=murphy.first1;

ftmp2=(m2.x-ml2b.x)*(m2.x-ml2b.x)+(m2.y-ml2b.y)*(m2.y-ml2b.y);
ftmp1=(m2.x-ml2.x)*(m2.x-ml2.x)+(m2.y-ml2.y)*(m2.y-ml2.y);
	if (fabs(ftmp2)>fabs(ftmp1)){
	ftmp1=ml2b.x;
	ftmp2=ml2b.y;
	ml2b.x=ml2.x;
	ml2b.y=ml2.y;
	ml2.x=ftmp1;
	ml2.y=ftmp2;
	ftmp1=ml1b.x;
	ftmp2=ml1b.y;
	ml1b.x=ml1.x;
	ml1b.y=ml1.y;
	ml1.x=ftmp1;
	ml1.y=ftmp2;
	}
	plot_PicBuf(murphy.pb,&ml1b,murphy.color+3);
	plot_PicBuf(murphy.pb,&ml2b,murphy.color+4);


if ( (m1.x < ml1b.x && m2.x > ml2b.x) ||
     (m1.x > ml1b.x && m2.x < ml2b.x) ) {
	ftmp1=ml2b.x;
	ftmp2=ml2b.y;
	ml2b.x=m2.x;
	ml2b.y=m2.y;
	m2.x=ftmp1;
	m2.y=ftmp2;
}

if ((fabs(m2.x-ml2b.x)< 100) && fabs(m1.x-ml1b.x) <100) {
#if 1
/*draw outline of miter segment */
       p_act = bresenham_init (&m2, &m1);
      do {                
	 plot_PicBuf (murphy.pb, p_act, murphy.color+1);
      } while (bresenham_next() != BRESENHAM_ERR);
       p_act = bresenham_init (&m1, &ml1b);
      do {                
	 plot_PicBuf (murphy.pb, p_act, murphy.color+1);
      } while (bresenham_next() != BRESENHAM_ERR);
       p_act = bresenham_init (&ml1b, &ml2b);
      do {                
	 plot_PicBuf (murphy.pb, p_act, murphy.color+1);
      } while (bresenham_next() != BRESENHAM_ERR);
       p_act = bresenham_init (&ml2b, &m2);
      do {                
	 plot_PicBuf (murphy.pb, p_act, murphy.color+1);
      } while (bresenham_next() != BRESENHAM_ERR);
#endif
	polygon_PicBuf (m1,m2,ml1b,ml2b);
      
}
}	
	
	murphy.last2=ml2;
	murphy.last1=ml1;
	murphy.first1=ml1b;
	murphy.first2=ml2b;
}

void polygon_PicBuf (DevPt p1, DevPt p4, DevPt p2, DevPt p3 )
{
struct {
int x;
int y;
 }polygon [8];
int xmin,ymin,xmax,ymax;
DevPt start,end,*p_act;
double denominator;
double A1,B1,C1,A2,B2,C2;
int scany;
int segx,segy,numlines;
int i,j,k;
/*
fprintf (stderr,"in polydraw: (%d,%d) (%d,%d) (%d,%d) (%d,%d)\n",p1.x,p1.y,p2.x,p2.y,p3.x,p3.y,
p4.x,p4.y);
*/
polygon[0].x=p1.x;
polygon[0].y=p1.y;
polygon[1].x=p2.x;
polygon[1].y=p2.y;

polygon[2].x=p2.x;
polygon[2].y=p2.y;
polygon[3].x=p3.x;
polygon[3].y=p3.y;

polygon[4].x=p3.x;
polygon[4].y=p3.y;
polygon[5].x=p4.x;
polygon[5].y=p4.y;

polygon[6].x=p4.x;
polygon[6].y=p4.y;
polygon[7].x=p1.x;
polygon[7].y=p1.y;
/*
fprintf(stderr,"pline0 %d %d - %d %d\n",polygon[0].x,polygon[0].y,polygon[1].x,polygon[1].y);
fprintf(stderr,"pline1 %d %d - %d %d\n",polygon[2].x,polygon[2].y,polygon[3].x,polygon[3].y);
fprintf(stderr,"pline2 %d %d - %d %d\n",polygon[4].x,polygon[4].y,polygon[5].x,polygon[5].y);
fprintf(stderr,"pline3 %d %d - %d %d\n",polygon[6].x,polygon[6].y,polygon[7].x,polygon[7].y);
*/

xmin=MIN(p1.x,p2.x);
xmin=MIN(xmin,p3.x);
xmin=MIN(xmin,p4.x);
xmax=MAX(p1.x,p2.x);
xmax=MAX(xmax,p3.x);
xmax=MAX(xmax,p4.x);
ymin=MIN(p1.y,p2.y);
ymin=MIN(ymin,p3.y);
ymin=MIN(ymin,p4.y);
ymax=MAX(p1.y,p2.y);
ymax=MAX(ymax,p3.y);
ymax=MAX(ymax,p4.y);
		
/*
xmin=xmin-2;
xmax=xmax+2;
*/

numlines = 1+ ymax - ymin;

/* start at lowest y , run scanlines parallel x across polygon */
/* looking for intersections with edges */

for (i = 0; i <= numlines; i++) { /* for all scanlines ...*/ 
k=-1;
start.x=start.y=end.x=end.y=0;
scany = ymin + i;
/*
if(scany >= ymax || scany<=ymin) {
continue;
}
*/
/* coefficients for current scan line */
A1=0.;
B1=xmin-xmax;
C1= scany*(xmax-xmin);
	   
for (j =0 ; j <= 6  ; j=j+2) { /*for all polygon edges*/

/* coefficients for this edge */
A2=polygon[j+1].y-polygon[j].y;
B2=polygon[j].x-polygon[j+1].x;
C2=polygon[j].x*(polygon[j].y-polygon[j+1].y) + polygon[j].y*(polygon[j+1].x-polygon[j].x);

/*determine coordinates of intersection */
denominator = A1*B2-A2*B1;
if (fabs(denominator)>1.e-5){ /* zero means parallel lines */

segx= lrint((B1*C2-B2*C1) /denominator);     /*x coordinate of intersection */
segy= (C1*A2-C2*A1) /denominator;     /*y coordinate of intersection */

/*fprintf(stderr,"seg x,y= %d %d\n",segx,segy);*/
if ( (segx > xmax) || (segx<xmin) || 
     (segx < MIN(polygon[j].x,polygon[j+1].x)) ||
     (segx > MAX(polygon[j].x,polygon[j+1].x)) ) {
/*fprintf(stderr,"intersection  at %d %d is not within (%d,%d)-(%d,%d)\n",segx,segy,polygon[j].x,polygon[j].y,polygon[j+1].x,polygon[j+1].y ) ; */
     }else{

                        k++;
		if (k==0){
		start.x=segx;
		start.y=scany;
		}else if (fabs(segx- start.x)>1) {
		end.x=segx;
		end.y=scany;
		}else if (k>=0) k--;
          	}  /* if crossing withing range */ 
    } /*if not parallel*/

 } /*next edge */   
/*fprintf(stderr,"k=%d\n",k);*/
if (k>=1) {
/*fprintf(stderr,"fillline %d %d - %d %d\n",start.x,start.y,end.x,end.y);*/
	p_act= bresenham_init (&start,&end);
		do{
			plot_PicBuf(murphy.pb,p_act, murphy.color);
		} while (bresenham_next() != BRESENHAM_ERR);

}

} /* next scanline */

}
