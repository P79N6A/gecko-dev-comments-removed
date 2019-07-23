






































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
#include "nsSVGViewBox.h"
#include "nsSVGPreserveAspectRatio.h"

#ifdef MOZ_SMIL
class nsSMILTimeContainer;
#endif 

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
                                      nsINodeInfo *aNodeInfo,
                                      PRBool aFromParser);
  nsSVGSVGElement(nsINodeInfo* aNodeInfo, PRBool aFromParser);
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

  
  nsresult GetCurrentScaleNumber(nsIDOMSVGNumber **aResult);

  




  NS_IMETHOD SetCurrentScaleTranslate(float s, float x, float y);

  




  NS_IMETHOD SetCurrentTranslate(float x, float y);

  



  void RecordCurrentScaleTranslate();

  



  float GetPreviousTranslate_x() { return mPreviousTranslate_x; }
  float GetPreviousTranslate_y() { return mPreviousTranslate_y; }
  float GetPreviousScale() { return mPreviousScale; }

#ifdef MOZ_SMIL
  nsSMILTimeContainer* GetTimedDocumentRoot();
#endif 

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
#ifdef MOZ_SMIL
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
#endif 

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     nsISVGValue::modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     nsISVGValue::modificationType aModType);

  
  virtual void DidChangeLength(PRUint8 aAttrEnum, PRBool aDoSetAttr);
  virtual void DidChangeEnum(PRUint8 aAttrEnum, PRBool aDoSetAttr);
  virtual void DidChangeViewBox(PRBool aDoSetAttr);
  virtual void DidChangePreserveAspectRatio(PRBool aDoSetAttr);

  
  float GetLength(PRUint8 mCtxType);
  float GetMMPerPx(PRUint8 mCtxType = 0);

  
  nsresult GetViewboxToViewportTransform(nsIDOMSVGMatrix **_retval);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  svgFloatSize GetViewportSize() const {
    return svgFloatSize(mViewportWidth, mViewportHeight);
  }

  void SetViewportSize(const svgFloatSize& aSize) {
    mViewportWidth  = aSize.width;
    mViewportHeight = aSize.height;
  }

protected:
  
  PRBool IsEventName(nsIAtom* aName);

#ifdef MOZ_SMIL
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep, PRBool aNullParent);
#endif 

  
  void GetOffsetToAncestor(nsIContent* ancestor, float &x, float &y);

  PRBool IsRoot() {
    NS_ASSERTION((IsInDoc() && !GetParent()) ==
                 (GetOwnerDoc() && (GetOwnerDoc()->GetRootContent() == this)),
                 "Can't determine if we're root");
    return IsInDoc() && !GetParent();
  }

#ifdef MOZ_SMIL
  








  PRBool WillBeOutermostSVG(nsIContent* aParent,
                            nsIContent* aBindingParent) const;
#endif 

  
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

  virtual nsSVGViewBox *GetViewBox();
  virtual nsSVGPreserveAspectRatio *GetPreserveAspectRatio();

  nsSVGViewBox             mViewBox;
  nsSVGPreserveAspectRatio mPreserveAspectRatio;

  nsSVGSVGElement                  *mCoordCtx;

  
  
  
  
  
  
  
  
  float mViewportWidth, mViewportHeight;

  float mCoordCtxMmPerPx;

#ifdef MOZ_SMIL
  
  
  nsAutoPtr<nsSMILTimeContainer> mTimedDocumentRoot;
#endif 

  
  
  
  nsCOMPtr<nsIDOMSVGPoint>          mCurrentTranslate;
  nsCOMPtr<nsIDOMSVGNumber>         mCurrentScale;
  float                             mPreviousTranslate_x;
  float                             mPreviousTranslate_y;
  float                             mPreviousScale;
  PRInt32                           mRedrawSuspendCount;
  PRPackedBool                      mDispatchEvent;

#ifdef MOZ_SMIL
  
  
  
  
  PRPackedBool                      mStartAnimationOnBindToTree;
#endif 
};

#endif
