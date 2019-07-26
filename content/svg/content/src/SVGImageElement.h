




#ifndef mozilla_dom_SVGImageElement_h
#define mozilla_dom_SVGImageElement_h

#include "nsIDOMSVGImageElement.h"
#include "nsIDOMSVGURIReference.h"
#include "nsImageLoadingContent.h"
#include "nsSVGLength2.h"
#include "nsSVGPathGeometryElement.h"
#include "nsSVGString.h"
#include "SVGAnimatedPreserveAspectRatio.h"

nsresult NS_NewSVGImageElement(nsIContent **aResult,
                               already_AddRefed<nsINodeInfo> aNodeInfo);

typedef nsSVGPathGeometryElement SVGImageElementBase;

class nsSVGImageFrame;

namespace mozilla {
namespace dom {
class DOMSVGAnimatedPreserveAspectRatio;

class SVGImageElement : public SVGImageElementBase,
                        public nsIDOMSVGImageElement,
                        public nsIDOMSVGURIReference,
                        public nsImageLoadingContent
{
  friend class ::nsSVGImageFrame;

protected:
  SVGImageElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~SVGImageElement();
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap) MOZ_OVERRIDE;
  friend nsresult (::NS_NewSVGImageElement(nsIContent **aResult,
                                           already_AddRefed<nsINodeInfo> aNodeInfo));

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGIMAGEELEMENT
  NS_DECL_NSIDOMSVGURIREFERENCE

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGImageElementBase::)

  
  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify);
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep, bool aNullParent);

  virtual nsEventStates IntrinsicState() const;

  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* name) const;

  
  virtual void ConstructPath(gfxContext *aCtx);

  
  virtual bool HasValidDimensions() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  nsresult CopyInnerTo(mozilla::dom::Element* aDest);

  void MaybeLoadSVGImage();

  bool IsImageSrcSetDisabled() const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  already_AddRefed<SVGAnimatedLength> X();
  already_AddRefed<SVGAnimatedLength> Y();
  already_AddRefed<SVGAnimatedLength> Width();
  already_AddRefed<SVGAnimatedLength> Height();
  already_AddRefed<DOMSVGAnimatedPreserveAspectRatio> PreserveAspectRatio();
  already_AddRefed<nsIDOMSVGAnimatedString> Href();

protected:
  nsresult LoadSVGImage(bool aForce, bool aNotify);

  virtual LengthAttributesInfo GetLengthInfo();
  virtual SVGAnimatedPreserveAspectRatio *GetPreserveAspectRatio();
  virtual StringAttributesInfo GetStringInfo();

  enum { ATTR_X, ATTR_Y, ATTR_WIDTH, ATTR_HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  SVGAnimatedPreserveAspectRatio mPreserveAspectRatio;

  enum { HREF };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];
};

} 
} 

#endif 
