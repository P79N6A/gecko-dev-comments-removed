






#ifndef mozilla_WidgetUtils_h
#define mozilla_WidgetUtils_h

#include "nsRect.h"
#include "mozilla/gfx/Matrix.h"

namespace mozilla {


enum ScreenRotation {
  ROTATION_0 = 0,
  ROTATION_90,
  ROTATION_180,
  ROTATION_270,

  ROTATION_COUNT
};

gfx::Matrix ComputeTransformForRotation(const nsIntRect& aBounds,
                                        ScreenRotation aRotation);

} 

#endif 
