

















#ifndef __AFCJK_H__
#define __AFCJK_H__

#include "afhints.h"
#include "aflatin.h"


FT_BEGIN_HEADER


  

  AF_DECLARE_WRITING_SYSTEM_CLASS( af_cjk_writing_system_class )


  
  
  
  
  
  
  


  





#define AF_CJK_IS_TOP_BLUE( b ) \
          ( (b)->properties & AF_BLUE_PROPERTY_CJK_TOP )
#define AF_CJK_IS_HORIZ_BLUE( b ) \
          ( (b)->properties & AF_BLUE_PROPERTY_CJK_HORIZ )
#define AF_CJK_IS_FILLED_BLUE( b ) \
          ( (b)->properties & AF_BLUE_PROPERTY_CJK_FILL )
#define AF_CJK_IS_RIGHT_BLUE  AF_CJK_IS_TOP_BLUE

#define AF_CJK_MAX_WIDTHS  16


  enum
  {
    AF_CJK_BLUE_ACTIVE     = 1 << 0,  
    AF_CJK_BLUE_TOP        = 1 << 1,  
    AF_CJK_BLUE_ADJUSTMENT = 1 << 2,  
                                      
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
    AF_CJKBlueRec  blues[AF_BLUE_STRINGSET_MAX];

    FT_Fixed       org_scale;
    FT_Pos         org_delta;

  } AF_CJKAxisRec, *AF_CJKAxis;


  typedef struct  AF_CJKMetricsRec_
  {
    AF_StyleMetricsRec  root;
    FT_UInt             units_per_em;
    AF_CJKAxisRec       axis[AF_DIMENSION_MAX];

  } AF_CJKMetricsRec, *AF_CJKMetrics;


#ifdef AF_CONFIG_OPTION_CJK
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
                              FT_Face        face );
#endif 




FT_END_HEADER

#endif 



