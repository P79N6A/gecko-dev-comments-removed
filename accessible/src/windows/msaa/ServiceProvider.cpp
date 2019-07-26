





#include "ServiceProvider.h"

#include "ApplicationAccessibleWrap.h"
#include "Compatibility.h"
#include "DocAccessible.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "uiaRawElmProvider.h"

#include "mozilla/Preferences.h"
#include "nsIDocShell.h"

#include "ISimpleDOMNode_i.c"

namespace mozilla {
namespace a11y {

IMPL_IUNKNOWN_QUERY_HEAD(ServiceProvider)
  IMPL_IUNKNOWN_QUERY_IFACE(IServiceProvider)
  return mAccessible->QueryInterface(aIID, aInstancePtr);
A11Y_TRYBLOCK_END
  }




STDMETHODIMP
ServiceProvider::QueryService(REFGUID aGuidService, REFIID aIID,
                              void** aInstancePtr)
{
  if (!aInstancePtr)
    return E_INVALIDARG;

  *aInstancePtr = NULL;

  
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

    nsCOMPtr<nsIDocShell> docShell =
      nsCoreUtils::GetDocShellFor(mAccessible->GetNode());
    if (!docShell)
      return E_UNEXPECTED;

    
    
    nsCOMPtr<nsIDocShellTreeItem> root;
    docShell->GetSameTypeRootTreeItem(getter_AddRefs(root));
    if (!root)
      return E_UNEXPECTED;


    
    
    int32_t itemType;
    root->GetItemType(&itemType);
    if (itemType != nsIDocShellTreeItem::typeContent)
      return E_NOINTERFACE;

    
    DocAccessible* docAcc = nsAccUtils::GetDocAccessibleFor(root);
    if (!docAcc)
      return E_UNEXPECTED;

    *aInstancePtr = static_cast<IAccessible*>(docAcc);

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
