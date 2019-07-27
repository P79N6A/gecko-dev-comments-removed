

















#ifndef _nsFrameManagerBase_h_
#define _nsFrameManagerBase_h_

#include "nsDebug.h"
#include "pldhash.h"
#include "mozilla/Attributes.h"

class nsIFrame;
class nsIPresShell;
class nsStyleSet;

class nsFrameManagerBase
{
public:
  nsFrameManagerBase()
    : mPresShell(nullptr)
    , mStyleSet(nullptr)
    , mRootFrame(nullptr)
    , mUndisplayedMap(nullptr)
    , mDisplayContentsMap(nullptr)
    , mIsDestroyingFrames(false)
  {
  }

  bool IsDestroyingFrames() { return mIsDestroyingFrames; }

  




  nsIFrame* GetRootFrame() const { return mRootFrame; }
  void      SetRootFrame(nsIFrame* aRootFrame)
  {
    NS_ASSERTION(!mRootFrame, "already have a root frame");
    mRootFrame = aRootFrame;
  }

  static uint32_t GetGlobalGenerationNumber() { return sGlobalGenerationNumber; }

protected:
  class UndisplayedMap;

  
  nsIPresShell* MOZ_NON_OWNING_REF mPresShell;
  
  nsStyleSet*                     mStyleSet;
  nsIFrame*                       mRootFrame;
  PLDHashTable                    mPlaceholderMap;
  UndisplayedMap*                 mUndisplayedMap;
  UndisplayedMap*                 mDisplayContentsMap;
  bool                            mIsDestroyingFrames;  

  
  
  
  
  
  
  
  
  static uint32_t                 sGlobalGenerationNumber;
};

#endif
