



































#include "nsGkAtoms.h"
#include "nsSVGAnimatedAngle.h"
#include "nsSVGAnimatedRect.h"
#include "nsSVGLength.h"
#include "nsSVGEnum.h"
#include "nsSVGAngle.h"
#include "nsSVGRect.h"
#include "nsSVGAnimatedEnumeration.h"
#include "nsCOMPtr.h"
#include "nsISVGValueUtils.h"
#include "nsSVGAnimatedPreserveAspectRatio.h"
#include "nsSVGPreserveAspectRatio.h"
#include "nsSVGMatrix.h"
#include "nsDOMError.h"
#include "nsSVGUtils.h"
#include "nsSVGMarkerElement.h"

nsSVGElement::LengthInfo nsSVGMarkerElement::sLengthInfo[4] =
{
  { &nsGkAtoms::refX, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::refY, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
  { &nsGkAtoms::markerWidth, 3, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::markerHeight, 3, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Marker)




NS_IMPL_ADDREF_INHERITED(nsSVGMarkerElement,nsSVGMarkerElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGMarkerElement,nsSVGMarkerElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGMarkerElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGFitToViewBox)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGMarkerElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGMarkerElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGMarkerElementBase)




nsSVGMarkerElement::nsSVGMarkerElement(nsINodeInfo *aNodeInfo)
  : nsSVGMarkerElementBase(aNodeInfo), mCoordCtx(nsnull)
{
}

nsresult
nsSVGMarkerElement::Init()
{
  nsresult rv = nsSVGMarkerElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);

  
  static struct nsSVGEnumMapping gMarkerUnits[] = {
    {&nsGkAtoms::strokeWidth, SVG_MARKERUNITS_STROKEWIDTH},
    {&nsGkAtoms::userSpaceOnUse, SVG_MARKERUNITS_USERSPACEONUSE},
    {nsnull, 0}
  };
  
  

  
  {
    nsCOMPtr<nsISVGEnum> units;
    rv = NS_NewSVGEnum(getter_AddRefs(units), SVG_MARKERUNITS_STROKEWIDTH, gMarkerUnits);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedEnumeration(getter_AddRefs(mMarkerUnits), units);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::markerUnits, mMarkerUnits);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsIDOMSVGAngle> angle;
    rv = NS_NewSVGAngle(getter_AddRefs(angle), 0.0f);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedAngle(getter_AddRefs(mOrient), angle);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::orient, mOrient);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsIDOMSVGRect> viewbox;
    rv = NS_NewSVGRect(getter_AddRefs(viewbox));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedRect(getter_AddRefs(mViewBox), viewbox);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::viewBox, mViewBox);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsIDOMSVGPreserveAspectRatio> preserveAspectRatio;
    rv = NS_NewSVGPreserveAspectRatio(getter_AddRefs(preserveAspectRatio));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedPreserveAspectRatio(
      getter_AddRefs(mPreserveAspectRatio),
      preserveAspectRatio);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::preserveAspectRatio,
                           mPreserveAspectRatio);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  
  return NS_OK;
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGMarkerElement)





  NS_IMETHODIMP nsSVGMarkerElement::GetViewBox(nsIDOMSVGAnimatedRect * *aViewBox)
{
  *aViewBox = mViewBox;
  NS_ADDREF(*aViewBox);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGMarkerElement::GetPreserveAspectRatio(nsIDOMSVGAnimatedPreserveAspectRatio * *aPreserveAspectRatio)
{
  *aPreserveAspectRatio = mPreserveAspectRatio;
  NS_ADDREF(*aPreserveAspectRatio);
  return NS_OK;
}





NS_IMETHODIMP nsSVGMarkerElement::GetRefX(nsIDOMSVGAnimatedLength * *aRefX)
{
  return mLengthAttributes[REFX].ToDOMAnimatedLength(aRefX, this);
}


NS_IMETHODIMP nsSVGMarkerElement::GetRefY(nsIDOMSVGAnimatedLength * *aRefY)
{
  return mLengthAttributes[REFY].ToDOMAnimatedLength(aRefY, this);
}


NS_IMETHODIMP nsSVGMarkerElement::GetMarkerUnits(nsIDOMSVGAnimatedEnumeration * *aMarkerUnits)
{
  *aMarkerUnits = mMarkerUnits;
  NS_IF_ADDREF(*aMarkerUnits);
  return NS_OK;
}


NS_IMETHODIMP nsSVGMarkerElement::GetMarkerWidth(nsIDOMSVGAnimatedLength * *aMarkerWidth)
{
  return mLengthAttributes[MARKERWIDTH].ToDOMAnimatedLength(aMarkerWidth, this);
}


NS_IMETHODIMP nsSVGMarkerElement::GetMarkerHeight(nsIDOMSVGAnimatedLength * *aMarkerHeight)
{
  return mLengthAttributes[MARKERHEIGHT].ToDOMAnimatedLength(aMarkerHeight, this);
}


NS_IMETHODIMP nsSVGMarkerElement::GetOrientType(nsIDOMSVGAnimatedEnumeration * *aOrientType)
{
  static struct nsSVGEnumMapping gOrientType[] = {
    {&nsGkAtoms::_auto, SVG_MARKER_ORIENT_AUTO},
    {nsnull, 0}
  };

  nsresult rv;
  nsCOMPtr<nsISVGEnum> orient;
  rv = NS_NewSVGEnum(getter_AddRefs(orient), SVG_MARKER_ORIENT_ANGLE, gOrientType);
  NS_ENSURE_SUCCESS(rv,rv);
  nsCOMPtr<nsIDOMSVGAnimatedEnumeration> orientType;
  rv = NS_NewSVGAnimatedEnumeration(getter_AddRefs(orientType), orient);
  NS_ENSURE_SUCCESS(rv,rv);

  nsIDOMSVGAngle *a;
  mOrient->GetBaseVal(&a);
  nsAutoString value;
  a->GetValueAsString(value);
  if (value.EqualsLiteral("auto")) {
    orientType->SetBaseVal(SVG_MARKER_ORIENT_AUTO);
  } else {
    orientType->SetBaseVal(SVG_MARKER_ORIENT_ANGLE);
  }

  *aOrientType = orientType;
  NS_IF_ADDREF(*aOrientType);
  return NS_OK;
}


NS_IMETHODIMP nsSVGMarkerElement::GetOrientAngle(nsIDOMSVGAnimatedAngle * *aOrientAngle)
{
  *aOrientAngle = mOrient;
  NS_IF_ADDREF(*aOrientAngle);
  return NS_OK;
}


NS_IMETHODIMP nsSVGMarkerElement::SetOrientToAuto()
{
  nsIDOMSVGAngle *a;
  mOrient->GetBaseVal(&a);
  a->SetValueAsString(NS_LITERAL_STRING("auto"));
  return NS_OK;
}


NS_IMETHODIMP nsSVGMarkerElement::SetOrientToAngle(nsIDOMSVGAngle *angle)
{
  if (!angle)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;

  nsIDOMSVGAngle *a;
  mOrient->GetBaseVal(&a);
  float f;
  angle->GetValue(&f);
  a->SetValue(f);
  return NS_OK;
}




NS_IMETHODIMP
nsSVGMarkerElement::DidModifySVGObservable(nsISVGValue* observable,
                                           nsISVGValue::modificationType aModType)
{
  mViewBoxToViewportTransform = nsnull;
  return NS_OK;
}




NS_IMETHODIMP_(PRBool)
nsSVGMarkerElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sFEFloodMap,
    sFiltersMap,
    sFontSpecificationMap,
    sGradientStopMap,
    sMarkersMap,
    sTextContentElementsMap,
    sViewportsMap
  };

  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGMarkerElementBase::IsAttributeMapped(name);
}




nsresult
nsSVGMarkerElement::UnsetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                              PRBool aNotify)
{
  if (aNamespaceID == kNameSpaceID_None &&
      aName == nsGkAtoms::viewBox && mCoordCtx) {
    nsCOMPtr<nsIDOMSVGRect> vb;
    mViewBox->GetAnimVal(getter_AddRefs(vb));
    vb->SetX(0);
    vb->SetY(0);
    vb->SetWidth(mLengthAttributes[MARKERWIDTH].GetAnimValue(mCoordCtx));
    vb->SetHeight(mLengthAttributes[MARKERHEIGHT].GetAnimValue(mCoordCtx));
  }

  return nsSVGMarkerElementBase::UnsetAttr(aNamespaceID, aName, aNotify);
}




void
nsSVGMarkerElement::DidChangeLength(PRUint8 aAttrEnum, PRBool aDoSetAttr)
{
  nsSVGMarkerElementBase::DidChangeLength(aAttrEnum, aDoSetAttr);

  mViewBoxToViewportTransform = nsnull;

  if (mCoordCtx && !HasAttr(kNameSpaceID_None, nsGkAtoms::viewBox) &&
      (aAttrEnum == MARKERWIDTH || aAttrEnum == MARKERHEIGHT)) {
    nsCOMPtr<nsIDOMSVGRect> vb;
    mViewBox->GetAnimVal(getter_AddRefs(vb));
    vb->SetWidth(mLengthAttributes[MARKERWIDTH].GetAnimValue(mCoordCtx));
    vb->SetHeight(mLengthAttributes[MARKERHEIGHT].GetAnimValue(mCoordCtx));
  }
}

void 
nsSVGMarkerElement::SetParentCoordCtxProvider(nsSVGSVGElement *aContext)
{
  mCoordCtx = aContext;
  mViewBoxToViewportTransform = nsnull;

  if (mCoordCtx && !HasAttr(kNameSpaceID_None, nsGkAtoms::viewBox)) {
    nsCOMPtr<nsIDOMSVGRect> vb;
    mViewBox->GetAnimVal(getter_AddRefs(vb));
    vb->SetWidth(mLengthAttributes[MARKERWIDTH].GetAnimValue(mCoordCtx));
    vb->SetHeight(mLengthAttributes[MARKERHEIGHT].GetAnimValue(mCoordCtx));
  }
}

nsSVGElement::LengthAttributesInfo
nsSVGMarkerElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
}




nsresult
nsSVGMarkerElement::GetMarkerTransform(float aStrokeWidth,
                                       float aX, float aY, float aAngle,
                                       nsIDOMSVGMatrix **_retval)
{
  float scale = 1.0;
  PRUint16 val;
  mMarkerUnits->GetAnimVal(&val);
  if (val == SVG_MARKERUNITS_STROKEWIDTH)
    scale = aStrokeWidth;

  nsCOMPtr<nsIDOMSVGAngle> a;
  mOrient->GetAnimVal(getter_AddRefs(a));
  nsAutoString value;
  a->GetValueAsString(value);
  if (!value.EqualsLiteral("auto"))
     a->GetValue(&aAngle);

  nsCOMPtr<nsIDOMSVGMatrix> matrix;
  NS_NewSVGMatrix(getter_AddRefs(matrix),
                  cos(aAngle) * scale,   sin(aAngle) * scale,
                  -sin(aAngle) * scale,  cos(aAngle) * scale,
                  aX,                    aY);
    
  *_retval = matrix;
  NS_IF_ADDREF(*_retval);
  return NS_OK;
}

nsresult
nsSVGMarkerElement::GetViewboxToViewportTransform(nsIDOMSVGMatrix **_retval)
{
  nsresult rv = NS_OK;

  if (!mViewBoxToViewportTransform) {
    float viewportWidth =
      mLengthAttributes[MARKERWIDTH].GetAnimValue(mCoordCtx);
    float viewportHeight = 
      mLengthAttributes[MARKERHEIGHT].GetAnimValue(mCoordCtx);
    
    float viewboxX, viewboxY, viewboxWidth, viewboxHeight;
    {
      nsCOMPtr<nsIDOMSVGRect> vb;
      mViewBox->GetAnimVal(getter_AddRefs(vb));
      NS_ASSERTION(vb, "could not get viewbox");
      vb->GetX(&viewboxX);
      vb->GetY(&viewboxY);
      vb->GetWidth(&viewboxWidth);
      vb->GetHeight(&viewboxHeight);
    }
    if (viewboxWidth==0.0f || viewboxHeight==0.0f) {
      NS_ERROR("XXX. We shouldn't get here. Viewbox width/height is set to 0. Need to disable display of element as per specs.");
      viewboxWidth = 1.0f;
      viewboxHeight = 1.0f;
    }

    float refX =
      mLengthAttributes[REFX].GetAnimValue(mCoordCtx);
    float refY = 
      mLengthAttributes[REFY].GetAnimValue(mCoordCtx);

    nsCOMPtr<nsIDOMSVGMatrix> vb2vp =
      nsSVGUtils::GetViewBoxTransform(viewportWidth, viewportHeight,
                                      viewboxX, viewboxY,
                                      viewboxWidth, viewboxHeight,
                                      mPreserveAspectRatio,
                                      PR_TRUE);
    NS_ENSURE_TRUE(vb2vp, NS_ERROR_OUT_OF_MEMORY);
    nsSVGUtils::TransformPoint(vb2vp, &refX, &refY);

    nsCOMPtr<nsIDOMSVGMatrix> translate;
    NS_NewSVGMatrix(getter_AddRefs(translate),
                    1.0f, 0.0f, 0.0f, 1.0f, -refX, -refY);
    NS_ENSURE_TRUE(translate, NS_ERROR_OUT_OF_MEMORY);
    translate->Multiply(vb2vp, getter_AddRefs(mViewBoxToViewportTransform));
  }

  *_retval = mViewBoxToViewportTransform;
  NS_IF_ADDREF(*_retval);
  return rv;
}


