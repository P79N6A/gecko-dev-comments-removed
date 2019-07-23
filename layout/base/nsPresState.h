









































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
    , mDisabledSet(PR_FALSE)
    , mDisabled(PR_FALSE)
  {}

  NS_HIDDEN_(nsresult) SetScrollState(const nsRect& aState);

  nsRect               GetScrollState();

  NS_HIDDEN_(void)     ClearNonScrollState();

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
  PRPackedBool mDisabledSet;
  PRPackedBool mDisabled;
  nsAutoPtr<nsRect> mScrollState;
};

#endif 
