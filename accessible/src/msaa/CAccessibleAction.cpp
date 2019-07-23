







































#include "CAccessibleAction.h"

#include "AccessibleAction_i.c"

#include "nsIAccessible.h"

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIDOMDOMStringList.h"



STDMETHODIMP
CAccessibleAction::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_IAccessibleAction == iid) {
    nsCOMPtr<nsIAccessible> acc(do_QueryInterface(this));
    if (!acc)
      return E_FAIL;

    *ppv = NS_STATIC_CAST(IAccessibleAction*, this);
    (NS_REINTERPRET_CAST(IUnknown*, *ppv))->AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}



STDMETHODIMP
CAccessibleAction::nActions(long *aNumActions)
{
  nsCOMPtr<nsIAccessible> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  PRUint8 count = 0;
  nsresult rv = acc->GetNumActions(&count);
  *aNumActions = count;

  if (NS_SUCCEEDED(rv))
    return NS_OK;
  return E_FAIL;
}

STDMETHODIMP
CAccessibleAction::doAction(long aActionIndex)
{
  nsCOMPtr<nsIAccessible> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  PRUint8 index = NS_STATIC_CAST(PRUint8, aActionIndex);
  if (NS_SUCCEEDED(acc->DoAction(index)))
    return S_OK;
  return E_FAIL;
}

STDMETHODIMP
CAccessibleAction::get_description(long aActionIndex, BSTR *aDescription)
{
  *aDescription = NULL;

  nsCOMPtr<nsIAccessible> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  nsAutoString description;
  PRUint8 index = NS_STATIC_CAST(PRUint8, aActionIndex);
  if (NS_FAILED(acc->GetActionDescription(index, description)))
    return E_FAIL;

  if (!description.IsVoid()) {
    return ::SysReAllocStringLen(aDescription, description.get(),
                                 description.Length());
  }

  return S_OK;
}

STDMETHODIMP
CAccessibleAction::get_keyBinding(long aActionIndex, long aNumMaxBinding,
                                 BSTR **aKeyBinding,
                                 long *aNumBinding)
{
  *aKeyBinding = NULL;
  aNumBinding = 0;

  nsCOMPtr<nsIAccessible> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  nsCOMPtr<nsIDOMDOMStringList> keys;
  PRUint8 index = NS_STATIC_CAST(PRUint8, aActionIndex);
  nsresult rv = acc->GetKeyBindings(index, getter_AddRefs(keys));
  if (NS_FAILED(rv))
    return E_FAIL;

  PRUint32 length = 0;
  keys->GetLength(&length);

  PRBool aUseNumMaxBinding = length > NS_STATIC_CAST(PRUint32, aNumMaxBinding);

  PRUint32 maxBinding = NS_STATIC_CAST(PRUint32, aNumMaxBinding);

  PRUint32 numBinding = length > maxBinding ? maxBinding : length;
  *aNumBinding = numBinding;

  *aKeyBinding = new BSTR[numBinding];
  if (!*aKeyBinding)
    return E_OUTOFMEMORY;

  for (PRUint32 i = 0; i < numBinding; i++) {
    nsAutoString key;
    keys->Item(i, key);
    HRESULT hr = ::SysReAllocStringLen(aKeyBinding[i], key.get(),
                                       key.Length());
    if (FAILED(hr))
      return hr;
  }

  return S_OK;
}

STDMETHODIMP
CAccessibleAction::get_name(long aActionIndex, BSTR *aName)
{
  *aName = NULL;

  nsCOMPtr<nsIAccessible> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  nsAutoString name;
  PRUint8 index = NS_STATIC_CAST(PRUint8, aActionIndex);
  if (NS_FAILED(acc->GetActionName(index, name)))
    return E_FAIL;

  if (!name.IsVoid())
    return ::SysReAllocStringLen(aName, name.get(), name.Length());

  return S_OK;
}

STDMETHODIMP
CAccessibleAction::get_localizedName(long aActionIndex, BSTR *aLocalizedName)
{
  return E_NOTIMPL;
}

