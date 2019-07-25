






































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
  virtual nsAccessible* AnchorAt(PRUint32 aAnchorIndex);
  virtual already_AddRefed<nsIURI> AnchorURIAt(PRUint32 aAnchorIndex);

protected:

  
  virtual void CacheChildren();

private:
  
  nsCOMPtr<nsIDOMHTMLMapElement> mMapElement;
};





class nsHTMLAreaAccessible : public nsHTMLLinkAccessible
{
public:

  nsHTMLAreaAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  

  NS_IMETHOD GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height);

  
  virtual void Description(nsString& aDescription);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual PRUint64 NativeState();
  virtual nsAccessible* ChildAtPoint(PRInt32 aX, PRInt32 aY,
                                     EWhichChildAtPoint aWhichChild);

  
  virtual PRUint32 StartOffset();
  virtual PRUint32 EndOffset();

protected:

  
  virtual void CacheChildren();
};

#endif  
