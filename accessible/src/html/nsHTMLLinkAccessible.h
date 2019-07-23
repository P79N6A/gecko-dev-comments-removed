





































#ifndef _nsHTMLLinkAccessible_H_
#define _nsHTMLLinkAccessible_H_

#include "nsBaseWidgetAccessible.h"

class nsHTMLLinkAccessible : public nsLinkableAccessible
{
  NS_DECL_ISUPPORTS_INHERITED

public:
  nsHTMLLinkAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
  
  
  NS_IMETHOD GetName(nsAString& _retval); 
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
};

#endif  
