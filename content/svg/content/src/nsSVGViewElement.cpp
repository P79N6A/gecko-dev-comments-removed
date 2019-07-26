




#include "nsSVGViewElement.h"
#include "DOMSVGStringList.h"

using namespace mozilla;
using namespace mozilla::dom;

nsSVGElement::StringListInfo nsSVGViewElement::sStringListInfo[1] =
{
  { &nsGkAtoms::viewTarget }
};

nsSVGEnumMapping nsSVGViewElement::sZoomAndPanMap[] = {
  {&nsGkAtoms::disable, nsIDOMSVGZoomAndPan::SVG_ZOOMANDPAN_DISABLE},
  {&nsGkAtoms::magnify, nsIDOMSVGZoomAndPan::SVG_ZOOMANDPAN_MAGNIFY},
  {nullptr, 0}
};

nsSVGElement::EnumInfo nsSVGViewElement::sEnumInfo[1] =
{
  { &nsGkAtoms::zoomAndPan,
    sZoomAndPanMap,
    nsIDOMSVGZoomAndPan::SVG_ZOOMANDPAN_MAGNIFY
  }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(View)




NS_IMPL_ADDREF_INHERITED(nsSVGViewElement,nsSVGViewElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGViewElement,nsSVGViewElementBase)

DOMCI_NODE_DATA(SVGViewElement, nsSVGViewElement)

NS_INTERFACE_TABLE_HEAD(nsSVGViewElement)
  NS_NODE_INTERFACE_TABLE6(nsSVGViewElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGViewElement,
                           nsIDOMSVGFitToViewBox,
                           nsIDOMSVGZoomAndPan)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGViewElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGViewElementBase)




nsSVGViewElement::nsSVGViewElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGViewElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGViewElement)





NS_IMETHODIMP
nsSVGViewElement::GetZoomAndPan(uint16_t *aZoomAndPan)
{
  *aZoomAndPan = mEnumAttributes[ZOOMANDPAN].GetAnimValue();
  return NS_OK;
}

NS_IMETHODIMP
nsSVGViewElement::SetZoomAndPan(uint16_t aZoomAndPan)
{
  if (aZoomAndPan == nsIDOMSVGZoomAndPan::SVG_ZOOMANDPAN_DISABLE ||
      aZoomAndPan == nsIDOMSVGZoomAndPan::SVG_ZOOMANDPAN_MAGNIFY) {
    mEnumAttributes[ZOOMANDPAN].SetBaseValue(aZoomAndPan, this);
    return NS_OK;
  }

  return NS_ERROR_RANGE_ERR;
}





NS_IMETHODIMP
nsSVGViewElement::GetViewBox(nsIDOMSVGAnimatedRect * *aViewBox)
{
  return mViewBox.ToDOMAnimatedRect(aViewBox, this);
}


NS_IMETHODIMP
nsSVGViewElement::GetPreserveAspectRatio(nsISupports
                                         **aPreserveAspectRatio)
{
  nsRefPtr<DOMSVGAnimatedPreserveAspectRatio> ratio;
  mPreserveAspectRatio.ToDOMAnimatedPreserveAspectRatio(getter_AddRefs(ratio), this);
  ratio.forget(aPreserveAspectRatio);
  return NS_OK;
}





NS_IMETHODIMP nsSVGViewElement::GetViewTarget(nsIDOMSVGStringList * *aViewTarget)
{
  *aViewTarget = DOMSVGStringList::GetDOMWrapper(
                   &mStringListAttributes[VIEW_TARGET], this, false, VIEW_TARGET).get();
  return NS_OK;
}




nsSVGElement::EnumAttributesInfo
nsSVGViewElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGViewBox *
nsSVGViewElement::GetViewBox()
{
  return &mViewBox;
}

SVGAnimatedPreserveAspectRatio *
nsSVGViewElement::GetPreserveAspectRatio()
{
  return &mPreserveAspectRatio;
}

nsSVGElement::StringListAttributesInfo
nsSVGViewElement::GetStringListInfo()
{
  return StringListAttributesInfo(mStringListAttributes, sStringListInfo,
                                  ArrayLength(sStringListInfo));
}
