






#include "nsColumnSetFrame.h"
#include "nsCSSRendering.h"
#include "nsDisplayList.h"

using namespace mozilla;
using namespace mozilla::layout;









nsContainerFrame*
NS_NewColumnSetFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, nsFrameState aStateFlags)
{
  nsColumnSetFrame* it = new (aPresShell) nsColumnSetFrame(aContext);
  it->AddStateBits(aStateFlags | NS_BLOCK_MARGIN_ROOT);
  return it;
}

NS_IMPL_FRAMEARENA_HELPERS(nsColumnSetFrame)

nsColumnSetFrame::nsColumnSetFrame(nsStyleContext* aContext)
  : nsContainerFrame(aContext), mLastBalanceBSize(NS_INTRINSICSIZE),
    mLastFrameStatus(NS_FRAME_COMPLETE)
{
}

nsIAtom*
nsColumnSetFrame::GetType() const
{
  return nsGkAtoms::columnSetFrame;
}

static void
PaintColumnRule(nsIFrame* aFrame, nsRenderingContext* aCtx,
                const nsRect& aDirtyRect, nsPoint aPt)
{
  static_cast<nsColumnSetFrame*>(aFrame)->PaintColumnRule(aCtx, aDirtyRect, aPt);
}

void
nsColumnSetFrame::PaintColumnRule(nsRenderingContext* aCtx,
                                  const nsRect& aDirtyRect,
                                  const nsPoint& aPt)
{
  nsIFrame* child = mFrames.FirstChild();
  if (!child)
    return;  

  nsIFrame* nextSibling = child->GetNextSibling();
  if (!nextSibling)
    return;  

  bool isRTL = StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
  const nsStyleColumn* colStyle = StyleColumn();

  uint8_t ruleStyle;
  
  if (colStyle->mColumnRuleStyle == NS_STYLE_BORDER_STYLE_INSET)
    ruleStyle = NS_STYLE_BORDER_STYLE_RIDGE;
  else if (colStyle->mColumnRuleStyle == NS_STYLE_BORDER_STYLE_OUTSET)
    ruleStyle = NS_STYLE_BORDER_STYLE_GROOVE;
  else
    ruleStyle = colStyle->mColumnRuleStyle;

  nsPresContext* presContext = PresContext();
  nscoord ruleWidth = colStyle->GetComputedColumnRuleWidth();
  if (!ruleWidth)
    return;

  nscolor ruleColor =
    GetVisitedDependentColor(eCSSProperty__moz_column_rule_color);

  
  
  
  
  
  nsStyleBorder border(presContext);
  border.SetBorderWidth(NS_SIDE_LEFT, ruleWidth);
  border.SetBorderStyle(NS_SIDE_LEFT, ruleStyle);
  border.SetBorderColor(NS_SIDE_LEFT, ruleColor);

  
  
  nsRect contentRect = GetContentRect() - GetRect().TopLeft() + aPt;
  nsSize ruleSize(ruleWidth, contentRect.height);

  while (nextSibling) {
    
    nsIFrame* leftSibling = isRTL ? nextSibling : child;
    nsIFrame* rightSibling = isRTL ? child : nextSibling;

    
    
    nsPoint edgeOfLeftSibling = leftSibling->GetRect().TopRight() + aPt;
    nsPoint edgeOfRightSibling = rightSibling->GetRect().TopLeft() + aPt;
    nsPoint linePt((edgeOfLeftSibling.x + edgeOfRightSibling.x - ruleWidth) / 2,
                   contentRect.y);

    nsRect lineRect(linePt, ruleSize);
    
    Sides skipSides(mozilla::eSideBitsTopBottom);
    skipSides |= mozilla::eSideBitsRight;
    nsCSSRendering::PaintBorderWithStyleBorder(presContext, *aCtx, this,
        aDirtyRect, lineRect, border, StyleContext(),
        skipSides);

    child = nextSibling;
    nextSibling = nextSibling->GetNextSibling();
  }
}

static nscoord
GetAvailableContentISize(const nsHTMLReflowState& aReflowState)
{
  if (aReflowState.AvailableWidth() == NS_INTRINSICSIZE) {
    return NS_INTRINSICSIZE;
  }
  nscoord borderPaddingWidth =
    aReflowState.ComputedPhysicalBorderPadding().left +
    aReflowState.ComputedPhysicalBorderPadding().right;
  return std::max(0, aReflowState.AvailableWidth() - borderPaddingWidth);
}

nscoord
nsColumnSetFrame::GetAvailableContentBSize(const nsHTMLReflowState& aReflowState)
{
  if (aReflowState.AvailableHeight() == NS_INTRINSICSIZE) {
    return NS_INTRINSICSIZE;
  }

  nsMargin bp = aReflowState.ComputedPhysicalBorderPadding();
  bp.ApplySkipSides(GetSkipSides(&aReflowState));
  bp.bottom = aReflowState.ComputedPhysicalBorderPadding().bottom;
  return std::max(0, aReflowState.AvailableHeight() - bp.TopBottom());
}

static nscoord
GetColumnGap(nsColumnSetFrame*    aFrame,
             const nsStyleColumn* aColStyle)
{
  if (eStyleUnit_Normal == aColStyle->mColumnGap.GetUnit())
    return aFrame->StyleFont()->mFont.size;
  if (eStyleUnit_Coord == aColStyle->mColumnGap.GetUnit()) {
    nscoord colGap = aColStyle->mColumnGap.GetCoordValue();
    NS_ASSERTION(colGap >= 0, "negative column gap");
    return colGap;
  }

  NS_NOTREACHED("Unknown gap type");
  return 0;
}

nsColumnSetFrame::ReflowConfig
nsColumnSetFrame::ChooseColumnStrategy(const nsHTMLReflowState& aReflowState,
                                       bool aForceAuto = false,
                                       nscoord aFeasibleBSize = NS_INTRINSICSIZE,
                                       nscoord aInfeasibleBSize = 0)
{
  nscoord knownFeasibleBSize = aFeasibleBSize;
  nscoord knownInfeasibleBSize = aInfeasibleBSize;

  const nsStyleColumn* colStyle = StyleColumn();
  nscoord availContentISize = GetAvailableContentISize(aReflowState);
  if (aReflowState.ComputedWidth() != NS_INTRINSICSIZE) {
    availContentISize = aReflowState.ComputedWidth();
  }

  nscoord consumedBSize = GetConsumedBSize();

  
  
  
  nscoord computedBSize = GetEffectiveComputedBSize(aReflowState,
                                                    consumedBSize);
  nscoord colBSize = GetAvailableContentBSize(aReflowState);

  if (aReflowState.ComputedHeight() != NS_INTRINSICSIZE) {
    colBSize = aReflowState.ComputedHeight();
  } else if (aReflowState.ComputedMaxHeight() != NS_INTRINSICSIZE) {
    colBSize = std::min(colBSize, aReflowState.ComputedMaxHeight());
  }

  nscoord colGap = GetColumnGap(this, colStyle);
  int32_t numColumns = colStyle->mColumnCount;

  
  const bool isBalancing = colStyle->mColumnFill == NS_STYLE_COLUMN_FILL_BALANCE
                           && !aForceAuto;
  if (isBalancing) {
    const uint32_t MAX_NESTED_COLUMN_BALANCING = 2;
    uint32_t cnt = 0;
    for (const nsHTMLReflowState* rs = aReflowState.parentReflowState;
         rs && cnt < MAX_NESTED_COLUMN_BALANCING; rs = rs->parentReflowState) {
      if (rs->mFlags.mIsColumnBalancing) {
        ++cnt;
      }
    }
    if (cnt == MAX_NESTED_COLUMN_BALANCING) {
      numColumns = 1;
    }
  }

  nscoord colISize;
  
  
  if (colStyle->mColumnWidth.GetUnit() == eStyleUnit_Coord) {
    colISize = colStyle->mColumnWidth.GetCoordValue();
    NS_ASSERTION(colISize >= 0, "negative column width");
    
    
    
    
    if (availContentISize != NS_INTRINSICSIZE && colGap + colISize > 0
        && numColumns > 0) {
      
      
      int32_t maxColumns =
        std::min(nscoord(nsStyleColumn::kMaxColumnCount),
                 (availContentISize + colGap) / (colGap + colISize));
      numColumns = std::max(1, std::min(numColumns, maxColumns));
    }
  } else if (numColumns > 0 && availContentISize != NS_INTRINSICSIZE) {
    nscoord iSizeMinusGaps = availContentISize - colGap * (numColumns - 1);
    colISize = iSizeMinusGaps / numColumns;
  } else {
    colISize = NS_INTRINSICSIZE;
  }
  
  
  colISize = std::max(1, std::min(colISize, availContentISize));

  nscoord expectedISizeLeftOver = 0;

  if (colISize != NS_INTRINSICSIZE && availContentISize != NS_INTRINSICSIZE) {
    

    
    
    if (numColumns <= 0) {
      
      
      
      if (colGap + colISize > 0) {
        numColumns = (availContentISize + colGap) / (colGap + colISize);
        
        numColumns = std::min(nscoord(nsStyleColumn::kMaxColumnCount),
                              numColumns);
      }
      if (numColumns <= 0) {
        numColumns = 1;
      }
    }

    
    nscoord extraSpace =
      std::max(0, availContentISize - (colISize * numColumns +
                                       colGap * (numColumns - 1)));
    nscoord extraToColumns = extraSpace / numColumns;
    colISize += extraToColumns;
    expectedISizeLeftOver = extraSpace - (extraToColumns * numColumns);
  }

  if (isBalancing) {
    if (numColumns <= 0) {
      
      
      numColumns = 1;
    }
    colBSize = std::min(mLastBalanceBSize, colBSize);
  } else {
    
    
    numColumns = INT32_MAX;

    
    
    
    
    
    
    
    
    colBSize = std::max(colBSize, nsPresContext::CSSPixelsToAppUnits(1));
  }

#ifdef DEBUG_roc
  printf("*** nsColumnSetFrame::ChooseColumnStrategy: numColumns=%d, colISize=%d,"
         " expectedISizeLeftOver=%d, colBSize=%d, colGap=%d\n",
         numColumns, colISize, expectedISizeLeftOver, colBSize, colGap);
#endif
  ReflowConfig config = { numColumns, colISize, expectedISizeLeftOver, colGap,
                          colBSize, isBalancing, knownFeasibleBSize,
                          knownInfeasibleBSize, computedBSize, consumedBSize };
  return config;
}

bool
nsColumnSetFrame::ReflowColumns(nsHTMLReflowMetrics& aDesiredSize,
                                const nsHTMLReflowState& aReflowState,
                                nsReflowStatus& aReflowStatus,
                                ReflowConfig& aConfig,
                                bool aLastColumnUnbounded,
                                nsCollapsingMargin* aCarriedOutBEndMargin,
                                ColumnBalanceData& aColData)
{
  bool feasible = ReflowChildren(aDesiredSize, aReflowState,
                                 aReflowStatus, aConfig, aLastColumnUnbounded,
                                 aCarriedOutBEndMargin, aColData);

  if (aColData.mHasExcessBSize) {
    aConfig = ChooseColumnStrategy(aReflowState, true);

    
    
    
    feasible = ReflowChildren(aDesiredSize, aReflowState, aReflowStatus,
                              aConfig, aLastColumnUnbounded,
                              aCarriedOutBEndMargin, aColData);
  }

  return feasible;
}

static void MoveChildTo(nsIFrame* aParent, nsIFrame* aChild, nsPoint aOrigin)
{
  if (aChild->GetPosition() == aOrigin) {
    return;
  }

  aChild->SetPosition(aOrigin);
  nsContainerFrame::PlaceFrameView(aChild);
}

nscoord
nsColumnSetFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nscoord width = 0;
  DISPLAY_MIN_WIDTH(this, width);
  if (mFrames.FirstChild()) {
    width = mFrames.FirstChild()->GetMinISize(aRenderingContext);
  }
  const nsStyleColumn* colStyle = StyleColumn();
  nscoord colISize;
  if (colStyle->mColumnWidth.GetUnit() == eStyleUnit_Coord) {
    colISize = colStyle->mColumnWidth.GetCoordValue();
    
    
    
    width = std::min(width, colISize);
  } else {
    NS_ASSERTION(colStyle->mColumnCount > 0,
                 "column-count and column-width can't both be auto");
    
    
    colISize = width;
    width *= colStyle->mColumnCount;
    
    
    width = std::max(width, colISize);
  }
  
  
  return width;
}

nscoord
nsColumnSetFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  
  
  
  
  nscoord result = 0;
  DISPLAY_PREF_WIDTH(this, result);
  const nsStyleColumn* colStyle = StyleColumn();
  nscoord colGap = GetColumnGap(this, colStyle);

  nscoord colISize;
  if (colStyle->mColumnWidth.GetUnit() == eStyleUnit_Coord) {
    colISize = colStyle->mColumnWidth.GetCoordValue();
  } else if (mFrames.FirstChild()) {
    colISize = mFrames.FirstChild()->GetPrefISize(aRenderingContext);
  } else {
    colISize = 0;
  }

  int32_t numColumns = colStyle->mColumnCount;
  if (numColumns <= 0) {
    
    numColumns = 1;
  }
  
  nscoord width = colISize*numColumns + colGap*(numColumns - 1);
  
  
  result = std::max(width, colISize);
  return result;
}

bool
nsColumnSetFrame::ReflowChildren(nsHTMLReflowMetrics&     aDesiredSize,
                                 const nsHTMLReflowState& aReflowState,
                                 nsReflowStatus&          aStatus,
                                 const ReflowConfig&      aConfig,
                                 bool                     aUnboundedLastColumn,
                                 nsCollapsingMargin*      aCarriedOutBEndMargin,
                                 ColumnBalanceData&       aColData)
{
  aColData.Reset();
  bool allFit = true;
  bool RTL = StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
  bool shrinkingBSizeOnly = !NS_SUBTREE_DIRTY(this) &&
    mLastBalanceBSize > aConfig.mColMaxBSize;
  
#ifdef DEBUG_roc
  printf("*** Doing column reflow pass: mLastBalanceBSize=%d, mColMaxBSize=%d, RTL=%d\n, mBalanceColCount=%d, mColISize=%d, mColGap=%d\n",
         mLastBalanceBSize, aConfig.mColMaxBSize, RTL, aConfig.mBalanceColCount,
         aConfig.mColISize, aConfig.mColGap);
#endif

  DrainOverflowColumns();

  const bool colBSizeChanged = mLastBalanceBSize != aConfig.mColMaxBSize;

  if (colBSizeChanged) {
    mLastBalanceBSize = aConfig.mColMaxBSize;
    
    
    
    
    
    
    
  }

  
  nsMargin borderPadding = aReflowState.ComputedPhysicalBorderPadding();
  borderPadding.ApplySkipSides(GetSkipSides(&aReflowState));
  
  nsRect contentRect(0, 0, 0, 0);
  nsOverflowAreas overflowRects;

  nsIFrame* child = mFrames.FirstChild();
  nsPoint childOrigin = nsPoint(borderPadding.left, borderPadding.top);
  
  
  
  
  if (RTL) {
    nscoord availISize = aReflowState.AvailableWidth();
    if (aReflowState.ComputedWidth() != NS_INTRINSICSIZE) {
      availISize = aReflowState.ComputedWidth();
    }
    if (availISize != NS_INTRINSICSIZE) {
      childOrigin.x += availISize - aConfig.mColISize;
#ifdef DEBUG_roc
      printf("*** childOrigin.x = %d\n", childOrigin.x);
#endif
    }
  }
  int columnCount = 0;
  int contentBEnd = 0;
  bool reflowNext = false;

  while (child) {
    
    
    
    
    
    
    
    
    
    
    bool skipIncremental = !aReflowState.ShouldReflowAllKids()
      && !NS_SUBTREE_DIRTY(child)
      && child->GetNextSibling()
      && !(aUnboundedLastColumn && columnCount == aConfig.mBalanceColCount - 1)
      && !NS_SUBTREE_DIRTY(child->GetNextSibling());
    
    
    
    
    
    
    
    
    bool skipResizeBSizeShrink = shrinkingBSizeOnly
      && child->GetScrollableOverflowRect().YMost() <= aConfig.mColMaxBSize;

    nscoord childContentBEnd = 0;
    WritingMode wm = child->GetWritingMode();
    if (!reflowNext && (skipIncremental || skipResizeBSizeShrink)) {
      
      MoveChildTo(this, child, childOrigin);
      
      
      nsIFrame* kidNext = child->GetNextSibling();
      if (kidNext) {
        aStatus = (kidNext->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)
                  ? NS_FRAME_OVERFLOW_INCOMPLETE
                  : NS_FRAME_NOT_COMPLETE;
      } else {
        aStatus = mLastFrameStatus;
      }
      childContentBEnd = nsLayoutUtils::CalculateContentBEnd(wm, child);
#ifdef DEBUG_roc
      printf("*** Skipping child #%d %p (incremental %d, resize height shrink %d): status = %d\n",
             columnCount, (void*)child, skipIncremental, skipResizeBSizeShrink, aStatus);
#endif
    } else {
      nsSize physicalSize(aConfig.mColISize, aConfig.mColMaxBSize);

      if (aUnboundedLastColumn && columnCount == aConfig.mBalanceColCount - 1) {
        physicalSize.height = GetAvailableContentBSize(aReflowState);
      }
      LogicalSize availSize(wm, physicalSize);
      LogicalSize computedSize = aReflowState.ComputedSize(wm);

      if (reflowNext)
        child->AddStateBits(NS_FRAME_IS_DIRTY);

      nsHTMLReflowState kidReflowState(PresContext(), aReflowState, child,
                                       availSize, availSize.ISize(wm),
                                       computedSize.BSize(wm));
      kidReflowState.mFlags.mIsTopOfPage = true;
      kidReflowState.mFlags.mTableIsSplittable = false;
      kidReflowState.mFlags.mIsColumnBalancing = aConfig.mBalanceColCount < INT32_MAX;

      
      
      kidReflowState.mFlags.mMustReflowPlaceholders = !colBSizeChanged;

#ifdef DEBUG_roc
      printf("*** Reflowing child #%d %p: availHeight=%d\n",
             columnCount, (void*)child,availSize.BSize(wm));
#endif

      
      
      if (child->GetNextSibling() &&
          !(GetStateBits() & NS_FRAME_IS_DIRTY) &&
        !(child->GetNextSibling()->GetStateBits() & NS_FRAME_IS_DIRTY)) {
        kidReflowState.mFlags.mNextInFlowUntouched = true;
      }

      nsHTMLReflowMetrics kidDesiredSize(wm, aDesiredSize.mFlags);

      
      
      
      
      
      

      
      ReflowChild(child, PresContext(), kidDesiredSize, kidReflowState,
                  childOrigin.x + kidReflowState.ComputedPhysicalMargin().left,
                  childOrigin.y + kidReflowState.ComputedPhysicalMargin().top,
                  0, aStatus);

      reflowNext = (aStatus & NS_FRAME_REFLOW_NEXTINFLOW) != 0;
    
#ifdef DEBUG_roc
      printf("*** Reflowed child #%d %p: status = %d, desiredSize=%d,%d CarriedOutBottomMargin=%d\n",
             columnCount, (void*)child, aStatus, kidDesiredSize.Width(), kidDesiredSize.Height(),
             kidDesiredSize.mCarriedOutBEndMargin.get());
#endif

      NS_FRAME_TRACE_REFLOW_OUT("Column::Reflow", aStatus);

      *aCarriedOutBEndMargin = kidDesiredSize.mCarriedOutBEndMargin;
      
      FinishReflowChild(child, PresContext(), kidDesiredSize,
                        &kidReflowState, childOrigin.x, childOrigin.y, 0);

      childContentBEnd = nsLayoutUtils::CalculateContentBEnd(wm, child);
      if (childContentBEnd > aConfig.mColMaxBSize) {
        allFit = false;
      }
      if (childContentBEnd > availSize.BSize(wm)) {
        aColData.mMaxOverflowingBSize = std::max(childContentBEnd,
            aColData.mMaxOverflowingBSize);
      }
    }

    contentRect.UnionRect(contentRect, child->GetRect());

    ConsiderChildOverflow(overflowRects, child);
    contentBEnd = std::max(contentBEnd, childContentBEnd);
    aColData.mLastBSize = childContentBEnd;
    aColData.mSumBSize += childContentBEnd;

    
    nsIFrame* kidNextInFlow = child->GetNextInFlow();

    if (NS_FRAME_IS_FULLY_COMPLETE(aStatus) && !NS_FRAME_IS_TRUNCATED(aStatus)) {
      NS_ASSERTION(!kidNextInFlow, "next in flow should have been deleted");
      child = nullptr;
      break;
    } else {
      ++columnCount;
      
      
      
      
      if (!kidNextInFlow) {
        NS_ASSERTION(aStatus & NS_FRAME_REFLOW_NEXTINFLOW,
                     "We have to create a continuation, but the block doesn't want us to reflow it?");

        
        kidNextInFlow = CreateNextInFlow(child);
      }

      
      
      if (NS_FRAME_OVERFLOW_IS_INCOMPLETE(aStatus)) {
        if (!(kidNextInFlow->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)) {
          aStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
          reflowNext = true;
          kidNextInFlow->AddStateBits(NS_FRAME_IS_OVERFLOW_CONTAINER);
        }
      }
      else if (kidNextInFlow->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER) {
        aStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
        reflowNext = true;
        kidNextInFlow->RemoveStateBits(NS_FRAME_IS_OVERFLOW_CONTAINER);
      }

      if ((contentBEnd > aReflowState.ComputedMaxBSize() ||
           contentBEnd > aReflowState.ComputedBSize()) &&
           aConfig.mBalanceColCount < INT32_MAX) {
        
        
        
        aColData.mHasExcessBSize = true;
      }

      if (columnCount >= aConfig.mBalanceColCount) {
        
        aStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
        kidNextInFlow->AddStateBits(NS_FRAME_IS_DIRTY);
        
        
        const nsFrameList& continuationColumns = mFrames.RemoveFramesAfter(child);
        if (continuationColumns.NotEmpty()) {
          SetOverflowFrames(continuationColumns);
        }
        child = nullptr;
        break;
      }
    }

    if (PresContext()->HasPendingInterrupt()) {
      
      
      
      
      
      
      break;
    }

    
    child = child->GetNextSibling();

    if (child) {
      if (!RTL) {
        childOrigin.x += aConfig.mColISize + aConfig.mColGap;
      } else {
        childOrigin.x -= aConfig.mColISize + aConfig.mColGap;
      }
      
#ifdef DEBUG_roc
      printf("*** NEXT CHILD ORIGIN.x = %d\n", childOrigin.x);
#endif
    }
  }

  if (PresContext()->CheckForInterrupt(this) &&
      (GetStateBits() & NS_FRAME_IS_DIRTY)) {
    

    
    
    
    
    
    
    for (; child; child = child->GetNextSibling()) {
      child->AddStateBits(NS_FRAME_IS_DIRTY);
    }
  }
  
  aColData.mMaxBSize = contentBEnd;
  contentRect.height = std::max(contentRect.height, contentBEnd);
  mLastFrameStatus = aStatus;
  
  
  contentRect -= nsPoint(borderPadding.left, borderPadding.top);

  WritingMode wm = aReflowState.GetWritingMode();
  LogicalSize contentSize(wm, nsSize(contentRect.XMost(), contentRect.YMost()));

  
  
  if (aConfig.mComputedBSize != NS_INTRINSICSIZE) {
    if (aReflowState.AvailableHeight() != NS_INTRINSICSIZE) {
      contentSize.BSize(wm) = std::min(contentSize.BSize(wm),
                                       aConfig.mComputedBSize);
    } else {
      contentSize.BSize(wm) = aConfig.mComputedBSize;
    }
  } else {
    
    
    
    
    
    contentSize.BSize(wm) =
      aReflowState.ApplyMinMaxHeight(contentSize.BSize(wm),
                                     aConfig.mConsumedBSize);
  }
  if (aReflowState.ComputedISize() != NS_INTRINSICSIZE) {
    contentSize.ISize(wm) = aReflowState.ComputedISize();
  } else {
    contentSize.ISize(wm) =
      aReflowState.ApplyMinMaxWidth(contentSize.ISize(wm));
  }

  LogicalMargin bp(wm, borderPadding);
  contentSize.ISize(wm) += bp.IStartEnd(wm);
  contentSize.BSize(wm) += bp.BStartEnd(wm);
  aDesiredSize.SetSize(wm, contentSize);
  aDesiredSize.mOverflowAreas = overflowRects;
  aDesiredSize.UnionOverflowAreasWithDesiredBounds();

#ifdef DEBUG_roc
  printf("*** DONE PASS feasible=%d\n", allFit && NS_FRAME_IS_FULLY_COMPLETE(aStatus)
         && !NS_FRAME_IS_TRUNCATED(aStatus));
#endif
  return allFit && NS_FRAME_IS_FULLY_COMPLETE(aStatus)
    && !NS_FRAME_IS_TRUNCATED(aStatus);
}

void
nsColumnSetFrame::DrainOverflowColumns()
{
  
  
  nsPresContext* presContext = PresContext();
  nsColumnSetFrame* prev = static_cast<nsColumnSetFrame*>(GetPrevInFlow());
  if (prev) {
    AutoFrameListPtr overflows(presContext, prev->StealOverflowFrames());
    if (overflows) {
      nsContainerFrame::ReparentFrameViewList(*overflows, prev, this);

      mFrames.InsertFrames(this, nullptr, *overflows);
    }
  }
  
  
  
  AutoFrameListPtr overflows(presContext, StealOverflowFrames());
  if (overflows) {
    
    
    mFrames.AppendFrames(nullptr, *overflows);
  }
}

void
nsColumnSetFrame::FindBestBalanceBSize(const nsHTMLReflowState& aReflowState,
                                       nsPresContext* aPresContext,
                                       ReflowConfig& aConfig,
                                       ColumnBalanceData& aColData,
                                       nsHTMLReflowMetrics& aDesiredSize,
                                       nsCollapsingMargin& aOutMargin,
                                       bool& aUnboundedLastColumn,
                                       bool& aRunWasFeasible,
                                       nsReflowStatus& aStatus)
{
  bool feasible = aRunWasFeasible;

  nsMargin bp = aReflowState.ComputedPhysicalBorderPadding();
  bp.ApplySkipSides(GetSkipSides());
  bp.bottom = aReflowState.ComputedPhysicalBorderPadding().bottom;

  nscoord availableContentBSize =
    GetAvailableContentBSize(aReflowState);

  
  
  

  
  
  
  bool maybeContinuousBreakingDetected = false;

  while (!aPresContext->HasPendingInterrupt()) {
    nscoord lastKnownFeasibleBSize = aConfig.mKnownFeasibleBSize;

    
    if (feasible) {
      
      aConfig.mKnownFeasibleBSize = std::min(aConfig.mKnownFeasibleBSize,
                                              aColData.mMaxBSize);
      aConfig.mKnownFeasibleBSize = std::min(aConfig.mKnownFeasibleBSize,
                                              mLastBalanceBSize);

      
      
      
      
      if (mFrames.GetLength() == aConfig.mBalanceColCount) {
        aConfig.mKnownInfeasibleBSize = std::max(aConfig.mKnownInfeasibleBSize,
                                       aColData.mLastBSize - 1);
      }
    } else {
      aConfig.mKnownInfeasibleBSize = std::max(aConfig.mKnownInfeasibleBSize,
                                                mLastBalanceBSize);
      
      
      
      aConfig.mKnownInfeasibleBSize = std::max(aConfig.mKnownInfeasibleBSize,
                                         aColData.mMaxOverflowingBSize - 1);

      if (aUnboundedLastColumn) {
        
        
        aConfig.mKnownFeasibleBSize = std::min(aConfig.mKnownFeasibleBSize,
                                                aColData.mMaxBSize);
      }
    }

#ifdef DEBUG_roc
    printf("*** nsColumnSetFrame::Reflow balancing knownInfeasible=%d knownFeasible=%d\n",
           aConfig.mKnownInfeasibleBSize, aConfig.mKnownFeasibleBSize);
#endif


    if (aConfig.mKnownInfeasibleBSize >= aConfig.mKnownFeasibleBSize - 1) {
      
      break;
    }

    if (aConfig.mKnownInfeasibleBSize >= availableContentBSize) {
      break;
    }

    if (lastKnownFeasibleBSize - aConfig.mKnownFeasibleBSize == 1) {
      
      
      
      maybeContinuousBreakingDetected = true;
    }

    nscoord nextGuess = (aConfig.mKnownFeasibleBSize + aConfig.mKnownInfeasibleBSize)/2;
    
    if (aConfig.mKnownFeasibleBSize - nextGuess < 600 &&
        !maybeContinuousBreakingDetected) {
      
      
      
      nextGuess = aConfig.mKnownFeasibleBSize - 1;
    } else if (aUnboundedLastColumn) {
      
      
      
      nextGuess = aColData.mSumBSize/aConfig.mBalanceColCount + 600;
      
      nextGuess = clamped(nextGuess, aConfig.mKnownInfeasibleBSize + 1,
                                     aConfig.mKnownFeasibleBSize - 1);
    } else if (aConfig.mKnownFeasibleBSize == NS_INTRINSICSIZE) {
      
      
      
      nextGuess = aConfig.mKnownInfeasibleBSize*2 + 600;
    }
    
    nextGuess = std::min(availableContentBSize, nextGuess);

#ifdef DEBUG_roc
    printf("*** nsColumnSetFrame::Reflow balancing choosing next guess=%d\n", nextGuess);
#endif

    aConfig.mColMaxBSize = nextGuess;

    aUnboundedLastColumn = false;
    AddStateBits(NS_FRAME_IS_DIRTY);
    feasible = ReflowColumns(aDesiredSize, aReflowState, aStatus, aConfig, false,
                             &aOutMargin, aColData);

    if (!aConfig.mIsBalancing) {
      
      
      break;
    }
  }

  if (aConfig.mIsBalancing && !feasible &&
      !aPresContext->HasPendingInterrupt()) {
    
    
    bool skip = false;
    if (aConfig.mKnownInfeasibleBSize >= availableContentBSize) {
      aConfig.mColMaxBSize = availableContentBSize;
      if (mLastBalanceBSize == availableContentBSize) {
        skip = true;
      }
    } else {
      aConfig.mColMaxBSize = aConfig.mKnownFeasibleBSize;
    }
    if (!skip) {
      
      
      
      
      AddStateBits(NS_FRAME_IS_DIRTY);
      feasible = ReflowColumns(aDesiredSize, aReflowState, aStatus, aConfig,
                               availableContentBSize == NS_UNCONSTRAINEDSIZE,
                               &aOutMargin, aColData);
    }
  }

  aRunWasFeasible = feasible;
}

void
nsColumnSetFrame::Reflow(nsPresContext*           aPresContext,
                         nsHTMLReflowMetrics&     aDesiredSize,
                         const nsHTMLReflowState& aReflowState,
                         nsReflowStatus&          aStatus)
{
  
  nsPresContext::InterruptPreventer noInterrupts(aPresContext);

  DO_GLOBAL_REFLOW_COUNT("nsColumnSetFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  
  aStatus = NS_FRAME_COMPLETE;

  
  if (aReflowState.ComputedHeight() != NS_AUTOHEIGHT) {
    NS_ASSERTION(aReflowState.ComputedHeight() != NS_INTRINSICSIZE,
                 "Unexpected computed height");
    AddStateBits(NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
  }
  else {
    RemoveStateBits(NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
  }

#ifdef DEBUG
  nsFrameList::Enumerator oc(GetChildList(kOverflowContainersList));
  for (; !oc.AtEnd(); oc.Next()) {
    MOZ_ASSERT(!IS_TRUE_OVERFLOW_CONTAINER(oc.get()));
  }
  nsFrameList::Enumerator eoc(GetChildList(kExcessOverflowContainersList));
  for (; !eoc.AtEnd(); eoc.Next()) {
    MOZ_ASSERT(!IS_TRUE_OVERFLOW_CONTAINER(eoc.get()));
  }
#endif

  nsOverflowAreas ocBounds;
  nsReflowStatus ocStatus = NS_FRAME_COMPLETE;
  if (GetPrevInFlow()) {
    ReflowOverflowContainerChildren(aPresContext, aReflowState, ocBounds, 0,
                                    ocStatus);
  }

  

  ReflowConfig config = ChooseColumnStrategy(aReflowState);
  
  
  
  
  
  
  
  nsIFrame* nextInFlow = GetNextInFlow();
  bool unboundedLastColumn = config.mIsBalancing && !nextInFlow;
  nsCollapsingMargin carriedOutBottomMargin;
  ColumnBalanceData colData;
  colData.mHasExcessBSize = false;

  bool feasible = ReflowColumns(aDesiredSize, aReflowState, aStatus, config,
                                unboundedLastColumn, &carriedOutBottomMargin,
                                colData);

  
  
  
  if (config.mIsBalancing && !aPresContext->HasPendingInterrupt()) {
    FindBestBalanceBSize(aReflowState, aPresContext, config, colData,
                          aDesiredSize, carriedOutBottomMargin,
                          unboundedLastColumn, feasible, aStatus);
  }

  if (aPresContext->HasPendingInterrupt() &&
      aReflowState.AvailableHeight() == NS_UNCONSTRAINEDSIZE) {
    
    
    aStatus = NS_FRAME_COMPLETE;
  }

  NS_ASSERTION(NS_FRAME_IS_FULLY_COMPLETE(aStatus) ||
               aReflowState.AvailableHeight() != NS_UNCONSTRAINEDSIZE,
               "Column set should be complete if the available height is unconstrained");

  
  aDesiredSize.mOverflowAreas.UnionWith(ocBounds);
  NS_MergeReflowStatusInto(&aStatus, ocStatus);

  FinishReflowWithAbsoluteFrames(aPresContext, aDesiredSize, aReflowState, aStatus, false);

  aDesiredSize.mCarriedOutBEndMargin = carriedOutBottomMargin;

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

void
nsColumnSetFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                   const nsRect&           aDirtyRect,
                                   const nsDisplayListSet& aLists)
{
  DisplayBorderBackgroundOutline(aBuilder, aLists);

  if (IsVisibleForPainting(aBuilder)) {
    aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
      nsDisplayGenericOverflow(aBuilder, this, ::PaintColumnRule, "ColumnRule",
                               nsDisplayItem::TYPE_COLUMN_RULE));
  }

  
  for (nsFrameList::Enumerator e(mFrames); !e.AtEnd(); e.Next()) {
    BuildDisplayListForChild(aBuilder, e.get(), aDirtyRect, aLists);
  }
}

#ifdef DEBUG
void
nsColumnSetFrame::SetInitialChildList(ChildListID     aListID,
                                      nsFrameList&    aChildList)
{
  MOZ_ASSERT(aListID == kPrincipalList, "unexpected child list");
  MOZ_ASSERT(aChildList.OnlyChild(),
             "initial child list must have exactly one child");
  nsContainerFrame::SetInitialChildList(kPrincipalList, aChildList);
}

void
nsColumnSetFrame::AppendFrames(ChildListID     aListID,
                               nsFrameList&    aFrameList)
{
  MOZ_CRASH("unsupported operation");
}

void
nsColumnSetFrame::InsertFrames(ChildListID     aListID,
                               nsIFrame*       aPrevFrame,
                               nsFrameList&    aFrameList)
{
  MOZ_CRASH("unsupported operation");
}

void
nsColumnSetFrame::RemoveFrame(ChildListID     aListID,
                              nsIFrame*       aOldFrame)
{
  MOZ_CRASH("unsupported operation");
}
#endif
