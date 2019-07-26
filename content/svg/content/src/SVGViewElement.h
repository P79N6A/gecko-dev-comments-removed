




#ifndef mozilla_dom_SVGViewElement_h
#define mozilla_dom_SVGViewElement_h

#include "nsIDOMSVGViewElement.h"
#include "nsIDOMSVGFitToViewBox.h"
#include "nsIDOMSVGZoomAndPan.h"
#include "nsSVGElement.h"
#include "nsSVGEnum.h"
#include "nsSVGViewBox.h"
#include "SVGAnimatedPreserveAspectRatio.h"
#include "SVGStringList.h"

typedef nsSVGElement SVGViewElementBase;

class nsSVGOuterSVGFrame;

nsresult NS_NewSVGViewElement(nsIContent **aResult,
                              already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
class SVGFragmentIdentifier;

namespace dom {
class SVGSVGElement;

class SVGViewElement : public SVGViewElementBase,
                       public nsIDOMSVGViewElement,
                       public nsIDOMSVGFitToViewBox,
                       public nsIDOMSVGZoomAndPan
{
protected:
  friend class mozilla::SVGFragmentIdentifier;
  friend class SVGSVGElement;
  friend class ::nsSVGOuterSVGFrame;
  SVGViewElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  friend nsresult (::NS_NewSVGViewElement(nsIContent **aResult,
                                          already_AddRefed<nsINodeInfo> aNodeInfo));
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope, bool *triedToWrap) MOZ_OVERRIDE;

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGVIEWELEMENT
  NS_DECL_NSIDOMSVGFITTOVIEWBOX
  NS_DECL_NSIDOMSVGZOOMANDPAN

  
  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGViewElementBase::)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  uint16_t ZoomAndPan() { return mEnumAttributes[ZOOMANDPAN].GetAnimValue(); }
  void SetZoomAndPan(uint16_t aZoomAndPan, ErrorResult& rv);
  already_AddRefed<nsIDOMSVGAnimatedRect> ViewBox();
  already_AddRefed<DOMSVGAnimatedPreserveAspectRatio> PreserveAspectRatio();
  already_AddRefed<nsIDOMSVGStringList> ViewTarget();

private:

  

  virtual EnumAttributesInfo GetEnumInfo();

  enum { ZOOMANDPAN };
  nsSVGEnum mEnumAttributes[1];
  static nsSVGEnumMapping sZoomAndPanMap[];
  static EnumInfo sEnumInfo[1];

  virtual nsSVGViewBox *GetViewBox();
  virtual SVGAnimatedPreserveAspectRatio *GetPreserveAspectRatio();

  nsSVGViewBox                   mViewBox;
  SVGAnimatedPreserveAspectRatio mPreserveAspectRatio;

  virtual StringListAttributesInfo GetStringListInfo();

  enum { VIEW_TARGET };
  SVGStringList mStringListAttributes[1];
  static StringListInfo sStringListInfo[1];
};

} 
} 

#endif 
