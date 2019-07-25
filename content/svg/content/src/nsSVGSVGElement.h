






































#ifndef __NS_SVGSVGELEMENT_H__
#define __NS_SVGSVGELEMENT_H__

#include "nsSVGStylableElement.h"
#include "nsIDOMSVGSVGElement.h"
#include "nsIDOMSVGFitToViewBox.h"
#include "nsIDOMSVGLocatable.h"
#include "nsIDOMSVGZoomAndPan.h"
#include "nsIDOMSVGMatrix.h"
#include "nsIDOMSVGPoint.h"
#include "nsSVGLength2.h"
#include "nsSVGEnum.h"
#include "nsSVGViewBox.h"
#include "SVGAnimatedPreserveAspectRatio.h"
#include "mozilla/dom/FromParser.h"

class nsSMILTimeContainer;

typedef nsSVGStylableElement nsSVGSVGElementBase;

class nsSVGSVGElement;

class nsSVGTranslatePoint {
public:
  nsSVGTranslatePoint(float aX, float aY) :
    mX(aX), mY(aY) {}

  void SetX(float aX)
    { mX = aX; }
  void SetY(float aY)
    { mY = aY; }
  float GetX() const
    { return mX; }
  float GetY() const
    { return mY; }

  nsresult ToDOMVal(nsSVGSVGElement *aElement, nsIDOMSVGPoint **aResult);

private:

  struct DOMVal : public nsIDOMSVGPoint {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMVal)

    DOMVal(nsSVGTranslatePoint* aVal, nsSVGSVGElement *aElement)
      : mVal(aVal), mElement(aElement) {}

    NS_IMETHOD GetX(float *aValue)
      { *aValue = mVal->GetX(); return NS_OK; }
    NS_IMETHOD GetY(float *aValue)
      { *aValue = mVal->GetY(); return NS_OK; }

    NS_IMETHOD SetX(float aValue);
    NS_IMETHOD SetY(float aValue);

    NS_IMETHOD MatrixTransform(nsIDOMSVGMatrix *matrix,
                               nsIDOMSVGPoint **_retval);

    nsSVGTranslatePoint *mVal; 
    nsRefPtr<nsSVGSVGElement> mElement;
  };

  float mX;
  float mY;
};

class svgFloatSize {
public:
  svgFloatSize(float aWidth, float aHeight)
    : width(aWidth)
    , height(aHeight)
  {}
  bool operator!=(const svgFloatSize& rhs) {
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
  friend class nsSVGImageFrame;

protected:
  friend nsresult NS_NewSVGSVGElement(nsIContent **aResult,
                                      already_AddRefed<nsINodeInfo> aNodeInfo,
                                      mozilla::dom::FromParser aFromParser);
  nsSVGSVGElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                  mozilla::dom::FromParser aFromParser);
  
public:
  typedef mozilla::SVGAnimatedPreserveAspectRatio SVGAnimatedPreserveAspectRatio;
  typedef mozilla::SVGPreserveAspectRatio SVGPreserveAspectRatio;

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsSVGSVGElement, nsSVGSVGElementBase)
  NS_DECL_NSIDOMSVGSVGELEMENT
  NS_DECL_NSIDOMSVGFITTOVIEWBOX
  NS_DECL_NSIDOMSVGLOCATABLE
  NS_DECL_NSIDOMSVGZOOMANDPAN
  
  
  NS_FORWARD_NSIDOMNODE(nsSVGSVGElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGSVGElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGSVGElementBase::)

  




  NS_IMETHOD SetCurrentScaleTranslate(float s, float x, float y);

  




  NS_IMETHOD SetCurrentTranslate(float x, float y);

  


  const nsSVGTranslatePoint& GetCurrentTranslate() { return mCurrentTranslate; }
  float GetCurrentScale() { return mCurrentScale; }

  



  const nsSVGTranslatePoint& GetPreviousTranslate() { return mPreviousTranslate; }
  float GetPreviousScale() { return mPreviousScale; }

  nsSMILTimeContainer* GetTimedDocumentRoot();

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  
  virtual gfxMatrix PrependLocalTransformTo(const gfxMatrix &aMatrix) const;
  
  
  float GetLength(PRUint8 mCtxType);

  
  gfxMatrix GetViewBoxTransform() const;
  bool      HasValidViewbox() const { return mViewBox.IsValid(); }

  
  
  
  virtual void FlushImageTransformInvalidation();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  svgFloatSize GetViewportSize() const {
    return svgFloatSize(mViewportWidth, mViewportHeight);
  }

  void SetViewportSize(const svgFloatSize& aSize) {
    mViewportWidth  = aSize.width;
    mViewportHeight = aSize.height;
  }

  virtual nsXPCClassInfo* GetClassInfo();

private:
  
  
  
  void SetImageOverridePreserveAspectRatio(const SVGPreserveAspectRatio& aPAR);
  void ClearImageOverridePreserveAspectRatio();
  const SVGPreserveAspectRatio* GetImageOverridePreserveAspectRatio() const;

  
  
  
  
  bool ShouldSynthesizeViewBox() const;

protected:
  
  bool IsEventName(nsIAtom* aName);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep, bool aNullParent);

  

  bool IsRoot() const {
    NS_ASSERTION((IsInDoc() && !GetParent()) ==
                 (OwnerDoc() && (OwnerDoc()->GetRootElement() == this)),
                 "Can't determine if we're root");
    return IsInDoc() && !GetParent();
  }

  



  bool IsInner() const {
    const nsIContent *parent = GetFlattenedTreeParent();
    return parent && parent->GetNameSpaceID() == kNameSpaceID_SVG &&
           parent->Tag() != nsGkAtoms::foreignObject;
  }

  








  bool WillBeOutermostSVG(nsIContent* aParent,
                            nsIContent* aBindingParent) const;

  
  void InvalidateTransformNotifyFrame();

  
  
  
  bool HasPreserveAspectRatio();

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
  virtual SVGAnimatedPreserveAspectRatio *GetPreserveAspectRatio();

  nsSVGViewBox                   mViewBox;
  SVGAnimatedPreserveAspectRatio mPreserveAspectRatio;

  nsSVGSVGElement               *mCoordCtx;

  
  
  
  
  
  
  
  
  float mViewportWidth, mViewportHeight;

  
  
  nsAutoPtr<nsSMILTimeContainer> mTimedDocumentRoot;

  
  
  
  nsSVGTranslatePoint               mCurrentTranslate;
  float                             mCurrentScale;
  nsSVGTranslatePoint               mPreviousTranslate;
  float                             mPreviousScale;
  PRInt32                           mRedrawSuspendCount;

  
  
  
  
  bool                              mStartAnimationOnBindToTree;
  bool                              mImageNeedsTransformInvalidation;
  bool                              mIsPaintingSVGImageElement;
};

#endif
