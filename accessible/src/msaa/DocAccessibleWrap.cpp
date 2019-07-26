




#include "mozilla/dom/TabChild.h"

#include "Compatibility.h"
#include "DocAccessibleWrap.h"
#include "ISimpleDOMDocument_i.c"
#include "nsCoreUtils.h"
#include "nsIAccessibilityService.h"
#include "nsWinUtils.h"
#include "Role.h"
#include "RootAccessible.h"
#include "Statistics.h"

#include "nsIDocShell.h"
#include "nsIDocShellTreeNode.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsISelectionController.h"
#include "nsIServiceManager.h"
#include "nsIURI.h"
#include "nsIViewManager.h"
#include "nsIWebNavigation.h"

using namespace mozilla;
using namespace mozilla::a11y;









DocAccessibleWrap::
  DocAccessibleWrap(nsIDocument* aDocument, nsIContent* aRootContent,
                    nsIPresShell* aPresShell) :
  DocAccessible(aDocument, aRootContent, aPresShell), mHWND(NULL)
{
}

DocAccessibleWrap::~DocAccessibleWrap()
{
}




STDMETHODIMP_(ULONG)
DocAccessibleWrap::AddRef()
{
  return nsAccessNode::AddRef();
}

STDMETHODIMP_(ULONG) DocAccessibleWrap::Release()
{
  return nsAccessNode::Release();
}


STDMETHODIMP
DocAccessibleWrap::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_ISimpleDOMDocument != iid)
    return HyperTextAccessibleWrap::QueryInterface(iid, ppv);

  statistics::ISimpleDOMUsed();
  *ppv = static_cast<ISimpleDOMDocument*>(this);
  (reinterpret_cast<IUnknown*>(*ppv))->AddRef();
  return S_OK;
}

STDMETHODIMP
DocAccessibleWrap::get_URL( BSTR __RPC_FAR *aURL)
{
__try {
  *aURL = NULL;

  nsAutoString URL;
  nsresult rv = GetURL(URL);
  if (NS_FAILED(rv))
    return E_FAIL;

  if (URL.IsEmpty())
    return S_FALSE;

  *aURL = ::SysAllocStringLen(URL.get(), URL.Length());
  return *aURL ? S_OK : E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
DocAccessibleWrap::get_title(  BSTR __RPC_FAR *aTitle)
{
__try {
  *aTitle = NULL;

  nsAutoString title;
  nsresult rv = GetTitle(title);
  if (NS_FAILED(rv))
    return E_FAIL;

  *aTitle = ::SysAllocStringLen(title.get(), title.Length());
  return *aTitle ? S_OK : E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
DocAccessibleWrap::get_mimeType( BSTR __RPC_FAR *aMimeType)
{
__try {
  *aMimeType = NULL;

  nsAutoString mimeType;
  nsresult rv = GetMimeType(mimeType);
  if (NS_FAILED(rv))
    return E_FAIL;

  if (mimeType.IsEmpty())
    return S_FALSE;

  *aMimeType = ::SysAllocStringLen(mimeType.get(), mimeType.Length());
  return *aMimeType ? S_OK : E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
DocAccessibleWrap::get_docType( BSTR __RPC_FAR *aDocType)
{
__try {
  *aDocType = NULL;

  nsAutoString docType;
  nsresult rv = GetDocType(docType);
  if (NS_FAILED(rv))
    return E_FAIL;

  if (docType.IsEmpty())
    return S_FALSE;

  *aDocType = ::SysAllocStringLen(docType.get(), docType.Length());
  return *aDocType ? S_OK : E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
DocAccessibleWrap::get_nameSpaceURIForID(  short aNameSpaceID,
   BSTR __RPC_FAR *aNameSpaceURI)
{
__try {
  *aNameSpaceURI = NULL;

  if (aNameSpaceID < 0)
    return E_INVALIDARG;  

  nsAutoString nameSpaceURI;
  nsresult rv = GetNameSpaceURIForID(aNameSpaceID, nameSpaceURI);
  if (NS_FAILED(rv))
    return E_FAIL;

  if (nameSpaceURI.IsEmpty())
    return S_FALSE;

  *aNameSpaceURI = ::SysAllocStringLen(nameSpaceURI.get(),
                                       nameSpaceURI.Length());

  return *aNameSpaceURI ? S_OK : E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
DocAccessibleWrap::put_alternateViewMediaTypes(  BSTR __RPC_FAR *aCommaSeparatedMediaTypes)
{
__try {
  *aCommaSeparatedMediaTypes = NULL;
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_NOTIMPL;
}

STDMETHODIMP
DocAccessibleWrap::get_accValue(
       VARIANT varChild,
       BSTR __RPC_FAR *pszValue)
{
  
  *pszValue = NULL;
  
  HRESULT hr = AccessibleWrap::get_accValue(varChild, pszValue);
  if (FAILED(hr) || *pszValue || varChild.lVal != CHILDID_SELF)
    return hr;
  
  roles::Role role = Role();
  if (role != roles::DOCUMENT && role != roles::APPLICATION && 
      role != roles::DIALOG && role != roles::ALERT) 
    return hr;

  return get_URL(pszValue);
}




void
DocAccessibleWrap::Shutdown()
{
  
  if (nsWinUtils::IsWindowEmulationStarted()) {
    
    if (nsCoreUtils::IsTabDocument(mDocument)) {
      sHWNDCache.Remove(mHWND);
      ::DestroyWindow(static_cast<HWND>(mHWND));
    }

    mHWND = nullptr;
  }

  DocAccessible::Shutdown();
}




void*
DocAccessibleWrap::GetNativeWindow() const
{
  return mHWND ? mHWND : DocAccessible::GetNativeWindow();
}




void
DocAccessibleWrap::DoInitialUpdate()
{
  DocAccessible::DoInitialUpdate();

  if (nsWinUtils::IsWindowEmulationStarted()) {
    
    if (nsCoreUtils::IsTabDocument(mDocument)) {
      mozilla::dom::TabChild* tabChild =
        mozilla::dom::GetTabChildFrom(mDocument->GetShell());

      a11y::RootAccessible* rootDocument = RootAccessible();

      mozilla::WindowsHandle nativeData = NULL;
      if (tabChild)
        tabChild->SendGetWidgetNativeData(&nativeData);
      else
        nativeData = reinterpret_cast<mozilla::WindowsHandle>(
          rootDocument->GetNativeWindow());

      bool isActive = true;
      int32_t x = CW_USEDEFAULT, y = CW_USEDEFAULT, width = 0, height = 0;
      if (Compatibility::IsDolphin()) {
        GetBounds(&x, &y, &width, &height);
        int32_t rootX = 0, rootY = 0, rootWidth = 0, rootHeight = 0;
        rootDocument->GetBounds(&rootX, &rootY, &rootWidth, &rootHeight);
        x = rootX - x;
        y -= rootY;

        nsCOMPtr<nsISupports> container = mDocument->GetContainer();
        nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(container);
        docShell->GetIsActive(&isActive);
      }

      HWND parentWnd = reinterpret_cast<HWND>(nativeData);
      mHWND = nsWinUtils::CreateNativeWindow(kClassNameTabContent, parentWnd,
                                             x, y, width, height, isActive);

      sHWNDCache.Put(mHWND, this);

    } else {
      DocAccessible* parentDocument = ParentDocument();
      if (parentDocument)
        mHWND = parentDocument->GetNativeWindow();
    }
  }
}
