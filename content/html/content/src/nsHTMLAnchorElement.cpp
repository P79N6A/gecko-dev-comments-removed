






































#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMNSHTMLAnchorElement2.h"
#include "nsGenericHTMLElement.h"
#include "nsILink.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsIEventStateManager.h"


#include "nsIContentIterator.h"
#include "nsIDOMText.h"

#include "nsHTMLDNSPrefetch.h"

#include "Link.h"
using namespace mozilla::dom;

nsresult NS_NewPreContentIterator(nsIContentIterator** aInstancePtrResult);

class nsHTMLAnchorElement : public nsGenericHTMLElement,
                            public nsIDOMHTMLAnchorElement,
                            public nsIDOMNSHTMLAnchorElement2,
                            public nsILink,
                            public Link
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

  
  NS_IMETHOD LinkAdded() { return NS_OK; }
  NS_IMETHOD LinkRemoved() { return NS_OK; }

  
  NS_IMETHOD GetDraggable(PRBool* aDraggable);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  virtual PRBool IsHTMLFocusable(PRBool *aIsFocusable, PRInt32 *aTabIndex);

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual PRBool IsLink(nsIURI** aURI) const;
  virtual void GetLinkTarget(nsAString& aTarget);
  virtual nsLinkState GetLinkState() const;
  virtual void SetLinkState(nsLinkState aState);
  virtual already_AddRefed<nsIURI> GetHrefURI() const;

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
  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual void DropCachedHref();

protected:
  void ResetLinkCacheState();
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Anchor)

nsHTMLAnchorElement::nsHTMLAnchorElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLAnchorElement::~nsHTMLAnchorElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLAnchorElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLAnchorElement, nsGenericElement) 



NS_INTERFACE_TABLE_HEAD(nsHTMLAnchorElement)
  NS_HTML_CONTENT_INTERFACE_TABLE4(nsHTMLAnchorElement,
                                   nsIDOMHTMLAnchorElement,
                                   nsIDOMNSHTMLAnchorElement,
                                   nsIDOMNSHTMLAnchorElement2,
                                   nsILink)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLAnchorElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLAnchorElement)


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

NS_IMETHODIMP
nsHTMLAnchorElement::GetDraggable(PRBool* aDraggable)
{
  
  
  if (HasAttr(kNameSpaceID_None, nsGkAtoms::href)) {
    *aDraggable = !AttrValueIs(kNameSpaceID_None, nsGkAtoms::draggable,
                               nsGkAtoms::_false, eIgnoreCase);
    return NS_OK;
  }

  
  return nsGenericHTMLElement::GetDraggable(aDraggable);
}

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

  
  if (aDocument && nsHTMLDNSPrefetch::IsAllowed(GetOwnerDoc())) {
    nsHTMLDNSPrefetch::PrefetchLow(this);
  }
  return rv;
}

void
nsHTMLAnchorElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  if (IsInDoc()) {
    RegUnRegAccessKey(PR_FALSE);
    ResetLinkCacheState();
  }
    
  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
}

NS_IMETHODIMP
nsHTMLAnchorElement::Blur()
{
  return nsGenericHTMLElement::Blur();
}

NS_IMETHODIMP
nsHTMLAnchorElement::Focus()
{
  return nsGenericHTMLElement::Focus();
}

PRBool
nsHTMLAnchorElement::IsHTMLFocusable(PRBool *aIsFocusable, PRInt32 *aTabIndex)
{
  if (nsGenericHTMLElement::IsHTMLFocusable(aIsFocusable, aTabIndex)) {
    return PR_TRUE;
  }

  
  nsIDocument* doc = GetCurrentDoc();
  if (doc) {
    nsIPresShell* presShell = doc->GetPrimaryShell();
    if (presShell) {
      nsPresContext* presContext = presShell->GetPresContext();
      if (presContext && !presContext->GetLinkHandler()) {
        *aIsFocusable = PR_FALSE;
        return PR_FALSE;
      }
    }
  }

  if (IsEditable()) {
    if (aTabIndex) {
      *aTabIndex = -1;
    }

    *aIsFocusable = PR_FALSE;

    return PR_TRUE;
  }

  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::tabindex)) {
    
    nsCOMPtr<nsIURI> absURI;
    if (!IsLink(getter_AddRefs(absURI))) {
      
      
      if (aTabIndex) {
        *aTabIndex = -1;
      }

      *aIsFocusable = PR_FALSE;

      return PR_FALSE;
    }
  }

  if (aTabIndex && (sTabFocusModel & eTabFocus_linksMask) == 0) {
    *aTabIndex = -1;
  }

  *aIsFocusable = PR_TRUE;

  return PR_FALSE;
}

nsresult
nsHTMLAnchorElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  return PreHandleEventForAnchors(aVisitor);
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

#define IMPL_URI_PART(_part)                                 \
  NS_IMETHODIMP                                              \
  nsHTMLAnchorElement::Get##_part(nsAString& a##_part)       \
  {                                                          \
    return Get##_part##FromHrefURI(a##_part);                \
  }                                                          \
  NS_IMETHODIMP                                              \
  nsHTMLAnchorElement::Set##_part(const nsAString& a##_part) \
  {                                                          \
    return Set##_part##InHrefURI(a##_part);                  \
  }

IMPL_URI_PART(Protocol)
IMPL_URI_PART(Host)
IMPL_URI_PART(Hostname)
IMPL_URI_PART(Pathname)
IMPL_URI_PART(Search)
IMPL_URI_PART(Port)
IMPL_URI_PART(Hash)

#undef IMPL_URI_PART

NS_IMETHODIMP    
nsHTMLAnchorElement::GetText(nsAString& aText)
{
  aText.Truncate();

  
  
  
  
  
  nsCOMPtr<nsIContentIterator> iter;
  nsresult rv = NS_NewPreContentIterator(getter_AddRefs(iter));
  NS_ENSURE_SUCCESS(rv, rv);

  
  iter->Init(this);

  
  
  iter->Last();

  while (!iter->IsDone()) {
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

nsLinkState
nsHTMLAnchorElement::GetLinkState() const
{
  return Link::GetLinkState();
}

void
nsHTMLAnchorElement::SetLinkState(nsLinkState aState)
{
  Link::SetLinkState(aState);
}

already_AddRefed<nsIURI>
nsHTMLAnchorElement::GetHrefURI() const
{
  return GetHrefURIForAnchors();
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
      ResetLinkCacheState();
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
    ResetLinkCacheState();
  }

  if (aAttribute == nsGkAtoms::accesskey &&
      kNameSpaceID_None == aNameSpaceID) {
    RegUnRegAccessKey(PR_FALSE);
  }

  return nsGenericHTMLElement::UnsetAttr(aNameSpaceID, aAttribute, aNotify);
}

PRBool
nsHTMLAnchorElement::ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None && aAttribute == nsGkAtoms::href) {
    return aResult.ParseLazyURIValue(aValue);
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

void
nsHTMLAnchorElement::DropCachedHref()
{
  nsAttrValue* attr =
    const_cast<nsAttrValue*>(mAttrsAndChildren.GetAttr(nsGkAtoms::href));

  if (!attr || attr->Type() != nsAttrValue::eLazyURIValue)
    return;

  attr->DropCachedURI();
}

void
nsHTMLAnchorElement::ResetLinkCacheState()
{
  Link::ResetLinkState();

  
  
  const nsAttrValue* attr = mAttrsAndChildren.GetAttr(nsGkAtoms::href);
  if (attr && attr->Type() == nsAttrValue::eLazyURIValue) {
    const_cast<nsAttrValue*>(attr)->DropCachedURI();
  }
}
