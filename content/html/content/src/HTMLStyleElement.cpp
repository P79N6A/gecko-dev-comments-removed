



#include "mozilla/dom/HTMLStyleElement.h"
#include "mozilla/dom/HTMLStyleElementBinding.h"
#include "nsIDOMLinkStyle.h"
#include "nsIDOMEventTarget.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsIDOMStyleSheet.h"
#include "nsIStyleSheet.h"
#include "nsNetUtil.h"
#include "nsIDocument.h"
#include "nsUnicharUtils.h"
#include "nsThreadUtils.h"
#include "nsContentUtils.h"
#include "nsStubMutationObserver.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Style)

DOMCI_NODE_DATA(HTMLStyleElement, mozilla::dom::HTMLStyleElement)

namespace mozilla {
namespace dom {

HTMLStyleElement::HTMLStyleElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
  AddMutationObserver(this);
  SetIsDOMBinding();
}

HTMLStyleElement::~HTMLStyleElement()
{
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(HTMLStyleElement,
                                                  nsGenericHTMLElement)
  tmp->nsStyleLinkElement::Traverse(cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(HTMLStyleElement,
                                                nsGenericHTMLElement)
  tmp->nsStyleLinkElement::Unlink();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(HTMLStyleElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLStyleElement, Element)



NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(HTMLStyleElement)
  NS_HTML_CONTENT_INTERFACE_TABLE4(HTMLStyleElement,
                                   nsIDOMHTMLStyleElement,
                                   nsIDOMLinkStyle,
                                   nsIStyleSheetLinkingElement,
                                   nsIMutationObserver)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLStyleElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLStyleElement)


NS_IMPL_ELEMENT_CLONE(HTMLStyleElement)


NS_IMETHODIMP
HTMLStyleElement::GetDisabled(bool* aDisabled)
{
  NS_ENSURE_ARG_POINTER(aDisabled);

  *aDisabled = Disabled();
  return NS_OK;
}

bool
HTMLStyleElement::Disabled()
{
  nsCOMPtr<nsIDOMStyleSheet> ss = do_QueryInterface(GetSheet());
  if (!ss) {
    return false;
  }

  bool disabled = false;
  ss->GetDisabled(&disabled);

  return disabled;
}

NS_IMETHODIMP
HTMLStyleElement::SetDisabled(bool aDisabled)
{
  ErrorResult error;
  SetDisabled(aDisabled, error);
  return error.ErrorCode();
}

void
HTMLStyleElement::SetDisabled(bool aDisabled, ErrorResult& aError)
{
  nsCOMPtr<nsIDOMStyleSheet> ss = do_QueryInterface(GetSheet());
  if (!ss) {
    return;
  }

  aError.Throw(ss->SetDisabled(aDisabled));
}

NS_IMPL_STRING_ATTR(HTMLStyleElement, Media, media)
NS_IMPL_BOOL_ATTR(HTMLStyleElement, Scoped, scoped)
NS_IMPL_STRING_ATTR(HTMLStyleElement, Type, type)

void
HTMLStyleElement::CharacterDataChanged(nsIDocument* aDocument,
                                       nsIContent* aContent,
                                       CharacterDataChangeInfo* aInfo)
{
  ContentChanged(aContent);
}

void
HTMLStyleElement::ContentAppended(nsIDocument* aDocument,
                                  nsIContent* aContainer,
                                  nsIContent* aFirstNewContent,
                                  int32_t aNewIndexInContainer)
{
  ContentChanged(aContainer);
}

void
HTMLStyleElement::ContentInserted(nsIDocument* aDocument,
                                  nsIContent* aContainer,
                                  nsIContent* aChild,
                                  int32_t aIndexInContainer)
{
  ContentChanged(aChild);
}

void
HTMLStyleElement::ContentRemoved(nsIDocument* aDocument,
                                 nsIContent* aContainer,
                                 nsIContent* aChild,
                                 int32_t aIndexInContainer,
                                 nsIContent* aPreviousSibling)
{
  ContentChanged(aChild);
}

void
HTMLStyleElement::ContentChanged(nsIContent* aContent)
{
  if (nsContentUtils::IsInSameAnonymousTree(this, aContent)) {
    UpdateStyleSheetInternal(nullptr);
  }
}

nsresult
HTMLStyleElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                             nsIContent* aBindingParent,
                             bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  void (HTMLStyleElement::*update)() = &HTMLStyleElement::UpdateStyleSheetInternal;
  nsContentUtils::AddScriptRunner(NS_NewRunnableMethod(this, update));

  return rv;  
}

void
HTMLStyleElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  nsCOMPtr<nsIDocument> oldDoc = GetCurrentDoc();

  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
  UpdateStyleSheetInternal(oldDoc);
}

nsresult
HTMLStyleElement::SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                          nsIAtom* aPrefix, const nsAString& aValue,
                          bool aNotify)
{
  nsresult rv = nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix,
                                              aValue, aNotify);
  if (NS_SUCCEEDED(rv) && aNameSpaceID == kNameSpaceID_None) {
    if (aName == nsGkAtoms::title ||
        aName == nsGkAtoms::media ||
        aName == nsGkAtoms::type) {
      UpdateStyleSheetInternal(nullptr, true);
    } else if (aName == nsGkAtoms::scoped) {
      UpdateStyleSheetScopedness(true);
    }
  }

  return rv;
}

nsresult
HTMLStyleElement::UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                            bool aNotify)
{
  nsresult rv = nsGenericHTMLElement::UnsetAttr(aNameSpaceID, aAttribute,
                                                aNotify);
  if (NS_SUCCEEDED(rv) && aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::title ||
        aAttribute == nsGkAtoms::media ||
        aAttribute == nsGkAtoms::type) {
      UpdateStyleSheetInternal(nullptr, true);
    } else if (aAttribute == nsGkAtoms::scoped) {
      UpdateStyleSheetScopedness(false);
    }
  }

  return rv;
}

void
HTMLStyleElement::GetInnerHTML(nsAString& aInnerHTML, ErrorResult& aError)
{
  nsContentUtils::GetNodeTextContent(this, false, aInnerHTML);
}

void
HTMLStyleElement::SetInnerHTML(const nsAString& aInnerHTML,
                               ErrorResult& aError)
{
  SetEnableUpdates(false);

  aError = nsContentUtils::SetNodeTextContent(this, aInnerHTML, true);

  SetEnableUpdates(true);

  UpdateStyleSheetInternal(nullptr);
}

already_AddRefed<nsIURI>
HTMLStyleElement::GetStyleSheetURL(bool* aIsInline)
{
  *aIsInline = true;
  return nullptr;
}

void
HTMLStyleElement::GetStyleSheetInfo(nsAString& aTitle,
                                    nsAString& aType,
                                    nsAString& aMedia,
                                    bool* aIsScoped,
                                    bool* aIsAlternate)
{
  aTitle.Truncate();
  aType.Truncate();
  aMedia.Truncate();
  *aIsAlternate = false;

  nsAutoString title;
  GetAttr(kNameSpaceID_None, nsGkAtoms::title, title);
  title.CompressWhitespace();
  aTitle.Assign(title);

  GetAttr(kNameSpaceID_None, nsGkAtoms::media, aMedia);
  
  
  nsContentUtils::ASCIIToLower(aMedia);

  GetAttr(kNameSpaceID_None, nsGkAtoms::type, aType);

  *aIsScoped = HasAttr(kNameSpaceID_None, nsGkAtoms::scoped);

  nsAutoString mimeType;
  nsAutoString notUsed;
  nsContentUtils::SplitMimeType(aType, mimeType, notUsed);
  if (!mimeType.IsEmpty() && !mimeType.LowerCaseEqualsLiteral("text/css")) {
    return;
  }

  
  
  aType.AssignLiteral("text/css");
}

JSObject*
HTMLStyleElement::WrapNode(JSContext *aCx, JSObject *aScope,
                           bool *aTriedToWrap)
{
  return HTMLStyleElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

} 
} 

