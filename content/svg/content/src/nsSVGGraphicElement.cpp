






































#include "nsSVGGraphicElement.h"
#include "nsSVGTransformList.h"
#include "nsSVGAnimatedTransformList.h"
#include "nsGkAtoms.h"
#include "nsSVGMatrix.h"
#include "nsIDOMEventTarget.h"
#include "nsBindingManager.h"
#include "nsIFrame.h"
#include "nsISVGChildFrame.h"
#include "nsIDOMSVGPoint.h"
#include "nsSVGUtils.h"
#include "nsDOMError.h"




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
  return nsSVGUtils::GetNearestViewportElement(this, aNearestViewportElement);
}


NS_IMETHODIMP nsSVGGraphicElement::GetFarthestViewportElement(nsIDOMSVGElement * *aFarthestViewportElement)
{
  return nsSVGUtils::GetFarthestViewportElement(this, aFarthestViewportElement);
}


NS_IMETHODIMP nsSVGGraphicElement::GetBBox(nsIDOMSVGRect **_retval)
{
  *_retval = nsnull;

  nsIFrame* frame = GetPrimaryFrame(Flush_Layout);

  if (!frame || (frame->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD))
    return NS_ERROR_FAILURE;

  nsISVGChildFrame* svgframe = do_QueryFrame(frame);
  NS_ASSERTION(svgframe, "wrong frame type");
  if (svgframe) {
    svgframe->SetMatrixPropagation(PR_FALSE);
    svgframe->NotifySVGChanged(nsISVGChildFrame::SUPPRESS_INVALIDATION |
                               nsISVGChildFrame::TRANSFORM_CHANGED);
    nsresult rv = svgframe->GetBBox(_retval);
    svgframe->SetMatrixPropagation(PR_TRUE);
    svgframe->NotifySVGChanged(nsISVGChildFrame::SUPPRESS_INVALIDATION |
                               nsISVGChildFrame::TRANSFORM_CHANGED);
    return rv;
  }
  return NS_ERROR_FAILURE;
}


nsresult
nsSVGGraphicElement::AppendLocalTransform(nsIDOMSVGMatrix *aCTM,
                                          nsIDOMSVGMatrix **_retval)
{
  if (!mTransforms) {
    *_retval = aCTM;
    NS_ADDREF(*_retval);
    return NS_OK;
  }

  
  nsCOMPtr<nsIDOMSVGTransformList> transforms;
  mTransforms->GetAnimVal(getter_AddRefs(transforms));
  NS_ENSURE_TRUE(transforms, NS_ERROR_FAILURE);
  nsCOMPtr<nsIDOMSVGMatrix> matrix =
    nsSVGTransformList::GetConsolidationMatrix(transforms);
  if (!matrix) {
    *_retval = aCTM;
    NS_ADDREF(*_retval);
    return NS_OK;
  }
  return aCTM->Multiply(matrix, _retval);  
}


NS_IMETHODIMP nsSVGGraphicElement::GetCTM(nsIDOMSVGMatrix **_retval)
{
  nsresult rv;
  *_retval = nsnull;

  nsIDocument* currentDoc = GetCurrentDoc();
  if (currentDoc) {
    
    currentDoc->FlushPendingNotifications(Flush_Layout);
  }

  nsBindingManager *bindingManager = nsnull;
  
  
  
  
  nsIDocument* ownerDoc = GetOwnerDoc();
  if (ownerDoc) {
    bindingManager = ownerDoc->BindingManager();
  }

  nsIContent* parent = nsnull;
  nsCOMPtr<nsIDOMSVGMatrix> parentCTM;

  if (bindingManager) {
    
    parent = bindingManager->GetInsertionParent(this);
  }
  if (!parent) {
    
    parent = GetParent();
  }

  nsCOMPtr<nsIDOMSVGLocatable> locatableElement = do_QueryInterface(parent);
  if (!locatableElement) {
    
    NS_WARNING("SVGGraphicElement without an SVGLocatable parent");
    return NS_ERROR_FAILURE;
  }

  
  rv = locatableElement->GetCTM(getter_AddRefs(parentCTM));
  if (NS_FAILED(rv)) return rv;

  return AppendLocalTransform(parentCTM, _retval);
}


NS_IMETHODIMP nsSVGGraphicElement::GetScreenCTM(nsIDOMSVGMatrix **_retval)
{
  nsresult rv;
  *_retval = nsnull;

  nsIDocument* currentDoc = GetCurrentDoc();
  if (currentDoc) {
    
    currentDoc->FlushPendingNotifications(Flush_Layout);
  }

  nsBindingManager *bindingManager = nsnull;
  
  
  
  
  nsIDocument* ownerDoc = GetOwnerDoc();
  if (ownerDoc) {
    bindingManager = ownerDoc->BindingManager();
  }

  nsIContent* parent = nsnull;
  nsCOMPtr<nsIDOMSVGMatrix> parentScreenCTM;

  if (bindingManager) {
    
    parent = bindingManager->GetInsertionParent(this);
  }
  if (!parent) {
    
    parent = GetParent();
  }

  nsCOMPtr<nsIDOMSVGLocatable> locatableElement = do_QueryInterface(parent);
  if (!locatableElement) {
    
    NS_WARNING("SVGGraphicElement without an SVGLocatable parent");
    return NS_ERROR_FAILURE;
  }

  
  rv = locatableElement->GetScreenCTM(getter_AddRefs(parentScreenCTM));
  if (NS_FAILED(rv)) return rv;

  return AppendLocalTransform(parentScreenCTM, _retval);
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

  
  rv = GetScreenCTM(getter_AddRefs(ourScreenCTM));
  if (NS_FAILED(rv)) return rv;
  rv = target->GetScreenCTM(getter_AddRefs(targetScreenCTM));
  if (NS_FAILED(rv)) return rv;
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

already_AddRefed<nsIDOMSVGMatrix>
nsSVGGraphicElement::GetLocalTransformMatrix()
{
  if (!mTransforms)
    return nsnull;

  nsresult rv;

  nsCOMPtr<nsIDOMSVGTransformList> transforms;
  rv = mTransforms->GetAnimVal(getter_AddRefs(transforms));
  NS_ENSURE_SUCCESS(rv, nsnull);

  return nsSVGTransformList::GetConsolidationMatrix(transforms);
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
