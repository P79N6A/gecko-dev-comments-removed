





































#ifndef __NS_SVGPATTERNELEMENT_H__
#define __NS_SVGPATTERNELEMENT_H__

#include "nsSVGStylableElement.h"
#include "nsIDOMSVGURIReference.h"
#include "nsIDOMSVGFitToViewBox.h"
#include "nsIDOMSVGPatternElement.h"
#include "nsIDOMSVGUnitTypes.h"
#include "nsSVGLength2.h"
#include "nsSVGEnum.h"
#include "nsSVGString.h"
#include "nsSVGViewBox.h"
#include "SVGAnimatedPreserveAspectRatio.h"
#include "SVGAnimatedTransformList.h"



typedef nsSVGStylableElement nsSVGPatternElementBase;

class nsSVGPatternElement : public nsSVGPatternElementBase,
                            public nsIDOMSVGURIReference,
                            public nsIDOMSVGFitToViewBox,
                            public nsIDOMSVGPatternElement,
                            public nsIDOMSVGUnitTypes
{
  friend class nsSVGPatternFrame;

protected:
  friend nsresult NS_NewSVGPatternElement(nsIContent **aResult,
                                          already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGPatternElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  typedef mozilla::SVGAnimatedPreserveAspectRatio SVGAnimatedPreserveAspectRatio;

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMSVGPATTERNELEMENT

  
  NS_DECL_NSIDOMSVGURIREFERENCE

  
  NS_DECL_NSIDOMSVGFITTOVIEWBOX

  NS_FORWARD_NSIDOMNODE(nsSVGElement::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGElement::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGElement::)

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* name) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual mozilla::SVGAnimatedTransformList* GetAnimatedTransformList();
  virtual nsIAtom* GetTransformListAttrName() const {
    return nsGkAtoms::patternTransform;
  }
protected:

  virtual LengthAttributesInfo GetLengthInfo();
  virtual EnumAttributesInfo GetEnumInfo();
  virtual nsSVGViewBox *GetViewBox();
  virtual SVGAnimatedPreserveAspectRatio *GetPreserveAspectRatio();
  virtual StringAttributesInfo GetStringInfo();

  
  enum { X, Y, WIDTH, HEIGHT };
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

#endif
