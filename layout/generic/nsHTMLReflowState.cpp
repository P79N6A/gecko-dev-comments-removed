






#include "nsHTMLReflowState.h"

#include "LayoutLogging.h"
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
#include "nsIPercentBSizeObserver.h"
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
  mPercentBSizeObserver = nullptr;

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
                                   WritingMode aContainingBlockWritingMode,
                                   nscoord aContainingBlockISize)
  : frame(aFrame)
  , rendContext(aRenderingContext)
  , mWritingMode(aFrame->GetWritingMode())
{
  MOZ_ASSERT(!aFrame->IsFlexOrGridItem(),
             "We're about to resolve percent margin & padding "
             "values against CB inline size, which is incorrect for "
             "flex/grid items");
  LogicalSize cbSize(aContainingBlockWritingMode, aContainingBlockISize,
                     aContainingBlockISize);
  InitOffsets(aContainingBlockWritingMode, cbSize, frame->GetType());
}




nsHTMLReflowState::nsHTMLReflowState(
                     nsPresContext*           aPresContext,
                     const nsHTMLReflowState& aParentReflowState,
                     nsIFrame*                aFrame,
                     const LogicalSize&       aAvailableSpace,
                     const LogicalSize*       aContainingBlockSize,
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
  NS_PRECONDITION(!mFlags.mSpecialBSizeReflow ||
                  !NS_SUBTREE_DIRTY(aFrame),
                  "frame should be clean when getting special bsize reflow");

  parentReflowState = &aParentReflowState;

  
  
  
  if (!mFlags.mSpecialBSizeReflow)
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
  mPercentBSizeObserver = (aParentReflowState.mPercentBSizeObserver &&
                            aParentReflowState.mPercentBSizeObserver->NeedsToObserve(*this))
                           ? aParentReflowState.mPercentBSizeObserver : nullptr;

  if ((aFlags & DUMMY_PARENT_REFLOW_STATE) ||
      (parentReflowState->mFlags.mDummyParentReflowState &&
       frame->GetType() == nsGkAtoms::tableFrame)) {
    mFlags.mDummyParentReflowState = true;
  }

  if (!(aFlags & CALLER_WILL_INIT)) {
    Init(aPresContext, aContainingBlockSize);
  }
}

inline nscoord
nsCSSOffsetState::ComputeISizeValue(nscoord aContainingBlockISize,
                                    nscoord aContentEdgeToBoxSizing,
                                    nscoord aBoxSizingToMarginEdge,
                                    const nsStyleCoord& aCoord)
{
  return nsLayoutUtils::ComputeISizeValue(rendContext, frame,
                                          aContainingBlockISize,
                                          aContentEdgeToBoxSizing,
                                          aBoxSizingToMarginEdge,
                                          aCoord);
}

nscoord
nsCSSOffsetState::ComputeISizeValue(nscoord aContainingBlockISize,
                                    uint8_t aBoxSizing,
                                    const nsStyleCoord& aCoord)
{
  WritingMode wm = GetWritingMode();
  nscoord inside = 0, outside = ComputedLogicalBorderPadding().IStartEnd(wm) +
                                ComputedLogicalMargin().IStartEnd(wm);
  switch (aBoxSizing) {
    case NS_STYLE_BOX_SIZING_BORDER:
      inside = ComputedLogicalBorderPadding().IStartEnd(wm);
      break;
    case NS_STYLE_BOX_SIZING_PADDING:
      inside = ComputedLogicalPadding().IStartEnd(wm);
      break;
  }
  outside -= inside;

  return ComputeISizeValue(aContainingBlockISize, inside,
                           outside, aCoord);
}

nscoord
nsCSSOffsetState::ComputeBSizeValue(nscoord aContainingBlockBSize,
                                    uint8_t aBoxSizing,
                                    const nsStyleCoord& aCoord)
{
  WritingMode wm = GetWritingMode();
  nscoord inside = 0;
  switch (aBoxSizing) {
    case NS_STYLE_BOX_SIZING_BORDER:
      inside = ComputedLogicalBorderPadding().BStartEnd(wm);
      break;
    case NS_STYLE_BOX_SIZING_PADDING:
      inside = ComputedLogicalPadding().BStartEnd(wm);
      break;
  }
  return nsLayoutUtils::ComputeBSizeValue(aContainingBlockBSize,
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
nsHTMLReflowState::Init(nsPresContext*     aPresContext,
                        const LogicalSize* aContainingBlockSize,
                        const nsMargin*    aBorder,
                        const nsMargin*    aPadding)
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

  LAYOUT_WARN_IF_FALSE(AvailableISize() != NS_UNCONSTRAINEDSIZE,
                       "have unconstrained inline-size; this should only "
                       "result from very large sizes, not attempts at "
                       "intrinsic inline-size calculation");

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

  LogicalSize cbSize(mWritingMode, -1, -1);
  if (aContainingBlockSize) {
    cbSize = *aContainingBlockSize;
  }

  InitConstraints(aPresContext, cbSize, aBorder, aPadding, type);

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
    const nsStyleCoord& bSizeCoord = mStylePosition->BSize(mWritingMode);
    const nsStyleCoord& maxBSizeCoord = mStylePosition->MaxBSize(mWritingMode);
    if ((bSizeCoord.GetUnit() != eStyleUnit_Auto ||
         maxBSizeCoord.GetUnit() != eStyleUnit_None) &&
         
         (frame->GetContent() &&
        !(frame->GetContent()->IsAnyOfHTMLElements(nsGkAtoms::body,
                                                   nsGkAtoms::html)))) {

      
      
      
      nsIFrame* containingBlk = frame;
      while (containingBlk) {
        const nsStylePosition* stylePos = containingBlk->StylePosition();
        const nsStyleCoord& bSizeCoord = stylePos->BSize(mWritingMode);
        const nsStyleCoord& maxBSizeCoord = stylePos->MaxBSize(mWritingMode);
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

  if (parentReflowState &&
      parentReflowState->GetWritingMode().IsOrthogonalTo(mWritingMode)) {
    
    
    
    
    
    if (type == nsGkAtoms::columnSetFrame &&
        eStyleUnit_Auto == mStylePosition->ISize(mWritingMode).GetUnit()) {
      ComputedISize() = NS_UNCONSTRAINEDSIZE;
    } else {
      AvailableBSize() = NS_UNCONSTRAINEDSIZE;
    }
  }

  LAYOUT_WARN_IF_FALSE((mFrameType == NS_CSS_FRAME_TYPE_INLINE &&
                        !frame->IsFrameOfType(nsIFrame::eReplaced)) ||
                       type == nsGkAtoms::textFrame ||
                       ComputedISize() != NS_UNCONSTRAINEDSIZE,
                       "have unconstrained inline-size; this should only "
                       "result from very large sizes, not attempts at "
                       "intrinsic inline-size calculation");
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
  const WritingMode wm = mWritingMode; 
  bool isIResize = (frame->ISize(wm) !=
                     ComputedISize() +
                       ComputedLogicalBorderPadding().IStartEnd(wm)) ||
                     aPresContext->PresShell()->IsReflowOnZoomPending();

  if ((frame->GetStateBits() & NS_FRAME_FONT_INFLATION_FLOW_ROOT) &&
      nsLayoutUtils::FontSizeInflationEnabled(aPresContext)) {
    
    
    bool dirty = nsFontInflationData::UpdateFontInflationDataISizeFor(*this) &&
                 
                 
                 
                 
                 !mFlags.mDummyParentReflowState;

    if (dirty || (!frame->GetParent() && isIResize)) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      

      
      
      
      
      
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

  SetIResize(!(frame->GetStateBits() & NS_FRAME_IS_DIRTY) &&
             isIResize);

  
  
  if (IS_TABLE_CELL(aFrameType) &&
      (mFlags.mSpecialBSizeReflow ||
       (frame->FirstInFlow()->GetStateBits() &
         NS_TABLE_CELL_HAD_SPECIAL_REFLOW)) &&
      (frame->GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_BSIZE)) {
    
    
    
    SetBResize(true);
  } else if (mCBReflowState && !nsLayoutUtils::IsNonWrapperBlock(frame)) {
    
    
    
    
    SetBResize(mCBReflowState->IsBResize());
  } else if (ComputedBSize() == NS_AUTOHEIGHT) {
    if (eCompatibility_NavQuirks == aPresContext->CompatibilityMode() &&
        mCBReflowState) {
      SetBResize(mCBReflowState->IsBResize());
    } else {
      SetBResize(IsIResize());
    }
    SetBResize(IsBResize() || NS_SUBTREE_DIRTY(frame));
  } else {
    
    SetBResize(frame->BSize(wm) !=
               ComputedBSize() + ComputedLogicalBorderPadding().BStartEnd(wm));
  }

  bool dependsOnCBBSize =
    (mStylePosition->BSizeDependsOnContainer(wm) &&
     
     mStylePosition->BSize(wm).GetUnit() != eStyleUnit_Auto) ||
    mStylePosition->MinBSizeDependsOnContainer(wm) ||
    mStylePosition->MaxBSizeDependsOnContainer(wm) ||
    mStylePosition->OffsetHasPercent(wm.PhysicalSide(eLogicalSideBStart)) ||
    mStylePosition->mOffset.GetBEndUnit(wm) != eStyleUnit_Auto ||
    frame->IsBoxFrame();

  if (mStyleText->mLineHeight.GetUnit() == eStyleUnit_Enumerated) {
    NS_ASSERTION(mStyleText->mLineHeight.GetIntValue() ==
                 NS_STYLE_LINE_HEIGHT_BLOCK_HEIGHT,
                 "bad line-height value");

    
    frame->AddStateBits(NS_FRAME_CONTAINS_RELATIVE_BSIZE);
    
    dependsOnCBBSize |= !nsLayoutUtils::IsNonWrapperBlock(frame);
  }

  
  
  
  
  
  
  if (!IsBResize() && mCBReflowState &&
      (IS_TABLE_CELL(mCBReflowState->frame->GetType()) || 
       mCBReflowState->mFlags.mHeightDependsOnAncestorCell) &&
      !mCBReflowState->mFlags.mSpecialBSizeReflow && 
      dependsOnCBBSize) {
    SetBResize(true);
    mFlags.mHeightDependsOnAncestorCell = true;
  }

  

  
  
  
  
  
  
  
  if (dependsOnCBBSize && mCBReflowState) {
    const nsHTMLReflowState *rs = this;
    bool hitCBReflowState = false;
    do {
      rs = rs->parentReflowState;
      if (!rs) {
        break;
      }
        
      if (rs->frame->GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_BSIZE)
        break; 
      rs->frame->AddStateBits(NS_FRAME_CONTAINS_RELATIVE_BSIZE);
      
      
      
      if (rs == mCBReflowState) {
        hitCBReflowState = true;
      }

      
      
      
    } while (!hitCBReflowState ||
             (eCompatibility_NavQuirks == aPresContext->CompatibilityMode() &&
              !IsQuirkContainingBlockHeight(rs, rs->frame->GetType())));
    
    
    
    
    
    
  }
  if (frame->GetStateBits() & NS_FRAME_IS_DIRTY) {
    
    
    frame->RemoveStateBits(NS_FRAME_CONTAINS_RELATIVE_BSIZE);
  }
}

nscoord
nsHTMLReflowState::GetContainingBlockContentISize(WritingMode aWritingMode) const
{
  if (!mCBReflowState) {
    return 0;
  }
  return mCBReflowState->GetWritingMode().IsOrthogonalTo(aWritingMode)
    ? mCBReflowState->ComputedBSize()
    : mCBReflowState->ComputedISize();
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
nsHTMLReflowState::ComputeRelativeOffsets(WritingMode aWM,
                                          nsIFrame* aFrame,
                                          const LogicalSize& aCBSize,
                                          nsMargin& aComputedOffsets)
{
  LogicalMargin offsets(aWM);
  mozilla::css::Side inlineStart = aWM.PhysicalSide(eLogicalSideIStart);
  mozilla::css::Side inlineEnd   = aWM.PhysicalSide(eLogicalSideIEnd);
  mozilla::css::Side blockStart  = aWM.PhysicalSide(eLogicalSideBStart);
  mozilla::css::Side blockEnd    = aWM.PhysicalSide(eLogicalSideBEnd);

  const nsStylePosition* position = aFrame->StylePosition();

  
  
  
  
  bool inlineStartIsAuto =
    eStyleUnit_Auto == position->mOffset.GetUnit(inlineStart);
  bool inlineEndIsAuto =
    eStyleUnit_Auto == position->mOffset.GetUnit(inlineEnd);

  
  
  if (!inlineStartIsAuto && !inlineEndIsAuto) {
    inlineEndIsAuto = true;
  }

  if (inlineStartIsAuto) {
    if (inlineEndIsAuto) {
      
      offsets.IStart(aWM) = offsets.IEnd(aWM) = 0;
    } else {
      
      offsets.IEnd(aWM) = nsLayoutUtils::
        ComputeCBDependentValue(aCBSize.ISize(aWM),
                                position->mOffset.Get(inlineEnd));

      
      offsets.IStart(aWM) = -offsets.IEnd(aWM);
    }

  } else {
    NS_ASSERTION(inlineEndIsAuto, "unexpected specified constraint");

    
    offsets.IStart(aWM) = nsLayoutUtils::
      ComputeCBDependentValue(aCBSize.ISize(aWM),
                              position->mOffset.Get(inlineStart));

    
    offsets.IEnd(aWM) = -offsets.IStart(aWM);
  }

  
  
  
  
  bool blockStartIsAuto =
    eStyleUnit_Auto == position->mOffset.GetUnit(blockStart);
  bool blockEndIsAuto =
    eStyleUnit_Auto == position->mOffset.GetUnit(blockEnd);

  
  
  if (NS_AUTOHEIGHT == aCBSize.BSize(aWM)) {
    if (position->OffsetHasPercent(blockStart)) {
      blockStartIsAuto = true;
    }
    if (position->OffsetHasPercent(blockEnd)) {
      blockEndIsAuto = true;
    }
  }

  
  if (!blockStartIsAuto && !blockEndIsAuto) {
    blockEndIsAuto = true;
  }

  if (blockStartIsAuto) {
    if (blockEndIsAuto) {
      
      offsets.BStart(aWM) = offsets.BEnd(aWM) = 0;
    } else {
      
      offsets.BEnd(aWM) = nsLayoutUtils::
        ComputeBSizeDependentValue(aCBSize.BSize(aWM),
                                   position->mOffset.Get(blockEnd));

      
      offsets.BStart(aWM) = -offsets.BEnd(aWM);
    }

  } else {
    NS_ASSERTION(blockEndIsAuto, "unexpected specified constraint");

    
    offsets.BStart(aWM) = nsLayoutUtils::
      ComputeBSizeDependentValue(aCBSize.BSize(aWM),
                                 position->mOffset.Get(blockStart));

    
    offsets.BEnd(aWM) = -offsets.BStart(aWM);
  }

  
  aComputedOffsets = offsets.GetPhysicalMargin(aWM);
  FrameProperties props = aFrame->Properties();
  nsMargin* physicalOffsets = static_cast<nsMargin*>
    (props.Get(nsIFrame::ComputedOffsetProperty()));
  if (physicalOffsets) {
    *physicalOffsets = aComputedOffsets;
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
                                               nscoord& aCBIStartEdge,
                                               nscoord& aCBISize)
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

  WritingMode wm = aFrame->GetWritingMode();
  if (state) {
    WritingMode stateWM = state->GetWritingMode();
    aCBIStartEdge =
      state->ComputedLogicalBorderPadding().ConvertTo(wm, stateWM).IStart(wm);
    aCBISize = state->ComputedSize(wm).ISize(wm);
  } else {
    


    NS_ASSERTION(!(aFrame->GetStateBits() & NS_FRAME_IN_REFLOW),
                 "aFrame shouldn't be in reflow; we'll lie if it is");
    LogicalMargin borderPadding = aFrame->GetLogicalUsedBorderAndPadding(wm);
    aCBIStartEdge = borderPadding.IStart(wm);
    aCBISize = aFrame->ISize(wm) - borderPadding.IStartEnd(wm);
  }

  return aFrame;
}







struct nsHypotheticalBox {
  
  nscoord       mIStart, mIEnd;
  
  nscoord       mBStart;
  WritingMode   mWritingMode;
#ifdef DEBUG
  bool          mIStartIsExact, mIEndIsExact;
#endif

  nsHypotheticalBox() {
#ifdef DEBUG
    mIStartIsExact = mIEndIsExact = false;
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
nsHTMLReflowState::CalculateInlineBorderPaddingMargin(
                       nscoord aContainingBlockISize,
                       nscoord* aInsideBoxSizing,
                       nscoord* aOutsideBoxSizing)
{
  WritingMode wm = GetWritingMode();
  mozilla::css::Side inlineStart = wm.PhysicalSide(eLogicalSideIStart);
  mozilla::css::Side inlineEnd   = wm.PhysicalSide(eLogicalSideIEnd);

  const LogicalMargin& border =
    LogicalMargin(wm, mStyleBorder->GetComputedBorder());
  LogicalMargin padding(wm), margin(wm);

  
  nsMargin stylePadding;
  if (mStylePadding->GetPadding(stylePadding)) {
    padding = LogicalMargin(wm, stylePadding);
  } else {
    
    padding.IStart(wm) = nsLayoutUtils::
      ComputeCBDependentValue(aContainingBlockISize,
                              mStylePadding->mPadding.Get(inlineStart));
    padding.IEnd(wm) = nsLayoutUtils::
      ComputeCBDependentValue(aContainingBlockISize,
                              mStylePadding->mPadding.Get(inlineEnd));
  }

  
  nsMargin styleMargin;
  if (mStyleMargin->GetMargin(styleMargin)) {
    margin = LogicalMargin(wm, styleMargin);
  } else {
    
    if (eStyleUnit_Auto == mStyleMargin->mMargin.GetUnit(inlineStart)) {
      
      margin.IStart(wm) = 0;  
    } else {
      margin.IStart(wm) = nsLayoutUtils::
        ComputeCBDependentValue(aContainingBlockISize,
                                mStyleMargin->mMargin.Get(inlineStart));
    }
    if (eStyleUnit_Auto == mStyleMargin->mMargin.GetUnit(inlineEnd)) {
      
      margin.IEnd(wm) = 0;  
    } else {
      margin.IEnd(wm) = nsLayoutUtils::
        ComputeCBDependentValue(aContainingBlockISize,
                                mStyleMargin->mMargin.Get(inlineEnd));
    }
  }

  nscoord outside =
    padding.IStartEnd(wm) + border.IStartEnd(wm) + margin.IStartEnd(wm);
  nscoord inside = 0;
  switch (mStylePosition->mBoxSizing) {
    case NS_STYLE_BOX_SIZING_BORDER:
      inside += border.IStartEnd(wm);
      
    case NS_STYLE_BOX_SIZING_PADDING:
      inside += padding.IStartEnd(wm);
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
  for (nsIFrame* f : aFrame->PrincipalChildList()) {
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
                                            const nsHTMLReflowState* cbrs,
                                            nsHypotheticalBox& aHypotheticalBox,
                                            nsIAtom*          aFrameType)
{
  NS_ASSERTION(mStyleDisplay->mOriginalDisplay != NS_STYLE_DISPLAY_NONE,
               "mOriginalDisplay has not been properly initialized");

  
  
  nscoord blockIStartContentEdge, blockContentISize;
  nsIFrame* containingBlock =
    GetHypotheticalBoxContainer(aPlaceholderFrame, blockIStartContentEdge,
                                blockContentISize);

  
  
  
  WritingMode wm = containingBlock->GetWritingMode();
  aHypotheticalBox.mWritingMode = wm;

  nsStyleCoord styleISize = mStylePosition->ISize(wm);
  bool isAutoISize = styleISize.GetUnit() == eStyleUnit_Auto;
  nsSize      intrinsicSize;
  bool        knowIntrinsicSize = false;
  if (NS_FRAME_IS_REPLACED(mFrameType) && isAutoISize) {
    
    knowIntrinsicSize = GetIntrinsicSizeFor(frame, intrinsicSize, aFrameType);
  }

  
  
  nscoord boxISize;
  bool    knowBoxISize = false;
  if ((NS_STYLE_DISPLAY_INLINE == mStyleDisplay->mOriginalDisplay) &&
      !NS_FRAME_IS_REPLACED(mFrameType)) {
    
    
    

  } else {
    

    
    
    
    
    nscoord insideBoxSizing, outsideBoxSizing;
    CalculateInlineBorderPaddingMargin(blockContentISize,
                                       &insideBoxSizing, &outsideBoxSizing);

    if (NS_FRAME_IS_REPLACED(mFrameType) && isAutoISize) {
      
      
      if (knowIntrinsicSize) {
        boxISize = LogicalSize(wm, intrinsicSize).ISize(wm) +
                   outsideBoxSizing + insideBoxSizing;
        knowBoxISize = true;
      }

    } else if (isAutoISize) {
      
      boxISize = blockContentISize;
      knowBoxISize = true;

    } else {
      
      
      
      boxISize = ComputeISizeValue(blockContentISize,
                                   insideBoxSizing, outsideBoxSizing,
                                   styleISize) +
                 insideBoxSizing + outsideBoxSizing;
      knowBoxISize = true;
    }
  }

  
  
  
  
  WritingMode cbwm = cbrs->GetWritingMode();
  nscoord containerWidth = containingBlock->GetStateBits() & NS_FRAME_IN_REFLOW
    ? cbrs->ComputedWidth() +
      cbrs->ComputedLogicalBorderPadding().LeftRight(cbwm)
    : containingBlock->GetSize().width;
  LogicalPoint placeholderOffset(wm, aPlaceholderFrame->GetOffsetTo(containingBlock),
                                 containerWidth);

  
  if (wm.IsVertical() && !wm.IsBidiLTR()) {
    placeholderOffset.I(wm) = cbrs->ComputedHeight() +
      cbrs->ComputedLogicalBorderPadding().TopBottom(cbwm) -
      placeholderOffset.I(wm);
  }

  
  
  
  
  nsBlockFrame* blockFrame =
    nsLayoutUtils::GetAsBlock(containingBlock->GetContentInsertionFrame());
  if (blockFrame) {
    LogicalPoint blockOffset(wm, blockFrame->GetOffsetTo(containingBlock), 0);
    bool isValid;
    nsBlockInFlowLineIterator iter(blockFrame, aPlaceholderFrame, &isValid);
    if (!isValid) {
      
      
      aHypotheticalBox.mBStart = placeholderOffset.B(wm);
    } else {
      NS_ASSERTION(iter.GetContainer() == blockFrame,
                   "Found placeholder in wrong block!");
      nsBlockFrame::line_iterator lineBox = iter.GetLine();

      
      
      LogicalRect lineBounds =
        lineBox->GetBounds().ConvertTo(wm, lineBox->mWritingMode,
                                       lineBox->mContainerWidth);
      if (mStyleDisplay->IsOriginalDisplayInlineOutsideStyle()) {
        
        
        aHypotheticalBox.mBStart = lineBounds.BStart(wm) + blockOffset.B(wm);
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
            
            
            
            aHypotheticalBox.mBStart = lineBounds.BStart(wm) + blockOffset.B(wm);
          } else {
            
            
            aHypotheticalBox.mBStart = lineBounds.BEnd(wm) + blockOffset.B(wm);
          }
        } else {
          
          aHypotheticalBox.mBStart = placeholderOffset.B(wm);
        }
      }
    }
  } else {
    
    
    
    aHypotheticalBox.mBStart = placeholderOffset.B(wm);
  }

  
  
  
  if (mStyleDisplay->IsOriginalDisplayInlineOutsideStyle()) {
    
    aHypotheticalBox.mIStart = placeholderOffset.I(wm);
  } else {
    aHypotheticalBox.mIStart = blockIStartContentEdge;
  }
#ifdef DEBUG
  aHypotheticalBox.mIStartIsExact = true;
#endif

  if (knowBoxISize) {
    aHypotheticalBox.mIEnd = aHypotheticalBox.mIStart + boxISize;
#ifdef DEBUG
    aHypotheticalBox.mIEndIsExact = true;
#endif
  } else {
    
    
    
    aHypotheticalBox.mIEnd = blockIStartContentEdge + blockContentISize;
#ifdef DEBUG
    aHypotheticalBox.mIEndIsExact = false;
#endif
  }

  
  
  
  
  
  nsPoint cbOffset;
  if (mStyleDisplay->mPosition == NS_STYLE_POSITION_FIXED &&
      
      nsLayoutUtils::IsReallyFixedPos(frame)) {
    
    
    
    
    
    cbOffset.MoveTo(0, 0);
    do {
      cbOffset += containingBlock->GetPositionIgnoringScrolling();
      nsContainerFrame* parent = containingBlock->GetParent();
      if (!parent) {
        
        
        
        
        
        
        cbOffset -= containingBlock->GetOffsetTo(cbrs->frame);
        break;
      }
      containingBlock = parent;
    } while (containingBlock != cbrs->frame);
  } else {
    
    
    
    
    cbOffset = containingBlock->GetOffsetTo(cbrs->frame);
  }
  nscoord cbrsWidth = cbrs->ComputedWidth() +
                        cbrs->ComputedLogicalBorderPadding().LeftRight(cbwm);
  LogicalPoint logCBOffs(wm, cbOffset, cbrsWidth - containerWidth);
  aHypotheticalBox.mIStart += logCBOffs.I(wm);
  aHypotheticalBox.mIEnd += logCBOffs.I(wm);
  aHypotheticalBox.mBStart += logCBOffs.B(wm);

  
  
  
  LogicalMargin border =
    cbrs->ComputedLogicalBorderPadding() - cbrs->ComputedLogicalPadding();
  border = border.ConvertTo(wm, cbrs->GetWritingMode());
  aHypotheticalBox.mIStart -= border.IStart(wm);
  aHypotheticalBox.mIEnd -= border.IStart(wm);
  aHypotheticalBox.mBStart -= border.BStart(wm);
}

void
nsHTMLReflowState::InitAbsoluteConstraints(nsPresContext* aPresContext,
                                           const nsHTMLReflowState* cbrs,
                                           const LogicalSize& aCBSize,
                                           nsIAtom* aFrameType)
{
  WritingMode wm = GetWritingMode();
  WritingMode cbwm = cbrs->GetWritingMode();
  NS_PRECONDITION(aCBSize.BSize(cbwm) != NS_AUTOHEIGHT,
                  "containing block bsize must be constrained");

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
    CalculateHypotheticalBox(aPresContext, placeholderFrame, cbrs,
                             hypotheticalBox, aFrameType);
  }

  
  

  bool iStartIsAuto = false, iEndIsAuto = false;
  bool bStartIsAuto = false, bEndIsAuto = false;

  
  LogicalSize cbSize = aCBSize;

  LogicalMargin offsets = ComputedLogicalOffsets().ConvertTo(cbwm, wm);

  if (eStyleUnit_Auto == mStylePosition->mOffset.GetIStartUnit(cbwm)) {
    offsets.IStart(cbwm) = 0;
    iStartIsAuto = true;
  } else {
    offsets.IStart(cbwm) = nsLayoutUtils::
      ComputeCBDependentValue(cbSize.ISize(cbwm),
                              mStylePosition->mOffset.GetIStart(cbwm));
  }
  if (eStyleUnit_Auto == mStylePosition->mOffset.GetIEndUnit(cbwm)) {
    offsets.IEnd(cbwm) = 0;
    iEndIsAuto = true;
  } else {
    offsets.IEnd(cbwm) = nsLayoutUtils::
      ComputeCBDependentValue(cbSize.ISize(cbwm),
                              mStylePosition->mOffset.GetIEnd(cbwm));
  }

  if (iStartIsAuto && iEndIsAuto) {
    NS_ASSERTION(hypotheticalBox.mIStartIsExact, "should always have "
                 "exact value on containing block's start side");
    if (cbwm.IsBidiLTR() != hypotheticalBox.mWritingMode.IsBidiLTR()) {
      offsets.IEnd(cbwm) = hypotheticalBox.mIStart;
      iEndIsAuto = false;
    } else {
      offsets.IStart(cbwm) = hypotheticalBox.mIStart;
      iStartIsAuto = false;
    }
  }

  if (eStyleUnit_Auto == mStylePosition->mOffset.GetBStartUnit(cbwm)) {
    offsets.BStart(cbwm) = 0;
    bStartIsAuto = true;
  } else {
    offsets.BStart(cbwm) = nsLayoutUtils::
      ComputeBSizeDependentValue(cbSize.BSize(cbwm),
                                 mStylePosition->mOffset.GetBStart(cbwm));
  }
  if (eStyleUnit_Auto == mStylePosition->mOffset.GetBEndUnit(cbwm)) {
    offsets.BEnd(cbwm) = 0;
    bEndIsAuto = true;
  } else {
    offsets.BEnd(cbwm) = nsLayoutUtils::
      ComputeBSizeDependentValue(cbSize.BSize(cbwm),
                                 mStylePosition->mOffset.GetBEnd(cbwm));
  }

  if (bStartIsAuto && bEndIsAuto) {
    
    NS_ASSERTION(hypotheticalBox.mWritingMode.GetBlockDir() == cbwm.GetBlockDir(),
                 "block direction mismatch");
    offsets.BStart(cbwm) = hypotheticalBox.mBStart;
    bStartIsAuto = false;
  }

  SetComputedLogicalOffsets(offsets.ConvertTo(wm, cbwm));

  bool iSizeIsAuto = eStyleUnit_Auto == mStylePosition->ISize(cbwm).GetUnit();
  bool bSizeIsAuto = eStyleUnit_Auto == mStylePosition->BSize(cbwm).GetUnit();

  typedef nsIFrame::ComputeSizeFlags ComputeSizeFlags;
  ComputeSizeFlags computeSizeFlags = ComputeSizeFlags::eDefault;
  if (wm.IsOrthogonalTo(cbwm)) {
    if (bStartIsAuto || bEndIsAuto) {
      computeSizeFlags =
        ComputeSizeFlags(computeSizeFlags | ComputeSizeFlags::eShrinkWrap);
    }
  } else {
    if (iStartIsAuto || iEndIsAuto) {
      computeSizeFlags =
        ComputeSizeFlags(computeSizeFlags | ComputeSizeFlags::eShrinkWrap);
    }
  }

  LogicalSize computedSize(wm);
  {
    AutoMaybeDisableFontInflation an(frame);

    computedSize =
      frame->ComputeSize(rendContext, wm, cbSize.ConvertTo(wm, cbwm),
                         cbSize.ConvertTo(wm, cbwm).ISize(wm), 
                         ComputedLogicalMargin().Size(wm) +
                           ComputedLogicalOffsets().Size(wm),
                         ComputedLogicalBorderPadding().Size(wm) -
                           ComputedLogicalPadding().Size(wm),
                         ComputedLogicalPadding().Size(wm),
                         computeSizeFlags);
    ComputedISize() = computedSize.ISize(wm);
    ComputedBSize() = computedSize.BSize(wm);
    NS_ASSERTION(ComputedISize() >= 0, "Bogus inline-size");
    NS_ASSERTION(ComputedBSize() == NS_UNCONSTRAINEDSIZE ||
                 ComputedBSize() >= 0, "Bogus block-size");
  }
  computedSize = computedSize.ConvertTo(cbwm, wm);

  
  

  LogicalMargin margin = ComputedLogicalMargin().ConvertTo(cbwm, wm);
  const LogicalMargin borderPadding =
    ComputedLogicalBorderPadding().ConvertTo(cbwm, wm);

  if (iStartIsAuto) {
    
    
    
    if (iSizeIsAuto) {
      
      
      
      offsets.IStart(cbwm) = NS_AUTOOFFSET;
    } else {
      offsets.IStart(cbwm) =
        cbSize.ISize(cbwm) - offsets.IEnd(cbwm) -
        computedSize.ISize(cbwm) - margin.IStartEnd(cbwm) -
        borderPadding.IStartEnd(cbwm);
    }
  } else if (iEndIsAuto) {
    
    
    
    if (iSizeIsAuto) {
      
      
      
      offsets.IEnd(cbwm) = NS_AUTOOFFSET;
    } else {
      offsets.IEnd(cbwm) =
        cbSize.ISize(cbwm) - offsets.IStart(cbwm) -
        computedSize.ISize(cbwm) - margin.IStartEnd(cbwm) -
        borderPadding.IStartEnd(cbwm);
    }
  } else {
    

    if (wm.IsOrthogonalTo(cbwm)) {
      
      
      
      nscoord autoISize = cbSize.ISize(cbwm) - margin.IStartEnd(cbwm) -
        borderPadding.IStartEnd(cbwm) - offsets.IStartEnd(cbwm);
      if (autoISize < 0) {
        autoISize = 0;
      }

      if (computedSize.ISize(cbwm) == NS_UNCONSTRAINEDSIZE) {
        
        
        computedSize.ISize(cbwm) = autoISize;

        
        LogicalSize maxSize = ComputedMaxSize(cbwm);
        LogicalSize minSize = ComputedMinSize(cbwm);
        if (computedSize.ISize(cbwm) > maxSize.ISize(cbwm)) {
          computedSize.ISize(cbwm) = maxSize.ISize(cbwm);
        }
        if (computedSize.ISize(cbwm) < minSize.ISize(cbwm)) {
          computedSize.ISize(cbwm) = minSize.ISize(cbwm);
        }
      }
    }

    
    
    
    
    
    

    nscoord availMarginSpace =
      aCBSize.ISize(cbwm) - offsets.IStartEnd(cbwm) - margin.IStartEnd(cbwm) -
      borderPadding.IStartEnd(cbwm) - computedSize.ISize(cbwm);
    bool marginIStartIsAuto =
      eStyleUnit_Auto == mStyleMargin->mMargin.GetIStartUnit(cbwm);
    bool marginIEndIsAuto =
      eStyleUnit_Auto == mStyleMargin->mMargin.GetIEndUnit(cbwm);

    if (marginIStartIsAuto) {
      if (marginIEndIsAuto) {
        if (availMarginSpace < 0) {
          
          
          
          margin.IEnd(cbwm) = availMarginSpace;
        } else {
          
          
          margin.IStart(cbwm) = availMarginSpace / 2;
          margin.IEnd(cbwm) = availMarginSpace - margin.IStart(cbwm);
        }
      } else {
        
        margin.IStart(cbwm) = availMarginSpace;
      }
    } else {
      if (marginIEndIsAuto) {
        
        margin.IEnd(cbwm) = availMarginSpace;
      } else {
        
        
        
        
        
        
        
        
        offsets.IEnd(cbwm) += availMarginSpace;
      }
    }
  }

  if (bStartIsAuto) {
    
    if (bSizeIsAuto) {
      offsets.BStart(cbwm) = NS_AUTOOFFSET;
    } else {
      offsets.BStart(cbwm) = cbSize.BSize(cbwm) - margin.BStartEnd(cbwm) -
        borderPadding.BStartEnd(cbwm) - computedSize.BSize(cbwm) -
        offsets.BEnd(cbwm);
    }
  } else if (bEndIsAuto) {
    
    if (bSizeIsAuto) {
      offsets.BEnd(cbwm) = NS_AUTOOFFSET;
    } else {
      offsets.BEnd(cbwm) = cbSize.BSize(cbwm) - margin.BStartEnd(cbwm) -
        borderPadding.BStartEnd(cbwm) - computedSize.BSize(cbwm) -
        offsets.BStart(cbwm);
    }
  } else {
    
    nscoord autoBSize = cbSize.BSize(cbwm) - margin.BStartEnd(cbwm) -
      borderPadding.BStartEnd(cbwm) - offsets.BStartEnd(cbwm);
    if (autoBSize < 0) {
      autoBSize = 0;
    }

    if (computedSize.BSize(cbwm) == NS_UNCONSTRAINEDSIZE) {
      
      
      computedSize.BSize(cbwm) = autoBSize;

      
      LogicalSize maxSize = ComputedMaxSize(cbwm);
      LogicalSize minSize = ComputedMinSize(cbwm);
      if (computedSize.BSize(cbwm) > maxSize.BSize(cbwm)) {
        computedSize.BSize(cbwm) = maxSize.BSize(cbwm);
      }
      if (computedSize.BSize(cbwm) < minSize.BSize(cbwm)) {
        computedSize.BSize(cbwm) = minSize.BSize(cbwm);
      }
    }

    
    
    
    
    nscoord availMarginSpace = autoBSize - computedSize.BSize(cbwm);
    bool marginBStartIsAuto =
      eStyleUnit_Auto == mStyleMargin->mMargin.GetBStartUnit(cbwm);
    bool marginBEndIsAuto =
      eStyleUnit_Auto == mStyleMargin->mMargin.GetBEndUnit(cbwm);

    if (marginBStartIsAuto) {
      if (marginBEndIsAuto) {
        if (availMarginSpace < 0) {
          
          margin.BEnd(cbwm) = availMarginSpace;
        } else {
          
          
          margin.BStart(cbwm) = availMarginSpace / 2;
          margin.BEnd(cbwm) = availMarginSpace - margin.BStart(cbwm);
        }
      } else {
        
        margin.BStart(cbwm) = availMarginSpace;
      }
    } else {
      if (marginBEndIsAuto) {
        
        margin.BEnd(cbwm) = availMarginSpace;
      } else {
        
        
        
        offsets.BEnd(cbwm) += availMarginSpace;
      }
    }
  }
  ComputedBSize() = computedSize.ConvertTo(wm, cbwm).BSize(wm);
  ComputedISize() = computedSize.ConvertTo(wm, cbwm).ISize(wm);

  SetComputedLogicalOffsets(offsets.ConvertTo(wm, cbwm));
  SetComputedLogicalMargin(margin.ConvertTo(wm, cbwm));
}



nscoord
GetBlockMarginBorderPadding(const nsHTMLReflowState* aReflowState)
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

      result -= GetBlockMarginBorderPadding(firstAncestorRS);
      result -= GetBlockMarginBorderPadding(secondAncestorRS);

#ifdef DEBUG
      
      if (firstAncestorRS) {
        nsIContent* frameContent = firstAncestorRS->frame->GetContent();
        if (frameContent) {
          NS_ASSERTION(frameContent->IsHTMLElement(nsGkAtoms::html),
                       "First ancestor is not HTML");
        }
      }
      if (secondAncestorRS) {
        nsIContent* frameContent = secondAncestorRS->frame->GetContent();
        if (frameContent) {
          NS_ASSERTION(frameContent->IsHTMLElement(nsGkAtoms::body),
                       "Second ancestor is not BODY");
        }
      }
#endif
      
    }
    
    else if (nsGkAtoms::blockFrame == frameType &&
             rs->parentReflowState &&
             nsGkAtoms::canvasFrame ==
               rs->parentReflowState->frame->GetType()) {
      
      result -= GetBlockMarginBorderPadding(secondAncestorRS);
    }
    break;
  }

  
  return std::max(result, 0);
}



LogicalSize
nsHTMLReflowState::ComputeContainingBlockRectangle(
                     nsPresContext*           aPresContext,
                     const nsHTMLReflowState* aContainingBlockRS)
{
  
  
  LogicalSize cbSize = aContainingBlockRS->ComputedSize();

  WritingMode wm = aContainingBlockRS->GetWritingMode();

  
  
  if (NS_FRAME_GET_TYPE(mFrameType) == NS_CSS_FRAME_TYPE_ABSOLUTE ||
      (frame->GetType() == nsGkAtoms::tableFrame &&
       frame->IsAbsolutelyPositioned() &&
       (frame->GetParent()->GetStateBits() & NS_FRAME_OUT_OF_FLOW))) {
    
    if (NS_FRAME_GET_TYPE(aContainingBlockRS->mFrameType) == NS_CSS_FRAME_TYPE_INLINE) {
      
      
      
      
      
      

      LogicalMargin computedBorder =
        aContainingBlockRS->ComputedLogicalBorderPadding() -
        aContainingBlockRS->ComputedLogicalPadding();
      cbSize.ISize(wm) = aContainingBlockRS->frame->ISize(wm) -
                         computedBorder.IStartEnd(wm);
      NS_ASSERTION(cbSize.ISize(wm) >= 0,
                   "Negative containing block width!");
      cbSize.BSize(wm) = aContainingBlockRS->frame->BSize(wm) -
                         computedBorder.BStartEnd(wm);
      NS_ASSERTION(cbSize.BSize(wm) >= 0,
                   "Negative containing block height!");
    } else {
      
      
      cbSize.ISize(wm) +=
        aContainingBlockRS->ComputedLogicalPadding().IStartEnd(wm);
      cbSize.BSize(wm) +=
        aContainingBlockRS->ComputedLogicalPadding().BStartEnd(wm);
    }
  } else {
    
    
    
    
    if (!wm.IsVertical() &&
        NS_AUTOHEIGHT == cbSize.Height(wm)) {
      if (eCompatibility_NavQuirks == aPresContext->CompatibilityMode() &&
          mStylePosition->mHeight.GetUnit() == eStyleUnit_Percent) {
        cbSize.Height(wm) = CalcQuirkContainingBlockHeight(aContainingBlockRS);
      }
    }
  }

  return cbSize.ConvertTo(GetWritingMode(), wm);
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
IsSideCaption(nsIFrame* aFrame, const nsStyleDisplay* aStyleDisplay,
              WritingMode aWM)
{
  if (aStyleDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE_CAPTION) {
    return false;
  }
  uint8_t captionSide = aFrame->StyleTableBorder()->LogicalCaptionSide(aWM);
  return captionSide == NS_STYLE_CAPTION_SIDE_ISTART ||
         captionSide == NS_STYLE_CAPTION_SIDE_IEND;
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






static LogicalSize
OffsetPercentBasis(const nsIFrame*    aFrame,
                   WritingMode        aWM,
                   const LogicalSize& aContainingBlockSize)
{
  LogicalSize offsetPercentBasis = aContainingBlockSize;
  if (!aFrame->IsFlexOrGridItem()) {
    offsetPercentBasis.BSize(aWM) = offsetPercentBasis.ISize(aWM);
  } else if (offsetPercentBasis.BSize(aWM) == NS_AUTOHEIGHT) {
    offsetPercentBasis.BSize(aWM) = 0;
  }

  return offsetPercentBasis;
}




void
nsHTMLReflowState::InitConstraints(nsPresContext*     aPresContext,
                                   const LogicalSize& aContainingBlockSize,
                                   const nsMargin*    aBorder,
                                   const nsMargin*    aPadding,
                                   nsIAtom*           aFrameType)
{
  WritingMode wm = GetWritingMode();
  DISPLAY_INIT_CONSTRAINTS(frame, this,
                           aContainingBlockSize.ISize(wm),
                           aContainingBlockSize.BSize(wm),
                           aBorder, aPadding);

  
  
  if (nullptr == parentReflowState || mFlags.mDummyParentReflowState) {
    
    InitOffsets(wm, OffsetPercentBasis(frame, wm, aContainingBlockSize),
                aFrameType, aBorder, aPadding);
    
    
    ComputedPhysicalMargin().SizeTo(0, 0, 0, 0);
    ComputedPhysicalOffsets().SizeTo(0, 0, 0, 0);

    ComputedISize() =
      AvailableISize() - ComputedLogicalBorderPadding().IStartEnd(wm);
    if (ComputedISize() < 0) {
      ComputedISize() = 0;
    }
    if (AvailableBSize() != NS_UNCONSTRAINEDSIZE) {
      ComputedBSize() =
        AvailableBSize() - ComputedLogicalBorderPadding().BStartEnd(wm);
      if (ComputedBSize() < 0) {
        ComputedBSize() = 0;
      }
    } else {
      ComputedBSize() = NS_UNCONSTRAINEDSIZE;
    }

    ComputedMinWidth() = ComputedMinHeight() = 0;
    ComputedMaxWidth() = ComputedMaxHeight() = NS_UNCONSTRAINEDSIZE;
  } else {
    
    const nsHTMLReflowState* cbrs = mCBReflowState;
    NS_ASSERTION(nullptr != cbrs, "no containing block");

    
    
    LogicalSize cbSize = (aContainingBlockSize == LogicalSize(wm, -1, -1))
      ? ComputeContainingBlockRectangle(aPresContext, cbrs)
      : aContainingBlockSize;

    
    
    nsIAtom* fType;
    if (NS_AUTOHEIGHT == cbSize.BSize(wm)) {
      
      
      
      if (cbrs->parentReflowState) {
        fType = cbrs->frame->GetType();
        if (IS_TABLE_CELL(fType)) {
          
          cbSize.BSize(wm) = cbrs->ComputedSize(wm).BSize(wm);
        }
      }
    }

    
    

    
    
    WritingMode cbwm = cbrs->GetWritingMode();
    InitOffsets(cbwm, OffsetPercentBasis(frame, cbwm,
                                         cbSize.ConvertTo(cbwm, wm)),
                aFrameType, aBorder, aPadding);

    
    const nsStyleCoord &blockSize = mStylePosition->BSize(wm);
    nsStyleUnit blockSizeUnit = blockSize.GetUnit();

    
    
    
    
    if (blockSize.HasPercent()) {
      if (NS_AUTOHEIGHT == cbSize.BSize(wm)) {
        
        
        
        if (NS_FRAME_REPLACED(NS_CSS_FRAME_TYPE_INLINE) == mFrameType ||
            NS_FRAME_REPLACED_CONTAINS_BLOCK(
                NS_CSS_FRAME_TYPE_INLINE) == mFrameType) {
          
          NS_ASSERTION(nullptr != cbrs, "no containing block");
          
          if (!wm.IsVertical() &&
              eCompatibility_NavQuirks == aPresContext->CompatibilityMode()) {
            if (!IS_TABLE_CELL(fType)) {
              cbSize.BSize(wm) = CalcQuirkContainingBlockHeight(cbrs);
              if (cbSize.BSize(wm) == NS_AUTOHEIGHT) {
                blockSizeUnit = eStyleUnit_Auto;
              }
            }
            else {
              blockSizeUnit = eStyleUnit_Auto;
            }
          }
          
          
          
          else
          {
            nscoord computedBSize = cbrs->ComputedSize(wm).BSize(wm);
            if (NS_AUTOHEIGHT != computedBSize) {
              cbSize.BSize(wm) = computedBSize;
            }
            else {
              blockSizeUnit = eStyleUnit_Auto;
            }
          }
        }
        else {
          
          blockSizeUnit = eStyleUnit_Auto;
        }
      }
    }

    
    
    
    
    
    
    if (mStyleDisplay->IsRelativelyPositioned(frame) &&
        NS_STYLE_POSITION_RELATIVE == mStyleDisplay->mPosition) {
      ComputeRelativeOffsets(cbwm, frame, cbSize.ConvertTo(cbwm, wm),
                             ComputedPhysicalOffsets());
    } else {
      
      ComputedPhysicalOffsets().SizeTo(0, 0, 0, 0);
    }

    
    
    ComputeMinMaxValues(cbSize);

    

    if (NS_CSS_FRAME_TYPE_INTERNAL_TABLE == mFrameType) {
      
      
      bool rowOrRowGroup = false;
      const nsStyleCoord &inlineSize = mStylePosition->ISize(wm);
      nsStyleUnit inlineSizeUnit = inlineSize.GetUnit();
      if ((NS_STYLE_DISPLAY_TABLE_ROW == mStyleDisplay->mDisplay) ||
          (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == mStyleDisplay->mDisplay)) {
        
        inlineSizeUnit = eStyleUnit_Auto;
        rowOrRowGroup = true;
      }

      
      if (eStyleUnit_Auto == inlineSizeUnit ||
          (inlineSize.IsCalcUnit() && inlineSize.CalcHasPercent())) {
        ComputedISize() = AvailableISize();

        if ((ComputedISize() != NS_UNCONSTRAINEDSIZE) && !rowOrRowGroup){
          
          
          ComputedISize() -= ComputedLogicalBorderPadding().IStartEnd(wm);
          if (ComputedISize() < 0)
            ComputedISize() = 0;
        }
        NS_ASSERTION(ComputedISize() >= 0, "Bogus computed width");

      } else {
        NS_ASSERTION(inlineSizeUnit == inlineSize.GetUnit(),
                     "unexpected width unit change");
        ComputedISize() = ComputeISizeValue(cbSize.ISize(wm),
                                            mStylePosition->mBoxSizing,
                                            inlineSize);
      }

      
      if ((NS_STYLE_DISPLAY_TABLE_COLUMN == mStyleDisplay->mDisplay) ||
          (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == mStyleDisplay->mDisplay)) {
        
        blockSizeUnit = eStyleUnit_Auto;
      }
      
      if (eStyleUnit_Auto == blockSizeUnit ||
          (blockSize.IsCalcUnit() && blockSize.CalcHasPercent())) {
        ComputedBSize() = NS_AUTOHEIGHT;
      } else {
        NS_ASSERTION(blockSizeUnit == blockSize.GetUnit(),
                     "unexpected block size unit change");
        ComputedBSize() = ComputeBSizeValue(cbSize.BSize(wm),
                                            mStylePosition->mBoxSizing,
                                            blockSize);
      }

      
      ComputedMinWidth() = ComputedMinHeight() = 0;
      ComputedMaxWidth() = ComputedMaxHeight() = NS_UNCONSTRAINEDSIZE;

    } else if (NS_FRAME_GET_TYPE(mFrameType) == NS_CSS_FRAME_TYPE_ABSOLUTE) {
      
      InitAbsoluteConstraints(aPresContext, cbrs, cbSize.ConvertTo(cbrs->GetWritingMode(), wm), aFrameType);
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

      if (cbSize.ISize(wm) == NS_UNCONSTRAINEDSIZE) {
        
        
        cbSize.ISize(wm) = AvailableISize();
      }

      LogicalSize size =
        frame->ComputeSize(rendContext, wm, cbSize, AvailableISize(),
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
          !IsSideCaption(frame, mStyleDisplay, cbwm) &&
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
nsCSSOffsetState::InitOffsets(WritingMode aWM,
                              const LogicalSize& aPercentBasis,
                              nsIAtom* aFrameType,
                              const nsMargin *aBorder,
                              const nsMargin *aPadding)
{
  DISPLAY_INIT_OFFSETS(frame, this, aPercentBasis, aBorder, aPadding);

  
  
  nsPresContext *presContext = frame->PresContext();
  FrameProperties props(presContext->PropertyTable(), frame);
  props.Delete(nsIFrame::UsedBorderProperty());

  
  
  
  
  bool needMarginProp = ComputeMargin(aWM, aPercentBasis);
  
  
  
  
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
    needPaddingProp = ComputePadding(aWM, aPercentBasis, aFrameType);
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
      SetComputedLogicalBorderPadding(
        tableFrame->GetIncludedOuterBCBorder(mWritingMode));
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

  LAYOUT_WARN_IF_FALSE(NS_UNCONSTRAINEDSIZE != computedISizeCBWM &&
                       NS_UNCONSTRAINEDSIZE != availISizeCBWM,
                       "have unconstrained inline-size; this should only "
                       "result from very large sizes, not attempts at "
                       "intrinsic inline-size calculation");

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

  
  
  const nsStyleSides& styleSides = mStyleMargin->mMargin;
  bool isAutoStartMargin = eStyleUnit_Auto == styleSides.GetIStartUnit(cbWM);
  bool isAutoEndMargin = eStyleUnit_Auto == styleSides.GetIEndUnit(cbWM);
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
nsCSSOffsetState::ComputeMargin(WritingMode aWM,
                                const LogicalSize& aPercentBasis)
{
  
  if (frame->IsSVGText()) {
    return false;
  }

  
  const nsStyleMargin *styleMargin = frame->StyleMargin();

  bool isCBDependent = !styleMargin->GetMargin(ComputedPhysicalMargin());
  if (isCBDependent) {
    
    
    
    LogicalMargin m(aWM);
    m.IStart(aWM) = nsLayoutUtils::
      ComputeCBDependentValue(aPercentBasis.ISize(aWM),
                              styleMargin->mMargin.GetIStart(aWM));
    m.IEnd(aWM) = nsLayoutUtils::
      ComputeCBDependentValue(aPercentBasis.ISize(aWM),
                              styleMargin->mMargin.GetIEnd(aWM));

    m.BStart(aWM) = nsLayoutUtils::
      ComputeCBDependentValue(aPercentBasis.BSize(aWM),
                              styleMargin->mMargin.GetBStart(aWM));
    m.BEnd(aWM) = nsLayoutUtils::
      ComputeCBDependentValue(aPercentBasis.BSize(aWM),
                              styleMargin->mMargin.GetBEnd(aWM));

    SetComputedLogicalMargin(aWM, m);
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
nsCSSOffsetState::ComputePadding(WritingMode aWM,
                                 const LogicalSize& aPercentBasis,
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
    
    
    
    
    LogicalMargin p(aWM);
    p.IStart(aWM) = std::max(0, nsLayoutUtils::
      ComputeCBDependentValue(aPercentBasis.ISize(aWM),
                              stylePadding->mPadding.GetIStart(aWM)));
    p.IEnd(aWM) = std::max(0, nsLayoutUtils::
      ComputeCBDependentValue(aPercentBasis.ISize(aWM),
                              stylePadding->mPadding.GetIEnd(aWM)));

    p.BStart(aWM) = std::max(0, nsLayoutUtils::
      ComputeCBDependentValue(aPercentBasis.BSize(aWM),
                              stylePadding->mPadding.GetBStart(aWM)));
    p.BEnd(aWM) = std::max(0, nsLayoutUtils::
      ComputeCBDependentValue(aPercentBasis.BSize(aWM),
                              stylePadding->mPadding.GetBEnd(aWM)));

    SetComputedLogicalPadding(aWM, p);
  }
  return isCBDependent;
}

void
nsHTMLReflowState::ComputeMinMaxValues(const LogicalSize&aCBSize)
{
  WritingMode wm = GetWritingMode();

  const nsStyleCoord& minISize = mStylePosition->MinISize(wm);
  const nsStyleCoord& maxISize = mStylePosition->MaxISize(wm);
  const nsStyleCoord& minBSize = mStylePosition->MinBSize(wm);
  const nsStyleCoord& maxBSize = mStylePosition->MaxBSize(wm);

  
  
  
  if (eStyleUnit_Auto == minISize.GetUnit()) {
    ComputedMinISize() = 0;
  } else {
    ComputedMinISize() = ComputeISizeValue(aCBSize.ISize(wm),
                                           mStylePosition->mBoxSizing,
                                           minISize);
  }

  if (eStyleUnit_None == maxISize.GetUnit()) {
    
    ComputedMaxISize() = NS_UNCONSTRAINEDSIZE;  
  } else {
    ComputedMaxISize() = ComputeISizeValue(aCBSize.ISize(wm),
                                           mStylePosition->mBoxSizing,
                                           maxISize);
  }

  
  
  if (ComputedMinISize() > ComputedMaxISize()) {
    ComputedMaxISize() = ComputedMinISize();
  }

  
  
  
  
  
  

  
  
  
  if (eStyleUnit_Auto == minBSize.GetUnit() ||
      (NS_AUTOHEIGHT == aCBSize.BSize(wm) &&
       minBSize.HasPercent()) ||
      (mFrameType == NS_CSS_FRAME_TYPE_INTERNAL_TABLE &&
       minBSize.IsCalcUnit() && minBSize.CalcHasPercent()) ||
      mFlags.mIsFlexContainerMeasuringHeight) {
    ComputedMinBSize() = 0;
  } else {
    ComputedMinBSize() = ComputeBSizeValue(aCBSize.BSize(wm),
                                           mStylePosition->mBoxSizing,
                                           minBSize);
  }
  nsStyleUnit maxBSizeUnit = maxBSize.GetUnit();
  if (eStyleUnit_None == maxBSizeUnit) {
    
    ComputedMaxBSize() = NS_UNCONSTRAINEDSIZE;  
  } else {
    
    
    
    
    
    
    if ((NS_AUTOHEIGHT == aCBSize.BSize(wm) &&
         maxBSize.HasPercent()) ||
        (mFrameType == NS_CSS_FRAME_TYPE_INTERNAL_TABLE &&
         maxBSize.IsCalcUnit() && maxBSize.CalcHasPercent()) ||
        mFlags.mIsFlexContainerMeasuringHeight) {
      ComputedMaxBSize() = NS_UNCONSTRAINEDSIZE;
    } else {
      ComputedMaxBSize() = ComputeBSizeValue(aCBSize.BSize(wm),
                                             mStylePosition->mBoxSizing,
                                             maxBSize);
    }
  }

  
  
  if (ComputedMinBSize() > ComputedMaxBSize()) {
    ComputedMaxBSize() = ComputedMinBSize();
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
