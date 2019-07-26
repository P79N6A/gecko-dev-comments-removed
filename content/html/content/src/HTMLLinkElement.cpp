




#include "mozilla/dom/HTMLLinkElement.h"

#include "mozilla/dom/HTMLLinkElementBinding.h"
#include "nsGenericHTMLElement.h"
#include "nsILink.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsIDOMStyleSheet.h"
#include "nsIStyleSheet.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsIDocument.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsContentUtils.h"
#include "nsPIDOMWindow.h"
#include "nsAsyncDOMEvent.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Link)

namespace mozilla {
namespace dom {

HTMLLinkElement::HTMLLinkElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo),
    Link(this)
{
  SetIsDOMBinding();
}

HTMLLinkElement::~HTMLLinkElement()
{
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(HTMLLinkElement,
                                                  nsGenericHTMLElement)
  tmp->nsStyleLinkElement::Traverse(cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(HTMLLinkElement,
                                                nsGenericHTMLElement)
  tmp->nsStyleLinkElement::Unlink();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(HTMLLinkElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLLinkElement, Element)



NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(HTMLLinkElement)
  NS_HTML_CONTENT_INTERFACE_TABLE5(HTMLLinkElement,
                                   nsIDOMHTMLLinkElement,
                                   nsIDOMLinkStyle,
                                   nsILink,
                                   nsIStyleSheetLinkingElement,
                                   Link)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLLinkElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(HTMLLinkElement)

bool
HTMLLinkElement::GetDisabled(ErrorResult& aRv)
{
  nsCOMPtr<nsIDOMStyleSheet> ss = do_QueryInterface(GetSheet());
  if (!ss) {
    return false;
  }

  bool disabled = false;
  aRv = ss->GetDisabled(&disabled);
  return disabled;
}

NS_IMETHODIMP
HTMLLinkElement::GetDisabled(bool* aDisabled)
{
  ErrorResult rv;
  *aDisabled = GetDisabled(rv);
  return rv.ErrorCode();
}

void
HTMLLinkElement::SetDisabled(bool aDisabled, ErrorResult& aRv)
{
  nsCOMPtr<nsIDOMStyleSheet> ss = do_QueryInterface(GetSheet());
  if (!ss) {
    return;
  }

  aRv = ss->SetDisabled(aDisabled);
}

NS_IMETHODIMP
HTMLLinkElement::SetDisabled(bool aDisabled)
{
  ErrorResult rv;
  SetDisabled(aDisabled, rv);
  return rv.ErrorCode();
}


NS_IMPL_STRING_ATTR(HTMLLinkElement, Charset, charset)
NS_IMPL_URI_ATTR(HTMLLinkElement, Href, href)
NS_IMPL_STRING_ATTR(HTMLLinkElement, Hreflang, hreflang)
NS_IMPL_STRING_ATTR(HTMLLinkElement, Media, media)
NS_IMPL_STRING_ATTR(HTMLLinkElement, Rel, rel)
NS_IMPL_STRING_ATTR(HTMLLinkElement, Rev, rev)
NS_IMPL_STRING_ATTR(HTMLLinkElement, Target, target)
NS_IMPL_STRING_ATTR(HTMLLinkElement, Type, type)
NS_IMPL_STRING_ATTR(HTMLLinkElement, CrossOrigin, crossorigin)

void
HTMLLinkElement::GetItemValueText(nsAString& aValue)
{
  GetHref(aValue);
}

void
HTMLLinkElement::SetItemValueText(const nsAString& aValue)
{
  SetHref(aValue);
}

nsresult
HTMLLinkElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
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
  }

  void (HTMLLinkElement::*update)() = &HTMLLinkElement::UpdateStyleSheetInternal;
  nsContentUtils::AddScriptRunner(NS_NewRunnableMethod(this, update));

  CreateAndDispatchEvent(aDocument, NS_LITERAL_STRING("DOMLinkAdded"));

  return rv;
}

NS_IMETHODIMP
HTMLLinkElement::LinkAdded()
{
  CreateAndDispatchEvent(OwnerDoc(), NS_LITERAL_STRING("DOMLinkAdded"));
  return NS_OK;
}

NS_IMETHODIMP
HTMLLinkElement::LinkRemoved()
{
  CreateAndDispatchEvent(OwnerDoc(), NS_LITERAL_STRING("DOMLinkRemoved"));
  return NS_OK;
}

void
HTMLLinkElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  
  
  Link::ResetLinkState(false, Link::ElementHasHref());

  
  
  nsCOMPtr<nsIDocument> oldDoc = GetCurrentDoc();
  if (oldDoc) {
    oldDoc->UnregisterPendingLinkUpdate(this);
  }
  CreateAndDispatchEvent(oldDoc, NS_LITERAL_STRING("DOMLinkRemoved"));
  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
  UpdateStyleSheetInternal(oldDoc);
}

bool
HTMLLinkElement::ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None &&
      aAttribute == nsGkAtoms::crossorigin) {
    ParseCORSValue(aValue, aResult);
    return true;
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

void
HTMLLinkElement::CreateAndDispatchEvent(nsIDocument* aDoc,
                                        const nsAString& aEventName)
{
  if (!aDoc)
    return;

  
  
  
  
  
  
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::_empty, &nsGkAtoms::stylesheet, nullptr};

  if (!nsContentUtils::HasNonEmptyAttr(this, kNameSpaceID_None,
                                       nsGkAtoms::rev) &&
      FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::rel,
                      strings, eIgnoreCase) != ATTR_VALUE_NO_MATCH)
    return;

  nsRefPtr<nsAsyncDOMEvent> event = new nsAsyncDOMEvent(this, aEventName, true,
                                                        true);
  
  
  event->PostDOMEvent();
}

nsresult
HTMLLinkElement::SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                         nsIAtom* aPrefix, const nsAString& aValue,
                         bool aNotify)
{
  nsresult rv = nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix,
                                              aValue, aNotify);

  
  
  
  
  
  if (aName == nsGkAtoms::href && kNameSpaceID_None == aNameSpaceID) {
    Link::ResetLinkState(!!aNotify, true);
  }

  if (NS_SUCCEEDED(rv) && aNameSpaceID == kNameSpaceID_None &&
      (aName == nsGkAtoms::href ||
       aName == nsGkAtoms::rel ||
       aName == nsGkAtoms::title ||
       aName == nsGkAtoms::media ||
       aName == nsGkAtoms::type)) {
    bool dropSheet = false;
    if (aName == nsGkAtoms::rel && GetSheet()) {
      uint32_t linkTypes = nsStyleLinkElement::ParseLinkTypes(aValue);
      dropSheet = !(linkTypes & STYLESHEET);          
    }
    
    UpdateStyleSheetInternal(nullptr,
                             dropSheet ||
                             (aName == nsGkAtoms::title ||
                              aName == nsGkAtoms::media ||
                              aName == nsGkAtoms::type));
  }

  return rv;
}

nsresult
HTMLLinkElement::UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                           bool aNotify)
{
  nsresult rv = nsGenericHTMLElement::UnsetAttr(aNameSpaceID, aAttribute,
                                                aNotify);
  
  
  if (NS_SUCCEEDED(rv) && aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::href ||
       aAttribute == nsGkAtoms::rel ||
       aAttribute == nsGkAtoms::title ||
       aAttribute == nsGkAtoms::media ||
       aAttribute == nsGkAtoms::type)) {
    UpdateStyleSheetInternal(nullptr, true);
  }

  
  
  
  
  
  if (aAttribute == nsGkAtoms::href && kNameSpaceID_None == aNameSpaceID) {
    Link::ResetLinkState(!!aNotify, false);
  }

  return rv;
}

nsresult
HTMLLinkElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  return PreHandleEventForAnchors(aVisitor);
}

nsresult
HTMLLinkElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  return PostHandleEventForAnchors(aVisitor);
}

bool
HTMLLinkElement::IsLink(nsIURI** aURI) const
{
  return IsHTMLLink(aURI);
}

void
HTMLLinkElement::GetLinkTarget(nsAString& aTarget)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::target, aTarget);
  if (aTarget.IsEmpty()) {
    GetBaseTarget(aTarget);
  }
}

nsLinkState
HTMLLinkElement::GetLinkState() const
{
  return Link::GetLinkState();
}

already_AddRefed<nsIURI>
HTMLLinkElement::GetHrefURI() const
{
  return GetHrefURIForAnchors();
}

already_AddRefed<nsIURI>
HTMLLinkElement::GetStyleSheetURL(bool* aIsInline)
{
  *aIsInline = false;
  nsAutoString href;
  GetAttr(kNameSpaceID_None, nsGkAtoms::href, href);
  if (href.IsEmpty()) {
    return nullptr;
  }
  return Link::GetURI();
}

void
HTMLLinkElement::GetStyleSheetInfo(nsAString& aTitle,
                                   nsAString& aType,
                                   nsAString& aMedia,
                                   bool* aIsScoped,
                                   bool* aIsAlternate)
{
  aTitle.Truncate();
  aType.Truncate();
  aMedia.Truncate();
  *aIsScoped = false;
  *aIsAlternate = false;

  nsAutoString rel;
  GetAttr(kNameSpaceID_None, nsGkAtoms::rel, rel);
  uint32_t linkTypes = nsStyleLinkElement::ParseLinkTypes(rel);
  
  if (!(linkTypes & STYLESHEET)) {
    return;
  }

  nsAutoString title;
  GetAttr(kNameSpaceID_None, nsGkAtoms::title, title);
  title.CompressWhitespace();
  aTitle.Assign(title);

  
  if (linkTypes & ALTERNATE) {
    if (aTitle.IsEmpty()) { 
      return;
    } else {
      *aIsAlternate = true;
    }
  }

  GetAttr(kNameSpaceID_None, nsGkAtoms::media, aMedia);
  
  
  nsContentUtils::ASCIIToLower(aMedia);

  nsAutoString mimeType;
  nsAutoString notUsed;
  GetAttr(kNameSpaceID_None, nsGkAtoms::type, aType);
  nsContentUtils::SplitMimeType(aType, mimeType, notUsed);
  if (!mimeType.IsEmpty() && !mimeType.LowerCaseEqualsLiteral("text/css")) {
    return;
  }

  
  
  aType.AssignLiteral("text/css");

  return;
}

CORSMode
HTMLLinkElement::GetCORSMode() const
{
  return AttrValueToCORSMode(GetParsedAttr(nsGkAtoms::crossorigin)); 
}

nsEventStates
HTMLLinkElement::IntrinsicState() const
{
  return Link::LinkState() | nsGenericHTMLElement::IntrinsicState();
}

size_t
HTMLLinkElement::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  return nsGenericHTMLElement::SizeOfExcludingThis(aMallocSizeOf) +
         Link::SizeOfExcludingThis(aMallocSizeOf);
}

JSObject*
HTMLLinkElement::WrapNode(JSContext* aCx, JSObject* aScope)
{
  return HTMLLinkElementBinding::Wrap(aCx, aScope, this);
}

} 
} 
