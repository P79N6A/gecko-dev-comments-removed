




#ifndef mozilla_dom_SVGEllipseElement_h
#define mozilla_dom_SVGEllipseElement_h

#include "nsSVGPathGeometryElement.h"
#include "nsIDOMSVGEllipseElement.h"
#include "nsSVGLength2.h"

nsresult NS_NewSVGEllipseElement(nsIContent **aResult,
                                 already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGPathGeometryElement SVGEllipseElementBase;

class SVGEllipseElement MOZ_FINAL : public SVGEllipseElementBase,
                                    public nsIDOMSVGEllipseElement
{
protected:
  SVGEllipseElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope, bool *triedToWrap) MOZ_OVERRIDE;
  friend nsresult (::NS_NewSVGEllipseElement(nsIContent **aResult,
                                             already_AddRefed<nsINodeInfo> aNodeInfo));

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGELLIPSEELEMENT

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGEllipseElementBase::)

  
  virtual bool HasValidDimensions() const;

  
  virtual void ConstructPath(gfxContext *aCtx);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  already_AddRefed<nsIDOMSVGAnimatedLength> Cx();
  already_AddRefed<nsIDOMSVGAnimatedLength> Cy();
  already_AddRefed<nsIDOMSVGAnimatedLength> Rx();
  already_AddRefed<nsIDOMSVGAnimatedLength> Ry();

protected:

  virtual LengthAttributesInfo GetLengthInfo();

  enum { CX, CY, RX, RY };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];
};

} 
} 

#endif 
