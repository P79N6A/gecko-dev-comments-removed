




































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

nsIFrame*
NS_NewGfxRadioControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsGfxRadioControlFrame(aContext);
}

nsGfxRadioControlFrame::nsGfxRadioControlFrame(nsStyleContext* aContext):
  nsFormControlFrame(aContext)
{
}

nsGfxRadioControlFrame::~nsGfxRadioControlFrame()
{
}


NS_IMETHODIMP
nsGfxRadioControlFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(aInstancePtr, "null out param");

  if (aIID.Equals(NS_GET_IID(nsIRadioControlFrame))) {
    *aInstancePtr = static_cast<nsIRadioControlFrame*>(this);
    return NS_OK;
  }

  return nsFormControlFrame::QueryInterface(aIID, aInstancePtr);
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsGfxRadioControlFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    return accService->CreateHTMLRadioButtonAccessible(static_cast<nsIFrame*>(this), aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif


nsStyleContext*
nsGfxRadioControlFrame::GetAdditionalStyleContext(PRInt32 aIndex) const
{
  switch (aIndex) {
  case NS_GFX_RADIO_CONTROL_FRAME_FACE_CONTEXT_INDEX:
    return mRadioButtonFaceStyle;
    break;
  default:
    return nsnull;
  }
}


void
nsGfxRadioControlFrame::SetAdditionalStyleContext(PRInt32 aIndex, 
                                                  nsStyleContext* aStyleContext)
{
  switch (aIndex) {
  case NS_GFX_RADIO_CONTROL_FRAME_FACE_CONTEXT_INDEX:
    mRadioButtonFaceStyle = aStyleContext;
    break;
  }
}


NS_IMETHODIMP
nsGfxRadioControlFrame::SetRadioButtonFaceStyleContext(nsStyleContext *aRadioButtonFaceStyleContext)
{
  mRadioButtonFaceStyle = aRadioButtonFaceStyleContext;
  return NS_OK;
}


void
nsGfxRadioControlFrame::PaintRadioButtonFromStyle(
    nsIRenderingContext& aRenderingContext, nsPoint aPt, const nsRect& aDirtyRect)
{
  const nsStyleBorder* myBorder = mRadioButtonFaceStyle->GetStyleBorder();
  
  const nsStyleBackground* myColor = mRadioButtonFaceStyle->GetStyleBackground();
  const nsStyleColor* color = mRadioButtonFaceStyle->GetStyleColor();
  const nsStylePadding* myPadding = mRadioButtonFaceStyle->GetStylePadding();
  const nsStylePosition* myPosition = mRadioButtonFaceStyle->GetStylePosition();

  NS_ASSERTION(myPosition->mWidth.GetUnit() == eStyleUnit_Coord &&
               myPosition->mHeight.GetUnit() == eStyleUnit_Coord,
               "styles for :-moz-radio are incorrect or author-accessible");
  nscoord width = myPosition->mWidth.GetCoordValue();
  nscoord height = myPosition->mHeight.GetCoordValue();
  
  nscoord x = (mRect.width - width) / 2;
  nscoord y = (mRect.height - height) / 2;
  nsRect rect = nsRect(x, y, width, height) + aPt;

  
  
  
  
  
  
  nsStyleBackground tmpColor     = *myColor;
  tmpColor.mBackgroundColor = color->mColor;
  nsPresContext* pc = PresContext();
  nsCSSRendering::PaintBackgroundWithSC(pc, aRenderingContext,
                                        this, aDirtyRect, rect,
                                        tmpColor, *myBorder, *myPadding, PR_FALSE);
  nsCSSRendering::PaintBorder(pc, aRenderingContext, this,
                              aDirtyRect, rect, *myBorder, mRadioButtonFaceStyle, 0);
}

class nsDisplayRadioButtonFromStyle : public nsDisplayItem {
public:
  nsDisplayRadioButtonFromStyle(nsGfxRadioControlFrame* aFrame)
    : nsDisplayItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayRadioButtonFromStyle);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayRadioButtonFromStyle() {
    MOZ_COUNT_DTOR(nsDisplayRadioButtonFromStyle);
  }
#endif
  
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("RadioButton")
};

void
nsDisplayRadioButtonFromStyle::Paint(nsDisplayListBuilder* aBuilder,
     nsIRenderingContext* aCtx, const nsRect& aDirtyRect) {
  static_cast<nsGfxRadioControlFrame*>(mFrame)->
    PaintRadioButtonFromStyle(*aCtx, aBuilder->ToReferenceFrame(mFrame), aDirtyRect);
}


NS_IMETHODIMP
nsGfxRadioControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists)
{
  nsresult rv = nsFormControlFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (!IsVisibleForPainting(aBuilder))
    return NS_OK;
  
  if (IsThemed())
    return NS_OK; 

  if (!mRadioButtonFaceStyle)
    return NS_OK;
  
  PRBool checked = PR_TRUE;
  GetCurrentCheckState(&checked); 
  if (!checked)
    return NS_OK;
    
  return aLists.Content()->AppendNewToTop(new (aBuilder)
      nsDisplayRadioButtonFromStyle(this));
}



NS_IMETHODIMP
nsGfxRadioControlFrame::OnChecked(nsPresContext* aPresContext,
                                  PRBool aChecked)
{
  Invalidate(GetOverflowRect(), PR_FALSE);
  return NS_OK;
}

