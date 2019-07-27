





#ifndef mozilla_a11y_xpcAccessibleApplication_h_
#define mozilla_a11y_xpcAccessibleApplication_h_

#include "nsIAccessibleApplication.h"
#include "ApplicationAccessible.h"
#include "xpcAccessibleGeneric.h"

namespace mozilla {
namespace a11y {




class xpcAccessibleApplication : public xpcAccessibleGeneric,
                                 public nsIAccessibleApplication
{
public:
  explicit xpcAccessibleApplication(Accessible* aIntl) :
    xpcAccessibleGeneric(aIntl) { }

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetAppName(nsAString& aName) MOZ_FINAL;
  NS_IMETHOD GetAppVersion(nsAString& aVersion) MOZ_FINAL;
  NS_IMETHOD GetPlatformName(nsAString& aName) MOZ_FINAL;
  NS_IMETHOD GetPlatformVersion(nsAString& aVersion) MOZ_FINAL;

protected:
  virtual ~xpcAccessibleApplication() {}

private:
  ApplicationAccessible* Intl() { return mIntl->AsApplication(); }

  xpcAccessibleApplication(const xpcAccessibleApplication&) MOZ_DELETE;
  xpcAccessibleApplication& operator =(const xpcAccessibleApplication&) MOZ_DELETE;
};

} 
} 

#endif
