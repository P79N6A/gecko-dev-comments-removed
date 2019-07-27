




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
#include "nsRegion.h"                   
#include "LayersTypes.h"

namespace mozilla {
namespace gfx {
class Matrix;
}

namespace layers {

class TextureClient;
class PaintedLayer;
















class RotatedBuffer {
public:
  typedef gfxContentType ContentType;

  RotatedBuffer(const gfx::IntRect& aBufferRect,
                const gfx::IntPoint& aBufferRotation)
    : mBufferRect(aBufferRect)
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

  




  const gfx::IntRect& BufferRect() const { return mBufferRect; }
  const gfx::IntPoint& BufferRotation() const { return mBufferRotation; }

  virtual bool HaveBuffer() const = 0;
  virtual bool HaveBufferOnWhite() const = 0;

  virtual TemporaryRef<gfx::SourceSurface> GetSourceSurface(ContextSource aSource) const = 0;

protected:

  enum XSide {
    LEFT, RIGHT
  };
  enum YSide {
    TOP, BOTTOM
  };
  gfx::IntRect GetQuadrantRectangle(XSide aXSide, YSide aYSide) const;

  gfx::Rect GetSourceRectangle(XSide aXSide, YSide aYSide) const;

  




  void DrawBufferQuadrant(gfx::DrawTarget* aTarget, XSide aXSide, YSide aYSide,
                          ContextSource aSource,
                          float aOpacity,
                          gfx::CompositionOp aOperator,
                          gfx::SourceSurface* aMask,
                          const gfx::Matrix* aMaskTransform) const;

  
  gfx::IntRect             mBufferRect;
  









  gfx::IntPoint            mBufferRotation;
  
  
  bool                  mDidSelfCopy;
};

class SourceRotatedBuffer : public RotatedBuffer
{
public:
  SourceRotatedBuffer(gfx::SourceSurface* aSource, gfx::SourceSurface* aSourceOnWhite,
                      const gfx::IntRect& aBufferRect,
                      const gfx::IntPoint& aBufferRotation)
    : RotatedBuffer(aBufferRect, aBufferRotation)
    , mSource(aSource)
    , mSourceOnWhite(aSourceOnWhite)
  { }

  virtual TemporaryRef<gfx::SourceSurface> GetSourceSurface(ContextSource aSource) const;

  virtual bool HaveBuffer() const { return !!mSource; }
  virtual bool HaveBufferOnWhite() const { return !!mSourceOnWhite; }

private:
  RefPtr<gfx::SourceSurface> mSource;
  RefPtr<gfx::SourceSurface> mSourceOnWhite;
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

  explicit RotatedContentBuffer(BufferSizePolicy aBufferSizePolicy)
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
      : mRegionToDraw()
      , mRegionToInvalidate()
      , mMode(SurfaceMode::SURFACE_NONE)
      , mClip(DrawRegionClip::NONE)
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
  



















  PaintState BeginPaint(PaintedLayer* aLayer,
                        uint32_t aFlags);

  struct DrawIterator {
    friend class RotatedContentBuffer;
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
    BUFFER_COMPONENT_ALPHA = 0x02 
                                  
  };
  






  virtual void
  CreateBuffer(ContentType aType, const gfx::IntRect& aRect, uint32_t aFlags,
               RefPtr<gfx::DrawTarget>* aBlackDT, RefPtr<gfx::DrawTarget>* aWhiteDT) = 0;

  




  gfx::DrawTarget* GetDTBuffer() { return mDTBuffer; }
  gfx::DrawTarget* GetDTBufferOnWhite() { return mDTBufferOnWhite; }

  virtual TemporaryRef<gfx::SourceSurface> GetSourceSurface(ContextSource aSource) const;

  




  void DrawTo(PaintedLayer* aLayer,
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
  BorrowDrawTargetForQuadrantUpdate(const gfx::IntRect& aBounds,
                                    ContextSource aSource,
                                    DrawIterator* aIter);

  static bool IsClippingCheap(gfx::DrawTarget* aTarget, const nsIntRegion& aRegion);

protected:
  



  gfxContentType BufferContentType();
  bool BufferSizeOkFor(const gfx::IntSize& aSize);
  


  bool EnsureBuffer();
  bool EnsureBufferOnWhite();

  
  void FlushBuffers();

  



  virtual bool HaveBuffer() const;
  virtual bool HaveBufferOnWhite() const;

  






  virtual void FinalizeFrame(const nsIntRegion& aRegionToDraw) {}

  RefPtr<gfx::DrawTarget> mDTBuffer;
  RefPtr<gfx::DrawTarget> mDTBufferOnWhite;

  




  TextureClient* mBufferProvider;
  TextureClient* mBufferProviderOnWhite;

  BufferSizePolicy      mBufferSizePolicy;
};

}
}

#endif
