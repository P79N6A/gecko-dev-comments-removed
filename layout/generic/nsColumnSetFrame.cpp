






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
  : nsContainerFrame(aContext), mLastBalanceHeight(NS_INTRINSICSIZE),
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
    nsCSSRendering::PaintBorderWithStyleBorder(presContext, *aCtx, this,
        aDirtyRect, lineRect, border, StyleContext(),
        
        (1 << NS_SIDE_TOP | 1 << NS_SIDE_RIGHT | 1 << NS_SIDE_BOTTOM));

    child = nextSibling;
    nextSibling = nextSibling->GetNextSibling();
  }
}

nsresult
nsColumnSetFrame::SetInitialChildList(ChildListID     aListID,
                                      nsFrameList&    aChildList)
{
  if (aListID == kAbsoluteList) {
    return nsContainerFrame::SetInitialChildList(aListID, aChildList);
  }

  NS_ASSERTION(aListID == kPrincipalList,
               "Only default child list supported");
  NS_ASSERTION(aChildList.OnlyChild(),
               "initial child list must have exaisRevertingctly one child");
  
  return nsContainerFrame::SetInitialChildList(kPrincipalList, aChildList);
}

static nscoord
GetAvailableContentWidth(const nsHTMLReflowState& aReflowState)
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
nsColumnSetFrame::GetAvailableContentHeight(const nsHTMLReflowState& aReflowState)
{
  if (aReflowState.AvailableHeight() == NS_INTRINSICSIZE) {
    return NS_INTRINSICSIZE;
  }

  nsMargin bp = aReflowState.ComputedPhysicalBorderPadding();
  ApplySkipSides(bp, &aReflowState);
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
                                       nscoord aFeasibleHeight = NS_INTRINSICSIZE,
                                       nscoord aInfeasibleHeight = 0)

{
  nscoord knownFeasibleHeight = aFeasibleHeight;
  nscoord knownInfeasibleHeight = aInfeasibleHeight;

  const nsStyleColumn* colStyle = StyleColumn();
  nscoord availContentWidth = GetAvailableContentWidth(aReflowState);
  if (aReflowState.ComputedWidth() != NS_INTRINSICSIZE) {
    availContentWidth = aReflowState.ComputedWidth();
  }

  nscoord consumedHeight = GetConsumedHeight();

  
  
  
  nscoord computedHeight = GetEffectiveComputedHeight(aReflowState,
                                                      consumedHeight);
  nscoord colHeight = GetAvailableContentHeight(aReflowState);

  if (aReflowState.ComputedHeight() != NS_INTRINSICSIZE) {
    colHeight = aReflowState.ComputedHeight();
  } else if (aReflowState.ComputedMaxHeight() != NS_INTRINSICSIZE) {
    colHeight = std::min(colHeight, aReflowState.ComputedMaxHeight());
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

  nscoord colWidth;
  if (colStyle->mColumnWidth.GetUnit() == eStyleUnit_Coord) {
    colWidth = colStyle->mColumnWidth.GetCoordValue();
    NS_ASSERTION(colWidth >= 0, "negative column width");
    
    
    
    
    if (availContentWidth != NS_INTRINSICSIZE && colGap + colWidth > 0
        && numColumns > 0) {
      
      
      int32_t maxColumns =
        std::min(nscoord(nsStyleColumn::kMaxColumnCount),
                 (availContentWidth + colGap)/(colGap + colWidth));
      numColumns = std::max(1, std::min(numColumns, maxColumns));
    }
  } else if (numColumns > 0 && availContentWidth != NS_INTRINSICSIZE) {
    nscoord widthMinusGaps = availContentWidth - colGap*(numColumns - 1);
    colWidth = widthMinusGaps/numColumns;
  } else {
    colWidth = NS_INTRINSICSIZE;
  }
  
  
  colWidth = std::max(1, std::min(colWidth, availContentWidth));

  nscoord expectedWidthLeftOver = 0;

  if (colWidth != NS_INTRINSICSIZE && availContentWidth != NS_INTRINSICSIZE) {
    

    
    
    if (numColumns <= 0) {
      
      
      
      if (colGap + colWidth > 0) {
        numColumns = (availContentWidth + colGap)/(colGap + colWidth);
        
        numColumns = std::min(nscoord(nsStyleColumn::kMaxColumnCount),
                              numColumns);
      }
      if (numColumns <= 0) {
        numColumns = 1;
      }
    }

    
    nscoord extraSpace =
      std::max(0, availContentWidth - (colWidth*numColumns + colGap*(numColumns - 1)));
    nscoord extraToColumns = extraSpace/numColumns;
    colWidth += extraToColumns;
    expectedWidthLeftOver = extraSpace - (extraToColumns*numColumns);
  }

  if (isBalancing) {
    if (numColumns <= 0) {
      
      
      numColumns = 1;
    }
    colHeight = std::min(mLastBalanceHeight, colHeight);
  } else {
    
    
    numColumns = INT32_MAX;

    
    
    
    
    
    
    
    
    colHeight = std::max(colHeight, nsPresContext::CSSPixelsToAppUnits(1));
  }

#ifdef DEBUG_roc
  printf("*** nsColumnSetFrame::ChooseColumnStrategy: numColumns=%d, colWidth=%d, expectedWidthLeftOver=%d, colHeight=%d, colGap=%d\n",
         numColumns, colWidth, expectedWidthLeftOver, colHeight, colGap);
#endif
  ReflowConfig config = { numColumns, colWidth, expectedWidthLeftOver, colGap,
                          colHeight, isBalancing, knownFeasibleHeight,
                          knownInfeasibleHeight, computedHeight, consumedHeight };
  return config;
}

bool
nsColumnSetFrame::ReflowColumns(nsHTMLReflowMetrics& aDesiredSize,
                                const nsHTMLReflowState& aReflowState,
                                nsReflowStatus& aReflowStatus,
                                ReflowConfig& aConfig,
                                bool aLastColumnUnbounded,
                                nsCollapsingMargin* aCarriedOutBottomMargin,
                                ColumnBalanceData& aColData)
{
  bool feasible = ReflowChildren(aDesiredSize, aReflowState,
                                 aReflowStatus, aConfig, aLastColumnUnbounded,
                                 aCarriedOutBottomMargin, aColData);

  if (aColData.mHasExcessHeight) {
    aConfig = ChooseColumnStrategy(aReflowState, true);

    
    
    
    feasible = ReflowChildren(aDesiredSize, aReflowState, aReflowStatus,
                              aConfig, aLastColumnUnbounded,
                              aCarriedOutBottomMargin, aColData);
  }

  return feasible;
}

static void MoveChildTo(nsIFrame* aParent, nsIFrame* aChild, nsPoint aOrigin) {
  if (aChild->GetPosition() == aOrigin) {
    return;
  }

  aChild->SetPosition(aOrigin);
  nsContainerFrame::PlaceFrameView(aChild);
}

nscoord
nsColumnSetFrame::GetMinWidth(nsRenderingContext *aRenderingContext) {
  nscoord width = 0;
  DISPLAY_MIN_WIDTH(this, width);
  if (mFrames.FirstChild()) {
    width = mFrames.FirstChild()->GetMinWidth(aRenderingContext);
  }
  const nsStyleColumn* colStyle = StyleColumn();
  nscoord colWidth;
  if (colStyle->mColumnWidth.GetUnit() == eStyleUnit_Coord) {
    colWidth = colStyle->mColumnWidth.GetCoordValue();
    
    
    
    width = std::min(width, colWidth);
  } else {
    NS_ASSERTION(colStyle->mColumnCount > 0,
                 "column-count and column-width can't both be auto");
    
    
    colWidth = width;
    width *= colStyle->mColumnCount;
    
    
    width = std::max(width, colWidth);
  }
  
  
  return width;
}

nscoord
nsColumnSetFrame::GetPrefWidth(nsRenderingContext *aRenderingContext) {
  
  
  
  
  nscoord result = 0;
  DISPLAY_PREF_WIDTH(this, result);
  const nsStyleColumn* colStyle = StyleColumn();
  nscoord colGap = GetColumnGap(this, colStyle);

  nscoord colWidth;
  if (colStyle->mColumnWidth.GetUnit() == eStyleUnit_Coord) {
    colWidth = colStyle->mColumnWidth.GetCoordValue();
  } else if (mFrames.FirstChild()) {
    colWidth = mFrames.FirstChild()->GetPrefWidth(aRenderingContext);
  } else {
    colWidth = 0;
  }

  int32_t numColumns = colStyle->mColumnCount;
  if (numColumns <= 0) {
    
    numColumns = 1;
  }
  
  nscoord width = colWidth*numColumns + colGap*(numColumns - 1);
  
  
  result = std::max(width, colWidth);
  return result;
}

bool
nsColumnSetFrame::ReflowChildren(nsHTMLReflowMetrics&     aDesiredSize,
                                 const nsHTMLReflowState& aReflowState,
                                 nsReflowStatus&          aStatus,
                                 const ReflowConfig&      aConfig,
                                 bool                     aUnboundedLastColumn,
                                 nsCollapsingMargin*      aBottomMarginCarriedOut,
                                 ColumnBalanceData&       aColData)
{
  aColData.Reset();
  bool allFit = true;
  bool RTL = StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
  bool shrinkingHeightOnly = !NS_SUBTREE_DIRTY(this) &&
    mLastBalanceHeight > aConfig.mColMaxHeight;
  
#ifdef DEBUG_roc
  printf("*** Doing column reflow pass: mLastBalanceHeight=%d, mColMaxHeight=%d, RTL=%d\n, mBalanceColCount=%d, mColWidth=%d, mColGap=%d\n",
         mLastBalanceHeight, aConfig.mColMaxHeight, RTL, aConfig.mBalanceColCount,
         aConfig.mColWidth, aConfig.mColGap);
#endif

  DrainOverflowColumns();

  const bool colHeightChanged = mLastBalanceHeight != aConfig.mColMaxHeight;

  if (colHeightChanged) {
    mLastBalanceHeight = aConfig.mColMaxHeight;
    
    
    
    
    
    
    
  }

  
  nsMargin borderPadding = aReflowState.ComputedPhysicalBorderPadding();
  ApplySkipSides(borderPadding, &aReflowState);
  
  nsRect contentRect(0, 0, 0, 0);
  nsOverflowAreas overflowRects;

  nsIFrame* child = mFrames.FirstChild();
  nsPoint childOrigin = nsPoint(borderPadding.left, borderPadding.top);
  
  
  
  
  if (RTL) {
    nscoord availWidth = aReflowState.AvailableWidth();
    if (aReflowState.ComputedWidth() != NS_INTRINSICSIZE) {
      availWidth = aReflowState.ComputedWidth();
    }
    if (availWidth != NS_INTRINSICSIZE) {
      childOrigin.x += availWidth - aConfig.mColWidth;
#ifdef DEBUG_roc
      printf("*** childOrigin.x = %d\n", childOrigin.x);
#endif
    }
  }
  int columnCount = 0;
  int contentBottom = 0;
  bool reflowNext = false;

  while (child) {
    
    
    
    
    
    
    
    
    
    
    bool skipIncremental = !aReflowState.ShouldReflowAllKids()
      && !NS_SUBTREE_DIRTY(child)
      && child->GetNextSibling()
      && !(aUnboundedLastColumn && columnCount == aConfig.mBalanceColCount - 1)
      && !NS_SUBTREE_DIRTY(child->GetNextSibling());
    
    
    
    
    
    
    
    
    bool skipResizeHeightShrink = shrinkingHeightOnly
      && child->GetScrollableOverflowRect().YMost() <= aConfig.mColMaxHeight;

    nscoord childContentBottom = 0;
    if (!reflowNext && (skipIncremental || skipResizeHeightShrink)) {
      
      MoveChildTo(this, child, childOrigin);
      
      
      nsIFrame* kidNext = child->GetNextSibling();
      if (kidNext) {
        aStatus = (kidNext->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)
                  ? NS_FRAME_OVERFLOW_INCOMPLETE
                  : NS_FRAME_NOT_COMPLETE;
      } else {
        aStatus = mLastFrameStatus;
      }
      childContentBottom = nsLayoutUtils::CalculateContentBottom(child);
#ifdef DEBUG_roc
      printf("*** Skipping child #%d %p (incremental %d, resize height shrink %d): status = %d\n",
             columnCount, (void*)child, skipIncremental, skipResizeHeightShrink, aStatus);
#endif
    } else {
      nsSize availSize(aConfig.mColWidth, aConfig.mColMaxHeight);
      
      if (aUnboundedLastColumn && columnCount == aConfig.mBalanceColCount - 1) {
        availSize.height = GetAvailableContentHeight(aReflowState);
      }

      if (reflowNext)
        child->AddStateBits(NS_FRAME_IS_DIRTY);

      nsHTMLReflowState kidReflowState(PresContext(), aReflowState, child,
                                       availSize, availSize.width,
                                       aReflowState.ComputedHeight());
      kidReflowState.mFlags.mIsTopOfPage = true;
      kidReflowState.mFlags.mTableIsSplittable = false;
      kidReflowState.mFlags.mIsColumnBalancing = aConfig.mBalanceColCount < INT32_MAX;

      
      
      kidReflowState.mFlags.mMustReflowPlaceholders = !colHeightChanged;

#ifdef DEBUG_roc
      printf("*** Reflowing child #%d %p: availHeight=%d\n",
             columnCount, (void*)child,availSize.height);
#endif

      
      
      if (child->GetNextSibling() &&
          !(GetStateBits() & NS_FRAME_IS_DIRTY) &&
        !(child->GetNextSibling()->GetStateBits() & NS_FRAME_IS_DIRTY)) {
        kidReflowState.mFlags.mNextInFlowUntouched = true;
      }
    
      nsHTMLReflowMetrics kidDesiredSize(aReflowState.GetWritingMode(),
                                         aDesiredSize.mFlags);

      
      
      
      
      
      

      
      ReflowChild(child, PresContext(), kidDesiredSize, kidReflowState,
                  childOrigin.x + kidReflowState.ComputedPhysicalMargin().left,
                  childOrigin.y + kidReflowState.ComputedPhysicalMargin().top,
                  0, aStatus);

      reflowNext = (aStatus & NS_FRAME_REFLOW_NEXTINFLOW) != 0;
    
#ifdef DEBUG_roc
      printf("*** Reflowed child #%d %p: status = %d, desiredSize=%d,%d CarriedOutBottomMargin=%d\n",
             columnCount, (void*)child, aStatus, kidDesiredSize.Width(), kidDesiredSize.Height(),
             kidDesiredSize.mCarriedOutBottomMargin.get());
#endif

      NS_FRAME_TRACE_REFLOW_OUT("Column::Reflow", aStatus);

      *aBottomMarginCarriedOut = kidDesiredSize.mCarriedOutBottomMargin;
      
      FinishReflowChild(child, PresContext(), kidDesiredSize,
                        &kidReflowState, childOrigin.x, childOrigin.y, 0);

      childContentBottom = nsLayoutUtils::CalculateContentBottom(child);
      if (childContentBottom > aConfig.mColMaxHeight) {
        allFit = false;
      }
      if (childContentBottom > availSize.height) {
        aColData.mMaxOverflowingHeight = std::max(childContentBottom,
            aColData.mMaxOverflowingHeight);
      }
    }

    contentRect.UnionRect(contentRect, child->GetRect());

    ConsiderChildOverflow(overflowRects, child);
    contentBottom = std::max(contentBottom, childContentBottom);
    aColData.mLastHeight = childContentBottom;
    aColData.mSumHeight += childContentBottom;

    
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

        
        nsresult rv = CreateNextInFlow(child, kidNextInFlow);
        
        if (NS_FAILED(rv)) {
          NS_NOTREACHED("Couldn't create continuation");
          child = nullptr;
          break;
        }
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

      if ((contentBottom > aReflowState.ComputedMaxHeight() ||
           contentBottom > aReflowState.ComputedHeight()) &&
           aConfig.mBalanceColCount < INT32_MAX) {
        
        
        
        aColData.mHasExcessHeight = true;
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
        childOrigin.x += aConfig.mColWidth + aConfig.mColGap;
      } else {
        childOrigin.x -= aConfig.mColWidth + aConfig.mColGap;
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
  
  aColData.mMaxHeight = contentBottom;
  contentRect.height = std::max(contentRect.height, contentBottom);
  mLastFrameStatus = aStatus;
  
  
  contentRect -= nsPoint(borderPadding.left, borderPadding.top);
  
  nsSize contentSize = nsSize(contentRect.XMost(), contentRect.YMost());

  
  if (aConfig.mComputedHeight != NS_INTRINSICSIZE) {
    if (aReflowState.AvailableHeight() != NS_INTRINSICSIZE) {
      contentSize.height = std::min(contentSize.height,
                                    aConfig.mComputedHeight);
    } else {
      contentSize.height = aConfig.mComputedHeight;
    }
  } else {
    
    
    
    
    
    contentSize.height = aReflowState.ApplyMinMaxHeight(contentSize.height,
                                                        aConfig.mConsumedHeight);
  }
  if (aReflowState.ComputedWidth() != NS_INTRINSICSIZE) {
    contentSize.width = aReflowState.ComputedWidth();
  } else {
    contentSize.width = aReflowState.ApplyMinMaxWidth(contentSize.width);
  }

  aDesiredSize.Height() = contentSize.height +
                        borderPadding.TopBottom();
  aDesiredSize.Width() = contentSize.width +
                       borderPadding.LeftRight();
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
nsColumnSetFrame::FindBestBalanceHeight(const nsHTMLReflowState& aReflowState,
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
  ApplySkipSides(bp);
  bp.bottom = aReflowState.ComputedPhysicalBorderPadding().bottom;

  nscoord availableContentHeight =
    GetAvailableContentHeight(aReflowState);

  
  
  

  
  
  
  bool maybeContinuousBreakingDetected = false;

  while (!aPresContext->HasPendingInterrupt()) {
    nscoord lastKnownFeasibleHeight = aConfig.mKnownFeasibleHeight;

    
    if (feasible) {
      
      aConfig.mKnownFeasibleHeight = std::min(aConfig.mKnownFeasibleHeight,
                                              aColData.mMaxHeight);
      aConfig.mKnownFeasibleHeight = std::min(aConfig.mKnownFeasibleHeight,
                                              mLastBalanceHeight);

      
      
      
      
      if (mFrames.GetLength() == aConfig.mBalanceColCount) {
        aConfig.mKnownInfeasibleHeight = std::max(aConfig.mKnownInfeasibleHeight,
                                       aColData.mLastHeight - 1);
      }
    } else {
      aConfig.mKnownInfeasibleHeight = std::max(aConfig.mKnownInfeasibleHeight,
                                                mLastBalanceHeight);
      
      
      
      aConfig.mKnownInfeasibleHeight = std::max(aConfig.mKnownInfeasibleHeight,
                                         aColData.mMaxOverflowingHeight - 1);

      if (aUnboundedLastColumn) {
        
        
        aConfig.mKnownFeasibleHeight = std::min(aConfig.mKnownFeasibleHeight,
                                                aColData.mMaxHeight);
      }
    }

#ifdef DEBUG_roc
    printf("*** nsColumnSetFrame::Reflow balancing knownInfeasible=%d knownFeasible=%d\n",
           aConfig.mKnownInfeasibleHeight, aConfig.mKnownFeasibleHeight);
#endif


    if (aConfig.mKnownInfeasibleHeight >= aConfig.mKnownFeasibleHeight - 1) {
      
      break;
    }

    if (aConfig.mKnownInfeasibleHeight >= availableContentHeight) {
      break;
    }

    if (lastKnownFeasibleHeight - aConfig.mKnownFeasibleHeight == 1) {
      
      
      
      maybeContinuousBreakingDetected = true;
    }

    nscoord nextGuess = (aConfig.mKnownFeasibleHeight + aConfig.mKnownInfeasibleHeight)/2;
    
    if (aConfig.mKnownFeasibleHeight - nextGuess < 600 &&
        !maybeContinuousBreakingDetected) {
      
      
      
      nextGuess = aConfig.mKnownFeasibleHeight - 1;
    } else if (aUnboundedLastColumn) {
      
      
      
      nextGuess = aColData.mSumHeight/aConfig.mBalanceColCount + 600;
      
      nextGuess = clamped(nextGuess, aConfig.mKnownInfeasibleHeight + 1,
                                     aConfig.mKnownFeasibleHeight - 1);
    } else if (aConfig.mKnownFeasibleHeight == NS_INTRINSICSIZE) {
      
      
      
      nextGuess = aConfig.mKnownInfeasibleHeight*2 + 600;
    }
    
    nextGuess = std::min(availableContentHeight, nextGuess);

#ifdef DEBUG_roc
    printf("*** nsColumnSetFrame::Reflow balancing choosing next guess=%d\n", nextGuess);
#endif

    aConfig.mColMaxHeight = nextGuess;

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
    if (aConfig.mKnownInfeasibleHeight >= availableContentHeight) {
      aConfig.mColMaxHeight = availableContentHeight;
      if (mLastBalanceHeight == availableContentHeight) {
        skip = true;
      }
    } else {
      aConfig.mColMaxHeight = aConfig.mKnownFeasibleHeight;
    }
    if (!skip) {
      
      
      
      
      AddStateBits(NS_FRAME_IS_DIRTY);
      feasible = ReflowColumns(aDesiredSize, aReflowState, aStatus, aConfig,
                               availableContentHeight == NS_UNCONSTRAINEDSIZE,
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

  

  ReflowConfig config = ChooseColumnStrategy(aReflowState);
  
  
  
  
  
  
  
  nsIFrame* nextInFlow = GetNextInFlow();
  bool unboundedLastColumn = config.mIsBalancing && !nextInFlow;
  nsCollapsingMargin carriedOutBottomMargin;
  ColumnBalanceData colData;
  colData.mHasExcessHeight = false;

  bool feasible = ReflowColumns(aDesiredSize, aReflowState, aStatus, config,
                                unboundedLastColumn, &carriedOutBottomMargin,
                                colData);

  
  
  
  if (config.mIsBalancing && !aPresContext->HasPendingInterrupt()) {
    FindBestBalanceHeight(aReflowState, aPresContext, config, colData,
                          aDesiredSize, carriedOutBottomMargin,
                          unboundedLastColumn, feasible, aStatus);
  }

  if (aPresContext->HasPendingInterrupt() &&
      aReflowState.AvailableHeight() == NS_UNCONSTRAINEDSIZE) {
    
    
    aStatus = NS_FRAME_COMPLETE;
  }

  FinishReflowWithAbsoluteFrames(aPresContext, aDesiredSize, aReflowState, aStatus, false);

  aDesiredSize.mCarriedOutBottomMargin = carriedOutBottomMargin;

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);

  NS_ASSERTION(NS_FRAME_IS_FULLY_COMPLETE(aStatus) ||
               aReflowState.AvailableHeight() != NS_UNCONSTRAINEDSIZE,
               "Column set should be complete if the available height is unconstrained");
}

void
nsColumnSetFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                   const nsRect&           aDirtyRect,
                                   const nsDisplayListSet& aLists) {
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

nsresult
nsColumnSetFrame::AppendFrames(ChildListID     aListID,
                               nsFrameList&    aFrameList)
{
  if (aListID == kAbsoluteList) {
    return nsContainerFrame::AppendFrames(aListID, aFrameList);
  }

  NS_ERROR("unexpected child list");
  return NS_ERROR_INVALID_ARG;
}

nsresult
nsColumnSetFrame::InsertFrames(ChildListID     aListID,
                               nsIFrame*       aPrevFrame,
                               nsFrameList&    aFrameList)
{
  if (aListID == kAbsoluteList) {
    return nsContainerFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
  }

  NS_ERROR("unexpected child list");
  return NS_ERROR_INVALID_ARG;
}

nsresult
nsColumnSetFrame::RemoveFrame(ChildListID     aListID,
                              nsIFrame*       aOldFrame)
{
  if (aListID == kAbsoluteList) {
    return nsContainerFrame::RemoveFrame(aListID, aOldFrame);
  }

  NS_ERROR("unexpected child list");
  return NS_ERROR_INVALID_ARG;
}
