



#ifndef mozilla_PaintTracker_h
#define mozilla_PaintTracker_h

#include "mozilla/Attributes.h"
#include "nsDebug.h"

namespace mozilla {

class MOZ_STACK_CLASS PaintTracker
{
public:
  PaintTracker() {
    ++gPaintTracker;
  }
  ~PaintTracker() {
    NS_ASSERTION(gPaintTracker > 0, "Mismatched constructor/destructor");
    --gPaintTracker;
  }

  static bool IsPainting() {
    return !!gPaintTracker;
  }

private:
  static int gPaintTracker;
};

} 

#endif 
