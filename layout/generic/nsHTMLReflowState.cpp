






































#include "nsCOMPtr.h"
#include "nsStyleConsts.h"
#include "nsCSSAnonBoxes.h"
#include "nsFrame.h"
#include "nsIContent.h"
#include "nsGkAtoms.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIDeviceContext.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"
#include "nsBlockFrame.h"
#include "nsLineBox.h"
#include "nsImageFrame.h"
#include "nsTableFrame.h"
#include "nsIServiceManager.h"
#include "nsIPercentHeightObserver.h"
#include "nsContentUtils.h"
#include "nsLayoutUtils.h"
#ifdef IBMBIDI
#include "nsBidiUtils.h"
#endif

#ifdef NS_DEBUG
#undef NOISY_VERTICAL_ALIGN
#else
#undef NOISY_VERTICAL_ALIGN
#endif


static PRPackedBool sPrefIsLoaded = PR_FALSE;
static PRPackedBool sBlinkIsAllowed = PR_TRUE;

enum eNormalLineHeightControl {
  eUninitialized = -1,
  eNoExternalLeading = 0,   
  eIncludeExternalLeading,  
  eCompensateLeading        
};

#ifdef FONT_LEADING_APIS_V2
static eNormalLineHeightControl sNormalLineHeightControl = eUninitialized;
#endif



nsHTMLReflowState::nsHTMLReflowState(nsPresContext*       aPresContext,
                                     nsIFrame*            aFrame,
                                     nsIRenderingContext* aRenderingContext,
                                     const nsSize&        aAvailableSpace)
  : nsCSSOffsetState(aFrame, aRenderingContext)
  , mReflowDepth(0)
{
  NS_PRECONDITION(aPresContext, "no pres context");
  NS_PRECONDITION(aRenderingContext, "no rendering context");
  NS_PRECONDITION(aFrame, "no frame");
  parentReflowState = nsnull;
  availableWidth = aAvailableSpace.width;
  availableHeight = aAvailableSpace.height;
  mSpaceManager = nsnull;
  mLineLayout = nsnull;
  mFlags.mSpecialHeightReflow = PR_FALSE;
  mFlags.mIsTopOfPage = PR_FALSE;
  mFlags.mTableIsSplittable = PR_FALSE;
  mFlags.mNextInFlowUntouched = PR_FALSE;
  mFlags.mAssumingHScrollbar = mFlags.mAssumingVScrollbar = PR_FALSE;
  mFlags.mHasClearance = PR_FALSE;
  mDiscoveredClearance = nsnull;
  mPercentHeightObserver = nsnull;
  mPercentHeightReflowInitiator = nsnull;
  Init(aPresContext);
#ifdef IBMBIDI
  mRightEdge = NS_UNCONSTRAINEDSIZE;
#endif
}

static PRBool CheckNextInFlowParenthood(nsIFrame* aFrame, nsIFrame* aParent)
{
  nsIFrame* frameNext = aFrame->GetNextInFlow();
  nsIFrame* parentNext = aParent->GetNextInFlow();
  return frameNext && parentNext && frameNext->GetParent() == parentNext;
}




nsHTMLReflowState::nsHTMLReflowState(nsPresContext*           aPresContext,
                                     const nsHTMLReflowState& aParentReflowState,
                                     nsIFrame*                aFrame,
                                     const nsSize&            aAvailableSpace,
                                     nscoord                  aContainingBlockWidth,
                                     nscoord                  aContainingBlockHeight,
                                     PRBool                   aInit)
  : nsCSSOffsetState(aFrame, aParentReflowState.rendContext)
  , mReflowDepth(aParentReflowState.mReflowDepth + 1)
  , mFlags(aParentReflowState.mFlags)
{
  NS_PRECONDITION(aPresContext, "no pres context");
  NS_PRECONDITION(aFrame, "no frame");
  NS_PRECONDITION((aContainingBlockWidth == -1) ==
                    (aContainingBlockHeight == -1),
                  "cb width and height should only be non-default together");
  NS_PRECONDITION(aInit == PR_TRUE || aInit == PR_FALSE,
                  "aInit out of range for PRBool");
  NS_PRECONDITION(!mFlags.mSpecialHeightReflow ||
                  !(aFrame->GetStateBits() & (NS_FRAME_IS_DIRTY |
                                              NS_FRAME_HAS_DIRTY_CHILDREN)),
                  "frame should be clean when getting special height reflow");

  parentReflowState = &aParentReflowState;

  
  
  
  if (!mFlags.mSpecialHeightReflow)
    frame->AddStateBits(parentReflowState->frame->GetStateBits() &
                        NS_FRAME_IS_DIRTY);

  availableWidth = aAvailableSpace.width;
  availableHeight = aAvailableSpace.height;

  mSpaceManager = aParentReflowState.mSpaceManager;
  mLineLayout = aParentReflowState.mLineLayout;
  mFlags.mIsTopOfPage = aParentReflowState.mFlags.mIsTopOfPage;
  mFlags.mNextInFlowUntouched = aParentReflowState.mFlags.mNextInFlowUntouched &&
    CheckNextInFlowParenthood(aFrame, aParentReflowState.frame);
  mFlags.mAssumingHScrollbar = mFlags.mAssumingVScrollbar = PR_FALSE;
  mFlags.mHasClearance = PR_FALSE;
  mDiscoveredClearance = nsnull;
  mPercentHeightObserver = (aParentReflowState.mPercentHeightObserver && 
                            aParentReflowState.mPercentHeightObserver->NeedsToObserve(*this)) 
                           ? aParentReflowState.mPercentHeightObserver : nsnull;
  mPercentHeightReflowInitiator = aParentReflowState.mPercentHeightReflowInitiator;

  if (aInit) {
    Init(aPresContext, aContainingBlockWidth, aContainingBlockHeight);
  }

#ifdef IBMBIDI
  mRightEdge = aParentReflowState.mRightEdge;
#endif 
}

inline void
nsCSSOffsetState::ComputeWidthDependentValue(nscoord aContainingBlockWidth,
                                             const nsStyleCoord& aCoord,
                                             nscoord& aResult)
{
  aResult = nsLayoutUtils::ComputeWidthDependentValue(rendContext, frame,
                                                      aContainingBlockWidth,
                                                      aCoord);
}

inline nscoord
nsCSSOffsetState::ComputeWidthValue(nscoord aContainingBlockWidth,
                                    nscoord aContentEdgeToBoxSizing,
                                    nscoord aBoxSizingToMarginEdge,
                                    const nsStyleCoord& aCoord)
{
  return nsLayoutUtils::ComputeWidthValue(rendContext, frame,
                                          aContainingBlockWidth,
                                          aContentEdgeToBoxSizing,
                                          aBoxSizingToMarginEdge,
                                          aCoord);
}

nscoord
nsCSSOffsetState::ComputeWidthValue(nscoord aContainingBlockWidth,
                                    PRUint8 aBoxSizing,
                                    const nsStyleCoord& aCoord)
{
  nscoord inside = 0, outside = mComputedBorderPadding.LeftRight() +
                                mComputedMargin.LeftRight();
  switch (aBoxSizing) {
    case NS_STYLE_BOX_SIZING_BORDER:
      inside = mComputedBorderPadding.LeftRight();
      break;
    case NS_STYLE_BOX_SIZING_PADDING:
      inside = mComputedPadding.LeftRight();
      break;
  }
  outside -= inside;

  return ComputeWidthValue(aContainingBlockWidth, inside,
                           outside, aCoord);
}

inline void
nsCSSOffsetState::ComputeHeightDependentValue(nscoord aContainingBlockHeight,
                                              const nsStyleCoord& aCoord,
                                              nscoord& aResult)
{
  aResult = nsLayoutUtils::ComputeHeightDependentValue(rendContext, frame,
                                                       aContainingBlockHeight,
                                                       aCoord);
}

void
nsHTMLReflowState::SetComputedWidth(nscoord aComputedWidth)
{
  NS_ASSERTION(frame, "Must have a frame!");
  
  
  
  
  
  
  
  
  
  
  
  

  NS_PRECONDITION(aComputedWidth >= 0, "Invalid computed width");
  if (mComputedWidth != aComputedWidth) {
    mComputedWidth = aComputedWidth;
    if (frame->GetType() != nsGkAtoms::viewportFrame) { 
      InitResizeFlags(frame->PresContext());
    }
  }
}

void
nsHTMLReflowState::Init(nsPresContext* aPresContext,
                        nscoord         aContainingBlockWidth,
                        nscoord         aContainingBlockHeight,
                        const nsMargin* aBorder,
                        const nsMargin* aPadding)
{
  NS_ASSERTION(availableWidth != NS_UNCONSTRAINEDSIZE,
               "shouldn't use unconstrained widths anymore");

  mStylePosition = frame->GetStylePosition();
  mStyleDisplay = frame->GetStyleDisplay();
  mStyleVisibility = frame->GetStyleVisibility();
  mStyleBorder = frame->GetStyleBorder();
  mStyleMargin = frame->GetStyleMargin();
  mStylePadding = frame->GetStylePadding();
  mStyleText = frame->GetStyleText();

  InitFrameType();
  InitCBReflowState();

  InitConstraints(aPresContext, aContainingBlockWidth, aContainingBlockHeight, aBorder, aPadding);

  InitResizeFlags(aPresContext);

  NS_ASSERTION((mFrameType == NS_CSS_FRAME_TYPE_INLINE &&
                !frame->IsFrameOfType(nsIFrame::eReplaced)) ||
               frame->GetType() == nsGkAtoms::textFrame ||
               mComputedWidth != NS_UNCONSTRAINEDSIZE,
               "shouldn't use unconstrained widths anymore");
}

void nsHTMLReflowState::InitCBReflowState()
{
  if (!parentReflowState) {
    mCBReflowState = nsnull;
    return;
  }

  if (parentReflowState->frame->IsContainingBlock() ||
      
      
      (NS_FRAME_GET_TYPE(mFrameType) == NS_CSS_FRAME_TYPE_ABSOLUTE)) {
    
    if (parentReflowState->parentReflowState &&
        IS_TABLE_CELL(parentReflowState->parentReflowState->frame->GetType())) {
      mCBReflowState = parentReflowState->parentReflowState;
    } else {
      mCBReflowState = parentReflowState;
    }
      
    return;
  }
  
  mCBReflowState = parentReflowState->mCBReflowState;
}

void
nsHTMLReflowState::InitResizeFlags(nsPresContext* aPresContext)
{
  mFlags.mHResize = !(frame->GetStateBits() & NS_FRAME_IS_DIRTY) &&
                    frame->GetSize().width !=
                      mComputedWidth + mComputedBorderPadding.LeftRight();

  
  
  if (mFlags.mSpecialHeightReflow && IS_TABLE_CELL(frame->GetType()) &&
      (frame->GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT)) {
    mFlags.mVResize = PR_TRUE;
  } else if (mCBReflowState && !frame->IsContainingBlock()) {
    
    
    mFlags.mVResize = mCBReflowState->mFlags.mVResize;
  } else if (mComputedHeight == NS_AUTOHEIGHT) {
    if (eCompatibility_NavQuirks == aPresContext->CompatibilityMode() &&
        mCBReflowState) {
      
      mFlags.mVResize = mCBReflowState->mFlags.mVResize;
    } else {
      mFlags.mVResize = mFlags.mHResize || 
                        (frame->GetStateBits() &
                         (NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN));
    }
  } else {
    
    mFlags.mVResize = frame->GetSize().height !=
                        mComputedHeight + mComputedBorderPadding.TopBottom();
  }

  
  
  
  
  
  
  
  if ((mStylePosition->mHeight.GetUnit() == eStyleUnit_Percent ||
       mStylePosition->mMinHeight.GetUnit() == eStyleUnit_Percent ||
       mStylePosition->mMaxHeight.GetUnit() == eStyleUnit_Percent ||
       mStylePosition->mOffset.GetTopUnit() == eStyleUnit_Percent ||
       mStylePosition->mOffset.GetBottomUnit() == eStyleUnit_Percent ||
       frame->IsBoxFrame()) &&
      mCBReflowState) {
    const nsHTMLReflowState *rs = this;
    do {
      rs = rs->parentReflowState;
      if (rs->frame->GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT)
        break; 
      rs->frame->AddStateBits(NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
    } while (rs != mCBReflowState);
  }

  if (frame->GetStateBits() & NS_FRAME_IS_DIRTY) {
    
    
    frame->RemoveStateBits(NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
  }
}


nscoord
nsHTMLReflowState::GetContainingBlockContentWidth(const nsHTMLReflowState* aReflowState)
{
  const nsHTMLReflowState* rs = aReflowState->mCBReflowState;
  if (!rs)
    return 0;
  return rs->mComputedWidth;
}


nsIFrame*
nsHTMLReflowState::GetContainingBlockFor(const nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "Must have frame to work with");
  nsIFrame* container = aFrame->GetParent();
  if (aFrame->GetStyleDisplay()->IsAbsolutelyPositioned()) {
    
    
    return container;
  }
  while (container && !container->IsContainingBlock()) {
    container = container->GetParent();
  }
  return container;
}

void
nsHTMLReflowState::InitFrameType()
{
  const nsStyleDisplay *disp = mStyleDisplay;
  nsCSSFrameType frameType;

  
  
  
  
  
  if (frame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
    if (disp->IsAbsolutelyPositioned()) {
      frameType = NS_CSS_FRAME_TYPE_ABSOLUTE;
    }
    else if (NS_STYLE_FLOAT_NONE != disp->mFloats) {
      frameType = NS_CSS_FRAME_TYPE_FLOATING;
    } else {
      NS_ASSERTION(disp->mDisplay == NS_STYLE_DISPLAY_POPUP,
                   "unknown out of flow frame type");
      frameType = NS_CSS_FRAME_TYPE_UNKNOWN;
    }
  }
  else {
    switch (disp->mDisplay) {
    case NS_STYLE_DISPLAY_BLOCK:
    case NS_STYLE_DISPLAY_LIST_ITEM:
    case NS_STYLE_DISPLAY_TABLE:
    case NS_STYLE_DISPLAY_TABLE_CAPTION:
      frameType = NS_CSS_FRAME_TYPE_BLOCK;
      break;

    case NS_STYLE_DISPLAY_INLINE:
    case NS_STYLE_DISPLAY_INLINE_BLOCK:
    case NS_STYLE_DISPLAY_MARKER:
    case NS_STYLE_DISPLAY_INLINE_TABLE:
    case NS_STYLE_DISPLAY_INLINE_BOX:
    case NS_STYLE_DISPLAY_INLINE_GRID:
    case NS_STYLE_DISPLAY_INLINE_STACK:
      frameType = NS_CSS_FRAME_TYPE_INLINE;
      break;

    case NS_STYLE_DISPLAY_RUN_IN:
    case NS_STYLE_DISPLAY_COMPACT:
      
      frameType = NS_CSS_FRAME_TYPE_BLOCK;
      break;

    case NS_STYLE_DISPLAY_TABLE_CELL:
    case NS_STYLE_DISPLAY_TABLE_ROW_GROUP:
    case NS_STYLE_DISPLAY_TABLE_COLUMN:
    case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP:
    case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
    case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
    case NS_STYLE_DISPLAY_TABLE_ROW:
      frameType = NS_CSS_FRAME_TYPE_INTERNAL_TABLE;
      break;

    case NS_STYLE_DISPLAY_NONE:
    default:
      frameType = NS_CSS_FRAME_TYPE_UNKNOWN;
      break;
    }
  }

  
  if (frame->IsFrameOfType(nsIFrame::eReplacedContainsBlock)) {
    frameType = NS_FRAME_REPLACED_CONTAINS_BLOCK(frameType);
  } else if (frame->IsFrameOfType(nsIFrame::eReplaced)) {
    frameType = NS_FRAME_REPLACED(frameType);
  }

  mFrameType = frameType;
}

void
nsHTMLReflowState::ComputeRelativeOffsets(const nsHTMLReflowState* cbrs,
                                          nscoord aContainingBlockWidth,
                                          nscoord aContainingBlockHeight)
{
  nsStyleCoord  coord;

  
  
  
  PRBool  leftIsAuto = eStyleUnit_Auto == mStylePosition->mOffset.GetLeftUnit();
  PRBool  rightIsAuto = eStyleUnit_Auto == mStylePosition->mOffset.GetRightUnit();

  
  
  if (NS_UNCONSTRAINEDSIZE == aContainingBlockWidth) {
    if (eStyleUnit_Percent == mStylePosition->mOffset.GetLeftUnit()) {
      leftIsAuto = PR_TRUE;
    }
    if (eStyleUnit_Percent == mStylePosition->mOffset.GetRightUnit()) {
      rightIsAuto = PR_TRUE;
    }
  }

  
  
  if (!leftIsAuto && !rightIsAuto) {
    if (mCBReflowState &&
        NS_STYLE_DIRECTION_RTL == mCBReflowState->mStyleVisibility->mDirection) {
      leftIsAuto = PR_TRUE;
    } else {
      rightIsAuto = PR_TRUE;
    }
  }

  if (leftIsAuto) {
    if (rightIsAuto) {
      
      mComputedOffsets.left = mComputedOffsets.right = 0;
    } else {
      
      ComputeWidthDependentValue(aContainingBlockWidth,
                                 mStylePosition->mOffset.GetRight(coord),
                                 mComputedOffsets.right);
      
      
      mComputedOffsets.left = -mComputedOffsets.right;
    }

  } else {
    NS_ASSERTION(rightIsAuto, "unexpected specified constraint");
    
    
    ComputeWidthDependentValue(aContainingBlockWidth,
                               mStylePosition->mOffset.GetLeft(coord),
                               mComputedOffsets.left);

    
    mComputedOffsets.right = -mComputedOffsets.left;
  }

  
  
  
  PRBool  topIsAuto = eStyleUnit_Auto == mStylePosition->mOffset.GetTopUnit();
  PRBool  bottomIsAuto = eStyleUnit_Auto == mStylePosition->mOffset.GetBottomUnit();

  
  
  if (NS_AUTOHEIGHT == aContainingBlockHeight) {
    if (eStyleUnit_Percent == mStylePosition->mOffset.GetTopUnit()) {
      topIsAuto = PR_TRUE;
    }
    if (eStyleUnit_Percent == mStylePosition->mOffset.GetBottomUnit()) {
      bottomIsAuto = PR_TRUE;
    }
  }

  
  if (!topIsAuto && !bottomIsAuto) {
    bottomIsAuto = PR_TRUE;
  }

  if (topIsAuto) {
    if (bottomIsAuto) {
      
      mComputedOffsets.top = mComputedOffsets.bottom = 0;
    } else {
      
      ComputeHeightDependentValue(aContainingBlockHeight,
                                  mStylePosition->mOffset.GetBottom(coord),
                                  mComputedOffsets.bottom);
      
      
      mComputedOffsets.top = -mComputedOffsets.bottom;
    }

  } else {
    NS_ASSERTION(bottomIsAuto, "unexpected specified constraint");
    
    
    ComputeHeightDependentValue(aContainingBlockHeight,
                                mStylePosition->mOffset.GetTop(coord),
                                mComputedOffsets.top);

    
    mComputedOffsets.bottom = -mComputedOffsets.top;
  }
}

nsIFrame*
nsHTMLReflowState::GetNearestContainingBlock(nsIFrame* aFrame, nscoord& aCBLeftEdge,
                                             nscoord& aCBWidth)
{
  for (aFrame = aFrame->GetParent(); aFrame && !aFrame->IsContainingBlock();
       aFrame = aFrame->GetParent())
    ;

  NS_ASSERTION(aFrame, "Must find containing block somewhere");
  NS_ASSERTION(aFrame != frame, "How did that happen?");

  

  

  const nsHTMLReflowState* state;
  if (aFrame->GetStateBits() & NS_FRAME_IN_REFLOW) {
    for (state = parentReflowState; state && state->frame != aFrame;
         state = state->parentReflowState) {
      
    }
  } else {
    state = nsnull;
  }
  
  if (state) {
    aCBLeftEdge = state->mComputedBorderPadding.left;
    aCBWidth = state->mComputedWidth;
  } else {
    


    NS_ASSERTION(!(aFrame->GetStateBits() & NS_FRAME_IN_REFLOW),
                 "aFrame shouldn't be in reflow; we'll lie if it is");
    nsMargin borderPadding = aFrame->GetUsedBorderAndPadding();
    aCBLeftEdge = borderPadding.left;
    aCBWidth = aFrame->GetSize().width - borderPadding.LeftRight();
  }

  return aFrame;
}







struct nsHypotheticalBox {
  nscoord       mLeft, mRight;
  nscoord       mTop;
  PRPackedBool  mLeftIsExact, mRightIsExact;

  nsHypotheticalBox() {
    mLeftIsExact = mRightIsExact = PR_FALSE;
  }
};
      
static PRBool
GetIntrinsicSizeFor(nsIFrame* aFrame, nsSize& aIntrinsicSize)
{
  
  PRBool    result = PR_FALSE;

  
  
  
  
  if (aFrame->GetType() == nsGkAtoms::imageFrame) {
    nsImageFrame* imageFrame = (nsImageFrame*)aFrame;

    imageFrame->GetIntrinsicImageSize(aIntrinsicSize);
    result = (aIntrinsicSize != nsSize(0, 0));
  }
  return result;
}






void
nsHTMLReflowState::CalculateHorizBorderPaddingMargin(
                       nscoord aContainingBlockWidth,
                       nscoord* aInsideBoxSizing,
                       nscoord* aOutsideBoxSizing)
{
  const nsMargin& border = mStyleBorder->GetBorder();
  nsMargin padding, margin;

  
  if (!mStylePadding->GetPadding(padding)) {
    nsStyleCoord left, right;

    
    ComputeWidthDependentValue(aContainingBlockWidth,
                               mStylePadding->mPadding.GetLeft(left),
                               padding.left);
    ComputeWidthDependentValue(aContainingBlockWidth,
                               mStylePadding->mPadding.GetRight(right),
                               padding.right);
  }

  
  if (!mStyleMargin->GetMargin(margin)) {
    nsStyleCoord left, right;

    
    if (eStyleUnit_Auto == mStyleMargin->mMargin.GetLeftUnit()) {
      
      margin.left = 0;  
    } else {
      ComputeWidthDependentValue(aContainingBlockWidth,
                                 mStyleMargin->mMargin.GetLeft(left),
                                 margin.left);
    }
    if (eStyleUnit_Auto == mStyleMargin->mMargin.GetRightUnit()) {
      
      margin.right = 0;  
    } else {
      ComputeWidthDependentValue(aContainingBlockWidth,
                                 mStyleMargin->mMargin.GetRight(right),
                                 margin.right);
    }
  }

  nscoord outside =
    padding.LeftRight() + border.LeftRight() + margin.LeftRight();
  nscoord inside = 0;
  switch (mStylePosition->mBoxSizing) {
    case NS_STYLE_BOX_SIZING_BORDER:
      inside += border.LeftRight();
      
    case NS_STYLE_BOX_SIZING_PADDING:
      inside += padding.LeftRight();
  }
  outside -= inside;
  *aInsideBoxSizing = inside;
  *aOutsideBoxSizing = outside;
  return;
}





static PRBool AreAllEarlierInFlowFramesEmpty(nsIFrame* aFrame,
  nsIFrame* aDescendant, PRBool* aFound) {
  if (aFrame == aDescendant) {
    *aFound = PR_TRUE;
    return PR_TRUE;
  }
  if (!aFrame->IsSelfEmpty()) {
    *aFound = PR_FALSE;
    return PR_FALSE;
  }
  for (nsIFrame* f = aFrame->GetFirstChild(nsnull); f; f = f->GetNextSibling()) {
    PRBool allEmpty = AreAllEarlierInFlowFramesEmpty(f, aDescendant, aFound);
    if (*aFound || !allEmpty) {
      return allEmpty;
    }
  }
  *aFound = PR_FALSE;
  return PR_TRUE;
}




void
nsHTMLReflowState::CalculateHypotheticalBox(nsPresContext*    aPresContext,
                                            nsIFrame*         aPlaceholderFrame,
                                            nsIFrame*         aContainingBlock,
                                            nscoord           aBlockLeftContentEdge,
                                            nscoord           aBlockContentWidth,
                                            const nsHTMLReflowState* cbrs,
                                            nsHypotheticalBox& aHypotheticalBox)
{
  NS_ASSERTION(mStyleDisplay->mOriginalDisplay != NS_STYLE_DISPLAY_NONE,
               "mOriginalDisplay has not been properly initialized");
  
  
  
  
  PRBool isAutoWidth = mStylePosition->mWidth.GetUnit() == eStyleUnit_Auto;
  nsSize      intrinsicSize;
  PRBool      knowIntrinsicSize = PR_FALSE;
  if (NS_FRAME_IS_REPLACED(mFrameType) && isAutoWidth) {
    
    knowIntrinsicSize = GetIntrinsicSizeFor(frame, intrinsicSize);
  }

  
  
  nscoord boxWidth;
  PRBool  knowBoxWidth = PR_FALSE;
  if ((NS_STYLE_DISPLAY_INLINE == mStyleDisplay->mOriginalDisplay) &&
      !NS_FRAME_IS_REPLACED(mFrameType)) {
    
    

  } else {
    

    
    
    
    nscoord insideBoxSizing, outsideBoxSizing;
    CalculateHorizBorderPaddingMargin(aBlockContentWidth,
                                      &insideBoxSizing, &outsideBoxSizing);

    if (NS_FRAME_IS_REPLACED(mFrameType) && isAutoWidth) {
      
      
      if (knowIntrinsicSize) {
        boxWidth = intrinsicSize.width + outsideBoxSizing + insideBoxSizing;
        knowBoxWidth = PR_TRUE;
      }

    } else if (isAutoWidth) {
      
      boxWidth = aBlockContentWidth;
      knowBoxWidth = PR_TRUE;
    
    } else {
      
      
      
      boxWidth = ComputeWidthValue(aBlockContentWidth,
                                   insideBoxSizing, outsideBoxSizing,
                                   mStylePosition->mWidth) + 
                 insideBoxSizing + outsideBoxSizing;
      knowBoxWidth = PR_TRUE;
    }
  }
  
  
  const nsStyleVisibility* blockVis = aContainingBlock->GetStyleVisibility();

  
  
  
  
  nsPoint placeholderOffset = aPlaceholderFrame->GetOffsetTo(aContainingBlock);

  
  nsBlockFrame* blockFrame;
  if (NS_SUCCEEDED(aContainingBlock->QueryInterface(kBlockFrameCID,
                                  NS_REINTERPRET_CAST(void**, &blockFrame)))) {
    
    
    nsIFrame *blockChild =
      nsLayoutUtils::FindChildContainingDescendant(blockFrame, aPlaceholderFrame);
    nsBlockFrame::line_iterator lineBox = blockFrame->FindLineFor(blockChild);

    
    
    if (NS_STYLE_DISPLAY_INLINE == mStyleDisplay->mOriginalDisplay) {
      
      
      aHypotheticalBox.mTop = lineBox->mBounds.y;
    } else {
      
      
      
      
      
      
      if (lineBox != blockFrame->end_lines()) {
        nsIFrame * firstFrame = lineBox->mFirstChild;
        PRBool found = PR_FALSE;
        PRBool allEmpty = PR_TRUE;
        while (firstFrame) { 
          allEmpty = AreAllEarlierInFlowFramesEmpty(firstFrame,
            aPlaceholderFrame, &found);
          if (found || !allEmpty)
            break;
          firstFrame = firstFrame->GetNextSibling();
        }
        NS_ASSERTION(firstFrame, "Couldn't find placeholder!");

        if (allEmpty) {
          
          
          
          aHypotheticalBox.mTop = lineBox->mBounds.y;
        } else {
          
          
          aHypotheticalBox.mTop = lineBox->mBounds.YMost();
        }
      } else {
        
        aHypotheticalBox.mTop = placeholderOffset.y;
      }
    }
  } else {
    
    
    
    aHypotheticalBox.mTop = placeholderOffset.y;
  }

  
  
  if (NS_STYLE_DIRECTION_LTR == blockVis->mDirection) {
    
    
    if (NS_STYLE_DISPLAY_INLINE == mStyleDisplay->mOriginalDisplay) {
      
      aHypotheticalBox.mLeft = placeholderOffset.x;
    } else {
      aHypotheticalBox.mLeft = aBlockLeftContentEdge;
    }
    aHypotheticalBox.mLeftIsExact = PR_TRUE;

    if (knowBoxWidth) {
      aHypotheticalBox.mRight = aHypotheticalBox.mLeft + boxWidth;
      aHypotheticalBox.mRightIsExact = PR_TRUE;
    } else {
      
      
      
      aHypotheticalBox.mRight = aBlockLeftContentEdge + aBlockContentWidth;
      aHypotheticalBox.mRightIsExact = PR_FALSE;
    }

  } else {
    
    if (NS_STYLE_DISPLAY_INLINE == mStyleDisplay->mOriginalDisplay) {
      aHypotheticalBox.mRight = placeholderOffset.x;
    } else {
      aHypotheticalBox.mRight = aBlockLeftContentEdge + aBlockContentWidth;
    }
    aHypotheticalBox.mRightIsExact = PR_TRUE;
    
    if (knowBoxWidth) {
      aHypotheticalBox.mLeft = aHypotheticalBox.mRight - boxWidth;
      aHypotheticalBox.mLeftIsExact = PR_TRUE;
    } else {
      
      
      
      aHypotheticalBox.mLeft = aBlockLeftContentEdge;
      aHypotheticalBox.mLeftIsExact = PR_FALSE;
    }

  }

  
  
  
  
  
  nsPoint cbOffset;
  if (mStyleDisplay->mPosition == NS_STYLE_POSITION_FIXED) {
    
    
    
    
    
    cbOffset.MoveTo(0, 0);
    do {
      NS_ASSERTION(aContainingBlock,
                   "Should hit cbrs->frame before we run off the frame tree!");
      cbOffset += aContainingBlock->GetPositionIgnoringScrolling();
      aContainingBlock = aContainingBlock->GetParent();
    } while (aContainingBlock != cbrs->frame);
  } else {
    cbOffset = aContainingBlock->GetOffsetTo(cbrs->frame);
  }
  aHypotheticalBox.mLeft += cbOffset.x;
  aHypotheticalBox.mTop += cbOffset.y;
  aHypotheticalBox.mRight += cbOffset.x;
  
  
  
  
  nsMargin border = cbrs->mComputedBorderPadding - cbrs->mComputedPadding;
  aHypotheticalBox.mLeft -= border.left;
  aHypotheticalBox.mRight -= border.right;
  aHypotheticalBox.mTop -= border.top;
}

void
nsHTMLReflowState::InitAbsoluteConstraints(nsPresContext* aPresContext,
                                           const nsHTMLReflowState* cbrs,
                                           nscoord containingBlockWidth,
                                           nscoord containingBlockHeight)
{
  NS_PRECONDITION(containingBlockHeight != NS_AUTOHEIGHT,
                  "containing block height must be constrained");

  
  nsIFrame*     placeholderFrame;

  aPresContext->PresShell()->GetPlaceholderFrameFor(frame, &placeholderFrame);
  NS_ASSERTION(nsnull != placeholderFrame, "no placeholder frame");

  
  
  nscoord cbLeftEdge, cbWidth;
  nsIFrame* cbFrame = GetNearestContainingBlock(placeholderFrame, cbLeftEdge,
                                                cbWidth);
  
  
  
  
  nsHypotheticalBox hypotheticalBox;
  if (((eStyleUnit_Auto == mStylePosition->mOffset.GetLeftUnit()) &&
       (eStyleUnit_Auto == mStylePosition->mOffset.GetRightUnit())) ||
      ((eStyleUnit_Auto == mStylePosition->mOffset.GetTopUnit()) &&
       (eStyleUnit_Auto == mStylePosition->mOffset.GetBottomUnit()))) {

    CalculateHypotheticalBox(aPresContext, placeholderFrame, cbFrame,
                             cbLeftEdge, cbWidth, cbrs, hypotheticalBox);
  }

  
  
  PRBool        leftIsAuto = PR_FALSE, rightIsAuto = PR_FALSE;
  nsStyleCoord  coord;
  if (eStyleUnit_Auto == mStylePosition->mOffset.GetLeftUnit()) {
    mComputedOffsets.left = 0;
    leftIsAuto = PR_TRUE;
  } else {
    ComputeWidthDependentValue(containingBlockWidth,
                               mStylePosition->mOffset.GetLeft(coord),
                               mComputedOffsets.left);
  }
  if (eStyleUnit_Auto == mStylePosition->mOffset.GetRightUnit()) {
    mComputedOffsets.right = 0;
    rightIsAuto = PR_TRUE;
  } else {
    ComputeWidthDependentValue(containingBlockWidth,
                               mStylePosition->mOffset.GetRight(coord),
                               mComputedOffsets.right);
  }

  PRUint8 direction = cbrs ? cbrs->mStyleVisibility->mDirection : NS_STYLE_DIRECTION_LTR;

  
  
  if (leftIsAuto && rightIsAuto) {
    
    
    if (NS_STYLE_DIRECTION_LTR == direction) {
      if (hypotheticalBox.mLeftIsExact) {
        mComputedOffsets.left = hypotheticalBox.mLeft;
        leftIsAuto = PR_FALSE;
      } else {
        
        
        mComputedOffsets.right = hypotheticalBox.mRight;
        rightIsAuto = PR_FALSE;
      }
    } else {
      if (hypotheticalBox.mRightIsExact) {
        mComputedOffsets.right = containingBlockWidth - hypotheticalBox.mRight;
        rightIsAuto = PR_FALSE;
      } else {
        
        
        mComputedOffsets.left = hypotheticalBox.mLeft;
        leftIsAuto = PR_FALSE;
      }
    }
  }

  
  PRBool      topIsAuto = PR_FALSE, bottomIsAuto = PR_FALSE;
  if (eStyleUnit_Auto == mStylePosition->mOffset.GetTopUnit()) {
    mComputedOffsets.top = 0;
    topIsAuto = PR_TRUE;
  } else {
    nsStyleCoord c;
    ComputeHeightDependentValue(containingBlockHeight,
                                mStylePosition->mOffset.GetTop(c),
                                mComputedOffsets.top);
  }
  if (eStyleUnit_Auto == mStylePosition->mOffset.GetBottomUnit()) {
    mComputedOffsets.bottom = 0;        
    bottomIsAuto = PR_TRUE;
  } else {
    nsStyleCoord c;
    ComputeHeightDependentValue(containingBlockHeight,
                                mStylePosition->mOffset.GetBottom(c),
                                mComputedOffsets.bottom);
  }

  if (topIsAuto && bottomIsAuto) {
    
    mComputedOffsets.top = hypotheticalBox.mTop;
    topIsAuto = PR_FALSE;
  }

  PRBool widthIsAuto = eStyleUnit_Auto == mStylePosition->mWidth.GetUnit();
  PRBool heightIsAuto = eStyleUnit_Auto == mStylePosition->mHeight.GetUnit();

  PRBool shrinkWrap = leftIsAuto || rightIsAuto;
  nsSize size =
    frame->ComputeSize(rendContext,
                       nsSize(containingBlockWidth,
                              containingBlockHeight),
                       containingBlockWidth, 
                       nsSize(mComputedMargin.LeftRight() +
                                mComputedOffsets.LeftRight(),
                              mComputedMargin.TopBottom() +
                                mComputedOffsets.TopBottom()),
                       nsSize(mComputedBorderPadding.LeftRight() -
                                mComputedPadding.LeftRight(),
                              mComputedBorderPadding.TopBottom() -
                                mComputedPadding.TopBottom()),
                       nsSize(mComputedPadding.LeftRight(),
                              mComputedPadding.TopBottom()),
                       shrinkWrap);
  mComputedWidth = size.width;
  mComputedHeight = size.height;

  
  

  if (leftIsAuto) {
    
    
    
    if (widthIsAuto) {
      
      
      
      mComputedOffsets.left = NS_AUTOOFFSET;
    } else {
      mComputedOffsets.left = containingBlockWidth - mComputedMargin.left -
        mComputedBorderPadding.left - mComputedWidth - mComputedBorderPadding.right - 
        mComputedMargin.right - mComputedOffsets.right;

    }
  } else if (rightIsAuto) {
    
    
    
    if (widthIsAuto) {
      
      
      
      mComputedOffsets.right = NS_AUTOOFFSET;
    } else {
      mComputedOffsets.right = containingBlockWidth - mComputedOffsets.left -
        mComputedMargin.left - mComputedBorderPadding.left - mComputedWidth -
        mComputedBorderPadding.right - mComputedMargin.right;
    }
  } else {
    
    
    
    
    
    

    nscoord availMarginSpace = containingBlockWidth -
                               mComputedOffsets.LeftRight() -
                               mComputedMargin.LeftRight() -
                               mComputedBorderPadding.LeftRight() -
                               mComputedWidth;
    PRBool marginLeftIsAuto =
      eStyleUnit_Auto == mStyleMargin->mMargin.GetLeftUnit();
    PRBool marginRightIsAuto =
      eStyleUnit_Auto == mStyleMargin->mMargin.GetRightUnit();

    if (availMarginSpace < 0 ||
        (!marginLeftIsAuto && !marginRightIsAuto)) {
      
      
      
      if (NS_STYLE_DIRECTION_LTR == direction) {
        
        mComputedOffsets.right += availMarginSpace;
      } else {
        
        mComputedOffsets.left += availMarginSpace;
      }
    } else if (marginLeftIsAuto) {
      if (marginRightIsAuto) {
        
        
        mComputedMargin.left = availMarginSpace / 2;
        mComputedMargin.right = availMarginSpace - mComputedMargin.left;
      } else {
        
        mComputedMargin.left = availMarginSpace - mComputedMargin.right;
      }
    } else {
      
      mComputedMargin.right = availMarginSpace - mComputedMargin.left;
    }
  }

  if (topIsAuto) {
    
    if (heightIsAuto) {
      mComputedOffsets.top = NS_AUTOOFFSET;
    } else {
      mComputedOffsets.top = containingBlockHeight - mComputedMargin.top -
        mComputedBorderPadding.top - mComputedHeight - mComputedBorderPadding.bottom - 
        mComputedMargin.bottom - mComputedOffsets.bottom;
    }
  } else if (bottomIsAuto) {
    
    if (heightIsAuto) {
      mComputedOffsets.bottom = NS_AUTOOFFSET;
    } else {
      mComputedOffsets.bottom = containingBlockHeight - mComputedOffsets.top -
        mComputedMargin.top - mComputedBorderPadding.top - mComputedHeight -
        mComputedBorderPadding.bottom - mComputedMargin.bottom;
    }
  } else {
    
    nscoord autoHeight = containingBlockHeight -
                         mComputedOffsets.TopBottom() -
                         mComputedMargin.TopBottom() -
                         mComputedBorderPadding.TopBottom();
    if (autoHeight < 0) {
      autoHeight = 0;
    }

    if (mComputedHeight == NS_UNCONSTRAINEDSIZE) {
      
      
      mComputedHeight = autoHeight;

      
      if (mComputedHeight > mComputedMaxHeight)
        mComputedHeight = mComputedMaxHeight;
      if (mComputedHeight < mComputedMinHeight)
        mComputedHeight = mComputedMinHeight;
    }

    
    
    
    
    nscoord availMarginSpace = autoHeight - mComputedHeight;
    PRBool marginTopIsAuto =
      eStyleUnit_Auto == mStyleMargin->mMargin.GetTopUnit();
    PRBool marginBottomIsAuto =
      eStyleUnit_Auto == mStyleMargin->mMargin.GetBottomUnit();

    if (availMarginSpace < 0 || (!marginTopIsAuto && !marginBottomIsAuto)) {
      
      
      
      mComputedOffsets.bottom += availMarginSpace;
    } else if (marginTopIsAuto) {
      if (marginBottomIsAuto) {
        
        
        mComputedMargin.top = availMarginSpace / 2;
        mComputedMargin.bottom = availMarginSpace - mComputedMargin.top;
      } else {
        
        mComputedMargin.top = availMarginSpace - mComputedMargin.bottom;
      }
    } else {
      
      mComputedMargin.bottom = availMarginSpace - mComputedMargin.top;
    }
  }
}

nscoord 
GetVerticalMarginBorderPadding(const nsHTMLReflowState* aReflowState)
{
  nscoord result = 0;
  if (!aReflowState) return result;

  
  nsMargin margin = aReflowState->mComputedMargin;
  if (NS_AUTOMARGIN == margin.top) 
    margin.top = 0;
  if (NS_AUTOMARGIN == margin.bottom) 
    margin.bottom = 0;

  result += margin.top + margin.bottom;
  result += aReflowState->mComputedBorderPadding.top + 
            aReflowState->mComputedBorderPadding.bottom;

  return result;
}









nscoord
CalcQuirkContainingBlockHeight(const nsHTMLReflowState* aCBReflowState)
{
  nsHTMLReflowState* firstAncestorRS = nsnull; 
  nsHTMLReflowState* secondAncestorRS = nsnull; 
  
  
  
  
  nscoord result = NS_AUTOHEIGHT; 
                             
  const nsHTMLReflowState* rs = aCBReflowState;
  for (; rs && rs->frame; rs = (nsHTMLReflowState *)(rs->parentReflowState)) { 
    nsIAtom* frameType = rs->frame->GetType();
    
    
    if (nsGkAtoms::blockFrame == frameType ||
        nsGkAtoms::areaFrame == frameType ||
        nsGkAtoms::scrollFrame == frameType) {
      
      if (nsGkAtoms::areaFrame == frameType) {
        
        if (rs->frame->GetStyleContext()->GetPseudoType() ==
            nsCSSAnonBoxes::scrolledContent) {
          continue;
        }
      }

      secondAncestorRS = firstAncestorRS;
      firstAncestorRS = (nsHTMLReflowState*)rs;

      
      
      
      
      if (NS_AUTOHEIGHT == rs->mComputedHeight) {
        if (rs->frame->GetStyleDisplay()->IsAbsolutelyPositioned()) {
          break;
        } else {
          continue;
        }
      }
    }
    else if (nsGkAtoms::canvasFrame == frameType) {
      
      
      nsHTMLReflowState* scrollState = (nsHTMLReflowState *)rs->parentReflowState;
      if (nsGkAtoms::scrollFrame == scrollState->frame->GetType()) {
        rs = scrollState;
      }
    }
    else if (nsGkAtoms::pageContentFrame == frameType) {
      nsIFrame* prevInFlow = rs->frame->GetPrevInFlow();
      
      if (prevInFlow) 
        break;
    }
    else {
      break;
    }

    
    
    result = (nsGkAtoms::pageContentFrame == frameType)
             ? rs->availableHeight : rs->mComputedHeight;
    
    if (NS_AUTOHEIGHT == result) return result;

    
    
    if ((nsGkAtoms::canvasFrame == frameType) || 
        (nsGkAtoms::pageContentFrame == frameType)) {

      result -= GetVerticalMarginBorderPadding(firstAncestorRS); 
      result -= GetVerticalMarginBorderPadding(secondAncestorRS); 

#ifdef DEBUG
      
      if (firstAncestorRS) {
        nsIContent* frameContent = firstAncestorRS->frame->GetContent();
        if (frameContent) {
          nsIAtom *contentTag = frameContent->Tag();
          NS_ASSERTION(contentTag == nsGkAtoms::html, "First ancestor is not HTML");
        }
      }
      if (secondAncestorRS) {
        nsIContent* frameContent = secondAncestorRS->frame->GetContent();
        if (frameContent) {
          nsIAtom *contentTag = frameContent->Tag();
          NS_ASSERTION(contentTag == nsGkAtoms::body, "Second ancestor is not BODY");
        }
      }
#endif
      
    }
    
    
    else if (nsGkAtoms::areaFrame == frameType) {
      
      if (nsGkAtoms::canvasFrame == rs->parentReflowState->frame->GetType()) {
        result -= GetVerticalMarginBorderPadding(secondAncestorRS);
      }
    }
    break;
  }

  
  return PR_MAX(result, 0);
}


void
nsHTMLReflowState::ComputeContainingBlockRectangle(nsPresContext*          aPresContext,
                                                   const nsHTMLReflowState* aContainingBlockRS,
                                                   nscoord&                 aContainingBlockWidth,
                                                   nscoord&                 aContainingBlockHeight)
{
  
  
  aContainingBlockWidth = aContainingBlockRS->mComputedWidth;
  aContainingBlockHeight = aContainingBlockRS->mComputedHeight;
  
  if (NS_FRAME_GET_TYPE(mFrameType) == NS_CSS_FRAME_TYPE_ABSOLUTE) {
    
    if (NS_FRAME_GET_TYPE(aContainingBlockRS->mFrameType) == NS_CSS_FRAME_TYPE_INLINE) {
      
      
      
      
      
      
      nsMargin computedBorder = aContainingBlockRS->mComputedBorderPadding -
        aContainingBlockRS->mComputedPadding;
      aContainingBlockWidth = aContainingBlockRS->frame->GetRect().width -
        computedBorder.LeftRight();;
      NS_ASSERTION(aContainingBlockWidth >= 0,
                   "Negative containing block width!");
      aContainingBlockHeight = aContainingBlockRS->frame->GetRect().height -
        computedBorder.TopBottom();
      NS_ASSERTION(aContainingBlockHeight >= 0,
                   "Negative containing block height!");
    } else {
      
      
      aContainingBlockWidth += aContainingBlockRS->mComputedPadding.LeftRight();

      
      
      
      
      if ((NS_AUTOHEIGHT == aContainingBlockHeight) &&
          nsLayoutUtils::IsInitialContainingBlock(aContainingBlockRS->frame)) {

        
        const nsHTMLReflowState* rs = aContainingBlockRS->parentReflowState;
        while (rs) {
          aContainingBlockHeight = rs->mComputedHeight;
          rs = rs->parentReflowState;
        }

      } else {
        aContainingBlockHeight +=
          aContainingBlockRS->mComputedPadding.TopBottom();
      }
    }
  } else {
    
    
    if (NS_AUTOHEIGHT == aContainingBlockHeight) {
      if (eCompatibility_NavQuirks == aPresContext->CompatibilityMode() &&
          mStylePosition->mHeight.GetUnit() == eStyleUnit_Percent) {
        aContainingBlockHeight = CalcQuirkContainingBlockHeight(aContainingBlockRS);
      }
    }
  }
}


PR_STATIC_CALLBACK(int)
PrefsChanged(const char *aPrefName, void *instance)
{
  sBlinkIsAllowed =
    nsContentUtils::GetBoolPref("browser.blink_allowed", sBlinkIsAllowed);

  return 0; 
}




static PRBool BlinkIsAllowed(void)
{
  if (!sPrefIsLoaded) {
    
    nsContentUtils::RegisterPrefCallback("browser.blink_allowed", PrefsChanged,
                                         nsnull);
    PrefsChanged(nsnull, nsnull);
    sPrefIsLoaded = PR_TRUE;
  }
  return sBlinkIsAllowed;
}

#ifdef FONT_LEADING_APIS_V2
static eNormalLineHeightControl GetNormalLineHeightCalcControl(void)
{
  if (sNormalLineHeightControl == eUninitialized) {
    
    
    sNormalLineHeightControl =
      NS_STATIC_CAST(eNormalLineHeightControl,
                     nsContentUtils::GetIntPref("browser.display.normal_lineheight_calc_control", eNoExternalLeading));
  }
  return sNormalLineHeightControl;
}
#endif




void
nsHTMLReflowState::InitConstraints(nsPresContext* aPresContext,
                                   nscoord         aContainingBlockWidth,
                                   nscoord         aContainingBlockHeight,
                                   const nsMargin* aBorder,
                                   const nsMargin* aPadding)
{
  
  
  if (nsnull == parentReflowState) {
    
    InitOffsets(aContainingBlockWidth, aBorder, aPadding);
    
    
    mComputedMargin.SizeTo(0, 0, 0, 0);
    mComputedOffsets.SizeTo(0, 0, 0, 0);

    mComputedWidth = availableWidth - mComputedBorderPadding.LeftRight();
    if (mComputedWidth < 0)
      mComputedWidth = 0;
    if (availableHeight != NS_UNCONSTRAINEDSIZE) {
      mComputedHeight = availableHeight - mComputedBorderPadding.TopBottom();
      if (mComputedHeight < 0)
        mComputedHeight = 0;
    } else {
      mComputedHeight = NS_UNCONSTRAINEDSIZE;
    }

    mComputedMinWidth = mComputedMinHeight = 0;
    mComputedMaxWidth = mComputedMaxHeight = NS_UNCONSTRAINEDSIZE;
  } else {
    
    const nsHTMLReflowState* cbrs = mCBReflowState;
    NS_ASSERTION(nsnull != cbrs, "no containing block");

    
    
    if (aContainingBlockWidth == -1) {
      ComputeContainingBlockRectangle(aPresContext, cbrs, aContainingBlockWidth, 
                                      aContainingBlockHeight);
    }

#if 0
    nsFrame::ListTag(stdout, frame); printf(": cb=");
    nsFrame::ListTag(stdout, cbrs->frame); printf(" size=%d,%d\n", aContainingBlockWidth, aContainingBlockHeight);
#endif

    
    
    nsIAtom* fType;
    if (NS_AUTOHEIGHT == aContainingBlockHeight) {
      
      
      
      
      
      if (cbrs->parentReflowState) {
        nsIFrame* f = cbrs->parentReflowState->frame;
        fType = f->GetType();
        if (nsGkAtoms::scrollFrame == fType) {
          
          aContainingBlockHeight = cbrs->parentReflowState->mComputedHeight;
        }
        else {
          fType = cbrs->frame->GetType();
          if (IS_TABLE_CELL(fType)) {
            
            aContainingBlockHeight = cbrs->mComputedHeight;
          }
        }
      }
    }

    InitOffsets(aContainingBlockWidth, aBorder, aPadding);

    nsStyleUnit heightUnit = mStylePosition->mHeight.GetUnit();

    
    
    
    if (eStyleUnit_Percent == heightUnit) {
      if (NS_AUTOHEIGHT == aContainingBlockHeight) {
        
        
        
        if (NS_FRAME_REPLACED(NS_CSS_FRAME_TYPE_INLINE) == mFrameType ||
            NS_FRAME_REPLACED_CONTAINS_BLOCK(
                NS_CSS_FRAME_TYPE_INLINE) == mFrameType) {
          
          NS_ASSERTION(nsnull != cbrs, "no containing block");
          
          if (eCompatibility_NavQuirks == aPresContext->CompatibilityMode()) {
            if (!IS_TABLE_CELL(fType)) {
              aContainingBlockHeight = CalcQuirkContainingBlockHeight(cbrs);
              if (aContainingBlockHeight == NS_AUTOHEIGHT) {
                heightUnit = eStyleUnit_Auto;
              }
            }
            else {
              heightUnit = eStyleUnit_Auto;
            }
          }
          
          
          else 
          {
            if (NS_AUTOHEIGHT != cbrs->mComputedHeight)
              aContainingBlockHeight = cbrs->mComputedHeight;
            else
              heightUnit = eStyleUnit_Auto;
          }
        }
        else {
          
          heightUnit = eStyleUnit_Auto;
        }
      }
    }

    
    
    
    if (NS_STYLE_POSITION_RELATIVE == mStyleDisplay->mPosition) {
      ComputeRelativeOffsets(cbrs, aContainingBlockWidth, aContainingBlockHeight);
    } else {
      
      mComputedOffsets.SizeTo(0, 0, 0, 0);
    }

    
    
    ComputeMinMaxValues(aContainingBlockWidth, aContainingBlockHeight, cbrs);

    

    if (NS_CSS_FRAME_TYPE_INTERNAL_TABLE == mFrameType) {
      
      
      PRBool rowOrRowGroup = PR_FALSE;
      nsStyleUnit widthUnit = mStylePosition->mWidth.GetUnit();
      if ((NS_STYLE_DISPLAY_TABLE_ROW == mStyleDisplay->mDisplay) ||
          (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == mStyleDisplay->mDisplay)) {
        
        widthUnit = eStyleUnit_Auto;
        rowOrRowGroup = PR_TRUE;
      }

      if (eStyleUnit_Auto == widthUnit) {
        mComputedWidth = availableWidth;

        if ((mComputedWidth != NS_UNCONSTRAINEDSIZE) && !rowOrRowGroup){
          
          
          mComputedWidth -= mComputedBorderPadding.left +
            mComputedBorderPadding.right;
        }
      
      } else {
        NS_ASSERTION(widthUnit == mStylePosition->mWidth.GetUnit(),
                     "unexpected width unit change");
        mComputedWidth = ComputeWidthValue(aContainingBlockWidth,
                                           mStylePosition->mBoxSizing,
                                           mStylePosition->mWidth);
      }

      
      if ((NS_STYLE_DISPLAY_TABLE_COLUMN == mStyleDisplay->mDisplay) ||
          (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == mStyleDisplay->mDisplay)) {
        
        heightUnit = eStyleUnit_Auto;
      }
      if (eStyleUnit_Auto == heightUnit) {
        mComputedHeight = NS_AUTOHEIGHT;
      } else {
        NS_ASSERTION(heightUnit == mStylePosition->mHeight.GetUnit(),
                     "unexpected height unit change");
        ComputeHeightDependentValue(aContainingBlockHeight,
                                    mStylePosition->mHeight,
                                    mComputedHeight);
      }

      
      mComputedMinWidth = mComputedMinHeight = 0;
      mComputedMaxWidth = mComputedMaxHeight = NS_UNCONSTRAINEDSIZE;

    } else if (NS_FRAME_GET_TYPE(mFrameType) == NS_CSS_FRAME_TYPE_ABSOLUTE) {
      
      InitAbsoluteConstraints(aPresContext, cbrs, aContainingBlockWidth,
                              aContainingBlockHeight);
    } else {
      PRBool isBlock =
        NS_CSS_FRAME_TYPE_BLOCK == NS_FRAME_GET_TYPE(mFrameType);
      nsSize size =
        frame->ComputeSize(rendContext,
                           nsSize(aContainingBlockWidth,
                                  aContainingBlockHeight),
                           availableWidth,
                           nsSize(mComputedMargin.LeftRight(),
                                  mComputedMargin.TopBottom()),
                           nsSize(mComputedBorderPadding.LeftRight() -
                                    mComputedPadding.LeftRight(),
                                  mComputedBorderPadding.TopBottom() -
                                    mComputedPadding.TopBottom()),
                           nsSize(mComputedPadding.LeftRight(),
                                  mComputedPadding.TopBottom()),
                           !isBlock);

      mComputedWidth = size.width;
      mComputedHeight = size.height;

      if (isBlock)
        CalculateBlockSideMargins(availableWidth, mComputedWidth);
    }
  }
  
  mFlags.mBlinks = (parentReflowState && parentReflowState->mFlags.mBlinks);
  if (!mFlags.mBlinks && BlinkIsAllowed()) {
    const nsStyleTextReset* st = frame->GetStyleTextReset();
    mFlags.mBlinks = 
      ((st->mTextDecoration & NS_STYLE_TEXT_DECORATION_BLINK) != 0);
  }
}

void
nsCSSOffsetState::InitOffsets(nscoord aContainingBlockWidth,
                              const nsMargin *aBorder,
                              const nsMargin *aPadding)
{
  
  
  
  
  ComputeMargin(aContainingBlockWidth);

  const nsStyleDisplay *disp = frame->GetStyleDisplay();
  PRBool isThemed = frame->IsThemed(disp);
  nsPresContext *presContext = frame->PresContext();

  if (isThemed &&
      presContext->GetTheme()->GetWidgetPadding(presContext->DeviceContext(),
                                                frame, disp->mAppearance,
                                                &mComputedPadding)) {
    mComputedPadding.top = presContext->DevPixelsToAppUnits(mComputedPadding.top);
    mComputedPadding.right = presContext->DevPixelsToAppUnits(mComputedPadding.right);
    mComputedPadding.bottom = presContext->DevPixelsToAppUnits(mComputedPadding.bottom);
    mComputedPadding.left = presContext->DevPixelsToAppUnits(mComputedPadding.left);
  }
  else if (aPadding) { 
    mComputedPadding.top    = aPadding->top;
    mComputedPadding.right  = aPadding->right;
    mComputedPadding.bottom = aPadding->bottom;
    mComputedPadding.left   = aPadding->left;
  }
  else {
    ComputePadding(aContainingBlockWidth);
  }

  if (isThemed) {
    presContext->GetTheme()->GetWidgetBorder(presContext->DeviceContext(),
                                             frame, disp->mAppearance,
                                             &mComputedBorderPadding);
    mComputedBorderPadding.top =
      presContext->DevPixelsToAppUnits(mComputedBorderPadding.top);
    mComputedBorderPadding.right =
      presContext->DevPixelsToAppUnits(mComputedBorderPadding.right);
    mComputedBorderPadding.bottom =
      presContext->DevPixelsToAppUnits(mComputedBorderPadding.bottom);
    mComputedBorderPadding.left =
      presContext->DevPixelsToAppUnits(mComputedBorderPadding.left);
  }
  else if (aBorder) {  
    mComputedBorderPadding = *aBorder;
  }
  else {
    mComputedBorderPadding = frame->GetStyleBorder()->GetBorder();
  }
  mComputedBorderPadding += mComputedPadding;

  if (frame->GetType() == nsGkAtoms::tableFrame) {
    nsTableFrame *tableFrame = NS_STATIC_CAST(nsTableFrame*, frame);

    if (tableFrame->IsBorderCollapse()) {
      
      
      
      
      mComputedPadding.SizeTo(0,0,0,0);
      mComputedBorderPadding = tableFrame->GetIncludedOuterBCBorder();
    }
  }
}








void
nsHTMLReflowState::CalculateBlockSideMargins(nscoord aAvailWidth,
                                             nscoord aComputedWidth)
{
  NS_ASSERTION(NS_UNCONSTRAINEDSIZE != aComputedWidth &&
               NS_UNCONSTRAINEDSIZE != aAvailWidth,
               "this shouldn't happen anymore");

  nscoord sum = mComputedMargin.left + mComputedBorderPadding.left +
    aComputedWidth + mComputedBorderPadding.right + mComputedMargin.right;
  if (sum == aAvailWidth)
    
    return;

  
  

  
  nscoord availMarginSpace = aAvailWidth - sum;

  
  
  if (availMarginSpace < 0) {
    if (mCBReflowState &&
        mCBReflowState->mStyleVisibility->mDirection == NS_STYLE_DIRECTION_RTL) {
      mComputedMargin.left += availMarginSpace;
    } else {
      mComputedMargin.right += availMarginSpace;
    }
    return;
  }

  
  
  PRBool isAutoLeftMargin =
    eStyleUnit_Auto == mStyleMargin->mMargin.GetLeftUnit();
  PRBool isAutoRightMargin =
    eStyleUnit_Auto == mStyleMargin->mMargin.GetRightUnit();
  if (!isAutoLeftMargin && !isAutoRightMargin) {
    
    
    
    
    const nsHTMLReflowState* prs = parentReflowState;
    if (frame->GetType() == nsGkAtoms::tableFrame) {
      NS_ASSERTION(prs->frame->GetType() == nsGkAtoms::tableOuterFrame,
                   "table not inside outer table");
      
      
      prs = prs->parentReflowState;
    }
    if (prs &&
        (prs->mStyleText->mTextAlign == NS_STYLE_TEXT_ALIGN_MOZ_LEFT ||
         prs->mStyleText->mTextAlign == NS_STYLE_TEXT_ALIGN_MOZ_CENTER ||
         prs->mStyleText->mTextAlign == NS_STYLE_TEXT_ALIGN_MOZ_RIGHT)) {
      isAutoLeftMargin =
        prs->mStyleText->mTextAlign != NS_STYLE_TEXT_ALIGN_MOZ_LEFT;
      isAutoRightMargin =
        prs->mStyleText->mTextAlign != NS_STYLE_TEXT_ALIGN_MOZ_RIGHT;
    }
    
    
    else if (mCBReflowState &&
             NS_STYLE_DIRECTION_RTL == mCBReflowState->mStyleVisibility->mDirection) {
      isAutoLeftMargin = PR_TRUE;
    }
    else {
      isAutoRightMargin = PR_TRUE;
    }
  }

  
  if (isAutoLeftMargin) {
    if (isAutoRightMargin) {
      
      mComputedMargin.left = availMarginSpace / 2;
      mComputedMargin.right = availMarginSpace - mComputedMargin.left;
    } else {
      mComputedMargin.left += availMarginSpace;
    }
  } else if (isAutoRightMargin) {
    mComputedMargin.right += availMarginSpace;
  }
}

#define NORMAL_LINE_HEIGHT_FACTOR 1.2f    // in term of emHeight 








static nscoord
GetNormalLineHeight(nsIFontMetrics* aFontMetrics)
{
  NS_PRECONDITION(nsnull != aFontMetrics, "no font metrics");

  nscoord normalLineHeight;

#ifdef FONT_LEADING_APIS_V2
  nscoord externalLeading, internalLeading, emHeight;
  aFontMetrics->GetExternalLeading(externalLeading);
  aFontMetrics->GetInternalLeading(internalLeading);
  aFontMetrics->GetEmHeight(emHeight);
  switch (GetNormalLineHeightCalcControl()) {
  case eIncludeExternalLeading:
    normalLineHeight = emHeight+ internalLeading + externalLeading;
    break;
  case eCompensateLeading:
    if (!internalLeading && !externalLeading)
      normalLineHeight = NSToCoordRound(emHeight * NORMAL_LINE_HEIGHT_FACTOR);
    else
      normalLineHeight = emHeight+ internalLeading + externalLeading;
    break;
  default:
    
    normalLineHeight = emHeight + internalLeading;
  }
#else
  aFontMetrics->GetNormalLineHeight(normalLineHeight);
#endif 
  return normalLineHeight;
}


static nscoord
ComputeLineHeight(nsIRenderingContext* aRenderingContext,
                  nsIDeviceContext* aDeviceContext,
                  nsStyleContext* aStyleContext)
{
  NS_PRECONDITION(aRenderingContext || aDeviceContext,
                  "Need to have a way of getting a device context");

  nscoord lineHeight;

  const nsStyleFont* font = aStyleContext->GetStyleFont();
  const nsStyleCoord& lhCoord = aStyleContext->GetStyleText()->mLineHeight;
  
  nsStyleUnit unit = lhCoord.GetUnit();

  if (unit == eStyleUnit_Coord) {
    
    lineHeight = lhCoord.GetCoordValue();
  } else if (unit == eStyleUnit_Factor) {
    
    
    
    float factor = lhCoord.GetFactorValue();
    lineHeight = NSToCoordRound(factor * font->mFont.size);
  } else {
    NS_ASSERTION(eStyleUnit_Normal == unit, "bad unit");
    nsCOMPtr<nsIDeviceContext> deviceContext = aDeviceContext;
    if (NS_UNLIKELY(!deviceContext)) {
      aRenderingContext->GetDeviceContext(*getter_AddRefs(deviceContext));
    }
    const nsStyleVisibility* vis = aStyleContext->GetStyleVisibility();
    nsCOMPtr<nsIFontMetrics> fm;
    deviceContext->GetMetricsFor(font->mFont, vis->mLangGroup,
                                 *getter_AddRefs(fm));
    lineHeight = GetNormalLineHeight(fm);
  }
  return lineHeight;
}

nscoord
nsHTMLReflowState::CalcLineHeight(nsIRenderingContext* aRenderingContext,
                                  nsIFrame* aFrame)
{
  NS_ASSERTION(aFrame && aFrame->GetStyleContext(),
               "Bogus data passed in to CalcLineHeight");

  nscoord lineHeight = ComputeLineHeight(aRenderingContext, nsnull,
                                         aFrame->GetStyleContext());

  NS_ASSERTION(lineHeight >= 0, "ComputeLineHeight screwed up");

  return lineHeight;
}

nscoord
nsHTMLReflowState::CalcLineHeight(nsStyleContext* aStyleContext,
                                  nsIDeviceContext* aDeviceContext)
{
  NS_PRECONDITION(aStyleContext, "Must have a style context");
  NS_PRECONDITION(aDeviceContext, "Must have a device context");
  
  nscoord lineHeight = ComputeLineHeight(nsnull, aDeviceContext,
                                         aStyleContext);

  NS_ASSERTION(lineHeight >= 0, "ComputeLineHeight screwed up");

  return lineHeight;
}


void
nsCSSOffsetState::DestroyMarginFunc(void*    aFrame,
                                    nsIAtom* aPropertyName,
                                    void*    aPropertyValue,
                                    void*    aDtorData)
{
  delete NS_STATIC_CAST(nsMargin*, aPropertyValue);
}

void
nsCSSOffsetState::ComputeMargin(nscoord aContainingBlockWidth)
{
  
  const nsStyleMargin *styleMargin = frame->GetStyleMargin();
  if (!styleMargin->GetMargin(mComputedMargin)) {
    
    if (NS_UNCONSTRAINEDSIZE == aContainingBlockWidth) {
      mComputedMargin.left = 0;
      mComputedMargin.right = 0;

      if (eStyleUnit_Coord == styleMargin->mMargin.GetLeftUnit()) {
        nsStyleCoord left;
        
        styleMargin->mMargin.GetLeft(left),
        mComputedMargin.left = left.GetCoordValue();
      }
      if (eStyleUnit_Coord == styleMargin->mMargin.GetRightUnit()) {
        nsStyleCoord right;
        
        styleMargin->mMargin.GetRight(right),
        mComputedMargin.right = right.GetCoordValue();
      }

    } else {
      nsStyleCoord left, right;

      ComputeWidthDependentValue(aContainingBlockWidth,
                                 styleMargin->mMargin.GetLeft(left),
                                 mComputedMargin.left);
      ComputeWidthDependentValue(aContainingBlockWidth,
                                 styleMargin->mMargin.GetRight(right),
                                 mComputedMargin.right);
    }

    nsStyleCoord top, bottom;
    
    
    
    
    ComputeWidthDependentValue(aContainingBlockWidth,
                               styleMargin->mMargin.GetTop(top),
                               mComputedMargin.top);
    ComputeWidthDependentValue(aContainingBlockWidth,
                               styleMargin->mMargin.GetBottom(bottom),
                               mComputedMargin.bottom);

    
    frame->SetProperty(nsGkAtoms::usedMarginProperty,
                       new nsMargin(mComputedMargin),
                       DestroyMarginFunc);
  }
}

void
nsCSSOffsetState::ComputePadding(nscoord aContainingBlockWidth)
{
  
  const nsStylePadding *stylePadding = frame->GetStylePadding();
  if (!stylePadding->GetPadding(mComputedPadding)) {
    
    nsStyleCoord left, right, top, bottom;

    ComputeWidthDependentValue(aContainingBlockWidth,
                               stylePadding->mPadding.GetLeft(left),
                               mComputedPadding.left);
    ComputeWidthDependentValue(aContainingBlockWidth,
                               stylePadding->mPadding.GetRight(right),
                               mComputedPadding.right);

    
    
    ComputeWidthDependentValue(aContainingBlockWidth,
                               stylePadding->mPadding.GetTop(top),
                               mComputedPadding.top);
    ComputeWidthDependentValue(aContainingBlockWidth,
                               stylePadding->mPadding.GetBottom(bottom),
                               mComputedPadding.bottom);

    frame->SetProperty(nsGkAtoms::usedPaddingProperty,
                       new nsMargin(mComputedPadding),
                       DestroyMarginFunc);
  }
  
  
  nsIAtom* frameType = frame->GetType();
  if (nsGkAtoms::tableRowGroupFrame == frameType ||
      nsGkAtoms::tableColGroupFrame == frameType ||
      nsGkAtoms::tableRowFrame      == frameType ||
      nsGkAtoms::tableColFrame      == frameType) {
    mComputedPadding.top    = 0;
    mComputedPadding.right  = 0;
    mComputedPadding.bottom = 0;
    mComputedPadding.left   = 0;
  }
}

void
nsHTMLReflowState::ApplyMinMaxConstraints(nscoord* aFrameWidth,
                                          nscoord* aFrameHeight) const
{
  if (aFrameWidth) {
    if (NS_UNCONSTRAINEDSIZE != mComputedMaxWidth) {
      *aFrameWidth = PR_MIN(*aFrameWidth, mComputedMaxWidth);
    }
    *aFrameWidth = PR_MAX(*aFrameWidth, mComputedMinWidth);
  }

  if (aFrameHeight) {
    if (NS_UNCONSTRAINEDSIZE != mComputedMaxHeight) {
      *aFrameHeight = PR_MIN(*aFrameHeight, mComputedMaxHeight);
    }
    *aFrameHeight = PR_MAX(*aFrameHeight, mComputedMinHeight);
  }
}

void
nsHTMLReflowState::ComputeMinMaxValues(nscoord aContainingBlockWidth,
                                       nscoord aContainingBlockHeight,
                                       const nsHTMLReflowState* aContainingBlockRS)
{
  mComputedMinWidth = ComputeWidthValue(aContainingBlockWidth,
                                        mStylePosition->mBoxSizing,
                                        mStylePosition->mMinWidth);

  if (eStyleUnit_Null == mStylePosition->mMaxWidth.GetUnit()) {
    
    mComputedMaxWidth = NS_UNCONSTRAINEDSIZE;  
  } else {
    mComputedMaxWidth = ComputeWidthValue(aContainingBlockWidth,
                                          mStylePosition->mBoxSizing,
                                          mStylePosition->mMaxWidth);
  }

  
  
  if (mComputedMinWidth > mComputedMaxWidth) {
    mComputedMaxWidth = mComputedMinWidth;
  }

  
  
  if ((NS_AUTOHEIGHT == aContainingBlockHeight) &&
      (eStyleUnit_Percent == mStylePosition->mMinHeight.GetUnit())) {
    mComputedMinHeight = 0;
  } else {
    ComputeHeightDependentValue(aContainingBlockHeight,
                                mStylePosition->mMinHeight, mComputedMinHeight);
  }
  nsStyleUnit maxHeightUnit = mStylePosition->mMaxHeight.GetUnit();
  if (eStyleUnit_Null == maxHeightUnit) {
    
    mComputedMaxHeight = NS_UNCONSTRAINEDSIZE;  
  } else {
    
    
    if ((NS_AUTOHEIGHT == aContainingBlockHeight) && 
        (eStyleUnit_Percent == maxHeightUnit)) {
      mComputedMaxHeight = NS_UNCONSTRAINEDSIZE;
    } else {
      ComputeHeightDependentValue(aContainingBlockHeight,
                                  mStylePosition->mMaxHeight, mComputedMaxHeight);
    }
  }

  
  
  if (mComputedMinHeight > mComputedMaxHeight) {
    mComputedMaxHeight = mComputedMinHeight;
  }
}

void
nsHTMLReflowState::SetTruncated(const nsHTMLReflowMetrics& aMetrics,
                                nsReflowStatus* aStatus) const
{
  if (availableHeight != NS_UNCONSTRAINEDSIZE &&
      availableHeight < aMetrics.height &&
      !mFlags.mIsTopOfPage) {
    *aStatus |= NS_FRAME_TRUNCATED;
  } else {
    *aStatus &= ~NS_FRAME_TRUNCATED;
  }
}
