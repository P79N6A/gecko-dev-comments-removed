




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

  
  virtual mozilla::a11y::ENameValueFlag Name(nsString& aName) MOZ_OVERRIDE;
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;

  
  virtual bool IsWidget() const MOZ_OVERRIDE;
  virtual Accessible* ContainerWidget() const MOZ_OVERRIDE;

protected:
  ~XULAlertAccessible();
};

} 
} 

#endif
