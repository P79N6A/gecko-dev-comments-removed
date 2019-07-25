









































#ifndef nsPresState_h_
#define nsPresState_h_

#include "prtypes.h"
#include "nsPoint.h"
#include "nsAutoPtr.h"
#include "nsRect.h"

class nsPresState
{
public:
  nsPresState()
    : mContentData(nsnull)
    , mScrollState(0, 0)
    , mDisabledSet(false)
    , mDisabled(false)
  {}

  void SetScrollState(const nsPoint& aState)
  {
    mScrollState = aState;
  }

  nsPoint GetScrollState()
  {
    return mScrollState;
  }

  void ClearNonScrollState()
  {
    mContentData = nsnull;
    mDisabledSet = false;
  }

  bool GetDisabled()
  {
    return mDisabled;
  }

  void SetDisabled(bool aDisabled)
  {
    mDisabled = aDisabled;
    mDisabledSet = true;
  }

  bool IsDisabledSet()
  {
    return mDisabledSet;
  }

  nsISupports* GetStateProperty()
  {
    return mContentData;
  }

  void SetStateProperty(nsISupports *aProperty)
  {
    mContentData = aProperty;
  }


protected:
  nsCOMPtr<nsISupports> mContentData;
  nsPoint mScrollState;
  bool mDisabledSet;
  bool mDisabled;
};

#endif 
