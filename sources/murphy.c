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
	DevPt last1,last2;
} murphy;


#ifdef NORINT 
#define lrint(a) ((long)(a+0.5))
#endif

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
}

void murphy_wideline(DevPt p0, DevPt p1, int width) {  /* implements figure 5A - draws lines parallel to ideal line */

   float offset = width/2;

   DevPt pt,ptx,ml1,*p_act;
DevPt fill1,fill2;
int dist1,dist2,miter;

   int d0, d1;                    /* difference terms d0=perpendicular to line, d1=along line */
int i;
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
	if (q==0) ml1=pt;
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
	if (miter==1){
	for (i=0;i<width*2;i++){
	fill1.x=murphy.last1.x-i* (murphy.last1.x-murphy.last2.x)/(width*2);
	fill1.y=murphy.last1.y-i* (murphy.last1.y-murphy.last2.y)/(width*2);
	fill2.x=ptx.x-i*(ptx.x-pt.x)/(width*2);
        fill2.y=ptx.y-i*(ptx.y-pt.y)/(width*2);
       p_act = bresenham_init (&fill2, &fill1);
      do {                
	 plot_PicBuf (murphy.pb, p_act, murphy.color);
      } while (bresenham_next() != BRESENHAM_ERR);
      }
    }
	murphy.last2=pt;
	murphy.last1=ml1;
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
	if (miter==1){
	for (i=0;i<width*2;i++){
	fill1.x=murphy.last1.x-i* (murphy.last1.x-murphy.last2.x)/(width*2);
	fill1.y=murphy.last1.y-i* (murphy.last1.y-murphy.last2.y)/(width*2);
	fill2.x=ptx.x-i*(ptx.x-pt.x)/(width*2);
        fill2.y=ptx.y-i*(ptx.y-pt.y)/(width*2);
       p_act = bresenham_init (&fill2, &fill1);
      do {                
	 plot_PicBuf (murphy.pb, p_act, murphy.color);
      } while (bresenham_next() != BRESENHAM_ERR);
      }
    }
    
	murphy.last2=pt;
	murphy.last1=ml1;
}

