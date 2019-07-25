






































#include "nsSVGTextFrame.h"


#include "nsGkAtoms.h"
#include "nsIDOMSVGRect.h"
#include "nsIDOMSVGTextElement.h"
#include "nsISVGGlyphFragmentNode.h"
#include "nsSVGGlyphFrame.h"
#include "nsSVGGraphicElement.h"
#include "nsSVGPathElement.h"
#include "nsSVGTextPathFrame.h"
#include "nsSVGUtils.h"
#include "SVGLengthList.h"

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

    NotifySVGChanged(TRANSFORM_CHANGED);
   
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
  UpdateGlyphPositioning(false);

  return nsSVGTextFrameBase::GetNumberOfChars();
}

float
nsSVGTextFrame::GetComputedTextLength()
{
  UpdateGlyphPositioning(false);

  return nsSVGTextFrameBase::GetComputedTextLength();
}

float
nsSVGTextFrame::GetSubStringLength(PRUint32 charnum, PRUint32 nchars)
{
  UpdateGlyphPositioning(false);

  return nsSVGTextFrameBase::GetSubStringLength(charnum, nchars);
}

PRInt32
nsSVGTextFrame::GetCharNumAtPosition(nsIDOMSVGPoint *point)
{
  UpdateGlyphPositioning(false);

  return nsSVGTextFrameBase::GetCharNumAtPosition(point);
}

NS_IMETHODIMP
nsSVGTextFrame::GetStartPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)
{
  UpdateGlyphPositioning(false);

  return nsSVGTextFrameBase::GetStartPositionOfChar(charnum,  _retval);
}

NS_IMETHODIMP
nsSVGTextFrame::GetEndPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)
{
  UpdateGlyphPositioning(false);

  return nsSVGTextFrameBase::GetEndPositionOfChar(charnum,  _retval);
}

NS_IMETHODIMP
nsSVGTextFrame::GetExtentOfChar(PRUint32 charnum, nsIDOMSVGRect **_retval)
{
  UpdateGlyphPositioning(false);

  return nsSVGTextFrameBase::GetExtentOfChar(charnum,  _retval);
}

NS_IMETHODIMP
nsSVGTextFrame::GetRotationOfChar(PRUint32 charnum, float *_retval)
{
  UpdateGlyphPositioning(false);

  return nsSVGTextFrameBase::GetRotationOfChar(charnum,  _retval);
}




void
nsSVGTextFrame::NotifySVGChanged(PRUint32 aFlags)
{
  NS_ABORT_IF_FALSE(!(aFlags & DO_NOT_NOTIFY_RENDERING_OBSERVERS) ||
                    (GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD),
                    "Must be NS_STATE_SVG_NONDISPLAY_CHILD!");

  NS_ABORT_IF_FALSE(aFlags & (TRANSFORM_CHANGED | COORD_CONTEXT_CHANGED),
                    "Invalidation logic may need adjusting");

  bool updateGlyphMetrics = false;
  
  if (aFlags & COORD_CONTEXT_CHANGED) {
    updateGlyphMetrics = true;
  }

  if (aFlags & TRANSFORM_CHANGED) {
    if (mCanvasTM && mCanvasTM->IsSingular()) {
      
      updateGlyphMetrics = true;
    }
    
    mCanvasTM = nsnull;
  }

  nsSVGTextFrameBase::NotifySVGChanged(aFlags);

  if (updateGlyphMetrics) {
    
    

    
    
    
    NotifyGlyphMetricsChange();
  }
}

NS_IMETHODIMP
nsSVGTextFrame::PaintSVG(nsRenderingContext* aContext,
                         const nsIntRect *aDirtyRect)
{
  UpdateGlyphPositioning(true);
  
  return nsSVGTextFrameBase::PaintSVG(aContext, aDirtyRect);
}

NS_IMETHODIMP_(nsIFrame*)
nsSVGTextFrame::GetFrameForPoint(const nsPoint &aPoint)
{
  UpdateGlyphPositioning(true);
  
  return nsSVGTextFrameBase::GetFrameForPoint(aPoint);
}

void
nsSVGTextFrame::UpdateBounds()
{
  NS_ASSERTION(nsSVGUtils::OuterSVGIsCallingUpdateBounds(this),
               "This call is probaby a wasteful mistake");

  NS_ABORT_IF_FALSE(!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD),
                    "UpdateBounds mechanism not designed for this");

  if (!nsSVGUtils::NeedsUpdatedBounds(this)) {
    NS_ASSERTION(!mPositioningDirty, "How did this happen?");
    return;
  }

  
  
  
  mPositioningDirty = true;

  UpdateGlyphPositioning(false);

  
  
  nsSVGTextFrameBase::UpdateBounds();

  
  
}

SVGBBox
nsSVGTextFrame::GetBBoxContribution(const gfxMatrix &aToBBoxUserspace,
                                    PRUint32 aFlags)
{
  UpdateGlyphPositioning(true);

  return nsSVGTextFrameBase::GetBBoxContribution(aToBBoxUserspace, aFlags);
}




gfxMatrix
nsSVGTextFrame::GetCanvasTM()
{
  if (!mCanvasTM) {
    NS_ASSERTION(mParent, "null parent");

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);
    nsSVGGraphicElement *content = static_cast<nsSVGGraphicElement*>(mContent);

    gfxMatrix tm = content->PrependLocalTransformsTo(parent->GetCanvasTM());

    mCanvasTM = new gfxMatrix(tm);
  }

  return *mCanvasTM;
}




void
nsSVGTextFrame::NotifyGlyphMetricsChange()
{
  nsSVGUtils::InvalidateAndScheduleBoundsUpdate(this);

  mPositioningDirty = true;
}

void
nsSVGTextFrame::SetWhitespaceHandling(nsSVGGlyphFrame *aFrame)
{
  SetWhitespaceCompression();

  nsSVGGlyphFrame* firstFrame = aFrame;
  bool trimLeadingWhitespace = true;
  nsSVGGlyphFrame* lastNonWhitespaceFrame = aFrame;

  
  
  
  while (aFrame) {
    if (!aFrame->IsAllWhitespace()) {
      lastNonWhitespaceFrame = aFrame;
    }

    aFrame->SetTrimLeadingWhitespace(trimLeadingWhitespace);
    trimLeadingWhitespace = aFrame->EndsWithWhitespace();

    aFrame = aFrame->GetNextGlyphFrame();
  }

  
  
  
  
  aFrame = firstFrame;
  while (aFrame != lastNonWhitespaceFrame) {
    aFrame->SetTrimTrailingWhitespace(false);
    aFrame = aFrame->GetNextGlyphFrame();
  }

  
  
  
  while (aFrame) {
    aFrame->SetTrimTrailingWhitespace(true);
    aFrame = aFrame->GetNextGlyphFrame();
  }
}

void
nsSVGTextFrame::UpdateGlyphPositioning(bool aForceGlobalTransform)
{
  if (!mPositioningDirty)
    return;

  mPositioningDirty = false;

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
}
