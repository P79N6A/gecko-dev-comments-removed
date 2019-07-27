





#ifndef mozilla_dom_SVGFETurbulenceElement_h
#define mozilla_dom_SVGFETurbulenceElement_h

#include "nsSVGEnum.h"
#include "nsSVGFilters.h"
#include "nsSVGNumber2.h"
#include "nsSVGInteger.h"
#include "nsSVGString.h"

nsresult NS_NewSVGFETurbulenceElement(nsIContent **aResult,
                                      already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGFE SVGFETurbulenceElementBase;

class SVGFETurbulenceElement : public SVGFETurbulenceElementBase
{
  friend nsresult (::NS_NewSVGFETurbulenceElement(nsIContent **aResult,
                                                  already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));
protected:
  explicit SVGFETurbulenceElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : SVGFETurbulenceElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

public:
  virtual bool SubregionIsUnionOfRegions() override { return false; }

  virtual FilterPrimitiveDescription
    GetPrimitiveDescription(nsSVGFilterInstance* aInstance,
                            const IntRect& aFilterSubregion,
                            const nsTArray<bool>& aInputsAreTainted,
                            nsTArray<mozilla::RefPtr<SourceSurface>>& aInputImages) override;
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const override;
  virtual nsSVGString& GetResultImageName() override { return mStringAttributes[RESULT]; }

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  
  already_AddRefed<SVGAnimatedNumber> BaseFrequencyX();
  already_AddRefed<SVGAnimatedNumber> BaseFrequencyY();
  already_AddRefed<SVGAnimatedInteger> NumOctaves();
  already_AddRefed<SVGAnimatedNumber> Seed();
  already_AddRefed<SVGAnimatedEnumeration> StitchTiles();
  already_AddRefed<SVGAnimatedEnumeration> Type();

protected:
  virtual NumberAttributesInfo GetNumberInfo() override;
  virtual NumberPairAttributesInfo GetNumberPairInfo() override;
  virtual IntegerAttributesInfo GetIntegerInfo() override;
  virtual EnumAttributesInfo GetEnumInfo() override;
  virtual StringAttributesInfo GetStringInfo() override;

  enum { SEED }; 
  nsSVGNumber2 mNumberAttributes[1];
  static NumberInfo sNumberInfo[1];

  enum { BASE_FREQ };
  nsSVGNumberPair mNumberPairAttributes[1];
  static NumberPairInfo sNumberPairInfo[1];

  enum { OCTAVES };
  nsSVGInteger mIntegerAttributes[1];
  static IntegerInfo sIntegerInfo[1];

  enum { TYPE, STITCHTILES };
  nsSVGEnum mEnumAttributes[2];
  static nsSVGEnumMapping sTypeMap[];
  static nsSVGEnumMapping sStitchTilesMap[];
  static EnumInfo sEnumInfo[2];

  enum { RESULT };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];
};

} 
} 

#endif 
