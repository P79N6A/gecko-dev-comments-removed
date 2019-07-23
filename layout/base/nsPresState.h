









































#ifndef nsPresState_h_
#define nsPresState_h_

#include "prtypes.h"
#include "nsStringFwd.h"
#include "nsInterfaceHashtable.h"
#include "nsPoint.h"
#include "nsAutoPtr.h"
#include "nsRect.h"

class nsPresState
{
public:
  nsPresState() : mContentState(nsnull), mDisabledSet(PR_FALSE), 
                  mScrollState(nsnull) {};

  PRBool GetDisabled() {
    return mDisabled;
  }

  PRBool DisabledIsSet() {
    return mDisabledSet;
  }

  void SetDisabled(PRBool aDisabled) {
    mDisabled = aDisabled;
    mDisabledSet = PR_TRUE;
  }

  nsISupports* GetStateProperty() {
    return mContentState;
  }

  void SetStateProperty(nsISupports *aProperty) {
    mContentState = aProperty;
  }

  NS_HIDDEN_(nsresult) SetScrollState(const nsRect& aState);

  nsRect GetScrollState();


protected:
  nsCOMPtr<nsISupports> mContentState;
  PRPackedBool mDisabled;
  PRPackedBool mDisabledSet;

  nsAutoPtr<nsRect> mScrollState;
};

NS_HIDDEN_(nsresult) NS_NewPresState(nsPresState **aState);

#endif 
