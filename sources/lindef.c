#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include "lindef.h"
#include "hpgl.h"

double     CurrentLinePatLen;
LineType   CurrentLineType;
signed int CurrentLinePattern;

LINESTYLE lt;

void set_line_style(SCHAR index, ...){
   SCHAR count;
   double factor,percentage;
   va_list ap;

   SCHAR val;

   va_start(ap,index);

   for(count=0,percentage=0;count < LT_ELEMENTS;count++) {

      val=va_arg(ap,int);

      if(val < 0) {
         break;
      } else  {
          lt[index-LT_MIN][count] = val;
          percentage += val;
      }
   } 

   lt[index-LT_MIN][count] = -1;

   if(fabs(percentage - 100.) > 0.5) {
/*      fprintf(stderr,"\nset_line_style: style %d percentage != 100 - adjusting\n",index);*/
      factor=100.0/percentage;
      for(count=0;count < LT_ELEMENTS;count++) {
        if(lt[index-LT_MIN][count] < 0) {
           break;
        } else  {
            lt[index-LT_MIN][count] *= factor;
        }
      }
   }

   va_end(ap);

}

void set_line_style_by_UL(FILE *hd){ 
   SCHAR index,count;
   double factor,percentage;
   float tmp;

   if(read_float(&tmp,hd)) {
     set_line_style_defaults();   /* reset to defaults */
     return;
   } else {
     index = (int) tmp;
   }

   for(count=0,percentage=0;(read_float(&tmp,hd)==0);count++) {  /* while there is an argument */
     lt[index-LT_MIN][count] = (int) tmp;
     percentage += (int) tmp;
   }

   lt[index-LT_MIN][count] = -1;

   if(fabs(percentage - 100.) > 0.5) {
/*      fprintf(stderr,"\nset_line_style_by_UL: style %d percentage != 100\n",index);*/
      factor=100.0/percentage;
      for(count=0;count < LT_ELEMENTS;count++) {
        if(lt[index-LT_MIN][count] < 0) {
           break;
        } else  {
            lt[index-LT_MIN][count] *= factor;
        }
      }
   }                                                                                                                    

}


void set_line_style_defaults() {
/*                 Line  gap   Line  gap   Line   gap   Line  TERM        */

/*-8 */
   set_line_style(-8, 25, 10, 0, 10, 10, 10, 0, 10, 25, -1);
/*-7 */
   set_line_style(-7, 35, 10, 0, 10, 0, 10, 35, -1);
/*-6 */
   set_line_style(-6, 25, 10, 10, 10, 10, 10, 25, -1);
/*-5 */
   set_line_style(-5, 35, 10, 10, 10, 35, -1);
/*-4 */
   set_line_style(-4, 40, 10,  0, 10, 40, -1);
/*-3 */
   set_line_style(-3, 35, 30, 35, -1);
/*-2 */
   set_line_style(-2, 25, 50, 25, -1);
/*-1 */
   set_line_style(-1,  0,100, 0, -1);
/* 0 */
   set_line_style( 0,  0,100, -1);
/* 1 */
   set_line_style( 1,  1, 99, -1);
/* 2 */
   set_line_style( 2, 50, 50, -1);
/* 3 */
   set_line_style( 3, 70, 30, -1);
/* 4 */
   set_line_style( 4, 80, 10,  0, 10, -1);
/* 5 */
   set_line_style( 5, 70, 10, 10, 10, -1);
/* 6 */
   set_line_style( 6, 50, 10, 10, 10, 10, 10, -1);
/* 7 */
   set_line_style( 7, 70, 10, 0, 10, 0, 10, -1);
/* 8 */
   set_line_style( 8, 50, 10, 0, 10, 10, 10, 0, 10, -1);
}

void print_line_style(void) {
   int i,j;

   printf("\n");

   for(j=0;j < LT_PATTERNS;j++){
      if(lt[j][0] < 0)
          continue;
      else
         printf("LT %3d ",LT_MIN+j);
         
      for (i =0 ;i <= LT_ELEMENTS;i++) {
         if(lt[j][i] < 0)
            break;
         printf("%3.00f ",lt[j][i]);
      }
      printf("\n");
   }
}

