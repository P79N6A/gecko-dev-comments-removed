






































#ifndef _nsHTMLAreaAccessible_H_
#define _nsHTMLAreaAccessible_H_

#include "nsHTMLLinkAccessible.h"
#include "nsHTMLImageAccessibleWrap.h"

#include "nsIDOMHTMLMapElement.h"




class nsHTMLImageMapAccessible : public nsHTMLImageAccessibleWrap
{
public:
  nsHTMLImageMapAccessible(nsIContent *aContent, nsIWeakReference *aShell,
                           nsIDOMHTMLMapElement *aMapElm);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetAnchorCount(PRInt32 *aAnchorCount);
  NS_IMETHOD GetURI(PRInt32 aIndex, nsIURI **aURI);
  NS_IMETHOD GetAnchor(PRInt32 aIndex, nsIAccessible **aAccessible);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);

protected:

  
  virtual void CacheChildren();

private:
  
  nsCOMPtr<nsIDOMHTMLMapElement> mMapElement;
};





class nsHTMLAreaAccessible : public nsHTMLLinkAccessible
{
public:
  using nsAccessible::GetChildAtPoint;

  nsHTMLAreaAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_IMETHOD GetDescription(nsAString& aDescription);

  NS_IMETHOD GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   PRBool aDeepestChild,
                                   nsIAccessible **aChild);

protected:

  
  virtual void CacheChildren();
};

#endif  
