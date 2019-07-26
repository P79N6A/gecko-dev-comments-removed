




#include "mozilla/Util.h"

#include "nsGkAtoms.h"
#include "nsCOMPtr.h"
#include "SVGAnimatedPreserveAspectRatio.h"
#include "nsError.h"
#include "mozilla/dom/SVGMarkerElement.h"
#include "gfxMatrix.h"
#include "nsContentUtils.h" 
#include "SVGContentUtils.h"
#include "SVGAngle.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Marker)

DOMCI_NODE_DATA(SVGMarkerElement, mozilla::dom::SVGMarkerElement)

namespace mozilla {
namespace dom {

nsSVGElement::LengthInfo SVGMarkerElement::sLengthInfo[4] =
{
  { &nsGkAtoms::refX, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::refY, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y },
  { &nsGkAtoms::markerWidth, 3, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::markerHeight, 3, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y },
};

nsSVGEnumMapping SVGMarkerElement::sUnitsMap[] = {
  {&nsGkAtoms::strokeWidth, nsIDOMSVGMarkerElement::SVG_MARKERUNITS_STROKEWIDTH},
  {&nsGkAtoms::userSpaceOnUse, nsIDOMSVGMarkerElement::SVG_MARKERUNITS_USERSPACEONUSE},
  {nullptr, 0}
};

nsSVGElement::EnumInfo SVGMarkerElement::sEnumInfo[1] =
{
  { &nsGkAtoms::markerUnits,
    sUnitsMap,
    nsIDOMSVGMarkerElement::SVG_MARKERUNITS_STROKEWIDTH
  }
};

nsSVGElement::AngleInfo SVGMarkerElement::sAngleInfo[1] =
{
  { &nsGkAtoms::orient, 0, SVG_ANGLETYPE_UNSPECIFIED }
};




NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGOrientType::DOMAnimatedEnum, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGOrientType::DOMAnimatedEnum)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGOrientType::DOMAnimatedEnum)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGOrientType::DOMAnimatedEnum)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedEnumeration)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimatedEnumeration)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF_INHERITED(SVGMarkerElement,SVGMarkerElementBase)
NS_IMPL_RELEASE_INHERITED(SVGMarkerElement,SVGMarkerElementBase)

NS_INTERFACE_TABLE_HEAD(SVGMarkerElement)
  NS_NODE_INTERFACE_TABLE5(SVGMarkerElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGFitToViewBox,
                           nsIDOMSVGMarkerElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGMarkerElement)
NS_INTERFACE_MAP_END_INHERITING(SVGMarkerElementBase)




nsresult
nsSVGOrientType::SetBaseValue(uint16_t aValue,
                              nsSVGElement *aSVGElement)
{
  if (aValue == nsIDOMSVGMarkerElement::SVG_MARKER_ORIENT_AUTO ||
      aValue == nsIDOMSVGMarkerElement::SVG_MARKER_ORIENT_ANGLE) {
    SetBaseValue(aValue);
    aSVGElement->SetAttr(
      kNameSpaceID_None, nsGkAtoms::orient, nullptr,
      (aValue ==nsIDOMSVGMarkerElement::SVG_MARKER_ORIENT_AUTO ?
        NS_LITERAL_STRING("auto") : NS_LITERAL_STRING("0")),
      true);
    return NS_OK;
  }
  return NS_ERROR_DOM_SYNTAX_ERR;
}

nsresult
nsSVGOrientType::ToDOMAnimatedEnum(nsIDOMSVGAnimatedEnumeration **aResult,
                                   nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimatedEnum(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}

SVGMarkerElement::SVGMarkerElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGMarkerElementBase(aNodeInfo), mCoordCtx(nullptr)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGMarkerElement)





  NS_IMETHODIMP SVGMarkerElement::GetViewBox(nsIDOMSVGAnimatedRect * *aViewBox)
{
  return mViewBox.ToDOMAnimatedRect(aViewBox, this);
}


NS_IMETHODIMP
SVGMarkerElement::GetPreserveAspectRatio(nsISupports
                                         **aPreserveAspectRatio)
{
  nsRefPtr<DOMSVGAnimatedPreserveAspectRatio> ratio;
  mPreserveAspectRatio.ToDOMAnimatedPreserveAspectRatio(getter_AddRefs(ratio), this);
  ratio.forget(aPreserveAspectRatio);
  return NS_OK;
}





NS_IMETHODIMP SVGMarkerElement::GetRefX(nsIDOMSVGAnimatedLength * *aRefX)
{
  return mLengthAttributes[REFX].ToDOMAnimatedLength(aRefX, this);
}


NS_IMETHODIMP SVGMarkerElement::GetRefY(nsIDOMSVGAnimatedLength * *aRefY)
{
  return mLengthAttributes[REFY].ToDOMAnimatedLength(aRefY, this);
}


NS_IMETHODIMP SVGMarkerElement::GetMarkerUnits(nsIDOMSVGAnimatedEnumeration * *aMarkerUnits)
{
  return mEnumAttributes[MARKERUNITS].ToDOMAnimatedEnum(aMarkerUnits, this);
}


NS_IMETHODIMP SVGMarkerElement::GetMarkerWidth(nsIDOMSVGAnimatedLength * *aMarkerWidth)
{
  return mLengthAttributes[MARKERWIDTH].ToDOMAnimatedLength(aMarkerWidth, this);
}


NS_IMETHODIMP SVGMarkerElement::GetMarkerHeight(nsIDOMSVGAnimatedLength * *aMarkerHeight)
{
  return mLengthAttributes[MARKERHEIGHT].ToDOMAnimatedLength(aMarkerHeight, this);
}


NS_IMETHODIMP SVGMarkerElement::GetOrientType(nsIDOMSVGAnimatedEnumeration * *aOrientType)
{
  return mOrientType.ToDOMAnimatedEnum(aOrientType, this);
}


NS_IMETHODIMP SVGMarkerElement::GetOrientAngle(nsISupports * *aOrientAngle)
{
  return mAngleAttributes[ORIENT].ToDOMAnimatedAngle(aOrientAngle, this);
}


NS_IMETHODIMP SVGMarkerElement::SetOrientToAuto()
{
  SetAttr(kNameSpaceID_None, nsGkAtoms::orient, nullptr,
          NS_LITERAL_STRING("auto"), true);
  return NS_OK;
}


NS_IMETHODIMP SVGMarkerElement::SetOrientToAngle(nsISupports *aAngle)
{
  nsCOMPtr<dom::SVGAngle> angle = do_QueryInterface(aAngle);
  if (!angle)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;

  float f = angle->Value();
  NS_ENSURE_FINITE(f, NS_ERROR_DOM_SVG_WRONG_TYPE_ERR);
  mAngleAttributes[ORIENT].SetBaseValue(f, this, true);

  return NS_OK;
}




NS_IMETHODIMP_(bool)
SVGMarkerElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sFEFloodMap,
    sFiltersMap,
    sFontSpecificationMap,
    sGradientStopMap,
    sLightingEffectsMap,
    sMarkersMap,
    sTextContentElementsMap,
    sViewportsMap,
    sColorMap,
    sFillStrokeMap,
    sGraphicsMap
  };

  return FindAttributeDependence(name, map) ||
    SVGMarkerElementBase::IsAttributeMapped(name);
}




bool
SVGMarkerElement::GetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                          nsAString &aResult) const
{
  if (aNameSpaceID == kNameSpaceID_None &&
      aName == nsGkAtoms::orient &&
      mOrientType.GetBaseValue() == SVG_MARKER_ORIENT_AUTO) {
    aResult.AssignLiteral("auto");
    return true;
  }
  return SVGMarkerElementBase::GetAttr(aNameSpaceID, aName, aResult);
}

bool
SVGMarkerElement::ParseAttribute(int32_t aNameSpaceID, nsIAtom* aName,
                                 const nsAString& aValue,
                                 nsAttrValue& aResult)
{
  if (aNameSpaceID == kNameSpaceID_None && aName == nsGkAtoms::orient) {
    if (aValue.EqualsLiteral("auto")) {
      mOrientType.SetBaseValue(SVG_MARKER_ORIENT_AUTO);
      aResult.SetTo(aValue);
      return true;
    }
    mOrientType.SetBaseValue(SVG_MARKER_ORIENT_ANGLE);
  }
  return SVGMarkerElementBase::ParseAttribute(aNameSpaceID, aName,
                                              aValue, aResult);
}

nsresult
SVGMarkerElement::UnsetAttr(int32_t aNamespaceID, nsIAtom* aName,
                            bool aNotify)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aName == nsGkAtoms::orient) {
      mOrientType.SetBaseValue(SVG_MARKER_ORIENT_ANGLE);
    }
  }

  return nsSVGElement::UnsetAttr(aNamespaceID, aName, aNotify);
}




void
SVGMarkerElement::SetParentCoordCtxProvider(SVGSVGElement *aContext)
{
  mCoordCtx = aContext;
  mViewBoxToViewportTransform = nullptr;
}

 bool
SVGMarkerElement::HasValidDimensions() const
{
  return (!mLengthAttributes[MARKERWIDTH].IsExplicitlySet() ||
           mLengthAttributes[MARKERWIDTH].GetAnimValInSpecifiedUnits() > 0) &&
         (!mLengthAttributes[MARKERHEIGHT].IsExplicitlySet() || 
           mLengthAttributes[MARKERHEIGHT].GetAnimValInSpecifiedUnits() > 0);
}

nsSVGElement::LengthAttributesInfo
SVGMarkerElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}

nsSVGElement::AngleAttributesInfo
SVGMarkerElement::GetAngleInfo()
{
  return AngleAttributesInfo(mAngleAttributes, sAngleInfo,
                             ArrayLength(sAngleInfo));
}

nsSVGElement::EnumAttributesInfo
SVGMarkerElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGViewBox *
SVGMarkerElement::GetViewBox()
{
  return &mViewBox;
}

SVGAnimatedPreserveAspectRatio *
SVGMarkerElement::GetPreserveAspectRatio()
{
  return &mPreserveAspectRatio;
}




gfxMatrix
SVGMarkerElement::GetMarkerTransform(float aStrokeWidth,
                                     float aX, float aY, float aAutoAngle)
{
  gfxFloat scale = mEnumAttributes[MARKERUNITS].GetAnimValue() ==
                     SVG_MARKERUNITS_STROKEWIDTH ? aStrokeWidth : 1.0;

  gfxFloat angle = mOrientType.GetAnimValue() == SVG_MARKER_ORIENT_AUTO ?
                    aAutoAngle :
                    mAngleAttributes[ORIENT].GetAnimValue() * M_PI / 180.0;

  return gfxMatrix(cos(angle) * scale,   sin(angle) * scale,
                   -sin(angle) * scale,  cos(angle) * scale,
                   aX,                    aY);
}

nsSVGViewBoxRect
SVGMarkerElement::GetViewBoxRect()
{
  if (mViewBox.IsExplicitlySet()) {
    return mViewBox.GetAnimValue();
  }
  return nsSVGViewBoxRect(
           0, 0,
           mLengthAttributes[MARKERWIDTH].GetAnimValue(mCoordCtx),
           mLengthAttributes[MARKERHEIGHT].GetAnimValue(mCoordCtx));
}

gfxMatrix
SVGMarkerElement::GetViewBoxTransform()
{
  if (!mViewBoxToViewportTransform) {
    float viewportWidth =
      mLengthAttributes[MARKERWIDTH].GetAnimValue(mCoordCtx);
    float viewportHeight = 
      mLengthAttributes[MARKERHEIGHT].GetAnimValue(mCoordCtx);
   
    nsSVGViewBoxRect viewbox = GetViewBoxRect();

    NS_ABORT_IF_FALSE(viewbox.width > 0.0f && viewbox.height > 0.0f,
                      "Rendering should be disabled");

    gfxMatrix viewBoxTM =
      SVGContentUtils::GetViewBoxTransform(this,
                                           viewportWidth, viewportHeight,
                                           viewbox.x, viewbox.y,
                                           viewbox.width, viewbox.height,
                                           mPreserveAspectRatio);

    float refX = mLengthAttributes[REFX].GetAnimValue(mCoordCtx);
    float refY = mLengthAttributes[REFY].GetAnimValue(mCoordCtx);

    gfxPoint ref = viewBoxTM.Transform(gfxPoint(refX, refY));

    gfxMatrix TM = viewBoxTM * gfxMatrix().Translate(gfxPoint(-ref.x, -ref.y));

    mViewBoxToViewportTransform = new gfxMatrix(TM);
  }

  return *mViewBoxToViewportTransform;
}

} 
} 
