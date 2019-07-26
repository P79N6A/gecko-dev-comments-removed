





#include "nsIDOMHTMLScriptElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsIDocument.h"
#include "nsScriptElement.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsContentUtils.h"
#include "nsUnicharUtils.h"  
#include "jsapi.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIXPConnect.h"
#include "nsServiceManagerUtils.h"
#include "nsError.h"
#include "nsIArray.h"
#include "nsTArray.h"
#include "nsDOMJSUtils.h"
#include "mozilla/dom/HTMLScriptElement.h"

NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(Script)

DOMCI_NODE_DATA(HTMLScriptElement, mozilla::dom::HTMLScriptElement)

namespace mozilla {
namespace dom {

HTMLScriptElement::HTMLScriptElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                                     FromParser aFromParser)
  : nsGenericHTMLElement(aNodeInfo)
  , nsScriptElement(aFromParser)
{
  AddMutationObserver(this);
}

HTMLScriptElement::~HTMLScriptElement()
{
}


NS_IMPL_ADDREF_INHERITED(HTMLScriptElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLScriptElement, Element)


NS_INTERFACE_TABLE_HEAD(HTMLScriptElement)
  NS_HTML_CONTENT_INTERFACE_TABLE4(HTMLScriptElement,
                                   nsIDOMHTMLScriptElement,
                                   nsIScriptLoaderObserver,
                                   nsIScriptElement,
                                   nsIMutationObserver)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLScriptElement,
                                               nsGenericHTMLElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(HTMLScriptElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


nsresult
HTMLScriptElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDocument) {
    MaybeProcessScript();
  }

  return NS_OK;
}

bool
HTMLScriptElement::ParseAttribute(int32_t aNamespaceID,
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

nsresult
HTMLScriptElement::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nullptr;

  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  HTMLScriptElement* it =
    new HTMLScriptElement(ni.forget(), NOT_FROM_PARSER);

  nsCOMPtr<nsINode> kungFuDeathGrip = it;
  nsresult rv = const_cast<HTMLScriptElement*>(this)->CopyInnerTo(it);
  NS_ENSURE_SUCCESS(rv, rv);

  
  it->mAlreadyStarted = mAlreadyStarted;
  it->mLineNumber = mLineNumber;
  it->mMalformed = mMalformed;

  kungFuDeathGrip.swap(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
HTMLScriptElement::GetText(nsAString& aValue)
{
  nsContentUtils::GetNodeTextContent(this, false, aValue);
  return NS_OK;
}

NS_IMETHODIMP
HTMLScriptElement::SetText(const nsAString& aValue)
{
  return nsContentUtils::SetNodeTextContent(this, aValue, true);
}


NS_IMPL_STRING_ATTR(HTMLScriptElement, Charset, charset)
NS_IMPL_BOOL_ATTR(HTMLScriptElement, Defer, defer)
NS_IMPL_URI_ATTR(HTMLScriptElement, Src, src)
NS_IMPL_STRING_ATTR(HTMLScriptElement, Type, type)
NS_IMPL_STRING_ATTR(HTMLScriptElement, HtmlFor, _for)
NS_IMPL_STRING_ATTR(HTMLScriptElement, Event, event)
NS_IMPL_STRING_ATTR(HTMLScriptElement, CrossOrigin, crossorigin)

nsresult
HTMLScriptElement::GetAsync(bool* aValue)
{
  *aValue = mForceAsync || GetBoolAttr(nsGkAtoms::async);
  return NS_OK;
}

nsresult
HTMLScriptElement::SetAsync(bool aValue)
{
  mForceAsync = false;
  return SetBoolAttr(nsGkAtoms::async, aValue);
}

nsresult
HTMLScriptElement::AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify)
{
  if (nsGkAtoms::async == aName && kNameSpaceID_None == aNamespaceID) {
    mForceAsync = false;
  }
  return nsGenericHTMLElement::AfterSetAttr(aNamespaceID, aName, aValue,
                                            aNotify);
}

void
HTMLScriptElement::GetInnerHTML(nsAString& aInnerHTML, ErrorResult& aError)
{
  nsContentUtils::GetNodeTextContent(this, false, aInnerHTML);
}

void
HTMLScriptElement::SetInnerHTML(const nsAString& aInnerHTML,
                                ErrorResult& aError)
{
  aError = nsContentUtils::SetNodeTextContent(this, aInnerHTML, true);
}




void
HTMLScriptElement::GetScriptType(nsAString& type)
{
  GetType(type);
}

void
HTMLScriptElement::GetScriptText(nsAString& text)
{
  GetText(text);
}

void
HTMLScriptElement::GetScriptCharset(nsAString& charset)
{
  GetCharset(charset);
}

void
HTMLScriptElement::FreezeUriAsyncDefer()
{
  if (mFrozen) {
    return;
  }

  
  
  if (HasAttr(kNameSpaceID_None, nsGkAtoms::src)) {
    nsAutoString src;
    GetSrc(src);
    NS_NewURI(getter_AddRefs(mUri), src);
    
    mExternal = true;

    bool defer, async;
    GetAsync(&async);
    GetDefer(&defer);

    mDefer = !async && defer;
    mAsync = async;
  }

  mFrozen = true;
}

CORSMode
HTMLScriptElement::GetCORSMode() const
{
  return AttrValueToCORSMode(GetParsedAttr(nsGkAtoms::crossorigin));
}

bool
HTMLScriptElement::HasScriptContent()
{
  return (mFrozen ? mExternal : HasAttr(kNameSpaceID_None, nsGkAtoms::src)) ||
         nsContentUtils::HasNonEmptyTextContent(this);
}

} 
} 
