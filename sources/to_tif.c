/* Created by M.Liberi from to_pcx.c */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "bresnham.h"
#include "hp2xx.h"
#include "tiffio.h"

int PicBuf_to_TIF (const GEN_PAR *pg, const OUT_PAR *po)
{
  TIFF		*w=NULL;
  RowBuf	*row=NULL;
  int		x,y, W, H, D, B, S;
  Byte		*tifbuf;
  unsigned short r[256], g[256], b[256];
  char		tmp[16];

  if (!pg->quiet)
    Eprintf ("\nWriting TIFF output\n");
  if (*po->outfile == '-'){
    if (!(tmpnam(tmp))){
      PError ("hp2xx -- error creating temp file");
      return 1;
    }
    if ((w=TIFFOpen(tmp,"w"))==NULL){
      PError ("hp2xx -- opening TIFF temp file");
      return ERROR;
    }
  }
  else
    if ((w=TIFFOpen(po->outfile,"w"))==NULL){
      PError ("hp2xx -- opening output file");
      return ERROR;
    }
  W=po->picbuf->nr;
  H=po->picbuf->nc;
  D=po->picbuf->depth;
  B=po->picbuf->nb;

  TIFFSetField(w, TIFFTAG_IMAGEWIDTH, H);
  TIFFSetField(w, TIFFTAG_IMAGELENGTH,W);
  if (D==1){
    if (pg->Clut[0][0]+pg->Clut[0][1]+pg->Clut[0][2]>0)
      TIFFSetField(w, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
    else
      TIFFSetField(w, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(w, TIFFTAG_BITSPERSAMPLE, 1);
  } else {
    for (x=0; x<pg->maxpens; ++x)
      r[x]=pg->Clut[x][0]<<8|pg->Clut[x][0],
      g[x]=pg->Clut[x][1]<<8|pg->Clut[x][1],
      b[x]=pg->Clut[x][2]<<8|pg->Clut[x][2];
    TIFFSetField(w, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
    TIFFSetField(w, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(w, TIFFTAG_COLORMAP, r, g, b);
  }
  TIFFSetField(w, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(w, TIFFTAG_SAMPLESPERPIXEL, 1);
  TIFFSetField(w, TIFFTAG_ORIENTATION, ORIENTATION_BOTRIGHT);
/* LZW is patented by Unisys - only license holders should use next line*/
/*  TIFFSetField(w, TIFFTAG_COMPRESSION, COMPRESSION_LZW);*/
  TIFFSetField(w, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);

  S=TIFFScanlineSize(w);
  if (!pg->quiet){
    Eprintf("W=%d, H=%d, D=%d, scanlinesize=%d\n", W, H, D, S);
    /*
    for (x=0; x<=pg->maxpens; ++x)
      Eprintf("%d. %d,%d,%d\n",x, pg->Clut[x][0],pg->Clut[x][1],pg->Clut[x][1]);
    */
  }

  if ((tifbuf=malloc(S))==NULL){
    Eprintf("malloc error!\n");
    TIFFClose(w);
    return 1;
  }

  for (y=0; y<W; ++y)
  {
    if ((row=get_RowBuf(po->picbuf, y))==NULL)
      break;
    memset(tifbuf,0,S);
    for (x=0; x<H; ++x){
      Byte C=index_from_RowBuf(row, x, po->picbuf);
      if (D==1)
	switch(C){
	  case xxBackground:
	    break;
	  default:
	    tifbuf[x/8]|=1<<(7-x%8);
	}
      else
	tifbuf[x]=C;
    }
    if (TIFFWriteScanline(w, tifbuf, y, 0) < 0){
      PError("TIFFWriteScanline");
      break;
    }
  }

  free(tifbuf);
  TIFFClose(w);
  if (*po->outfile == '-'){
    int c;
    FILE *r;

    if (!(r=fopen(tmp,"rb"))){
      PError("fopen");
      return ERROR;
    }
    while ((c=fgetc(r))!=EOF)
      fputc(c,stdout);
    fclose(r);
    unlink(tmp);
  }
  return 0;
}
