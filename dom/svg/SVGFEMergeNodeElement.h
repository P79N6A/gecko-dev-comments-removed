




#ifndef mozilla_dom_SVGFEMergeNodeElement_h
#define mozilla_dom_SVGFEMergeNodeElement_h

#include "nsSVGFilters.h"

nsresult NS_NewSVGFEMergeNodeElement(nsIContent** aResult,
                                     already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

typedef SVGFEUnstyledElement SVGFEMergeNodeElementBase;

class SVGFEMergeNodeElement : public SVGFEMergeNodeElementBase
{
  friend nsresult (::NS_NewSVGFEMergeNodeElement(nsIContent **aResult,
                                                 already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));
protected:
  explicit SVGFEMergeNodeElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : SVGFEMergeNodeElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;

public:
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const MOZ_OVERRIDE;

  const nsSVGString* GetIn1() { return &mStringAttributes[IN1]; }

  
  already_AddRefed<SVGAnimatedString> In1();

protected:
  virtual StringAttributesInfo GetStringInfo() MOZ_OVERRIDE;

  enum { IN1 };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];
};

} 
} 

#endif 
