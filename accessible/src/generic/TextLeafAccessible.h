




#ifndef mozilla_a11y_TextLeafAccessible_h__
#define mozilla_a11y_TextLeafAccessible_h__

#include "nsBaseWidgetAccessible.h"

namespace mozilla {
namespace a11y {
 



class TextLeafAccessible : public nsLinkableAccessible
{
public:
  TextLeafAccessible(nsIContent* aContent, nsDocAccessible* aDoc);
  virtual ~TextLeafAccessible();

  
  virtual mozilla::a11y::role NativeRole();
  virtual void AppendTextTo(nsAString& aText, PRUint32 aStartOffset = 0,
                            PRUint32 aLength = PR_UINT32_MAX);
  virtual ENameValueFlag Name(nsString& aName);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties* aAttributes);

  
  void SetText(const nsAString& aText) { mText = aText; }
  const nsString& Text() const { return mText; }

protected:
  
  virtual void CacheChildren();

protected:
  nsString mText;
};

} 
} 




inline mozilla::a11y::TextLeafAccessible*
nsAccessible::AsTextLeaf()
{
  return mFlags & eTextLeafAccessible ?
    static_cast<mozilla::a11y::TextLeafAccessible*>(this) : nsnull;
}

#endif

