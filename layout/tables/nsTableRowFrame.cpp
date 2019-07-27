




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
                        aAvailableSpace, nullptr, aFlags)
  {
  }

  void FixUp(const LogicalSize& aAvailSpace);
};

void nsTableCellReflowState::FixUp(const LogicalSize& aAvailSpace)
{
  
  NS_WARN_IF_FALSE(NS_UNCONSTRAINEDSIZE != aAvailSpace.ISize(mWritingMode),
                   "have unconstrained inline-size; this should only result from "
                   "very large sizes, not attempts at intrinsic inline size "
                   "calculation");
  if (NS_UNCONSTRAINEDSIZE != ComputedISize()) {
    nscoord computedISize = aAvailSpace.ISize(mWritingMode) -
      ComputedLogicalBorderPadding().IStartEnd(mWritingMode);
    computedISize = std::max(0, computedISize);
    SetComputedISize(computedISize);
  }
  if (NS_UNCONSTRAINEDSIZE != ComputedBSize() &&
      NS_UNCONSTRAINEDSIZE != aAvailSpace.BSize(mWritingMode)) {
    nscoord computedBSize = aAvailSpace.BSize(mWritingMode) -
      ComputedLogicalBorderPadding().BStartEnd(mWritingMode);
    computedBSize = std::max(0, computedBSize);
    SetComputedBSize(computedBSize);
  }
}

void
nsTableRowFrame::InitChildReflowState(nsPresContext&          aPresContext,
                                      const LogicalSize&      aAvailSize,
                                      bool                    aBorderCollapse,
                                      nsTableCellReflowState& aReflowState)
{
  nsMargin collapseBorder;
  nsMargin* pCollapseBorder = nullptr;
  if (aBorderCollapse) {
    
    nsBCTableCellFrame* bcCellFrame = (nsBCTableCellFrame*)aReflowState.frame;
    if (bcCellFrame) {
      WritingMode wm = GetWritingMode();
      collapseBorder = bcCellFrame->GetBorderWidth(wm).GetPhysicalMargin(wm);
      pCollapseBorder = &collapseBorder;
    }
  }
  aReflowState.Init(&aPresContext, nullptr, pCollapseBorder);
  aReflowState.FixUp(aAvailSize);
}

void
nsTableRowFrame::SetFixedBSize(nscoord aValue)
{
  nscoord bsize = std::max(0, aValue);
  if (HasFixedBSize()) {
    if (bsize > mStyleFixedBSize) {
      mStyleFixedBSize = bsize;
    }
  }
  else {
    mStyleFixedBSize = bsize;
    if (bsize > 0) {
      SetHasFixedBSize(true);
    }
  }
}

void
nsTableRowFrame::SetPctBSize(float aPctValue,
                             bool  aForce)
{
  nscoord bsize = std::max(0, NSToCoordRound(aPctValue * 100.0f));
  if (HasPctBSize()) {
    if ((bsize > mStylePctBSize) || aForce) {
      mStylePctBSize = bsize;
    }
  }
  else {
    mStylePctBSize = bsize;
    if (bsize > 0) {
      SetHasPctBSize(true);
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
  ResetBSize(0);
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

  nsTableFrame* tableFrame = GetTableFrame();
  if (tableFrame->IsBorderCollapse() &&
      tableFrame->BCRecalcNeeded(aOldStyleContext, StyleContext())) {
    TableArea damageArea(0, GetRowIndex(), tableFrame->GetColCount(), 1);
    tableFrame->AddBCDamageArea(damageArea);
  }
}

void
nsTableRowFrame::AppendFrames(ChildListID  aListID,
                              nsFrameList& aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");

  DrainSelfOverflowList(); 
  const nsFrameList::Slice& newCells = mFrames.AppendFrames(nullptr, aFrameList);

  
  nsTableFrame* tableFrame = GetTableFrame();
  for (nsFrameList::Enumerator e(newCells) ; !e.AtEnd(); e.Next()) {
    nsIFrame *childFrame = e.get();
    NS_ASSERTION(IS_TABLE_CELL(childFrame->GetType()),
                 "Not a table cell frame/pseudo frame construction failure");
    tableFrame->AppendCell(static_cast<nsTableCellFrame&>(*childFrame), GetRowIndex());
  }

  PresContext()->PresShell()->FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
  tableFrame->SetGeometryDirty();
}


void
nsTableRowFrame::InsertFrames(ChildListID  aListID,
                              nsIFrame*    aPrevFrame,
                              nsFrameList& aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");
  DrainSelfOverflowList(); 
  
  const nsFrameList::Slice& newCells = mFrames.InsertFrames(nullptr, aPrevFrame, aFrameList);

  
  nsTableFrame* tableFrame = GetTableFrame();
  nsIAtom* cellFrameType = tableFrame->IsBorderCollapse() ? nsGkAtoms::bcTableCellFrame : nsGkAtoms::tableCellFrame;
  nsTableCellFrame* prevCellFrame = (nsTableCellFrame *)nsTableFrame::GetFrameAtOrBefore(this, aPrevFrame, cellFrameType);
  nsTArray<nsTableCellFrame*> cellChildren;
  for (nsFrameList::Enumerator e(newCells); !e.AtEnd(); e.Next()) {
    nsIFrame *childFrame = e.get();
    NS_ASSERTION(IS_TABLE_CELL(childFrame->GetType()),
                 "Not a table cell frame/pseudo frame construction failure");
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
nsTableRowFrame::RemoveFrame(ChildListID aListID,
                             nsIFrame*   aOldFrame)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");

  MOZ_ASSERT((nsTableCellFrame*)do_QueryFrame(aOldFrame));
  nsTableCellFrame* cellFrame = static_cast<nsTableCellFrame*>(aOldFrame);
  
  nsTableFrame* tableFrame = GetTableFrame();
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
GetBSizeOfRowsSpannedBelowFirst(nsTableCellFrame& aTableCellFrame,
                                nsTableFrame&     aTableFrame,
                                const WritingMode aWM)
{
  nscoord bsize = 0;
  int32_t rowSpan = aTableFrame.GetEffectiveRowSpan(aTableCellFrame);
  
  nsIFrame* nextRow = aTableCellFrame.GetParent()->GetNextSibling();
  for (int32_t rowX = 1; ((rowX < rowSpan) && nextRow);) {
    if (nsGkAtoms::tableRowFrame == nextRow->GetType()) {
      bsize += nextRow->BSize(aWM);
      rowX++;
    }
    bsize += aTableFrame.GetRowSpacing(rowX);
    nextRow = nextRow->GetNextSibling();
  }
  return bsize;
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
  
  nsTableFrame* tableFrame = GetTableFrame();
  nsTableIterator iter(*this);
  nsIFrame* childFrame = iter.First();

  WritingMode wm = GetWritingMode();
  nsHTMLReflowMetrics desiredSize(wm);
  desiredSize.SetSize(wm, GetLogicalSize(wm));
  desiredSize.SetOverflowAreasToDesiredBounds();

  while (childFrame) {
    nsTableCellFrame *cellFrame = do_QueryFrame(childFrame);
    if (cellFrame) {
      nscoord cellBSize = BSize(wm) +
        GetBSizeOfRowsSpannedBelowFirst(*cellFrame, *tableFrame, wm);

      
      LogicalSize cellSize = cellFrame->GetLogicalSize(wm);
      nsRect cellVisualOverflow = cellFrame->GetVisualOverflowRect();
      if (cellSize.BSize(wm) != cellBSize) {
        cellSize.BSize(wm) = cellBSize;
        nsRect cellOldRect = cellFrame->GetRect();
        cellFrame->SetSize(wm, cellSize);
        nsTableFrame::InvalidateTableFrame(cellFrame, cellOldRect,
                                           cellVisualOverflow,
                                           false);
      }

      
      
      cellFrame->BlockDirAlignChild(wm, mMaxCellAscent);

      
      
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

nscoord nsTableRowFrame::GetRowBaseline(WritingMode aWM)
{
  if (mMaxCellAscent) {
    return mMaxCellAscent;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsTableIterator iter(*this);
  nsIFrame* childFrame = iter.First();
  nscoord ascent = 0;
  nscoord containerWidth = GetRect().width;
   while (childFrame) {
    if (IS_TABLE_CELL(childFrame->GetType())) {
      nsIFrame* firstKid = childFrame->GetFirstPrincipalChild();
      ascent = std::max(ascent,
                        LogicalRect(aWM, firstKid->GetNormalRect(),
                                    containerWidth).BEnd(aWM));
    }
    
    childFrame = iter.Next();
  }
  return ascent;
}

nscoord
nsTableRowFrame::GetBSize(nscoord aPctBasis) const
{
  nscoord bsize = 0;
  if ((aPctBasis > 0) && HasPctBSize()) {
    bsize = NSToCoordRound(GetPctBSize() * (float)aPctBasis);
  }
  if (HasFixedBSize()) {
    bsize = std::max(bsize, GetFixedBSize());
  }
  return std::max(bsize, GetContentBSize());
}

void
nsTableRowFrame::ResetBSize(nscoord aFixedBSize)
{
  SetHasFixedBSize(false);
  SetHasPctBSize(false);
  SetFixedBSize(0);
  SetPctBSize(0);
  SetContentBSize(0);

  if (aFixedBSize > 0) {
    SetFixedBSize(aFixedBSize);
  }

  mMaxCellAscent = 0;
  mMaxCellDescent = 0;
}

void
nsTableRowFrame::UpdateBSize(nscoord           aBSize,
                             nscoord           aAscent,
                             nscoord           aDescent,
                             nsTableFrame*     aTableFrame,
                             nsTableCellFrame* aCellFrame)
{
  if (!aTableFrame || !aCellFrame) {
    NS_ASSERTION(false , "invalid call");
    return;
  }

  if (aBSize != NS_UNCONSTRAINEDSIZE) {
    if (!(aCellFrame->HasVerticalAlignBaseline())) { 
      if (GetBSize() < aBSize) {
        int32_t rowSpan = aTableFrame->GetEffectiveRowSpan(*aCellFrame);
        if (rowSpan == 1) {
          SetContentBSize(aBSize);
        }
      }
    }
    else { 
      NS_ASSERTION((aAscent != NS_UNCONSTRAINEDSIZE) &&
                   (aDescent != NS_UNCONSTRAINEDSIZE), "invalid call");
      
      if (mMaxCellAscent < aAscent) {
        mMaxCellAscent = aAscent;
      }
      
      if (mMaxCellDescent < aDescent) {
        int32_t rowSpan = aTableFrame->GetEffectiveRowSpan(*aCellFrame);
        if (rowSpan == 1) {
          mMaxCellDescent = aDescent;
        }
      }
      
      if (GetBSize() < mMaxCellAscent + mMaxCellDescent) {
        SetContentBSize(mMaxCellAscent + mMaxCellDescent);
      }
    }
  }
}

nscoord
nsTableRowFrame::CalcBSize(const nsHTMLReflowState& aReflowState)
{
  nsTableFrame* tableFrame = GetTableFrame();
  nscoord computedBSize = (NS_UNCONSTRAINEDSIZE == aReflowState.ComputedBSize())
                            ? 0 : aReflowState.ComputedBSize();
  ResetBSize(computedBSize);

  WritingMode wm = aReflowState.GetWritingMode();
  const nsStylePosition* position = StylePosition();
  const nsStyleCoord& bsizeStyleCoord = position->BSize(wm);
  if (bsizeStyleCoord.ConvertsToLength()) {
    SetFixedBSize(nsRuleNode::ComputeCoordPercentCalc(bsizeStyleCoord, 0));
  }
  else if (eStyleUnit_Percent == bsizeStyleCoord.GetUnit()) {
    SetPctBSize(bsizeStyleCoord.GetPercentValue());
  }
  

  for (nsIFrame* kidFrame = mFrames.FirstChild(); kidFrame;
       kidFrame = kidFrame->GetNextSibling()) {
    nsTableCellFrame *cellFrame = do_QueryFrame(kidFrame);
    if (cellFrame) {
      MOZ_ASSERT(cellFrame->GetWritingMode() == wm);
      LogicalSize desSize = cellFrame->GetDesiredSize();
      if ((NS_UNCONSTRAINEDSIZE == aReflowState.AvailableBSize()) && !GetPrevInFlow()) {
        CalculateCellActualBSize(cellFrame, desSize.BSize(wm), wm);
      }
      
      nscoord ascent;
       if (!kidFrame->GetFirstPrincipalChild()->GetFirstPrincipalChild())
         ascent = desSize.BSize(wm);
       else
         ascent = cellFrame->GetCellBaseline();
       nscoord descent = desSize.BSize(wm) - ascent;
       UpdateBSize(desSize.BSize(wm), ascent, descent, tableFrame, cellFrame);
    }
  }
  return GetBSize();
}







class nsDisplayTableRowBackground : public nsDisplayTableItem {
public:
  nsDisplayTableRowBackground(nsDisplayListBuilder* aBuilder,
                              nsTableRowFrame*      aFrame) :
    nsDisplayTableItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayTableRowBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTableRowBackground() {
    MOZ_COUNT_DTOR(nsDisplayTableRowBackground);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext*   aCtx) override;
  NS_DISPLAY_DECL_NAME("TableRowBackground", TYPE_TABLE_ROW_BACKGROUND)
};

void
nsDisplayTableRowBackground::Paint(nsDisplayListBuilder* aBuilder,
                                   nsRenderingContext*   aCtx)
{
  auto rowFrame = static_cast<nsTableRowFrame*>(mFrame);
  TableBackgroundPainter painter(rowFrame->GetTableFrame(),
                                 TableBackgroundPainter::eOrigin_TableRow,
                                 mFrame->PresContext(), *aCtx,
                                 mVisibleRect, ToReferenceFrame(),
                                 aBuilder->GetBackgroundPaintFlags());

  DrawResult result = painter.PaintRow(rowFrame);
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
nsTableRowFrame::CalculateCellActualBSize(nsTableCellFrame* aCellFrame,
                                          nscoord&          aDesiredBSize,
                                          WritingMode       aWM)
{
  nscoord specifiedBSize = 0;

  
  const nsStylePosition* position = aCellFrame->StylePosition();

  int32_t rowSpan = GetTableFrame()->GetEffectiveRowSpan(*aCellFrame);

  const nsStyleCoord& bsizeStyleCoord = position->BSize(aWM);
  switch (bsizeStyleCoord.GetUnit()) {
    case eStyleUnit_Calc: {
      if (bsizeStyleCoord.CalcHasPercent()) {
        
        break;
      }
      
    }
    case eStyleUnit_Coord: {
      nscoord outsideBoxSizing = 0;
      
      
      
      
      
      if (PresContext()->CompatibilityMode() != eCompatibility_NavQuirks) {
        switch (position->mBoxSizing) {
          case NS_STYLE_BOX_SIZING_CONTENT:
            outsideBoxSizing =
              aCellFrame->GetLogicalUsedBorderAndPadding(aWM).BStartEnd(aWM);
            break;
          case NS_STYLE_BOX_SIZING_PADDING:
            outsideBoxSizing =
              aCellFrame->GetLogicalUsedBorder(aWM).BStartEnd(aWM);
            break;
          default:
            
            break;
        }
      }

      specifiedBSize =
        nsRuleNode::ComputeCoordPercentCalc(bsizeStyleCoord, 0) +
        outsideBoxSizing;

      if (1 == rowSpan) {
        SetFixedBSize(specifiedBSize);
      }
      break;
    }
    case eStyleUnit_Percent: {
      if (1 == rowSpan) {
        SetPctBSize(bsizeStyleCoord.GetPercentValue());
      }
      
      
      break;
    }
    case eStyleUnit_Auto:
    default:
      break;
  }

  
  
  if (specifiedBSize > aDesiredBSize) {
    aDesiredBSize = specifiedBSize;
  }

  return NS_OK;
}



static nscoord
CalcAvailISize(nsTableFrame&     aTableFrame,
               nsTableCellFrame& aCellFrame)
{
  nscoord cellAvailISize = 0;
  int32_t colIndex;
  aCellFrame.GetColIndex(colIndex);
  int32_t colspan = aTableFrame.GetEffectiveColSpan(aCellFrame);
  NS_ASSERTION(colspan > 0, "effective colspan should be positive");

  for (int32_t spanX = 0; spanX < colspan; spanX++) {
    cellAvailISize += aTableFrame.GetColumnISize(colIndex + spanX);
    if (spanX > 0 &&
        aTableFrame.ColumnHasCellSpacingBefore(colIndex + spanX)) {
      cellAvailISize += aTableFrame.GetColSpacing(colIndex + spanX - 1);
    }
  }
  return cellAvailISize;
}

nscoord
GetSpaceBetween(int32_t       aPrevColIndex,
                int32_t       aColIndex,
                int32_t       aColSpan,
                nsTableFrame& aTableFrame,
                bool          aCheckVisibility)
{
  nscoord space = 0;
  int32_t colX;
  for (colX = aPrevColIndex + 1; aColIndex > colX; colX++) {
    bool isCollapsed = false;
    if (!aCheckVisibility) {
      space += aTableFrame.GetColumnISize(colX);
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
        space += aTableFrame.GetColumnISize(colX);
    }
    if (!isCollapsed && aTableFrame.ColumnHasCellSpacingBefore(colX)) {
      space += aTableFrame.GetColSpacing(colX - 1);
    }
  }
  return space;
}


static
nscoord CalcBSizeFromUnpaginatedBSize(nsPresContext*   aPresContext,
                                      nsTableRowFrame& aRow,
                                      WritingMode      aWM)
{
  nscoord bsize = 0;
  nsTableRowFrame* firstInFlow =
    static_cast<nsTableRowFrame*>(aRow.FirstInFlow());
  if (firstInFlow->HasUnpaginatedBSize()) {
    bsize = firstInFlow->GetUnpaginatedBSize(aPresContext);
    for (nsIFrame* prevInFlow = aRow.GetPrevInFlow(); prevInFlow;
         prevInFlow = prevInFlow->GetPrevInFlow()) {
      bsize -= prevInFlow->BSize(aWM);
    }
  }
  return std::max(bsize, 0);
}

void
nsTableRowFrame::ReflowChildren(nsPresContext*           aPresContext,
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
  
  int32_t firstPrevColIndex = -1;
  int32_t prevColIndex  = firstPrevColIndex;
  nscoord iCoord = 0; 

  
  nscoord cellMaxBSize = 0;

  
  WritingMode wm = aReflowState.GetWritingMode();
  nscoord containerWidth = aReflowState.ComputedWidth();
  if (containerWidth == NS_UNCONSTRAINEDSIZE) {
    containerWidth = 0; 
  } else {
    containerWidth += aReflowState.ComputedPhysicalBorderPadding().LeftRight();
  }

  for (nsIFrame* kidFrame = iter.First(); kidFrame; kidFrame = iter.Next()) {
    nsTableCellFrame *cellFrame = do_QueryFrame(kidFrame);
    if (!cellFrame) {
      
      NS_NOTREACHED("yikes, a non-row child");

      
      nsTableCellReflowState
        kidReflowState(aPresContext, aReflowState, kidFrame,
                       LogicalSize(kidFrame->GetWritingMode(), 0, 0),
                       nsHTMLReflowState::CALLER_WILL_INIT);
      InitChildReflowState(*aPresContext, LogicalSize(wm), false, kidReflowState);
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
    else if ((NS_UNCONSTRAINEDSIZE != aReflowState.AvailableBSize())) {
      
      
      if (aTableFrame.GetEffectiveRowSpan(*cellFrame) > 1) {
        doReflowChild = false;
      }
    }
    if (aReflowState.mFlags.mSpecialHeightReflow) {
      if (!isPaginated && !(cellFrame->GetStateBits() &
                            NS_FRAME_CONTAINS_RELATIVE_BSIZE)) {
        continue;
      }
    }

    int32_t cellColIndex;
    cellFrame->GetColIndex(cellColIndex);
    cellColSpan = aTableFrame.GetEffectiveColSpan(*cellFrame);

    
    if (prevColIndex != (cellColIndex - 1)) {
      iCoord += GetSpaceBetween(prevColIndex, cellColIndex, cellColSpan, aTableFrame,
                                false);
    }

    
    prevColIndex = cellColIndex + (cellColSpan - 1);

    
    nsRect kidRect = kidFrame->GetRect();
    LogicalPoint origKidNormalPosition =
      kidFrame->GetLogicalNormalPosition(wm, containerWidth);
    
    
    MOZ_ASSERT(origKidNormalPosition.B(wm) == 0);
    nsRect kidVisualOverflow = kidFrame->GetVisualOverflowRect();
    LogicalPoint kidPosition(wm, iCoord, 0);
    bool firstReflow =
      (kidFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW) != 0;

    if (doReflowChild) {
      
      
      nscoord availCellISize = CalcAvailISize(aTableFrame, *cellFrame);

      Maybe<nsTableCellReflowState> kidReflowState;
      nsHTMLReflowMetrics desiredSize(aReflowState);

      
      
      
      
      
      WritingMode wm = aReflowState.GetWritingMode();
      NS_ASSERTION(cellFrame->GetWritingMode() == wm,
                   "expected consistent writing-mode within table");
      LogicalSize cellDesiredSize = cellFrame->GetDesiredSize();
      if ((availCellISize != cellFrame->GetPriorAvailISize())       ||
          (cellDesiredSize.ISize(wm) > cellFrame->GetPriorAvailISize()) ||
          (GetStateBits() & NS_FRAME_IS_DIRTY)                      ||
          isPaginated                                               ||
          NS_SUBTREE_DIRTY(cellFrame)                               ||
          
          (cellFrame->GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_BSIZE) ||
          HasPctBSize()) {
        
        
        LogicalSize kidAvailSize(wm, availCellISize, aReflowState.AvailableBSize());

        
        kidReflowState.emplace(aPresContext, aReflowState, kidFrame,
                               kidAvailSize,
                               
                               uint32_t(nsHTMLReflowState::CALLER_WILL_INIT));
        InitChildReflowState(*aPresContext, kidAvailSize, borderCollapse,
                             *kidReflowState);

        nsReflowStatus status;
        ReflowChild(kidFrame, aPresContext, desiredSize, *kidReflowState,
                    wm, kidPosition, containerWidth, 0, status);

        
        
        if (NS_FRAME_IS_NOT_COMPLETE(status)) {
          aStatus = NS_FRAME_NOT_COMPLETE;
        }
      } else {
        if (iCoord != origKidNormalPosition.I(wm)) {
          kidFrame->InvalidateFrameSubtree();
        }

        desiredSize.SetSize(wm, cellDesiredSize);
        desiredSize.mOverflowAreas = cellFrame->GetOverflowAreas();

        
        
        if (!aTableFrame.IsFloating()) {
          
          
          
          nsTableFrame::RePositionViews(kidFrame);
        }
      }

      if (NS_UNCONSTRAINEDSIZE == aReflowState.AvailableBSize()) {
        if (!GetPrevInFlow()) {
          
          
          CalculateCellActualBSize(cellFrame, desiredSize.BSize(wm), wm);
        }
        
        nscoord ascent;
        if (!kidFrame->GetFirstPrincipalChild()->GetFirstPrincipalChild()) {
          ascent = desiredSize.BSize(wm);
        } else {
          ascent = ((nsTableCellFrame *)kidFrame)->GetCellBaseline();
        }
        nscoord descent = desiredSize.BSize(wm) - ascent;
        UpdateBSize(desiredSize.BSize(wm), ascent, descent, &aTableFrame, cellFrame);
      } else {
        cellMaxBSize = std::max(cellMaxBSize, desiredSize.BSize(wm));
        int32_t rowSpan = aTableFrame.GetEffectiveRowSpan((nsTableCellFrame&)*kidFrame);
        if (1 == rowSpan) {
          SetContentBSize(cellMaxBSize);
        }
      }

      
      desiredSize.ISize(wm) = availCellISize;

      if (kidReflowState) {
        
        kidReflowState->ApplyRelativePositioning(&kidPosition, containerWidth);
      } else if (kidFrame->IsRelativelyPositioned()) {
        
        
        
        LogicalMargin computedOffsets(wm, *static_cast<nsMargin*>
          (kidFrame->Properties().Get(nsIFrame::ComputedOffsetProperty())));
        nsHTMLReflowState::ApplyRelativePositioning(kidFrame, wm, computedOffsets,
                                                    &kidPosition, containerWidth);
      }

      
      
      
      
      
      FinishReflowChild(kidFrame, aPresContext, desiredSize, nullptr,
                        wm, kidPosition,
                        wm.IsVerticalRL() && containerWidth == 0
                          ? desiredSize.Width()
                          : containerWidth,
                        0);

      nsTableFrame::InvalidateTableFrame(kidFrame, kidRect, kidVisualOverflow,
                                         firstReflow);

      iCoord += desiredSize.ISize(wm);
    } else {
      if (iCoord != origKidNormalPosition.I(wm)) {
        
        kidFrame->InvalidateFrameSubtree();
        
        
        kidFrame->MovePositionBy(wm,
          LogicalPoint(wm, iCoord - origKidNormalPosition.I(wm), 0));
        nsTableFrame::RePositionViews(kidFrame);
        
        kidFrame->InvalidateFrameSubtree();
      }
      
      iCoord += kidFrame->ISize(wm);

      if (kidFrame->GetNextInFlow()) {
        aStatus = NS_FRAME_NOT_COMPLETE;
      }
    }
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, kidFrame);
    iCoord += aTableFrame.GetColSpacing(cellColIndex);
  }

  
  
  aDesiredSize.ISize(wm) = aReflowState.AvailableISize();

  if (aReflowState.mFlags.mSpecialHeightReflow) {
    aDesiredSize.BSize(wm) = BSize(wm);
  } else if (NS_UNCONSTRAINEDSIZE == aReflowState.AvailableBSize()) {
    aDesiredSize.BSize(wm) = CalcBSize(aReflowState);
    if (GetPrevInFlow()) {
      nscoord bsize = CalcBSizeFromUnpaginatedBSize(aPresContext, *this, wm);
      aDesiredSize.BSize(wm) = std::max(aDesiredSize.BSize(wm), bsize);
    } else {
      if (isPaginated && HasStyleBSize()) {
        
        SetHasUnpaginatedBSize(true);
        SetUnpaginatedBSize(aPresContext, aDesiredSize.BSize(wm));
      }
      if (isPaginated && HasUnpaginatedBSize()) {
        aDesiredSize.BSize(wm) = std::max(aDesiredSize.BSize(wm),
                                          GetUnpaginatedBSize(aPresContext));
      }
    }
  } else { 
    
    
    nscoord styleBSize = CalcBSizeFromUnpaginatedBSize(aPresContext, *this,
                                                       wm);
    if (styleBSize > aReflowState.AvailableBSize()) {
      styleBSize = aReflowState.AvailableBSize();
      NS_FRAME_SET_INCOMPLETE(aStatus);
    }
    aDesiredSize.BSize(wm) = std::max(cellMaxBSize, styleBSize);
  }

  if (wm.IsVerticalRL()) {
    
    
    
    for (nsIFrame* kidFrame = iter.First(); kidFrame; kidFrame = iter.Next()) {
      nsTableCellFrame *cellFrame = do_QueryFrame(kidFrame);
      if (!cellFrame) {
        continue;
      }
      if (kidFrame->BSize(wm) != aDesiredSize.BSize(wm)) {
        kidFrame->MovePositionBy(wm,
          LogicalPoint(wm, 0, kidFrame->BSize(wm) - aDesiredSize.BSize(wm)));
        nsTableFrame::RePositionViews(kidFrame);
        
      }
    }
  }

  aDesiredSize.UnionOverflowAreasWithDesiredBounds();
  FinishAndStoreOverflow(&aDesiredSize);
}




void
nsTableRowFrame::Reflow(nsPresContext*           aPresContext,
                        nsHTMLReflowMetrics&     aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus&          aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsTableRowFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  WritingMode wm = aReflowState.GetWritingMode();

  nsTableFrame* tableFrame = GetTableFrame();
  const nsStyleVisibility* rowVis = StyleVisibility();
  bool collapseRow = (NS_STYLE_VISIBILITY_COLLAPSE == rowVis->mVisible);
  if (collapseRow) {
    tableFrame->SetNeedToCollapse(true);
  }

  
  nsTableFrame::CheckRequestSpecialHeightReflow(aReflowState);

  
  InitHasCellWithStyleBSize(tableFrame);

  ReflowChildren(aPresContext, aDesiredSize, aReflowState, *tableFrame, aStatus);

  if (aPresContext->IsPaginated() && !NS_FRAME_IS_FULLY_COMPLETE(aStatus) &&
      ShouldAvoidBreakInside(aReflowState)) {
    aStatus = NS_INLINE_LINE_BREAK_BEFORE();
  }

  
  
  aDesiredSize.ISize(wm) = aReflowState.AvailableISize();

  
  
  if (!(GetParent()->GetStateBits() & NS_FRAME_FIRST_REFLOW) &&
      nsSize(aDesiredSize.Width(), aDesiredSize.Height()) != mRect.Size()) {
    InvalidateFrame();
  }

  
  
  
  PushDirtyBitToAbsoluteFrames();

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}






nscoord
nsTableRowFrame::ReflowCellFrame(nsPresContext*           aPresContext,
                                 const nsHTMLReflowState& aReflowState,
                                 bool                     aIsTopOfPage,
                                 nsTableCellFrame*        aCellFrame,
                                 nscoord                  aAvailableBSize,
                                 nsReflowStatus&          aStatus)
{
  WritingMode wm = aReflowState.GetWritingMode();

  
  nscoord containerWidth = aCellFrame->GetSize().width;
  LogicalRect cellRect = aCellFrame->GetLogicalRect(wm, containerWidth);
  nsRect cellVisualOverflow = aCellFrame->GetVisualOverflowRect();

  LogicalSize cellSize = cellRect.Size(wm);
  LogicalSize availSize(wm, cellRect.ISize(wm), aAvailableBSize);
  bool borderCollapse = GetTableFrame()->IsBorderCollapse();
  NS_ASSERTION(aCellFrame->GetWritingMode() == wm,
               "expected consistent writing-mode within table");
  nsTableCellReflowState
    cellReflowState(aPresContext, aReflowState, aCellFrame, availSize,
                    nsHTMLReflowState::CALLER_WILL_INIT);
  InitChildReflowState(*aPresContext, availSize, borderCollapse, cellReflowState);
  cellReflowState.mFlags.mIsTopOfPage = aIsTopOfPage;

  nsHTMLReflowMetrics desiredSize(aReflowState);

  ReflowChild(aCellFrame, aPresContext, desiredSize, cellReflowState,
              0, 0, NS_FRAME_NO_MOVE_FRAME, aStatus);
  bool fullyComplete = NS_FRAME_IS_COMPLETE(aStatus) && !NS_FRAME_IS_TRUNCATED(aStatus);
  if (fullyComplete) {
    desiredSize.BSize(wm) = aAvailableBSize;
  }
  aCellFrame->SetSize(wm, LogicalSize(wm, cellSize.ISize(wm),
                                      desiredSize.BSize(wm)));

  
  
  
  if (fullyComplete) {
    aCellFrame->BlockDirAlignChild(wm, mMaxCellAscent);
  }

  nsTableFrame::InvalidateTableFrame(aCellFrame,
                                     cellRect.GetPhysicalRect(wm, containerWidth),
                                     cellVisualOverflow,
                                     (aCellFrame->GetStateBits() &
                                      NS_FRAME_FIRST_REFLOW) != 0);

  aCellFrame->DidReflow(aPresContext, nullptr, nsDidReflowStatus::FINISHED);

  return desiredSize.BSize(wm);
}

nscoord
nsTableRowFrame::CollapseRowIfNecessary(nscoord aRowOffset,
                                        nscoord aISize,
                                        bool    aCollapseGroup,
                                        bool&   aDidCollapse)
{
  const nsStyleVisibility* rowVis = StyleVisibility();
  bool collapseRow = (NS_STYLE_VISIBILITY_COLLAPSE == rowVis->mVisible);
  nsTableFrame* tableFrame =
    static_cast<nsTableFrame*>(GetTableFrame()->FirstInFlow());
  if (collapseRow) {
    tableFrame->SetNeedToCollapse(true);
  }

  if (aRowOffset != 0) {
    
    InvalidateFrameSubtree();
  }

  WritingMode wm = GetWritingMode();

  nscoord parentWidth = GetParent()->GetRect().width;
  LogicalRect rowRect = GetLogicalRect(wm, parentWidth);
  nsRect oldRect = mRect;
  nsRect oldVisualOverflow = GetVisualOverflowRect();

  rowRect.BStart(wm) -= aRowOffset;
  rowRect.ISize(wm)  = aISize;
  nsOverflowAreas overflow;
  nscoord shift = 0;
  nscoord containerWidth = mRect.width;

  if (aCollapseGroup || collapseRow) {
    aDidCollapse = true;
    shift = rowRect.BSize(wm);
    nsTableCellFrame* cellFrame = GetFirstCell();
    if (cellFrame) {
      int32_t rowIndex;
      cellFrame->GetRowIndex(rowIndex);
      shift += tableFrame->GetRowSpacing(rowIndex);
      while (cellFrame) {
        LogicalRect cRect = cellFrame->GetLogicalRect(wm, containerWidth);
        
        
        
        
        if (aRowOffset == 0) {
          InvalidateFrame();
        }
        cRect.BSize(wm) = 0;
        cellFrame->SetRect(wm, cRect, containerWidth);
        cellFrame = cellFrame->GetNextCell();
      }
    } else {
      shift += tableFrame->GetRowSpacing(GetRowIndex());
    }
    rowRect.BSize(wm) = 0;
  }
  else { 
    nsTableIterator iter(*this);
    
    
    int32_t firstPrevColIndex = -1;
    int32_t prevColIndex  = firstPrevColIndex;
    nscoord iPos = 0; 

    int32_t colIncrement = 1;

    nsIFrame* kidFrame = iter.First();
    while (kidFrame) {
      nsTableCellFrame *cellFrame = do_QueryFrame(kidFrame);
      if (cellFrame) {
        int32_t cellColIndex;
        cellFrame->GetColIndex(cellColIndex);
        int32_t cellColSpan = tableFrame->GetEffectiveColSpan(*cellFrame);

        
        
        if (prevColIndex != (cellColIndex - 1)) {
          iPos += GetSpaceBetween(prevColIndex, cellColIndex, cellColSpan,
                                  *tableFrame, true);
        }
        LogicalRect cRect(wm, iPos, 0, 0, rowRect.BSize(wm));

        
        prevColIndex = cellColIndex + cellColSpan - 1;
        int32_t startIndex = cellColIndex;
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
            cRect.ISize(wm) += tableFrame->GetColumnISize(colX);
            isVisible = true;
            if ((actualColSpan > 1)) {
              nsTableColFrame* nextColFrame =
                tableFrame->GetColFrame(colX + colIncrement);
              const nsStyleVisibility* nextColVis =
              nextColFrame->StyleVisibility();
              if ( (NS_STYLE_VISIBILITY_COLLAPSE != nextColVis->mVisible) &&
                  tableFrame->ColumnHasCellSpacingBefore(colX + colIncrement)) {
                cRect.ISize(wm) += tableFrame->GetColSpacing(cellColIndex);
              }
            }
          }
        }
        iPos += cRect.ISize(wm);
        if (isVisible) {
          iPos += tableFrame->GetColSpacing(cellColIndex);
        }
        int32_t actualRowSpan = tableFrame->GetEffectiveRowSpan(*cellFrame);
        nsTableRowFrame* rowFrame = GetNextRow();
        for (actualRowSpan--; actualRowSpan > 0 && rowFrame; actualRowSpan--) {
          const nsStyleVisibility* nextRowVis = rowFrame->StyleVisibility();
          bool collapseNextRow = (NS_STYLE_VISIBILITY_COLLAPSE ==
                                    nextRowVis->mVisible);
          if (!collapseNextRow) {
            LogicalRect nextRect = rowFrame->GetLogicalRect(wm,
                                                            containerWidth);
            cRect.BSize(wm) +=
              nextRect.BSize(wm) +
              tableFrame->GetRowSpacing(rowFrame->GetRowIndex());
          }
          rowFrame = rowFrame->GetNextRow();
        }

        nsRect oldCellRect = cellFrame->GetRect();
        LogicalPoint oldCellNormalPos =
          cellFrame->GetLogicalNormalPosition(wm, containerWidth);

        nsRect oldCellVisualOverflow = cellFrame->GetVisualOverflowRect();

        if (aRowOffset == 0 && cRect.Origin(wm) != oldCellNormalPos) {
          
          cellFrame->InvalidateFrameSubtree();
        }

        cellFrame->MovePositionBy(wm, cRect.Origin(wm) - oldCellNormalPos);
        cellFrame->SetSize(wm, cRect.Size(wm));

        
        
        LogicalRect cellBounds(wm, 0, 0, cRect.ISize(wm), cRect.BSize(wm));
        nsRect cellPhysicalBounds =
          cellBounds.GetPhysicalRect(wm, containerWidth);
        nsOverflowAreas cellOverflow(cellPhysicalBounds, cellPhysicalBounds);
        cellFrame->FinishAndStoreOverflow(cellOverflow,
                                          cRect.Size(wm).GetPhysicalSize(wm));
        nsTableFrame::RePositionViews(cellFrame);
        ConsiderChildOverflow(overflow, cellFrame);

        if (aRowOffset == 0) {
          nsTableFrame::InvalidateTableFrame(cellFrame, oldCellRect,
                                             oldCellVisualOverflow, false);
        }
      }
      kidFrame = iter.Next(); 
    }
  }

  SetRect(wm, rowRect, containerWidth);
  overflow.UnionAllWith(nsRect(0, 0, rowRect.Width(wm), rowRect.Height(wm)));
  FinishAndStoreOverflow(overflow, rowRect.Size(wm).GetPhysicalSize(wm));

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
nsTableRowFrame::SetUnpaginatedBSize(nsPresContext* aPresContext,
                                     nscoord        aValue)
{
  NS_ASSERTION(!GetPrevInFlow(), "program error");
  
  aPresContext->PropertyTable()->
    Set(this, RowUnpaginatedHeightProperty(), NS_INT32_TO_PTR(aValue));
}

nscoord
nsTableRowFrame::GetUnpaginatedBSize(nsPresContext* aPresContext)
{
  FrameProperties props = FirstInFlow()->Properties();
  return NS_PTR_TO_INT32(props.Get(RowUnpaginatedHeightProperty()));
}

void nsTableRowFrame::SetContinuousBCBorderWidth(LogicalSide aForSide,
                                                 BCPixelSize aPixelValue)
{
  switch (aForSide) {
    case eLogicalSideIEnd:
      mIEndContBorderWidth = aPixelValue;
      return;
    case eLogicalSideBStart:
      mBStartContBorderWidth = aPixelValue;
      return;
    case eLogicalSideIStart:
      mIStartContBorderWidth = aPixelValue;
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





void nsTableRowFrame::InitHasCellWithStyleBSize(nsTableFrame* aTableFrame)
{
  nsTableIterator iter(*this);
  WritingMode wm = GetWritingMode();

  for (nsIFrame* kidFrame = iter.First(); kidFrame; kidFrame = iter.Next()) {
    nsTableCellFrame *cellFrame = do_QueryFrame(kidFrame);
    if (!cellFrame) {
      NS_NOTREACHED("Table row has a non-cell child.");
      continue;
    }
    
    const nsStyleCoord &cellBSize = cellFrame->StylePosition()->BSize(wm);
    if (aTableFrame->GetEffectiveRowSpan(*cellFrame) == 1 &&
        cellBSize.GetUnit() != eStyleUnit_Auto &&
         
        (!cellBSize.IsCalcUnit() || !cellBSize.HasPercent())) {
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
