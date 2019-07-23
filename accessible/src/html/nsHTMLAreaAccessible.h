





































#ifndef _nsHTMLAreaAccessible_H_
#define _nsHTMLAreaAccessible_H_

#include "nsHTMLLinkAccessible.h"




class nsHTMLAreaAccessible : public nsHTMLLinkAccessible
{

public:
  nsHTMLAreaAccessible(nsIDOMNode *domNode, nsIAccessible *accParent,
                       nsIWeakReference* aShell);

  
  NS_IMETHOD GetDescription(nsAString& aDescription);

  NS_IMETHOD GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   PRBool aDeepestChild,
                                   nsIAccessible **aChild);

protected:

  
  virtual void CacheChildren();
};

#endif  
