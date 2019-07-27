




#ifndef mozilla_a11y_HTMLImageMapAccessible_h__
#define mozilla_a11y_HTMLImageMapAccessible_h__

#include "HTMLLinkAccessible.h"
#include "ImageAccessibleWrap.h"
#include "nsIDOMHTMLMapElement.h"

namespace mozilla {
namespace a11y {




class HTMLImageMapAccessible final : public ImageAccessibleWrap
{
public:
  HTMLImageMapAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual a11y::role NativeRole() override;

  
  virtual uint32_t AnchorCount() override;
  virtual Accessible* AnchorAt(uint32_t aAnchorIndex) override;
  virtual already_AddRefed<nsIURI> AnchorURIAt(uint32_t aAnchorIndex) override;

  


  void UpdateChildAreas(bool aDoFireEvents = true);

  


  Accessible* GetChildAccessibleFor(const nsINode* aNode) const;

protected:
  virtual ~HTMLImageMapAccessible() { }

  
  virtual void CacheChildren() override;
};




class HTMLAreaAccessible final : public HTMLLinkAccessible
{
public:

  HTMLAreaAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual void Description(nsString& aDescription) override;
  virtual Accessible* ChildAtPoint(int32_t aX, int32_t aY,
                                   EWhichChildAtPoint aWhichChild) override;
  virtual nsRect RelativeBounds(nsIFrame** aBoundingFrame) const override;

  
  virtual uint32_t StartOffset() override;
  virtual uint32_t EndOffset() override;

protected:
  
  virtual ENameValueFlag NativeName(nsString& aName) override;
  virtual void CacheChildren() override;
};





inline HTMLImageMapAccessible*
Accessible::AsImageMap()
{
  return IsImageMap() ? static_cast<HTMLImageMapAccessible*>(this) : nullptr;
}

} 
} 

#endif
