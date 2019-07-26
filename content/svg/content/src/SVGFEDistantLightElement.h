




#ifndef mozilla_dom_SVGFEDistantLightElement_h
#define mozilla_dom_SVGFEDistantLightElement_h

#include "nsSVGFilters.h"

typedef SVGFEUnstyledElement nsSVGFEDistantLightElementBase;

class nsSVGFEDistantLightElement : public nsSVGFEDistantLightElementBase,
                                   public nsIDOMSVGFEDistantLightElement
{
  friend nsresult NS_NewSVGFEDistantLightElement(nsIContent **aResult,
                                                 already_AddRefed<nsINodeInfo> aNodeInfo);
protected:
  nsSVGFEDistantLightElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFEDistantLightElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGFEDISTANTLIGHTELEMENT

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFEDistantLightElementBase::)
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual NumberAttributesInfo GetNumberInfo();

  enum { AZIMUTH, ELEVATION };
  nsSVGNumber2 mNumberAttributes[2];
  static NumberInfo sNumberInfo[2];
};

#endif 
