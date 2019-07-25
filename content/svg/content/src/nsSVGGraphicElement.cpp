






































#include "mozilla/Util.h"

#include "nsSVGGraphicElement.h"
#include "nsSVGSVGElement.h"
#include "DOMSVGAnimatedTransformList.h"
#include "DOMSVGMatrix.h"
#include "nsGkAtoms.h"
#include "nsIDOMEventTarget.h"
#include "nsIFrame.h"
#include "nsISVGChildFrame.h"
#include "nsIDOMSVGPoint.h"
#include "nsSVGUtils.h"
#include "nsDOMError.h"
#include "nsSVGRect.h"
#include "nsContentUtils.h"

using namespace mozilla;




NS_IMPL_ADDREF_INHERITED(nsSVGGraphicElement, nsSVGGraphicElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGGraphicElement, nsSVGGraphicElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGGraphicElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGLocatable)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTransformable)
NS_INTERFACE_MAP_END_INHERITING(nsSVGGraphicElementBase)




nsSVGGraphicElement::nsSVGGraphicElement(already_AddRefed<nsINodeInfo> aNodeInfo)
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
  NS_IF_ADDREF(*aFarthestViewportElement = nsSVGUtils::GetOuterSVGElement(this));
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
  gfxMatrix m = nsSVGUtils::GetCTM(this, false);
  *aCTM = m.IsSingular() ? nsnull : new DOMSVGMatrix(m);
  NS_IF_ADDREF(*aCTM);
  return NS_OK;
}


NS_IMETHODIMP nsSVGGraphicElement::GetScreenCTM(nsIDOMSVGMatrix * *aCTM)
{
  gfxMatrix m = nsSVGUtils::GetCTM(this, true);
  *aCTM = m.IsSingular() ? nsnull : new DOMSVGMatrix(m);
  NS_IF_ADDREF(*aCTM);
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





NS_IMETHODIMP nsSVGGraphicElement::GetTransform(
    nsIDOMSVGAnimatedTransformList **aTransform)
{
  *aTransform =
    DOMSVGAnimatedTransformList::GetDOMWrapper(GetAnimatedTransformList(), this)
    .get();
  return NS_OK;
}




NS_IMETHODIMP_(bool)
nsSVGGraphicElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sColorMap,
    sFillStrokeMap,
    sGraphicsMap
  };
  
  return FindAttributeDependence(name, map, ArrayLength(map)) ||
    nsSVGGraphicElementBase::IsAttributeMapped(name);
}




bool
nsSVGGraphicElement::IsEventName(nsIAtom* aName)
{
  return nsContentUtils::IsEventAttributeName(aName, EventNameType_SVGGraphic);
}

gfxMatrix
nsSVGGraphicElement::PrependLocalTransformTo(const gfxMatrix &aMatrix) const
{
  gfxMatrix result(aMatrix);

  
  
  
  if (mAnimateMotionTransform) {
    result.PreMultiply(*mAnimateMotionTransform);
  }

  if (mTransforms) {
    result.PreMultiply(mTransforms->GetAnimValue().GetConsolidationMatrix());
  }

  return result;
}

void
nsSVGGraphicElement::SetAnimateMotionTransform(const gfxMatrix* aMatrix)
{
  mAnimateMotionTransform = aMatrix ? new gfxMatrix(*aMatrix) : nsnull;
  DidAnimateTransformList();
}

SVGAnimatedTransformList*
nsSVGGraphicElement::GetAnimatedTransformList()
{
  if (!mTransforms) {
    mTransforms = new SVGAnimatedTransformList();
  }
  return mTransforms;
}
