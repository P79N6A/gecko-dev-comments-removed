






































#ifndef _nsHTMLAreaAccessible_H_
#define _nsHTMLAreaAccessible_H_

#include "nsHTMLLinkAccessible.h"
#include "nsHTMLImageAccessibleWrap.h"

#include "nsIDOMHTMLMapElement.h"




class nsHTMLImageMapAccessible : public nsHTMLImageAccessibleWrap
{
public:
  nsHTMLImageMapAccessible(nsIContent* aContent, nsDocAccessible* aDoc);
  virtual ~nsHTMLImageMapAccessible() { }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual mozilla::a11y::role NativeRole();

  
  virtual PRUint32 AnchorCount();
  virtual nsAccessible* AnchorAt(PRUint32 aAnchorIndex);
  virtual already_AddRefed<nsIURI> AnchorURIAt(PRUint32 aAnchorIndex);

  


  void UpdateChildAreas(bool aDoFireEvents = true);

protected:

  
  virtual void CacheChildren();
};




inline nsHTMLImageMapAccessible*
nsAccessible::AsImageMap()
{
  return IsImageMapAccessible() ?
    static_cast<nsHTMLImageMapAccessible*>(this) : nsnull;
}





class nsHTMLAreaAccessible : public nsHTMLLinkAccessible
{
public:

  nsHTMLAreaAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual bool IsPrimaryForNode() const;

  
  virtual void Description(nsString& aDescription);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsAccessible* ChildAtPoint(PRInt32 aX, PRInt32 aY,
                                     EWhichChildAtPoint aWhichChild);
  virtual void GetBoundsRect(nsRect& aBounds, nsIFrame** aBoundingFrame);

  
  virtual PRUint32 StartOffset();
  virtual PRUint32 EndOffset();

protected:

  
  virtual void CacheChildren();
};

#endif  
