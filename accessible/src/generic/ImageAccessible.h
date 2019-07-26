




#ifndef mozilla_a11y_ImageAccessible_h__
#define mozilla_a11y_ImageAccessible_h__

#include "BaseAccessibles.h"
#include "nsIAccessibleImage.h"

class nsGenericHTMLElement;

namespace mozilla {
namespace a11y {






class ImageAccessible : public LinkableAccessible,
                        public nsIAccessibleImage
{
public:
  ImageAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);
  NS_IMETHOD DoAction(uint8_t index);

  
  NS_DECL_NSIACCESSIBLEIMAGE

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual a11y::role NativeRole();
  virtual uint64_t NativeState();
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

  
  virtual uint8_t ActionCount();

private:
  


  bool HasLongDesc() const
  {
    nsCOMPtr<nsIURI> uri = GetLongDescURI();
    return uri;
  }

  


  already_AddRefed<nsIURI> GetLongDescURI() const;

  










  inline bool IsLongDescIndex(uint8_t aIndex);

};

} 
} 




inline mozilla::a11y::ImageAccessible*
Accessible::AsImage()
{
  return IsImage() ?
    static_cast<mozilla::a11y::ImageAccessible*>(this) : nullptr;
}

#endif

