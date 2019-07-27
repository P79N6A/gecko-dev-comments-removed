




#ifndef mozilla_dom_SVGSymbolElement_h
#define mozilla_dom_SVGSymbolElement_h

#include "mozilla/dom/SVGTests.h"
#include "nsSVGElement.h"
#include "nsSVGViewBox.h"
#include "SVGAnimatedPreserveAspectRatio.h"

nsresult NS_NewSVGSymbolElement(nsIContent **aResult,
                                already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGElement SVGSymbolElementBase;

class SVGSymbolElement MOZ_FINAL : public SVGSymbolElementBase,
                                   public SVGTests
{
protected:
  friend nsresult (::NS_NewSVGSymbolElement(nsIContent **aResult,
                                            already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));
  explicit SVGSymbolElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);
  ~SVGSymbolElement();
  virtual JSObject* WrapNode(JSContext *cx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;

public:
  

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* name) const MOZ_OVERRIDE;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  already_AddRefed<SVGAnimatedRect> ViewBox();
  already_AddRefed<DOMSVGAnimatedPreserveAspectRatio> PreserveAspectRatio();

protected:
  virtual nsSVGViewBox *GetViewBox() MOZ_OVERRIDE;
  virtual SVGAnimatedPreserveAspectRatio *GetPreserveAspectRatio() MOZ_OVERRIDE;

  nsSVGViewBox mViewBox;
  SVGAnimatedPreserveAspectRatio mPreserveAspectRatio;
};

} 
} 

#endif 
