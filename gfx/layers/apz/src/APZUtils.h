





#ifndef mozilla_layers_APZUtils_h
#define mozilla_layers_APZUtils_h

#include <stdint.h>                     
#include "mozilla/gfx/Point.h"
#include "mozilla/FloatingPoint.h"

namespace mozilla {
namespace layers {

enum HitTestResult {
  HitNothing,
  HitLayer,
  HitDispatchToContentRegion,
};

enum CancelAnimationFlags : uint32_t {
  Default = 0,            
  ExcludeOverscroll = 1   
};

enum class ScrollSource {
  
  DOM,

  
  Touch,

  
  Wheel
};

typedef uint32_t TouchBehaviorFlags;

template <typename Units>
static bool IsZero(const gfx::PointTyped<Units>& aPoint)
{
  return FuzzyEqualsAdditive(aPoint.x, 0.0f)
      && FuzzyEqualsAdditive(aPoint.y, 0.0f);
}

}
}

#endif
