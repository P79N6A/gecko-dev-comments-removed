




#ifndef mozilla_dom_SVGFEFloodElement_h
#define mozilla_dom_SVGFEFloodElement_h

#include "nsSVGFilters.h"

nsresult NS_NewSVGFEFloodElement(nsIContent **aResult,
                                 already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGFE SVGFEFloodElementBase;

class SVGFEFloodElement : public SVGFEFloodElementBase,
                          public nsIDOMSVGElement
{
  friend nsresult (::NS_NewSVGFEFloodElement(nsIContent **aResult,
                                             already_AddRefed<nsINodeInfo> aNodeInfo));
protected:
  SVGFEFloodElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGFEFloodElementBase(aNodeInfo)
  {
    SetIsDOMBinding();
  }
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope, bool *triedToWrap) MOZ_OVERRIDE;

public:
  virtual bool SubregionIsUnionOfRegions() { return false; }

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);

  NS_FORWARD_NSIDOMSVGELEMENT(SVGFEFloodElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual bool OperatesOnSRGB(nsSVGFilterInstance*,
                              int32_t, Image*) { return true; }

  virtual StringAttributesInfo GetStringInfo();

  enum { RESULT };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];
};

} 
} 

#endif 
