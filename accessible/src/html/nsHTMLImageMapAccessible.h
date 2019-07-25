






































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

  
  virtual PRUint32 NativeRole();

  
  virtual PRUint32 AnchorCount();
  virtual nsAccessible* GetAnchor(PRUint32 aAnchorIndex);
  virtual already_AddRefed<nsIURI> GetAnchorURI(PRUint32 aAnchorIndex);

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
  virtual PRUint64 NativeState();
  virtual nsAccessible* GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                        EWhichChildAtPoint aWhichChild);

  
  virtual PRUint32 StartOffset();
  virtual PRUint32 EndOffset();

protected:

  
  virtual void CacheChildren();
};

#endif  
