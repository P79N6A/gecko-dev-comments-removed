





































#ifndef _nsHTMLImageAccessible_H_
#define _nsHTMLImageAccessible_H_

#include "nsBaseWidgetAccessible.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsIAccessibleImage.h"






class nsHTMLImageAccessible : public nsLinkableAccessible,
                              public nsIAccessibleImage
{

  NS_DECL_ISUPPORTS_INHERITED

public:
  
  enum { eAction_ShowLongDescription = 1 };

  nsHTMLImageAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetName(nsAString& _retval); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetRole(PRUint32 *_retval);
  NS_IMETHOD DoAction(PRUint8 index);

  
  NS_IMETHOD Shutdown();

  
  NS_DECL_NSIACCESSIBLEIMAGE

protected:
  virtual void CacheChildren();
  already_AddRefed<nsIAccessible> GetAreaAccessible(PRInt32 aAreaNum);
  nsCOMPtr<nsIDOMHTMLMapElement> mMapElement;

  
  
  
  nsAccessNodeHashtable *mAccessNodeCache;
};

#endif

