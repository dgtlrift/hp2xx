/*
   Copyright (c) 1991 - 1993 Heinz W. Werntges.  All rights reserved.
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

/** hp2xx : A converter of HPGL files into popular raster and vector formats
 **
 **	xx =  mf (MetaFont), eps (PostScript), pcl (HP-PCL Level3),
 **	      pre(view), pcx (PaintBrush-Format), img (GEM),
 **	      pic (ATARI bitmap), pbm (Portable Bitmap), ...
 **
 ** (c) 1992, 1993  Heinz W. Werntges and the HP2xx team
 **
 ** Author:
 **
 **	HWW	Heinz W. Werntges  (werntges@convex.rz.uni-duesseldorf.de)
 **		Fruchtstr. 2,  40223 Duesseldorf, Germany
 **
 **
 ** Amiga stuff & PBM & ILBM by:
 **
 **	CHL	Claus H. Langhans (Claus_Langhans@wildcat.fido.de)
 **		Unter den Nussbaeumen 19, 65779 Kelkheim (Ts.), Germany
 **
 **
 ** Atari stuff & IMG by:
 **
 **	NM	Norbert Meyer (sorry - no email)
 **		Reinhold-Schneider-Str. 83, 40595 Duesseldorf
 **
 **	AS	Andreas Schwab (schwab@ls5.informatik.uni-dortmund.de)
 **
 **	JE	Joern Eggers (II040EG@DTHAC11.bitnet)
 **
 ** OS/2 Full-Screen previewer by:
 **
 **	HSz	Host Szillat (szillat@iaas-berlin.d400.de)
 **
 ** X11 previewer by:
 **
 **	MSch	Michael Schoene (michael@nero.uni-bonn.de)
 **		Karolinger Str. 81,  40223 Duesseldorf, Germany
 **
 **
 ** Many thanks to all the other fine people who supported HP2xx by testing,
 ** correcting, porting, suggesting improvements, and otherwise spending
 ** their time on HP2xx, especially:
 **
 ** -- Michael Schmitz
 **  & Gerhard Steger 	 for their invaluable help with VAX/VMS,
 ** -- Juergen Gross     for access to a HP 9000 machine
 ** -- Roland Emmerich   for proofreading, beta-testing, and his HGC support
 ** -- Jon Gillian       for valuable suggestions
 **-------------------------------------------------------------------------
 **/

/** hp2xx.c:
 **
 ** 91/01/12  V 1.00  HWW  PC version
 ** 91/01/17  V 1.01  HWW  Disk swapping added
 ** 91/01/19  V 1.02  HWW  Reorganized: mf mode revived, pcx mode added
 ** 91/01/29  V 1.03  HWW  SUN portation; HP-specials added
 ** 91/01/31  V 1.04  HWW  (-F) FF added
 ** 91/02/01  V 1.05  HWW  Flag -S added (Deskjet specials)
 ** 91/02/15  V 1.06  HWW  stdlib.h & VAX_C supported
 ** 91/02/19  V 2.01  HWW  file unchanged, but new package release!
 ** 91/06/09  V 2.10  HWW  (beta) More options; options x & y changed to o & O
 ** 91/06/15  V 2.11  HWW  (beta) VGA option added; PCX bug (on SUN) removed
 ** 91/06/20  V 2.12  HWW  Rotate (-r) option & Copyright added
 ** 91/06/20  V 2.12b HWW  Minor reformatting
 ** 91/06/29  V 2.13a HWW  PostScript support
 ** 91/08/28  V 2.14  HWW  ATARI 32K format added (prelim.)
 ** 91/10/15  V 2.14c HWW  atari & stad formats; hwtools.h eliminated
 ** 91/10/20  V 2.14d HWW  ATARI-->pic (packing removed)
 ** 91/10/24  V 2.15  HWW  ATARI-->pic stable now; "LT;" & "LT0;" supported
 ** 91/11/22  V 2.16b HWW  "SPn;" support
 ** 92/01/12  V 2.17  HWW  HPGL scanner improved, to_ps() debugged, ps --> eps,
 **			  More portabel code (ATARI acknowledged)
 ** 92/02/06  V 2.18e HWW  Better parser, IMG & various previews supported,
 **			  preview control via -h, -w; -W removed!
 ** 92/02/21  V 2.19d HWW  LB etc. supported, PG; (option -P) added
 ** 92/03/03  V 2.20c HWW  Modes added: -m epic,em,pac(ATARI) / LB bugs fixed
 ** 92/04/15  V 2.20d HWW  Width & height treated symmetrically now
 ** 92/04/28  V 2.20e HWW  -m pre: -d, -D effective again, default 75; PBM added
 ** 92/05/02  V 2.21a HWW  -m pre is default now. New opt: -l logfile
 ** 92/05/19  V 3.00c HWW  Color support (VGA preview), multiple input files
 ** 92/05/25  V 3.00f HWW  Color support (more modes)
 ** 92/06/09  V 3.01d HWW  XT,YT,TL,SM supported; ATARI update acknowledged;
 **			   Mode list added for easier expansion; debugged
 ** 92/10/20  V 3.02a HWW  LT now fully supported
 ** 92/11/08  V 3.02b HWW  Page range allowed
 ** 92/12/10  V 3.02c HWW  Mode "cad" added; DJ version: bug fix extended;
 ** 92/12/12  V 3.02d HWW  2 bug fixes for option -f
 ** 92/12/16  V 3.03a HWW  Option -t (+truesize) added
 ** 92/12/27  V 3.03b HWW  -S int (for Deskjet color support)
 ** 93/01/02  V 3.10a HWW  Source brushed up; ready for distribution
 ** 93/02/08  V 3.12a HWW  Support of Joern Eggers' cs mode added (ATARI only)
 **			   HAS_X11, HAS_SUNVIEW: added/modified
 ** 93/04/13  V 3.12b HWW  UC added (A. Treindl), small changes; CMYK added,
 **			   New previewer selection scheme // NEW RELEASE 3.1.2
 ** 93/04/25  V 3.12c HWW  Minor bug fixes (to_pcl.c, hpgl.c / Eric)
 ** 93/05/20  V 3.12d HWW  LT1 adjustment; '--version' option added (GNU std)
 ** 93/07/20  V 3.12e HWW  SilentWait() modified (UNIX) according to A. Bagge,
 **			   new OS/2 previewer by HSz, TIFF compression for PCL mode
 ** 93/09/03  V 3.13a HWW  Wait() renamed to NormalWait() (name collision on VAX),
 **			   Option -P : Option parser modified; -m hpgl added
 ** 93/09/22  V 3.13b HWW  Fix in autoset_outfile_name()
 **/

char	*VERS_NO = "3.14";
char	*VERS_DATE = "93/11/15";
char	*VERS_COPYRIGHT = "(c) 1991 - 1993  Heinz W. Werntges";
#if defined(AMIGA)
char	*VERS_ADDITIONS =
	"\tAmiga additions (V 2.00) by Claus Langhans (92/12/16)\n";
#elif defined (ATARI)
char	*VERS_ADDITIONS =
	"\tAtari additions (V 2.10) by N. Meyer / J. Eggers / A. Schwab  (93/01/xx)\n";
#else
char	*VERS_ADDITIONS = "";
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "bresnham.h"
#include "hp2xx.h"
#include "getopt.h"



/**
 ** When adding your special mode, add a symbol here.
 ** Please not the alphabetical order (and keep it).
 **/

typedef	enum{
	XX_CAD, XX_CS, XX_EM, XX_EPIC, XX_EPS, XX_HPGL, XX_ILBM, XX_IMG,
	XX_MF, XX_PBM, XX_PCL, XX_PCX, XX_PAC, XX_PIC, XX_PRE, XX_RGIP,
	XX_TERM	/* Dummy: terminator	*/
} hp2xx_mode;


/**
 ** When adding your special mode, add a line here.
 ** Please not the alphabetical order (and keep it).
 **/

static struct {
	hp2xx_mode	mode;
	char		*modestr;
} ModeList[] = {
	{XX_CAD,	"cad"},	/* LaTeX: TeXcad compatible output	*/
#ifdef	ATARI
	{XX_CS,		"cs"},	/* LaTeX using \special{...} for C. Strunk's TeX	*/
#endif
	{XX_EM,		"em"},	/* LaTeX using \special{em:...}		*/
	{XX_EPIC,	"epic"},/* LaTeX using epic.sty macros		*/
	{XX_EPS,	"eps"},	/* Encapulated PostScript		*/
	{XX_HPGL,	"hpgl"},/* Simplified HP-GL			*/
#ifdef	AMIGA
	{XX_ILBM,	"ilbm"},/* Special AMIGA format			*/
#endif
	{XX_IMG,	"img"},	/* Digital Research IMG raster format	*/
	{XX_MF,		"mf"},	/* Metafont source output		*/
	{XX_PBM,	"pbm"},	/* Portable Bitmap			*/
	{XX_PCL,	"pcl"},	/* HP-PCL Level 5 printer code		*/
	{XX_PCX,	"pcx"},	/* Paintbrush's PCX raster format	*/
#ifdef	PIC_PAC
	{XX_PAC,	"pac"},	/* for ATARI, e.g. used by StaD		*/
	{XX_PIC,	"pic"},	/* for ATARI. Try to replace by IMG	*/
#endif
	{XX_RGIP,	"rgip"},/* Uniplex RGIP vector format		*/
	{XX_PRE,	"pre"},	/* DEFAULT: Preview on screen		*/
	{XX_TERM,	""},	/* Dummy: List terminator		*/
};


static	Logfile_flag = FALSE;


void	SilentWait (void)
{
char	dummy[80];
#ifdef UNIX
FILE	*tty;
#endif
/**
 ** Get anything typed including '\n' if stderr does NOT go to a file
 ** or else the user may be invisibly prompted.
 **
 ** According to a suggestion from A. Bagge, in UNIX pipe mode stdin
 ** will be replaced by /dev/tty.
 **/
  if (!Logfile_flag)
  {
#ifdef UNIX
	if ((tty = fopen("/dev/tty","r")) != NULL)
	{
		fgets (dummy, 80, tty);
		fclose(tty);
	}
	else
#endif
		fgets (dummy,80,stdin);
  }
}




void	NormalWait (void)
{
#ifdef	UNIX
  if (getenv("TERM") == (char *) NULL)
	return;
#endif
  printf ("\nPress <Return> to continue ...\n");
  SilentWait ();
}




void	print_supported_modes(void)
{
int	i;

  fprintf(stderr, "%s", ModeList[0].modestr);
  for (i=1; ModeList[i].mode != XX_TERM; i++)
	fprintf(stderr, ",%s", ModeList[i].modestr);
  fprintf(stderr, "\n");
}




void	Send_version(void)
{
  fprintf(stderr,"\n%s\n%s\n%s\n%s\n%s\n",
  "hp2xx is free software and you are welcome to distribute copies of it",
  "  under certain conditions. There is absolutely no warranty for hp2xx!",
  "For full details, read file COPYING (shipped along with this package),",
  "  or write to:\t\tFree Software Foundation, Inc.",
  "\t\t\t675 Mass Ave, Cambridge, MA 02139, USA");

  fprintf(stderr,"\n%s\tV %s  (%s)   %s\n%s",
	"HP2xx:\tA HPGL converter (xx = mf, eps, pcl, pcx, img ...)\n",
	VERS_NO, VERS_DATE, VERS_COPYRIGHT, VERS_ADDITIONS);
}



void	usage_msg (PAR *p)
{
if (p->quiet)
	return;

Send_version();

#ifdef ATARI
fprintf (stderr,"Usage:\tdouble-click on HP2XX.TTP, type parameters into\n");
fprintf (stderr,"\tcommand-line with following syntax:\n");
fprintf (stderr,"\t[options] [hpglfile]\n");
fprintf (stderr,"\n\t(if command-line offers not enough space for all parameters\n");
fprintf (stderr,"\tuse a command-line interpreter for starting HP2xx)\n");
#else
fprintf (stderr,"Usage:\thp2xx [options] [file1 [file2 ...]]\n");
#endif	/* ATARI */

fprintf (stderr,"\tUnix: Filter usage (.. | hp2xx -q -f- [options] | ..) ok\n");
NormalWait();

fprintf (stderr,"\nOpt fmt   defaults\tComment:\n");
fprintf (stderr,
"---------------------------------------------------------------------------\n");
fprintf (stderr,"-m strg   %s\t\tMode. Valid mode strings are:\n\t\t\t", p->mode);
print_supported_modes();

fprintf (stderr,"-f strg   (auto gen.)\tName of output file ('-' = to stdout)\n");
fprintf (stderr,"-l strg   (stderr)\tName of log file\n");
fprintf (stderr,"-p strg   %1d%1d%1d%1d%1d%1d%1d%1d\tPensize(s) (in 1/10 mm (mf,ps) or dots (rest)).\n",
	p->pensize[1], p->pensize[2], p->pensize[3], p->pensize[4],
	p->pensize[5], p->pensize[6], p->pensize[7], p->pensize[8]);
fprintf (stderr,"\t\t\t\"strg\" must consist of 1 to 8 digits '0'-'9'\n");
fprintf (stderr,"-c strg   %1d%1d%1d%1d%1d%1d%1d%1d\tPen color(s) (see manual for details).\n",
	p->pencolor[1], p->pencolor[2], p->pencolor[3], p->pencolor[4],
	p->pencolor[5], p->pencolor[6], p->pencolor[7], p->pencolor[8]);
fprintf (stderr,"-P n:n    %d:%d\t\tPage range (0:0 = all pages).\n",
	p->first_page, p->last_page);
fprintf (stderr,"-q        %s\t\tQuiet mode (no diagnostics)\n",
	FLAGSTATE(p->quiet));
fprintf (stderr,"-r float%5.1f\t\tRotation angle [deg]. -r90 = landscape\n",
	p->rotation);
fprintf (stderr,"-s strg   %s\tName of swap file\n",
	p->swapfile);

fprintf (stderr,"\nBitmap controls:\n");
fprintf (stderr,"-d int    %d\t\tDPI value for x or x&y, if -D unused.\n",
	p->dpi_x);
fprintf (stderr,"-D int    %d\t\tDPI value for y ONLY\n", p->dpi_x);
	 /* x, not y! */
NormalWait();

fprintf (stderr,"\nPCL-exclusive options:\n");
fprintf (stderr,"-i         %s\tPre-initialize printer\n",
	FLAGSTATE (p->init_p));
fprintf (stderr,"-F         %s\tSend a FormFeed at end\n",
	FLAGSTATE(p->formfeed));
fprintf (stderr,"-S int     %d\tUse Deskjet special commands (0=off, 1=B/W, 3=CMY, 4=CMYK)\n",
	p->specials);
fprintf (stderr,"NOTE:    \tOnly valid for -d: 300/150/100/75; -D invalid!\n");

fprintf (stderr,"\nPCL / PostScript / Preview options:\n");
fprintf (stderr,"-o float %5.1f\tX offset [mm] of picture\n", p->xoff);
fprintf (stderr,"-O float %5.1f\tY offset [mm] of picture\n", p->yoff);

fprintf (stderr,"\nSize controls:\n");

fprintf (stderr,"-a float %5.1f\tAspect factor (x/y correction). Valid: > 0.0\n",
	p->aspectfactor);
fprintf (stderr,"-h float %5.1f\tHeight [mm] of picture\n",p->height);
fprintf (stderr,"-w float %5.1f\tWidth  [mm] of picture\n",p->width );
fprintf (stderr,"-t         %s\tShow true HPGL size. Disables -a -h -w !\n",
	FLAGSTATE (p->truesize));
fprintf (stderr,"-x float   -\tManual HPGL-coord range presetting: x0\n");
fprintf (stderr,"-X float   -\tManual HPGL-coord range presetting: x1\n");
fprintf (stderr,"-y float   -\tManual HPGL-coord range presetting: y0\n");
fprintf (stderr,"-Y float   -\tManual HPGL-coord range presetting: y1\n");

#ifdef DOS
fprintf (stderr,"\n-V int   %d\tVGA mode byte (decimal). Change at own risk!\n",
		p->vga_mode);
#endif

NormalWait();

fprintf (stderr,"Corresponding long options:\n\n");
fprintf (stderr,
	"hp2xx   [--mode] [--colors] [--pensizes] [--pages] [--quiet]\n");
fprintf (stderr,"\t[--width] [--height] [--aspectfactor] [--truesize]\n");
fprintf (stderr,"\t[--x0] [--x1] [--y0] [--y1] [--xoffset] [--yoffset]\n");
fprintf (stderr,"\t[--DPI] [--DPI_x] [--DPI_y]\n");
fprintf (stderr,"\t[--outfile] [--logfile] [--swapfile]\n");
fprintf (stderr,"\t[--PCL_formfeed] [--PCL_init] [--PCL_Deskjet]\n");
#ifdef DOS
fprintf (stderr,"\t[--VGAmodebyte]");
#endif
fprintf (stderr,"\t[--help] [--version]\n");


#ifdef PURE_C
fprintf(stderr,"\nPress RETURN key\n");
getchar();
#endif
}




void	reset_par (PAR *pp)
/**
 ** Reset some parameter struct elements which may have been changed
 ** by action() to their defaults
 **/
{
  pp->x0	=  1e10;	/* HP7550A's range is about     */
  pp->x1	= -1e10;	/* [-2^24, 2^24], so we're safe */
  pp->y0	=  1e10;
  pp->y1	= -1e10;
  pp->pen	= 1;
}




void	preset_par (PAR *pp)
/**
 ** Pre-set constant parameter struct elements with reasonable defaults
 **/
{
int	i;

  pp->logfile	= "";
  pp->outfile	= "";
  pp->swapfile	= "hp2xx.swp";
  pp->mode	= "pre";
  pp->aspectfactor = 1.0;
  pp->xoff	= 0.0;
  pp->yoff	= 0.0;
  pp->height	= 200.0;
  pp->width	= 200.0;
  pp->truesize	= FALSE;
  pp->rotation	= 0.0;
  pp->first_page= 0;
  pp->last_page	= 0;
  pp->init_p	= FALSE;
  pp->formfeed	= FALSE;
  pp->quiet	= FALSE;
  pp->specials	= 0;
  pp->dpi_x	= 75;
  pp->dpi_y	= 0;
  pp->vga_mode	= 18;		/* 0x12: VGA 640x480, 16 colors */
  pp->maxpensize= 1;		/* in pixel or 1/10 mm		*/
  pp->maxcolor	= 1;		/* max. color index		*/
  pp->pensize[0]= 0;		/* in pixel or 1/10 mm		*/
  pp->pencolor[0]= xxBackground;
  for (i=1; i<=8; i++)
  {
	pp->pensize [i]	= 1;	/* in pixel or 1/10 mm		*/
	pp->pencolor[i]	= xxForeground;
  }
  pp->is_color	= FALSE;
  pp->Clut[xxBackground][0]= 255;  pp->Clut[xxBackground][1]	= 255;
  pp->Clut[xxBackground][2]= 255;
  pp->Clut[xxForeground][0]= 0;	   pp->Clut[xxForeground][1]	= 0;
  pp->Clut[xxForeground][2]= 0;
  pp->Clut[xxRed][0]	= 255;	   pp->Clut[xxRed][1]		= 0;
  pp->Clut[xxRed][2]	= 0;
  pp->Clut[xxGreen][0]	= 0;	   pp->Clut[xxGreen][1]		= 255;
  pp->Clut[xxGreen][2]	= 0;
  pp->Clut[xxBlue][0]	= 0;	   pp->Clut[xxBlue][1]		= 0;
  pp->Clut[xxBlue][2]	= 255;
  pp->Clut[xxCyan][0]	= 0;	   pp->Clut[xxCyan][1]		= 255;
  pp->Clut[xxCyan][2]	= 255;
  pp->Clut[xxMagenta][0]= 255;	   pp->Clut[xxMagenta][1]	= 0;
  pp->Clut[xxMagenta][2]= 255;
  pp->Clut[xxYellow][0]	= 255;	   pp->Clut[xxYellow][1]	= 255;
  pp->Clut[xxYellow][2]	= 0;
  reset_par (pp);
}




void	Send_Copyright(void)
/**
 **	Remnant of older (non-GNU) releases. Leave here if you like
 **/
{
static	unsigned char msg[] =
	{0xaf,0xa8,0xcd,0xd5,0x97,0xdd,0xdd,0x9f,
	 0x85,0x8d,0xc6,0x8c,0x85,0xed,0x8b,0x85,
	 0xf2,0xc0,0xd7,0xcb,0xd1,0xc2,0xc0,0xd6,
	 0xaf,0xa8,0xa5};
unsigned char	*p;

  p = msg;
  while (*p!=0xa5)
	putc ((*p++ ^ 0xa5), stderr);
  exit  (COPYNOTE);
}




void	autoset_outfile_name(PAR *pp, char *inp_name)
{
int	len, i;

  if (*pp->outfile=='-')/* If output explicitly to stdout:		*/
	return;		/*    then nothing's to do here			*/

  if (isalpha(*pp->outfile))/* If this looks like an output file name:	*/
	return;		/*    Just accept it! Add validity check later?	*/

  if (inp_name == NULL)	/* If input from stdin				*/
	len = 0;
  else
	len = strlen(inp_name);

  if (len == 0)		/* If input from stdin:				*/
  {			/*    then supply a default file name		*/
	pp->outfile="hp2xx.out";
	return;
  }

  if (strcmp(pp->mode,"pre") == 0)
	return;		/* If preview mode:				*/
			/*    then output file name is unused		*/

  for (i=len-1; i; i--)	/* Search for (last) '.' char in path		*/
	if (inp_name[i] == '.')
		break;
  i++;

/**
 ** We can assume a valid inp_name file name here.
 ** If not, the following fopen() in main() will fail
 ** and no harm will be done by an incorrect output file name.
 **/

  if ((pp->outfile = malloc(len+2+strlen(pp->mode))) == NULL)
  {
	fprintf(stderr,"Error: No mem for output file name!\n");
	perror ("autoset_outfile_name");
	exit   (ERROR);
  }
  strcpy(pp->outfile, inp_name);

  if (i==1 || len-i > 3) /* No or non-DOS extension: Add mode string	*/
  {
	strcat(pp->outfile, ".");
	strcat(pp->outfile, pp->mode);	/* Mode string is extension!	*/
  }
  else
	strcpy(pp->outfile+i, pp->mode);	/* Replace extension	*/
}





void	action (PAR *p, FILE *hd)
{
FILE		*td;
PicBuf		*picbuf;
DevPt		Maxdotcoord;
hp2xx_mode	xx_mode=XX_TERM;
int		i;

  if (!p->quiet)
	Send_version();
  for (i=0; ModeList[i].mode != XX_TERM; i++)
	if (strncmp(p->mode, ModeList[i].modestr,
		strlen(ModeList[i].modestr)) == 0)
	{
		xx_mode = ModeList[i].mode;
		break;
	}

/**
 ** Read HPGL data, put them into compact temporary binary file, and obtain
 ** scaling data (xmin/xmax/ymin/ymax in plotter coordinates)
 **/

#if defined(DOS) && defined (GNU)
	/**
	 ** GNU libc.a (DJ's DOS port) bug fix (part 1 of 2):
	 **	 tmpfile() does not seem to work!
	 ** See code below for part 2/2 (removing hp2xx.$$$).
	 ** NOTE:
	 **	If program terminates abnormally,
	 **	delete hp2xx.$$$ manually!!
	 **/
  if ((td = fopen("hp2xx.$$$","w+b")) == NULL)
#elif defined(AMIGA)
  if ((td = fopen("t:hp2xx.tmp","w+b")) == NULL)
#else
  if ((td = tmpfile()) == NULL)
#endif
  {
	perror("hp2xx -- opening temporary file");
	exit (ERROR);
  }
  read_HPGL (p, hd, td, &Maxdotcoord);
  if (hd != stdin)
	fclose (hd);

/**********************************************************
 ** Vector modes:
 **/
  switch (xx_mode)
  {
    case XX_MF:
	rewind	(td);	    /* Rewind temp file after filling it*/
	to_mftex(p, td, 0); /* Output conversion/generation	*/
	fclose	(td);	    /* Close & unlink temp file 	*/
	return;

    case XX_EM:
	rewind	(td);
	to_mftex(p, td, 1);
	fclose	(td);
	return;

    case XX_EPIC:
	rewind	(td);
	to_mftex(p, td, 2);
	fclose	(td);
	return;

    case XX_CAD:
	rewind	(td);
	to_mftex(p, td, 3);
	fclose	(td);
	return;

#ifdef	ATARI
    case XX_CS:
	rewind	(td);
	to_mftex(p, td, 4);
	fclose	(td);
	return;
#endif

    case XX_HPGL:
	rewind	(td);
	to_mftex(p, td, 5);
	fclose	(td);
	return;

    case XX_EPS:
	rewind	(td);
	to_eps	(p, td);
	fclose	(td);
	return;

    case XX_RGIP:
	rewind	(td);
	to_rgip	(p, td);
	fclose	(td);
	return;

    /* default: drop through	*/
    default:
	break;
  }


/**********************************************************
 ** Common part for all pixel-oriented formats:
 ** 	Buffer allocation / vector-to-raster conversion
 **/

/**
 ** 1) Allocate virtual plotter area
 **/
  Maxdotcoord.x += (p->maxpensize - 1);
  Maxdotcoord.y += (p->maxpensize - 1);
  if ((picbuf = allocate_PicBuf (&Maxdotcoord, p)) == NULL)
  {
	fprintf(stderr,"Fatal error: cannot allocate %d*%d picture buffer\n",
		Maxdotcoord.x, Maxdotcoord.y);
	exit(ERROR);
  }

/**
 ** 2) Read vectors from temporary file and plot them in memory buffer
 **/
  tmpfile_to_PicBuf (picbuf, p, td);
  fclose (td); /* Close & unlink tmpfile */

#if defined(DOS) && defined (GNU)
/**
 ** GNU libc.a (DJ's DOS port) bug fix (part 2 of 2):
 **/
  unlink ("hp2xx.$$$");
#endif



/**********************************************************
 ** Raster modes:
 **/

  switch (xx_mode)
  {
  case XX_PCL:	PicBuf_to_PCL (picbuf,p); break; /* HP PCL Level 3	*/
  case XX_PCX:	PicBuf_to_PCX (picbuf,p); break; /* Paintbrush PCX	*/
#ifdef	PIC_PAC
  case XX_PIC:	PicBuf_to_PIC (picbuf,p); break; /* ATARI 32K format	*/
  case XX_PAC:	PicBuf_to_PAC (picbuf,p); break; /* ATARI STaD format	*/
#endif						 /*   To be phased out!	*/
#ifdef	AMIGA
  case XX_ILBM:	PicBuf_to_ILBM(picbuf,p); break; /* AMIGA IFF-ILBM fmt	*/
#endif
  case XX_IMG:	PicBuf_to_IMG (picbuf,p); break; /* GEM's IMG format	*/
  case XX_PBM:	PicBuf_to_PBM (picbuf,p); break; /* Portable BitMap fmt	*/

  case XX_PRE:
/**********************************************************
 ** Previewers (depending on hardware platform):
 **/
#if   defined(HAS_DOS_DJGR)
		PicBuf_to_DJ_GR (picbuf,p); break;
#elif defined(HAS_DOS_HGC)
		PicBuf_to_HGC	(picbuf,p); break;
#elif defined(HAS_DOS_VGA)
		PicBuf_to_VGA	(picbuf,p); break;
#elif defined(HAS_OS2_EMX)
		PicBuf_to_OS2	(picbuf,p); break;
#elif defined(HAS_OS2_PM)
		PicBuf_to_PM	(picbuf,p); break;
#elif defined(HAS_UNIX_X11)
		PicBuf_to_X11	(picbuf,p); break;
#elif defined(HAS_UNIX_SUNVIEW)
		PicBuf_to_Sunview(picbuf,p);break;
#elif defined(ATARI)
		PicBuf_to_ATARI	(picbuf,p); break;
#elif defined(AMIGA)
		PicBuf_to_AMIGA	(picbuf,p); break;
#elif defined(VAX)
		PicBuf_to_UIS	(picbuf,p); break;
#else
		PicBuf_to_Dummy	(); break;
#endif	/* defined(...)	*/

/**********************************************************
 ** Done -- clean up
 **/
  default:	fprintf(stderr,"%s: Not implemented!\n", p->mode); break;
  }

  free_PicBuf (picbuf, p->swapfile);
}





/**
 ** main(): Process command line & call action routine
 **/

int	main (int argc, char *argv[])
{
PAR	par;
double	width, height;
int	c, i,j, longind;
char	*p, cdummy;
FILE	*hd;

char	*shortopts = "a:c:d:D:f:h:l:m:o:O:p:P:r:s:S:V:w:x:X:y:Y:FHiqtv";
struct	option longopts[] = {
	{"mode",	1, NULL,	'm'},
	{"pencolors",	1, NULL,	'c'},
	{"pensizes",	1, NULL,	'p'},
	{"pages",	1, NULL,	'P'},
	{"quiet",	0, NULL,	'q'},

	{"DPI",		1, NULL,	'd'},
	{"DPI_x",	1, NULL,	'd'},
	{"DPI_y",	1, NULL,	'D'},

	{"PCL_formfeed",0, NULL,	'F'},
	{"PCL_init",	0, NULL,	'i'},
	{"PCL_Deskjet",	1, NULL,	'S'},

	{"outfile",	1, NULL,	'f'},
	{"logfile",	1, NULL,	'l'},
	{"swapfile",	1, NULL,	's'},

	{"aspectfactor",1, NULL,	'a'},
	{"height",	1, NULL,	'h'},
	{"width",	1, NULL,	'w'},
	{"truesize",	0, NULL,	't'},

	{"x0",		1, NULL,	'x'},
	{"x1",		1, NULL,	'X'},
	{"y0",		1, NULL,	'y'},
	{"y1",		1, NULL,	'Y'},

	{"xoffset",	1, NULL,	'o'},
	{"yoffset",	1, NULL,	'O'},

#ifdef DOS
	{"VGAmodebyte",1, NULL,	'V'},
#endif
	{"help",	0, NULL,	'H'},
	{"version",	0, NULL,	'v'}
};

  preset_par (&par);
  width = par.width;	/* Copy defaults	*/
  height= par.height;

  if (argc == 1)
  {
	usage_msg (&par);
	exit (ERROR);
  }

  while ((c=getopt_long(argc,argv, shortopts, longopts, &longind)) != EOF)
	switch (c)	/* Easy addition of options ... */
	{
	  case 'a':
		par.aspectfactor = atof (optarg);
		if (par.aspectfactor <= 0.0)
		{
			fprintf(stderr,"Aspect factor: %g illegal\n",
				par.aspectfactor);
			exit(ERROR);
		}
		break;

	  case 'c':
		i = strlen(optarg);
		if ((i<1) || (i>8))
		{
			fprintf(stderr,"Invalid pencolor string: %s\n", optarg);
			exit(ERROR);
		}
		for (j=1, p = optarg; j <= i; j++, p++)
		{
		    switch (*p-'0')
		    {
			case xxBackground:par.pencolor[j] = xxBackground; break;
			case xxForeground:par.pencolor[j] = xxForeground; break;
			case xxRed:	  par.pencolor[j] = xxRed;	  break;
			case xxGreen:	  par.pencolor[j] = xxGreen;	  break;
			case xxBlue:	  par.pencolor[j] = xxBlue;	  break;
			case xxCyan:	  par.pencolor[j] = xxCyan;	  break;
			case xxMagenta:	  par.pencolor[j] = xxMagenta;	  break;
			case xxYellow:	  par.pencolor[j] = xxYellow;	  break;
			default :
				  fprintf(stderr,
				    "Invalid color of pen %d: %c\n", j, *p);
				  exit(ERROR);
		    }
		    if (par.pencolor[j] != xxBackground &&
			par.pencolor[j] != xxForeground)
				par.is_color = TRUE;
		}
		break;

	  case 'd':
		switch (par.dpi_x = atoi (optarg))
		{
		  case 75:
			break;
		  case 100:
		  case 150:
		  case 300:
			if ((!par.quiet) && (strcmp(par.mode,"pcl")==0))
			fprintf(stderr,
			"Warning: DPI setting is no PCL level 3 feature!\n");
			break;
		  default:
			if ((!par.quiet) && (strcmp(par.mode,"pcl")==0))
			fprintf(stderr,
			"Warning: DPI value %d is invalid for PCL mode\n",
				par.dpi_x);
			break;
		}
		break;

	  case 'D':
		par.dpi_y = atoi (optarg);
		if ((!par.quiet) && (strcmp(par.mode,"pcl")==0))
			fprintf(stderr,"Warning: %s\n",
			"Different DPI for x & y is invalid for PCL mode");
		break;

	  case 'F':
		par.formfeed = TRUE;
		break;

	  case 'f':
		par.outfile = optarg;
		break;

	  case 'h':
		par.height = height = atof (optarg);
		if (height < 0.1)
			fprintf(stderr,
				"Warning: Small height: %g mm\n", height);
		if (height > 300.0)
			fprintf(stderr,
				"Warning: Huge  height: %g mm\n", height);
		break;

	  case 'i':
		par.init_p = TRUE;
		break;

	  case 'l':
		par.logfile = optarg;
		if (freopen(par.logfile, "w", stderr) == NULL)
		{
			perror ("Cannot open log file");
			fprintf(stderr, "Error redirecting stderr\n");
			fprintf(stderr, "Continuing with output to stderr\n");
		}
		else
			Logfile_flag = TRUE;
		break;

	  case 'm':
		par.mode = optarg;
		for (i=0; ModeList[i].mode != XX_TERM; i++)
			if (strcmp(ModeList[i].modestr, par.mode) == 0)
				break;
		if (ModeList[i].mode == XX_TERM)
		{
			fprintf(stderr,
			"'%s': unknown mode!\n", par.mode);
			fprintf(stderr,"Supported are:\n\t");
			print_supported_modes();
			Send_Copyright();
		}
		break;

	  case 'o':
		par.xoff = atof (optarg);
		if (par.xoff < 0.0)
		{
			fprintf(stderr,"Illegal X offset: %g < 0\n",
				par.xoff);
			exit(ERROR);
		}
		if (par.xoff > 210.0)	/* About DIN A4 width */
		{
			fprintf(stderr,"Illegal X offset: %g > 210\n",
				par.xoff);
			exit(ERROR);
		}
		break;

	  case 'O':
		par.yoff = atof (optarg);
		if (par.yoff < 0.0)
		{
			fprintf(stderr,"Illegal Y offset: %g < 0\n",
				par.yoff);
			exit(ERROR);
		}
		if (par.yoff > 300.0)	/* About DIN A4 height */
		{
			fprintf(stderr,"Illegal Y offset: %g > 300\n",
				par.yoff);
			exit(ERROR);
		}
		break;

	  case 'p':
		i = strlen(optarg);
		if ((i<1) || (i>8))
		{
			fprintf(stderr,"Invalid pensize string: %s\n", optarg);
			exit(ERROR);
		}
		for (j=1, p = optarg; j <= i; j++, p++)
		{
			if ((*p < '0') || (*p > '9'))
			{
				fprintf(stderr,"Invalid size of pen %d: %c\n",
					j, *p);
				exit(ERROR);
			}
			par.pensize[j] = *p - '0';
			if (par.maxpensize < par.pensize[j])
				par.maxpensize = par.pensize[j];
		}
		break;

	  case 'P':
		if (*optarg == ':')
		{
			par.first_page = 0;
			optarg++;
			if (sscanf(optarg,"%d", &par.last_page) != 1)
				par.last_page = 0;
		}
		else
			switch (sscanf(optarg,"%d%c%d",
				&par.first_page, &cdummy, &par.last_page))
			{
			  case 1:
				par.last_page = par.first_page;
				break;

			  case 2:
				if (cdummy == ':')
				{
					par.last_page = 0;
					break;
				}
				/* not ':' Syntax error -- drop through	*/
			  case 3:
				if (cdummy == ':')
					break;
				/* not ':' Syntax error -- drop through	*/
			  default:
				fprintf(stderr,"Illegal page range.\n");
				usage_msg (&par);
				exit(ERROR);
			}
		break;

	  case 'q':
		par.quiet = TRUE;
		break;

	  case 'r':
		par.rotation = atof(optarg);
		break;

	  case 'S':
		par.specials = atoi (optarg);
		break;

	  case 's':
		par.swapfile = optarg;
		break;

	  case 't':
		par.truesize = TRUE;
		break;

	  case 'V':
		par.vga_mode = atoi (optarg);
		break;

	  case 'w':
		par.width = width = atof (optarg);
		if (width < 0.1)
			fprintf(stderr,
				"Warning: Small width: %g mm\n", width);
		if (width > 300.0)
			fprintf(stderr,
				"Warning: Huge  width: %g mm\n", width);
		break;

	  case 'v':
		Send_version();
		exit (NOERROR);

	  case 'x':
		par.x0 = atof (optarg);
		break;

	  case 'X':
		par.x1 = atof (optarg);
		break;

	  case 'y':
		par.y0 = atof (optarg);
		break;

	  case 'Y':
		par.y1 = atof (optarg);
		break;

	  case 'H':
	  case '?':
	  default:
		usage_msg (&par);
		exit (ERROR);
	}

/**
 ** Place consistency checks here
 ** - I'm just not in the mood for writing them myself ...
 **/

  if (par.dpi_y == 0)
	par.dpi_y = par.dpi_x;

  if (optind == argc)		/* No  filename: use stdin	*/
  {
	autoset_outfile_name (&par, NULL);
	action (&par, stdin);
  }
  else	for ( ; optind < argc; optind++)
	{			/* Multiple-input file handling: */
		autoset_outfile_name (&par, argv[optind]);
		if ((hd=fopen (argv[optind], READ_BIN)) == NULL)
		{
			perror("hp2xx");
			exit (ERROR);
		}
		action (&par, hd);	/* action() closes file	*/
		reset_par (&par);
		par.width = width;	/* Restore width/height	*/
		par.height= height;	/* Changed in hpgl.c !	*/
	}

  if (*par.logfile)
	fclose (stderr);
  return NOERROR;
}
