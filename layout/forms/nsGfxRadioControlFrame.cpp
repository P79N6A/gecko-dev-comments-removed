




































#include "nsGfxRadioControlFrame.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsCSSRendering.h"
#include "nsRenderingContext.h"
#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif
#include "nsIServiceManager.h"
#include "nsITheme.h"
#include "nsDisplayList.h"
#include "nsCSSAnonBoxes.h"

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
already_AddRefed<nsAccessible>
nsGfxRadioControlFrame::CreateAccessible()
{
  nsAccessibilityService* accService = nsIPresShell::AccService();
  if (accService) {
    return accService->CreateHTMLRadioButtonAccessible(mContent,
                                                       PresContext()->PresShell());
  }

  return nsnull;
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

  aCtx->SetColor(aFrame->GetStyleColor()->mColor);
  aCtx->FillEllipse(rect);
}

NS_IMETHODIMP
nsGfxRadioControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists)
{
  nsresult rv = nsFormControlFrame::BuildDisplayList(aBuilder, aDirtyRect,
                                                     aLists);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (!IsVisibleForPainting(aBuilder))
    return NS_OK;
  
  if (IsThemed())
    return NS_OK; 

  PRBool checked = PR_TRUE;
  GetCurrentCheckState(&checked); 
  if (!checked)
    return NS_OK;
    
  return aLists.Content()->AppendNewToTop(new (aBuilder)
    nsDisplayGeneric(aBuilder, this, PaintCheckedRadioButton,
                     "CheckedRadioButton",
                     nsDisplayItem::TYPE_CHECKED_RADIOBUTTON));
}
