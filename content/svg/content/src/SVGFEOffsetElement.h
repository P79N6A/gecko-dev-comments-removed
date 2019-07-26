




#ifndef mozilla_dom_SVGFEOffsetElement_h
#define mozilla_dom_SVGFEOffsetElement_h

#include "nsSVGFilter.h"

namespace mozilla {
namespace dom {

typedef nsSVGFE nsSVGFEOffsetElementBase;

class nsSVGFEOffsetElement : public nsSVGFEOffsetElementBase,
                             public nsIDOMSVGFEOffsetElement
{
  friend nsresult NS_NewSVGFEOffsetElement(nsIContent **aResult,
                                           already_AddRefed<nsINodeInfo> aNodeInfo);
protected:
  nsSVGFEOffsetElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFEOffsetElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFEOffsetElementBase::)

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
  virtual void ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance);
  virtual nsIntRect ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
          const nsSVGFilterInstance& aInstance);

  
  NS_DECL_NSIDOMSVGFEOFFSETELEMENT

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFEOffsetElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  nsIntPoint GetOffset(const nsSVGFilterInstance& aInstance);

  virtual NumberAttributesInfo GetNumberInfo();
  virtual StringAttributesInfo GetStringInfo();

  enum { DX, DY };
  nsSVGNumber2 mNumberAttributes[2];
  static NumberInfo sNumberInfo[2];

  enum { RESULT, IN1 };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];
};

} 
} 

#endif 
