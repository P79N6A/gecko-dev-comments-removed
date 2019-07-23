

















#include "aftypes.h"
#include "aflatin.h"


#ifdef AF_CONFIG_OPTION_INDIC

#include "afindic.h"
#include "aferrors.h"
#include "afcjk.h"


#ifdef AF_USE_WARPER
#include "afwarp.h"
#endif


  static FT_Error
  af_indic_metrics_init( AF_LatinMetrics  metrics,
                         FT_Face          face )
  {
    
    return af_cjk_metrics_init( metrics, face );
  }


  static void
  af_indic_metrics_scale( AF_LatinMetrics  metrics,
                          AF_Scaler        scaler )
  {
    
    af_cjk_metrics_scale( metrics, scaler );
  }


  static FT_Error
  af_indic_hints_init( AF_GlyphHints    hints,
                       AF_LatinMetrics  metrics )
  {
    
    return af_cjk_hints_init( hints, metrics );
  }


  static FT_Error
  af_indic_hints_apply( AF_GlyphHints    hints,
                        FT_Outline*      outline,
                        AF_LatinMetrics  metrics)
  {
    
    return af_cjk_hints_apply( hints, outline, metrics );
  }


  
  
  
  
  
  
  


  static const AF_Script_UniRangeRec  af_indic_uniranges[] =
  {
#if 0
    { 0x0100,  0xFFFF },  
#endif
    { 0x0900, 0x0DFF},    
    { 0,       0 }
  };


  FT_CALLBACK_TABLE_DEF const AF_ScriptClassRec
  af_indic_script_class =
  {
    AF_SCRIPT_INDIC,
    af_indic_uniranges,

    sizeof( AF_LatinMetricsRec ),

    (AF_Script_InitMetricsFunc) af_indic_metrics_init,
    (AF_Script_ScaleMetricsFunc)af_indic_metrics_scale,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   af_indic_hints_init,
    (AF_Script_ApplyHintsFunc)  af_indic_hints_apply
  };

#else 

  static const AF_Script_UniRangeRec  af_indic_uniranges[] =
  {
    { 0, 0 }
  };


  FT_CALLBACK_TABLE_DEF const AF_ScriptClassRec
  af_indic_script_class =
  {
    AF_SCRIPT_INDIC,
    af_indic_uniranges,

    sizeof( AF_LatinMetricsRec ),

    (AF_Script_InitMetricsFunc) NULL,
    (AF_Script_ScaleMetricsFunc)NULL,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   NULL,
    (AF_Script_ApplyHintsFunc)  NULL
  };

#endif 



