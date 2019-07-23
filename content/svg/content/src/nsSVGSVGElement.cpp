







































#include "nsGkAtoms.h"
#include "nsSVGLength.h"
#include "nsSVGAngle.h"
#include "nsCOMPtr.h"
#include "nsIPresShell.h"
#include "nsContentUtils.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsSVGMatrix.h"
#include "nsSVGPoint.h"
#include "nsSVGTransform.h"
#include "nsIDOMEventTarget.h"
#include "nsIFrame.h"
#include "nsISVGSVGFrame.h" 
#include "nsSVGNumber.h"
#include "nsSVGRect.h"
#include "nsISVGValueUtils.h"
#include "nsDOMError.h"
#include "nsISVGChildFrame.h"
#include "nsGUIEvent.h"
#include "nsSVGUtils.h"
#include "nsSVGSVGElement.h"

#ifdef MOZ_SMIL
#include "nsEventDispatcher.h"
#include "nsSMILTimeContainer.h"
#include "nsSMILAnimationController.h"
#include "nsSMILTypes.h"
#include "nsIContentIterator.h"

nsresult NS_NewContentIterator(nsIContentIterator** aInstancePtrResult);
#endif 

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGTranslatePoint::DOMVal, mElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGTranslatePoint::DOMVal)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGTranslatePoint::DOMVal)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGTranslatePoint::DOMVal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPoint)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGPoint)
NS_INTERFACE_MAP_END

nsresult
nsSVGTranslatePoint::ToDOMVal(nsSVGSVGElement *aElement,
                              nsIDOMSVGPoint **aResult)
{
  *aResult = new DOMVal(this, aElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  
  NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGTranslatePoint::DOMVal::SetX(float aValue)
{
  NS_ENSURE_FINITE(aValue, NS_ERROR_ILLEGAL_VALUE);
  return mElement->SetCurrentTranslate(aValue, mVal->GetY());
}

NS_IMETHODIMP
nsSVGTranslatePoint::DOMVal::SetY(float aValue)
{
  NS_ENSURE_FINITE(aValue, NS_ERROR_ILLEGAL_VALUE);
  return mElement->SetCurrentTranslate(mVal->GetX(), aValue);
}


NS_IMETHODIMP
nsSVGTranslatePoint::DOMVal::MatrixTransform(nsIDOMSVGMatrix *matrix,
                                             nsIDOMSVGPoint **_retval)
{
  if (!matrix)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;

  float a, b, c, d, e, f;
  matrix->GetA(&a);
  matrix->GetB(&b);
  matrix->GetC(&c);
  matrix->GetD(&d);
  matrix->GetE(&e);
  matrix->GetF(&f);

  float x = mVal->GetX();
  float y = mVal->GetY();
  
  return NS_NewSVGPoint(_retval, a*x + c*y + e, b*x + d*y + f);
}

nsSVGElement::LengthInfo nsSVGSVGElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::y, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
  { &nsGkAtoms::width, 100, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::height, 100, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
};

nsSVGEnumMapping nsSVGSVGElement::sZoomAndPanMap[] = {
  {&nsGkAtoms::disable, nsIDOMSVGZoomAndPan::SVG_ZOOMANDPAN_DISABLE},
  {&nsGkAtoms::magnify, nsIDOMSVGZoomAndPan::SVG_ZOOMANDPAN_MAGNIFY},
  {nsnull, 0}
};

nsSVGElement::EnumInfo nsSVGSVGElement::sEnumInfo[1] =
{
  { &nsGkAtoms::zoomAndPan,
    sZoomAndPanMap,
    nsIDOMSVGZoomAndPan::SVG_ZOOMANDPAN_MAGNIFY
  }
};


nsresult
NS_NewSVGSVGElement(nsIContent **aResult, nsINodeInfo *aNodeInfo,
                    PRBool aFromParser)
{
  nsSVGSVGElement *it = new nsSVGSVGElement(aNodeInfo, aFromParser);
  if (!it)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(it);

  nsresult rv = it->Init();

  if (NS_FAILED(rv)) {
    NS_RELEASE(it);
    return rv;
  }

  *aResult = it;

  return rv;
}




NS_IMPL_ADDREF_INHERITED(nsSVGSVGElement,nsSVGSVGElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGSVGElement,nsSVGSVGElementBase)

NS_INTERFACE_TABLE_HEAD(nsSVGSVGElement)
  NS_NODE_INTERFACE_TABLE7(nsSVGSVGElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGSVGElement,
                           nsIDOMSVGFitToViewBox, nsIDOMSVGLocatable,
                           nsIDOMSVGZoomAndPan)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGSVGElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGSVGElementBase)




nsSVGSVGElement::nsSVGSVGElement(nsINodeInfo* aNodeInfo, PRBool aFromParser)
  : nsSVGSVGElementBase(aNodeInfo),
    mCoordCtx(nsnull),
    mViewportWidth(0),
    mViewportHeight(0),
    mCoordCtxMmPerPx(0),
    mCurrentTranslate(0.0f, 0.0f),
    mCurrentScale(1.0f),
    mPreviousTranslate(0.0f, 0.0f),
    mPreviousScale(1.0f),
    mRedrawSuspendCount(0)
#ifdef MOZ_SMIL
    ,mStartAnimationOnBindToTree(!aFromParser)
#endif 
{
}





nsresult
nsSVGSVGElement::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nsnull;

  nsSVGSVGElement *it = new nsSVGSVGElement(aNodeInfo, PR_FALSE);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsINode> kungFuDeathGrip = it;
  nsresult rv = it->Init();
  rv |= CopyInnerTo(it);
  if (NS_SUCCEEDED(rv)) {
    kungFuDeathGrip.swap(*aResult);
  }

  return rv;
}






NS_IMETHODIMP
nsSVGSVGElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  return mLengthAttributes[X].ToDOMAnimatedLength(aX, this);
}


NS_IMETHODIMP
nsSVGSVGElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  return mLengthAttributes[Y].ToDOMAnimatedLength(aY, this);
}


NS_IMETHODIMP
nsSVGSVGElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  return mLengthAttributes[WIDTH].ToDOMAnimatedLength(aWidth, this);
}


NS_IMETHODIMP
nsSVGSVGElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  return mLengthAttributes[HEIGHT].ToDOMAnimatedLength(aHeight, this);
}


NS_IMETHODIMP
nsSVGSVGElement::GetContentScriptType(nsAString & aContentScriptType)
{
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::GetContentScriptType");
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsSVGSVGElement::SetContentScriptType(const nsAString & aContentScriptType)
{
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::SetContentScriptType");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::GetContentStyleType(nsAString & aContentStyleType)
{
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::GetContentStyleType");
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsSVGSVGElement::SetContentStyleType(const nsAString & aContentStyleType)
{
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::SetContentStyleType");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::GetViewport(nsIDOMSVGRect * *aViewport)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::GetPixelUnitToMillimeterX(float *aPixelUnitToMillimeterX)
{
  
  
  nsPresContext *context = nsContentUtils::GetContextForContent(this);
  if (!context) {
    *aPixelUnitToMillimeterX = 0.28f; 
    return NS_OK;
  }

  *aPixelUnitToMillimeterX = 25.4f / nsPresContext::AppUnitsToIntCSSPixels(context->AppUnitsPerInch());
  return NS_OK;
}


NS_IMETHODIMP
nsSVGSVGElement::GetPixelUnitToMillimeterY(float *aPixelUnitToMillimeterY)
{
  return GetPixelUnitToMillimeterX(aPixelUnitToMillimeterY);
}


NS_IMETHODIMP
nsSVGSVGElement::GetScreenPixelToMillimeterX(float *aScreenPixelToMillimeterX)
{
  
  
  nsPresContext *context = nsContentUtils::GetContextForContent(this);
  if (!context) {
    *aScreenPixelToMillimeterX = 0.28f; 
    return NS_OK;
  }

  *aScreenPixelToMillimeterX = 25.4f /
      nsPresContext::AppUnitsToIntCSSPixels(context->AppUnitsPerInch());
  return NS_OK;
}


NS_IMETHODIMP
nsSVGSVGElement::GetScreenPixelToMillimeterY(float *aScreenPixelToMillimeterY)
{
  return GetScreenPixelToMillimeterX(aScreenPixelToMillimeterY);
}


NS_IMETHODIMP
nsSVGSVGElement::GetUseCurrentView(PRBool *aUseCurrentView)
{
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::GetUseCurrentView");
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsSVGSVGElement::SetUseCurrentView(PRBool aUseCurrentView)
{
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::SetUseCurrentView");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::GetCurrentView(nsIDOMSVGViewSpec * *aCurrentView)
{
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::GetCurrentView");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::GetCurrentScale(float *aCurrentScale)
{
  *aCurrentScale = mCurrentScale;
  return NS_OK;
}

#define CURRENT_SCALE_MAX 16.0f
#define CURRENT_SCALE_MIN 0.0625f

NS_IMETHODIMP
nsSVGSVGElement::SetCurrentScale(float aCurrentScale)
{
  return SetCurrentScaleTranslate(aCurrentScale,
    mCurrentTranslate.GetX(), mCurrentTranslate.GetY());
}


NS_IMETHODIMP
nsSVGSVGElement::GetCurrentTranslate(nsIDOMSVGPoint * *aCurrentTranslate)
{
  return mCurrentTranslate.ToDOMVal(this, aCurrentTranslate);
}


NS_IMETHODIMP
nsSVGSVGElement::SuspendRedraw(PRUint32 max_wait_milliseconds, PRUint32 *_retval)
{
  *_retval = 1;

  if (++mRedrawSuspendCount > 1) 
    return NS_OK;

  nsIFrame* frame = GetPrimaryFrame();
#ifdef DEBUG
  
  
  
  
  
  
  NS_ASSERTION(frame, "suspending redraw w/o frame");
#endif
  if (frame) {
    nsISVGSVGFrame* svgframe = do_QueryFrame(frame);
    NS_ASSERTION(svgframe, "wrong frame type");
    if (svgframe) {
      svgframe->SuspendRedraw();
    }
  }
  
  return NS_OK;
}


NS_IMETHODIMP
nsSVGSVGElement::UnsuspendRedraw(PRUint32 suspend_handle_id)
{
  if (mRedrawSuspendCount == 0) {
    NS_ASSERTION(1==0, "unbalanced suspend/unsuspend calls");
    return NS_ERROR_FAILURE;
  }
                 
  if (mRedrawSuspendCount > 1) {
    --mRedrawSuspendCount;
    return NS_OK;
  }
  
  return UnsuspendRedrawAll();
}


NS_IMETHODIMP
nsSVGSVGElement::UnsuspendRedrawAll()
{
  mRedrawSuspendCount = 0;

  nsIFrame* frame = GetPrimaryFrame();
#ifdef DEBUG
  NS_ASSERTION(frame, "unsuspending redraw w/o frame");
#endif
  if (frame) {
    nsISVGSVGFrame* svgframe = do_QueryFrame(frame);
    NS_ASSERTION(svgframe, "wrong frame type");
    if (svgframe) {
      svgframe->UnsuspendRedraw();
    }
  }  
  return NS_OK;
}


NS_IMETHODIMP
nsSVGSVGElement::ForceRedraw()
{
  nsIDocument* doc = GetCurrentDoc();
  if (!doc) return NS_ERROR_FAILURE;

  doc->FlushPendingNotifications(Flush_Display);

  return NS_OK;
}


NS_IMETHODIMP
nsSVGSVGElement::PauseAnimations()
{
#ifdef MOZ_SMIL
  if (NS_SMILEnabled()) {
    if (mTimedDocumentRoot) {
      mTimedDocumentRoot->Pause(nsSMILTimeContainer::PAUSE_SCRIPT);
    }
    
    return NS_OK;
  }
#endif 
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::PauseAnimations");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::UnpauseAnimations()
{
#ifdef MOZ_SMIL
  if (NS_SMILEnabled()) {
    if (mTimedDocumentRoot) {
      mTimedDocumentRoot->Resume(nsSMILTimeContainer::PAUSE_SCRIPT);
    }
    
    return NS_OK;
  }
#endif 
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::UnpauseAnimations");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::AnimationsPaused(PRBool *_retval)
{
#ifdef MOZ_SMIL
  if (NS_SMILEnabled()) {
    nsSMILTimeContainer* root = GetTimedDocumentRoot();
    *_retval = root && root->IsPausedByType(nsSMILTimeContainer::PAUSE_SCRIPT);
    return NS_OK;
  }
#endif 
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::AnimationsPaused");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::GetCurrentTime(float *_retval)
{
#ifdef MOZ_SMIL
  if (NS_SMILEnabled()) {
    nsSMILTimeContainer* root = GetTimedDocumentRoot();
    if (root) {
      double fCurrentTimeMs = double(root->GetCurrentTime());
      *_retval = (float)(fCurrentTimeMs / PR_MSEC_PER_SEC);
    } else {
      *_retval = 0.f;
    }
    return NS_OK;
  }
#endif 
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::GetCurrentTime");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::SetCurrentTime(float seconds)
{
  NS_ENSURE_FINITE(seconds, NS_ERROR_ILLEGAL_VALUE);
#ifdef MOZ_SMIL
  if (NS_SMILEnabled()) {
    if (mTimedDocumentRoot) {
      double fMilliseconds = double(seconds) * PR_MSEC_PER_SEC;
      
      
      nsSMILTime lMilliseconds = PRInt64(NS_round(fMilliseconds));
      mTimedDocumentRoot->SetCurrentTime(lMilliseconds);
      
      
      
      
      
      
      
      nsIDocument* doc = GetCurrentDoc();
      if (doc) {
        nsSMILAnimationController* smilController = doc->GetAnimationController();
        if (smilController) {
          smilController->Resample();
        }
      }
    } 
      
    return NS_OK;
  }
#endif 
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::SetCurrentTime");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::GetIntersectionList(nsIDOMSVGRect *rect,
                                     nsIDOMSVGElement *referenceElement,
                                     nsIDOMNodeList **_retval)
{
  
  
  

  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::GetIntersectionList");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::GetEnclosureList(nsIDOMSVGRect *rect,
                                  nsIDOMSVGElement *referenceElement,
                                  nsIDOMNodeList **_retval)
{
  
  
  

  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::GetEnclosureList");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::CheckIntersection(nsIDOMSVGElement *element,
                                   nsIDOMSVGRect *rect,
                                   PRBool *_retval)
{
  
  
  

  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::CheckIntersection");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::CheckEnclosure(nsIDOMSVGElement *element,
                                nsIDOMSVGRect *rect,
                                PRBool *_retval)
{
  
  
  

  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::CheckEnclosure");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::DeSelectAll()
{
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::DeSelectAll");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::CreateSVGNumber(nsIDOMSVGNumber **_retval)
{
  return NS_NewSVGNumber(_retval);
}


NS_IMETHODIMP
nsSVGSVGElement::CreateSVGLength(nsIDOMSVGLength **_retval)
{
  return NS_NewSVGLength(reinterpret_cast<nsISVGLength**>(_retval));
}


NS_IMETHODIMP
nsSVGSVGElement::CreateSVGAngle(nsIDOMSVGAngle **_retval)
{
  return NS_NewDOMSVGAngle(_retval);
}


NS_IMETHODIMP
nsSVGSVGElement::CreateSVGPoint(nsIDOMSVGPoint **_retval)
{
  return NS_NewSVGPoint(_retval);
}


NS_IMETHODIMP
nsSVGSVGElement::CreateSVGMatrix(nsIDOMSVGMatrix **_retval)
{
  return NS_NewSVGMatrix(_retval);
}


NS_IMETHODIMP
nsSVGSVGElement::CreateSVGRect(nsIDOMSVGRect **_retval)
{
  return NS_NewSVGRect(_retval);
}


NS_IMETHODIMP
nsSVGSVGElement::CreateSVGTransform(nsIDOMSVGTransform **_retval)
{
  return NS_NewSVGTransform(_retval);
}


NS_IMETHODIMP
nsSVGSVGElement::CreateSVGTransformFromMatrix(nsIDOMSVGMatrix *matrix, 
                                              nsIDOMSVGTransform **_retval)
{
  if (!matrix)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;

  nsresult rv = NS_NewSVGTransform(_retval);
  if (NS_FAILED(rv))
    return rv;

  (*_retval)->SetMatrix(matrix);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGSVGElement::CreateSVGString(nsAString & _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::GetElementById(const nsAString & elementId, nsIDOMElement **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}





NS_IMETHODIMP
nsSVGSVGElement::GetViewBox(nsIDOMSVGAnimatedRect * *aViewBox)
{
  return mViewBox.ToDOMAnimatedRect(aViewBox, this);
}


NS_IMETHODIMP
nsSVGSVGElement::GetPreserveAspectRatio(nsIDOMSVGAnimatedPreserveAspectRatio
                                        **aPreserveAspectRatio)
{
  return mPreserveAspectRatio.ToDOMAnimatedPreserveAspectRatio(aPreserveAspectRatio, this);
}





NS_IMETHODIMP
nsSVGSVGElement::GetNearestViewportElement(nsIDOMSVGElement * *aNearestViewportElement)
{
  return nsSVGUtils::GetNearestViewportElement(this, aNearestViewportElement);
}


NS_IMETHODIMP
nsSVGSVGElement::GetFarthestViewportElement(nsIDOMSVGElement * *aFarthestViewportElement)
{
  return nsSVGUtils::GetFarthestViewportElement(this, aFarthestViewportElement);
}


NS_IMETHODIMP
nsSVGSVGElement::GetBBox(nsIDOMSVGRect **_retval)
{
  *_retval = nsnull;

  nsIFrame* frame = GetPrimaryFrame(Flush_Layout);

  if (!frame || (frame->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD))
    return NS_ERROR_FAILURE;

  nsISVGChildFrame* svgframe = do_QueryFrame(frame);
  if (svgframe) {
    *_retval = nsSVGUtils::GetBBox(frame).get();
    return NS_OK;
  } else {
    
    return NS_ERROR_NOT_IMPLEMENTED;
  }
}


NS_IMETHODIMP
nsSVGSVGElement::GetCTM(nsIDOMSVGMatrix **_retval)
{
  nsresult rv;
  *_retval = nsnull;

  nsIDocument* currentDoc = GetCurrentDoc();
  if (currentDoc) {
    
    currentDoc->FlushPendingNotifications(Flush_Layout);
  }

  

  nsCOMPtr<nsIContent> element = this;
  nsCOMPtr<nsIContent> ancestor;
  unsigned short ancestorCount = 0;
  nsCOMPtr<nsIDOMSVGMatrix> ancestorCTM;

  while (1) {
    ancestor = nsSVGUtils::GetParentElement(element);
    if (!ancestor) {
      
      break;
    }

    nsSVGSVGElement *viewportElement = QI_AND_CAST_TO_NSSVGSVGELEMENT(ancestor);
    if (viewportElement) {
      rv = viewportElement->GetViewboxToViewportTransform(getter_AddRefs(ancestorCTM));
      if (NS_FAILED(rv)) return rv;
      break;
    }

    nsCOMPtr<nsIDOMSVGLocatable> locatableElement = do_QueryInterface(ancestor);
    if (locatableElement) {
      rv = locatableElement->GetCTM(getter_AddRefs(ancestorCTM));
      if (NS_FAILED(rv)) return rv;
      break;
    }

    
    element = ancestor;
    ancestorCount++;
  }

  

  if (!ancestorCTM) {
    
    float s=1, x=0, y=0;
    if (IsRoot()) {
      
      s = mCurrentScale;
      x = mCurrentTranslate.GetX();
      y = mCurrentTranslate.GetY();
    }
    else {
      
      GetOffsetToAncestor(nsnull, x, y);
    }
    rv = NS_NewSVGMatrix(getter_AddRefs(ancestorCTM), s, 0, 0, s, x, y);
    if (NS_FAILED(rv)) return rv;
  }
  else {
    
    float x=0, y=0;
    nsCOMPtr<nsIDOMSVGMatrix> tmp;
    if (ancestorCount == 0) {
      
      
      x = mLengthAttributes[X].GetAnimValue(static_cast<nsSVGElement*>
                                                       (this));
      y = mLengthAttributes[Y].GetAnimValue(static_cast<nsSVGElement*>
                                                       (this));
    }
    else {
      
#if 0
      nsCOMPtr<nsIDOMSVGForeignObjectElement> foreignObject
                                              = do_QueryInterface(ancestor);
      if (!foreignObject) {
        NS_ERROR("the none-SVG content in the parent chain between us and our "
                 "SVG ancestor isn't rooted in a foreignObject element");
        return NS_ERROR_FAILURE;
      }
#endif
      
      
      GetOffsetToAncestor(ancestor, x, y);
    }
    rv = ancestorCTM->Translate(x, y, getter_AddRefs(tmp));
    if (NS_FAILED(rv)) return rv;
    ancestorCTM.swap(tmp);
  }

  

  nsCOMPtr<nsIDOMSVGMatrix> tmp;
  rv = GetViewboxToViewportTransform(getter_AddRefs(tmp));
  if (NS_FAILED(rv)) return rv;
  return ancestorCTM->Multiply(tmp, _retval);  
}


NS_IMETHODIMP
nsSVGSVGElement::GetScreenCTM(nsIDOMSVGMatrix **_retval)
{
  nsresult rv;
  *_retval = nsnull;

  nsIDocument* currentDoc = GetCurrentDoc();
  if (currentDoc) {
    
    currentDoc->FlushPendingNotifications(Flush_Layout);
  }

  

  nsCOMPtr<nsIContent> element = this;
  nsCOMPtr<nsIContent> ancestor;
  unsigned short ancestorCount = 0;
  nsCOMPtr<nsIDOMSVGMatrix> ancestorScreenCTM;

  while (1) {
    ancestor = nsSVGUtils::GetParentElement(element);
    if (!ancestor) {
      
      break;
    }

    nsCOMPtr<nsIDOMSVGLocatable> locatableElement = do_QueryInterface(ancestor);
    if (locatableElement) {
      rv = locatableElement->GetScreenCTM(getter_AddRefs(ancestorScreenCTM));
      if (NS_FAILED(rv)) return rv;
      break;
    }

    
    element = ancestor;
    ancestorCount++;
  }

  

  if (!ancestorScreenCTM) {
    
    float s=1, x=0, y=0;
    if (IsRoot()) {
      
      s = mCurrentScale;
      x = mCurrentTranslate.GetX();
      y = mCurrentTranslate.GetY();
    }
    else {
      
      GetOffsetToAncestor(nsnull, x, y);
    }
    rv = NS_NewSVGMatrix(getter_AddRefs(ancestorScreenCTM), s, 0, 0, s, x, y);
    if (NS_FAILED(rv)) return rv;
  }
  else {
    
    float x=0, y=0;
    nsCOMPtr<nsIDOMSVGMatrix> tmp;
    if (ancestorCount == 0) {
      
      
      x = mLengthAttributes[X].GetAnimValue(static_cast<nsSVGElement*>
                                                       (this));
      y = mLengthAttributes[Y].GetAnimValue(static_cast<nsSVGElement*>
                                                       (this));
    }
    else {
      
#if 0
      nsCOMPtr<nsIDOMSVGForeignObjectElement> foreignObject
                                              = do_QueryInterface(ancestor);
      if (!foreignObject) {
        NS_ERROR("the none-SVG content in the parent chain between us and our "
                 "SVG ancestor isn't rooted in a foreignObject element");
        return NS_ERROR_FAILURE;
      }
#endif
      
      
      GetOffsetToAncestor(ancestor, x, y);
    }
    rv = ancestorScreenCTM->Translate(x, y, getter_AddRefs(tmp));
    if (NS_FAILED(rv)) return rv;
    ancestorScreenCTM.swap(tmp);
  }

  

  nsCOMPtr<nsIDOMSVGMatrix> tmp;
  rv = GetViewboxToViewportTransform(getter_AddRefs(tmp));
  if (NS_FAILED(rv)) return rv;
  return ancestorScreenCTM->Multiply(tmp, _retval);  
}


NS_IMETHODIMP
nsSVGSVGElement::GetTransformToElement(nsIDOMSVGElement *element,
                                       nsIDOMSVGMatrix **_retval)
{
  if (!element)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;

  nsresult rv;
  *_retval = nsnull;
  nsCOMPtr<nsIDOMSVGMatrix> ourScreenCTM;
  nsCOMPtr<nsIDOMSVGMatrix> targetScreenCTM;
  nsCOMPtr<nsIDOMSVGMatrix> tmp;
  nsCOMPtr<nsIDOMSVGLocatable> target = do_QueryInterface(element, &rv);
  if (NS_FAILED(rv)) return rv;

  
  rv = GetScreenCTM(getter_AddRefs(ourScreenCTM));
  if (NS_FAILED(rv)) return rv;
  rv = target->GetScreenCTM(getter_AddRefs(targetScreenCTM));
  if (NS_FAILED(rv)) return rv;
  rv = targetScreenCTM->Inverse(getter_AddRefs(tmp));
  if (NS_FAILED(rv)) return rv;
  return tmp->Multiply(ourScreenCTM, _retval);  
}





NS_IMETHODIMP
nsSVGSVGElement::GetZoomAndPan(PRUint16 *aZoomAndPan)
{
  *aZoomAndPan = mEnumAttributes[ZOOMANDPAN].GetAnimValue();
  return NS_OK;
}

NS_IMETHODIMP
nsSVGSVGElement::SetZoomAndPan(PRUint16 aZoomAndPan)
{
  if (aZoomAndPan == nsIDOMSVGZoomAndPan::SVG_ZOOMANDPAN_DISABLE ||
      aZoomAndPan == nsIDOMSVGZoomAndPan::SVG_ZOOMANDPAN_MAGNIFY) {
    mEnumAttributes[ZOOMANDPAN].SetBaseValue(aZoomAndPan, this, PR_TRUE);
    return NS_OK;
  }

  return NS_ERROR_DOM_SVG_INVALID_VALUE_ERR;
}




NS_IMETHODIMP
nsSVGSVGElement::SetCurrentScaleTranslate(float s, float x, float y)
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
    nsCOMPtr<nsIPresShell> presShell = doc->GetPrimaryShell();
    NS_ASSERTION(presShell, "no presShell");
    if (presShell && IsRoot()) {
      PRBool scaling = (s != mCurrentScale);
      nsEventStatus status = nsEventStatus_eIgnore;
      nsGUIEvent event(PR_TRUE, scaling ? NS_SVG_ZOOM : NS_SVG_SCROLL, 0);
      event.eventStructType = scaling ? NS_SVGZOOM_EVENT : NS_SVG_EVENT;
      presShell->HandleDOMEventWithTarget(this, &event, &status);
      InvalidateTransformNotifyFrame();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSVGSVGElement::SetCurrentTranslate(float x, float y)
{
  return SetCurrentScaleTranslate(mCurrentScale, x, y);
}

#ifdef MOZ_SMIL
nsSMILTimeContainer*
nsSVGSVGElement::GetTimedDocumentRoot()
{
  nsSMILTimeContainer *result = nsnull;

  if (mTimedDocumentRoot) {
    result = mTimedDocumentRoot;
  } else {
    
    nsCOMPtr<nsIDOMSVGSVGElement> outerSVGDOM;

    nsresult rv = GetOwnerSVGElement(getter_AddRefs(outerSVGDOM));

    if (NS_SUCCEEDED(rv) && outerSVGDOM) {
      nsSVGSVGElement *outerSVG =
        static_cast<nsSVGSVGElement*>(outerSVGDOM.get());
      result = outerSVG->GetTimedDocumentRoot();
    }
  }

  return result;
}
#endif 




NS_IMETHODIMP_(PRBool)
nsSVGSVGElement::IsAttributeMapped(const nsIAtom* name) const
{
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

  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGSVGElementBase::IsAttributeMapped(name);
}




#ifdef MOZ_SMIL
nsresult
nsSVGSVGElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  if (aVisitor.mEvent->message == NS_SVG_LOAD) {
    if (mTimedDocumentRoot) {
      mTimedDocumentRoot->Begin();
    }
  }
  return nsSVGSVGElementBase::PreHandleEvent(aVisitor);
}
#endif 




PRBool
nsSVGSVGElement::IsEventName(nsIAtom* aName)
{
  





  return nsContentUtils::IsEventAttributeName(aName,
         (EventNameType_SVGGraphic | EventNameType_SVGSVG));
}




nsresult
nsSVGSVGElement::GetViewboxToViewportTransform(nsIDOMSVGMatrix **_retval)
{
  *_retval = nsnull;

  float viewportWidth, viewportHeight;
  nsSVGSVGElement *ctx = GetCtx();
  if (!ctx) {
    
    viewportWidth = mViewportWidth;
    viewportHeight = mViewportHeight;
  } else {
    viewportWidth = mLengthAttributes[WIDTH].GetAnimValue(ctx);
    viewportHeight = mLengthAttributes[HEIGHT].GetAnimValue(ctx);
  }

  nsSVGViewBoxRect viewbox;
  if (mViewBox.IsValid()) {
    viewbox = mViewBox.GetAnimValue();
  } else {
    viewbox.x = viewbox.y = 0.0f;
    viewbox.width  = viewportWidth;
    viewbox.height = viewportHeight;
  }

  if (viewbox.width <= 0.0f || viewbox.height <= 0.0f) {
    return NS_ERROR_FAILURE; 
  }

  nsCOMPtr<nsIDOMSVGMatrix> xform =
    nsSVGUtils::GetViewBoxTransform(viewportWidth, viewportHeight,
                                    viewbox.x, viewbox.y,
                                    viewbox.width, viewbox.height,
                                    mPreserveAspectRatio);
  xform.swap(*_retval);

  return NS_OK;
}

#ifdef MOZ_SMIL
nsresult
nsSVGSVGElement::BindToTree(nsIDocument* aDocument,
                            nsIContent* aParent,
                            nsIContent* aBindingParent,
                            PRBool aCompileEventHandlers)
{
  nsSMILAnimationController* smilController = nsnull;

  if (aDocument) {
    smilController = aDocument->GetAnimationController();
    if (smilController) {
      
      if (WillBeOutermostSVG(aParent, aBindingParent)) {
        
        if (!mTimedDocumentRoot) {
          mTimedDocumentRoot = new nsSMILTimeContainer();
          NS_ENSURE_TRUE(mTimedDocumentRoot, NS_ERROR_OUT_OF_MEMORY);
        }
      } else {
        
        
        
        mTimedDocumentRoot = nsnull;
        mStartAnimationOnBindToTree = PR_TRUE;
      }
    }
  }

  nsresult rv = nsSVGSVGElementBase::BindToTree(aDocument, aParent,
                                                aBindingParent,
                                                aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv,rv);

  if (mTimedDocumentRoot && smilController) {
    rv = mTimedDocumentRoot->SetParent(smilController);
    if (mStartAnimationOnBindToTree) {
      mTimedDocumentRoot->Begin();
    }
  }

  return rv;
}

void
nsSVGSVGElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  if (mTimedDocumentRoot) {
    mTimedDocumentRoot->SetParent(nsnull);
  }

  nsSVGSVGElementBase::UnbindFromTree(aDeep, aNullParent);
}
#endif 





void nsSVGSVGElement::GetOffsetToAncestor(nsIContent* ancestor,
                                          float &x, float &y)
{
  x = 0.0f;
  y = 0.0f;

  nsIDocument *document = GetCurrentDoc();
  if (!document) return;

  
  
  
  document->FlushPendingNotifications(Flush_Layout);
  
  nsIPresShell *presShell = document->GetPrimaryShell();
  if (!presShell) {
    return;
  }

  nsPresContext *context = presShell->GetPresContext();
  if (!context) {
    return;
  }

  nsIFrame* frame = presShell->GetPrimaryFrameFor(this);
  nsIFrame* ancestorFrame = ancestor ?
                            presShell->GetPrimaryFrameFor(ancestor) :
                            presShell->GetRootFrame();

  if (frame && ancestorFrame) {
    nsPoint point = frame->GetOffsetTo(ancestorFrame);
    x = nsPresContext::AppUnitsToFloatCSSPixels(point.x);
    y = nsPresContext::AppUnitsToFloatCSSPixels(point.y);
  }
}

#ifdef MOZ_SMIL
PRBool
nsSVGSVGElement::WillBeOutermostSVG(nsIContent* aParent,
                                    nsIContent* aBindingParent) const
{
  nsIContent* parent = aBindingParent ? aBindingParent : aParent;

  while (parent && parent->GetNameSpaceID() == kNameSpaceID_SVG) {
    nsIAtom* tag = parent->Tag();
    if (tag == nsGkAtoms::foreignObject) {
      
      return PR_FALSE;
    }
    if (tag == nsGkAtoms::svg) {
      return PR_FALSE;
    }
    parent = parent->GetParent();
  }

  return PR_TRUE;
}
#endif 

void
nsSVGSVGElement::InvalidateTransformNotifyFrame()
{
  nsIDocument* doc = GetCurrentDoc();
  if (!doc) return;
  nsIPresShell* presShell = doc->GetPrimaryShell();
  if (!presShell) return;

  nsIFrame* frame = presShell->GetPrimaryFrameFor(this);
  if (frame) {
    nsISVGSVGFrame* svgframe = do_QueryFrame(frame);
    if (svgframe) {
      svgframe->NotifyViewportChange();
    }
#ifdef DEBUG
    else {
      
      
      
      NS_WARNING("wrong frame type");
    }
#endif
  }
}




float
nsSVGSVGElement::GetLength(PRUint8 aCtxType)
{
  float h, w;

  if (mViewBox.IsValid()) {
    const nsSVGViewBoxRect& viewbox = mViewBox.GetAnimValue();
    w = viewbox.width;
    h = viewbox.height;
  } else {
    nsSVGSVGElement *ctx = GetCtx();
    if (ctx) {
      w = mLengthAttributes[WIDTH].GetAnimValue(ctx);
      h = mLengthAttributes[HEIGHT].GetAnimValue(ctx);
    } else {
      w = mViewportWidth;
      h = mViewportHeight;
    }
  }

  w = PR_MAX(w, 0.0f);
  h = PR_MAX(h, 0.0f);

  switch (aCtxType) {
  case nsSVGUtils::X:
    return w;
  case nsSVGUtils::Y:
    return h;
  case nsSVGUtils::XY:
    return float(nsSVGUtils::ComputeNormalizedHypotenuse(w, h));
  }
  return 0;
}

float
nsSVGSVGElement::GetMMPerPx(PRUint8 aCtxType)
{
  if (mCoordCtxMmPerPx == 0.0f) {
    GetScreenPixelToMillimeterX(&mCoordCtxMmPerPx);
  }
  return mCoordCtxMmPerPx;
}




 gfxMatrix
nsSVGSVGElement::PrependLocalTransformTo(const gfxMatrix &aMatrix)
{
  NS_ASSERTION(GetCtx(), "Should not be called on outer-<svg>");

  float x, y;
  GetAnimatedLengthValues(&x, &y, nsnull);
  gfxMatrix matrix = aMatrix;
  matrix.PreMultiply(gfxMatrix().Translate(gfxPoint(x, y)));

  nsCOMPtr<nsIDOMSVGMatrix> viewBoxTM;
  nsresult res = GetViewboxToViewportTransform(getter_AddRefs(viewBoxTM));
  if (NS_FAILED(res)) {
    return gfxMatrix(0.0, 0.0, 0.0, 0.0, 0.0, 0.0); 
  }
  return matrix.PreMultiply(nsSVGUtils::ConvertSVGMatrixToThebes(viewBoxTM));
}

void
nsSVGSVGElement::DidChangeLength(PRUint8 aAttrEnum, PRBool aDoSetAttr)
{
  nsSVGSVGElementBase::DidChangeLength(aAttrEnum, aDoSetAttr);

  InvalidateTransformNotifyFrame();
}

nsSVGElement::LengthAttributesInfo
nsSVGSVGElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
}

void
nsSVGSVGElement::DidChangeEnum(PRUint8 aAttrEnum, PRBool aDoSetAttr)
{
  nsSVGSVGElementBase::DidChangeEnum(aAttrEnum, aDoSetAttr);

  InvalidateTransformNotifyFrame();
}

nsSVGElement::EnumAttributesInfo
nsSVGSVGElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            NS_ARRAY_LENGTH(sEnumInfo));
}

void
nsSVGSVGElement::DidChangeViewBox(PRBool aDoSetAttr)
{
  nsSVGSVGElementBase::DidChangeViewBox(aDoSetAttr);

  InvalidateTransformNotifyFrame();
}

nsSVGViewBox *
nsSVGSVGElement::GetViewBox()
{
  return &mViewBox;
}

void
nsSVGSVGElement::DidChangePreserveAspectRatio(PRBool aDoSetAttr)
{
  nsSVGSVGElementBase::DidChangePreserveAspectRatio(aDoSetAttr);

  InvalidateTransformNotifyFrame();
}

nsSVGPreserveAspectRatio *
nsSVGSVGElement::GetPreserveAspectRatio()
{
  return &mPreserveAspectRatio;
}
