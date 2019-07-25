




































#ifndef THEBESLAYERBUFFER_H_
#define THEBESLAYERBUFFER_H_

#include "gfxContext.h"
#include "gfxASurface.h"
#include "nsRegion.h"

namespace mozilla {
namespace layers {

class ThebesLayer;



















class ThebesLayerBuffer {
public:
  ThebesLayerBuffer() : mBufferRotation(0,0)
  {
    MOZ_COUNT_CTOR(ThebesLayerBuffer);
  }
  ~ThebesLayerBuffer()
  {
    MOZ_COUNT_DTOR(ThebesLayerBuffer);
  }

  



  void Clear()
  {
    mBuffer = nsnull;
    mBufferRect.Empty();
  }

  







  struct PaintState {
    nsRefPtr<gfxContext> mContext;
    nsIntRegion mRegionToDraw;
    nsIntRegion mRegionToInvalidate;
  };
  



  enum {
    OPAQUE_CONTENT = 0x01
  };
  







  PaintState BeginPaint(ThebesLayer* aLayer, gfxContext* aTarget,
                        PRUint32 aFlags);
  



  void DrawTo(ThebesLayer* aLayer, PRUint32 aFlags, gfxContext* aTarget);

protected:
  enum XSide {
    LEFT, RIGHT
  };
  enum YSide {
    TOP, BOTTOM
  };
  nsIntRect GetQuadrantRectangle(XSide aXSide, YSide aYSide);
  void DrawBufferQuadrant(gfxContext* aTarget, XSide aXSide, YSide aYSide);
  void DrawBufferWithRotation(gfxContext* aTarget);

private:
  nsRefPtr<gfxASurface> mBuffer;
  
  nsIntRect             mBufferRect;
  









  nsIntPoint            mBufferRotation;
};

}
}

#endif
