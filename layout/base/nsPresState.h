









































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
  NS_HIDDEN_(nsresult) Init();

  NS_HIDDEN_(nsresult) GetStatePropertyAsSupports(const nsAString& aName,
                                                  nsISupports** aResult);

  NS_HIDDEN_(nsresult) SetStatePropertyAsSupports(const nsAString& aName,
                                                  nsISupports* aValue);

  NS_HIDDEN_(nsresult) GetStateProperty(const nsAString& aProperty,
                                        nsAString& aResult);

  NS_HIDDEN_(nsresult) SetStateProperty(const nsAString& aProperty,
                                        const nsAString& aValue);

  NS_HIDDEN_(nsresult) RemoveStateProperty(const nsAString& aProperty);

  NS_HIDDEN_(nsresult) SetScrollState(const nsRect& aState);

  nsRect GetScrollState();


protected:
  nsInterfaceHashtable<nsStringHashKey,nsISupports> mPropertyTable;
  nsAutoPtr<nsRect> mScrollState;
};

NS_HIDDEN_(nsresult) NS_NewPresState(nsPresState **aState);

#endif 
