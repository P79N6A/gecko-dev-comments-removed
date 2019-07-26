




#ifndef mozilla_dom_SVGFEMergeElement_h
#define mozilla_dom_SVGFEMergeElement_h

#include "nsSVGFilters.h"

nsresult NS_NewSVGFEMergeElement(nsIContent **aResult,
                                 already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGFE SVGFEMergeElementBase;

class SVGFEMergeElement : public SVGFEMergeElementBase
{
  friend nsresult (::NS_NewSVGFEMergeElement(nsIContent **aResult,
                                             already_AddRefed<nsINodeInfo> aNodeInfo));
protected:
  SVGFEMergeElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGFEMergeElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext *cx,
                             JS::Handle<JSObject*> scope) MOZ_OVERRIDE;

public:
  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect) MOZ_OVERRIDE;
  virtual nsSVGString& GetResultImageName() MOZ_OVERRIDE { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources) MOZ_OVERRIDE;

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;
protected:
  virtual StringAttributesInfo GetStringInfo() MOZ_OVERRIDE;

  enum { RESULT };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];
};

} 
} 

#endif 
