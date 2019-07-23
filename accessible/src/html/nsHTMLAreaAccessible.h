





































#ifndef _nsHTMLAreaAccessible_H_
#define _nsHTMLAreaAccessible_H_

#include "nsBaseWidgetAccessible.h"




class nsHTMLAreaAccessible : public nsLinkableAccessible
{

public:
  nsHTMLAreaAccessible(nsIDOMNode *domNode, nsIAccessible *accParent, nsIWeakReference* aShell);
  NS_IMETHOD GetName(nsAString & _retval); 
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetFirstChild(nsIAccessible **_retval);
  NS_IMETHOD GetLastChild(nsIAccessible **_retval);
  NS_IMETHOD GetChildCount(PRInt32 *_retval);
  NS_IMETHOD GetDescription(nsAString& _retval);
  NS_IMETHOD GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height);
};

#endif  
