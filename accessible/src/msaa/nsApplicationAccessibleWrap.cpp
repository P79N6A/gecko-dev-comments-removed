







































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
__try {
  *aName = NULL;

  if (!sAppInfo)
    return E_FAIL;

  nsCAutoString cname;
  nsresult rv = sAppInfo->GetName(cname);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  if (cname.IsEmpty())
    return S_FALSE;

  NS_ConvertUTF8toUTF16 name(cname);
  *aName = ::SysAllocStringLen(name.get(), name.Length());
  return *aName ? S_OK : E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
nsApplicationAccessibleWrap::get_appVersion(BSTR *aVersion)
{
__try {
  *aVersion = NULL;

  if (!sAppInfo)
    return E_FAIL;

  nsCAutoString cversion;
  nsresult rv = sAppInfo->GetVersion(cversion);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  if (cversion.IsEmpty())
    return S_FALSE;

  NS_ConvertUTF8toUTF16 version(cversion);
  *aVersion = ::SysAllocStringLen(version.get(), version.Length());
  return *aVersion ? S_OK : E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
nsApplicationAccessibleWrap::get_toolkitName(BSTR *aName)
{
__try {
  *aName = ::SysAllocString(L"Gecko");
  return *aName ? S_OK : E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
nsApplicationAccessibleWrap::get_toolkitVersion(BSTR *aVersion)
{
__try {
  *aVersion = NULL;

  if (!sAppInfo)
    return E_FAIL;

  nsCAutoString cversion;
  nsresult rv = sAppInfo->GetPlatformVersion(cversion);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  if (cversion.IsEmpty())
    return S_FALSE;

  NS_ConvertUTF8toUTF16 version(cversion);
  *aVersion = ::SysAllocStringLen(version.get(), version.Length());
  return *aVersion ? S_OK : E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
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

