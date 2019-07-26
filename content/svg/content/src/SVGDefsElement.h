




#ifndef mozilla_dom_SVGDefsElement_h
#define mozilla_dom_SVGDefsElement_h

#include "SVGGraphicsElement.h"

nsresult NS_NewSVGDefsElement(nsIContent **aResult,
                              already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

class SVGDefsElement MOZ_FINAL : public SVGGraphicsElement
{
protected:
  friend nsresult (::NS_NewSVGDefsElement(nsIContent **aResult,
                                          already_AddRefed<nsINodeInfo> aNodeInfo));
  SVGDefsElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

public:

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};

} 
} 

#endif 
