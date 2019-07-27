




#include "nsTableCellFrame.h"

#include "gfxUtils.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/Helpers.h"
#include "nsTableFrame.h"
#include "nsTableColFrame.h"
#include "nsTableRowFrame.h"
#include "nsTableRowGroupFrame.h"
#include "nsTablePainter.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsCSSRendering.h"
#include "nsIContent.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValueInlines.h"
#include "nsHTMLParts.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsNameSpaceManager.h"
#include "nsDisplayList.h"
#include "nsLayoutUtils.h"
#include "nsTextFrame.h"
#include "FrameLayerBuilder.h"
#include <algorithm>


#include "nsFrameSelection.h"
#include "mozilla/LookAndFeel.h"

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::image;

nsTableCellFrame::nsTableCellFrame(nsStyleContext* aContext,
                                   nsTableFrame* aTableFrame)
  : nsContainerFrame(aContext)
  , mDesiredSize(aTableFrame->GetWritingMode())
{
  mColIndex = 0;
  mPriorAvailISize = 0;

  SetContentEmpty(false);
  SetHasPctOverBSize(false);
}

nsTableCellFrame::~nsTableCellFrame()
{
}

NS_IMPL_FRAMEARENA_HELPERS(nsTableCellFrame)

nsTableCellFrame*
nsTableCellFrame::GetNextCell() const
{
  nsIFrame* childFrame = GetNextSibling();
  while (childFrame) {
    nsTableCellFrame *cellFrame = do_QueryFrame(childFrame);
    if (cellFrame) {
      return cellFrame;
    }
    childFrame = childFrame->GetNextSibling();
  }
  return nullptr;
}

void
nsTableCellFrame::Init(nsIContent*       aContent,
                       nsContainerFrame* aParent,
                       nsIFrame*         aPrevInFlow)
{
  
  nsContainerFrame::Init(aContent, aParent, aPrevInFlow);

  if (GetStateBits() & NS_FRAME_FONT_INFLATION_CONTAINER) {
    AddStateBits(NS_FRAME_FONT_INFLATION_FLOW_ROOT);
  }

  if (aPrevInFlow) {
    
    nsTableCellFrame* cellFrame = (nsTableCellFrame*)aPrevInFlow;
    int32_t           colIndex;
    cellFrame->GetColIndex(colIndex);
    SetColIndex(colIndex);
  }
}

void
nsTableCellFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  if (GetStateBits() & NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN) {
    nsTableFrame::UnregisterPositionedTablePart(this, aDestructRoot);
  }

  nsContainerFrame::DestroyFrom(aDestructRoot);
}



void
nsTableCellFrame::NotifyPercentBSize(const nsHTMLReflowState& aReflowState)
{
  
  
  
  
  
  

  
  const nsHTMLReflowState *cellRS = aReflowState.mCBReflowState;

  if (cellRS && cellRS->frame == this &&
      (cellRS->ComputedBSize() == NS_UNCONSTRAINEDSIZE ||
       cellRS->ComputedBSize() == 0)) { 
    
    
    

    
    
    

    if (nsTableFrame::AncestorsHaveStyleBSize(*cellRS) ||
        (GetTableFrame()->GetEffectiveRowSpan(*this) == 1 &&
         (cellRS->parentReflowState->frame->GetStateBits() &
          NS_ROW_HAS_CELL_WITH_STYLE_HEIGHT))) {

      for (const nsHTMLReflowState *rs = aReflowState.parentReflowState;
           rs != cellRS;
           rs = rs->parentReflowState) {
        rs->frame->AddStateBits(NS_FRAME_CONTAINS_RELATIVE_BSIZE);
      }

      nsTableFrame::RequestSpecialBSizeReflow(*cellRS);
    }
  }
}


bool
nsTableCellFrame::NeedsToObserve(const nsHTMLReflowState& aReflowState)
{
  const nsHTMLReflowState *rs = aReflowState.parentReflowState;
  if (!rs)
    return false;
  if (rs->frame == this) {
    
    
    
    return true;
  }
  rs = rs->parentReflowState;
  if (!rs) {
    return false;
  }

  
  
  nsIAtom *fType = aReflowState.frame->GetType();
  if (fType == nsGkAtoms::tableFrame) {
    return true;
  }

  
  
  
  
  
  
  return rs->frame == this &&
         (PresContext()->CompatibilityMode() == eCompatibility_NavQuirks ||
          fType == nsGkAtoms::tableOuterFrame);
}

nsresult
nsTableCellFrame::GetRowIndex(int32_t &aRowIndex) const
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
nsTableCellFrame::GetColIndex(int32_t &aColIndex) const
{
  if (GetPrevInFlow()) {
    return static_cast<nsTableCellFrame*>(FirstInFlow())->GetColIndex(aColIndex);
  }
  else {
    aColIndex = mColIndex;
    return  NS_OK;
  }
}

nsresult
nsTableCellFrame::AttributeChanged(int32_t         aNameSpaceID,
                                   nsIAtom*        aAttribute,
                                   int32_t         aModType)
{
  
  
  if (aNameSpaceID == kNameSpaceID_None && aAttribute == nsGkAtoms::nowrap &&
      PresContext()->CompatibilityMode() == eCompatibility_NavQuirks) {
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange, NS_FRAME_IS_DIRTY);
  }
  
  GetTableFrame()->AttributeChangedFor(this, mContent, aAttribute);
  return NS_OK;
}

 void
nsTableCellFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsContainerFrame::DidSetStyleContext(aOldStyleContext);

  if (!aOldStyleContext) 
    return;

  nsTableFrame* tableFrame = GetTableFrame();
  if (tableFrame->IsBorderCollapse() &&
      tableFrame->BCRecalcNeeded(aOldStyleContext, StyleContext())) {
    int32_t colIndex, rowIndex;
    GetColIndex(colIndex);
    GetRowIndex(rowIndex);
    
    
    TableArea damageArea(colIndex, rowIndex, GetColSpan(),
      std::min(GetRowSpan(), tableFrame->GetRowCount() - rowIndex));
    tableFrame->AddBCDamageArea(damageArea);
  }
}

#ifdef DEBUG
void
nsTableCellFrame::AppendFrames(ChildListID     aListID,
                               nsFrameList&    aFrameList)
{
  MOZ_CRASH("unsupported operation");
}

void
nsTableCellFrame::InsertFrames(ChildListID     aListID,
                               nsIFrame*       aPrevFrame,
                               nsFrameList&    aFrameList)
{
  MOZ_CRASH("unsupported operation");
}

void
nsTableCellFrame::RemoveFrame(ChildListID     aListID,
                              nsIFrame*       aOldFrame)
{
  MOZ_CRASH("unsupported operation");
}
#endif

void nsTableCellFrame::SetColIndex(int32_t aColIndex)
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
nsTableCellFrame::DecorateForSelection(nsRenderingContext& aRenderingContext,
                                       nsPoint aPt)
{
  NS_ASSERTION(IsSelected(), "Should only be called for selected cells");
  int16_t displaySelection;
  nsPresContext* presContext = PresContext();
  displaySelection = DisplaySelection(presContext);
  if (displaySelection) {
    nsRefPtr<nsFrameSelection> frameSelection =
      presContext->PresShell()->FrameSelection();

    if (frameSelection->GetTableCellSelection()) {
      nscolor       bordercolor;
      if (displaySelection == nsISelectionController::SELECTION_DISABLED) {
        bordercolor = NS_RGB(176,176,176);
      }
      else {
        bordercolor =
          LookAndFeel::GetColor(LookAndFeel::eColorID_TextSelectBackground);
      }
      nscoord threePx = nsPresContext::CSSPixelsToAppUnits(3);
      if ((mRect.width > threePx) && (mRect.height > threePx))
      {
        
        bordercolor = EnsureDifferentColors(bordercolor,
                                            StyleBackground()->mBackgroundColor);

        int32_t appUnitsPerDevPixel = PresContext()->AppUnitsPerDevPixel();
        Point devPixelOffset = NSPointToPoint(aPt, appUnitsPerDevPixel);

        DrawTarget* drawTarget = aRenderingContext.GetDrawTarget();
        AutoRestoreTransform autoRestoreTransform(drawTarget);
        drawTarget->SetTransform(
          drawTarget->GetTransform().PreTranslate(devPixelOffset));

        ColorPattern color(ToDeviceColor(bordercolor));

        nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);

        StrokeLineWithSnapping(nsPoint(onePixel, 0), nsPoint(mRect.width, 0),
                               appUnitsPerDevPixel, *drawTarget, color);
        StrokeLineWithSnapping(nsPoint(0, onePixel), nsPoint(0, mRect.height),
                               appUnitsPerDevPixel, *drawTarget, color);
        StrokeLineWithSnapping(nsPoint(onePixel, mRect.height),
                               nsPoint(mRect.width, mRect.height),
                               appUnitsPerDevPixel, *drawTarget, color);
        StrokeLineWithSnapping(nsPoint(mRect.width, onePixel),
                               nsPoint(mRect.width, mRect.height),
                               appUnitsPerDevPixel, *drawTarget, color);
        
        nsRect r(onePixel, onePixel,
                 mRect.width - onePixel, mRect.height - onePixel);
        Rect devPixelRect =
          NSRectToSnappedRect(r, appUnitsPerDevPixel, *drawTarget);
        drawTarget->StrokeRect(devPixelRect, color);
        
        StrokeLineWithSnapping(nsPoint(2*onePixel, mRect.height-2*onePixel),
                               nsPoint(mRect.width-onePixel, mRect.height- (2*onePixel)),
                               appUnitsPerDevPixel, *drawTarget, color);
        StrokeLineWithSnapping(nsPoint(mRect.width - (2*onePixel), 2*onePixel),
                               nsPoint(mRect.width - (2*onePixel), mRect.height-onePixel),
                               appUnitsPerDevPixel, *drawTarget, color);
      }
    }
  }
}

DrawResult
nsTableCellFrame::PaintBackground(nsRenderingContext& aRenderingContext,
                                  const nsRect&        aDirtyRect,
                                  nsPoint              aPt,
                                  uint32_t             aFlags)
{
  nsRect rect(aPt, GetSize());
  return nsCSSRendering::PaintBackground(PresContext(), aRenderingContext, this,
                                         aDirtyRect, rect, aFlags);
}


DrawResult
nsTableCellFrame::PaintCellBackground(nsRenderingContext& aRenderingContext,
                                      const nsRect& aDirtyRect, nsPoint aPt,
                                      uint32_t aFlags)
{
  if (!StyleVisibility()->IsVisible()) {
    return DrawResult::SUCCESS;
  }

  return PaintBackground(aRenderingContext, aDirtyRect, aPt, aFlags);
}

nsresult
nsTableCellFrame::ProcessBorders(nsTableFrame* aFrame,
                                 nsDisplayListBuilder* aBuilder,
                                 const nsDisplayListSet& aLists)
{
  const nsStyleBorder* borderStyle = StyleBorder();
  if (aFrame->IsBorderCollapse() || !borderStyle->HasBorder())
    return NS_OK;

  if (!GetContentEmpty() ||
      StyleTableBorder()->mEmptyCells == NS_STYLE_TABLE_EMPTY_CELLS_SHOW) {
    aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
                                              nsDisplayBorder(aBuilder, this));
  }

  return NS_OK;
}

class nsDisplayTableCellBackground : public nsDisplayTableItem {
public:
  nsDisplayTableCellBackground(nsDisplayListBuilder* aBuilder,
                               nsTableCellFrame* aFrame) :
    nsDisplayTableItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayTableCellBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTableCellBackground() {
    MOZ_COUNT_DTOR(nsDisplayTableCellBackground);
  }
#endif

  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState,
                       nsTArray<nsIFrame*> *aOutFrames) override {
    aOutFrames->AppendElement(mFrame);
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) override;
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder,
                           bool* aSnap) override;
  NS_DISPLAY_DECL_NAME("TableCellBackground", TYPE_TABLE_CELL_BACKGROUND)
};

void nsDisplayTableCellBackground::Paint(nsDisplayListBuilder* aBuilder,
                                         nsRenderingContext* aCtx)
{
  DrawResult result = static_cast<nsTableCellFrame*>(mFrame)->
    PaintBackground(*aCtx, mVisibleRect, ToReferenceFrame(),
                    aBuilder->GetBackgroundPaintFlags());

  nsDisplayTableItemGeometry::UpdateDrawResult(this, result);
}

nsRect
nsDisplayTableCellBackground::GetBounds(nsDisplayListBuilder* aBuilder,
                                        bool* aSnap)
{
  
  
  return nsDisplayItem::GetBounds(aBuilder, aSnap);
}

void nsTableCellFrame::InvalidateFrame(uint32_t aDisplayItemKey)
{
  nsIFrame::InvalidateFrame(aDisplayItemKey);
  GetParent()->InvalidateFrameWithRect(GetVisualOverflowRect() + GetPosition(), aDisplayItemKey);
}

void nsTableCellFrame::InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey)
{
  nsIFrame::InvalidateFrameWithRect(aRect, aDisplayItemKey);
  
  
  
  GetParent()->InvalidateFrameWithRect(aRect + GetPosition(), aDisplayItemKey);
}

static void
PaintTableCellSelection(nsIFrame* aFrame, nsRenderingContext* aCtx,
                        const nsRect& aRect, nsPoint aPt)
{
  static_cast<nsTableCellFrame*>(aFrame)->DecorateForSelection(*aCtx, aPt);
}

void
nsTableCellFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                   const nsRect&           aDirtyRect,
                                   const nsDisplayListSet& aLists)
{
  DO_GLOBAL_REFLOW_COUNT_DSP("nsTableCellFrame");
  if (IsVisibleInSelection(aBuilder)) {
    nsTableFrame* tableFrame = GetTableFrame();
    int32_t emptyCellStyle = GetContentEmpty() && !tableFrame->IsBorderCollapse() ?
                                StyleTableBorder()->mEmptyCells
                                : NS_STYLE_TABLE_EMPTY_CELLS_SHOW;
    
    if (StyleVisibility()->IsVisible() &&
        (NS_STYLE_TABLE_EMPTY_CELLS_HIDE != emptyCellStyle)) {
      
      const nsStyleBorder* borderStyle = StyleBorder();
      bool hasBoxShadow = !!borderStyle->mBoxShadow;
      if (hasBoxShadow) {
        aLists.BorderBackground()->AppendNewToTop(
          new (aBuilder) nsDisplayBoxShadowOuter(aBuilder, this));
      }
    
      
      if (aBuilder->IsForEventDelivery() ||
          !StyleBackground()->IsTransparent() || StyleDisplay()->mAppearance) {
        if (!tableFrame->IsBorderCollapse() ||
            aBuilder->IsAtRootOfPseudoStackingContext() ||
            aBuilder->IsForEventDelivery()) {
          
          
          
          nsDisplayTableItem* item =
            new (aBuilder) nsDisplayTableCellBackground(aBuilder, this);
          aLists.BorderBackground()->AppendNewToTop(item);
          item->UpdateForFrameBackground(this);
        } else {
          
          
          nsDisplayTableItem* currentItem = aBuilder->GetCurrentTableItem();
          if (currentItem) {
            currentItem->UpdateForFrameBackground(this);
          }
        }
      }
    
      
      if (hasBoxShadow) {
        aLists.BorderBackground()->AppendNewToTop(
          new (aBuilder) nsDisplayBoxShadowInner(aBuilder, this));
      }
    
      
      ProcessBorders(tableFrame, aBuilder, aLists);
    
      
      if (IsSelected()) {
        aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
          nsDisplayGeneric(aBuilder, this, ::PaintTableCellSelection,
                           "TableCellSelection",
                           nsDisplayItem::TYPE_TABLE_CELL_SELECTION));
      }
    }
    
    
    DisplayOutline(aBuilder, aLists);
  }

  
  
  nsAutoPushCurrentTableItem pushTableItem;
  pushTableItem.Push(aBuilder, nullptr);

  nsIFrame* kid = mFrames.FirstChild();
  NS_ASSERTION(kid && !kid->GetNextSibling(), "Table cells should have just one child");
  
  
  
  
  
  
  BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
}

nsIFrame::LogicalSides
nsTableCellFrame::GetLogicalSkipSides(const nsHTMLReflowState* aReflowState) const
{
  if (MOZ_UNLIKELY(StyleBorder()->mBoxDecorationBreak ==
                     NS_STYLE_BOX_DECORATION_BREAK_CLONE)) {
    return LogicalSides();
  }

  LogicalSides skip;
  if (nullptr != GetPrevInFlow()) {
    skip |= eLogicalSideBitsBStart;
  }
  if (nullptr != GetNextInFlow()) {
    skip |= eLogicalSideBitsBEnd;
  }
  return skip;
}

 nsMargin
nsTableCellFrame::GetBorderOverflow()
{
  return nsMargin(0, 0, 0, 0);
}



void nsTableCellFrame::BlockDirAlignChild(WritingMode aWM, nscoord aMaxAscent)
{
  
  LogicalMargin borderPadding = GetLogicalUsedBorderAndPadding(aWM);

  nscoord bStartInset = borderPadding.BStart(aWM);
  nscoord bEndInset = borderPadding.BEnd(aWM);

  uint8_t verticalAlignFlags = GetVerticalAlign();

  nscoord bSize = BSize(aWM);
  nsIFrame* firstKid = mFrames.FirstChild();
  nscoord containerWidth = mRect.width;
  NS_ASSERTION(firstKid, "Frame construction error, a table cell always has "
                         "an inner cell frame");
  LogicalRect kidRect = firstKid->GetLogicalRect(aWM, containerWidth);
  nscoord childBSize = kidRect.BSize(aWM);

  
  nscoord kidBStart = 0;
  switch (verticalAlignFlags)
  {
    case NS_STYLE_VERTICAL_ALIGN_BASELINE:
      
      
      kidBStart = bStartInset + aMaxAscent - GetCellBaseline();
    break;

    case NS_STYLE_VERTICAL_ALIGN_TOP:
      
      kidBStart = bStartInset;
    break;

    case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
      
      kidBStart = bSize - childBSize - bEndInset;
    break;

    default:
    case NS_STYLE_VERTICAL_ALIGN_MIDDLE:
      
      kidBStart = (bSize - childBSize - bEndInset + bStartInset) / 2;
  }
  
  kidBStart = std::max(0, kidBStart);

  if (kidBStart != kidRect.BStart(aWM)) {
    
    firstKid->InvalidateFrameSubtree();
  }

  firstKid->SetPosition(aWM, LogicalPoint(aWM, kidRect.IStart(aWM),
                                          kidBStart), containerWidth);
  nsHTMLReflowMetrics desiredSize(aWM);
  desiredSize.SetSize(aWM, GetLogicalSize(aWM));

  nsRect overflow(nsPoint(0,0), GetSize());
  overflow.Inflate(GetBorderOverflow());
  desiredSize.mOverflowAreas.SetAllTo(overflow);
  ConsiderChildOverflow(desiredSize.mOverflowAreas, firstKid);
  FinishAndStoreOverflow(&desiredSize);
  if (kidBStart != kidRect.BStart(aWM)) {
    
    
    nsContainerFrame::PositionChildViews(firstKid);

    
    firstKid->InvalidateFrameSubtree();
  }
  if (HasView()) {
    nsContainerFrame::SyncFrameViewAfterReflow(PresContext(), this,
                                               GetView(),
                                               desiredSize.VisualOverflow(), 0);
  }
}

bool
nsTableCellFrame::UpdateOverflow()
{
  nsRect bounds(nsPoint(0,0), GetSize());
  bounds.Inflate(GetBorderOverflow());
  nsOverflowAreas overflowAreas(bounds, bounds);

  nsLayoutUtils::UnionChildOverflow(this, overflowAreas);

  return FinishAndStoreOverflow(overflowAreas, GetSize());
}



uint8_t
nsTableCellFrame::GetVerticalAlign() const
{
  const nsStyleCoord& verticalAlign = StyleTextReset()->mVerticalAlign;
  if (verticalAlign.GetUnit() == eStyleUnit_Enumerated) {
    uint8_t value = verticalAlign.GetIntValue();
    if (value == NS_STYLE_VERTICAL_ALIGN_TOP ||
        value == NS_STYLE_VERTICAL_ALIGN_MIDDLE ||
        value == NS_STYLE_VERTICAL_ALIGN_BOTTOM) {
      return value;
    }
  }
  return NS_STYLE_VERTICAL_ALIGN_BASELINE;
}

bool
nsTableCellFrame::CellHasVisibleContent(nscoord       height,
                                        nsTableFrame* tableFrame,
                                        nsIFrame*     kidFrame)
{
  
  if (height > 0)
    return true;
  if (tableFrame->IsBorderCollapse())
    return true;
  nsIFrame* innerFrame = kidFrame->GetFirstPrincipalChild();
  while(innerFrame) {
    nsIAtom* frameType = innerFrame->GetType();
    if (nsGkAtoms::textFrame == frameType) {
       nsTextFrame* textFrame = static_cast<nsTextFrame*>(innerFrame);
       if (textFrame->HasNoncollapsedCharacters())
         return true;
    }
    else if (nsGkAtoms::placeholderFrame != frameType) {
      return true;
    }
    else {
      nsIFrame *floatFrame = nsLayoutUtils::GetFloatFromPlaceholder(innerFrame);
      if (floatFrame)
        return true;
    }
    innerFrame = innerFrame->GetNextSibling();
  }	
  return false;
}

nscoord
nsTableCellFrame::GetCellBaseline() const
{
  
  
  nsIFrame *inner = mFrames.FirstChild();
  nscoord borderPadding = GetUsedBorderAndPadding().top;
  nscoord result;
  if (nsLayoutUtils::GetFirstLineBaseline(GetWritingMode(), inner, &result))
    return result + borderPadding;
  return inner->GetContentRectRelativeToSelf().YMost() +
         borderPadding;
}

int32_t nsTableCellFrame::GetRowSpan()
{
  int32_t rowSpan=1;
  nsGenericHTMLElement *hc = nsGenericHTMLElement::FromContent(mContent);

  
  if (hc && !StyleContext()->GetPseudo()) {
    const nsAttrValue* attr = hc->GetParsedAttr(nsGkAtoms::rowspan);
    
    
    if (attr && attr->Type() == nsAttrValue::eInteger) {
       rowSpan = attr->GetIntegerValue();
    }
  }
  return rowSpan;
}

int32_t nsTableCellFrame::GetColSpan()
{
  int32_t colSpan=1;
  nsGenericHTMLElement *hc = nsGenericHTMLElement::FromContent(mContent);

  
  if (hc && !StyleContext()->GetPseudo()) {
    const nsAttrValue* attr = hc->GetParsedAttr(nsGkAtoms::colspan);
    
    
    if (attr && attr->Type() == nsAttrValue::eInteger) {
       colSpan = attr->GetIntegerValue();
    }
  }
  return colSpan;
}

 nscoord
nsTableCellFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_MIN_WIDTH(this, result);

  nsIFrame *inner = mFrames.FirstChild();
  result = nsLayoutUtils::IntrinsicForContainer(aRenderingContext, inner,
                                                    nsLayoutUtils::MIN_ISIZE);
  return result;
}

 nscoord
nsTableCellFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_PREF_WIDTH(this, result);

  nsIFrame *inner = mFrames.FirstChild();
  result = nsLayoutUtils::IntrinsicForContainer(aRenderingContext, inner,
                                                nsLayoutUtils::PREF_ISIZE);
  return result;
}

 nsIFrame::IntrinsicISizeOffsetData
nsTableCellFrame::IntrinsicISizeOffsets()
{
  IntrinsicISizeOffsetData result = nsContainerFrame::IntrinsicISizeOffsets();

  result.hMargin = 0;
  result.hPctMargin = 0;

  WritingMode wm = GetWritingMode();
  result.hBorder = GetBorderWidth(wm).IStartEnd(wm);

  return result;
}

#ifdef DEBUG
#define PROBABLY_TOO_LARGE 1000000
static
void DebugCheckChildSize(nsIFrame*            aChild,
                         nsHTMLReflowMetrics& aMet)
{
  WritingMode wm = aMet.GetWritingMode();
  if ((aMet.ISize(wm) < 0) || (aMet.ISize(wm) > PROBABLY_TOO_LARGE)) {
    printf("WARNING: cell content %p has large inline size %d \n",
           static_cast<void*>(aChild), int32_t(aMet.ISize(wm)));
  }
}
#endif




static nscoord
CalcUnpaginatedBSize(nsPresContext*     aPresContext,
                      nsTableCellFrame& aCellFrame,
                      nsTableFrame&     aTableFrame,
                      nscoord           aBlockDirBorderPadding)
{
  const nsTableCellFrame* firstCellInFlow =
    static_cast<nsTableCellFrame*>(aCellFrame.FirstInFlow());
  nsTableFrame* firstTableInFlow  =
    static_cast<nsTableFrame*>(aTableFrame.FirstInFlow());
  nsTableRowFrame* row =
    static_cast<nsTableRowFrame*>(firstCellInFlow->GetParent());
  nsTableRowGroupFrame* firstRGInFlow =
    static_cast<nsTableRowGroupFrame*>(row->GetParent());

  int32_t rowIndex;
  firstCellInFlow->GetRowIndex(rowIndex);
  int32_t rowSpan = aTableFrame.GetEffectiveRowSpan(*firstCellInFlow);

  nscoord computedBSize = firstTableInFlow->GetRowSpacing(rowIndex,
                                                          rowIndex + rowSpan - 1);
  computedBSize -= aBlockDirBorderPadding;
  int32_t rowX;
  for (row = firstRGInFlow->GetFirstRow(), rowX = 0; row; row = row->GetNextRow(), rowX++) {
    if (rowX > rowIndex + rowSpan - 1) {
      break;
    }
    else if (rowX >= rowIndex) {
      computedBSize += row->GetUnpaginatedBSize(aPresContext);
    }
  }
  return computedBSize;
}

void
nsTableCellFrame::Reflow(nsPresContext*           aPresContext,
                         nsHTMLReflowMetrics&     aDesiredSize,
                         const nsHTMLReflowState& aReflowState,
                         nsReflowStatus&          aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsTableCellFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  if (aReflowState.mFlags.mSpecialHeightReflow) {
    FirstInFlow()->AddStateBits(NS_TABLE_CELL_HAD_SPECIAL_REFLOW);
  }

  
  nsTableFrame::CheckRequestSpecialBSizeReflow(aReflowState);

  aStatus = NS_FRAME_COMPLETE;
  WritingMode wm = aReflowState.GetWritingMode();
  LogicalSize availSize(wm, aReflowState.AvailableISize(),
                            aReflowState.AvailableBSize());

  LogicalMargin borderPadding = aReflowState.ComputedLogicalPadding();
  LogicalMargin border = GetBorderWidth(wm);
  borderPadding += border;

  
  availSize.ISize(wm) -= borderPadding.IStartEnd(wm);
  if (NS_UNCONSTRAINEDSIZE != availSize.BSize(wm)) {
    availSize.BSize(wm) -= borderPadding.BStartEnd(wm);
  }

  
  
  if (availSize.BSize(wm) < 0) {
    availSize.BSize(wm) = 1;
  }

  nsHTMLReflowMetrics kidSize(wm, aDesiredSize.mFlags);
  kidSize.ClearSize();
  SetPriorAvailISize(aReflowState.AvailableISize());
  nsIFrame* firstKid = mFrames.FirstChild();
  NS_ASSERTION(firstKid, "Frame construction error, a table cell always has an inner cell frame");
  nsTableFrame* tableFrame = GetTableFrame();

  if (aReflowState.mFlags.mSpecialHeightReflow) {
    const_cast<nsHTMLReflowState&>(aReflowState).
      SetComputedBSize(BSize(wm) - borderPadding.BStartEnd(wm));
    DISPLAY_REFLOW_CHANGE();
  }
  else if (aPresContext->IsPaginated()) {
    nscoord computedUnpaginatedBSize =
      CalcUnpaginatedBSize(aPresContext, (nsTableCellFrame&)*this,
                           *tableFrame, borderPadding.BStartEnd(wm));
    if (computedUnpaginatedBSize > 0) {
      const_cast<nsHTMLReflowState&>(aReflowState).SetComputedBSize(computedUnpaginatedBSize);
      DISPLAY_REFLOW_CHANGE();
    }
  }
  else {
    SetHasPctOverBSize(false);
  }

  WritingMode kidWM = firstKid->GetWritingMode();
  nsHTMLReflowState kidReflowState(aPresContext, aReflowState, firstKid,
                                   availSize.ConvertTo(kidWM, wm));

  
  
  
  if (!aReflowState.mFlags.mSpecialHeightReflow) {
    
    
    
    kidReflowState.mPercentBSizeObserver = this;
  }
  
  kidReflowState.mFlags.mSpecialHeightReflow = false;

  if (aReflowState.mFlags.mSpecialHeightReflow ||
      (FirstInFlow()->GetStateBits() & NS_TABLE_CELL_HAD_SPECIAL_REFLOW)) {
    
    
    
    kidReflowState.SetBResize(true);
  }

  nscoord containerWidth;
  if (aReflowState.ComputedWidth() == NS_UNCONSTRAINEDSIZE) {
    containerWidth = 0; 
                        
  } else {
    containerWidth = aReflowState.ComputedWidth() +
      aReflowState.ComputedPhysicalBorderPadding().LeftRight();
  }
  LogicalPoint kidOrigin(wm, borderPadding.IStart(wm),
                         borderPadding.BStart(wm));
  nsRect origRect = firstKid->GetRect();
  nsRect origVisualOverflow = firstKid->GetVisualOverflowRect();
  bool firstReflow = (firstKid->GetStateBits() & NS_FRAME_FIRST_REFLOW) != 0;

  ReflowChild(firstKid, aPresContext, kidSize, kidReflowState,
              wm, kidOrigin, containerWidth, 0, aStatus);
  if (NS_FRAME_OVERFLOW_IS_INCOMPLETE(aStatus)) {
    
    
    NS_FRAME_SET_INCOMPLETE(aStatus);
    printf("Set table cell incomplete %p\n", static_cast<void*>(this));
  }

  
  if (GetStateBits() & NS_FRAME_IS_DIRTY) {
    InvalidateFrameSubtree();
  }

#ifdef DEBUG
  DebugCheckChildSize(firstKid, kidSize);
#endif

  
  
  nsIFrame* prevInFlow = GetPrevInFlow();
  bool isEmpty;
  if (prevInFlow) {
    isEmpty = static_cast<nsTableCellFrame*>(prevInFlow)->GetContentEmpty();
  } else {
    isEmpty = !CellHasVisibleContent(kidSize.Height(), tableFrame, firstKid);
  }
  SetContentEmpty(isEmpty);

  
  FinishReflowChild(firstKid, aPresContext, kidSize, &kidReflowState,
                    wm, kidOrigin, containerWidth, 0);

  nsTableFrame::InvalidateTableFrame(firstKid, origRect, origVisualOverflow,
                                     firstReflow);

  
  
  LogicalSize cellSize(wm);
  cellSize.BSize(wm) = kidSize.BSize(wm);

  if (NS_UNCONSTRAINEDSIZE != cellSize.BSize(wm)) {
    cellSize.BSize(wm) += borderPadding.BStartEnd(wm);
  }

  
  cellSize.ISize(wm) = kidSize.ISize(wm);      

  
  if (NS_UNCONSTRAINEDSIZE != cellSize.ISize(wm)) {
    cellSize.ISize(wm) += borderPadding.IStartEnd(wm);
  }

  
  aDesiredSize.SetSize(wm, cellSize);

  

  if (aReflowState.mFlags.mSpecialHeightReflow) {
    if (aDesiredSize.BSize(wm) > BSize(wm)) {
      
      
      SetHasPctOverBSize(true);
    }
    if (NS_UNCONSTRAINEDSIZE == aReflowState.AvailableBSize()) {
      aDesiredSize.BSize(wm) = BSize(wm);
    }
  }

  
  
  if (!(GetParent()->GetStateBits() & NS_FRAME_FIRST_REFLOW) &&
      nsSize(aDesiredSize.Width(), aDesiredSize.Height()) != mRect.Size()) {
    InvalidateFrame();
  }

  
  SetDesiredSize(aDesiredSize);

  
  
  
  PushDirtyBitToAbsoluteFrames();

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}



NS_QUERYFRAME_HEAD(nsTableCellFrame)
  NS_QUERYFRAME_ENTRY(nsTableCellFrame)
  NS_QUERYFRAME_ENTRY(nsITableCellLayout)
  NS_QUERYFRAME_ENTRY(nsIPercentBSizeObserver)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

#ifdef ACCESSIBILITY
a11y::AccType
nsTableCellFrame::AccessibleType()
{
  return a11y::eHTMLTableCellType;
}
#endif


NS_IMETHODIMP
nsTableCellFrame::GetCellIndexes(int32_t &aRowIndex, int32_t &aColIndex)
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

nsTableCellFrame*
NS_NewTableCellFrame(nsIPresShell*   aPresShell,
                     nsStyleContext* aContext,
                     nsTableFrame* aTableFrame)
{
  if (aTableFrame->IsBorderCollapse())
    return new (aPresShell) nsBCTableCellFrame(aContext, aTableFrame);
  else
    return new (aPresShell) nsTableCellFrame(aContext, aTableFrame);
}

NS_IMPL_FRAMEARENA_HELPERS(nsBCTableCellFrame)

LogicalMargin
nsTableCellFrame::GetBorderWidth(WritingMode aWM) const
{
  return LogicalMargin(aWM, StyleBorder()->GetComputedBorder());
}

nsIAtom*
nsTableCellFrame::GetType() const
{
  return nsGkAtoms::tableCellFrame;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsTableCellFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("TableCell"), aResult);
}
#endif



nsBCTableCellFrame::nsBCTableCellFrame(nsStyleContext* aContext,
                                       nsTableFrame* aTableFrame)
  : nsTableCellFrame(aContext, aTableFrame)
{
  mBStartBorder = mIEndBorder = mBEndBorder = mIStartBorder = 0;
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
  WritingMode wm = GetWritingMode();
  return GetBorderWidth(wm).GetPhysicalMargin(wm);
}

 bool
nsBCTableCellFrame::GetBorderRadii(const nsSize& aFrameSize,
                                   const nsSize& aBorderArea,
                                   Sides aSkipSides,
                                   nscoord aRadii[8]) const
{
  NS_FOR_CSS_HALF_CORNERS(corner) {
    aRadii[corner] = 0;
  }
  return false;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsBCTableCellFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("BCTableCell"), aResult);
}
#endif

LogicalMargin
nsBCTableCellFrame::GetBorderWidth(WritingMode aWM) const
{
  int32_t pixelsToTwips = nsPresContext::AppUnitsPerCSSPixel();
  return LogicalMargin(aWM,
                       BC_BORDER_END_HALF_COORD(pixelsToTwips, mBStartBorder),
                       BC_BORDER_START_HALF_COORD(pixelsToTwips, mIEndBorder),
                       BC_BORDER_START_HALF_COORD(pixelsToTwips, mBEndBorder),
                       BC_BORDER_END_HALF_COORD(pixelsToTwips, mIStartBorder));
}

BCPixelSize
nsBCTableCellFrame::GetBorderWidth(LogicalSide aSide) const
{
  switch(aSide) {
  case eLogicalSideBStart:
    return BC_BORDER_END_HALF(mBStartBorder);
  case eLogicalSideIEnd:
    return BC_BORDER_START_HALF(mIEndBorder);
  case eLogicalSideBEnd:
    return BC_BORDER_START_HALF(mBEndBorder);
  default:
    return BC_BORDER_END_HALF(mIStartBorder);
  }
}

void
nsBCTableCellFrame::SetBorderWidth(LogicalSide aSide, BCPixelSize aValue)
{
  switch(aSide) {
  case eLogicalSideBStart:
    mBStartBorder = aValue;
    break;
  case eLogicalSideIEnd:
    mIEndBorder = aValue;
    break;
  case eLogicalSideBEnd:
    mBEndBorder = aValue;
    break;
  default:
    mIStartBorder = aValue;
  }
}

 nsMargin
nsBCTableCellFrame::GetBorderOverflow()
{
  WritingMode wm = GetWritingMode();
  int32_t p2t = nsPresContext::AppUnitsPerCSSPixel();
  LogicalMargin halfBorder(wm,
                           BC_BORDER_START_HALF_COORD(p2t, mBStartBorder),
                           BC_BORDER_END_HALF_COORD(p2t, mIEndBorder),
                           BC_BORDER_END_HALF_COORD(p2t, mBEndBorder),
                           BC_BORDER_START_HALF_COORD(p2t, mIStartBorder));
  return halfBorder.GetPhysicalMargin(wm);
}


DrawResult
nsBCTableCellFrame::PaintBackground(nsRenderingContext& aRenderingContext,
                                    const nsRect&        aDirtyRect,
                                    nsPoint              aPt,
                                    uint32_t             aFlags)
{
  
  
  WritingMode wm = GetWritingMode();
  nsMargin borderWidth = GetBorderWidth(wm).GetPhysicalMargin(wm);

  nsStyleBorder myBorder(*StyleBorder());

  NS_FOR_CSS_SIDES(side) {
    myBorder.SetBorderWidth(side, borderWidth.Side(side));
  }

  nsRect rect(aPt, GetSize());
  
  
  return nsCSSRendering::PaintBackgroundWithSC(PresContext(), aRenderingContext,
                                               this, aDirtyRect, rect,
                                               StyleContext(), myBorder,
                                               aFlags, nullptr);
}
