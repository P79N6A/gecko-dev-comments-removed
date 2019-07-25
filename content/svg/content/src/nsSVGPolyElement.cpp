



































#include "nsSVGPolyElement.h"
#include "DOMSVGPointList.h"
#include "gfxContext.h"
#include "nsSVGUtils.h"

using namespace mozilla;




NS_IMPL_ADDREF_INHERITED(nsSVGPolyElement,nsSVGPolyElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGPolyElement,nsSVGPolyElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGPolyElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedPoints)
NS_INTERFACE_MAP_END_INHERITING(nsSVGPolyElementBase)




nsSVGPolyElement::nsSVGPolyElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGPolyElementBase(aNodeInfo)
{

}





NS_IMETHODIMP 
nsSVGPolyElement::GetPoints(nsIDOMSVGPointList * *aPoints)
{
  void *key = mPoints.GetBaseValKey();
  *aPoints = DOMSVGPointList::GetDOMWrapper(key, this, PR_FALSE).get();
  return NS_OK;
}


NS_IMETHODIMP 
nsSVGPolyElement::GetAnimatedPoints(nsIDOMSVGPointList * *aAnimatedPoints)
{
  void *key = mPoints.GetAnimValKey();
  *aAnimatedPoints = DOMSVGPointList::GetDOMWrapper(key, this, PR_TRUE).get();
  return NS_OK;
}




NS_IMETHODIMP_(PRBool)
nsSVGPolyElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sMarkersMap
  };
  
  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGPolyElementBase::IsAttributeMapped(name);
}




PRBool
nsSVGPolyElement::AttributeDefinesGeometry(const nsIAtom *aName)
{
  if (aName == nsGkAtoms::points)
    return PR_TRUE;

  return PR_FALSE;
}

void
nsSVGPolyElement::GetMarkPoints(nsTArray<nsSVGMark> *aMarks)
{
  const SVGPointList &points = mPoints.GetAnimValue();

  if (!points.Length())
    return;

  float px = 0.0, py = 0.0, prevAngle = 0.0;

  for (PRUint32 i = 0; i < points.Length(); ++i) {
    float x = points[i].mX;
    float y = points[i].mY;
    float angle = atan2(y-py, x-px);
    if (i == 1)
      aMarks->ElementAt(aMarks->Length() - 1).angle = angle;
    else if (i > 1)
      aMarks->ElementAt(aMarks->Length() - 1).angle =
        nsSVGUtils::AngleBisect(prevAngle, angle);

    aMarks->AppendElement(nsSVGMark(x, y, 0));

    prevAngle = angle;
    px = x;
    py = y;
  }

  aMarks->ElementAt(aMarks->Length() - 1).angle = prevAngle;
}

void
nsSVGPolyElement::ConstructPath(gfxContext *aCtx)
{
  const SVGPointList &points = mPoints.GetAnimValue();

  if (!points.Length())
    return;

  aCtx->MoveTo(points[0]);
  for (PRUint32 i = 1; i < points.Length(); ++i) {
    aCtx->LineTo(points[i]);
  }
}

