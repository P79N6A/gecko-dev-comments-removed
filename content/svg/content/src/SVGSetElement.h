




#ifndef mozilla_dom_SVGSetElement_h
#define mozilla_dom_SVGSetElement_h

#include "mozilla/dom/SVGAnimationElement.h"
#include "nsIDOMSVGSetElement.h"
#include "nsSMILSetAnimationFunction.h"

nsresult NS_NewSVGSetElement(nsIContent **aResult,
                             already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

class SVGSetElement MOZ_FINAL : public SVGAnimationElement,
                                public nsIDOMSVGSetElement
{
protected:
  SVGSetElement(already_AddRefed<nsINodeInfo> aNodeInfo);

  nsSMILSetAnimationFunction mAnimationFunction;

  friend nsresult (::NS_NewSVGSetElement(nsIContent **aResult,
                                         already_AddRefed<nsINodeInfo> aNodeInfo));

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGSETELEMENT

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGAnimationElement::)
  NS_FORWARD_NSIDOMSVGANIMATIONELEMENT(SVGAnimationElement::)

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  virtual nsSMILAnimationFunction& AnimationFunction();

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
};

} 
} 

#endif 
