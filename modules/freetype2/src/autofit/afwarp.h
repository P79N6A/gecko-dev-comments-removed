

















#ifndef __AFWARP_H__
#define __AFWARP_H__

#include "afhints.h"

FT_BEGIN_HEADER

#define AF_WARPER_SCALE

#define AF_WARPER_FLOOR( x )  ( (x) & ~63 )
#define AF_WARPER_CEIL( x )   AF_WARPER_FLOOR( (x) + 63 )


  typedef FT_Int32  AF_WarpScore;

  typedef struct  AF_WarperRec_
  {
    FT_Pos        x1, x2;
    FT_Pos        t1, t2;
    FT_Pos        x1min, x1max;
    FT_Pos        x2min, x2max;
    FT_Pos        w0, wmin, wmax;

    FT_Fixed      best_scale;
    FT_Pos        best_delta;
    AF_WarpScore  best_score;
    AF_WarpScore  best_distort;

  } AF_WarperRec, *AF_Warper;


  FT_LOCAL( void )
  af_warper_compute( AF_Warper      warper,
                     AF_GlyphHints  hints,
                     AF_Dimension   dim,
                     FT_Fixed      *a_scale,
                     FT_Fixed      *a_delta );


FT_END_HEADER


#endif 



