




#include "mozilla/Util.h"

#include "nsSVGMpathElement.h"
#include "nsAutoPtr.h"
#include "nsDebug.h"
#include "nsSVGPathElement.h"
#include "nsSVGAnimateMotionElement.h"
#include "nsContentUtils.h"

using namespace mozilla;
using namespace mozilla::dom;

nsSVGElement::StringInfo nsSVGMpathElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, false }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Mpath)


NS_IMPL_CYCLE_COLLECTION_CLASS(nsSVGMpathElement)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsSVGMpathElement,
                                                nsSVGMpathElementBase)
  tmp->UnlinkHrefTarget(false);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsSVGMpathElement,
                                                  nsSVGMpathElementBase)
  tmp->mHrefTarget.Traverse(&cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END




NS_IMPL_ADDREF_INHERITED(nsSVGMpathElement,nsSVGMpathElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGMpathElement,nsSVGMpathElementBase)

DOMCI_NODE_DATA(SVGMpathElement, nsSVGMpathElement)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsSVGMpathElement)
  NS_NODE_INTERFACE_TABLE6(nsSVGMpathElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,  nsIDOMSVGURIReference,
                           nsIDOMSVGMpathElement, nsIMutationObserver)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGMpathElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGMpathElementBase)


#ifdef _MSC_VER



#pragma warning(push)
#pragma warning(disable:4355)
#endif
nsSVGMpathElement::nsSVGMpathElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGMpathElementBase(aNodeInfo),
    mHrefTarget(this)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
{
}

nsSVGMpathElement::~nsSVGMpathElement()
{
  UnlinkHrefTarget(false);
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGMpathElement)





NS_IMETHODIMP
nsSVGMpathElement::GetHref(nsIDOMSVGAnimatedString** aHref)
{
  return mStringAttributes[HREF].ToDOMAnimatedString(aHref, this);
}




nsresult
nsSVGMpathElement::BindToTree(nsIDocument* aDocument,
                              nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers)
{
  NS_ABORT_IF_FALSE(!mHrefTarget.get(),
                    "Shouldn't have href-target yet "
                    "(or it should've been cleared)");
  nsresult rv = nsSVGMpathElementBase::BindToTree(aDocument, aParent,
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
nsSVGMpathElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  UnlinkHrefTarget(true);
  nsSVGMpathElementBase::UnbindFromTree(aDeep, aNullParent);
}

bool
nsSVGMpathElement::ParseAttribute(int32_t aNamespaceID,
                                  nsIAtom* aAttribute,
                                  const nsAString& aValue,
                                  nsAttrValue& aResult)
{
  bool returnVal =
    nsSVGMpathElementBase::ParseAttribute(aNamespaceID, aAttribute,
                                          aValue, aResult);
  if (aNamespaceID == kNameSpaceID_XLink &&
      aAttribute == nsGkAtoms::href &&
      IsInDoc()) {
    
    
    UpdateHrefTarget(GetParent(), aValue);
  }
  return returnVal;
}

nsresult
nsSVGMpathElement::UnsetAttr(int32_t aNamespaceID,
                             nsIAtom* aAttribute, bool aNotify)
{
  nsresult rv = nsSVGMpathElementBase::UnsetAttr(aNamespaceID, aAttribute,
                                                 aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aNamespaceID == kNameSpaceID_XLink &&
      aAttribute == nsGkAtoms::href) {
    UnlinkHrefTarget(true);
  }

  return NS_OK;
}




nsSVGElement::StringAttributesInfo
nsSVGMpathElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}




void
nsSVGMpathElement::AttributeChanged(nsIDocument* aDocument,
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
nsSVGMpathElement::GetReferencedPath()
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
nsSVGMpathElement::UpdateHrefTarget(nsIContent* aParent,
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
nsSVGMpathElement::UnlinkHrefTarget(bool aNotifyParent)
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
nsSVGMpathElement::NotifyParentOfMpathChange(nsIContent* aParent)
{
  if (aParent && aParent->IsSVG(nsGkAtoms::animateMotion)) {

    nsSVGAnimateMotionElement* animateMotionParent =
      static_cast<nsSVGAnimateMotionElement*>(aParent);

    animateMotionParent->MpathChanged();
    AnimationNeedsResample();
  }
}
