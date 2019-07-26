




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
#include "nsISupportsImpl.h"            
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsTraceRefcnt.h"              

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

  RotatedBuffer(gfxASurface* aBuffer, gfxASurface* aBufferOnWhite,
                const nsIntRect& aBufferRect,
                const nsIntPoint& aBufferRotation)
    : mBuffer(aBuffer)
    , mBufferOnWhite(aBufferOnWhite)
    , mBufferRect(aBufferRect)
    , mBufferRotation(aBufferRotation)
  { }
  RotatedBuffer(gfx::DrawTarget* aDTBuffer, gfx::DrawTarget* aDTBufferOnWhite,
                const nsIntRect& aBufferRect,
                const nsIntPoint& aBufferRotation)
    : mDTBuffer(aDTBuffer)
    , mDTBufferOnWhite(aDTBufferOnWhite)
    , mBufferRect(aBufferRect)
    , mBufferRotation(aBufferRotation)
  { }
  RotatedBuffer() { }

  


  enum ContextSource {
    BUFFER_BLACK, 
    BUFFER_WHITE, 
    BUFFER_BOTH 
  };
  void DrawBufferWithRotation(gfxContext* aTarget, ContextSource aSource,
                              float aOpacity = 1.0,
                              gfxASurface* aMask = nullptr,
                              const gfxMatrix* aMaskTransform = nullptr) const;

  void DrawBufferWithRotation(gfx::DrawTarget* aTarget, ContextSource aSource,
                              float aOpacity = 1.0,
                              gfx::CompositionOp aOperator = gfx::OP_OVER,
                              gfx::SourceSurface* aMask = nullptr,
                              const gfx::Matrix* aMaskTransform = nullptr) const;

  




  const nsIntRect& BufferRect() const { return mBufferRect; }
  const nsIntPoint& BufferRotation() const { return mBufferRotation; }

  virtual bool HaveBuffer() const { return mBuffer || mDTBuffer; }
  virtual bool HaveBufferOnWhite() const { return mBufferOnWhite || mDTBufferOnWhite; }

protected:

  enum XSide {
    LEFT, RIGHT
  };
  enum YSide {
    TOP, BOTTOM
  };
  nsIntRect GetQuadrantRectangle(XSide aXSide, YSide aYSide) const;

  




  void DrawBufferQuadrant(gfxContext* aTarget, XSide aXSide, YSide aYSide,
                          ContextSource aSource,
                          float aOpacity,
                          gfxASurface* aMask,
                          const gfxMatrix* aMaskTransform) const;
  void DrawBufferQuadrant(gfx::DrawTarget* aTarget, XSide aXSide, YSide aYSide,
                          ContextSource aSource,
                          float aOpacity,
                          gfx::CompositionOp aOperator,
                          gfx::SourceSurface* aMask,
                          const gfx::Matrix* aMaskTransform) const;

  nsRefPtr<gfxASurface> mBuffer;
  nsRefPtr<gfxASurface> mBufferOnWhite;
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
    mBuffer = nullptr;
    mBufferOnWhite = nullptr;
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
               gfxASurface** aBlackSurface, gfxASurface** aWhiteSurface,
               RefPtr<gfx::DrawTarget>* aBlackDT, RefPtr<gfx::DrawTarget>* aWhiteDT) = 0;
  virtual bool SupportsAzureContent() const 
  { return false; }

  




  gfxASurface* GetBuffer() { return mBuffer; }
  gfxASurface* GetBufferOnWhite() { return mBufferOnWhite; }
  gfx::DrawTarget* GetDTBuffer() { return mDTBuffer; }
  gfx::DrawTarget* GetDTBufferOnWhite() { return mDTBufferOnWhite; }

  




  void DrawTo(ThebesLayer* aLayer, gfxContext* aTarget, float aOpacity,
              gfxASurface* aMask, const gfxMatrix* aMaskTransform);

protected:
  
  bool IsAzureBuffer();

  already_AddRefed<gfxASurface>
  SetBuffer(gfxASurface* aBuffer,
            const nsIntRect& aBufferRect, const nsIntPoint& aBufferRotation)
  {
    MOZ_ASSERT(!SupportsAzureContent());
    nsRefPtr<gfxASurface> tmp = mBuffer.forget();
    mBuffer = aBuffer;
    mBufferRect = aBufferRect;
    mBufferRotation = aBufferRotation;
    return tmp.forget();
  }

  already_AddRefed<gfxASurface>
  SetBufferOnWhite(gfxASurface* aBuffer)
  {
    MOZ_ASSERT(!SupportsAzureContent());
    nsRefPtr<gfxASurface> tmp = mBufferOnWhite.forget();
    mBufferOnWhite = aBuffer;
    return tmp.forget();
  }

  TemporaryRef<gfx::DrawTarget>
  SetDTBuffer(gfx::DrawTarget* aBuffer,
            const nsIntRect& aBufferRect, const nsIntPoint& aBufferRotation)
  {
    MOZ_ASSERT(SupportsAzureContent());
    RefPtr<gfx::DrawTarget> tmp = mDTBuffer.forget();
    mDTBuffer = aBuffer;
    mBufferRect = aBufferRect;
    mBufferRotation = aBufferRotation;
    return tmp.forget();
  }

  TemporaryRef<gfx::DrawTarget>
  SetDTBufferOnWhite(gfx::DrawTarget* aBuffer)
  {
    MOZ_ASSERT(SupportsAzureContent());
    RefPtr<gfx::DrawTarget> tmp = mDTBufferOnWhite.forget();
    mDTBufferOnWhite = aBuffer;
    return tmp.forget();
  }

  










  void SetBufferProvider(DeprecatedTextureClient* aClient)
  {
    
    
    MOZ_ASSERT(!aClient || (!mBuffer && !mDTBuffer));

    mBufferProvider = aClient;
    if (!mBufferProvider) {
      mBuffer = nullptr;
      mDTBuffer = nullptr;
    } 
  }
  
  void SetBufferProviderOnWhite(DeprecatedTextureClient* aClient)
  {
    
    
    MOZ_ASSERT(!aClient || (!mBufferOnWhite && !mDTBufferOnWhite));

    mBufferProviderOnWhite = aClient;
    if (!mBufferProviderOnWhite) {
      mBufferOnWhite = nullptr;
      mDTBufferOnWhite = nullptr;
    } 
  }

  





  already_AddRefed<gfxContext>
  GetContextForQuadrantUpdate(const nsIntRect& aBounds, ContextSource aSource, nsIntPoint* aTopLeft = nullptr);

  static bool IsClippingCheap(gfxContext* aTarget, const nsIntRegion& aRegion);

protected:
  
  

  



  gfxContentType BufferContentType();
  bool BufferSizeOkFor(const nsIntSize& aSize);
  


  void EnsureBuffer();
  void EnsureBufferOnWhite();
  



  virtual bool HaveBuffer() const;
  virtual bool HaveBufferOnWhite() const;

  




  DeprecatedTextureClient* mBufferProvider;
  DeprecatedTextureClient* mBufferProviderOnWhite;

  BufferSizePolicy      mBufferSizePolicy;
};

}
}

#endif
