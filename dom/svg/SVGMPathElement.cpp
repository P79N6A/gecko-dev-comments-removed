




#include "mozilla/ArrayUtils.h"

#include "mozilla/dom/SVGMPathElement.h"
#include "nsDebug.h"
#include "mozilla/dom/SVGAnimateMotionElement.h"
#include "mozilla/dom/SVGPathElement.h"
#include "nsContentUtils.h"
#include "mozilla/dom/SVGMPathElementBinding.h"
#include "nsIURI.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(MPath)

namespace mozilla {
namespace dom {

JSObject*
SVGMPathElement::WrapNode(JSContext *aCx)
{
  return SVGMPathElementBinding::Wrap(aCx, this);
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
  NS_INTERFACE_TABLE_INHERITED(SVGMPathElement, nsIDOMNode, nsIDOMElement,
                               nsIDOMSVGElement,
                               nsIMutationObserver)
NS_INTERFACE_TABLE_TAIL_INHERITING(SVGMPathElementBase)


SVGMPathElement::SVGMPathElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGMPathElementBase(aNodeInfo),
    mHrefTarget(MOZ_THIS_IN_INITIALIZER_LIST())
{
}

SVGMPathElement::~SVGMPathElement()
{
  UnlinkHrefTarget(false);
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGMPathElement)

already_AddRefed<SVGAnimatedString>
SVGMPathElement::Href()
{
  return mStringAttributes[HREF].ToDOMAnimatedString(this);
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




SVGPathElement*
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
    return static_cast<SVGPathElement*>(genericTarget);
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

    SVGAnimateMotionElement* animateMotionParent =
      static_cast<SVGAnimateMotionElement*>(aParent);

    animateMotionParent->MpathChanged();
    AnimationNeedsResample();
  }
}

} 
} 

