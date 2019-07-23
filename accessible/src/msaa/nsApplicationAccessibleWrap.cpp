







































#include "nsApplicationAccessibleWrap.h"

#include "AccessibleApplication_i.c"


NS_IMPL_ISUPPORTS_INHERITED0(nsApplicationAccessibleWrap,
                             nsApplicationAccessible)



STDMETHODIMP
nsApplicationAccessibleWrap::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_IAccessibleApplication == iid) {
    *ppv = NS_STATIC_CAST(IAccessibleApplication*, this);
    (NS_REINTERPRET_CAST(IUnknown*, *ppv))->AddRef();
    return S_OK;
  }

  return nsAccessibleWrap::QueryInterface(iid, ppv);
}



STDMETHODIMP
nsApplicationAccessibleWrap::get_appName(BSTR *aName)
{
  return E_NOTIMPL;
}

STDMETHODIMP
nsApplicationAccessibleWrap::get_appVersion(BSTR *aVersion)
{
  return E_NOTIMPL;
}

STDMETHODIMP
nsApplicationAccessibleWrap::get_toolkitName(BSTR *aName)
{
  return E_NOTIMPL;
}

STDMETHODIMP
nsApplicationAccessibleWrap::get_toolkitVersion(BSTR *aVersion)
{
  return E_NOTIMPL;
}



void
nsApplicationAccessibleWrap::PreCreate()
{
}

