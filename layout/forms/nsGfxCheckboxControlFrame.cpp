




































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


nsIFrame*
NS_NewGfxCheckboxControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
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






NS_IMETHODIMP
nsGfxCheckboxControlFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(aInstancePtr, "null out param");

  if (aIID.Equals(NS_GET_IID(nsICheckboxControlFrame))) {
    *aInstancePtr = static_cast<nsICheckboxControlFrame*>(this);
    return NS_OK;
  }

  return nsFormControlFrame::QueryInterface(aIID, aInstancePtr);
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsGfxCheckboxControlFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    return accService->CreateHTMLCheckboxAccessible(static_cast<nsIFrame*>(this), aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif


NS_IMETHODIMP
nsGfxCheckboxControlFrame::SetCheckboxFaceStyleContext(nsStyleContext *aCheckboxFaceStyleContext)
{
  mCheckButtonFaceStyle = aCheckboxFaceStyleContext;
  return NS_OK;
}



nsStyleContext*
nsGfxCheckboxControlFrame::GetAdditionalStyleContext(PRInt32 aIndex) const
{
  switch (aIndex) {
  case NS_GFX_CHECKBOX_CONTROL_FRAME_FACE_CONTEXT_INDEX:
    return mCheckButtonFaceStyle;
    break;
  default:
    return nsnull;
  }
}




void
nsGfxCheckboxControlFrame::SetAdditionalStyleContext(PRInt32 aIndex, 
                                                     nsStyleContext* aStyleContext)
{
  switch (aIndex) {
  case NS_GFX_CHECKBOX_CONTROL_FRAME_FACE_CONTEXT_INDEX:
    mCheckButtonFaceStyle = aStyleContext;
    break;
  }
}



NS_IMETHODIMP
nsGfxCheckboxControlFrame::OnChecked(nsPresContext* aPresContext,
                                     PRBool aChecked)
{
  Invalidate(GetOverflowRect(), PR_FALSE);
  return NS_OK;
}

static void PaintCheckMarkFromStyle(nsIFrame* aFrame,
     nsIRenderingContext* aCtx, const nsRect& aDirtyRect, nsPoint aPt) {
  static_cast<nsGfxCheckboxControlFrame*>(aFrame)
    ->PaintCheckBoxFromStyle(*aCtx, aPt, aDirtyRect);
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

  PaintCheckMark(aRenderingContext, checkRect);
}


NS_IMETHODIMP
nsGfxCheckboxControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                            const nsRect&           aDirtyRect,
                                            const nsDisplayListSet& aLists)
{
  nsresult rv = nsFormControlFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  if (!GetCheckboxState() || !IsVisibleForPainting(aBuilder))
    return NS_OK;   
    
  if (IsThemed())
    return NS_OK; 

  
  if (mCheckButtonFaceStyle) {
    
    
    
    const nsStyleBackground* myBackground = mCheckButtonFaceStyle->GetStyleBackground();
    if (!myBackground->IsTransparent())
      return aLists.Content()->AppendNewToTop(new (aBuilder)
          nsDisplayGeneric(this, PaintCheckMarkFromStyle, "CheckMarkFromStyle"));
  }

  return aLists.Content()->AppendNewToTop(new (aBuilder) nsDisplayCheckMark(this));
}

void
nsGfxCheckboxControlFrame::PaintCheckBoxFromStyle(
    nsIRenderingContext& aRenderingContext, nsPoint aPt, const nsRect& aDirtyRect) {
  const nsStylePadding* myPadding = mCheckButtonFaceStyle->GetStylePadding();
  const nsStylePosition* myPosition = mCheckButtonFaceStyle->GetStylePosition();
  const nsStyleBorder* myBorder = mCheckButtonFaceStyle->GetStyleBorder();
  const nsStyleBackground* myBackground = mCheckButtonFaceStyle->GetStyleBackground();

  NS_ASSERTION(myPosition->mWidth.GetUnit() == eStyleUnit_Coord &&
               myPosition->mHeight.GetUnit() == eStyleUnit_Coord,
               "styles for :-moz-checkbox are incorrect or author-accessible");
  nscoord width = myPosition->mWidth.GetCoordValue();
  nscoord height = myPosition->mHeight.GetCoordValue();
  
  nscoord x = (mRect.width - width) / 2;
  nscoord y = (mRect.height - height) / 2;
  nsRect rect(aPt.x + x, aPt.y + y, width, height);

  nsCSSRendering::PaintBackgroundWithSC(PresContext(), aRenderingContext,
                                        this, aDirtyRect, rect, *myBackground,
                                        *myBorder, *myPadding, PR_FALSE);
  nsCSSRendering::PaintBorder(PresContext(), aRenderingContext, this,
                              aDirtyRect, rect, *myBorder, mCheckButtonFaceStyle, 0);
}


PRBool
nsGfxCheckboxControlFrame::GetCheckboxState ( )
{
  nsCOMPtr<nsIDOMHTMLInputElement> elem(do_QueryInterface(mContent));
  PRBool retval = PR_FALSE;
  elem->GetChecked(&retval);
  return retval;
}
