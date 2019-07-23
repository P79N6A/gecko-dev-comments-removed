





































#include "nsAccessNodeWrap.h"
#include "ISimpleDOMNode_i.c"
#include "nsAccessibilityAtoms.h"
#include "nsIAccessibilityService.h"
#include "nsIAccessible.h"
#include "nsAttrName.h"
#include "nsIDocument.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsIDOMViewCSS.h"
#include "nsIFrame.h"
#include "nsINameSpaceManager.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPresShell.h"
#include "nsPIDOMWindow.h"
#include "nsRootAccessible.h"
#include "nsIServiceManager.h"
#include "AccessibleApplication.h"
#include "nsApplicationAccessibleWrap.h"


HINSTANCE nsAccessNodeWrap::gmAccLib = nsnull;
HINSTANCE nsAccessNodeWrap::gmUserLib = nsnull;
LPFNACCESSIBLEOBJECTFROMWINDOW nsAccessNodeWrap::gmAccessibleObjectFromWindow = nsnull;
LPFNNOTIFYWINEVENT nsAccessNodeWrap::gmNotifyWinEvent = nsnull;
LPFNGETGUITHREADINFO nsAccessNodeWrap::gmGetGUIThreadInfo = nsnull;

PRBool nsAccessNodeWrap::gIsEnumVariantSupportDisabled = 0;

PRBool nsAccessNodeWrap::gIsIA2Disabled = PR_FALSE;

nsIAccessibleTextChangeEvent *nsAccessNodeWrap::gTextEvent = nsnull;



#define CTRLTAB_DISALLOW_FOR_SCREEN_READERS_PREF "browser.ctrlTab.disallowForScreenReaders"














nsAccessNodeWrap::nsAccessNodeWrap(nsIDOMNode *aNode, nsIWeakReference* aShell): 
  nsAccessNode(aNode, aShell)
{
}




nsAccessNodeWrap::~nsAccessNodeWrap()
{
}





NS_IMPL_ISUPPORTS_INHERITED1(nsAccessNodeWrap, nsAccessNode, nsIWinAccessNode);





NS_IMETHODIMP
nsAccessNodeWrap::QueryNativeInterface(REFIID aIID, void** aInstancePtr)
{
  return QueryInterface(aIID, aInstancePtr);
}





STDMETHODIMP nsAccessNodeWrap::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = nsnull;

  if (IID_IUnknown == iid || IID_ISimpleDOMNode == iid)
    *ppv = static_cast<ISimpleDOMNode*>(this);

  if (nsnull == *ppv)
    return E_NOINTERFACE;      
   
  (reinterpret_cast<IUnknown*>(*ppv))->AddRef(); 
  return S_OK;
}

STDMETHODIMP
nsAccessNodeWrap::QueryService(REFGUID guidService, REFIID iid, void** ppv)
{
  static const GUID IID_SimpleDOMDeprecated = {0x0c539790,0x12e4,0x11cf,0xb6,0x61,0x00,0xaa,0x00,0x4c,0xd6,0xd8};
  if (guidService != IID_ISimpleDOMNode &&
      guidService != IID_SimpleDOMDeprecated &&
      guidService != IID_IAccessible &&  guidService != IID_IAccessible2 &&
      guidService != IID_IAccessibleApplication)
    return E_INVALIDARG;

  
  if (iid == IID_IAccessibleApplication) {
    nsRefPtr<nsApplicationAccessibleWrap> app =
      GetApplicationAccessible();
    nsresult rv = app->QueryNativeInterface(iid, ppv);
    return NS_SUCCEEDED(rv) ? S_OK : E_NOINTERFACE;
  }

  













  return QueryInterface(iid, ppv);
}





STDMETHODIMP nsAccessNodeWrap::get_nodeInfo( 
     BSTR __RPC_FAR *aNodeName,
     short __RPC_FAR *aNameSpaceID,
     BSTR __RPC_FAR *aNodeValue,
     unsigned int __RPC_FAR *aNumChildren,
     unsigned int __RPC_FAR *aUniqueID,
     unsigned short __RPC_FAR *aNodeType)
{
__try{
  *aNodeName = nsnull;
  *aNodeValue = nsnull;

  if (!mDOMNode)
    return E_FAIL;
 
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));

  PRUint16 nodeType = 0;
  mDOMNode->GetNodeType(&nodeType);
  *aNodeType=static_cast<unsigned short>(nodeType);

  if (*aNodeType !=  NODETYPE_TEXT) {
    nsAutoString nodeName;
    mDOMNode->GetNodeName(nodeName);
    *aNodeName =   ::SysAllocString(nodeName.get());
  }

  nsAutoString nodeValue;

  mDOMNode->GetNodeValue(nodeValue);
  *aNodeValue = ::SysAllocString(nodeValue.get());
  *aNameSpaceID = content ? static_cast<short>(content->GetNameSpaceID()) : 0;

  
  
  
  
  void *uniqueID;
  GetUniqueID(&uniqueID);
  *aUniqueID = - NS_PTR_TO_INT32(uniqueID);

  *aNumChildren = 0;
  PRUint32 numChildren = 0;
  nsCOMPtr<nsIDOMNodeList> nodeList;
  mDOMNode->GetChildNodes(getter_AddRefs(nodeList));
  if (nodeList && NS_OK == nodeList->GetLength(&numChildren))
    *aNumChildren = static_cast<unsigned int>(numChildren);

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return S_OK;
}


       
STDMETHODIMP nsAccessNodeWrap::get_attributes( 
     unsigned short aMaxAttribs,
     BSTR __RPC_FAR *aAttribNames,
     short __RPC_FAR *aNameSpaceIDs,
     BSTR __RPC_FAR *aAttribValues,
     unsigned short __RPC_FAR *aNumAttribs)
{
__try{
  *aNumAttribs = 0;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (!content) 
    return E_FAIL;

  PRUint32 numAttribs = content->GetAttrCount();

  if (numAttribs > aMaxAttribs)
    numAttribs = aMaxAttribs;
  *aNumAttribs = static_cast<unsigned short>(numAttribs);

  for (PRUint32 index = 0; index < numAttribs; index++) {
    aNameSpaceIDs[index] = 0; aAttribValues[index] = aAttribNames[index] = nsnull;
    nsAutoString attributeValue;
    const char *pszAttributeName; 

    const nsAttrName* name = content->GetAttrNameAt(index);
    aNameSpaceIDs[index] = static_cast<short>(name->NamespaceID());
    name->LocalName()->GetUTF8String(&pszAttributeName);
    aAttribNames[index] = ::SysAllocString(NS_ConvertUTF8toUTF16(pszAttributeName).get());
    content->GetAttr(name->NamespaceID(), name->LocalName(), attributeValue);
    aAttribValues[index] = ::SysAllocString(attributeValue.get());
  }
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK; 
}
        

STDMETHODIMP nsAccessNodeWrap::get_attributesForNames( 
     unsigned short aNumAttribs,
     BSTR __RPC_FAR *aAttribNames,
     short __RPC_FAR *aNameSpaceID,
     BSTR __RPC_FAR *aAttribValues)
{
__try {
  nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(mDOMNode));
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));

  if (!domElement || !content) 
    return E_FAIL;

  if (!content->GetDocument())
    return E_FAIL;

  nsCOMPtr<nsINameSpaceManager> nameSpaceManager =
    do_GetService(NS_NAMESPACEMANAGER_CONTRACTID);

  PRInt32 index;

  for (index = 0; index < aNumAttribs; index++) {
    aAttribValues[index] = nsnull;
    if (aAttribNames[index]) {
      nsAutoString attributeValue, nameSpaceURI;
      nsAutoString attributeName(nsDependentString(static_cast<PRUnichar*>(aAttribNames[index])));
      nsresult rv;

      if (aNameSpaceID[index]>0 && 
        NS_SUCCEEDED(nameSpaceManager->GetNameSpaceURI(aNameSpaceID[index], nameSpaceURI)))
          rv = domElement->GetAttributeNS(nameSpaceURI, attributeName, attributeValue);
      else 
        rv = domElement->GetAttribute(attributeName, attributeValue);

      if (NS_SUCCEEDED(rv))
        aAttribValues[index] = ::SysAllocString(attributeValue.get());
    }
  }
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK; 
}


STDMETHODIMP nsAccessNodeWrap::get_computedStyle( 
     unsigned short aMaxStyleProperties,
     boolean aUseAlternateView,
     BSTR __RPC_FAR *aStyleProperties,
     BSTR __RPC_FAR *aStyleValues,
     unsigned short __RPC_FAR *aNumStyleProperties)
{
__try{
  *aNumStyleProperties = 0;

  if (IsDefunct())
    return E_FAIL;

  nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
  nsCoreUtils::GetComputedStyleDeclaration(EmptyString(), mDOMNode,
                                           getter_AddRefs(cssDecl));
  NS_ENSURE_TRUE(cssDecl, E_FAIL);

  PRUint32 length;
  cssDecl->GetLength(&length);

  PRUint32 index, realIndex;
  for (index = realIndex = 0; index < length && realIndex < aMaxStyleProperties; index ++) {
    nsAutoString property, value;
    if (NS_SUCCEEDED(cssDecl->Item(index, property)) && property.CharAt(0) != '-')  
      cssDecl->GetPropertyValue(property, value);  
    if (!value.IsEmpty()) {
      aStyleProperties[realIndex] =   ::SysAllocString(property.get());
      aStyleValues[realIndex]     =   ::SysAllocString(value.get());
      ++realIndex;
    }
  }
  *aNumStyleProperties = static_cast<unsigned short>(realIndex);
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}


STDMETHODIMP nsAccessNodeWrap::get_computedStyleForProperties( 
     unsigned short aNumStyleProperties,
     boolean aUseAlternateView,
     BSTR __RPC_FAR *aStyleProperties,
     BSTR __RPC_FAR *aStyleValues)
{
__try {
  if (IsDefunct())
    return E_FAIL;
 
  nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
  nsCoreUtils::GetComputedStyleDeclaration(EmptyString(), mDOMNode,
                                           getter_AddRefs(cssDecl));
  NS_ENSURE_TRUE(cssDecl, E_FAIL);

  PRUint32 index;
  for (index = 0; index < aNumStyleProperties; index ++) {
    nsAutoString value;
    if (aStyleProperties[index])
      cssDecl->GetPropertyValue(nsDependentString(static_cast<PRUnichar*>(aStyleProperties[index])), value);  
    aStyleValues[index] = ::SysAllocString(value.get());
  }
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP nsAccessNodeWrap::scrollTo( boolean aScrollTopLeft)
{
__try {
  PRUint32 scrollType =
    aScrollTopLeft ? nsIAccessibleScrollType::SCROLL_TYPE_TOP_LEFT :
                     nsIAccessibleScrollType::SCROLL_TYPE_BOTTOM_RIGHT;

  nsresult rv = ScrollTo(scrollType);
  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

ISimpleDOMNode* nsAccessNodeWrap::MakeAccessNode(nsIDOMNode *node)
{
  if (!node) 
    return NULL;

  nsAccessNodeWrap *newNode = NULL;
  
  nsCOMPtr<nsIContent> content(do_QueryInterface(node));
  nsCOMPtr<nsIDocument> doc;

  if (content) 
    doc = content->GetDocument();
  else {
    
    doc = do_QueryInterface(node);
    content = do_QueryInterface(node);
  }

  if (!doc)
    return NULL;

  nsCOMPtr<nsIAccessibilityService> accService(do_GetService("@mozilla.org/accessibilityService;1"));
  if (!accService)
    return NULL;

  ISimpleDOMNode *iNode = NULL;
  nsCOMPtr<nsIAccessible> nsAcc;
  accService->GetAccessibleInWeakShell(node, mWeakShell, getter_AddRefs(nsAcc));
  if (nsAcc) {
    nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(nsAcc));
    NS_ASSERTION(accessNode, "nsIAccessible impl does not inherit from nsIAccessNode");
    IAccessible *msaaAccessible;
    nsAcc->GetNativeInterface((void**)&msaaAccessible); 
    msaaAccessible->QueryInterface(IID_ISimpleDOMNode, (void**)&iNode); 
    msaaAccessible->Release(); 
  }
  else {
    newNode = new nsAccessNodeWrap(node, mWeakShell);
    if (!newNode)
      return NULL;

    newNode->Init();
    iNode = static_cast<ISimpleDOMNode*>(newNode);
    iNode->AddRef();
  }

  return iNode;
}


STDMETHODIMP nsAccessNodeWrap::get_parentNode(ISimpleDOMNode __RPC_FAR *__RPC_FAR *aNode)
{
__try {
  if (!mDOMNode)
    return E_FAIL;
 
  nsCOMPtr<nsIDOMNode> node;
  mDOMNode->GetParentNode(getter_AddRefs(node));
  *aNode = MakeAccessNode(node);
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP nsAccessNodeWrap::get_firstChild(ISimpleDOMNode __RPC_FAR *__RPC_FAR *aNode)
{
__try {
  if (!mDOMNode)
    return E_FAIL;
 
  nsCOMPtr<nsIDOMNode> node;
  mDOMNode->GetFirstChild(getter_AddRefs(node));
  *aNode = MakeAccessNode(node);
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP nsAccessNodeWrap::get_lastChild(ISimpleDOMNode __RPC_FAR *__RPC_FAR *aNode)
{
  __try {
  if (!mDOMNode)
    return E_FAIL;

  nsCOMPtr<nsIDOMNode> node;
  mDOMNode->GetLastChild(getter_AddRefs(node));
  *aNode = MakeAccessNode(node);
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP nsAccessNodeWrap::get_previousSibling(ISimpleDOMNode __RPC_FAR *__RPC_FAR *aNode)
{
__try {
  if (!mDOMNode)
    return E_FAIL;

  nsCOMPtr<nsIDOMNode> node;
  mDOMNode->GetPreviousSibling(getter_AddRefs(node));
  *aNode = MakeAccessNode(node);
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP nsAccessNodeWrap::get_nextSibling(ISimpleDOMNode __RPC_FAR *__RPC_FAR *aNode)
{
__try {
  if (!mDOMNode)
    return E_FAIL;

  nsCOMPtr<nsIDOMNode> node;
  mDOMNode->GetNextSibling(getter_AddRefs(node));
  *aNode = MakeAccessNode(node);
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP 
nsAccessNodeWrap::get_childAt(unsigned aChildIndex,
                              ISimpleDOMNode __RPC_FAR *__RPC_FAR *aNode)
{
__try {
  *aNode = nsnull;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (!content)
    return E_FAIL;  

  nsCOMPtr<nsIDOMNode> node =
    do_QueryInterface(content->GetChildAt(aChildIndex));

  if (!node)
    return E_FAIL; 

  *aNode = MakeAccessNode(node);
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP 
nsAccessNodeWrap::get_innerHTML(BSTR __RPC_FAR *aInnerHTML)
{
__try {
  *aInnerHTML = nsnull;

  nsCOMPtr<nsIDOMNSHTMLElement> domNSElement(do_QueryInterface(mDOMNode));
  if (!domNSElement)
    return E_FAIL; 

  nsAutoString innerHTML;
  domNSElement->GetInnerHTML(innerHTML);
  if (innerHTML.IsEmpty())
    return S_FALSE;

  *aInnerHTML = ::SysAllocStringLen(innerHTML.get(), innerHTML.Length());
  if (!*aInnerHTML)
    return E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP 
nsAccessNodeWrap::get_language(BSTR __RPC_FAR *aLanguage)
{
__try {
  *aLanguage = NULL;

  nsAutoString language;
  if (NS_FAILED(GetLanguage(language))) {
    return E_FAIL;
  }

  if (language.IsEmpty())
    return S_FALSE;

  *aLanguage = ::SysAllocStringLen(language.get(), language.Length());
  if (!*aLanguage)
    return E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP 
nsAccessNodeWrap::get_localInterface( 
     void __RPC_FAR *__RPC_FAR *localInterface)
{
__try {
  *localInterface = static_cast<nsIAccessNode*>(this);
  NS_ADDREF_THIS();
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return S_OK;
}
 
void nsAccessNodeWrap::InitAccessibility()
{
  NS_ASSERTION(!gIsAccessibilityActive,
               "Accessibility was initialized already!");

  nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefBranch) {
    prefBranch->GetBoolPref("accessibility.disableenumvariant", &gIsEnumVariantSupportDisabled);
  }

  if (!gmUserLib) {
    gmUserLib =::LoadLibraryW(L"USER32.DLL");
  }

  if (gmUserLib) {
    if (!gmNotifyWinEvent)
      gmNotifyWinEvent = (LPFNNOTIFYWINEVENT)GetProcAddress(gmUserLib,"NotifyWinEvent");
    if (!gmGetGUIThreadInfo)
      gmGetGUIThreadInfo = (LPFNGETGUITHREADINFO)GetProcAddress(gmUserLib,"GetGUIThreadInfo");
  }

  DoATSpecificProcessing();
  
  nsAccessNode::InitXPAccessibility();
}

void nsAccessNodeWrap::ShutdownAccessibility()
{
  NS_IF_RELEASE(gTextEvent);
  ::DestroyCaret();

  NS_ASSERTION(gIsAccessibilityActive, "Accessibility was shutdown already!");

  nsAccessNode::ShutdownXPAccessibility();
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

PRBool nsAccessNodeWrap::IsOnlyMsaaCompatibleJawsPresent()
{
  HMODULE jhookhandle = ::GetModuleHandleW(L"jhook");
  if (!jhookhandle)
    return PR_FALSE;  

  PRUnichar fileName[MAX_PATH];
  ::GetModuleFileNameW(jhookhandle, fileName, MAX_PATH);

  DWORD dummy;
  DWORD length = ::GetFileVersionInfoSizeW(fileName, &dummy);

  LPBYTE versionInfo = new BYTE[length];
  ::GetFileVersionInfoW(fileName, 0, length, versionInfo);

  UINT uLen;
  VS_FIXEDFILEINFO *fixedFileInfo;
  ::VerQueryValueW(versionInfo, L"\\", (LPVOID*)&fixedFileInfo, &uLen);
  DWORD dwFileVersionMS = fixedFileInfo->dwFileVersionMS;
  DWORD dwFileVersionLS = fixedFileInfo->dwFileVersionLS;
  delete [] versionInfo;

  DWORD dwLeftMost = HIWORD(dwFileVersionMS);

  DWORD dwSecondRight = HIWORD(dwFileVersionLS);


  return (dwLeftMost < 8
          || (dwLeftMost == 8 && dwSecondRight < 2173));
}

void nsAccessNodeWrap::TurnOffNewTabSwitchingForJawsAndWE()
{
  HMODULE srHandle = ::GetModuleHandleW(L"jhook");
  if (!srHandle) {
    
    srHandle = ::GetModuleHandleW(L"gwm32inc");
    if (!srHandle) {
      
      return;
    }
  }

  
  
  
  nsCOMPtr<nsIPrefBranch> prefs (do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefs) {
    PRBool hasDisallowNewCtrlTabPref = PR_FALSE;
    nsresult rv = prefs->PrefHasUserValue(CTRLTAB_DISALLOW_FOR_SCREEN_READERS_PREF,
             &hasDisallowNewCtrlTabPref);
    if (NS_SUCCEEDED(rv) && hasDisallowNewCtrlTabPref) {
      
      
      
      
      return;
    }
    
    
    prefs->SetBoolPref(CTRLTAB_DISALLOW_FOR_SCREEN_READERS_PREF, PR_TRUE);
  }
}

void nsAccessNodeWrap::DoATSpecificProcessing()
{
  if (IsOnlyMsaaCompatibleJawsPresent())
    
    gIsIA2Disabled  = PR_TRUE;

  TurnOffNewTabSwitchingForJawsAndWE();
}
