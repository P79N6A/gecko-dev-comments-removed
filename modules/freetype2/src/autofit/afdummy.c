


















#include "afdummy.h"
#include "afhints.h"
#include "aferrors.h"


  static FT_Error
  af_dummy_hints_init( AF_GlyphHints    hints,
                       AF_StyleMetrics  metrics )
  {
    af_glyph_hints_rescale( hints, metrics );

    hints->x_scale = metrics->scaler.x_scale;
    hints->y_scale = metrics->scaler.y_scale;
    hints->x_delta = metrics->scaler.x_delta;
    hints->y_delta = metrics->scaler.y_delta;

    return FT_Err_Ok;
  }


  static FT_Error
  af_dummy_hints_apply( AF_GlyphHints  hints,
                        FT_Outline*    outline )
  {
    FT_Error  error;


    error = af_glyph_hints_reload( hints, outline );
    if ( !error )
      af_glyph_hints_save( hints, outline );

    return error;
  }


  AF_DEFINE_WRITING_SYSTEM_CLASS(
    af_dummy_writing_system_class,

    AF_WRITING_SYSTEM_DUMMY,

    sizeof ( AF_StyleMetricsRec ),

    (AF_WritingSystem_InitMetricsFunc) NULL,
    (AF_WritingSystem_ScaleMetricsFunc)NULL,
    (AF_WritingSystem_DoneMetricsFunc) NULL,

    (AF_WritingSystem_InitHintsFunc)   af_dummy_hints_init,
    (AF_WritingSystem_ApplyHintsFunc)  af_dummy_hints_apply
  )



