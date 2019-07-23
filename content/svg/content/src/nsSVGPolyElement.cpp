



































#include "nsSVGPolyElement.h"
#include "gfxContext.h"




NS_IMPL_ADDREF_INHERITED(nsSVGPolyElement,nsSVGPolyElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGPolyElement,nsSVGPolyElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGPolyElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedPoints)
NS_INTERFACE_MAP_END_INHERITING(nsSVGPolyElementBase)




nsSVGPolyElement::nsSVGPolyElement(nsINodeInfo* aNodeInfo)
  : nsSVGPolyElementBase(aNodeInfo)
{

}

nsresult
nsSVGPolyElement::Init()
{
  nsresult rv = nsSVGPolyElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);

  
  
  
  rv = nsSVGPointList::Create(getter_AddRefs(mPoints));
  NS_ENSURE_SUCCESS(rv,rv);
  rv = AddMappedSVGValue(nsGkAtoms::points, mPoints);
  NS_ENSURE_SUCCESS(rv,rv);

  return rv;
}





NS_IMETHODIMP 
nsSVGPolyElement::GetPoints(nsIDOMSVGPointList * *aPoints)
{
  *aPoints = mPoints;
  NS_ADDREF(*aPoints);
  return NS_OK;
}


NS_IMETHODIMP 
nsSVGPolyElement::GetAnimatedPoints(nsIDOMSVGPointList * *aAnimatedPoints)
{
  *aAnimatedPoints = mPoints;
  NS_ADDREF(*aAnimatedPoints);
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
nsSVGPolyElement::IsDependentAttribute(nsIAtom *aName)
{
  if (aName == nsGkAtoms::points)
    return PR_TRUE;

  return PR_FALSE;
}

void
nsSVGPolyElement::GetMarkPoints(nsTArray<nsSVGMark> *aMarks)
{
  if (!mPoints)
    return;

  PRUint32 count;
  mPoints->GetNumberOfItems(&count);
  if (count == 0)
    return;

  float px = 0.0, py = 0.0, prevAngle;

  for (PRUint32 i = 0; i < count; ++i) {
    nsCOMPtr<nsIDOMSVGPoint> point;
    mPoints->GetItem(i, getter_AddRefs(point));

    float x, y;
    point->GetX(&x);
    point->GetY(&y);

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
  if (!mPoints)
    return;

  PRUint32 count;
  mPoints->GetNumberOfItems(&count);
  if (count == 0)
    return;

  PRUint32 i;
  for (i = 0; i < count; ++i) {
    nsCOMPtr<nsIDOMSVGPoint> point;
    mPoints->GetItem(i, getter_AddRefs(point));

    float x, y;
    point->GetX(&x);
    point->GetY(&y);
    if (i == 0)
      aCtx->MoveTo(gfxPoint(x, y));
    else
      aCtx->LineTo(gfxPoint(x, y));
  }
}

