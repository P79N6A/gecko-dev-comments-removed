









#include "nsStyleChangeList.h"
#include "nsStyleConsts.h"
#include "nsIFrame.h"
#include "nsIContent.h"
#include "nsCRT.h"

static const uint32_t kGrowArrayBy = 10;

nsStyleChangeList::nsStyleChangeList()
  : mArray(mBuffer),
    mArraySize(kStyleChangeBufferSize),
    mCount(0)
{
  MOZ_COUNT_CTOR(nsStyleChangeList);
}

nsStyleChangeList::~nsStyleChangeList()
{
  MOZ_COUNT_DTOR(nsStyleChangeList);
  Clear();
}

nsresult 
nsStyleChangeList::ChangeAt(int32_t aIndex, nsIFrame*& aFrame, nsIContent*& aContent, 
                            nsChangeHint& aHint) const
{
  if ((0 <= aIndex) && (aIndex < mCount)) {
    aFrame = mArray[aIndex].mFrame;
    aContent = mArray[aIndex].mContent;
    aHint = mArray[aIndex].mHint;
    return NS_OK;
  }
  return NS_ERROR_ILLEGAL_VALUE;
}

nsresult 
nsStyleChangeList::ChangeAt(int32_t aIndex, const nsStyleChangeData** aChangeData) const
{
  if ((0 <= aIndex) && (aIndex < mCount)) {
    *aChangeData = &mArray[aIndex];
    return NS_OK;
  }
  return NS_ERROR_ILLEGAL_VALUE;
}

nsresult 
nsStyleChangeList::AppendChange(nsIFrame* aFrame, nsIContent* aContent, nsChangeHint aHint)
{
  NS_ASSERTION(aFrame || (aHint & nsChangeHint_ReconstructFrame),
               "must have frame");
  NS_ASSERTION(aContent || !(aHint & nsChangeHint_ReconstructFrame),
               "must have content");
  
  NS_ASSERTION(!aContent || aContent->IsElement(),
               "Shouldn't be trying to restyle non-elements directly");
  NS_ASSERTION(!(aHint & nsChangeHint_ReflowFrame) ||
               (aHint & nsChangeHint_NeedReflow),
               "Reflow hint bits set without actually asking for a reflow");

  if ((0 < mCount) && (aHint & nsChangeHint_ReconstructFrame)) { 
    if (aContent) {
      for (int32_t index = mCount - 1; index >= 0; --index) {
        if (aContent == mArray[index].mContent) { 
          aContent->Release();
          mCount--;
          if (index < mCount) { 
            ::memmove(&mArray[index], &mArray[index + 1], 
                      (mCount - index) * sizeof(nsStyleChangeData));
          }
        }
      }
    }
  }

  int32_t last = mCount - 1;
  if ((0 < mCount) && aFrame && (aFrame == mArray[last].mFrame)) { 
    NS_UpdateHint(mArray[last].mHint, aHint);
  }
  else {
    if (mCount == mArraySize) {
      int32_t newSize = mArraySize + kGrowArrayBy;
      nsStyleChangeData* newArray = new nsStyleChangeData[newSize];
      if (newArray) {
        memcpy(newArray, mArray, mCount * sizeof(nsStyleChangeData));
        if (mArray != mBuffer) {
          delete [] mArray;
        }
        mArray = newArray;
        mArraySize = newSize;
      }
      else {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
    mArray[mCount].mFrame = aFrame;
    mArray[mCount].mContent = aContent;
    if (aContent) {
      aContent->AddRef();
    }
    mArray[mCount].mHint = aHint;
    mCount++;
  }
  return NS_OK;
}

void
nsStyleChangeList::Clear()
{
  for (int32_t index = mCount - 1; index >= 0; --index) {
    nsIContent* content = mArray[index].mContent;
    if (content) {
      content->Release();
    }
  }
  if (mArray != mBuffer) {
    delete [] mArray;
    mArray = mBuffer;
    mArraySize = kStyleChangeBufferSize;
  }
  mCount = 0;
}

