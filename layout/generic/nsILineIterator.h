



































#ifndef nsILineIterator_h___
#define nsILineIterator_h___

#include "nscore.h"
#include "nsCoord.h"

class nsIFrame;
struct nsRect;





#define NS_LINE_FLAG_IS_BLOCK           0x1


#define NS_LINE_FLAG_ENDS_IN_BREAK      0x4











class nsILineIterator
{
protected:
  ~nsILineIterator() { }

public:
  virtual void DisposeLineIterator() = 0;

  


  virtual PRInt32 GetNumLines() = 0;

  





  virtual bool GetDirection() = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  NS_IMETHOD GetLine(PRInt32 aLineNumber,
                     nsIFrame** aFirstFrameOnLine,
                     PRInt32* aNumFramesOnLine,
                     nsRect& aLineBounds,
                     PRUint32* aLineFlags) = 0;

  





  virtual PRInt32 FindLineContaining(nsIFrame* aFrame,
                                     PRInt32 aStartLine = 0) = 0;

  
  
  
  
  NS_IMETHOD FindFrameAt(PRInt32 aLineNumber,
                         nscoord aX,
                         nsIFrame** aFrameFound,
                         bool* aXIsBeforeFirstFrame,
                         bool* aXIsAfterLastFrame) = 0;

  
  
  NS_IMETHOD GetNextSiblingOnLine(nsIFrame*& aFrame, PRInt32 aLineNumber) = 0;

#ifdef IBMBIDI
  
  
  NS_IMETHOD CheckLineOrder(PRInt32                  aLine,
                            bool                     *aIsReordered,
                            nsIFrame                 **aFirstVisual,
                            nsIFrame                 **aLastVisual) = 0;
#endif
};

class nsAutoLineIterator
{
public:
  nsAutoLineIterator() : mRawPtr(nsnull) { }
  nsAutoLineIterator(nsILineIterator *i) : mRawPtr(i) { }

  ~nsAutoLineIterator() {
    if (mRawPtr)
      mRawPtr->DisposeLineIterator();
  }

  operator nsILineIterator*() { return mRawPtr; }
  nsILineIterator* operator->() { return mRawPtr; }

  nsILineIterator* operator=(nsILineIterator* i) {
    if (i == mRawPtr)
      return i;

    if (mRawPtr)
      mRawPtr->DisposeLineIterator();

    mRawPtr = i;
    return i;
  }

private:
  nsILineIterator* mRawPtr;
};

#endif 
