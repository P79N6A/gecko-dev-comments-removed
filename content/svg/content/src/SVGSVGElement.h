




#ifndef mozilla_dom_SVGSVGElement_h
#define mozilla_dom_SVGSVGElement_h

#include "mozilla/dom/FromParser.h"
#include "nsISVGPoint.h"
#include "nsSVGEnum.h"
#include "nsSVGLength2.h"
#include "SVGGraphicsElement.h"
#include "SVGImageContext.h"
#include "nsSVGViewBox.h"
#include "SVGPreserveAspectRatio.h"
#include "SVGAnimatedPreserveAspectRatio.h"
#include "mozilla/Attributes.h"

nsresult NS_NewSVGSVGElement(nsIContent **aResult,
                             already_AddRefed<nsINodeInfo> aNodeInfo,
                             mozilla::dom::FromParser aFromParser);

class nsIDOMSVGNumber;
class nsSMILTimeContainer;
class nsSVGOuterSVGFrame;
class nsSVGInnerSVGFrame;
class nsSVGImageFrame;

namespace mozilla {
class DOMSVGAnimatedPreserveAspectRatio;
class DOMSVGTransform;
class SVGFragmentIdentifier;
class AutoSVGRenderingState;

namespace dom {
class SVGAngle;
class SVGMatrix;
class SVGViewElement;

class SVGSVGElement;

class DOMSVGTranslatePoint MOZ_FINAL : public nsISVGPoint {
public:
  DOMSVGTranslatePoint(SVGPoint* aPt, SVGSVGElement *aElement)
    : nsISVGPoint(aPt), mElement(aElement) {}

  DOMSVGTranslatePoint(DOMSVGTranslatePoint* aPt)
    : nsISVGPoint(&aPt->mPt), mElement(aPt->mElement) {}

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DOMSVGTranslatePoint, nsISVGPoint)

  virtual nsISVGPoint* Clone();

  
  virtual float X() { return mPt.GetX(); }
  virtual float Y() { return mPt.GetY(); }
  virtual void SetX(float aValue, ErrorResult& rv);
  virtual void SetY(float aValue, ErrorResult& rv);
  virtual already_AddRefed<nsISVGPoint> MatrixTransform(SVGMatrix& matrix);

  virtual nsISupports* GetParentObject() MOZ_OVERRIDE;

  nsRefPtr<SVGSVGElement> mElement;
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
                                public nsIDOMSVGElement
{
  friend class ::nsSVGOuterSVGFrame;
  friend class ::nsSVGInnerSVGFrame;
  friend class mozilla::SVGFragmentIdentifier;
  friend class mozilla::AutoSVGRenderingState;

  SVGSVGElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                FromParser aFromParser);
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;

  friend nsresult (::NS_NewSVGSVGElement(nsIContent **aResult,
                                         already_AddRefed<nsINodeInfo> aNodeInfo,
                                         mozilla::dom::FromParser aFromParser));

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SVGSVGElement, SVGSVGElementBase)

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGSVGElementBase::)

  




  void SetCurrentScaleTranslate(float s, float x, float y);

  


  const SVGPoint& GetCurrentTranslate() { return mCurrentTranslate; }
  float GetCurrentScale() { return mCurrentScale; }

  



  const SVGPoint& GetPreviousTranslate() { return mPreviousTranslate; }
  float GetPreviousScale() { return mPreviousScale; }

  nsSMILTimeContainer* GetTimedDocumentRoot();

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  virtual bool IsEventAttributeName(nsIAtom* aName) MOZ_OVERRIDE;

  
  virtual gfxMatrix PrependLocalTransformsTo(const gfxMatrix &aMatrix,
                      TransformTypes aWhich = eAllTransforms) const;
  virtual bool HasValidDimensions() const;

  
  float GetLength(uint8_t mCtxType);

  

  











  bool HasViewBoxRect() const;

  






  bool ShouldSynthesizeViewBox() const;

  bool HasViewBoxOrSyntheticViewBox() const {
    return HasViewBoxRect() || ShouldSynthesizeViewBox();
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

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  already_AddRefed<SVGAnimatedLength> X();
  already_AddRefed<SVGAnimatedLength> Y();
  already_AddRefed<SVGAnimatedLength> Width();
  already_AddRefed<SVGAnimatedLength> Height();
  float PixelUnitToMillimeterX();
  float PixelUnitToMillimeterY();
  float ScreenPixelToMillimeterX();
  float ScreenPixelToMillimeterY();
  bool UseCurrentView();
  float CurrentScale();
  void SetCurrentScale(float aCurrentScale);
  already_AddRefed<nsISVGPoint> CurrentTranslate();
  void SetCurrentTranslate(float x, float y);
  uint32_t SuspendRedraw(uint32_t max_wait_milliseconds);
  void UnsuspendRedraw(uint32_t suspend_handle_id);
  void UnsuspendRedrawAll();
  void ForceRedraw(ErrorResult& rv);
  void PauseAnimations();
  void UnpauseAnimations();
  bool AnimationsPaused();
  float GetCurrentTime();
  void SetCurrentTime(float seconds);
  already_AddRefed<nsIDOMSVGNumber> CreateSVGNumber();
  already_AddRefed<nsIDOMSVGLength> CreateSVGLength();
  already_AddRefed<SVGAngle> CreateSVGAngle();
  already_AddRefed<nsISVGPoint> CreateSVGPoint();
  already_AddRefed<SVGMatrix> CreateSVGMatrix();
  already_AddRefed<nsIDOMSVGRect> CreateSVGRect();
  already_AddRefed<DOMSVGTransform> CreateSVGTransform();
  already_AddRefed<DOMSVGTransform> CreateSVGTransformFromMatrix(SVGMatrix& matrix);
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

  
  
  
  SVGPoint mCurrentTranslate;
  float    mCurrentScale;
  SVGPoint mPreviousTranslate;
  float    mPreviousScale;

  
  
  
  
  bool     mStartAnimationOnBindToTree;
  bool     mImageNeedsTransformInvalidation;
  bool     mIsPaintingSVGImageElement;
  bool     mHasChildrenOnlyTransform;
  bool     mUseCurrentView;
};

} 



class NS_STACK_CLASS AutoSVGRenderingState
{
public:
  AutoSVGRenderingState(const SVGImageContext* aSVGContext,
                        float aFrameTime,
                        dom::SVGSVGElement* aRootElem
                        MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : mHaveOverrides(!!aSVGContext)
    , mRootElem(aRootElem)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    MOZ_ASSERT(mRootElem, "No SVG node to manage?");
    if (mHaveOverrides) {
      
      
      
      mRootElem->SetImageOverridePreserveAspectRatio(
          aSVGContext->GetPreserveAspectRatio());
    }

    mOriginalTime = mRootElem->GetCurrentTime();
    mRootElem->SetCurrentTime(aFrameTime); 
  }

  ~AutoSVGRenderingState()
  {
    mRootElem->SetCurrentTime(mOriginalTime);
    if (mHaveOverrides) {
      mRootElem->ClearImageOverridePreserveAspectRatio();
    }
  }

private:
  const bool mHaveOverrides;
  float mOriginalTime;
  const nsRefPtr<dom::SVGSVGElement> mRootElem;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 

#endif
