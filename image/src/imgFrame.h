





#ifndef mozilla_image_src_imgFrame_h
#define mozilla_image_src_imgFrame_h

#include "mozilla/MemoryReporting.h"
#include "mozilla/Monitor.h"
#include "mozilla/Move.h"
#include "mozilla/VolatileBuffer.h"
#include "gfxDrawable.h"
#include "imgIContainer.h"
#include "MainThreadUtils.h"

namespace mozilla {
namespace image {

class ImageRegion;
class DrawableFrameRef;
class RawAccessFrameRef;

enum class BlendMethod : int8_t {
  
  
  SOURCE,

  
  
  OVER
};

enum class DisposalMethod : int8_t {
  CLEAR_ALL = -1,  
  NOT_SPECIFIED,   
  KEEP,            
  CLEAR,           
  RESTORE_PREVIOUS 
};

enum class Opacity : uint8_t {
  OPAQUE,
  SOME_TRANSPARENCY
};









struct AnimationData
{
  AnimationData(uint8_t* aRawData, uint32_t aPaletteDataLength,
                int32_t aRawTimeout, const nsIntRect& aRect,
                BlendMethod aBlendMethod, DisposalMethod aDisposalMethod,
                bool aHasAlpha)
    : mRawData(aRawData)
    , mPaletteDataLength(aPaletteDataLength)
    , mRawTimeout(aRawTimeout)
    , mRect(aRect)
    , mBlendMethod(aBlendMethod)
    , mDisposalMethod(aDisposalMethod)
    , mHasAlpha(aHasAlpha)
  { }

  uint8_t* mRawData;
  uint32_t mPaletteDataLength;
  int32_t mRawTimeout;
  nsIntRect mRect;
  BlendMethod mBlendMethod;
  DisposalMethod mDisposalMethod;
  bool mHasAlpha;
};









struct ScalingData
{
  ScalingData(uint8_t* aRawData,
              gfx::IntSize aSize,
              uint32_t aBytesPerRow,
              gfx::SurfaceFormat aFormat)
    : mRawData(aRawData)
    , mSize(aSize)
    , mBytesPerRow(aBytesPerRow)
    , mFormat(aFormat)
  { }

  uint8_t* mRawData;
  gfx::IntSize mSize;
  uint32_t mBytesPerRow;
  gfx::SurfaceFormat mFormat;
};

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

  







  nsresult InitForDecoder(const nsIntSize& aImageSize,
                          const nsIntRect& aRect,
                          SurfaceFormat aFormat,
                          uint8_t aPaletteDepth = 0,
                          bool aNonPremult = false);

  nsresult InitForDecoder(const nsIntSize& aSize,
                          SurfaceFormat aFormat,
                          uint8_t aPaletteDepth = 0)
  {
    return InitForDecoder(aSize, nsIntRect(0, 0, aSize.width, aSize.height),
                          aFormat, aPaletteDepth);
  }


  









  nsresult InitWithDrawable(gfxDrawable* aDrawable,
                            const nsIntSize& aSize,
                            const SurfaceFormat aFormat,
                            GraphicsFilter aFilter,
                            uint32_t aImageFlags);

  






  nsresult ReinitForDecoder(const nsIntSize& aImageSize,
                            const nsIntRect& aRect,
                            SurfaceFormat aFormat,
                            uint8_t aPaletteDepth = 0,
                            bool aNonPremult = false);

  DrawableFrameRef DrawableRef();
  RawAccessFrameRef RawAccessRef();

  









  void SetRawAccessOnly();

  bool Draw(gfxContext* aContext, const ImageRegion& aRegion,
            GraphicsFilter aFilter, uint32_t aImageFlags);

  nsresult ImageUpdated(const nsIntRect& aUpdateRect);

  
















  void Finish(Opacity aFrameOpacity = Opacity::SOME_TRANSPARENCY,
              DisposalMethod aDisposalMethod = DisposalMethod::KEEP,
              int32_t aRawTimeout = 0,
              BlendMethod aBlendMethod = BlendMethod::OVER);

  






  void Abort();

  


  bool IsImageComplete() const;

  







  void WaitUntilComplete() const;

  IntSize GetImageSize() const { return mImageSize; }
  nsIntRect GetRect() const;
  IntSize GetSize() const { return mSize; }
  bool NeedsPadding() const { return mOffset != nsIntPoint(0, 0); }
  void GetImageData(uint8_t** aData, uint32_t* length) const;
  uint8_t* GetImageData() const;

  bool GetIsPaletted() const;
  void GetPaletteData(uint32_t** aPalette, uint32_t* length) const;
  uint32_t* GetPaletteData() const;
  uint8_t GetPaletteDepth() const { return mPaletteDepth; }

  




  SurfaceFormat GetFormat() const;

  AnimationData GetAnimationData() const;
  ScalingData GetScalingData() const;

  bool GetCompositingFailed() const;
  void SetCompositingFailed(bool val);

  void SetOptimizable();

  Color SinglePixelColor() const;
  bool IsSinglePixel() const;

  TemporaryRef<SourceSurface> GetSurface();
  TemporaryRef<DrawTarget> GetDrawTarget();

  size_t SizeOfExcludingThis(gfxMemoryLocation aLocation,
                             MallocSizeOf aMallocSizeOf) const;

private: 

  ~imgFrame();

  nsresult LockImageData();
  nsresult UnlockImageData();
  nsresult Optimize();
  nsresult Deoptimize();

  void AssertImageDataLocked() const;

  bool IsImageCompleteInternal() const;
  nsresult ImageUpdatedInternal(const nsIntRect& aUpdateRect);
  void GetImageDataInternal(uint8_t** aData, uint32_t* length) const;
  uint32_t GetImageBytesPerRow() const;
  uint32_t GetImageDataLength() const;
  int32_t GetStride() const;
  TemporaryRef<SourceSurface> GetSurfaceInternal();

  uint32_t PaletteDataLength() const
  {
    return mPaletteDepth ? (size_t(1) << mPaletteDepth) * sizeof(uint32_t)
                         : 0;
  }

  struct SurfaceWithFormat {
    nsRefPtr<gfxDrawable> mDrawable;
    SurfaceFormat mFormat;
    SurfaceWithFormat() { }
    SurfaceWithFormat(gfxDrawable* aDrawable, SurfaceFormat aFormat)
      : mDrawable(aDrawable), mFormat(aFormat)
    { }
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
  friend class DrawableFrameRef;
  friend class RawAccessFrameRef;
  friend class UnlockImageDataRunnable;

  
  
  

  mutable Monitor mMonitor;

  RefPtr<DataSourceSurface> mImageSurface;
  RefPtr<SourceSurface> mOptSurface;

  RefPtr<VolatileBuffer> mVBuf;
  VolatileBufferPtr<uint8_t> mVBufPtr;

  nsIntRect mDecoded;

  
  int32_t mLockCount;

  
  int32_t mTimeout; 

  DisposalMethod mDisposalMethod;
  BlendMethod    mBlendMethod;
  SurfaceFormat  mFormat;

  bool mHasNoAlpha;
  bool mAborted;


  
  
  

  IntSize      mImageSize;
  IntSize      mSize;
  nsIntPoint   mOffset;

  
  
  
  
  uint8_t*     mPalettedImageData;
  uint8_t      mPaletteDepth;

  bool mNonPremult;


  
  
  

  
  Color        mSinglePixelColor;

  bool mSinglePixel;
  bool mCompositingFailed;
  bool mOptimizable;
};






class DrawableFrameRef final
{
public:
  DrawableFrameRef() { }

  explicit DrawableFrameRef(imgFrame* aFrame)
    : mFrame(aFrame)
    , mRef(aFrame->mVBuf)
  {
    if (mRef.WasBufferPurged()) {
      mFrame = nullptr;
      mRef = nullptr;
    }
  }

  DrawableFrameRef(DrawableFrameRef&& aOther)
    : mFrame(aOther.mFrame.forget())
    , mRef(Move(aOther.mRef))
  { }

  DrawableFrameRef& operator=(DrawableFrameRef&& aOther)
  {
    MOZ_ASSERT(this != &aOther, "Self-moves are prohibited");
    mFrame = aOther.mFrame.forget();
    mRef = Move(aOther.mRef);
    return *this;
  }

  explicit operator bool() const { return bool(mFrame); }

  imgFrame* operator->()
  {
    MOZ_ASSERT(mFrame);
    return mFrame;
  }

  const imgFrame* operator->() const
  {
    MOZ_ASSERT(mFrame);
    return mFrame;
  }

  imgFrame* get() { return mFrame; }
  const imgFrame* get() const { return mFrame; }

  void reset()
  {
    mFrame = nullptr;
    mRef = nullptr;
  }

private:
  nsRefPtr<imgFrame> mFrame;
  VolatileBufferPtr<uint8_t> mRef;
};













class RawAccessFrameRef final
{
public:
  RawAccessFrameRef() { }

  explicit RawAccessFrameRef(imgFrame* aFrame)
    : mFrame(aFrame)
  {
    MOZ_ASSERT(mFrame, "Need a frame");

    if (NS_FAILED(mFrame->LockImageData())) {
      mFrame->UnlockImageData();
      mFrame = nullptr;
    }
  }

  RawAccessFrameRef(RawAccessFrameRef&& aOther)
    : mFrame(aOther.mFrame.forget())
  { }

  ~RawAccessFrameRef()
  {
    if (mFrame) {
      mFrame->UnlockImageData();
    }
  }

  RawAccessFrameRef& operator=(RawAccessFrameRef&& aOther)
  {
    MOZ_ASSERT(this != &aOther, "Self-moves are prohibited");

    if (mFrame) {
      mFrame->UnlockImageData();
    }

    mFrame = aOther.mFrame.forget();

    return *this;
  }

  explicit operator bool() const { return bool(mFrame); }

  imgFrame* operator->()
  {
    MOZ_ASSERT(mFrame);
    return mFrame.get();
  }

  const imgFrame* operator->() const
  {
    MOZ_ASSERT(mFrame);
    return mFrame;
  }

  imgFrame* get() { return mFrame; }
  const imgFrame* get() const { return mFrame; }

  void reset()
  {
    if (mFrame) {
      mFrame->UnlockImageData();
    }
    mFrame = nullptr;
  }

private:
  nsRefPtr<imgFrame> mFrame;
};

} 
} 

#endif 
