




#ifndef mozilla_dom_SVGClipPathElement_h
#define mozilla_dom_SVGClipPathElement_h

#include "nsSVGEnum.h"
#include "mozilla/dom/SVGTransformableElement.h"

class nsSVGClipPathFrame;

nsresult NS_NewSVGClipPathElement(nsIContent **aResult,
                                  already_AddRefed<nsINodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

typedef SVGTransformableElement SVGClipPathElementBase;

class SVGClipPathElement MOZ_FINAL : public SVGClipPathElementBase
{
  friend class ::nsSVGClipPathFrame;

protected:
  friend nsresult (::NS_NewSVGClipPathElement(nsIContent **aResult,
                                              already_AddRefed<nsINodeInfo>&& aNodeInfo));
  SVGClipPathElement(already_AddRefed<nsINodeInfo>& aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx) MOZ_OVERRIDE;

public:
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  already_AddRefed<SVGAnimatedEnumeration> ClipPathUnits();

protected:

  enum { CLIPPATHUNITS };
  nsSVGEnum mEnumAttributes[1];
  static EnumInfo sEnumInfo[1];

  virtual EnumAttributesInfo GetEnumInfo() MOZ_OVERRIDE;
};

} 
} 

#endif
