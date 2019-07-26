




#include "mozilla/StandardInteger.h"
#include "mozilla/Util.h"
#include "mozilla/Likely.h"

#include "nsGkAtoms.h"
#include "nsLayoutUtils.h"
#include "DOMSVGNumber.h"
#include "DOMSVGLength.h"
#include "nsSVGAngle.h"
#include "nsCOMPtr.h"
#include "nsIPresShell.h"
#include "nsContentUtils.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "mozilla/dom/SVGMatrix.h"
#include "DOMSVGPoint.h"
#include "nsIDOMEventTarget.h"
#include "nsIFrame.h"
#include "nsISVGSVGFrame.h" 
#include "nsSVGRect.h"
#include "nsError.h"
#include "nsISVGChildFrame.h"
#include "nsGUIEvent.h"
#include "mozilla/dom/SVGSVGElement.h"
#include "mozilla/dom/SVGSVGElementBinding.h"
#include "nsSVGUtils.h"
#include "mozilla/dom/SVGViewElement.h"
#include "nsStyleUtil.h"
#include "SVGContentUtils.h"

#include "nsEventDispatcher.h"
#include "nsSMILTimeContainer.h"
#include "nsSMILAnimationController.h"
#include "nsSMILTypes.h"
#include "nsIContentIterator.h"
#include "SVGAngle.h"

DOMCI_NODE_DATA(SVGSVGElement, mozilla::dom::SVGSVGElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT_CHECK_PARSER(SVG)

namespace mozilla {
namespace dom {

JSObject*
SVGSVGElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGSVGElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(nsSVGTranslatePoint::DOMVal, mElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGTranslatePoint::DOMVal)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGTranslatePoint::DOMVal)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGTranslatePoint::DOMVal)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  
  
  NS_INTERFACE_MAP_ENTRY(mozilla::nsISVGPoint)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

nsresult
nsSVGTranslatePoint::ToDOMVal(SVGSVGElement *aElement,
                              nsISupports **aResult)
{
  NS_ADDREF(*aResult = new DOMVal(this, aElement));
  return NS_OK;
}

nsISupports*
nsSVGTranslatePoint::DOMVal::GetParentObject()
{
  return static_cast<nsIDOMSVGSVGElement*>(mElement);
}

void
nsSVGTranslatePoint::DOMVal::SetX(float aValue, ErrorResult& rv)
{
  rv = mElement->SetCurrentTranslate(aValue, mVal->GetY());
}

void
nsSVGTranslatePoint::DOMVal::SetY(float aValue, ErrorResult& rv)
{
  rv = mElement->SetCurrentTranslate(mVal->GetX(), aValue);
}

already_AddRefed<nsISVGPoint>
nsSVGTranslatePoint::DOMVal::MatrixTransform(SVGMatrix& matrix)
{
  float a = matrix.A(), b = matrix.B(), c = matrix.C();
  float d = matrix.D(), e = matrix.E(), f = matrix.F();
  float x = mVal->GetX();
  float y = mVal->GetY();

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
  NS_NODE_INTERFACE_TABLE5(SVGSVGElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGSVGElement,
                           nsIDOMSVGFitToViewBox)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGSVGElement)
NS_INTERFACE_MAP_END_INHERITING(SVGSVGElementBase)




SVGSVGElement::SVGSVGElement(already_AddRefed<nsINodeInfo> aNodeInfo,
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
  SetIsDOMBinding();
}





nsresult
SVGSVGElement::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nullptr;
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  SVGSVGElement *it = new SVGSVGElement(ni.forget(), NOT_FROM_PARSER);

  nsCOMPtr<nsINode> kungFuDeathGrip = it;
  nsresult rv1 = it->Init();
  nsresult rv2 = const_cast<SVGSVGElement*>(this)->CopyInnerTo(it);
  if (NS_SUCCEEDED(rv1) && NS_SUCCEEDED(rv2)) {
    kungFuDeathGrip.swap(*aResult);
  }

  return NS_FAILED(rv1) ? rv1 : rv2;
}






NS_IMETHODIMP
SVGSVGElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  *aX = X().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedLength>
SVGSVGElement::X()
{
  return mLengthAttributes[ATTR_X].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP
SVGSVGElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  *aY = Y().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedLength>
SVGSVGElement::Y()
{
  return mLengthAttributes[ATTR_Y].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP
SVGSVGElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  *aWidth = Width().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedLength>
SVGSVGElement::Width()
{
  return mLengthAttributes[ATTR_WIDTH].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP
SVGSVGElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  *aHeight = Height().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedLength>
SVGSVGElement::Height()
{
  return mLengthAttributes[ATTR_HEIGHT].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP
SVGSVGElement::GetPixelUnitToMillimeterX(float *aPixelUnitToMillimeterX)
{
  *aPixelUnitToMillimeterX = PixelUnitToMillimeterX();
  return NS_OK;
}

float
SVGSVGElement::PixelUnitToMillimeterX()
{
  return MM_PER_INCH_FLOAT / 96;
}


NS_IMETHODIMP
SVGSVGElement::GetPixelUnitToMillimeterY(float *aPixelUnitToMillimeterY)
{
  return GetPixelUnitToMillimeterX(aPixelUnitToMillimeterY);
}

float
SVGSVGElement::PixelUnitToMillimeterY()
{
  return PixelUnitToMillimeterX();
}


NS_IMETHODIMP
SVGSVGElement::GetScreenPixelToMillimeterX(float *aScreenPixelToMillimeterX)
{
  *aScreenPixelToMillimeterX = ScreenPixelToMillimeterX();
  return NS_OK;
}

float
SVGSVGElement::ScreenPixelToMillimeterX()
{
  return MM_PER_INCH_FLOAT / 96;
}


NS_IMETHODIMP
SVGSVGElement::GetScreenPixelToMillimeterY(float *aScreenPixelToMillimeterY)
{
  return GetScreenPixelToMillimeterX(aScreenPixelToMillimeterY);
}

float
SVGSVGElement::ScreenPixelToMillimeterY()
{
  return ScreenPixelToMillimeterX();
}


NS_IMETHODIMP
SVGSVGElement::GetUseCurrentView(bool *aUseCurrentView)
{
  *aUseCurrentView = UseCurrentView();
  return NS_OK;
}

bool
SVGSVGElement::UseCurrentView()
{
  return mUseCurrentView;
}


NS_IMETHODIMP
SVGSVGElement::GetCurrentScale(float *aCurrentScale)
{
  *aCurrentScale = CurrentScale();
  return NS_OK;
}

float
SVGSVGElement::CurrentScale()
{
  return mCurrentScale;
}

#define CURRENT_SCALE_MAX 16.0f
#define CURRENT_SCALE_MIN 0.0625f

NS_IMETHODIMP
SVGSVGElement::SetCurrentScale(float aCurrentScale)
{
  return SetCurrentScaleTranslate(aCurrentScale,
    mCurrentTranslate.GetX(), mCurrentTranslate.GetY());
}


NS_IMETHODIMP
SVGSVGElement::GetCurrentTranslate(nsISupports * *aCurrentTranslate)
{
  *aCurrentTranslate = CurrentTranslate().get();
  return NS_OK;
}

already_AddRefed<nsISVGPoint>
SVGSVGElement::CurrentTranslate()
{
  nsCOMPtr<nsISVGPoint> point;
  mCurrentTranslate.ToDOMVal(this, getter_AddRefs(point));
  return point.forget();
}


NS_IMETHODIMP
SVGSVGElement::SuspendRedraw(uint32_t max_wait_milliseconds, uint32_t *_retval)
{
  *_retval = SuspendRedraw(max_wait_milliseconds);
  return NS_OK;
}

uint32_t
SVGSVGElement::SuspendRedraw(uint32_t max_wait_milliseconds)
{
  
  
  return 1;
}


NS_IMETHODIMP
SVGSVGElement::UnsuspendRedraw(uint32_t suspend_handle_id)
{
  
  return NS_OK;
}


NS_IMETHODIMP
SVGSVGElement::UnsuspendRedrawAll()
{
  
  return NS_OK;
}


NS_IMETHODIMP
SVGSVGElement::ForceRedraw()
{
  ErrorResult rv;
  ForceRedraw(rv);
  return rv.ErrorCode();
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


NS_IMETHODIMP
SVGSVGElement::PauseAnimations()
{
  ErrorResult rv;
  PauseAnimations(rv);
  return rv.ErrorCode();
}

void
SVGSVGElement::PauseAnimations(ErrorResult& rv)
{
  if (NS_SMILEnabled()) {
    if (mTimedDocumentRoot) {
      mTimedDocumentRoot->Pause(nsSMILTimeContainer::PAUSE_SCRIPT);
    }
    
    return;
  }
  NS_NOTYETIMPLEMENTED("SVGSVGElement::PauseAnimations");
  rv.Throw(NS_ERROR_NOT_IMPLEMENTED);
}


NS_IMETHODIMP
SVGSVGElement::UnpauseAnimations()
{
  ErrorResult rv;
  UnpauseAnimations(rv);
  return rv.ErrorCode();
}

void
SVGSVGElement::UnpauseAnimations(ErrorResult& rv)
{
  if (NS_SMILEnabled()) {
    if (mTimedDocumentRoot) {
      mTimedDocumentRoot->Resume(nsSMILTimeContainer::PAUSE_SCRIPT);
    }
    
    return;
  }
  NS_NOTYETIMPLEMENTED("SVGSVGElement::UnpauseAnimations");
  rv.Throw(NS_ERROR_NOT_IMPLEMENTED);
}


NS_IMETHODIMP
SVGSVGElement::AnimationsPaused(bool *_retval)
{
  ErrorResult rv;
  *_retval = AnimationsPaused(rv);
  return rv.ErrorCode();
}

bool
SVGSVGElement::AnimationsPaused(ErrorResult& rv)
{
  if (NS_SMILEnabled()) {
    nsSMILTimeContainer* root = GetTimedDocumentRoot();
    return root && root->IsPausedByType(nsSMILTimeContainer::PAUSE_SCRIPT);
  }
  NS_NOTYETIMPLEMENTED("SVGSVGElement::AnimationsPaused");
  rv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return false;
}


NS_IMETHODIMP
SVGSVGElement::GetCurrentTime(float *_retval)
{
  ErrorResult rv;
  *_retval = GetCurrentTime(rv);
  return rv.ErrorCode();
}

float
SVGSVGElement::GetCurrentTime(ErrorResult& rv)
{
  if (NS_SMILEnabled()) {
    nsSMILTimeContainer* root = GetTimedDocumentRoot();
    if (root) {
      double fCurrentTimeMs = double(root->GetCurrentTime());
      return (float)(fCurrentTimeMs / PR_MSEC_PER_SEC);
    } else {
      return 0.f;
    }
  }
  NS_NOTYETIMPLEMENTED("SVGSVGElement::GetCurrentTime");
  rv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return 0.f;
}


NS_IMETHODIMP
SVGSVGElement::SetCurrentTime(float seconds)
{
  NS_ENSURE_FINITE(seconds, NS_ERROR_ILLEGAL_VALUE);
  ErrorResult rv;
  SetCurrentTime(seconds, rv);
  return rv.ErrorCode();
}

void
SVGSVGElement::SetCurrentTime(float seconds, ErrorResult &rv)
{
  if (NS_SMILEnabled()) {
    if (mTimedDocumentRoot) {
      
      FlushAnimations();
      double fMilliseconds = double(seconds) * PR_MSEC_PER_SEC;
      
      
      nsSMILTime lMilliseconds = int64_t(NS_round(fMilliseconds));
      mTimedDocumentRoot->SetCurrentTime(lMilliseconds);
      AnimationNeedsResample();
      
      
      
      
      
      FlushAnimations();
    } 
      
    return;
  }
  NS_NOTYETIMPLEMENTED("SVGSVGElement::SetCurrentTime");
  rv.Throw(NS_ERROR_NOT_IMPLEMENTED);
}


NS_IMETHODIMP
SVGSVGElement::CreateSVGNumber(nsIDOMSVGNumber **_retval)
{
  *_retval = CreateSVGNumber().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGNumber>
SVGSVGElement::CreateSVGNumber()
{
  nsCOMPtr<nsIDOMSVGNumber> number = new DOMSVGNumber();
  return number.forget();
}


NS_IMETHODIMP
SVGSVGElement::CreateSVGLength(nsIDOMSVGLength **_retval)
{
  *_retval = CreateSVGLength().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGLength>
SVGSVGElement::CreateSVGLength()
{
  nsCOMPtr<nsIDOMSVGLength> length = new DOMSVGLength();
  return length.forget();
}


NS_IMETHODIMP
SVGSVGElement::CreateSVGAngle(nsISupports **_retval)
{
  *_retval =  CreateSVGAngle().get();
  return NS_OK;
}

already_AddRefed<SVGAngle>
SVGSVGElement::CreateSVGAngle()
{
  nsSVGAngle* angle = new nsSVGAngle();
  angle->Init();
  nsRefPtr<SVGAngle> svgangle = new SVGAngle(angle, this, SVGAngle::CreatedValue);
  return svgangle.forget();
}


NS_IMETHODIMP
SVGSVGElement::CreateSVGPoint(nsISupports **_retval)
{
  *_retval = CreateSVGPoint().get();
  return NS_OK;
}

already_AddRefed<nsISVGPoint>
SVGSVGElement::CreateSVGPoint()
{
  nsCOMPtr<nsISVGPoint> point = new DOMSVGPoint(0, 0);
  return point.forget();
}


NS_IMETHODIMP
SVGSVGElement::CreateSVGMatrix(nsISupports **_retval)
{
  *_retval = CreateSVGMatrix().get();
  return NS_OK;
}

already_AddRefed<SVGMatrix>
SVGSVGElement::CreateSVGMatrix()
{
  nsRefPtr<SVGMatrix> matrix = new SVGMatrix();
  return matrix.forget();
}


NS_IMETHODIMP
SVGSVGElement::CreateSVGRect(nsIDOMSVGRect **_retval)
{
  *_retval = CreateSVGRect().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGRect>
SVGSVGElement::CreateSVGRect()
{
  nsCOMPtr<nsIDOMSVGRect> rect;
  NS_NewSVGRect(getter_AddRefs(rect));
  return rect.forget();
}


NS_IMETHODIMP
SVGSVGElement::CreateSVGTransform(nsISupports **_retval)
{
  *_retval = CreateSVGTransform().get();
  return NS_OK;
}

already_AddRefed<DOMSVGTransform>
SVGSVGElement::CreateSVGTransform()
{
  nsRefPtr<DOMSVGTransform> transform = new DOMSVGTransform();
  return transform.forget();
}


NS_IMETHODIMP
SVGSVGElement::CreateSVGTransformFromMatrix(nsISupports *matrix,
                                            nsISupports **_retval)
{
  nsCOMPtr<SVGMatrix> domItem = do_QueryInterface(matrix);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }

  *_retval = CreateSVGTransformFromMatrix(*domItem).get();
  return NS_OK;
}

already_AddRefed<DOMSVGTransform>
SVGSVGElement::CreateSVGTransformFromMatrix(SVGMatrix& matrix)
{
  nsRefPtr<DOMSVGTransform> transform = new DOMSVGTransform(matrix.Matrix());
  return transform.forget();
}


NS_IMETHODIMP
SVGSVGElement::GetElementById(const nsAString & elementId, nsIDOMElement **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  ErrorResult rv;
  nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(GetElementById(elementId, rv));
  elem.forget(_retval);
  return rv.ErrorCode();
}

Element*
SVGSVGElement::GetElementById(const nsAString& elementId, ErrorResult& rv)
{
  nsAutoString selector(NS_LITERAL_STRING("#"));
  nsStyleUtil::AppendEscapedCSSIdent(PromiseFlatString(elementId), selector);
  nsIContent* element = QuerySelector(selector, rv);
  if (!rv.Failed() && element) {
    return element->AsElement();
  }
  return nullptr;
}





NS_IMETHODIMP
SVGSVGElement::GetViewBox(nsIDOMSVGAnimatedRect * *aViewBox)
{
  *aViewBox = ViewBox().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedRect>
SVGSVGElement::ViewBox()
{
  nsCOMPtr<nsIDOMSVGAnimatedRect> rect;
  mViewBox.ToDOMAnimatedRect(getter_AddRefs(rect), this);
  return rect.forget();
}


NS_IMETHODIMP
SVGSVGElement::GetPreserveAspectRatio(nsISupports
                                      **aPreserveAspectRatio)
{
  *aPreserveAspectRatio = PreserveAspectRatio().get();
  return NS_OK;
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




NS_IMETHODIMP
SVGSVGElement::SetCurrentScaleTranslate(float s, float x, float y)
{
  NS_ENSURE_FINITE3(s, x, y, NS_ERROR_ILLEGAL_VALUE);

  if (s == mCurrentScale &&
      x == mCurrentTranslate.GetX() && y == mCurrentTranslate.GetY()) {
    return NS_OK;
  }

  
  if (s < CURRENT_SCALE_MIN)
    s = CURRENT_SCALE_MIN;
  else if (s > CURRENT_SCALE_MAX)
    s = CURRENT_SCALE_MAX;
  
  
  
  
  
  
  
  
  
  mPreviousScale = mCurrentScale;
  mPreviousTranslate = mCurrentTranslate;
  
  mCurrentScale = s;
  mCurrentTranslate = nsSVGTranslatePoint(x, y);

  
  nsIDocument* doc = GetCurrentDoc();
  if (doc) {
    nsCOMPtr<nsIPresShell> presShell = doc->GetShell();
    if (presShell && IsRoot()) {
      bool scaling = (mPreviousScale != mCurrentScale);
      nsEventStatus status = nsEventStatus_eIgnore;
      nsGUIEvent event(true, scaling ? NS_SVG_ZOOM : NS_SVG_SCROLL, 0);
      event.eventStructType = scaling ? NS_SVGZOOM_EVENT : NS_SVG_EVENT;
      presShell->HandleDOMEventWithTarget(this, &event, &status);
      InvalidateTransformNotifyFrame();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
SVGSVGElement::SetCurrentTranslate(float x, float y)
{
  return SetCurrentScaleTranslate(mCurrentScale, x, y);
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
SVGSVGElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
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




gfxMatrix
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
    return gfxMatrix(0.0, 0.0, 0.0, 0.0, 0.0, 0.0); 
  }

  nsSVGViewBoxRect viewBox =
    GetViewBoxWithSynthesis(viewportWidth, viewportHeight);

  if (viewBox.width <= 0.0f || viewBox.height <= 0.0f) {
    return gfxMatrix(0.0, 0.0, 0.0, 0.0, 0.0, 0.0); 
  }

  return SVGContentUtils::GetViewBoxTransform(this,
                                              viewportWidth, viewportHeight,
                                              viewBox.x, viewBox.y,
                                              viewBox.width, viewBox.height,
                                              GetPreserveAspectRatioWithOverride());
}

void
SVGSVGElement::UpdateHasChildrenOnlyTransform()
{
  bool hasChildrenOnlyTransform =
    HasViewBoxOrSyntheticViewBox() ||
    (IsRoot() && (mCurrentTranslate != nsSVGTranslatePoint(0.0f, 0.0f) ||
                  mCurrentScale != 1.0f));
  mHasChildrenOnlyTransform = hasChildrenOnlyTransform;
}

void
SVGSVGElement::ChildrenOnlyTransformChanged(uint32_t aFlags)
{
  
  NS_ABORT_IF_FALSE(!(GetPrimaryFrame()->GetStateBits() &
                      NS_STATE_SVG_NONDISPLAY_CHILD),
                    "Non-display SVG frames don't maintain overflow rects");

  nsChangeHint changeHint;

  bool hadChildrenOnlyTransform = mHasChildrenOnlyTransform;

  UpdateHasChildrenOnlyTransform();

  if (hadChildrenOnlyTransform != mHasChildrenOnlyTransform) {
    
    
    changeHint = nsChangeHint_ReconstructFrame;
  } else {
    
    changeHint = nsChangeHint(nsChangeHint_RepaintFrame |
                   nsChangeHint_UpdateOverflow |
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
          NS_ENSURE_TRUE(mTimedDocumentRoot, NS_ERROR_OUT_OF_MEMORY);
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
  nsIFrame* frame = GetPrimaryFrame();
  if (frame) {
    nsISVGSVGFrame* svgframe = do_QueryFrame(frame);
    
    if (svgframe) {
      svgframe->NotifyViewportOrTransformChanged(
                  nsISVGChildFrame::TRANSFORM_CHANGED);
    }
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
  if (viewElement && viewElement->mViewBox.IsExplicitlySet()) {
    return viewElement->mViewBox.GetAnimValue();
  }
  if (mViewBox.IsExplicitlySet()) {
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

  
  
  
  if (!((viewElement && viewElement->mViewBox.IsExplicitlySet()) ||
        mViewBox.IsExplicitlySet()) &&
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

  
  if (viewElement && viewElement->mViewBox.IsExplicitlySet()) {
    viewbox = &viewElement->mViewBox.GetAnimValue();
  } else if (mViewBox.IsExplicitlySet()) {
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

  w = NS_MAX(w, 0.0f);
  h = NS_MAX(h, 0.0f);

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

  if (IsInner()) {
    float x, y;
    const_cast<SVGSVGElement*>(this)->GetAnimatedLengthValues(&x, &y, nullptr);
    if (aWhich == eAllTransforms) {
      
      return GetViewBoxTransform() * gfxMatrix().Translate(gfxPoint(x, y)) * aMatrix;
    }
    if (aWhich == eUserSpaceToParent) {
      return gfxMatrix().Translate(gfxPoint(x, y)) * aMatrix;
    }
    NS_ABORT_IF_FALSE(aWhich == eChildToUserSpace, "Unknown TransformTypes");
    return GetViewBoxTransform(); 
  }

  if (aWhich == eUserSpaceToParent) {
    
    return aMatrix;
  }

  if (IsRoot()) {
    gfxMatrix zoomPanTM;
    zoomPanTM.Translate(gfxPoint(mCurrentTranslate.GetX(), mCurrentTranslate.GetY()));
    zoomPanTM.Scale(mCurrentScale, mCurrentScale);
    gfxMatrix matrix = mFragmentIdentifierTransform ? 
                         *mFragmentIdentifierTransform * aMatrix : aMatrix;
    return GetViewBoxTransform() * zoomPanTM * matrix;
  }

  
  return GetViewBoxTransform() * aMatrix;
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
SVGSVGElement::HasViewBox() const
{
  SVGViewElement* viewElement = GetCurrentViewElement();
  if (viewElement && viewElement->mViewBox.IsExplicitlySet()) {
    return true;
  }
  return mViewBox.IsExplicitlySet();
}

bool
SVGSVGElement::ShouldSynthesizeViewBox() const
{
  NS_ABORT_IF_FALSE(!HasViewBox(),
                    "Should only be called if we lack a viewBox");

  nsIDocument* doc = GetCurrentDoc();
  return doc &&
    doc->IsBeingUsedAsImage() &&
    !mIsPaintingSVGImageElement &&
    !GetParent();
}



static void
ReleasePreserveAspectRatioPropertyValue(void*    aObject,       
                                        nsIAtom* aPropertyName, 
                                        void*    aPropertyValue,
                                        void*    aData          )
{
  SVGPreserveAspectRatio* valPtr =
    static_cast<SVGPreserveAspectRatio*>(aPropertyValue);
  delete valPtr;
}

bool
SVGSVGElement::SetPreserveAspectRatioProperty(const SVGPreserveAspectRatio& aPAR)
{
  SVGPreserveAspectRatio* pAROverridePtr = new SVGPreserveAspectRatio(aPAR);
  nsresult rv = SetProperty(nsGkAtoms::overridePreserveAspectRatio,
                            pAROverridePtr,
                            ReleasePreserveAspectRatioPropertyValue,
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

  bool hasViewBox = HasViewBox();
  if (!hasViewBox && ShouldSynthesizeViewBox()) {
    
    
    
    mImageNeedsTransformInvalidation = true;
  }
  mIsPaintingSVGImageElement = true;

  if (!hasViewBox) {
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
  if (!HasViewBox() && ShouldSynthesizeViewBox()) {
    
    
    
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


static void
ReleaseViewBoxPropertyValue(void*    aObject,       
                            nsIAtom* aPropertyName, 
                            void*    aPropertyValue,
                            void*    aData          )
{
  nsSVGViewBoxRect* valPtr =
    static_cast<nsSVGViewBoxRect*>(aPropertyValue);
  delete valPtr;
}

bool
SVGSVGElement::SetViewBoxProperty(const nsSVGViewBoxRect& aViewBox)
{
  nsSVGViewBoxRect* pViewBoxOverridePtr = new nsSVGViewBoxRect(aViewBox);
  nsresult rv = SetProperty(nsGkAtoms::viewBox,
                            pViewBoxOverridePtr,
                            ReleaseViewBoxPropertyValue,
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

} 
} 
