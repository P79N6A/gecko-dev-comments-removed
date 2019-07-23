







































#include "CAccessibleValue.h"

#include "AccessibleValue_i.c"

#include "nsIAccessibleValue.h"

#include "nsCOMPtr.h"



STDMETHODIMP
CAccessibleValue::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_IAccessibleValue == iid) {
    nsCOMPtr<nsIAccessibleValue> valueAcc(do_QueryInterface(this));
    if (!valueAcc)
      return E_NOINTERFACE;

    *ppv = static_cast<IAccessibleValue*>(this);
    (reinterpret_cast<IUnknown*>(*ppv))->AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}



STDMETHODIMP
CAccessibleValue::get_currentValue(VARIANT *aCurrentValue)
{
  VariantInit(aCurrentValue);

  nsCOMPtr<nsIAccessibleValue> valueAcc(do_QueryInterface(this));
  if (!valueAcc)
    return E_FAIL;

  double currentValue = 0;
  nsresult rv = valueAcc->GetCurrentValue(&currentValue);
  if (NS_FAILED(rv))
    return E_FAIL;

  aCurrentValue->vt = VT_R8;
  aCurrentValue->dblVal = currentValue;

  return NS_OK;
}

STDMETHODIMP
CAccessibleValue::setCurrentValue(VARIANT aValue)
{
  nsCOMPtr<nsIAccessibleValue> valueAcc(do_QueryInterface(this));
  if (!valueAcc)
    return E_FAIL;

  if (aValue.vt != VT_R8)
    return E_INVALIDARG;

  nsresult rv = valueAcc->SetCurrentValue(aValue.dblVal);
  return NS_FAILED(rv) ? E_FAIL : S_OK;
}

STDMETHODIMP
CAccessibleValue::get_maximumValue(VARIANT *aMaximumValue)
{
  VariantInit(aMaximumValue);

  nsCOMPtr<nsIAccessibleValue> valueAcc(do_QueryInterface(this));
  if (!valueAcc)
    return E_FAIL;

  double maximumValue = 0;
  nsresult rv = valueAcc->GetMaximumValue(&maximumValue);
  if (NS_FAILED(rv))
    return E_FAIL;

  aMaximumValue->vt = VT_R8;
  aMaximumValue->dblVal = maximumValue;

  return NS_OK;
}

STDMETHODIMP
CAccessibleValue::get_minimumValue(VARIANT *aMinimumValue)
{
  VariantInit(aMinimumValue);

  nsCOMPtr<nsIAccessibleValue> valueAcc(do_QueryInterface(this));
  if (!valueAcc)
    return E_FAIL;

  double minimumValue = 0;
  nsresult rv = valueAcc->GetMinimumValue(&minimumValue);
  if (NS_FAILED(rv))
    return E_FAIL;

  aMinimumValue->vt = VT_R8;
  aMinimumValue->dblVal = minimumValue;

  return NS_OK;
}

