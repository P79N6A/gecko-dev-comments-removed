












































#ifndef _nsDocAccessibleWrap_H_
#define _nsDocAccessibleWrap_H_

#include "nsDocAccessible.h"

class nsDocAccessibleWrap: public nsDocAccessible
{
public:
  nsDocAccessibleWrap(nsIDOMNode *aNode, nsIWeakReference *aShell);
  virtual ~nsDocAccessibleWrap();

  NS_IMETHOD FireToolkitEvent(PRUint32 aEvent, nsIAccessible* aAccessible,
                              void* aData);
  NS_IMETHOD FireAccessibleEvent(nsIAccessibleEvent *aEvent);

protected:
  PRBool mActivated;
};

#endif
