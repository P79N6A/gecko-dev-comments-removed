




































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
PaintCheckMark(nsIRenderingContext& aRenderingContext,
               const nsRect& aRect)
{
  
  const PRInt32 checkPolygonX[] = { -3, -1,  3,  3, -1, -3 };
  const PRInt32 checkPolygonY[] = { -1,  1, -3, -1,  3,  1 };
  const PRInt32 checkNumPoints = sizeof(checkPolygonX) / sizeof(PRInt32);
  const PRInt32 checkSize      = 9; 
                                    

  
  nscoord paintScale = PR_MIN(aRect.width, aRect.height) / checkSize;
  nsPoint paintCenter(aRect.x + aRect.width  / 2,
                      aRect.y + aRect.height / 2);

  nsPoint paintPolygon[checkNumPoints];
  
  for (PRInt32 polyIndex = 0; polyIndex < checkNumPoints; polyIndex++) {
    paintPolygon[polyIndex] = paintCenter +
                              nsPoint(checkPolygonX[polyIndex] * paintScale,
                                      checkPolygonY[polyIndex] * paintScale);
  }

  aRenderingContext.FillPolygon(paintPolygon, checkNumPoints);
}

static void
PaintIndeterminateMark(nsIRenderingContext& aRenderingContext,
                       const nsRect& aRect)
{
  
  nsRect fillRect = aRect;
  fillRect.height /= 4;
  fillRect.y += (aRect.height - fillRect.height) / 2;

  aRenderingContext.FillRect(fillRect);
}


nsIFrame*
NS_NewGfxCheckboxControlFrame(nsIPresShell* aPresShell,
                              nsStyleContext* aContext)
{
  return new (aPresShell) nsGfxCheckboxControlFrame(aContext);
}




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

class nsDisplayCheckMark : public nsDisplayItem {
public:
  nsDisplayCheckMark(nsGfxCheckboxControlFrame* aFrame)
    : nsDisplayItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayCheckMark);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayCheckMark() {
    MOZ_COUNT_DTOR(nsDisplayCheckMark);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("CheckMark")
};

void
nsDisplayCheckMark::Paint(nsDisplayListBuilder* aBuilder,
     nsIRenderingContext* aCtx, const nsRect& aDirtyRect) {
  static_cast<nsGfxCheckboxControlFrame*>(mFrame)->
    PaintCheckBox(*aCtx, aBuilder->ToReferenceFrame(mFrame), aDirtyRect);
}



void
nsGfxCheckboxControlFrame::PaintCheckBox(nsIRenderingContext& aRenderingContext,
                                         nsPoint aPt,
                                         const nsRect& aDirtyRect)
{
  
  
  nsRect checkRect(aPt, mRect.Size());
  checkRect.Deflate(GetUsedBorderAndPadding());

  const nsStyleColor* color = GetStyleColor();
  aRenderingContext.SetColor(color->mColor);

  if (IsIndeterminate())
    PaintIndeterminateMark(aRenderingContext, checkRect);
  else
    PaintCheckMark(aRenderingContext, checkRect);
}


NS_IMETHODIMP
nsGfxCheckboxControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                            const nsRect&           aDirtyRect,
                                            const nsDisplayListSet& aLists)
{
  nsresult rv = nsFormControlFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  if ((!IsChecked() && !IsIndeterminate()) || !IsVisibleForPainting(aBuilder))
    return NS_OK;   
    
  if (IsThemed())
    return NS_OK; 

  return aLists.Content()->AppendNewToTop(new (aBuilder)
                                          nsDisplayCheckMark(this));
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
