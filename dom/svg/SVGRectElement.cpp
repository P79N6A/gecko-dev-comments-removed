




#include "mozilla/dom/SVGRectElement.h"
#include "nsGkAtoms.h"
#include "mozilla/dom/SVGRectElementBinding.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/Matrix.h"
#include "mozilla/gfx/Rect.h"
#include "mozilla/gfx/PathHelpers.h"
#include <algorithm>

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Rect)

using namespace mozilla::gfx;

namespace mozilla {
namespace dom {

class SVGAnimatedLength;

JSObject*
SVGRectElement::WrapNode(JSContext *aCx)
{
  return SVGRectElementBinding::Wrap(aCx, this);
}

nsSVGElement::LengthInfo SVGRectElement::sLengthInfo[6] =
{
  { &nsGkAtoms::x, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::y, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y },
  { &nsGkAtoms::width, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::height, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y },
  { &nsGkAtoms::rx, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::ry, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y }
};




SVGRectElement::SVGRectElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGRectElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGRectElement)



already_AddRefed<SVGAnimatedLength>
SVGRectElement::X()
{
  return mLengthAttributes[ATTR_X].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGRectElement::Y()
{
  return mLengthAttributes[ATTR_Y].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGRectElement::Width()
{
  return mLengthAttributes[ATTR_WIDTH].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGRectElement::Height()
{
  return mLengthAttributes[ATTR_HEIGHT].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGRectElement::Rx()
{
  return mLengthAttributes[ATTR_RX].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGRectElement::Ry()
{
  return mLengthAttributes[ATTR_RY].ToDOMAnimatedLength(this);
}




 bool
SVGRectElement::HasValidDimensions() const
{
  return mLengthAttributes[ATTR_WIDTH].IsExplicitlySet() &&
         mLengthAttributes[ATTR_WIDTH].GetAnimValInSpecifiedUnits() > 0 &&
         mLengthAttributes[ATTR_HEIGHT].IsExplicitlySet() &&
         mLengthAttributes[ATTR_HEIGHT].GetAnimValInSpecifiedUnits() > 0;
}

nsSVGElement::LengthAttributesInfo
SVGRectElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}




bool
SVGRectElement::GetGeometryBounds(
  Rect* aBounds, const StrokeOptions& aStrokeOptions, const Matrix& aTransform)
{
  Rect rect;
  Float rx, ry;
  GetAnimatedLengthValues(&rect.x, &rect.y, &rect.width,
                          &rect.height, &rx, &ry, nullptr);

  if (rect.IsEmpty()) {
    
    rect.SetEmpty(); 
    
    *aBounds = aTransform.TransformBounds(rect);
    return true;
  }

  if (!aTransform.IsRectilinear()) {
    
    rx = std::max(rx, 0.0f);
    ry = std::max(ry, 0.0f);

    if (rx != 0 || ry != 0) {
      return false;
    }
  }

  if (aStrokeOptions.mLineWidth > 0.f) {
    rect.Inflate(aStrokeOptions.mLineWidth / 2.f);
  }

  *aBounds = aTransform.TransformBounds(rect);
  return true;
}

void
SVGRectElement::GetAsSimplePath(SimplePath* aSimplePath)
{
  float x, y, width, height, rx, ry;
  GetAnimatedLengthValues(&x, &y, &width, &height, &rx, &ry, nullptr);

  if (width <= 0 || height <= 0) {
    aSimplePath->Reset();
    return;
  }

  rx = std::max(rx, 0.0f);
  ry = std::max(ry, 0.0f);

  if (rx != 0 || ry != 0) {
    aSimplePath->Reset();
    return;
  }

  aSimplePath->SetRect(x, y, width, height);
}

TemporaryRef<Path>
SVGRectElement::BuildPath(PathBuilder* aBuilder)
{
  float x, y, width, height, rx, ry;
  GetAnimatedLengthValues(&x, &y, &width, &height, &rx, &ry, nullptr);

  if (width <= 0 || height <= 0) {
    return nullptr;
  }

  rx = std::max(rx, 0.0f);
  ry = std::max(ry, 0.0f);

  if (rx == 0 && ry == 0) {
    
    Rect r(x, y, width, height);
    aBuilder->MoveTo(r.TopLeft());
    aBuilder->LineTo(r.TopRight());
    aBuilder->LineTo(r.BottomRight());
    aBuilder->LineTo(r.BottomLeft());
    aBuilder->Close();
  } else {
    
    
    bool hasRx = mLengthAttributes[ATTR_RX].IsExplicitlySet();
    bool hasRy = mLengthAttributes[ATTR_RY].IsExplicitlySet();
    MOZ_ASSERT(hasRx || hasRy);

    if (hasRx && !hasRy) {
      ry = rx;
    } else if (hasRy && !hasRx) {
      rx = ry;
    }

    
    rx = std::min(rx, width / 2);
    ry = std::min(ry, height / 2);

    RectCornerRadii radii(rx, ry);
    AppendRoundedRectToPath(aBuilder, Rect(x, y, width, height), radii);
  }

  return aBuilder->Finish();
}

} 
} 
