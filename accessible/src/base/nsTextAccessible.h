





































#ifndef _nsTextAccessible_H_
#define _nsTextAccessible_H_

#include "nsBaseWidgetAccessible.h"




class nsTextAccessible : public nsLinkableAccessible
{
public:
  nsTextAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual void AppendTextTo(nsAString& aText, PRUint32 aStartOffset = 0,
                            PRUint32 aLength = PR_UINT32_MAX);

  
  void SetText(const nsAString& aText) { mText = aText; }
  const nsString& Text() const { return mText; }

protected:
  
  virtual void CacheChildren();

protected:
  nsString mText;
};





inline nsTextAccessible*
nsAccessible::AsTextLeaf()
{
  return mFlags & eTextLeafAccessible ?
    static_cast<nsTextAccessible*>(this) : nsnull;
}

#endif

