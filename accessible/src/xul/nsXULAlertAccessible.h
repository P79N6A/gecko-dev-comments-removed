




#ifndef _nsXULAlertAccessible_H_
#define _nsXULAlertAccessible_H_

#include "nsAccessibleWrap.h"





class nsXULAlertAccessible : public nsAccessibleWrap
{
public:
  nsXULAlertAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual mozilla::a11y::ENameValueFlag Name(nsString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual bool IsWidget() const;
  virtual nsAccessible* ContainerWidget() const;
};

#endif
