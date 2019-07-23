




































#ifndef __NS_SVGTEXTPATHELEMENT_H__
#define __NS_SVGTEXTPATHELEMENT_H__

#include "nsSVGEnum.h"
#include "nsSVGLength2.h"

typedef nsSVGStylableElement nsSVGTextPathElementBase;

class nsSVGTextPathElement : public nsSVGTextPathElementBase,
                             public nsIDOMSVGTextPathElement,
                             public nsIDOMSVGURIReference
{
friend class nsSVGTextPathFrame;

protected:
  friend nsresult NS_NewSVGTextPathElement(nsIContent **aResult,
                                        nsINodeInfo *aNodeInfo);
  nsSVGTextPathElement(nsINodeInfo* aNodeInfo);
  nsresult Init();
  
public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGTEXTPATHELEMENT
  NS_DECL_NSIDOMSVGTEXTCONTENTELEMENT
  NS_DECL_NSIDOMSVGURIREFERENCE

  
  
  NS_FORWARD_NSIDOMNODE(nsSVGTextPathElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGTextPathElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGTextPathElementBase::)

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:

  virtual LengthAttributesInfo GetLengthInfo();
  virtual EnumAttributesInfo GetEnumInfo();

  virtual PRBool IsEventName(nsIAtom* aName);

  already_AddRefed<nsISVGTextContentMetrics> GetTextContentMetrics();

  enum { STARTOFFSET };
  nsSVGLength2 mLengthAttributes[1];
  static LengthInfo sLengthInfo[1];

  enum { METHOD, SPACING };
  nsSVGEnum mEnumAttributes[2];
  static nsSVGEnumMapping sMethodMap[];
  static nsSVGEnumMapping sSpacingMap[];
  static EnumInfo sEnumInfo[2];

  nsCOMPtr<nsIDOMSVGAnimatedString> mHref;
};

#endif
