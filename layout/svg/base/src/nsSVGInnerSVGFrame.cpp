





































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
  nsSVGInnerSVGFrame(nsStyleContext* aContext);
  
   
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }  

public:
  
  NS_IMETHOD DidSetStyleContext();

  
  
  

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGInnerSVG"), aResult);
  }
#endif

  
  NS_IMETHOD PaintSVG(nsSVGRenderState *aContext, nsRect *aDirtyRect);
  NS_IMETHOD NotifyCanvasTMChanged(PRBool suppressInvalidation);
  NS_IMETHOD SetMatrixPropagation(PRBool aPropagate);
  NS_IMETHOD SetOverrideCTM(nsIDOMSVGMatrix *aCTM);
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
  return new (aPresShell) nsSVGInnerSVGFrame(aContext);
}

nsSVGInnerSVGFrame::nsSVGInnerSVGFrame(nsStyleContext* aContext) :
  nsSVGInnerSVGFrameBase(aContext), mPropagateTransform(PR_TRUE)
{
#ifdef DEBUG

#endif
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
  nsresult rv = NS_OK;

  gfxContext *gfx = aContext->GetGfxContext();

  gfx->Save();

  if (GetStyleDisplay()->IsScrollableOverflow()) {
    nsSVGSVGElement *svg = NS_STATIC_CAST(nsSVGSVGElement*, mContent);

    float x, y, width, height;
    svg->GetAnimatedLengthValues(&x, &y, &width, &height, nsnull);

    nsCOMPtr<nsIDOMSVGMatrix> clipTransform;
    if (!mPropagateTransform) {
      NS_NewSVGMatrix(getter_AddRefs(clipTransform));
    } else {
      nsSVGContainerFrame *parent = NS_STATIC_CAST(nsSVGContainerFrame*,
                                                   mParent);
      clipTransform = parent->GetCanvasTM();
    }

    if (clipTransform)
      nsSVGUtils::SetClipRect(gfx, clipTransform, x, y, width, height);
  }

  rv = nsSVGInnerSVGFrameBase::PaintSVG(aContext, aDirtyRect);

  gfx->Restore();

  return rv;
}

NS_IMETHODIMP
nsSVGInnerSVGFrame::NotifyCanvasTMChanged(PRBool suppressInvalidation)
{
  
  mCanvasTM = nsnull;

  return nsSVGInnerSVGFrameBase::NotifyCanvasTMChanged(suppressInvalidation);
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

NS_IMETHODIMP
nsSVGInnerSVGFrame::GetFrameForPointSVG(float x, float y, nsIFrame** hit)
{
  if (GetStyleDisplay()->IsScrollableOverflow()) {
    float clipX, clipY, clipWidth, clipHeight;
    nsCOMPtr<nsIDOMSVGMatrix> clipTransform;

    nsSVGElement *svg = NS_STATIC_CAST(nsSVGElement*, mContent);
    svg->GetAnimatedLengthValues(&clipX, &clipY, &clipWidth, &clipHeight, nsnull);

    nsSVGContainerFrame *parent = NS_STATIC_CAST(nsSVGContainerFrame*,
                                                 mParent);
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
  
  mCanvasTM = nsnull;
  
  
  SuspendRedraw();
  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    nsISVGChildFrame* SVGFrame = nsnull;
    CallQueryInterface(kid, &SVGFrame);
    if (SVGFrame)
      SVGFrame->NotifyCanvasTMChanged(PR_FALSE); 
    kid = kid->GetNextSibling();
  }
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
    nsSVGContainerFrame *containerFrame = NS_STATIC_CAST(nsSVGContainerFrame*,
                                                         mParent);
    nsCOMPtr<nsIDOMSVGMatrix> parentTM = containerFrame->GetCanvasTM();
    NS_ASSERTION(parentTM, "null TM");

    
    float x, y;
    nsSVGSVGElement *svg = NS_STATIC_CAST(nsSVGSVGElement*, mContent);
    svg->GetAnimatedLengthValues(&x, &y, nsnull);

    nsCOMPtr<nsIDOMSVGMatrix> xyTM;
    parentTM->Translate(x, y, getter_AddRefs(xyTM));

    
    nsCOMPtr<nsIDOMSVGMatrix> viewBoxToViewportTM;
    nsSVGSVGElement *svgElement = NS_STATIC_CAST(nsSVGSVGElement*, mContent);
    svgElement->GetViewboxToViewportTransform(getter_AddRefs(viewBoxToViewportTM));
    xyTM->Multiply(viewBoxToViewportTM, getter_AddRefs(mCanvasTM));
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

NS_IMETHODIMP
nsSVGInnerSVGFrame::DidSetStyleContext()
{
  nsSVGUtils::StyleEffects(this);
  return NS_OK;
}
