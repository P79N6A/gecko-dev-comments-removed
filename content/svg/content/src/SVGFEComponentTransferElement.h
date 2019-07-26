




#ifndef mozilla_dom_SVGFEComponentTransferElement_h
#define mozilla_dom_SVGFEComponentTransferElement_h

#include "nsSVGFilters.h"

typedef nsSVGFE nsSVGFEComponentTransferElementBase;

nsresult NS_NewSVGFEComponentTransferElement(nsIContent **aResult,
                                             already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

class nsSVGFEComponentTransferElement : public nsSVGFEComponentTransferElementBase,
                                        public nsIDOMSVGFEComponentTransferElement
{
  friend nsresult (::NS_NewSVGFEComponentTransferElement(nsIContent **aResult,
                                                         already_AddRefed<nsINodeInfo> aNodeInfo));
protected:
  nsSVGFEComponentTransferElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFEComponentTransferElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFEComponentTransferElementBase::)

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources);

  
  NS_DECL_NSIDOMSVGFECOMPONENTTRANSFERELEMENT

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFEComponentTransferElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual bool OperatesOnPremultipledAlpha(int32_t) { return false; }

  virtual StringAttributesInfo GetStringInfo();

  enum { RESULT, IN1 };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];
};

} 
} 

#endif 
