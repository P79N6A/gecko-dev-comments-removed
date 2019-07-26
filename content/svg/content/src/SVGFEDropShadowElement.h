




#ifndef mozilla_dom_SVGFEDropShadowElement_h
#define mozilla_dom_SVGFEDropShadowElement_h

#include "nsSVGFilters.h"
#include "nsSVGNumber2.h"
#include "nsSVGNumberPair.h"
#include "nsSVGString.h"

nsresult NS_NewSVGFEDropShadowElement(nsIContent **aResult,
                                      already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGFE SVGFEDropShadowElementBase;

class SVGFEDropShadowElement : public SVGFEDropShadowElementBase
{
  friend nsresult (::NS_NewSVGFEDropShadowElement(nsIContent **aResult,
                                                  already_AddRefed<nsINodeInfo> aNodeInfo));
protected:
  SVGFEDropShadowElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGFEDropShadowElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext* aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

public:
  virtual FilterPrimitiveDescription
    GetPrimitiveDescription(nsSVGFilterInstance* aInstance,
                            const IntRect& aFilterSubregion,
                            const nsTArray<bool>& aInputsAreTainted,
                            nsTArray<mozilla::RefPtr<SourceSurface>>& aInputImages) MOZ_OVERRIDE;
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const MOZ_OVERRIDE;
  virtual nsSVGString& GetResultImageName() MOZ_OVERRIDE { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo >& aSources) MOZ_OVERRIDE;

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  already_AddRefed<SVGAnimatedString> In1();
  already_AddRefed<SVGAnimatedNumber> Dx();
  already_AddRefed<SVGAnimatedNumber> Dy();
  already_AddRefed<SVGAnimatedNumber> StdDeviationX();
  already_AddRefed<SVGAnimatedNumber> StdDeviationY();
  void SetStdDeviation(float stdDeviationX, float stdDeviationY);

protected:
  virtual NumberAttributesInfo GetNumberInfo() MOZ_OVERRIDE;
  virtual NumberPairAttributesInfo GetNumberPairInfo() MOZ_OVERRIDE;
  virtual StringAttributesInfo GetStringInfo() MOZ_OVERRIDE;

  enum { DX, DY };
  nsSVGNumber2 mNumberAttributes[2];
  static NumberInfo sNumberInfo[2];

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
