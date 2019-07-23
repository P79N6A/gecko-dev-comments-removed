




































#include "nsGfxCheckboxControlFrame.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsCSSRendering.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif
#include "nsIServiceManager.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsDisplayList.h"
#include "nsCSSAnonBoxes.h"
#include "nsIDOMNSHTMLInputElement.h"

static void
PaintCheckMark(nsIFrame* aFrame,
               nsIRenderingContext* aCtx,
               const nsRect& aDirtyRect,
               nsPoint aPt)
{
  nsRect rect(aPt, aFrame->GetSize());
  rect.Deflate(aFrame->GetUsedBorderAndPadding());

  
  const PRInt32 checkPolygonX[] = { -3, -1,  3,  3, -1, -3 };
  const PRInt32 checkPolygonY[] = { -1,  1, -3, -1,  3,  1 };
  const PRInt32 checkNumPoints = sizeof(checkPolygonX) / sizeof(PRInt32);
  const PRInt32 checkSize      = 9; 
                                    

  
  nscoord paintScale = NS_MIN(rect.width, rect.height) / checkSize;
  nsPoint paintCenter(rect.x + rect.width  / 2,
                      rect.y + rect.height / 2);

  nsPoint paintPolygon[checkNumPoints];
  
  for (PRInt32 polyIndex = 0; polyIndex < checkNumPoints; polyIndex++) {
    paintPolygon[polyIndex] = paintCenter +
                              nsPoint(checkPolygonX[polyIndex] * paintScale,
                                      checkPolygonY[polyIndex] * paintScale);
  }

  aCtx->SetColor(aFrame->GetStyleColor()->mColor);
  aCtx->FillPolygon(paintPolygon, checkNumPoints);
}

static void
PaintIndeterminateMark(nsIFrame* aFrame,
                       nsIRenderingContext* aCtx,
                       const nsRect& aDirtyRect,
                       nsPoint aPt)
{
  nsRect rect(aPt, aFrame->GetSize());
  rect.Deflate(aFrame->GetUsedBorderAndPadding());

  rect.y += (rect.height - rect.height/4) / 2;
  rect.height /= 4;

  aCtx->SetColor(aFrame->GetStyleColor()->mColor);
  aCtx->FillRect(rect);
}


nsIFrame*
NS_NewGfxCheckboxControlFrame(nsIPresShell* aPresShell,
                              nsStyleContext* aContext)
{
  return new (aPresShell) nsGfxCheckboxControlFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsGfxCheckboxControlFrame)




nsGfxCheckboxControlFrame::nsGfxCheckboxControlFrame(nsStyleContext* aContext)
: nsFormControlFrame(aContext)
{
}

nsGfxCheckboxControlFrame::~nsGfxCheckboxControlFrame()
{
}


NS_QUERYFRAME_HEAD(nsGfxCheckboxControlFrame)
  NS_QUERYFRAME_ENTRY(nsICheckboxControlFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsFormControlFrame)

#ifdef ACCESSIBILITY
NS_IMETHODIMP
nsGfxCheckboxControlFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService
    = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    return accService->CreateHTMLCheckboxAccessible(
      static_cast<nsIFrame*>(this), aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif


NS_IMETHODIMP
nsGfxCheckboxControlFrame::OnChecked(nsPresContext* aPresContext,
                                     PRBool aChecked)
{
  InvalidateOverflowRect();
  return NS_OK;
}


NS_IMETHODIMP
nsGfxCheckboxControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                            const nsRect&           aDirtyRect,
                                            const nsDisplayListSet& aLists)
{
  nsresult rv = nsFormControlFrame::BuildDisplayList(aBuilder, aDirtyRect,
                                                     aLists);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  if ((!IsChecked() && !IsIndeterminate()) || !IsVisibleForPainting(aBuilder))
    return NS_OK;   
    
  if (IsThemed())
    return NS_OK; 

  return aLists.Content()->AppendNewToTop(new (aBuilder)
    nsDisplayGeneric(this,
                     IsIndeterminate()
                     ? PaintIndeterminateMark : PaintCheckMark,
                     "CheckedCheckbox"));
}


PRBool
nsGfxCheckboxControlFrame::IsChecked()
{
  nsCOMPtr<nsIDOMHTMLInputElement> elem(do_QueryInterface(mContent));
  PRBool retval = PR_FALSE;
  elem->GetChecked(&retval);
  return retval;
}

PRBool
nsGfxCheckboxControlFrame::IsIndeterminate()
{
  nsCOMPtr<nsIDOMNSHTMLInputElement> elem(do_QueryInterface(mContent));
  PRBool retval = PR_FALSE;
  elem->GetIndeterminate(&retval);
  return retval;
}
