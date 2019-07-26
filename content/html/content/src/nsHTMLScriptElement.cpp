




#include "nsIDOMHTMLScriptElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
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

using namespace mozilla;
using namespace mozilla::dom;

class nsHTMLScriptElement : public nsGenericHTMLElement,
                            public nsIDOMHTMLScriptElement,
                            public nsScriptElement
{
public:
  using nsGenericElement::GetText;
  using nsGenericElement::SetText;

  nsHTMLScriptElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                      FromParser aFromParser);
  virtual ~nsHTMLScriptElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT_BASIC(nsGenericHTMLElement::)
  NS_IMETHOD Focus() {
    return nsGenericHTMLElement::Focus();
  }
  NS_IMETHOD GetInnerHTML(nsAString& aInnerHTML);
  NS_IMETHOD SetInnerHTML(const nsAString& aInnerHTML);

  
  NS_DECL_NSIDOMHTMLSCRIPTELEMENT

  
  virtual void GetScriptType(nsAString& type);
  virtual void GetScriptText(nsAString& text);
  virtual void GetScriptCharset(nsAString& charset);
  virtual void FreezeUriAsyncDefer();
  virtual CORSMode GetCORSMode() const;

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify);

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  
  virtual bool HasScriptContent();
};


NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(Script)


nsHTMLScriptElement::nsHTMLScriptElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                                         FromParser aFromParser)
  : nsGenericHTMLElement(aNodeInfo)
  , nsScriptElement(aFromParser)
{
  AddMutationObserver(this);
}

nsHTMLScriptElement::~nsHTMLScriptElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLScriptElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLScriptElement, nsGenericElement)

DOMCI_NODE_DATA(HTMLScriptElement, nsHTMLScriptElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLScriptElement)
  NS_HTML_CONTENT_INTERFACE_TABLE4(nsHTMLScriptElement,
                                   nsIDOMHTMLScriptElement,
                                   nsIScriptLoaderObserver,
                                   nsIScriptElement,
                                   nsIMutationObserver)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLScriptElement,
                                               nsGenericHTMLElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(HTMLScriptElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


nsresult
nsHTMLScriptElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
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
nsHTMLScriptElement::ParseAttribute(int32_t aNamespaceID,
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
nsHTMLScriptElement::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nullptr;

  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  nsHTMLScriptElement* it =
    new nsHTMLScriptElement(ni.forget(), NOT_FROM_PARSER);

  nsCOMPtr<nsINode> kungFuDeathGrip = it;
  nsresult rv = const_cast<nsHTMLScriptElement*>(this)->CopyInnerTo(it);
  NS_ENSURE_SUCCESS(rv, rv);

  
  it->mAlreadyStarted = mAlreadyStarted;
  it->mLineNumber = mLineNumber;
  it->mMalformed = mMalformed;

  kungFuDeathGrip.swap(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLScriptElement::GetText(nsAString& aValue)
{
  nsContentUtils::GetNodeTextContent(this, false, aValue);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLScriptElement::SetText(const nsAString& aValue)
{
  return nsContentUtils::SetNodeTextContent(this, aValue, true);
}


NS_IMPL_STRING_ATTR(nsHTMLScriptElement, Charset, charset)
NS_IMPL_BOOL_ATTR(nsHTMLScriptElement, Defer, defer)
NS_IMPL_URI_ATTR(nsHTMLScriptElement, Src, src)
NS_IMPL_STRING_ATTR(nsHTMLScriptElement, Type, type)
NS_IMPL_STRING_ATTR(nsHTMLScriptElement, HtmlFor, _for)
NS_IMPL_STRING_ATTR(nsHTMLScriptElement, Event, event)
NS_IMPL_STRING_ATTR(nsHTMLScriptElement, CrossOrigin, crossorigin)

nsresult
nsHTMLScriptElement::GetAsync(bool* aValue)
{
  *aValue = mForceAsync || GetBoolAttr(nsGkAtoms::async);
  return NS_OK;
}

nsresult
nsHTMLScriptElement::SetAsync(bool aValue)
{
  mForceAsync = false;
  return SetBoolAttr(nsGkAtoms::async, aValue);
}

nsresult
nsHTMLScriptElement::AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                  const nsAttrValue* aValue, bool aNotify)
{
  if (nsGkAtoms::async == aName && kNameSpaceID_None == aNamespaceID) {
    mForceAsync = false;
  }
  return nsGenericHTMLElement::AfterSetAttr(aNamespaceID, aName, aValue,
                                            aNotify);
}

nsresult
nsHTMLScriptElement::GetInnerHTML(nsAString& aInnerHTML)
{
  nsContentUtils::GetNodeTextContent(this, false, aInnerHTML);
  return NS_OK;
}

nsresult
nsHTMLScriptElement::SetInnerHTML(const nsAString& aInnerHTML)
{
  return nsContentUtils::SetNodeTextContent(this, aInnerHTML, true);
}




void
nsHTMLScriptElement::GetScriptType(nsAString& type)
{
  GetType(type);
}

void
nsHTMLScriptElement::GetScriptText(nsAString& text)
{
  GetText(text);
}

void
nsHTMLScriptElement::GetScriptCharset(nsAString& charset)
{
  GetCharset(charset);
}

void
nsHTMLScriptElement::FreezeUriAsyncDefer()
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
nsHTMLScriptElement::GetCORSMode() const
{
  return AttrValueToCORSMode(GetParsedAttr(nsGkAtoms::crossorigin));
}

bool
nsHTMLScriptElement::HasScriptContent()
{
  return (mFrozen ? mExternal : HasAttr(kNameSpaceID_None, nsGkAtoms::src)) ||
         nsContentUtils::HasNonEmptyTextContent(this);
}
