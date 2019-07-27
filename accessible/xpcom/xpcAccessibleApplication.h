





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

  
  NS_IMETHOD GetAppName(nsAString& aName) final override;
  NS_IMETHOD GetAppVersion(nsAString& aVersion) final override;
  NS_IMETHOD GetPlatformName(nsAString& aName) final override;
  NS_IMETHOD GetPlatformVersion(nsAString& aVersion) final override;

protected:
  virtual ~xpcAccessibleApplication() {}

private:
  ApplicationAccessible* Intl() { return mIntl->AsApplication(); }

  xpcAccessibleApplication(const xpcAccessibleApplication&) = delete;
  xpcAccessibleApplication& operator =(const xpcAccessibleApplication&) = delete;
};

} 
} 

#endif
