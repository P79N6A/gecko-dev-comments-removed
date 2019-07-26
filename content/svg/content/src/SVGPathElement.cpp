




#include "mozilla/Util.h"

#include "nsGkAtoms.h"
#include "DOMSVGPathSeg.h"
#include "DOMSVGPathSegList.h"
#include "nsCOMPtr.h"
#include "nsContentUtils.h"
#include "mozilla/dom/SVGPathElement.h"
#include "DOMSVGPoint.h"
#include "gfxContext.h"

DOMCI_NODE_DATA(SVGPathElement, mozilla::dom::SVGPathElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Path)

namespace mozilla {
namespace dom {

nsSVGElement::NumberInfo SVGPathElement::sNumberInfo = 
{ &nsGkAtoms::pathLength, 0, false };




NS_IMPL_ADDREF_INHERITED(SVGPathElement,SVGPathElementBase)
NS_IMPL_RELEASE_INHERITED(SVGPathElement,SVGPathElementBase)

NS_INTERFACE_TABLE_HEAD(SVGPathElement)
  NS_NODE_INTERFACE_TABLE5(SVGPathElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGPathElement, nsIDOMSVGAnimatedPathData)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGPathElement)
NS_INTERFACE_MAP_END_INHERITING(SVGPathElementBase)




SVGPathElement::SVGPathElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGPathElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGPathElement)





NS_IMETHODIMP
SVGPathElement::GetPathLength(nsIDOMSVGAnimatedNumber * *aPathLength)
{
  return mPathLength.ToDOMAnimatedNumber(aPathLength, this);
}


NS_IMETHODIMP
SVGPathElement::GetTotalLength(float *_retval)
{
  *_retval = 0;

  nsRefPtr<gfxFlattenedPath> flat = GetFlattenedPath(gfxMatrix());

  if (!flat)
    return NS_ERROR_FAILURE;

  *_retval = flat->GetLength();

  return NS_OK;
}


NS_IMETHODIMP
SVGPathElement::GetPointAtLength(float distance, nsISupports **_retval)
{
  NS_ENSURE_FINITE(distance, NS_ERROR_ILLEGAL_VALUE);

  nsRefPtr<gfxFlattenedPath> flat = GetFlattenedPath(gfxMatrix());
  if (!flat)
    return NS_ERROR_FAILURE;

  float totalLength = flat->GetLength();
  if (mPathLength.IsExplicitlySet()) {
    float pathLength = mPathLength.GetAnimValue();
    if (pathLength <= 0) {
      return NS_ERROR_FAILURE;
    }
    distance *= totalLength / pathLength;
  }
  distance = NS_MAX(0.f,         distance);
  distance = NS_MIN(totalLength, distance);

  NS_ADDREF(*_retval = new DOMSVGPoint(flat->FindPoint(gfxPoint(distance, 0))));
  return NS_OK;
}


NS_IMETHODIMP
SVGPathElement::GetPathSegAtLength(float distance, uint32_t *_retval)
{
  NS_ENSURE_FINITE(distance, NS_ERROR_ILLEGAL_VALUE);
  *_retval = mD.GetAnimValue().GetPathSegAtLength(distance);
  return NS_OK;
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegClosePath(nsISupports **_retval)
{
  nsISupports* seg = NS_NewSVGPathSegClosePath();
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegMovetoAbs(float x, float y, nsISupports **_retval)
{
  NS_ENSURE_FINITE2(x, y, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegMovetoAbs(x, y);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegMovetoRel(float x, float y, nsISupports **_retval)
{
  NS_ENSURE_FINITE2(x, y, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegMovetoRel(x, y);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegLinetoAbs(float x, float y, nsISupports **_retval)
{
  NS_ENSURE_FINITE2(x, y, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegLinetoAbs(x, y);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegLinetoRel(float x, float y, nsISupports **_retval)
{
  NS_ENSURE_FINITE2(x, y, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegLinetoRel(x, y);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegCurvetoCubicAbs(float x, float y, float x1, float y1, float x2, float y2, nsISupports **_retval)
{
  NS_ENSURE_FINITE6(x, y, x1, y1, x2, y2, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegCurvetoCubicAbs(x, y, x1, y1, x2, y2);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegCurvetoCubicRel(float x, float y, float x1, float y1, float x2, float y2, nsISupports **_retval)
{
  NS_ENSURE_FINITE6(x, y, x1, y1, x2, y2, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegCurvetoCubicRel(x, y, x1, y1, x2, y2);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegCurvetoQuadraticAbs(float x, float y, float x1, float y1, nsISupports **_retval)
{
  NS_ENSURE_FINITE4(x, y, x1, y1, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegCurvetoQuadraticAbs(x, y, x1, y1);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegCurvetoQuadraticRel(float x, float y, float x1, float y1, nsISupports **_retval)
{
  NS_ENSURE_FINITE4(x, y, x1, y1, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegCurvetoQuadraticRel(x, y, x1, y1);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegArcAbs(float x, float y, float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag, nsISupports **_retval)
{
  NS_ENSURE_FINITE5(x, y, r1, r2, angle, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegArcAbs(x, y, r1, r2, angle,
                                                 largeArcFlag, sweepFlag);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegArcRel(float x, float y, float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag, nsISupports **_retval)
{
  NS_ENSURE_FINITE5(x, y, r1, r2, angle, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegArcRel(x, y, r1, r2, angle,
                                                 largeArcFlag, sweepFlag);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegLinetoHorizontalAbs(float x, nsISupports **_retval)
{
  NS_ENSURE_FINITE(x, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegLinetoHorizontalAbs(x);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegLinetoHorizontalRel(float x, nsISupports **_retval)
{
  NS_ENSURE_FINITE(x, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegLinetoHorizontalRel(x);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegLinetoVerticalAbs(float y, nsISupports **_retval)
{
  NS_ENSURE_FINITE(y, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegLinetoVerticalAbs(y);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegLinetoVerticalRel(float y, nsISupports **_retval)
{
  NS_ENSURE_FINITE(y, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegLinetoVerticalRel(y);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegCurvetoCubicSmoothAbs(float x, float y, float x2, float y2, nsISupports **_retval)
{
  NS_ENSURE_FINITE4(x, y, x2, y2, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegCurvetoCubicSmoothAbs(x, y, x2, y2);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegCurvetoCubicSmoothRel(float x, float y, float x2, float y2, nsISupports **_retval)
{
  NS_ENSURE_FINITE4(x, y, x2, y2, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegCurvetoCubicSmoothRel(x, y, x2, y2);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegCurvetoQuadraticSmoothAbs(float x, float y, nsISupports **_retval)
{
  NS_ENSURE_FINITE2(x, y, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegCurvetoQuadraticSmoothAbs(x, y);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
}


NS_IMETHODIMP
SVGPathElement::CreateSVGPathSegCurvetoQuadraticSmoothRel(float x, float y, nsISupports **_retval)
{
  NS_ENSURE_FINITE2(x, y, NS_ERROR_ILLEGAL_VALUE);
  nsISupports* seg = NS_NewSVGPathSegCurvetoQuadraticSmoothRel(x, y);
  NS_ENSURE_TRUE(seg, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(seg, _retval);
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





NS_IMETHODIMP SVGPathElement::GetPathSegList(nsISupports * *aPathSegList)
{
  void *key = mD.GetBaseValKey();
  *aPathSegList = DOMSVGPathSegList::GetDOMWrapper(key, this, false).get();
  return *aPathSegList ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP SVGPathElement::GetAnimatedPathSegList(nsISupports * *aAnimatedPathSegList)
{
  void *key = mD.GetAnimValKey();
  *aAnimatedPathSegList =
    DOMSVGPathSegList::GetDOMWrapper(key, this, true).get();
  return *aAnimatedPathSegList ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
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

already_AddRefed<gfxFlattenedPath>
SVGPathElement::GetFlattenedPath(const gfxMatrix &aMatrix)
{
  return mD.GetAnimValue().ToFlattenedPath(aMatrix);
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

gfxFloat
SVGPathElement::GetPathLengthScale(PathLengthScaleForType aFor)
{
  NS_ABORT_IF_FALSE(aFor == eForTextPath || aFor == eForStroking,
                    "Unknown enum");
  if (mPathLength.IsExplicitlySet()) {
    float authorsPathLengthEstimate = mPathLength.GetAnimValue();
    if (authorsPathLengthEstimate > 0) {
      gfxMatrix matrix;
      if (aFor == eForTextPath) {
        
        
        
        matrix = PrependLocalTransformsTo(matrix);
      }
      nsRefPtr<gfxFlattenedPath> path = GetFlattenedPath(matrix);
      if (path) {
        return path->GetLength() / authorsPathLengthEstimate;
      }
    }
  }
  return 1.0;
}

} 
} 
