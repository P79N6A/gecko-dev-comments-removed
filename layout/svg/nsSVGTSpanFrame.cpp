





#include "nsSVGTSpanFrame.h"


#include "nsSVGEffects.h"
#include "nsSVGIntegrationUtils.h"
#include "nsSVGUtils.h"




nsIFrame*
NS_NewSVGTSpanFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGTSpanFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGTSpanFrame)

nsIAtom *
nsSVGTSpanFrame::GetType() const
{
  return nsGkAtoms::svgTSpanFrame;
}




NS_QUERYFRAME_HEAD(nsSVGTSpanFrame)
  NS_QUERYFRAME_ENTRY(nsISVGGlyphFragmentNode)
NS_QUERYFRAME_TAIL_INHERITING(nsSVGTSpanFrameBase)




#ifdef DEBUG
void
nsSVGTSpanFrame::Init(nsIContent* aContent,
                      nsIFrame* aParent,
                      nsIFrame* aPrevInFlow)
{
  NS_ASSERTION(aParent, "null parent");

  
  
  
  
  if (GetType() == nsGkAtoms::svgTSpanFrame) {
    nsIFrame* ancestorFrame = nsSVGUtils::GetFirstNonAAncestorFrame(aParent);
    NS_ASSERTION(ancestorFrame, "Must have ancestor");

    nsSVGTextContainerFrame *metrics = do_QueryFrame(ancestorFrame);
    NS_ASSERTION(metrics,
                 "trying to construct an SVGTSpanFrame for an invalid "
                 "container");

    NS_ASSERTION(aContent->IsSVG() && (aContent->Tag() == nsGkAtoms::altGlyph ||
                                       aContent->Tag() == nsGkAtoms::tspan),
                 "Content is not an SVG tspan or altGlyph");
  }

  nsSVGTSpanFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

NS_IMETHODIMP
nsSVGTSpanFrame::AttributeChanged(int32_t         aNameSpaceID,
                                  nsIAtom*        aAttribute,
                                  int32_t         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::x ||
       aAttribute == nsGkAtoms::y ||
       aAttribute == nsGkAtoms::dx ||
       aAttribute == nsGkAtoms::dy ||
       aAttribute == nsGkAtoms::rotate)) {
    nsSVGEffects::InvalidateRenderingObservers(this);
    nsSVGUtils::ScheduleReflowSVG(this);
    NotifyGlyphMetricsChange();
  }

  return NS_OK;
}




gfxMatrix
nsSVGTSpanFrame::GetCanvasTM(uint32_t aFor, nsIFrame* aTransformRoot)
{
  if (!(GetStateBits() & NS_FRAME_IS_NONDISPLAY) && !aTransformRoot) {
    if ((aFor == FOR_PAINTING && NS_SVGDisplayListPaintingEnabled()) ||
        (aFor == FOR_HIT_TESTING && NS_SVGDisplayListHitTestingEnabled())) {
      return nsSVGIntegrationUtils::GetCSSPxToDevPxMatrix(this);
    }
  }
  NS_ASSERTION(mParent, "null parent");
  return static_cast<nsSVGContainerFrame*>(mParent)->
      GetCanvasTM(aFor, aTransformRoot);
}




uint32_t
nsSVGTSpanFrame::GetNumberOfChars()
{
  return nsSVGTSpanFrameBase::GetNumberOfChars();
}

float
nsSVGTSpanFrame::GetComputedTextLength()
{
  return nsSVGTSpanFrameBase::GetComputedTextLength();
}

float
nsSVGTSpanFrame::GetSubStringLength(uint32_t charnum, uint32_t nchars)
{
  return nsSVGTSpanFrameBase::GetSubStringLength(charnum, nchars);
}

int32_t
nsSVGTSpanFrame::GetCharNumAtPosition(mozilla::nsISVGPoint *point)
{
  return nsSVGTSpanFrameBase::GetCharNumAtPosition(point);
}

NS_IMETHODIMP_(void)
nsSVGTSpanFrame::SetWhitespaceCompression(bool)
{
  nsSVGTSpanFrameBase::SetWhitespaceCompression();
}
