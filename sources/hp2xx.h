#ifndef	__HP2XX_H
#define	__HP2XX_H
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

/** hp2xx.h: Header for Project "hp2xx"
 **
 ** 91/01/13  V 1.00  HWW  Originating
 ** 91/01/19  V 1.01  HWW  Reorganized
 ** 91/01/31  V 1.02  HWW  Tested on SUN; some flags added
 ** 91/02/15  V 1.03  HWW  SUN release; stdlib.h supported
 ** 91/02/18  V 1.04  HWW  plot_picbuf() is macro; new type: HPGL_Pt
 ** 91/06/18  V 1.05  HWW  New PAR elements; ANSI_C supported
 ** 91/06/15  V 1.06  HWW  VGA options added (PC only)
 ** 91/06/20  V 1.07  HWW  Rotation added
 ** 91/09/08  V 1.08  HWW  ATARI 32K format added
 ** 91/10/09  V 1.08a HWW  ATARI 32K format: modified
 ** 91/10/14  V 1.09  HWW  ATARI: PAC format with compression now,
 **			     hwtools.h: excerpt incorporated
 ** 91/10/20  V 1.09b HWW  PAC (compression) removed; atari --> pic (renamed)
 ** 91/10/25  V 1.10  HWW  VAX_C: M_PI added
 ** 91/11/20  V 1.11  HWW  SPn; support
 ** 91/12/22  V 1.11b HWW  Some redefs in PS part
 ** 92/01/11  V 1.12  HWW  Norbert Meyer's ATARI portability changes built in
 ** 92/01/26  V 1.12a HWW  Some prototypes added
 ** 92/01/31  V 1.12c HWW  M_PI def changed ; SUNVIEW etc. + comments added
 ** 92/02/06  V 1.12d HWW  arcs(), circles() added
 ** 92/02/19  V 1.13c HWW  LB etc. supported
 ** 92/02/21  V 1.14a HWW  PG supported
 ** 92/05/06  V 1.14b HWW  Log files & color supported (starting)
 ** 92/05/19  V 2.00a HWW  Color supported
 ** 92/05/28  V 2.00b HWW  Some prototypes changed/added
 ** 92/06/09  V 2.00c HWW  debugged (GNU)
 ** 92/10/15  V 2.01a HWW  LineType added
 ** 92/11/08  V 2.01c HWW  page --> first_page, last_page; GNU --> DJ_GR
 ** 92/12/13  V 2.02a HWW  truesize flag added; colors renamed
 ** 93/04/13  V 2.10a HWW  User-serviceable parts & redundant prototypes
 **			   removed
 ** 93/05/23  V 2.10b HWW  READ_BIN, WRITE_BIN: Now ok for OS/2, too
 ** 93/07/25  V 2.10c HWW  Some prototypes added
 ** 93/09/02  V 2.10d HWW  Some #defines added;
 **			   rect(), to_rgip(): prototypes added
 ** 94/01/02  V 2.11a HWW  PlotCmd_from_tmpfile(): new type; center_mode
 ** 94/02/10  V 3.00  HWW  New concept: Now three central parameter structs ...
 **/


/**
 ** Working with Pure C (the official descendant of Borland's
 ** Turbo C for Atari computers)
 **/

#ifdef __PUREC__ /* __PUREC__ is pre-defined by the Pure-C  compiler */
#ifndef PURE_C
#define PURE_C
#endif
#endif

/**
 ** Automatic definition of obsolete symbol by Turbo-C and Pure-C:
 **/

#ifdef __TURBOC__

#ifndef TURBO_C
#define TURBO_C
#endif

#endif



#ifdef GNU
#define   getch getkey
#endif


#ifndef M_PI
#define M_PI	3.141592654	/* Some machines don't know PI */
#endif

#ifndef EOF
#define EOF (-1)
#endif

#ifndef	MIN
#define MIN(x,y)  ((x)<(y) ? (x) : (y))
#endif

#ifndef	MAX
#define MAX(x,y)  ((x)<(y) ? (y) : (x))
#endif


#define	TRUE	1
#define	FALSE	0

/**
 ** Exit codes
 **/

#ifdef VAX
/**
 ** Only VAX exit code 0 provokes error treatment:
 **/
#define ERROR 0
#define NOERROR 1
#define COPYNOTE 2

#else	/* !VAX (regular case)	*/

#define ERROR (-1)
#define NOERROR 0
#define COPYNOTE 1

#endif

#define	ESC	'\033'
#define	CR	'\015'
#define	FF	'\014'
#define	LF	'\012'
#define	BEL	'\007'

#define	MAX_LB_LEN	150	/* Max num of chars per label	*/


#ifndef VOID
#ifdef	__STDC__
#define	VOID	void
#else
#define	VOID	char
#endif
#endif	/* ifndef VOID */

/* #endif */



/**
 ** Color codes used within hp2xx
 **/

#define	xxBackground	0
#define	xxForeground	1
#define	xxRed		2
#define	xxGreen		3
#define	xxBlue		4
#define	xxCyan		5
#define	xxMagenta	6
#define	xxYellow	7


/**
 ** Misc. typedefs
 **/

typedef unsigned char	Byte;


/**
 ** When adding your special mode, add a symbol here.
 ** Please note the alphabetical order (and keep it).
 **/

typedef	enum{
	XX_CAD, XX_CS, XX_EM, XX_EPIC, XX_EPS, XX_HPGL, XX_ILBM, XX_IMG,
	XX_MF, XX_PBM, XX_PCL, XX_PCX, XX_PAC, XX_PIC, XX_PRE, XX_RGIP,
	XX_TERM	/* Dummy: terminator	*/
} hp2xx_mode;



/**
 ** This struct handles a single row of our raster picture buffer
 **/

typedef	struct Row {
	Byte		*buf;	/* Ptr to actual data buffer which holds the bits	*/
	int		index;	/* Index of this row. Unique for each row. Ordered	*/
	struct Row	*prev, *next;	/* Rows are elements of a double-linked list	*/
} RowBuf;


/**
 ** Struct holding the whole raster picture
 **/

typedef	struct
{
   int	nr, nc, nb;	/* Number of rows / columns / bytes per row	*/
   int	depth;		/* Depth: Number of bit planes (1 to 4)		*/
   RowBuf *row;		/* Array pointer of all rows			*/
   char	*sf_name;	/* Swapfile name				*/
   FILE	*sd;		/* Swapfile pointer				*/
}	PicBuf;


typedef	struct {
	float	x,y;
} HPGL_Pt;


typedef enum {
	NOP, MOVE_TO, DRAW_TO, PLOT_AT, SET_PEN, CMD_EOF
} PlotCmd;


typedef struct {
	hp2xx_mode	mode;
	char		*modestr;
} mode_list;


/**
 ** Line types: No pattern number = solid line, 0 = dot at end coordinates,
 **		!= 0: various patterns, consisting of
 **			l)ong lines, s)hort lines, d)ots and g)aps.
 **		< 0:  adaptive line types (ada)
 **		> 0:  fixed line types (fix)
 **
 ** NOTE: See HP-GL manuals for details
 **/

typedef	enum
{
	LT_lgsgs_ada	= -6,
	LT_lgs_ada	= -5,
	LT_lgd_ada	= -4,
	LT_ls_ada	= -3,
	LT_l_ada	= -2,
	LT_d_ada	= -1,

	LT_plot_at	=  0,
				/* Line patterns:	*/
	LT_d_fix	=  1,	/*	.	 	*/
	LT_l_fix	=  2,	/*	-----	 	*/
	LT_ls_fix	=  3,	/*	------- 	*/
	LT_lgd_fix	=  4,	/*	------- .	*/
	LT_lgs_fix	=  5,	/*	------- -	*/
	LT_lgsgs_fix	=  6,	/*	----- - -	*/

	LT_solid	=  7,
	LT_default	=  7
}
	LineType;



/**
 ** Input parameters: Used mainly during input file processing
 **/

typedef struct			/* Corresponding option(s)	*/
{
   int	first_page, last_page;	/* -P first_page:last_page	*/
   int	center_mode;		/* -C				*/
   int	truesize;		/* -t				*/
   double  width, height;	/* -w width -h height		*/
   double  aspectfactor;	/* -a aspectfactor		*/
   double  rotation;		/* -r rotation			*/
   double  x0, x1, y0, y1;	/* -x x0 -X x1 -y y0 -Y y1	*/
   double  xoff, yoff;		/* -o xoff  -O yoff		*/
   char	*in_file;		/* Input file name ("-" = stdin)*/
   FILE	*hd;			/* (internally needed)		*/
}	IN_PAR;



/**
 ** Output parameters: Used mainly during output file generation
 **/

typedef struct			/* Corresponding option(s)	*/
{
   Byte	vga_mode;		/* -V vga_mode			*/
   int	vga_width;		/* (internally needed)		*/
   int	dpi_x, dpi_y;		/* -d dpi_x  -y dpi_y		*/
   int	init_p;			/* -i  (PCL only)		*/
   int	formfeed;		/* -F  (PCL only)		*/
   int  specials;		/* -s specials  (PCL only)	*/
   char	  *outfile;		/* -f outfile ("-" = stdout)	*/
   double xmin,ymin, xmax,ymax; /* (internally needed)		*/
   double xoff, yoff;		/* Internal copies from IN_PAR	*/
   double width, height;	/* Internal copies from IN_PAR	*/
   double HP_to_xdots;		/* (internally needed)		*/
   double HP_to_ydots;		/* (internally needed)		*/
   PicBuf *picbuf;		/* (internally needed)		*/
}	OUT_PAR;



/**
 ** General parameters: Used at various places
 **/

typedef struct			/* Corresponding option(s)	*/
{
   char  *mode;			/* -m mode			*/
   char  *logfile;		/* -l logfile			*/
   char	 *swapfile;		/* -s swapfile			*/
   int	 quiet;			/* -q				*/
   int	 pensize[9];		/* -p xxxxxxxx			*/
   int	 pencolor[9];		/* -c xxxxxxxx			*/
   int	 maxpensize;		/* (internally needed)		*/
   int	 is_color;		/* (internally needed)		*/
   int	 maxcolor;		/* (internally needed)		*/
   Byte	 	Clut[16][3];	/* (internally needed)		*/
   FILE	 	*td;		/* (internally needed)		*/
   hp2xx_mode	xx_mode;	/* (internally needed)		*/
}	GEN_PAR;


#define	DEFAULT_PEN_NO		1

#define	FLAGSTATE(flag)		(flag) ? "ON" : "off"


#if defined(TURBO_C) || defined (GNU) || defined (OS2)
#define	READ_BIN	"rb"
#define	WRITE_BIN	"w+b"
#else
#define	READ_BIN	"r"
#define	WRITE_BIN	"w+"
#endif


/**
 ** Prototypes:
 **/

void	Send_version	(void);
void	Send_Copyright	(void);
void	usage_msg	(const GEN_PAR*, const IN_PAR*, const OUT_PAR*);
void	print_supported_modes(void);
void	preset_par	(GEN_PAR*, IN_PAR*, OUT_PAR*);
void	reset_par	(IN_PAR*);
void	autoset_outfile_name (const char*, const char*, char**);

int	HPGL_to_TMP	(GEN_PAR*, IN_PAR*);
int	TMP_to_VEC	(const GEN_PAR*, const OUT_PAR*);
int	TMP_to_BUF	(const GEN_PAR*, OUT_PAR*);
int	BUF_to_RAS	(const GEN_PAR*, const OUT_PAR*);

void	cleanup_g	(GEN_PAR*);
void	cleanup_i	(IN_PAR*);
void	cleanup_o	(OUT_PAR*);
void	cleanup		(GEN_PAR*, IN_PAR*, OUT_PAR*);

void	Eprintf		(const char*, ...);
void	PError		(const char*);
void	SilentWait	(void);
void	NormalWait	(void);

void	plot_user_char	(FILE *);
void	read_HPGL	(const GEN_PAR*, const IN_PAR*);
void	adjust_input_transform	(const GEN_PAR*, const IN_PAR*, OUT_PAR*);
PlotCmd	PlotCmd_from_tmpfile	(void);
void	HPGL_Pt_from_tmpfile	(HPGL_Pt *);
void	Pen_action_to_tmpfile	(PlotCmd, const HPGL_Pt*, int);
int	read_float		(float*, FILE*);

void	to_ATARI	(GEN_PAR*, FILE *);
int	to_mftex	(const GEN_PAR*, const OUT_PAR*, int);
int	to_eps		(const GEN_PAR*, const OUT_PAR*);
int	to_rgip		(const GEN_PAR*, const OUT_PAR*);

void	size_PicBuf	(const GEN_PAR*, const OUT_PAR*, int*, int*);
PicBuf	*allocate_PicBuf(const GEN_PAR*, int, int);
void	free_PicBuf	(PicBuf *);
void	tmpfile_to_PicBuf(const GEN_PAR*, const OUT_PAR*);
int	index_from_PicBuf(const PicBuf*, const DevPt *);
int	index_from_RowBuf(const RowBuf*, int, const PicBuf *);
RowBuf	*get_RowBuf	(const PicBuf*, int);

int	PicBuf_to_PCL	(const GEN_PAR*, const OUT_PAR*);
int	PicBuf_to_PCX	(const GEN_PAR*, const OUT_PAR*);

int	PicBuf_to_IMG	(const GEN_PAR*, const OUT_PAR*);
int	PicBuf_to_PBM	(const GEN_PAR*, const OUT_PAR*);
int	PicBuf_to_ILBM	(const GEN_PAR*, const OUT_PAR*);

#ifdef PIC_PAC
int	PicBuf_to_PIC	(const GEN_PAR*, const OUT_PAR*);
int	PicBuf_to_PAC	(const GEN_PAR*, const OUT_PAR*);
#endif	/* PIC_PAC */

int	PicBuf_to_AMIGA	(const GEN_PAR*, const OUT_PAR*);
int	PicBuf_to_ATARI	(const GEN_PAR*, const OUT_PAR*);
int	PicBuf_to_DJ_GR	(const GEN_PAR*, const OUT_PAR*);
int	PicBuf_to_Dummy	(void);
int	PicBuf_to_PM	(const GEN_PAR*, const OUT_PAR*);
int	PicBuf_to_OS2	(const GEN_PAR*, const OUT_PAR*);
int	PicBuf_to_Sunview(const GEN_PAR*,const OUT_PAR*);
int	PicBuf_to_UIS	(const GEN_PAR*, const OUT_PAR*);
int	PicBuf_to_HGC	(const GEN_PAR*, const OUT_PAR*);
int	PicBuf_to_VGA	(const GEN_PAR*, const OUT_PAR*);
int	PicBuf_to_X11	(const GEN_PAR*, const OUT_PAR*);

#endif	/*	__HP2XX_H	*/
