




































#ifndef MOZILLA_GFX_TOOLS_H_
#define MOZILLA_GFX_TOOLS_H_

#include "Types.h"

namespace mozilla {
namespace gfx {

static inline bool
IsOperatorBoundByMask(CompositionOp aOp) {
  switch (aOp) {
  case OP_IN:
  case OP_OUT:
  case OP_DEST_IN:
  case OP_DEST_ATOP:
  case OP_SOURCE:
    return false;
  default:
    return true;
  }
}

template <class T>
struct ClassStorage
{
  char bytes[sizeof(T)];

  const T *addr() const { return (const T *)bytes; }
  T *addr() { return (T *)(void *)bytes; }
};

static inline bool
FuzzyEqual(Float aA, Float aB, Float aErr)
{
  if ((aA + aErr > aB) && (aA - aErr < aB)) {
    return true;
  }
  return false;
}

static inline Float
Distance(Point aA, Point aB)
{
  return hypotf(aB.x - aA.x, aB.y - aA.y);
}

}
}

#endif 
