

















#include "aftypes.h"
#include "aflatin.h"


#ifdef AF_CONFIG_OPTION_INDIC

#include "afindic.h"
#include "aferrors.h"
#include "afcjk.h"


#ifdef AF_CONFIG_OPTION_USE_WARPER
#include "afwarp.h"
#endif


  static FT_Error
  af_indic_metrics_init( AF_CJKMetrics  metrics,
                         FT_Face        face )
  {
    
    FT_CharMap  oldmap = face->charmap;


    metrics->units_per_em = face->units_per_EM;

    if ( FT_Select_Charmap( face, FT_ENCODING_UNICODE ) )
      face->charmap = NULL;
    else
    {
      af_cjk_metrics_init_widths( metrics, face, 0x7530 );
#if 0
      
      af_cjk_metrics_init_blues( metrics, face, af_cjk_blue_chars );
#endif
      af_cjk_metrics_check_digits( metrics, face );
    }

    FT_Set_Charmap( face, oldmap );

    return AF_Err_Ok;
  }


  static void
  af_indic_metrics_scale( AF_CJKMetrics  metrics,
                          AF_Scaler      scaler )
  {
    
    af_cjk_metrics_scale( metrics, scaler );
  }


  static FT_Error
  af_indic_hints_init( AF_GlyphHints  hints,
                       AF_CJKMetrics  metrics )
  {
    
    return af_cjk_hints_init( hints, metrics );
  }


  static FT_Error
  af_indic_hints_apply( AF_GlyphHints  hints,
                        FT_Outline*    outline,
                        AF_CJKMetrics  metrics )
  {
    
    return af_cjk_hints_apply( hints, outline, metrics );
  }


  
  
  
  
  
  
  


  static const AF_Script_UniRangeRec  af_indic_uniranges[] =
  {
#if 0
    AF_UNIRANGE_REC( 0x0100UL, 0xFFFFUL ),  
#endif
    AF_UNIRANGE_REC( 0x0900UL, 0x0DFFUL),    
    AF_UNIRANGE_REC( 0x0F00UL, 0x0FFFUL),    
    AF_UNIRANGE_REC( 0x1900UL, 0x194FUL),    
    AF_UNIRANGE_REC( 0x1B80UL, 0x1BBFUL),    
    AF_UNIRANGE_REC( 0x1C80UL, 0x1CDFUL),    
    AF_UNIRANGE_REC( 0xA800UL, 0xA82FUL),    
    AF_UNIRANGE_REC( 0x11800UL, 0x118DFUL),  
    AF_UNIRANGE_REC(      0UL,      0UL)
  };


  AF_DEFINE_SCRIPT_CLASS( af_indic_script_class,
    AF_SCRIPT_INDIC,
    af_indic_uniranges,

    sizeof ( AF_CJKMetricsRec ),

    (AF_Script_InitMetricsFunc) af_indic_metrics_init,
    (AF_Script_ScaleMetricsFunc)af_indic_metrics_scale,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   af_indic_hints_init,
    (AF_Script_ApplyHintsFunc)  af_indic_hints_apply
  )

#else 

  static const AF_Script_UniRangeRec  af_indic_uniranges[] =
  {
    { 0, 0 }
  };


  AF_DEFINE_SCRIPT_CLASS( af_indic_script_class,
    AF_SCRIPT_INDIC,
    af_indic_uniranges,

    sizeof ( AF_CJKMetricsRec ),

    (AF_Script_InitMetricsFunc) NULL,
    (AF_Script_ScaleMetricsFunc)NULL,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   NULL,
    (AF_Script_ApplyHintsFunc)  NULL
  )

#endif 



