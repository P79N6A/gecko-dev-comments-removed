







































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
  nsCOMPtr<nsIAccessible> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  PRUint8 count = 0;
  nsresult rv = acc->GetNumActions(&count);
  *aNumActions = count;

  if (NS_SUCCEEDED(rv))
    return NS_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleAction::doAction(long aActionIndex)
{
__try {
  nsCOMPtr<nsIAccessible> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  PRUint8 index = static_cast<PRUint8>(aActionIndex);
  if (NS_SUCCEEDED(acc->DoAction(index)))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleAction::get_description(long aActionIndex, BSTR *aDescription)
{
__try {
  *aDescription = NULL;

  nsCOMPtr<nsIAccessible> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  nsAutoString description;
  PRUint8 index = static_cast<PRUint8>(aActionIndex);
  if (NS_FAILED(acc->GetActionDescription(index, description)))
    return E_FAIL;

  if (description.IsVoid())
    return S_FALSE;

  *aDescription = ::SysAllocStringLen(description.get(),
                                      description.Length());
  if (!*aDescription)
    return E_OUTOFMEMORY;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP
CAccessibleAction::get_keyBinding(long aActionIndex, long aNumMaxBinding,
                                 BSTR **aKeyBinding,
                                 long *aNumBinding)
{
__try {
  *aKeyBinding = NULL;
  *aNumBinding = 0;

  nsCOMPtr<nsIAccessible> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  nsCOMPtr<nsIDOMDOMStringList> keys;
  PRUint8 index = static_cast<PRUint8>(aActionIndex);
  nsresult rv = acc->GetKeyBindings(index, getter_AddRefs(keys));
  if (NS_FAILED(rv))
    return E_FAIL;

  PRUint32 length = 0;
  keys->GetLength(&length);

  PRBool aUseNumMaxBinding = length > static_cast<PRUint32>(aNumMaxBinding);

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
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP
CAccessibleAction::get_name(long aActionIndex, BSTR *aName)
{
__try {
  *aName = NULL;

  nsCOMPtr<nsIAccessible> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  nsAutoString name;
  PRUint8 index = static_cast<PRUint8>(aActionIndex);
  if (NS_FAILED(acc->GetActionName(index, name)))
    return E_FAIL;

  if (name.IsEmpty())
    return S_FALSE;

  *aName = ::SysAllocStringLen(name.get(), name.Length());
  if (!*aName)
    return E_OUTOFMEMORY;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP
CAccessibleAction::get_localizedName(long aActionIndex, BSTR *aLocalizedName)
{
__try {
  *aLocalizedName = NULL;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_NOTIMPL;
}

