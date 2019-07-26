




#include "mozilla/dom/SVGEllipseElement.h"
#include "mozilla/dom/SVGEllipseElementBinding.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/PathHelpers.h"
#include "gfxContext.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Ellipse)

using namespace mozilla::gfx;

namespace mozilla {
namespace dom {

JSObject*
SVGEllipseElement::WrapNode(JSContext *aCx)
{
  return SVGEllipseElementBinding::Wrap(aCx, this);
}

nsSVGElement::LengthInfo SVGEllipseElement::sLengthInfo[4] =
{
  { &nsGkAtoms::cx, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::cy, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y },
  { &nsGkAtoms::rx, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::ry, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y },
};




SVGEllipseElement::SVGEllipseElement(already_AddRefed<nsINodeInfo>& aNodeInfo)
  : SVGEllipseElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGEllipseElement)




already_AddRefed<SVGAnimatedLength>
SVGEllipseElement::Cx()
{
  return mLengthAttributes[CX].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGEllipseElement::Cy()
{
  return mLengthAttributes[CY].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGEllipseElement::Rx()
{
  return mLengthAttributes[RX].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGEllipseElement::Ry()
{
  return mLengthAttributes[RY].ToDOMAnimatedLength(this);
}




 bool
SVGEllipseElement::HasValidDimensions() const
{
  return mLengthAttributes[RX].IsExplicitlySet() &&
         mLengthAttributes[RX].GetAnimValInSpecifiedUnits() > 0 &&
         mLengthAttributes[RY].IsExplicitlySet() &&
         mLengthAttributes[RY].GetAnimValInSpecifiedUnits() > 0;
}

nsSVGElement::LengthAttributesInfo
SVGEllipseElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}




void
SVGEllipseElement::ConstructPath(gfxContext *aCtx)
{
  if (!aCtx->IsCairo()) {
    RefPtr<Path> path = BuildPath();
    if (path) {
      nsRefPtr<gfxPath> gfxpath = new gfxPath(path);
      aCtx->SetPath(gfxpath);
    }
    return;
  }

  float x, y, rx, ry;

  GetAnimatedLengthValues(&x, &y, &rx, &ry, nullptr);

  if (rx > 0.0f && ry > 0.0f) {
    aCtx->Ellipse(gfxPoint(x, y), gfxSize(2.0*rx, 2.0*ry));
  }
}

TemporaryRef<Path>
SVGEllipseElement::BuildPath()
{
  float x, y, rx, ry;
  GetAnimatedLengthValues(&x, &y, &rx, &ry, nullptr);

  if (rx <= 0.0f || ry <= 0.0f) {
    return nullptr;
  }

  RefPtr<PathBuilder> pathBuilder = CreatePathBuilder();

  ArcToBezier(pathBuilder.get(), Point(x, y), Size(rx, ry), 0, Float(2*M_PI), false);

  return pathBuilder->Finish();
}

} 
} 
