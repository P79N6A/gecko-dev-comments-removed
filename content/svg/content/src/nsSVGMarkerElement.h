



































#ifndef __NS_SVGMARKERELEMENT_H__
#define __NS_SVGMARKERELEMENT_H__

#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGMarkerElement.h"
#include "nsIDOMSVGFitToViewBox.h"
#include "nsSVGLength2.h"

typedef nsSVGGraphicElement nsSVGMarkerElementBase;

class nsSVGMarkerElement : public nsSVGMarkerElementBase,
                           public nsIDOMSVGMarkerElement,
                           public nsIDOMSVGFitToViewBox
{
  friend class nsSVGMarkerFrame;

protected:
  friend nsresult NS_NewSVGMarkerElement(nsIContent **aResult,
                                         nsINodeInfo *aNodeInfo);
  nsSVGMarkerElement(nsINodeInfo* aNodeInfo);
  nsresult Init();

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGMARKERELEMENT
  NS_DECL_NSIDOMSVGFITTOVIEWBOX

  
  NS_FORWARD_NSIDOMNODE(nsSVGElement::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGElement::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGElement::)

  
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     nsISVGValue::modificationType aModType);

  
  NS_IMETHODIMP_(PRBool) IsAttributeMapped(const nsIAtom* name) const;

  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);

  
  virtual void DidChangeLength(PRUint8 aAttrEnum, PRBool aDoSetAttr);

  
  nsresult GetMarkerTransform(float aStrokeWidth,
                              float aX, float aY, float aAngle,
                              nsIDOMSVGMatrix **_retval);
  nsresult GetViewboxToViewportTransform(nsIDOMSVGMatrix **_retval);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:

  void SetParentCoordCtxProvider(nsSVGSVGElement *aContext);

  virtual LengthAttributesInfo GetLengthInfo();
 
  enum { REFX, REFY, MARKERWIDTH, MARKERHEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  nsSVGSVGElement                       *mCoordCtx;
  nsCOMPtr<nsIDOMSVGAnimatedEnumeration> mMarkerUnits;
  nsCOMPtr<nsIDOMSVGAnimatedAngle>       mOrient;

  nsCOMPtr<nsIDOMSVGAnimatedRect>        mViewBox;
  nsCOMPtr<nsIDOMSVGAnimatedPreserveAspectRatio> mPreserveAspectRatio;
  nsCOMPtr<nsIDOMSVGMatrix>         mViewBoxToViewportTransform;
};

#endif
