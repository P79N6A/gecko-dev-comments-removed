




#include "mozilla/dom/SVGCircleElement.h"
#include "mozilla/gfx/2D.h"
#include "nsGkAtoms.h"
#include "gfxContext.h"
#include "mozilla/dom/SVGCircleElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Circle)

using namespace mozilla::gfx;

namespace mozilla {
namespace dom {

JSObject*
SVGCircleElement::WrapNode(JSContext *aCx)
{
  return SVGCircleElementBinding::Wrap(aCx, this);
}

nsSVGElement::LengthInfo SVGCircleElement::sLengthInfo[3] =
{
  { &nsGkAtoms::cx, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::cy, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y },
  { &nsGkAtoms::r, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::XY }
};




SVGCircleElement::SVGCircleElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGCircleElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGCircleElement)



already_AddRefed<SVGAnimatedLength>
SVGCircleElement::Cx()
{
  return mLengthAttributes[ATTR_CX].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGCircleElement::Cy()
{
  return mLengthAttributes[ATTR_CY].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGCircleElement::R()
{
  return mLengthAttributes[ATTR_R].ToDOMAnimatedLength(this);
}




 bool
SVGCircleElement::HasValidDimensions() const
{
  return mLengthAttributes[ATTR_R].IsExplicitlySet() &&
         mLengthAttributes[ATTR_R].GetAnimValInSpecifiedUnits() > 0;
}

nsSVGElement::LengthAttributesInfo
SVGCircleElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}




void
SVGCircleElement::ConstructPath(gfxContext *aCtx)
{
  float x, y, r;

  GetAnimatedLengthValues(&x, &y, &r, nullptr);

  if (r > 0.0f)
    aCtx->Arc(gfxPoint(x, y), r, 0, 2*M_PI);
}

TemporaryRef<Path>
SVGCircleElement::BuildPath(PathBuilder* aBuilder)
{
  float x, y, r;
  GetAnimatedLengthValues(&x, &y, &r, nullptr);

  if (r <= 0.0f) {
    return nullptr;
  }

  RefPtr<PathBuilder> pathBuilder = aBuilder ? aBuilder : CreatePathBuilder();

  pathBuilder->Arc(Point(x, y), r, 0, Float(2*M_PI));

  return pathBuilder->Finish();
}

} 
} 
