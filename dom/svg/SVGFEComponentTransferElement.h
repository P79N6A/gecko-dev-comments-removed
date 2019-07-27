




#ifndef mozilla_dom_SVGFEComponentTransferElement_h
#define mozilla_dom_SVGFEComponentTransferElement_h

#include "nsSVGFilters.h"

typedef nsSVGFE SVGFEComponentTransferElementBase;

nsresult NS_NewSVGFEComponentTransferElement(nsIContent **aResult,
                                             already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

class SVGFEComponentTransferElement : public SVGFEComponentTransferElementBase
{
  friend nsresult (::NS_NewSVGFEComponentTransferElement(nsIContent **aResult,
                                                         already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));
protected:
  explicit SVGFEComponentTransferElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : SVGFEComponentTransferElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;

public:
  virtual FilterPrimitiveDescription
    GetPrimitiveDescription(nsSVGFilterInstance* aInstance,
                            const IntRect& aFilterSubregion,
                            const nsTArray<bool>& aInputsAreTainted,
                            nsTArray<mozilla::RefPtr<SourceSurface>>& aInputImages) MOZ_OVERRIDE;
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const MOZ_OVERRIDE;
  virtual nsSVGString& GetResultImageName() MOZ_OVERRIDE { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources) MOZ_OVERRIDE;

  
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  already_AddRefed<SVGAnimatedString> In1();

protected:
  virtual StringAttributesInfo GetStringInfo() MOZ_OVERRIDE;

  enum { RESULT, IN1 };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];
};

} 
} 

#endif 
