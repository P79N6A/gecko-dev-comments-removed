




#ifndef mozilla_dom_SVGFETileElement_h
#define mozilla_dom_SVGFETileElement_h

#include "nsSVGFilters.h"

nsresult NS_NewSVGFETileElement(nsIContent **aResult,
                                already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGFE SVGFETileElementBase;

class SVGFETileElement : public SVGFETileElementBase
{
  friend nsresult (::NS_NewSVGFETileElement(nsIContent **aResult,
                                            already_AddRefed<nsINodeInfo> aNodeInfo));
protected:
  SVGFETileElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGFETileElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

public:
  virtual bool SubregionIsUnionOfRegions() { return false; }

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources);
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);
  virtual void ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance);
  virtual nsIntRect ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
          const nsSVGFilterInstance& aInstance);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  already_AddRefed<nsIDOMSVGAnimatedString> In1();

protected:
  virtual StringAttributesInfo GetStringInfo();

  enum { RESULT, IN1 };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];
};

} 
} 

#endif 
