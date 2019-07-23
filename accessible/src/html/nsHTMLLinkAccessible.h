





































#ifndef _nsHTMLLinkAccessible_H_
#define _nsHTMLLinkAccessible_H_

#include "nsBaseWidgetAccessible.h"

class nsHTMLLinkAccessible : public nsLinkableAccessible
{
  NS_DECL_ISUPPORTS_INHERITED

public:
  nsHTMLLinkAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell, nsIFrame *aFrame);
  
  
  NS_IMETHOD GetName(nsAString& _retval); 
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *_retval);
  NS_IMETHOD Shutdown() { mFrame = nsnull; return nsLinkableAccessible::Shutdown(); }
  
  
  NS_IMETHOD_(nsIFrame *) GetFrame(void);

  
  NS_IMETHOD FireToolkitEvent(PRUint32 aEvent, nsIAccessible *aTarget,
                              void *aData);

private:
  nsIFrame *mFrame;  
};

#endif  
