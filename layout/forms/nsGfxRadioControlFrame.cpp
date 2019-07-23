




































#include "nsGfxRadioControlFrame.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsCSSRendering.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
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

NS_QUERYFRAME_HEAD(nsGfxRadioControlFrame)
  NS_QUERYFRAME_ENTRY(nsIRadioControlFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsFormControlFrame)

#ifdef ACCESSIBILITY
NS_IMETHODIMP
nsGfxRadioControlFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService
    = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    return accService->CreateHTMLRadioButtonAccessible(
      static_cast<nsIFrame*>(this), aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif



static void
PaintCheckedRadioButton(nsIFrame* aFrame,
                        nsIRenderingContext* aCtx,
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
    nsDisplayGeneric(this, PaintCheckedRadioButton, "CheckedRadioButton"));
}

NS_IMETHODIMP
nsGfxRadioControlFrame::OnChecked(nsPresContext* aPresContext,
                                  PRBool aChecked)
{
  InvalidateOverflowRect();
  return NS_OK;
}
