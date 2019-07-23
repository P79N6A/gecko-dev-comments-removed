



















































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
protected:
  class UndisplayedMap;

  
  nsIPresShell*                   mPresShell;
  
  nsStyleSet*                     mStyleSet;
  nsIFrame*                       mRootFrame;
  PLDHashTable                    mPrimaryFrameMap;
  PLDHashTable                    mPlaceholderMap;
  UndisplayedMap*                 mUndisplayedMap;
  PRBool                          mIsDestroyingFrames;
};

#endif
