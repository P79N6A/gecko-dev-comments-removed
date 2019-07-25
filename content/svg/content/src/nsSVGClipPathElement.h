



































#ifndef __NS_SVGCLIPPATHELEMENT_H__
#define __NS_SVGCLIPPATHELEMENT_H__

#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGClipPathElement.h"
#include "nsIDOMSVGUnitTypes.h"
#include "nsSVGEnum.h"

typedef nsSVGGraphicElement nsSVGClipPathElementBase;

class nsSVGClipPathElement : public nsSVGClipPathElementBase,
                             public nsIDOMSVGClipPathElement,
                             public nsIDOMSVGUnitTypes
{
  friend class nsSVGClipPathFrame;

protected:
  friend nsresult NS_NewSVGClipPathElement(nsIContent **aResult,
                                           already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGClipPathElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGCLIPPATHELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGClipPathElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGClipPathElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGClipPathElementBase::)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
protected:

  
  enum { CLIPPATHUNITS };
  nsSVGEnum mEnumAttributes[1];
  static EnumInfo sEnumInfo[1];

  virtual EnumAttributesInfo GetEnumInfo();
};

#endif
