




#ifndef mozilla_a11y_TextLeafAccessible_h__
#define mozilla_a11y_TextLeafAccessible_h__

#include "BaseAccessibles.h"

namespace mozilla {
namespace a11y {
 



class TextLeafAccessible : public LinkableAccessible
{
public:
  TextLeafAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~TextLeafAccessible();

  
  virtual mozilla::a11y::role NativeRole() MOZ_OVERRIDE;
  virtual void AppendTextTo(nsAString& aText, uint32_t aStartOffset = 0,
                            uint32_t aLength = UINT32_MAX);
  virtual ENameValueFlag Name(nsString& aName);

  
  void SetText(const nsAString& aText) { mText = aText; }
  const nsString& Text() const { return mText; }

protected:
  
  virtual void CacheChildren();

protected:
  nsString mText;
};





inline TextLeafAccessible*
Accessible::AsTextLeaf()
{
  return IsTextLeaf() ? static_cast<TextLeafAccessible*>(this) : nullptr;
}

} 
} 

#endif

