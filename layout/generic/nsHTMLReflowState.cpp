







































#include "nsCOMPtr.h"
#include "nsStyleConsts.h"
#include "nsCSSAnonBoxes.h"
#include "nsFrame.h"
#include "nsIContent.h"
#include "nsGkAtoms.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsFontMetrics.h"
#include "nsBlockFrame.h"
#include "nsLineBox.h"
#include "nsImageFrame.h"
#include "nsTableFrame.h"
#include "nsTableCellFrame.h"
#include "nsIServiceManager.h"
#include "nsIPercentHeightObserver.h"
#include "nsLayoutUtils.h"
#include "mozilla/Preferences.h"
#ifdef IBMBIDI
#include "nsBidiUtils.h"
#endif

#ifdef NS_DEBUG
#undef NOISY_VERTICAL_ALIGN
#else
#undef NOISY_VERTICAL_ALIGN
#endif

using namespace mozilla;
using namespace mozilla::layout;


static bool sPrefIsLoaded = false;
static bool sBlinkIsAllowed = true;

enum eNormalLineHeightControl {
  eUninitialized = -1,
  eNoExternalLeading = 0,   
  eIncludeExternalLeading,  
  eCompensateLeading        
};

static eNormalLineHeightControl sNormalLineHeightControl = eUninitialized;



nsHTMLReflowState::nsHTMLReflowState(nsPresContext*       aPresContext,
                                     nsIFrame*            aFrame,
                                     nsRenderingContext* aRenderingContext,
                                     const nsSize&        aAvailableSpace)
  : nsCSSOffsetState(aFrame, aRenderingContext)
  , mBlockDelta(0)
  , mReflowDepth(0)
  , mRestoreCurrentInflationContainer(aPresContext->mCurrentInflationContainer)
  , mRestoreCurrentInflationContainerWidth(aPresContext->
                                             mCurrentInflationContainerWidth)
{
  NS_PRECONDITION(aPresContext, "no pres context");
  NS_PRECONDITION(aRenderingContext, "no rendering context");
  NS_PRECONDITION(aFrame, "no frame");
  parentReflowState = nsnull;
  availableWidth = aAvailableSpace.width;
  availableHeight = aAvailableSpace.height;
  mFloatManager = nsnull;
  mLineLayout = nsnull;
  memset(&mFlags, 0, sizeof(mFlags));
  mDiscoveredClearance = nsnull;
  mPercentHeightObserver = nsnull;
  Init(aPresContext);
}

static bool CheckNextInFlowParenthood(nsIFrame* aFrame, nsIFrame* aParent)
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
                                     bool                     aInit)
  : nsCSSOffsetState(aFrame, aParentReflowState.rendContext)
  , mBlockDelta(0)
  , mReflowDepth(aParentReflowState.mReflowDepth + 1)
  , mFlags(aParentReflowState.mFlags)
  , mRestoreCurrentInflationContainer(aPresContext->mCurrentInflationContainer)
  , mRestoreCurrentInflationContainerWidth(aPresContext->
                                             mCurrentInflationContainerWidth)
{
  NS_PRECONDITION(aPresContext, "no pres context");
  NS_PRECONDITION(aFrame, "no frame");
  NS_PRECONDITION((aContainingBlockWidth == -1) ==
                    (aContainingBlockHeight == -1),
                  "cb width and height should only be non-default together");
  NS_PRECONDITION(!mFlags.mSpecialHeightReflow ||
                  !NS_SUBTREE_DIRTY(aFrame),
                  "frame should be clean when getting special height reflow");

  parentReflowState = &aParentReflowState;

  
  
  
  if (!mFlags.mSpecialHeightReflow)
    frame->AddStateBits(parentReflowState->frame->GetStateBits() &
                        NS_FRAME_IS_DIRTY);

  availableWidth = aAvailableSpace.width;
  availableHeight = aAvailableSpace.height;

  mFloatManager = aParentReflowState.mFloatManager;
  if (frame->IsFrameOfType(nsIFrame::eLineParticipant))
    mLineLayout = aParentReflowState.mLineLayout;
  else
    mLineLayout = nsnull;

  
  
  
  mFlags.mNextInFlowUntouched = aParentReflowState.mFlags.mNextInFlowUntouched &&
    CheckNextInFlowParenthood(aFrame, aParentReflowState.frame);
  mFlags.mAssumingHScrollbar = mFlags.mAssumingVScrollbar = false;
  mFlags.mHasClearance = false;
  mFlags.mIsColumnBalancing = false;

  mDiscoveredClearance = nsnull;
  mPercentHeightObserver = (aParentReflowState.mPercentHeightObserver && 
                            aParentReflowState.mPercentHeightObserver->NeedsToObserve(*this)) 
                           ? aParentReflowState.mPercentHeightObserver : nsnull;

  if (aInit) {
    Init(aPresContext, aContainingBlockWidth, aContainingBlockHeight);
  }
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

void
nsHTMLReflowState::SetComputedWidth(nscoord aComputedWidth)
{
  NS_ASSERTION(frame, "Must have a frame!");
  
  
  
  
  
  
  
  
  
  
  
  

  NS_PRECONDITION(aComputedWidth >= 0, "Invalid computed width");
  if (mComputedWidth != aComputedWidth) {
    mComputedWidth = aComputedWidth;
    nsIAtom* frameType = frame->GetType();
    if (frameType != nsGkAtoms::viewportFrame) { 
      InitResizeFlags(frame->PresContext(), frameType);
    }
  }
}

void
nsHTMLReflowState::SetComputedHeight(nscoord aComputedHeight)
{
  NS_ASSERTION(frame, "Must have a frame!");
  
  
  
  
  
  
  
  

  NS_PRECONDITION(aComputedHeight >= 0, "Invalid computed height");
  if (mComputedHeight != aComputedHeight) {
    mComputedHeight = aComputedHeight;
    InitResizeFlags(frame->PresContext(), frame->GetType());
  }
}

void
nsHTMLReflowState::Init(nsPresContext* aPresContext,
                        nscoord         aContainingBlockWidth,
                        nscoord         aContainingBlockHeight,
                        const nsMargin* aBorder,
                        const nsMargin* aPadding)
{
  NS_WARN_IF_FALSE(availableWidth != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained width; this should only result from "
                   "very large sizes, not attempts at intrinsic width "
                   "calculation");

  mStylePosition = frame->GetStylePosition();
  mStyleDisplay = frame->GetStyleDisplay();
  mStyleVisibility = frame->GetStyleVisibility();
  mStyleBorder = frame->GetStyleBorder();
  mStyleMargin = frame->GetStyleMargin();
  mStylePadding = frame->GetStylePadding();
  mStyleText = frame->GetStyleText();

  nsIAtom* type = frame->GetType();

  InitFrameType(type);
  InitCBReflowState();

  InitConstraints(aPresContext, aContainingBlockWidth, aContainingBlockHeight,
                  aBorder, aPadding, type);

  InitResizeFlags(aPresContext, type);

  nsIFrame *parent = frame->GetParent();
  if (parent &&
      (parent->GetStateBits() & NS_FRAME_IN_CONSTRAINED_HEIGHT) &&
      !(parent->GetType() == nsGkAtoms::scrollFrame &&
        parent->GetStyleDisplay()->mOverflowY != NS_STYLE_OVERFLOW_HIDDEN)) {
    frame->AddStateBits(NS_FRAME_IN_CONSTRAINED_HEIGHT);
  } else if ((mStylePosition->mHeight.GetUnit() != eStyleUnit_Auto ||
              mStylePosition->mMaxHeight.GetUnit() != eStyleUnit_None) &&
              
              
             (frame->GetContent() &&
            !(frame->GetContent()->IsHTML(nsGkAtoms::body) ||
              frame->GetContent()->IsHTML(nsGkAtoms::html)))) {
    frame->AddStateBits(NS_FRAME_IN_CONSTRAINED_HEIGHT);
  } else {
    frame->RemoveStateBits(NS_FRAME_IN_CONSTRAINED_HEIGHT);
  }

  NS_WARN_IF_FALSE((mFrameType == NS_CSS_FRAME_TYPE_INLINE &&
                    !frame->IsFrameOfType(nsIFrame::eReplaced)) ||
                   type == nsGkAtoms::textFrame ||
                   mComputedWidth != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained width; this should only result from "
                   "very large sizes, not attempts at intrinsic width "
                   "calculation");
}

void nsHTMLReflowState::InitCBReflowState()
{
  if (!parentReflowState) {
    mCBReflowState = nsnull;
    return;
  }

  if (parentReflowState->frame == frame->GetContainingBlock()) {
    
    
    if (frame->GetType() == nsGkAtoms::tableFrame) {
      mCBReflowState = parentReflowState->mCBReflowState;
    } else {
      mCBReflowState = parentReflowState;
    }
  } else {
    mCBReflowState = parentReflowState->mCBReflowState;
  }
}









static bool
IsQuirkContainingBlockHeight(const nsHTMLReflowState* rs, nsIAtom* aFrameType)
{
  if (nsGkAtoms::blockFrame == aFrameType ||
#ifdef MOZ_XUL
      nsGkAtoms::XULLabelFrame == aFrameType ||
#endif
      nsGkAtoms::scrollFrame == aFrameType) {
    
    
    if (NS_AUTOHEIGHT == rs->ComputedHeight()) {
      if (!rs->frame->GetStyleDisplay()->IsAbsolutelyPositioned()) {
        return false;
      }
    }
  }
  return true;
}


void
nsHTMLReflowState::InitResizeFlags(nsPresContext* aPresContext, nsIAtom* aFrameType)
{
  mFlags.mHResize = !(frame->GetStateBits() & NS_FRAME_IS_DIRTY) &&
                    frame->GetSize().width !=
                      mComputedWidth + mComputedBorderPadding.LeftRight();
  if (mFlags.mHResize &&
      nsLayoutUtils::FontSizeInflationEnabled(aPresContext)) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    
    
    
    frame->AddStateBits(NS_FRAME_IS_DIRTY);
  }

  
  
  if (IS_TABLE_CELL(aFrameType) &&
      (mFlags.mSpecialHeightReflow ||
       (frame->GetFirstInFlow()->GetStateBits() &
         NS_TABLE_CELL_HAD_SPECIAL_REFLOW)) &&
      (frame->GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT)) {
    
    
    
    mFlags.mVResize = true;
  } else if (mCBReflowState && !nsLayoutUtils::IsNonWrapperBlock(frame)) {
    
    
    
    
    mFlags.mVResize = mCBReflowState->mFlags.mVResize;
  } else if (mComputedHeight == NS_AUTOHEIGHT) {
    if (eCompatibility_NavQuirks == aPresContext->CompatibilityMode() &&
        mCBReflowState) {
      mFlags.mVResize = mCBReflowState->mFlags.mVResize;
    } else {
      mFlags.mVResize = mFlags.mHResize;
    }
    mFlags.mVResize = mFlags.mVResize || NS_SUBTREE_DIRTY(frame);
  } else {
    
    mFlags.mVResize = frame->GetSize().height !=
                        mComputedHeight + mComputedBorderPadding.TopBottom();
  }

  bool dependsOnCBHeight =
    (mStylePosition->HeightDependsOnContainer() &&
     
     mStylePosition->mHeight.GetUnit() != eStyleUnit_Auto) ||
    (mStylePosition->MinHeightDependsOnContainer() &&
     
     mStylePosition->mMinHeight.GetUnit() != eStyleUnit_Auto) ||
    (mStylePosition->MaxHeightDependsOnContainer() &&
     
     mStylePosition->mMaxHeight.GetUnit() != eStyleUnit_Auto) ||
    mStylePosition->OffsetHasPercent(NS_SIDE_TOP) ||
    mStylePosition->mOffset.GetBottomUnit() != eStyleUnit_Auto ||
    frame->IsBoxFrame();

  if (mStyleText->mLineHeight.GetUnit() == eStyleUnit_Enumerated) {
    NS_ASSERTION(mStyleText->mLineHeight.GetIntValue() ==
                 NS_STYLE_LINE_HEIGHT_BLOCK_HEIGHT,
                 "bad line-height value");

    
    frame->AddStateBits(NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
    
    dependsOnCBHeight |= !nsLayoutUtils::IsNonWrapperBlock(frame);
  }

  
  
  
  
  
  
  if (!mFlags.mVResize && mCBReflowState &&
      (IS_TABLE_CELL(mCBReflowState->frame->GetType()) || 
       mCBReflowState->mFlags.mHeightDependsOnAncestorCell) &&
      !mCBReflowState->mFlags.mSpecialHeightReflow && 
      dependsOnCBHeight) {
    mFlags.mVResize = true;
    mFlags.mHeightDependsOnAncestorCell = true;
  }

  

  
  
  
  
  
  
  
  if (dependsOnCBHeight && mCBReflowState) {
    const nsHTMLReflowState *rs = this;
    bool hitCBReflowState = false;
    do {
      rs = rs->parentReflowState;
      if (!rs) {
        break;
      }
        
      if (rs->frame->GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT)
        break; 
      rs->frame->AddStateBits(NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
      
      
      
      if (rs == mCBReflowState) {
        hitCBReflowState = true;
      }

    } while (!hitCBReflowState ||
             (eCompatibility_NavQuirks == aPresContext->CompatibilityMode() &&
              !IsQuirkContainingBlockHeight(rs, rs->frame->GetType())));
    
    
    
    
    
    
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

void
nsHTMLReflowState::InitFrameType(nsIAtom* aFrameType)
{
  const nsStyleDisplay *disp = mStyleDisplay;
  nsCSSFrameType frameType;

  
  
  
  

  
  

  DISPLAY_INIT_TYPE(frame, this);

  if (aFrameType == nsGkAtoms::tableFrame) {
    mFrameType = NS_CSS_FRAME_TYPE_BLOCK;
    return;
  }

  NS_ASSERTION(frame->GetStyleDisplay()->IsAbsolutelyPositioned() ==
                 disp->IsAbsolutelyPositioned(),
               "Unexpected position style");
  NS_ASSERTION(frame->GetStyleDisplay()->IsFloating() ==
                 disp->IsFloating(), "Unexpected float style");
  if (frame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
    if (disp->IsAbsolutelyPositioned()) {
      frameType = NS_CSS_FRAME_TYPE_ABSOLUTE;
      
      
      if (frame->GetPrevInFlow())
        frameType = NS_CSS_FRAME_TYPE_BLOCK;
    }
    else if (disp->IsFloating()) {
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
    case NS_STYLE_DISPLAY_INLINE_TABLE:
    case NS_STYLE_DISPLAY_INLINE_BOX:
    case NS_STYLE_DISPLAY_INLINE_GRID:
    case NS_STYLE_DISPLAY_INLINE_STACK:
      frameType = NS_CSS_FRAME_TYPE_INLINE;
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
                                          nscoord aContainingBlockHeight,
                                          nsPresContext* aPresContext)
{
  
  
  
  bool    leftIsAuto = eStyleUnit_Auto == mStylePosition->mOffset.GetLeftUnit();
  bool    rightIsAuto = eStyleUnit_Auto == mStylePosition->mOffset.GetRightUnit();

  
  
  if (!leftIsAuto && !rightIsAuto) {
    if (mCBReflowState &&
        NS_STYLE_DIRECTION_RTL == mCBReflowState->mStyleVisibility->mDirection) {
      leftIsAuto = true;
    } else {
      rightIsAuto = true;
    }
  }

  if (leftIsAuto) {
    if (rightIsAuto) {
      
      mComputedOffsets.left = mComputedOffsets.right = 0;
    } else {
      
      mComputedOffsets.right = nsLayoutUtils::
        ComputeWidthDependentValue(aContainingBlockWidth,
                                   mStylePosition->mOffset.GetRight());

      
      mComputedOffsets.left = -mComputedOffsets.right;
    }

  } else {
    NS_ASSERTION(rightIsAuto, "unexpected specified constraint");
    
    
    mComputedOffsets.left = nsLayoutUtils::
      ComputeWidthDependentValue(aContainingBlockWidth,
                                 mStylePosition->mOffset.GetLeft());

    
    mComputedOffsets.right = -mComputedOffsets.left;
  }

  
  
  
  bool    topIsAuto = eStyleUnit_Auto == mStylePosition->mOffset.GetTopUnit();
  bool    bottomIsAuto = eStyleUnit_Auto == mStylePosition->mOffset.GetBottomUnit();

  
  
  if (NS_AUTOHEIGHT == aContainingBlockHeight) {
    if (mStylePosition->OffsetHasPercent(NS_SIDE_TOP)) {
      topIsAuto = true;
    }
    if (mStylePosition->OffsetHasPercent(NS_SIDE_BOTTOM)) {
      bottomIsAuto = true;
    }
  }

  
  if (!topIsAuto && !bottomIsAuto) {
    bottomIsAuto = true;
  }

  if (topIsAuto) {
    if (bottomIsAuto) {
      
      mComputedOffsets.top = mComputedOffsets.bottom = 0;
    } else {
      
      mComputedOffsets.bottom = nsLayoutUtils::
        ComputeHeightDependentValue(aContainingBlockHeight,
                                    mStylePosition->mOffset.GetBottom());
      
      
      mComputedOffsets.top = -mComputedOffsets.bottom;
    }

  } else {
    NS_ASSERTION(bottomIsAuto, "unexpected specified constraint");
    
    
    mComputedOffsets.top = nsLayoutUtils::
      ComputeHeightDependentValue(aContainingBlockHeight,
                                  mStylePosition->mOffset.GetTop());

    
    mComputedOffsets.bottom = -mComputedOffsets.top;
  }

  
  FrameProperties props(aPresContext->PropertyTable(), frame);
  nsPoint* offsets = static_cast<nsPoint*>
    (props.Get(nsIFrame::ComputedOffsetProperty()));
  if (offsets) {
    offsets->MoveTo(mComputedOffsets.left, mComputedOffsets.top);
  } else {
    props.Set(nsIFrame::ComputedOffsetProperty(),
              new nsPoint(mComputedOffsets.left, mComputedOffsets.top));
  }
}

nsIFrame*
nsHTMLReflowState::GetHypotheticalBoxContainer(nsIFrame* aFrame,
                                               nscoord& aCBLeftEdge,
                                               nscoord& aCBWidth)
{
  aFrame = aFrame->GetContainingBlock();
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
#ifdef DEBUG
  bool          mLeftIsExact, mRightIsExact;
#endif

  nsHypotheticalBox() {
#ifdef DEBUG
    mLeftIsExact = mRightIsExact = false;
#endif
  }
};
      
static bool
GetIntrinsicSizeFor(nsIFrame* aFrame, nsSize& aIntrinsicSize, nsIAtom* aFrameType)
{
  
  bool success = false;

  
  
  
  
  if (aFrameType == nsGkAtoms::imageFrame) {
    nsImageFrame* imageFrame = (nsImageFrame*)aFrame;

    if (NS_SUCCEEDED(imageFrame->GetIntrinsicImageSize(aIntrinsicSize))) {
      success = (aIntrinsicSize != nsSize(0, 0));
    }
  }
  return success;
}






void
nsHTMLReflowState::CalculateHorizBorderPaddingMargin(
                       nscoord aContainingBlockWidth,
                       nscoord* aInsideBoxSizing,
                       nscoord* aOutsideBoxSizing)
{
  const nsMargin& border = mStyleBorder->GetActualBorder();
  nsMargin padding, margin;

  
  if (!mStylePadding->GetPadding(padding)) {
    
    padding.left = nsLayoutUtils::
      ComputeWidthDependentValue(aContainingBlockWidth,
                                 mStylePadding->mPadding.GetLeft());
    padding.right = nsLayoutUtils::
      ComputeWidthDependentValue(aContainingBlockWidth,
                                 mStylePadding->mPadding.GetRight());
  }

  
  if (!mStyleMargin->GetMargin(margin)) {
    
    if (eStyleUnit_Auto == mStyleMargin->mMargin.GetLeftUnit()) {
      
      margin.left = 0;  
    } else {
      margin.left = nsLayoutUtils::
        ComputeWidthDependentValue(aContainingBlockWidth,
                                   mStyleMargin->mMargin.GetLeft());
    }
    if (eStyleUnit_Auto == mStyleMargin->mMargin.GetRightUnit()) {
      
      margin.right = 0;  
    } else {
      margin.right = nsLayoutUtils::
        ComputeWidthDependentValue(aContainingBlockWidth,
                                   mStyleMargin->mMargin.GetRight());
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





static bool AreAllEarlierInFlowFramesEmpty(nsIFrame* aFrame,
  nsIFrame* aDescendant, bool* aFound) {
  if (aFrame == aDescendant) {
    *aFound = true;
    return true;
  }
  if (!aFrame->IsSelfEmpty()) {
    *aFound = false;
    return false;
  }
  for (nsIFrame* f = aFrame->GetFirstPrincipalChild(); f; f = f->GetNextSibling()) {
    bool allEmpty = AreAllEarlierInFlowFramesEmpty(f, aDescendant, aFound);
    if (*aFound || !allEmpty) {
      return allEmpty;
    }
  }
  *aFound = false;
  return true;
}






void
nsHTMLReflowState::CalculateHypotheticalBox(nsPresContext*    aPresContext,
                                            nsIFrame*         aPlaceholderFrame,
                                            nsIFrame*         aContainingBlock,
                                            nscoord           aBlockLeftContentEdge,
                                            nscoord           aBlockContentWidth,
                                            const nsHTMLReflowState* cbrs,
                                            nsHypotheticalBox& aHypotheticalBox,
                                            nsIAtom*          aFrameType)
{
  NS_ASSERTION(mStyleDisplay->mOriginalDisplay != NS_STYLE_DISPLAY_NONE,
               "mOriginalDisplay has not been properly initialized");
  
  
  
  
  bool isAutoWidth = mStylePosition->mWidth.GetUnit() == eStyleUnit_Auto;
  nsSize      intrinsicSize;
  bool        knowIntrinsicSize = false;
  if (NS_FRAME_IS_REPLACED(mFrameType) && isAutoWidth) {
    
    knowIntrinsicSize = GetIntrinsicSizeFor(frame, intrinsicSize, aFrameType);
  }

  
  
  nscoord boxWidth;
  bool    knowBoxWidth = false;
  if ((NS_STYLE_DISPLAY_INLINE == mStyleDisplay->mOriginalDisplay) &&
      !NS_FRAME_IS_REPLACED(mFrameType)) {
    
    

  } else {
    

    
    
    
    nscoord insideBoxSizing, outsideBoxSizing;
    CalculateHorizBorderPaddingMargin(aBlockContentWidth,
                                      &insideBoxSizing, &outsideBoxSizing);

    if (NS_FRAME_IS_REPLACED(mFrameType) && isAutoWidth) {
      
      
      if (knowIntrinsicSize) {
        boxWidth = intrinsicSize.width + outsideBoxSizing + insideBoxSizing;
        knowBoxWidth = true;
      }

    } else if (isAutoWidth) {
      
      boxWidth = aBlockContentWidth;
      knowBoxWidth = true;
    
    } else {
      
      
      
      boxWidth = ComputeWidthValue(aBlockContentWidth,
                                   insideBoxSizing, outsideBoxSizing,
                                   mStylePosition->mWidth) + 
                 insideBoxSizing + outsideBoxSizing;
      knowBoxWidth = true;
    }
  }
  
  
  const nsStyleVisibility* blockVis = aContainingBlock->GetStyleVisibility();

  
  
  
  
  nsPoint placeholderOffset = aPlaceholderFrame->GetOffsetTo(aContainingBlock);

  
  
  
  
  nsBlockFrame* blockFrame =
    nsLayoutUtils::GetAsBlock(aContainingBlock->GetContentInsertionFrame());
  if (blockFrame) {
    nscoord blockYOffset = blockFrame->GetOffsetTo(aContainingBlock).y;
    bool isValid;
    nsBlockInFlowLineIterator iter(blockFrame, aPlaceholderFrame, &isValid);
    if (!isValid) {
      
      
      aHypotheticalBox.mTop = placeholderOffset.y;
    } else {
      NS_ASSERTION(iter.GetContainer() == blockFrame,
                   "Found placeholder in wrong block!");
      nsBlockFrame::line_iterator lineBox = iter.GetLine();

      
      
      if (mStyleDisplay->IsOriginalDisplayInlineOutside()) {
        
        
        aHypotheticalBox.mTop = lineBox->mBounds.y + blockYOffset;
      } else {
        
        
        
        
        
        
        if (lineBox != iter.End()) {
          nsIFrame * firstFrame = lineBox->mFirstChild;
          bool found = false;
          bool allEmpty = true;
          while (firstFrame) { 
            allEmpty = AreAllEarlierInFlowFramesEmpty(firstFrame,
              aPlaceholderFrame, &found);
            if (found || !allEmpty)
              break;
            firstFrame = firstFrame->GetNextSibling();
          }
          NS_ASSERTION(firstFrame, "Couldn't find placeholder!");

          if (allEmpty) {
            
            
            
            aHypotheticalBox.mTop = lineBox->mBounds.y + blockYOffset;
          } else {
            
            
            aHypotheticalBox.mTop = lineBox->mBounds.YMost() + blockYOffset;
          }
        } else {
          
          aHypotheticalBox.mTop = placeholderOffset.y;
        }
      }
    }
  } else {
    
    
    
    aHypotheticalBox.mTop = placeholderOffset.y;
  }

  
  
  if (NS_STYLE_DIRECTION_LTR == blockVis->mDirection) {
    
    
    if (mStyleDisplay->IsOriginalDisplayInlineOutside()) {
      
      aHypotheticalBox.mLeft = placeholderOffset.x;
    } else {
      aHypotheticalBox.mLeft = aBlockLeftContentEdge;
    }
#ifdef DEBUG
    aHypotheticalBox.mLeftIsExact = true;
#endif

    if (knowBoxWidth) {
      aHypotheticalBox.mRight = aHypotheticalBox.mLeft + boxWidth;
#ifdef DEBUG
      aHypotheticalBox.mRightIsExact = true;
#endif
    } else {
      
      
      
      aHypotheticalBox.mRight = aBlockLeftContentEdge + aBlockContentWidth;
#ifdef DEBUG
      aHypotheticalBox.mRightIsExact = false;
#endif
    }

  } else {
    
    if (mStyleDisplay->IsOriginalDisplayInlineOutside()) {
      aHypotheticalBox.mRight = placeholderOffset.x;
    } else {
      aHypotheticalBox.mRight = aBlockLeftContentEdge + aBlockContentWidth;
    }
#ifdef DEBUG
    aHypotheticalBox.mRightIsExact = true;
#endif
    
    if (knowBoxWidth) {
      aHypotheticalBox.mLeft = aHypotheticalBox.mRight - boxWidth;
#ifdef DEBUG
      aHypotheticalBox.mLeftIsExact = true;
#endif
    } else {
      
      
      
      aHypotheticalBox.mLeft = aBlockLeftContentEdge;
#ifdef DEBUG
      aHypotheticalBox.mLeftIsExact = false;
#endif
    }

  }

  
  
  
  
  
  nsPoint cbOffset;
  if (mStyleDisplay->mPosition == NS_STYLE_POSITION_FIXED &&
      
      nsLayoutUtils::IsReallyFixedPos(frame)) {
    
    
    
    
    
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
  aHypotheticalBox.mRight -= border.left;
  aHypotheticalBox.mTop -= border.top;
}

void
nsHTMLReflowState::InitAbsoluteConstraints(nsPresContext* aPresContext,
                                           const nsHTMLReflowState* cbrs,
                                           nscoord containingBlockWidth,
                                           nscoord containingBlockHeight,
                                           nsIAtom* aFrameType)
{
  NS_PRECONDITION(containingBlockHeight != NS_AUTOHEIGHT,
                  "containing block height must be constrained");

  NS_ASSERTION(aFrameType != nsGkAtoms::tableFrame,
               "InitAbsoluteConstraints should not be called on table frames");
  NS_ASSERTION(frame->GetStateBits() & NS_FRAME_OUT_OF_FLOW,
               "Why are we here?");

  
  nsIFrame*     placeholderFrame;

  placeholderFrame = aPresContext->PresShell()->GetPlaceholderFrameFor(frame);
  NS_ASSERTION(nsnull != placeholderFrame, "no placeholder frame");

  
  
  
  nsHypotheticalBox hypotheticalBox;
  if (((eStyleUnit_Auto == mStylePosition->mOffset.GetLeftUnit()) &&
       (eStyleUnit_Auto == mStylePosition->mOffset.GetRightUnit())) ||
      ((eStyleUnit_Auto == mStylePosition->mOffset.GetTopUnit()) &&
       (eStyleUnit_Auto == mStylePosition->mOffset.GetBottomUnit()))) {
    
    
    nscoord cbLeftEdge, cbWidth;
    nsIFrame* cbFrame = GetHypotheticalBoxContainer(placeholderFrame,
                                                    cbLeftEdge,
                                                    cbWidth);

    CalculateHypotheticalBox(aPresContext, placeholderFrame, cbFrame,
                             cbLeftEdge, cbWidth, cbrs, hypotheticalBox, aFrameType);
  }

  
  
  bool          leftIsAuto = false, rightIsAuto = false;
  if (eStyleUnit_Auto == mStylePosition->mOffset.GetLeftUnit()) {
    mComputedOffsets.left = 0;
    leftIsAuto = true;
  } else {
    mComputedOffsets.left = nsLayoutUtils::
      ComputeWidthDependentValue(containingBlockWidth,
                                 mStylePosition->mOffset.GetLeft());
  }
  if (eStyleUnit_Auto == mStylePosition->mOffset.GetRightUnit()) {
    mComputedOffsets.right = 0;
    rightIsAuto = true;
  } else {
    mComputedOffsets.right = nsLayoutUtils::
      ComputeWidthDependentValue(containingBlockWidth,
                                 mStylePosition->mOffset.GetRight());
  }

  
  
  if (leftIsAuto && rightIsAuto) {
    
    
    if (NS_STYLE_DIRECTION_LTR == placeholderFrame->GetContainingBlock()
                                    ->GetStyleVisibility()->mDirection) {
      NS_ASSERTION(hypotheticalBox.mLeftIsExact, "should always have "
                   "exact value on containing block's start side");
      mComputedOffsets.left = hypotheticalBox.mLeft;
      leftIsAuto = false;
    } else {
      NS_ASSERTION(hypotheticalBox.mRightIsExact, "should always have "
                   "exact value on containing block's start side");
      mComputedOffsets.right = containingBlockWidth - hypotheticalBox.mRight;
      rightIsAuto = false;
    }
  }

  
  bool        topIsAuto = false, bottomIsAuto = false;
  if (eStyleUnit_Auto == mStylePosition->mOffset.GetTopUnit()) {
    mComputedOffsets.top = 0;
    topIsAuto = true;
  } else {
    mComputedOffsets.top = nsLayoutUtils::
      ComputeHeightDependentValue(containingBlockHeight,
                                  mStylePosition->mOffset.GetTop());
  }
  if (eStyleUnit_Auto == mStylePosition->mOffset.GetBottomUnit()) {
    mComputedOffsets.bottom = 0;        
    bottomIsAuto = true;
  } else {
    mComputedOffsets.bottom = nsLayoutUtils::
      ComputeHeightDependentValue(containingBlockHeight,
                                  mStylePosition->mOffset.GetBottom());
  }

  if (topIsAuto && bottomIsAuto) {
    
    mComputedOffsets.top = hypotheticalBox.mTop;
    topIsAuto = false;
  }

  bool widthIsAuto = eStyleUnit_Auto == mStylePosition->mWidth.GetUnit();
  bool heightIsAuto = eStyleUnit_Auto == mStylePosition->mHeight.GetUnit();

  PRUint32 computeSizeFlags = 0;
  if (leftIsAuto || rightIsAuto) {
    computeSizeFlags |= nsIFrame::eShrinkWrap;
  }

  {
    AutoMaybeNullInflationContainer an(frame);

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
                         computeSizeFlags);
    mComputedWidth = size.width;
    mComputedHeight = size.height;
  }
  NS_ASSERTION(mComputedWidth >= 0, "Bogus width");
  NS_ASSERTION(mComputedHeight == NS_UNCONSTRAINEDSIZE ||
               mComputedHeight >= 0, "Bogus height");

  
  

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
    bool marginLeftIsAuto =
      eStyleUnit_Auto == mStyleMargin->mMargin.GetLeftUnit();
    bool marginRightIsAuto =
      eStyleUnit_Auto == mStyleMargin->mMargin.GetRightUnit();

    if (marginLeftIsAuto) {
      if (marginRightIsAuto) {
        if (availMarginSpace < 0) {
          
          
          if (cbrs &&
              NS_STYLE_DIRECTION_RTL == cbrs->mStyleVisibility->mDirection) {
            
            mComputedMargin.left = availMarginSpace;
          } else {
            
            mComputedMargin.right = availMarginSpace;
          }
        } else {
          
          
          mComputedMargin.left = availMarginSpace / 2;
          mComputedMargin.right = availMarginSpace - mComputedMargin.left;
        }
      } else {
        
        mComputedMargin.left = availMarginSpace;
      }
    } else {
      if (marginRightIsAuto) {
        
        mComputedMargin.right = availMarginSpace;
      } else {
        
        
        
        
        
        
        
        if (cbrs &&
            NS_STYLE_DIRECTION_RTL == cbrs->mStyleVisibility->mDirection) {
          
          mComputedOffsets.left += availMarginSpace;
        } else {
          
          mComputedOffsets.right += availMarginSpace;
        }
      }
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
    bool marginTopIsAuto =
      eStyleUnit_Auto == mStyleMargin->mMargin.GetTopUnit();
    bool marginBottomIsAuto =
      eStyleUnit_Auto == mStyleMargin->mMargin.GetBottomUnit();

    if (marginTopIsAuto) {
      if (marginBottomIsAuto) {
        if (availMarginSpace < 0) {
          
          mComputedMargin.bottom = availMarginSpace;
        } else {
          
          
          mComputedMargin.top = availMarginSpace / 2;
          mComputedMargin.bottom = availMarginSpace - mComputedMargin.top;
        }
      } else {
        
        mComputedMargin.top = availMarginSpace;
      }
    } else {
      if (marginBottomIsAuto) {
        
        mComputedMargin.bottom = availMarginSpace;
      } else {
        
        
        
        mComputedOffsets.bottom += availMarginSpace;
      }
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











static nscoord
CalcQuirkContainingBlockHeight(const nsHTMLReflowState* aCBReflowState)
{
  nsHTMLReflowState* firstAncestorRS = nsnull; 
  nsHTMLReflowState* secondAncestorRS = nsnull; 
  
  
  
  
  nscoord result = NS_AUTOHEIGHT; 
                             
  const nsHTMLReflowState* rs = aCBReflowState;
  for (; rs; rs = (nsHTMLReflowState *)(rs->parentReflowState)) { 
    nsIAtom* frameType = rs->frame->GetType();
    
    
    if (nsGkAtoms::blockFrame == frameType ||
#ifdef MOZ_XUL
        nsGkAtoms::XULLabelFrame == frameType ||
#endif
        nsGkAtoms::scrollFrame == frameType) {

      secondAncestorRS = firstAncestorRS;
      firstAncestorRS = (nsHTMLReflowState*)rs;

      
      
      
      
      if (NS_AUTOHEIGHT == rs->ComputedHeight()) {
        if (rs->frame->GetStyleDisplay()->IsAbsolutelyPositioned()) {
          break;
        } else {
          continue;
        }
      }
    }
    else if (nsGkAtoms::canvasFrame == frameType) {
      
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
             ? rs->availableHeight : rs->ComputedHeight();
    
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
    
    else if (nsGkAtoms::blockFrame == frameType &&
             rs->parentReflowState &&
             nsGkAtoms::canvasFrame ==
               rs->parentReflowState->frame->GetType()) {
      
      result -= GetVerticalMarginBorderPadding(secondAncestorRS);
    }
    break;
  }

  
  return NS_MAX(result, 0);
}


void
nsHTMLReflowState::ComputeContainingBlockRectangle(nsPresContext*          aPresContext,
                                                   const nsHTMLReflowState* aContainingBlockRS,
                                                   nscoord&                 aContainingBlockWidth,
                                                   nscoord&                 aContainingBlockHeight)
{
  
  
  aContainingBlockWidth = aContainingBlockRS->mComputedWidth;
  aContainingBlockHeight = aContainingBlockRS->mComputedHeight;

  
  
  if (NS_FRAME_GET_TYPE(mFrameType) == NS_CSS_FRAME_TYPE_ABSOLUTE ||
      (frame->GetType() == nsGkAtoms::tableFrame &&
       frame->GetStyleDisplay()->IsAbsolutelyPositioned() &&
       (frame->GetParent()->GetStateBits() & NS_FRAME_OUT_OF_FLOW))) {
    
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
      aContainingBlockHeight += aContainingBlockRS->mComputedPadding.TopBottom();
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


static int
PrefsChanged(const char *aPrefName, void *instance)
{
  sBlinkIsAllowed =
    Preferences::GetBool("browser.blink_allowed", sBlinkIsAllowed);

  return 0; 
}




static bool BlinkIsAllowed(void)
{
  if (!sPrefIsLoaded) {
    
    Preferences::RegisterCallback(PrefsChanged, "browser.blink_allowed");
    PrefsChanged(nsnull, nsnull);
    sPrefIsLoaded = true;
  }
  return sBlinkIsAllowed;
}

static eNormalLineHeightControl GetNormalLineHeightCalcControl(void)
{
  if (sNormalLineHeightControl == eUninitialized) {
    
    
    PRInt32 val =
      Preferences::GetInt("browser.display.normal_lineheight_calc_control",
                          eNoExternalLeading);
    sNormalLineHeightControl = static_cast<eNormalLineHeightControl>(val);
  }
  return sNormalLineHeightControl;
}

static inline bool
IsSideCaption(nsIFrame* aFrame, const nsStyleDisplay* aStyleDisplay)
{
  if (aStyleDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE_CAPTION)
    return false;
  PRUint8 captionSide = aFrame->GetStyleTableBorder()->mCaptionSide;
  return captionSide == NS_STYLE_CAPTION_SIDE_LEFT ||
         captionSide == NS_STYLE_CAPTION_SIDE_RIGHT;
}




void
nsHTMLReflowState::InitConstraints(nsPresContext* aPresContext,
                                   nscoord         aContainingBlockWidth,
                                   nscoord         aContainingBlockHeight,
                                   const nsMargin* aBorder,
                                   const nsMargin* aPadding,
                                   nsIAtom* aFrameType)
{
  DISPLAY_INIT_CONSTRAINTS(frame, this,
                           aContainingBlockWidth, aContainingBlockHeight,
                           aBorder, aPadding);

  
  
  if (nsnull == parentReflowState) {
    
    InitOffsets(aContainingBlockWidth, aFrameType, aBorder, aPadding);
    
    
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

    
    
    nsIAtom* fType;
    if (NS_AUTOHEIGHT == aContainingBlockHeight) {
      
      
      
      if (cbrs->parentReflowState) {
        fType = cbrs->frame->GetType();
        if (IS_TABLE_CELL(fType)) {
          
          aContainingBlockHeight = cbrs->mComputedHeight;
        }
      }
    }

    InitOffsets(aContainingBlockWidth, aFrameType, aBorder, aPadding);

    const nsStyleCoord &height = mStylePosition->mHeight;
    nsStyleUnit heightUnit = height.GetUnit();

    
    
    
    
    if (height.HasPercent()) {
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
      ComputeRelativeOffsets(cbrs, aContainingBlockWidth, aContainingBlockHeight, aPresContext);
    } else {
      
      mComputedOffsets.SizeTo(0, 0, 0, 0);
    }

    
    
    ComputeMinMaxValues(aContainingBlockWidth, aContainingBlockHeight, cbrs);

    

    if (NS_CSS_FRAME_TYPE_INTERNAL_TABLE == mFrameType) {
      
      
      bool rowOrRowGroup = false;
      const nsStyleCoord &width = mStylePosition->mWidth;
      nsStyleUnit widthUnit = width.GetUnit();
      if ((NS_STYLE_DISPLAY_TABLE_ROW == mStyleDisplay->mDisplay) ||
          (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == mStyleDisplay->mDisplay)) {
        
        widthUnit = eStyleUnit_Auto;
        rowOrRowGroup = true;
      }

      
      if (eStyleUnit_Auto == widthUnit || width.IsCalcUnit()) {
        mComputedWidth = availableWidth;

        if ((mComputedWidth != NS_UNCONSTRAINEDSIZE) && !rowOrRowGroup){
          
          
          mComputedWidth -= mComputedBorderPadding.left +
            mComputedBorderPadding.right;
          if (mComputedWidth < 0)
            mComputedWidth = 0;
        }
        NS_ASSERTION(mComputedWidth >= 0, "Bogus computed width");
      
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
      
      if (eStyleUnit_Auto == heightUnit || height.IsCalcUnit()) {
        mComputedHeight = NS_AUTOHEIGHT;
      } else {
        NS_ASSERTION(heightUnit == mStylePosition->mHeight.GetUnit(),
                     "unexpected height unit change");
        mComputedHeight = nsLayoutUtils::
          ComputeHeightValue(aContainingBlockHeight, mStylePosition->mHeight);
      }

      
      mComputedMinWidth = mComputedMinHeight = 0;
      mComputedMaxWidth = mComputedMaxHeight = NS_UNCONSTRAINEDSIZE;

    } else if (NS_FRAME_GET_TYPE(mFrameType) == NS_CSS_FRAME_TYPE_ABSOLUTE) {
      
      InitAbsoluteConstraints(aPresContext, cbrs, aContainingBlockWidth,
                              aContainingBlockHeight, aFrameType);
    } else {
      AutoMaybeNullInflationContainer an(frame);

      bool isBlock = NS_CSS_FRAME_TYPE_BLOCK == NS_FRAME_GET_TYPE(mFrameType);
      PRUint32 computeSizeFlags = isBlock ? 0 : nsIFrame::eShrinkWrap;

      
      
      if (isBlock &&
          ((aFrameType == nsGkAtoms::legendFrame &&
            frame->GetStyleContext()->GetPseudo() != nsCSSAnonBoxes::scrolledContent) ||
           (aFrameType == nsGkAtoms::scrollFrame &&
            frame->GetContentInsertionFrame()->GetType() == nsGkAtoms::legendFrame))) {
        computeSizeFlags |= nsIFrame::eShrinkWrap;
      }

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
                           computeSizeFlags);

      mComputedWidth = size.width;
      mComputedHeight = size.height;
      NS_ASSERTION(mComputedWidth >= 0, "Bogus width");
      NS_ASSERTION(mComputedHeight == NS_UNCONSTRAINEDSIZE ||
                   mComputedHeight >= 0, "Bogus height");

      
      if (isBlock && !IsSideCaption(frame, mStyleDisplay) &&
          frame->GetStyleDisplay()->mDisplay != NS_STYLE_DISPLAY_INLINE_TABLE)
        CalculateBlockSideMargins(availableWidth, mComputedWidth, aFrameType);
    }
  }
  
  mFlags.mBlinks = (parentReflowState && parentReflowState->mFlags.mBlinks);
  if (!mFlags.mBlinks && BlinkIsAllowed()) {
    const nsStyleTextReset* st = frame->GetStyleTextReset();
    mFlags.mBlinks = (st->mTextBlink != NS_STYLE_TEXT_BLINK_NONE);
  }

  if (nsLayoutUtils::IsContainerForFontSizeInflation(frame)) {
    aPresContext->mCurrentInflationContainer = frame;
    aPresContext->mCurrentInflationContainerWidth = mComputedWidth;
  }
}

static void
UpdateProp(FrameProperties& aProps,
           const FramePropertyDescriptor* aProperty,
           bool aNeeded,
           nsMargin& aNewValue)
{
  if (aNeeded) {
    nsMargin* propValue = static_cast<nsMargin*>(aProps.Get(aProperty));
    if (propValue) {
      *propValue = aNewValue;
    } else {
      aProps.Set(aProperty, new nsMargin(aNewValue));
    }
  } else {
    aProps.Delete(aProperty);
  }
}

void
nsCSSOffsetState::InitOffsets(nscoord aContainingBlockWidth,
                              nsIAtom* aFrameType,
                              const nsMargin *aBorder,
                              const nsMargin *aPadding)
{
  DISPLAY_INIT_OFFSETS(frame, this, aContainingBlockWidth, aBorder, aPadding);

  
  
  nsPresContext *presContext = frame->PresContext();
  FrameProperties props(presContext->PropertyTable(), frame);
  props.Delete(nsIFrame::UsedBorderProperty());

  
  
  
  
  bool needMarginProp = ComputeMargin(aContainingBlockWidth);
  
  
  
  
  ::UpdateProp(props, nsIFrame::UsedMarginProperty(), needMarginProp,
               mComputedMargin);


  const nsStyleDisplay *disp = frame->GetStyleDisplay();
  bool isThemed = frame->IsThemed(disp);
  bool needPaddingProp;
  nsIntMargin widget;
  if (isThemed &&
      presContext->GetTheme()->GetWidgetPadding(presContext->DeviceContext(),
                                                frame, disp->mAppearance,
                                                &widget)) {
    mComputedPadding.top = presContext->DevPixelsToAppUnits(widget.top);
    mComputedPadding.right = presContext->DevPixelsToAppUnits(widget.right);
    mComputedPadding.bottom = presContext->DevPixelsToAppUnits(widget.bottom);
    mComputedPadding.left = presContext->DevPixelsToAppUnits(widget.left);
    needPaddingProp = false;
  }
  else if (aPadding) { 
    mComputedPadding = *aPadding;
    needPaddingProp = frame->GetStylePadding()->IsWidthDependent();
  }
  else {
    needPaddingProp = ComputePadding(aContainingBlockWidth, aFrameType);
  }

  if (isThemed) {
    nsIntMargin widget;
    presContext->GetTheme()->GetWidgetBorder(presContext->DeviceContext(),
                                             frame, disp->mAppearance,
                                             &widget);
    mComputedBorderPadding.top =
      presContext->DevPixelsToAppUnits(widget.top);
    mComputedBorderPadding.right =
      presContext->DevPixelsToAppUnits(widget.right);
    mComputedBorderPadding.bottom =
      presContext->DevPixelsToAppUnits(widget.bottom);
    mComputedBorderPadding.left =
      presContext->DevPixelsToAppUnits(widget.left);
  }
  else if (aBorder) {  
    mComputedBorderPadding = *aBorder;
  }
  else {
    mComputedBorderPadding = frame->GetStyleBorder()->GetActualBorder();
  }
  mComputedBorderPadding += mComputedPadding;

  if (aFrameType == nsGkAtoms::tableFrame) {
    nsTableFrame *tableFrame = static_cast<nsTableFrame*>(frame);

    if (tableFrame->IsBorderCollapse()) {
      
      
      
      
      mComputedPadding.SizeTo(0,0,0,0);
      mComputedBorderPadding = tableFrame->GetIncludedOuterBCBorder();
    }

    
    
    mComputedMargin.SizeTo(0, 0, 0, 0);
  } else if (aFrameType == nsGkAtoms::scrollbarFrame) {
    
    
    
    nsSize size(frame->GetSize());
    if (size.width == 0 || size.height == 0) {
      mComputedPadding.SizeTo(0,0,0,0);
      mComputedBorderPadding.SizeTo(0,0,0,0);
    }
  }
  ::UpdateProp(props, nsIFrame::UsedPaddingProperty(), needPaddingProp,
               mComputedPadding);
}








void
nsHTMLReflowState::CalculateBlockSideMargins(nscoord aAvailWidth,
                                             nscoord aComputedWidth,
                                             nsIAtom* aFrameType)
{
  NS_WARN_IF_FALSE(NS_UNCONSTRAINEDSIZE != aComputedWidth &&
                   NS_UNCONSTRAINEDSIZE != aAvailWidth,
                   "have unconstrained width; this should only result from "
                   "very large sizes, not attempts at intrinsic width "
                   "calculation");

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

  
  
  bool isAutoLeftMargin =
    eStyleUnit_Auto == mStyleMargin->mMargin.GetLeftUnit();
  bool isAutoRightMargin =
    eStyleUnit_Auto == mStyleMargin->mMargin.GetRightUnit();
  if (!isAutoLeftMargin && !isAutoRightMargin) {
    
    
    
    
    const nsHTMLReflowState* prs = parentReflowState;
    if (aFrameType == nsGkAtoms::tableFrame) {
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
      isAutoLeftMargin = true;
    }
    else {
      isAutoRightMargin = true;
    }
  }

  
  
  

  if (isAutoLeftMargin) {
    if (isAutoRightMargin) {
      
      nscoord forLeft = availMarginSpace / 2;
      mComputedMargin.left  += forLeft;
      mComputedMargin.right += availMarginSpace - forLeft;
    } else {
      mComputedMargin.left += availMarginSpace;
    }
  } else if (isAutoRightMargin) {
    mComputedMargin.right += availMarginSpace;
  }
}

#define NORMAL_LINE_HEIGHT_FACTOR 1.2f    // in term of emHeight 








static nscoord
GetNormalLineHeight(nsFontMetrics* aFontMetrics)
{
  NS_PRECONDITION(nsnull != aFontMetrics, "no font metrics");

  nscoord normalLineHeight;

  nscoord externalLeading = aFontMetrics->ExternalLeading();
  nscoord internalLeading = aFontMetrics->InternalLeading();
  nscoord emHeight = aFontMetrics->EmHeight();
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
  return normalLineHeight;
}

static inline nscoord
ComputeLineHeight(nsStyleContext* aStyleContext,
                  nscoord aBlockHeight,
                  float aFontSizeInflation)
{
  const nsStyleCoord& lhCoord = aStyleContext->GetStyleText()->mLineHeight;

  if (lhCoord.GetUnit() == eStyleUnit_Coord) {
    nscoord result = lhCoord.GetCoordValue();
    if (aFontSizeInflation != 1.0f) {
      result = NSToCoordRound(result * aFontSizeInflation);
    }
    return result;
  }

  if (lhCoord.GetUnit() == eStyleUnit_Factor)
    
    
    
    return NSToCoordRound(lhCoord.GetFactorValue() * aFontSizeInflation *
                          aStyleContext->GetStyleFont()->mFont.size);

  NS_ASSERTION(lhCoord.GetUnit() == eStyleUnit_Normal ||
               lhCoord.GetUnit() == eStyleUnit_Enumerated,
               "bad line-height unit");
  
  if (lhCoord.GetUnit() == eStyleUnit_Enumerated) {
    NS_ASSERTION(lhCoord.GetIntValue() == NS_STYLE_LINE_HEIGHT_BLOCK_HEIGHT,
                 "bad line-height value");
    if (aBlockHeight != NS_AUTOHEIGHT) {
      return aBlockHeight;
    }
  }

  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForStyleContext(aStyleContext,
                                               getter_AddRefs(fm),
                                               aFontSizeInflation);
  return GetNormalLineHeight(fm);
}

nscoord
nsHTMLReflowState::CalcLineHeight() const
{
  nscoord blockHeight =
    nsLayoutUtils::IsNonWrapperBlock(frame) ? mComputedHeight :
    (mCBReflowState ? mCBReflowState->mComputedHeight : NS_AUTOHEIGHT);

  return CalcLineHeight(frame->GetStyleContext(), blockHeight,
                        nsLayoutUtils::FontSizeInflationFor(frame,
                          nsLayoutUtils::eInReflow));
}

 nscoord
nsHTMLReflowState::CalcLineHeight(nsStyleContext* aStyleContext,
                                  nscoord aBlockHeight,
                                  float aFontSizeInflation)
{
  NS_PRECONDITION(aStyleContext, "Must have a style context");

  nscoord lineHeight =
    ComputeLineHeight(aStyleContext, aBlockHeight, aFontSizeInflation);

  NS_ASSERTION(lineHeight >= 0, "ComputeLineHeight screwed up");

  return lineHeight;
}

bool
nsCSSOffsetState::ComputeMargin(nscoord aContainingBlockWidth)
{
  
  const nsStyleMargin *styleMargin = frame->GetStyleMargin();
  bool isWidthDependent = !styleMargin->GetMargin(mComputedMargin);
  if (isWidthDependent) {
    
    mComputedMargin.left = nsLayoutUtils::
      ComputeWidthDependentValue(aContainingBlockWidth,
                                 styleMargin->mMargin.GetLeft());
    mComputedMargin.right = nsLayoutUtils::
      ComputeWidthDependentValue(aContainingBlockWidth,
                                 styleMargin->mMargin.GetRight());

    
    
    
    
    mComputedMargin.top = nsLayoutUtils::
      ComputeWidthDependentValue(aContainingBlockWidth,
                                 styleMargin->mMargin.GetTop());
    mComputedMargin.bottom = nsLayoutUtils::
      ComputeWidthDependentValue(aContainingBlockWidth,
                                 styleMargin->mMargin.GetBottom());
  }
  return isWidthDependent;
}

bool
nsCSSOffsetState::ComputePadding(nscoord aContainingBlockWidth, nsIAtom* aFrameType)
{
  
  const nsStylePadding *stylePadding = frame->GetStylePadding();
  bool isWidthDependent = !stylePadding->GetPadding(mComputedPadding);
  
  
  if (nsGkAtoms::tableRowGroupFrame == aFrameType ||
      nsGkAtoms::tableColGroupFrame == aFrameType ||
      nsGkAtoms::tableRowFrame      == aFrameType ||
      nsGkAtoms::tableColFrame      == aFrameType) {
    mComputedPadding.SizeTo(0,0,0,0);
  }
  else if (isWidthDependent) {
    
    
    mComputedPadding.left = NS_MAX(0, nsLayoutUtils::
      ComputeWidthDependentValue(aContainingBlockWidth,
                                 stylePadding->mPadding.GetLeft()));
    mComputedPadding.right = NS_MAX(0, nsLayoutUtils::
      ComputeWidthDependentValue(aContainingBlockWidth,
                                 stylePadding->mPadding.GetRight()));

    
    
    mComputedPadding.top = NS_MAX(0, nsLayoutUtils::
      ComputeWidthDependentValue(aContainingBlockWidth,
                                 stylePadding->mPadding.GetTop()));
    mComputedPadding.bottom = NS_MAX(0, nsLayoutUtils::
      ComputeWidthDependentValue(aContainingBlockWidth,
                                 stylePadding->mPadding.GetBottom()));
  }
  return isWidthDependent;
}

void
nsHTMLReflowState::ApplyMinMaxConstraints(nscoord* aFrameWidth,
                                          nscoord* aFrameHeight) const
{
  if (aFrameWidth) {
    if (NS_UNCONSTRAINEDSIZE != mComputedMaxWidth) {
      *aFrameWidth = NS_MIN(*aFrameWidth, mComputedMaxWidth);
    }
    *aFrameWidth = NS_MAX(*aFrameWidth, mComputedMinWidth);
  }

  if (aFrameHeight) {
    if (NS_UNCONSTRAINEDSIZE != mComputedMaxHeight) {
      *aFrameHeight = NS_MIN(*aFrameHeight, mComputedMaxHeight);
    }
    *aFrameHeight = NS_MAX(*aFrameHeight, mComputedMinHeight);
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

  if (eStyleUnit_None == mStylePosition->mMaxWidth.GetUnit()) {
    
    mComputedMaxWidth = NS_UNCONSTRAINEDSIZE;  
  } else {
    mComputedMaxWidth = ComputeWidthValue(aContainingBlockWidth,
                                          mStylePosition->mBoxSizing,
                                          mStylePosition->mMaxWidth);
  }

  
  
  if (mComputedMinWidth > mComputedMaxWidth) {
    mComputedMaxWidth = mComputedMinWidth;
  }

  
  
  
  
  const nsStyleCoord &minHeight = mStylePosition->mMinHeight;
  if ((NS_AUTOHEIGHT == aContainingBlockHeight &&
       minHeight.HasPercent()) ||
      (mFrameType == NS_CSS_FRAME_TYPE_INTERNAL_TABLE &&
       minHeight.IsCalcUnit())) {
    mComputedMinHeight = 0;
  } else {
    mComputedMinHeight = nsLayoutUtils::
      ComputeHeightValue(aContainingBlockHeight, minHeight);
  }
  const nsStyleCoord &maxHeight = mStylePosition->mMaxHeight;
  nsStyleUnit maxHeightUnit = maxHeight.GetUnit();
  if (eStyleUnit_None == maxHeightUnit) {
    
    mComputedMaxHeight = NS_UNCONSTRAINEDSIZE;  
  } else {
    
    
    
    
    if ((NS_AUTOHEIGHT == aContainingBlockHeight && 
         maxHeight.HasPercent()) ||
        (mFrameType == NS_CSS_FRAME_TYPE_INTERNAL_TABLE &&
         maxHeight.IsCalcUnit())) {
      mComputedMaxHeight = NS_UNCONSTRAINEDSIZE;
    } else {
      mComputedMaxHeight = nsLayoutUtils::
        ComputeHeightValue(aContainingBlockHeight, maxHeight);
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
