




#ifndef mozilla_a11y_HTMLImageMapAccessible_h__
#define mozilla_a11y_HTMLImageMapAccessible_h__

#include "HTMLLinkAccessible.h"
#include "ImageAccessibleWrap.h"
#include "nsIDOMHTMLMapElement.h"

namespace mozilla {
namespace a11y {




class HTMLImageMapAccessible MOZ_FINAL : public ImageAccessibleWrap
{
public:
  HTMLImageMapAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual a11y::role NativeRole() MOZ_OVERRIDE;

  
  virtual uint32_t AnchorCount();
  virtual Accessible* AnchorAt(uint32_t aAnchorIndex);
  virtual already_AddRefed<nsIURI> AnchorURIAt(uint32_t aAnchorIndex);

  


  void UpdateChildAreas(bool aDoFireEvents = true);

  


  Accessible* GetChildAccessibleFor(const nsINode* aNode) const;

protected:
  virtual ~HTMLImageMapAccessible() { }

  
  virtual void CacheChildren();
};




class HTMLAreaAccessible MOZ_FINAL : public HTMLLinkAccessible
{
public:

  HTMLAreaAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual void Description(nsString& aDescription);
  virtual Accessible* ChildAtPoint(int32_t aX, int32_t aY,
                                   EWhichChildAtPoint aWhichChild);
  virtual nsRect RelativeBounds(nsIFrame** aBoundingFrame) const MOZ_OVERRIDE;

  
  virtual uint32_t StartOffset();
  virtual uint32_t EndOffset();

protected:
  
  virtual ENameValueFlag NativeName(nsString& aName) MOZ_OVERRIDE;
  virtual void CacheChildren();
};





inline HTMLImageMapAccessible*
Accessible::AsImageMap()
{
  return IsImageMap() ? static_cast<HTMLImageMapAccessible*>(this) : nullptr;
}

} 
} 

#endif
