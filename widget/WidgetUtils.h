






#ifndef mozilla_WidgetUtils_h
#define mozilla_WidgetUtils_h

#include "gfxMatrix.h"

namespace mozilla {


enum ScreenRotation {
  ROTATION_0 = 0,
  ROTATION_90,
  ROTATION_180,
  ROTATION_270,

  ROTATION_COUNT
};

gfxMatrix ComputeTransformForRotation(const nsIntRect& aBounds,
                                      ScreenRotation aRotation);

} 

#endif 
