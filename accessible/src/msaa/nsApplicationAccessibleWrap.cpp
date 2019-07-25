







































#include "nsApplicationAccessibleWrap.h"

#include "AccessibleApplication_i.c"

#include "nsIGfxInfo.h"
#include "nsIPersistentProperties2.h"
#include "nsServiceManagerUtils.h"



NS_IMPL_ISUPPORTS_INHERITED0(nsApplicationAccessibleWrap,
                             nsApplicationAccessible)

NS_IMETHODIMP
nsApplicationAccessibleWrap::GetAttributes(nsIPersistentProperties** aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);
  *aAttributes = nsnull;

  nsCOMPtr<nsIPersistentProperties> attributes =
    do_CreateInstance(NS_PERSISTENTPROPERTIES_CONTRACTID);
  NS_ENSURE_STATE(attributes);

  nsCOMPtr<nsIGfxInfo> gfxInfo = do_GetService("@mozilla.org/gfx/info;1");
  if (gfxInfo) {
    bool isD2DEnabled = false;
    gfxInfo->GetD2DEnabled(&isD2DEnabled);
    nsAutoString unused;
    attributes->SetStringProperty(
      NS_LITERAL_CSTRING("D2D"),
      isD2DEnabled ? NS_LITERAL_STRING("true") : NS_LITERAL_STRING("false"),
        unused);
  }

  attributes.swap(*aAttributes);
  return NS_OK;
}




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

