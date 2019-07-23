




































#include "nsTableFrame.h"
#include "nsTableColFrame.h"
#include "nsTableCellFrame.h"
#include "nsTableFrame.h"
#include "nsTableRowGroupFrame.h"
#include "nsTablePainter.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIRenderingContext.h"
#include "nsCSSRendering.h"
#include "nsIContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLParts.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsCOMPtr.h"
#include "nsIDOMHTMLTableCellElement.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsINameSpaceManager.h"
#include "nsDisplayList.h"
#include "nsLayoutUtils.h"


#include "nsFrameSelection.h"
#include "nsILookAndFeel.h"


nsTableCellFrame::nsTableCellFrame(nsStyleContext* aContext) :
  nsHTMLContainerFrame(aContext)
{
  mColIndex = 0;
  mPriorAvailWidth = 0;

  SetContentEmpty(PR_FALSE);
  SetHasPctOverHeight(PR_FALSE);
}

nsTableCellFrame::~nsTableCellFrame()
{
}

nsTableCellFrame*  
nsTableCellFrame::GetNextCell() const
{
  nsIFrame* childFrame = GetNextSibling();
  while (childFrame) {
    if (IS_TABLE_CELL(childFrame->GetType())) {
      return (nsTableCellFrame*)childFrame;
    }
    childFrame = childFrame->GetNextSibling();
  }
  return nsnull;
}

NS_IMETHODIMP
nsTableCellFrame::Init(nsIContent*      aContent,
                       nsIFrame*        aParent,
                       nsIFrame*        aPrevInFlow)
{
  
  nsresult rv = nsHTMLContainerFrame::Init(aContent, aParent, aPrevInFlow);

  if (aPrevInFlow) {
    
    nsTableCellFrame* cellFrame = (nsTableCellFrame*)aPrevInFlow;
    PRInt32           colIndex;
    cellFrame->GetColIndex(colIndex);
    SetColIndex(colIndex);
  }

  return rv;
}



void
nsTableCellFrame::NotifyPercentHeight(const nsHTMLReflowState& aReflowState)
{
  
  
  
  
  
  

  
  const nsHTMLReflowState *cellRS = aReflowState.mCBReflowState;

  if (cellRS && cellRS->frame == this &&
      (cellRS->ComputedHeight() == NS_UNCONSTRAINEDSIZE ||
       cellRS->ComputedHeight() == 0)) { 
    
    
    

    
    
    

    if (nsTableFrame::AncestorsHaveStyleHeight(*cellRS) ||
        (nsTableFrame::GetTableFrame(this)->GetEffectiveRowSpan(*this) == 1 &&
         (cellRS->parentReflowState->frame->GetStateBits() &
          NS_ROW_HAS_CELL_WITH_STYLE_HEIGHT))) {

      for (const nsHTMLReflowState *rs = aReflowState.parentReflowState;
           rs != cellRS;
           rs = rs->parentReflowState) {
        rs->frame->AddStateBits(NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
      }
      
      nsTableFrame::RequestSpecialHeightReflow(*cellRS);
    }
  }
}


PRBool 
nsTableCellFrame::NeedsToObserve(const nsHTMLReflowState& aReflowState)
{
  const nsHTMLReflowState *rs = aReflowState.parentReflowState;
  if (!rs)
    return PR_FALSE;
  if (rs->frame == this) {
    
    
    
    return PR_TRUE;
  }
  rs = rs->parentReflowState;
  if (!rs) {
    return PR_FALSE;
  }

  
  
  nsIAtom *fType = aReflowState.frame->GetType();
  if (fType == nsGkAtoms::tableFrame) {
    return PR_TRUE;
  }

  
  
  
  return rs->frame == this &&
         (PresContext()->CompatibilityMode() == eCompatibility_NavQuirks ||
          fType == nsGkAtoms::tableOuterFrame);
}

nsresult 
nsTableCellFrame::GetRowIndex(PRInt32 &aRowIndex) const
{
  nsresult result;
  nsTableRowFrame* row = static_cast<nsTableRowFrame*>(GetParent());
  if (row) {
    aRowIndex = row->GetRowIndex();
    result = NS_OK;
  }
  else {
    aRowIndex = 0;
    result = NS_ERROR_NOT_INITIALIZED;
  }
  return result;
}

nsresult 
nsTableCellFrame::GetColIndex(PRInt32 &aColIndex) const
{  
  if (GetPrevInFlow()) {
    return ((nsTableCellFrame*)GetFirstInFlow())->GetColIndex(aColIndex);
  }
  else {
    aColIndex = mColIndex;
    return  NS_OK;
  }
}

NS_IMETHODIMP
nsTableCellFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                   nsIAtom*        aAttribute,
                                   PRInt32         aModType)
{
  
  
  if (aNameSpaceID == kNameSpaceID_None && aAttribute == nsGkAtoms::nowrap &&
      PresContext()->CompatibilityMode() == eCompatibility_NavQuirks) {
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange, NS_FRAME_IS_DIRTY);
  }
  
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (tableFrame) {
    tableFrame->AttributeChangedFor(this, mContent, aAttribute); 
  }
  return NS_OK;
}

NS_IMETHODIMP
nsTableCellFrame::AppendFrames(nsIAtom*        aListName,
                               nsIFrame*       aFrameList)
{
  NS_PRECONDITION(PR_FALSE, "unsupported operation");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsTableCellFrame::InsertFrames(nsIAtom*        aListName,
                               nsIFrame*       aPrevFrame,
                               nsIFrame*       aFrameList)
{
  NS_PRECONDITION(PR_FALSE, "unsupported operation");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsTableCellFrame::RemoveFrame(nsIAtom*        aListName,
                              nsIFrame*       aOldFrame)
{
  NS_PRECONDITION(PR_FALSE, "unsupported operation");
  return NS_ERROR_NOT_IMPLEMENTED;
}

void nsTableCellFrame::SetColIndex(PRInt32 aColIndex)
{  
  mColIndex = aColIndex;
}

 nsMargin
nsTableCellFrame::GetUsedMargin() const
{
  return nsMargin(0,0,0,0);
}


inline nscolor EnsureDifferentColors(nscolor colorA, nscolor colorB)
{
    if (colorA == colorB)
    {
      nscolor res;
      res = NS_RGB(NS_GET_R(colorA) ^ 0xff,
                   NS_GET_G(colorA) ^ 0xff,
                   NS_GET_B(colorA) ^ 0xff);
      return res;
    }
    return colorA;
}

void
nsTableCellFrame::DecorateForSelection(nsIRenderingContext& aRenderingContext,
                                       nsPoint aPt)
{
  NS_ASSERTION(GetStateBits() & NS_FRAME_SELECTED_CONTENT,
               "Should only be called for selected cells");
  PRInt16 displaySelection;
  nsPresContext* presContext = PresContext();
  displaySelection = DisplaySelection(presContext);
  if (displaySelection) {
    nsCOMPtr<nsFrameSelection> frameSelection =
      presContext->PresShell()->FrameSelection();

    if (frameSelection->GetTableCellSelection()) {
      nscolor       bordercolor;
      if (displaySelection == nsISelectionController::SELECTION_DISABLED) {
        bordercolor = NS_RGB(176,176,176);
      }
      else {
        presContext->LookAndFeel()->
          GetColor(nsILookAndFeel::eColor_TextSelectBackground,
                   bordercolor);
      }
      nscoord threePx = nsPresContext::CSSPixelsToAppUnits(3);
      if ((mRect.width > threePx) && (mRect.height > threePx))
      {
        
        bordercolor = EnsureDifferentColors(bordercolor,
                                            GetStyleBackground()->mBackgroundColor);
        nsIRenderingContext::AutoPushTranslation
            translate(&aRenderingContext, aPt.x, aPt.y);
        nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);

        aRenderingContext.SetColor(bordercolor);
        aRenderingContext.DrawLine(onePixel, 0, mRect.width, 0);
        aRenderingContext.DrawLine(0, onePixel, 0, mRect.height);
        aRenderingContext.DrawLine(onePixel, mRect.height, mRect.width, mRect.height);
        aRenderingContext.DrawLine(mRect.width, onePixel, mRect.width, mRect.height);
        
        aRenderingContext.DrawRect(onePixel, onePixel, mRect.width-onePixel,
                                   mRect.height-onePixel);
        
        aRenderingContext.DrawLine(2*onePixel, mRect.height-2*onePixel,
                                   mRect.width-onePixel, mRect.height- (2*onePixel));
        aRenderingContext.DrawLine(mRect.width - (2*onePixel), 2*onePixel,
                                   mRect.width - (2*onePixel), mRect.height-onePixel);
      }
    }
  }
}

void
nsTableCellFrame::PaintBackground(nsIRenderingContext& aRenderingContext,
                                  const nsRect&        aDirtyRect,
                                  nsPoint              aPt)
{
  nsRect rect(aPt, GetSize());
  nsCSSRendering::PaintBackground(PresContext(), aRenderingContext, this,
                                  aDirtyRect, rect, *GetStyleBorder(),
                                  *GetStylePadding(), PR_TRUE);
}


void
nsTableCellFrame::PaintCellBackground(nsIRenderingContext& aRenderingContext,
                                      const nsRect& aDirtyRect, nsPoint aPt)
{
  if (!GetStyleVisibility()->IsVisible())
    return;
  if (GetContentEmpty() &&
      NS_STYLE_TABLE_EMPTY_CELLS_HIDE == GetStyleTableBorder()->mEmptyCells)
    return;

  PaintBackground(aRenderingContext, aDirtyRect, aPt);
}

class nsDisplayTableCellBackground : public nsDisplayTableItem {
public:
  nsDisplayTableCellBackground(nsTableCellFrame* aFrame) : nsDisplayTableItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayTableCellBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTableCellBackground() {
    MOZ_COUNT_DTOR(nsDisplayTableCellBackground);
  }
#endif

  virtual nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt,
                            HitTestState* aState) { return mFrame; }
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder);

  NS_DISPLAY_DECL_NAME("TableCellBackground")
};

void nsDisplayTableCellBackground::Paint(nsDisplayListBuilder* aBuilder,
     nsIRenderingContext* aCtx, const nsRect& aDirtyRect)
{
  static_cast<nsTableCellFrame*>(mFrame)->
    PaintBackground(*aCtx, aDirtyRect, aBuilder->ToReferenceFrame(mFrame));
}

nsRect
nsDisplayTableCellBackground::GetBounds(nsDisplayListBuilder* aBuilder)
{
  
  
  return nsDisplayItem::GetBounds(aBuilder);
}

static void
PaintTableCellSelection(nsIFrame* aFrame, nsIRenderingContext* aCtx,
                        const nsRect& aRect, nsPoint aPt)
{
  static_cast<nsTableCellFrame*>(aFrame)->DecorateForSelection(*aCtx, aPt);
}

NS_IMETHODIMP
nsTableCellFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                   const nsRect&           aDirtyRect,
                                   const nsDisplayListSet& aLists)
{
  if (!IsVisibleInSelection(aBuilder))
    return NS_OK;

  DO_GLOBAL_REFLOW_COUNT_DSP("nsTableCellFrame");

  PRInt32 emptyCellStyle = GetContentEmpty() ? GetStyleTableBorder()->mEmptyCells
      : NS_STYLE_TABLE_EMPTY_CELLS_SHOW;
  
  if (GetStyleVisibility()->IsVisible() &&
      (NS_STYLE_TABLE_EMPTY_CELLS_HIDE != emptyCellStyle)) {
    nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);

    PRBool isRoot = aBuilder->IsAtRootOfPseudoStackingContext();
    if (!isRoot) {
      nsDisplayTableItem* currentItem = aBuilder->GetCurrentTableItem();
      NS_ASSERTION(currentItem, "No current table item???");
      currentItem->UpdateForFrameBackground(this);
    }

    
    if (aBuilder->IsForEventDelivery() ||
        (((!tableFrame->IsBorderCollapse() || isRoot) &&
        (!GetStyleBackground()->IsTransparent() || GetStyleDisplay()->mAppearance)))) {
      
      
      
      nsDisplayTableItem* item = new (aBuilder) nsDisplayTableCellBackground(this);
      nsresult rv = aLists.BorderBackground()->AppendNewToTop(item);
      NS_ENSURE_SUCCESS(rv, rv);
      item->UpdateForFrameBackground(this);
    }
    
    
    if (!tableFrame->IsBorderCollapse() && HasBorder() &&
        emptyCellStyle == NS_STYLE_TABLE_EMPTY_CELLS_SHOW) {
      nsresult rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
          nsDisplayBorder(this));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    PRBool isSelected =
      (GetStateBits() & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT;
    if (isSelected) {
      nsresult rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
          nsDisplayGeneric(this, ::PaintTableCellSelection, "TableCellSelection"));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  nsresult rv = DisplayOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool quirkyClip = HasPctOverHeight() &&
    eCompatibility_NavQuirks == PresContext()->CompatibilityMode();
  nsIFrame* kid = mFrames.FirstChild();
  NS_ASSERTION(kid && !kid->GetNextSibling(), "Table cells should have just one child");
  if (!quirkyClip) {
    
    
    
    
    
    
    return BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
  }
    
  
  nsDisplayListCollection set;
  rv = BuildDisplayListForChild(aBuilder, kid, aDirtyRect, set);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRect clip = GetOverflowRect();
  if (quirkyClip) {
    clip = nsRect(nsPoint(0, 0), GetSize());
  }
  return OverflowClip(aBuilder, set, aLists, clip + aBuilder->ToReferenceFrame(this));
}


NS_IMETHODIMP
nsTableCellFrame::SetSelected(nsPresContext* aPresContext,
                              nsIDOMRange*    aRange,
                              PRBool          aSelected,
                              nsSpread        aSpread)
{
  
#if 0
  if ((aSpread == eSpreadDown)){
    nsIFrame* kid = GetFirstChild(nsnull);
    while (nsnull != kid) {
      kid->SetSelected(nsnull, aSelected, eSpreadDown);
      kid = kid->GetNextSibling();
    }
  }
  
#endif
  
  
  
  nsFrame::SetSelected(aPresContext, aRange, aSelected, aSpread);

  nsCOMPtr<nsFrameSelection> frameSelection =
    aPresContext->PresShell()->FrameSelection();
  if (frameSelection->GetTableCellSelection()) {
    
    Invalidate(GetOverflowRect(), PR_FALSE);
  }
  return NS_OK;
}

PRIntn
nsTableCellFrame::GetSkipSides() const
{
  PRIntn skip = 0;
  if (nsnull != GetPrevInFlow()) {
    skip |= 1 << NS_SIDE_TOP;
  }
  if (nsnull != GetNextInFlow()) {
    skip |= 1 << NS_SIDE_BOTTOM;
  }
  return skip;
}

PRBool nsTableCellFrame::ParentDisablesSelection() const 
{
  PRBool returnval;
  if (NS_FAILED(GetSelected(&returnval)))
    return PR_FALSE;
  if (returnval)
    return PR_TRUE;
  return nsFrame::ParentDisablesSelection();
}

 void
nsTableCellFrame::GetSelfOverflow(nsRect& aOverflowArea)
{
  aOverflowArea = nsRect(nsPoint(0,0), GetSize());
}



void nsTableCellFrame::VerticallyAlignChild(nscoord aMaxAscent)
{
  const nsStyleTextReset* textStyle = GetStyleTextReset();
  
  nsMargin borderPadding = GetUsedBorderAndPadding();

  nscoord topInset = borderPadding.top;
  nscoord bottomInset = borderPadding.bottom;

  
  
  
  
  
  PRUint8 verticalAlignFlags = NS_STYLE_VERTICAL_ALIGN_BASELINE;
  if (textStyle->mVerticalAlign.GetUnit() == eStyleUnit_Enumerated) {
    verticalAlignFlags = textStyle->mVerticalAlign.GetIntValue();
    if (verticalAlignFlags != NS_STYLE_VERTICAL_ALIGN_TOP &&
        verticalAlignFlags != NS_STYLE_VERTICAL_ALIGN_MIDDLE &&
        verticalAlignFlags != NS_STYLE_VERTICAL_ALIGN_BOTTOM)
    {
      verticalAlignFlags = NS_STYLE_VERTICAL_ALIGN_BASELINE;
    }
  }

  nscoord height = mRect.height;
  nsIFrame* firstKid = mFrames.FirstChild();
  NS_ASSERTION(firstKid, "Frame construction error, a table cell always has an inner cell frame");
  nsRect kidRect = firstKid->GetRect();
  nscoord childHeight = kidRect.height;

  
  nscoord kidYTop = 0;
  switch (verticalAlignFlags) 
  {
    case NS_STYLE_VERTICAL_ALIGN_BASELINE:
      
      
      kidYTop = topInset + aMaxAscent - GetCellBaseline();
    break;

    case NS_STYLE_VERTICAL_ALIGN_TOP:
      
      kidYTop = topInset;
    break;

    case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
      
      kidYTop = height - childHeight - bottomInset;
    break;

    default:
    case NS_STYLE_VERTICAL_ALIGN_MIDDLE:
      
      kidYTop = (height - childHeight - bottomInset + topInset) / 2;
  }
  
  kidYTop = PR_MAX(0, kidYTop);

  if (kidYTop != kidRect.y) {
    
    firstKid->InvalidateOverflowRect();
  }
  
  firstKid->SetPosition(nsPoint(kidRect.x, kidYTop));
  nsHTMLReflowMetrics desiredSize;
  desiredSize.width = mRect.width;
  desiredSize.height = mRect.height;
  GetSelfOverflow(desiredSize.mOverflowArea);
  ConsiderChildOverflow(desiredSize.mOverflowArea, firstKid);
  FinishAndStoreOverflow(&desiredSize);
  if (kidYTop != kidRect.y) {
    
    
    nsContainerFrame::PositionChildViews(firstKid);

    
    firstKid->InvalidateOverflowRect();
  }
  if (HasView()) {
    nsContainerFrame::SyncFrameViewAfterReflow(PresContext(), this,
                                               GetView(),
                                               &desiredSize.mOverflowArea, 0);
  }
}






PRBool
nsTableCellFrame::HasVerticalAlignBaseline()
{
  const nsStyleTextReset* textStyle = GetStyleTextReset();
  if (textStyle->mVerticalAlign.GetUnit() == eStyleUnit_Enumerated) {
    PRUint8 verticalAlignFlags = textStyle->mVerticalAlign.GetIntValue();
    if (verticalAlignFlags == NS_STYLE_VERTICAL_ALIGN_TOP ||
        verticalAlignFlags == NS_STYLE_VERTICAL_ALIGN_MIDDLE ||
        verticalAlignFlags == NS_STYLE_VERTICAL_ALIGN_BOTTOM)
    {
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}

nscoord
nsTableCellFrame::GetCellBaseline() const
{
  
  
  nsIFrame *inner = mFrames.FirstChild();
  nscoord borderPadding = GetUsedBorderAndPadding().top;
  nscoord result;
  if (nsLayoutUtils::GetFirstLineBaseline(inner, &result))
    return result + borderPadding;
  return inner->GetContentRect().YMost() - inner->GetPosition().y +
         borderPadding;
}

PRInt32 nsTableCellFrame::GetRowSpan()
{  
  PRInt32 rowSpan=1;
  nsGenericHTMLElement *hc = nsGenericHTMLElement::FromContent(mContent);

  
  if (hc && !GetStyleContext()->GetPseudoType()) {
    const nsAttrValue* attr = hc->GetParsedAttr(nsGkAtoms::rowspan);
    
    
    if (attr && attr->Type() == nsAttrValue::eInteger) { 
       rowSpan = attr->GetIntegerValue(); 
    }
  }
  return rowSpan;
}

PRInt32 nsTableCellFrame::GetColSpan()
{  
  PRInt32 colSpan=1;
  nsGenericHTMLElement *hc = nsGenericHTMLElement::FromContent(mContent);

  
  if (hc && !GetStyleContext()->GetPseudoType()) {
    const nsAttrValue* attr = hc->GetParsedAttr(nsGkAtoms::colspan); 
    
    
    if (attr && attr->Type() == nsAttrValue::eInteger) { 
       colSpan = attr->GetIntegerValue(); 
    }
  }
  return colSpan;
}

 nscoord
nsTableCellFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_MIN_WIDTH(this, result);

  nsIFrame *inner = mFrames.FirstChild();
  result = nsLayoutUtils::IntrinsicForContainer(aRenderingContext, inner,
                                                    nsLayoutUtils::MIN_WIDTH);
  return result;
}

 nscoord
nsTableCellFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_PREF_WIDTH(this, result);

  nsIFrame *inner = mFrames.FirstChild();
  result = nsLayoutUtils::IntrinsicForContainer(aRenderingContext, inner,
                                                nsLayoutUtils::PREF_WIDTH);
  return result;
}

 nsIFrame::IntrinsicWidthOffsetData
nsTableCellFrame::IntrinsicWidthOffsets(nsIRenderingContext* aRenderingContext)
{
  IntrinsicWidthOffsetData result =
    nsHTMLContainerFrame::IntrinsicWidthOffsets(aRenderingContext);

  result.hMargin = 0;
  result.hPctMargin = 0;

  nsMargin border;
  GetBorderWidth(border);
  result.hBorder = border.LeftRight();

  return result;
}

#ifdef DEBUG
#define PROBABLY_TOO_LARGE 1000000
static
void DebugCheckChildSize(nsIFrame*            aChild, 
                         nsHTMLReflowMetrics& aMet, 
                         nsSize&              aAvailSize)
{
  if ((aMet.width < 0) || (aMet.width > PROBABLY_TOO_LARGE)) {
    printf("WARNING: cell content %p has large width %d \n", aChild, aMet.width);
  }
}
#endif




static nscoord
CalcUnpaginagedHeight(nsPresContext*       aPresContext,
                      nsTableCellFrame&     aCellFrame, 
                      nsTableFrame&         aTableFrame,
                      nscoord               aVerticalBorderPadding)
{
  const nsTableCellFrame* firstCellInFlow   = (nsTableCellFrame*)aCellFrame.GetFirstInFlow();
  nsTableFrame*           firstTableInFlow  = (nsTableFrame*)aTableFrame.GetFirstInFlow();
  nsTableRowFrame*        row
    = static_cast<nsTableRowFrame*>(firstCellInFlow->GetParent());
  nsTableRowGroupFrame*   firstRGInFlow
    = static_cast<nsTableRowGroupFrame*>(row->GetParent());

  PRInt32 rowIndex;
  firstCellInFlow->GetRowIndex(rowIndex);
  PRInt32 rowSpan = aTableFrame.GetEffectiveRowSpan(*firstCellInFlow);
  nscoord cellSpacing = firstTableInFlow->GetCellSpacingX();

  nscoord computedHeight = ((rowSpan - 1) * cellSpacing) - aVerticalBorderPadding;
  PRInt32 rowX;
  for (row = firstRGInFlow->GetFirstRow(), rowX = 0; row; row = row->GetNextRow(), rowX++) {
    if (rowX > rowIndex + rowSpan - 1) {
      break;
    }
    else if (rowX >= rowIndex) {
      computedHeight += row->GetUnpaginatedHeight(aPresContext);
    }
  }
  return computedHeight;
}

NS_METHOD nsTableCellFrame::Reflow(nsPresContext*          aPresContext,
                                   nsHTMLReflowMetrics&     aDesiredSize,
                                   const nsHTMLReflowState& aReflowState,
                                   nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsTableCellFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  if (aReflowState.mFlags.mSpecialHeightReflow) {
    GetFirstInFlow()->AddStateBits(NS_TABLE_CELL_HAD_SPECIAL_REFLOW);
  }

  
  nscoord availHeight = aReflowState.availableHeight;

  
  nsTableFrame::CheckRequestSpecialHeightReflow(aReflowState);

  aStatus = NS_FRAME_COMPLETE;
  nsSize availSize(aReflowState.availableWidth, availHeight);

  
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (!tableFrame)
    ABORT1(NS_ERROR_NULL_POINTER);

  nsMargin borderPadding = aReflowState.mComputedPadding;
  nsMargin border;
  GetBorderWidth(border);
  borderPadding += border;
  
  nscoord topInset    = borderPadding.top;
  nscoord rightInset  = borderPadding.right;
  nscoord bottomInset = borderPadding.bottom;
  nscoord leftInset   = borderPadding.left;

  
  availSize.width -= leftInset+rightInset;
  if (NS_UNCONSTRAINEDSIZE!=availSize.height)
    availSize.height -= topInset+bottomInset;

  
  
  if (availSize.height < 0)
    availSize.height = 1;

  nsHTMLReflowMetrics kidSize(aDesiredSize.mFlags);
  kidSize.width = kidSize.height = 0;
  SetPriorAvailWidth(aReflowState.availableWidth);
  nsIFrame* firstKid = mFrames.FirstChild();
  NS_ASSERTION(firstKid, "Frame construction error, a table cell always has an inner cell frame");

  nscoord computedPaginatedHeight = 0;

  if (aReflowState.mFlags.mSpecialHeightReflow) {
    const_cast<nsHTMLReflowState&>(aReflowState).SetComputedHeight(mRect.height - topInset - bottomInset);
    DISPLAY_REFLOW_CHANGE();
  }
  else if (aPresContext->IsPaginated()) {
    computedPaginatedHeight = CalcUnpaginagedHeight(aPresContext, (nsTableCellFrame&)*this, *tableFrame, topInset + bottomInset);
    if (computedPaginatedHeight > 0) {
      const_cast<nsHTMLReflowState&>(aReflowState).SetComputedHeight(computedPaginatedHeight);
      DISPLAY_REFLOW_CHANGE();
    }
  }      
  else {
    SetHasPctOverHeight(PR_FALSE);
  }

  nsHTMLReflowState kidReflowState(aPresContext, aReflowState, firstKid,
                                   availSize);

  
  
  
  if (!aReflowState.mFlags.mSpecialHeightReflow) {
    
    
    
    kidReflowState.mPercentHeightObserver = this;
  }
  
  kidReflowState.mFlags.mSpecialHeightReflow = PR_FALSE;
  
  if (aReflowState.mFlags.mSpecialHeightReflow ||
      (GetFirstInFlow()->GetStateBits() & NS_TABLE_CELL_HAD_SPECIAL_REFLOW)) {
    
    
    
    kidReflowState.mFlags.mVResize = PR_TRUE;
  }

  nsPoint kidOrigin(leftInset, topInset);
  nsRect origRect = firstKid->GetRect();
  nsRect origOverflowRect = firstKid->GetOverflowRect();
  PRBool firstReflow = (firstKid->GetStateBits() & NS_FRAME_FIRST_REFLOW) != 0;

  ReflowChild(firstKid, aPresContext, kidSize, kidReflowState,
              kidOrigin.x, kidOrigin.y, NS_FRAME_INVALIDATE_ON_MOVE, aStatus);
  if (NS_FRAME_OVERFLOW_IS_INCOMPLETE(aStatus)) {
    
    
    NS_FRAME_SET_INCOMPLETE(aStatus);
    printf("Set table cell incomplete %p\n", this);
  }

  
  if (GetStateBits() & NS_FRAME_IS_DIRTY) {
    Invalidate(GetOverflowRect(), PR_FALSE);
  }

#ifdef NS_DEBUG
  DebugCheckChildSize(firstKid, kidSize, availSize);
#endif

  
  
  nsIFrame* prevInFlow = GetPrevInFlow();
  PRBool isEmpty;
  if (prevInFlow) {
    isEmpty = static_cast<nsTableCellFrame*>(prevInFlow)->GetContentEmpty();
  } else {
    
    
    
    
    
    isEmpty = kidSize.height == 0;
  }
  SetContentEmpty(isEmpty);

  
  FinishReflowChild(firstKid, aPresContext, &kidReflowState, kidSize,
                    kidOrigin.x, kidOrigin.y, 0);

  nsTableFrame::InvalidateFrame(firstKid, origRect, origOverflowRect,
                                firstReflow);
    
  
  nscoord cellHeight = kidSize.height;

  if (NS_UNCONSTRAINEDSIZE != cellHeight) {
    cellHeight += topInset + bottomInset;
  }

  
  nscoord cellWidth = kidSize.width;      

  
  if (NS_UNCONSTRAINEDSIZE != cellWidth) {
    cellWidth += leftInset + rightInset;    
  }

  
  aDesiredSize.width   = cellWidth;
  aDesiredSize.height  = cellHeight;

  

  if (aReflowState.mFlags.mSpecialHeightReflow) {
    if (aDesiredSize.height > mRect.height) {
      
      
      SetHasPctOverHeight(PR_TRUE);
    }
    if (NS_UNCONSTRAINEDSIZE == aReflowState.availableHeight) {
      aDesiredSize.height = mRect.height;
    }
  }

  
  
  if (!(GetParent()->GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    CheckInvalidateSizeChange(aDesiredSize);
  }

  
  SetDesiredSize(aDesiredSize);

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}



NS_IMPL_ADDREF_INHERITED(nsTableCellFrame, nsHTMLContainerFrame)
NS_IMPL_RELEASE_INHERITED(nsTableCellFrame, nsHTMLContainerFrame)

NS_IMETHODIMP
nsTableCellFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(aInstancePtr, "null out param");

  if (aIID.Equals(NS_GET_IID(nsITableCellLayout))) {
    *aInstancePtr = static_cast<nsITableCellLayout*>(this);
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIPercentHeightObserver))) {
    *aInstancePtr = static_cast<nsIPercentHeightObserver*>(this);
    return NS_OK;
  }

  return nsHTMLContainerFrame::QueryInterface(aIID, aInstancePtr);
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsTableCellFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    return accService->CreateHTMLTableCellAccessible(static_cast<nsIFrame*>(this), aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif


NS_IMETHODIMP
nsTableCellFrame::GetCellIndexes(PRInt32 &aRowIndex, PRInt32 &aColIndex)
{
  nsresult res = GetRowIndex(aRowIndex);
  if (NS_FAILED(res))
  {
    aColIndex = 0;
    return res;
  }
  aColIndex = mColIndex;
  return  NS_OK;
}

nsIFrame*
NS_NewTableCellFrame(nsIPresShell*   aPresShell,
                     nsStyleContext* aContext,
                     PRBool          aIsBorderCollapse)
{
  if (aIsBorderCollapse)
    return new (aPresShell) nsBCTableCellFrame(aContext);
  else
    return new (aPresShell) nsTableCellFrame(aContext);
}

nsMargin* 
nsTableCellFrame::GetBorderWidth(nsMargin&  aBorder) const
{
  aBorder = GetStyleBorder()->GetActualBorder();
  return &aBorder;
}

nsIAtom*
nsTableCellFrame::GetType() const
{
  return nsGkAtoms::tableCellFrame;
}

 PRBool
nsTableCellFrame::IsContainingBlock() const
{
  return PR_TRUE;
}

#ifdef DEBUG
NS_IMETHODIMP
nsTableCellFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("TableCell"), aResult);
}
#endif



nsBCTableCellFrame::nsBCTableCellFrame(nsStyleContext* aContext)
:nsTableCellFrame(aContext)
{
  mTopBorder = mRightBorder = mBottomBorder = mLeftBorder = 0;
}

nsBCTableCellFrame::~nsBCTableCellFrame()
{
}

nsIAtom*
nsBCTableCellFrame::GetType() const
{
  return nsGkAtoms::bcTableCellFrame;
}

 nsMargin
nsBCTableCellFrame::GetUsedBorder() const
{
  nsMargin result;
  GetBorderWidth(result);
  return result;
}

#ifdef DEBUG
NS_IMETHODIMP
nsBCTableCellFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("BCTableCell"), aResult);
}
#endif

nsMargin* 
nsBCTableCellFrame::GetBorderWidth(nsMargin&  aBorder) const
{
  PRInt32 aPixelsToTwips = nsPresContext::AppUnitsPerCSSPixel();
  aBorder.top    = BC_BORDER_BOTTOM_HALF_COORD(aPixelsToTwips, mTopBorder);
  aBorder.right  = BC_BORDER_LEFT_HALF_COORD(aPixelsToTwips, mRightBorder);
  aBorder.bottom = BC_BORDER_TOP_HALF_COORD(aPixelsToTwips, mBottomBorder);
  aBorder.left   = BC_BORDER_RIGHT_HALF_COORD(aPixelsToTwips, mLeftBorder);
  return &aBorder;
}

BCPixelSize
nsBCTableCellFrame::GetBorderWidth(PRUint8 aSide) const
{
  switch(aSide) {
  case NS_SIDE_TOP:
    return BC_BORDER_BOTTOM_HALF(mTopBorder);
  case NS_SIDE_RIGHT:
    return BC_BORDER_LEFT_HALF(mRightBorder);
  case NS_SIDE_BOTTOM:
    return BC_BORDER_TOP_HALF(mBottomBorder);
  default:
    return BC_BORDER_RIGHT_HALF(mLeftBorder);
  }
}

void 
nsBCTableCellFrame::SetBorderWidth(PRUint8 aSide,
                                   BCPixelSize aValue)
{
  switch(aSide) {
  case NS_SIDE_TOP:
    mTopBorder = aValue;
    break;
  case NS_SIDE_RIGHT:
    mRightBorder = aValue;
    break;
  case NS_SIDE_BOTTOM:
    mBottomBorder = aValue;
    break;
  default:
    mLeftBorder = aValue;
  }
}

 void
nsBCTableCellFrame::GetSelfOverflow(nsRect& aOverflowArea)
{
  nsMargin halfBorder;
  PRInt32 p2t = nsPresContext::AppUnitsPerCSSPixel();
  halfBorder.top = BC_BORDER_TOP_HALF_COORD(p2t, mTopBorder);
  halfBorder.right = BC_BORDER_RIGHT_HALF_COORD(p2t, mRightBorder);
  halfBorder.bottom = BC_BORDER_BOTTOM_HALF_COORD(p2t, mBottomBorder);
  halfBorder.left = BC_BORDER_LEFT_HALF_COORD(p2t, mLeftBorder);

  nsRect overflow(nsPoint(0,0), GetSize());
  overflow.Inflate(halfBorder);
  aOverflowArea = overflow;
}


void
nsBCTableCellFrame::PaintBackground(nsIRenderingContext& aRenderingContext,
                                    const nsRect&        aDirtyRect,
                                    nsPoint              aPt)
{
  
  
  nsMargin borderWidth;
  GetBorderWidth(borderWidth);

  nsStyleBorder myBorder(*GetStyleBorder());

  NS_FOR_CSS_SIDES(side) {
    myBorder.SetBorderWidth(side, borderWidth.side(side));
  }

  nsRect rect(aPt, GetSize());
  nsCSSRendering::PaintBackground(PresContext(), aRenderingContext, this,
                                  aDirtyRect, rect, myBorder, *GetStylePadding(),
                                  PR_TRUE);
}
