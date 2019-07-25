







































#include "CAccessibleAction.h"

#include "AccessibleAction_i.c"

#include "nsAccessible.h"



STDMETHODIMP
CAccessibleAction::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_IAccessibleAction == iid) {
    *ppv = static_cast<IAccessibleAction*>(this);
    (reinterpret_cast<IUnknown*>(*ppv))->AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}



STDMETHODIMP
CAccessibleAction::nActions(long* aActionCount)
{
__try {
  if (!aActionCount)
    return E_INVALIDARG;

  *aActionCount = 0;

  nsRefPtr<nsAccessible> acc(do_QueryObject(this));
  if (!acc || acc->IsDefunct())
    return E_FAIL;

  *aActionCount = acc->ActionCount();
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
CAccessibleAction::doAction(long aActionIndex)
{
__try {
  nsCOMPtr<nsIAccessible> acc(do_QueryObject(this));
  if (!acc)
    return E_FAIL;

  PRUint8 index = static_cast<PRUint8>(aActionIndex);
  nsresult rv = acc->DoAction(index);
  return GetHRESULT(rv);

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
CAccessibleAction::get_description(long aActionIndex, BSTR *aDescription)
{
__try {
  *aDescription = NULL;

  nsCOMPtr<nsIAccessible> acc(do_QueryObject(this));
  if (!acc)
    return E_FAIL;

  nsAutoString description;
  PRUint8 index = static_cast<PRUint8>(aActionIndex);
  nsresult rv = acc->GetActionDescription(index, description);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  if (description.IsEmpty())
    return S_FALSE;

  *aDescription = ::SysAllocStringLen(description.get(),
                                      description.Length());
  return *aDescription ? S_OK : E_OUTOFMEMORY;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
CAccessibleAction::get_keyBinding(long aActionIndex, long aNumMaxBinding,
                                  BSTR **aKeyBinding,
                                  long *aNumBinding)
{
__try {
  if (!aKeyBinding)
    return E_INVALIDARG;
  *aKeyBinding = NULL;

  if (!aNumBinding)
    return E_INVALIDARG;
  *aNumBinding = 0;

  if (aActionIndex != 0 || aNumMaxBinding < 1)
    return E_INVALIDARG;

  nsRefPtr<nsAccessible> acc(do_QueryObject(this));
  if (!acc || acc->IsDefunct())
    return E_FAIL;

  
  KeyBinding keyBinding = acc->AccessKey();
  if (keyBinding.IsEmpty())
    return S_FALSE;

  keyBinding = acc->KeyboardShortcut();
  if (keyBinding.IsEmpty())
    return S_FALSE;

  nsAutoString keyStr;
  keyBinding.ToString(keyStr);

  *aKeyBinding = static_cast<BSTR*>(::CoTaskMemAlloc(sizeof(BSTR*)));
  if (!*aKeyBinding)
    return E_OUTOFMEMORY;

  *(aKeyBinding[0]) = ::SysAllocStringLen(keyStr.get(), keyStr.Length());
  if (!*(aKeyBinding[0])) {
    ::CoTaskMemFree(*aKeyBinding);
    return E_OUTOFMEMORY;
  }

  *aNumBinding = 1;
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
CAccessibleAction::get_name(long aActionIndex, BSTR *aName)
{
__try {
  *aName = NULL;

  nsCOMPtr<nsIAccessible> acc(do_QueryObject(this));
  if (!acc)
    return E_FAIL;

  nsAutoString name;
  PRUint8 index = static_cast<PRUint8>(aActionIndex);
  nsresult rv = acc->GetActionName(index, name);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  if (name.IsEmpty())
    return S_FALSE;

  *aName = ::SysAllocStringLen(name.get(), name.Length());
  return *aName ? S_OK : E_OUTOFMEMORY;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
CAccessibleAction::get_localizedName(long aActionIndex, BSTR *aLocalizedName)
{
__try {
  *aLocalizedName = NULL;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_NOTIMPL;
}

