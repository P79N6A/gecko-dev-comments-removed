




#ifndef THEBESLAYERBUFFER_H_
#define THEBESLAYERBUFFER_H_

#include "gfxContext.h"
#include "gfxASurface.h"
#include "gfxPlatform.h"
#include "nsRegion.h"

namespace mozilla {
namespace layers {

class AutoOpenSurface;
class ThebesLayer;



















class ThebesLayerBuffer {
public:
  typedef gfxASurface::gfxContentType ContentType;

  






  enum BufferSizePolicy {
    SizedToVisibleBounds,
    ContainsVisibleBounds
  };

  ThebesLayerBuffer(BufferSizePolicy aBufferSizePolicy)
    : mBufferProvider(nullptr)
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
    mBuffer = nullptr;
    mDTBuffer = nullptr;
    mBufferProvider = nullptr;
    mBufferRect.SetEmpty();
  }

  








  struct PaintState {
    PaintState()
      : mDidSelfCopy(false)
    {}

    nsRefPtr<gfxContext> mContext;
    nsIntRegion mRegionToDraw;
    nsIntRegion mRegionToInvalidate;
    bool mDidSelfCopy;
  };

  enum {
    PAINT_WILL_RESAMPLE = 0x01,
    PAINT_NO_ROTATION = 0x02
  };
  















  PaintState BeginPaint(ThebesLayer* aLayer, ContentType aContentType,
                        uint32_t aFlags);

  enum {
    ALLOW_REPEAT = 0x01
  };
  




  virtual already_AddRefed<gfxASurface>
  CreateBuffer(ContentType aType, const nsIntSize& aSize, uint32_t aFlags) = 0;

  


  virtual TemporaryRef<gfx::DrawTarget>
  CreateDrawTarget(const gfx::IntSize& aSize, gfx::SurfaceFormat aFormat)
  {
    return gfxPlatform::GetPlatform()->CreateOffscreenDrawTarget(aSize, aFormat);
  }

  


  TemporaryRef<gfx::DrawTarget>
  CreateDrawTarget(const nsIntSize& aSize, ContentType aContent) 
  {
    gfx::SurfaceFormat format = gfx::SurfaceFormatForImageFormat(
      gfxPlatform::GetPlatform()->OptimalFormatForContent(aContent));
    return CreateDrawTarget(gfx::IntSize(aSize.width, aSize.height), format);
  }

  




  gfx::DrawTarget* GetDT() { return mDTBuffer; }
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
                          float aOpacity,
                          gfxASurface* aMask,
                          const gfxMatrix* aMaskTransform);
  void DrawBufferWithRotation(gfxContext* aTarget, float aOpacity,
                              gfxASurface* aMask = nullptr,
                              const gfxMatrix* aMaskTransform = nullptr);

  




  const nsIntRect& BufferRect() const { return mBufferRect; }
  const nsIntPoint& BufferRotation() const { return mBufferRotation; }

  already_AddRefed<gfxASurface>
  SetBuffer(gfxASurface* aBuffer,
            const nsIntRect& aBufferRect, const nsIntPoint& aBufferRotation)
  {
    nsRefPtr<gfxASurface> tmp = mBuffer.forget();
    mBuffer = aBuffer;
    mBufferRect = aBufferRect;
    mBufferRotation = aBufferRotation;
    return tmp.forget();
  }

  TemporaryRef<gfx::DrawTarget>
  SetDT(gfx::DrawTarget* aBuffer,
            const nsIntRect& aBufferRect, const nsIntPoint& aBufferRotation)
  {
    RefPtr<gfx::DrawTarget> tmp = mDTBuffer.forget();
    mDTBuffer = aBuffer;
    mBufferRect = aBufferRect;
    mBufferRotation = aBufferRotation;
    return tmp.forget();
  }

  










  void SetBufferProvider(AutoOpenSurface* aProvider)
  {
    mBufferProvider = aProvider;
    if (!mBufferProvider) {
      mBuffer = nullptr;
      mDTBuffer = nullptr;
    } else {
      
      
      MOZ_ASSERT(!mBuffer);
    }
  }

  



  already_AddRefed<gfxContext>
  GetContextForQuadrantUpdate(const nsIntRect& aBounds);

private:
  
  

  



  gfxASurface::gfxContentType BufferContentType();
  bool BufferSizeOkFor(const nsIntSize& aSize);
  


  void EnsureBuffer();

  


  bool BufferValid() {
    return mBuffer || mDTBuffer;
  }

  



  bool HaveBuffer();

  RefPtr<gfx::DrawTarget> mDTBuffer;
  nsRefPtr<gfxASurface> mBuffer;
  



  AutoOpenSurface* mBufferProvider;
  
  nsIntRect             mBufferRect;
  









  nsIntPoint            mBufferRotation;
  BufferSizePolicy      mBufferSizePolicy;
};

}
}

#endif
