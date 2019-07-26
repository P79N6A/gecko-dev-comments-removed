




#include "mozilla/Util.h"

#include "mozilla/dom/SVGMPathElement.h"
#include "nsAutoPtr.h"
#include "nsDebug.h"
#include "nsSVGPathElement.h"
#include "nsSVGAnimateMotionElement.h"
#include "nsContentUtils.h"
#include "mozilla/dom/SVGMPathElementBinding.h"

DOMCI_NODE_DATA(SVGMpathElement, mozilla::dom::SVGMPathElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(MPath)

namespace mozilla {
namespace dom {

JSObject*
SVGMPathElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGMPathElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

nsSVGElement::StringInfo SVGMPathElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, false }
};


NS_IMPL_CYCLE_COLLECTION_CLASS(SVGMPathElement)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(SVGMPathElement,
                                                SVGMPathElementBase)
  tmp->UnlinkHrefTarget(false);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(SVGMPathElement,
                                                  SVGMPathElementBase)
  tmp->mHrefTarget.Traverse(&cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END




NS_IMPL_ADDREF_INHERITED(SVGMPathElement,SVGMPathElementBase)
NS_IMPL_RELEASE_INHERITED(SVGMPathElement,SVGMPathElementBase)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(SVGMPathElement)
  NS_NODE_INTERFACE_TABLE6(SVGMPathElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,  nsIDOMSVGURIReference,
                           nsIDOMSVGMpathElement, nsIMutationObserver)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGMpathElement)
NS_INTERFACE_MAP_END_INHERITING(SVGMPathElementBase)


#ifdef _MSC_VER



#pragma warning(push)
#pragma warning(disable:4355)
#endif
SVGMPathElement::SVGMPathElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGMPathElementBase(aNodeInfo),
    mHrefTarget(this)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
{
  SetIsDOMBinding();
}

SVGMPathElement::~SVGMPathElement()
{
  UnlinkHrefTarget(false);
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGMPathElement)





already_AddRefed<nsIDOMSVGAnimatedString>
SVGMPathElement::Href()
{
  nsCOMPtr<nsIDOMSVGAnimatedString> href;
  mStringAttributes[HREF].ToDOMAnimatedString(getter_AddRefs(href), this);
  return href.forget();
}

NS_IMETHODIMP
SVGMPathElement::GetHref(nsIDOMSVGAnimatedString** aHref)
{
  *aHref = Href().get();
  return NS_OK;
}




nsresult
SVGMPathElement::BindToTree(nsIDocument* aDocument,
                            nsIContent* aParent,
                            nsIContent* aBindingParent,
                            bool aCompileEventHandlers)
{
  NS_ABORT_IF_FALSE(!mHrefTarget.get(),
                    "Shouldn't have href-target yet "
                    "(or it should've been cleared)");
  nsresult rv = SVGMPathElementBase::BindToTree(aDocument, aParent,
                                                aBindingParent,
                                                aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv,rv);

  if (aDocument) {
    const nsAttrValue* hrefAttrValue =
      mAttrsAndChildren.GetAttr(nsGkAtoms::href, kNameSpaceID_XLink);
    if (hrefAttrValue) {
      UpdateHrefTarget(aParent, hrefAttrValue->GetStringValue());
    }
  }

  return NS_OK;
}

void
SVGMPathElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  UnlinkHrefTarget(true);
  SVGMPathElementBase::UnbindFromTree(aDeep, aNullParent);
}

bool
SVGMPathElement::ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult)
{
  bool returnVal =
    SVGMPathElementBase::ParseAttribute(aNamespaceID, aAttribute,
                                          aValue, aResult);
  if (aNamespaceID == kNameSpaceID_XLink &&
      aAttribute == nsGkAtoms::href &&
      IsInDoc()) {
    
    
    UpdateHrefTarget(GetParent(), aValue);
  }
  return returnVal;
}

nsresult
SVGMPathElement::UnsetAttr(int32_t aNamespaceID,
                           nsIAtom* aAttribute, bool aNotify)
{
  nsresult rv = SVGMPathElementBase::UnsetAttr(aNamespaceID, aAttribute,
                                               aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aNamespaceID == kNameSpaceID_XLink &&
      aAttribute == nsGkAtoms::href) {
    UnlinkHrefTarget(true);
  }

  return NS_OK;
}




nsSVGElement::StringAttributesInfo
SVGMPathElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}




void
SVGMPathElement::AttributeChanged(nsIDocument* aDocument,
                                  Element* aElement,
                                  int32_t aNameSpaceID,
                                  nsIAtom* aAttribute,
                                  int32_t aModType)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::d) {
      NotifyParentOfMpathChange(GetParent());
    }
  }
}




nsSVGPathElement*
SVGMPathElement::GetReferencedPath()
{
  if (!HasAttr(kNameSpaceID_XLink, nsGkAtoms::href)) {
    NS_ABORT_IF_FALSE(!mHrefTarget.get(),
                      "We shouldn't have an xlink:href target "
                      "if we don't have an xlink:href attribute");
    return nullptr;
  }

  nsIContent* genericTarget = mHrefTarget.get();
  if (genericTarget && genericTarget->IsSVG(nsGkAtoms::path)) {
    return static_cast<nsSVGPathElement*>(genericTarget);
  }
  return nullptr;
}




void
SVGMPathElement::UpdateHrefTarget(nsIContent* aParent,
                                  const nsAString& aHrefStr)
{
  nsCOMPtr<nsIURI> targetURI;
  nsCOMPtr<nsIURI> baseURI = GetBaseURI();
  nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI),
                                            aHrefStr, OwnerDoc(), baseURI);

  
  if (mHrefTarget.get()) {
    mHrefTarget.get()->RemoveMutationObserver(this);
  }

  if (aParent) {
    
    
    
    mHrefTarget.Reset(aParent, targetURI);
  } else {
    
    
    mHrefTarget.Unlink();
  }

  
  if (mHrefTarget.get()) {
    mHrefTarget.get()->AddMutationObserver(this);
  }

  NotifyParentOfMpathChange(aParent);
}

void
SVGMPathElement::UnlinkHrefTarget(bool aNotifyParent)
{
  
  if (mHrefTarget.get()) {
    mHrefTarget.get()->RemoveMutationObserver(this);
  }
  mHrefTarget.Unlink();

  if (aNotifyParent) {
    NotifyParentOfMpathChange(GetParent());
  }
}

void
SVGMPathElement::NotifyParentOfMpathChange(nsIContent* aParent)
{
  if (aParent && aParent->IsSVG(nsGkAtoms::animateMotion)) {

    nsSVGAnimateMotionElement* animateMotionParent =
      static_cast<nsSVGAnimateMotionElement*>(aParent);

    animateMotionParent->MpathChanged();
    AnimationNeedsResample();
  }
}

} 
} 

