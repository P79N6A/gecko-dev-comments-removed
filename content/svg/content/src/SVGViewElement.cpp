




#include "mozilla/dom/SVGViewElement.h"
#include "mozilla/dom/SVGViewElementBinding.h"
#include "DOMSVGStringList.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(View)

namespace mozilla {
namespace dom {

JSObject*
SVGViewElement::WrapNode(JSContext *aCx)
{
  return SVGViewElementBinding::Wrap(aCx, this);
}

nsSVGElement::StringListInfo SVGViewElement::sStringListInfo[1] =
{
  { &nsGkAtoms::viewTarget }
};

nsSVGEnumMapping SVGViewElement::sZoomAndPanMap[] = {
  {&nsGkAtoms::disable, SVG_ZOOMANDPAN_DISABLE},
  {&nsGkAtoms::magnify, SVG_ZOOMANDPAN_MAGNIFY},
  {nullptr, 0}
};

nsSVGElement::EnumInfo SVGViewElement::sEnumInfo[1] =
{
  { &nsGkAtoms::zoomAndPan,
    sZoomAndPanMap,
    SVG_ZOOMANDPAN_MAGNIFY
  }
};




SVGViewElement::SVGViewElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGViewElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGViewElement)

void
SVGViewElement::SetZoomAndPan(uint16_t aZoomAndPan, ErrorResult& rv)
{
  if (aZoomAndPan == SVG_ZOOMANDPAN_DISABLE ||
      aZoomAndPan == SVG_ZOOMANDPAN_MAGNIFY) {
    mEnumAttributes[ZOOMANDPAN].SetBaseValue(aZoomAndPan, this);
    return;
  }

  rv.Throw(NS_ERROR_RANGE_ERR);
}



already_AddRefed<SVGAnimatedRect>
SVGViewElement::ViewBox()
{
  return mViewBox.ToSVGAnimatedRect(this);
}

already_AddRefed<DOMSVGAnimatedPreserveAspectRatio>
SVGViewElement::PreserveAspectRatio()
{
  nsRefPtr<DOMSVGAnimatedPreserveAspectRatio> ratio;
  mPreserveAspectRatio.ToDOMAnimatedPreserveAspectRatio(getter_AddRefs(ratio), this);
  return ratio.forget();
}



already_AddRefed<DOMSVGStringList>
SVGViewElement::ViewTarget()
{
  return DOMSVGStringList::GetDOMWrapper(
           &mStringListAttributes[VIEW_TARGET], this, false, VIEW_TARGET);
}




nsSVGElement::EnumAttributesInfo
SVGViewElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGViewBox *
SVGViewElement::GetViewBox()
{
  return &mViewBox;
}

SVGAnimatedPreserveAspectRatio *
SVGViewElement::GetPreserveAspectRatio()
{
  return &mPreserveAspectRatio;
}

nsSVGElement::StringListAttributesInfo
SVGViewElement::GetStringListInfo()
{
  return StringListAttributesInfo(mStringListAttributes, sStringListInfo,
                                  ArrayLength(sStringListInfo));
}

} 
} 
