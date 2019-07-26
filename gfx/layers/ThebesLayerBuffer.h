




#ifndef THEBESLAYERBUFFER_H_
#define THEBESLAYERBUFFER_H_

#include <stdint.h>                     
#include "gfxASurface.h"                
#include "gfxContext.h"                 
#include "mozilla/Assertions.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/2D.h"             
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsTraceRefcnt.h"              
#include "LayersTypes.h"

struct gfxMatrix;
struct nsIntSize;

namespace mozilla {
namespace gfx {
class Matrix;
}

namespace layers {

class DeprecatedTextureClient;
class ThebesLayer;
















class RotatedBuffer {
public:
  typedef gfxContentType ContentType;

  RotatedBuffer(gfx::DrawTarget* aDTBuffer, gfx::DrawTarget* aDTBufferOnWhite,
                const nsIntRect& aBufferRect,
                const nsIntPoint& aBufferRotation)
    : mDTBuffer(aDTBuffer)
    , mDTBufferOnWhite(aDTBufferOnWhite)
    , mBufferRect(aBufferRect)
    , mBufferRotation(aBufferRotation)
    , mDidSelfCopy(false)
  { }
  RotatedBuffer()
    : mDidSelfCopy(false)
  { }

  


  enum ContextSource {
    BUFFER_BLACK, 
    BUFFER_WHITE, 
    BUFFER_BOTH 
  };
  void DrawBufferWithRotation(gfx::DrawTarget* aTarget, ContextSource aSource,
                              float aOpacity = 1.0,
                              gfx::CompositionOp aOperator = gfx::OP_OVER,
                              gfx::SourceSurface* aMask = nullptr,
                              const gfx::Matrix* aMaskTransform = nullptr) const;

  




  const nsIntRect& BufferRect() const { return mBufferRect; }
  const nsIntPoint& BufferRotation() const { return mBufferRotation; }

  virtual bool HaveBuffer() const { return mDTBuffer; }
  virtual bool HaveBufferOnWhite() const { return mDTBufferOnWhite; }

protected:

  enum XSide {
    LEFT, RIGHT
  };
  enum YSide {
    TOP, BOTTOM
  };
  nsIntRect GetQuadrantRectangle(XSide aXSide, YSide aYSide) const;

  gfx::Rect GetSourceRectangle(XSide aXSide, YSide aYSide) const;

  




  void DrawBufferQuadrant(gfx::DrawTarget* aTarget, XSide aXSide, YSide aYSide,
                          ContextSource aSource,
                          float aOpacity,
                          gfx::CompositionOp aOperator,
                          gfx::SourceSurface* aMask,
                          const gfx::Matrix* aMaskTransform) const;

  RefPtr<gfx::DrawTarget> mDTBuffer;
  RefPtr<gfx::DrawTarget> mDTBufferOnWhite;
  
  nsIntRect             mBufferRect;
  









  nsIntPoint            mBufferRotation;
  
  
  bool                  mDidSelfCopy;
};





class ThebesLayerBuffer : public RotatedBuffer {
public:
  typedef gfxContentType ContentType;

  






  enum BufferSizePolicy {
    SizedToVisibleBounds,
    ContainsVisibleBounds
  };

  ThebesLayerBuffer(BufferSizePolicy aBufferSizePolicy)
    : mBufferProvider(nullptr)
    , mBufferProviderOnWhite(nullptr)
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
    mDTBuffer = nullptr;
    mDTBufferOnWhite = nullptr;
    mBufferProvider = nullptr;
    mBufferProviderOnWhite = nullptr;
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
    DrawRegionClip mClip;
  };

  enum {
    PAINT_WILL_RESAMPLE = 0x01,
    PAINT_NO_ROTATION = 0x02
  };
  















  PaintState BeginPaint(ThebesLayer* aLayer, ContentType aContentType,
                        uint32_t aFlags);

  enum {
    ALLOW_REPEAT = 0x01,
    BUFFER_COMPONENT_ALPHA = 0x02 
                                  
  };
  







  virtual void
  CreateBuffer(ContentType aType, const nsIntRect& aRect, uint32_t aFlags,
               RefPtr<gfx::DrawTarget>* aBlackDT, RefPtr<gfx::DrawTarget>* aWhiteDT) = 0;

  




  gfx::DrawTarget* GetDTBuffer() { return mDTBuffer; }
  gfx::DrawTarget* GetDTBufferOnWhite() { return mDTBufferOnWhite; }

  




  void DrawTo(ThebesLayer* aLayer, gfxContext* aTarget, float aOpacity,
              gfxASurface* aMask, const gfxMatrix* aMaskTransform);

protected:
  TemporaryRef<gfx::DrawTarget>
  SetDTBuffer(gfx::DrawTarget* aBuffer,
              const nsIntRect& aBufferRect,
              const nsIntPoint& aBufferRotation)
  {
    RefPtr<gfx::DrawTarget> tmp = mDTBuffer.forget();
    mDTBuffer = aBuffer;
    mBufferRect = aBufferRect;
    mBufferRotation = aBufferRotation;
    return tmp.forget();
  }

  TemporaryRef<gfx::DrawTarget>
  SetDTBufferOnWhite(gfx::DrawTarget* aBuffer)
  {
    RefPtr<gfx::DrawTarget> tmp = mDTBufferOnWhite.forget();
    mDTBufferOnWhite = aBuffer;
    return tmp.forget();
  }

  










  void SetBufferProvider(DeprecatedTextureClient* aClient)
  {
    
    
    MOZ_ASSERT(!aClient || !mDTBuffer);

    mBufferProvider = aClient;
    if (!mBufferProvider) {
      mDTBuffer = nullptr;
    } 
  }
  
  void SetBufferProviderOnWhite(DeprecatedTextureClient* aClient)
  {
    
    
    MOZ_ASSERT(!aClient || !mDTBufferOnWhite);

    mBufferProviderOnWhite = aClient;
    if (!mBufferProviderOnWhite) {
      mDTBufferOnWhite = nullptr;
    } 
  }

  





  already_AddRefed<gfxContext>
  GetContextForQuadrantUpdate(const nsIntRect& aBounds, ContextSource aSource, nsIntPoint* aTopLeft = nullptr);

  static bool IsClippingCheap(gfxContext* aTarget, const nsIntRegion& aRegion);

protected:
  



  gfxContentType BufferContentType();
  bool BufferSizeOkFor(const nsIntSize& aSize);
  


  bool EnsureBuffer();
  bool EnsureBufferOnWhite();
  



  virtual bool HaveBuffer() const;
  virtual bool HaveBufferOnWhite() const;

  




  DeprecatedTextureClient* mBufferProvider;
  DeprecatedTextureClient* mBufferProviderOnWhite;

  BufferSizePolicy      mBufferSizePolicy;
};

}
}

#endif
