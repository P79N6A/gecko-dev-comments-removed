





#include "nsGkAtoms.h"
#include "nsNetUtil.h"
#include "nsContentUtils.h"
#include "mozilla/dom/SVGScriptElement.h"
#include "mozilla/dom/SVGScriptElementBinding.h"

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




NS_IMPL_ISUPPORTS_INHERITED6(SVGScriptElement, SVGScriptElementBase,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement,
                             nsIScriptLoaderObserver,
                             nsIScriptElement, nsIMutationObserver)




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
SVGScriptElement::GetCrossOrigin(nsAString & aOrigin)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::crossorigin, aOrigin);
}

void
SVGScriptElement::SetCrossOrigin(const nsAString & aOrigin, ErrorResult& rv)
{
  rv = SetAttr(kNameSpaceID_None, nsGkAtoms::crossorigin, aOrigin, true);
}

already_AddRefed<nsIDOMSVGAnimatedString>
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

