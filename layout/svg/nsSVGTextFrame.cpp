





#include "nsSVGTextFrame.h"


#include "nsGkAtoms.h"
#include "mozilla/dom/SVGIRect.h"
#include "nsISVGGlyphFragmentNode.h"
#include "nsSVGGlyphFrame.h"
#include "nsSVGIntegrationUtils.h"
#include "nsSVGTextPathFrame.h"
#include "nsSVGUtils.h"
#include "SVGGraphicsElement.h"
#include "SVGLengthList.h"

using namespace mozilla;




nsIFrame*
NS_NewSVGTextFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGTextFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGTextFrame)



#ifdef DEBUG
void
nsSVGTextFrame::Init(nsIContent* aContent,
                     nsIFrame* aParent,
                     nsIFrame* aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::text),
               "Content is not an SVG text");

  nsSVGTextFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

NS_IMETHODIMP
nsSVGTextFrame::AttributeChanged(int32_t         aNameSpaceID,
                                 nsIAtom*        aAttribute,
                                 int32_t         aModType)
{
  if (aNameSpaceID != kNameSpaceID_None)
    return NS_OK;

  if (aAttribute == nsGkAtoms::transform) {
    nsSVGUtils::InvalidateBounds(this, false);
    nsSVGUtils::ScheduleReflowSVG(this);
    NotifySVGChanged(TRANSFORM_CHANGED);
  } else if (aAttribute == nsGkAtoms::x ||
             aAttribute == nsGkAtoms::y ||
             aAttribute == nsGkAtoms::dx ||
             aAttribute == nsGkAtoms::dy ||
             aAttribute == nsGkAtoms::rotate) {
    nsSVGUtils::InvalidateBounds(this, false);
    nsSVGUtils::ScheduleReflowSVG(this);
    NotifyGlyphMetricsChange();
  }

 return NS_OK;
}

nsIAtom *
nsSVGTextFrame::GetType() const
{
  return nsGkAtoms::svgTextFrame;
}

void
nsSVGTextFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                 const nsRect&           aDirtyRect,
                                 const nsDisplayListSet& aLists)
{
  UpdateGlyphPositioning(true);
  nsSVGTextFrameBase::BuildDisplayList(aBuilder, aDirtyRect, aLists);
}



uint32_t
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
nsSVGTextFrame::GetSubStringLength(uint32_t charnum, uint32_t nchars)
{
  UpdateGlyphPositioning(false);

  return nsSVGTextFrameBase::GetSubStringLength(charnum, nchars);
}

int32_t
nsSVGTextFrame::GetCharNumAtPosition(nsISVGPoint *point)
{
  UpdateGlyphPositioning(false);

  return nsSVGTextFrameBase::GetCharNumAtPosition(point);
}

NS_IMETHODIMP
nsSVGTextFrame::GetStartPositionOfChar(uint32_t charnum, nsISupports **_retval)
{
  UpdateGlyphPositioning(false);

  return nsSVGTextFrameBase::GetStartPositionOfChar(charnum,  _retval);
}

NS_IMETHODIMP
nsSVGTextFrame::GetEndPositionOfChar(uint32_t charnum, nsISupports **_retval)
{
  UpdateGlyphPositioning(false);

  return nsSVGTextFrameBase::GetEndPositionOfChar(charnum,  _retval);
}

NS_IMETHODIMP
nsSVGTextFrame::GetExtentOfChar(uint32_t charnum, dom::SVGIRect **_retval)
{
  UpdateGlyphPositioning(false);

  return nsSVGTextFrameBase::GetExtentOfChar(charnum,  _retval);
}

NS_IMETHODIMP
nsSVGTextFrame::GetRotationOfChar(uint32_t charnum, float *_retval)
{
  UpdateGlyphPositioning(false);

  return nsSVGTextFrameBase::GetRotationOfChar(charnum,  _retval);
}




void
nsSVGTextFrame::NotifySVGChanged(uint32_t aFlags)
{
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
    
    mCanvasTM = nullptr;
  }

  if (updateGlyphMetrics) {
    
    
    
    
    
    
    nsSVGUtils::ScheduleReflowSVG(this);
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
  NS_ASSERTION(!NS_SVGDisplayListPaintingEnabled() ||
               (mState & NS_STATE_SVG_NONDISPLAY_CHILD),
               "If display lists are enabled, only painting of non-display "
               "SVG should take this code path");

  UpdateGlyphPositioning(true);
  
  return nsSVGTextFrameBase::PaintSVG(aContext, aDirtyRect);
}

NS_IMETHODIMP_(nsIFrame*)
nsSVGTextFrame::GetFrameForPoint(const nsPoint &aPoint)
{
  NS_ASSERTION(!NS_SVGDisplayListHitTestingEnabled() ||
               (mState & NS_STATE_SVG_NONDISPLAY_CHILD),
               "If display lists are enabled, only hit-testing of non-display "
               "SVG should take this code path");

  UpdateGlyphPositioning(true);
  
  return nsSVGTextFrameBase::GetFrameForPoint(aPoint);
}

void
nsSVGTextFrame::ReflowSVG()
{
  NS_ASSERTION(nsSVGUtils::OuterSVGIsCallingReflowSVG(this),
               "This call is probably a wasteful mistake");

  NS_ABORT_IF_FALSE(!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD),
                    "ReflowSVG mechanism not designed for this");

  if (!nsSVGUtils::NeedsReflowSVG(this)) {
    NS_ASSERTION(!mPositioningDirty, "How did this happen?");
    return;
  }

  
  
  
  mPositioningDirty = true;

  UpdateGlyphPositioning(false);

  
  
  
  nsSVGTextFrameBase::ReflowSVG();
}

SVGBBox
nsSVGTextFrame::GetBBoxContribution(const gfxMatrix &aToBBoxUserspace,
                                    uint32_t aFlags)
{
  UpdateGlyphPositioning(true);

  return nsSVGTextFrameBase::GetBBoxContribution(aToBBoxUserspace, aFlags);
}




gfxMatrix
nsSVGTextFrame::GetCanvasTM(uint32_t aFor)
{
  if (!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)) {
    if ((aFor == FOR_PAINTING && NS_SVGDisplayListPaintingEnabled()) ||
        (aFor == FOR_HIT_TESTING && NS_SVGDisplayListHitTestingEnabled())) {
      return nsSVGIntegrationUtils::GetCSSPxToDevPxMatrix(this);
    }
  }

  if (!mCanvasTM) {
    NS_ASSERTION(mParent, "null parent");

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);
    dom::SVGGraphicsElement *content = static_cast<dom::SVGGraphicsElement*>(mContent);

    gfxMatrix tm =
      content->PrependLocalTransformsTo(parent->GetCanvasTM(aFor));

    mCanvasTM = new gfxMatrix(tm);
  }

  return *mCanvasTM;
}




static void
MarkDirtyBitsOnDescendants(nsIFrame *aFrame)
{
  
  
  
  if (aFrame->GetStateBits() & (NS_FRAME_FIRST_REFLOW)) {
    
    return;
  }
  nsIFrame* kid = aFrame->GetFirstPrincipalChild();
  while (kid) {
    nsISVGChildFrame* svgkid = do_QueryFrame(kid);
    if (svgkid) {
      MarkDirtyBitsOnDescendants(kid);
      kid->AddStateBits(NS_FRAME_IS_DIRTY);
    }
    kid = kid->GetNextSibling();
  }
}

void
nsSVGTextFrame::NotifyGlyphMetricsChange()
{
  
  
  MarkDirtyBitsOnDescendants(this);

  nsSVGUtils::InvalidateBounds(this, false);
  nsSVGUtils::ScheduleReflowSVG(this);

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

    
  
    uint8_t anchor = firstFrame->GetTextAnchor();

    






















#if 0
    if (StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
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
