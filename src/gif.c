/* to_gif.c 
 * MK 04/09/02 
 * derived from (or more realistically - a badly hacked version of)
 * ppmtogif.c - read a portable pixmap and produce a GIF file
 * as included in netpbm-10.8, the successor to pbmplus maintained
 * by Bryan Henderson (see netpbm.sourceforge.net)
 * NOTE: not all code is actually used in hp2xx 
 * the original authorship and copyright statements from ppmtogif.c are
 * given below.
** 
**
** Based on GIFENCOD by David Rowley <mgardi@watdscu.waterloo.edu>.A
** Lempel-Zim compression based on "compress".
**
** Modified by Marcel Wijkstra <wijkstra@fwi.uva.nl>
**
** The non-LZW GIF generation stuff was adapted from the Independent
** JPEG Group's djpeg on 2001.09.29.  The uncompressed output subroutines
** are derived directly from the corresponding subroutines in djpeg's
** wrgif.c source file.  Its copyright notice say:

 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
   The reference README file is README.JPEG in the Netpbm package.
**
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** The Graphics Interchange Format(c) is the Copyright property of
** CompuServe Incorporated.  GIF(sm) is a Service Mark property of
** CompuServe Incorporated.
*/

/* TODO: merge the LZW and uncompressed subroutines.  They are separate
   only because they had two different lineages and the code is too
   complicated for me quickly to rewrite it.
*/

/* end of original ppmtogif header */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bresnham.h"
#include "hp2xx.h"
#include "pendef.h"
#include "to_gif.h"

#define MAXCMAPSIZE 256


/*
 * a code_int must be able to hold 2**BITS values of type int, and also -1
 */
typedef int             code_int;

#ifdef SIGNED_COMPARE_SLOW
typedef unsigned long int count_int;
typedef unsigned short int count_short;
#else /*SIGNED_COMPARE_SLOW*/
typedef long int          count_int;
#endif /*SIGNED_COMPARE_SLOW*/

struct cmap {
    /* This is the information for the GIF colormap (aka palette). */

    int red[MAXCMAPSIZE], green[MAXCMAPSIZE], blue[MAXCMAPSIZE];
        /* These arrays arrays map a color index, as is found in
           the raster part of the GIF, to an intensity value for the indicated
           RGB component.
        */
    int perm[MAXCMAPSIZE], permi[MAXCMAPSIZE];
        /* perm[i] is the position in the sorted colormap of the color which
           is at position i in the unsorted colormap.  permi[] is the inverse
           function of perm[].
        */
    int cmapsize;
        /* Number of entries in the GIF colormap.  I.e. number of colors
           in the image, plus possibly one fake transparency color.
        */
    int transparent;
        /* color index number in GIF palette of the color that is to be
           transparent.  -1 if no color is transparent.
        */
    int maxval;
        /* The maxval for the colors in 'cht'. */
};

struct gif_dest {
    /* This structure controls output of uncompressed GIF raster */

    /* State for packing variable-width codes into a bitstream */
    int n_bits;         /* current number of bits/code */
    int maxcode;        /* maximum code, given n_bits */
    int cur_accum;      /* holds bits not yet output */
    int cur_bits;       /* # of bits in cur_accum */

    /* State for GIF code assignment */
    int ClearCode;      /* clear code (doesn't change) */
    int EOFCode;        /* EOF code (ditto) */
    int code_counter;   /* counts output symbols */
};
typedef struct colorhist_item* colorhist_vector;
struct colorhist_item
    {
        unsigned int color;
            int value;
                };
                
                
static int GetPixel(unsigned char ** const , int const ,
         unsigned int ** const , unsigned int const , 
         struct cmap * const ,
         int const , int const ) ;
static void BumpPixel(void);
static void char_init(void);
static void flush_char(void);
static void cl_hash(count_int);


static __inline__ int sqr(const int x) {
    return x*x;
}


/*
 * Write out a word to the GIF file
 */
static void
Putword(int const w, FILE * const fp) {

    fputc( w & 0xff, fp );
    fputc( (w / 256) & 0xff, fp );
}




static __inline__ int
GetPixel(unsigned char ** const pixels, int const input_maxval,
         unsigned int ** const alpha, unsigned int const alpha_threshold, 
         struct cmap * const cmapP,
         int const x, int const y) {
/*----------------------------------------------------------------------------
   Return the colormap index of the pixel at location (x,y) in the PPM
   raster 'pixels', using colormap *cmapP.
-----------------------------------------------------------------------------*/
    int colorindex;

    if (alpha && alpha[y][x] < alpha_threshold)
        colorindex = cmapP->transparent;
    else {
    (void)input_maxval; /* suppress gcc warning */
#if 0
        int presort_colorindex;
        presort_colorindex = ppm_lookupcolor(cmapP->cht, &pixels[y][x]);
        if (presort_colorindex == -1)
            presort_colorindex = 
                closestcolor(pixels[y][x], input_maxval, cmapP);
        colorindex = cmapP->perm[presort_colorindex];
#endif
    }
    colorindex=pixels[y][x];
    return colorindex;
}


#define TRUE 1
#define FALSE 0

static int Width, Height;
static int curx, cury;
static long CountDown;
static int Pass = 0;
static int Interlace;

/*
 * Bump the 'curx' and 'cury' to point to the next pixel
 */
static void
BumpPixel()
{
        /*
         * Bump the current X position
         */
        ++curx;

        /*
         * If we are at the end of a scan line, set curx back to the beginning
         * If we are interlaced, bump the cury to the appropriate spot,
         * otherwise, just increment it.
         */
        if( curx == Width ) {
                curx = 0;

                if( !Interlace )
                        ++cury;
                else {
                     switch( Pass ) {

                       case 0:
                          cury += 8;
                          if( cury >= Height ) {
                                ++Pass;
                                cury = 4;
                          }
                          break;

                       case 1:
                          cury += 8;
                          if( cury >= Height ) {
                                ++Pass;
                                cury = 2;
                          }
                          break;

                       case 2:
                          cury += 4;
                          if( cury >= Height ) {
                             ++Pass;
                             cury = 1;
                          }
                          break;

                       case 3:
                          cury += 2;
                          break;
                        }
                }
        }
}



static __inline__ int
GIFNextPixel(unsigned char ** const pixels, int const input_maxval,
             unsigned int ** const alpha, unsigned int const alpha_threshold, 
             struct cmap * const cmapP) {
/*----------------------------------------------------------------------------
   Return the pre-sort color index (index into the unsorted GIF color map)
   of the next pixel to be processed from the input image.

   'alpha_threshold' is the gray level such that a pixel in the alpha
   map whose value is less that that represents a transparent pixel
   in the output.
-----------------------------------------------------------------------------*/
    int r;

    if( CountDown == 0 )
        return EOF;
    
    --CountDown;
    
    r = GetPixel(pixels, input_maxval, alpha, alpha_threshold, cmapP, 
                 curx, cury);
    
    BumpPixel();
    
    return r;
}



static void
write_transparent_color_index_extension(FILE *fp, const int Transparent) {
/*----------------------------------------------------------------------------
   Write out extension for transparent color index.
-----------------------------------------------------------------------------*/

    fputc( '!', fp );
    fputc( 0xf9, fp );
    fputc( 4, fp );
    fputc( 1, fp );
    fputc( 0, fp );
    fputc( 0, fp );
    fputc( Transparent, fp );
    fputc( 0, fp );
}



static void
write_comment_extension(FILE *fp, const char comment[]) {
/*----------------------------------------------------------------------------
   Write out extension for a comment
-----------------------------------------------------------------------------*/
    char *segment;
    
    fputc('!', fp);   /* Identifies an extension */
    fputc(0xfe, fp);  /* Identifies a comment */

    /* Write it out in segments no longer than 255 characters */
    for (segment = (char *) comment; 
         segment < comment+strlen(comment); 
         segment += 255) {

        const int length_this_segment = MIN(255, strlen(segment));

        fputc(length_this_segment, fp);

        fwrite(segment, 1, length_this_segment, fp);
    }

    fputc(0, fp);   /* No more comment blocks in this extension */
}



/***************************************************************************
 *
 *  GIFCOMPR.C       - GIF Image compression routines
 *
 *  Lempel-Ziv compression based on 'compress'.  GIF modifications by
 *  David Rowley (mgardi@watdcsu.waterloo.edu)
 *
 ***************************************************************************/

/*
 * General DEFINEs
 */

#define BITS    12

#define HSIZE  5003            /* 80% occupancy */

#ifdef NO_UCHAR
 typedef char   char_type;
#else /*NO_UCHAR*/
 typedef        unsigned char   char_type;
#endif /*NO_UCHAR*/

/*
 *
 * GIF Image compression - modified 'compress'
 *
 * Based on: compress.c - File compression ala IEEE Computer, June 1984.
 *
 * By Authors:  Spencer W. Thomas       (decvax!harpo!utah-cs!utah-gr!thomas)
 *              Jim McKie               (decvax!mcvax!jim)
 *              Steve Davies            (decvax!vax135!petsd!peora!srd)
 *              Ken Turkowski           (decvax!decwrl!turtlevax!ken)
 *              James A. Woods          (decvax!ihnp4!ames!jaw)
 *              Joe Orost               (decvax!vax135!petsd!joe)
 *
 */
#include <ctype.h>

#define ARGVAL() (*++(*argv) || (--argc && *++argv))

static int n_bits;                        /* number of bits/code */
static int maxbits = BITS;                /* user settable max # bits/code */
static code_int maxcode;                  /* maximum code, given n_bits */
static code_int maxmaxcode = (code_int)1 << BITS; /* should NEVER generate this code */
#ifdef COMPATIBLE               /* But wrong! */
# define MAXCODE(n_bits)        ((code_int) 1 << (n_bits) - 1)
#else /*COMPATIBLE*/
# define MAXCODE(n_bits)        (((code_int) 1 << (n_bits)) - 1)
#endif /*COMPATIBLE*/

static count_int htab [HSIZE];
static unsigned short codetab [HSIZE];
#define HashTabOf(i)       htab[i]
#define CodeTabOf(i)    codetab[i]

static code_int hsize = HSIZE;                 /* for dynamic table sizing */

/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**BITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */

#define tab_prefixof(i) CodeTabOf(i)
#define tab_suffixof(i)        ((char_type*)(htab))[i]
#define de_stack               ((char_type*)&tab_suffixof((code_int)1<<BITS))

static code_int free_ent = 0;                  /* first unused entry */

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
static int clear_flg = 0;

static int offset;
static long int in_count = 1;            /* length of input */
static long int out_count = 0;           /* # of codes output (for debugging) */

/*
 * compress stdin to stdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */

static int g_init_bits;
static FILE* g_outfile;

static int ClearCode;
static int EOFCode;

/***************************************************************************
*                          BYTE OUTPUTTER                 
***************************************************************************/

static char accum[256];
    /* The current packet, under construction */
static int a_count;
    /* Number of characters so far in the current packet */

static void
char_init() {
    a_count = 0;
}



static void
flush_char() {
/*----------------------------------------------------------------------------
   Write the current packet to the output file, then reset the current 
   packet to empty.
-----------------------------------------------------------------------------*/
int i;
    if( a_count > 0 ) {
        fputc( a_count, g_outfile );
        i = fwrite( accum, 1, a_count, g_outfile );
	if (i != a_count) fprintf(stderr,"GIFwrite: flushed only %d of %d bytes\n",i,a_count);
        a_count = 0;
	memset(accum,0,256);
    }
}



static void
char_out( int const c ) {
/*----------------------------------------------------------------------------
   Add a character to the end of the current packet, and if it is 254
   character, flush the packet to the output file.
-----------------------------------------------------------------------------*/
    accum[ a_count++ ] = c;
    if( a_count >= 254 )
        flush_char();
}



static unsigned long const masks[] = { 0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
                                       0x001F, 0x003F, 0x007F, 0x00FF,
                                       0x01FF, 0x03FF, 0x07FF, 0x0FFF,
                                       0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };

static void
output( code_int const code, FILE * const outfile ) {
/*----------------------------------------------------------------------------
   Output one GIF code to the file.

   The code is represented as cur_bits+n_bits/8 bytes in the file.

 * Inputs:
 *      code:   A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *              that n_bits =< (long)wordsize - 1.
 * Assumptions:
 *      Chars are 8 bits long.
 * Algorithm:
 *      Maintain a BITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.

-----------------------------------------------------------------------------*/
    /* Note: this initialization of static variables by the loader must go
       if we want to use the compressor more than once per invocation
       (e.g. we want to create a multi-image gif).
    */
    static unsigned long cur_accum = 0;
    static int cur_bits = 0;

    g_outfile = outfile;

    cur_accum &= masks[ cur_bits ];

    if( cur_bits > 0 )
        cur_accum |= ((long)code << cur_bits);
    else
        cur_accum = code;

    cur_bits += n_bits;

    while( cur_bits >= 8 ) {
        char_out( (unsigned int)(cur_accum & 0xff) );
        cur_accum >>= 8;
        cur_bits -= 8;
    }

    /*
     * If the next entry is going to be too big for the code size,
     * then increase it, if possible.
     */
   if ( free_ent > maxcode || clear_flg ) {

            if( clear_flg ) {

                maxcode = MAXCODE (n_bits = g_init_bits);
                clear_flg = 0;

            } else {

                ++n_bits;
                if ( n_bits == maxbits )
                    maxcode = maxmaxcode;
                else
                    maxcode = MAXCODE(n_bits);
            }
        }

    if( code == EOFCode ) {
        /*
         * At EOF, write the rest of the buffer.
         */
        while( cur_bits > 0 ) {
                char_out( (unsigned int)(cur_accum & 0xff) );
                cur_accum >>= 8;
                cur_bits -= 8;  
                    /* ?? what if cur_bits < 8?  Apparently only an issue 
                       if we extend this program to do multiple images */
        }

        flush_char();

        fflush( g_outfile );

        if( ferror( g_outfile ) )
            Eprintf( "error writing output file" );
    }
}



static void
cl_hash(hsize)          /* reset code table */
register count_int hsize;
{

        register count_int *htab_p = htab+hsize;

        register long i;
        register long m1 = -1;

        i = hsize - 16;
        do {                            /* might use Sys V memset(3) here */
                *(htab_p-16) = m1;
                *(htab_p-15) = m1;
                *(htab_p-14) = m1;
                *(htab_p-13) = m1;
                *(htab_p-12) = m1;
                *(htab_p-11) = m1;
                *(htab_p-10) = m1;
                *(htab_p-9) = m1;
                *(htab_p-8) = m1;
                *(htab_p-7) = m1;
                *(htab_p-6) = m1;
                *(htab_p-5) = m1;
                *(htab_p-4) = m1;
                *(htab_p-3) = m1;
                *(htab_p-2) = m1;
                *(htab_p-1) = m1;
                htab_p -= 16;
        } while ((i -= 16) >= 0);

        for ( i += 16; i > 0; --i )
                *--htab_p = m1;
}


/*
 * Clear out the hash table
 */
static void
cl_block (FILE * const outfile)     /* table clear for block compress */
{

        cl_hash ( (count_int) hsize );
        free_ent = ClearCode + 2;
        clear_flg = 1;

        output( (code_int)ClearCode, outfile );
}



static void
write_raster_LZW(unsigned char ** const pixels, int const input_maxval,
                 unsigned int ** const alpha, unsigned int const alpha_maxval, 
                 struct cmap * const cmapP, 
                 int const init_bits, FILE * const outfile) {
/*----------------------------------------------------------------------------
   Write the raster to file 'outfile'.

   The raster to write is 'pixels', which has maxval 'input_maxval',
   modified by alpha mask 'alpha', which has maxval 'alpha_maxval'.

   Use the colormap 'cmapP' to generate the raster ('pixels' is 
   composed of RGB samples; the GIF raster is colormap indices).

   Write the raster using LZW compression.
-----------------------------------------------------------------------------*/

    code_int ent;
    code_int disp;
    code_int hsize_reg;
    int hshift;
    unsigned char eof;
    unsigned int const alpha_threshold = (alpha_maxval + 1) / 2;
        /* gray levels below this in the alpha mask indicate transparent
           pixels in the output image.
        */
    
    /*
     * Set up the globals:  g_init_bits - initial number of bits
     *                      g_outfile   - pointer to output file
     */
    g_init_bits = init_bits;

    /*
     * Set up the necessary values
     */
    offset = 0;
    out_count = 0;
    clear_flg = 0;
    in_count = 1;
    maxcode = MAXCODE(n_bits = g_init_bits);

    ClearCode = (1 << (init_bits - 1));
    EOFCode = ClearCode + 1;
    free_ent = ClearCode + 2;

    char_init();

    ent = GIFNextPixel(pixels, input_maxval, alpha, alpha_threshold, cmapP);

    {
        long fcode;
        hshift = 0;
        for ( fcode = (long) hsize;  fcode < 65536L; fcode *= 2L )
            ++hshift;
        hshift = 8 - hshift;                /* set hash code range bound */
    }
    hsize_reg = hsize;
    cl_hash( (count_int) hsize_reg);            /* clear hash table */

    output( (code_int)ClearCode, outfile );

    eof = FALSE;
    while (!eof) {
        int gifpixel;
            /* The value for the pixel in the GIF image.  I.e. the colormap
               index.  Or -1 to indicate "no more pixels."
            */
        gifpixel = GIFNextPixel(pixels, 
                                input_maxval, alpha, alpha_threshold, cmapP);
        if (gifpixel == EOF) eof = TRUE;
        if (!eof) {
            long const fcode = (long) (((long) gifpixel << maxbits) + ent);
            code_int i;
                /* xor hashing */

            ++in_count;

            i = (((code_int)gifpixel << hshift) ^ ent);    

            if ( HashTabOf (i) == fcode ) {
                ent = CodeTabOf (i);
                continue;
            } else if ( (long)HashTabOf (i) < 0 )      /* empty slot */
                goto nomatch;
            disp = hsize_reg - i;        /* secondary hash (after G. Knott) */
            if ( i == 0 )
                disp = 1;
        probe:
            if ( (i -= disp) < 0 )
                i += hsize_reg;

            if ( HashTabOf (i) == fcode ) {
                ent = CodeTabOf (i);
                continue;
            }
            if ( (long)HashTabOf (i) > 0 )
                goto probe;
        nomatch:
            output ( (code_int) ent, outfile );
            ++out_count;
            ent = gifpixel;
            if ( free_ent < maxmaxcode ) {  /* } */
                CodeTabOf (i) = free_ent++; /* code -> hashtable */
                HashTabOf (i) = fcode;
            } else
                cl_block(outfile);
        }
    }
    /*
     * Put out the final code.
     */
    output( (code_int)ent, outfile );
    ++out_count;
    output( (code_int) EOFCode, outfile );
}



/* Routine to convert variable-width codes into a byte stream */

static void
output_uncompressed(struct gif_dest * const dinfo, int const code,
                    FILE * const outfile) {

    g_outfile = outfile;

    /* Emit a code of n_bits bits */
    /* Uses cur_accum and cur_bits to reblock into 8-bit bytes */
    dinfo->cur_accum |= ((int) code) << dinfo->cur_bits;
    dinfo->cur_bits += dinfo->n_bits;

    while (dinfo->cur_bits >= 8) {
        char_out(dinfo->cur_accum & 0xFF);
        dinfo->cur_accum >>= 8;
        dinfo->cur_bits -= 8;
    }
}


static void
write_raster_uncompressed_init (struct gif_dest * const dinfo, 
                                int const i_bits, FILE * const outfile)
/* Initialize pseudo-compressor */
{
  /* init all the state variables */
  dinfo->n_bits = i_bits;
  dinfo->maxcode = MAXCODE(dinfo->n_bits);
  dinfo->ClearCode = (1 << (i_bits - 1));
  dinfo->EOFCode = dinfo->ClearCode + 1;
  dinfo->code_counter = dinfo->ClearCode + 2;
  /* init output buffering vars */
  dinfo->cur_accum = 0;
  dinfo->cur_bits = 0;
  /* GIF specifies an initial Clear code */
  output_uncompressed(dinfo, dinfo->ClearCode, outfile);
}


static void
write_raster_uncompressed_pixel (struct gif_dest * const dinfo, 
                                 int const c, FILE * const outfile)
/* Accept and "compress" one pixel value.
 * The given value must be less than n_bits wide.
 */
{
  /* Output the given pixel value as a symbol. */
  output_uncompressed(dinfo, c, outfile);
  /* Issue Clear codes often enough to keep the reader from ratcheting up
   * its symbol size.
   */
  if (dinfo->code_counter < dinfo->maxcode) {
    dinfo->code_counter++;
  } else {
    output_uncompressed(dinfo, dinfo->ClearCode, outfile);
    dinfo->code_counter = dinfo->ClearCode + 2;	/* reset the counter */
  }
}


static void
write_raster_uncompressed_term (struct gif_dest * const dinfo,
                                FILE * const outfile)
/* Clean up at end */
{
  /* Send an EOF code */
  output_uncompressed(dinfo, dinfo->EOFCode, outfile);
  /* Flush the bit-packing buffer */
  if (dinfo->cur_bits > 0) {
    char_out(dinfo->cur_accum & 0xFF);
  }
  /* Flush the packet buffer */
  flush_char();
}



static void
write_raster_uncompressed(unsigned char ** const pixels, int const input_maxval,
                          unsigned int ** const alpha, unsigned int const alpha_maxval, 
                          struct cmap * const cmapP, 
                          int const init_bits, FILE * const outfile) {
/*----------------------------------------------------------------------------
   Write the raster to file 'outfile'.
   
   Same as write_raster_LZW(), except written out one code per
   pixel (plus some clear codes), so no compression.  And no use
   of the LZW patent.
-----------------------------------------------------------------------------*/
    unsigned int const alpha_threshold = (alpha_maxval + 1) / 2;
        /* gray levels below this in the alpha mask indicate transparent
           pixels in the output image.
        */
    unsigned char eof;

    struct gif_dest gif_dest;

    write_raster_uncompressed_init(&gif_dest, init_bits, outfile);

    g_outfile = outfile;

    eof = FALSE;
    while (!eof) {
        int gifpixel;
            /* The value for the pixel in the GIF image.  I.e. the colormap
               index.  Or -1 to indicate "no more pixels."
            */
        gifpixel = GIFNextPixel(pixels, 
                                input_maxval, alpha, alpha_threshold, cmapP);
        if (gifpixel == EOF) eof = TRUE;
        if (!eof) {
            write_raster_uncompressed_pixel(&gif_dest, gifpixel, outfile);
        }
    }
    write_raster_uncompressed_term(&gif_dest, outfile);
}



/******************************************************************************
 *
 * GIF Specific routines
 *
 *****************************************************************************/

static void
writeGifHeader(FILE * const fp,
               int const Width, int const Height, int const Background, 
               int const BitsPerPixel, struct cmap * const cmapP,
               const char comment[]) {

    int B;
    int const Resolution = BitsPerPixel;
    int const ColorMapSize = 1 << BitsPerPixel;

    /* Calculate number of bits we are expecting */
    CountDown = (long)Width * (long)Height;

    /* Indicate which pass we are on (if interlace) */
    Pass = 0;

    /* Set up the current x and y position */
    curx = cury = 0;

    /* Write the Magic header */
    if (cmapP->transparent != -1 || comment)
        fwrite("GIF89a", 1, 6, fp);
    else
        fwrite("GIF87a", 1, 6, fp);

    /* Write out the screen width and height */
    Putword( Width, fp );
    Putword( Height, fp );

    /* Indicate that there is a global colour map */
    B = 0x80;       /* Yes, there is a color map */

    /* OR in the resolution */
    B |= (Resolution - 1) << 4;

    /* OR in the Bits per Pixel */
    B |= (BitsPerPixel - 1);

    /* Write it out */
    fputc( B, fp );

    /* Write out the Background color */
    fputc( Background, fp );

    /* Byte of 0's (future expansion) */
    fputc( 0, fp );

    {
        /* Write out the Global Color Map */
        int i;
        for( i=0; i < ColorMapSize; ++i ) {
            fputc( cmapP->red[i], fp );
            fputc( cmapP->green[i], fp );
            fputc( cmapP->blue[i], fp );
        }
    }
        
    if ( cmapP->transparent >= 0 ) 
        write_transparent_color_index_extension(fp, cmapP->transparent);

    if ( comment )
        write_comment_extension(fp, comment);
}



static void
GIFEncode(FILE * const fp, 
          unsigned char ** const pixels, int const input_maxval,
          int const GWidth, int const GHeight, 
          unsigned int ** const alpha, unsigned int const alpha_maxval,
          int const GInterlace, int const Background, 
          int const BitsPerPixel, struct cmap * const cmapP,
          const char comment[], unsigned char const nolzw) {

    int const LeftOfs = 0;
    int const TopOfs = 0;
    int InitCodeSize;
    
    writeGifHeader(fp, GWidth, GHeight, Background,
                   BitsPerPixel, cmapP, comment);

    /* Write an Image separator */
    fputc( ',', fp );

    /* Write the Image header */

    Putword( LeftOfs, fp );
    Putword( TopOfs, fp );
    Putword( GWidth, fp );
    Putword( GHeight, fp );

    /* Write out whether or not the image is interlaced */
    if( GInterlace )
        fputc( 0x40, fp );
    else
        fputc( 0x00, fp );

    /* The initial code size */
    if( BitsPerPixel <= 1 )
        InitCodeSize = 2;
    else
        InitCodeSize = BitsPerPixel;

    /* Write out the initial code size */
    fputc( InitCodeSize, fp );

    /* Set some global variables for BumpPixel() */
    Interlace = GInterlace;
    Width = GWidth;
    Height = GHeight;

    /* Write the actual raster */
    if (nolzw)
        write_raster_uncompressed(pixels, 
                                  input_maxval, alpha, alpha_maxval, cmapP, 
                                  InitCodeSize+1, fp);
    else
        write_raster_LZW(pixels, 
                         input_maxval, alpha, alpha_maxval, cmapP, 
                         InitCodeSize+1, fp);

    /* Write out a Zero-length packet (to end the series) */
    fputc( 0, fp );

    /* Write the GIF file terminator */
    fputc( ';', fp );

    /* And close the file */
/*    fclose( fp );*/
}



#if 0
#define PPM_GETR(p) (((p) & 0x3ff00000) >> 20)
#define PPM_GETG(p) (((p) & 0xffc00) >> 10)
#define PPM_GETB(p) ((p) & 0x3ff)

static void
sort_colormap(int const sort, struct cmap * const cmapP) {
/*----------------------------------------------------------------------------
   Sort (in place) the colormap *cmapP.

   Create the perm[] and permi[] mappings for the colormap.

   'sort' is logical:  true means to sort the colormap by red intensity,
   then by green intensity, then by blue intensity.  False means a null
   sort -- leave it in the same order in which we found it.
-----------------------------------------------------------------------------*/
    int * const Red = cmapP->red;
    int * const Blue = cmapP->blue;
    int * const Green = cmapP->green;
    int * const perm = cmapP->perm;
    int * const permi = cmapP->permi;
    unsigned int const cmapsize = cmapP->cmapsize;
    
    int i;

    for (i=0; i < cmapsize; i++)
        permi[i] = i;

    if (sort) {
        Eprintf("sorting colormap");
        for (i=0; i < cmapsize; i++) {
            int j;
            for (j=i+1; j < cmapsize; j++)
                if (((Red[i]*MAXCMAPSIZE)+Green[i])*MAXCMAPSIZE+Blue[i] >
                    ((Red[j]*MAXCMAPSIZE)+Green[j])*MAXCMAPSIZE+Blue[j]) {
                    int tmp;
                    
                    tmp=permi[i]; permi[i]=permi[j]; permi[j]=tmp;
                    tmp=Red[i]; Red[i]=Red[j]; Red[j]=tmp;
                    tmp=Green[i]; Green[i]=Green[j]; Green[j]=tmp;
                    tmp=Blue[i]; Blue[i]=Blue[j]; Blue[j]=tmp; } }
    }

    for (i=0; i < cmapsize; i++)
        perm[permi[i]] = i;
}



static void
normalize_to_255(colorhist_vector const chv, struct cmap * const cmapP) {
/*----------------------------------------------------------------------------
   With a PPM color histogram vector 'chv' as input, produce a colormap
   of integers 0-255 as output in *cmapP.
-----------------------------------------------------------------------------*/
    int i;
    int const maxval = cmapP->maxval;

    if ( maxval != 255 )
        Eprintf(
            "maxval is not 255 - automatically rescaling colors" );

    for ( i = 0; i < cmapP->cmapsize; ++i ) {
        if ( maxval == 255 ) {
            cmapP->red[i] = (int) PPM_GETR( chv[i].color );
            cmapP->green[i] = (int) PPM_GETG( chv[i].color );
            cmapP->blue[i] = (int) PPM_GETB( chv[i].color );
        } else {
            cmapP->red[i] = (int) PPM_GETR( chv[i].color ) * 255 / maxval;
            cmapP->green[i] = (int) PPM_GETG( chv[i].color ) * 255 / maxval;
            cmapP->blue[i] = (int) PPM_GETB( chv[i].color ) * 255 / maxval;
        }
    }
}
#endif




void pdImageGIF (pdImagePtr im, FILE * ifp) {
#if 0
    int rows, cols;
    unsigned char ** pixels;   
    int input_maxval;  /* Maxval for 'pixels' */
#endif
    int BitsPerPixel;
    unsigned int alpha_maxval=0; /* Maxval for 'alpha' */
    unsigned int ** alpha=NULL;     /* The supplied alpha mask; NULL if none */
int interlace =0;
int nolzw=0;
int i;
char *comment="created by hp2xx";

    struct cmap cmap;
        /* The colormap, with all its accessories */
#if 0
    int fake_transparent;
#endif
        /* colormap index of the fake transparency color we're using to
           implement the alpha mask.  Undefined if we're not doing an alpha
           mask.
        */


#if 0    
      get_alpha(cmdline.alpha_filespec, cols, rows, &alpha, &alpha_maxval);

      compute_ppm_colormap(pixels, cols, rows, input_maxval, (alpha != NULL), 
                           cmdline.mapfile, 
                           &chv, &cmap.cht, &cmap.maxval, &cmap.cmapsize);

    /* Now turn the ppm colormap into the appropriate GIF colormap. */

      normalize_to_255(chv, &cmap);

      ppm_freecolorhist(chv);

    if (alpha) {
        /* Add a fake entry to the end of the colormap for transparency.  
           Make its color black. 
        */
          add_to_colormap(&cmap, cmdline.alphacolor, &fake_transparent);
    }
    sort_colormap(cmdline.sort, &cmap);
#endif

    BitsPerPixel = im->colorsTotal >2 ? 8:1 ;
	for (i=0;i<im->colorsTotal;i++) {
		cmap.red[i]=im->red[i];
		cmap.green[i]=im->green[i];
		cmap.blue[i]=im->blue[i];
		}
#if 0
    if (alpha) {
        cmap.transparent = cmap.perm[fake_transparent];
    } else {
        if (cmdline.transparent)
            cmap.transparent = 
                compute_transparent(cmdline.transparent, &cmap);
        else 
            cmap.transparent = -1;
    }
#endif
	cmap.transparent= im->transparent;    
    /* All set, let's do it. */
    GIFEncode(ifp, im->pixels, im->colorsTotal, im->sx, im->sy, 
              alpha, alpha_maxval, 
              interlace, 0, BitsPerPixel, &cmap, comment,
              nolzw);


    return;
}


