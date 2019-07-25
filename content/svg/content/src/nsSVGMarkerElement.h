



































#ifndef __NS_SVGMARKERELEMENT_H__
#define __NS_SVGMARKERELEMENT_H__

#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGMarkerElement.h"
#include "nsIDOMSVGFitToViewBox.h"
#include "nsSVGLength2.h"
#include "nsSVGEnum.h"
#include "nsSVGAngle.h"
#include "nsSVGViewBox.h"
#include "SVGAnimatedPreserveAspectRatio.h"

class nsSVGOrientType
{
public:
  nsSVGOrientType()
   : mAnimVal(nsIDOMSVGMarkerElement::SVG_MARKER_ORIENT_ANGLE),
     mBaseVal(nsIDOMSVGMarkerElement::SVG_MARKER_ORIENT_ANGLE) {}

  nsresult SetBaseValue(PRUint16 aValue,
                        nsSVGElement *aSVGElement);

  
  
  void SetBaseValue(PRUint16 aValue)
    { mAnimVal = mBaseVal = PRUint8(aValue); }
  
  void SetAnimValue(PRUint16 aValue)
    { mAnimVal = PRUint8(aValue); }

  PRUint16 GetBaseValue() const
    { return mBaseVal; }
  PRUint16 GetAnimValue() const
    { return mAnimVal; }

  nsresult ToDOMAnimatedEnum(nsIDOMSVGAnimatedEnumeration **aResult,
                             nsSVGElement* aSVGElement);

private:
  nsSVGEnumValue mAnimVal;
  nsSVGEnumValue mBaseVal;

  struct DOMAnimatedEnum : public nsIDOMSVGAnimatedEnumeration
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedEnum)

    DOMAnimatedEnum(nsSVGOrientType* aVal,
                    nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    nsSVGOrientType *mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(PRUint16* aResult)
      { *aResult = mVal->GetBaseValue(); return NS_OK; }
    NS_IMETHOD SetBaseVal(PRUint16 aValue)
      { return mVal->SetBaseValue(aValue, mSVGElement); }
    NS_IMETHOD GetAnimVal(PRUint16* aResult)
      { *aResult = mVal->GetAnimValue(); return NS_OK; }
  };
};

typedef nsSVGGraphicElement nsSVGMarkerElementBase;

class nsSVGMarkerElement : public nsSVGMarkerElementBase,
                           public nsIDOMSVGMarkerElement,
                           public nsIDOMSVGFitToViewBox
{
  friend class nsSVGMarkerFrame;

protected:
  friend nsresult NS_NewSVGMarkerElement(nsIContent **aResult,
                                         already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGMarkerElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  typedef mozilla::SVGAnimatedPreserveAspectRatio SVGAnimatedPreserveAspectRatio;

  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGMARKERELEMENT
  NS_DECL_NSIDOMSVGFITTOVIEWBOX

  
  NS_FORWARD_NSIDOMNODE(nsSVGElement::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGElement::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGElement::)

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* name) const;

  virtual PRBool GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                         nsAString& aResult) const;
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);

  
  gfxMatrix GetMarkerTransform(float aStrokeWidth,
                               float aX, float aY, float aAutoAngle);
  nsSVGViewBoxRect GetViewBoxRect();
  gfxMatrix GetViewBoxTransform();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  nsSVGOrientType* GetOrientType() { return &mOrientType; }

  virtual nsXPCClassInfo* GetClassInfo();
protected:

  virtual PRBool ParseAttribute(PRInt32 aNameSpaceID, nsIAtom* aName,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

  void SetParentCoordCtxProvider(nsSVGSVGElement *aContext);

  virtual LengthAttributesInfo GetLengthInfo();
  virtual AngleAttributesInfo GetAngleInfo();
  virtual EnumAttributesInfo GetEnumInfo();
  virtual nsSVGViewBox *GetViewBox();
  virtual SVGAnimatedPreserveAspectRatio *GetPreserveAspectRatio();

  enum { REFX, REFY, MARKERWIDTH, MARKERHEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  enum { MARKERUNITS };
  nsSVGEnum mEnumAttributes[1];
  static nsSVGEnumMapping sUnitsMap[];
  static EnumInfo sEnumInfo[1];

  enum { ORIENT };
  nsSVGAngle mAngleAttributes[1];
  static AngleInfo sAngleInfo[1];

  nsSVGViewBox             mViewBox;
  SVGAnimatedPreserveAspectRatio mPreserveAspectRatio;

  
  nsSVGOrientType                        mOrientType;

  nsSVGSVGElement                       *mCoordCtx;
  nsCOMPtr<nsIDOMSVGMatrix>         mViewBoxToViewportTransform;
};

#endif
