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
 ** A bag for many options and internally needed data to be passed
 ** to various functions.
 **/

typedef struct {
	int	dpi_x, dpi_y, first_page, last_page,
		pen, pensize[9], maxpensize, is_color, truesize,
		quiet, init_p, formfeed, specials, vga_width;
	int	pencolor[9], maxcolor;
	Byte	Clut[16][3];
	double	xoff, yoff, x0, x1, y0, y1, width, height,
		aspectfactor, rotation;
	char	*outfile, *swapfile, *logfile, *mode;
	Byte	vga_mode;
} PAR;



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

typedef	struct {
			/* Number of rows / columns / bytes per row	*/
	int	nr, nc, nb;
			/* Depth: Number of bit planes (1 to 4)		*/
	int	depth;
			/* Swapfile handle				*/
	FILE	*sd;
			/* Array pointer of all rows			*/
	RowBuf	*row;
} PicBuf;


typedef	struct {
	float	x,y;
} HPGL_Pt;


typedef enum {
	NOP, MOVE_TO, DRAW_TO, PLOT_AT, SET_PEN
} PlotCmd;


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

void	SilentWait	(void);
/* void	Wait		(void); */

void	HPcoord_to_dotcoord
			(HPGL_Pt *, DevPt *);
void	init_HPGL	(FILE *, PAR *);
void	reset_HPGL	(void);
void	evaluate_HPGL	(PAR *, DevPt *);
void	read_HPGL	(PAR *, FILE *, FILE *, DevPt *);

void	read_ESC_cmd	(FILE *);
int	read_float	(float *, FILE *);
void	read_HPGL_cmd	(int,	FILE *);
void	read_string	(char *,FILE *);
void	read_symbol_char(FILE *);

void	arcs		(int,	FILE *);
void	lines		(int,	FILE *);
void	rect		(int,	FILE *);
void	circles		(FILE *);
void	ax_ticks	(int);

void	plot_user_char	(FILE *);

void	Pen_action_to_tmpfile	(PlotCmd, HPGL_Pt *, int);
void	PlotCmd_to_tmpfile	(PlotCmd);
void	HPGL_Pt_to_tmpfile	(HPGL_Pt *);
int	PlotCmd_from_tmpfile	(void);
void	HPGL_Pt_from_tmpfile	(HPGL_Pt *);

void	to_ATARI	(PAR *, FILE *);
void	to_mftex	(PAR *, FILE *, int);
void	to_eps		(PAR *, FILE *);
void	to_rgip		(PAR *, FILE *);

PicBuf	*allocate_PicBuf(DevPt *, PAR *);
void	free_PicBuf	(PicBuf *, char *);
void	tmpfile_to_PicBuf(PicBuf*, PAR *, FILE *);
void	line_PicBuf	(PicBuf *, DevPt *, DevPt *, PAR *);
int	index_from_PicBuf(PicBuf*, DevPt *);
int	index_from_RowBuf(RowBuf*, int, PicBuf *);
RowBuf	*get_RowBuf	(PicBuf*, int);

void	PicBuf_to_PCL	(PicBuf *, PAR *);
void	PicBuf_to_PCX	(PicBuf *, PAR *);

Byte	get_byte_IMG	(int, int, PicBuf *);
int	vert_rep_IMG	(int, PicBuf *);
int	empty_SR_IMG	(int, int, PicBuf *);
int	full_SR_IMG	(int, int, PicBuf *);
int	PR_IMG		(int, int, PicBuf *);
void	write_byte_IMG	(Byte, PicBuf *, PAR *, FILE *);
void	write_VR_IMG	(Byte, PicBuf *, PAR *, FILE *);
void	write_PR_IMG	(Byte, Byte, Byte, PicBuf *, PAR *, FILE *);
void	write_empty_SR_IMG	(Byte, PicBuf *, PAR *, FILE *);
void	write_full_SR_IMG	(Byte, PicBuf *, PAR *, FILE *);
void	write_BS_IMG	(Byte, PicBuf *, PAR *, FILE *);
void	PicBuf_to_IMG	(PicBuf *, PAR *);

void	PicBuf_to_PBM	(PicBuf *, PAR *);

void	PicBuf_to_ILBM	(PicBuf *, PAR *);

#ifdef PIC_PAC
int	Init_PIC_files	(char *, FILE **, int, int, int);
void	PicBuf_to_PIC	(PicBuf *, PAR *);
void	RowBuf_to_PIC	(RowBuf *, int, FILE **);

int	Screenpos_PAC	(int *, int *, int);
int	Pack_PAC	(Byte *, Byte, Byte, Byte, int, FILE *);
void	Analyze_PAC	(Byte *, Byte *, Byte *, Byte *);
void	Screen_for_PAC	(PicBuf *, Byte *, int, int);
void	Name_PAC	(char *, char *, int, int, int);
void	PicBuf_to_PAC	(PicBuf *, PAR *);
#endif	/* PIC_PAC */

void	PicBuf_to_AMIGA	(PicBuf *, PAR *);
void	PicBuf_to_ATARI	(PicBuf *, PAR *);
void	PicBuf_to_DJ_GR	(PicBuf *, PAR *);
void	PicBuf_to_Dummy	(void);
void	PicBuf_to_PM	(PicBuf *, PAR *);
void	PicBuf_to_OS2	(PicBuf *, PAR *);
void	PicBuf_to_Sunview(PicBuf *, PAR *);
void	PicBuf_to_UIS	(PicBuf *, PAR *);
void	PicBuf_to_HGC	(PicBuf *, PAR *);
void	PicBuf_to_VGA	(PicBuf *, PAR *);
void	PicBuf_to_X11	(PicBuf *, PAR *);

