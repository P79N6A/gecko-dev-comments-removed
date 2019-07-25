




































#ifndef MOZILLA_GFX_GRADIENTSTOPSD2D_H_
#define MOZILLA_GFX_GRADIENTSTOPSD2D_H_

#include "2D.h"

#include <D2D1.h>

namespace mozilla {
namespace gfx {

class GradientStopsD2D : public GradientStops
{
public:
  GradientStopsD2D(ID2D1GradientStopCollection *aStopCollection)
    : mStopCollection(aStopCollection)
  {}

  virtual BackendType GetBackendType() const { return BACKEND_DIRECT2D; }

private:
  friend class DrawTargetD2D;

  mutable RefPtr<ID2D1GradientStopCollection> mStopCollection;
};

}
}

#endif 
