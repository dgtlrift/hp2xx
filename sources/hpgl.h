#include "lindef.h"
#include "pendef.h"
#include "bresnham.h"
#include "hp2xx.h"

int read_float (float *pnum, FILE * hd);

void PlotCmd_to_tmpfile (PlotCmd cmd);

void HPGL_Pt_to_tmpfile (const HPGL_Pt * pf);

#ifdef __TURBOC__
#define	HYPOT(x,y)	sqrt((x)*(x)+(y)*(y))
#else
#define	HYPOT(x,y)	hypot(x,y)
#endif

extern short silent_mode;

extern long vec_cntr_w;
extern short record_off;
extern FILE *td;
