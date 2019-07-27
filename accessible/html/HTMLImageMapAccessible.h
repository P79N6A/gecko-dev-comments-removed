




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

  
  virtual uint32_t AnchorCount() MOZ_OVERRIDE;
  virtual Accessible* AnchorAt(uint32_t aAnchorIndex) MOZ_OVERRIDE;
  virtual already_AddRefed<nsIURI> AnchorURIAt(uint32_t aAnchorIndex) MOZ_OVERRIDE;

  


  void UpdateChildAreas(bool aDoFireEvents = true);

  


  Accessible* GetChildAccessibleFor(const nsINode* aNode) const;

protected:
  virtual ~HTMLImageMapAccessible() { }

  
  virtual void CacheChildren() MOZ_OVERRIDE;
};




class HTMLAreaAccessible MOZ_FINAL : public HTMLLinkAccessible
{
public:

  HTMLAreaAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual void Description(nsString& aDescription) MOZ_OVERRIDE;
  virtual Accessible* ChildAtPoint(int32_t aX, int32_t aY,
                                   EWhichChildAtPoint aWhichChild) MOZ_OVERRIDE;
  virtual nsRect RelativeBounds(nsIFrame** aBoundingFrame) const MOZ_OVERRIDE;

  
  virtual uint32_t StartOffset() MOZ_OVERRIDE;
  virtual uint32_t EndOffset() MOZ_OVERRIDE;

protected:
  
  virtual ENameValueFlag NativeName(nsString& aName) MOZ_OVERRIDE;
  virtual void CacheChildren() MOZ_OVERRIDE;
};





inline HTMLImageMapAccessible*
Accessible::AsImageMap()
{
  return IsImageMap() ? static_cast<HTMLImageMapAccessible*>(this) : nullptr;
}

} 
} 

#endif
