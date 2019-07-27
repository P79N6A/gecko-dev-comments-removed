




#ifndef mozilla_a11y_XULAlertAccessible_h__
#define mozilla_a11y_XULAlertAccessible_h__

#include "AccessibleWrap.h"

namespace mozilla {
namespace a11y {





class XULAlertAccessible : public AccessibleWrap
{
public:
  XULAlertAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual mozilla::a11y::ENameValueFlag Name(nsString& aName) override;
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;

  
  virtual bool IsWidget() const override;
  virtual Accessible* ContainerWidget() const override;

protected:
  ~XULAlertAccessible();
};

} 
} 

#endif
