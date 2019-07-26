




#ifndef mozilla_dom_SVGPatternElement_h
#define mozilla_dom_SVGPatternElement_h

#include "nsSVGEnum.h"
#include "nsSVGLength2.h"
#include "nsSVGString.h"
#include "nsSVGElement.h"
#include "nsSVGViewBox.h"
#include "SVGAnimatedPreserveAspectRatio.h"
#include "SVGAnimatedTransformList.h"

class nsSVGPatternFrame;

nsresult NS_NewSVGPatternElement(nsIContent **aResult,
                                 already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
class DOMSVGAnimatedTransformList;

namespace dom {

typedef nsSVGElement SVGPatternElementBase;

class SVGPatternElement MOZ_FINAL : public SVGPatternElementBase,
                                    public nsIDOMSVGElement
{
  friend class ::nsSVGPatternFrame;

protected:
  friend nsresult (::NS_NewSVGPatternElement(nsIContent **aResult,
                                             already_AddRefed<nsINodeInfo> aNodeInfo));
  SVGPatternElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

public:
  typedef mozilla::SVGAnimatedPreserveAspectRatio SVGAnimatedPreserveAspectRatio;

  
  NS_DECL_ISUPPORTS_INHERITED

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGElement::)

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* name) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  virtual bool HasValidDimensions() const;

  virtual mozilla::SVGAnimatedTransformList*
    GetAnimatedTransformList(uint32_t aFlags = 0);
  virtual nsIAtom* GetTransformListAttrName() const {
    return nsGkAtoms::patternTransform;
  }

  
  already_AddRefed<nsIDOMSVGAnimatedRect> ViewBox();
  already_AddRefed<DOMSVGAnimatedPreserveAspectRatio> PreserveAspectRatio();
  already_AddRefed<nsIDOMSVGAnimatedEnumeration> PatternUnits();
  already_AddRefed<nsIDOMSVGAnimatedEnumeration> PatternContentUnits();
  already_AddRefed<DOMSVGAnimatedTransformList> PatternTransform();
  already_AddRefed<SVGAnimatedLength> X();
  already_AddRefed<SVGAnimatedLength> Y();
  already_AddRefed<SVGAnimatedLength> Width();
  already_AddRefed<SVGAnimatedLength> Height();
  already_AddRefed<nsIDOMSVGAnimatedString> Href();

protected:

  virtual LengthAttributesInfo GetLengthInfo();
  virtual EnumAttributesInfo GetEnumInfo();
  virtual nsSVGViewBox *GetViewBox();
  virtual SVGAnimatedPreserveAspectRatio *GetPreserveAspectRatio();
  virtual StringAttributesInfo GetStringInfo();

  enum { ATTR_X, ATTR_Y, ATTR_WIDTH, ATTR_HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  enum { PATTERNUNITS, PATTERNCONTENTUNITS };
  nsSVGEnum mEnumAttributes[2];
  static EnumInfo sEnumInfo[2];

  nsAutoPtr<mozilla::SVGAnimatedTransformList> mPatternTransform;

  enum { HREF };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];

  
  nsSVGViewBox mViewBox;
  SVGAnimatedPreserveAspectRatio mPreserveAspectRatio;
};

} 
} 

#endif 
