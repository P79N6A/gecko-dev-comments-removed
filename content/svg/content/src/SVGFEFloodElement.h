




#ifndef mozilla_dom_SVGFEFloodElement_h
#define mozilla_dom_SVGFEFloodElement_h

#include "nsSVGFilters.h"

nsresult NS_NewSVGFEFloodElement(nsIContent **aResult,
                                 already_AddRefed<nsINodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGFE SVGFEFloodElementBase;

class SVGFEFloodElement : public SVGFEFloodElementBase
{
  friend nsresult (::NS_NewSVGFEFloodElement(nsIContent **aResult,
                                             already_AddRefed<nsINodeInfo>&& aNodeInfo));
protected:
  SVGFEFloodElement(already_AddRefed<nsINodeInfo>& aNodeInfo)
    : SVGFEFloodElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext *cx) MOZ_OVERRIDE;

public:
  virtual bool SubregionIsUnionOfRegions() MOZ_OVERRIDE { return false; }

  virtual FilterPrimitiveDescription
    GetPrimitiveDescription(nsSVGFilterInstance* aInstance,
                            const IntRect& aFilterSubregion,
                            const nsTArray<bool>& aInputsAreTainted,
                            nsTArray<mozilla::RefPtr<SourceSurface>>& aInputImages) MOZ_OVERRIDE;
  virtual nsSVGString& GetResultImageName() MOZ_OVERRIDE { return mStringAttributes[RESULT]; }

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

protected:
  virtual bool ProducesSRGB() MOZ_OVERRIDE { return true; }

  virtual StringAttributesInfo GetStringInfo() MOZ_OVERRIDE;

  enum { RESULT };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];
};

} 
} 

#endif 
