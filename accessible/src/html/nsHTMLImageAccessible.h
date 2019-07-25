





































#ifndef _nsHTMLImageAccessible_H_
#define _nsHTMLImageAccessible_H_

#include "nsBaseWidgetAccessible.h"
#include "nsIAccessibleImage.h"






class nsHTMLImageAccessible : public nsLinkableAccessible,
                              public nsIAccessibleImage
{
public:
  nsHTMLImageAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  NS_DECL_NSIACCESSIBLEIMAGE

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

  
  virtual PRUint8 ActionCount();

private:
  




  bool HasLongDesc();
  
  








  bool IsValidLongDescIndex(PRUint8 aIndex);
};



inline nsHTMLImageAccessible*
nsAccessible::AsImage()
{
  return IsImageAccessible() ?
    static_cast<nsHTMLImageAccessible*>(this) : nsnull;
}

#endif

