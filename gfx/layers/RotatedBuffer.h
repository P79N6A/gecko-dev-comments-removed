




#ifndef ROTATEDBUFFER_H_
#define ROTATEDBUFFER_H_

#include "gfxTypes.h"
#include <stdint.h>                     
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
#include "LayersTypes.h"

struct nsIntSize;

namespace mozilla {
namespace gfx {
class Matrix;
}

namespace layers {

class TextureClient;
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
                              gfx::CompositionOp aOperator = gfx::CompositionOp::OP_OVER,
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



class BorrowDrawTarget
{
protected:
  void ReturnDrawTarget(gfx::DrawTarget*& aReturned);

  
  
  
  RefPtr<gfx::DrawTarget> mLoanedDrawTarget;
  gfx::Matrix mLoanedTransform;
};





class RotatedContentBuffer : public RotatedBuffer
                           , public BorrowDrawTarget
{
public:
  typedef gfxContentType ContentType;

  






  enum BufferSizePolicy {
    SizedToVisibleBounds,
    ContainsVisibleBounds
  };

  RotatedContentBuffer(BufferSizePolicy aBufferSizePolicy)
    : mBufferProvider(nullptr)
    , mBufferProviderOnWhite(nullptr)
    , mBufferSizePolicy(aBufferSizePolicy)
  {
    MOZ_COUNT_CTOR(RotatedContentBuffer);
  }
  virtual ~RotatedContentBuffer()
  {
    MOZ_COUNT_DTOR(RotatedContentBuffer);
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
      : mMode(SurfaceMode::SURFACE_NONE)
      , mContentType(gfxContentType::SENTINEL)
      , mDidSelfCopy(false)
    {}

    nsIntRegion mRegionToDraw;
    nsIntRegion mRegionToInvalidate;
    SurfaceMode mMode;
    DrawRegionClip mClip;
    ContentType mContentType;
    bool mDidSelfCopy;
  };

  enum {
    PAINT_WILL_RESAMPLE = 0x01,
    PAINT_NO_ROTATION = 0x02,
    PAINT_CAN_DRAW_ROTATED = 0x04
  };
  



















  PaintState BeginPaint(ThebesLayer* aLayer,
                        uint32_t aFlags);

  struct DrawIterator {
    friend class RotatedContentBuffer;
    friend class ContentClientIncremental;
    DrawIterator()
      : mCount(0)
    {}

    nsIntRegion mDrawRegion;

  private:
    uint32_t mCount;
  };

  














  gfx::DrawTarget* BorrowDrawTargetForPainting(PaintState& aPaintState,
                                               DrawIterator* aIter = nullptr);

  enum {
    ALLOW_REPEAT = 0x01,
    BUFFER_COMPONENT_ALPHA = 0x02 
                                  
  };
  







  virtual void
  CreateBuffer(ContentType aType, const nsIntRect& aRect, uint32_t aFlags,
               RefPtr<gfx::DrawTarget>* aBlackDT, RefPtr<gfx::DrawTarget>* aWhiteDT) = 0;

  




  gfx::DrawTarget* GetDTBuffer() { return mDTBuffer; }
  gfx::DrawTarget* GetDTBufferOnWhite() { return mDTBufferOnWhite; }

  




  void DrawTo(ThebesLayer* aLayer,
              gfx::DrawTarget* aTarget,
              float aOpacity,
              gfx::CompositionOp aOp,
              gfx::SourceSurface* aMask,
              const gfx::Matrix* aMaskTransform);

protected:
  
  void SetBufferProvider(TextureClient* aClient)
  {
    
    
    MOZ_ASSERT(!aClient || !mDTBuffer);

    mBufferProvider = aClient;
    if (!mBufferProvider) {
      mDTBuffer = nullptr;
    }
  }

  void SetBufferProviderOnWhite(TextureClient* aClient)
  {
    
    
    MOZ_ASSERT(!aClient || !mDTBufferOnWhite);

    mBufferProviderOnWhite = aClient;
    if (!mBufferProviderOnWhite) {
      mDTBufferOnWhite = nullptr;
    }
  }

  












  gfx::DrawTarget*
  BorrowDrawTargetForQuadrantUpdate(const nsIntRect& aBounds,
                                    ContextSource aSource,
                                    DrawIterator* aIter);

  static bool IsClippingCheap(gfx::DrawTarget* aTarget, const nsIntRegion& aRegion);

protected:
  



  gfxContentType BufferContentType();
  bool BufferSizeOkFor(const nsIntSize& aSize);
  


  bool EnsureBuffer();
  bool EnsureBufferOnWhite();

  
  void FlushBuffers();

  



  virtual bool HaveBuffer() const;
  virtual bool HaveBufferOnWhite() const;

  






  virtual void FinalizeFrame(const nsIntRegion& aRegionToDraw) {}

  




  TextureClient* mBufferProvider;
  TextureClient* mBufferProviderOnWhite;

  BufferSizePolicy      mBufferSizePolicy;
};

}
}

#endif
