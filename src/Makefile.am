# what flags you want to pass to the C compiler & linker
AM_CFLAGS = --pedantic -Wall -std=c99 -O2
AM_LDFLAGS =

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
bin_PROGRAMS = hp2xx
COMMON_INCS	= hp2xx.h bresnham.h murphy.h
ALL_INCS	= $(COMMON_INCS) chardraw.h charset0.h charset5.h charset7.h \
		getopt.h x11.h ilbm.h iff.h clip.h
SRCS	= clip.c fillpoly.c\
	  hp2xx.c hpgl.c picbuf.c bresnham.c murphy.c chardraw.c getopt.c getopt1.c \
	  $(ALL_INCS)\
	  to_vec.c to_pcx.c to_pcl.c to_eps.c to_ps.c to_img.c to_pbm.c  \
	  to_rgip.c to_fig.c std_main.c $(PREVIEWER).c pendef.c lindef.c $(EX_SRC)
hp2xx_SOURCES = $(SRCS)
#targetbinary1_SOURCES = targetbinary1.c myheader.h [...]
#targetbinary2_SOURCES = targetbinary2.c
#targetbinaryN_SOURCES = targetbinaryN.c

