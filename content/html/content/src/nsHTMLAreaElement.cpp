




































#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMNSHTMLAreaElement2.h"
#include "nsIDOMEventReceiver.h"
#include "nsGenericHTMLElement.h"
#include "nsILink.h"
#include "nsIPresShell.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIEventStateManager.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsReadableUtils.h"
#include "nsIDocument.h"

class nsHTMLAreaElement : public nsGenericHTMLElement,
                          public nsIDOMHTMLAreaElement,
                          public nsIDOMNSHTMLAreaElement2,
                          public nsILink
{
public:
  nsHTMLAreaElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLAreaElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLAREAELEMENT

  
  NS_DECL_NSIDOMNSHTMLAREAELEMENT

  
  NS_DECL_NSIDOMNSHTMLAREAELEMENT2

  
  NS_IMETHOD GetLinkState(nsLinkState &aState);
  NS_IMETHOD SetLinkState(nsLinkState aState);
  NS_IMETHOD GetHrefURI(nsIURI** aURI);
  NS_IMETHOD LinkAdded() { return NS_OK; }
  NS_IMETHOD LinkRemoved() { return NS_OK; }

  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual PRBool IsLink(nsIURI** aURI) const;
  virtual void GetLinkTarget(nsAString& aTarget);

  virtual void SetFocus(nsPresContext* aPresContext);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
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


NS_IMPL_NS_NEW_HTML_ELEMENT(Area)


nsHTMLAreaElement::nsHTMLAreaElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo),
    mLinkState(eLinkState_Unknown)
{
}

nsHTMLAreaElement::~nsHTMLAreaElement()
{
}

NS_IMPL_ADDREF_INHERITED(nsHTMLAreaElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLAreaElement, nsGenericElement) 



NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLAreaElement, nsGenericHTMLElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLAreaElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSHTMLAreaElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSHTMLAreaElement2)
  NS_INTERFACE_MAP_ENTRY(nsILink)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLAreaElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(nsHTMLAreaElement)


NS_IMPL_STRING_ATTR(nsHTMLAreaElement, AccessKey, accesskey)
NS_IMPL_STRING_ATTR(nsHTMLAreaElement, Alt, alt)
NS_IMPL_STRING_ATTR(nsHTMLAreaElement, Coords, coords)
NS_IMPL_URI_ATTR(nsHTMLAreaElement, Href, href)
NS_IMPL_BOOL_ATTR(nsHTMLAreaElement, NoHref, nohref)
NS_IMPL_STRING_ATTR(nsHTMLAreaElement, Shape, shape)
NS_IMPL_INT_ATTR_DEFAULT_VALUE(nsHTMLAreaElement, TabIndex, tabindex, 0)

NS_IMETHODIMP
nsHTMLAreaElement::GetTarget(nsAString& aValue)
{
  if (!GetAttr(kNameSpaceID_None, nsGkAtoms::target, aValue)) {
    GetBaseTarget(aValue);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAreaElement::SetTarget(const nsAString& aValue)
{
  return SetAttr(kNameSpaceID_None, nsGkAtoms::target, aValue, PR_TRUE);
}

nsresult
nsHTMLAreaElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  return PostHandleEventForAnchors(aVisitor);
}

PRBool
nsHTMLAreaElement::IsLink(nsIURI** aURI) const
{
  return IsHTMLLink(aURI);
}

void
nsHTMLAreaElement::GetLinkTarget(nsAString& aTarget)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::target, aTarget);
  if (aTarget.IsEmpty()) {
    GetBaseTarget(aTarget);
  }
}

void
nsHTMLAreaElement::SetFocus(nsPresContext* aPresContext)
{
  if (!aPresContext ||
      !aPresContext->EventStateManager()->SetContentState(this, 
                                                          NS_EVENT_STATE_FOCUS)) {
    return;
  }
  nsCOMPtr<nsIPresShell> presShell = aPresContext->GetPresShell();
  if (presShell) {
    presShell->ScrollContentIntoView(this, NS_PRESSHELL_SCROLL_ANYWHERE,
                                     NS_PRESSHELL_SCROLL_ANYWHERE);
  }
}

nsresult
nsHTMLAreaElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
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
nsHTMLAreaElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  if (IsInDoc()) {
    RegUnRegAccessKey(PR_FALSE);
    GetCurrentDoc()->ForgetLink(this);
    
    
    mLinkState = eLinkState_Unknown;
  }

  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
}

nsresult
nsHTMLAreaElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify)
{
  if (aName == nsGkAtoms::accesskey && aNameSpaceID == kNameSpaceID_None) {
    RegUnRegAccessKey(PR_FALSE);
  }

  if (aName == nsGkAtoms::href && aNameSpaceID == kNameSpaceID_None) {
    nsIDocument* doc = GetCurrentDoc();
    if (doc) {
      doc->ForgetLink(this);
      
      
      
    }
    SetLinkState(eLinkState_Unknown);
  }

  nsresult rv =
    nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix, aValue, aNotify);

  if (aName == nsGkAtoms::accesskey && aNameSpaceID == kNameSpaceID_None &&
      !aValue.IsEmpty()) {
    RegUnRegAccessKey(PR_TRUE);
  }

  return rv;
}

nsresult
nsHTMLAreaElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify)
{
  if (aAttribute == nsGkAtoms::accesskey &&
      aNameSpaceID == kNameSpaceID_None) {
    RegUnRegAccessKey(PR_FALSE);
  }

  return nsGenericHTMLElement::UnsetAttr(aNameSpaceID, aAttribute, aNotify);
}

NS_IMETHODIMP    
nsHTMLAreaElement::GetProtocol(nsAString& aProtocol)
{
  nsAutoString href;
  
  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  
  return GetProtocolFromHrefString(href, aProtocol, GetOwnerDoc());
}

NS_IMETHODIMP
nsHTMLAreaElement::SetProtocol(const nsAString& aProtocol)
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
nsHTMLAreaElement::GetHost(nsAString& aHost)
{
  nsAutoString href;
  
  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  return GetHostFromHrefString(href, aHost);
}

NS_IMETHODIMP
nsHTMLAreaElement::SetHost(const nsAString& aHost)
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
nsHTMLAreaElement::GetHostname(nsAString& aHostname)
{
  nsAutoString href;
  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  return GetHostnameFromHrefString(href, aHostname);
}

NS_IMETHODIMP
nsHTMLAreaElement::SetHostname(const nsAString& aHostname)
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
nsHTMLAreaElement::GetPathname(nsAString& aPathname)
{
  nsAutoString href;
 
  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  return GetPathnameFromHrefString(href, aPathname);
}

NS_IMETHODIMP
nsHTMLAreaElement::SetPathname(const nsAString& aPathname)
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
nsHTMLAreaElement::GetSearch(nsAString& aSearch)
{
  nsAutoString href;

  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  return GetSearchFromHrefString(href, aSearch);
}

NS_IMETHODIMP
nsHTMLAreaElement::SetSearch(const nsAString& aSearch)
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
nsHTMLAreaElement::GetPort(nsAString& aPort)
{
  nsAutoString href;
  
  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  return GetPortFromHrefString(href, aPort);
}

NS_IMETHODIMP
nsHTMLAreaElement::SetPort(const nsAString& aPort)
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
nsHTMLAreaElement::GetHash(nsAString& aHash)
{
  nsAutoString href;

  nsresult rv = GetHref(href);
  if (NS_FAILED(rv))
    return rv;

  return GetHashFromHrefString(href, aHash);
}

NS_IMETHODIMP
nsHTMLAreaElement::SetHash(const nsAString& aHash)
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
nsHTMLAreaElement::ToString(nsAString& aSource)
{
  return GetHref(aSource);
}

NS_IMETHODIMP    
nsHTMLAreaElement::GetPing(nsAString& aValue)
{
  return GetURIListAttr(nsGkAtoms::ping, aValue);
}

NS_IMETHODIMP
nsHTMLAreaElement::SetPing(const nsAString& aValue)
{
  return SetAttr(kNameSpaceID_None, nsGkAtoms::ping, aValue, PR_TRUE);
}

NS_IMETHODIMP
nsHTMLAreaElement::GetLinkState(nsLinkState &aState)
{
  aState = mLinkState;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAreaElement::SetLinkState(nsLinkState aState)
{
  mLinkState = aState;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAreaElement::GetHrefURI(nsIURI** aURI)
{
  return GetHrefURIForAnchors(aURI);
}
