





































#ifndef __NS_SVGGRADIENTELEMENT_H__
#define __NS_SVGGRADIENTELEMENT_H__

#include "nsIDOMSVGURIReference.h"
#include "nsIDOMSVGGradientElement.h"
#include "nsIDOMSVGUnitTypes.h"
#include "nsSVGStylableElement.h"
#include "nsSVGLength2.h"
#include "nsSVGEnum.h"
#include "nsSVGString.h"
#include "SVGAnimatedTransformList.h"



typedef nsSVGStylableElement nsSVGGradientElementBase;

class nsSVGGradientElement : public nsSVGGradientElementBase,
                             public nsIDOMSVGURIReference,
                             public nsIDOMSVGUnitTypes
{
  friend class nsSVGGradientFrame;

protected:
  nsSVGGradientElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMSVGGRADIENTELEMENT

  
  NS_DECL_NSIDOMSVGURIREFERENCE

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual mozilla::SVGAnimatedTransformList* GetAnimatedTransformList();
  virtual nsIAtom* GetTransformListAttrName() const {
    return nsGkAtoms::gradientTransform;
  }

protected:
  virtual EnumAttributesInfo GetEnumInfo();
  virtual StringAttributesInfo GetStringInfo();

  enum { GRADIENTUNITS, SPREADMETHOD };
  nsSVGEnum mEnumAttributes[2];
  static nsSVGEnumMapping sSpreadMethodMap[];
  static EnumInfo sEnumInfo[2];

  enum { HREF };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];

  
  nsAutoPtr<mozilla::SVGAnimatedTransformList> mGradientTransform;
};



typedef nsSVGGradientElement nsSVGLinearGradientElementBase;

class nsSVGLinearGradientElement : public nsSVGLinearGradientElementBase,
                                   public nsIDOMSVGLinearGradientElement
{
  friend class nsSVGLinearGradientFrame;

protected:
  friend nsresult NS_NewSVGLinearGradientElement(nsIContent **aResult,
                                                 already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGLinearGradientElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGGRADIENTELEMENT(nsSVGLinearGradientElementBase::)

  
  NS_DECL_NSIDOMSVGLINEARGRADIENTELEMENT

  
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGLinearGradientElementBase::)

  NS_FORWARD_NSIDOMNODE(nsSVGLinearGradientElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGLinearGradientElementBase::)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
protected:

  virtual LengthAttributesInfo GetLengthInfo();

  
  enum { X1, Y1, X2, Y2 };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];
};



typedef nsSVGGradientElement nsSVGRadialGradientElementBase;

class nsSVGRadialGradientElement : public nsSVGRadialGradientElementBase,
                                   public nsIDOMSVGRadialGradientElement
{
  friend class nsSVGRadialGradientFrame;

protected:
  friend nsresult NS_NewSVGRadialGradientElement(nsIContent **aResult,
                                                 already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGRadialGradientElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGGRADIENTELEMENT(nsSVGRadialGradientElementBase::)

  
  NS_DECL_NSIDOMSVGRADIALGRADIENTELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGRadialGradientElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGRadialGradientElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGRadialGradientElementBase::)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
protected:

  virtual LengthAttributesInfo GetLengthInfo();

  
  enum { CX, CY, R, FX, FY };
  nsSVGLength2 mLengthAttributes[5];
  static LengthInfo sLengthInfo[5];
};

#endif
