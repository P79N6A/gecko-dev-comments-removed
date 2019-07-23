





































#include "nsIDOMSVGTextElement.h"
#include "nsSVGTextFrame.h"
#include "nsWeakReference.h"
#include "nsIDOMSVGLengthList.h"
#include "nsIDOMSVGLength.h"
#include "nsIDOMSVGAnimatedNumber.h"
#include "nsISVGGlyphFragmentNode.h"
#include "nsISVGGlyphFragmentLeaf.h"
#include "nsSVGOuterSVGFrame.h"
#include "nsIDOMSVGRect.h"
#include "nsISVGTextContentMetrics.h"
#include "nsSVGRect.h"
#include "nsSVGMatrix.h"
#include "nsGkAtoms.h"
#include "nsSVGTextPathFrame.h"
#include "nsSVGPathElement.h"
#include "nsSVGUtils.h"
#include "nsSVGGraphicElement.h"




nsIFrame*
NS_NewSVGTextFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext)
{
  nsCOMPtr<nsIDOMSVGTextElement> text = do_QueryInterface(aContent);
  if (!text) {
    NS_ERROR("Can't create frame! Content is not an SVG text");
    return nsnull;
  }

  return new (aPresShell) nsSVGTextFrame(aContext);
}




NS_IMETHODIMP
nsSVGTextFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                 nsIAtom*        aAttribute,
                                 PRInt32         aModType)
{
  if (aNameSpaceID != kNameSpaceID_None)
    return NS_OK;

  if (aAttribute == nsGkAtoms::transform) {
    

    
    mCanvasTM = nsnull;

    nsSVGUtils::NotifyChildrenOfSVGChange(this, TRANSFORM_CHANGED);
   
  } else if (aAttribute == nsGkAtoms::x ||
             aAttribute == nsGkAtoms::y ||
             aAttribute == nsGkAtoms::dx ||
             aAttribute == nsGkAtoms::dy) {
    NotifyGlyphMetricsChange();
  }

 return NS_OK;
}

NS_IMETHODIMP
nsSVGTextFrame::DidSetStyleContext()
{
  nsSVGUtils::StyleEffects(this);

  return NS_OK;
}

nsIAtom *
nsSVGTextFrame::GetType() const
{
  return nsGkAtoms::svgTextFrame;
}



NS_IMETHODIMP
nsSVGTextFrame::GetNumberOfChars(PRInt32 *_retval)
{
  UpdateGlyphPositioning(PR_FALSE);

  return nsSVGTextFrameBase::GetNumberOfChars(_retval);
}

NS_IMETHODIMP
nsSVGTextFrame::GetComputedTextLength(float *_retval)
{
  UpdateGlyphPositioning(PR_FALSE);

  return nsSVGTextFrameBase::GetComputedTextLength(_retval);
}

NS_IMETHODIMP
nsSVGTextFrame::GetSubStringLength(PRUint32 charnum, PRUint32 nchars, float *_retval)
{
  UpdateGlyphPositioning(PR_FALSE);

  return nsSVGTextFrameBase::GetSubStringLength(charnum, nchars, _retval);
}

NS_IMETHODIMP
nsSVGTextFrame::GetStartPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)
{
  UpdateGlyphPositioning(PR_FALSE);

  return nsSVGTextFrameBase::GetStartPositionOfChar(charnum,  _retval);
}

NS_IMETHODIMP
nsSVGTextFrame::GetEndPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)
{
  UpdateGlyphPositioning(PR_FALSE);

  return nsSVGTextFrameBase::GetEndPositionOfChar(charnum,  _retval);
}

NS_IMETHODIMP
nsSVGTextFrame::GetExtentOfChar(PRUint32 charnum, nsIDOMSVGRect **_retval)
{
  UpdateGlyphPositioning(PR_FALSE);

  return nsSVGTextFrameBase::GetExtentOfChar(charnum,  _retval);
}

NS_IMETHODIMP
nsSVGTextFrame::GetRotationOfChar(PRUint32 charnum, float *_retval)
{
  UpdateGlyphPositioning(PR_FALSE);

  return nsSVGTextFrameBase::GetRotationOfChar(charnum,  _retval);
}

NS_IMETHODIMP
nsSVGTextFrame::GetCharNumAtPosition(nsIDOMSVGPoint *point, PRInt32 *_retval)
{
  UpdateGlyphPositioning(PR_FALSE);

  return nsSVGTextFrameBase::GetCharNumAtPosition(point,  _retval);
}





void
nsSVGTextFrame::NotifySVGChanged(PRUint32 aFlags)
{
  if (aFlags & TRANSFORM_CHANGED) {
    
    mCanvasTM = nsnull;
  }

  if (aFlags & COORD_CONTEXT_CHANGED) {
    
    

    
    
    
    NotifyGlyphMetricsChange();
  }

  nsSVGTextFrameBase::NotifySVGChanged(aFlags);
}

NS_IMETHODIMP
nsSVGTextFrame::NotifyRedrawSuspended()
{
  mMetricsState = suspended;

  return nsSVGTextFrameBase::NotifyRedrawSuspended();
}

NS_IMETHODIMP
nsSVGTextFrame::NotifyRedrawUnsuspended()
{
  mMetricsState = unsuspended;
  UpdateGlyphPositioning(PR_FALSE);
  return nsSVGTextFrameBase::NotifyRedrawUnsuspended();
}

NS_IMETHODIMP
nsSVGTextFrame::SetMatrixPropagation(PRBool aPropagate)
{
  mPropagateTransform = aPropagate;
  return NS_OK;
}

NS_IMETHODIMP
nsSVGTextFrame::SetOverrideCTM(nsIDOMSVGMatrix *aCTM)
{
  mOverrideCTM = aCTM;
  return NS_OK;
}

already_AddRefed<nsIDOMSVGMatrix>
nsSVGTextFrame::GetOverrideCTM()
{
  nsIDOMSVGMatrix *matrix = mOverrideCTM.get();
  NS_IF_ADDREF(matrix);
  return matrix;
}

NS_IMETHODIMP
nsSVGTextFrame::PaintSVG(nsSVGRenderState* aContext, nsRect *aDirtyRect)
{
  UpdateGlyphPositioning(PR_TRUE);
  
  return nsSVGTextFrameBase::PaintSVG(aContext, aDirtyRect);
}

NS_IMETHODIMP
nsSVGTextFrame::GetFrameForPointSVG(float x, float y, nsIFrame** hit)
{
  UpdateGlyphPositioning(PR_TRUE);
  
  return nsSVGTextFrameBase::GetFrameForPointSVG(x, y, hit);
}

NS_IMETHODIMP
nsSVGTextFrame::UpdateCoveredRegion()
{
  UpdateGlyphPositioning(PR_TRUE);
  
  return nsSVGTextFrameBase::UpdateCoveredRegion();
}

NS_IMETHODIMP
nsSVGTextFrame::GetBBox(nsIDOMSVGRect **_retval)
{
  UpdateGlyphPositioning(PR_TRUE);

  return nsSVGTextFrameBase::GetBBox(_retval);
}




already_AddRefed<nsIDOMSVGMatrix>
nsSVGTextFrame::GetCanvasTM()
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

    
    nsSVGGraphicElement *element =
      static_cast<nsSVGGraphicElement*>(mContent);
    nsCOMPtr<nsIDOMSVGMatrix> localTM = element->GetLocalTransformMatrix();
    
    if (localTM)
      parentTM->Multiply(localTM, getter_AddRefs(mCanvasTM));
    else
      mCanvasTM = parentTM;
  }

  nsIDOMSVGMatrix* retval = mCanvasTM.get();
  NS_IF_ADDREF(retval);
  return retval;
}




void
nsSVGTextFrame::NotifyGlyphMetricsChange()
{
  mPositioningDirty = PR_TRUE;
}

static void
GetSingleValue(nsISVGGlyphFragmentLeaf *fragment,
               nsIDOMSVGLengthList *list, float *val)
{
  if (!list)
    return;

  PRUint32 count = 0;
  list->GetNumberOfItems(&count);
#ifdef DEBUG
  if (count > 1)
    NS_WARNING("multiple lengths for x/y attributes on <text> elements not implemented yet!");
#endif
  if (count) {
    nsCOMPtr<nsIDOMSVGLength> length;
    list->GetItem(0, getter_AddRefs(length));
    length->GetValue(val);
  }
}

void
nsSVGTextFrame::UpdateGlyphPositioning(PRBool aForceGlobalTransform)
{
  if (mMetricsState == suspended || !mPositioningDirty)
    return;

  SetWhitespaceHandling();

  nsISVGGlyphFragmentNode* node = GetFirstGlyphFragmentChildNode();
  if (!node) return;

  
  
  
  PRUint8 baseline;
  switch(GetStyleSVGReset()->mDominantBaseline) {
    case NS_STYLE_DOMINANT_BASELINE_TEXT_BEFORE_EDGE:
      baseline = nsISVGGlyphFragmentLeaf::BASELINE_TEXT_BEFORE_EDGE;
      break;
    case NS_STYLE_DOMINANT_BASELINE_TEXT_AFTER_EDGE:
      baseline = nsISVGGlyphFragmentLeaf::BASELINE_TEXT_AFTER_EDGE;
      break;
    case NS_STYLE_DOMINANT_BASELINE_MIDDLE:
      baseline = nsISVGGlyphFragmentLeaf::BASELINE_MIDDLE;
      break;
    case NS_STYLE_DOMINANT_BASELINE_CENTRAL:
      baseline = nsISVGGlyphFragmentLeaf::BASELINE_CENTRAL;
      break;
    case NS_STYLE_DOMINANT_BASELINE_MATHEMATICAL:
      baseline = nsISVGGlyphFragmentLeaf::BASELINE_MATHEMATICAL;
      break;
    case NS_STYLE_DOMINANT_BASELINE_IDEOGRAPHIC:
      baseline = nsISVGGlyphFragmentLeaf::BASELINE_IDEOGRAPHC;
      break;
    case NS_STYLE_DOMINANT_BASELINE_HANGING:
      baseline = nsISVGGlyphFragmentLeaf::BASELINE_HANGING;
      break;
    case NS_STYLE_DOMINANT_BASELINE_AUTO:
    case NS_STYLE_DOMINANT_BASELINE_USE_SCRIPT:
    case NS_STYLE_DOMINANT_BASELINE_ALPHABETIC:
    default:
      baseline = nsISVGGlyphFragmentLeaf::BASELINE_ALPHABETIC;
      break;
  }

  nsISVGGlyphFragmentLeaf *fragment, *firstFragment;

  firstFragment = node->GetFirstGlyphFragment();
  if (!firstFragment) {
    mPositioningDirty = PR_FALSE;
    return;
  }

  float x = 0, y = 0;

  {
    nsCOMPtr<nsIDOMSVGLengthList> list = GetX();
    GetSingleValue(firstFragment, list, &x);
  }
  {
    nsCOMPtr<nsIDOMSVGLengthList> list = GetY();
    GetSingleValue(firstFragment, list, &y);
  }

  
  while (firstFragment) {
    {
      nsCOMPtr<nsIDOMSVGLengthList> list = firstFragment->GetX();
      GetSingleValue(firstFragment, list, &x);
    }
    {
      nsCOMPtr<nsIDOMSVGLengthList> list = firstFragment->GetY();
      GetSingleValue(firstFragment, list, &y);
    }

    
    nsSVGTextPathFrame *textPath = firstFragment->FindTextPathParent();
    if (textPath) {
      x = textPath->GetStartOffset();
    }

    
  
    PRUint8 anchor = firstFragment->GetTextAnchor();

    float chunkLength = 0.0f;
    if (anchor != NS_STYLE_TEXT_ANCHOR_START) {
      
    
      fragment = firstFragment;
      while (fragment) {
        float dx = 0.0f;
        nsCOMPtr<nsIDOMSVGLengthList> list = fragment->GetDx();
        GetSingleValue(fragment, list, &dx);
        chunkLength += dx + fragment->GetAdvance(aForceGlobalTransform);
        fragment = fragment->GetNextGlyphFragment();
        if (fragment && fragment->IsAbsolutelyPositioned())
          break;
      }
    }

    if (anchor == NS_STYLE_TEXT_ANCHOR_MIDDLE)
      x -= chunkLength/2.0f;
    else if (anchor == NS_STYLE_TEXT_ANCHOR_END)
      x -= chunkLength;
  
    
  
    fragment = firstFragment;
    while (fragment) {

      float dx = 0.0f, dy = 0.0f;
      {
        nsCOMPtr<nsIDOMSVGLengthList> list = fragment->GetDx();
        GetSingleValue(fragment, list, &dx);
      }
      {
        nsCOMPtr<nsIDOMSVGLengthList> list = fragment->GetDy();
        GetSingleValue(fragment, list, &dy);
      }

      float baseline_offset =
        fragment->GetBaselineOffset(baseline, aForceGlobalTransform);
      fragment->SetGlyphPosition(x + dx, y + dy - baseline_offset);

      x += dx + fragment->GetAdvance(aForceGlobalTransform);
      y += dy;
      fragment = fragment->GetNextGlyphFragment();
      if (fragment && fragment->IsAbsolutelyPositioned())
        break;
    }
    firstFragment = fragment;
  }

  mPositioningDirty = PR_FALSE;
}
