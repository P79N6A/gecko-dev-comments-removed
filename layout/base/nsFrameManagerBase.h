

















#ifndef _nsFrameManagerBase_h_
#define _nsFrameManagerBase_h_

#include "pldhash.h"

class nsIPresShell;
class nsStyleSet;
class nsIContent;
class nsPlaceholderFrame;
class nsIFrame;
class nsStyleContext;
class nsIAtom;
class nsStyleChangeList;
class nsILayoutHistoryState;

class nsFrameManagerBase
{
public:
  nsFrameManagerBase()
  {
    memset(this, '\0', sizeof(nsFrameManagerBase));
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

  
  nsIPresShell*                   mPresShell;
  
  nsStyleSet*                     mStyleSet;
  nsIFrame*                       mRootFrame;
  PLDHashTable                    mPlaceholderMap;
  UndisplayedMap*                 mUndisplayedMap;
  bool                            mIsDestroyingFrames;  

  
  
  
  
  
  
  
  
  static uint32_t                 sGlobalGenerationNumber;
};

#endif
