





































#ifndef __NS_SVGGRADIENTELEMENT_H__
#define __NS_SVGGRADIENTELEMENT_H__

#include "nsIDOMSVGAnimatedEnum.h"
#include "nsIDOMSVGURIReference.h"
#include "nsIDOMSVGGradientElement.h"
#include "nsSVGStylableElement.h"
#include "nsSVGLength2.h"



typedef nsSVGStylableElement nsSVGGradientElementBase;

class nsSVGGradientElement : public nsSVGGradientElementBase,
                             public nsIDOMSVGURIReference
{
protected:
  nsSVGGradientElement(nsINodeInfo* aNodeInfo);
  nsresult Init();

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMSVGGRADIENTELEMENT

  
  NS_DECL_NSIDOMSVGURIREFERENCE

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

protected:
  
  
  nsCOMPtr<nsIDOMSVGAnimatedEnumeration> mGradientUnits;
  nsCOMPtr<nsIDOMSVGAnimatedTransformList> mGradientTransform;
  nsCOMPtr<nsIDOMSVGAnimatedEnumeration> mSpreadMethod;

  
  nsCOMPtr<nsIDOMSVGAnimatedString> mHref;
};



typedef nsSVGGradientElement nsSVGLinearGradientElementBase;

class nsSVGLinearGradientElement : public nsSVGLinearGradientElementBase,
                                   public nsIDOMSVGLinearGradientElement
{
  friend class nsSVGLinearGradientFrame;

protected:
  friend nsresult NS_NewSVGLinearGradientElement(nsIContent **aResult,
                                                 nsINodeInfo *aNodeInfo);
  nsSVGLinearGradientElement(nsINodeInfo* aNodeInfo);

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGGRADIENTELEMENT(nsSVGLinearGradientElementBase::)

  
  NS_DECL_NSIDOMSVGLINEARGRADIENTELEMENT

  
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGLinearGradientElementBase::)

  NS_FORWARD_NSIDOMNODE(nsSVGLinearGradientElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGLinearGradientElementBase::)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

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
                                                 nsINodeInfo *aNodeInfo);
  nsSVGRadialGradientElement(nsINodeInfo* aNodeInfo);

public:
  

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGGRADIENTELEMENT(nsSVGRadialGradientElementBase::)

  
  NS_DECL_NSIDOMSVGRADIALGRADIENTELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGRadialGradientElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGRadialGradientElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGRadialGradientElementBase::)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:

  virtual LengthAttributesInfo GetLengthInfo();

  
  enum { CX, CY, R, FX, FY };
  nsSVGLength2 mLengthAttributes[5];
  static LengthInfo sLengthInfo[5];
};

#endif
