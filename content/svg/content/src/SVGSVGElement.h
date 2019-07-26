




#ifndef mozilla_dom_SVGSVGElement_h
#define mozilla_dom_SVGSVGElement_h

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

nsresult NS_NewSVGSVGElement(nsIContent **aResult,
                             already_AddRefed<nsINodeInfo> aNodeInfo,
                             mozilla::dom::FromParser aFromParser);

class nsSMILTimeContainer;
class nsSVGOuterSVGFrame;
class nsSVGInnerSVGFrame;
class nsSVGImageFrame;

namespace mozilla {
class DOMSVGAnimatedPreserveAspectRatio;
class DOMSVGMatrix;
class DOMSVGTransform;
class SVGFragmentIdentifier;

namespace dom {
class SVGAngle;
class SVGViewElement;

class SVGSVGElement;

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

  nsresult ToDOMVal(SVGSVGElement *aElement, nsISupports **aResult);

  bool operator!=(const nsSVGTranslatePoint &rhs) const {
    return mX != rhs.mX || mY != rhs.mY;
  }

private:

  struct DOMVal MOZ_FINAL : public nsISVGPoint {
    DOMVal(nsSVGTranslatePoint* aVal, SVGSVGElement *aElement)
      : nsISVGPoint(), mVal(aVal), mElement(aElement) {}

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMVal)

    
    virtual float X() { return mVal->GetX(); }
    virtual float Y() { return mVal->GetY(); }
    virtual void SetX(float aValue, ErrorResult& rv);
    virtual void SetY(float aValue, ErrorResult& rv);
    virtual already_AddRefed<nsISVGPoint> MatrixTransform(DOMSVGMatrix& matrix);

    virtual nsISupports* GetParentObject() MOZ_OVERRIDE;

    nsSVGTranslatePoint *mVal; 
    nsRefPtr<SVGSVGElement> mElement;
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

typedef SVGGraphicsElement SVGSVGElementBase;

class SVGSVGElement MOZ_FINAL : public SVGSVGElementBase,
                                public nsIDOMSVGSVGElement,
                                public nsIDOMSVGFitToViewBox,
                                public nsIDOMSVGZoomAndPan
{
  friend class ::nsSVGOuterSVGFrame;
  friend class ::nsSVGInnerSVGFrame;
  friend class ::nsSVGImageFrame;
  friend class mozilla::SVGFragmentIdentifier;

  SVGSVGElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                FromParser aFromParser);
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap) MOZ_OVERRIDE;

  friend nsresult (::NS_NewSVGSVGElement(nsIContent **aResult,
                                         already_AddRefed<nsINodeInfo> aNodeInfo,
                                         mozilla::dom::FromParser aFromParser));

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SVGSVGElement, SVGSVGElementBase)
  NS_DECL_NSIDOMSVGSVGELEMENT
  NS_DECL_NSIDOMSVGFITTOVIEWBOX
  NS_DECL_NSIDOMSVGZOOMANDPAN

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGSVGElementBase::)

  




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

  
  already_AddRefed<nsIDOMSVGAnimatedLength> X();
  already_AddRefed<nsIDOMSVGAnimatedLength> Y();
  already_AddRefed<nsIDOMSVGAnimatedLength> Width();
  already_AddRefed<nsIDOMSVGAnimatedLength> Height();
  float PixelUnitToMillimeterX();
  float PixelUnitToMillimeterY();
  float ScreenPixelToMillimeterX();
  float ScreenPixelToMillimeterY();
  bool UseCurrentView();
  
  
  float CurrentScale();
  already_AddRefed<nsISVGPoint> CurrentTranslate();
  uint32_t SuspendRedraw(uint32_t max_wait_milliseconds);
  
  void ForceRedraw(ErrorResult& rv);
  void PauseAnimations(ErrorResult& rv);
  void UnpauseAnimations(ErrorResult& rv);
  bool AnimationsPaused(ErrorResult& rv);
  float GetCurrentTime(ErrorResult& rv);
  void SetCurrentTime(float seconds, ErrorResult& rv);
  already_AddRefed<nsIDOMSVGNumber> CreateSVGNumber();
  already_AddRefed<nsIDOMSVGLength> CreateSVGLength();
  already_AddRefed<SVGAngle> CreateSVGAngle();
  already_AddRefed<nsISVGPoint> CreateSVGPoint();
  already_AddRefed<DOMSVGMatrix> CreateSVGMatrix();
  already_AddRefed<nsIDOMSVGRect> CreateSVGRect();
  already_AddRefed<DOMSVGTransform> CreateSVGTransform();
  already_AddRefed<DOMSVGTransform> CreateSVGTransformFromMatrix(DOMSVGMatrix& matrix);
  Element* GetElementById(const nsAString& elementId, ErrorResult& rv);
  already_AddRefed<nsIDOMSVGAnimatedRect> ViewBox();
  already_AddRefed<DOMSVGAnimatedPreserveAspectRatio> PreserveAspectRatio();
  uint16_t ZoomAndPan();
  void SetZoomAndPan(uint16_t aZoomAndPan, ErrorResult& rv);

private:
  

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep, bool aNullParent);

  

  SVGViewElement* GetCurrentViewElement() const;

  
  
  
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

  enum { ATTR_X, ATTR_Y, ATTR_WIDTH, ATTR_HEIGHT };
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

} 
} 

#endif 
