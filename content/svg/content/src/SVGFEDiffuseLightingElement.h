




#ifndef mozilla_dom_SVGFEDiffuseLightingElement_h
#define mozilla_dom_SVGFEDiffuseLightingElement_h

#include "nsSVGFilters.h"

namespace mozilla {
namespace dom {

typedef nsSVGFELightingElement nsSVGFEDiffuseLightingElementBase;

class nsSVGFEDiffuseLightingElement : public nsSVGFEDiffuseLightingElementBase,
                                      public nsIDOMSVGFEDiffuseLightingElement
{
  friend nsresult NS_NewSVGFEDiffuseLightingElement(nsIContent **aResult,
                                                    already_AddRefed<nsINodeInfo> aNodeInfo);
protected:
  nsSVGFEDiffuseLightingElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFEDiffuseLightingElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMSVGFEDIFFUSELIGHTINGELEMENT

  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFEDiffuseLightingElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFEDiffuseLightingElementBase::)
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual void LightPixel(const float *N, const float *L,
                          nscolor color, uint8_t *targetData);

};

} 
} 

#endif 
