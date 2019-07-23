





































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
#include "nsSVGRect.h"
#include "nsSVGMatrix.h"
#include "nsGkAtoms.h"
#include "nsSVGTextPathFrame.h"
#include "nsSVGPathElement.h"
#include "nsSVGUtils.h"
#include "nsSVGGraphicElement.h"




nsIFrame*
NS_NewSVGTextFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGTextFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGTextFrame)



#ifdef DEBUG
NS_IMETHODIMP
nsSVGTextFrame::Init(nsIContent* aContent,
                     nsIFrame* aParent,
                     nsIFrame* aPrevInFlow)
{
  nsCOMPtr<nsIDOMSVGTextElement> text = do_QueryInterface(aContent);
  NS_ASSERTION(text, "Content is not an SVG text");

  return nsSVGTextFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

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

nsIAtom *
nsSVGTextFrame::GetType() const
{
  return nsGkAtoms::svgTextFrame;
}



PRUint32
nsSVGTextFrame::GetNumberOfChars()
{
  UpdateGlyphPositioning(PR_FALSE);

  return nsSVGTextFrameBase::GetNumberOfChars();
}

float
nsSVGTextFrame::GetComputedTextLength()
{
  UpdateGlyphPositioning(PR_FALSE);

  return nsSVGTextFrameBase::GetComputedTextLength();
}

float
nsSVGTextFrame::GetSubStringLength(PRUint32 charnum, PRUint32 nchars)
{
  UpdateGlyphPositioning(PR_FALSE);

  return nsSVGTextFrameBase::GetSubStringLength(charnum, nchars);
}

PRInt32
nsSVGTextFrame::GetCharNumAtPosition(nsIDOMSVGPoint *point)
{
  UpdateGlyphPositioning(PR_FALSE);

  return nsSVGTextFrameBase::GetCharNumAtPosition(point);
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




void
nsSVGTextFrame::NotifySVGChanged(PRUint32 aFlags)
{
  if (aFlags & TRANSFORM_CHANGED) {
    
    mCanvasTM = nsnull;
  }

  nsSVGTextFrameBase::NotifySVGChanged(aFlags);

  if (aFlags & COORD_CONTEXT_CHANGED) {
    
    

    
    
    
    NotifyGlyphMetricsChange();
  }
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
nsSVGTextFrame::PaintSVG(nsSVGRenderState* aContext,
                         const nsIntRect *aDirtyRect)
{
  UpdateGlyphPositioning(PR_TRUE);
  
  return nsSVGTextFrameBase::PaintSVG(aContext, aDirtyRect);
}

NS_IMETHODIMP_(nsIFrame*)
nsSVGTextFrame::GetFrameForPoint(const nsPoint &aPoint)
{
  UpdateGlyphPositioning(PR_TRUE);
  
  return nsSVGTextFrameBase::GetFrameForPoint(aPoint);
}

NS_IMETHODIMP
nsSVGTextFrame::UpdateCoveredRegion()
{
  UpdateGlyphPositioning(PR_TRUE);
  
  return nsSVGTextFrameBase::UpdateCoveredRegion();
}

NS_IMETHODIMP
nsSVGTextFrame::InitialUpdate()
{
  nsresult rv = nsSVGTextFrameBase::InitialUpdate();
  
  UpdateGlyphPositioning(PR_FALSE);

  return rv;
}  

gfxRect
nsSVGTextFrame::GetBBoxContribution(const gfxMatrix &aToBBoxUserspace)
{
  UpdateGlyphPositioning(PR_TRUE);

  return nsSVGTextFrameBase::GetBBoxContribution(aToBBoxUserspace);
}




gfxMatrix
nsSVGTextFrame::GetCanvasTM()
{
  if (!mCanvasTM) {
    NS_ASSERTION(mParent, "null parent");

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);
    nsSVGGraphicElement *content = static_cast<nsSVGGraphicElement*>(mContent);

    gfxMatrix tm = content->PrependLocalTransformTo(parent->GetCanvasTM());

    mCanvasTM = NS_NewSVGMatrix(tm);
  }

  return nsSVGUtils::ConvertSVGMatrixToThebes(mCanvasTM);
}




void
nsSVGTextFrame::NotifyGlyphMetricsChange()
{
  mPositioningDirty = PR_TRUE;
  UpdateGlyphPositioning(PR_FALSE);
}

static void
GetSingleValue(nsIDOMSVGLengthList *list, float *val)
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

  mPositioningDirty = PR_FALSE;

  nsISVGGlyphFragmentLeaf *fragment, *firstFragment;

  firstFragment = node->GetFirstGlyphFragment();
  if (!firstFragment) {
    return;
  }

  float x = 0, y = 0;

  {
    nsCOMPtr<nsIDOMSVGLengthList> list = GetX();
    GetSingleValue(list, &x);
  }
  {
    nsCOMPtr<nsIDOMSVGLengthList> list = GetY();
    GetSingleValue(list, &y);
  }

  
  while (firstFragment) {
    {
      nsCOMPtr<nsIDOMSVGLengthList> list = firstFragment->GetX();
      GetSingleValue(list, &x);
    }
    {
      nsCOMPtr<nsIDOMSVGLengthList> list = firstFragment->GetY();
      GetSingleValue(list, &y);
    }

    
    nsSVGTextPathFrame *textPath = firstFragment->FindTextPathParent();
    if (textPath) {
      if (!textPath->GetPathFrame()) {
        
        return;
      }
      x = textPath->GetStartOffset();
    }

    
  
    PRUint8 anchor = firstFragment->GetTextAnchor();

    float chunkLength = 0.0f;
    if (anchor != NS_STYLE_TEXT_ANCHOR_START) {
      
    
      fragment = firstFragment;
      while (fragment) {
        float dx = 0.0f;
        nsCOMPtr<nsIDOMSVGLengthList> list = fragment->GetDx();
        GetSingleValue(list, &dx);
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
        GetSingleValue(list, &dx);
      }
      {
        nsCOMPtr<nsIDOMSVGLengthList> list = fragment->GetDy();
        GetSingleValue(list, &dy);
      }

      fragment->SetGlyphPosition(x + dx, y + dy, aForceGlobalTransform);

      x += dx + fragment->GetAdvance(aForceGlobalTransform);
      y += dy;
      fragment = fragment->GetNextGlyphFragment();
      if (fragment && fragment->IsAbsolutelyPositioned())
        break;
    }
    firstFragment = fragment;
  }
}
