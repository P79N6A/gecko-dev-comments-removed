




#include "nsGfxCheckboxControlFrame.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsCSSRendering.h"
#include "nsRenderingContext.h"
#include "nsIServiceManager.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsDisplayList.h"
#include "nsCSSAnonBoxes.h"
#include <algorithm>

using namespace mozilla;

static void
PaintCheckMark(nsIFrame* aFrame,
               nsRenderingContext* aCtx,
               const nsRect& aDirtyRect,
               nsPoint aPt)
{
  nsRect rect(aPt, aFrame->GetSize());
  rect.Deflate(aFrame->GetUsedBorderAndPadding());

  
  const int32_t checkPolygonX[] = { -3, -1,  3,  3, -1, -3 };
  const int32_t checkPolygonY[] = { -1,  1, -3, -1,  3,  1 };
  const int32_t checkNumPoints = sizeof(checkPolygonX) / sizeof(int32_t);
  const int32_t checkSize      = 9; 
                                    

  
  nscoord paintScale = std::min(rect.width, rect.height) / checkSize;
  nsPoint paintCenter(rect.x + rect.width  / 2,
                      rect.y + rect.height / 2);

  nsPoint paintPolygon[checkNumPoints];
  
  for (int32_t polyIndex = 0; polyIndex < checkNumPoints; polyIndex++) {
    paintPolygon[polyIndex] = paintCenter +
                              nsPoint(checkPolygonX[polyIndex] * paintScale,
                                      checkPolygonY[polyIndex] * paintScale);
  }

  aCtx->SetColor(aFrame->GetStyleColor()->mColor);
  aCtx->FillPolygon(paintPolygon, checkNumPoints);
}

static void
PaintIndeterminateMark(nsIFrame* aFrame,
                       nsRenderingContext* aCtx,
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

#ifdef ACCESSIBILITY
a11y::AccType
nsGfxCheckboxControlFrame::AccessibleType()
{
  return a11y::eHTMLCheckboxType;
}
#endif


NS_IMETHODIMP
nsGfxCheckboxControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                            const nsRect&           aDirtyRect,
                                            const nsDisplayListSet& aLists)
{
  nsFormControlFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  
  
  if ((!IsChecked() && !IsIndeterminate()) || !IsVisibleForPainting(aBuilder))
    return NS_OK;   
    
  if (IsThemed())
    return NS_OK; 

  aLists.Content()->AppendNewToTop(new (aBuilder)
    nsDisplayGeneric(aBuilder, this,
                     IsIndeterminate()
                     ? PaintIndeterminateMark : PaintCheckMark,
                     "CheckedCheckbox",
                     nsDisplayItem::TYPE_CHECKED_CHECKBOX));
  return NS_OK;
}


bool
nsGfxCheckboxControlFrame::IsChecked()
{
  nsCOMPtr<nsIDOMHTMLInputElement> elem(do_QueryInterface(mContent));
  bool retval = false;
  elem->GetChecked(&retval);
  return retval;
}

bool
nsGfxCheckboxControlFrame::IsIndeterminate()
{
  nsCOMPtr<nsIDOMHTMLInputElement> elem(do_QueryInterface(mContent));
  bool retval = false;
  elem->GetIndeterminate(&retval);
  return retval;
}
