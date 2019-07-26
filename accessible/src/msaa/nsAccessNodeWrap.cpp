




#include "nsAccessNodeWrap.h"

#include "AccessibleApplication.h"
#include "ApplicationAccessibleWrap.h"
#include "sdnAccessible.h"

#include "Compatibility.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsWinUtils.h"
#include "RootAccessible.h"
#include "Statistics.h"

#include "nsAttrName.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMHTMLElement.h"
#include "nsINameSpaceManager.h"
#include "nsPIDOMWindow.h"
#include "nsIServiceManager.h"

using namespace mozilla;
using namespace mozilla::a11y;





nsAccessNodeWrap::
  nsAccessNodeWrap(nsIContent* aContent, DocAccessible* aDoc) :
  nsAccessNode(aContent, aDoc)
{
}

nsAccessNodeWrap::~nsAccessNodeWrap()
{
}





NS_IMPL_ISUPPORTS_INHERITED0(nsAccessNodeWrap, nsAccessNode)

STDMETHODIMP nsAccessNodeWrap::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = nullptr;

  if (IID_IUnknown == iid) {
    *ppv = static_cast<IUnknown*>(this);
  } else {
    return E_NOINTERFACE; 
  }

  (reinterpret_cast<IUnknown*>(*ppv))->AddRef();
  return S_OK;
}

STDMETHODIMP
nsAccessNodeWrap::QueryService(REFGUID guidService, REFIID iid, void** ppv)
{
  *ppv = nullptr;

  
  
  
  
  
  static const GUID SID_IAccessibleContentDocument =
    { 0xa5d8e1f3,0x3571,0x4d8f,{0x95,0x21,0x07,0xed,0x28,0xfb,0x07,0x2e} };
  if (guidService == SID_IAccessibleContentDocument) {
    if (iid != IID_IAccessible)
      return E_NOINTERFACE;

    nsCOMPtr<nsIDocShell> docShell = nsCoreUtils::GetDocShellFor(mContent);
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

    *ppv = static_cast<IAccessible*>(docAcc);

    (reinterpret_cast<IUnknown*>(*ppv))->AddRef();
    return S_OK;
  }

  
  if (guidService == IID_IAccessibleApplication ||
      (Compatibility::IsJAWS() && iid == IID_IAccessibleApplication)) {
    ApplicationAccessibleWrap* applicationAcc =
      static_cast<ApplicationAccessibleWrap*>(ApplicationAcc());
    if (!applicationAcc)
      return E_NOINTERFACE;

    return applicationAcc->QueryInterface(iid, ppv);
  }

  













  static const GUID IID_SimpleDOMDeprecated =
    { 0x0c539790,0x12e4,0x11cf,{0xb6,0x61,0x00,0xaa,0x00,0x4c,0xd6,0xd8} };
  if (guidService == IID_ISimpleDOMNode ||
      guidService == IID_SimpleDOMDeprecated ||
      guidService == IID_IAccessible ||  guidService == IID_IAccessible2)
    return QueryInterface(iid, ppv);

  return E_INVALIDARG;
}
 
int nsAccessNodeWrap::FilterA11yExceptions(unsigned int aCode, EXCEPTION_POINTERS *aExceptionInfo)
{
  if (aCode == EXCEPTION_ACCESS_VIOLATION) {
#ifdef MOZ_CRASHREPORTER
    
    
    
    nsCOMPtr<nsICrashReporter> crashReporter =
      do_GetService("@mozilla.org/toolkit/crash-reporter;1");
    if (crashReporter) {
      crashReporter->WriteMinidumpForException(aExceptionInfo);
    }
#endif
  }
  else {
    NS_NOTREACHED("We should only be catching crash exceptions");
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

HRESULT
GetHRESULT(nsresult aResult)
{
  switch (aResult) {
    case NS_OK:
      return S_OK;

    case NS_ERROR_INVALID_ARG: case NS_ERROR_INVALID_POINTER:
      return E_INVALIDARG;

    case NS_ERROR_OUT_OF_MEMORY:
      return E_OUTOFMEMORY;

    case NS_ERROR_NOT_IMPLEMENTED:
      return E_NOTIMPL;

    default:
      return E_FAIL;
  }
}

nsRefPtrHashtable<nsPtrHashKey<void>, DocAccessible> nsAccessNodeWrap::sHWNDCache;

LRESULT CALLBACK
nsAccessNodeWrap::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  
  
  

  switch (msg) {
    case WM_GETOBJECT:
    {
      if (lParam == OBJID_CLIENT) {
        DocAccessible* document = sHWNDCache.GetWeak(static_cast<void*>(hWnd));
        if (document) {
          IAccessible* msaaAccessible = NULL;
          document->GetNativeInterface((void**)&msaaAccessible); 
          if (msaaAccessible) {
            LRESULT result = ::LresultFromObject(IID_IAccessible, wParam,
                                                 msaaAccessible); 
            msaaAccessible->Release(); 
            return result;
          }
        }
      }
      return 0;
    }
    case WM_NCHITTEST:
    {
      LRESULT lRet = ::DefWindowProc(hWnd, msg, wParam, lParam);
      if (HTCLIENT == lRet)
        lRet = HTTRANSPARENT;
      return lRet;
    }
  }

  return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
