




#include "mozilla/dom/SVGPathElement.h"

#include <algorithm>

#include "DOMSVGPathSeg.h"
#include "DOMSVGPathSegList.h"
#include "DOMSVGPoint.h"
#include "gfx2DGlue.h"
#include "gfxPlatform.h"
#include "mozilla/dom/SVGPathElementBinding.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"
#include "nsCOMPtr.h"
#include "nsComputedDOMStyle.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsStyleStruct.h"
#include "SVGContentUtils.h"

class gfxContext;

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Path)

using namespace mozilla::gfx;

namespace mozilla {
namespace dom {

JSObject*
SVGPathElement::WrapNode(JSContext *aCx)
{
  return SVGPathElementBinding::Wrap(aCx, this);
}

nsSVGElement::NumberInfo SVGPathElement::sNumberInfo = 
{ &nsGkAtoms::pathLength, 0, false };




SVGPathElement::SVGPathElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGPathElementBase(aNodeInfo)
{
}




size_t
SVGPathElement::SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  return SVGPathElementBase::SizeOfExcludingThis(aMallocSizeOf) +
         mD.SizeOfExcludingThis(aMallocSizeOf);
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGPathElement)

already_AddRefed<SVGAnimatedNumber>
SVGPathElement::PathLength()
{
  return mPathLength.ToDOMAnimatedNumber(this);
}

float
SVGPathElement::GetTotalLength()
{
  RefPtr<Path> flat = GetPathForLengthOrPositionMeasuring();
  return flat ? flat->ComputeLength() : 0.f;
}

already_AddRefed<nsISVGPoint>
SVGPathElement::GetPointAtLength(float distance, ErrorResult& rv)
{
  RefPtr<Path> path = GetPathForLengthOrPositionMeasuring();
  if (!path) {
    rv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  float totalLength = path->ComputeLength();
  if (mPathLength.IsExplicitlySet()) {
    float pathLength = mPathLength.GetAnimValue();
    if (pathLength <= 0) {
      rv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }
    distance *= totalLength / pathLength;
  }
  distance = std::max(0.f,         distance);
  distance = std::min(totalLength, distance);

  nsCOMPtr<nsISVGPoint> point =
    new DOMSVGPoint(path->ComputePointAtLength(distance));
  return point.forget();
}

uint32_t
SVGPathElement::GetPathSegAtLength(float distance)
{
  return mD.GetAnimValue().GetPathSegAtLength(distance);
}

already_AddRefed<DOMSVGPathSegClosePath>
SVGPathElement::CreateSVGPathSegClosePath()
{
  nsRefPtr<DOMSVGPathSegClosePath> pathSeg = new DOMSVGPathSegClosePath();
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegMovetoAbs>
SVGPathElement::CreateSVGPathSegMovetoAbs(float x, float y)
{
  nsRefPtr<DOMSVGPathSegMovetoAbs> pathSeg = new DOMSVGPathSegMovetoAbs(x, y);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegMovetoRel>
SVGPathElement::CreateSVGPathSegMovetoRel(float x, float y)
{
  nsRefPtr<DOMSVGPathSegMovetoRel> pathSeg = new DOMSVGPathSegMovetoRel(x, y);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegLinetoAbs>
SVGPathElement::CreateSVGPathSegLinetoAbs(float x, float y)
{
  nsRefPtr<DOMSVGPathSegLinetoAbs> pathSeg = new DOMSVGPathSegLinetoAbs(x, y);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegLinetoRel>
SVGPathElement::CreateSVGPathSegLinetoRel(float x, float y)
{
  nsRefPtr<DOMSVGPathSegLinetoRel> pathSeg = new DOMSVGPathSegLinetoRel(x, y);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegCurvetoCubicAbs>
SVGPathElement::CreateSVGPathSegCurvetoCubicAbs(float x, float y, float x1, float y1, float x2, float y2)
{
  
  
  
  nsRefPtr<DOMSVGPathSegCurvetoCubicAbs> pathSeg =
    new DOMSVGPathSegCurvetoCubicAbs(x1, y1, x2, y2, x, y);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegCurvetoCubicRel>
SVGPathElement::CreateSVGPathSegCurvetoCubicRel(float x, float y, float x1, float y1, float x2, float y2)
{
  
  nsRefPtr<DOMSVGPathSegCurvetoCubicRel> pathSeg =
    new DOMSVGPathSegCurvetoCubicRel(x1, y1, x2, y2, x, y);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegCurvetoQuadraticAbs>
SVGPathElement::CreateSVGPathSegCurvetoQuadraticAbs(float x, float y, float x1, float y1)
{
  
  nsRefPtr<DOMSVGPathSegCurvetoQuadraticAbs> pathSeg =
    new DOMSVGPathSegCurvetoQuadraticAbs(x1, y1, x, y);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegCurvetoQuadraticRel>
SVGPathElement::CreateSVGPathSegCurvetoQuadraticRel(float x, float y, float x1, float y1)
{
  
  nsRefPtr<DOMSVGPathSegCurvetoQuadraticRel> pathSeg =
    new DOMSVGPathSegCurvetoQuadraticRel(x1, y1, x, y);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegArcAbs>
SVGPathElement::CreateSVGPathSegArcAbs(float x, float y, float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag)
{
  
  nsRefPtr<DOMSVGPathSegArcAbs> pathSeg =
    new DOMSVGPathSegArcAbs(r1, r2, angle, largeArcFlag, sweepFlag, x, y);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegArcRel>
SVGPathElement::CreateSVGPathSegArcRel(float x, float y, float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag)
{
  
  nsRefPtr<DOMSVGPathSegArcRel> pathSeg =
    new DOMSVGPathSegArcRel(r1, r2, angle, largeArcFlag, sweepFlag, x, y);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegLinetoHorizontalAbs>
SVGPathElement::CreateSVGPathSegLinetoHorizontalAbs(float x)
{
  nsRefPtr<DOMSVGPathSegLinetoHorizontalAbs> pathSeg =
    new DOMSVGPathSegLinetoHorizontalAbs(x);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegLinetoHorizontalRel>
SVGPathElement::CreateSVGPathSegLinetoHorizontalRel(float x)
{
  nsRefPtr<DOMSVGPathSegLinetoHorizontalRel> pathSeg =
    new DOMSVGPathSegLinetoHorizontalRel(x);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegLinetoVerticalAbs>
SVGPathElement::CreateSVGPathSegLinetoVerticalAbs(float y)
{
  nsRefPtr<DOMSVGPathSegLinetoVerticalAbs> pathSeg =
    new DOMSVGPathSegLinetoVerticalAbs(y);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegLinetoVerticalRel>
SVGPathElement::CreateSVGPathSegLinetoVerticalRel(float y)
{
  nsRefPtr<DOMSVGPathSegLinetoVerticalRel> pathSeg =
    new DOMSVGPathSegLinetoVerticalRel(y);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegCurvetoCubicSmoothAbs>
SVGPathElement::CreateSVGPathSegCurvetoCubicSmoothAbs(float x, float y, float x2, float y2)
{
  
  nsRefPtr<DOMSVGPathSegCurvetoCubicSmoothAbs> pathSeg =
    new DOMSVGPathSegCurvetoCubicSmoothAbs(x2, y2, x, y);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegCurvetoCubicSmoothRel>
SVGPathElement::CreateSVGPathSegCurvetoCubicSmoothRel(float x, float y, float x2, float y2)
{
  
  nsRefPtr<DOMSVGPathSegCurvetoCubicSmoothRel> pathSeg =
    new DOMSVGPathSegCurvetoCubicSmoothRel(x2, y2, x, y);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegCurvetoQuadraticSmoothAbs>
SVGPathElement::CreateSVGPathSegCurvetoQuadraticSmoothAbs(float x, float y)
{
  nsRefPtr<DOMSVGPathSegCurvetoQuadraticSmoothAbs> pathSeg =
    new DOMSVGPathSegCurvetoQuadraticSmoothAbs(x, y);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegCurvetoQuadraticSmoothRel>
SVGPathElement::CreateSVGPathSegCurvetoQuadraticSmoothRel(float x, float y)
{
  nsRefPtr<DOMSVGPathSegCurvetoQuadraticSmoothRel> pathSeg =
    new DOMSVGPathSegCurvetoQuadraticSmoothRel(x, y);
  return pathSeg.forget();
}

already_AddRefed<DOMSVGPathSegList>
SVGPathElement::PathSegList()
{
  return DOMSVGPathSegList::GetDOMWrapper(mD.GetBaseValKey(), this, false);
}

already_AddRefed<DOMSVGPathSegList>
SVGPathElement::AnimatedPathSegList()
{
  return DOMSVGPathSegList::GetDOMWrapper(mD.GetAnimValKey(), this, true);
}




 bool
SVGPathElement::HasValidDimensions() const
{
  return !mD.GetAnimValue().IsEmpty();
}

nsSVGElement::NumberAttributesInfo
SVGPathElement::GetNumberInfo()
{
  return NumberAttributesInfo(&mPathLength, &sNumberInfo, 1);
}




NS_IMETHODIMP_(bool)
SVGPathElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sMarkersMap
  };

  return FindAttributeDependence(name, map) ||
    SVGPathElementBase::IsAttributeMapped(name);
}

TemporaryRef<Path>
SVGPathElement::GetPathForLengthOrPositionMeasuring()
{
  return mD.GetAnimValue().ToPathForLengthOrPositionMeasuring();
}




bool
SVGPathElement::AttributeDefinesGeometry(const nsIAtom *aName)
{
  return aName == nsGkAtoms::d ||
         aName == nsGkAtoms::pathLength;
}

bool
SVGPathElement::IsMarkable()
{
  return true;
}

void
SVGPathElement::GetMarkPoints(nsTArray<nsSVGMark> *aMarks)
{
  mD.GetAnimValue().GetMarkerPositioningData(aMarks);
}

void
SVGPathElement::ConstructPath(gfxContext *aCtx)
{
  mD.GetAnimValue().ConstructPath(aCtx);
}

float
SVGPathElement::GetPathLengthScale(PathLengthScaleForType aFor)
{
  NS_ABORT_IF_FALSE(aFor == eForTextPath || aFor == eForStroking,
                    "Unknown enum");
  if (mPathLength.IsExplicitlySet()) {
    float authorsPathLengthEstimate = mPathLength.GetAnimValue();
    if (authorsPathLengthEstimate > 0) {
      RefPtr<Path> path = GetPathForLengthOrPositionMeasuring();
      if (!path) {
        
        
        return 0.0;
      }
      if (aFor == eForTextPath) {
        
        
        
        gfxMatrix matrix = PrependLocalTransformsTo(gfxMatrix());
        if (!matrix.IsIdentity()) {
          RefPtr<PathBuilder> builder =
            path->TransformedCopyToBuilder(ToMatrix(matrix));
          path = builder->Finish();
        }
      }
      return path->ComputeLength() / authorsPathLengthEstimate;
    }
  }
  return 1.0;
}

TemporaryRef<Path>
SVGPathElement::BuildPath(PathBuilder* aBuilder)
{
  
  
  
  
  
  

  uint8_t strokeLineCap = NS_STYLE_STROKE_LINECAP_BUTT;
  Float strokeWidth = 0;

  nsRefPtr<nsStyleContext> styleContext =
    nsComputedDOMStyle::GetStyleContextForElementNoFlush(this, nullptr, nullptr);
  if (styleContext) {
    const nsStyleSVG* style = styleContext->StyleSVG();
    
    
    
    
    if (style->mStrokeLinecap == NS_STYLE_STROKE_LINECAP_SQUARE) {
      strokeLineCap = style->mStrokeLinecap;
      strokeWidth = SVGContentUtils::GetStrokeWidth(this, styleContext, nullptr);
    }
  }

  RefPtr<PathBuilder> builder;
  if (aBuilder) {
    builder = aBuilder;
  } else {
    RefPtr<DrawTarget> drawTarget =
      gfxPlatform::GetPlatform()->ScreenReferenceDrawTarget();
    
    
    
    
    
    
    
    RefPtr<PathBuilder> builder =
      drawTarget->CreatePathBuilder(GetFillRule());
  }

  return mD.GetAnimValue().BuildPath(builder, strokeLineCap, strokeWidth);
}

} 
} 
