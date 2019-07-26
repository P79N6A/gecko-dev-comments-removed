




#ifndef mozilla_dom_SVGFEImageElement_h
#define mozilla_dom_SVGFEImageElement_h

#include "nsSVGFilters.h"

class SVGFEImageFrame;

nsresult NS_NewSVGFEImageElement(nsIContent **aResult,
                                 already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGFE SVGFEImageElementBase;

class SVGFEImageElement : public SVGFEImageElementBase,
                          public nsIDOMSVGElement,
                          public nsImageLoadingContent
{
  friend class ::SVGFEImageFrame;

protected:
  friend nsresult (::NS_NewSVGFEImageElement(nsIContent **aResult,
                                             already_AddRefed<nsINodeInfo> aNodeInfo));
  SVGFEImageElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~SVGFEImageElement();
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;

public:
  virtual bool SubregionIsUnionOfRegions() { return false; }

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);

  NS_FORWARD_NSIDOMSVGELEMENT(SVGFEImageElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify);
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep, bool aNullParent);
  virtual nsEventStates IntrinsicState() const;

  NS_IMETHODIMP Notify(imgIRequest *aRequest, int32_t aType, const nsIntRect* aData);

  void MaybeLoadSVGImage();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  already_AddRefed<nsIDOMSVGAnimatedString> Href();
  already_AddRefed<DOMSVGAnimatedPreserveAspectRatio> PreserveAspectRatio();

private:
  
  void Invalidate();

  nsresult LoadSVGImage(bool aForce, bool aNotify);

protected:
  virtual bool OperatesOnSRGB(nsSVGFilterInstance*,
                                int32_t, Image*) { return true; }

  virtual SVGAnimatedPreserveAspectRatio *GetPreserveAspectRatio();
  virtual StringAttributesInfo GetStringInfo();

  enum { RESULT, HREF };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];

  SVGAnimatedPreserveAspectRatio mPreserveAspectRatio;
};

} 
} 

#endif
