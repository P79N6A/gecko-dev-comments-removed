





































#include "nsDocAccessibleWrap.h"
#include "ISimpleDOMDocument_i.c"
#include "nsIAccessibilityService.h"
#include "nsRootAccessible.h"
#include "nsWinUtils.h"

#include "nsIDocShell.h"
#include "nsIDocShellTreeNode.h"
#include "nsIFrame.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsISelectionController.h"
#include "nsIServiceManager.h"
#include "nsIURI.h"
#include "nsIViewManager.h"
#include "nsIWebNavigation.h"









nsDocAccessibleWrap::
  nsDocAccessibleWrap(nsIDocument *aDocument, nsIContent *aRootContent,
                      nsIWeakReference *aShell) :
  nsDocAccessible(aDocument, aRootContent, aShell), mHWND(NULL)
{
}

nsDocAccessibleWrap::~nsDocAccessibleWrap()
{
}




STDMETHODIMP_(ULONG) nsDocAccessibleWrap::AddRef()
{
  return nsAccessNode::AddRef();
}

STDMETHODIMP_(ULONG) nsDocAccessibleWrap::Release()
{
  return nsAccessNode::Release();
}


STDMETHODIMP nsDocAccessibleWrap::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_ISimpleDOMDocument == iid)
    *ppv = static_cast<ISimpleDOMDocument*>(this);

  if (NULL == *ppv)
    return nsHyperTextAccessibleWrap::QueryInterface(iid, ppv);
    
  (reinterpret_cast<IUnknown*>(*ppv))->AddRef();
  return S_OK;
}

nsAccessible*
nsDocAccessibleWrap::GetXPAccessibleFor(const VARIANT& aVarChild)
{
  
  
  

  if (aVarChild.vt == VT_I4 && aVarChild.lVal < 0) {
    
    void* uniqueID = reinterpret_cast<void*>(-aVarChild.lVal);
    return GetAccessibleByUniqueIDInSubtree(uniqueID);
  }

  return nsAccessibleWrap::GetXPAccessibleFor(aVarChild);
}

STDMETHODIMP nsDocAccessibleWrap::get_URL( BSTR __RPC_FAR *aURL)
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

STDMETHODIMP nsDocAccessibleWrap::get_title(  BSTR __RPC_FAR *aTitle)
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

STDMETHODIMP nsDocAccessibleWrap::get_mimeType( BSTR __RPC_FAR *aMimeType)
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

STDMETHODIMP nsDocAccessibleWrap::get_docType( BSTR __RPC_FAR *aDocType)
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

STDMETHODIMP nsDocAccessibleWrap::get_nameSpaceURIForID(  short aNameSpaceID,
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
nsDocAccessibleWrap::put_alternateViewMediaTypes(  BSTR __RPC_FAR *aCommaSeparatedMediaTypes)
{
__try {
  *aCommaSeparatedMediaTypes = NULL;
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_NOTIMPL;
}

STDMETHODIMP nsDocAccessibleWrap::get_accValue(
       VARIANT varChild,
       BSTR __RPC_FAR *pszValue)
{
  
  *pszValue = NULL;
  
  HRESULT hr = nsAccessibleWrap::get_accValue(varChild, pszValue);
  if (FAILED(hr) || *pszValue || varChild.lVal != CHILDID_SELF)
    return hr;
  
  PRUint32 role = Role();
  if (role != nsIAccessibleRole::ROLE_DOCUMENT &&
      role != nsIAccessibleRole::ROLE_APPLICATION &&
      role != nsIAccessibleRole::ROLE_DIALOG &&
      role != nsIAccessibleRole::ROLE_ALERT)
    return hr;

  return get_URL(pszValue);
}




PRBool
nsDocAccessibleWrap::Init()
{
  if (nsWinUtils::IsWindowEmulationEnabled()) {
    
    if (nsWinUtils::IsTabDocument(mDocument)) {
      nsRefPtr<nsRootAccessible> root = GetRootAccessible();
      mHWND = nsWinUtils::CreateNativeWindow(kClassNameTabContent,
                                             static_cast<HWND>(root->GetNativeWindow()));

      nsAccessibleWrap::sHWNDCache.Put(mHWND, this);

    } else {
      nsDocAccessible* parentDocument = ParentDocument();
      if (parentDocument)
        mHWND = parentDocument->GetNativeWindow();
    }
  }

  return nsDocAccessible::Init();
}

void
nsDocAccessibleWrap::Shutdown()
{
  if (nsWinUtils::IsWindowEmulationEnabled()) {
    
    if (nsWinUtils::IsTabDocument(mDocument)) {
      nsAccessibleWrap::sHWNDCache.Remove(mHWND);
      ::DestroyWindow(static_cast<HWND>(mHWND));
    }

    mHWND = nsnull;
  }

  nsDocAccessible::Shutdown();
}




void*
nsDocAccessibleWrap::GetNativeWindow() const
{
  return mHWND ? mHWND : nsDocAccessible::GetNativeWindow();
}
