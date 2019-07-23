







































#include "nsHTMLContainerFrame.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsISupports.h"
#include "nsIAtom.h"
#include "nsPresContext.h"
#include "nsHTMLParts.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsCOMPtr.h"

class nsColumnSetFrame : public nsHTMLContainerFrame {
public:
  nsColumnSetFrame(nsStyleContext* aContext);

  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsIFrame*       aChildList);

  NS_IMETHOD Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus);
                               
  NS_IMETHOD  AppendFrames(nsIAtom*        aListName,
                           nsIFrame*       aFrameList);
  NS_IMETHOD  InsertFrames(nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsIFrame*       aFrameList);
  NS_IMETHOD  RemoveFrame(nsIAtom*        aListName,
                          nsIFrame*       aOldFrame);

  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);  
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);

  virtual nsIFrame* GetContentInsertionFrame() {
    return GetFirstChild(nsnull)->GetContentInsertionFrame();
  }

  virtual nsresult StealFrame(nsPresContext* aPresContext,
                              nsIFrame*      aChild,
                              PRBool         aForceNormal)
  { 
    return nsContainerFrame::StealFrame(aPresContext, aChild, PR_TRUE);
  }

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  virtual nsIAtom* GetType() const;

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
  
  




  void DrainOverflowColumns();

  






  ReflowConfig ChooseColumnStrategy(const nsHTMLReflowState& aReflowState);

  



  PRBool ReflowChildren(nsHTMLReflowMetrics& aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus& aStatus,
                        const ReflowConfig& aConfig,
                        PRBool aLastColumnUnbounded,
                        nsCollapsingMargin* aCarriedOutBottomMargin);
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

nsColumnSetFrame::nsColumnSetFrame(nsStyleContext* aContext)
  : nsHTMLContainerFrame(aContext), mLastBalanceHeight(NS_INTRINSICSIZE),
    mLastFrameStatus(NS_FRAME_COMPLETE)
{
}

nsIAtom*
nsColumnSetFrame::GetType() const
{
  return nsGkAtoms::columnSetFrame;
}

NS_IMETHODIMP
nsColumnSetFrame::SetInitialChildList(nsIAtom*        aListName,
                                      nsIFrame*       aChildList)
{
  NS_ASSERTION(!aListName, "Only default child list supported");
  NS_ASSERTION(aChildList && !aChildList->GetNextSibling(),
               "initial child list must have exactly one child");
  
  return nsHTMLContainerFrame::SetInitialChildList(nsnull, aChildList);
}

static nscoord GetAvailableContentWidth(const nsHTMLReflowState& aReflowState) {
  if (aReflowState.availableWidth == NS_INTRINSICSIZE) {
    return NS_INTRINSICSIZE;
  }
  nscoord borderPaddingWidth =
    aReflowState.mComputedBorderPadding.left +
    aReflowState.mComputedBorderPadding.right;
  return PR_MAX(0, aReflowState.availableWidth - borderPaddingWidth);
}

static nscoord GetAvailableContentHeight(const nsHTMLReflowState& aReflowState) {
  if (aReflowState.availableHeight == NS_INTRINSICSIZE) {
    return NS_INTRINSICSIZE;
  }
  nscoord borderPaddingHeight =
    aReflowState.mComputedBorderPadding.top +
    aReflowState.mComputedBorderPadding.bottom;
  return PR_MAX(0, aReflowState.availableHeight - borderPaddingHeight);
}

static nscoord
GetColumnGap(nsColumnSetFrame* aFrame, const nsStyleColumn* aColStyle) {
  switch (aColStyle->mColumnGap.GetUnit()) {
    case eStyleUnit_Coord:
      return aColStyle->mColumnGap.GetCoordValue();
    case eStyleUnit_Normal:
      return aFrame->GetStyleFont()->mFont.size;
    default:
      NS_NOTREACHED("Unknown gap type");
  }
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

  nscoord colWidth = NS_INTRINSICSIZE;
  if (colStyle->mColumnWidth.GetUnit() == eStyleUnit_Coord) {
    colWidth = colStyle->mColumnWidth.GetCoordValue();

    
    
    
    
    if (availContentWidth != NS_INTRINSICSIZE && colWidth + colGap > 0
        && numColumns > 0) {
      
      
      PRInt32 maxColumns = (availContentWidth + colGap)/(colGap + colWidth);
      numColumns = PR_MAX(1, PR_MIN(numColumns, maxColumns));
    }
  } else if (numColumns > 0 && availContentWidth != NS_INTRINSICSIZE) {
    nscoord widthMinusGaps = availContentWidth - colGap*(numColumns - 1);
    colWidth = widthMinusGaps/numColumns;
  }
  
  
  colWidth = PR_MAX(1, PR_MIN(colWidth, availContentWidth));

  nscoord expectedWidthLeftOver = 0;

  if (colWidth != NS_INTRINSICSIZE && availContentWidth != NS_INTRINSICSIZE) {
    

    
    
    if (numColumns <= 0) {
      
      
      
      numColumns = (availContentWidth + colGap)/(colGap + colWidth);
      if (numColumns <= 0) {
        numColumns = 1;
      }
    }

    
    nscoord extraSpace =
      PR_MAX(0, availContentWidth - (colWidth*numColumns + colGap*(numColumns - 1)));
    nscoord extraToColumns = extraSpace/numColumns;
    colWidth += extraToColumns;
    expectedWidthLeftOver = extraSpace - (extraToColumns*numColumns);
  }

  
  
  if (aReflowState.ComputedHeight() == NS_INTRINSICSIZE) {
    
    if (numColumns <= 0) {
      
      
      numColumns = 1;
    }
    colHeight = PR_MIN(mLastBalanceHeight, GetAvailableContentHeight(aReflowState));
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
  
  nsRect* overflowArea = aChild->GetOverflowAreaProperty(PR_FALSE);
  nsRect r = overflowArea ? *overflowArea : nsRect(nsPoint(0, 0), aChild->GetSize());
  r += aChild->GetPosition();
  aParent->Invalidate(r);
  r -= aChild->GetPosition();
  aChild->SetPosition(aOrigin);
  r += aOrigin;
  aParent->Invalidate(r);
  PlaceFrameView(aChild);
}

nscoord
nsColumnSetFrame::GetMinWidth(nsIRenderingContext *aRenderingContext) {
  nscoord width = 0;
  if (mFrames.FirstChild()) {
    width = mFrames.FirstChild()->GetMinWidth(aRenderingContext);
  }
  const nsStyleColumn* colStyle = GetStyleColumn();
  if (colStyle->mColumnWidth.GetUnit() == eStyleUnit_Coord) {
    
    
    
    width = PR_MIN(width, colStyle->mColumnWidth.GetCoordValue());
  } else {
    NS_ASSERTION(colStyle->mColumnCount > 0, "column-count and column-width can't both be auto");
    
    
    width *= colStyle->mColumnCount;
  }
  
  
  return width;
}

nscoord
nsColumnSetFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext) {
  
  
  
  
  const nsStyleColumn* colStyle = GetStyleColumn();
  nscoord colGap = GetColumnGap(this, colStyle);

  nscoord colWidth;
  if (colStyle->mColumnWidth.GetUnit() == eStyleUnit_Coord) {
    colWidth = colStyle->mColumnWidth.GetCoordValue();
  } else {
    if (mFrames.FirstChild()) {
      colWidth = mFrames.FirstChild()->GetPrefWidth(aRenderingContext);
    } else {
      colWidth = 0;
    }
  }

  PRInt32 numColumns = colStyle->mColumnCount;
   if (numColumns <= 0) {
    
    numColumns = 1;
  }

  return colWidth*numColumns + colGap*(numColumns - 1);
}

PRBool
nsColumnSetFrame::ReflowChildren(nsHTMLReflowMetrics&     aDesiredSize,
                                 const nsHTMLReflowState& aReflowState,
                                 nsReflowStatus&          aStatus,
                                 const ReflowConfig&      aConfig,
                                 PRBool                   aUnboundedLastColumn,
                                 nsCollapsingMargin*      aBottomMarginCarriedOut) {
  PRBool allFit = PR_TRUE;
  PRBool RTL = GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
  PRBool shrinkingHeightOnly = !NS_SUBTREE_DIRTY(this) &&
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
  nsRect overflowRect(0, 0, 0, 0);
  
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
  PRBool reflowNext = PR_FALSE;

  while (child) {
    
    
    
    
    PRBool skipIncremental = !(GetStateBits() & NS_FRAME_IS_DIRTY)
      && !NS_SUBTREE_DIRTY(child)
      && child->GetNextSibling()
      && !NS_SUBTREE_DIRTY(child->GetNextSibling());
    
    
    
    
    
    
    PRBool skipResizeHeightShrink = shrinkingHeightOnly
      && !(child->GetStateBits() & NS_FRAME_IS_DIRTY)
      && child->GetOverflowRect().YMost() <= aConfig.mColMaxHeight;
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
      kidReflowState.mFlags.mIsTopOfPage = PR_TRUE;
      kidReflowState.mFlags.mTableIsSplittable = PR_FALSE;
          
#ifdef DEBUG_roc
      printf("*** Reflowing child #%d %p: availHeight=%d\n",
             columnCount, (void*)child,availSize.height);
#endif

      
      
      if (child->GetNextSibling() &&
          !(GetStateBits() & NS_FRAME_IS_DIRTY) &&
        !(child->GetNextSibling()->GetStateBits() & NS_FRAME_IS_DIRTY)) {
        kidReflowState.mFlags.mNextInFlowUntouched = PR_TRUE;
      }
    
      nsHTMLReflowMetrics kidDesiredSize(aDesiredSize.mFlags);

      
      
      
      
      
      

      
      ReflowChild(child, PresContext(), kidDesiredSize, kidReflowState,
                  childOrigin.x + kidReflowState.mComputedMargin.left,
                  childOrigin.y + kidReflowState.mComputedMargin.top,
                  0, aStatus);

      if (kidDesiredSize.height > aConfig.mColMaxHeight) {
        allFit = PR_FALSE;
      }
      
      reflowNext = (aStatus & NS_FRAME_REFLOW_NEXTINFLOW) != 0;
    
#ifdef DEBUG_roc
      printf("*** Reflowed child #%d %p: status = %d, desiredSize=%d,%d\n",
             columnCount, (void*)child, aStatus, kidDesiredSize.width, kidDesiredSize.height);
#endif

      NS_FRAME_TRACE_REFLOW_OUT("Column::Reflow", aStatus);

      *aBottomMarginCarriedOut = kidDesiredSize.mCarriedOutBottomMargin;
      
      FinishReflowChild(child, PresContext(), &kidReflowState, 
                        kidDesiredSize, childOrigin.x, childOrigin.y, 0);
    }

    contentRect.UnionRect(contentRect, child->GetRect());

    ConsiderChildOverflow(overflowRect, child);

    
    nsIFrame* kidNextInFlow = child->GetNextInFlow();

    if (NS_FRAME_IS_FULLY_COMPLETE(aStatus) && !NS_FRAME_IS_TRUNCATED(aStatus)) {
      NS_ASSERTION(!kidNextInFlow, "next in flow should have been deleted");
      break;
    } else {
      ++columnCount;
      
      
      
      
      if (!kidNextInFlow) {
        NS_ASSERTION(aStatus & NS_FRAME_REFLOW_NEXTINFLOW,
                     "We have to create a continuation, but the block doesn't want us to reflow it?");

        
        nsresult rv = CreateNextInFlow(PresContext(), this, child, kidNextInFlow);
        
        if (NS_FAILED(rv)) {
          NS_NOTREACHED("Couldn't create continuation");
          break;
        }
      }

      
      
      if (NS_FRAME_OVERFLOW_IS_INCOMPLETE(aStatus)) {
        if (!(kidNextInFlow->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)) {
          aStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
          reflowNext = PR_TRUE;
          kidNextInFlow->AddStateBits(NS_FRAME_IS_OVERFLOW_CONTAINER);
        }
      }
      else if (kidNextInFlow->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER) {
        aStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
        reflowNext = PR_TRUE;
        kidNextInFlow->RemoveStateBits(NS_FRAME_IS_OVERFLOW_CONTAINER);
      }
        
      if (columnCount >= aConfig.mBalanceColCount) {
        
        aStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
        kidNextInFlow->AddStateBits(NS_FRAME_IS_DIRTY);
        
        
        
        nsIFrame* continuationColumns = child->GetNextSibling();
        if (continuationColumns) {
          SetOverflowFrames(PresContext(), continuationColumns);
          child->SetNextSibling(nsnull);
        }
        break;
      }
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
  
  
  if (RTL && childOrigin.x != targetX) {
    overflowRect = nsRect(0, 0, 0, 0);
    contentRect = nsRect(0, 0, 0, 0);
    PRInt32 deltaX = targetX - childOrigin.x;
#ifdef DEBUG_roc
    printf("*** CHILDORIGIN.x = %d, targetX = %d, DELTAX = %d\n", childOrigin.x, targetX, deltaX);
#endif
    for (child = mFrames.FirstChild(); child; child = child->GetNextSibling()) {
      MoveChildTo(this, child, child->GetPosition() + nsPoint(deltaX, 0));
      ConsiderChildOverflow(overflowRect, child);
      contentRect.UnionRect(contentRect, child->GetRect());
    }
  }

  mLastFrameStatus = aStatus;
  
  
  contentRect -= nsPoint(borderPadding.left, borderPadding.top);
  
  nsSize contentSize = nsSize(contentRect.XMost(), contentRect.YMost());

  
  if (aReflowState.ComputedHeight() != NS_INTRINSICSIZE) {
    contentSize.height = aReflowState.ComputedHeight();
  } else {
    if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMaxHeight) {
      contentSize.height = PR_MIN(aReflowState.mComputedMaxHeight, contentSize.height);
    }
    if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMinHeight) {
      contentSize.height = PR_MAX(aReflowState.mComputedMinHeight, contentSize.height);
    }
  }
  if (aReflowState.ComputedWidth() != NS_INTRINSICSIZE) {
    contentSize.width = aReflowState.ComputedWidth();
  } else {
    if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMaxWidth) {
      contentSize.width = PR_MIN(aReflowState.mComputedMaxWidth, contentSize.width);
    }
    if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMinWidth) {
      contentSize.width = PR_MAX(aReflowState.mComputedMinWidth, contentSize.width);
    }
  }
    
  aDesiredSize.height = borderPadding.top + contentSize.height +
    borderPadding.bottom;
  aDesiredSize.width = contentSize.width + borderPadding.left + borderPadding.right;
  overflowRect.UnionRect(overflowRect, nsRect(0, 0, aDesiredSize.width, aDesiredSize.height));
  aDesiredSize.mOverflowArea = overflowRect;
  
#ifdef DEBUG_roc
  printf("*** DONE PASS feasible=%d\n", allFit && NS_FRAME_IS_COMPLETE(aStatus)
         && !NS_FRAME_IS_TRUNCATED(aStatus));
#endif
  return allFit && NS_FRAME_IS_COMPLETE(aStatus)
    && !NS_FRAME_IS_TRUNCATED(aStatus);
}

static nscoord ComputeSumOfChildHeights(nsIFrame* aFrame) {
  nscoord totalHeight = 0;
  for (nsIFrame* f = aFrame->GetFirstChild(nsnull); f; f = f->GetNextSibling()) {
    
    
    totalHeight += f->GetSize().height;
  }
  return totalHeight;
}

void
nsColumnSetFrame::DrainOverflowColumns()
{
  
  
  nsColumnSetFrame* prev = static_cast<nsColumnSetFrame*>(GetPrevInFlow());
  if (prev) {
    nsIFrame* overflows = prev->GetOverflowFrames(PresContext(), PR_TRUE);
    if (overflows) {
      
      nsIFrame* lastFrame = nsnull;
      for (nsIFrame* f = overflows; f; f = f->GetNextSibling()) {
        f->SetParent(this);

        
        
        nsHTMLContainerFrame::ReparentFrameView(PresContext(), f, prev, this);

        
        lastFrame = f;
      }

      NS_ASSERTION(lastFrame, "overflow list was created with no frames");
      lastFrame->SetNextSibling(mFrames.FirstChild());
      
      mFrames.SetFrames(overflows);
    }
  }
  
  
  
  nsIFrame* overflows = GetOverflowFrames(PresContext(), PR_TRUE);
  if (overflows) {
    mFrames.AppendFrames(this, overflows);
  }
}

NS_IMETHODIMP 
nsColumnSetFrame::Reflow(nsPresContext*          aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsColumnSetFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  
  aStatus = NS_FRAME_COMPLETE;

  

  ReflowConfig config = ChooseColumnStrategy(aReflowState);
  PRBool isBalancing = config.mBalanceColCount < PR_INT32_MAX;
  
  
  
  
  
  
  
  nsIFrame* nextInFlow = GetNextInFlow();
  PRBool unboundedLastColumn = isBalancing && nextInFlow;
  nsCollapsingMargin carriedOutBottomMargin;
  PRBool feasible = ReflowChildren(aDesiredSize, aReflowState,
    aStatus, config, unboundedLastColumn, &carriedOutBottomMargin);

  if (isBalancing) {
    nscoord availableContentHeight = GetAvailableContentHeight(aReflowState);
  
    
    
    
    nscoord knownFeasibleHeight = NS_INTRINSICSIZE;
    nscoord knownInfeasibleHeight = 0;
    
    
    
    PRBool maybeContinuousBreakingDetected = PR_FALSE;

    while (1) {
      nscoord lastKnownFeasibleHeight = knownFeasibleHeight;

      nscoord maxHeight = 0;
      for (nsIFrame* f = mFrames.FirstChild(); f; f = f->GetNextSibling()) {
        
        
        
        
        maxHeight = PR_MAX(maxHeight, f->GetOverflowRect().YMost());
      }

      
      if (feasible) {
        
        knownFeasibleHeight = PR_MIN(knownFeasibleHeight, maxHeight);
        knownFeasibleHeight = PR_MIN(knownFeasibleHeight, mLastBalanceHeight);

        
        
        
        
        if (mFrames.GetLength() == config.mBalanceColCount) {
          knownInfeasibleHeight = PR_MAX(knownInfeasibleHeight,
                                         mFrames.LastChild()->GetSize().height - 1);
        }
      } else {
        knownInfeasibleHeight = PR_MAX(knownInfeasibleHeight, mLastBalanceHeight);

        if (unboundedLastColumn) {
          
          
          knownFeasibleHeight = PR_MIN(knownFeasibleHeight, maxHeight);
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
        
        
        
        maybeContinuousBreakingDetected = PR_TRUE;
      }

      nscoord nextGuess = (knownFeasibleHeight + knownInfeasibleHeight)/2;
      
      if (knownFeasibleHeight - nextGuess < 600 &&
          !maybeContinuousBreakingDetected) {
        
        
        
        nextGuess = knownFeasibleHeight - 1;
      } else if (unboundedLastColumn) {
        
        
        
        nextGuess = ComputeSumOfChildHeights(this)/config.mBalanceColCount + 600;
        
        nextGuess = PR_MIN(PR_MAX(nextGuess, knownInfeasibleHeight + 1),
                           knownFeasibleHeight - 1);
      } else if (knownFeasibleHeight == NS_INTRINSICSIZE) {
        
        
        
        nextGuess = knownInfeasibleHeight*2 + 600;
      }
      
      nextGuess = PR_MIN(availableContentHeight, nextGuess);

#ifdef DEBUG_roc
      printf("*** nsColumnSetFrame::Reflow balancing choosing next guess=%d\n", nextGuess);
#endif

      config.mColMaxHeight = nextGuess;
      
      unboundedLastColumn = PR_FALSE;
      AddStateBits(NS_FRAME_IS_DIRTY);
      feasible = ReflowChildren(aDesiredSize, aReflowState,
                                aStatus, config, PR_FALSE, 
                                &carriedOutBottomMargin);
    }

    if (!feasible) {
      
      
      PRBool skip = PR_FALSE;
      if (knownInfeasibleHeight >= availableContentHeight) {
        config.mColMaxHeight = availableContentHeight;
        if (mLastBalanceHeight == availableContentHeight) {
          skip = PR_TRUE;
        }
      } else {
        config.mColMaxHeight = knownFeasibleHeight;
      }
      if (!skip) {
        AddStateBits(NS_FRAME_IS_DIRTY);
        ReflowChildren(aDesiredSize, aReflowState,
                       aStatus, config, PR_FALSE, &carriedOutBottomMargin);
      }
    }
  }
  
  CheckInvalidateSizeChange(PresContext(), aDesiredSize, aReflowState);

  FinishAndStoreOverflow(&aDesiredSize);
  aDesiredSize.mCarriedOutBottomMargin = carriedOutBottomMargin;

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);

  return NS_OK;
}

NS_IMETHODIMP
nsColumnSetFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                   const nsRect&           aDirtyRect,
                                   const nsDisplayListSet& aLists) {
  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);
  
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
nsColumnSetFrame::AppendFrames(nsIAtom*        aListName,
                               nsIFrame*       aFrameList)
{
  NS_NOTREACHED("AppendFrames not supported");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsColumnSetFrame::InsertFrames(nsIAtom*        aListName,
                               nsIFrame*       aPrevFrame,
                               nsIFrame*       aFrameList)
{
  NS_NOTREACHED("InsertFrames not supported");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsColumnSetFrame::RemoveFrame(nsIAtom*        aListName,
                              nsIFrame*       aOldFrame)
{
  NS_NOTREACHED("RemoveFrame not supported");
  return NS_ERROR_NOT_IMPLEMENTED;
}
