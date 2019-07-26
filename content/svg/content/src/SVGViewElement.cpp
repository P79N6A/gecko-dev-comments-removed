




#include "mozilla/dom/SVGViewElement.h"
#include "mozilla/dom/SVGViewElementBinding.h"
#include "DOMSVGStringList.h"

DOMCI_NODE_DATA(SVGViewElement, mozilla::dom::SVGViewElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(View)

namespace mozilla {
namespace dom {

JSObject*
SVGViewElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGViewElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
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




NS_IMPL_ADDREF_INHERITED(SVGViewElement,SVGViewElementBase)
NS_IMPL_RELEASE_INHERITED(SVGViewElement,SVGViewElementBase)

NS_INTERFACE_TABLE_HEAD(SVGViewElement)
  NS_NODE_INTERFACE_TABLE5(SVGViewElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGViewElement,
                           nsIDOMSVGFitToViewBox)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGViewElement)
NS_INTERFACE_MAP_END_INHERITING(SVGViewElementBase)




SVGViewElement::SVGViewElement(already_AddRefed<nsINodeInfo> aNodeInfo)
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





NS_IMETHODIMP
SVGViewElement::GetViewBox(nsIDOMSVGAnimatedRect * *aViewBox)
{
  *aViewBox = ViewBox().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedRect>
SVGViewElement::ViewBox()
{
  nsCOMPtr<nsIDOMSVGAnimatedRect> box;
  mViewBox.ToDOMAnimatedRect(getter_AddRefs(box), this);
  return box.forget();
}


NS_IMETHODIMP
SVGViewElement::GetPreserveAspectRatio(nsISupports
                                       **aPreserveAspectRatio)
{
  *aPreserveAspectRatio = PreserveAspectRatio().get();
  return NS_OK;
}

already_AddRefed<DOMSVGAnimatedPreserveAspectRatio>
SVGViewElement::PreserveAspectRatio()
{
  nsRefPtr<DOMSVGAnimatedPreserveAspectRatio> ratio;
  mPreserveAspectRatio.ToDOMAnimatedPreserveAspectRatio(getter_AddRefs(ratio), this);
  return ratio.forget();
}





NS_IMETHODIMP SVGViewElement::GetViewTarget(nsIDOMSVGStringList * *aViewTarget)
{
  *aViewTarget = ViewTarget().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGStringList>
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
