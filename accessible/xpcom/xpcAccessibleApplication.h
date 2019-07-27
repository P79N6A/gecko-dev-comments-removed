





#ifndef mozilla_a11y_xpcAccessibleApplication_h_
#define mozilla_a11y_xpcAccessibleApplication_h_

#include "nsIAccessibleApplication.h"

class nsIAccessible;

namespace mozilla {
namespace a11y {

class xpcAccessibleApplication : public nsIAccessibleApplication
{
public:
  NS_IMETHOD GetAppName(nsAString& aName) MOZ_FINAL;
  NS_IMETHOD GetAppVersion(nsAString& aVersion) MOZ_FINAL;
  NS_IMETHOD GetPlatformName(nsAString& aName) MOZ_FINAL;
  NS_IMETHOD GetPlatformVersion(nsAString& aVersion) MOZ_FINAL;

private:
  xpcAccessibleApplication() { }
  friend class ApplicationAccessible;

  xpcAccessibleApplication(const xpcAccessibleApplication&) MOZ_DELETE;
  xpcAccessibleApplication& operator =(const xpcAccessibleApplication&) MOZ_DELETE;
};

} 
} 

#endif
