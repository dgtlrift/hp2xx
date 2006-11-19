/*
   Copyright (c) 2001  Martin Kroeker  All rights reserved.
   
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

/** to_pdf.c:   Converter to Adobe Portable Document (PDF) Format using pdflib
 ** 
 ** 01/06/19  V 1.00 MK   derived from to_eps.c
 **/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#ifdef PDFLIB
#include <pdflib.h>
#else
#include <hpdf.h>
#endif
#include "bresnham.h"
#include "hp2xx.h"
/*#include "pendef.h"*/
#include "lindef.h"


static int linecount = 0;
static float xcoord2mm, ycoord2mm;
static float xmin, ymin;

/* globals for states since we have to init them for each file now */
static PEN_W lastwidth;
static int lastcap;
static int lastjoin;
static int lastlimit;
static Byte lastred, lastgreen, lastblue;

#ifdef PDFLIB
typedef PDF* PDFHANDLE;
#else
typedef HPDF_Doc PDFHANDLE;
HPDF_Page page;
#endif


int to_pdf(const GEN_PAR *, const OUT_PAR *);
void pdf_init(const GEN_PAR *, const OUT_PAR *, PDFHANDLE , PEN_W);
void pdf_set_linewidth(double, PDFHANDLE );
void pdf_set_linecap(LineEnds type, double pensize, PDFHANDLE );
void pdf_set_linejoin(LineJoins type, LineLimit limit, double pensize,
		      PDFHANDLE fd);
void pdf_set_color(PEN_C pencolor, PDFHANDLE  fd);
int pdf_end(PDFHANDLE  ,const OUT_PAR *);
int pdf_perm_file(int *, char *, char *, const OUT_PAR *);
int ini_parse(const OUT_PAR *, const char *, const char *, char *);

#ifndef PDFLIB
#define PAGEMODE if (openpath==1) { HPDF_Page_Stroke(page); openpath=0; }
#else
#define PAGEMODE if (openpath==1) { PDF_stroke(md); openpath=0; }
#endif

/**
 ** Close graphics file
 **/
int pdf_end(PDFHANDLE  fd, const OUT_PAR *po)
{

#ifdef PDFLIB
        PDF_end_page_ext(fd,"");
	PDF_end_document(fd,"");
	PDF_delete(fd);
	PDF_shutdown();
#else

int Rc=0;
int pdf_perm=0;
char owner_perm[254];
char user_perm[254];

    /* include user's security (Horst Liesinger 2005-02-15) */
    if (po->ps_incres) {
      Rc=pdf_perm_file(&pdf_perm, owner_perm, user_perm, po);
/*      printf("pdf_perm_file: Rc=%i Ownerpwd:%s Userpwd:%s Enable:%i\n",Rc, owner_perm, user_perm, pdf_perm); */
      if (Rc==0) {
/* pdf_doc_set_password(pdf_doc doc, const char* owner_passwd, const char* user_passwd) */
        HPDF_SetPassword(fd, owner_perm, user_perm);
/*  int pdf_doc_set_permission(pdf_doc doc, int permission); */
        HPDF_SetPermission(fd, pdf_perm);
      }
    }

	if (HPDF_SaveToFile(fd, po->outfile) !=0) {
		PError("hp2xx (pdf)");
		HPDF_Free(fd);
		return ERROR;
	}
	HPDF_Free(fd);
#endif
	linecount = 0;
	return 0;
}


/**
 ** Set line width
 **/
void pdf_set_linewidth(double width, PDFHANDLE  fd)
{
	double newwidth;

	if (width == 0.0) {
		newwidth = 0.0001;
	} else {
		newwidth = width;
	}

	if (fabs(newwidth - lastwidth) >= 0.01) {
#ifdef PDFLIB
		PDF_setlinewidth(fd, newwidth * MM_TO_PS_POINT);
#else
                HPDF_Page_SetLineWidth(page, newwidth * MM_TO_PS_POINT);
#endif
		lastwidth = newwidth;
	}
}


/**
 ** Set line Ends
 **/
void pdf_set_linecap(LineEnds type, double pensize, PDFHANDLE fd)
{
	int newcap;

	if (pensize > 0.35) {
		switch (type) {
		case LAE_butt:
			newcap = 0;
			break;
		case LAE_triangular:	/* triangular not implemented in PS/PDF */
			newcap = 1;
			break;
		case LAE_round:
			newcap = 1;
			break;
		case LAE_square:
			newcap = 2;
			break;
		default:
			newcap = 0;
			break;
		}
	} else {
		newcap = 1;
	}

	if (newcap != lastcap) {
#ifdef PDFLIB
		PDF_setlinecap(fd, newcap);
#else		
                HPDF_Page_SetLineCap (page, newcap);
#endif
		lastcap = newcap;
	}
}


/**
 ** Set line Joins
 **/
void pdf_set_linejoin(LineJoins type, LineLimit limit, double pensize,
		      PDFHANDLE fd)
{
	int newjoin;
	int newlimit = lastlimit;


	if (pensize > 0.35) {
		switch (type) {
		case LAJ_plain_miter:
			newjoin = 0;
			newlimit = 5;	/* arbitrary value */
			break;
		case LAJ_bevel_miter:	/* not available */
			newjoin = 0;
			newlimit = limit;
			break;
		case LAJ_triangular:	/* not available */
			newjoin = 1;
			break;
		case LAJ_round:
			newjoin = 1;
			break;
		case LAJ_bevelled:
			newjoin = 2;
			break;
		case LAJ_nojoin:	/* not available */
			newjoin = 1;
			break;
		default:
			newjoin = 0;
			newlimit = 5;	/* arbitrary value */
			break;
		}
	} else {
		newjoin = 1;
	}

	if (newjoin != lastjoin) {
#ifdef PDFLIB
		PDF_setlinejoin(fd, newjoin);
#else
		HPDF_Page_SetLineJoin(page, newjoin);
#endif
		lastjoin = newjoin;
	}

	if (newlimit != lastlimit) {
#ifdef PDFLIB
		PDF_setmiterlimit(fd, newlimit * MM_TO_PS_POINT);
#else
		HPDF_Page_SetMiterLimit(page, newlimit * MM_TO_PS_POINT);
#endif
		lastlimit = newlimit;
	}
}


/**
 ** Set RGB color
 **/
void pdf_set_color(PEN_C pencolor, PDFHANDLE fd)
{

	if ((pt.clut[pencolor][0] != lastred) ||
	    (pt.clut[pencolor][1] != lastgreen)
	    || (pt.clut[pencolor][2] != lastblue)) {
#ifdef PDFLIB
			PDF_setrgbcolor(fd,
				(double) pt.clut[pencolor][0] / 255.0,
				(double) pt.clut[pencolor][1] / 255.0,
				(double) pt.clut[pencolor][2] / 255.0);
#else				    
			HPDF_Page_SetRGBStroke(page,
				(double) pt.clut[pencolor][0] / 255.0,
				(double) pt.clut[pencolor][1] / 255.0,
				(double) pt.clut[pencolor][2] / 255.0);
#endif


		lastred = pt.clut[pencolor][0];
		lastgreen = pt.clut[pencolor][1];
		lastblue = pt.clut[pencolor][2];
	} else {
		return;
	}
}


/**
 ** basic PDF definitions
 **/

void pdf_init(const GEN_PAR * pg, const OUT_PAR * po, PDFHANDLE fd,
	      PEN_W pensize)
{
	long left, right, low, high;
	double hmxpenw;

	lastwidth = -1.0;
	lastcap = lastjoin = lastlimit = -1;
	lastred = lastgreen = lastblue = -1;

	hmxpenw = pg->maxpensize / 20.0;	/* Half max. pen width, in mm   */

	left = (long) floor(fabs(po->xoff - hmxpenw) * MM_TO_PS_POINT);
	low = (long) floor(fabs(po->yoff - hmxpenw) * MM_TO_PS_POINT);
	right =
	    (long) ceil((po->xoff + po->width + hmxpenw) * MM_TO_PS_POINT);
	high =
	    (long) ceil((po->yoff + po->height + hmxpenw) *
			MM_TO_PS_POINT);

#ifdef PDFLIB
	PDF_begin_page_ext(fd, (double) right, (double) high,"");
	pdf_set_linewidth(pensize, fd);
	pdf_set_linecap(CurrentLineAttr.End, pensize, fd);
	pdf_set_linejoin(CurrentLineAttr.Join, CurrentLineAttr.Limit,
			 pensize, fd);
#else	
        HPDF_SetCompressionMode(fd,HPDF_COMP_ALL);
	page=HPDF_AddPage(fd);
	HPDF_Page_SetWidth(page, (float) right);
	HPDF_Page_SetHeight(page, (float) high);
	pdf_set_linewidth(pensize, fd);
	pdf_set_linecap(CurrentLineAttr.End, pensize, fd);
	pdf_set_linejoin(CurrentLineAttr.Join, CurrentLineAttr.Limit,
			 pensize, fd);
#endif
}



/**
 ** Higher-level interface: Output Portable Document Format
 **/

int to_pdf(const GEN_PAR * pg, const OUT_PAR * po)
{
	PlotCmd cmd;
        PDFHANDLE md;
	HPGL_Pt pt1 = { 0, 0 };
	int pen_no = 0, err;
	int openpath;
	PEN_W pensize;

#ifdef PDFLIB
        int Rc=0;
        int pdf_perm=0;
        char owner_perm[254];
        char user_perm[254];
        char optionstring[1024];
	PDF_boot();
#endif	

	err = 0;
	if (!pg->quiet)
		Eprintf("\n\n- Writing PDF code to \"%s\"\n",
			*po->outfile == '-' ? "stdout" : po->outfile);

	/* Init. of PDF file: */

#ifdef PDFLIB
	md = PDF_new();

    /* include user's security (Horst Liesinger 2005-02-15) */
        if (po->ps_incres) {
           Rc=pdf_perm_file(&pdf_perm, owner_perm, user_perm, po);
           sprintf(optionstring,"masterpassword=%s userpassword=%s permissions {",owner_perm,user_perm);
           if ((pdf_perm&32)==0) strcat(optionstring,"noannots ");
           if ((pdf_perm&16)==0) strcat(optionstring,"nocopy ");
           if ((pdf_perm&8)==0) strcat(optionstring,"nomodify ");
           if ((pdf_perm&4)==0) strcat(optionstring,"noprint ");
           strcat(optionstring,"}");
           } else {
           strcpy(optionstring,"");	
        }
	if (PDF_begin_document(md, po->outfile,0,optionstring) == -1) {
		PError("hp2xx (pdf)");
		return ERROR;
	}	
#else	
	md = HPDF_New(NULL,NULL);
	if (md == NULL) {
		PError("hp2xx (pdf)");
		return ERROR;
	}
	
#endif
	/* header */

	pensize = pt.width[DEFAULT_PEN_NO];	/* Default pen    */
	pdf_init(pg, po, md, pensize);


	/* Factor for transformation of HP coordinates to mm  */

	xcoord2mm = po->width / (po->xmax - po->xmin) * MM_TO_PS_POINT;
	ycoord2mm = po->height / (po->ymax - po->ymin) * MM_TO_PS_POINT;
	xmin = po->xmin;
	ymin = po->ymin;

/**
 ** Command loop: While temporary file not empty: process command.
 **/
	openpath = 0;
	while ((cmd = PlotCmd_from_tmpfile()) != CMD_EOF) {
		switch (cmd) {
		case NOP:
			break;

		case SET_PEN:
			if ((pen_no = fgetc(pg->td)) == EOF) {
				PError("Unexpected end of temp. file: ");
				err = ERROR;
				goto PDF_exit;
			}
			pensize = pt.width[pen_no];
			break;

		case DEF_PW:
			if (!load_pen_width_table(pg->td,0)) {
				PError("Unexpected end of temp. file");
				err = ERROR;
				goto PDF_exit;
			}
			pensize = pt.width[pen_no];
			break;

		case DEF_PC:
			err = load_pen_color_table(pg->td,0);
			if (err < 0) {
				PError("Unexpected end of temp. file");
				err = ERROR;
				goto PDF_exit;
			}
			break;

		case DEF_LA:
			if (load_line_attr(pg->td,0) < 0) {
				PError("Unexpected end of temp. file");
				err = ERROR;
				goto PDF_exit;
			}
			break;

		case MOVE_TO:
			pensize = pt.width[pen_no];
			PAGEMODE;
			pdf_set_linewidth((double) pensize, md);
			pdf_set_linecap(CurrentLineAttr.End,
					(double) pensize, md);
			pdf_set_linejoin(CurrentLineAttr.Join,
					 CurrentLineAttr.Limit,
					 (double) pensize, md);
			pdf_set_color(pt.color[pen_no], md);

			HPGL_Pt_from_tmpfile(&pt1);
#ifdef PDFLIB
			PDF_moveto(md,
#else			
			HPDF_Page_MoveTo(page,
#endif			
			   		(pt1.x - xmin) * xcoord2mm,
			   		(pt1.y - ymin) * ycoord2mm);
			openpath = 1;
			break;

		case DRAW_TO:
			pensize = pt.width[pen_no];
			pdf_set_linewidth((double) pensize, md);
			pdf_set_linecap(CurrentLineAttr.End,
					(double) pensize, md);
			pdf_set_linejoin(CurrentLineAttr.Join,
					 CurrentLineAttr.Limit,
					 (double) pensize, md);

			pdf_set_color(pt.color[pen_no], md);

			HPGL_Pt_from_tmpfile(&pt1);
#ifdef PDFLIB
			PDF_lineto(md,
#else			
			HPDF_Page_LineTo(page,
#endif			
					(pt1.x - xmin) * xcoord2mm,
				   	(pt1.y - ymin) * ycoord2mm);
			openpath = 1;
			break;

		case PLOT_AT:
			pensize = pt.width[pen_no];
			PAGEMODE;

			pdf_set_color(pt.color[pen_no], md);

			HPGL_Pt_from_tmpfile(&pt1);
#ifdef PDFLIB
			PDF_save(md);
			PDF_setlinewidth(md, 0.00001);
			PDF_circle(md,(pt1.x - xmin) * xcoord2mm,
				   (pt1.y - ymin) * ycoord2mm,
				   pensize / 2 * MM_TO_PS_POINT);
			PDF_fill(md);
			PDF_restore(md);
#else				   
			HPDF_Page_GSave(page);
			HPDF_Page_MoveTo(page,(pt1.x-xmin)*xcoord2mm,
				  (pt1.y-ymin)*ycoord2mm); 
			HPDF_Page_LineTo(page,(pt1.x-xmin+0.00001)*xcoord2mm,
				  (pt1.y-ymin+0.00001)*ycoord2mm); 
			HPDF_Page_Stroke(page);	  
			HPDF_Page_GRestore(page);
#endif
			break;

		default:
			Eprintf("Illegal cmd in temp. file!");
			err = ERROR;
			goto PDF_exit;
		}
	}

	/* Finish up */
	PAGEMODE;
	return pdf_end(md,po);

PDF_exit:
	if (!pg->quiet)
		Eprintf("\n");
	return err;
}

/**
 ** Read Permissions and passwords     added by Horst Liesinger 2006-02-16
 **/
int pdf_perm_file(int *pdf_perm, char *owner_perm, char *user_perm, const OUT_PAR *po)
{
int Rc=0;
int RC=0;
char stmp[100];
const char * PDF_SEC="PDF_PERMISSION";
const char * PDF_Us="user";
const char * PDF_Own="owner";
const char * PDF_En="enable";
/*
ENABLE_READ	=  0
ENABLE_PRINT	=  4
ENABLE_EDIT_ALL	=  8
ENABLE_COPY	= 16
ENABLE_EDIT	= 32
*/

/*  printf ("\npdf_perm_file: started\n"); */
  if (po->ps_incres) {
    Rc=ini_parse(po, PDF_SEC, PDF_Own, owner_perm);
      if (Rc!=0) {
        RC++;
        if (Rc==-1)
         Eprintf("Warning: cannot open security file %s: %s\n",po->ps_incres,strerror(errno));
        if (Rc==1)
         Eprintf("Warning: cannot read value for %s in security file %s\n",PDF_Own,po->ps_incres);
        if (Rc==2)
         Eprintf("Warning: cannot read variable %s in security file %s\n",PDF_Own,po->ps_incres);
        if (Rc==3)
         Eprintf("Warning: cannot read section %s in security file %s\n",PDF_SEC,po->ps_incres);
      } else {
        Rc=ini_parse(po, PDF_SEC, PDF_Us, user_perm);
         if (Rc!=0) {
           RC++;
           Eprintf("Warning: cannot read value for %s in security file %s\n",PDF_Us,po->ps_incres);
         } else {
           Rc=ini_parse(po, PDF_SEC, PDF_En, stmp);
            if (Rc!=0) {
              RC++;
              Eprintf("Warning: cannot read value for %s in security file %s\n",PDF_En,po->ps_incres);
            } else {
              Rc=sscanf(stmp,"%i",pdf_perm);
              if ( Rc != 1 ) {
                RC++;
                Eprintf("Warning:  value %s for %s in security file %s is not an integer\n",stmp ,PDF_En,po->ps_incres);
              }
            }
         }
      }
    }
/*  printf ("pdf_perm_file: end\n"); */
  return RC;
}

/**
 ** Read ini file      added by Horst Liesinger 2006-02-16
 **/
int ini_parse(const OUT_PAR *po,const char * section,const char * variable, char * value)
{
#if 0
char * line = NULL;
ssize_t read;
#else
char *line=malloc(512*sizeof(char));
size_t read;
#endif
int lineNum=0;
int i=0;
int t=0;
int secOK=1;
int varOK=1;
int valOK=1;
size_t len = 0;
char tmp[256];
char Var[256];
char Val[256];
FILE *Fi;
/*
 printf("File: >%s<\n",po->ps_incres);
 printf("section: >%s<\n",section);
 printf("variable: >%s<\n",variable);
*/
 Fi = fopen(po->ps_incres,"r");
 if (NULL==Fi) {
/*  printf("Warning: cannot open security file %s\n",po->ps_incres); */
  return -1;
 } else {
#if 0
  while ((read = getline(&line, &len, Fi)) != -1) {
#else
  while (fgets(line, 512, Fi) != NULL) {
    read=strlen(line);
#endif
    lineNum++;
    while (( line[0] == ' ' ) || ( line[0] == 9 )) {
      line++;
      read--;
    }
    while (( line[read-1] == 10 ) || ( line[read-1] == 13 ) || (line[read-1] == ' ' ) || (line[read-1] == 9))
      line[read-- -1]=(int)NULL;
    if ( line[0] == '[' ) {
      sscanf(line,"[%s]",tmp);
      while ( strlen(tmp) && ( (tmp[strlen(tmp)-1] == ' ') || (tmp[strlen(tmp)-1] == 9) || (tmp[strlen(tmp)-1] == ']' ) )) {
      tmp[strlen(tmp)-1] = (int)NULL;
      }
    }else{
      if ( line[0] != (int) NULL ) {
       if ( strcmp(tmp,section) == 0 ) {
         secOK=0;
         i=0;
         Var[i]=(int)NULL;
         while (( line[0] != ' ' ) && ( line[0] != '=' ) && ( line[0] != (int)NULL) ){
           Var[i++]=line[0];
           Var[i]=(int)NULL;
           line++;
           read--;
         }
         if ( strcmp(Var,variable)==0 ) {
           varOK=0;
           while (( line[0] == ' ' ) || ( line[0] == 9 )) {
             line++;
             read--;
           }
           if (line[0] == '=') {
             valOK=0;
             line++;
             while (( line[0] == ' ' ) || ( line[0] == 9 )) { line++;}
             t=0;
 /*                         = "                  = '                  = `     */
             if (( line[0] == 34 ) || ( line[0] == 39 ) || ( line[0] == 96 )) {
               t=line[0];
               line++;
             }
             i=0;
             Val[i]=(int)NULL;
             while ((line[0] != ';') && (line[0] != '#') && (line[0] != t) && (line[0] != (int)NULL)) {
               Val[i++]=line[0];
               Val[i]=(int)NULL;
               line++;
             }
             while ( strlen(Val) && ( (Val[strlen(Val)-1] == ' ') || (Val[strlen(Val)-1] == 9) ) ) {
              Val[strlen(Val)-1] = (int)NULL;
             }
           }else{
             Val[0]=(int)NULL;
           }
           sprintf(value,"%s", Val);
         }
       }
      }
    }
  len=0;
  }
#if 0
  if (line)
    free(line);
#endif
 fclose (Fi);

 return (secOK + valOK + varOK);
 }
}

