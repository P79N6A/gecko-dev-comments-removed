




#ifndef mozilla_dom_SVGFEPointLightElement_h
#define mozilla_dom_SVGFEPointLightElement_h

#include "nsSVGFilters.h"

typedef SVGFEUnstyledElement nsSVGFEPointLightElementBase;

nsresult NS_NewSVGFEPointLightElement(nsIContent **aResult,
                                      already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

class nsSVGFEPointLightElement : public nsSVGFEPointLightElementBase,
                                 public nsIDOMSVGFEPointLightElement
{
  friend nsresult (::NS_NewSVGFEPointLightElement(nsIContent **aResult,
                                                  already_AddRefed<nsINodeInfo> aNodeInfo));
protected:
  nsSVGFEPointLightElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFEPointLightElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITEDpublic:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGFEPOINTLIGHTELEMENT

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFEPointLightElementBase::)
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual NumberAttributesInfo GetNumberInfo();

  enum { X, Y, Z };
  nsSVGNumber2 mNumberAttributes[3];
  static NumberInfo sNumberInfo[3];
};

} 
} 

#endif 
