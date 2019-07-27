





#include "mozilla/dom/HTMLLinkElement.h"

#include "mozilla/AsyncEventDispatcher.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/EventStates.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/HTMLLinkElementBinding.h"
#include "nsContentUtils.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsDOMTokenList.h"
#include "nsIDocument.h"
#include "nsIDOMEvent.h"
#include "nsIDOMStyleSheet.h"
#include "nsINode.h"
#include "nsIStyleSheet.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsPIDOMWindow.h"
#include "nsReadableUtils.h"
#include "nsStyleConsts.h"
#include "nsUnicharUtils.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Link)

namespace mozilla {
namespace dom {

HTMLLinkElement::HTMLLinkElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
  , Link(MOZ_THIS_IN_INITIALIZER_LIST())
{
}

HTMLLinkElement::~HTMLLinkElement()
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(HTMLLinkElement)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(HTMLLinkElement,
                                                  nsGenericHTMLElement)
  tmp->nsStyleLinkElement::Traverse(cb);
  tmp->Link::Traverse(cb);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRelList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mImportLoader)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(HTMLLinkElement,
                                                nsGenericHTMLElement)
  tmp->nsStyleLinkElement::Unlink();
  tmp->Link::Unlink();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mRelList)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mImportLoader)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(HTMLLinkElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLLinkElement, Element)



NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(HTMLLinkElement)
  NS_INTERFACE_TABLE_INHERITED(HTMLLinkElement,
                               nsIDOMHTMLLinkElement,
                               nsIStyleSheetLinkingElement,
                               Link)
NS_INTERFACE_TABLE_TAIL_INHERITING(nsGenericHTMLElement)


NS_IMPL_ELEMENT_CLONE(HTMLLinkElement)

bool
HTMLLinkElement::Disabled()
{
  CSSStyleSheet* ss = GetSheet();
  return ss && ss->Disabled();
}

NS_IMETHODIMP
HTMLLinkElement::GetMozDisabled(bool* aDisabled)
{
  *aDisabled = Disabled();
  return NS_OK;
}

void
HTMLLinkElement::SetDisabled(bool aDisabled)
{
  CSSStyleSheet* ss = GetSheet();
  if (ss) {
    ss->SetDisabled(aDisabled);
  }
}

NS_IMETHODIMP
HTMLLinkElement::SetMozDisabled(bool aDisabled)
{
  SetDisabled(aDisabled);
  return NS_OK;
}


NS_IMPL_STRING_ATTR(HTMLLinkElement, Charset, charset)
NS_IMPL_URI_ATTR(HTMLLinkElement, Href, href)
NS_IMPL_STRING_ATTR(HTMLLinkElement, Hreflang, hreflang)
NS_IMPL_STRING_ATTR(HTMLLinkElement, Media, media)
NS_IMPL_STRING_ATTR(HTMLLinkElement, Rel, rel)
NS_IMPL_STRING_ATTR(HTMLLinkElement, Rev, rev)
NS_IMPL_STRING_ATTR(HTMLLinkElement, Target, target)
NS_IMPL_STRING_ATTR(HTMLLinkElement, Type, type)

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

  
  if (aDocument && !GetContainingShadow()) {
    aDocument->RegisterPendingLinkUpdate(this);
  }

  void (HTMLLinkElement::*update)() = &HTMLLinkElement::UpdateStyleSheetInternal;
  nsContentUtils::AddScriptRunner(NS_NewRunnableMethod(this, update));

  void (HTMLLinkElement::*updateImport)() = &HTMLLinkElement::UpdateImport;
  nsContentUtils::AddScriptRunner(NS_NewRunnableMethod(this, updateImport));

  CreateAndDispatchEvent(aDocument, NS_LITERAL_STRING("DOMLinkAdded"));

  return rv;
}

void
HTMLLinkElement::LinkAdded()
{
  CreateAndDispatchEvent(OwnerDoc(), NS_LITERAL_STRING("DOMLinkAdded"));
}

void
HTMLLinkElement::LinkRemoved()
{
  CreateAndDispatchEvent(OwnerDoc(), NS_LITERAL_STRING("DOMLinkRemoved"));
}

void
HTMLLinkElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  
  
  Link::ResetLinkState(false, Link::ElementHasHref());

  
  
  nsCOMPtr<nsIDocument> oldDoc = GetCurrentDoc();

  
  
  ShadowRoot* oldShadowRoot = GetBindingParent() ?
    GetBindingParent()->GetShadowRoot() : nullptr;

  if (oldDoc && !oldShadowRoot) {
    oldDoc->UnregisterPendingLinkUpdate(this);
  }
  CreateAndDispatchEvent(oldDoc, NS_LITERAL_STRING("DOMLinkRemoved"));
  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);

  UpdateStyleSheetInternal(oldDoc, oldShadowRoot);
  UpdateImport();
}

bool
HTMLLinkElement::ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::crossorigin) {
      ParseCORSValue(aValue, aResult);
      return true;
    }

    if (aAttribute == nsGkAtoms::sizes) {
      aResult.ParseAtomArray(aValue);
      return true;
    }
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

  nsRefPtr<AsyncEventDispatcher> asyncDispatcher =
    new AsyncEventDispatcher(this, aEventName, true, true);
  
  
  asyncDispatcher->PostDOMEvent();
}

void
HTMLLinkElement::UpdateImport()
{
  
  nsCOMPtr<nsIDocument> doc = GetCurrentDoc();
  if (!doc) {
    
    
    
    if (mImportLoader) {
      mImportLoader->RemoveLinkElement(this);
      mImportLoader = nullptr;
    }
    return;
  }

  
  
  if (!doc->IsMasterDocument()) {
    nsContentUtils::LogSimpleConsoleError(
      NS_LITERAL_STRING("Nested imports are not supported yet"),
      "Imports");
    return;
  }

  
  nsAutoString rel;
  GetAttr(kNameSpaceID_None, nsGkAtoms::rel, rel);
  uint32_t linkTypes = nsStyleLinkElement::ParseLinkTypes(rel, NodePrincipal());
  if (!(linkTypes & eHTMLIMPORT)) {
    mImportLoader = nullptr;
    return;
  }

  nsCOMPtr<nsIURI> uri = GetHrefURI();
  if (!uri) {
    mImportLoader = nullptr;
    return;
  }

  if (!nsStyleLinkElement::IsImportEnabled(NodePrincipal())) {
    
    return;
  }

  nsRefPtr<ImportManager> manager = doc->ImportManager();
  MOZ_ASSERT(manager, "ImportManager should be created lazily when needed");

  {
    
    
    nsAutoScriptBlocker scriptBlocker;
    
    mImportLoader = manager->Get(uri, this, doc);
  }
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
    CreateAndDispatchEvent(OwnerDoc(), NS_LITERAL_STRING("DOMLinkChanged"));
  }

  if (NS_SUCCEEDED(rv) && aNameSpaceID == kNameSpaceID_None &&
      (aName == nsGkAtoms::href ||
       aName == nsGkAtoms::rel ||
       aName == nsGkAtoms::title ||
       aName == nsGkAtoms::media ||
       aName == nsGkAtoms::type)) {
    bool dropSheet = false;
    if (aName == nsGkAtoms::rel) {
      uint32_t linkTypes = nsStyleLinkElement::ParseLinkTypes(aValue,
                                                              NodePrincipal());
      if (GetSheet()) {
        dropSheet = !(linkTypes & nsStyleLinkElement::eSTYLESHEET);
      } else if (linkTypes & eHTMLIMPORT) {
        UpdateImport();
      }
    }

    if (aName == nsGkAtoms::href) {
      UpdateImport();
    }
    
    UpdateStyleSheetInternal(nullptr, nullptr,
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
  
  
  if (NS_SUCCEEDED(rv) && aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::href ||
        aAttribute == nsGkAtoms::rel ||
        aAttribute == nsGkAtoms::title ||
        aAttribute == nsGkAtoms::media ||
        aAttribute == nsGkAtoms::type) {
      UpdateStyleSheetInternal(nullptr, nullptr, true);
    }
    if (aAttribute == nsGkAtoms::href ||
        aAttribute == nsGkAtoms::rel) {
      UpdateImport();
    }
  }

  
  
  
  
  
  if (aAttribute == nsGkAtoms::href && kNameSpaceID_None == aNameSpaceID) {
    Link::ResetLinkState(!!aNotify, false);
    CreateAndDispatchEvent(OwnerDoc(), NS_LITERAL_STRING("DOMLinkChanged"));
  }

  return rv;
}

nsresult
HTMLLinkElement::PreHandleEvent(EventChainPreVisitor& aVisitor)
{
  return PreHandleEventForAnchors(aVisitor);
}

nsresult
HTMLLinkElement::PostHandleEvent(EventChainPostVisitor& aVisitor)
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

nsDOMTokenList* 
HTMLLinkElement::RelList()
{
  if (!mRelList) {
    mRelList = new nsDOMTokenList(this, nsGkAtoms::rel);
  }
  return mRelList;
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
  nsCOMPtr<nsIURI> uri = Link::GetURI();
  return uri.forget();
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
  uint32_t linkTypes = nsStyleLinkElement::ParseLinkTypes(rel, NodePrincipal());
  
  if (!(linkTypes & nsStyleLinkElement::eSTYLESHEET)) {
    return;
  }

  nsAutoString title;
  GetAttr(kNameSpaceID_None, nsGkAtoms::title, title);
  title.CompressWhitespace();
  aTitle.Assign(title);

  
  if (linkTypes & nsStyleLinkElement::eALTERNATE) {
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

EventStates
HTMLLinkElement::IntrinsicState() const
{
  return Link::LinkState() | nsGenericHTMLElement::IntrinsicState();
}

size_t
HTMLLinkElement::SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  return nsGenericHTMLElement::SizeOfExcludingThis(aMallocSizeOf) +
         Link::SizeOfExcludingThis(aMallocSizeOf);
}

JSObject*
HTMLLinkElement::WrapNode(JSContext* aCx)
{
  return HTMLLinkElementBinding::Wrap(aCx, this);
}

already_AddRefed<nsIDocument>
HTMLLinkElement::GetImport()
{
  return mImportLoader ? mImportLoader->GetImport() : nullptr;
}

} 
} 
