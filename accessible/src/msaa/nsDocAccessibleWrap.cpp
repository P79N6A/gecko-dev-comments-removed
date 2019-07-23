





































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
#include "nsIWidget.h"







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

void nsDocAccessibleWrap::GetXPAccessibleFor(const VARIANT& aVarChild, nsIAccessible **aXPAccessible)
{
  *aXPAccessible = nsnull;
  if (!mWeakShell)
    return; 

  if (aVarChild.lVal < 0) {
    
    void *uniqueID = (void*)(-aVarChild.lVal);  
    nsCOMPtr<nsIAccessNode> accessNode;
    GetCachedAccessNode(uniqueID, getter_AddRefs(accessNode));
    nsCOMPtr<nsIAccessible> accessible(do_QueryInterface(accessNode));
    NS_IF_ADDREF(*aXPAccessible = accessible);
    return;
  }

  nsDocAccessible::GetXPAccessibleFor(aVarChild, aXPAccessible);
}

STDMETHODIMP nsDocAccessibleWrap::get_accChild( 
       VARIANT varChild,
       IDispatch __RPC_FAR *__RPC_FAR *ppdispChild)
{
__try {
  *ppdispChild = NULL;

  if (varChild.vt == VT_I4 && varChild.lVal < 0) {
    
    
    nsCOMPtr<nsIAccessible> xpAccessible;
    GetXPAccessibleFor(varChild, getter_AddRefs(xpAccessible));
    if (xpAccessible) {
      IAccessible *msaaAccessible;
      xpAccessible->GetNativeInterface((void**)&msaaAccessible);
      *ppdispChild = static_cast<IDispatch*>(msaaAccessible);
      return S_OK;
    }
    else if (mDocument) {
      
      
      
      nsIDocument* parentDoc = mDocument->GetParentDocument();
      if (parentDoc) {
        nsIPresShell *parentShell = parentDoc->GetPrimaryShell();
        nsCOMPtr<nsIWeakReference> weakParentShell(do_GetWeakReference(parentShell));
        if (weakParentShell) {
          nsCOMPtr<nsIAccessibleDocument> parentDocAccessible = 
            nsAccessNode::GetDocAccessibleFor(weakParentShell);
          nsCOMPtr<nsIAccessible> accessible(do_QueryInterface(parentDocAccessible));
          IAccessible *msaaParentDoc;
          if (accessible) {
            accessible->GetNativeInterface((void**)&msaaParentDoc);
            HRESULT rv = msaaParentDoc->get_accChild(varChild, ppdispChild);
            msaaParentDoc->Release();
            return rv;
          }
        }
      }
    }
    return E_FAIL;
  }

  
  return nsAccessibleWrap::get_accChild(varChild, ppdispChild);
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

NS_IMETHODIMP nsDocAccessibleWrap::FireAnchorJumpEvent()
{
  
  
  
  
  
  
  nsDocAccessible::FireAnchorJumpEvent();
  if (!mIsAnchorJumped)
    return NS_OK;

  nsCOMPtr<nsIDOMNode> focusNode;
  if (mIsAnchor) {
    nsCOMPtr<nsISelectionController> selCon(do_QueryReferent(mWeakShell));
    if (!selCon) {
      return NS_OK;
    }
    nsCOMPtr<nsISelection> domSel;
    selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel));
    if (!domSel) {
      return NS_OK;
    }
    domSel->GetFocusNode(getter_AddRefs(focusNode));
  }
  else {
    focusNode = mDOMNode; 
  }

  nsCOMPtr<nsIAccessible> accessible = GetFirstAvailableAccessible(focusNode, PR_TRUE);
  nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_SCROLLING_START,
                           accessible);

  return NS_OK;
}

STDMETHODIMP nsDocAccessibleWrap::get_URL( BSTR __RPC_FAR *aURL)
{
__try {
  *aURL = NULL;
  nsAutoString URL;
  if (NS_SUCCEEDED(GetURL(URL))) {
    *aURL= ::SysAllocString(URL.get());
    return S_OK;
  }
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP nsDocAccessibleWrap::get_title(  BSTR __RPC_FAR *aTitle)
{
__try {
  *aTitle = NULL;
  nsAutoString title;
  if (NS_SUCCEEDED(GetTitle(title))) { 
    *aTitle= ::SysAllocString(title.get());
    return S_OK;
  }
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP nsDocAccessibleWrap::get_mimeType( BSTR __RPC_FAR *aMimeType)
{
__try {
  *aMimeType = NULL;
  nsAutoString mimeType;
  if (NS_SUCCEEDED(GetMimeType(mimeType))) {
    *aMimeType= ::SysAllocString(mimeType.get());
    return S_OK;
  }
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP nsDocAccessibleWrap::get_docType( BSTR __RPC_FAR *aDocType)
{
__try {
  *aDocType = NULL;
  nsAutoString docType;
  if (NS_SUCCEEDED(GetDocType(docType))) {
    *aDocType= ::SysAllocString(docType.get());
    return S_OK;
  }
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP nsDocAccessibleWrap::get_nameSpaceURIForID(  short aNameSpaceID,
   BSTR __RPC_FAR *aNameSpaceURI)
{
__try {
  if (aNameSpaceID < 0) {
    return E_FAIL;  
  }
  *aNameSpaceURI = NULL;
  nsAutoString nameSpaceURI;
  if (NS_SUCCEEDED(GetNameSpaceURIForID(aNameSpaceID, nameSpaceURI))) {
    *aNameSpaceURI = ::SysAllocString(nameSpaceURI.get());
    return S_OK;
  }
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP nsDocAccessibleWrap::put_alternateViewMediaTypes(  BSTR __RPC_FAR *commaSeparatedMediaTypes)
{
  return E_NOTIMPL;
}

