




#ifndef mozilla_dom_SVGStopElement_h
#define mozilla_dom_SVGStopElement_h

#include "nsSVGElement.h"
#include "nsSVGNumber2.h"

nsresult NS_NewSVGStopElement(nsIContent **aResult,
                              already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

typedef nsSVGElement SVGStopElementBase;

namespace mozilla {
namespace dom {

class SVGStopElement MOZ_FINAL : public SVGStopElementBase
{
protected:
  friend nsresult (::NS_NewSVGStopElement(nsIContent **aResult,
                                          already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));
  explicit SVGStopElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);
  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

public:
  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  already_AddRefed<SVGAnimatedNumber> Offset();

protected:

  virtual NumberAttributesInfo GetNumberInfo() MOZ_OVERRIDE;
  nsSVGNumber2 mOffset;
  static NumberInfo sNumberInfo;
};

} 
} 

#endif 
