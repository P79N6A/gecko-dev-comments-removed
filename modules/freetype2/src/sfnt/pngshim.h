

















#ifndef __PNGSHIM_H__
#define __PNGSHIM_H__


#include <ft2build.h>
#include "ttload.h"


FT_BEGIN_HEADER

#ifdef FT_CONFIG_OPTION_USE_PNG

  FT_LOCAL( FT_Error )
  Load_SBit_Png( FT_Bitmap*       map,
                 FT_Int           x_offset,
                 FT_Int           y_offset,
                 FT_Int           pix_bits,
                 TT_SBit_Metrics  metrics,
                 FT_Memory        memory,
                 FT_Byte*         data,
                 FT_UInt          png_len );

#endif

FT_END_HEADER

#endif 



