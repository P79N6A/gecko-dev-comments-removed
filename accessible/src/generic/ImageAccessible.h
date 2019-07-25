




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

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  NS_DECL_NSIACCESSIBLEIMAGE

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

  
  virtual PRUint8 ActionCount();

private:
  


  bool HasLongDesc() const
  {
    nsCOMPtr<nsIURI> uri = GetLongDescURI();
    return uri;
  }

  


  already_AddRefed<nsIURI> GetLongDescURI() const;

  










  inline bool IsLongDescIndex(PRUint8 aIndex);

};

} 
} 




inline mozilla::a11y::ImageAccessible*
Accessible::AsImage()
{
  return IsImage() ?
    static_cast<mozilla::a11y::ImageAccessible*>(this) : nsnull;
}

#endif

