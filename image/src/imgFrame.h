





#ifndef imgFrame_h
#define imgFrame_h

#include "mozilla/MemoryReporting.h"
#include "mozilla/Mutex.h"
#include "mozilla/VolatileBuffer.h"
#include "gfxDrawable.h"
#include "imgIContainer.h"

namespace mozilla {
namespace image {

class ImageRegion;

class imgFrame
{
  typedef gfx::Color Color;
  typedef gfx::DataSourceSurface DataSourceSurface;
  typedef gfx::DrawTarget DrawTarget;
  typedef gfx::IntSize IntSize;
  typedef gfx::SourceSurface SourceSurface;
  typedef gfx::SurfaceFormat SurfaceFormat;

public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(imgFrame)
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(imgFrame)

  imgFrame();

  nsresult Init(int32_t aX, int32_t aY, int32_t aWidth, int32_t aHeight, SurfaceFormat aFormat, uint8_t aPaletteDepth = 0);
  nsresult Optimize();

  bool Draw(gfxContext* aContext, const ImageRegion& aRegion,
            const nsIntMargin& aPadding, GraphicsFilter aFilter,
            uint32_t aImageFlags);

  nsresult ImageUpdated(const nsIntRect &aUpdateRect);

  nsIntRect GetRect() const;
  IntSize GetSize() const { return mSize; }
  int32_t GetStride() const;
  SurfaceFormat GetFormat() const;
  bool GetNeedsBackground() const;
  uint32_t GetImageBytesPerRow() const;
  uint32_t GetImageDataLength() const;
  bool GetIsPaletted() const;
  bool GetHasAlpha() const;
  void GetImageData(uint8_t **aData, uint32_t *length) const;
  uint8_t* GetImageData() const;
  void GetPaletteData(uint32_t **aPalette, uint32_t *length) const;
  uint32_t* GetPaletteData() const;

  int32_t GetRawTimeout() const;
  void SetRawTimeout(int32_t aTimeout);

  int32_t GetFrameDisposalMethod() const;
  void SetFrameDisposalMethod(int32_t aFrameDisposalMethod);
  int32_t GetBlendMethod() const;
  void SetBlendMethod(int32_t aBlendMethod);
  bool ImageComplete() const;

  void SetHasNoAlpha();
  void SetAsNonPremult(bool aIsNonPremult);

  bool GetCompositingFailed() const;
  void SetCompositingFailed(bool val);

  nsresult LockImageData();
  nsresult UnlockImageData();

  void SetDiscardable();

  TemporaryRef<SourceSurface> GetSurface();
  TemporaryRef<DrawTarget> GetDrawTarget();

  Color
  SinglePixelColor()
  {
    return mSinglePixelColor;
  }

  bool IsSinglePixel()
  {
    return mSinglePixel;
  }

  TemporaryRef<SourceSurface> CachedSurface();

  size_t SizeOfExcludingThisWithComputedFallbackIfHeap(
           gfxMemoryLocation aLocation,
           MallocSizeOf aMallocSizeOf) const;

  uint8_t GetPaletteDepth() const { return mPaletteDepth; }
  uint32_t PaletteDataLength() const {
    if (!mPaletteDepth)
      return 0;

    return ((1 << mPaletteDepth) * sizeof(uint32_t));
  }

private: 

  ~imgFrame();

  struct SurfaceWithFormat {
    nsRefPtr<gfxDrawable> mDrawable;
    SurfaceFormat mFormat;
    SurfaceWithFormat() {}
    SurfaceWithFormat(gfxDrawable* aDrawable, SurfaceFormat aFormat)
     : mDrawable(aDrawable), mFormat(aFormat) {}
    bool IsValid() { return !!mDrawable; }
  };

  SurfaceWithFormat SurfaceForDrawing(bool               aDoPadding,
                                      bool               aDoPartialDecode,
                                      bool               aDoTile,
                                      gfxContext*        aContext,
                                      const nsIntMargin& aPadding,
                                      gfxRect&           aImageRect,
                                      ImageRegion&       aRegion,
                                      SourceSurface*     aSurface);

private: 
  RefPtr<DataSourceSurface> mImageSurface;
  RefPtr<SourceSurface> mOptSurface;

  IntSize      mSize;
  nsIntPoint   mOffset;

  nsIntRect    mDecoded;

  mutable Mutex mDecodedMutex;

  
  
  
  
  uint8_t*     mPalettedImageData;

  
  Color        mSinglePixelColor;

  int32_t      mTimeout; 
  int32_t      mDisposalMethod;

  
  int32_t mLockCount;

  RefPtr<VolatileBuffer> mVBuf;
  VolatileBufferPtr<uint8_t> mVBufPtr;

  SurfaceFormat mFormat;
  uint8_t      mPaletteDepth;
  int8_t       mBlendMethod;
  bool mSinglePixel;
  bool mCompositingFailed;
  bool mNonPremult;
  bool mDiscardable;

  
  bool mInformedDiscardTracker;
};

  
  
  class AutoFrameLocker
  {
  public:
    explicit AutoFrameLocker(imgFrame* frame)
      : mFrame(frame)
      , mSucceeded(NS_SUCCEEDED(frame->LockImageData()))
    {}

    ~AutoFrameLocker()
    {
      if (mSucceeded) {
        mFrame->UnlockImageData();
      }
    }

    
    bool Succeeded() { return mSucceeded; }

  private:
    nsRefPtr<imgFrame> mFrame;
    bool mSucceeded;
  };

} 
} 

#endif 
