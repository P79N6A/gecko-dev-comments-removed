






































#include "nsGkAtoms.h"
#include "nsSVGLength.h"
#include "nsSVGAngle.h"
#include "nsCOMPtr.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsSVGAnimatedRect.h"
#include "nsSVGAnimatedPreserveAspectRatio.h"
#include "nsSVGMatrix.h"
#include "nsSVGPoint.h"
#include "nsSVGTransform.h"
#include "nsIDOMEventTarget.h"
#include "nsBindingManager.h"
#include "nsIFrame.h"
#include "nsISVGSVGFrame.h" 
#include "nsSVGNumber.h"
#include "nsSVGRect.h"
#include "nsSVGPreserveAspectRatio.h"
#include "nsISVGValueUtils.h"
#include "nsDOMError.h"
#include "nsSVGEnum.h"
#include "nsISVGChildFrame.h"
#include "nsGUIEvent.h"
#include "nsSVGUtils.h"
#include "nsSVGSVGElement.h"

nsSVGElement::LengthInfo nsSVGSVGElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::y, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
  { &nsGkAtoms::width, 100, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::height, 100, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
};

NS_IMPL_NS_NEW_SVG_ELEMENT(SVG)




NS_IMPL_ADDREF_INHERITED(nsSVGSVGElement,nsSVGSVGElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGSVGElement,nsSVGSVGElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGFitToViewBox)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGLocatable)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGZoomAndPan)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGSVGElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGSVGElementBase)




nsSVGSVGElement::nsSVGSVGElement(nsINodeInfo* aNodeInfo)
  : nsSVGSVGElementBase(aNodeInfo),
    mCoordCtx(nsnull),
    mCoordCtxMmPerPx(0),
    mRedrawSuspendCount(0)
{
}

nsSVGSVGElement::~nsSVGSVGElement()
{
  if (mPreserveAspectRatio) {
    NS_REMOVE_SVGVALUE_OBSERVER(mPreserveAspectRatio);
  }
  if (mViewBox) {
    NS_REMOVE_SVGVALUE_OBSERVER(mViewBox);
  }
}

  
nsresult
nsSVGSVGElement::Init()
{
  nsresult rv = nsSVGSVGElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);
  
  
  
  
  {
    nsCOMPtr<nsIDOMSVGRect> viewbox;
    rv = NS_NewSVGRect(getter_AddRefs(viewbox));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedRect(getter_AddRefs(mViewBox), viewbox);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::viewBox, mViewBox);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsIDOMSVGPreserveAspectRatio> preserveAspectRatio;
    rv = NS_NewSVGPreserveAspectRatio(getter_AddRefs(preserveAspectRatio));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedPreserveAspectRatio(
                                          getter_AddRefs(mPreserveAspectRatio),
                                          preserveAspectRatio);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::preserveAspectRatio,
                           mPreserveAspectRatio);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  
  

  
  static struct nsSVGEnumMapping zoomMap[] = {
        {&nsGkAtoms::disable, nsIDOMSVGZoomAndPan::SVG_ZOOMANDPAN_DISABLE},
        {&nsGkAtoms::magnify, nsIDOMSVGZoomAndPan::SVG_ZOOMANDPAN_MAGNIFY},
        {nsnull, 0}
  };

  
  {
    rv = NS_NewSVGEnum(getter_AddRefs(mZoomAndPan),
                       nsIDOMSVGZoomAndPan::SVG_ZOOMANDPAN_MAGNIFY, zoomMap);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::zoomAndPan, mZoomAndPan);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    rv = NS_NewSVGNumber(getter_AddRefs(mCurrentScale), 1.0f);
    NS_ENSURE_SUCCESS(rv,rv);
    NS_ADD_SVGVALUE_OBSERVER(mCurrentScale);
  }

  
  {
    rv = NS_NewSVGPoint(getter_AddRefs(mCurrentTranslate));
    NS_ENSURE_SUCCESS(rv,rv);
    NS_ADD_SVGVALUE_OBSERVER(mCurrentTranslate);
  }

  
  RecordCurrentScaleTranslate();
  mDispatchEvent = PR_TRUE;

  return rv;
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGSVGElement)






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
  
  

  *aPixelUnitToMillimeterX = 0.28f; 

  nsIDocument* doc = GetCurrentDoc();
  if (!doc) return NS_OK;
  
  nsIPresShell *presShell = doc->GetPrimaryShell();
  if (!presShell) return NS_OK;
  
  
  nsPresContext *context = presShell->GetPresContext();
  if (!context) return NS_OK;

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
  
  

  *aScreenPixelToMillimeterX = 0.28f; 

  nsIDocument* doc = GetCurrentDoc();
  if (!doc) return NS_OK;
    
  nsIPresShell *presShell = doc->GetPrimaryShell();
  if (!presShell) return NS_OK;
  
  
  nsPresContext *context = presShell->GetPresContext();
  if (!context) return NS_OK;

  *aScreenPixelToMillimeterX = 25.4f / context->AppUnitsToDevPixels(context->AppUnitsPerInch());
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
  return mCurrentScale->GetValue(aCurrentScale);
}

#define CURRENT_SCALE_MAX 16.0f
#define CURRENT_SCALE_MIN 0.0625f

NS_IMETHODIMP
nsSVGSVGElement::SetCurrentScale(float aCurrentScale)
{
  
  if (aCurrentScale < CURRENT_SCALE_MIN)
    aCurrentScale = CURRENT_SCALE_MIN;
  else if (aCurrentScale > CURRENT_SCALE_MAX)
    aCurrentScale = CURRENT_SCALE_MAX;

  return mCurrentScale->SetValue(aCurrentScale);

  
  
}


NS_IMETHODIMP
nsSVGSVGElement::GetCurrentTranslate(nsIDOMSVGPoint * *aCurrentTranslate)
{
  *aCurrentTranslate = mCurrentTranslate;
  NS_ADDREF(*aCurrentTranslate);
  return NS_OK;
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
    nsISVGSVGFrame* svgframe;
    CallQueryInterface(frame, &svgframe);
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
    nsISVGSVGFrame* svgframe;
    CallQueryInterface(frame, &svgframe);
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
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::PauseAnimations");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::UnpauseAnimations()
{
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::UnpauseAnimations");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::AnimationsPaused(PRBool *_retval)
{
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::AnimationsPaused");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::GetCurrentTime(float *_retval)
{
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::GetCurrentTime");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::SetCurrentTime(float seconds)
{
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
  return NS_NewSVGLength(NS_REINTERPRET_CAST(nsISVGLength**, _retval));
}


NS_IMETHODIMP
nsSVGSVGElement::CreateSVGAngle(nsIDOMSVGAngle **_retval)
{
  return NS_NewSVGAngle(_retval);
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
  *aViewBox = mViewBox;
  NS_ADDREF(*aViewBox);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGSVGElement::GetPreserveAspectRatio(nsIDOMSVGAnimatedPreserveAspectRatio * *aPreserveAspectRatio)
{
  *aPreserveAspectRatio = mPreserveAspectRatio;
  NS_ADDREF(*aPreserveAspectRatio);
  return NS_OK;
}





NS_IMETHODIMP
nsSVGSVGElement::GetNearestViewportElement(nsIDOMSVGElement * *aNearestViewportElement)
{
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::GetNearestViewportElement");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::GetFarthestViewportElement(nsIDOMSVGElement * *aFarthestViewportElement)
{
  NS_NOTYETIMPLEMENTED("nsSVGSVGElement::GetFarthestViewportElement");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGSVGElement::GetBBox(nsIDOMSVGRect **_retval)
{
  *_retval = nsnull;

  nsIFrame* frame = GetPrimaryFrame(Flush_Layout);

  if (frame) {
    nsISVGChildFrame* svgframe;
    frame->QueryInterface(NS_GET_IID(nsISVGChildFrame),(void**)&svgframe);
    if (svgframe) {
      svgframe->SetMatrixPropagation(PR_FALSE);
      svgframe->NotifyCanvasTMChanged(PR_TRUE);
      nsresult rv = svgframe->GetBBox(_retval);
      svgframe->SetMatrixPropagation(PR_TRUE);
      svgframe->NotifyCanvasTMChanged(PR_TRUE);
      return rv;
    } else {
      
      return NS_ERROR_NOT_IMPLEMENTED;
    }
  }
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsSVGSVGElement::GetCTM(nsIDOMSVGMatrix **_retval)
{
  nsresult rv;
  *_retval = nsnull;

  

  nsBindingManager *bindingManager = nsnull;
  
  
  
  
  nsIDocument* ownerDoc = GetOwnerDoc();
  if (ownerDoc) {
    bindingManager = ownerDoc->BindingManager();
  }

  nsCOMPtr<nsIContent> element = this;
  nsCOMPtr<nsIContent> ancestor;
  unsigned short ancestorCount = 0;
  nsCOMPtr<nsIDOMSVGMatrix> ancestorCTM;

  while (1) {
    ancestor = nsnull;
    if (bindingManager) {
      
      ancestor = bindingManager->GetInsertionParent(element);
    }
    if (!ancestor) {
      
      ancestor = element->GetParent();
    }
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
    if (ownerDoc &&
        ownerDoc->GetRootContent() == NS_STATIC_CAST(nsIContent*, this)) {
      
      mCurrentScale->GetValue(&s);
      mCurrentTranslate->GetX(&x);
      mCurrentTranslate->GetY(&y);
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
      
      
      x = mLengthAttributes[X].GetAnimValue(NS_STATIC_CAST(nsSVGElement*,
                                                           this));
      y = mLengthAttributes[Y].GetAnimValue(NS_STATIC_CAST(nsSVGElement*,
                                                           this));
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

  

  nsBindingManager *bindingManager = nsnull;
  
  
  
  
  nsIDocument* ownerDoc = GetOwnerDoc();
  if (ownerDoc) {
    bindingManager = ownerDoc->BindingManager();
  }

  nsCOMPtr<nsIContent> element = this;
  nsCOMPtr<nsIContent> ancestor;
  unsigned short ancestorCount = 0;
  nsCOMPtr<nsIDOMSVGMatrix> ancestorScreenCTM;

  while (1) {
    ancestor = nsnull;
    if (bindingManager) {
      
      ancestor = bindingManager->GetInsertionParent(element);
    }
    if (!ancestor) {
      
      ancestor = element->GetParent();
    }
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
    if (ownerDoc &&
        ownerDoc->GetRootContent() == NS_STATIC_CAST(nsIContent*, this)) {
      
      mCurrentScale->GetValue(&s);
      mCurrentTranslate->GetX(&x);
      mCurrentTranslate->GetY(&y);
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
      
      
      x = mLengthAttributes[X].GetAnimValue(NS_STATIC_CAST(nsSVGElement*,
                                                           this));
      y = mLengthAttributes[Y].GetAnimValue(NS_STATIC_CAST(nsSVGElement*,
                                                           this));
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
  return mZoomAndPan->GetIntegerValue(*aZoomAndPan);
}

NS_IMETHODIMP
nsSVGSVGElement::SetZoomAndPan(PRUint16 aZoomAndPan)
{
  if (aZoomAndPan == nsIDOMSVGZoomAndPan::SVG_ZOOMANDPAN_DISABLE ||
      aZoomAndPan == nsIDOMSVGZoomAndPan::SVG_ZOOMANDPAN_MAGNIFY)
    return mZoomAndPan->SetIntegerValue(aZoomAndPan);

  return NS_ERROR_DOM_SVG_INVALID_VALUE_ERR;
}




NS_IMETHODIMP
nsSVGSVGElement::GetCurrentScaleNumber(nsIDOMSVGNumber **aResult)
{
  *aResult = mCurrentScale;
  NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGSVGElement::GetZoomAndPanEnum(nsISVGEnum **aResult)
{
  *aResult = mZoomAndPan;
  NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGSVGElement::SetCurrentScaleTranslate(float s, float x, float y)
{
  RecordCurrentScaleTranslate();
  mDispatchEvent = PR_FALSE;
  SetCurrentScale(s);  
  mCurrentTranslate->SetX(x);
  mCurrentTranslate->SetY(y);
  mDispatchEvent = PR_TRUE;

  
  nsIDocument* doc = GetCurrentDoc();
  if (doc) {
    nsCOMPtr<nsIPresShell> presShell = doc->GetPrimaryShell();
    NS_ASSERTION(presShell, "no presShell");
    if (presShell &&
        doc->GetRootContent() == NS_STATIC_CAST(nsIContent*, this)) {
      nsEventStatus status = nsEventStatus_eIgnore;
      nsGUIEvent event(PR_TRUE, NS_SVG_ZOOM, 0);
      event.eventStructType = NS_SVGZOOM_EVENT;
      presShell->HandleDOMEventWithTarget(this, &event, &status);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSVGSVGElement::SetCurrentTranslate(float x, float y)
{
  RecordCurrentScaleTranslate();
  mDispatchEvent = PR_FALSE;
  mCurrentTranslate->SetX(x);
  mCurrentTranslate->SetY(y);
  mDispatchEvent = PR_TRUE;

  
  nsIDocument* doc = GetCurrentDoc();
  if (doc) {
    nsCOMPtr<nsIPresShell> presShell = doc->GetPrimaryShell();
    NS_ASSERTION(presShell, "no presShell");
    if (presShell &&
        doc->GetRootContent() == NS_STATIC_CAST(nsIContent*, this)) {
      nsEventStatus status = nsEventStatus_eIgnore;
      nsEvent event(PR_TRUE, NS_SVG_SCROLL);
      event.eventStructType = NS_SVG_EVENT;
      presShell->HandleDOMEventWithTarget(this, &event, &status);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP_(void)
nsSVGSVGElement::RecordCurrentScaleTranslate()
{
  
  
  
  
  
  
  
  
  mCurrentScale->GetValue(&mPreviousScale);
  mCurrentTranslate->GetX(&mPreviousTranslate_x);
  mCurrentTranslate->GetY(&mPreviousTranslate_y);
}

NS_IMETHODIMP_(float)
nsSVGSVGElement::GetPreviousTranslate_x()
{
  return mPreviousTranslate_x;
}

NS_IMETHODIMP_(float)
nsSVGSVGElement::GetPreviousTranslate_y()
{
  return mPreviousTranslate_y;
}

NS_IMETHODIMP_(float)
nsSVGSVGElement::GetPreviousScale()
{
  return mPreviousScale;
}




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
    sMarkersMap,
    sTextContentElementsMap,
    sViewportsMap
  };

  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGSVGElementBase::IsAttributeMapped(name);
}

nsresult
nsSVGSVGElement::AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                              const nsAString* aValue, PRBool aNotify)
{
  nsSVGSVGElementBase::AfterSetAttr(aNameSpaceID, aName, aValue, aNotify);

  
  
  
  
  if (aNameSpaceID == kNameSpaceID_None && aName == nsGkAtoms::viewBox) {
    InvalidateTransformNotifyFrame();
  }

  return NS_OK;
}

nsresult
nsSVGSVGElement::UnsetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                           PRBool aNotify)
{
  nsSVGSVGElementBase::UnsetAttr(aNamespaceID, aName, aNotify);

  if (aNamespaceID == kNameSpaceID_None && aName == nsGkAtoms::viewBox) {
    InvalidateTransformNotifyFrame();
  }

  return NS_OK;
}




NS_IMETHODIMP
nsSVGSVGElement::WillModifySVGObservable(nsISVGValue* observable,
                                         nsISVGValue::modificationType aModType)
{
  if (mDispatchEvent) {
    
    
    
    nsCOMPtr<nsIDOMSVGNumber> n = do_QueryInterface(observable);
    if (n && n==mCurrentScale) {
      RecordCurrentScaleTranslate();
    }
    else {
      nsCOMPtr<nsIDOMSVGPoint> p = do_QueryInterface(observable);
      if (p && p==mCurrentTranslate) {
        RecordCurrentScaleTranslate();
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSVGSVGElement::DidModifySVGObservable (nsISVGValue* observable,
                                         nsISVGValue::modificationType aModType)
{
  nsIDocument* doc = GetCurrentDoc();
  if (!doc) return NS_ERROR_FAILURE;
  nsCOMPtr<nsIPresShell> presShell = doc->GetPrimaryShell();
  NS_ASSERTION(presShell, "no presShell");
  if (!presShell) return NS_ERROR_FAILURE;

  
  
  
  nsCOMPtr<nsIDOMSVGNumber> n = do_QueryInterface(observable);
  if (n && n==mCurrentScale) {
    if (mDispatchEvent &&
        doc->GetRootContent() == NS_STATIC_CAST(nsIContent*, this)) {
      nsEventStatus status = nsEventStatus_eIgnore;
      nsGUIEvent event(PR_TRUE, NS_SVG_ZOOM, 0);
      event.eventStructType = NS_SVGZOOM_EVENT;
      presShell->HandleDOMEventWithTarget(this, &event, &status);
    }
    else {
      return NS_OK;  
    }
  }
  else {
    nsCOMPtr<nsIDOMSVGPoint> p = do_QueryInterface(observable);
    if (p && p==mCurrentTranslate) {
      if (mDispatchEvent &&
          doc->GetRootContent() == NS_STATIC_CAST(nsIContent*, this)) {
        nsEventStatus status = nsEventStatus_eIgnore;
        nsEvent event(PR_TRUE, NS_SVG_SCROLL);
        event.eventStructType = NS_SVG_EVENT;
        presShell->HandleDOMEventWithTarget(this, &event, &status);
      }
      else {
        return NS_OK;  
      }
    }
  }

  
  nsCOMPtr<nsIDOMSVGAnimatedRect> r = do_QueryInterface(observable);
  if (r != mViewBox) {
    InvalidateTransformNotifyFrame();
  }

  return NS_OK;
}




PRBool
nsSVGSVGElement::IsEventName(nsIAtom* aName)
{
  





  return nsContentUtils::IsEventAttributeName(aName,
         (EventNameType_SVGGraphic | EventNameType_SVGSVG));
}




nsresult
nsSVGSVGElement::GetViewboxToViewportTransform(nsIDOMSVGMatrix **_retval)
{
  nsresult rv = NS_OK;

  float viewportWidth, viewportHeight;
  nsSVGSVGElement *ctx = GetCtx();
  if (!ctx) {
    
    viewportWidth = mViewportWidth;
    viewportHeight = mViewportHeight;
  } else {
    viewportWidth = mLengthAttributes[WIDTH].GetAnimValue(ctx);
    viewportHeight = mLengthAttributes[HEIGHT].GetAnimValue(ctx);
  }

  float viewboxX, viewboxY, viewboxWidth, viewboxHeight;
  if (HasAttr(kNameSpaceID_None, nsGkAtoms::viewBox)) {
    nsCOMPtr<nsIDOMSVGRect> vb;
    mViewBox->GetAnimVal(getter_AddRefs(vb));
    NS_ASSERTION(vb, "could not get viewbox");
    vb->GetX(&viewboxX);
    vb->GetY(&viewboxY);
    vb->GetWidth(&viewboxWidth);
    vb->GetHeight(&viewboxHeight);
  } else {
    viewboxX = viewboxY = 0.0f;
    viewboxWidth = viewportWidth;
    viewboxHeight = viewportHeight;
  }

  if (viewboxWidth==0.0f || viewboxHeight==0.0f) {
    NS_ERROR("XXX. We shouldn't get here. Viewbox width/height is set to 0. Need to disable display of element as per specs.");
    viewboxWidth = 1.0f;
    viewboxHeight = 1.0f;
  }

  nsCOMPtr<nsIDOMSVGMatrix> xform =
    nsSVGUtils::GetViewBoxTransform(viewportWidth, viewportHeight,
                                    viewboxX, viewboxY,
                                    viewboxWidth, viewboxHeight,
                                    mPreserveAspectRatio);
  xform.swap(*_retval);

  return rv;
}





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

void
nsSVGSVGElement::InvalidateTransformNotifyFrame()
{
  nsIDocument* doc = GetCurrentDoc();
  if (!doc) return;
  nsIPresShell* presShell = doc->GetPrimaryShell();
  if (!presShell) return;

  nsIFrame* frame = presShell->GetPrimaryFrameFor(this);
  if (frame) {
    nsISVGSVGFrame* svgframe;
    CallQueryInterface(frame, &svgframe);
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




void
nsSVGSVGElement::SetCoordCtxRect(nsIDOMSVGRect* aCtxRect)
{
  if (mLengthAttributes[WIDTH].GetSpecifiedUnitType() ==
      nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
    aCtxRect->GetWidth(&mViewportWidth);
    mViewportWidth *=
      mLengthAttributes[WIDTH].GetAnimValInSpecifiedUnits() / 100.0f;
  } else {
    mViewportWidth = mLengthAttributes[WIDTH].GetAnimValue(this);
  }

  if (mLengthAttributes[HEIGHT].GetSpecifiedUnitType() ==
      nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
    aCtxRect->GetHeight(&mViewportHeight);
    mViewportHeight *=
      mLengthAttributes[HEIGHT].GetAnimValInSpecifiedUnits() / 100.0f;
  } else {
    mViewportHeight = mLengthAttributes[HEIGHT].GetAnimValue(this);
  }
}

already_AddRefed<nsIDOMSVGRect>
nsSVGSVGElement::GetCtxRect() {
  nsCOMPtr<nsIDOMSVGRect> vb;
  if (HasAttr(kNameSpaceID_None, nsGkAtoms::viewBox)) {
    mViewBox->GetAnimVal(getter_AddRefs(vb));
  } else {
    nsSVGSVGElement *ctx = GetCtx();
    float w, h;
    if (ctx) {
      w = mLengthAttributes[WIDTH].GetAnimValue(ctx);
      h = mLengthAttributes[HEIGHT].GetAnimValue(ctx);
    } else {
      w = mViewportWidth;
      h = mViewportHeight;
    }
    NS_NewSVGRect(getter_AddRefs(vb), 0, 0, w, h);
  }

  nsIDOMSVGRect *retval = nsnull;
  vb.swap(retval);
  return retval;
}

float
nsSVGSVGElement::GetLength(PRUint8 aCtxType) {
  float h, w;

  if (HasAttr(kNameSpaceID_None, nsGkAtoms::viewBox)) {
    nsCOMPtr<nsIDOMSVGRect> vb;
    mViewBox->GetAnimVal(getter_AddRefs(vb));
    vb->GetHeight(&h);
    vb->GetWidth(&w);
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

  switch (aCtxType) {
  case nsSVGUtils::X:
    return w;
  case nsSVGUtils::Y:
    return h;
  case nsSVGUtils::XY:
    return (float)sqrt((w*w+h*h)/2.0);
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
