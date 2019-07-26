




#ifndef mozilla_dom_SVGFECompositeElement_h
#define mozilla_dom_SVGFECompositeElement_h

#include "nsSVGFilters.h"

namespace mozilla {
namespace dom {

typedef nsSVGFE nsSVGFECompositeElementBase;

class nsSVGFECompositeElement : public nsSVGFECompositeElementBase,
                                public nsIDOMSVGFECompositeElement
{
  friend nsresult NS_NewSVGFECompositeElement(nsIContent **aResult,
                                              already_AddRefed<nsINodeInfo> aNodeInfo);
protected:
  nsSVGFECompositeElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFECompositeElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFECompositeElementBase::)

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources);
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);

  
  NS_DECL_NSIDOMSVGFECOMPOSITEELEMENT

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFECompositeElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual NumberAttributesInfo GetNumberInfo();
  virtual EnumAttributesInfo GetEnumInfo();
  virtual StringAttributesInfo GetStringInfo();

  enum { K1, K2, K3, K4 };
  nsSVGNumber2 mNumberAttributes[4];
  static NumberInfo sNumberInfo[4];

  enum { OPERATOR };
  nsSVGEnum mEnumAttributes[1];
  static nsSVGEnumMapping sOperatorMap[];
  static EnumInfo sEnumInfo[1];

  enum { RESULT, IN1, IN2 };
  nsSVGString mStringAttributes[3];
  static StringInfo sStringInfo[3];
};

} 
} 

#endif 
