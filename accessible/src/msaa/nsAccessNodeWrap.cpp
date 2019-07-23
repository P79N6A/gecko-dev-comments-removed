





































#include "nsAccessNodeWrap.h"
#include "ISimpleDOMNode_i.c"
#include "nsAccessibilityAtoms.h"
#include "nsIAccessibilityService.h"
#include "nsIAccessible.h"
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
#include "nsIServiceManager.h"
#include "nsIServiceManager.h"
#include "nsAttrName.h"


HINSTANCE nsAccessNodeWrap::gmAccLib = nsnull;
HINSTANCE nsAccessNodeWrap::gmUserLib = nsnull;
LPFNACCESSIBLEOBJECTFROMWINDOW nsAccessNodeWrap::gmAccessibleObjectFromWindow = nsnull;
LPFNNOTIFYWINEVENT nsAccessNodeWrap::gmNotifyWinEvent = nsnull;
LPFNGETGUITHREADINFO nsAccessNodeWrap::gmGetGUIThreadInfo = nsnull;

PRBool nsAccessNodeWrap::gIsEnumVariantSupportDisabled = 0;













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
    *ppv = NS_STATIC_CAST(ISimpleDOMNode*, this);

  if (nsnull == *ppv)
    return E_NOINTERFACE;      
   
  (NS_REINTERPRET_CAST(IUnknown*, *ppv))->AddRef(); 
  return S_OK;
}





STDMETHODIMP nsAccessNodeWrap::get_nodeInfo( 
     BSTR __RPC_FAR *aNodeName,
     short __RPC_FAR *aNameSpaceID,
     BSTR __RPC_FAR *aNodeValue,
     unsigned int __RPC_FAR *aNumChildren,
     unsigned int __RPC_FAR *aUniqueID,
     unsigned short __RPC_FAR *aNodeType)
{
  if (!mDOMNode)
    return E_FAIL;
 
  *aNodeName = nsnull;
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));

  PRUint16 nodeType = 0;
  mDOMNode->GetNodeType(&nodeType);
  *aNodeType=NS_STATIC_CAST(unsigned short, nodeType);

  if (*aNodeType !=  NODETYPE_TEXT) {
    nsAutoString nodeName;
    mDOMNode->GetNodeName(nodeName);
    *aNodeName =   ::SysAllocString(nodeName.get());
  }

  nsAutoString nodeValue;

  mDOMNode->GetNodeValue(nodeValue);
  *aNodeValue = ::SysAllocString(nodeValue.get());
  *aNameSpaceID = content ? NS_STATIC_CAST(short, content->GetNameSpaceID()) : 0;

  
  
  
  
  void *uniqueID;
  GetUniqueID(&uniqueID);
  *aUniqueID = - NS_PTR_TO_INT32(uniqueID);

  *aNumChildren = 0;
  PRUint32 numChildren = 0;
  nsCOMPtr<nsIDOMNodeList> nodeList;
  mDOMNode->GetChildNodes(getter_AddRefs(nodeList));
  if (nodeList && NS_OK == nodeList->GetLength(&numChildren))
    *aNumChildren = NS_STATIC_CAST(unsigned int, numChildren);

  return S_OK;
}


       
STDMETHODIMP nsAccessNodeWrap::get_attributes( 
     unsigned short aMaxAttribs,
     BSTR __RPC_FAR *aAttribNames,
     short __RPC_FAR *aNameSpaceIDs,
     BSTR __RPC_FAR *aAttribValues,
     unsigned short __RPC_FAR *aNumAttribs)
{
  *aNumAttribs = 0;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (!content) 
    return E_FAIL;

  PRUint32 numAttribs = content->GetAttrCount();

  if (numAttribs > aMaxAttribs)
    numAttribs = aMaxAttribs;
  *aNumAttribs = NS_STATIC_CAST(unsigned short, numAttribs);

  for (PRUint32 index = 0; index < numAttribs; index++) {
    aNameSpaceIDs[index] = 0; aAttribValues[index] = aAttribNames[index] = nsnull;
    nsAutoString attributeValue;
    const char *pszAttributeName; 

    const nsAttrName* name = content->GetAttrNameAt(index);
    aNameSpaceIDs[index] = NS_STATIC_CAST(short, name->NamespaceID());
    name->LocalName()->GetUTF8String(&pszAttributeName);
    aAttribNames[index] = ::SysAllocString(NS_ConvertUTF8toUTF16(pszAttributeName).get());
    content->GetAttr(name->NamespaceID(), name->LocalName(), attributeValue);
    aAttribValues[index] = ::SysAllocString(attributeValue.get());
  }

  return S_OK; 
}
        

STDMETHODIMP nsAccessNodeWrap::get_attributesForNames( 
     unsigned short aNumAttribs,
     BSTR __RPC_FAR *aAttribNames,
     short __RPC_FAR *aNameSpaceID,
     BSTR __RPC_FAR *aAttribValues)
{
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
      nsAutoString attributeName(nsDependentString(NS_STATIC_CAST(PRUnichar*,aAttribNames[index])));
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

  return S_OK; 
}


STDMETHODIMP nsAccessNodeWrap::get_computedStyle( 
     unsigned short aMaxStyleProperties,
     boolean aUseAlternateView,
     BSTR __RPC_FAR *aStyleProperties,
     BSTR __RPC_FAR *aStyleValues,
     unsigned short __RPC_FAR *aNumStyleProperties)
{
  nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(mDOMNode));
  if (!domElement)
    return E_FAIL;
  
  *aNumStyleProperties = 0;
  nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
  GetComputedStyleDeclaration(EmptyString(), domElement, getter_AddRefs(cssDecl));
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
  *aNumStyleProperties = NS_STATIC_CAST(unsigned short, realIndex);

  return S_OK;
}


STDMETHODIMP nsAccessNodeWrap::get_computedStyleForProperties( 
     unsigned short aNumStyleProperties,
     boolean aUseAlternateView,
     BSTR __RPC_FAR *aStyleProperties,
     BSTR __RPC_FAR *aStyleValues)
{
  nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(mDOMNode));
  if (!domElement)
    return E_FAIL;
 
  nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
  GetComputedStyleDeclaration(EmptyString(), domElement, getter_AddRefs(cssDecl));
  NS_ENSURE_TRUE(cssDecl, E_FAIL);

  PRUint32 index;
  for (index = 0; index < aNumStyleProperties; index ++) {
    nsAutoString value;
    if (aStyleProperties[index])
      cssDecl->GetPropertyValue(nsDependentString(NS_STATIC_CAST(PRUnichar*,aStyleProperties[index])), value);  
    aStyleValues[index] = ::SysAllocString(value.get());
  }

  return S_OK;
}

STDMETHODIMP nsAccessNodeWrap::scrollTo( boolean aScrollTopLeft)
{
  PRUint32 scrollType =
    aScrollTopLeft ? nsIAccessibleScrollType::SCROLL_TYPE_TOP_LEFT :
                     nsIAccessibleScrollType::SCROLL_TYPE_BOTTOM_RIGHT;

  nsresult rv = ScrollTo(scrollType);
  if (NS_SUCCEEDED(rv))
    return S_OK;

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
    iNode = NS_STATIC_CAST(ISimpleDOMNode*, newNode);
    iNode->AddRef();
  }

  return iNode;
}


STDMETHODIMP nsAccessNodeWrap::get_parentNode(ISimpleDOMNode __RPC_FAR *__RPC_FAR *aNode)
{
  if (!mDOMNode)
    return E_FAIL;
 
  nsCOMPtr<nsIDOMNode> node;
  mDOMNode->GetParentNode(getter_AddRefs(node));
  *aNode = MakeAccessNode(node);

  return S_OK;
}

STDMETHODIMP nsAccessNodeWrap::get_firstChild(ISimpleDOMNode __RPC_FAR *__RPC_FAR *aNode)
{
  if (!mDOMNode)
    return E_FAIL;
 
  nsCOMPtr<nsIDOMNode> node;
  mDOMNode->GetFirstChild(getter_AddRefs(node));
  *aNode = MakeAccessNode(node);

  return S_OK;
}

STDMETHODIMP nsAccessNodeWrap::get_lastChild(ISimpleDOMNode __RPC_FAR *__RPC_FAR *aNode)
{
  if (!mDOMNode)
    return E_FAIL;

  nsCOMPtr<nsIDOMNode> node;
  mDOMNode->GetLastChild(getter_AddRefs(node));
  *aNode = MakeAccessNode(node);

  return S_OK;
}

STDMETHODIMP nsAccessNodeWrap::get_previousSibling(ISimpleDOMNode __RPC_FAR *__RPC_FAR *aNode)
{
  if (!mDOMNode)
    return E_FAIL;

  nsCOMPtr<nsIDOMNode> node;
  mDOMNode->GetPreviousSibling(getter_AddRefs(node));
  *aNode = MakeAccessNode(node);

  return S_OK;
}

STDMETHODIMP nsAccessNodeWrap::get_nextSibling(ISimpleDOMNode __RPC_FAR *__RPC_FAR *aNode)
{
  if (!mDOMNode)
    return E_FAIL;

  nsCOMPtr<nsIDOMNode> node;
  mDOMNode->GetNextSibling(getter_AddRefs(node));
  *aNode = MakeAccessNode(node);

  return S_OK;
}

STDMETHODIMP 
nsAccessNodeWrap::get_childAt(unsigned aChildIndex,
                              ISimpleDOMNode __RPC_FAR *__RPC_FAR *aNode)
{
  *aNode = nsnull;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (!content)
    return E_FAIL;  

  nsCOMPtr<nsIDOMNode> node =
    do_QueryInterface(content->GetChildAt(aChildIndex));

  if (!node)
    return E_FAIL; 

  *aNode = MakeAccessNode(node);

  return S_OK;
}

STDMETHODIMP 
nsAccessNodeWrap::get_innerHTML(BSTR __RPC_FAR *aInnerHTML)
{
  *aInnerHTML = nsnull;

  nsCOMPtr<nsIDOMNSHTMLElement> domNSElement(do_QueryInterface(mDOMNode));
  if (!domNSElement)
    return E_FAIL; 

  nsAutoString innerHTML;
  domNSElement->GetInnerHTML(innerHTML);
  *aInnerHTML = ::SysAllocString(innerHTML.get());

  return S_OK;
}

STDMETHODIMP 
nsAccessNodeWrap::get_language(BSTR __RPC_FAR *aLanguage)
{
  *aLanguage = nsnull;
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (!content) {
    return E_FAIL;
  }

  nsAutoString language;
  for (nsIContent *walkUp = content; walkUp = walkUp->GetParent(); walkUp) {
    if (walkUp->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::lang, language)) {
      break;
    }
  }

  if (language.IsEmpty()) { 
    nsIDocument *doc = content->GetOwnerDoc();
    if (doc) {
      doc->GetHeaderData(nsAccessibilityAtoms::headerContentLanguage, language);
    }
  }
 
  *aLanguage = ::SysAllocString(language.get());
  return S_OK;
}

STDMETHODIMP 
nsAccessNodeWrap::get_localInterface( 
     void __RPC_FAR *__RPC_FAR *localInterface)
{
  *localInterface = NS_STATIC_CAST(nsIAccessNode*, this);
  NS_ADDREF_THIS();
  return S_OK;
}
 
void nsAccessNodeWrap::InitAccessibility()
{
  if (gIsAccessibilityActive) {
    return;
  }

  nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefBranch) {
    prefBranch->GetBoolPref("accessibility.disableenumvariant", &gIsEnumVariantSupportDisabled);
  }

  if (!gmUserLib) {
    gmUserLib =::LoadLibrary("USER32.DLL");
  }

  if (gmUserLib) {
    if (!gmNotifyWinEvent)
      gmNotifyWinEvent = (LPFNNOTIFYWINEVENT)GetProcAddress(gmUserLib,"NotifyWinEvent");
    if (!gmGetGUIThreadInfo)
      gmGetGUIThreadInfo = (LPFNGETGUITHREADINFO)GetProcAddress(gmUserLib,"GetGUIThreadInfo");
  }

  nsAccessNode::InitXPAccessibility();
}

void nsAccessNodeWrap::ShutdownAccessibility()
{
  if (!gIsAccessibilityActive) {
    return;
  }

  nsAccessNode::ShutdownXPAccessibility();
}
