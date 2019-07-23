

















#ifndef __AFCJK_H__
#define __AFCJK_H__

#include "afhints.h"


FT_BEGIN_HEADER


  

  FT_CALLBACK_TABLE const AF_ScriptClassRec
  af_cjk_script_class;


  FT_LOCAL( FT_Error )
  af_cjk_metrics_init( AF_LatinMetrics  metrics,
                       FT_Face          face );

  FT_LOCAL( void )
  af_cjk_metrics_scale( AF_LatinMetrics  metrics,
                        AF_Scaler        scaler );

  FT_LOCAL( FT_Error )
  af_cjk_hints_init( AF_GlyphHints    hints,
                     AF_LatinMetrics  metrics );

  FT_LOCAL( FT_Error )
  af_cjk_hints_apply( AF_GlyphHints    hints,
                      FT_Outline*      outline,
                      AF_LatinMetrics  metrics );



FT_END_HEADER

#endif 



