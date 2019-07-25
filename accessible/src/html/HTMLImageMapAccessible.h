




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

  
  virtual uint32_t AnchorCount();
  virtual Accessible* AnchorAt(uint32_t aAnchorIndex);
  virtual already_AddRefed<nsIURI> AnchorURIAt(uint32_t aAnchorIndex);

  


  void UpdateChildAreas(bool aDoFireEvents = true);

protected:

  
  virtual void CacheChildren();
};




class HTMLAreaAccessible : public HTMLLinkAccessible
{
public:

  HTMLAreaAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual void Description(nsString& aDescription);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual Accessible* ChildAtPoint(int32_t aX, int32_t aY,
                                   EWhichChildAtPoint aWhichChild);
  virtual void GetBoundsRect(nsRect& aBounds, nsIFrame** aBoundingFrame);

  
  virtual uint32_t StartOffset();
  virtual uint32_t EndOffset();

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
