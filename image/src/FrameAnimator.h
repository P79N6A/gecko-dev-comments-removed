





#ifndef mozilla_image_src_FrameAnimator_h
#define mozilla_image_src_FrameAnimator_h

#include "mozilla/MemoryReporting.h"
#include "mozilla/TimeStamp.h"
#include "gfx2DGlue.h"
#include "gfxTypes.h"
#include "imgFrame.h"
#include "nsCOMPtr.h"
#include "nsRect.h"
#include "SurfaceCache.h"

namespace mozilla {
namespace image {

class RasterImage;

class FrameAnimator
{
public:
  FrameAnimator(RasterImage* aImage,
                gfx::IntSize aSize,
                uint16_t aAnimationMode)
    : mImage(aImage)
    , mSize(aSize)
    , mCurrentAnimationFrameIndex(0)
    , mLoopRemainingCount(-1)
    , mLastCompositedFrameIndex(-1)
    , mLoopCount(-1)
    , mAnimationMode(aAnimationMode)
    , mDoneDecoding(false)
  { }

  



  struct RefreshResult
  {
    
    nsIntRect dirtyRect;

    
    bool frameAdvanced : 1;

    
    bool animationFinished : 1;

    
    
    bool error : 1;

    RefreshResult()
      : frameAdvanced(false)
      , animationFinished(false)
      , error(false)
    { }

    void Accumulate(const RefreshResult& other)
    {
      frameAdvanced = frameAdvanced || other.frameAdvanced;
      animationFinished = animationFinished || other.animationFinished;
      error = error || other.error;
      dirtyRect = dirtyRect.Union(other.dirtyRect);
    }
  };

  






  RefreshResult RequestRefresh(const TimeStamp& aTime);

  



  void SetDoneDecoding(bool aDone);

  



  void ResetAnimation();

  




  void SetAnimationMode(uint16_t aAnimationMode);

  



  void UnionFirstFrameRefreshArea(const nsIntRect& aRect);

  



  void InitAnimationFrameTimeIfNecessary();

  


  void SetAnimationFrameTime(const TimeStamp& aTime);

  


  uint32_t GetCurrentAnimationFrameIndex() const;

  


  nsIntRect GetFirstFrameRefreshArea() const;

  




  DrawableFrameRef GetCompositedFrame(uint32_t aFrameNum);

  





  int32_t GetTimeoutForFrame(uint32_t aFrameNum) const;

  



  void SetLoopCount(int32_t aLoopCount) { mLoopCount = aLoopCount; }
  int32_t LoopCount() const { return mLoopCount; }

  




  void CollectSizeOfCompositingSurfaces(nsTArray<SurfaceMemoryCounter>& aCounters,
                                        MallocSizeOf aMallocSizeOf) const;

private: 
  






  int32_t GetSingleLoopTime() const;

  











  RefreshResult AdvanceFrame(TimeStamp aTime);

  




  TimeStamp GetCurrentImgFrameEndTime() const;

  bool DoBlend(nsIntRect* aDirtyRect, uint32_t aPrevFrameIndex,
               uint32_t aNextFrameIndex);

  


  RawAccessFrameRef GetRawFrame(uint32_t aFrameNum) const;

  






  static void ClearFrame(uint8_t* aFrameData, const nsIntRect& aFrameRect);

  
  static void ClearFrame(uint8_t* aFrameData, const nsIntRect& aFrameRect,
                         const nsIntRect& aRectToClear);

  
  static bool CopyFrameImage(const uint8_t* aDataSrc, const nsIntRect& aRectSrc,
                             uint8_t* aDataDest, const nsIntRect& aRectDest);

  















  static nsresult DrawFrameTo(const uint8_t* aSrcData,
                              const nsIntRect& aSrcRect,
                              uint32_t aSrcPaletteLength, bool aSrcHasAlpha,
                              uint8_t* aDstPixels, const nsIntRect& aDstRect,
                              BlendMethod aBlendMethod);

private: 
  
  RasterImage* mImage;

  
  gfx::IntSize mSize;

  







  RawAccessFrameRef mCompositingFrame;

  





  RawAccessFrameRef mCompositingPrevFrame;

  
  nsIntRect mFirstFrameRefreshArea;

  
  TimeStamp mCurrentAnimationFrameTime;

  
  uint32_t mCurrentAnimationFrameIndex;

  
  int32_t mLoopRemainingCount;

  
  int32_t mLastCompositedFrameIndex;

  
  int32_t mLoopCount;

  
  uint16_t mAnimationMode;

  
  bool mDoneDecoding;
};

} 
} 

#endif 
