




#include "mozilla/Util.h"

#include "nsSVGAElement.h"
#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGAElement.h"
#include "nsIDOMSVGURIReference.h"
#include "nsILink.h"
#include "nsSVGString.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "nsContentUtils.h"

using namespace mozilla;

nsSVGElement::StringInfo nsSVGAElement::sStringInfo[2] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, true },
  { &nsGkAtoms::target, kNameSpaceID_None, true }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(A)





NS_IMPL_ADDREF_INHERITED(nsSVGAElement, nsSVGAElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGAElement, nsSVGAElementBase)

DOMCI_NODE_DATA(SVGAElement, nsSVGAElement)

NS_INTERFACE_TABLE_HEAD(nsSVGAElement)
  NS_NODE_INTERFACE_TABLE8(nsSVGAElement,
                           nsIDOMNode,
                           nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGAElement,
                           nsIDOMSVGTests,
                           nsIDOMSVGURIReference,
                           nsILink,
                           Link)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGAElementBase)





nsSVGAElement::nsSVGAElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGAElementBase(aNodeInfo),
    Link(this)
{
}





NS_IMETHODIMP
nsSVGAElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  return mStringAttributes[HREF].ToDOMAnimatedString(aHref, this);
}





nsresult
nsSVGAElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  nsresult rv = nsGenericElement::PreHandleEvent(aVisitor);
  NS_ENSURE_SUCCESS(rv, rv);

  return PreHandleEventForLinks(aVisitor);
}

nsresult
nsSVGAElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  return PostHandleEventForLinks(aVisitor);
}

NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGAElement)






NS_IMETHODIMP
nsSVGAElement::GetTarget(nsIDOMSVGAnimatedString * *aTarget)
{
  return mStringAttributes[TARGET].ToDOMAnimatedString(aTarget, this);
}





nsresult
nsSVGAElement::BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                          nsIContent *aBindingParent,
                          bool aCompileEventHandlers)
{
  Link::ResetLinkState(false);

  nsresult rv = nsSVGAElementBase::BindToTree(aDocument, aParent,
                                              aBindingParent,
                                              aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (aDocument) {
    aDocument->RegisterPendingLinkUpdate(this);
  }

  return NS_OK;
}

void
nsSVGAElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  
  
  Link::ResetLinkState(false);
  
  nsIDocument* doc = GetCurrentDoc();
  if (doc) {
    doc->UnregisterPendingLinkUpdate(this);
  }

  nsSVGAElementBase::UnbindFromTree(aDeep, aNullParent);
}

nsLinkState
nsSVGAElement::GetLinkState() const
{
  return Link::GetLinkState();
}

already_AddRefed<nsIURI>
nsSVGAElement::GetHrefURI() const
{
  nsCOMPtr<nsIURI> hrefURI;
  return IsLink(getter_AddRefs(hrefURI)) ? hrefURI.forget() : nullptr;
}


NS_IMETHODIMP_(bool)
nsSVGAElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sFEFloodMap,
    sFiltersMap,
    sFontSpecificationMap,
    sGradientStopMap,
    sLightingEffectsMap,
    sMarkersMap,
    sTextContentElementsMap,
    sViewportsMap
  };

  return FindAttributeDependence(name, map) ||
    nsSVGAElementBase::IsAttributeMapped(name);
}

bool
nsSVGAElement::IsFocusable(int32_t *aTabIndex, bool aWithMouse)
{
  nsCOMPtr<nsIURI> uri;
  if (IsLink(getter_AddRefs(uri))) {
    if (aTabIndex) {
      *aTabIndex = ((sTabFocusModel & eTabFocus_linksMask) == 0 ? -1 : 0);
    }
    return true;
  }

  if (aTabIndex) {
    *aTabIndex = -1;
  }

  return false;
}

bool
nsSVGAElement::IsLink(nsIURI** aURI) const
{
  
  
  
  
  
  
  
  
  

  static nsIContent::AttrValuesArray sTypeVals[] =
    { &nsGkAtoms::_empty, &nsGkAtoms::simple, nullptr };

  static nsIContent::AttrValuesArray sShowVals[] =
    { &nsGkAtoms::_empty, &nsGkAtoms::_new, &nsGkAtoms::replace, nullptr };

  static nsIContent::AttrValuesArray sActuateVals[] =
    { &nsGkAtoms::_empty, &nsGkAtoms::onRequest, nullptr };

  
  const nsAttrValue* href = mAttrsAndChildren.GetAttr(nsGkAtoms::href,
                                                      kNameSpaceID_XLink);
  if (href &&
      FindAttrValueIn(kNameSpaceID_XLink, nsGkAtoms::type,
                      sTypeVals, eCaseMatters) !=
                      nsIContent::ATTR_VALUE_NO_MATCH &&
      FindAttrValueIn(kNameSpaceID_XLink, nsGkAtoms::show,
                      sShowVals, eCaseMatters) !=
                      nsIContent::ATTR_VALUE_NO_MATCH &&
      FindAttrValueIn(kNameSpaceID_XLink, nsGkAtoms::actuate,
                      sActuateVals, eCaseMatters) !=
                      nsIContent::ATTR_VALUE_NO_MATCH) {
    nsCOMPtr<nsIURI> baseURI = GetBaseURI();
    
    nsAutoString str;
    mStringAttributes[HREF].GetAnimValue(str, this);
    nsContentUtils::NewURIWithDocumentCharset(aURI, str,
                                              OwnerDoc(), baseURI);
    
    return !!*aURI;
  }

  *aURI = nullptr;
  return false;
}

void
nsSVGAElement::GetLinkTarget(nsAString& aTarget)
{
  mStringAttributes[TARGET].GetAnimValue(aTarget, this);
  if (aTarget.IsEmpty()) {

    static nsIContent::AttrValuesArray sShowVals[] =
      { &nsGkAtoms::_new, &nsGkAtoms::replace, nullptr };

    switch (FindAttrValueIn(kNameSpaceID_XLink, nsGkAtoms::show,
                            sShowVals, eCaseMatters)) {
    case 0:
      aTarget.AssignLiteral("_blank");
      return;
    case 1:
      return;
    }
    nsIDocument* ownerDoc = OwnerDoc();
    if (ownerDoc) {
      ownerDoc->GetBaseTarget(aTarget);
    }
  }
}

nsEventStates
nsSVGAElement::IntrinsicState() const
{
  return Link::LinkState() | nsSVGAElementBase::IntrinsicState();
}

nsresult
nsSVGAElement::SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                       nsIAtom* aPrefix, const nsAString& aValue,
                       bool aNotify)
{
  nsresult rv = nsSVGAElementBase::SetAttr(aNameSpaceID, aName, aPrefix,
                                           aValue, aNotify);

  
  
  
  
  
  if (aName == nsGkAtoms::href && aNameSpaceID == kNameSpaceID_XLink) {
    Link::ResetLinkState(!!aNotify);
  }

  return rv;
}

nsresult
nsSVGAElement::UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttr,
                         bool aNotify)
{
  nsresult rv = nsSVGAElementBase::UnsetAttr(aNameSpaceID, aAttr, aNotify);

  
  
  
  
  
  if (aAttr == nsGkAtoms::href && aNameSpaceID == kNameSpaceID_XLink) {
    Link::ResetLinkState(!!aNotify);
  }

  return rv;
}




nsSVGElement::StringAttributesInfo
nsSVGAElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}
