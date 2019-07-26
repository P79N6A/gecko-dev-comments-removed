




#ifndef __NS_SVGSVGELEMENT_H__
#define __NS_SVGSVGELEMENT_H__

#include "mozilla/dom/FromParser.h"
#include "nsIDOMSVGFitToViewBox.h"
#include "nsIDOMSVGLocatable.h"
#include "nsISVGPoint.h"
#include "nsIDOMSVGSVGElement.h"
#include "nsIDOMSVGZoomAndPan.h"
#include "nsSVGEnum.h"
#include "nsSVGLength2.h"
#include "SVGGraphicsElement.h"
#include "nsSVGViewBox.h"
#include "SVGPreserveAspectRatio.h"
#include "SVGAnimatedPreserveAspectRatio.h"
#include "mozilla/Attributes.h"

class nsSMILTimeContainer;
class nsSVGViewElement;
namespace mozilla {
  class DOMSVGMatrix;
  class SVGFragmentIdentifier;
}

typedef mozilla::dom::SVGGraphicsElement nsSVGSVGElementBase;

class nsSVGSVGElement;

class nsSVGTranslatePoint {
public:
  nsSVGTranslatePoint()
    : mX(0.0f)
    , mY(0.0f)
  {}

  nsSVGTranslatePoint(float aX, float aY)
    : mX(aX)
    , mY(aY)
  {}

  void SetX(float aX)
    { mX = aX; }
  void SetY(float aY)
    { mY = aY; }
  float GetX() const
    { return mX; }
  float GetY() const
    { return mY; }

  nsresult ToDOMVal(nsSVGSVGElement *aElement, nsISupports **aResult);

  bool operator!=(const nsSVGTranslatePoint &rhs) const {
    return mX != rhs.mX || mY != rhs.mY;
  }

private:

  struct DOMVal MOZ_FINAL : public mozilla::nsISVGPoint {
    DOMVal(nsSVGTranslatePoint* aVal, nsSVGSVGElement *aElement)
      : mozilla::nsISVGPoint(), mVal(aVal), mElement(aElement) {}

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMVal)

    
    virtual float X() { return mVal->GetX(); }
    virtual float Y() { return mVal->GetY(); }
    virtual void SetX(float aValue, mozilla::ErrorResult& rv);
    virtual void SetY(float aValue, mozilla::ErrorResult& rv);
    virtual already_AddRefed<mozilla::nsISVGPoint> MatrixTransform(mozilla::DOMSVGMatrix& matrix);

    virtual nsISupports* GetParentObject() MOZ_OVERRIDE;

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
                        public nsIDOMSVGZoomAndPan
{
  friend class nsSVGOuterSVGFrame;
  friend class nsSVGInnerSVGFrame;
  friend class nsSVGImageFrame;
  friend class mozilla::SVGFragmentIdentifier;

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
  NS_DECL_NSIDOMSVGZOOMANDPAN
  
  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
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

  virtual bool IsEventAttributeName(nsIAtom* aName) MOZ_OVERRIDE;

  
  virtual gfxMatrix PrependLocalTransformsTo(const gfxMatrix &aMatrix,
                      TransformTypes aWhich = eAllTransforms) const;
  virtual bool HasValidDimensions() const;
 
  
  float GetLength(uint8_t mCtxType);

  

  











  bool HasViewBox() const;

  






  bool ShouldSynthesizeViewBox() const;

  bool HasViewBoxOrSyntheticViewBox() const {
    return HasViewBox() || ShouldSynthesizeViewBox();
  }

  gfxMatrix GetViewBoxTransform() const;

  bool HasChildrenOnlyTransform() const {
    return mHasChildrenOnlyTransform;
  }

  void UpdateHasChildrenOnlyTransform();

  enum ChildrenOnlyTransformChangedFlags {
    eDuringReflow = 1
  };

  









  void ChildrenOnlyTransformChanged(uint32_t aFlags = 0);

  
  
  
  virtual void FlushImageTransformInvalidation();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  
  bool IsOverriddenBy(const nsAString &aViewID) const {
    return mCurrentViewID && mCurrentViewID->Equals(aViewID);
  }

  svgFloatSize GetViewportSize() const {
    return svgFloatSize(mViewportWidth, mViewportHeight);
  }

  void SetViewportSize(const svgFloatSize& aSize) {
    mViewportWidth  = aSize.width;
    mViewportHeight = aSize.height;
  }

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

private:
  

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep, bool aNullParent);

  

  nsSVGViewElement* GetCurrentViewElement() const;

  
  
  
  void SetImageOverridePreserveAspectRatio(const SVGPreserveAspectRatio& aPAR);
  void ClearImageOverridePreserveAspectRatio();

  
  bool SetPreserveAspectRatioProperty(const SVGPreserveAspectRatio& aPAR);
  const SVGPreserveAspectRatio* GetPreserveAspectRatioProperty() const;
  bool ClearPreserveAspectRatioProperty();
  bool SetViewBoxProperty(const nsSVGViewBoxRect& aViewBox);
  const nsSVGViewBoxRect* GetViewBoxProperty() const;
  bool ClearViewBoxProperty();
  bool SetZoomAndPanProperty(uint16_t aValue);
  uint16_t GetZoomAndPanProperty() const;
  bool ClearZoomAndPanProperty();

  bool IsRoot() const {
    NS_ASSERTION((IsInDoc() && !GetParent()) ==
                 (OwnerDoc() && (OwnerDoc()->GetRootElement() == this)),
                 "Can't determine if we're root");
    return IsInDoc() && !GetParent();
  }

  



  bool IsInner() const {
    const nsIContent *parent = GetFlattenedTreeParent();
    return parent && parent->IsSVG() &&
           parent->Tag() != nsGkAtoms::foreignObject;
  }

  








  bool WillBeOutermostSVG(nsIContent* aParent,
                          nsIContent* aBindingParent) const;

  
  void InvalidateTransformNotifyFrame();

  
  
  
  bool HasPreserveAspectRatio();

 




  nsSVGViewBoxRect GetViewBoxWithSynthesis(
      float aViewportWidth, float aViewportHeight) const;
  



  SVGPreserveAspectRatio GetPreserveAspectRatioWithOverride() const;

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

  nsAutoPtr<gfxMatrix>           mFragmentIdentifierTransform;
  nsAutoPtr<nsString>            mCurrentViewID;

  
  
  
  
  
  
  
  
  float mViewportWidth, mViewportHeight;

  
  
  nsAutoPtr<nsSMILTimeContainer> mTimedDocumentRoot;

  
  
  
  nsSVGTranslatePoint               mCurrentTranslate;
  float                             mCurrentScale;
  nsSVGTranslatePoint               mPreviousTranslate;
  float                             mPreviousScale;

  
  
  
  
  bool                              mStartAnimationOnBindToTree;
  bool                              mImageNeedsTransformInvalidation;
  bool                              mIsPaintingSVGImageElement;
  bool                              mHasChildrenOnlyTransform;
  bool                              mUseCurrentView;
};

#endif
