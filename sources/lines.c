static void
lines (int relative, FILE * hd)
/**
 ** Examples of anticipated commands:
 **
 **	PA PD0,0,80,50,90,20PU140,30PD150,80;
 **	PU0,0;PD20.53,40.32,30.08,60.2,40,90,;PU100,300;PD120,340...
 **/
{
  HPGL_Pt p;
  int numcmds=0;

  for (;;)
    {
       if (read_float (&p.x, hd)){     /* No number found      */
          if (numcmds >0 ) return;
               if(pen_down){  /*simulate dot created by 'real' pen on PD;PU;*/
                     Pen_action_to_tmpfile   (MOVE_TO, &p_last, scale_flag);
                     Pen_action_to_tmpfile   (DRAW_TO, &p_last, scale_flag);
			}
                return ;
                }                                                                             

      if (read_float (&p.y, hd))	/* x without y invalid! */
	par_err_exit (2, PA);
      line(relative,p);
      numcmds++;
    }      
}

