





#include "nsGkAtoms.h"
#include "nsNetUtil.h"
#include "nsContentUtils.h"
#include "mozilla/dom/SVGScriptElement.h"
#include "mozilla/dom/SVGScriptElementBinding.h"

DOMCI_NODE_DATA(SVGScriptElement, mozilla::dom::SVGScriptElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT_CHECK_PARSER(Script)

namespace mozilla {
namespace dom {

JSObject*
SVGScriptElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGScriptElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

nsSVGElement::StringInfo SVGScriptElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, false }
};




NS_IMPL_ADDREF_INHERITED(SVGScriptElement,SVGScriptElementBase)
NS_IMPL_RELEASE_INHERITED(SVGScriptElement,SVGScriptElementBase)

NS_INTERFACE_TABLE_HEAD(SVGScriptElement)
  NS_NODE_INTERFACE_TABLE8(SVGScriptElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGScriptElement,
                           nsIDOMSVGURIReference, nsIScriptLoaderObserver,
                           nsIScriptElement, nsIMutationObserver)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGScriptElement)
NS_INTERFACE_MAP_END_INHERITING(SVGScriptElementBase)




SVGScriptElement::SVGScriptElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                                   FromParser aFromParser)
  : SVGScriptElementBase(aNodeInfo)
  , nsScriptElement(aFromParser)
{
  SetIsDOMBinding();
  AddMutationObserver(this);
}




nsresult
SVGScriptElement::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nullptr;

  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  SVGScriptElement* it = new SVGScriptElement(ni.forget(), NOT_FROM_PARSER);

  nsCOMPtr<nsINode> kungFuDeathGrip = it;
  nsresult rv1 = it->Init();
  nsresult rv2 = const_cast<SVGScriptElement*>(this)->CopyInnerTo(it);
  NS_ENSURE_SUCCESS(rv1, rv1);
  NS_ENSURE_SUCCESS(rv2, rv2);

  
  it->mAlreadyStarted = mAlreadyStarted;
  it->mLineNumber = mLineNumber;
  it->mMalformed = mMalformed;

  kungFuDeathGrip.swap(*aResult);

  return NS_OK;
}





NS_IMPL_STRING_ATTR(SVGScriptElement, Type, type)

NS_IMPL_STRING_ATTR(SVGScriptElement, CrossOrigin, crossorigin)





NS_IMETHODIMP
SVGScriptElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  *aHref = Href().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedString>
SVGScriptElement::Href()
{
  nsCOMPtr<nsIDOMSVGAnimatedString> href;
  mStringAttributes[HREF].ToDOMAnimatedString(getter_AddRefs(href), this);
  return href.forget();
}




void
SVGScriptElement::GetScriptType(nsAString& type)
{
  GetType(type);
}

void
SVGScriptElement::GetScriptText(nsAString& text)
{
  nsContentUtils::GetNodeTextContent(this, false, text);
}

void
SVGScriptElement::GetScriptCharset(nsAString& charset)
{
  charset.Truncate();
}

void
SVGScriptElement::FreezeUriAsyncDefer()
{
  if (mFrozen) {
    return;
  }

  if (mStringAttributes[HREF].IsExplicitlySet()) {
    
    
    nsAutoString src;
    mStringAttributes[HREF].GetAnimValue(src, this);

    nsCOMPtr<nsIURI> baseURI = GetBaseURI();
    NS_NewURI(getter_AddRefs(mUri), src, nullptr, baseURI);
    
    mExternal = true;
  }
  
  mFrozen = true;
}




bool
SVGScriptElement::HasScriptContent()
{
  return (mFrozen ? mExternal : mStringAttributes[HREF].IsExplicitlySet()) ||
         nsContentUtils::HasNonEmptyTextContent(this);
}




nsSVGElement::StringAttributesInfo
SVGScriptElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}




nsresult
SVGScriptElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                             nsIContent* aBindingParent,
                             bool aCompileEventHandlers)
{
  nsresult rv = SVGScriptElementBase::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDocument) {
    MaybeProcessScript();
  }

  return NS_OK;
}

nsresult
SVGScriptElement::AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                               const nsAttrValue* aValue, bool aNotify)
{
  if (aNamespaceID == kNameSpaceID_XLink && aName == nsGkAtoms::href) {
    MaybeProcessScript();
  }
  return SVGScriptElementBase::AfterSetAttr(aNamespaceID, aName,
                                            aValue, aNotify);
}

bool
SVGScriptElement::ParseAttribute(int32_t aNamespaceID,
                                 nsIAtom* aAttribute,
                                 const nsAString& aValue,
                                 nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None &&
      aAttribute == nsGkAtoms::crossorigin) {
    ParseCORSValue(aValue, aResult);
    return true;
  }

  return SVGScriptElementBase::ParseAttribute(aNamespaceID, aAttribute,
                                              aValue, aResult);
}

CORSMode
SVGScriptElement::GetCORSMode() const
{
  return AttrValueToCORSMode(GetParsedAttr(nsGkAtoms::crossorigin));
}

} 
} 

