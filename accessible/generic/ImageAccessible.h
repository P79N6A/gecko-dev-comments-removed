




#ifndef mozilla_a11y_ImageAccessible_h__
#define mozilla_a11y_ImageAccessible_h__

#include "BaseAccessibles.h"
#include "xpcAccessibleImage.h"

class nsGenericHTMLElement;

namespace mozilla {
namespace a11y {






class ImageAccessible : public LinkableAccessible,
                        public xpcAccessibleImage
{
public:
  ImageAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;
  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes() MOZ_OVERRIDE;

  
  virtual uint8_t ActionCount() MOZ_OVERRIDE;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) MOZ_OVERRIDE;
  virtual bool DoAction(uint8_t aIndex) MOZ_OVERRIDE;

  
  nsIntPoint Position(uint32_t aCoordType);
  nsIntSize Size();

protected:
  virtual ~ImageAccessible();

  
  virtual ENameValueFlag NativeName(nsString& aName) MOZ_OVERRIDE;

private:
  


  bool HasLongDesc() const
  {
    nsCOMPtr<nsIURI> uri = GetLongDescURI();
    return uri;
  }

  


  already_AddRefed<nsIURI> GetLongDescURI() const;

  










  inline bool IsLongDescIndex(uint8_t aIndex);

};




inline ImageAccessible*
Accessible::AsImage()
{
  return IsImage() ? static_cast<ImageAccessible*>(this) : nullptr;
}

} 
} 

#endif

