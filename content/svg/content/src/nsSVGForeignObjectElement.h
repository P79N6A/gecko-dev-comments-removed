





































#ifndef __NS_SVGFOREIGNOBJECTELEMENT_H__
#define __NS_SVGFOREIGNOBJECTELEMENT_H__

#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGForeignObjectElem.h"
#include "nsSVGLength2.h"

typedef nsSVGGraphicElement nsSVGForeignObjectElementBase;

class nsSVGForeignObjectElement : public nsSVGForeignObjectElementBase,
                                  public nsIDOMSVGForeignObjectElement
{
  friend class nsSVGForeignObjectFrame;

protected:
  friend nsresult NS_NewSVGForeignObjectElement(nsIContent **aResult,
                                                already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGForeignObjectElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGFOREIGNOBJECTELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGForeignObjectElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGForeignObjectElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGForeignObjectElementBase::)

  
  virtual gfxMatrix PrependLocalTransformTo(const gfxMatrix &aMatrix) const;

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* name) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
protected:

  virtual LengthAttributesInfo GetLengthInfo();
  
  enum { X, Y, WIDTH, HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];
};

#endif
