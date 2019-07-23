





































#ifndef _nsHTMLAreaAccessible_H_
#define _nsHTMLAreaAccessible_H_

#include "nsHTMLLinkAccessible.h"




class nsHTMLAreaAccessible : public nsHTMLLinkAccessible
{

public:
  nsHTMLAreaAccessible(nsIDOMNode *domNode, nsIAccessible *accParent,
                       nsIWeakReference* aShell);

  
  NS_IMETHOD GetDescription(nsAString& aDescription);

  NS_IMETHOD GetFirstChild(nsIAccessible **_retval);
  NS_IMETHOD GetLastChild(nsIAccessible **_retval);
  NS_IMETHOD GetChildCount(PRInt32 *_retval);

  NS_IMETHOD GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height);
  NS_IMETHOD GetChildAtPoint(PRInt32 aX, PRInt32 aY, nsIAccessible **aAccessible)
    { NS_ENSURE_ARG_POINTER(aAccessible); NS_ADDREF(*aAccessible = this); return NS_OK; } 

  
  virtual nsresult GetNameInternal(nsAString& aName);
};

#endif  
