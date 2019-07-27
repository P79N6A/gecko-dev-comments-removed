






#include "nsHTMLReflowState.h"

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
#include "nsFlexContainerFrame.h"
#include "nsImageFrame.h"
#include "nsTableFrame.h"
#include "nsTableCellFrame.h"
#include "nsIPercentHeightObserver.h"
#include "nsLayoutUtils.h"
#include "mozilla/Preferences.h"
#include "nsFontInflationData.h"
#include "StickyScrollContainer.h"
#include "nsIFrameInlines.h"
#include "CounterStyleManager.h"
#include <algorithm>
#include "mozilla/dom/HTMLInputElement.h"

#ifdef DEBUG
#undef NOISY_VERTICAL_ALIGN
#else
#undef NOISY_VERTICAL_ALIGN
#endif

using namespace mozilla;
using namespace mozilla::css;
using namespace mozilla::dom;
using namespace mozilla::layout;

enum eNormalLineHeightControl {
  eUninitialized = -1,
  eNoExternalLeading = 0,   
  eIncludeExternalLeading,  
  eCompensateLeading        
};

static eNormalLineHeightControl sNormalLineHeightControl = eUninitialized;



nsHTMLReflowState::nsHTMLReflowState(nsPresContext*       aPresContext,
                                     nsIFrame*            aFrame,
                                     nsRenderingContext*  aRenderingContext,
                                     const LogicalSize&   aAvailableSpace,
                                     uint32_t             aFlags)
  : nsCSSOffsetState(aFrame, aRenderingContext)
  , mBlockDelta(0)
  , mOrthogonalLimit(NS_UNCONSTRAINEDSIZE)
  , mReflowDepth(0)
{
  NS_PRECONDITION(aRenderingContext, "no rendering context");
  MOZ_ASSERT(aPresContext, "no pres context");
  MOZ_ASSERT(aFrame, "no frame");
  MOZ_ASSERT(aPresContext == aFrame->PresContext(), "wrong pres context");
  parentReflowState = nullptr;
  AvailableISize() = aAvailableSpace.ISize(mWritingMode);
  AvailableBSize() = aAvailableSpace.BSize(mWritingMode);
  mFloatManager = nullptr;
  mLineLayout = nullptr;
  memset(&mFlags, 0, sizeof(mFlags));
  mDiscoveredClearance = nullptr;
  mPercentHeightObserver = nullptr;

  if (aFlags & DUMMY_PARENT_REFLOW_STATE) {
    mFlags.mDummyParentReflowState = true;
  }

  if (!(aFlags & CALLER_WILL_INIT)) {
    Init(aPresContext);
  }
}

static bool CheckNextInFlowParenthood(nsIFrame* aFrame, nsIFrame* aParent)
{
  nsIFrame* frameNext = aFrame->GetNextInFlow();
  nsIFrame* parentNext = aParent->GetNextInFlow();
  return frameNext && parentNext && frameNext->GetParent() == parentNext;
}












static  nscoord
FontSizeInflationListMarginAdjustment(const nsIFrame* aFrame)
{
  float inflation = nsLayoutUtils::FontSizeInflationFor(aFrame);
  if (aFrame->IsFrameOfType(nsIFrame::eBlockFrame)) {
    const nsBlockFrame* blockFrame = static_cast<const nsBlockFrame*>(aFrame);

    
    
    if (inflation > 1.0f &&
        blockFrame->HasBullet() &&
        inflation > 1.0f) {

      auto listStyleType = aFrame->StyleList()->GetCounterStyle()->GetStyle();
      if (listStyleType != NS_STYLE_LIST_STYLE_NONE &&
          listStyleType != NS_STYLE_LIST_STYLE_DISC &&
          listStyleType != NS_STYLE_LIST_STYLE_CIRCLE &&
          listStyleType != NS_STYLE_LIST_STYLE_SQUARE &&
          listStyleType != NS_STYLE_LIST_STYLE_DISCLOSURE_CLOSED &&
          listStyleType != NS_STYLE_LIST_STYLE_DISCLOSURE_OPEN) {
        
        
        
        
        
        return nsPresContext::CSSPixelsToAppUnits(40) * (inflation - 1);
      }

    }
  }

  return 0;
}





nsCSSOffsetState::nsCSSOffsetState(nsIFrame *aFrame,
                                   nsRenderingContext *aRenderingContext,
                                   nscoord aContainingBlockWidth)
  : frame(aFrame)
  , rendContext(aRenderingContext)
  , mWritingMode(aFrame->GetWritingMode())
{
  MOZ_ASSERT(!aFrame->IsFlexOrGridItem(),
             "We're about to resolve vertical percent margin & padding "
             "values against CB width, which is incorrect for flex/grid items");
  InitOffsets(aContainingBlockWidth, aContainingBlockWidth, frame->GetType());
}




nsHTMLReflowState::nsHTMLReflowState(nsPresContext*           aPresContext,
                                     const nsHTMLReflowState& aParentReflowState,
                                     nsIFrame*                aFrame,
                                     const LogicalSize&       aAvailableSpace,
                                     nscoord                  aContainingBlockWidth,
                                     nscoord                  aContainingBlockHeight,
                                     uint32_t                 aFlags)
  : nsCSSOffsetState(aFrame, aParentReflowState.rendContext)
  , mBlockDelta(0)
  , mOrthogonalLimit(NS_UNCONSTRAINEDSIZE)
  , mReflowDepth(aParentReflowState.mReflowDepth + 1)
  , mFlags(aParentReflowState.mFlags)
{
  MOZ_ASSERT(aPresContext, "no pres context");
  MOZ_ASSERT(aFrame, "no frame");
  MOZ_ASSERT(aPresContext == aFrame->PresContext(), "wrong pres context");
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

  AvailableISize() = aAvailableSpace.ISize(mWritingMode);
  AvailableBSize() = aAvailableSpace.BSize(mWritingMode);

  if (mWritingMode.IsOrthogonalTo(aParentReflowState.GetWritingMode())) {
    
    
    
    if (AvailableISize() == NS_UNCONSTRAINEDSIZE &&
        aParentReflowState.ComputedBSize() != NS_UNCONSTRAINEDSIZE) {
      AvailableISize() = aParentReflowState.ComputedBSize();
    }
  }

  mFloatManager = aParentReflowState.mFloatManager;
  if (frame->IsFrameOfType(nsIFrame::eLineParticipant))
    mLineLayout = aParentReflowState.mLineLayout;
  else
    mLineLayout = nullptr;

  
  
  
  mFlags.mNextInFlowUntouched = aParentReflowState.mFlags.mNextInFlowUntouched &&
    CheckNextInFlowParenthood(aFrame, aParentReflowState.frame);
  mFlags.mAssumingHScrollbar = mFlags.mAssumingVScrollbar = false;
  mFlags.mHasClearance = false;
  mFlags.mIsColumnBalancing = false;
  mFlags.mIsFlexContainerMeasuringHeight = false;
  mFlags.mDummyParentReflowState = false;

  mDiscoveredClearance = nullptr;
  mPercentHeightObserver = (aParentReflowState.mPercentHeightObserver &&
                            aParentReflowState.mPercentHeightObserver->NeedsToObserve(*this))
                           ? aParentReflowState.mPercentHeightObserver : nullptr;

  if ((aFlags & DUMMY_PARENT_REFLOW_STATE) ||
      (parentReflowState->mFlags.mDummyParentReflowState &&
       frame->GetType() == nsGkAtoms::tableFrame)) {
    mFlags.mDummyParentReflowState = true;
  }

  if (!(aFlags & CALLER_WILL_INIT)) {
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
                                    uint8_t aBoxSizing,
                                    const nsStyleCoord& aCoord)
{
  nscoord inside = 0, outside = ComputedPhysicalBorderPadding().LeftRight() +
                                ComputedPhysicalMargin().LeftRight();
  switch (aBoxSizing) {
    case NS_STYLE_BOX_SIZING_BORDER:
      inside = ComputedPhysicalBorderPadding().LeftRight();
      break;
    case NS_STYLE_BOX_SIZING_PADDING:
      inside = ComputedPhysicalPadding().LeftRight();
      break;
  }
  outside -= inside;

  return ComputeWidthValue(aContainingBlockWidth, inside,
                           outside, aCoord);
}

nscoord
nsCSSOffsetState::ComputeHeightValue(nscoord aContainingBlockHeight,
                                     uint8_t aBoxSizing,
                                     const nsStyleCoord& aCoord)
{
  nscoord inside = 0;
  switch (aBoxSizing) {
    case NS_STYLE_BOX_SIZING_BORDER:
      inside = ComputedPhysicalBorderPadding().TopBottom();
      break;
    case NS_STYLE_BOX_SIZING_PADDING:
      inside = ComputedPhysicalPadding().TopBottom();
      break;
  }
  return nsLayoutUtils::ComputeHeightValue(aContainingBlockHeight, 
                                           inside, aCoord);
}

void
nsHTMLReflowState::SetComputedWidth(nscoord aComputedWidth)
{
  NS_ASSERTION(frame, "Must have a frame!");
  
  
  
  
  
  
  
  
  
  
  
  

  NS_PRECONDITION(aComputedWidth >= 0, "Invalid computed width");
  if (ComputedWidth() != aComputedWidth) {
    ComputedWidth() = aComputedWidth;
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
  if (ComputedHeight() != aComputedHeight) {
    ComputedHeight() = aComputedHeight;
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
  if (AvailableISize() == NS_UNCONSTRAINEDSIZE) {
    
    
    for (const nsHTMLReflowState *parent = parentReflowState;
         parent != nullptr; parent = parent->parentReflowState) {
      if (parent->GetWritingMode().IsOrthogonalTo(mWritingMode) &&
          parent->mOrthogonalLimit != NS_UNCONSTRAINEDSIZE) {
        AvailableISize() = parent->mOrthogonalLimit;
        break;
      }
    }
  }

  NS_WARN_IF_FALSE(AvailableISize() != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained inline-size; this should only result from "
                   "very large sizes, not attempts at intrinsic inline-size "
                   "calculation");

  mStylePosition = frame->StylePosition();
  mStyleDisplay = frame->StyleDisplay();
  mStyleVisibility = frame->StyleVisibility();
  mStyleBorder = frame->StyleBorder();
  mStyleMargin = frame->StyleMargin();
  mStylePadding = frame->StylePadding();
  mStyleText = frame->StyleText();

  nsIAtom* type = frame->GetType();

  InitFrameType(type);
  InitCBReflowState();

  InitConstraints(aPresContext, aContainingBlockWidth, aContainingBlockHeight,
                  aBorder, aPadding, type);

  InitResizeFlags(aPresContext, type);

  nsIFrame *parent = frame->GetParent();
  if (parent &&
      (parent->GetStateBits() & NS_FRAME_IN_CONSTRAINED_BSIZE) &&
      !(parent->GetType() == nsGkAtoms::scrollFrame &&
        parent->StyleDisplay()->mOverflowY != NS_STYLE_OVERFLOW_HIDDEN)) {
    frame->AddStateBits(NS_FRAME_IN_CONSTRAINED_BSIZE);
  } else if (type == nsGkAtoms::svgForeignObjectFrame) {
    
    frame->AddStateBits(NS_FRAME_IN_CONSTRAINED_BSIZE);
  } else {
    const bool vertical = mWritingMode.IsVertical();
    const nsStyleCoord& bSizeCoord =
      vertical ? mStylePosition->mWidth : mStylePosition->mHeight;
    const nsStyleCoord& maxBSizeCoord =
      vertical ? mStylePosition->mMaxWidth : mStylePosition->mMaxHeight;
    if ((bSizeCoord.GetUnit() != eStyleUnit_Auto ||
         maxBSizeCoord.GetUnit() != eStyleUnit_None) &&
         
         (frame->GetContent() &&
        !(frame->GetContent()->IsAnyOfHTMLElements(nsGkAtoms::body,
                                                   nsGkAtoms::html)))) {

      
      
      
      nsIFrame* containingBlk = frame;
      while (containingBlk) {
        const nsStylePosition* stylePos = containingBlk->StylePosition();
        const nsStyleCoord& bSizeCoord =
          vertical ? stylePos->mWidth : stylePos->mHeight;
        const nsStyleCoord& maxBSizeCoord =
          vertical ? stylePos->mMaxWidth : stylePos->mMaxHeight;
        if ((bSizeCoord.IsCoordPercentCalcUnit() &&
             !bSizeCoord.HasPercent()) ||
            (maxBSizeCoord.IsCoordPercentCalcUnit() &&
             !maxBSizeCoord.HasPercent())) {
          frame->AddStateBits(NS_FRAME_IN_CONSTRAINED_BSIZE);
          break;
        } else if ((bSizeCoord.IsCoordPercentCalcUnit() &&
                    bSizeCoord.HasPercent()) ||
                   (maxBSizeCoord.IsCoordPercentCalcUnit() &&
                    maxBSizeCoord.HasPercent())) {
          if (!(containingBlk = containingBlk->GetContainingBlock())) {
            
            
            frame->RemoveStateBits(NS_FRAME_IN_CONSTRAINED_BSIZE);
            break;
          }

          continue;
        } else {
          frame->RemoveStateBits(NS_FRAME_IN_CONSTRAINED_BSIZE);
          break;
        }
      }
    } else {
      frame->RemoveStateBits(NS_FRAME_IN_CONSTRAINED_BSIZE);
    }
  }

  NS_WARN_IF_FALSE((mFrameType == NS_CSS_FRAME_TYPE_INLINE &&
                    !frame->IsFrameOfType(nsIFrame::eReplaced)) ||
                   type == nsGkAtoms::textFrame ||
                   ComputedISize() != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained inline-size; this should only result from "
                   "very large sizes, not attempts at intrinsic inline-size "
                   "calculation");
}

void nsHTMLReflowState::InitCBReflowState()
{
  if (!parentReflowState) {
    mCBReflowState = nullptr;
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
      if (!rs->frame->IsAbsolutelyPositioned()) {
        return false;
      }
    }
  }
  return true;
}


void
nsHTMLReflowState::InitResizeFlags(nsPresContext* aPresContext, nsIAtom* aFrameType)
{
  bool isHResize = (frame->GetSize().width !=
                     ComputedWidth() + ComputedPhysicalBorderPadding().LeftRight()) ||
                     aPresContext->PresShell()->IsReflowOnZoomPending();

  if ((frame->GetStateBits() & NS_FRAME_FONT_INFLATION_FLOW_ROOT) &&
      nsLayoutUtils::FontSizeInflationEnabled(aPresContext)) {
    
    
    bool dirty = nsFontInflationData::UpdateFontInflationDataISizeFor(*this) &&
                 
                 
                 
                 
                 !mFlags.mDummyParentReflowState;

    if (dirty || (!frame->GetParent() && isHResize)) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      

      
      
      
      
      
      if (frame->GetType() == nsGkAtoms::svgForeignObjectFrame) {
        
        frame->AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
        nsIFrame *kid = frame->GetFirstPrincipalChild();
        if (kid) {
          kid->AddStateBits(NS_FRAME_IS_DIRTY);
        }
      } else {
        frame->AddStateBits(NS_FRAME_IS_DIRTY);
      }

      
      
      
      
      

      
      
      
      
      
      
      

      
      
      
      
      

      nsAutoTArray<nsIFrame*, 32> stack;
      stack.AppendElement(frame);

      do {
        nsIFrame *f = stack.ElementAt(stack.Length() - 1);
        stack.RemoveElementAt(stack.Length() - 1);

        nsIFrame::ChildListIterator lists(f);
        for (; !lists.IsDone(); lists.Next()) {
          nsFrameList::Enumerator childFrames(lists.CurrentList());
          for (; !childFrames.AtEnd(); childFrames.Next()) {
            nsIFrame* kid = childFrames.get();
            kid->MarkIntrinsicISizesDirty();
            stack.AppendElement(kid);
          }
        }
      } while (stack.Length() != 0);
    }
  }

  SetHResize(!(frame->GetStateBits() & NS_FRAME_IS_DIRTY) &&
             isHResize);

  
  
  if (IS_TABLE_CELL(aFrameType) &&
      (mFlags.mSpecialHeightReflow ||
       (frame->FirstInFlow()->GetStateBits() &
         NS_TABLE_CELL_HAD_SPECIAL_REFLOW)) &&
      (frame->GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT)) {
    
    
    
    SetVResize(true);
  } else if (mCBReflowState && !nsLayoutUtils::IsNonWrapperBlock(frame)) {
    
    
    
    
    SetVResize(mCBReflowState->IsVResize());
  } else if (ComputedHeight() == NS_AUTOHEIGHT) {
    if (eCompatibility_NavQuirks == aPresContext->CompatibilityMode() &&
        mCBReflowState) {
      SetVResize(mCBReflowState->IsVResize());
    } else {
      SetVResize(IsHResize());
    }
    SetVResize(IsVResize() || NS_SUBTREE_DIRTY(frame));
  } else {
    
    SetVResize(frame->GetSize().height !=
               ComputedHeight() + ComputedPhysicalBorderPadding().TopBottom());
  }

  bool dependsOnCBHeight =
    (mStylePosition->HeightDependsOnContainer() &&
     
     mStylePosition->mHeight.GetUnit() != eStyleUnit_Auto) ||
    mStylePosition->MinHeightDependsOnContainer() ||
    mStylePosition->MaxHeightDependsOnContainer() ||
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

  
  
  
  
  
  
  if (!IsVResize() && mCBReflowState &&
      (IS_TABLE_CELL(mCBReflowState->frame->GetType()) || 
       mCBReflowState->mFlags.mHeightDependsOnAncestorCell) &&
      !mCBReflowState->mFlags.mSpecialHeightReflow && 
      dependsOnCBHeight) {
    SetVResize(true);
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
  return rs->ComputedWidth();
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

  NS_ASSERTION(frame->StyleDisplay()->IsAbsolutelyPositionedStyle() ==
                 disp->IsAbsolutelyPositionedStyle(),
               "Unexpected position style");
  NS_ASSERTION(frame->StyleDisplay()->IsFloatingStyle() ==
                 disp->IsFloatingStyle(), "Unexpected float style");
  if (frame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
    if (disp->IsAbsolutelyPositioned(frame)) {
      frameType = NS_CSS_FRAME_TYPE_ABSOLUTE;
      
      
      if (frame->GetPrevInFlow())
        frameType = NS_CSS_FRAME_TYPE_BLOCK;
    }
    else if (disp->IsFloating(frame)) {
      frameType = NS_CSS_FRAME_TYPE_FLOATING;
    } else {
      NS_ASSERTION(disp->mDisplay == NS_STYLE_DISPLAY_POPUP,
                   "unknown out of flow frame type");
      frameType = NS_CSS_FRAME_TYPE_UNKNOWN;
    }
  }
  else {
    switch (GetDisplay()) {
    case NS_STYLE_DISPLAY_BLOCK:
    case NS_STYLE_DISPLAY_LIST_ITEM:
    case NS_STYLE_DISPLAY_TABLE:
    case NS_STYLE_DISPLAY_TABLE_CAPTION:
    case NS_STYLE_DISPLAY_FLEX:
    case NS_STYLE_DISPLAY_GRID:
    case NS_STYLE_DISPLAY_RUBY_TEXT_CONTAINER:
      frameType = NS_CSS_FRAME_TYPE_BLOCK;
      break;

    case NS_STYLE_DISPLAY_INLINE:
    case NS_STYLE_DISPLAY_INLINE_BLOCK:
    case NS_STYLE_DISPLAY_INLINE_TABLE:
    case NS_STYLE_DISPLAY_INLINE_BOX:
    case NS_STYLE_DISPLAY_INLINE_XUL_GRID:
    case NS_STYLE_DISPLAY_INLINE_STACK:
    case NS_STYLE_DISPLAY_INLINE_FLEX:
    case NS_STYLE_DISPLAY_INLINE_GRID:
    case NS_STYLE_DISPLAY_RUBY:
    case NS_STYLE_DISPLAY_RUBY_BASE:
    case NS_STYLE_DISPLAY_RUBY_TEXT:
    case NS_STYLE_DISPLAY_RUBY_BASE_CONTAINER:
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
nsHTMLReflowState::ComputeRelativeOffsets(uint8_t aCBDirection,
                                          nsIFrame* aFrame,
                                          nscoord aContainingBlockWidth,
                                          nscoord aContainingBlockHeight,
                                          nsMargin& aComputedOffsets)
{
  const nsStylePosition* position = aFrame->StylePosition();

  
  
  
  bool    leftIsAuto = eStyleUnit_Auto == position->mOffset.GetLeftUnit();
  bool    rightIsAuto = eStyleUnit_Auto == position->mOffset.GetRightUnit();

  
  
  if (!leftIsAuto && !rightIsAuto) {
    if (aCBDirection == NS_STYLE_DIRECTION_RTL) {
      leftIsAuto = true;
    } else {
      rightIsAuto = true;
    }
  }

  if (leftIsAuto) {
    if (rightIsAuto) {
      
      aComputedOffsets.left = aComputedOffsets.right = 0;
    } else {
      
      aComputedOffsets.right = nsLayoutUtils::
        ComputeCBDependentValue(aContainingBlockWidth,
                                position->mOffset.GetRight());

      
      aComputedOffsets.left = -aComputedOffsets.right;
    }

  } else {
    NS_ASSERTION(rightIsAuto, "unexpected specified constraint");
    
    
    aComputedOffsets.left = nsLayoutUtils::
      ComputeCBDependentValue(aContainingBlockWidth,
                              position->mOffset.GetLeft());

    
    aComputedOffsets.right = -aComputedOffsets.left;
  }

  
  
  
  bool    topIsAuto = eStyleUnit_Auto == position->mOffset.GetTopUnit();
  bool    bottomIsAuto = eStyleUnit_Auto == position->mOffset.GetBottomUnit();

  
  
  if (NS_AUTOHEIGHT == aContainingBlockHeight) {
    if (position->OffsetHasPercent(NS_SIDE_TOP)) {
      topIsAuto = true;
    }
    if (position->OffsetHasPercent(NS_SIDE_BOTTOM)) {
      bottomIsAuto = true;
    }
  }

  
  if (!topIsAuto && !bottomIsAuto) {
    bottomIsAuto = true;
  }

  if (topIsAuto) {
    if (bottomIsAuto) {
      
      aComputedOffsets.top = aComputedOffsets.bottom = 0;
    } else {
      
      aComputedOffsets.bottom = nsLayoutUtils::
        ComputeHeightDependentValue(aContainingBlockHeight,
                                    position->mOffset.GetBottom());
      
      
      aComputedOffsets.top = -aComputedOffsets.bottom;
    }

  } else {
    NS_ASSERTION(bottomIsAuto, "unexpected specified constraint");
    
    
    aComputedOffsets.top = nsLayoutUtils::
      ComputeHeightDependentValue(aContainingBlockHeight,
                                  position->mOffset.GetTop());

    
    aComputedOffsets.bottom = -aComputedOffsets.top;
  }

  
  FrameProperties props = aFrame->Properties();
  nsMargin* offsets = static_cast<nsMargin*>
    (props.Get(nsIFrame::ComputedOffsetProperty()));
  if (offsets) {
    *offsets = aComputedOffsets;
  } else {
    props.Set(nsIFrame::ComputedOffsetProperty(),
              new nsMargin(aComputedOffsets));
  }
}

 void
nsHTMLReflowState::ApplyRelativePositioning(nsIFrame* aFrame,
                                            const nsMargin& aComputedOffsets,
                                            nsPoint* aPosition)
{
  if (!aFrame->IsRelativelyPositioned()) {
    NS_ASSERTION(!aFrame->Properties().Get(nsIFrame::NormalPositionProperty()),
                 "We assume that changing the 'position' property causes "
                 "frame reconstruction.  If that ever changes, this code "
                 "should call "
                 "props.Delete(nsIFrame::NormalPositionProperty())");
    return;
  }

  
  FrameProperties props = aFrame->Properties();
  nsPoint* normalPosition = static_cast<nsPoint*>
    (props.Get(nsIFrame::NormalPositionProperty()));
  if (normalPosition) {
    *normalPosition = *aPosition;
  } else {
    props.Set(nsIFrame::NormalPositionProperty(), new nsPoint(*aPosition));
  }

  const nsStyleDisplay* display = aFrame->StyleDisplay();
  if (NS_STYLE_POSITION_RELATIVE == display->mPosition) {
    *aPosition += nsPoint(aComputedOffsets.left, aComputedOffsets.top);
  } else if (NS_STYLE_POSITION_STICKY == display->mPosition &&
             !aFrame->GetNextContinuation() &&
             !aFrame->GetPrevContinuation() &&
             !(aFrame->GetStateBits() & NS_FRAME_PART_OF_IBSPLIT)) {
    
    
    
    
    
    
    StickyScrollContainer* ssc =
      StickyScrollContainer::GetStickyScrollContainerForFrame(aFrame);
    if (ssc) {
      *aPosition = ssc->ComputePosition(aFrame);
    }
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
    state = nullptr;
  }
  
  if (state) {
    aCBLeftEdge = state->ComputedPhysicalBorderPadding().left;
    aCBWidth = state->ComputedWidth();
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
  const nsMargin& border = mStyleBorder->GetComputedBorder();
  nsMargin padding, margin;

  
  if (!mStylePadding->GetPadding(padding)) {
    
    padding.left = nsLayoutUtils::
      ComputeCBDependentValue(aContainingBlockWidth,
                              mStylePadding->mPadding.GetLeft());
    padding.right = nsLayoutUtils::
      ComputeCBDependentValue(aContainingBlockWidth,
                              mStylePadding->mPadding.GetRight());
  }

  
  if (!mStyleMargin->GetMargin(margin)) {
    
    if (eStyleUnit_Auto == mStyleMargin->mMargin.GetLeftUnit()) {
      
      margin.left = 0;  
    } else {
      margin.left = nsLayoutUtils::
        ComputeCBDependentValue(aContainingBlockWidth,
                                mStyleMargin->mMargin.GetLeft());
    }
    if (eStyleUnit_Auto == mStyleMargin->mMargin.GetRightUnit()) {
      
      margin.right = 0;  
    } else {
      margin.right = nsLayoutUtils::
        ComputeCBDependentValue(aContainingBlockWidth,
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
  
  
  const nsStyleVisibility* blockVis = aContainingBlock->StyleVisibility();

  
  
  
  
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

      
      
      if (mStyleDisplay->IsOriginalDisplayInlineOutsideStyle()) {
        
        
        aHypotheticalBox.mTop = lineBox->GetPhysicalBounds().y + blockYOffset;
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
            
            
            
            aHypotheticalBox.mTop = lineBox->GetPhysicalBounds().y + blockYOffset;
          } else {
            
            
            aHypotheticalBox.mTop = lineBox->GetPhysicalBounds().YMost() + blockYOffset;
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
    
    
    if (mStyleDisplay->IsOriginalDisplayInlineOutsideStyle()) {
      
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
    
    if (mStyleDisplay->IsOriginalDisplayInlineOutsideStyle()) {
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
      cbOffset += aContainingBlock->GetPositionIgnoringScrolling();
      nsContainerFrame* parent = aContainingBlock->GetParent();
      if (!parent) {
        
        
        
        
        
        
        cbOffset -= aContainingBlock->GetOffsetTo(cbrs->frame);
        break;
      }
      aContainingBlock = parent;
    } while (aContainingBlock != cbrs->frame);
  } else {
    
    
    
    
    cbOffset = aContainingBlock->GetOffsetTo(cbrs->frame);
  }
  aHypotheticalBox.mLeft += cbOffset.x;
  aHypotheticalBox.mTop += cbOffset.y;
  aHypotheticalBox.mRight += cbOffset.x;
  
  
  
  
  nsMargin border = cbrs->ComputedPhysicalBorderPadding() - cbrs->ComputedPhysicalPadding();
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
  NS_ASSERTION(nullptr != placeholderFrame, "no placeholder frame");

  
  
  
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
    ComputedPhysicalOffsets().left = 0;
    leftIsAuto = true;
  } else {
    ComputedPhysicalOffsets().left = nsLayoutUtils::
      ComputeCBDependentValue(containingBlockWidth,
                              mStylePosition->mOffset.GetLeft());
  }
  if (eStyleUnit_Auto == mStylePosition->mOffset.GetRightUnit()) {
    ComputedPhysicalOffsets().right = 0;
    rightIsAuto = true;
  } else {
    ComputedPhysicalOffsets().right = nsLayoutUtils::
      ComputeCBDependentValue(containingBlockWidth,
                              mStylePosition->mOffset.GetRight());
  }

  
  
  if (leftIsAuto && rightIsAuto) {
    
    
    if (NS_STYLE_DIRECTION_LTR == placeholderFrame->GetContainingBlock()
                                    ->StyleVisibility()->mDirection) {
      NS_ASSERTION(hypotheticalBox.mLeftIsExact, "should always have "
                   "exact value on containing block's start side");
      ComputedPhysicalOffsets().left = hypotheticalBox.mLeft;
      leftIsAuto = false;
    } else {
      NS_ASSERTION(hypotheticalBox.mRightIsExact, "should always have "
                   "exact value on containing block's start side");
      ComputedPhysicalOffsets().right = containingBlockWidth - hypotheticalBox.mRight;
      rightIsAuto = false;
    }
  }

  
  bool        topIsAuto = false, bottomIsAuto = false;
  if (eStyleUnit_Auto == mStylePosition->mOffset.GetTopUnit()) {
    ComputedPhysicalOffsets().top = 0;
    topIsAuto = true;
  } else {
    ComputedPhysicalOffsets().top = nsLayoutUtils::
      ComputeHeightDependentValue(containingBlockHeight,
                                  mStylePosition->mOffset.GetTop());
  }
  if (eStyleUnit_Auto == mStylePosition->mOffset.GetBottomUnit()) {
    ComputedPhysicalOffsets().bottom = 0;        
    bottomIsAuto = true;
  } else {
    ComputedPhysicalOffsets().bottom = nsLayoutUtils::
      ComputeHeightDependentValue(containingBlockHeight,
                                  mStylePosition->mOffset.GetBottom());
  }

  if (topIsAuto && bottomIsAuto) {
    
    ComputedPhysicalOffsets().top = hypotheticalBox.mTop;
    topIsAuto = false;
  }

  bool widthIsAuto = eStyleUnit_Auto == mStylePosition->mWidth.GetUnit();
  bool heightIsAuto = eStyleUnit_Auto == mStylePosition->mHeight.GetUnit();

  typedef nsIFrame::ComputeSizeFlags ComputeSizeFlags;
  ComputeSizeFlags computeSizeFlags = ComputeSizeFlags::eDefault;
  if (leftIsAuto || rightIsAuto) {
    computeSizeFlags =
      ComputeSizeFlags(computeSizeFlags | ComputeSizeFlags::eShrinkWrap);
  }

  {
    AutoMaybeDisableFontInflation an(frame);

    WritingMode wm = GetWritingMode();
    
    LogicalSize cbSize(wm, nsSize(containingBlockWidth, containingBlockHeight));
    LogicalSize size =
      frame->ComputeSize(rendContext, wm, cbSize,
                         cbSize.ISize(wm), 
                         ComputedLogicalMargin().Size(wm) +
                           ComputedLogicalOffsets().Size(wm),
                         ComputedLogicalBorderPadding().Size(wm) -
                           ComputedLogicalPadding().Size(wm),
                         ComputedLogicalPadding().Size(wm),
                         computeSizeFlags);
    ComputedISize() = size.ISize(wm);
    ComputedBSize() = size.BSize(wm);
  }
  NS_ASSERTION(ComputedISize() >= 0, "Bogus inline-size");
  NS_ASSERTION(ComputedBSize() == NS_UNCONSTRAINEDSIZE ||
               ComputedBSize() >= 0, "Bogus block-size");

  
  

  if (leftIsAuto) {
    
    
    
    if (widthIsAuto) {
      
      
      
      ComputedPhysicalOffsets().left = NS_AUTOOFFSET;
    } else {
      ComputedPhysicalOffsets().left = containingBlockWidth - ComputedPhysicalMargin().left -
        ComputedPhysicalBorderPadding().left - ComputedWidth() - ComputedPhysicalBorderPadding().right - 
        ComputedPhysicalMargin().right - ComputedPhysicalOffsets().right;

    }
  } else if (rightIsAuto) {
    
    
    
    if (widthIsAuto) {
      
      
      
      ComputedPhysicalOffsets().right = NS_AUTOOFFSET;
    } else {
      ComputedPhysicalOffsets().right = containingBlockWidth - ComputedPhysicalOffsets().left -
        ComputedPhysicalMargin().left - ComputedPhysicalBorderPadding().left - ComputedWidth() -
        ComputedPhysicalBorderPadding().right - ComputedPhysicalMargin().right;
    }
  } else {
    
    
    
    
    
    

    nscoord availMarginSpace = containingBlockWidth -
                               ComputedPhysicalOffsets().LeftRight() -
                               ComputedPhysicalMargin().LeftRight() -
                               ComputedPhysicalBorderPadding().LeftRight() -
                               ComputedWidth();
    bool marginLeftIsAuto =
      eStyleUnit_Auto == mStyleMargin->mMargin.GetLeftUnit();
    bool marginRightIsAuto =
      eStyleUnit_Auto == mStyleMargin->mMargin.GetRightUnit();

    if (marginLeftIsAuto) {
      if (marginRightIsAuto) {
        if (availMarginSpace < 0) {
          
          
          if (cbrs &&
              NS_STYLE_DIRECTION_RTL == cbrs->mStyleVisibility->mDirection) {
            
            ComputedPhysicalMargin().left = availMarginSpace;
          } else {
            
            ComputedPhysicalMargin().right = availMarginSpace;
          }
        } else {
          
          
          ComputedPhysicalMargin().left = availMarginSpace / 2;
          ComputedPhysicalMargin().right = availMarginSpace - ComputedPhysicalMargin().left;
        }
      } else {
        
        ComputedPhysicalMargin().left = availMarginSpace;
      }
    } else {
      if (marginRightIsAuto) {
        
        ComputedPhysicalMargin().right = availMarginSpace;
      } else {
        
        
        
        
        
        
        
        if (cbrs &&
            NS_STYLE_DIRECTION_RTL == cbrs->mStyleVisibility->mDirection) {
          
          ComputedPhysicalOffsets().left += availMarginSpace;
        } else {
          
          ComputedPhysicalOffsets().right += availMarginSpace;
        }
      }
    }
  }

  if (topIsAuto) {
    
    if (heightIsAuto) {
      ComputedPhysicalOffsets().top = NS_AUTOOFFSET;
    } else {
      ComputedPhysicalOffsets().top = containingBlockHeight - ComputedPhysicalMargin().top -
        ComputedPhysicalBorderPadding().top - ComputedHeight() - ComputedPhysicalBorderPadding().bottom - 
        ComputedPhysicalMargin().bottom - ComputedPhysicalOffsets().bottom;
    }
  } else if (bottomIsAuto) {
    
    if (heightIsAuto) {
      ComputedPhysicalOffsets().bottom = NS_AUTOOFFSET;
    } else {
      ComputedPhysicalOffsets().bottom = containingBlockHeight - ComputedPhysicalOffsets().top -
        ComputedPhysicalMargin().top - ComputedPhysicalBorderPadding().top - ComputedHeight() -
        ComputedPhysicalBorderPadding().bottom - ComputedPhysicalMargin().bottom;
    }
  } else {
    
    nscoord autoHeight = containingBlockHeight -
                         ComputedPhysicalOffsets().TopBottom() -
                         ComputedPhysicalMargin().TopBottom() -
                         ComputedPhysicalBorderPadding().TopBottom();
    if (autoHeight < 0) {
      autoHeight = 0;
    }

    if (ComputedHeight() == NS_UNCONSTRAINEDSIZE) {
      
      
      ComputedHeight() = autoHeight;

      
      if (ComputedHeight() > ComputedMaxHeight())
        ComputedHeight() = ComputedMaxHeight();
      if (ComputedHeight() < ComputedMinHeight())
        ComputedHeight() = ComputedMinHeight();
    }

    
    
    
    
    nscoord availMarginSpace = autoHeight - ComputedHeight();
    bool marginTopIsAuto =
      eStyleUnit_Auto == mStyleMargin->mMargin.GetTopUnit();
    bool marginBottomIsAuto =
      eStyleUnit_Auto == mStyleMargin->mMargin.GetBottomUnit();

    if (marginTopIsAuto) {
      if (marginBottomIsAuto) {
        if (availMarginSpace < 0) {
          
          ComputedPhysicalMargin().bottom = availMarginSpace;
        } else {
          
          
          ComputedPhysicalMargin().top = availMarginSpace / 2;
          ComputedPhysicalMargin().bottom = availMarginSpace - ComputedPhysicalMargin().top;
        }
      } else {
        
        ComputedPhysicalMargin().top = availMarginSpace;
      }
    } else {
      if (marginBottomIsAuto) {
        
        ComputedPhysicalMargin().bottom = availMarginSpace;
      } else {
        
        
        
        ComputedPhysicalOffsets().bottom += availMarginSpace;
      }
    }
  }
}

nscoord 
GetVerticalMarginBorderPadding(const nsHTMLReflowState* aReflowState)
{
  nscoord result = 0;
  if (!aReflowState) return result;

  
  nsMargin margin = aReflowState->ComputedPhysicalMargin();
  if (NS_AUTOMARGIN == margin.top) 
    margin.top = 0;
  if (NS_AUTOMARGIN == margin.bottom) 
    margin.bottom = 0;

  result += margin.top + margin.bottom;
  result += aReflowState->ComputedPhysicalBorderPadding().top + 
            aReflowState->ComputedPhysicalBorderPadding().bottom;

  return result;
}











static nscoord
CalcQuirkContainingBlockHeight(const nsHTMLReflowState* aCBReflowState)
{
  const nsHTMLReflowState* firstAncestorRS = nullptr; 
  const nsHTMLReflowState* secondAncestorRS = nullptr; 
  
  
  
  
  nscoord result = NS_AUTOHEIGHT; 
                             
  const nsHTMLReflowState* rs = aCBReflowState;
  for (; rs; rs = rs->parentReflowState) {
    nsIAtom* frameType = rs->frame->GetType();
    
    
    if (nsGkAtoms::blockFrame == frameType ||
#ifdef MOZ_XUL
        nsGkAtoms::XULLabelFrame == frameType ||
#endif
        nsGkAtoms::scrollFrame == frameType) {

      secondAncestorRS = firstAncestorRS;
      firstAncestorRS = rs;

      
      
      
      
      if (NS_AUTOHEIGHT == rs->ComputedHeight()) {
        if (rs->frame->IsAbsolutelyPositioned()) {
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
             ? rs->AvailableHeight() : rs->ComputedHeight();
    
    if (NS_AUTOHEIGHT == result) return result;

    
    
    if ((nsGkAtoms::canvasFrame == frameType) || 
        (nsGkAtoms::pageContentFrame == frameType)) {

      result -= GetVerticalMarginBorderPadding(firstAncestorRS); 
      result -= GetVerticalMarginBorderPadding(secondAncestorRS); 

#ifdef DEBUG
      
      if (firstAncestorRS) {
        nsIContent* frameContent = firstAncestorRS->frame->GetContent();
        if (frameContent) {
          NS_ASSERTION(frameContent->IsHTMLElement(nsGkAtoms::html), "First ancestor is not HTML");
        }
      }
      if (secondAncestorRS) {
        nsIContent* frameContent = secondAncestorRS->frame->GetContent();
        if (frameContent) {
          NS_ASSERTION(frameContent->IsHTMLElement(nsGkAtoms::body), "Second ancestor is not BODY");
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

  
  return std::max(result, 0);
}



void
nsHTMLReflowState::ComputeContainingBlockRectangle(nsPresContext*          aPresContext,
                                                   const nsHTMLReflowState* aContainingBlockRS,
                                                   nscoord&                 aContainingBlockWidth,
                                                   nscoord&                 aContainingBlockHeight)
{
  
  
  aContainingBlockWidth = aContainingBlockRS->ComputedWidth();
  aContainingBlockHeight = aContainingBlockRS->ComputedHeight();

  
  
  if (NS_FRAME_GET_TYPE(mFrameType) == NS_CSS_FRAME_TYPE_ABSOLUTE ||
      (frame->GetType() == nsGkAtoms::tableFrame &&
       frame->IsAbsolutelyPositioned() &&
       (frame->GetParent()->GetStateBits() & NS_FRAME_OUT_OF_FLOW))) {
    
    if (NS_FRAME_GET_TYPE(aContainingBlockRS->mFrameType) == NS_CSS_FRAME_TYPE_INLINE) {
      
      
      
      
      
      
      nsMargin computedBorder = aContainingBlockRS->ComputedPhysicalBorderPadding() -
        aContainingBlockRS->ComputedPhysicalPadding();
      aContainingBlockWidth = aContainingBlockRS->frame->GetRect().width -
        computedBorder.LeftRight();
      NS_ASSERTION(aContainingBlockWidth >= 0,
                   "Negative containing block width!");
      aContainingBlockHeight = aContainingBlockRS->frame->GetRect().height -
        computedBorder.TopBottom();
      NS_ASSERTION(aContainingBlockHeight >= 0,
                   "Negative containing block height!");
    } else {
      
      
      aContainingBlockWidth += aContainingBlockRS->ComputedPhysicalPadding().LeftRight();
      aContainingBlockHeight += aContainingBlockRS->ComputedPhysicalPadding().TopBottom();
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

static eNormalLineHeightControl GetNormalLineHeightCalcControl(void)
{
  if (sNormalLineHeightControl == eUninitialized) {
    
    
    int32_t val =
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
  uint8_t captionSide = aFrame->StyleTableBorder()->mCaptionSide;
  return captionSide == NS_STYLE_CAPTION_SIDE_LEFT ||
         captionSide == NS_STYLE_CAPTION_SIDE_RIGHT;
}

static nsFlexContainerFrame*
GetFlexContainer(nsIFrame* aFrame)
{
  nsIFrame* parent = aFrame->GetParent();
  if (!parent ||
      parent->GetType() != nsGkAtoms::flexContainerFrame) {
    return nullptr;
  }

  return static_cast<nsFlexContainerFrame*>(parent);
}






static nscoord
BlockDirOffsetPercentBasis(const nsIFrame* aFrame,
                           nscoord aContainingBlockISize,
                           nscoord aContainingBlockBSize)
{
  if (!aFrame->IsFlexOrGridItem()) {
    return aContainingBlockISize;
  }

  if (aContainingBlockBSize == NS_AUTOHEIGHT) {
    return 0;
  }

  return aContainingBlockBSize;
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

  WritingMode wm = GetWritingMode();

  
  
  if (nullptr == parentReflowState || mFlags.mDummyParentReflowState) {
    
    LogicalSize cbSize(wm, nsSize(aContainingBlockWidth,
                                  aContainingBlockHeight));
    InitOffsets(cbSize.ISize(wm),
                BlockDirOffsetPercentBasis(frame, cbSize.ISize(wm),
                                           cbSize.BSize(wm)),
                aFrameType, aBorder, aPadding);
    
    
    ComputedPhysicalMargin().SizeTo(0, 0, 0, 0);
    ComputedPhysicalOffsets().SizeTo(0, 0, 0, 0);

    ComputedWidth() = AvailableWidth() - ComputedPhysicalBorderPadding().LeftRight();
    if (ComputedWidth() < 0)
      ComputedWidth() = 0;
    if (AvailableHeight() != NS_UNCONSTRAINEDSIZE) {
      ComputedHeight() = AvailableHeight() - ComputedPhysicalBorderPadding().TopBottom();
      if (ComputedHeight() < 0)
        ComputedHeight() = 0;
    } else {
      ComputedHeight() = NS_UNCONSTRAINEDSIZE;
    }

    ComputedMinWidth() = ComputedMinHeight() = 0;
    ComputedMaxWidth() = ComputedMaxHeight() = NS_UNCONSTRAINEDSIZE;
  } else {
    
    const nsHTMLReflowState* cbrs = mCBReflowState;
    NS_ASSERTION(nullptr != cbrs, "no containing block");

    
    
    if (aContainingBlockWidth == -1) {
      ComputeContainingBlockRectangle(aPresContext, cbrs, aContainingBlockWidth, 
                                      aContainingBlockHeight);
    }

    
    
    nsIAtom* fType;
    if (NS_AUTOHEIGHT == aContainingBlockHeight) {
      
      
      
      if (cbrs->parentReflowState) {
        fType = cbrs->frame->GetType();
        if (IS_TABLE_CELL(fType)) {
          
          aContainingBlockHeight = cbrs->ComputedHeight();
        }
      }
    }

    
    
    WritingMode cbwm = mCBReflowState->GetWritingMode();
    LogicalSize cbSize(cbwm, nsSize(aContainingBlockWidth,
                                    aContainingBlockHeight));
    InitOffsets(cbSize.ISize(cbwm),
                BlockDirOffsetPercentBasis(frame, cbSize.ISize(cbwm),
                                           cbSize.BSize(cbwm)),
                aFrameType, aBorder, aPadding);

    const nsStyleCoord &height = mStylePosition->mHeight;
    nsStyleUnit heightUnit = height.GetUnit();

    
    
    
    
    if (height.HasPercent()) {
      if (NS_AUTOHEIGHT == aContainingBlockHeight) {
        
        
        
        if (NS_FRAME_REPLACED(NS_CSS_FRAME_TYPE_INLINE) == mFrameType ||
            NS_FRAME_REPLACED_CONTAINS_BLOCK(
                NS_CSS_FRAME_TYPE_INLINE) == mFrameType) {
          
          NS_ASSERTION(nullptr != cbrs, "no containing block");
          
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
            if (NS_AUTOHEIGHT != cbrs->ComputedHeight())
              aContainingBlockHeight = cbrs->ComputedHeight();
            else
              heightUnit = eStyleUnit_Auto;
          }
        }
        else {
          
          heightUnit = eStyleUnit_Auto;
        }
      }
    }

    
    
    
    
    
    if (mStyleDisplay->IsRelativelyPositioned(frame) &&
        NS_STYLE_POSITION_RELATIVE == mStyleDisplay->mPosition) {
      uint8_t direction = NS_STYLE_DIRECTION_LTR;
      if (cbrs && NS_STYLE_DIRECTION_RTL == cbrs->mStyleVisibility->mDirection) {
        direction = NS_STYLE_DIRECTION_RTL;
      }
      ComputeRelativeOffsets(direction, frame, aContainingBlockWidth,
          aContainingBlockHeight, ComputedPhysicalOffsets());
    } else {
      
      ComputedPhysicalOffsets().SizeTo(0, 0, 0, 0);
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

      
      if (eStyleUnit_Auto == widthUnit ||
          (width.IsCalcUnit() && width.CalcHasPercent())) {
        ComputedWidth() = AvailableWidth();

        if ((ComputedWidth() != NS_UNCONSTRAINEDSIZE) && !rowOrRowGroup){
          
          
          ComputedWidth() -= ComputedPhysicalBorderPadding().left +
            ComputedPhysicalBorderPadding().right;
          if (ComputedWidth() < 0)
            ComputedWidth() = 0;
        }
        NS_ASSERTION(ComputedWidth() >= 0, "Bogus computed width");
      
      } else {
        NS_ASSERTION(widthUnit == mStylePosition->mWidth.GetUnit(),
                     "unexpected width unit change");
        ComputedWidth() = ComputeWidthValue(aContainingBlockWidth,
                                           mStylePosition->mBoxSizing,
                                           mStylePosition->mWidth);
      }

      
      if ((NS_STYLE_DISPLAY_TABLE_COLUMN == mStyleDisplay->mDisplay) ||
          (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == mStyleDisplay->mDisplay)) {
        
        heightUnit = eStyleUnit_Auto;
      }
      
      if (eStyleUnit_Auto == heightUnit ||
          (height.IsCalcUnit() && height.CalcHasPercent())) {
        ComputedHeight() = NS_AUTOHEIGHT;
      } else {
        NS_ASSERTION(heightUnit == mStylePosition->mHeight.GetUnit(),
                     "unexpected height unit change");
        ComputedHeight() = ComputeHeightValue(aContainingBlockHeight, 
                                             mStylePosition->mBoxSizing,
                                             mStylePosition->mHeight);
      }

      
      ComputedMinWidth() = ComputedMinHeight() = 0;
      ComputedMaxWidth() = ComputedMaxHeight() = NS_UNCONSTRAINEDSIZE;

    } else if (NS_FRAME_GET_TYPE(mFrameType) == NS_CSS_FRAME_TYPE_ABSOLUTE) {
      
      InitAbsoluteConstraints(aPresContext, cbrs, aContainingBlockWidth,
                              aContainingBlockHeight, aFrameType);
    } else {
      AutoMaybeDisableFontInflation an(frame);

      bool isBlock = NS_CSS_FRAME_TYPE_BLOCK == NS_FRAME_GET_TYPE(mFrameType);
      typedef nsIFrame::ComputeSizeFlags ComputeSizeFlags;
      ComputeSizeFlags computeSizeFlags =
        isBlock ? ComputeSizeFlags::eDefault : ComputeSizeFlags::eShrinkWrap;

      
      
      
      if (isBlock &&
          ((aFrameType == nsGkAtoms::legendFrame &&
            frame->StyleContext()->GetPseudo() != nsCSSAnonBoxes::scrolledContent) ||
           (aFrameType == nsGkAtoms::scrollFrame &&
            frame->GetContentInsertionFrame()->GetType() == nsGkAtoms::legendFrame) ||
           (mCBReflowState &&
            mCBReflowState->GetWritingMode().IsOrthogonalTo(mWritingMode)))) {
        computeSizeFlags =
          ComputeSizeFlags(computeSizeFlags | ComputeSizeFlags::eShrinkWrap);
      }

      const nsFlexContainerFrame* flexContainerFrame = GetFlexContainer(frame);
      if (flexContainerFrame) {
        computeSizeFlags =
          ComputeSizeFlags(computeSizeFlags | ComputeSizeFlags::eShrinkWrap);

        
        
        if (mFlags.mIsFlexContainerMeasuringHeight) {
          computeSizeFlags =
            ComputeSizeFlags(computeSizeFlags | ComputeSizeFlags::eUseAutoHeight);
        }
      } else {
        MOZ_ASSERT(!mFlags.mIsFlexContainerMeasuringHeight,
                   "We're not in a flex container, so the flag "
                   "'mIsFlexContainerMeasuringHeight' shouldn't be set");
      }

      LogicalSize cbSize(wm, nsSize(aContainingBlockWidth,
                                    aContainingBlockHeight));
      if (cbSize.ISize(wm) == NS_UNCONSTRAINEDSIZE) {
        
        
        cbSize.ISize(wm) = AvailableISize();
      }

      LogicalSize size =
        frame->ComputeSize(rendContext, wm, cbSize,
                           AvailableISize(),
                           ComputedLogicalMargin().Size(wm),
                           ComputedLogicalBorderPadding().Size(wm) -
                             ComputedLogicalPadding().Size(wm),
                           ComputedLogicalPadding().Size(wm),
                           computeSizeFlags);

      ComputedISize() = size.ISize(wm);
      ComputedBSize() = size.BSize(wm);
      NS_ASSERTION(ComputedISize() >= 0, "Bogus inline-size");
      NS_ASSERTION(ComputedBSize() == NS_UNCONSTRAINEDSIZE ||
                   ComputedBSize() >= 0, "Bogus block-size");

      
      if (isBlock &&
          !IsSideCaption(frame, mStyleDisplay) &&
          mStyleDisplay->mDisplay != NS_STYLE_DISPLAY_INLINE_TABLE &&
          !flexContainerFrame) {
        CalculateBlockSideMargins(aFrameType);
      }
    }
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
nsCSSOffsetState::InitOffsets(nscoord aInlineDirPercentBasis,
                              nscoord aBlockDirPercentBasis,
                              nsIAtom* aFrameType,
                              const nsMargin *aBorder,
                              const nsMargin *aPadding)
{
  DISPLAY_INIT_OFFSETS(frame, this,
                       aInlineDirPercentBasis,
                       aBlockDirPercentBasis,
                       aBorder, aPadding);

  
  
  nsPresContext *presContext = frame->PresContext();
  FrameProperties props(presContext->PropertyTable(), frame);
  props.Delete(nsIFrame::UsedBorderProperty());

  
  
  
  
  bool needMarginProp = ComputeMargin(aInlineDirPercentBasis,
                                      aBlockDirPercentBasis);
  
  
  
  
  ::UpdateProp(props, nsIFrame::UsedMarginProperty(), needMarginProp,
               ComputedPhysicalMargin());


  const nsStyleDisplay *disp = frame->StyleDisplay();
  bool isThemed = frame->IsThemed(disp);
  bool needPaddingProp;
  nsIntMargin widget;
  if (isThemed &&
      presContext->GetTheme()->GetWidgetPadding(presContext->DeviceContext(),
                                                frame, disp->mAppearance,
                                                &widget)) {
    ComputedPhysicalPadding().top = presContext->DevPixelsToAppUnits(widget.top);
    ComputedPhysicalPadding().right = presContext->DevPixelsToAppUnits(widget.right);
    ComputedPhysicalPadding().bottom = presContext->DevPixelsToAppUnits(widget.bottom);
    ComputedPhysicalPadding().left = presContext->DevPixelsToAppUnits(widget.left);
    needPaddingProp = false;
  }
  else if (frame->IsSVGText()) {
    ComputedPhysicalPadding().SizeTo(0, 0, 0, 0);
    needPaddingProp = false;
  }
  else if (aPadding) { 
    ComputedPhysicalPadding() = *aPadding;
    needPaddingProp = frame->StylePadding()->IsWidthDependent() ||
	  (frame->GetStateBits() & NS_FRAME_REFLOW_ROOT);
  }
  else {
    needPaddingProp = ComputePadding(aInlineDirPercentBasis,
                                     aBlockDirPercentBasis, aFrameType);
  }

  if (isThemed) {
    nsIntMargin widget;
    presContext->GetTheme()->GetWidgetBorder(presContext->DeviceContext(),
                                             frame, disp->mAppearance,
                                             &widget);
    ComputedPhysicalBorderPadding().top =
      presContext->DevPixelsToAppUnits(widget.top);
    ComputedPhysicalBorderPadding().right =
      presContext->DevPixelsToAppUnits(widget.right);
    ComputedPhysicalBorderPadding().bottom =
      presContext->DevPixelsToAppUnits(widget.bottom);
    ComputedPhysicalBorderPadding().left =
      presContext->DevPixelsToAppUnits(widget.left);
  }
  else if (frame->IsSVGText()) {
    ComputedPhysicalBorderPadding().SizeTo(0, 0, 0, 0);
  }
  else if (aBorder) {  
    ComputedPhysicalBorderPadding() = *aBorder;
  }
  else {
    ComputedPhysicalBorderPadding() = frame->StyleBorder()->GetComputedBorder();
  }
  ComputedPhysicalBorderPadding() += ComputedPhysicalPadding();

  if (aFrameType == nsGkAtoms::tableFrame) {
    nsTableFrame *tableFrame = static_cast<nsTableFrame*>(frame);

    if (tableFrame->IsBorderCollapse()) {
      
      
      
      
      ComputedPhysicalPadding().SizeTo(0,0,0,0);
      ComputedPhysicalBorderPadding() = tableFrame->GetIncludedOuterBCBorder();
    }

    
    
    ComputedPhysicalMargin().SizeTo(0, 0, 0, 0);
  } else if (aFrameType == nsGkAtoms::scrollbarFrame) {
    
    
    
    nsSize size(frame->GetSize());
    if (size.width == 0 || size.height == 0) {
      ComputedPhysicalPadding().SizeTo(0,0,0,0);
      ComputedPhysicalBorderPadding().SizeTo(0,0,0,0);
    }
  }
  ::UpdateProp(props, nsIFrame::UsedPaddingProperty(), needPaddingProp,
               ComputedPhysicalPadding());
}








void
nsHTMLReflowState::CalculateBlockSideMargins(nsIAtom* aFrameType)
{
  
  
  
  
  
  
  
  
  WritingMode cbWM =
    mCBReflowState ? mCBReflowState->GetWritingMode(): GetWritingMode();

  nscoord availISizeCBWM = AvailableSize(cbWM).ISize(cbWM);
  nscoord computedISizeCBWM = ComputedSize(cbWM).ISize(cbWM);
  if (computedISizeCBWM == NS_UNCONSTRAINEDSIZE) {
    
    
    computedISizeCBWM = availISizeCBWM;
  }

  NS_WARN_IF_FALSE(NS_UNCONSTRAINEDSIZE != computedISizeCBWM &&
                   NS_UNCONSTRAINEDSIZE != availISizeCBWM,
                   "have unconstrained inline-size; this should only result from "
                   "very large sizes, not attempts at intrinsic inline-size "
                   "calculation");

  LogicalMargin margin =
    ComputedLogicalMargin().ConvertTo(cbWM, mWritingMode);
  LogicalMargin borderPadding =
    ComputedLogicalBorderPadding().ConvertTo(cbWM, mWritingMode);
  nscoord sum = margin.IStartEnd(cbWM) +
    borderPadding.IStartEnd(cbWM) + computedISizeCBWM;
  if (sum == availISizeCBWM) {
    
    return;
  }

  
  

  
  nscoord availMarginSpace = availISizeCBWM - sum;

  
  
  if (availMarginSpace < 0) {
    margin.IEnd(cbWM) += availMarginSpace;
    SetComputedLogicalMargin(margin.ConvertTo(mWritingMode, cbWM));
    return;
  }

  
  
  bool isAutoStartMargin, isAutoEndMargin;
  const nsStyleSides& styleSides = mStyleMargin->mMargin;
  if (cbWM.IsVertical()) {
    if (cbWM.IsBidiLTR()) {
      isAutoStartMargin = eStyleUnit_Auto == styleSides.GetTopUnit();
      isAutoEndMargin = eStyleUnit_Auto == styleSides.GetBottomUnit();
    } else {
      isAutoStartMargin = eStyleUnit_Auto == styleSides.GetBottomUnit();
      isAutoEndMargin = eStyleUnit_Auto == styleSides.GetTopUnit();
    }
  } else {
    if (cbWM.IsBidiLTR()) {
      isAutoStartMargin = eStyleUnit_Auto == styleSides.GetLeftUnit();
      isAutoEndMargin = eStyleUnit_Auto == styleSides.GetRightUnit();
    } else {
      isAutoStartMargin = eStyleUnit_Auto == styleSides.GetRightUnit();
      isAutoEndMargin = eStyleUnit_Auto == styleSides.GetLeftUnit();
    }
  }
  if (!isAutoStartMargin && !isAutoEndMargin) {
    
    
    
    
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
      if (prs->mWritingMode.IsBidiLTR()) {
        isAutoStartMargin =
          prs->mStyleText->mTextAlign != NS_STYLE_TEXT_ALIGN_MOZ_LEFT;
        isAutoEndMargin =
          prs->mStyleText->mTextAlign != NS_STYLE_TEXT_ALIGN_MOZ_RIGHT;
      } else {
        isAutoStartMargin =
          prs->mStyleText->mTextAlign != NS_STYLE_TEXT_ALIGN_MOZ_RIGHT;
        isAutoEndMargin =
          prs->mStyleText->mTextAlign != NS_STYLE_TEXT_ALIGN_MOZ_LEFT;
      }
    }
    
    
    else {
      isAutoEndMargin = true;
    }
  }

  
  
  

  if (isAutoStartMargin) {
    if (isAutoEndMargin) {
      
      nscoord forStart = availMarginSpace / 2;
      margin.IStart(cbWM) += forStart;
      margin.IEnd(cbWM) += availMarginSpace - forStart;
    } else {
      margin.IStart(cbWM) += availMarginSpace;
    }
  } else if (isAutoEndMargin) {
    margin.IEnd(cbWM) += availMarginSpace;
  }
  SetComputedLogicalMargin(margin.ConvertTo(mWritingMode, cbWM));
}

#define NORMAL_LINE_HEIGHT_FACTOR 1.2f    // in term of emHeight 








static nscoord
GetNormalLineHeight(nsFontMetrics* aFontMetrics)
{
  NS_PRECONDITION(nullptr != aFontMetrics, "no font metrics");

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
                  nscoord aBlockBSize,
                  float aFontSizeInflation)
{
  const nsStyleCoord& lhCoord = aStyleContext->StyleText()->mLineHeight;

  if (lhCoord.GetUnit() == eStyleUnit_Coord) {
    nscoord result = lhCoord.GetCoordValue();
    if (aFontSizeInflation != 1.0f) {
      result = NSToCoordRound(result * aFontSizeInflation);
    }
    return result;
  }

  if (lhCoord.GetUnit() == eStyleUnit_Factor)
    
    
    
    return NSToCoordRound(lhCoord.GetFactorValue() * aFontSizeInflation *
                          aStyleContext->StyleFont()->mFont.size);

  NS_ASSERTION(lhCoord.GetUnit() == eStyleUnit_Normal ||
               lhCoord.GetUnit() == eStyleUnit_Enumerated,
               "bad line-height unit");
  
  if (lhCoord.GetUnit() == eStyleUnit_Enumerated) {
    NS_ASSERTION(lhCoord.GetIntValue() == NS_STYLE_LINE_HEIGHT_BLOCK_HEIGHT,
                 "bad line-height value");
    if (aBlockBSize != NS_AUTOHEIGHT) {
      return aBlockBSize;
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
  nscoord blockBSize =
    nsLayoutUtils::IsNonWrapperBlock(frame) ? ComputedBSize() :
    (mCBReflowState ? mCBReflowState->ComputedBSize() : NS_AUTOHEIGHT);

  return CalcLineHeight(frame->GetContent(), frame->StyleContext(), blockBSize,
                        nsLayoutUtils::FontSizeInflationFor(frame));
}

 nscoord
nsHTMLReflowState::CalcLineHeight(nsIContent* aContent,
                                  nsStyleContext* aStyleContext,
                                  nscoord aBlockBSize,
                                  float aFontSizeInflation)
{
  NS_PRECONDITION(aStyleContext, "Must have a style context");

  nscoord lineHeight =
    ComputeLineHeight(aStyleContext, aBlockBSize, aFontSizeInflation);

  NS_ASSERTION(lineHeight >= 0, "ComputeLineHeight screwed up");

  HTMLInputElement* input = HTMLInputElement::FromContentOrNull(aContent);
  if (input && input->IsSingleLineTextControl()) {
    
    
    nscoord lineHeightOne =
      aFontSizeInflation * aStyleContext->StyleFont()->mFont.size;
    if (lineHeight < lineHeightOne) {
      lineHeight = lineHeightOne;
    }
  }

  return lineHeight;
}

bool
nsCSSOffsetState::ComputeMargin(nscoord aInlineDirPercentBasis,
                                nscoord aBlockDirPercentBasis)
{
  
  if (frame->IsSVGText()) {
    return false;
  }

  
  const nsStyleMargin *styleMargin = frame->StyleMargin();
  bool isCBDependent = !styleMargin->GetMargin(ComputedPhysicalMargin());
  if (isCBDependent) {
    
    LogicalMargin m(mWritingMode);
    nscoord horizontalPercentBasis =
      mWritingMode.IsVertical() ? aBlockDirPercentBasis
                                : aInlineDirPercentBasis;
    m.Left(mWritingMode) = nsLayoutUtils::
      ComputeCBDependentValue(horizontalPercentBasis,
                              styleMargin->mMargin.GetLeft());
    m.Right(mWritingMode) = nsLayoutUtils::
      ComputeCBDependentValue(horizontalPercentBasis,
                              styleMargin->mMargin.GetRight());

    nscoord verticalPercentBasis =
      mWritingMode.IsVertical() ? aInlineDirPercentBasis
                                : aBlockDirPercentBasis;
    m.Top(mWritingMode) = nsLayoutUtils::
      ComputeCBDependentValue(verticalPercentBasis,
                              styleMargin->mMargin.GetTop());
    m.Bottom(mWritingMode) = nsLayoutUtils::
      ComputeCBDependentValue(verticalPercentBasis,
                              styleMargin->mMargin.GetBottom());

    SetComputedLogicalMargin(m);
  }

  nscoord marginAdjustment = FontSizeInflationListMarginAdjustment(frame);

  if (marginAdjustment > 0) {
    LogicalMargin m = ComputedLogicalMargin();
    m.IStart(mWritingMode) += marginAdjustment;
    SetComputedLogicalMargin(m);
  }

  return isCBDependent;
}

bool
nsCSSOffsetState::ComputePadding(nscoord aInlineDirPercentBasis,
                                 nscoord aBlockDirPercentBasis,
                                 nsIAtom* aFrameType)
{
  
  const nsStylePadding *stylePadding = frame->StylePadding();
  bool isCBDependent = !stylePadding->GetPadding(ComputedPhysicalPadding());
  
  
  if (nsGkAtoms::tableRowGroupFrame == aFrameType ||
      nsGkAtoms::tableColGroupFrame == aFrameType ||
      nsGkAtoms::tableRowFrame      == aFrameType ||
      nsGkAtoms::tableColFrame      == aFrameType) {
    ComputedPhysicalPadding().SizeTo(0,0,0,0);
  }
  else if (isCBDependent) {
    
    
    LogicalMargin p(mWritingMode);
    nscoord horizontalPercentBasis =
      mWritingMode.IsVertical() ? aBlockDirPercentBasis
                                : aInlineDirPercentBasis;
    p.Left(mWritingMode) = std::max(0, nsLayoutUtils::
      ComputeCBDependentValue(horizontalPercentBasis,
                              stylePadding->mPadding.GetLeft()));
    p.Right(mWritingMode) = std::max(0, nsLayoutUtils::
      ComputeCBDependentValue(horizontalPercentBasis,
                              stylePadding->mPadding.GetRight()));

    nscoord verticalPercentBasis =
      mWritingMode.IsVertical() ? aInlineDirPercentBasis
                                : aBlockDirPercentBasis;
    p.Top(mWritingMode) = std::max(0, nsLayoutUtils::
      ComputeCBDependentValue(verticalPercentBasis,
                              stylePadding->mPadding.GetTop()));
    p.Bottom(mWritingMode) = std::max(0, nsLayoutUtils::
      ComputeCBDependentValue(verticalPercentBasis,
                              stylePadding->mPadding.GetBottom()));

    SetComputedLogicalPadding(p);
  }
  return isCBDependent;
}

void
nsHTMLReflowState::ComputeMinMaxValues(nscoord aContainingBlockWidth,
                                       nscoord aContainingBlockHeight,
                                       const nsHTMLReflowState* aContainingBlockRS)
{
  
  
  
  if (eStyleUnit_Auto == mStylePosition->mMinWidth.GetUnit()) {
    ComputedMinWidth() = 0;
  } else {
    ComputedMinWidth() = ComputeWidthValue(aContainingBlockWidth,
                                          mStylePosition->mBoxSizing,
                                          mStylePosition->mMinWidth);
  }

  if (eStyleUnit_None == mStylePosition->mMaxWidth.GetUnit()) {
    
    ComputedMaxWidth() = NS_UNCONSTRAINEDSIZE;  
  } else {
    ComputedMaxWidth() = ComputeWidthValue(aContainingBlockWidth,
                                          mStylePosition->mBoxSizing,
                                          mStylePosition->mMaxWidth);
  }

  
  
  if (ComputedMinWidth() > ComputedMaxWidth()) {
    ComputedMaxWidth() = ComputedMinWidth();
  }

  
  
  
  
  
  

  
  
  
  const nsStyleCoord &minHeight = mStylePosition->mMinHeight;
  if (eStyleUnit_Auto == minHeight.GetUnit() ||
      (NS_AUTOHEIGHT == aContainingBlockHeight &&
       minHeight.HasPercent()) ||
      (mFrameType == NS_CSS_FRAME_TYPE_INTERNAL_TABLE &&
       minHeight.IsCalcUnit() && minHeight.CalcHasPercent()) ||
      mFlags.mIsFlexContainerMeasuringHeight) {
    ComputedMinHeight() = 0;
  } else {
    ComputedMinHeight() = ComputeHeightValue(aContainingBlockHeight, 
                                            mStylePosition->mBoxSizing, 
                                            minHeight);
  }
  const nsStyleCoord &maxHeight = mStylePosition->mMaxHeight;
  nsStyleUnit maxHeightUnit = maxHeight.GetUnit();
  if (eStyleUnit_None == maxHeightUnit) {
    
    ComputedMaxHeight() = NS_UNCONSTRAINEDSIZE;  
  } else {
    
    
    
    
    
    
    if ((NS_AUTOHEIGHT == aContainingBlockHeight && 
         maxHeight.HasPercent()) ||
        (mFrameType == NS_CSS_FRAME_TYPE_INTERNAL_TABLE &&
         maxHeight.IsCalcUnit() && maxHeight.CalcHasPercent()) ||
        mFlags.mIsFlexContainerMeasuringHeight) {
      ComputedMaxHeight() = NS_UNCONSTRAINEDSIZE;
    } else {
      ComputedMaxHeight() = ComputeHeightValue(aContainingBlockHeight, 
                                              mStylePosition->mBoxSizing,
                                              maxHeight);
    }
  }

  
  
  if (ComputedMinHeight() > ComputedMaxHeight()) {
    ComputedMaxHeight() = ComputedMinHeight();
  }
}

void
nsHTMLReflowState::SetTruncated(const nsHTMLReflowMetrics& aMetrics,
                                nsReflowStatus* aStatus) const
{
  if (AvailableHeight() != NS_UNCONSTRAINEDSIZE &&
      AvailableHeight() < aMetrics.Height() &&
      !mFlags.mIsTopOfPage) {
    *aStatus |= NS_FRAME_TRUNCATED;
  } else {
    *aStatus &= ~NS_FRAME_TRUNCATED;
  }
}

bool
nsHTMLReflowState::IsFloating() const
{
  return mStyleDisplay->IsFloating(frame);
}

uint8_t
nsHTMLReflowState::GetDisplay() const
{
  return mStyleDisplay->GetDisplay(frame);
}
