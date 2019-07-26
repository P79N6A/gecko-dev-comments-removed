




#ifndef mozilla_dom_SVGFEMergeElement_h
#define mozilla_dom_SVGFEMergeElement_h

#include "nsSVGFilters.h"

nsresult NS_NewSVGFEMergeElement(nsIContent **aResult,
                                 already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGFE SVGFEMergeElementBase;

class SVGFEMergeElement : public SVGFEMergeElementBase,
                          public nsIDOMSVGElement
{
  friend nsresult (::NS_NewSVGFEMergeElement(nsIContent **aResult,
                                             already_AddRefed<nsINodeInfo> aNodeInfo));
protected:
  SVGFEMergeElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGFEMergeElementBase(aNodeInfo)
  {
    SetIsDOMBinding();
  }
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources);

  NS_FORWARD_NSIDOMSVGELEMENT(SVGFEMergeElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual StringAttributesInfo GetStringInfo();

  enum { RESULT };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];
};

} 
} 

#endif 
