





#include "uiaRawElmProvider.h"

#include "AccessibleWrap.h"
#include "nsIPersistentProperties2.h"
#include "nsARIAMap.h"

using namespace mozilla;
using namespace mozilla::a11y;





IMPL_IUNKNOWN2(uiaRawElmProvider,
               IAccessibleEx,
               IRawElementProviderSimple)




STDMETHODIMP
uiaRawElmProvider::GetObjectForChild(long aIdChild,
                                     __RPC__deref_out_opt IAccessibleEx** aAccEx)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aAccEx)
    return E_INVALIDARG;

  *aAccEx = nullptr;

  return mAcc->IsDefunct() ? CO_E_OBJNOTCONNECTED : S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
uiaRawElmProvider::GetIAccessiblePair(__RPC__deref_out_opt IAccessible** aAcc,
                                      __RPC__out long* aIdChild)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aAcc || !aIdChild)
    return E_INVALIDARG;

  *aAcc = nullptr;
  *aIdChild = 0;

  if (mAcc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  *aIdChild = CHILDID_SELF;
  *aAcc = mAcc;
  mAcc->AddRef();

  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
uiaRawElmProvider::GetRuntimeId(__RPC__deref_out_opt SAFEARRAY** aRuntimeIds)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aRuntimeIds)
    return E_INVALIDARG;

  int ids[] = { UiaAppendRuntimeId, static_cast<int>(reinterpret_cast<intptr_t>(mAcc->UniqueID())) };
  *aRuntimeIds = SafeArrayCreateVector(VT_I4, 0, 2);
  if (!*aRuntimeIds)
    return E_OUTOFMEMORY;

  for (LONG i = 0; i < (LONG)ArrayLength(ids); i++)
    SafeArrayPutElement(*aRuntimeIds, &i, (void*)&(ids[i]));

  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
uiaRawElmProvider::ConvertReturnedElement(__RPC__in_opt IRawElementProviderSimple* aRawElmProvider,
                                          __RPC__deref_out_opt IAccessibleEx** aAccEx)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aRawElmProvider || !aAccEx)
    return E_INVALIDARG;

  *aAccEx = nullptr;

  void* instancePtr = nullptr;
  HRESULT hr = aRawElmProvider->QueryInterface(IID_IAccessibleEx, &instancePtr);
  if (SUCCEEDED(hr))
    *aAccEx = static_cast<IAccessibleEx*>(instancePtr);

  return hr;

  A11Y_TRYBLOCK_END
}




STDMETHODIMP
uiaRawElmProvider::get_ProviderOptions(__RPC__out enum ProviderOptions* aOptions)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aOptions)
    return E_INVALIDARG;

  
  *aOptions = ProviderOptions_ServerSideProvider;
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
uiaRawElmProvider::GetPatternProvider(PATTERNID aPatternId,
                                      __RPC__deref_out_opt IUnknown** aPatternProvider)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aPatternProvider)
    return E_INVALIDARG;

  *aPatternProvider = nullptr;
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
uiaRawElmProvider::GetPropertyValue(PROPERTYID aPropertyId,
                                    __RPC__out VARIANT* aPropertyValue)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aPropertyValue)
    return E_INVALIDARG;

  if (mAcc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  aPropertyValue->vt = VT_EMPTY;

  switch (aPropertyId) {
    
    case UIA_AcceleratorKeyPropertyId: {
      nsAutoString keyString;

      mAcc->KeyboardShortcut().ToString(keyString);

      if (!keyString.IsEmpty()) {
        aPropertyValue->vt = VT_BSTR;
        aPropertyValue->bstrVal = ::SysAllocString(keyString.get());
        return S_OK;
      }

      break;
    }

    
    case UIA_AccessKeyPropertyId: {
      nsAutoString keyString;

      mAcc->AccessKey().ToString(keyString);

      if (!keyString.IsEmpty()) {
        aPropertyValue->vt = VT_BSTR;
        aPropertyValue->bstrVal = ::SysAllocString(keyString.get());
        return S_OK;
      }

      break;
    }
    
    
    case UIA_AriaRolePropertyId: {
      nsAutoString xmlRoles;

      nsCOMPtr<nsIPersistentProperties> attributes = mAcc->Attributes();
      attributes->GetStringProperty(NS_LITERAL_CSTRING("xml-roles"), xmlRoles);

      if(!xmlRoles.IsEmpty()) {
        aPropertyValue->vt = VT_BSTR;
        aPropertyValue->bstrVal = ::SysAllocString(xmlRoles.get());
        return S_OK;
      }

      break;
    }

    
    case UIA_AriaPropertiesPropertyId: {
      nsAutoString ariaProperties;

      aria::AttrIterator attribIter(mAcc->GetContent());
      nsAutoString attribName, attribValue;
      while (attribIter.Next(attribName, attribValue)) {
        ariaProperties.Append(attribName);
        ariaProperties.Append('=');
        ariaProperties.Append(attribValue);
        ariaProperties.Append(';');
      }

      if (!ariaProperties.IsEmpty()) {
        
        ariaProperties.Truncate(ariaProperties.Length()-1);
        aPropertyValue->vt = VT_BSTR;
        aPropertyValue->bstrVal = ::SysAllocString(ariaProperties.get());
        return S_OK;
      }

      break;
    }
  }

  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
uiaRawElmProvider::get_HostRawElementProvider(__RPC__deref_out_opt IRawElementProviderSimple** aRawElmProvider)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aRawElmProvider)
    return E_INVALIDARG;

  
  *aRawElmProvider = nullptr;
  return S_OK;

  A11Y_TRYBLOCK_END
}
