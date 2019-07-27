




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
                             already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                             mozilla::dom::FromParser aFromParser);

class nsSMILTimeContainer;
class nsSVGOuterSVGFrame;
class nsSVGInnerSVGFrame;

namespace mozilla {
class AutoSVGRenderingState;
class DOMSVGAnimatedPreserveAspectRatio;
class DOMSVGLength;
class DOMSVGNumber;
class EventChainPreVisitor;
class SVGFragmentIdentifier;

namespace dom {
class SVGAngle;
class SVGAnimatedRect;
class SVGMatrix;
class SVGTransform;
class SVGViewElement;
class SVGIRect;

class SVGSVGElement;

class DOMSVGTranslatePoint final : public nsISVGPoint {
public:
  DOMSVGTranslatePoint(SVGPoint* aPt, SVGSVGElement *aElement)
    : nsISVGPoint(aPt, true), mElement(aElement) {}

  explicit DOMSVGTranslatePoint(DOMSVGTranslatePoint* aPt)
    : nsISVGPoint(&aPt->mPt, true), mElement(aPt->mElement) {}

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DOMSVGTranslatePoint, nsISVGPoint)

  virtual DOMSVGPoint* Copy() override;

  
  virtual float X() override { return mPt.GetX(); }
  virtual float Y() override { return mPt.GetY(); }
  virtual void SetX(float aValue, ErrorResult& rv) override;
  virtual void SetY(float aValue, ErrorResult& rv) override;
  virtual already_AddRefed<nsISVGPoint> MatrixTransform(SVGMatrix& matrix) override;

  virtual nsISupports* GetParentObject() override;

  nsRefPtr<SVGSVGElement> mElement;

private:
  ~DOMSVGTranslatePoint() {}
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

class SVGSVGElement final : public SVGSVGElementBase
{
  friend class ::nsSVGOuterSVGFrame;
  friend class ::nsSVGInnerSVGFrame;
  friend class mozilla::SVGFragmentIdentifier;
  friend class mozilla::AutoSVGRenderingState;

  SVGSVGElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
                FromParser aFromParser);
  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

  friend nsresult (::NS_NewSVGSVGElement(nsIContent **aResult,
                                         already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                                         mozilla::dom::FromParser aFromParser));

  ~SVGSVGElement();

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SVGSVGElement, SVGSVGElementBase)

  




  void SetCurrentScaleTranslate(float s, float x, float y);

  


  const SVGPoint& GetCurrentTranslate() { return mCurrentTranslate; }
  float GetCurrentScale() { return mCurrentScale; }

  



  const SVGPoint& GetPreviousTranslate() { return mPreviousTranslate; }
  float GetPreviousScale() { return mPreviousScale; }

  nsSMILTimeContainer* GetTimedDocumentRoot();

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const override;
  virtual nsresult PreHandleEvent(EventChainPreVisitor& aVisitor) override;

  virtual bool IsEventAttributeName(nsIAtom* aName) override;

  
  virtual gfxMatrix PrependLocalTransformsTo(const gfxMatrix &aMatrix,
                      TransformTypes aWhich = eAllTransforms) const override;
  virtual bool HasValidDimensions() const override;

  
  float GetLength(uint8_t mCtxType);

  

  





  int32_t GetIntrinsicWidth();
  int32_t GetIntrinsicHeight();

  











  bool HasViewBoxRect() const;

  






  bool ShouldSynthesizeViewBox() const;

  bool HasViewBoxOrSyntheticViewBox() const {
    return HasViewBoxRect() || ShouldSynthesizeViewBox();
  }

  gfx::Matrix GetViewBoxTransform() const;

  bool HasChildrenOnlyTransform() const {
    return mHasChildrenOnlyTransform;
  }

  void UpdateHasChildrenOnlyTransform();

  enum ChildrenOnlyTransformChangedFlags {
    eDuringReflow = 1
  };

  









  void ChildrenOnlyTransformChanged(uint32_t aFlags = 0);

  
  
  
  virtual void FlushImageTransformInvalidation();

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  
  
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
  void ForceRedraw();
  void PauseAnimations();
  void UnpauseAnimations();
  bool AnimationsPaused();
  float GetCurrentTime();
  void SetCurrentTime(float seconds);
  void DeselectAll();
  already_AddRefed<DOMSVGNumber> CreateSVGNumber();
  already_AddRefed<DOMSVGLength> CreateSVGLength();
  already_AddRefed<SVGAngle> CreateSVGAngle();
  already_AddRefed<nsISVGPoint> CreateSVGPoint();
  already_AddRefed<SVGMatrix> CreateSVGMatrix();
  already_AddRefed<SVGIRect> CreateSVGRect();
  already_AddRefed<SVGTransform> CreateSVGTransform();
  already_AddRefed<SVGTransform> CreateSVGTransformFromMatrix(SVGMatrix& matrix);
  using nsINode::GetElementById; 
  already_AddRefed<SVGAnimatedRect> ViewBox();
  already_AddRefed<DOMSVGAnimatedPreserveAspectRatio> PreserveAspectRatio();
  uint16_t ZoomAndPan();
  void SetZoomAndPan(uint16_t aZoomAndPan, ErrorResult& rv);

private:
  

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) override;
  virtual void UnbindFromTree(bool aDeep, bool aNullParent) override;

  

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
  bool SetTransformProperty(const SVGTransformList& aValue);
  const SVGTransformList* GetTransformProperty() const;
  bool ClearTransformProperty();

  bool IsRoot() const {
    NS_ASSERTION((IsInDoc() && !GetParent()) ==
                 (OwnerDoc() && (OwnerDoc()->GetRootElement() == this)),
                 "Can't determine if we're root");
    return IsInDoc() && !GetParent();
  }

  



  bool IsInner() const {
    const nsIContent *parent = GetFlattenedTreeParent();
    return parent && parent->IsSVGElement() &&
           !parent->IsSVGElement(nsGkAtoms::foreignObject);
  }

  








  bool WillBeOutermostSVG(nsIContent* aParent,
                          nsIContent* aBindingParent) const;

  
  void InvalidateTransformNotifyFrame();

  
  
  
  bool HasPreserveAspectRatio();

 




  nsSVGViewBoxRect GetViewBoxWithSynthesis(
      float aViewportWidth, float aViewportHeight) const;
  



  SVGPreserveAspectRatio GetPreserveAspectRatioWithOverride() const;

  virtual LengthAttributesInfo GetLengthInfo() override;

  enum { ATTR_X, ATTR_Y, ATTR_WIDTH, ATTR_HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  virtual EnumAttributesInfo GetEnumInfo() override;

  enum { ZOOMANDPAN };
  nsSVGEnum mEnumAttributes[1];
  static nsSVGEnumMapping sZoomAndPanMap[];
  static EnumInfo sEnumInfo[1];

  virtual nsSVGViewBox *GetViewBox() override;
  virtual SVGAnimatedPreserveAspectRatio *GetPreserveAspectRatio() override;

  nsSVGViewBox                   mViewBox;
  SVGAnimatedPreserveAspectRatio mPreserveAspectRatio;

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



class MOZ_STACK_CLASS AutoSVGRenderingState
{
public:
  AutoSVGRenderingState(const Maybe<SVGImageContext>& aSVGContext,
                        float aFrameTime,
                        dom::SVGSVGElement* aRootElem
                        MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : mHaveOverrides(aSVGContext.isSome() &&
                     aSVGContext->GetPreserveAspectRatio().isSome())
    , mRootElem(aRootElem)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    MOZ_ASSERT(mRootElem, "No SVG node to manage?");
    if (mHaveOverrides) {
      
      
      
      mRootElem->SetImageOverridePreserveAspectRatio(
          *aSVGContext->GetPreserveAspectRatio());
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
