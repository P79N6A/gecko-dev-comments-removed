




#ifndef mozilla_dom_SVGDefsElement_h
#define mozilla_dom_SVGDefsElement_h

#include "SVGGraphicsElement.h"

nsresult NS_NewSVGDefsElement(nsIContent **aResult,
                              already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

class SVGDefsElement MOZ_FINAL : public SVGGraphicsElement
{
protected:
  friend nsresult (::NS_NewSVGDefsElement(nsIContent **aResult,
                                          already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));
  explicit SVGDefsElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);
  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;

public:
  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const;
};

} 
} 

#endif 
