




#ifndef mozilla_dom_SVGFEImageElement_h
#define mozilla_dom_SVGFEImageElement_h

#include "nsSVGFilters.h"

namespace mozilla {
namespace dom {

typedef nsSVGFE nsSVGFEImageElementBase;

class nsSVGFEImageElement : public nsSVGFEImageElementBase,
                            public nsIDOMSVGFEImageElement,
                            public nsIDOMSVGURIReference,
                            public nsImageLoadingContent
{
  friend class SVGFEImageFrame;

protected:
  friend nsresult NS_NewSVGFEImageElement(nsIContent **aResult,
                                          already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGFEImageElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsSVGFEImageElement();

public:
  virtual bool SubregionIsUnionOfRegions() { return false; }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFEImageElementBase::)

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);

  NS_DECL_NSIDOMSVGFEIMAGEELEMENT
  NS_DECL_NSIDOMSVGURIREFERENCE

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFEImageElementBase::)

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

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
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
