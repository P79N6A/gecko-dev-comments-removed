




#include "nsShellService.h"
#include "nsString.h"

#include "AndroidBridge.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS(nsShellService, nsIShellService)

NS_IMETHODIMP
nsShellService::SwitchTask()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsShellService::CreateShortcut(const nsAString& aTitle, const nsAString& aURI,
                                const nsAString& aIconData, const nsAString& aIntent)
{
  if (!aTitle.Length() || !aURI.Length() || !aIconData.Length())
    return NS_ERROR_FAILURE;

  widget::GeckoAppShell::CreateShortcut(aTitle, aURI, aIconData);
  return NS_OK;
}
