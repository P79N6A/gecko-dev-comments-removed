





































#include "nsIDOMSVGTSpanElement.h"
#include "nsIDOMSVGAltGlyphElement.h"
#include "nsSVGTSpanFrame.h"
#include "nsSVGUtils.h"
#include "nsSVGTextFrame.h"
#include "nsSVGOuterSVGFrame.h"




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
NS_IMETHODIMP
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

    nsCOMPtr<nsIDOMSVGTSpanElement> tspan = do_QueryInterface(aContent);
    nsCOMPtr<nsIDOMSVGAltGlyphElement> altGlyph = do_QueryInterface(aContent);
    NS_ASSERTION(tspan || altGlyph, "Content is not an SVG tspan or altGlyph");
  }

  return nsSVGTSpanFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

NS_IMETHODIMP
nsSVGTSpanFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                  nsIAtom*        aAttribute,
                                  PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::x ||
       aAttribute == nsGkAtoms::y ||
       aAttribute == nsGkAtoms::dx ||
       aAttribute == nsGkAtoms::dy ||
       aAttribute == nsGkAtoms::rotate)) {
    NotifyGlyphMetricsChange();
  }

  return NS_OK;
}




gfxMatrix
nsSVGTSpanFrame::GetCanvasTM()
{
  NS_ASSERTION(mParent, "null parent");
  return static_cast<nsSVGContainerFrame*>(mParent)->GetCanvasTM();  
}




PRUint32
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
nsSVGTSpanFrame::GetSubStringLength(PRUint32 charnum, PRUint32 nchars)
{
  return nsSVGTSpanFrameBase::GetSubStringLength(charnum, nchars);
}

PRInt32
nsSVGTSpanFrame::GetCharNumAtPosition(nsIDOMSVGPoint *point)
{
  return nsSVGTSpanFrameBase::GetCharNumAtPosition(point);
}

NS_IMETHODIMP_(nsSVGGlyphFrame *)
nsSVGTSpanFrame::GetFirstGlyphFrame()
{
  
  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    nsISVGGlyphFragmentNode *node = do_QueryFrame(kid);
    if (node)
      return node->GetFirstGlyphFrame();
    kid = kid->GetNextSibling();
  }

  
  return GetNextGlyphFrame();

}

NS_IMETHODIMP_(nsSVGGlyphFrame *)
nsSVGTSpanFrame::GetNextGlyphFrame()
{
  nsIFrame* sibling = GetNextSibling();
  while (sibling) {
    nsISVGGlyphFragmentNode *node = do_QueryFrame(sibling);
    if (node)
      return node->GetFirstGlyphFrame();
    sibling = sibling->GetNextSibling();
  }

  
  
  NS_ASSERTION(GetParent(), "null parent");
  nsISVGGlyphFragmentNode *node = do_QueryFrame(GetParent());
  return node ? node->GetNextGlyphFrame() : nsnull;
}

NS_IMETHODIMP_(void)
nsSVGTSpanFrame::SetWhitespaceCompression(PRBool)
{
  nsSVGTSpanFrameBase::SetWhitespaceCompression();
}
