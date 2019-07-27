




#include "mozilla/ArrayUtils.h"

#include "nsCOMPtr.h"
#include "mozilla/dom/SVGDocument.h"
#include "mozilla/dom/SVGForeignObjectElement.h"
#include "mozilla/dom/SVGForeignObjectElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(ForeignObject)

namespace mozilla {
namespace dom {

JSObject*
SVGForeignObjectElement::WrapNode(JSContext *aCx)
{
  return SVGForeignObjectElementBinding::Wrap(aCx, this);
}

nsSVGElement::LengthInfo SVGForeignObjectElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::y, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y },
  { &nsGkAtoms::width, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::height, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y },
};




SVGForeignObjectElement::SVGForeignObjectElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGGraphicsElement(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGForeignObjectElement)



already_AddRefed<SVGAnimatedLength>
SVGForeignObjectElement::X()
{
  return mLengthAttributes[ATTR_X].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGForeignObjectElement::Y()
{
  return mLengthAttributes[ATTR_Y].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGForeignObjectElement::Width()
{
  return mLengthAttributes[ATTR_WIDTH].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGForeignObjectElement::Height()
{
  return mLengthAttributes[ATTR_HEIGHT].ToDOMAnimatedLength(this);
}




 gfxMatrix
SVGForeignObjectElement::PrependLocalTransformsTo(const gfxMatrix &aMatrix,
                                                  TransformTypes aWhich) const
{
  NS_ABORT_IF_FALSE(aWhich != eChildToUserSpace || aMatrix.IsIdentity(),
                    "Skipping eUserSpaceToParent transforms makes no sense");

  
  gfxMatrix fromUserSpace =
    SVGGraphicsElement::PrependLocalTransformsTo(aMatrix, aWhich);
  if (aWhich == eUserSpaceToParent) {
    return fromUserSpace;
  }
  
  float x, y;
  const_cast<SVGForeignObjectElement*>(this)->
    GetAnimatedLengthValues(&x, &y, nullptr);
  gfxMatrix toUserSpace = gfxMatrix().Translate(gfxPoint(x, y));
  if (aWhich == eChildToUserSpace) {
    return toUserSpace * aMatrix;
  }
  NS_ABORT_IF_FALSE(aWhich == eAllTransforms, "Unknown TransformTypes");
  return toUserSpace * fromUserSpace;
}

 bool
SVGForeignObjectElement::HasValidDimensions() const
{
  return mLengthAttributes[ATTR_WIDTH].IsExplicitlySet() &&
         mLengthAttributes[ATTR_WIDTH].GetAnimValInSpecifiedUnits() > 0 &&
         mLengthAttributes[ATTR_HEIGHT].IsExplicitlySet() &&
         mLengthAttributes[ATTR_HEIGHT].GetAnimValInSpecifiedUnits() > 0;
}




nsresult
SVGForeignObjectElement::BindToTree(nsIDocument* aDocument,
                                    nsIContent* aParent,
                                    nsIContent* aBindingParent,
                                    bool aCompileEventHandlers)
{
  nsresult rv = SVGGraphicsElement::BindToTree(aDocument, aParent,
                                               aBindingParent,
                                               aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDocument && aDocument->IsSVG()) {
    
    
    
    
    
    
    
    aDocument->AsSVGDocument()->EnsureNonSVGUserAgentStyleSheetsLoaded();
  }

  return rv;
}

NS_IMETHODIMP_(bool)
SVGForeignObjectElement::IsAttributeMapped(const nsIAtom* name) const
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
    SVGGraphicsElement::IsAttributeMapped(name);
}




nsSVGElement::LengthAttributesInfo
SVGForeignObjectElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}

} 
} 

