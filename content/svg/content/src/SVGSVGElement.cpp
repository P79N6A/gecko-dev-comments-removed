




#include <stdint.h>
#include "mozilla/ArrayUtils.h"
#include "mozilla/ContentEvents.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/Likely.h"

#include "nsGkAtoms.h"
#include "nsLayoutUtils.h"
#include "nsLayoutStylesheetCache.h"
#include "DOMSVGNumber.h"
#include "DOMSVGLength.h"
#include "nsSVGAngle.h"
#include "nsCOMPtr.h"
#include "nsIPresShell.h"
#include "nsContentUtils.h"
#include "nsIDocument.h"
#include "mozilla/dom/SVGMatrix.h"
#include "DOMSVGPoint.h"
#include "nsIFrame.h"
#include "nsFrameSelection.h"
#include "nsISVGSVGFrame.h" 
#include "mozilla/dom/SVGRect.h"
#include "nsError.h"
#include "nsISVGChildFrame.h"
#include "mozilla/dom/SVGSVGElement.h"
#include "mozilla/dom/SVGSVGElementBinding.h"
#include "nsSVGUtils.h"
#include "mozilla/dom/SVGViewElement.h"
#include "nsStyleUtil.h"
#include "SVGContentUtils.h"

#include "nsSMILTimeContainer.h"
#include "nsSMILAnimationController.h"
#include "nsSMILTypes.h"
#include "SVGAngle.h"
#include <algorithm>
#include "prtime.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT_CHECK_PARSER(SVG)

using namespace mozilla::gfx;

namespace mozilla {
namespace dom {

class SVGAnimatedLength;

JSObject*
SVGSVGElement::WrapNode(JSContext *aCx)
{
  return SVGSVGElementBinding::Wrap(aCx, this);
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(DOMSVGTranslatePoint, nsISVGPoint,
                                   mElement)

NS_IMPL_ADDREF_INHERITED(DOMSVGTranslatePoint, nsISVGPoint)
NS_IMPL_RELEASE_INHERITED(DOMSVGTranslatePoint, nsISVGPoint)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGTranslatePoint)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  
  
  NS_INTERFACE_MAP_ENTRY(mozilla::nsISVGPoint)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

SVGSVGElement::~SVGSVGElement()
{
}

DOMSVGPoint*
DOMSVGTranslatePoint::Copy()
{
  return new DOMSVGPoint(mPt.GetX(), mPt.GetY());
}

nsISupports*
DOMSVGTranslatePoint::GetParentObject()
{
  return static_cast<nsIDOMSVGElement*>(mElement);
}

void
DOMSVGTranslatePoint::SetX(float aValue, ErrorResult& rv)
{
  mElement->SetCurrentTranslate(aValue, mPt.GetY());
}

void
DOMSVGTranslatePoint::SetY(float aValue, ErrorResult& rv)
{
  mElement->SetCurrentTranslate(mPt.GetX(), aValue);
}

already_AddRefed<nsISVGPoint>
DOMSVGTranslatePoint::MatrixTransform(SVGMatrix& matrix)
{
  float a = matrix.A(), b = matrix.B(), c = matrix.C();
  float d = matrix.D(), e = matrix.E(), f = matrix.F();
  float x = mPt.GetX();
  float y = mPt.GetY();

  nsCOMPtr<nsISVGPoint> point = new DOMSVGPoint(a*x + c*y + e, b*x + d*y + f);
  return point.forget();
}

nsSVGElement::LengthInfo SVGSVGElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::y, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y },
  { &nsGkAtoms::width, 100, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::X },
  { &nsGkAtoms::height, 100, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::Y },
};

nsSVGEnumMapping SVGSVGElement::sZoomAndPanMap[] = {
  {&nsGkAtoms::disable, SVG_ZOOMANDPAN_DISABLE},
  {&nsGkAtoms::magnify, SVG_ZOOMANDPAN_MAGNIFY},
  {nullptr, 0}
};

nsSVGElement::EnumInfo SVGSVGElement::sEnumInfo[1] =
{
  { &nsGkAtoms::zoomAndPan,
    sZoomAndPanMap,
    SVG_ZOOMANDPAN_MAGNIFY
  }
};




NS_IMPL_CYCLE_COLLECTION_CLASS(SVGSVGElement)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(SVGSVGElement,
                                                SVGSVGElementBase)
  if (tmp->mTimedDocumentRoot) {
    tmp->mTimedDocumentRoot->Unlink();
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(SVGSVGElement,
                                                  SVGSVGElementBase)
  if (tmp->mTimedDocumentRoot) {
    tmp->mTimedDocumentRoot->Traverse(&cb);
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(SVGSVGElement,SVGSVGElementBase)
NS_IMPL_RELEASE_INHERITED(SVGSVGElement,SVGSVGElementBase)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(SVGSVGElement)
  NS_INTERFACE_TABLE_INHERITED(SVGSVGElement, nsIDOMNode, nsIDOMElement,
                               nsIDOMSVGElement)
NS_INTERFACE_TABLE_TAIL_INHERITING(SVGSVGElementBase)




SVGSVGElement::SVGSVGElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
                             FromParser aFromParser)
  : SVGSVGElementBase(aNodeInfo),
    mViewportWidth(0),
    mViewportHeight(0),
    mCurrentTranslate(0.0f, 0.0f),
    mCurrentScale(1.0f),
    mPreviousTranslate(0.0f, 0.0f),
    mPreviousScale(1.0f),
    mStartAnimationOnBindToTree(aFromParser == NOT_FROM_PARSER ||
                                aFromParser == FROM_PARSER_FRAGMENT ||
                                aFromParser == FROM_PARSER_XSLT),
    mImageNeedsTransformInvalidation(false),
    mIsPaintingSVGImageElement(false),
    mHasChildrenOnlyTransform(false),
    mUseCurrentView(false)
{
}





nsresult
SVGSVGElement::Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nullptr;
  already_AddRefed<mozilla::dom::NodeInfo> ni = nsRefPtr<mozilla::dom::NodeInfo>(aNodeInfo).forget();
  SVGSVGElement *it = new SVGSVGElement(ni, NOT_FROM_PARSER);

  nsCOMPtr<nsINode> kungFuDeathGrip = it;
  nsresult rv1 = it->Init();
  nsresult rv2 = const_cast<SVGSVGElement*>(this)->CopyInnerTo(it);
  if (NS_SUCCEEDED(rv1) && NS_SUCCEEDED(rv2)) {
    kungFuDeathGrip.swap(*aResult);
  }

  return NS_FAILED(rv1) ? rv1 : rv2;
}





already_AddRefed<SVGAnimatedLength>
SVGSVGElement::X()
{
  return mLengthAttributes[ATTR_X].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGSVGElement::Y()
{
  return mLengthAttributes[ATTR_Y].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGSVGElement::Width()
{
  return mLengthAttributes[ATTR_WIDTH].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGSVGElement::Height()
{
  return mLengthAttributes[ATTR_HEIGHT].ToDOMAnimatedLength(this);
}

float
SVGSVGElement::PixelUnitToMillimeterX()
{
  return MM_PER_INCH_FLOAT / 96;
}

float
SVGSVGElement::PixelUnitToMillimeterY()
{
  return PixelUnitToMillimeterX();
}

float
SVGSVGElement::ScreenPixelToMillimeterX()
{
  return MM_PER_INCH_FLOAT / 96;
}

float
SVGSVGElement::ScreenPixelToMillimeterY()
{
  return ScreenPixelToMillimeterX();
}

bool
SVGSVGElement::UseCurrentView()
{
  return mUseCurrentView;
}

float
SVGSVGElement::CurrentScale()
{
  return mCurrentScale;
}

#define CURRENT_SCALE_MAX 16.0f
#define CURRENT_SCALE_MIN 0.0625f

void
SVGSVGElement::SetCurrentScale(float aCurrentScale)
{
  SetCurrentScaleTranslate(aCurrentScale,
    mCurrentTranslate.GetX(), mCurrentTranslate.GetY());
}

already_AddRefed<nsISVGPoint>
SVGSVGElement::CurrentTranslate()
{
  nsCOMPtr<nsISVGPoint> point = new DOMSVGTranslatePoint(&mCurrentTranslate, this);
  return point.forget();
}

uint32_t
SVGSVGElement::SuspendRedraw(uint32_t max_wait_milliseconds)
{
  
  
  return 1;
}


void
SVGSVGElement::UnsuspendRedraw(uint32_t suspend_handle_id)
{
  
}


void
SVGSVGElement::UnsuspendRedrawAll()
{
  
}

void
SVGSVGElement::ForceRedraw(ErrorResult& rv)
{
  nsIDocument* doc = GetCurrentDoc();
  if (!doc) {
    rv.Throw(NS_ERROR_FAILURE);
    return;
  }

  doc->FlushPendingNotifications(Flush_Display);
}

void
SVGSVGElement::PauseAnimations()
{
  if (mTimedDocumentRoot) {
    mTimedDocumentRoot->Pause(nsSMILTimeContainer::PAUSE_SCRIPT);
  }
  
}

void
SVGSVGElement::UnpauseAnimations()
{
  if (mTimedDocumentRoot) {
    mTimedDocumentRoot->Resume(nsSMILTimeContainer::PAUSE_SCRIPT);
  }
  
}

bool
SVGSVGElement::AnimationsPaused()
{
  nsSMILTimeContainer* root = GetTimedDocumentRoot();
  return root && root->IsPausedByType(nsSMILTimeContainer::PAUSE_SCRIPT);
}

float
SVGSVGElement::GetCurrentTime()
{
  nsSMILTimeContainer* root = GetTimedDocumentRoot();
  if (root) {
    double fCurrentTimeMs = double(root->GetCurrentTime());
    return (float)(fCurrentTimeMs / PR_MSEC_PER_SEC);
  } else {
    return 0.f;
  }
}

void
SVGSVGElement::SetCurrentTime(float seconds)
{
  if (mTimedDocumentRoot) {
    
    FlushAnimations();
    double fMilliseconds = double(seconds) * PR_MSEC_PER_SEC;
    
    
    nsSMILTime lMilliseconds = int64_t(NS_round(fMilliseconds));
    mTimedDocumentRoot->SetCurrentTime(lMilliseconds);
    AnimationNeedsResample();
    
    
    
    
    
    FlushAnimations();
  }
  
}

void
SVGSVGElement::DeselectAll()
{
  nsIFrame* frame = GetPrimaryFrame();
  if (frame) {
    nsRefPtr<nsFrameSelection> frameSelection = frame->GetFrameSelection();
    frameSelection->ClearNormalSelection();
  }
}

already_AddRefed<DOMSVGNumber>
SVGSVGElement::CreateSVGNumber()
{
  nsRefPtr<DOMSVGNumber> number = new DOMSVGNumber(ToSupports(this));
  return number.forget();
}

already_AddRefed<DOMSVGLength>
SVGSVGElement::CreateSVGLength()
{
  nsCOMPtr<DOMSVGLength> length = new DOMSVGLength();
  return length.forget();
}

already_AddRefed<SVGAngle>
SVGSVGElement::CreateSVGAngle()
{
  nsSVGAngle* angle = new nsSVGAngle();
  angle->Init();
  nsRefPtr<SVGAngle> svgangle = new SVGAngle(angle, this, SVGAngle::CreatedValue);
  return svgangle.forget();
}

already_AddRefed<nsISVGPoint>
SVGSVGElement::CreateSVGPoint()
{
  nsCOMPtr<nsISVGPoint> point = new DOMSVGPoint(0, 0);
  return point.forget();
}

already_AddRefed<SVGMatrix>
SVGSVGElement::CreateSVGMatrix()
{
  nsRefPtr<SVGMatrix> matrix = new SVGMatrix();
  return matrix.forget();
}

already_AddRefed<SVGIRect>
SVGSVGElement::CreateSVGRect()
{
  return NS_NewSVGRect(this);
}

already_AddRefed<SVGTransform>
SVGSVGElement::CreateSVGTransform()
{
  nsRefPtr<SVGTransform> transform = new SVGTransform();
  return transform.forget();
}

already_AddRefed<SVGTransform>
SVGSVGElement::CreateSVGTransformFromMatrix(SVGMatrix& matrix)
{
  nsRefPtr<SVGTransform> transform = new SVGTransform(matrix.GetMatrix());
  return transform.forget();
}



already_AddRefed<SVGAnimatedRect>
SVGSVGElement::ViewBox()
{
  return mViewBox.ToSVGAnimatedRect(this);
}

already_AddRefed<DOMSVGAnimatedPreserveAspectRatio>
SVGSVGElement::PreserveAspectRatio()
{
  nsRefPtr<DOMSVGAnimatedPreserveAspectRatio> ratio;
  mPreserveAspectRatio.ToDOMAnimatedPreserveAspectRatio(getter_AddRefs(ratio), this);
  return ratio.forget();
}

uint16_t
SVGSVGElement::ZoomAndPan()
{
  SVGViewElement* viewElement = GetCurrentViewElement();
  if (viewElement && viewElement->mEnumAttributes[
                       SVGViewElement::ZOOMANDPAN].IsExplicitlySet()) {
    return viewElement->mEnumAttributes[
             SVGViewElement::ZOOMANDPAN].GetAnimValue();
  }
  return mEnumAttributes[ZOOMANDPAN].GetAnimValue();
}

void
SVGSVGElement::SetZoomAndPan(uint16_t aZoomAndPan, ErrorResult& rv)
{
  if (aZoomAndPan == SVG_ZOOMANDPAN_DISABLE ||
      aZoomAndPan == SVG_ZOOMANDPAN_MAGNIFY) {
    mEnumAttributes[ZOOMANDPAN].SetBaseValue(aZoomAndPan, this);
    return;
  }

  rv.Throw(NS_ERROR_RANGE_ERR);
}




void
SVGSVGElement::SetCurrentScaleTranslate(float s, float x, float y)
{
  if (s == mCurrentScale &&
      x == mCurrentTranslate.GetX() && y == mCurrentTranslate.GetY()) {
    return;
  }

  
  if (s < CURRENT_SCALE_MIN)
    s = CURRENT_SCALE_MIN;
  else if (s > CURRENT_SCALE_MAX)
    s = CURRENT_SCALE_MAX;
  
  
  
  
  
  
  
  
  
  mPreviousScale = mCurrentScale;
  mPreviousTranslate = mCurrentTranslate;
  
  mCurrentScale = s;
  mCurrentTranslate = SVGPoint(x, y);

  
  nsIDocument* doc = GetCurrentDoc();
  if (doc) {
    nsCOMPtr<nsIPresShell> presShell = doc->GetShell();
    if (presShell && IsRoot()) {
      nsEventStatus status = nsEventStatus_eIgnore;
      if (mPreviousScale != mCurrentScale) {
        InternalSVGZoomEvent svgZoomEvent(true, NS_SVG_ZOOM);
        presShell->HandleDOMEventWithTarget(this, &svgZoomEvent, &status);
      } else {
        WidgetEvent svgScrollEvent(true, NS_SVG_SCROLL);
        presShell->HandleDOMEventWithTarget(this, &svgScrollEvent, &status);
      }
      InvalidateTransformNotifyFrame();
    }
  }
}

void
SVGSVGElement::SetCurrentTranslate(float x, float y)
{
  SetCurrentScaleTranslate(mCurrentScale, x, y);
}

nsSMILTimeContainer*
SVGSVGElement::GetTimedDocumentRoot()
{
  if (mTimedDocumentRoot) {
    return mTimedDocumentRoot;
  }

  
  SVGSVGElement *outerSVGElement =
    SVGContentUtils::GetOuterSVGElement(this);

  if (outerSVGElement) {
    return outerSVGElement->GetTimedDocumentRoot();
  }
  
  return nullptr;
}




NS_IMETHODIMP_(bool)
SVGSVGElement::IsAttributeMapped(const nsIAtom* name) const
{
  
  
  
  
  
  
  
  
  

  if (!IsInner() && (name == nsGkAtoms::width || name == nsGkAtoms::height)) {
    return true;
  }

  static const MappedAttributeEntry* const map[] = {
    sColorMap,
    sFEFloodMap,
    sFillStrokeMap,
    sFiltersMap,
    sFontSpecificationMap,
    sGradientStopMap,
    sGraphicsMap,
    sLightingEffectsMap,
    sMarkersMap,
    sTextContentElementsMap,
    sViewportsMap
  };

  return FindAttributeDependence(name, map) ||
    SVGSVGElementBase::IsAttributeMapped(name);
}




nsresult
SVGSVGElement::PreHandleEvent(EventChainPreVisitor& aVisitor)
{
  if (aVisitor.mEvent->message == NS_SVG_LOAD) {
    if (mTimedDocumentRoot) {
      mTimedDocumentRoot->Begin();
      
      
      
      AnimationNeedsResample();
    }
  }
  return SVGSVGElementBase::PreHandleEvent(aVisitor);
}

bool
SVGSVGElement::IsEventAttributeName(nsIAtom* aName)
{
  





  return nsContentUtils::IsEventAttributeName(aName,
         (EventNameType_SVGGraphic | EventNameType_SVGSVG));
}











inline float
ComputeSynthesizedViewBoxDimension(const nsSVGLength2& aLength,
                                   float aViewportLength,
                                   const SVGSVGElement* aSelf)
{
  if (aLength.IsPercentage()) {
    return aViewportLength * aLength.GetAnimValInSpecifiedUnits() / 100.0f;
  }

  return aLength.GetAnimValue(const_cast<SVGSVGElement*>(aSelf));
}




gfx::Matrix
SVGSVGElement::GetViewBoxTransform() const
{
  float viewportWidth, viewportHeight;
  if (IsInner()) {
    SVGSVGElement *ctx = GetCtx();
    viewportWidth = mLengthAttributes[ATTR_WIDTH].GetAnimValue(ctx);
    viewportHeight = mLengthAttributes[ATTR_HEIGHT].GetAnimValue(ctx);
  } else {
    viewportWidth = mViewportWidth;
    viewportHeight = mViewportHeight;
  }

  if (viewportWidth <= 0.0f || viewportHeight <= 0.0f) {
    return gfx::Matrix(0.0, 0.0, 0.0, 0.0, 0.0, 0.0); 
  }

  nsSVGViewBoxRect viewBox =
    GetViewBoxWithSynthesis(viewportWidth, viewportHeight);

  if (viewBox.width <= 0.0f || viewBox.height <= 0.0f) {
    return gfx::Matrix(0.0, 0.0, 0.0, 0.0, 0.0, 0.0); 
  }

  return SVGContentUtils::GetViewBoxTransform(viewportWidth, viewportHeight,
                                              viewBox.x, viewBox.y,
                                              viewBox.width, viewBox.height,
                                              GetPreserveAspectRatioWithOverride());
}

void
SVGSVGElement::UpdateHasChildrenOnlyTransform()
{
  bool hasChildrenOnlyTransform =
    HasViewBoxOrSyntheticViewBox() ||
    (IsRoot() && (mCurrentTranslate != SVGPoint(0.0f, 0.0f) ||
                  mCurrentScale != 1.0f));
  mHasChildrenOnlyTransform = hasChildrenOnlyTransform;
}

void
SVGSVGElement::ChildrenOnlyTransformChanged(uint32_t aFlags)
{
  
  NS_ABORT_IF_FALSE(!(GetPrimaryFrame()->GetStateBits() &
                      NS_FRAME_IS_NONDISPLAY),
                    "Non-display SVG frames don't maintain overflow rects");

  nsChangeHint changeHint;

  bool hadChildrenOnlyTransform = mHasChildrenOnlyTransform;

  UpdateHasChildrenOnlyTransform();

  if (hadChildrenOnlyTransform != mHasChildrenOnlyTransform) {
    
    
    changeHint = nsChangeHint_ReconstructFrame;
  } else {
    
    changeHint = nsChangeHint(nsChangeHint_UpdateOverflow |
                              nsChangeHint_ChildrenOnlyTransform);
  }

  
  
  
  
  
  if ((changeHint & nsChangeHint_ReconstructFrame) ||
      !(aFlags & eDuringReflow)) {
    nsLayoutUtils::PostRestyleEvent(this, nsRestyleHint(0), changeHint);
  }
}

nsresult
SVGSVGElement::BindToTree(nsIDocument* aDocument,
                          nsIContent* aParent,
                          nsIContent* aBindingParent,
                          bool aCompileEventHandlers)
{
  nsSMILAnimationController* smilController = nullptr;

  if (aDocument) {
    smilController = aDocument->GetAnimationController();
    if (smilController) {
      
      if (WillBeOutermostSVG(aParent, aBindingParent)) {
        
        if (!mTimedDocumentRoot) {
          mTimedDocumentRoot = new nsSMILTimeContainer();
        }
      } else {
        
        
        
        mTimedDocumentRoot = nullptr;
        mStartAnimationOnBindToTree = true;
      }
    }
  }

  nsresult rv = SVGSVGElementBase::BindToTree(aDocument, aParent,
                                              aBindingParent,
                                              aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv,rv);

  if (aDocument) {
    
    
    
    aDocument->
      EnsureOnDemandBuiltInUASheet(nsLayoutStylesheetCache::SVGSheet());
  }

  if (mTimedDocumentRoot && smilController) {
    rv = mTimedDocumentRoot->SetParent(smilController);
    if (mStartAnimationOnBindToTree) {
      mTimedDocumentRoot->Begin();
      mStartAnimationOnBindToTree = false;
    }
  }

  return rv;
}

void
SVGSVGElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  if (mTimedDocumentRoot) {
    mTimedDocumentRoot->SetParent(nullptr);
  }

  SVGSVGElementBase::UnbindFromTree(aDeep, aNullParent);
}




bool
SVGSVGElement::WillBeOutermostSVG(nsIContent* aParent,
                                  nsIContent* aBindingParent) const
{
  nsIContent* parent = aBindingParent ? aBindingParent : aParent;

  while (parent && parent->IsSVG()) {
    nsIAtom* tag = parent->Tag();
    if (tag == nsGkAtoms::foreignObject) {
      
      return false;
    }
    if (tag == nsGkAtoms::svg) {
      return false;
    }
    parent = parent->GetParent();
  }

  return true;
}

void
SVGSVGElement::InvalidateTransformNotifyFrame()
{
  nsISVGSVGFrame* svgframe = do_QueryFrame(GetPrimaryFrame());
  
  if (svgframe) {
    svgframe->NotifyViewportOrTransformChanged(
                nsISVGChildFrame::TRANSFORM_CHANGED);
  }
}

bool
SVGSVGElement::HasPreserveAspectRatio()
{
  return HasAttr(kNameSpaceID_None, nsGkAtoms::preserveAspectRatio) ||
    mPreserveAspectRatio.IsAnimated();
}

SVGViewElement*
SVGSVGElement::GetCurrentViewElement() const
{
  if (mCurrentViewID) {
    nsIDocument* doc = GetCurrentDoc();
    if (doc) {
      Element *element = doc->GetElementById(*mCurrentViewID);
      if (element && element->IsSVG(nsGkAtoms::view)) {
        return static_cast<SVGViewElement*>(element);
      }
    }
  }
  return nullptr;
}

nsSVGViewBoxRect
SVGSVGElement::GetViewBoxWithSynthesis(
  float aViewportWidth, float aViewportHeight) const
{
  
  SVGViewElement* viewElement = GetCurrentViewElement();
  if (viewElement && viewElement->mViewBox.HasRect()) {
    return viewElement->mViewBox.GetAnimValue();
  }
  if (mViewBox.HasRect()) {
    return mViewBox.GetAnimValue();
  }

  if (ShouldSynthesizeViewBox()) {
    
    
    return nsSVGViewBoxRect(0, 0,
              ComputeSynthesizedViewBoxDimension(mLengthAttributes[ATTR_WIDTH],
                                                 mViewportWidth, this),
              ComputeSynthesizedViewBoxDimension(mLengthAttributes[ATTR_HEIGHT],
                                                 mViewportHeight, this));

  }

  
  
  return nsSVGViewBoxRect(0, 0, aViewportWidth, aViewportHeight);
}

SVGPreserveAspectRatio
SVGSVGElement::GetPreserveAspectRatioWithOverride() const
{
  nsIDocument* doc = GetCurrentDoc();
  if (doc && doc->IsBeingUsedAsImage()) {
    const SVGPreserveAspectRatio *pAROverridePtr = GetPreserveAspectRatioProperty();
    if (pAROverridePtr) {
      return *pAROverridePtr;
    }
  }

  SVGViewElement* viewElement = GetCurrentViewElement();

  
  
  
  if (!((viewElement && viewElement->mViewBox.HasRect()) ||
        mViewBox.HasRect()) &&
      ShouldSynthesizeViewBox()) {
    
    return SVGPreserveAspectRatio(SVG_PRESERVEASPECTRATIO_NONE, SVG_MEETORSLICE_SLICE);
  }

  if (viewElement && viewElement->mPreserveAspectRatio.IsExplicitlySet()) {
    return viewElement->mPreserveAspectRatio.GetAnimValue();
  }
  return mPreserveAspectRatio.GetAnimValue();
}




float
SVGSVGElement::GetLength(uint8_t aCtxType)
{
  float h, w;

  SVGViewElement* viewElement = GetCurrentViewElement();
  const nsSVGViewBoxRect* viewbox = nullptr;

  
  if (viewElement && viewElement->mViewBox.HasRect()) {
    viewbox = &viewElement->mViewBox.GetAnimValue();
  } else if (mViewBox.HasRect()) {
    viewbox = &mViewBox.GetAnimValue();
  }

  if (viewbox) {
    w = viewbox->width;
    h = viewbox->height;
  } else if (IsInner()) {
    SVGSVGElement *ctx = GetCtx();
    w = mLengthAttributes[ATTR_WIDTH].GetAnimValue(ctx);
    h = mLengthAttributes[ATTR_HEIGHT].GetAnimValue(ctx);
  } else if (ShouldSynthesizeViewBox()) {
    w = ComputeSynthesizedViewBoxDimension(mLengthAttributes[ATTR_WIDTH],
                                           mViewportWidth, this);
    h = ComputeSynthesizedViewBoxDimension(mLengthAttributes[ATTR_HEIGHT],
                                           mViewportHeight, this);
  } else {
    w = mViewportWidth;
    h = mViewportHeight;
  }

  w = std::max(w, 0.0f);
  h = std::max(h, 0.0f);

  switch (aCtxType) {
  case SVGContentUtils::X:
    return w;
  case SVGContentUtils::Y:
    return h;
  case SVGContentUtils::XY:
    return float(SVGContentUtils::ComputeNormalizedHypotenuse(w, h));
  }
  return 0;
}




 gfxMatrix
SVGSVGElement::PrependLocalTransformsTo(const gfxMatrix &aMatrix,
                                        TransformTypes aWhich) const
{
  NS_ABORT_IF_FALSE(aWhich != eChildToUserSpace || aMatrix.IsIdentity(),
                    "Skipping eUserSpaceToParent transforms makes no sense");

  
  gfxMatrix fromUserSpace =
    SVGSVGElementBase::PrependLocalTransformsTo(aMatrix, aWhich);
  if (aWhich == eUserSpaceToParent) {
    return fromUserSpace;
  }

  if (IsInner()) {
    float x, y;
    const_cast<SVGSVGElement*>(this)->GetAnimatedLengthValues(&x, &y, nullptr);
    if (aWhich == eAllTransforms) {
      
      return ThebesMatrix(GetViewBoxTransform()) * gfxMatrix().Translate(gfxPoint(x, y)) * fromUserSpace;
    }
    NS_ABORT_IF_FALSE(aWhich == eChildToUserSpace, "Unknown TransformTypes");
    return ThebesMatrix(GetViewBoxTransform()) * gfxMatrix().Translate(gfxPoint(x, y)) * aMatrix;
  }

  if (IsRoot()) {
    gfxMatrix zoomPanTM;
    zoomPanTM.Translate(gfxPoint(mCurrentTranslate.GetX(), mCurrentTranslate.GetY()));
    zoomPanTM.Scale(mCurrentScale, mCurrentScale);
    return ThebesMatrix(GetViewBoxTransform()) * zoomPanTM * fromUserSpace;
  }

  
  return ThebesMatrix(GetViewBoxTransform()) * fromUserSpace;
}

 bool
SVGSVGElement::HasValidDimensions() const
{
  return !IsInner() ||
    ((!mLengthAttributes[ATTR_WIDTH].IsExplicitlySet() ||
       mLengthAttributes[ATTR_WIDTH].GetAnimValInSpecifiedUnits() > 0) &&
     (!mLengthAttributes[ATTR_HEIGHT].IsExplicitlySet() ||
       mLengthAttributes[ATTR_HEIGHT].GetAnimValInSpecifiedUnits() > 0));
}

nsSVGElement::LengthAttributesInfo
SVGSVGElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}

nsSVGElement::EnumAttributesInfo
SVGSVGElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGViewBox *
SVGSVGElement::GetViewBox()
{
  return &mViewBox;
}

SVGAnimatedPreserveAspectRatio *
SVGSVGElement::GetPreserveAspectRatio()
{
  return &mPreserveAspectRatio;
}

bool
SVGSVGElement::HasViewBoxRect() const
{
  SVGViewElement* viewElement = GetCurrentViewElement();
  if (viewElement && viewElement->mViewBox.HasRect()) {
    return true;
  }
  return mViewBox.HasRect();
}

bool
SVGSVGElement::ShouldSynthesizeViewBox() const
{
  NS_ABORT_IF_FALSE(!HasViewBoxRect(),
                    "Should only be called if we lack a viewBox");

  nsIDocument* doc = GetCurrentDoc();
  return doc &&
    doc->IsBeingUsedAsImage() &&
    !mIsPaintingSVGImageElement &&
    !GetParent();
}

bool
SVGSVGElement::SetPreserveAspectRatioProperty(const SVGPreserveAspectRatio& aPAR)
{
  SVGPreserveAspectRatio* pAROverridePtr = new SVGPreserveAspectRatio(aPAR);
  nsresult rv = SetProperty(nsGkAtoms::overridePreserveAspectRatio,
                            pAROverridePtr,
                            nsINode::DeleteProperty<SVGPreserveAspectRatio>,
                            true);
  NS_ABORT_IF_FALSE(rv != NS_PROPTABLE_PROP_OVERWRITTEN,
                    "Setting override value when it's already set...?"); 

  if (MOZ_UNLIKELY(NS_FAILED(rv))) {
    
    delete pAROverridePtr;
    return false;
  }
  return true;
}

const SVGPreserveAspectRatio*
SVGSVGElement::GetPreserveAspectRatioProperty() const
{
  void* valPtr = GetProperty(nsGkAtoms::overridePreserveAspectRatio);
  if (valPtr) {
    return static_cast<SVGPreserveAspectRatio*>(valPtr);
  }
  return nullptr;
}

bool
SVGSVGElement::ClearPreserveAspectRatioProperty()
{
  void* valPtr = UnsetProperty(nsGkAtoms::overridePreserveAspectRatio);
  delete static_cast<SVGPreserveAspectRatio*>(valPtr);
  return valPtr;
}

void
SVGSVGElement::
  SetImageOverridePreserveAspectRatio(const SVGPreserveAspectRatio& aPAR)
{
#ifdef DEBUG
  NS_ABORT_IF_FALSE(GetCurrentDoc()->IsBeingUsedAsImage(),
                    "should only override preserveAspectRatio in images");
#endif

  bool hasViewBoxRect = HasViewBoxRect();
  if (!hasViewBoxRect && ShouldSynthesizeViewBox()) {
    
    
    
    mImageNeedsTransformInvalidation = true;
  }
  mIsPaintingSVGImageElement = true;

  if (!hasViewBoxRect) {
    return; 
  }

  if (aPAR.GetDefer() && HasPreserveAspectRatio()) {
    return; 
  }

  if (SetPreserveAspectRatioProperty(aPAR)) {
    mImageNeedsTransformInvalidation = true;
  }
}

void
SVGSVGElement::ClearImageOverridePreserveAspectRatio()
{
#ifdef DEBUG
  NS_ABORT_IF_FALSE(GetCurrentDoc()->IsBeingUsedAsImage(),
                    "should only override image preserveAspectRatio in images");
#endif

  mIsPaintingSVGImageElement = false;
  if (!HasViewBoxRect() && ShouldSynthesizeViewBox()) {
    
    
    
    mImageNeedsTransformInvalidation = true;
  }

  if (ClearPreserveAspectRatioProperty()) {
    mImageNeedsTransformInvalidation = true;
  }
}

void
SVGSVGElement::FlushImageTransformInvalidation()
{
  NS_ABORT_IF_FALSE(!GetParent(), "Should only be called on root node");
  NS_ABORT_IF_FALSE(GetCurrentDoc()->IsBeingUsedAsImage(),
                    "Should only be called on image documents");

  if (mImageNeedsTransformInvalidation) {
    InvalidateTransformNotifyFrame();
    mImageNeedsTransformInvalidation = false;
  }
}

bool
SVGSVGElement::SetViewBoxProperty(const nsSVGViewBoxRect& aViewBox)
{
  nsSVGViewBoxRect* pViewBoxOverridePtr = new nsSVGViewBoxRect(aViewBox);
  nsresult rv = SetProperty(nsGkAtoms::viewBox,
                            pViewBoxOverridePtr,
                            nsINode::DeleteProperty<nsSVGViewBoxRect>,
                            true);
  NS_ABORT_IF_FALSE(rv != NS_PROPTABLE_PROP_OVERWRITTEN,
                    "Setting override value when it's already set...?"); 

  if (MOZ_UNLIKELY(NS_FAILED(rv))) {
    
    delete pViewBoxOverridePtr;
    return false;
  }
  return true;
}

const nsSVGViewBoxRect*
SVGSVGElement::GetViewBoxProperty() const
{
  void* valPtr = GetProperty(nsGkAtoms::viewBox);
  if (valPtr) {
    return static_cast<nsSVGViewBoxRect*>(valPtr);
  }
  return nullptr;
}

bool
SVGSVGElement::ClearViewBoxProperty()
{
  void* valPtr = UnsetProperty(nsGkAtoms::viewBox);
  delete static_cast<nsSVGViewBoxRect*>(valPtr);
  return valPtr;
}

bool
SVGSVGElement::SetZoomAndPanProperty(uint16_t aValue)
{
  nsresult rv = SetProperty(nsGkAtoms::zoomAndPan,
                            reinterpret_cast<void*>(aValue),
                            nullptr, true);
  NS_ABORT_IF_FALSE(rv != NS_PROPTABLE_PROP_OVERWRITTEN,
                    "Setting override value when it's already set...?"); 

  return NS_SUCCEEDED(rv);
}

uint16_t
SVGSVGElement::GetZoomAndPanProperty() const
{
  void* valPtr = GetProperty(nsGkAtoms::zoomAndPan);
  if (valPtr) {
    return reinterpret_cast<uintptr_t>(valPtr);
  }
  return SVG_ZOOMANDPAN_UNKNOWN;
}

bool
SVGSVGElement::ClearZoomAndPanProperty()
{
  return UnsetProperty(nsGkAtoms::zoomAndPan);
}

bool
SVGSVGElement::SetTransformProperty(const SVGTransformList& aTransform)
{
  SVGTransformList* pTransformOverridePtr = new SVGTransformList(aTransform);
  nsresult rv = SetProperty(nsGkAtoms::transform,
                            pTransformOverridePtr,
                            nsINode::DeleteProperty<SVGTransformList>,
                            true);
  NS_ABORT_IF_FALSE(rv != NS_PROPTABLE_PROP_OVERWRITTEN,
                    "Setting override value when it's already set...?"); 

  if (MOZ_UNLIKELY(NS_FAILED(rv))) {
    
    delete pTransformOverridePtr;
    return false;
  }
  return true;
}

const SVGTransformList*
SVGSVGElement::GetTransformProperty() const
{
  void* valPtr = GetProperty(nsGkAtoms::transform);
  if (valPtr) {
    return static_cast<SVGTransformList*>(valPtr);
  }
  return nullptr;
}

bool
SVGSVGElement::ClearTransformProperty()
{
  return UnsetProperty(nsGkAtoms::transform);
}

} 
} 
