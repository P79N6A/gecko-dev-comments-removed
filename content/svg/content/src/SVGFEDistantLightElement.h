




#ifndef mozilla_dom_SVGFEDistantLightElement_h
#define mozilla_dom_SVGFEDistantLightElement_h

#include "nsSVGFilters.h"
#include "nsSVGNumber2.h"

typedef SVGFEUnstyledElement SVGFEDistantLightElementBase;

nsresult NS_NewSVGFEDistantLightElement(nsIContent **aResult,
                                        already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

class SVGFEDistantLightElement : public SVGFEDistantLightElementBase,
                                 public nsIDOMSVGElement
{
  friend nsresult (::NS_NewSVGFEDistantLightElement(nsIContent **aResult,
                                                    already_AddRefed<nsINodeInfo> aNodeInfo));
protected:
  SVGFEDistantLightElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGFEDistantLightElementBase(aNodeInfo)
  {
    SetIsDOMBinding();
  }
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  NS_FORWARD_NSIDOMSVGELEMENT(SVGFEDistantLightElementBase::)
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  already_AddRefed<nsIDOMSVGAnimatedNumber> Azimuth();
  already_AddRefed<nsIDOMSVGAnimatedNumber> Elevation();

protected:
  virtual NumberAttributesInfo GetNumberInfo();

  enum { AZIMUTH, ELEVATION };
  nsSVGNumber2 mNumberAttributes[2];
  static NumberInfo sNumberInfo[2];
};

} 
} 

#endif 
