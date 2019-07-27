



#ifndef nsILineIterator_h___
#define nsILineIterator_h___

#include "nscore.h"
#include "nsCoord.h"
#include "mozilla/Attributes.h"

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

  


  virtual int32_t GetNumLines() = 0;

  





  virtual bool GetDirection() = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  NS_IMETHOD GetLine(int32_t aLineNumber,
                     nsIFrame** aFirstFrameOnLine,
                     int32_t* aNumFramesOnLine,
                     nsRect& aLineBounds,
                     uint32_t* aLineFlags) = 0;

  





  virtual int32_t FindLineContaining(nsIFrame* aFrame,
                                     int32_t aStartLine = 0) = 0;

  
  
  
  
  NS_IMETHOD FindFrameAt(int32_t aLineNumber,
                         nscoord aX,
                         nsIFrame** aFrameFound,
                         bool* aXIsBeforeFirstFrame,
                         bool* aXIsAfterLastFrame) = 0;

  
  
  NS_IMETHOD GetNextSiblingOnLine(nsIFrame*& aFrame, int32_t aLineNumber) = 0;

  
  
  NS_IMETHOD CheckLineOrder(int32_t                  aLine,
                            bool                     *aIsReordered,
                            nsIFrame                 **aFirstVisual,
                            nsIFrame                 **aLastVisual) = 0;
};

class nsAutoLineIterator
{
public:
  nsAutoLineIterator() : mRawPtr(nullptr) { }
  MOZ_IMPLICIT nsAutoLineIterator(nsILineIterator *i) : mRawPtr(i) { }

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
