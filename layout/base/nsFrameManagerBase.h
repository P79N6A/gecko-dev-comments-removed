



















































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
  bool IsDestroyingFrames() { return mIsDestroyingFrames; }

  




  NS_HIDDEN_(nsIFrame*) GetRootFrame() const { return mRootFrame; }
  NS_HIDDEN_(void)      SetRootFrame(nsIFrame* aRootFrame)
  {
    NS_ASSERTION(!mRootFrame, "already have a root frame");
    mRootFrame = aRootFrame;
  }

protected:
  class UndisplayedMap;

  
  nsIPresShell*                   mPresShell;
  
  nsStyleSet*                     mStyleSet;
  nsIFrame*                       mRootFrame;
  PLDHashTable                    mPlaceholderMap;
  UndisplayedMap*                 mUndisplayedMap;
  bool                            mIsDestroyingFrames;  
};

#endif
