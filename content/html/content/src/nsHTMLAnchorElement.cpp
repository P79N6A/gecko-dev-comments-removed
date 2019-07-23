





































#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMNSHTMLAnchorElement2.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLDocument.h"
#include "nsGenericHTMLElement.h"
#include "nsILink.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIEventStateManager.h"
#include "nsIURL.h"
#include "nsIEventStateManager.h"
#include "nsIDOMEvent.h"
#include "nsNetUtil.h"
#include "nsCRT.h"


#include "nsIContentIterator.h"
#include "nsIDOMText.h"
#include "nsIEnumerator.h"

#include "nsCOMPtr.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"

nsresult NS_NewContentIterator(nsIContentIterator** aInstancePtrResult);

class nsHTMLAnchorElement : public nsGenericHTMLElement,
                            public nsIDOMHTMLAnchorElement,
                            public nsIDOMNSHTMLAnchorElement2,
                            public nsILink
{
public:
  nsHTMLAnchorElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLAnchorElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLANCHORELEMENT  

  
  NS_DECL_NSIDOMNSHTMLANCHORELEMENT

  
  NS_DECL_NSIDOMNSHTMLANCHORELEMENT2

  
  NS_IMETHOD GetLinkState(nsLinkState &aState);
  NS_IMETHOD SetLinkState(nsLinkState aState);
  NS_IMETHOD GetHrefURI(nsIURI** aURI);
  NS_IMETHOD LinkAdded() { return NS_OK; }
  NS_IMETHOD LinkRemoved() { return NS_OK; }

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  virtual void SetFocus(nsPresContext* aPresContext);
  virtual PRBool IsFocusable(PRBool *aTabIndex = nsnull);

  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual PRBool IsLink(nsIURI** aURI) const;
  virtual void GetLinkTarget(nsAString& aTarget);

  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:
  
  nsLinkState mLinkState;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Anchor)


nsHTMLAnchorElement::nsHTMLAnchorElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo),
    mLinkState(eLinkState_Unknown)
{
}

nsHTMLAnchorElement::~nsHTMLAnchorElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLAnchorElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLAnchorElement, nsGenericElement) 



NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLAnchorElement, nsGenericHTMLElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLAnchorElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSHTMLAnchorElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSHTMLAnchorElement2)
  NS_INTERFACE_MAP_ENTRY(nsILink)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLAnchorElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(nsHTMLAnchorElement)


NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, Charset, charset)
NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, Coords, coords)
NS_IMPL_URI_ATTR(nsHTMLAnchorElement, Href, href)
NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, Hreflang, hreflang)
NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, Name, name)
NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, Rel, rel)
NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, Rev, rev)
NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, Shape, shape)
NS_IMPL_INT_ATTR_DEFAULT_VALUE(nsHTMLAnchorElement, TabIndex, tabindex, 0)
NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, Type, type)
NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, AccessKey, accesskey)


nsresult
nsHTMLAnchorElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                nsIContent* aBindingParent,
                                PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDocument) {
    RegUnRegAccessKey(PR_TRUE);
  }

  return rv;
}

void
nsHTMLAnchorElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  if (IsInDoc()) {
    RegUnRegAccessKey(PR_FALSE);
    GetCurrentDoc()->ForgetLink(this);
    
    
    mLinkState = eLinkState_Unknown;
  }
    
  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
}

NS_IMETHODIMP
nsHTMLAnchorElement::Blur()
{
  if (ShouldBlur(this)) {
    SetElementFocus(PR_FALSE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAnchorElement::Focus()
{
  if (ShouldFocus(this)) {
    SetElementFocus(PR_TRUE);
  }

  return NS_OK;
}

void
nsHTMLAnchorElement::SetFocus(nsPresContext* aPresContext)
{
  if (!aPresContext) {
    return;
  }

  
  nsILinkHandler *handler = aPresContext->GetLinkHandler();
  if (handler && aPresContext->EventStateManager()->
                               SetContentState(this, NS_EVENT_STATE_FOCUS)) {
    nsCOMPtr<nsIPresShell> presShell = aPresContext->GetPresShell();
    if (presShell) {
      presShell->ScrollContentIntoView(this, NS_PRESSHELL_SCROLL_IF_NOT_VISIBLE,
                                       NS_PRESSHELL_SCROLL_IF_NOT_VISIBLE);
    }
  }
}

PRBool
nsHTMLAnchorElement::IsFocusable(PRInt32 *aTabIndex)
{
  if (!nsGenericHTMLElement::IsFocusable(aTabIndex)) {
    return PR_FALSE;
  }

  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::tabindex)) {
    
    nsCOMPtr<nsIURI> absURI;
    if (!IsLink(getter_AddRefs(absURI))) {
      
      
      if (aTabIndex) {
        *aTabIndex = -1;
      }
      return PR_FALSE;
    }
  }

  if (aTabIndex && (sTabFocusModel & eTabFocus_linksMask) == 0) {
    *aTabIndex = -1;
  }

  return PR_TRUE;
}

nsresult
nsHTMLAnchorElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  return PostHandleEventForAnchors(aVisitor);
}

PRBool
nsHTMLAnchorElement::IsLink(nsIURI** aURI) const
{
  return IsHTMLLink(aURI);
}

void
nsHTMLAnchorElement::GetLinkTarget(nsAString& aTarget)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::target, aTarget);
  if (aTarget.IsEmpty()) {
    GetBaseTarget(aTarget);
  }
}

NS_IMETHODIMP
nsHTMLAnchorElement::GetTarget(nsAString& aValue)
{
  if (!GetAttr(kNameSpaceID_None, nsGkAtoms::target, aValue)) {
    GetBaseTarget(aValue);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAnchorElement::SetTarget(const nsAString& aValue)
{
  return SetAttr(kNameSpaceID_None, nsGkAtoms::target, aValue, PR_TRUE);
}

NS_IMETHODIMP    
nsHTMLAnchorElement::GetProtocol(nsAString& aProtocol)
{
  nsAutoString href;

  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  
  return GetProtocolFromHrefString(href, aProtocol, GetOwnerDoc());
}

NS_IMETHODIMP
nsHTMLAnchorElement::SetProtocol(const nsAString& aProtocol)
{
  nsAutoString href, new_href;
  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  rv = SetProtocolInHrefString(href, aProtocol, new_href);
  if (NS_FAILED(rv))
    
    return NS_OK;

  return SetHref(new_href);
}

NS_IMETHODIMP    
nsHTMLAnchorElement::GetHost(nsAString& aHost)
{
  nsAutoString href;
  
  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  return GetHostFromHrefString(href, aHost);
}

NS_IMETHODIMP
nsHTMLAnchorElement::SetHost(const nsAString& aHost)
{
  nsAutoString href, new_href;
  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  rv = SetHostInHrefString(href, aHost, new_href);
  if (NS_FAILED(rv))
    
    return NS_OK;

  return SetHref(new_href);
}

NS_IMETHODIMP    
nsHTMLAnchorElement::GetHostname(nsAString& aHostname)
{
  nsAutoString href;
  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  return GetHostnameFromHrefString(href, aHostname);
}

NS_IMETHODIMP
nsHTMLAnchorElement::SetHostname(const nsAString& aHostname)
{
  nsAutoString href, new_href;
  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  rv = SetHostnameInHrefString(href, aHostname, new_href);
  if (NS_FAILED(rv))
    
    return NS_OK;
  
  return SetHref(new_href);
}

NS_IMETHODIMP    
nsHTMLAnchorElement::GetPathname(nsAString& aPathname)
{
  nsAutoString href;
 
  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  return GetPathnameFromHrefString(href, aPathname);
}

NS_IMETHODIMP
nsHTMLAnchorElement::SetPathname(const nsAString& aPathname)
{
  nsAutoString href, new_href;
  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  rv = SetPathnameInHrefString(href, aPathname, new_href);
  if (NS_FAILED(rv))
    
    return NS_OK;

  return SetHref(new_href);
}

NS_IMETHODIMP    
nsHTMLAnchorElement::GetSearch(nsAString& aSearch)
{
  nsAutoString href;

  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  return GetSearchFromHrefString(href, aSearch);
}

NS_IMETHODIMP
nsHTMLAnchorElement::SetSearch(const nsAString& aSearch)
{
  nsAutoString href, new_href;
  nsresult rv = GetHref(href);

  if (NS_FAILED(rv))
    return rv;

  rv = SetSearchInHrefString(href, aSearch, new_href);
  if (NS_FAILED(rv))
    
    return NS_OK;

  return SetHref(new_href);
}

NS_IMETHODIMP    
nsHTMLAnchorElement::GetPort(nsAString& aPort)
{
  nsAutoString href;
  
  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  return GetPortFromHrefString(href, aPort);
}

NS_IMETHODIMP
nsHTMLAnchorElement::SetPort(const nsAString& aPort)
{
  nsAutoString href, new_href;
  nsresult rv = GetHref(href);

  if (NS_FAILED(rv))
    return rv;

  rv = SetPortInHrefString(href, aPort, new_href);
  if (NS_FAILED(rv))
    
    return NS_OK;
  
  return SetHref(new_href);
}

NS_IMETHODIMP    
nsHTMLAnchorElement::GetHash(nsAString& aHash)
{
  nsAutoString href;

  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  return GetHashFromHrefString(href, aHash);
}

NS_IMETHODIMP
nsHTMLAnchorElement::SetHash(const nsAString& aHash)
{
  nsAutoString href, new_href;
  nsresult rv = GetHref(href);

  if (NS_FAILED(rv))
    return rv;

  rv = SetHashInHrefString(href, aHash, new_href);
  if (NS_FAILED(rv))
    
    return NS_OK;

  return SetHref(new_href);
}

NS_IMETHODIMP    
nsHTMLAnchorElement::GetText(nsAString& aText)
{
  aText.Truncate();

  
  
  
  
  
  nsCOMPtr<nsIContentIterator> iter;
  nsresult rv = NS_NewContentIterator(getter_AddRefs(iter));
  NS_ENSURE_SUCCESS(rv, rv);

  
  iter->Init(this);

  
  
  
  iter->Last();
  iter->Prev();

  while(!iter->IsDone()) {
    nsCOMPtr<nsIDOMText> textNode(do_QueryInterface(iter->GetCurrentNode()));
    if(textNode) {
      
      textNode->GetData(aText);
      break;
    }

    iter->Prev();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAnchorElement::ToString(nsAString& aSource)
{
  return GetHref(aSource);
}

NS_IMETHODIMP    
nsHTMLAnchorElement::GetPing(nsAString& aValue)
{
  return GetURIListAttr(nsGkAtoms::ping, aValue);
}

NS_IMETHODIMP
nsHTMLAnchorElement::SetPing(const nsAString& aValue)
{
  return SetAttr(kNameSpaceID_None, nsGkAtoms::ping, aValue, PR_TRUE);
}

NS_IMETHODIMP
nsHTMLAnchorElement::GetLinkState(nsLinkState &aState)
{
  aState = mLinkState;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAnchorElement::SetLinkState(nsLinkState aState)
{
  mLinkState = aState;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAnchorElement::GetHrefURI(nsIURI** aURI)
{
  return GetHrefURIForAnchors(aURI);
}

nsresult
nsHTMLAnchorElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                             nsIAtom* aPrefix, const nsAString& aValue,
                             PRBool aNotify)
{
  if (aName == nsGkAtoms::href && kNameSpaceID_None == aNameSpaceID) {
    nsAutoString val;
    GetHref(val);
    if (!val.Equals(aValue)) {
      nsIDocument* doc = GetCurrentDoc();
      if (doc) {
        doc->ForgetLink(this);
        
        
        
      }
      SetLinkState(eLinkState_Unknown);
    }
  }

  if (aName == nsGkAtoms::accesskey && kNameSpaceID_None == aNameSpaceID) {
    RegUnRegAccessKey(PR_FALSE);
  }

  nsresult rv = nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix,
                                              aValue, aNotify);

  if (aName == nsGkAtoms::accesskey && kNameSpaceID_None == aNameSpaceID &&
      !aValue.IsEmpty()) {
    RegUnRegAccessKey(PR_TRUE);
  }

  return rv;
}

nsresult
nsHTMLAnchorElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                               PRBool aNotify)
{
  if (aAttribute == nsGkAtoms::href && kNameSpaceID_None == aNameSpaceID) {
    SetLinkState(eLinkState_Unknown);
  }

  if (aAttribute == nsGkAtoms::accesskey &&
      kNameSpaceID_None == aNameSpaceID) {
    RegUnRegAccessKey(PR_FALSE);
  }

  return nsGenericHTMLElement::UnsetAttr(aNameSpaceID, aAttribute, aNotify);
}
