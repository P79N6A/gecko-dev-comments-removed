





































#include "nsIFrame.h"
#include "nsISVGChildFrame.h"
#include "nsSVGOuterSVGFrame.h"
#include "nsIDOMSVGAnimatedRect.h"
#include "nsSVGMatrix.h"
#include "nsSVGSVGElement.h"
#include "nsSVGContainerFrame.h"
#include "gfxContext.h"

typedef nsSVGDisplayContainerFrame nsSVGInnerSVGFrameBase;

class nsSVGInnerSVGFrame : public nsSVGInnerSVGFrameBase,
                           public nsISVGValueObserver,
                           public nsISVGSVGFrame
{
  friend nsIFrame*
  NS_NewSVGInnerSVGFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
protected:
  nsSVGInnerSVGFrame(nsStyleContext* aContext) :
    nsSVGInnerSVGFrameBase(aContext), mPropagateTransform(PR_TRUE) {}
  
   
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return 1; }
  NS_IMETHOD_(nsrefcnt) Release() { return 1; }

public:
  
  
  

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGInnerSVG"), aResult);
  }
#endif

  
  NS_IMETHOD PaintSVG(nsSVGRenderState *aContext, nsRect *aDirtyRect);
  virtual void NotifySVGChanged(PRUint32 aFlags);
  NS_IMETHOD SetMatrixPropagation(PRBool aPropagate);
  NS_IMETHOD SetOverrideCTM(nsIDOMSVGMatrix *aCTM);
  virtual already_AddRefed<nsIDOMSVGMatrix> GetOverrideCTM();
  NS_IMETHOD GetFrameForPointSVG(float x, float y, nsIFrame** hit);

  
  virtual already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     nsISVGValue::modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     nsISVGValue::modificationType aModType);

  
  

  
  NS_IMETHOD SuspendRedraw();
  NS_IMETHOD UnsuspendRedraw();
  NS_IMETHOD NotifyViewportChange();

protected:

  nsCOMPtr<nsIDOMSVGMatrix> mCanvasTM;
  nsCOMPtr<nsIDOMSVGMatrix> mOverrideCTM;

  PRPackedBool mPropagateTransform;
};




nsIFrame*
NS_NewSVGInnerSVGFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext)
{
  nsCOMPtr<nsIDOMSVGSVGElement> svg = do_QueryInterface(aContent);
  if (!svg) {
    NS_ERROR("Can't create frame! Content is not an SVG 'svg' element!");
    return nsnull;
  }

  return new (aPresShell) nsSVGInnerSVGFrame(aContext);
}




NS_INTERFACE_MAP_BEGIN(nsSVGInnerSVGFrame)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGSVGFrame)
NS_INTERFACE_MAP_END_INHERITING(nsSVGInnerSVGFrameBase)





nsIAtom *
nsSVGInnerSVGFrame::GetType() const
{
  return nsGkAtoms::svgInnerSVGFrame;
}




NS_IMETHODIMP
nsSVGInnerSVGFrame::PaintSVG(nsSVGRenderState *aContext, nsRect *aDirtyRect)
{
  gfxContextAutoSaveRestore autoSR;

  if (GetStyleDisplay()->IsScrollableOverflow()) {
    float x, y, width, height;
    static_cast<nsSVGSVGElement*>(mContent)->
      GetAnimatedLengthValues(&x, &y, &width, &height, nsnull);

    if (width <= 0 || height <= 0) {
      return NS_OK;
    }

    nsCOMPtr<nsIDOMSVGMatrix> clipTransform;
    if (!mPropagateTransform) {
      NS_NewSVGMatrix(getter_AddRefs(clipTransform));
    } else {
      clipTransform = static_cast<nsSVGContainerFrame*>(mParent)->GetCanvasTM();
    }

    if (clipTransform) {
      gfxContext *gfx = aContext->GetGfxContext();
      autoSR.SetContext(gfx);
      nsSVGUtils::SetClipRect(gfx, clipTransform, x, y, width, height);
    }
  }

  return nsSVGInnerSVGFrameBase::PaintSVG(aContext, aDirtyRect);
}

void
nsSVGInnerSVGFrame::NotifySVGChanged(PRUint32 aFlags)
{
  if (aFlags & COORD_CONTEXT_CHANGED) {

    nsSVGSVGElement *svg = static_cast<nsSVGSVGElement*>(mContent);

    
    
    

    if (!(aFlags & TRANSFORM_CHANGED) &&
        svg->mLengthAttributes[nsSVGSVGElement::X].IsPercentage() ||
        svg->mLengthAttributes[nsSVGSVGElement::Y].IsPercentage() ||
        (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::viewBox) &&
         (svg->mLengthAttributes[nsSVGSVGElement::WIDTH].IsPercentage() ||
          svg->mLengthAttributes[nsSVGSVGElement::HEIGHT].IsPercentage()))) {
    
      aFlags |= TRANSFORM_CHANGED;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
  }

  if (aFlags & TRANSFORM_CHANGED) {
    
    mCanvasTM = nsnull;
  }

  nsSVGInnerSVGFrameBase::NotifySVGChanged(aFlags);
}

NS_IMETHODIMP
nsSVGInnerSVGFrame::SetMatrixPropagation(PRBool aPropagate)
{
  mPropagateTransform = aPropagate;
  return NS_OK;
}

NS_IMETHODIMP
nsSVGInnerSVGFrame::SetOverrideCTM(nsIDOMSVGMatrix *aCTM)
{
  mOverrideCTM = aCTM;
  return NS_OK;
}

already_AddRefed<nsIDOMSVGMatrix>
nsSVGInnerSVGFrame::GetOverrideCTM()
{
  nsIDOMSVGMatrix *matrix = mOverrideCTM.get();
  NS_IF_ADDREF(matrix);
  return matrix;
}

NS_IMETHODIMP
nsSVGInnerSVGFrame::GetFrameForPointSVG(float x, float y, nsIFrame** hit)
{
  if (GetStyleDisplay()->IsScrollableOverflow()) {
    float clipX, clipY, clipWidth, clipHeight;
    nsCOMPtr<nsIDOMSVGMatrix> clipTransform;

    nsSVGElement *svg = static_cast<nsSVGElement*>(mContent);
    svg->GetAnimatedLengthValues(&clipX, &clipY, &clipWidth, &clipHeight, nsnull);

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>
                                             (mParent);
    clipTransform = parent->GetCanvasTM();

    if (!nsSVGUtils::HitTestRect(clipTransform,
                                 clipX, clipY, clipWidth, clipHeight,
                                 x, y)) {
      *hit = nsnull;
      return NS_OK;
    }
  }

  return nsSVGInnerSVGFrameBase::GetFrameForPointSVG(x, y, hit);
}




NS_IMETHODIMP
nsSVGInnerSVGFrame::SuspendRedraw()
{
  nsSVGOuterSVGFrame *outerSVGFrame = nsSVGUtils::GetOuterSVGFrame(this);
  if (!outerSVGFrame) {
    NS_ERROR("no outer svg frame");
    return NS_ERROR_FAILURE;
  }
  return outerSVGFrame->SuspendRedraw();
}

NS_IMETHODIMP
nsSVGInnerSVGFrame::UnsuspendRedraw()
{
  nsSVGOuterSVGFrame *outerSVGFrame = nsSVGUtils::GetOuterSVGFrame(this);
  if (!outerSVGFrame) {
    NS_ERROR("no outer svg frame");
    return NS_ERROR_FAILURE;
  }
  return outerSVGFrame->UnsuspendRedraw();
}

NS_IMETHODIMP
nsSVGInnerSVGFrame::NotifyViewportChange()
{
  PRUint32 flags = COORD_CONTEXT_CHANGED;

#if 1
  
  
  

  flags |= TRANSFORM_CHANGED;

  
  mCanvasTM = nsnull;
#else
  
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::viewBox)) {
    
    mCanvasTM = nsnull;

    flags |= TRANSFORM_CHANGED;
  }
#endif
  
  
  SuspendRedraw();
  nsSVGUtils::NotifyChildrenOfSVGChange(this, flags);
  UnsuspendRedraw();
  return NS_OK;
}




already_AddRefed<nsIDOMSVGMatrix>
nsSVGInnerSVGFrame::GetCanvasTM()
{
  if (!mPropagateTransform) {
    nsIDOMSVGMatrix *retval;
    if (mOverrideCTM) {
      retval = mOverrideCTM;
      NS_ADDREF(retval);
    } else {
      NS_NewSVGMatrix(&retval);
    }
    return retval;
  }

  

  if (!mCanvasTM) {
    
    NS_ASSERTION(mParent, "null parent");
    nsSVGContainerFrame *containerFrame = static_cast<nsSVGContainerFrame*>
                                                     (mParent);
    nsCOMPtr<nsIDOMSVGMatrix> parentTM = containerFrame->GetCanvasTM();
    NS_ASSERTION(parentTM, "null TM");

    
    float x, y;
    nsSVGSVGElement *svg = static_cast<nsSVGSVGElement*>(mContent);
    svg->GetAnimatedLengthValues(&x, &y, nsnull);

    nsCOMPtr<nsIDOMSVGMatrix> xyTM;
    parentTM->Translate(x, y, getter_AddRefs(xyTM));

    
    nsCOMPtr<nsIDOMSVGMatrix> viewBoxTM;
    nsSVGSVGElement *svgElement = static_cast<nsSVGSVGElement*>(mContent);
    nsresult res =
      svgElement->GetViewboxToViewportTransform(getter_AddRefs(viewBoxTM));
    if (NS_SUCCEEDED(res) && viewBoxTM) {
      xyTM->Multiply(viewBoxTM, getter_AddRefs(mCanvasTM));
    } else {
      NS_WARNING("We should propagate the fact that the viewBox is invalid.");
      mCanvasTM = xyTM;
    }
  }    

  nsIDOMSVGMatrix* retval = mCanvasTM.get();
  NS_IF_ADDREF(retval);
  return retval;
}




NS_IMETHODIMP
nsSVGInnerSVGFrame::WillModifySVGObservable(nsISVGValue* observable,
                                            nsISVGValue::modificationType aModType)
{
  return NS_OK;
}
	
NS_IMETHODIMP
nsSVGInnerSVGFrame::DidModifySVGObservable (nsISVGValue* observable,
                                            nsISVGValue::modificationType aModType)
{
  NotifyViewportChange();
  return NS_OK;
}
