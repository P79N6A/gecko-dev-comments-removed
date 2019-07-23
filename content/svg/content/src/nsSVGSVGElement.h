






































#ifndef __NS_SVGSVGELEMENT_H__
#define __NS_SVGSVGELEMENT_H__

#include "nsSVGStylableElement.h"
#include "nsISVGSVGElement.h"
#include "nsIDOMSVGFitToViewBox.h"
#include "nsIDOMSVGLocatable.h"
#include "nsIDOMSVGZoomAndPan.h"
#include "nsIDOMSVGMatrix.h"
#include "nsSVGLength2.h"

#define QI_TO_NSSVGSVGELEMENT(base)                                           \
  NS_STATIC_CAST(nsSVGSVGElement*,                                            \
    NS_STATIC_CAST(nsISVGSVGElement*,                                         \
      nsCOMPtr<nsISVGSVGElement>(do_QueryInterface(base))));

typedef nsSVGStylableElement nsSVGSVGElementBase;

class nsSVGSVGElement : public nsSVGSVGElementBase,
                        public nsISVGSVGElement, 
                        public nsIDOMSVGFitToViewBox,
                        public nsIDOMSVGLocatable,
                        public nsIDOMSVGZoomAndPan
{
  friend class nsSVGOuterSVGFrame;
  friend class nsSVGInnerSVGFrame;

protected:
  friend nsresult NS_NewSVGSVGElement(nsIContent **aResult,
                                      nsINodeInfo *aNodeInfo);
  nsSVGSVGElement(nsINodeInfo* aNodeInfo);
  virtual ~nsSVGSVGElement();
  nsresult Init();
  
  
  void SetCoordCtxRect(nsIDOMSVGRect* aCtxRect);

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISVGSVGELEMENT_IID)

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGSVGELEMENT
  NS_DECL_NSIDOMSVGFITTOVIEWBOX
  NS_DECL_NSIDOMSVGLOCATABLE
  NS_DECL_NSIDOMSVGZOOMANDPAN
  
  
  NS_FORWARD_NSIDOMNODE(nsSVGSVGElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGSVGElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGSVGElementBase::)

  
  NS_IMETHOD GetCurrentScaleNumber(nsIDOMSVGNumber **aResult);
  NS_IMETHOD GetZoomAndPanEnum(nsISVGEnum **aResult);
  NS_IMETHOD SetCurrentScaleTranslate(float s, float x, float y);
  NS_IMETHOD SetCurrentTranslate(float x, float y);
  NS_IMETHOD_(void) RecordCurrentScaleTranslate();
  NS_IMETHOD_(float) GetPreviousTranslate_x();
  NS_IMETHOD_(float) GetPreviousTranslate_y();
  NS_IMETHOD_(float) GetPreviousScale();

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                const nsAString* aValue, PRBool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     nsISVGValue::modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     nsISVGValue::modificationType aModType);

  
  virtual void DidChangeLength(PRUint8 aAttrEnum, PRBool aDoSetAttr);

  
  float GetLength(PRUint8 mCtxType);
  float GetMMPerPx(PRUint8 mCtxType = 0);
  already_AddRefed<nsIDOMSVGRect> GetCtxRect();

  
  nsresult GetViewboxToViewportTransform(nsIDOMSVGMatrix **_retval);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:
  
  PRBool IsEventName(nsIAtom* aName);

  
  void GetOffsetToAncestor(nsIContent* ancestor, float &x, float &y);

  
  void InvalidateTransformNotifyFrame();

  virtual LengthAttributesInfo GetLengthInfo();

  enum { X, Y, WIDTH, HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  nsSVGSVGElement                  *mCoordCtx;
  nsCOMPtr<nsIDOMSVGAnimatedRect>   mViewBox;
  nsCOMPtr<nsIDOMSVGAnimatedPreserveAspectRatio> mPreserveAspectRatio;

  float mViewportWidth, mViewportHeight;  
  float mCoordCtxMmPerPx;

  
  
  
  nsCOMPtr<nsISVGEnum>              mZoomAndPan;
  nsCOMPtr<nsIDOMSVGPoint>          mCurrentTranslate;
  nsCOMPtr<nsIDOMSVGNumber>         mCurrentScale;
  float                             mPreviousTranslate_x;
  float                             mPreviousTranslate_y;
  float                             mPreviousScale;
  PRInt32                           mRedrawSuspendCount;
  PRPackedBool                      mDispatchEvent;
};

#endif
