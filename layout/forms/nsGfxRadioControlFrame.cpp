




#include "nsGfxRadioControlFrame.h"

#include "gfx2DGlue.h"
#include "gfxUtils.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/PathHelpers.h"
#include "nsLayoutUtils.h"
#include "nsRenderingContext.h"
#include "nsDisplayList.h"

using namespace mozilla;
using namespace mozilla::gfx;

nsIFrame*
NS_NewGfxRadioControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsGfxRadioControlFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsGfxRadioControlFrame)

nsGfxRadioControlFrame::nsGfxRadioControlFrame(nsStyleContext* aContext):
  nsFormControlFrame(aContext)
{
}

nsGfxRadioControlFrame::~nsGfxRadioControlFrame()
{
}

#ifdef ACCESSIBILITY
a11y::AccType
nsGfxRadioControlFrame::AccessibleType()
{
  return a11y::eHTMLRadioButtonType;
}
#endif



static void
PaintCheckedRadioButton(nsIFrame* aFrame,
                        nsRenderingContext* aCtx,
                        const nsRect& aDirtyRect,
                        nsPoint aPt)
{
  
  
  nsRect rect(aPt, aFrame->GetSize());
  rect.Deflate(aFrame->GetUsedBorderAndPadding());
  rect.Deflate(nsPresContext::CSSPixelsToAppUnits(2),
               nsPresContext::CSSPixelsToAppUnits(2));

  Rect devPxRect =
    ToRect(nsLayoutUtils::RectToGfxRect(rect,
                                        aFrame->PresContext()->AppUnitsPerDevPixel()));

  ColorPattern color(ToDeviceColor(aFrame->StyleColor()->mColor));

  DrawTarget* drawTarget = aCtx->GetDrawTarget();
  RefPtr<PathBuilder> builder = drawTarget->CreatePathBuilder();
  AppendEllipseToPath(builder, devPxRect.Center(), devPxRect.Size());
  RefPtr<Path> ellipse = builder->Finish();
  drawTarget->Fill(ellipse, color);
}

void
nsGfxRadioControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists)
{
  nsFormControlFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);

  if (!IsVisibleForPainting(aBuilder))
    return;
  
  if (IsThemed())
    return; 

  bool checked = true;
  GetCurrentCheckState(&checked); 
  if (!checked)
    return;
    
  aLists.Content()->AppendNewToTop(new (aBuilder)
    nsDisplayGeneric(aBuilder, this, PaintCheckedRadioButton,
                     "CheckedRadioButton",
                     nsDisplayItem::TYPE_CHECKED_RADIOBUTTON));
}
