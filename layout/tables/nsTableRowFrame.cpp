




#include "mozilla/Maybe.h"

#include "nsTableRowFrame.h"
#include "nsTableRowGroupFrame.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsTableFrame.h"
#include "nsTableCellFrame.h"
#include "nsCSSRendering.h"
#include "nsHTMLParts.h"
#include "nsTableColGroupFrame.h"
#include "nsTableColFrame.h"
#include "nsCOMPtr.h"
#include "nsDisplayList.h"
#include "nsIFrameInlines.h"
#include <algorithm>

using namespace mozilla;

struct nsTableCellReflowState : public nsHTMLReflowState
{
  nsTableCellReflowState(nsPresContext*           aPresContext,
                         const nsHTMLReflowState& aParentReflowState,
                         nsIFrame*                aFrame,
                         const LogicalSize&       aAvailableSpace,
                         uint32_t                 aFlags = 0)
    : nsHTMLReflowState(aPresContext, aParentReflowState, aFrame,
                        aAvailableSpace, -1, -1, aFlags)
  {
  }

  void FixUp(const nsSize& aAvailSpace);
};

void nsTableCellReflowState::FixUp(const nsSize& aAvailSpace)
{
  
  NS_WARN_IF_FALSE(NS_UNCONSTRAINEDSIZE != aAvailSpace.width,
                   "have unconstrained width; this should only result from "
                   "very large sizes, not attempts at intrinsic width "
                   "calculation");
  if (NS_UNCONSTRAINEDSIZE != ComputedWidth()) {
    nscoord computedWidth =
      aAvailSpace.width - mComputedBorderPadding.LeftRight();
    computedWidth = std::max(0, computedWidth);
    SetComputedWidth(computedWidth);
  }
  if (NS_UNCONSTRAINEDSIZE != ComputedHeight() &&
      NS_UNCONSTRAINEDSIZE != aAvailSpace.height) {
    nscoord computedHeight =
      aAvailSpace.height - mComputedBorderPadding.TopBottom();
    computedHeight = std::max(0, computedHeight);
    SetComputedHeight(computedHeight);
  }
}

void
nsTableRowFrame::InitChildReflowState(nsPresContext&         aPresContext,
                                      const nsSize&           aAvailSize,
                                      bool                    aBorderCollapse,
                                      nsTableCellReflowState& aReflowState)
{
  nsMargin collapseBorder;
  nsMargin* pCollapseBorder = nullptr;
  if (aBorderCollapse) {
    
    nsBCTableCellFrame* bcCellFrame = (nsBCTableCellFrame*)aReflowState.frame;
    if (bcCellFrame) {
      pCollapseBorder = bcCellFrame->GetBorderWidth(collapseBorder);
    }
  }
  aReflowState.Init(&aPresContext, -1, -1, pCollapseBorder);
  aReflowState.FixUp(aAvailSize);
}

void 
nsTableRowFrame::SetFixedHeight(nscoord aValue)
{
  nscoord height = std::max(0, aValue);
  if (HasFixedHeight()) {
    if (height > mStyleFixedHeight) {
      mStyleFixedHeight = height;
    }
  }
  else {
    mStyleFixedHeight = height;
    if (height > 0) {
      SetHasFixedHeight(true);
    }
  }
}

void 
nsTableRowFrame::SetPctHeight(float  aPctValue,
                              bool aForce)
{
  nscoord height = std::max(0, NSToCoordRound(aPctValue * 100.0f));
  if (HasPctHeight()) {
    if ((height > mStylePctHeight) || aForce) {
      mStylePctHeight = height;
    }
  }
  else {
    mStylePctHeight = height;
    if (height > 0) {
      SetHasPctHeight(true);
    }
  }
}



NS_QUERYFRAME_HEAD(nsTableRowFrame)
  NS_QUERYFRAME_ENTRY(nsTableRowFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

nsTableRowFrame::nsTableRowFrame(nsStyleContext* aContext)
  : nsContainerFrame(aContext)
{
  mBits.mRowIndex = mBits.mFirstInserted = 0;
  ResetHeight(0);
}

nsTableRowFrame::~nsTableRowFrame()
{
}

void
nsTableRowFrame::Init(nsIContent*       aContent,
                      nsContainerFrame* aParent,
                      nsIFrame*         aPrevInFlow)
{
  
  nsContainerFrame::Init(aContent, aParent, aPrevInFlow);

  NS_ASSERTION(NS_STYLE_DISPLAY_TABLE_ROW == StyleDisplay()->mDisplay,
               "wrong display on table row frame");

  if (aPrevInFlow) {
    
    nsTableRowFrame* rowFrame = (nsTableRowFrame*)aPrevInFlow;
    
    SetRowIndex(rowFrame->GetRowIndex());
  }
}

void
nsTableRowFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  if (GetStateBits() & NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN) {
    nsTableFrame::UnregisterPositionedTablePart(this, aDestructRoot);
  }

  nsContainerFrame::DestroyFrom(aDestructRoot);
}

 void
nsTableRowFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsContainerFrame::DidSetStyleContext(aOldStyleContext);

  if (!aOldStyleContext) 
    return;
     
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (tableFrame->IsBorderCollapse() &&
      tableFrame->BCRecalcNeeded(aOldStyleContext, StyleContext())) {
    nsIntRect damageArea(0, GetRowIndex(), tableFrame->GetColCount(), 1);
    tableFrame->AddBCDamageArea(damageArea);
  }
}

void
nsTableRowFrame::AppendFrames(ChildListID     aListID,
                              nsFrameList&    aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");

  DrainSelfOverflowList(); 
  const nsFrameList::Slice& newCells = mFrames.AppendFrames(nullptr, aFrameList);

  
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  for (nsFrameList::Enumerator e(newCells) ; !e.AtEnd(); e.Next()) {
    nsIFrame *childFrame = e.get();
    NS_ASSERTION(IS_TABLE_CELL(childFrame->GetType()),"Not a table cell frame/pseudo frame construction failure");
    tableFrame->AppendCell(static_cast<nsTableCellFrame&>(*childFrame), GetRowIndex());
  }

  PresContext()->PresShell()->FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
  tableFrame->SetGeometryDirty();
}


void
nsTableRowFrame::InsertFrames(ChildListID     aListID,
                              nsIFrame*       aPrevFrame,
                              nsFrameList&    aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");
  DrainSelfOverflowList(); 
  
  const nsFrameList::Slice& newCells = mFrames.InsertFrames(nullptr, aPrevFrame, aFrameList);

  
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  nsIAtom* cellFrameType = tableFrame->IsBorderCollapse() ? nsGkAtoms::bcTableCellFrame : nsGkAtoms::tableCellFrame;
  nsTableCellFrame* prevCellFrame = (nsTableCellFrame *)nsTableFrame::GetFrameAtOrBefore(this, aPrevFrame, cellFrameType);
  nsTArray<nsTableCellFrame*> cellChildren;
  for (nsFrameList::Enumerator e(newCells); !e.AtEnd(); e.Next()) {
    nsIFrame *childFrame = e.get();
    NS_ASSERTION(IS_TABLE_CELL(childFrame->GetType()),"Not a table cell frame/pseudo frame construction failure");
    cellChildren.AppendElement(static_cast<nsTableCellFrame*>(childFrame));
  }
  
  int32_t colIndex = -1;
  if (prevCellFrame) {
    prevCellFrame->GetColIndex(colIndex);
  }
  tableFrame->InsertCells(cellChildren, GetRowIndex(), colIndex);
  
  PresContext()->PresShell()->FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
  tableFrame->SetGeometryDirty();
}

void
nsTableRowFrame::RemoveFrame(ChildListID     aListID,
                             nsIFrame*       aOldFrame)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");

  MOZ_ASSERT((nsTableCellFrame*)do_QueryFrame(aOldFrame));
  nsTableCellFrame* cellFrame = static_cast<nsTableCellFrame*>(aOldFrame);
  
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  tableFrame->RemoveCell(cellFrame, GetRowIndex());

  
  mFrames.DestroyFrame(aOldFrame);

  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);

  tableFrame->SetGeometryDirty();
}

 nsMargin
nsTableRowFrame::GetUsedMargin() const
{
  return nsMargin(0,0,0,0);
}

 nsMargin
nsTableRowFrame::GetUsedBorder() const
{
  return nsMargin(0,0,0,0);
}

 nsMargin
nsTableRowFrame::GetUsedPadding() const
{
  return nsMargin(0,0,0,0);
}

nscoord 
GetHeightOfRowsSpannedBelowFirst(nsTableCellFrame& aTableCellFrame,
                                 nsTableFrame&     aTableFrame)
{
  nscoord height = 0;
  int32_t rowSpan = aTableFrame.GetEffectiveRowSpan(aTableCellFrame);
  
  nsIFrame* nextRow = aTableCellFrame.GetParent()->GetNextSibling();
  for (int32_t rowX = 1; ((rowX < rowSpan) && nextRow);) {
    if (nsGkAtoms::tableRowFrame == nextRow->GetType()) {
      height += nextRow->GetSize().height;
      rowX++;
    }
    height += aTableFrame.GetRowSpacing(rowX);
    nextRow = nextRow->GetNextSibling();
  }
  return height;
}

nsTableCellFrame*  
nsTableRowFrame::GetFirstCell() 
{
  nsIFrame* childFrame = mFrames.FirstChild();
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
nsTableRowFrame::DidResize()
{
  
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  nsTableIterator iter(*this);
  nsIFrame* childFrame = iter.First();

  WritingMode wm = GetWritingMode();
  nsHTMLReflowMetrics desiredSize(wm);
  desiredSize.SetSize(wm, GetLogicalSize(wm));
  desiredSize.SetOverflowAreasToDesiredBounds();

  while (childFrame) {
    nsTableCellFrame *cellFrame = do_QueryFrame(childFrame);
    if (cellFrame) {
      nscoord cellHeight = mRect.height + GetHeightOfRowsSpannedBelowFirst(*cellFrame, *tableFrame);

      
      nsRect cellRect = cellFrame->GetRect();
      nsRect cellVisualOverflow = cellFrame->GetVisualOverflowRect();
      if (cellRect.height != cellHeight)
      {
        cellFrame->SetSize(nsSize(cellRect.width, cellHeight));
        nsTableFrame::InvalidateTableFrame(cellFrame, cellRect,
                                           cellVisualOverflow,
                                           false);
      }

      
      
      cellFrame->VerticallyAlignChild(mMaxCellAscent);
      
      
      
      ConsiderChildOverflow(desiredSize.mOverflowAreas, cellFrame);

      
      
    }
    
    childFrame = iter.Next();
  }
  FinishAndStoreOverflow(&desiredSize);
  if (HasView()) {
    nsContainerFrame::SyncFrameViewAfterReflow(PresContext(), this, GetView(),
                                               desiredSize.VisualOverflow(), 0);
  }
  
}



nscoord nsTableRowFrame::GetMaxCellAscent() const
{
  return mMaxCellAscent;
}

nscoord nsTableRowFrame::GetRowBaseline(WritingMode aWritingMode)
{
  if(mMaxCellAscent)
    return mMaxCellAscent;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsTableIterator iter(*this);
  nsIFrame* childFrame = iter.First();
  nscoord ascent = 0;
   while (childFrame) {
    if (IS_TABLE_CELL(childFrame->GetType())) {
      nsIFrame* firstKid = childFrame->GetFirstPrincipalChild();
      ascent = std::max(ascent, firstKid->GetNormalRect().YMost());
    }
    
    childFrame = iter.Next();
  }
  return ascent;
}
nscoord
nsTableRowFrame::GetHeight(nscoord aPctBasis) const
{
  nscoord height = 0;
  if ((aPctBasis > 0) && HasPctHeight()) {
    height = NSToCoordRound(GetPctHeight() * (float)aPctBasis);
  }
  if (HasFixedHeight()) {
    height = std::max(height, GetFixedHeight());
  }
  return std::max(height, GetContentHeight());
}

void 
nsTableRowFrame::ResetHeight(nscoord aFixedHeight)
{
  SetHasFixedHeight(false);
  SetHasPctHeight(false);
  SetFixedHeight(0);
  SetPctHeight(0);
  SetContentHeight(0);

  if (aFixedHeight > 0) {
    SetFixedHeight(aFixedHeight);
  }

  mMaxCellAscent = 0;
  mMaxCellDescent = 0;
}

void
nsTableRowFrame::UpdateHeight(nscoord           aHeight,
                              nscoord           aAscent,
                              nscoord           aDescent,
                              nsTableFrame*     aTableFrame,
                              nsTableCellFrame* aCellFrame)
{
  if (!aTableFrame || !aCellFrame) {
    NS_ASSERTION(false , "invalid call");
    return;
  }

  if (aHeight != NS_UNCONSTRAINEDSIZE) {
    if (!(aCellFrame->HasVerticalAlignBaseline())) { 
      if (GetHeight() < aHeight) {
        int32_t rowSpan = aTableFrame->GetEffectiveRowSpan(*aCellFrame);
        if (rowSpan == 1) {
          SetContentHeight(aHeight);
        }
      }
    }
    else { 
      NS_ASSERTION((aAscent != NS_UNCONSTRAINEDSIZE) && (aDescent != NS_UNCONSTRAINEDSIZE), "invalid call");
      
      if (mMaxCellAscent < aAscent) {
        mMaxCellAscent = aAscent;
      }
      
      if (mMaxCellDescent < aDescent) {
        int32_t rowSpan = aTableFrame->GetEffectiveRowSpan(*aCellFrame);
        if (rowSpan == 1) {
          mMaxCellDescent = aDescent;
        }
      }
      
      if (GetHeight() < mMaxCellAscent + mMaxCellDescent) {
        SetContentHeight(mMaxCellAscent + mMaxCellDescent);
      }
    }
  }
}

nscoord
nsTableRowFrame::CalcHeight(const nsHTMLReflowState& aReflowState)
{
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  nscoord computedHeight = (NS_UNCONSTRAINEDSIZE == aReflowState.ComputedHeight())
                            ? 0 : aReflowState.ComputedHeight();
  ResetHeight(computedHeight);

  const nsStylePosition* position = StylePosition();
  if (position->mHeight.ConvertsToLength()) {
    SetFixedHeight(nsRuleNode::ComputeCoordPercentCalc(position->mHeight, 0));
  }
  else if (eStyleUnit_Percent == position->mHeight.GetUnit()) {
    SetPctHeight(position->mHeight.GetPercentValue());
  }
  

  for (nsIFrame* kidFrame = mFrames.FirstChild(); kidFrame;
       kidFrame = kidFrame->GetNextSibling()) {
    nsTableCellFrame *cellFrame = do_QueryFrame(kidFrame);
    if (cellFrame) {
      WritingMode wm = cellFrame->GetWritingMode();
      LogicalSize desSize = cellFrame->GetDesiredSize();
      if ((NS_UNCONSTRAINEDSIZE == aReflowState.AvailableHeight()) && !GetPrevInFlow()) {
        CalculateCellActualHeight(cellFrame, desSize.BSize(wm));
      }
      
      nscoord ascent;
       if (!kidFrame->GetFirstPrincipalChild()->GetFirstPrincipalChild())
         ascent = desSize.BSize(wm);
       else
         ascent = cellFrame->GetCellBaseline();
       nscoord descent = desSize.BSize(wm) - ascent;
       UpdateHeight(desSize.BSize(wm), ascent, descent, tableFrame, cellFrame);
    }
  }
  return GetHeight();
}







class nsDisplayTableRowBackground : public nsDisplayTableItem {
public:
  nsDisplayTableRowBackground(nsDisplayListBuilder* aBuilder,
                              nsTableRowFrame* aFrame) :
    nsDisplayTableItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayTableRowBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTableRowBackground() {
    MOZ_COUNT_DTOR(nsDisplayTableRowBackground);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) override;
  NS_DISPLAY_DECL_NAME("TableRowBackground", TYPE_TABLE_ROW_BACKGROUND)
};

void
nsDisplayTableRowBackground::Paint(nsDisplayListBuilder* aBuilder,
                                   nsRenderingContext* aCtx)
{
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(mFrame);
  TableBackgroundPainter painter(tableFrame,
                                 TableBackgroundPainter::eOrigin_TableRow,
                                 mFrame->PresContext(), *aCtx,
                                 mVisibleRect, ToReferenceFrame(),
                                 aBuilder->GetBackgroundPaintFlags());

  DrawResult result =
    painter.PaintRow(static_cast<nsTableRowFrame*>(mFrame));

  nsDisplayTableItemGeometry::UpdateDrawResult(this, result);
}

void
nsTableRowFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                  const nsRect&           aDirtyRect,
                                  const nsDisplayListSet& aLists)
{
  nsDisplayTableItem* item = nullptr;
  if (IsVisibleInSelection(aBuilder)) {
    bool isRoot = aBuilder->IsAtRootOfPseudoStackingContext();
    if (isRoot) {
      
      
      
      
      
      
      item = new (aBuilder) nsDisplayTableRowBackground(aBuilder, this);
      aLists.BorderBackground()->AppendNewToTop(item);
    }
  }
  nsTableFrame::DisplayGenericTablePart(aBuilder, this, aDirtyRect, aLists, item);
}

nsIFrame::LogicalSides
nsTableRowFrame::GetLogicalSkipSides(const nsHTMLReflowState* aReflowState) const
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




nsresult
nsTableRowFrame::CalculateCellActualHeight(nsTableCellFrame* aCellFrame,
                                           nscoord&          aDesiredHeight)
{
  nscoord specifiedHeight = 0;
  
  
  const nsStylePosition* position = aCellFrame->StylePosition();

  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  int32_t rowSpan = tableFrame->GetEffectiveRowSpan(*aCellFrame);
  
  switch (position->mHeight.GetUnit()) {
    case eStyleUnit_Calc: {
      if (position->mHeight.CalcHasPercent()) {
        
        break;
      }
      
    }
    case eStyleUnit_Coord: {
      nscoord outsideBoxSizing = 0;
      
      
      
      
      
      if (PresContext()->CompatibilityMode() != eCompatibility_NavQuirks) {
        switch (position->mBoxSizing) {
          case NS_STYLE_BOX_SIZING_CONTENT:
            outsideBoxSizing = aCellFrame->GetUsedBorderAndPadding().TopBottom();
            break;
          case NS_STYLE_BOX_SIZING_PADDING:
            outsideBoxSizing = aCellFrame->GetUsedBorder().TopBottom();
            break;
          default:
            
            break;
        }
      }

      specifiedHeight =
        nsRuleNode::ComputeCoordPercentCalc(position->mHeight, 0) +
          outsideBoxSizing;

      if (1 == rowSpan) 
        SetFixedHeight(specifiedHeight);
      break;
    }
    case eStyleUnit_Percent: {
      if (1 == rowSpan) 
        SetPctHeight(position->mHeight.GetPercentValue());
      
      break;
    }
    case eStyleUnit_Auto:
    default:
      break;
  }

  
  if (specifiedHeight > aDesiredHeight)
    aDesiredHeight = specifiedHeight;

  return NS_OK;
}



static nscoord
CalcAvailWidth(nsTableFrame&     aTableFrame,
               nsTableCellFrame& aCellFrame)
{
  nscoord cellAvailWidth = 0;
  int32_t colIndex;
  aCellFrame.GetColIndex(colIndex);
  int32_t colspan = aTableFrame.GetEffectiveColSpan(aCellFrame);
  NS_ASSERTION(colspan > 0, "effective colspan should be positive");

  for (int32_t spanX = 0; spanX < colspan; spanX++) {
    cellAvailWidth += aTableFrame.GetColumnWidth(colIndex + spanX);
    if (spanX > 0 &&
        aTableFrame.ColumnHasCellSpacingBefore(colIndex + spanX)) {
      cellAvailWidth += aTableFrame.GetColSpacing(colIndex + spanX - 1);
    }
  }
  return cellAvailWidth;
}

nscoord
GetSpaceBetween(int32_t       aPrevColIndex,
                int32_t       aColIndex,
                int32_t       aColSpan,
                nsTableFrame& aTableFrame,
                bool          aIsLeftToRight,
                bool          aCheckVisibility)
{
  nscoord space = 0;
  int32_t colX;
  if (aIsLeftToRight) {
    for (colX = aPrevColIndex + 1; aColIndex > colX; colX++) {
      bool isCollapsed = false;
      if (!aCheckVisibility) {
        space += aTableFrame.GetColumnWidth(colX);
      }
      else {
        nsTableColFrame* colFrame = aTableFrame.GetColFrame(colX);
        const nsStyleVisibility* colVis = colFrame->StyleVisibility();
        bool collapseCol = (NS_STYLE_VISIBILITY_COLLAPSE == colVis->mVisible);
        nsIFrame* cgFrame = colFrame->GetParent();
        const nsStyleVisibility* groupVis = cgFrame->StyleVisibility();
        bool collapseGroup = (NS_STYLE_VISIBILITY_COLLAPSE ==
                                groupVis->mVisible);
        isCollapsed = collapseCol || collapseGroup;
        if (!isCollapsed)
          space += aTableFrame.GetColumnWidth(colX);
      }
      if (!isCollapsed && aTableFrame.ColumnHasCellSpacingBefore(colX)) {
        space += aTableFrame.GetColSpacing(colX - 1);
      }
    }
  } 
  else {
    int32_t lastCol = aColIndex + aColSpan - 1;
    for (colX = aPrevColIndex - 1; colX > lastCol; colX--) {
      bool isCollapsed = false;
      if (!aCheckVisibility) {
        space += aTableFrame.GetColumnWidth(colX);
      }
      else {
        nsTableColFrame* colFrame = aTableFrame.GetColFrame(colX);
        const nsStyleVisibility* colVis = colFrame->StyleVisibility();
        bool collapseCol = (NS_STYLE_VISIBILITY_COLLAPSE == colVis->mVisible);
        nsIFrame* cgFrame = colFrame->GetParent();
        const nsStyleVisibility* groupVis = cgFrame->StyleVisibility();
        bool collapseGroup = (NS_STYLE_VISIBILITY_COLLAPSE ==
                                groupVis->mVisible);
        isCollapsed = collapseCol || collapseGroup;
        if (!isCollapsed)
          space += aTableFrame.GetColumnWidth(colX);
      }
      if (!isCollapsed && aTableFrame.ColumnHasCellSpacingBefore(colX)) {
        space += aTableFrame.GetColSpacing(colX - 1);
      }
    }
  }
  return space;
}


static
nscoord CalcHeightFromUnpaginatedHeight(nsPresContext*   aPresContext,
                                        nsTableRowFrame& aRow)
{
  nscoord height = 0;
  nsTableRowFrame* firstInFlow =
    static_cast<nsTableRowFrame*>(aRow.FirstInFlow());
  if (firstInFlow->HasUnpaginatedHeight()) {
    height = firstInFlow->GetUnpaginatedHeight(aPresContext);
    for (nsIFrame* prevInFlow = aRow.GetPrevInFlow(); prevInFlow;
         prevInFlow = prevInFlow->GetPrevInFlow()) {
      height -= prevInFlow->GetSize().height;
    }
  }
  return std::max(height, 0);
}

void
nsTableRowFrame::ReflowChildren(nsPresContext*          aPresContext,
                                nsHTMLReflowMetrics&     aDesiredSize,
                                const nsHTMLReflowState& aReflowState,
                                nsTableFrame&            aTableFrame,
                                nsReflowStatus&          aStatus)
{
  aStatus = NS_FRAME_COMPLETE;

  
  const bool isPaginated = aPresContext->IsPaginated();
  const bool borderCollapse = aTableFrame.IsBorderCollapse();

  int32_t cellColSpan = 1;  
  
  nsTableIterator iter(*this);
  
  int32_t firstPrevColIndex = (iter.IsLeftToRight()) ? -1 : aTableFrame.GetColCount();
  int32_t prevColIndex  = firstPrevColIndex;
  nscoord x = 0; 

  
  nscoord cellMaxHeight = 0;

  
  for (nsIFrame* kidFrame = iter.First(); kidFrame; kidFrame = iter.Next()) {
    nsTableCellFrame *cellFrame = do_QueryFrame(kidFrame);
    if (!cellFrame) {
      
      NS_NOTREACHED("yikes, a non-row child");

      
      nsTableCellReflowState
        kidReflowState(aPresContext, aReflowState, kidFrame,
                       LogicalSize(kidFrame->GetWritingMode(), 0, 0),
                       nsHTMLReflowState::CALLER_WILL_INIT);
      InitChildReflowState(*aPresContext, nsSize(0,0), false, kidReflowState);
      nsHTMLReflowMetrics desiredSize(aReflowState);
      nsReflowStatus  status;
      ReflowChild(kidFrame, aPresContext, desiredSize, kidReflowState, 0, 0, 0, status);
      kidFrame->DidReflow(aPresContext, nullptr, nsDidReflowStatus::FINISHED);

      continue;
    }

    
    bool doReflowChild = true;
    if (!aReflowState.ShouldReflowAllKids() &&
        !aTableFrame.IsGeometryDirty() &&
        !NS_SUBTREE_DIRTY(kidFrame)) {
      if (!aReflowState.mFlags.mSpecialHeightReflow)
        doReflowChild = false;
    }
    else if ((NS_UNCONSTRAINEDSIZE != aReflowState.AvailableHeight())) {
      
      
      if (aTableFrame.GetEffectiveRowSpan(*cellFrame) > 1) {
        doReflowChild = false;
      }
    }
    if (aReflowState.mFlags.mSpecialHeightReflow) {
      if (!isPaginated && !(cellFrame->GetStateBits() &
                            NS_FRAME_CONTAINS_RELATIVE_HEIGHT)) {
        continue;
      }
    }

    int32_t cellColIndex;
    cellFrame->GetColIndex(cellColIndex);
    cellColSpan = aTableFrame.GetEffectiveColSpan(*cellFrame);

    
    if ((iter.IsLeftToRight() && (prevColIndex != (cellColIndex - 1))) ||
        (!iter.IsLeftToRight() && (prevColIndex != cellColIndex + cellColSpan))) {
      x += GetSpaceBetween(prevColIndex, cellColIndex, cellColSpan, aTableFrame, 
                           iter.IsLeftToRight(), false);
    }

    
    prevColIndex = (iter.IsLeftToRight()) ? cellColIndex + (cellColSpan - 1) : cellColIndex;

    
    nsRect kidRect = kidFrame->GetRect();
    nsPoint origKidNormalPosition = kidFrame->GetNormalPosition();
    MOZ_ASSERT(origKidNormalPosition.y == 0);
    nsRect kidVisualOverflow = kidFrame->GetVisualOverflowRect();
    nsPoint kidPosition(x, 0);
    bool firstReflow =
      (kidFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW) != 0;

    if (doReflowChild) {
      
      nscoord availCellWidth =
        CalcAvailWidth(aTableFrame, *cellFrame);

      Maybe<nsTableCellReflowState> kidReflowState;
      nsHTMLReflowMetrics desiredSize(aReflowState);

      
      
      
      
      
      WritingMode rowWM = aReflowState.GetWritingMode();
      WritingMode cellWM = cellFrame->GetWritingMode();
      LogicalSize cellDesiredSize = cellFrame->GetDesiredSize();
      if ((availCellWidth != cellFrame->GetPriorAvailWidth())       ||
          (cellDesiredSize.ISize(cellWM) > cellFrame->GetPriorAvailWidth()) ||
          (GetStateBits() & NS_FRAME_IS_DIRTY)                      ||
          isPaginated                                               ||
          NS_SUBTREE_DIRTY(cellFrame)                               ||
          
          (cellFrame->GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT) ||
          HasPctHeight()) {
        
        
        nsSize kidAvailSize(availCellWidth, aReflowState.AvailableHeight());

        
        kidReflowState.emplace(aPresContext, aReflowState, kidFrame,
                               LogicalSize(kidFrame->GetWritingMode(),
                                           kidAvailSize),
                               
                               uint32_t(nsHTMLReflowState::CALLER_WILL_INIT));
        InitChildReflowState(*aPresContext, kidAvailSize, borderCollapse,
                             *kidReflowState);

        nsReflowStatus status;
        ReflowChild(kidFrame, aPresContext, desiredSize, *kidReflowState,
                    x, 0, 0, status);

        
        
        if (NS_FRAME_IS_NOT_COMPLETE(status)) {
          aStatus = NS_FRAME_NOT_COMPLETE;
        }
      }
      else {
        if (x != origKidNormalPosition.x) {
          kidFrame->InvalidateFrameSubtree();
        }
        
        desiredSize.SetSize(cellWM, cellDesiredSize);
        desiredSize.mOverflowAreas = cellFrame->GetOverflowAreas();

        
        
        if (!aTableFrame.IsFloating()) {
          
          
          
          nsTableFrame::RePositionViews(kidFrame);
        }
      }
      
      if (NS_UNCONSTRAINEDSIZE == aReflowState.AvailableHeight()) {
        if (!GetPrevInFlow()) {
          
          
          CalculateCellActualHeight(cellFrame, desiredSize.Height());
        }
        
        nscoord ascent;
        if (!kidFrame->GetFirstPrincipalChild()->GetFirstPrincipalChild()) {
          ascent = desiredSize.BSize(rowWM);
        } else {
          ascent = ((nsTableCellFrame *)kidFrame)->GetCellBaseline();
        }
        nscoord descent = desiredSize.BSize(rowWM) - ascent;
        UpdateHeight(desiredSize.BSize(rowWM), ascent, descent, &aTableFrame, cellFrame);
      }
      else {
        cellMaxHeight = std::max(cellMaxHeight, desiredSize.Height());
        int32_t rowSpan = aTableFrame.GetEffectiveRowSpan((nsTableCellFrame&)*kidFrame);
        if (1 == rowSpan) {
          SetContentHeight(cellMaxHeight);
        }
      }

      
      desiredSize.ISize(rowWM) = availCellWidth;

      if (kidReflowState) {
        
        kidReflowState->ApplyRelativePositioning(&kidPosition);
      } else if (kidFrame->IsRelativelyPositioned()) {
        
        
        
        const nsMargin* computedOffsets = static_cast<nsMargin*>
          (kidFrame->Properties().Get(nsIFrame::ComputedOffsetProperty()));
        nsHTMLReflowState::ApplyRelativePositioning(kidFrame, *computedOffsets,
                                                    &kidPosition);
      }
      FinishReflowChild(kidFrame, aPresContext, desiredSize, nullptr,
                        kidPosition.x, kidPosition.y, 0);

      nsTableFrame::InvalidateTableFrame(kidFrame, kidRect, kidVisualOverflow,
                                         firstReflow);
      
      x += desiredSize.Width();  
    }
    else {
      if (x != origKidNormalPosition.x) {
        
        kidFrame->InvalidateFrameSubtree();
        
        
        kidFrame->MovePositionBy(nsPoint(x - origKidNormalPosition.x, 0));
        nsTableFrame::RePositionViews(kidFrame);
        
        kidFrame->InvalidateFrameSubtree();
      }
      
      x += kidRect.width;

      if (kidFrame->GetNextInFlow()) {
        aStatus = NS_FRAME_NOT_COMPLETE;
      }
    }
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, kidFrame);
    x += aTableFrame.GetColSpacing(cellColIndex);
  }

  
  aDesiredSize.Width() = aReflowState.AvailableWidth();

  if (aReflowState.mFlags.mSpecialHeightReflow) {
    aDesiredSize.Height() = mRect.height;
  }
  else if (NS_UNCONSTRAINEDSIZE == aReflowState.AvailableHeight()) {
    aDesiredSize.Height() = CalcHeight(aReflowState);
    if (GetPrevInFlow()) {
      nscoord height = CalcHeightFromUnpaginatedHeight(aPresContext, *this);
      aDesiredSize.Height() = std::max(aDesiredSize.Height(), height);
    }
    else {
      if (isPaginated && HasStyleHeight()) {
        
        SetHasUnpaginatedHeight(true);
        SetUnpaginatedHeight(aPresContext, aDesiredSize.Height());
      }
      if (isPaginated && HasUnpaginatedHeight()) {
        aDesiredSize.Height() = std::max(aDesiredSize.Height(), GetUnpaginatedHeight(aPresContext));
      }
    }
  }
  else { 
    
    
    nscoord styleHeight = CalcHeightFromUnpaginatedHeight(aPresContext, *this);
    if (styleHeight > aReflowState.AvailableHeight()) {
      styleHeight = aReflowState.AvailableHeight();
      NS_FRAME_SET_INCOMPLETE(aStatus);
    }
    aDesiredSize.Height() = std::max(cellMaxHeight, styleHeight);
  }
  aDesiredSize.UnionOverflowAreasWithDesiredBounds();
  FinishAndStoreOverflow(&aDesiredSize);
}




void
nsTableRowFrame::Reflow(nsPresContext*          aPresContext,
                        nsHTMLReflowMetrics&     aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus&          aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsTableRowFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  const nsStyleVisibility* rowVis = StyleVisibility();
  bool collapseRow = (NS_STYLE_VISIBILITY_COLLAPSE == rowVis->mVisible);
  if (collapseRow) {
    tableFrame->SetNeedToCollapse(true);
  }

  
  nsTableFrame::CheckRequestSpecialHeightReflow(aReflowState);

  
  InitHasCellWithStyleHeight(tableFrame);

  ReflowChildren(aPresContext, aDesiredSize, aReflowState, *tableFrame, aStatus);

  if (aPresContext->IsPaginated() && !NS_FRAME_IS_FULLY_COMPLETE(aStatus) &&
      ShouldAvoidBreakInside(aReflowState)) {
    aStatus = NS_INLINE_LINE_BREAK_BEFORE();
  }

  
  aDesiredSize.Width() = aReflowState.AvailableWidth();

  
  
  if (!(GetParent()->GetStateBits() & NS_FRAME_FIRST_REFLOW) &&
      nsSize(aDesiredSize.Width(), aDesiredSize.Height()) != mRect.Size()) {
    InvalidateFrame();
  }

  
  
  
  PushDirtyBitToAbsoluteFrames();

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}






nscoord 
nsTableRowFrame::ReflowCellFrame(nsPresContext*          aPresContext,
                                 const nsHTMLReflowState& aReflowState,
                                 bool                     aIsTopOfPage,
                                 nsTableCellFrame*        aCellFrame,
                                 nscoord                  aAvailableHeight,
                                 nsReflowStatus&          aStatus)
{
  
  nsRect cellRect = aCellFrame->GetRect();
  nsRect cellVisualOverflow = aCellFrame->GetVisualOverflowRect();
  
  nsSize availSize(cellRect.width, aAvailableHeight);
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  bool borderCollapse = tableFrame->IsBorderCollapse();
  nsTableCellReflowState
    cellReflowState(aPresContext, aReflowState, aCellFrame,
                    LogicalSize(aCellFrame->GetWritingMode(),
                                availSize),
                    nsHTMLReflowState::CALLER_WILL_INIT);
  InitChildReflowState(*aPresContext, availSize, borderCollapse, cellReflowState);
  cellReflowState.mFlags.mIsTopOfPage = aIsTopOfPage;

  nsHTMLReflowMetrics desiredSize(aReflowState);

  ReflowChild(aCellFrame, aPresContext, desiredSize, cellReflowState,
              0, 0, NS_FRAME_NO_MOVE_FRAME, aStatus);
  bool fullyComplete = NS_FRAME_IS_COMPLETE(aStatus) && !NS_FRAME_IS_TRUNCATED(aStatus);
  if (fullyComplete) {
    desiredSize.Height() = aAvailableHeight;
  }
  aCellFrame->SetSize(nsSize(cellRect.width, desiredSize.Height()));

  
  
  
  if (fullyComplete) {
    aCellFrame->VerticallyAlignChild(mMaxCellAscent);
  }
  
  nsTableFrame::InvalidateTableFrame(aCellFrame, cellRect,
                                     cellVisualOverflow,
                                     (aCellFrame->GetStateBits() &
                                      NS_FRAME_FIRST_REFLOW) != 0);
  
  aCellFrame->DidReflow(aPresContext, nullptr, nsDidReflowStatus::FINISHED);

  return desiredSize.Height();
}

nscoord
nsTableRowFrame::CollapseRowIfNecessary(nscoord aRowOffset,
                                        nscoord aWidth,
                                        bool    aCollapseGroup,
                                        bool& aDidCollapse)
{
  const nsStyleVisibility* rowVis = StyleVisibility();
  bool collapseRow = (NS_STYLE_VISIBILITY_COLLAPSE == rowVis->mVisible);
  nsTableFrame* tableFrame = static_cast<nsTableFrame*>(
    nsTableFrame::GetTableFrame(this)->FirstInFlow());
  if (collapseRow) {
    tableFrame->SetNeedToCollapse(true);
  }

  if (aRowOffset != 0) {
    
    InvalidateFrameSubtree();
  }
  
  nsRect rowRect = GetRect();
  nsRect oldRect = rowRect;
  nsRect oldVisualOverflow = GetVisualOverflowRect();
  
  rowRect.y -= aRowOffset;
  rowRect.width  = aWidth;
  nsOverflowAreas overflow;
  nscoord shift = 0;

  if (aCollapseGroup || collapseRow) {
    aDidCollapse = true;
    shift = rowRect.height;
    nsTableCellFrame* cellFrame = GetFirstCell();
    if (cellFrame) {
      int32_t rowIndex;
      cellFrame->GetRowIndex(rowIndex);
      shift += tableFrame->GetRowSpacing(rowIndex);
      while (cellFrame) {
        nsRect cRect = cellFrame->GetRect();
        
        
        
        
        if (aRowOffset == 0) {
          InvalidateFrame();
        }
        cRect.height = 0;
        cellFrame->SetRect(cRect);
        cellFrame = cellFrame->GetNextCell();
      }
    } else {
      shift += tableFrame->GetRowSpacing(GetRowIndex());
    }
    rowRect.height = 0;
  }
  else { 
    nsTableIterator iter(*this);
    
    
    int32_t firstPrevColIndex = (iter.IsLeftToRight()) ? -1 :
                                tableFrame->GetColCount();
    int32_t prevColIndex  = firstPrevColIndex;
    nscoord x = 0; 

    int32_t colIncrement = iter.IsLeftToRight() ? 1 : -1;

    nsIFrame* kidFrame = iter.First();
    while (kidFrame) {
      nsTableCellFrame *cellFrame = do_QueryFrame(kidFrame);
      if (cellFrame) {
        int32_t cellColIndex;
        cellFrame->GetColIndex(cellColIndex);
        int32_t cellColSpan = tableFrame->GetEffectiveColSpan(*cellFrame);

        
        
        if ((iter.IsLeftToRight() && (prevColIndex != (cellColIndex - 1))) ||
            (!iter.IsLeftToRight() &&
             (prevColIndex != cellColIndex + cellColSpan))) {
          x += GetSpaceBetween(prevColIndex, cellColIndex, cellColSpan,
                               *tableFrame, iter.IsLeftToRight(),
                               true);
        }
        nsRect cRect(x, 0, 0, rowRect.height);

        
        
        prevColIndex = (iter.IsLeftToRight()) ?
                       cellColIndex + (cellColSpan - 1) : cellColIndex;
        int32_t startIndex = (iter.IsLeftToRight()) ?
                             cellColIndex : cellColIndex + (cellColSpan - 1);
        int32_t actualColSpan = cellColSpan;
        bool isVisible = false;
        for (int32_t colX = startIndex; actualColSpan > 0;
             colX += colIncrement, actualColSpan--) {

          nsTableColFrame* colFrame = tableFrame->GetColFrame(colX);
          const nsStyleVisibility* colVis = colFrame->StyleVisibility();
          bool collapseCol = (NS_STYLE_VISIBILITY_COLLAPSE ==
                                colVis->mVisible);
          nsIFrame* cgFrame = colFrame->GetParent();
          const nsStyleVisibility* groupVis = cgFrame->StyleVisibility();
          bool collapseGroup = (NS_STYLE_VISIBILITY_COLLAPSE ==
                                  groupVis->mVisible);
          bool isCollapsed = collapseCol || collapseGroup;
          if (!isCollapsed) {
            cRect.width += tableFrame->GetColumnWidth(colX);
            isVisible = true;
            if ((actualColSpan > 1)) {
              nsTableColFrame* nextColFrame =
                tableFrame->GetColFrame(colX + colIncrement);
              const nsStyleVisibility* nextColVis =
              nextColFrame->StyleVisibility();
              if ( (NS_STYLE_VISIBILITY_COLLAPSE != nextColVis->mVisible) &&
                  tableFrame->ColumnHasCellSpacingBefore(colX + colIncrement)) {
                cRect.width += tableFrame->GetColSpacing(cellColIndex);
              }
            }
          }
        }
        x += cRect.width;
        if (isVisible)
          x += tableFrame->GetColSpacing(cellColIndex);
        int32_t actualRowSpan = tableFrame->GetEffectiveRowSpan(*cellFrame);
        nsTableRowFrame* rowFrame = GetNextRow();
        for (actualRowSpan--; actualRowSpan > 0 && rowFrame; actualRowSpan--) {
          const nsStyleVisibility* nextRowVis = rowFrame->StyleVisibility();
          bool collapseNextRow = (NS_STYLE_VISIBILITY_COLLAPSE ==
                                    nextRowVis->mVisible);
          if (!collapseNextRow) {
            nsRect nextRect = rowFrame->GetRect();
            cRect.height += nextRect.height +
                            tableFrame->GetRowSpacing(rowFrame->GetRowIndex());
          }
          rowFrame = rowFrame->GetNextRow();
        }

        nsRect oldCellRect = cellFrame->GetRect();
        nsPoint oldCellNormalPos = cellFrame->GetNormalPosition();
        nsRect oldCellVisualOverflow = cellFrame->GetVisualOverflowRect();

        if (aRowOffset == 0 && cRect.TopLeft() != oldCellNormalPos) {
          
          cellFrame->InvalidateFrameSubtree();
        }
        
        cellFrame->MovePositionBy(cRect.TopLeft() - oldCellNormalPos);
        cellFrame->SetSize(cRect.Size());

        
        
        nsRect cellBounds(0, 0, cRect.width, cRect.height);
        nsOverflowAreas cellOverflow(cellBounds, cellBounds);
        cellFrame->FinishAndStoreOverflow(cellOverflow, cRect.Size());
        nsTableFrame::RePositionViews(cellFrame);
        ConsiderChildOverflow(overflow, cellFrame);
                
        if (aRowOffset == 0) {
          nsTableFrame::InvalidateTableFrame(cellFrame, oldCellRect,
                                             oldCellVisualOverflow,
                                             false);
        }
      }
      kidFrame = iter.Next(); 
    }
  }

  SetRect(rowRect);
  overflow.UnionAllWith(nsRect(0, 0, rowRect.width, rowRect.height));
  FinishAndStoreOverflow(overflow, rowRect.Size());

  nsTableFrame::RePositionViews(this);
  nsTableFrame::InvalidateTableFrame(this, oldRect, oldVisualOverflow, false);
  return shift;
}






void 
nsTableRowFrame::InsertCellFrame(nsTableCellFrame* aFrame,
                                 int32_t           aColIndex)
{
  
  nsTableCellFrame* priorCell = nullptr;
  for (nsIFrame* child = mFrames.FirstChild(); child;
       child = child->GetNextSibling()) {
    nsTableCellFrame *cellFrame = do_QueryFrame(child);
    if (cellFrame) {
      int32_t colIndex;
      cellFrame->GetColIndex(colIndex);
      if (colIndex < aColIndex) {
        priorCell = cellFrame;
      }
      else break;
    }
  }
  mFrames.InsertFrame(this, priorCell, aFrame);
}

nsIAtom*
nsTableRowFrame::GetType() const
{
  return nsGkAtoms::tableRowFrame;
}

nsTableRowFrame*  
nsTableRowFrame::GetNextRow() const
{
  nsIFrame* childFrame = GetNextSibling();
  while (childFrame) {
    nsTableRowFrame *rowFrame = do_QueryFrame(childFrame);
    if (rowFrame) {
	  NS_ASSERTION(NS_STYLE_DISPLAY_TABLE_ROW == childFrame->StyleDisplay()->mDisplay, "wrong display type on rowframe");
      return rowFrame;
    }
    childFrame = childFrame->GetNextSibling();
  }
  return nullptr;
}

NS_DECLARE_FRAME_PROPERTY(RowUnpaginatedHeightProperty, nullptr)

void 
nsTableRowFrame::SetUnpaginatedHeight(nsPresContext* aPresContext,
                                      nscoord        aValue)
{
  NS_ASSERTION(!GetPrevInFlow(), "program error");
  
  aPresContext->PropertyTable()->
    Set(this, RowUnpaginatedHeightProperty(), NS_INT32_TO_PTR(aValue));
}

nscoord
nsTableRowFrame::GetUnpaginatedHeight(nsPresContext* aPresContext)
{
  FrameProperties props = FirstInFlow()->Properties();
  return NS_PTR_TO_INT32(props.Get(RowUnpaginatedHeightProperty()));
}

void nsTableRowFrame::SetContinuousBCBorderWidth(uint8_t     aForSide,
                                                 BCPixelSize aPixelValue)
{
  switch (aForSide) {
    case NS_SIDE_RIGHT:
      mRightContBorderWidth = aPixelValue;
      return;
    case NS_SIDE_TOP:
      mTopContBorderWidth = aPixelValue;
      return;
    case NS_SIDE_LEFT:
      mLeftContBorderWidth = aPixelValue;
      return;
    default:
      NS_ERROR("invalid NS_SIDE arg");
  }
}
#ifdef ACCESSIBILITY
a11y::AccType
nsTableRowFrame::AccessibleType()
{
  return a11y::eHTMLTableRowType;
}
#endif





void nsTableRowFrame::InitHasCellWithStyleHeight(nsTableFrame* aTableFrame)
{
  nsTableIterator iter(*this);

  for (nsIFrame* kidFrame = iter.First(); kidFrame; kidFrame = iter.Next()) {
    nsTableCellFrame *cellFrame = do_QueryFrame(kidFrame);
    if (!cellFrame) {
      NS_NOTREACHED("Table row has a non-cell child.");
      continue;
    }
    
    const nsStyleCoord &cellHeight = cellFrame->StylePosition()->mHeight;
    if (aTableFrame->GetEffectiveRowSpan(*cellFrame) == 1 &&
        cellHeight.GetUnit() != eStyleUnit_Auto &&
         
        (!cellHeight.IsCalcUnit() || !cellHeight.HasPercent())) {
      AddStateBits(NS_ROW_HAS_CELL_WITH_STYLE_HEIGHT);
      return;
    }
  }
  RemoveStateBits(NS_ROW_HAS_CELL_WITH_STYLE_HEIGHT);
}
  
void 
nsTableRowFrame::InvalidateFrame(uint32_t aDisplayItemKey)
{
  nsIFrame::InvalidateFrame(aDisplayItemKey);
  GetParent()->InvalidateFrameWithRect(GetVisualOverflowRect() + GetPosition(), aDisplayItemKey);
}

void
nsTableRowFrame::InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey)
{
  nsIFrame::InvalidateFrameWithRect(aRect, aDisplayItemKey);
  
  
  
  GetParent()->InvalidateFrameWithRect(aRect + GetPosition(), aDisplayItemKey);
}



nsTableRowFrame* 
NS_NewTableRowFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTableRowFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTableRowFrame)

#ifdef DEBUG_FRAME_DUMP
nsresult
nsTableRowFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("TableRow"), aResult);
}
#endif
