

















#ifndef __HBSHIM_H__
#define __HBSHIM_H__


#include <ft2build.h>
#include FT_FREETYPE_H


#ifdef FT_CONFIG_OPTION_USE_HARFBUZZ

#include <hb.h>
#include <hb-ot.h>
#include <hb-ft.h>

#endif


FT_BEGIN_HEADER

  FT_Error
  af_get_coverage( AF_FaceGlobals  globals,
                   AF_StyleClass   style_class,
                   FT_Byte*        gstyles );

  FT_Error
  af_get_char_index( AF_StyleMetrics  metrics,
                     FT_ULong         charcode,
                     FT_ULong        *codepoint,
                     FT_Long         *y_offset );

 

FT_END_HEADER

#endif 



