/**
 ** to_gif.c: Graphics Interchange File (GIF) converter part of hp2xx,
 ** cloned from to_png.c (which ironically started out as to_gif ten years 
 ** ago) after expiration of the LZW patent
 **
 ** 04/09/02     V 1.00    MK copied from to_png.c, reversing row/column
 **                           order. Lots of code duplication to allow
 **                           compiling either GIF or PNG support. The
 **                           helper routines for actual GIF writing are
 **                           again copied from pbmplus (now netpbm)
 **/

#include <stdio.h>
#include <stdlib.h>
#include "bresnham.h"
#include "hp2xx.h"
#include "to_gif.h"
#include "pendef.h"
#define GGE >>=
#define MAXOUTPUTROWS 70

#define PDNCOL 256

int pdGImageColorAllocate(pdImagePtr, int, int, int);

int PicBuf_to_GIF(const GEN_PAR * pg, const OUT_PAR * po)
{
	FILE *fd;
	int row_c, /*byte_c, */ x;
	const RowBuf *row;
	const PicBuf *pb;
	int ppm[][3] = { {255, 255, 255}, {0, 0, 0} };
/*, {255,0,0}, {0,255,0},
		{0,0,255},{0,255,255},{255,0,255},{255,255,0}};
*/
	int colour;

/**
 ** gifdraw-parts
 **/
	pdImagePtr im;
	int pdcol;

	if (pg == NULL || po == NULL)
		return ERROR;
	pb = po->picbuf;
	if (pb == NULL)
		return ERROR;

	if (!pg->quiet)
		Eprintf("\nWriting GIF output: %s\n", po->outfile);
	if (*po->outfile != '-') {

/*
#ifdef VAX
	if ((fd = fopen(po->outfile, WRITE_BIN, "rfm=var", "mrs=512")) == NULL)
#else
*/
		if ((fd = fopen(po->outfile, WRITE_BIN)) == NULL)
/*
#endif
*/
			goto ERROR_EXIT;
	} else
		fd = stdout;

/**
 ** create image structure
 **/
	im = pdGImageCreate(pb->nc, pb->nr);

	if (pb->depth > 1) {
/** 
 ** allocate some colors ( ?? eight colors supported by hp2xx ?? )
 **/
		for (colour = 0; colour < PDNCOL; colour++)

			pdcol =
			    pdGImageColorAllocate(im, pt.clut[colour][0],
						 pt.clut[colour][1],
						 pt.clut[colour][2]);
		for (row_c = 0; row_c < pb->nr; row_c++) {
			row = get_RowBuf(pb, pb->nr - row_c - 1);
			if (row == NULL)
				continue;

			for (x = 0; x < pb->nc; x++) {
				colour = index_from_RowBuf(row, x, pb);
				pdGImageSetPixel(im, x, row_c, colour);
			}
			if ((!pg->quiet) && (row_c % 10 == 0))
				/* For the impatients among us ...   */
				Eprintf(".");
		}
	} else {
/** 
 ** allocate two colors ( ?? eight colors supported by hp2xx ?? )
 **/
		for (colour = 0; colour < 2; colour++)
			pdcol =
			    pdGImageColorAllocate(im, ppm[colour][0],
						 ppm[colour][1],
						 ppm[colour][2]);

		for (row_c = 0; row_c < pb->nr; row_c++) {
			row = get_RowBuf(pb, pb->nr - row_c - 1);
			if (row == NULL)
				continue;

			for (x = 0; x < pb->nc; x++) {
				colour = index_from_RowBuf(row, x, pb);
				pdGImageSetPixel(im, x,row_c, colour);
			}

			if ((!pg->quiet) && (row_c % 10 == 0))
				/* For the impatients among us ...   */
				Eprintf(".");
		}
	}

	pdImageGIF(im, fd);

/*	pdImageDestroy(im);*/

	fflush(fd);

	if (!pg->quiet)
		Eprintf("\n");
	if (fd != stdout)
		fclose(fd);
	return 0;

      ERROR_EXIT:
	PError("write_GIF");
	return ERROR;
}

/**
 ** PNG image support routines, derived from:
 **/

/* gd.c: implementation of the gifdraw module. Version 0.9.

        Written by Tom Boutell, 5/94-6/94.
        Copyright 1994, Cold Spring Harbor Labs.
        Permission granted to use this code in any fashion provided
        that this notice is retained and any alterations are
        labeled as such. It is requested, but not required, that
        you share extensions to this module with us so that we
        can incorporate them into new versions. */
/**
 ** PNG: use sy rows of sx pixels (instead of sx columns of sy pixels)
 **/

pdImagePtr pdGImageCreate(sx, sy)
int sx;
int sy;
{
	int i;
	pdImagePtr im;
	im = (pdImage *) malloc(sizeof(pdImage));
	im->pixels =
	    (unsigned char **) malloc(sizeof(unsigned char *) * sy);
	for (i = 0; (i < sy); i++) {
		im->pixels[i] = (unsigned char *) calloc((size_t) sx,
							 sizeof(unsigned
								char));
	}
	im->sx = sx;
	im->sy = sy;
	im->colorsTotal = 0;
	im->transparent = (-1);
	return im;
}

void pdGImageDestroy(im)
pdImagePtr im;
{
	int i;
	for (i = 0; (i < im->sy); i++) {
		free(im->pixels[i]);
	}
	free(im->pixels);
	free(im);
}

int pdGImageColorAllocate(im, r, g, b)
pdImagePtr im;
int r;
int g;
int b;
{
	int i;
	int ct = (-1);
	for (i = 0; (i < (im->colorsTotal)); i++) {
		if (im->open[i]) {
			ct = i;
			break;
		}
	}
	if (ct == (-1)) {
		ct = im->colorsTotal;
		if (ct == pdMaxColors) {
			return -1;
		}
		im->colorsTotal++;
	}
	im->red[ct] = r;
	im->green[ct] = g;
	im->blue[ct] = b;
	im->open[ct] = 0;
	return ct;
}

void pdGImageColorTransparent(im, color)
pdImagePtr im;
unsigned char color;
{
	im->transparent = color;
}

void pdGImageSetPixel(im, x, y, color)
pdImagePtr im;
int x;
int y;
unsigned char color;
{
	if (pdGImageBoundsSafe(im, x, y)) {
		im->pixels[y][x] = color;
	}
}

int pdGImageGetPixel(im, x, y)
pdImagePtr im;
int x;
int y;
{
	if (pdGImageBoundsSafe(im, x, y)) {
		return im->pixels[y][x];
	} else {
		return 0;
	}
}

int pdGImageBoundsSafe(im, x, y)
pdImagePtr im;
int x;
int y;
{
	return (!(((y < 0) || (y >= im->sy)) ||
		  ((x < 0) || (x >= im->sx))));
}
