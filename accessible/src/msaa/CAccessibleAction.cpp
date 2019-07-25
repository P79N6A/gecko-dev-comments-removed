







































#include "CAccessibleAction.h"

#include "AccessibleAction_i.c"

#include "nsIAccessible.h"
#include "nsAccessNodeWrap.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIDOMDOMStringList.h"



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
CAccessibleAction::nActions(long *aNumActions)
{
__try {
  *aNumActions = 0;

  nsCOMPtr<nsIAccessible> acc(do_QueryObject(this));
  if (!acc)
    return E_FAIL;

  PRUint8 count = 0;
  nsresult rv = acc->GetNumActions(&count);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  *aNumActions = count;
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
  *aKeyBinding = NULL;
  *aNumBinding = 0;

  nsCOMPtr<nsIAccessible> acc(do_QueryObject(this));
  if (!acc)
    return E_FAIL;

  nsCOMPtr<nsIDOMDOMStringList> keys;
  PRUint8 index = static_cast<PRUint8>(aActionIndex);
  nsresult rv = acc->GetKeyBindings(index, getter_AddRefs(keys));
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  PRUint32 length = 0;
  keys->GetLength(&length);
  if (length == 0)
    return S_FALSE;

  PRUint32 maxBinding = static_cast<PRUint32>(aNumMaxBinding);
  PRUint32 numBinding = length > maxBinding ? maxBinding : length;
  *aNumBinding = numBinding;

  *aKeyBinding = static_cast<BSTR*>(nsMemory::Alloc((numBinding) * sizeof(BSTR*)));
  if (!*aKeyBinding)
    return E_OUTOFMEMORY;

  PRBool outOfMemory = PR_FALSE;
  PRUint32 i = 0;
  for (; i < numBinding; i++) {
    nsAutoString key;
    keys->Item(i, key);
    *(aKeyBinding[i]) = ::SysAllocStringLen(key.get(), key.Length());

    if (!*(aKeyBinding[i])) {
      outOfMemory = PR_TRUE;
      break;
    }
  }

  if (outOfMemory) {
    for (PRUint32 j = 0; j < i; j++)
      ::SysFreeString(*(aKeyBinding[j]));

    nsMemory::Free(*aKeyBinding);
    *aKeyBinding = NULL;

    return E_OUTOFMEMORY;
  }
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

