

















#ifndef __AFLATIN_H__
#define __AFLATIN_H__

#include "afhints.h"


FT_BEGIN_HEADER


  

  FT_CALLBACK_TABLE const AF_ScriptClassRec
  af_latin_script_class;



#define AF_LATIN_CONSTANT( metrics, c ) \
  ( ( (c) * (FT_Long)( (AF_LatinMetrics)(metrics) )->units_per_em ) / 2048 )


  
  
  
  
  
  
  


  






  

  enum
  {
    AF_LATIN_BLUE_CAPITAL_TOP,
    AF_LATIN_BLUE_CAPITAL_BOTTOM,
    AF_LATIN_BLUE_SMALL_F_TOP,
    AF_LATIN_BLUE_SMALL_TOP,
    AF_LATIN_BLUE_SMALL_BOTTOM,
    AF_LATIN_BLUE_SMALL_MINOR,

    AF_LATIN_BLUE_MAX
  };


#define AF_LATIN_IS_TOP_BLUE( b )  ( (b) == AF_LATIN_BLUE_CAPITAL_TOP || \
                                     (b) == AF_LATIN_BLUE_SMALL_F_TOP || \
                                     (b) == AF_LATIN_BLUE_SMALL_TOP   )

#define AF_LATIN_MAX_WIDTHS  16
#define AF_LATIN_MAX_BLUES   AF_LATIN_BLUE_MAX


  enum
  {
    AF_LATIN_BLUE_ACTIVE     = 1 << 0,
    AF_LATIN_BLUE_TOP        = 1 << 1,
    AF_LATIN_BLUE_ADJUSTMENT = 1 << 2,  
                                        
    AF_LATIN_BLUE_FLAG_MAX
  };


  typedef struct  AF_LatinBlueRec_
  {
    AF_WidthRec  ref;
    AF_WidthRec  shoot;
    FT_UInt      flags;

  } AF_LatinBlueRec, *AF_LatinBlue;


  typedef struct  AF_LatinAxisRec_
  {
    FT_Fixed         scale;
    FT_Pos           delta;

    FT_UInt          width_count;
    AF_WidthRec      widths[AF_LATIN_MAX_WIDTHS];
    FT_Pos           edge_distance_threshold;
    FT_Pos           standard_width;
    FT_Bool          extra_light;

    
    FT_Bool          control_overshoot;
    FT_UInt          blue_count;
    AF_LatinBlueRec  blues[AF_LATIN_BLUE_MAX];

    FT_Fixed         org_scale;
    FT_Pos           org_delta;

  } AF_LatinAxisRec, *AF_LatinAxis;


  typedef struct  AF_LatinMetricsRec_
  {
    AF_ScriptMetricsRec  root;
    FT_UInt              units_per_em;
    AF_LatinAxisRec      axis[AF_DIMENSION_MAX];

  } AF_LatinMetricsRec, *AF_LatinMetrics;


  FT_LOCAL( FT_Error )
  af_latin_metrics_init( AF_LatinMetrics  metrics,
                         FT_Face          face );

  FT_LOCAL( void )
  af_latin_metrics_scale( AF_LatinMetrics  metrics,
                          AF_Scaler        scaler );

  FT_LOCAL( void )
  af_latin_metrics_init_widths( AF_LatinMetrics  metrics,
                                FT_Face          face,
                                FT_ULong         charcode );


  
  
  
  
  
  
  

  enum
  {
    AF_LATIN_HINTS_HORZ_SNAP   = 1 << 0, 
    AF_LATIN_HINTS_VERT_SNAP   = 1 << 1, 
    AF_LATIN_HINTS_STEM_ADJUST = 1 << 2, 
                                         
    AF_LATIN_HINTS_MONO        = 1 << 3  
                                         
  };


#define AF_LATIN_HINTS_DO_HORZ_SNAP( h )             \
  AF_HINTS_TEST_OTHER( h, AF_LATIN_HINTS_HORZ_SNAP )

#define AF_LATIN_HINTS_DO_VERT_SNAP( h )             \
  AF_HINTS_TEST_OTHER( h, AF_LATIN_HINTS_VERT_SNAP )

#define AF_LATIN_HINTS_DO_STEM_ADJUST( h )             \
  AF_HINTS_TEST_OTHER( h, AF_LATIN_HINTS_STEM_ADJUST )

#define AF_LATIN_HINTS_DO_MONO( h )             \
  AF_HINTS_TEST_OTHER( h, AF_LATIN_HINTS_MONO )


  



  FT_LOCAL( FT_Error )
  af_latin_hints_compute_segments( AF_GlyphHints  hints,
                                   AF_Dimension   dim );

  



  FT_LOCAL( void )
  af_latin_hints_link_segments( AF_GlyphHints  hints,
                                AF_Dimension   dim );

  



  FT_LOCAL( FT_Error )
  af_latin_hints_compute_edges( AF_GlyphHints  hints,
                                AF_Dimension   dim );

  FT_LOCAL( FT_Error )
  af_latin_hints_detect_features( AF_GlyphHints  hints,
                                  AF_Dimension   dim );



FT_END_HEADER

#endif 



