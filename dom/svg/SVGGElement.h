




#ifndef mozilla_dom_SVGGElement_h
#define mozilla_dom_SVGGElement_h

#include "mozilla/dom/SVGGraphicsElement.h"

nsresult NS_NewSVGGElement(nsIContent **aResult,
                           already_AddRefed<nsINodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

class SVGGElement MOZ_FINAL : public SVGGraphicsElement
{
protected:
  SVGGElement(already_AddRefed<nsINodeInfo>& aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx) MOZ_OVERRIDE;
  friend nsresult (::NS_NewSVGGElement(nsIContent **aResult,
                                       already_AddRefed<nsINodeInfo>&& aNodeInfo));

public:
  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};

} 
} 

#endif 
