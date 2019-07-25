

















#ifndef __AFCJK_H__
#define __AFCJK_H__

#include "afhints.h"
#include "aflatin.h"


FT_BEGIN_HEADER


  

  AF_DECLARE_SCRIPT_CLASS(af_cjk_script_class)

  

  




  enum
  {
    AF_CJK_BLUE_TOP,
    AF_CJK_BLUE_BOTTOM,
    AF_CJK_BLUE_LEFT,
    AF_CJK_BLUE_RIGHT,

    AF_CJK_BLUE_MAX
  };


#define AF_CJK_MAX_WIDTHS  16
#define AF_CJK_MAX_BLUES   AF_CJK_BLUE_MAX


  enum
  {
    AF_CJK_BLUE_ACTIVE     = 1 << 0,
    AF_CJK_BLUE_IS_TOP     = 1 << 1,
    AF_CJK_BLUE_IS_RIGHT   = 1 << 2,
    AF_CJK_BLUE_ADJUSTMENT = 1 << 3,  
                                      
    AF_CJK_BLUE_FLAG_MAX
  };


  typedef struct  AF_CJKBlueRec_
  {
    AF_WidthRec  ref;
    AF_WidthRec  shoot; 
    FT_UInt      flags;

  } AF_CJKBlueRec, *AF_CJKBlue;


  typedef struct  AF_CJKAxisRec_
  {
    FT_Fixed       scale;
    FT_Pos         delta;

    FT_UInt        width_count;
    AF_WidthRec    widths[AF_CJK_MAX_WIDTHS];
    FT_Pos         edge_distance_threshold;
    FT_Pos         standard_width;
    FT_Bool        extra_light;

    
    FT_Bool        control_overshoot;
    FT_UInt        blue_count;
    AF_CJKBlueRec  blues[AF_CJK_BLUE_MAX];

    FT_Fixed       org_scale;
    FT_Pos         org_delta;

  } AF_CJKAxisRec, *AF_CJKAxis;


  typedef struct  AF_CJKMetricsRec_
  {
    AF_ScriptMetricsRec  root;
    FT_UInt              units_per_em;
    AF_CJKAxisRec        axis[AF_DIMENSION_MAX];

  } AF_CJKMetricsRec, *AF_CJKMetrics;


  FT_LOCAL( FT_Error )
  af_cjk_metrics_init( AF_CJKMetrics  metrics,
                       FT_Face        face );

  FT_LOCAL( void )
  af_cjk_metrics_scale( AF_CJKMetrics  metrics,
                        AF_Scaler      scaler );

  FT_LOCAL( FT_Error )
  af_cjk_hints_init( AF_GlyphHints  hints,
                     AF_CJKMetrics  metrics );

  FT_LOCAL( FT_Error )
  af_cjk_hints_apply( AF_GlyphHints  hints,
                      FT_Outline*    outline,
                      AF_CJKMetrics  metrics );

  
  FT_LOCAL( void )
  af_cjk_metrics_check_digits( AF_CJKMetrics  metrics,
                               FT_Face        face );

  FT_LOCAL( void )
  af_cjk_metrics_init_widths( AF_CJKMetrics  metrics,
                              FT_Face        face,
                              FT_ULong       charcode );




FT_END_HEADER

#endif 



