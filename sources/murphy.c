
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "bresnham.h"
#include "murphy.h"
#include "picbuf.h"
#include "hp2xx.h"

static struct {
   int color;
   PicBuf *pb;
   int u,v;                       /* delta x , delta y */
   int ku,kt,kv,kd;               /* loop constants */
   int oct2;
   int quad4;
} murphy;

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

   DevPt pt;
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

   tk = 4 * murphy_normconst(pt.x-p0.x,pt.y-p0.y) * murphy_normconst(murphy.u,
   murphy.v); /* used here for constant thickness line */
   for (q = 0; dd <= tk; q++) {  /* outer loop, stepping perpendicular to line */

      murphy_paraline(pt,d1);         /* call to inner loop - right edge */

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
            if (dd > tk)
               return;          /* breakout on the extra line */
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
}

double murphy_normconst(int u, int v) {
   u = abs(u);
   v = abs(v);

#if 0
   /* 12% thickness error - uses add and shift only */
   return (u + v / 4);

   /* 2.7% thickness error, uses compare, add and shift only */
   return ((v + v + v) > u) ? u - (u / 8) + v / 2 : u + v / 8;
#endif

   /* ideal */
   return sqrt(u * u + v * v);
}
