







































#include "nsApplicationAccessibleWrap.h"

#include "AccessibleApplication_i.c"

#include "nsServiceManagerUtils.h"



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

  nsAutoString name;
  nsresult rv = GetAppName(name);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  if (name.IsEmpty())
    return S_FALSE;

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

  nsAutoString version;
  nsresult rv = GetAppVersion(version);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  if (version.IsEmpty())
    return S_FALSE;

  *aVersion = ::SysAllocStringLen(version.get(), version.Length());
  return *aVersion ? S_OK : E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
nsApplicationAccessibleWrap::get_toolkitName(BSTR *aName)
{
__try {
  nsAutoString name;
  nsresult rv = GetPlatformName(name);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  if (name.IsEmpty())
    return S_FALSE;

  *aName = ::SysAllocStringLen(name.get(), name.Length());
  return *aName ? S_OK : E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
nsApplicationAccessibleWrap::get_toolkitVersion(BSTR *aVersion)
{
__try {
  *aVersion = NULL;

  nsAutoString version;
  nsresult rv = GetPlatformVersion(version);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  if (version.IsEmpty())
    return S_FALSE;

  *aVersion = ::SysAllocStringLen(version.get(), version.Length());
  return *aVersion ? S_OK : E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}




void
nsApplicationAccessibleWrap::PreCreate()
{
}

void
nsApplicationAccessibleWrap::Unload()
{
}

