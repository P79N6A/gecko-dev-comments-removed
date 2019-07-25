




#ifndef mozilla_a11y_HTMLImageMapAccessible_h__
#define mozilla_a11y_HTMLImageMapAccessible_h__

#include "HTMLLinkAccessible.h"
#include "ImageAccessibleWrap.h"
#include "nsIDOMHTMLMapElement.h"

namespace mozilla {
namespace a11y {




class HTMLImageMapAccessible : public ImageAccessibleWrap
{
public:
  HTMLImageMapAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~HTMLImageMapAccessible() { }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual a11y::role NativeRole();

  
  virtual PRUint32 AnchorCount();
  virtual Accessible* AnchorAt(PRUint32 aAnchorIndex);
  virtual already_AddRefed<nsIURI> AnchorURIAt(PRUint32 aAnchorIndex);

  


  void UpdateChildAreas(bool aDoFireEvents = true);

protected:

  
  virtual void CacheChildren();
};




class HTMLAreaAccessible : public HTMLLinkAccessible
{
public:

  HTMLAreaAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual bool IsPrimaryForNode() const;

  
  virtual void Description(nsString& aDescription);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual Accessible* ChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   EWhichChildAtPoint aWhichChild);
  virtual void GetBoundsRect(nsRect& aBounds, nsIFrame** aBoundingFrame);

  
  virtual PRUint32 StartOffset();
  virtual PRUint32 EndOffset();

protected:

  
  virtual void CacheChildren();
};

} 
} 




inline mozilla::a11y::HTMLImageMapAccessible*
Accessible::AsImageMap()
{
  return IsImageMapAccessible() ?
    static_cast<mozilla::a11y::HTMLImageMapAccessible*>(this) : nullptr;
}

#endif
