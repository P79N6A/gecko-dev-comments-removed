




#ifndef DrawMode_h
#define DrawMode_h

#include "mozilla/TypedEnum.h"


MOZ_BEGIN_ENUM_CLASS(DrawMode, int)
  
  
  GLYPH_FILL = 1,
  
  GLYPH_STROKE = 2,
  
  
  GLYPH_PATH = 4,
  
  
  GLYPH_STROKE_UNDERNEATH = 8
MOZ_END_ENUM_CLASS(DrawMode)

#endif

