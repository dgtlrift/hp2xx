###########################################################################
#    Copyright (c) 1991 - 1993 Heinz W. Werntges.  All rights reserved.
#    Distributed by Free Software Foundation, Inc.
#
# This file is part of HP2xx.
#
# HP2xx is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
# to anyone for the consequences of using it or for whether it serves any
# particular purpose or works at all, unless he says so in writing.  Refer
# to the GNU General Public License, Version 2 or later, for full details
#
# Everyone is granted permission to copy, modify and redistribute
# HP2xx, but only under the conditions described in the GNU General Public
# License.  A copy of this license is supposed to have been
# given to you along with HP2xx so you can know your rights and
# responsibilities.  It should be in a file named COPYING.  Among other
# things, the copyright notice and this notice must be preserved on all
# copies.
###########################################################################
#
# Makefile for BORLAND C++ version of hp2xx
#
.AUTODEPEND

#		*Translator Definitions*

# There are two versions available which you select by un-commenting
# three lines and commenting out another three lines just below this
# text. Remember to EITHER comment out the standard version OR the other.

# HERCULES (HGC) patches:
#
#   o  Replace all HAS_DOS_VGA with HAS_DOS_HGC
#   o  Replace all to_vga...  with to_hgc...

# Standard version:

CC           = bcc +HP2XX.CFG -DDOS -DHAS_DOS_VGA
OBJ_EXTRAS   =
EXE_d_extras =

# Extended version, including modes PIC and PAC:
#
# CC           = bcc +HP2XX.CFG -DDOS -DHAS_DOS_VGA -DPIC_PAC
# OBJ_EXTRAS   = to_pic.obj + to_pac.obj +
# EXE_d_extras = to_pic.obj to_pac.obj

# No user-serviceable part below!
#############################################################################




TASM         = tasm
TLINK        = tlink


all:	hp2xx.exe

#		*Implicit Rules*
.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

#		*List Macros*


EXE_dependencies =  \
  hp2xx.obj \
  hpgl.obj \
  picbuf.obj \
  to_pbm.obj \
  to_img.obj \
  to_rgip.obj \
  to_mf.obj \
  to_pcl.obj \
  to_pcx.obj \
  to_eps.obj \
  to_vga.obj \
  bresnham.obj \
  getopt.obj \
  getopt1.obj \
  chardraw.obj \
  $(EXE_d_extras)

#		*Explicit Rules*
hp2xx.exe: hp2xx.cfg $(EXE_d_extras) $(EXE_dependencies)
  $(TLINK) /v/x/c/P-/LC:\BC\LIB;C:\BC\LIB\LOCAL c0l.obj+ hp2xx.obj+ \
  $(OBJ_EXTRAS) @&&|
hpgl.obj+
picbuf.obj+
to_pbm.obj+
to_rgip.obj+
to_img.obj+
to_mf.obj+
to_pcl.obj+
to_pcx.obj+
to_eps.obj+
to_vga.obj+
bresnham.obj+
getopt.obj+
getopt1.obj+
chardraw.obj
hp2xx
		# no map file
emu.lib+
mathl.lib+
cl.lib
|


#		*Individual File Dependencies*
hp2xx.obj: hp2xx.c

hpgl.obj: hpgl.c

picbuf.obj: picbuf.c

to_pbm.obj: to_pbm.c

to_rgip.obj: to_rgip.c

to_img.obj: to_img.c

to_mf.obj: to_mf.c

to_pcl.obj: to_pcl.c

to_pac.obj: to_pac.c

to_pic.obj: to_pic.c

to_pcx.obj: to_pcx.c

to_eps.obj: to_eps.c

to_vga.obj: to_vga.c

bresnham.obj: bresnham.c

getopt.obj: getopt.c

getopt1.obj: getopt1.c

chardraw.obj: chardraw.c

#		*Compiler Configuration File*
hp2xx.cfg: borland.mak
  copy &&|
-ml
-a
-O
-Z
-d
-H=HP2XX.SYM
-wpin
-wamb
-wamp
-wasm
-wpro
-wcln
-wdef
-wsig
-wnod
-wstv
-wucp
-wuse
-IC:\BC\INCLUDE;C:\BC\INCLUDE\SHARE
-LC:\BC\LIB;C:\BC\LIB\LOCAL
| hp2xx.cfg

