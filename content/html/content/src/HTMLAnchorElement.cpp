





#include "mozilla/dom/HTMLAnchorElement.h"
#include "mozilla/dom/HTMLAnchorElementBinding.h"

#include "nsCOMPtr.h"
#include "nsContentUtils.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsHTMLDNSPrefetch.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Anchor)

namespace mozilla {
namespace dom {

#define ANCHOR_ELEMENT_FLAG_BIT(n_) NODE_FLAG_BIT(ELEMENT_TYPE_SPECIFIC_BITS_OFFSET + (n_))


enum {
  
  HTML_ANCHOR_DNS_PREFETCH_REQUESTED =    ANCHOR_ELEMENT_FLAG_BIT(0),

  
  HTML_ANCHOR_DNS_PREFETCH_DEFERRED =     ANCHOR_ELEMENT_FLAG_BIT(1)
};


PR_STATIC_ASSERT(ELEMENT_TYPE_SPECIFIC_BITS_OFFSET + 1 < 32);

#undef ANCHOR_ELEMENT_FLAG_BIT

HTMLAnchorElement::~HTMLAnchorElement()
{
}

NS_IMPL_ADDREF_INHERITED(HTMLAnchorElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLAnchorElement, Element)


NS_INTERFACE_TABLE_HEAD(HTMLAnchorElement)
  NS_HTML_CONTENT_INTERFACES(nsGenericHTMLElement)
  NS_INTERFACE_TABLE_INHERITED3(HTMLAnchorElement,
                                nsIDOMHTMLAnchorElement,
                                nsILink,
                                Link)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE
NS_ELEMENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(HTMLAnchorElement)

JSObject*
HTMLAnchorElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aScope)
{
  return HTMLAnchorElementBinding::Wrap(aCx, aScope, this);
}

NS_IMPL_STRING_ATTR(HTMLAnchorElement, Charset, charset)
NS_IMPL_STRING_ATTR(HTMLAnchorElement, Coords, coords)
NS_IMPL_URI_ATTR(HTMLAnchorElement, Href, href)
NS_IMPL_STRING_ATTR(HTMLAnchorElement, Hreflang, hreflang)
NS_IMPL_STRING_ATTR(HTMLAnchorElement, Name, name)
NS_IMPL_STRING_ATTR(HTMLAnchorElement, Rel, rel)
NS_IMPL_STRING_ATTR(HTMLAnchorElement, Rev, rev)
NS_IMPL_STRING_ATTR(HTMLAnchorElement, Shape, shape)
NS_IMPL_STRING_ATTR(HTMLAnchorElement, Type, type)
NS_IMPL_STRING_ATTR(HTMLAnchorElement, Download, download)

int32_t
HTMLAnchorElement::TabIndexDefault()
{
  return 0;
}

void
HTMLAnchorElement::GetItemValueText(nsAString& aValue)
{
  GetHref(aValue);
}

void
HTMLAnchorElement::SetItemValueText(const nsAString& aValue)
{
  SetHref(aValue);
}

bool
HTMLAnchorElement::Draggable() const
{
  
  
  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::href)) {
    
    return nsGenericHTMLElement::Draggable();
  }

  return !AttrValueIs(kNameSpaceID_None, nsGkAtoms::draggable,
                      nsGkAtoms::_false, eIgnoreCase);
}

void
HTMLAnchorElement::OnDNSPrefetchRequested()
{
  UnsetFlags(HTML_ANCHOR_DNS_PREFETCH_DEFERRED);
  SetFlags(HTML_ANCHOR_DNS_PREFETCH_REQUESTED);
}

void
HTMLAnchorElement::OnDNSPrefetchDeferred()
{
  UnsetFlags(HTML_ANCHOR_DNS_PREFETCH_REQUESTED);
  SetFlags(HTML_ANCHOR_DNS_PREFETCH_DEFERRED);
}

bool
HTMLAnchorElement::HasDeferredDNSPrefetchRequest()
{
  return HasFlag(HTML_ANCHOR_DNS_PREFETCH_DEFERRED);
}

nsresult
HTMLAnchorElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers)
{
  Link::ResetLinkState(false, Link::ElementHasHref());

  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (aDocument) {
    aDocument->RegisterPendingLinkUpdate(this);
    if (nsHTMLDNSPrefetch::IsAllowed(OwnerDoc())) {
      nsHTMLDNSPrefetch::PrefetchLow(this);
    }
  }

  return rv;
}

void
HTMLAnchorElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  
  
  

  
  if (HasFlag(HTML_ANCHOR_DNS_PREFETCH_DEFERRED))
    UnsetFlags(HTML_ANCHOR_DNS_PREFETCH_DEFERRED);
  
  else if (HasFlag(HTML_ANCHOR_DNS_PREFETCH_REQUESTED)) {
    UnsetFlags(HTML_ANCHOR_DNS_PREFETCH_REQUESTED);
    
    
    nsHTMLDNSPrefetch::CancelPrefetchLow(this, NS_ERROR_ABORT);
  }
  
  
  
  Link::ResetLinkState(false, Link::ElementHasHref());
  
  nsIDocument* doc = GetCurrentDoc();
  if (doc) {
    doc->UnregisterPendingLinkUpdate(this);
  }

  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
}

bool
HTMLAnchorElement::IsHTMLFocusable(bool aWithMouse,
                                   bool *aIsFocusable, int32_t *aTabIndex)
{
  if (nsGenericHTMLElement::IsHTMLFocusable(aWithMouse, aIsFocusable, aTabIndex)) {
    return true;
  }

  
  nsIDocument* doc = GetCurrentDoc();
  if (doc) {
    nsIPresShell* presShell = doc->GetShell();
    if (presShell) {
      nsPresContext* presContext = presShell->GetPresContext();
      if (presContext && !presContext->GetLinkHandler()) {
        *aIsFocusable = false;
        return false;
      }
    }
  }

  if (IsEditable()) {
    if (aTabIndex) {
      *aTabIndex = -1;
    }

    *aIsFocusable = false;

    return true;
  }

  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::tabindex)) {
    
    if (!Link::HasURI()) {
      
      
      if (aTabIndex) {
        *aTabIndex = -1;
      }

      *aIsFocusable = false;

      return false;
    }
  }

  if (aTabIndex && (sTabFocusModel & eTabFocus_linksMask) == 0) {
    *aTabIndex = -1;
  }

  *aIsFocusable = true;

  return false;
}

nsresult
HTMLAnchorElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  return PreHandleEventForAnchors(aVisitor);
}

nsresult
HTMLAnchorElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  return PostHandleEventForAnchors(aVisitor);
}

bool
HTMLAnchorElement::IsLink(nsIURI** aURI) const
{
  return IsHTMLLink(aURI);
}

void
HTMLAnchorElement::GetLinkTarget(nsAString& aTarget)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::target, aTarget);
  if (aTarget.IsEmpty()) {
    GetBaseTarget(aTarget);
  }
}

NS_IMETHODIMP
HTMLAnchorElement::GetTarget(nsAString& aValue)
{
  if (!GetAttr(kNameSpaceID_None, nsGkAtoms::target, aValue)) {
    GetBaseTarget(aValue);
  }
  return NS_OK;
}

NS_IMETHODIMP
HTMLAnchorElement::SetTarget(const nsAString& aValue)
{
  return SetAttr(kNameSpaceID_None, nsGkAtoms::target, aValue, true);
}

#define IMPL_URI_PART(_part)                                 \
  NS_IMETHODIMP                                              \
  HTMLAnchorElement::Get##_part(nsAString& a##_part)         \
  {                                                          \
    Link::Get##_part(a##_part);                              \
    return NS_OK;                                            \
  }                                                          \
  NS_IMETHODIMP                                              \
  HTMLAnchorElement::Set##_part(const nsAString& a##_part)   \
  {                                                          \
    Link::Set##_part(a##_part);                              \
    return NS_OK;                                            \
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
HTMLAnchorElement::GetText(nsAString& aText)
{
  nsContentUtils::GetNodeTextContent(this, true, aText);
  return NS_OK;
}

NS_IMETHODIMP    
HTMLAnchorElement::SetText(const nsAString& aText)
{
  return nsContentUtils::SetNodeTextContent(this, aText, false);
}

NS_IMETHODIMP
HTMLAnchorElement::ToString(nsAString& aSource)
{
  return GetHref(aSource);
}

NS_IMETHODIMP    
HTMLAnchorElement::GetPing(nsAString& aValue)
{
  return GetURIListAttr(nsGkAtoms::ping, aValue);
}

NS_IMETHODIMP
HTMLAnchorElement::SetPing(const nsAString& aValue)
{
  return SetAttr(kNameSpaceID_None, nsGkAtoms::ping, aValue, true);
}

nsLinkState
HTMLAnchorElement::GetLinkState() const
{
  return Link::GetLinkState();
}

already_AddRefed<nsIURI>
HTMLAnchorElement::GetHrefURI() const
{
  nsCOMPtr<nsIURI> uri = Link::GetCachedURI();
  if (uri) {
    return uri.forget();
  }

  return GetHrefURIForAnchors();
}

nsresult
HTMLAnchorElement::SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify)
{
  bool reset = false;
  if (aName == nsGkAtoms::href && kNameSpaceID_None == aNameSpaceID) {
    
    
    if (!Link::HasCachedURI()) {
      reset = true;
    }
    
    else {
      nsAutoString val;
      GetHref(val);
      if (!val.Equals(aValue)) {
        reset = true;
      }
    }
  }

  nsresult rv = nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix,
                                              aValue, aNotify);

  
  
  
  
  
  if (reset) {
    Link::ResetLinkState(!!aNotify, true);
  }

  return rv;
}

nsresult
HTMLAnchorElement::UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify)
{
  nsresult rv = nsGenericHTMLElement::UnsetAttr(aNameSpaceID, aAttribute,
                                                aNotify);

  
  
  
  
  
  if (aAttribute == nsGkAtoms::href && kNameSpaceID_None == aNameSpaceID) {
    Link::ResetLinkState(!!aNotify, false);
  }

  return rv;
}

bool
HTMLAnchorElement::ParseAttribute(int32_t aNamespaceID,
                                  nsIAtom* aAttribute,
                                  const nsAString& aValue,
                                  nsAttrValue& aResult)
{
  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

nsEventStates
HTMLAnchorElement::IntrinsicState() const
{
  return Link::LinkState() | nsGenericHTMLElement::IntrinsicState();
}

size_t
HTMLAnchorElement::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  return nsGenericHTMLElement::SizeOfExcludingThis(aMallocSizeOf) +
         Link::SizeOfExcludingThis(aMallocSizeOf);
}

} 
} 
