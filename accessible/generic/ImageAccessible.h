




#ifndef mozilla_a11y_ImageAccessible_h__
#define mozilla_a11y_ImageAccessible_h__

#include "BaseAccessibles.h"

namespace mozilla {
namespace a11y {






class ImageAccessible : public LinkableAccessible
{
public:
  ImageAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;
  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes() override;

  
  virtual uint8_t ActionCount() override;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) override;
  virtual bool DoAction(uint8_t aIndex) override;

  
  nsIntPoint Position(uint32_t aCoordType);
  nsIntSize Size();

protected:
  virtual ~ImageAccessible();

  
  virtual ENameValueFlag NativeName(nsString& aName) override;

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

