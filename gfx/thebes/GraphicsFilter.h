




#ifndef GraphicsFilter_h
#define GraphicsFilter_h

#include "mozilla/TypedEnum.h"

enum class GraphicsFilter : int {
  FILTER_FAST,
  FILTER_GOOD,
  FILTER_BEST,
  FILTER_NEAREST,
  FILTER_BILINEAR,
  FILTER_GAUSSIAN,
  FILTER_SENTINEL
};

#endif

