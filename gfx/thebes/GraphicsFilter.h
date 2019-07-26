




#ifndef GraphicsFilter_h
#define GraphicsFilter_h

#include "mozilla/TypedEnum.h"

MOZ_BEGIN_ENUM_CLASS(GraphicsFilter, int)
  FILTER_FAST,
  FILTER_GOOD,
  FILTER_BEST,
  FILTER_NEAREST,
  FILTER_BILINEAR,
  FILTER_GAUSSIAN,
  FILTER_SENTINEL
MOZ_END_ENUM_CLASS(GraphicsFilter)

#endif

