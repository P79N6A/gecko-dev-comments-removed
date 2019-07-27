






#include "ApplicationAccessibleWrap.h"

#include "AccessibleApplication_i.c"
#include "IUnknownImpl.h"

#include "nsIGfxInfo.h"
#include "nsIPersistentProperties2.h"
#include "nsServiceManagerUtils.h"

using namespace mozilla;
using namespace mozilla::a11y;



NS_IMPL_ISUPPORTS_INHERITED0(ApplicationAccessibleWrap,
                             ApplicationAccessible)

already_AddRefed<nsIPersistentProperties>
ApplicationAccessibleWrap::NativeAttributes()
{
  nsCOMPtr<nsIPersistentProperties> attributes =
    do_CreateInstance(NS_PERSISTENTPROPERTIES_CONTRACTID);

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

  return attributes.forget();
}




STDMETHODIMP
ApplicationAccessibleWrap::QueryInterface(REFIID iid, void** ppv)
{
  if (!ppv)
    return E_INVALIDARG;

  *ppv = nullptr;

  if (IID_IAccessibleApplication == iid) {
    *ppv = static_cast<IAccessibleApplication*>(this);
    (reinterpret_cast<IUnknown*>(*ppv))->AddRef();
    return S_OK;
  }

  return AccessibleWrap::QueryInterface(iid, ppv);
}




STDMETHODIMP
ApplicationAccessibleWrap::get_appName(BSTR* aName)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aName)
    return E_INVALIDARG;

  *aName = nullptr;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  nsAutoString name;
  AppName(name);
  if (name.IsEmpty())
    return S_FALSE;

  *aName = ::SysAllocStringLen(name.get(), name.Length());
  return *aName ? S_OK : E_OUTOFMEMORY;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ApplicationAccessibleWrap::get_appVersion(BSTR* aVersion)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aVersion)
    return E_INVALIDARG;

  *aVersion = nullptr;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  nsAutoString version;
  AppVersion(version);
  if (version.IsEmpty())
    return S_FALSE;

  *aVersion = ::SysAllocStringLen(version.get(), version.Length());
  return *aVersion ? S_OK : E_OUTOFMEMORY;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ApplicationAccessibleWrap::get_toolkitName(BSTR* aName)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aName)
    return E_INVALIDARG;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  nsAutoString name;
  PlatformName(name);
  if (name.IsEmpty())
    return S_FALSE;

  *aName = ::SysAllocStringLen(name.get(), name.Length());
  return *aName ? S_OK : E_OUTOFMEMORY;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ApplicationAccessibleWrap::get_toolkitVersion(BSTR* aVersion)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aVersion)
    return E_INVALIDARG;

  *aVersion = nullptr;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  nsAutoString version;
  PlatformVersion(version);
  if (version.IsEmpty())
    return S_FALSE;

  *aVersion = ::SysAllocStringLen(version.get(), version.Length());
  return *aVersion ? S_OK : E_OUTOFMEMORY;

  A11Y_TRYBLOCK_END
}

