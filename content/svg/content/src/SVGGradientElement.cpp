




#include "mozilla/dom/SVGGradientElement.h"

#include "DOMSVGAnimatedTransformList.h"
#include "mozilla/dom/SVGRadialGradientElementBinding.h"
#include "mozilla/dom/SVGLinearGradientElementBinding.h"
#include "mozilla/Util.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "nsIDOMSVGAnimatedEnum.h"
#include "nsSVGElement.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(LinearGradient)
NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(RadialGradient)

namespace mozilla {
namespace dom {



nsSVGEnumMapping SVGGradientElement::sSpreadMethodMap[] = {
  {&nsGkAtoms::pad, SVG_SPREADMETHOD_PAD},
  {&nsGkAtoms::reflect, SVG_SPREADMETHOD_REFLECT},
  {&nsGkAtoms::repeat, SVG_SPREADMETHOD_REPEAT},
  {nullptr, 0}
};

nsSVGElement::EnumInfo SVGGradientElement::sEnumInfo[2] =
{
  { &nsGkAtoms::gradientUnits,
    sSVGUnitTypesMap,
    SVG_UNIT_TYPE_OBJECTBOUNDINGBOX
  },
  { &nsGkAtoms::spreadMethod,
    sSpreadMethodMap,
    SVG_SPREADMETHOD_PAD
  }
};

nsSVGElement::StringInfo SVGGradientElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, true }
};




NS_IMPL_ADDREF_INHERITED(SVGGradientElement, SVGGradientElementBase)
NS_IMPL_RELEASE_INHERITED(SVGGradientElement, SVGGradientElementBase)

NS_INTERFACE_MAP_BEGIN(SVGGradientElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGUnitTypes)
NS_INTERFACE_MAP_END_INHERITING(SVGGradientElementBase)




SVGGradientElement::SVGGradientElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGGradientElementBase(aNodeInfo)
{
  SetIsDOMBinding();
}




nsSVGElement::EnumAttributesInfo
SVGGradientElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGElement::StringAttributesInfo
SVGGradientElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}

already_AddRefed<nsIDOMSVGAnimatedEnumeration>
SVGGradientElement::GradientUnits()
{
  return mEnumAttributes[GRADIENTUNITS].ToDOMAnimatedEnum(this);
}


already_AddRefed<DOMSVGAnimatedTransformList>
SVGGradientElement::GradientTransform()
{
  
  
  return DOMSVGAnimatedTransformList::GetDOMWrapper(
           GetAnimatedTransformList(DO_ALLOCATE), this);
}

already_AddRefed<nsIDOMSVGAnimatedEnumeration>
SVGGradientElement::SpreadMethod()
{
  return mEnumAttributes[SPREADMETHOD].ToDOMAnimatedEnum(this);
}

already_AddRefed<nsIDOMSVGAnimatedString>
SVGGradientElement::Href()
{
  return mStringAttributes[HREF].ToDOMAnimatedString(this);
}




NS_IMETHODIMP_(bool)
SVGGradientElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sColorMap,
    sGradientStopMap
  };

  return FindAttributeDependence(name, map) ||
    SVGGradientElementBase::IsAttributeMapped(name);
}



JSObject*
SVGLinearGradientElement::WrapNode(JSContext* aCx, JSObject* aScope)
{
  return SVGLinearGradientElementBinding::Wrap(aCx, aScope, this);
}

nsSVGElement::LengthInfo SVGLinearGradientElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x1, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::X },
  { &nsGkAtoms::y1, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::Y },
  { &nsGkAtoms::x2, 100, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::X },
  { &nsGkAtoms::y2, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::Y },
};




NS_IMPL_ISUPPORTS_INHERITED3(SVGLinearGradientElement, SVGLinearGradientElementBase,
                             nsIDOMNode,
                             nsIDOMElement, nsIDOMSVGElement)




SVGLinearGradientElement::SVGLinearGradientElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGLinearGradientElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGLinearGradientElement)



already_AddRefed<SVGAnimatedLength>
SVGLinearGradientElement::X1()
{
  return mLengthAttributes[ATTR_X1].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGLinearGradientElement::Y1()
{
  return mLengthAttributes[ATTR_Y1].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGLinearGradientElement::X2()
{
  return mLengthAttributes[ATTR_X2].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGLinearGradientElement::Y2()
{
  return mLengthAttributes[ATTR_Y2].ToDOMAnimatedLength(this);
}




SVGAnimatedTransformList*
SVGGradientElement::GetAnimatedTransformList(uint32_t aFlags)
{
  if (!mGradientTransform && (aFlags & DO_ALLOCATE)) {
    mGradientTransform = new SVGAnimatedTransformList();
  }
  return mGradientTransform;
}

nsSVGElement::LengthAttributesInfo
SVGLinearGradientElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}



JSObject*
SVGRadialGradientElement::WrapNode(JSContext* aCx, JSObject* aScope)
{
  return SVGRadialGradientElementBinding::Wrap(aCx, aScope, this);
}

nsSVGElement::LengthInfo SVGRadialGradientElement::sLengthInfo[5] =
{
  { &nsGkAtoms::cx, 50, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::X },
  { &nsGkAtoms::cy, 50, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::Y },
  { &nsGkAtoms::r, 50, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::XY },
  { &nsGkAtoms::fx, 50, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::X },
  { &nsGkAtoms::fy, 50, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::Y },
};




NS_IMPL_ISUPPORTS_INHERITED3(SVGRadialGradientElement, SVGRadialGradientElementBase,
                             nsIDOMNode,
                             nsIDOMElement, nsIDOMSVGElement)




SVGRadialGradientElement::SVGRadialGradientElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGRadialGradientElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGRadialGradientElement)



already_AddRefed<SVGAnimatedLength>
SVGRadialGradientElement::Cx()
{
  return mLengthAttributes[ATTR_CX].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGRadialGradientElement::Cy()
{
  return mLengthAttributes[ATTR_CY].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGRadialGradientElement::R()
{
  return mLengthAttributes[ATTR_R].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGRadialGradientElement::Fx()
{
  return mLengthAttributes[ATTR_FX].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGRadialGradientElement::Fy()
{
  return mLengthAttributes[ATTR_FY].ToDOMAnimatedLength(this);
}




nsSVGElement::LengthAttributesInfo
SVGRadialGradientElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}

} 
} 
