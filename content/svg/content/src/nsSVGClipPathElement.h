



































#ifndef __NS_SVGCLIPPATHELEMENT_H__
#define __NS_SVGCLIPPATHELEMENT_H__

#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGClipPathElement.h"
#include "nsSVGAnimatedEnumeration.h"

typedef nsSVGGraphicElement nsSVGClipPathElementBase;

class nsSVGClipPathElement : public nsSVGClipPathElementBase,
                             public nsIDOMSVGClipPathElement
{
  friend class nsSVGClipPathFrame;

protected:
  friend nsresult NS_NewSVGClipPathElement(nsIContent **aResult,
                                           nsINodeInfo *aNodeInfo);
  nsSVGClipPathElement(nsINodeInfo *aNodeInfo);
  nsresult Init();

public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGCLIPPATHELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGClipPathElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGClipPathElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGClipPathElementBase::)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:

  
  nsCOMPtr<nsIDOMSVGAnimatedEnumeration> mClipPathUnits;

};

#endif
