





#include "nsGkAtoms.h"
#include "nsNetUtil.h"
#include "nsContentUtils.h"
#include "mozilla/dom/SVGScriptElement.h"
#include "mozilla/dom/SVGScriptElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT_CHECK_PARSER(Script)

namespace mozilla {
namespace dom {

JSObject*
SVGScriptElement::WrapNode(JSContext *aCx)
{
  return SVGScriptElementBinding::Wrap(aCx, this);
}

nsSVGElement::StringInfo SVGScriptElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, false }
};




NS_IMPL_ISUPPORTS_INHERITED(SVGScriptElement, SVGScriptElementBase,
                            nsIDOMNode, nsIDOMElement,
                            nsIDOMSVGElement,
                            nsIScriptLoaderObserver,
                            nsIScriptElement, nsIMutationObserver)




SVGScriptElement::SVGScriptElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
                                   FromParser aFromParser)
  : SVGScriptElementBase(aNodeInfo)
  , nsScriptElement(aFromParser)
{
  AddMutationObserver(this);
}

SVGScriptElement::~SVGScriptElement()
{
}




nsresult
SVGScriptElement::Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nullptr;

  already_AddRefed<mozilla::dom::NodeInfo> ni = nsRefPtr<mozilla::dom::NodeInfo>(aNodeInfo).forget();
  SVGScriptElement* it = new SVGScriptElement(ni, NOT_FROM_PARSER);

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


void
SVGScriptElement::GetType(nsAString & aType)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::type, aType);
}

void
SVGScriptElement::SetType(const nsAString & aType, ErrorResult& rv)
{
  rv = SetAttr(kNameSpaceID_None, nsGkAtoms::type, aType, true);
}

void
SVGScriptElement::GetCrossOrigin(nsAString & aCrossOrigin)
{
  
  
  
  GetEnumAttr(nsGkAtoms::crossorigin, nullptr, aCrossOrigin);
}

void
SVGScriptElement::SetCrossOrigin(const nsAString & aCrossOrigin,
                                 ErrorResult& aError)
{
  SetOrRemoveNullableStringAttr(nsGkAtoms::crossorigin, aCrossOrigin, aError);
}

already_AddRefed<SVGAnimatedString>
SVGScriptElement::Href()
{
  return mStringAttributes[HREF].ToDOMAnimatedString(this);
}




void
SVGScriptElement::GetScriptType(nsAString& type)
{
  GetType(type);
}

void
SVGScriptElement::GetScriptText(nsAString& text)
{
  if (!nsContentUtils::GetNodeTextContent(this, false, text)) {
    NS_RUNTIMEABORT("OOM");
  }
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

