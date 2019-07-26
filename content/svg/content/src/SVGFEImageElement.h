




#ifndef mozilla_dom_SVGFEImageElement_h
#define mozilla_dom_SVGFEImageElement_h

#include "nsSVGFilters.h"
#include "SVGAnimatedPreserveAspectRatio.h"

class SVGFEImageFrame;

nsresult NS_NewSVGFEImageElement(nsIContent **aResult,
                                 already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGFE SVGFEImageElementBase;

class SVGFEImageElement : public SVGFEImageElementBase,
                          public nsImageLoadingContent
{
  friend class ::SVGFEImageFrame;

protected:
  friend nsresult (::NS_NewSVGFEImageElement(nsIContent **aResult,
                                             already_AddRefed<nsINodeInfo> aNodeInfo));
  SVGFEImageElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~SVGFEImageElement();
  virtual JSObject* WrapNode(JSContext *aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

public:
  virtual bool SubregionIsUnionOfRegions() MOZ_OVERRIDE { return false; }

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual FilterPrimitiveDescription
    GetPrimitiveDescription(nsSVGFilterInstance* aInstance,
                            const IntRect& aFilterSubregion,
                            nsTArray<nsRefPtr<gfxASurface> >& aInputImages) MOZ_OVERRIDE;
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const MOZ_OVERRIDE;
  virtual nsSVGString& GetResultImageName() MOZ_OVERRIDE { return mStringAttributes[RESULT]; }

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify) MOZ_OVERRIDE;
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) MOZ_OVERRIDE;
  virtual void UnbindFromTree(bool aDeep, bool aNullParent) MOZ_OVERRIDE;
  virtual nsEventStates IntrinsicState() const MOZ_OVERRIDE;

  NS_IMETHODIMP Notify(imgIRequest *aRequest, int32_t aType, const nsIntRect* aData) MOZ_OVERRIDE;

  void MaybeLoadSVGImage();

  
  already_AddRefed<SVGAnimatedString> Href();
  already_AddRefed<DOMSVGAnimatedPreserveAspectRatio> PreserveAspectRatio();

private:
  
  void Invalidate();

  nsresult LoadSVGImage(bool aForce, bool aNotify);

protected:
  virtual bool ProducesSRGB() MOZ_OVERRIDE { return true; }

  virtual SVGAnimatedPreserveAspectRatio *GetPreserveAspectRatio() MOZ_OVERRIDE;
  virtual StringAttributesInfo GetStringInfo() MOZ_OVERRIDE;

  enum { RESULT, HREF };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];

  SVGAnimatedPreserveAspectRatio mPreserveAspectRatio;
};

} 
} 

#endif
