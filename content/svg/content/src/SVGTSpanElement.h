




#ifndef mozilla_dom_SVGTSpanElement_h
#define mozilla_dom_SVGTSpanElement_h

#include "mozilla/dom/SVGTextPositioningElement.h"

nsresult NS_NewSVGTSpanElement(nsIContent **aResult,
                               already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef SVGTextPositioningElement SVGTSpanElementBase;

class SVGTSpanElement MOZ_FINAL : public SVGTSpanElementBase
{
protected:
  friend nsresult (::NS_NewSVGTSpanElement(nsIContent **aResult,
                                           already_AddRefed<nsINodeInfo> aNodeInfo));
  SVGTSpanElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx,
                             JS::Handle<JSObject*> scope) MOZ_OVERRIDE;

public:
  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;
protected:

  
  virtual bool IsEventName(nsIAtom* aName);
};

} 
} 

#endif 
