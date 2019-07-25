





































#include "nsIDOMSVGTextElement.h"
#include "nsSVGTextFrame.h"
#include "nsWeakReference.h"
#include "SVGLengthList.h"
#include "nsIDOMSVGLength.h"
#include "nsIDOMSVGAnimatedNumber.h"
#include "nsISVGGlyphFragmentNode.h"
#include "nsSVGGlyphFrame.h"
#include "nsSVGOuterSVGFrame.h"
#include "nsIDOMSVGRect.h"
#include "nsSVGRect.h"
#include "nsSVGMatrix.h"
#include "nsGkAtoms.h"
#include "nsSVGTextPathFrame.h"
#include "nsSVGPathElement.h"
#include "nsSVGUtils.h"
#include "nsSVGGraphicElement.h"

using namespace mozilla;




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
             aAttribute == nsGkAtoms::dy ||
             aAttribute == nsGkAtoms::rotate) {
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

void
nsSVGTextFrame::SetWhitespaceHandling(nsSVGGlyphFrame *aFrame)
{
  SetWhitespaceCompression();

  PRBool trimLeadingWhitespace = PR_TRUE;
  nsSVGGlyphFrame* lastNonWhitespaceFrame = aFrame;

  while (aFrame) {
    if (!aFrame->IsAllWhitespace()) {
      lastNonWhitespaceFrame = aFrame;
    }

    aFrame->SetTrimLeadingWhitespace(trimLeadingWhitespace);
    trimLeadingWhitespace = aFrame->EndsWithWhitespace();

    aFrame = aFrame->GetNextGlyphFrame();
  }

  lastNonWhitespaceFrame->SetTrimTrailingWhitespace(PR_TRUE);
}

void
nsSVGTextFrame::UpdateGlyphPositioning(PRBool aForceGlobalTransform)
{
  if (mMetricsState == suspended || !mPositioningDirty)
    return;

  mPositioningDirty = PR_FALSE;

  nsISVGGlyphFragmentNode* node = GetFirstGlyphFragmentChildNode();
  if (!node)
    return;

  nsSVGGlyphFrame *frame, *firstFrame;

  firstFrame = node->GetFirstGlyphFrame();
  if (!firstFrame) {
    return;
  }

  SetWhitespaceHandling(firstFrame);

  BuildPositionList(0, 0);

  gfxPoint ctp(0.0, 0.0);

  
  while (firstFrame) {
    nsSVGTextPathFrame *textPath = firstFrame->FindTextPathParent();

    nsTArray<float> effectiveXList, effectiveYList;
    firstFrame->GetEffectiveXY(firstFrame->GetNumberOfChars(),
                               effectiveXList, effectiveYList);
    if (!effectiveXList.IsEmpty()) ctp.x = effectiveXList[0];
    if (!textPath && !effectiveYList.IsEmpty()) ctp.y = effectiveYList[0];

    
    if (textPath) {
      if (!textPath->GetPathFrame()) {
        
        return;
      }
      ctp.x = textPath->GetStartOffset();
    }

    
  
    PRUint8 anchor = firstFrame->GetTextAnchor();

    






















#if 0
    if (GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
      if (anchor == NS_STYLE_TEXT_ANCHOR_END) {
        anchor = NS_STYLE_TEXT_ANCHOR_START;
      } else if (anchor == NS_STYLE_TEXT_ANCHOR_START) {
        anchor = NS_STYLE_TEXT_ANCHOR_END;
      }
    }
#endif

    float chunkLength = 0.0f;
    if (anchor != NS_STYLE_TEXT_ANCHOR_START) {
      
    
      frame = firstFrame;
      while (frame) {
        chunkLength += frame->GetAdvance(aForceGlobalTransform);
        frame = frame->GetNextGlyphFrame();
        if (frame && frame->IsAbsolutelyPositioned())
          break;
      }
    }

    if (anchor == NS_STYLE_TEXT_ANCHOR_MIDDLE)
      ctp.x -= chunkLength/2.0f;
    else if (anchor == NS_STYLE_TEXT_ANCHOR_END)
      ctp.x -= chunkLength;
  
    
  
    frame = firstFrame;
    while (frame) {

      frame->SetGlyphPosition(&ctp, aForceGlobalTransform);
      frame = frame->GetNextGlyphFrame();
      if (frame && frame->IsAbsolutelyPositioned())
        break;
    }
    firstFrame = frame;
  }
  nsSVGUtils::UpdateGraphic(this);
}
