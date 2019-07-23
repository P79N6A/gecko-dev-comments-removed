





































#include "nsDOMWorkerNavigator.h"

#include "nsIClassInfoImpl.h"
#include "nsStringGlue.h"

#include "nsDOMThreadService.h"
#include "nsDOMWorkerMacros.h"

#define XPC_MAP_CLASSNAME nsDOMWorkerNavigator
#define XPC_MAP_QUOTED_CLASSNAME "Navigator"

#define XPC_MAP_FLAGS                                      \
  nsIXPCScriptable::USE_JSSTUB_FOR_ADDPROPERTY           | \
  nsIXPCScriptable::USE_JSSTUB_FOR_DELPROPERTY           | \
  nsIXPCScriptable::USE_JSSTUB_FOR_SETPROPERTY           | \
  nsIXPCScriptable::DONT_ENUM_QUERY_INTERFACE            | \
  nsIXPCScriptable::CLASSINFO_INTERFACES_ONLY            | \
  nsIXPCScriptable::DONT_REFLECT_INTERFACE_NAMES

#include "xpc_map_end.h"

NS_IMPL_THREADSAFE_ISUPPORTS3(nsDOMWorkerNavigator, nsIWorkerNavigator,
                                                    nsIClassInfo,
                                                    nsIXPCScriptable)

NS_IMPL_CI_INTERFACE_GETTER1(nsDOMWorkerNavigator, nsIWorkerNavigator)

NS_IMPL_THREADSAFE_DOM_CI_GETINTERFACES(nsDOMWorkerNavigator)
NS_IMPL_THREADSAFE_DOM_CI_ALL_THE_REST(nsDOMWorkerNavigator)

NS_IMETHODIMP
nsDOMWorkerNavigator::GetHelperForLanguage(PRUint32 aLanguage,
                                           nsISupports** _retval)
{
  if (aLanguage == nsIProgrammingLanguage::JAVASCRIPT) {
    NS_ADDREF(*_retval = NS_ISUPPORTS_CAST(nsIWorkerNavigator*, this));
  }
  else {
    *_retval = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerNavigator::GetAppName(nsAString& aAppName)
{
  nsDOMThreadService::get()->GetAppName(aAppName);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerNavigator::GetAppVersion(nsAString& aAppVersion)
{
  nsDOMThreadService::get()->GetAppVersion(aAppVersion);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerNavigator::GetPlatform(nsAString& aPlatform)
{
  nsDOMThreadService::get()->GetPlatform(aPlatform);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerNavigator::GetUserAgent(nsAString& aUserAgent)
{
  nsDOMThreadService::get()->GetUserAgent(aUserAgent);
  return NS_OK;
}
