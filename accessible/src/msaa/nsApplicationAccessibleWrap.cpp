







































#include "nsApplicationAccessibleWrap.h"

#include "AccessibleApplication_i.c"

#include "nsServiceManagerUtils.h"

nsIXULAppInfo* nsApplicationAccessibleWrap::sAppInfo = nsnull;


NS_IMPL_ISUPPORTS_INHERITED0(nsApplicationAccessibleWrap,
                             nsApplicationAccessible)



STDMETHODIMP
nsApplicationAccessibleWrap::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_IAccessibleApplication == iid) {
    *ppv = static_cast<IAccessibleApplication*>(this);
    (reinterpret_cast<IUnknown*>(*ppv))->AddRef();
    return S_OK;
  }

  return nsAccessibleWrap::QueryInterface(iid, ppv);
}



STDMETHODIMP
nsApplicationAccessibleWrap::get_appName(BSTR *aName)
{
  if (!sAppInfo)
    return E_FAIL;

  nsCAutoString cname;
  nsresult rv = sAppInfo->GetName(cname);

  if (NS_FAILED(rv))
    return E_FAIL;

  NS_ConvertUTF8toUTF16 name(cname);
  return ::SysReAllocStringLen(aName, name.get(), name.Length());
}

STDMETHODIMP
nsApplicationAccessibleWrap::get_appVersion(BSTR *aVersion)
{
  if (!sAppInfo)
    return E_FAIL;

  nsCAutoString cversion;
  nsresult rv = sAppInfo->GetVersion(cversion);

  if (NS_FAILED(rv))
    return E_FAIL;

  NS_ConvertUTF8toUTF16 version(cversion);
  return ::SysReAllocStringLen(aVersion, version.get(), version.Length());
}

STDMETHODIMP
nsApplicationAccessibleWrap::get_toolkitName(BSTR *aName)
{
  return ::SysReAllocString(aName, L"Gecko");
}

STDMETHODIMP
nsApplicationAccessibleWrap::get_toolkitVersion(BSTR *aVersion)
{
  if (!sAppInfo)
    return E_FAIL;

  nsCAutoString cversion;
  nsresult rv = sAppInfo->GetPlatformVersion(cversion);

  if (NS_FAILED(rv))
    return E_FAIL;

  NS_ConvertUTF8toUTF16 version(cversion);
  return ::SysReAllocStringLen(aVersion, version.get(), version.Length());
}



void
nsApplicationAccessibleWrap::PreCreate()
{
  nsresult rv = CallGetService("@mozilla.org/xre/app-info;1", &sAppInfo);
  NS_ASSERTION(NS_SUCCEEDED(rv), "No XUL application info service");
}

void
nsApplicationAccessibleWrap::Unload()
{
  NS_IF_RELEASE(sAppInfo);
}

