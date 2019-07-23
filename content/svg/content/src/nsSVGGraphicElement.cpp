






































#include "nsSVGGraphicElement.h"
#include "nsSVGTransformList.h"
#include "nsSVGAnimatedTransformList.h"
#include "nsGkAtoms.h"
#include "nsSVGMatrix.h"
#include "nsIDOMEventTarget.h"
#include "nsIFrame.h"
#include "nsISVGChildFrame.h"
#include "nsIDOMSVGPoint.h"
#include "nsSVGUtils.h"
#include "nsDOMError.h"
#include "nsSVGRect.h"




NS_IMPL_ADDREF_INHERITED(nsSVGGraphicElement, nsSVGGraphicElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGGraphicElement, nsSVGGraphicElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGGraphicElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGLocatable)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTransformable)
NS_INTERFACE_MAP_END_INHERITING(nsSVGGraphicElementBase)




nsSVGGraphicElement::nsSVGGraphicElement(nsINodeInfo *aNodeInfo)
  : nsSVGGraphicElementBase(aNodeInfo)
{
}





NS_IMETHODIMP nsSVGGraphicElement::GetNearestViewportElement(nsIDOMSVGElement * *aNearestViewportElement)
{
  *aNearestViewportElement = nsSVGUtils::GetNearestViewportElement(this).get();
  return NS_OK;
}


NS_IMETHODIMP nsSVGGraphicElement::GetFarthestViewportElement(nsIDOMSVGElement * *aFarthestViewportElement)
{
  *aFarthestViewportElement = nsSVGUtils::GetFarthestViewportElement(this).get();
  return NS_OK;
}


NS_IMETHODIMP nsSVGGraphicElement::GetBBox(nsIDOMSVGRect **_retval)
{
  *_retval = nsnull;

  nsIFrame* frame = GetPrimaryFrame(Flush_Layout);

  if (!frame || (frame->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD))
    return NS_ERROR_FAILURE;

  nsISVGChildFrame* svgframe = do_QueryFrame(frame);
  if (svgframe) {
    return NS_NewSVGRect(_retval, nsSVGUtils::GetBBox(frame));
  }
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsSVGGraphicElement::GetCTM(nsIDOMSVGMatrix * *aCTM)
{
  gfxMatrix m = nsSVGUtils::GetCTM(this, PR_FALSE);
  *aCTM = m.IsSingular() ? nsnull : NS_NewSVGMatrix(m).get();
  return NS_OK;
}


NS_IMETHODIMP nsSVGGraphicElement::GetScreenCTM(nsIDOMSVGMatrix * *aCTM)
{
  gfxMatrix m = nsSVGUtils::GetCTM(this, PR_TRUE);
  *aCTM = m.IsSingular() ? nsnull : NS_NewSVGMatrix(m).get();
  return NS_OK;
}


NS_IMETHODIMP nsSVGGraphicElement::GetTransformToElement(nsIDOMSVGElement *element, nsIDOMSVGMatrix **_retval)
{
  if (!element)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;

  nsresult rv;
  *_retval = nsnull;
  nsCOMPtr<nsIDOMSVGMatrix> ourScreenCTM;
  nsCOMPtr<nsIDOMSVGMatrix> targetScreenCTM;
  nsCOMPtr<nsIDOMSVGMatrix> tmp;
  nsCOMPtr<nsIDOMSVGLocatable> target = do_QueryInterface(element, &rv);
  if (NS_FAILED(rv)) return rv;

  
  GetScreenCTM(getter_AddRefs(ourScreenCTM));
  if (!ourScreenCTM) return NS_ERROR_DOM_SVG_MATRIX_NOT_INVERTABLE;
  target->GetScreenCTM(getter_AddRefs(targetScreenCTM));
  if (!targetScreenCTM) return NS_ERROR_DOM_SVG_MATRIX_NOT_INVERTABLE;
  rv = targetScreenCTM->Inverse(getter_AddRefs(tmp));
  if (NS_FAILED(rv)) return rv;
  return tmp->Multiply(ourScreenCTM, _retval);  
}





NS_IMETHODIMP nsSVGGraphicElement::GetTransform(nsIDOMSVGAnimatedTransformList * *aTransform)
{
  if (!mTransforms && NS_FAILED(CreateTransformList()))
    return NS_ERROR_OUT_OF_MEMORY;
      
  *aTransform = mTransforms;
  NS_ADDREF(*aTransform);
  return NS_OK;
}




NS_IMETHODIMP_(PRBool)
nsSVGGraphicElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sColorMap,
    sFillStrokeMap,
    sGraphicsMap
  };
  
  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGGraphicElementBase::IsAttributeMapped(name);
}




PRBool
nsSVGGraphicElement::IsEventName(nsIAtom* aName)
{
  return nsContentUtils::IsEventAttributeName(aName, EventNameType_SVGGraphic);
}

gfxMatrix
nsSVGGraphicElement::PrependLocalTransformTo(const gfxMatrix &aMatrix)
{
  if (!mTransforms)
    return aMatrix;

  nsresult rv;
  nsCOMPtr<nsIDOMSVGTransformList> transforms;
  rv = mTransforms->GetAnimVal(getter_AddRefs(transforms));
  NS_ENSURE_SUCCESS(rv, aMatrix);
  PRUint32 count;
  transforms->GetNumberOfItems(&count);
  if (count == 0)
    return aMatrix;

  nsCOMPtr<nsIDOMSVGMatrix> matrix =
    nsSVGTransformList::GetConsolidationMatrix(transforms);
  return gfxMatrix(aMatrix).PreMultiply(nsSVGUtils::ConvertSVGMatrixToThebes(matrix));
}

nsresult
nsSVGGraphicElement::BeforeSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                   const nsAString* aValue, PRBool aNotify)
{
  if (aNamespaceID == kNameSpaceID_None &&
      aName == nsGkAtoms::transform &&
      !mTransforms &&
      NS_FAILED(CreateTransformList()))
    return NS_ERROR_OUT_OF_MEMORY;

  return nsSVGGraphicElementBase::BeforeSetAttr(aNamespaceID, aName,
                                                aValue, aNotify);
}

nsresult
nsSVGGraphicElement::CreateTransformList()
{
  nsresult rv;

  
  nsCOMPtr<nsIDOMSVGTransformList> transformList;
  rv = nsSVGTransformList::Create(getter_AddRefs(transformList));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = NS_NewSVGAnimatedTransformList(getter_AddRefs(mTransforms),
                                      transformList);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = AddMappedSVGValue(nsGkAtoms::transform, mTransforms);
  if (NS_FAILED(rv)) {
    mTransforms = nsnull;
    return rv;
  }

  return NS_OK;
}
