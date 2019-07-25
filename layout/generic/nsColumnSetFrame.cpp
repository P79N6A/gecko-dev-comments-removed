







































#include "nsContainerFrame.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsISupports.h"
#include "nsIAtom.h"
#include "nsPresContext.h"
#include "nsHTMLParts.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsCOMPtr.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"
#include "nsCSSRendering.h"

using namespace mozilla;

class nsColumnSetFrame : public nsContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsColumnSetFrame(nsStyleContext* aContext);

  NS_IMETHOD SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList);

  NS_IMETHOD Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus);
                               
  NS_IMETHOD  AppendFrames(ChildListID     aListID,
                           nsFrameList&    aFrameList);
  NS_IMETHOD  InsertFrames(ChildListID     aListID,
                           nsIFrame*       aPrevFrame,
                           nsFrameList&    aFrameList);
  NS_IMETHOD  RemoveFrame(ChildListID     aListID,
                          nsIFrame*       aOldFrame);

  virtual void DestroyFrom(nsIFrame* aDestructRoot);
  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);  
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);

  virtual nsIFrame* GetContentInsertionFrame() {
    nsIFrame* frame = GetFirstPrincipalChild();

    
    if (!frame)
      return nsnull;

    return frame->GetContentInsertionFrame();
  }

  virtual nsresult StealFrame(nsPresContext* aPresContext,
                              nsIFrame*      aChild,
                              bool           aForceNormal)
  { 
    return nsContainerFrame::StealFrame(aPresContext, aChild, true);
  }

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  virtual nsIAtom* GetType() const;

  virtual void PaintColumnRule(nsRenderingContext* aCtx,
                               const nsRect&        aDirtyRect,
                               const nsPoint&       aPt);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("ColumnSet"), aResult);
  }
#endif

protected:
  nscoord        mLastBalanceHeight;
  nsReflowStatus mLastFrameStatus;

  virtual PRIntn GetSkipSides() const;

  


  struct ReflowConfig {
    PRInt32 mBalanceColCount;
    nscoord mColWidth;
    nscoord mExpectedWidthLeftOver;
    nscoord mColGap;
    nscoord mColMaxHeight;
  };

  


  struct ColumnBalanceData {
    
    nscoord mMaxHeight;
    
    nscoord mSumHeight;
    
    nscoord mLastHeight;
    
    
    nscoord mMaxOverflowingHeight;
    void Reset() {
      mMaxHeight = mSumHeight = mLastHeight = mMaxOverflowingHeight = 0;
    }
  };
  
  




  void DrainOverflowColumns();

  






  ReflowConfig ChooseColumnStrategy(const nsHTMLReflowState& aReflowState);

  



  bool ReflowChildren(nsHTMLReflowMetrics& aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus& aStatus,
                        const ReflowConfig& aConfig,
                        bool aLastColumnUnbounded,
                        nsCollapsingMargin* aCarriedOutBottomMargin,
                        ColumnBalanceData& aColData);
};









nsIFrame*
NS_NewColumnSetFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRUint32 aStateFlags)
{
  nsColumnSetFrame* it = new (aPresShell) nsColumnSetFrame(aContext);
  if (it) {
    
    it->AddStateBits(aStateFlags);
  }

  return it;
}

NS_IMPL_FRAMEARENA_HELPERS(nsColumnSetFrame)

nsColumnSetFrame::nsColumnSetFrame(nsStyleContext* aContext)
  : nsContainerFrame(aContext), mLastBalanceHeight(NS_INTRINSICSIZE),
    mLastFrameStatus(NS_FRAME_COMPLETE)
{
}

void
nsColumnSetFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  DestroyAbsoluteFrames(aDestructRoot);
  nsContainerFrame::DestroyFrom(aDestructRoot);
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

  bool isRTL = GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
  const nsStyleColumn* colStyle = GetStyleColumn();

  PRUint8 ruleStyle;
  
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
        aDirtyRect, lineRect, border, GetStyleContext(),
        
        (1 << NS_SIDE_TOP | 1 << NS_SIDE_RIGHT | 1 << NS_SIDE_BOTTOM));

    child = nextSibling;
    nextSibling = nextSibling->GetNextSibling();
  }
}

NS_IMETHODIMP
nsColumnSetFrame::SetInitialChildList(ChildListID     aListID,
                                      nsFrameList&    aChildList)
{
  NS_ASSERTION(aListID == kPrincipalList,
               "Only default child list supported");
  NS_ASSERTION(aChildList.OnlyChild(),
               "initial child list must have exactly one child");
  
  return nsContainerFrame::SetInitialChildList(kPrincipalList, aChildList);
}

static nscoord
GetAvailableContentWidth(const nsHTMLReflowState& aReflowState)
{
  if (aReflowState.availableWidth == NS_INTRINSICSIZE) {
    return NS_INTRINSICSIZE;
  }
  nscoord borderPaddingWidth =
    aReflowState.mComputedBorderPadding.left +
    aReflowState.mComputedBorderPadding.right;
  return NS_MAX(0, aReflowState.availableWidth - borderPaddingWidth);
}

static nscoord
GetAvailableContentHeight(const nsHTMLReflowState& aReflowState)
{
  if (aReflowState.availableHeight == NS_INTRINSICSIZE) {
    return NS_INTRINSICSIZE;
  }
  nscoord borderPaddingHeight =
    aReflowState.mComputedBorderPadding.top +
    aReflowState.mComputedBorderPadding.bottom;
  return NS_MAX(0, aReflowState.availableHeight - borderPaddingHeight);
}

static nscoord
GetColumnGap(nsColumnSetFrame*    aFrame,
             const nsStyleColumn* aColStyle)
{
  if (eStyleUnit_Normal == aColStyle->mColumnGap.GetUnit())
    return aFrame->GetStyleFont()->mFont.size;
  if (eStyleUnit_Coord == aColStyle->mColumnGap.GetUnit()) {
    nscoord colGap = aColStyle->mColumnGap.GetCoordValue();
    NS_ASSERTION(colGap >= 0, "negative column gap");
    return colGap;
  }

  NS_NOTREACHED("Unknown gap type");
  return 0;
}

nsColumnSetFrame::ReflowConfig
nsColumnSetFrame::ChooseColumnStrategy(const nsHTMLReflowState& aReflowState)
{
  const nsStyleColumn* colStyle = GetStyleColumn();
  nscoord availContentWidth = GetAvailableContentWidth(aReflowState);
  if (aReflowState.ComputedWidth() != NS_INTRINSICSIZE) {
    availContentWidth = aReflowState.ComputedWidth();
  }
  nscoord colHeight = GetAvailableContentHeight(aReflowState);
  if (aReflowState.ComputedHeight() != NS_INTRINSICSIZE) {
    colHeight = aReflowState.ComputedHeight();
  }

  nscoord colGap = GetColumnGap(this, colStyle);
  PRInt32 numColumns = colStyle->mColumnCount;

  nscoord colWidth;
  if (colStyle->mColumnWidth.GetUnit() == eStyleUnit_Coord) {
    colWidth = colStyle->mColumnWidth.GetCoordValue();
    NS_ASSERTION(colWidth >= 0, "negative column width");
    
    
    
    
    if (availContentWidth != NS_INTRINSICSIZE && colGap + colWidth > 0
        && numColumns > 0) {
      
      
      PRInt32 maxColumns = (availContentWidth + colGap)/(colGap + colWidth);
      numColumns = NS_MAX(1, NS_MIN(numColumns, maxColumns));
    }
  } else if (numColumns > 0 && availContentWidth != NS_INTRINSICSIZE) {
    nscoord widthMinusGaps = availContentWidth - colGap*(numColumns - 1);
    colWidth = widthMinusGaps/numColumns;
  } else {
    colWidth = NS_INTRINSICSIZE;
  }
  
  
  colWidth = NS_MAX(1, NS_MIN(colWidth, availContentWidth));

  nscoord expectedWidthLeftOver = 0;

  if (colWidth != NS_INTRINSICSIZE && availContentWidth != NS_INTRINSICSIZE) {
    

    
    
    if (numColumns <= 0) {
      
      
      
      if (colGap + colWidth > 0) {
        numColumns = (availContentWidth + colGap)/(colGap + colWidth);
      }
      if (numColumns <= 0) {
        numColumns = 1;
      }
    }

    
    nscoord extraSpace =
      NS_MAX(0, availContentWidth - (colWidth*numColumns + colGap*(numColumns - 1)));
    nscoord extraToColumns = extraSpace/numColumns;
    colWidth += extraToColumns;
    expectedWidthLeftOver = extraSpace - (extraToColumns*numColumns);
  }

  
  if (colStyle->mColumnFill == NS_STYLE_COLUMN_FILL_BALANCE) {
    

    if (numColumns <= 0) {
      
      
      numColumns = 1;
    }

    colHeight = NS_MIN(mLastBalanceHeight,
                       GetAvailableContentHeight(aReflowState));
  } else {
    
    

    numColumns = PR_INT32_MAX;
  }

#ifdef DEBUG_roc
  printf("*** nsColumnSetFrame::ChooseColumnStrategy: numColumns=%d, colWidth=%d, expectedWidthLeftOver=%d, colHeight=%d, colGap=%d\n",
         numColumns, colWidth, expectedWidthLeftOver, colHeight, colGap);
#endif
  ReflowConfig config = { numColumns, colWidth, expectedWidthLeftOver, colGap, colHeight };
  return config;
}


static void
PlaceFrameView(nsIFrame* aFrame)
{
  if (aFrame->HasView())
    nsContainerFrame::PositionFrameView(aFrame);
  else
    nsContainerFrame::PositionChildViews(aFrame);
}

static void MoveChildTo(nsIFrame* aParent, nsIFrame* aChild, nsPoint aOrigin) {
  if (aChild->GetPosition() == aOrigin) {
    return;
  }
  
  nsRect r = aChild->GetVisualOverflowRect();
  r += aChild->GetPosition();
  aParent->Invalidate(r);
  r -= aChild->GetPosition();
  aChild->SetPosition(aOrigin);
  r += aOrigin;
  aParent->Invalidate(r);
  PlaceFrameView(aChild);
}

nscoord
nsColumnSetFrame::GetMinWidth(nsRenderingContext *aRenderingContext) {
  nscoord width = 0;
  DISPLAY_MIN_WIDTH(this, width);
  if (mFrames.FirstChild()) {
    width = mFrames.FirstChild()->GetMinWidth(aRenderingContext);
  }
  const nsStyleColumn* colStyle = GetStyleColumn();
  nscoord colWidth;
  if (colStyle->mColumnWidth.GetUnit() == eStyleUnit_Coord) {
    colWidth = colStyle->mColumnWidth.GetCoordValue();
    
    
    
    width = NS_MIN(width, colWidth);
  } else {
    NS_ASSERTION(colStyle->mColumnCount > 0,
                 "column-count and column-width can't both be auto");
    
    
    colWidth = width;
    width *= colStyle->mColumnCount;
    
    
    width = NS_MAX(width, colWidth);
  }
  
  
  return width;
}

nscoord
nsColumnSetFrame::GetPrefWidth(nsRenderingContext *aRenderingContext) {
  
  
  
  
  nscoord result = 0;
  DISPLAY_PREF_WIDTH(this, result);
  const nsStyleColumn* colStyle = GetStyleColumn();
  nscoord colGap = GetColumnGap(this, colStyle);

  nscoord colWidth;
  if (colStyle->mColumnWidth.GetUnit() == eStyleUnit_Coord) {
    colWidth = colStyle->mColumnWidth.GetCoordValue();
  } else if (mFrames.FirstChild()) {
    colWidth = mFrames.FirstChild()->GetPrefWidth(aRenderingContext);
  } else {
    colWidth = 0;
  }

  PRInt32 numColumns = colStyle->mColumnCount;
  if (numColumns <= 0) {
    
    numColumns = 1;
  }
  
  nscoord width = colWidth*numColumns + colGap*(numColumns - 1);
  
  
  result = NS_MAX(width, colWidth);
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
  bool RTL = GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
  bool shrinkingHeightOnly = !NS_SUBTREE_DIRTY(this) &&
    mLastBalanceHeight > aConfig.mColMaxHeight;
  
#ifdef DEBUG_roc
  printf("*** Doing column reflow pass: mLastBalanceHeight=%d, mColMaxHeight=%d, RTL=%d\n, mBalanceColCount=%d, mColWidth=%d, mColGap=%d\n",
         mLastBalanceHeight, aConfig.mColMaxHeight, RTL, aConfig.mBalanceColCount,
         aConfig.mColWidth, aConfig.mColGap);
#endif

  DrainOverflowColumns();
  
  if (mLastBalanceHeight != aConfig.mColMaxHeight) {
    mLastBalanceHeight = aConfig.mColMaxHeight;
    
    
    
    
    
    
    
  }

  
  const nsMargin &borderPadding = aReflowState.mComputedBorderPadding;
  
  nsRect contentRect(0, 0, 0, 0);
  nsOverflowAreas overflowRects;

  nsIFrame* child = mFrames.FirstChild();
  nsPoint childOrigin = nsPoint(borderPadding.left, borderPadding.top);
  
  
  
  
  nscoord targetX = borderPadding.left;
  if (RTL) {
    nscoord availWidth = aReflowState.availableWidth;
    if (aReflowState.ComputedWidth() != NS_INTRINSICSIZE) {
      availWidth = aReflowState.ComputedWidth();
    }
    if (availWidth != NS_INTRINSICSIZE) {
      childOrigin.x += availWidth - aConfig.mColWidth;
      targetX += aConfig.mExpectedWidthLeftOver;
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
          
#ifdef DEBUG_roc
      printf("*** Reflowing child #%d %p: availHeight=%d\n",
             columnCount, (void*)child,availSize.height);
#endif

      
      
      if (child->GetNextSibling() &&
          !(GetStateBits() & NS_FRAME_IS_DIRTY) &&
        !(child->GetNextSibling()->GetStateBits() & NS_FRAME_IS_DIRTY)) {
        kidReflowState.mFlags.mNextInFlowUntouched = true;
      }
    
      nsHTMLReflowMetrics kidDesiredSize(aDesiredSize.mFlags);

      
      
      
      
      
      

      
      ReflowChild(child, PresContext(), kidDesiredSize, kidReflowState,
                  childOrigin.x + kidReflowState.mComputedMargin.left,
                  childOrigin.y + kidReflowState.mComputedMargin.top,
                  0, aStatus);

      reflowNext = (aStatus & NS_FRAME_REFLOW_NEXTINFLOW) != 0;
    
#ifdef DEBUG_roc
      printf("*** Reflowed child #%d %p: status = %d, desiredSize=%d,%d\n",
             columnCount, (void*)child, aStatus, kidDesiredSize.width, kidDesiredSize.height);
#endif

      NS_FRAME_TRACE_REFLOW_OUT("Column::Reflow", aStatus);

      *aBottomMarginCarriedOut = kidDesiredSize.mCarriedOutBottomMargin;
      
      FinishReflowChild(child, PresContext(), &kidReflowState, 
                        kidDesiredSize, childOrigin.x, childOrigin.y, 0);

      childContentBottom = nsLayoutUtils::CalculateContentBottom(child);
      if (childContentBottom > aConfig.mColMaxHeight) {
        allFit = false;
      }
      if (childContentBottom > availSize.height) {
        aColData.mMaxOverflowingHeight = NS_MAX(childContentBottom,
            aColData.mMaxOverflowingHeight);
      }
    }

    contentRect.UnionRect(contentRect, child->GetRect());

    ConsiderChildOverflow(overflowRects, child);
    contentBottom = NS_MAX(contentBottom, childContentBottom);
    aColData.mLastHeight = childContentBottom;
    aColData.mSumHeight += childContentBottom;

    
    nsIFrame* kidNextInFlow = child->GetNextInFlow();

    if (NS_FRAME_IS_FULLY_COMPLETE(aStatus) && !NS_FRAME_IS_TRUNCATED(aStatus)) {
      NS_ASSERTION(!kidNextInFlow, "next in flow should have been deleted");
      child = nsnull;
      break;
    } else {
      ++columnCount;
      
      
      
      
      if (!kidNextInFlow) {
        NS_ASSERTION(aStatus & NS_FRAME_REFLOW_NEXTINFLOW,
                     "We have to create a continuation, but the block doesn't want us to reflow it?");

        
        nsresult rv = CreateNextInFlow(PresContext(), child, kidNextInFlow);
        
        if (NS_FAILED(rv)) {
          NS_NOTREACHED("Couldn't create continuation");
          child = nsnull;
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
        
      if (columnCount >= aConfig.mBalanceColCount) {
        
        aStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
        kidNextInFlow->AddStateBits(NS_FRAME_IS_DIRTY);
        
        
        
        const nsFrameList& continuationColumns = mFrames.RemoveFramesAfter(child);
        if (continuationColumns.NotEmpty()) {
          SetOverflowFrames(PresContext(), continuationColumns);
        }
        child = nsnull;
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
  
  
  if (RTL && childOrigin.x != targetX) {
    overflowRects.Clear();
    contentRect = nsRect(0, 0, 0, 0);
    PRInt32 deltaX = targetX - childOrigin.x;
#ifdef DEBUG_roc
    printf("*** CHILDORIGIN.x = %d, targetX = %d, DELTAX = %d\n", childOrigin.x, targetX, deltaX);
#endif
    for (child = mFrames.FirstChild(); child; child = child->GetNextSibling()) {
      MoveChildTo(this, child, child->GetPosition() + nsPoint(deltaX, 0));
      ConsiderChildOverflow(overflowRects, child);
      contentRect.UnionRect(contentRect, child->GetRect());
    }
  }
  aColData.mMaxHeight = contentBottom;
  contentRect.height = NS_MAX(contentRect.height, contentBottom);
  mLastFrameStatus = aStatus;
  
  
  contentRect -= nsPoint(borderPadding.left, borderPadding.top);
  
  nsSize contentSize = nsSize(contentRect.XMost(), contentRect.YMost());

  
  if (aReflowState.ComputedHeight() != NS_INTRINSICSIZE) {
    contentSize.height = aReflowState.ComputedHeight();
  } else {
    if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMaxHeight) {
      contentSize.height = NS_MIN(aReflowState.mComputedMaxHeight, contentSize.height);
    }
    if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMinHeight) {
      contentSize.height = NS_MAX(aReflowState.mComputedMinHeight, contentSize.height);
    }
  }
  if (aReflowState.ComputedWidth() != NS_INTRINSICSIZE) {
    contentSize.width = aReflowState.ComputedWidth();
  } else {
    if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMaxWidth) {
      contentSize.width = NS_MIN(aReflowState.mComputedMaxWidth, contentSize.width);
    }
    if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMinWidth) {
      contentSize.width = NS_MAX(aReflowState.mComputedMinWidth, contentSize.width);
    }
  }
    
  aDesiredSize.height = borderPadding.top + contentSize.height +
    borderPadding.bottom;
  aDesiredSize.width = contentSize.width + borderPadding.left + borderPadding.right;
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
  
  
  nsColumnSetFrame* prev = static_cast<nsColumnSetFrame*>(GetPrevInFlow());
  if (prev) {
    nsAutoPtr<nsFrameList> overflows(prev->StealOverflowFrames());
    if (overflows) {
      nsContainerFrame::ReparentFrameViewList(PresContext(), *overflows,
                                              prev, this);

      mFrames.InsertFrames(this, nsnull, *overflows);
    }
  }
  
  
  
  nsAutoPtr<nsFrameList> overflows(StealOverflowFrames());
  if (overflows) {
    
    
    mFrames.AppendFrames(nsnull, *overflows);
  }
}

NS_IMETHODIMP 
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
                 "Unexpected mComputedHeight");
    AddStateBits(NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
  }
  else {
    RemoveStateBits(NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
  }

  

  ReflowConfig config = ChooseColumnStrategy(aReflowState);
  bool isBalancing = config.mBalanceColCount < PR_INT32_MAX;
  
  
  
  
  
  
  
  nsIFrame* nextInFlow = GetNextInFlow();
  bool unboundedLastColumn = isBalancing && !nextInFlow;
  nsCollapsingMargin carriedOutBottomMargin;
  ColumnBalanceData colData;
  bool feasible = ReflowChildren(aDesiredSize, aReflowState,
    aStatus, config, unboundedLastColumn, &carriedOutBottomMargin, colData);

  if (isBalancing && !aPresContext->HasPendingInterrupt()) {
    nscoord availableContentHeight = GetAvailableContentHeight(aReflowState);
  
    
    
    
    nscoord knownFeasibleHeight = NS_INTRINSICSIZE;
    nscoord knownInfeasibleHeight = 0;
    
    
    
    bool maybeContinuousBreakingDetected = false;

    while (!aPresContext->HasPendingInterrupt()) {
      nscoord lastKnownFeasibleHeight = knownFeasibleHeight;

      
      if (feasible) {
        
        knownFeasibleHeight = NS_MIN(knownFeasibleHeight, colData.mMaxHeight);
        knownFeasibleHeight = NS_MIN(knownFeasibleHeight, mLastBalanceHeight);

        
        
        
        
        if (mFrames.GetLength() == config.mBalanceColCount) {
          knownInfeasibleHeight = NS_MAX(knownInfeasibleHeight,
                                         colData.mLastHeight - 1);
        }
      } else {
        knownInfeasibleHeight = NS_MAX(knownInfeasibleHeight, mLastBalanceHeight);
        
        
        
        knownInfeasibleHeight = NS_MAX(knownInfeasibleHeight,
                                       colData.mMaxOverflowingHeight - 1);

        if (unboundedLastColumn) {
          
          
          knownFeasibleHeight = NS_MIN(knownFeasibleHeight,
                                       colData.mMaxHeight);
        }
      }

#ifdef DEBUG_roc
      printf("*** nsColumnSetFrame::Reflow balancing knownInfeasible=%d knownFeasible=%d\n",
             knownInfeasibleHeight, knownFeasibleHeight);
#endif

      if (knownInfeasibleHeight >= knownFeasibleHeight - 1) {
        
        break;
      }

      if (knownInfeasibleHeight >= availableContentHeight) {
        break;
      }

      if (lastKnownFeasibleHeight - knownFeasibleHeight == 1) {
        
        
        
        maybeContinuousBreakingDetected = true;
      }

      nscoord nextGuess = (knownFeasibleHeight + knownInfeasibleHeight)/2;
      
      if (knownFeasibleHeight - nextGuess < 600 &&
          !maybeContinuousBreakingDetected) {
        
        
        
        nextGuess = knownFeasibleHeight - 1;
      } else if (unboundedLastColumn) {
        
        
        
        nextGuess = colData.mSumHeight/config.mBalanceColCount + 600;
        
        nextGuess = clamped(nextGuess, knownInfeasibleHeight + 1,
                                       knownFeasibleHeight - 1);
      } else if (knownFeasibleHeight == NS_INTRINSICSIZE) {
        
        
        
        nextGuess = knownInfeasibleHeight*2 + 600;
      }
      
      nextGuess = NS_MIN(availableContentHeight, nextGuess);

#ifdef DEBUG_roc
      printf("*** nsColumnSetFrame::Reflow balancing choosing next guess=%d\n", nextGuess);
#endif

      config.mColMaxHeight = nextGuess;
      
      unboundedLastColumn = false;
      AddStateBits(NS_FRAME_IS_DIRTY);
      feasible = ReflowChildren(aDesiredSize, aReflowState,
                                aStatus, config, false, 
                                &carriedOutBottomMargin, colData);
    }

    if (!feasible && !aPresContext->HasPendingInterrupt()) {
      
      
      bool skip = false;
      if (knownInfeasibleHeight >= availableContentHeight) {
        config.mColMaxHeight = availableContentHeight;
        if (mLastBalanceHeight == availableContentHeight) {
          skip = true;
        }
      } else {
        config.mColMaxHeight = knownFeasibleHeight;
      }
      if (!skip) {
        
        
        
        
        AddStateBits(NS_FRAME_IS_DIRTY);
        ReflowChildren(aDesiredSize, aReflowState, aStatus, config,
                       availableContentHeight == NS_UNCONSTRAINEDSIZE,
                       &carriedOutBottomMargin, colData);
      }
    }
  }

  if (aPresContext->HasPendingInterrupt() &&
      aReflowState.availableHeight == NS_UNCONSTRAINEDSIZE) {
    
    
    aStatus = NS_FRAME_COMPLETE;
  }
  
  CheckInvalidateSizeChange(aDesiredSize);

  FinishReflowWithAbsoluteFrames(aPresContext, aDesiredSize, aReflowState, aStatus);
  aDesiredSize.mCarriedOutBottomMargin = carriedOutBottomMargin;

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);

  NS_ASSERTION(NS_FRAME_IS_FULLY_COMPLETE(aStatus) ||
               aReflowState.availableHeight != NS_UNCONSTRAINEDSIZE,
               "Column set should be complete if the available height is unconstrained");

  return NS_OK;
}

NS_IMETHODIMP
nsColumnSetFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                   const nsRect&           aDirtyRect,
                                   const nsDisplayListSet& aLists) {
  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
      nsDisplayGeneric(aBuilder, this, ::PaintColumnRule, "ColumnRule",
                       nsDisplayItem::TYPE_COLUMN_RULE));
  
  nsIFrame* kid = mFrames.FirstChild();
  
  while (kid) {
    nsresult rv = BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
    kid = kid->GetNextSibling();
  }
  return NS_OK;
}

PRIntn
nsColumnSetFrame::GetSkipSides() const
{
  return 0;
}

NS_IMETHODIMP
nsColumnSetFrame::AppendFrames(ChildListID     aListID,
                               nsFrameList&    aFrameList)
{
  NS_NOTREACHED("AppendFrames not supported");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsColumnSetFrame::InsertFrames(ChildListID     aListID,
                               nsIFrame*       aPrevFrame,
                               nsFrameList&    aFrameList)
{
  NS_NOTREACHED("InsertFrames not supported");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsColumnSetFrame::RemoveFrame(ChildListID     aListID,
                              nsIFrame*       aOldFrame)
{
  NS_NOTREACHED("RemoveFrame not supported");
  return NS_ERROR_NOT_IMPLEMENTED;
}
