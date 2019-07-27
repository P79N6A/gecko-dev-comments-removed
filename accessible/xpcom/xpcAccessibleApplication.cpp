





#include "xpcAccessibleApplication.h"

#include "ApplicationAccessible.h"

using namespace mozilla::a11y;




NS_IMPL_ISUPPORTS_INHERITED(xpcAccessibleApplication,
                            xpcAccessibleGeneric,
                            nsIAccessibleApplication)




NS_IMETHODIMP
xpcAccessibleApplication::GetAppName(nsAString& aName)
{
  aName.Truncate();

  if (!Intl())
    return NS_ERROR_FAILURE;

  Intl()->AppName(aName);
  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleApplication::GetAppVersion(nsAString& aVersion)
{
  aVersion.Truncate();

  if (!Intl())
    return NS_ERROR_FAILURE;

  Intl()->AppVersion(aVersion);
  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleApplication::GetPlatformName(nsAString& aName)
{
  aName.Truncate();

  if (!Intl())
    return NS_ERROR_FAILURE;

  Intl()->PlatformName(aName);
  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleApplication::GetPlatformVersion(nsAString& aVersion)
{
  aVersion.Truncate();

  if (!Intl())
    return NS_ERROR_FAILURE;

  Intl()->PlatformVersion(aVersion);
  return NS_OK;
}
