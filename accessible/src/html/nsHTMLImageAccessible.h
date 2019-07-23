





































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
  NS_IMETHOD GetState(PRUint32 *_retval); 
  NS_IMETHOD GetRole(PRUint32 *_retval);
  NS_IMETHOD DoAction(PRUint8 index);

  NS_IMETHOD GetImageBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height);

protected:
  virtual void CacheChildren();
  already_AddRefed<nsIAccessible> CreateAreaAccessible(PRInt32 areaNum);
  nsCOMPtr<nsIDOMHTMLMapElement> mMapElement;
};

#endif  
