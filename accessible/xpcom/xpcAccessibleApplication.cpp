





#include "xpcAccessibleApplication.h"

#include "ApplicationAccessible.h"

using namespace mozilla::a11y;

NS_IMETHODIMP
xpcAccessibleApplication::GetAppName(nsAString& aName)
{
  aName.Truncate();

  if (static_cast<ApplicationAccessible*>(this)->IsDefunct())
    return NS_ERROR_FAILURE;

  static_cast<ApplicationAccessible*>(this)->AppName(aName);
  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleApplication::GetAppVersion(nsAString& aVersion)
{
  aVersion.Truncate();

  if (static_cast<ApplicationAccessible*>(this)->IsDefunct())
    return NS_ERROR_FAILURE;

  static_cast<ApplicationAccessible*>(this)->AppVersion(aVersion);
  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleApplication::GetPlatformName(nsAString& aName)
{
  aName.Truncate();

  if (static_cast<ApplicationAccessible*>(this)->IsDefunct())
    return NS_ERROR_FAILURE;

  static_cast<ApplicationAccessible*>(this)->PlatformName(aName);
  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleApplication::GetPlatformVersion(nsAString& aVersion)
{
  aVersion.Truncate();

  if (static_cast<ApplicationAccessible*>(this)->IsDefunct())
    return NS_ERROR_FAILURE;

  static_cast<ApplicationAccessible*>(this)->PlatformVersion(aVersion);
  return NS_OK;
}
