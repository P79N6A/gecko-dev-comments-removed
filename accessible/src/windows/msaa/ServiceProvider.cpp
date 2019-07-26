





#include "ServiceProvider.h"

#include "ApplicationAccessibleWrap.h"
#include "Compatibility.h"
#include "DocAccessible.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "Relation.h"
#include "uiaRawElmProvider.h"

#include "mozilla/Preferences.h"
#include "nsIDocShell.h"

#include "ISimpleDOMNode_i.c"

namespace mozilla {
namespace a11y {

IMPL_IUNKNOWN_QUERY_HEAD(ServiceProvider)
  IMPL_IUNKNOWN_QUERY_IFACE(IServiceProvider)
IMPL_IUNKNOWN_QUERY_TAIL_AGGREGATED(mAccessible)





STDMETHODIMP
ServiceProvider::QueryService(REFGUID aGuidService, REFIID aIID,
                              void** aInstancePtr)
{
  if (!aInstancePtr)
    return E_INVALIDARG;

  *aInstancePtr = nullptr;

  
  if (aGuidService == IID_IAccessibleEx &&
      Preferences::GetBool("accessibility.uia.enable")) {
    uiaRawElmProvider* accEx = new uiaRawElmProvider(mAccessible);
    HRESULT hr = accEx->QueryInterface(aIID, aInstancePtr);
    if (FAILED(hr))
      delete accEx;

    return hr;
  }

  
  
  
  
  
  static const GUID SID_IAccessibleContentDocument =
    { 0xa5d8e1f3,0x3571,0x4d8f,{0x95,0x21,0x07,0xed,0x28,0xfb,0x07,0x2e} };
  if (aGuidService == SID_IAccessibleContentDocument) {
    if (aIID != IID_IAccessible)
      return E_NOINTERFACE;

    Relation rel = mAccessible->RelationByType(RelationType::CONTAINING_TAB_PANE);
    AccessibleWrap* tabDoc = static_cast<AccessibleWrap*>(rel.Next());
    if (!tabDoc)
      return E_NOINTERFACE;

    *aInstancePtr = static_cast<IAccessible*>(tabDoc);
    (reinterpret_cast<IUnknown*>(*aInstancePtr))->AddRef();
    return S_OK;
  }

  
  if (aGuidService == IID_IAccessibleApplication ||
      (Compatibility::IsJAWS() && aIID == IID_IAccessibleApplication)) {
    ApplicationAccessibleWrap* applicationAcc =
      static_cast<ApplicationAccessibleWrap*>(ApplicationAcc());
    if (!applicationAcc)
      return E_NOINTERFACE;

    return applicationAcc->QueryInterface(aIID, aInstancePtr);
  }

  static const GUID IID_SimpleDOMDeprecated =
    { 0x0c539790,0x12e4,0x11cf,{0xb6,0x61,0x00,0xaa,0x00,0x4c,0xd6,0xd8} };
  if (aGuidService == IID_ISimpleDOMNode ||
      aGuidService == IID_SimpleDOMDeprecated ||
      aGuidService == IID_IAccessible ||  aGuidService == IID_IAccessible2)
    return mAccessible->QueryInterface(aIID, aInstancePtr);

  return E_INVALIDARG;
}

} 
} 
