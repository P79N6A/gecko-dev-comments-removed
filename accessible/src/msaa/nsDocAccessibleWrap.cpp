





































#include "nsDocAccessibleWrap.h"
#include "ISimpleDOMDocument_i.c"
#include "nsIAccessibilityService.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeNode.h"
#include "nsIFrame.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPresShell.h"
#include "nsISelectionController.h"
#include "nsIServiceManager.h"
#include "nsIURI.h"
#include "nsIViewManager.h"
#include "nsIWebNavigation.h"







nsDocAccessibleWrap::nsDocAccessibleWrap(nsIDOMNode *aDOMNode, nsIWeakReference *aShell): 
  nsDocAccessible(aDOMNode, aShell)
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

void
nsDocAccessibleWrap::GetXPAccessibleFor(const VARIANT& aVarChild,
                                        nsIAccessible **aXPAccessible)
{
  *aXPAccessible = nsnull;

  if (IsDefunct())
    return;

  
  
  

  if (aVarChild.lVal < 0)
    GetXPAccessibleForChildID(aVarChild, aXPAccessible);
  else
    nsDocAccessible::GetXPAccessibleFor(aVarChild, aXPAccessible);
}

STDMETHODIMP
nsDocAccessibleWrap::get_accChild(VARIANT varChild,
                                  IDispatch __RPC_FAR *__RPC_FAR *ppdispChild)
{
__try {
  *ppdispChild = NULL;

  if (varChild.vt == VT_I4 && varChild.lVal < 0) {
    
    
    

    nsCOMPtr<nsIAccessible> xpAccessible;
    GetXPAccessibleForChildID(varChild, getter_AddRefs(xpAccessible));
    if (!xpAccessible)
      return E_FAIL;

    IAccessible *msaaAccessible = NULL;
    xpAccessible->GetNativeInterface((void**)&msaaAccessible);
    *ppdispChild = static_cast<IDispatch*>(msaaAccessible);

    return S_OK;
  }

  
  return nsAccessibleWrap::get_accChild(varChild, ppdispChild);
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
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
  
  PRUint32 role = nsAccUtils::Role(this);
  if (role != nsIAccessibleRole::ROLE_DOCUMENT &&
      role != nsIAccessibleRole::ROLE_APPLICATION &&
      role != nsIAccessibleRole::ROLE_DIALOG &&
      role != nsIAccessibleRole::ROLE_ALERT)
    return hr;

  return get_URL(pszValue);
}

struct nsSearchAccessibleInCacheArg
{
  nsCOMPtr<nsIAccessNode> mAccessNode;
  void *mUniqueID;
};

static PLDHashOperator
SearchAccessibleInCache(const void* aKey, nsIAccessNode* aAccessNode,
                        void* aUserArg)
{
  nsCOMPtr<nsIAccessibleDocument> docAccessible(do_QueryInterface(aAccessNode));
  NS_ASSERTION(docAccessible,
               "No doc accessible for the object in doc accessible cache!");

  if (docAccessible) {
    nsSearchAccessibleInCacheArg* arg =
      static_cast<nsSearchAccessibleInCacheArg*>(aUserArg);
    nsCOMPtr<nsIAccessNode> accessNode;
    docAccessible->GetCachedAccessNode(arg->mUniqueID,
                                       getter_AddRefs(accessNode));
    if (accessNode) {
      arg->mAccessNode = accessNode;
      return PL_DHASH_STOP;
    }
  }

  return PL_DHASH_NEXT;
}

void
nsDocAccessibleWrap::GetXPAccessibleForChildID(const VARIANT& aVarChild,
                                               nsIAccessible  **aAccessible)
{
  *aAccessible = nsnull;

  NS_PRECONDITION(aVarChild.vt == VT_I4 && aVarChild.lVal < 0,
                  "Variant doesn't point to child ID!");

  
  void *uniqueID = reinterpret_cast<void*>(-aVarChild.lVal);

  nsSearchAccessibleInCacheArg arg;
  arg.mUniqueID = uniqueID;

  gGlobalDocAccessibleCache.EnumerateRead(SearchAccessibleInCache,
                                          static_cast<void*>(&arg));
  if (arg.mAccessNode)
    CallQueryInterface(arg.mAccessNode, aAccessible);
}
