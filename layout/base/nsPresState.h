









































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
    , mDisabledSet(PR_FALSE)
    , mDisabled(PR_FALSE)
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
    mDisabledSet = PR_FALSE;
  }

  PRBool GetDisabled()
  {
    return mDisabled;
  }

  void SetDisabled(PRBool aDisabled)
  {
    mDisabled = aDisabled;
    mDisabledSet = PR_TRUE;
  }

  PRBool IsDisabledSet()
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
  PRPackedBool mDisabledSet;
  PRPackedBool mDisabled;
};

#endif 
