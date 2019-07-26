




#ifndef mozilla_dom_SVGFEDistantLightElement_h
#define mozilla_dom_SVGFEDistantLightElement_h

#include "nsSVGFilters.h"
#include "nsSVGNumber2.h"

typedef SVGFEUnstyledElement SVGFEDistantLightElementBase;

nsresult NS_NewSVGFEDistantLightElement(nsIContent **aResult,
                                        already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

class SVGFEDistantLightElement : public SVGFEDistantLightElementBase
{
  friend nsresult (::NS_NewSVGFEDistantLightElement(nsIContent **aResult,
                                                    already_AddRefed<nsINodeInfo> aNodeInfo));
protected:
  SVGFEDistantLightElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGFEDistantLightElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext* aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

public:
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const MOZ_OVERRIDE;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  already_AddRefed<nsIDOMSVGAnimatedNumber> Azimuth();
  already_AddRefed<nsIDOMSVGAnimatedNumber> Elevation();

protected:
  virtual NumberAttributesInfo GetNumberInfo() MOZ_OVERRIDE;

  enum { AZIMUTH, ELEVATION };
  nsSVGNumber2 mNumberAttributes[2];
  static NumberInfo sNumberInfo[2];
};

} 
} 

#endif 
