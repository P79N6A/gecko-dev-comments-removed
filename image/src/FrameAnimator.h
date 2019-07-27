





#ifndef mozilla_imagelib_FrameAnimator_h_
#define mozilla_imagelib_FrameAnimator_h_

#include "mozilla/TimeStamp.h"
#include "nsRect.h"

namespace mozilla {
namespace image {

class FrameBlender;

class FrameAnimator
{
public:
  FrameAnimator(FrameBlender& aBlender, uint16_t aAnimationMode);

  



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
    {}

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

  


  void SetFirstFrameRefreshArea(const nsIntRect& aRect);

  



  void UnionFirstFrameRefreshArea(const nsIntRect& aRect);

  



  void InitAnimationFrameTimeIfNecessary();

  


  void SetAnimationFrameTime(const TimeStamp& aTime);

  


  uint32_t GetCurrentAnimationFrameIndex() const;

  


  nsIntRect GetFirstFrameRefreshArea() const;

private: 
  






  int32_t GetSingleLoopTime() const;

  











  RefreshResult AdvanceFrame(TimeStamp aTime);

  




  TimeStamp GetCurrentImgFrameEndTime() const;

private: 
  
  nsIntRect mFirstFrameRefreshArea;

  
  TimeStamp mCurrentAnimationFrameTime;

  
  uint32_t mCurrentAnimationFrameIndex;

  
  int32_t mLoopCounter;

  
  FrameBlender& mFrameBlender;

  
  uint16_t mAnimationMode;

  
  bool mDoneDecoding;
};

} 
} 

#endif 
