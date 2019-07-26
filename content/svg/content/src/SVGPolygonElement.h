




#ifndef mozilla_dom_SVGPolygonElement_h
#define mozilla_dom_SVGPolygonElement_h

#include "nsSVGPolyElement.h"

nsresult NS_NewSVGPolygonElement(nsIContent **aResult,
                                 already_AddRefed<nsINodeInfo> aNodeInfo);

typedef nsSVGPolyElement SVGPolygonElementBase;

namespace mozilla {
namespace dom {

class SVGPolygonElement MOZ_FINAL : public SVGPolygonElementBase,
                                    public nsIDOMSVGElement
{
protected:
  SVGPolygonElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;
  friend nsresult (::NS_NewSVGPolygonElement(nsIContent **aResult,
                                             already_AddRefed<nsINodeInfo> aNodeInfo));

public:
  

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGPolygonElementBase::)

  
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks);
  virtual void ConstructPath(gfxContext *aCtx);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }
};

} 
} 

#endif 
