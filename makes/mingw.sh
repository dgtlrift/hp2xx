export PATH=/home/martin/mingw/i386-mingw32/bin:$PATH
gcc -v -I. bresnham.c chardraw.c clip.c fillpoly.c getopt1.c getopt.c hp2xx.c hpgl.c lindef.c murphy.c no_prev.c  pendef.c picbuf.c  std_main.c to_fig.c  to_eps.c to_pcl.c to_vec.c to_ps.c to_rgip.c to_pcx.c to_img.c to_pbm.c -lm -o hp2xx.exe 
