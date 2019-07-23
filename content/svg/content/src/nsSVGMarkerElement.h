



































#ifndef __NS_SVGMARKERELEMENT_H__
#define __NS_SVGMARKERELEMENT_H__

#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGMarkerElement.h"
#include "nsIDOMSVGFitToViewBox.h"
#include "nsSVGLength2.h"
#include "nsSVGEnum.h"
#include "nsSVGAngle.h"

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

  virtual PRBool GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                         nsAString& aResult) const;
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);
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
  virtual AngleAttributesInfo GetAngleInfo();
  virtual EnumAttributesInfo GetEnumInfo();

  enum { REFX, REFY, MARKERWIDTH, MARKERHEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  enum { MARKERUNITS, ORIENTTYPE = 0xFF };
  nsSVGEnum mEnumAttributes[1];
  static nsSVGEnumMapping sUnitsMap[];
  static EnumInfo sEnumInfo[1];

  enum { ORIENT };
  nsSVGAngle mAngleAttributes[1];
  static AngleInfo sAngleInfo[1];

  
  nsSVGEnum                              mOrientType;

  nsSVGSVGElement                       *mCoordCtx;
  nsCOMPtr<nsIDOMSVGAnimatedRect>        mViewBox;
  nsCOMPtr<nsIDOMSVGAnimatedPreserveAspectRatio> mPreserveAspectRatio;
  nsCOMPtr<nsIDOMSVGMatrix>         mViewBoxToViewportTransform;
};

#endif
