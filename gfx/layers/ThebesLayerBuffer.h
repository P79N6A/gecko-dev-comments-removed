




































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
  typedef gfxASurface::gfxContentType ContentType;

  






  enum BufferSizePolicy {
    SizedToVisibleBounds,
    ContainsVisibleBounds
  };

  ThebesLayerBuffer(BufferSizePolicy aBufferSizePolicy)
    : mBufferDims(0,0)
    , mBufferRotation(0,0)
    , mBufferSizePolicy(aBufferSizePolicy)
  {
    MOZ_COUNT_CTOR(ThebesLayerBuffer);
  }
  virtual ~ThebesLayerBuffer()
  {
    MOZ_COUNT_DTOR(ThebesLayerBuffer);
  }

  



  void Clear()
  {
    mBuffer = nsnull;
    mBufferDims.SizeTo(0, 0);
    mBufferRect.SetEmpty();
  }

  








  struct PaintState {
    nsRefPtr<gfxContext> mContext;
    nsIntRegion mRegionToDraw;
    nsIntRegion mRegionToInvalidate;
    PRPackedBool mDidSelfCopy;
  };

  enum {
    PAINT_WILL_RESAMPLE = 0x01
  };
  















  PaintState BeginPaint(ThebesLayer* aLayer, ContentType aContentType,
                        float aXResolution, float aYResolution,
                        PRUint32 aFlags);

  enum {
    ALLOW_REPEAT = 0x01
  };
  




  virtual already_AddRefed<gfxASurface>
  CreateBuffer(ContentType aType, const nsIntSize& aSize, PRUint32 aFlags) = 0;

  




  gfxASurface* GetBuffer() { return mBuffer; }

protected:
  enum XSide {
    LEFT, RIGHT
  };
  enum YSide {
    TOP, BOTTOM
  };
  nsIntRect GetQuadrantRectangle(XSide aXSide, YSide aYSide);
  void DrawBufferQuadrant(gfxContext* aTarget, XSide aXSide, YSide aYSide,
                          float aOpacity, float aXRes, float aYRes);
  void DrawBufferWithRotation(gfxContext* aTarget, float aOpacity,
                              float aXRes, float aYRes);

  










  const nsIntSize& BufferDims() const { return mBufferDims; }
  const nsIntRect& BufferRect() const { return mBufferRect; }
  const nsIntPoint& BufferRotation() const { return mBufferRotation; }

  already_AddRefed<gfxASurface>
  SetBuffer(gfxASurface* aBuffer, const nsIntSize& aBufferDims,
            const nsIntRect& aBufferRect, const nsIntPoint& aBufferRotation)
  {
    nsRefPtr<gfxASurface> tmp = mBuffer.forget();
    mBuffer = aBuffer;
    mBufferDims = aBufferDims;
    mBufferRect = aBufferRect;
    mBufferRotation = aBufferRotation;
    return tmp.forget();
  }

  



  already_AddRefed<gfxContext>
  GetContextForQuadrantUpdate(const nsIntRect& aBounds,
                              float aXResolution, float aYResolution);

private:
  PRBool BufferSizeOkFor(const nsIntSize& aSize)
  {
    return (aSize == mBufferDims ||
            (SizedToVisibleBounds != mBufferSizePolicy &&
             aSize < mBufferDims));
  }

  nsRefPtr<gfxASurface> mBuffer;
  




  nsIntSize             mBufferDims;
  
  nsIntRect             mBufferRect;
  









  nsIntPoint            mBufferRotation;
  BufferSizePolicy      mBufferSizePolicy;
};

}
}

#endif
