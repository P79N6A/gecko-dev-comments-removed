





#ifndef mozilla_dom_SVGFEGaussianBlurElement_h
#define mozilla_dom_SVGFEGaussianBlurElement_h

#include "nsSVGFilters.h"
#include "nsSVGNumberPair.h"
#include "nsSVGString.h"

nsresult NS_NewSVGFEGaussianBlurElement(nsIContent **aResult,
                                        already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGFE SVGFEGaussianBlurElementBase;

class SVGFEGaussianBlurElement : public SVGFEGaussianBlurElementBase
{
  friend nsresult (::NS_NewSVGFEGaussianBlurElement(nsIContent **aResult,
                                                    already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));
protected:
  explicit SVGFEGaussianBlurElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : SVGFEGaussianBlurElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

public:
  virtual FilterPrimitiveDescription
    GetPrimitiveDescription(nsSVGFilterInstance* aInstance,
                            const IntRect& aFilterSubregion,
                            const nsTArray<bool>& aInputsAreTainted,
                            nsTArray<mozilla::RefPtr<SourceSurface>>& aInputImages) override;
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const override;
  virtual nsSVGString& GetResultImageName() override { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo >& aSources) override;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  
  already_AddRefed<SVGAnimatedString> In1();
  already_AddRefed<SVGAnimatedNumber> StdDeviationX();
  already_AddRefed<SVGAnimatedNumber> StdDeviationY();
  void SetStdDeviation(float stdDeviationX, float stdDeviationY);

protected:
  virtual NumberPairAttributesInfo GetNumberPairInfo() override;
  virtual StringAttributesInfo GetStringInfo() override;

  enum { STD_DEV };
  nsSVGNumberPair mNumberPairAttributes[1];
  static NumberPairInfo sNumberPairInfo[1];

  enum { RESULT, IN1 };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];
};

} 
} 

#endif 
