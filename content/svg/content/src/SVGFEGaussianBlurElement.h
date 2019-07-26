




#ifndef mozilla_dom_SVGFEGaussianBlurElement_h
#define mozilla_dom_SVGFEGaussianBlurElement_h

#include "nsSVGFilters.h"
#include "nsSVGNumberPair.h"
#include "nsSVGString.h"

nsresult NS_NewSVGFEGaussianBlurElement(nsIContent **aResult,
                                        already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGFE SVGFEGaussianBlurElementBase;

class SVGFEGaussianBlurElement : public SVGFEGaussianBlurElementBase
{
  friend nsresult (::NS_NewSVGFEGaussianBlurElement(nsIContent **aResult,
                                                    already_AddRefed<nsINodeInfo> aNodeInfo));
protected:
  SVGFEGaussianBlurElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGFEGaussianBlurElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

public:
  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo >& aSources);
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);
  virtual void ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance);
  virtual nsIntRect ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
          const nsSVGFilterInstance& aInstance);


  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  already_AddRefed<nsIDOMSVGAnimatedString> In1();
  already_AddRefed<nsIDOMSVGAnimatedNumber> StdDeviationX();
  already_AddRefed<nsIDOMSVGAnimatedNumber> StdDeviationY();
  void SetStdDeviation(float stdDeviationX, float stdDeviationY);

protected:
  virtual NumberPairAttributesInfo GetNumberPairInfo();
  virtual StringAttributesInfo GetStringInfo();

  enum { STD_DEV };
  nsSVGNumberPair mNumberPairAttributes[1];
  static NumberPairInfo sNumberPairInfo[1];

  enum { RESULT, IN1 };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];

private:
  nsresult GetDXY(uint32_t *aDX, uint32_t *aDY, const nsSVGFilterInstance& aInstance);
  nsIntRect InflateRectForBlur(const nsIntRect& aRect, const nsSVGFilterInstance& aInstance);

  void GaussianBlur(const Image *aSource, const Image *aTarget,
                    const nsIntRect& aDataRect,
                    uint32_t aDX, uint32_t aDY);
};

} 
} 

#endif 
