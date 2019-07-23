






































#ifndef __NS_SVGSVGELEMENT_H__
#define __NS_SVGSVGELEMENT_H__

#include "nsSVGStylableElement.h"
#include "nsIDOMSVGSVGElement.h"
#include "nsIDOMSVGFitToViewBox.h"
#include "nsIDOMSVGLocatable.h"
#include "nsIDOMSVGZoomAndPan.h"
#include "nsIDOMSVGMatrix.h"
#include "nsSVGLength2.h"
#include "nsSVGEnum.h"
#include "nsSVGPreserveAspectRatio.h"

#define QI_AND_CAST_TO_NSSVGSVGELEMENT(base)                                  \
  (nsCOMPtr<nsIDOMSVGSVGElement>(do_QueryInterface(base)) ?                   \
   static_cast<nsSVGSVGElement*>(base.get()) : nsnull)

typedef nsSVGStylableElement nsSVGSVGElementBase;

class svgFloatSize {
public:
  svgFloatSize(float aWidth, float aHeight)
    : width(aWidth)
    , height(aHeight)
  {}
  PRBool operator!=(const svgFloatSize& rhs) {
    return width != rhs.width || height != rhs.height;
  }
  float width;
  float height;
};

class nsSVGSVGElement : public nsSVGSVGElementBase,
                        public nsIDOMSVGSVGElement,
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
  
public:

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGSVGELEMENT
  NS_DECL_NSIDOMSVGFITTOVIEWBOX
  NS_DECL_NSIDOMSVGLOCATABLE
  NS_DECL_NSIDOMSVGZOOMANDPAN
  
  
  NS_FORWARD_NSIDOMNODE(nsSVGSVGElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGSVGElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGSVGElementBase::)

  
  NS_IMETHOD GetCurrentScaleNumber(nsIDOMSVGNumber **aResult);

  




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
  virtual void DidChangeEnum(PRUint8 aAttrEnum, PRBool aDoSetAttr);
  virtual void DidChangePreserveAspectRatio(PRBool aDoSetAttr);

  
  float GetLength(PRUint8 mCtxType);
  float GetMMPerPx(PRUint8 mCtxType = 0);
  already_AddRefed<nsIDOMSVGRect> GetCtxRect();

  
  nsresult GetViewboxToViewportTransform(nsIDOMSVGMatrix **_retval);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  svgFloatSize GetViewportSize() {
    return svgFloatSize(mViewportWidth, mViewportHeight);
  }

  void SetViewportSize(svgFloatSize& aSize) {
    mViewportWidth  = aSize.width;
    mViewportHeight = aSize.height;
  }

protected:
  
  PRBool IsEventName(nsIAtom* aName);

  
  void GetOffsetToAncestor(nsIContent* ancestor, float &x, float &y);
  PRBool IsRoot() {
    NS_ASSERTION((IsInDoc() && !GetParent()) ==
                 (GetOwnerDoc() && (GetOwnerDoc()->GetRootContent() == this)),
                 "Can't determine if we're root");
    return IsInDoc() && !GetParent();
  }

  
  void InvalidateTransformNotifyFrame();

  virtual LengthAttributesInfo GetLengthInfo();

  enum { X, Y, WIDTH, HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  virtual EnumAttributesInfo GetEnumInfo();

  enum { ZOOMANDPAN };
  nsSVGEnum mEnumAttributes[1];
  static nsSVGEnumMapping sZoomAndPanMap[];
  static EnumInfo sEnumInfo[1];

  virtual nsSVGPreserveAspectRatio *GetPreserveAspectRatio();

  nsSVGPreserveAspectRatio mPreserveAspectRatio;

  nsSVGSVGElement                  *mCoordCtx;
  nsCOMPtr<nsIDOMSVGAnimatedRect>   mViewBox;

  
  
  
  
  
  
  
  
  float mViewportWidth, mViewportHeight;

  float mCoordCtxMmPerPx;

  
  
  
  nsCOMPtr<nsIDOMSVGPoint>          mCurrentTranslate;
  nsCOMPtr<nsIDOMSVGNumber>         mCurrentScale;
  float                             mPreviousTranslate_x;
  float                             mPreviousTranslate_y;
  float                             mPreviousScale;
  PRInt32                           mRedrawSuspendCount;
  PRPackedBool                      mDispatchEvent;
};

#endif
