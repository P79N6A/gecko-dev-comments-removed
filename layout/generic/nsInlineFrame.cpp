






#include "nsInlineFrame.h"
#include "nsCOMPtr.h"
#include "nsLineLayout.h"
#include "nsBlockFrame.h"
#include "nsPlaceholderFrame.h"
#include "nsGkAtoms.h"
#include "nsHTMLParts.h"
#include "nsStyleContext.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsCSSAnonBoxes.h"
#include "nsAutoPtr.h"
#include "nsFrameManager.h"
#include "nsDisplayList.h"
#include "mozilla/Likely.h"

#ifdef DEBUG
#undef NOISY_PUSHING
#endif

using namespace mozilla;






nsIFrame*
NS_NewInlineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsInlineFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsInlineFrame)

NS_QUERYFRAME_HEAD(nsInlineFrame)
  NS_QUERYFRAME_ENTRY(nsInlineFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

#ifdef DEBUG
NS_IMETHODIMP
nsInlineFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Inline"), aResult);
}
#endif

nsIAtom*
nsInlineFrame::GetType() const
{
  return nsGkAtoms::inlineFrame;
}

void
nsInlineFrame::InvalidateFrame(uint32_t aDisplayItemKey)
{
  if (IsSVGText()) {
    nsIFrame* svgTextFrame =
      nsLayoutUtils::GetClosestFrameOfType(GetParent(),
                                           nsGkAtoms::svgTextFrame2);
    svgTextFrame->InvalidateFrame();
    return;
  }
  nsInlineFrameBase::InvalidateFrame(aDisplayItemKey);
}

void
nsInlineFrame::InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey)
{
  if (IsSVGText()) {
    nsIFrame* svgTextFrame =
      nsLayoutUtils::GetClosestFrameOfType(GetParent(),
                                           nsGkAtoms::svgTextFrame2);
    svgTextFrame->InvalidateFrame();
    return;
  }
  nsInlineFrameBase::InvalidateFrameWithRect(aRect, aDisplayItemKey);
}

static inline bool
IsMarginZero(const nsStyleCoord &aCoord)
{
  return aCoord.GetUnit() == eStyleUnit_Auto ||
         nsLayoutUtils::IsMarginZero(aCoord);
}

 bool
nsInlineFrame::IsSelfEmpty()
{
#if 0
  
  
  if (GetPresContext()->CompatibilityMode() == eCompatibility_FullStandards) {
    return false;
  }
#endif
  const nsStyleMargin* margin = GetStyleMargin();
  const nsStyleBorder* border = GetStyleBorder();
  const nsStylePadding* padding = GetStylePadding();
  
  
  
  bool haveRight =
    border->GetComputedBorderWidth(NS_SIDE_RIGHT) != 0 ||
    !nsLayoutUtils::IsPaddingZero(padding->mPadding.GetRight()) ||
    !IsMarginZero(margin->mMargin.GetRight());
  bool haveLeft =
    border->GetComputedBorderWidth(NS_SIDE_LEFT) != 0 ||
    !nsLayoutUtils::IsPaddingZero(padding->mPadding.GetLeft()) ||
    !IsMarginZero(margin->mMargin.GetLeft());
  if (haveLeft || haveRight) {
    if (GetStateBits() & NS_FRAME_IS_SPECIAL) {
      bool haveStart, haveEnd;
      if (NS_STYLE_DIRECTION_LTR == GetStyleVisibility()->mDirection) {
        haveStart = haveLeft;
        haveEnd = haveRight;
      } else {
        haveStart = haveRight;
        haveEnd = haveLeft;
      }
      
      
      

      
      
      nsIFrame* firstCont = GetFirstContinuation();
      return
        (!haveStart || nsLayoutUtils::FrameIsNonFirstInIBSplit(firstCont)) &&
        (!haveEnd || nsLayoutUtils::FrameIsNonLastInIBSplit(firstCont));
    }
    return false;
  }
  return true;
}

bool
nsInlineFrame::IsEmpty()
{
  if (!IsSelfEmpty()) {
    return false;
  }

  for (nsIFrame *kid = mFrames.FirstChild(); kid; kid = kid->GetNextSibling()) {
    if (!kid->IsEmpty())
      return false;
  }

  return true;
}

bool
nsInlineFrame::PeekOffsetCharacter(bool aForward, int32_t* aOffset,
                                   bool aRespectClusters)
{
  
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  int32_t startOffset = *aOffset;
  if (startOffset < 0)
    startOffset = 1;
  if (aForward == (startOffset == 0)) {
    
    
    *aOffset = 1 - startOffset;
  }
  return false;
}

NS_IMETHODIMP
nsInlineFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists)
{
  nsresult rv = BuildDisplayListForInline(aBuilder, aDirtyRect, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  if (!mFrames.FirstChild()) {
    rv = DisplaySelectionOverlay(aBuilder, aLists.Content());
  }
  return rv;
}




 void
nsInlineFrame::AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                                 nsIFrame::InlineMinWidthData *aData)
{
  DoInlineIntrinsicWidth(aRenderingContext, aData, nsLayoutUtils::MIN_WIDTH);
}

 void
nsInlineFrame::AddInlinePrefWidth(nsRenderingContext *aRenderingContext,
                                  nsIFrame::InlinePrefWidthData *aData)
{
  DoInlineIntrinsicWidth(aRenderingContext, aData, nsLayoutUtils::PREF_WIDTH);
}

 nsSize
nsInlineFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                           nsSize aCBSize, nscoord aAvailableWidth,
                           nsSize aMargin, nsSize aBorder, nsSize aPadding,
                           uint32_t aFlags)
{
  
  return nsSize(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
}

nsRect
nsInlineFrame::ComputeTightBounds(gfxContext* aContext) const
{
  
  if (StyleContext()->HasTextDecorationLines()) {
    return GetVisualOverflowRect();
  }
  return ComputeSimpleTightBounds(aContext);
}

void
nsInlineFrame::ReparentFloatsForInlineChild(nsIFrame* aOurLineContainer,
                                            nsIFrame* aFrame,
                                            bool aReparentSiblings)
{
  
  
  NS_ASSERTION(aOurLineContainer->GetNextContinuation() ||
               aOurLineContainer->GetPrevContinuation(),
               "Don't call this when we have no continuation, it's a waste");
  if (!aFrame) {
    NS_ASSERTION(aReparentSiblings, "Why did we get called?");
    return;
  }

  nsIFrame* ancestor = aFrame;
  nsIFrame* ancestorBlockChild;
  do {
    ancestorBlockChild = ancestor;
    ancestor = ancestor->GetParent();
    if (!ancestor)
      return;
  } while (!ancestor->IsFloatContainingBlock());

  if (ancestor == aOurLineContainer)
    return;

  nsBlockFrame* ourBlock = nsLayoutUtils::GetAsBlock(aOurLineContainer);
  NS_ASSERTION(ourBlock, "Not a block, but broke vertically?");
  nsBlockFrame* frameBlock = nsLayoutUtils::GetAsBlock(ancestor);
  NS_ASSERTION(frameBlock, "ancestor not a block");

  const nsFrameList& blockChildren(ancestor->PrincipalChildList());
  bool isOverflow = !blockChildren.ContainsFrame(ancestorBlockChild);

  while (true) {
    ourBlock->ReparentFloats(aFrame, frameBlock, isOverflow, false);

    if (!aReparentSiblings)
      return;
    nsIFrame* next = aFrame->GetNextSibling();
    if (!next)
      return;
    if (next->GetParent() == aFrame->GetParent()) {
      aFrame = next;
      continue;
    }
    
    
    
    
    ReparentFloatsForInlineChild(aOurLineContainer, next, aReparentSiblings);
    return;
  }
}

static void
ReparentChildListStyle(nsPresContext* aPresContext,
                       const nsFrameList::Slice& aFrames,
                       nsIFrame* aParentFrame)
{
  nsFrameManager *frameManager = aPresContext->FrameManager();

  for (nsFrameList::Enumerator e(aFrames); !e.AtEnd(); e.Next()) {
    NS_ASSERTION(e.get()->GetParent() == aParentFrame, "Bogus parentage");
    frameManager->ReparentStyleContext(e.get());
  }
}

NS_IMETHODIMP
nsInlineFrame::Reflow(nsPresContext*          aPresContext,
                      nsHTMLReflowMetrics&     aMetrics,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsInlineFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);
  if (nullptr == aReflowState.mLineLayout) {
    return NS_ERROR_INVALID_ARG;
  }
  if (IsFrameTreeTooDeep(aReflowState, aMetrics, aStatus)) {
    return NS_OK;
  }

  bool    lazilySetParentPointer = false;

  nsIFrame* lineContainer = aReflowState.mLineLayout->GetLineContainerFrame();

   
  nsInlineFrame* prevInFlow = (nsInlineFrame*)GetPrevInFlow();
  if (nullptr != prevInFlow) {
    nsAutoPtr<nsFrameList> prevOverflowFrames(prevInFlow->StealOverflowFrames());

    if (prevOverflowFrames) {
      
      
      nsContainerFrame::ReparentFrameViewList(aPresContext,
                                              *prevOverflowFrames,
                                              prevInFlow, this);

      
      
      
      
      
      if ((GetStateBits() & NS_FRAME_FIRST_REFLOW) && mFrames.IsEmpty() &&
          !GetNextInFlow()) {
        
        
        
        
        mFrames.SetFrames(*prevOverflowFrames);
        lazilySetParentPointer = true;
      } else {
        
        if (lineContainer && lineContainer->GetPrevContinuation()) {
          ReparentFloatsForInlineChild(lineContainer,
                                       prevOverflowFrames->FirstChild(),
                                       true);
        }
        
        
        const nsFrameList::Slice& newFrames =
          mFrames.InsertFrames(this, nullptr, *prevOverflowFrames);
        
        
        
        
        
        if (aReflowState.mLineLayout->GetInFirstLine()) {
          ReparentChildListStyle(aPresContext, newFrames, this);
        }
      }
    }
  }

  
#ifdef DEBUG
  if (GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    
    
    
    nsFrameList* overflowFrames = GetOverflowFrames();
    NS_ASSERTION(!overflowFrames || overflowFrames->IsEmpty(),
                 "overflow list is not empty for initial reflow");
  }
#endif
  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    nsAutoPtr<nsFrameList> overflowFrames(StealOverflowFrames());
    if (overflowFrames) {
      NS_ASSERTION(mFrames.NotEmpty(), "overflow list w/o frames");
      if (!lazilySetParentPointer) {
        
        
        
        
        nsIFrame* firstChild = overflowFrames->FirstChild();
        if (lineContainer && lineContainer->GetPrevContinuation()) {
          ReparentFloatsForInlineChild(lineContainer, firstChild, true);
        }
        const bool inFirstLine = aReflowState.mLineLayout->GetInFirstLine();
        nsFrameManager* fm = PresContext()->FrameManager();
        for (nsIFrame* f = firstChild; f; f = f->GetNextSibling()) {
          f->SetParent(this);
          if (inFirstLine) {
            fm->ReparentStyleContext(f);
          }
        }
      }
      mFrames.AppendFrames(nullptr, *overflowFrames);
    }
  }

  
  
  InlineReflowState irs;
  irs.mPrevFrame = nullptr;
  irs.mLineContainer = lineContainer;
  irs.mLineLayout = aReflowState.mLineLayout;
  irs.mNextInFlow = (nsInlineFrame*) GetNextInFlow();
  irs.mSetParentPointer = lazilySetParentPointer;

  nsresult rv;
  if (mFrames.IsEmpty()) {
    
    
    bool complete;
    (void) PullOneFrame(aPresContext, irs, &complete);
  }

  rv = ReflowFrames(aPresContext, aReflowState, irs, aMetrics, aStatus);

  ReflowAbsoluteFrames(aPresContext, aMetrics, aReflowState, aStatus);

  
  

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
  return rv;
}

 bool
nsInlineFrame::CanContinueTextRun() const
{
  
  return true;
}

 void
nsInlineFrame::PullOverflowsFromPrevInFlow()
{
  nsInlineFrame* prevInFlow = static_cast<nsInlineFrame*>(GetPrevInFlow());
  if (prevInFlow) {
    nsAutoPtr<nsFrameList> prevOverflowFrames(prevInFlow->StealOverflowFrames());
    if (prevOverflowFrames) {
      
      nsContainerFrame::ReparentFrameViewList(PresContext(),
                                              *prevOverflowFrames,
                                              prevInFlow, this);
      mFrames.InsertFrames(this, nullptr, *prevOverflowFrames);
    }
  }
}

nsresult
nsInlineFrame::ReflowFrames(nsPresContext* aPresContext,
                            const nsHTMLReflowState& aReflowState,
                            InlineReflowState& irs,
                            nsHTMLReflowMetrics& aMetrics,
                            nsReflowStatus& aStatus)
{
  nsresult rv = NS_OK;
  aStatus = NS_FRAME_COMPLETE;

  nsLineLayout* lineLayout = aReflowState.mLineLayout;
  bool inFirstLine = aReflowState.mLineLayout->GetInFirstLine();
  nsFrameManager* frameManager = aPresContext->FrameManager();
  bool ltr = (NS_STYLE_DIRECTION_LTR == aReflowState.mStyleVisibility->mDirection);
  nscoord leftEdge = 0;
  
  
  if (!GetPrevContinuation() &&
      !nsLayoutUtils::FrameIsNonFirstInIBSplit(this)) {
    leftEdge = ltr ? aReflowState.mComputedBorderPadding.left
                   : aReflowState.mComputedBorderPadding.right;
  }
  nscoord availableWidth = aReflowState.availableWidth;
  NS_ASSERTION(availableWidth != NS_UNCONSTRAINEDSIZE,
               "should no longer use available widths");
  
  availableWidth -= leftEdge;
  availableWidth -= ltr ? aReflowState.mComputedBorderPadding.right
                        : aReflowState.mComputedBorderPadding.left;
  lineLayout->BeginSpan(this, &aReflowState, leftEdge,
                        leftEdge + availableWidth, &mBaseline);

  
  nsIFrame* frame = mFrames.FirstChild();
  bool done = false;
  while (frame) {
    
    if (irs.mSetParentPointer) {
      bool havePrevBlock =
        irs.mLineContainer && irs.mLineContainer->GetPrevContinuation();
      nsIFrame* child = frame;
      do {
        
        
        if (havePrevBlock) {
          
          
          
          
          
          
          
          
          ReparentFloatsForInlineChild(irs.mLineContainer, child, false);
        }
        child->SetParent(this);
        if (inFirstLine) {
          frameManager->ReparentStyleContext(child);
        }
        
        
        
        
        nsIFrame* nextSibling = child->GetNextSibling();
        child = child->GetNextInFlow();
        if (MOZ_UNLIKELY(child)) {
          while (child != nextSibling && nextSibling) {
            nextSibling = nextSibling->GetNextSibling();
          }
          if (!nextSibling) {
            child = nullptr;
          }
        }
        MOZ_ASSERT(!child || mFrames.ContainsFrame(child));
      } while (child);

      
      
      nsIFrame* realFrame = nsPlaceholderFrame::GetRealFrameFor(frame);
      if (realFrame->GetType() == nsGkAtoms::letterFrame) {
        nsIFrame* child = realFrame->GetFirstPrincipalChild();
        if (child) {
          NS_ASSERTION(child->GetType() == nsGkAtoms::textFrame,
                       "unexpected frame type");
          nsIFrame* nextInFlow = child->GetNextInFlow();
          for ( ; nextInFlow; nextInFlow = nextInFlow->GetNextInFlow()) {
            NS_ASSERTION(nextInFlow->GetType() == nsGkAtoms::textFrame,
                         "unexpected frame type");
            if (mFrames.ContainsFrame(nextInFlow)) {
              nextInFlow->SetParent(this);
              if (inFirstLine) {
                frameManager->ReparentStyleContext(nextInFlow);
              }
            }
            else {
#ifdef DEBUG              
              
              
              for ( ; nextInFlow; nextInFlow = nextInFlow->GetNextInFlow()) {
                NS_ASSERTION(!mFrames.ContainsFrame(nextInFlow),
                             "unexpected letter frame flow");
              }
#endif
              break;
            }
          }
        }
      }
    }
    MOZ_ASSERT(frame->GetParent() == this);

    if (!done) {
      bool reflowingFirstLetter = lineLayout->GetFirstLetterStyleOK();
      rv = ReflowInlineFrame(aPresContext, aReflowState, irs, frame, aStatus);
      done = NS_FAILED(rv) ||
             NS_INLINE_IS_BREAK(aStatus) || 
             (!reflowingFirstLetter && NS_FRAME_IS_NOT_COMPLETE(aStatus));
      if (done) {
        if (!irs.mSetParentPointer) {
          break;
        }
        
        nsFrameList* pushedFrames = GetOverflowFrames();
        if (pushedFrames && pushedFrames->FirstChild() == frame) {
          
          break;
        }
      } else {
        irs.mPrevFrame = frame;
      }
    }
    frame = frame->GetNextSibling();
  }

  
  if (!done && GetNextInFlow()) {
    while (true) {
      bool reflowingFirstLetter = lineLayout->GetFirstLetterStyleOK();
      bool isComplete;
      if (!frame) { 
                    
        frame = PullOneFrame(aPresContext, irs, &isComplete);
      }
#ifdef NOISY_PUSHING
      printf("%p pulled up %p\n", this, frame);
#endif
      if (nullptr == frame) {
        if (!isComplete) {
          aStatus = NS_FRAME_NOT_COMPLETE;
        }
        break;
      }
      rv = ReflowInlineFrame(aPresContext, aReflowState, irs, frame, aStatus);
      if (NS_FAILED(rv) ||
          NS_INLINE_IS_BREAK(aStatus) || 
          (!reflowingFirstLetter && NS_FRAME_IS_NOT_COMPLETE(aStatus))) {
        break;
      }
      irs.mPrevFrame = frame;
      frame = frame->GetNextSibling();
    }
  }

  NS_ASSERTION(!NS_FRAME_IS_COMPLETE(aStatus) || !GetOverflowFrames(),
               "We can't be complete AND have overflow frames!");

  
  
  
  
  
  
  
  aMetrics.width = lineLayout->EndSpan(this);

  

  
  
  
  if (!GetPrevContinuation() &&
      !nsLayoutUtils::FrameIsNonFirstInIBSplit(this)) {
    aMetrics.width += ltr ? aReflowState.mComputedBorderPadding.left
                          : aReflowState.mComputedBorderPadding.right;
  }

  






  if (NS_FRAME_IS_COMPLETE(aStatus) &&
      !GetLastInFlow()->GetNextContinuation() &&
      !nsLayoutUtils::FrameIsNonLastInIBSplit(this)) {
    aMetrics.width += ltr ? aReflowState.mComputedBorderPadding.right
                          : aReflowState.mComputedBorderPadding.left;
  }

  nsRefPtr<nsFontMetrics> fm;
  float inflation = nsLayoutUtils::FontSizeInflationFor(this);
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm), inflation);
  aReflowState.rendContext->SetFont(fm);

  if (fm) {
    
    
    
    
    
    
    
    
    
    
    aMetrics.ascent = fm->MaxAscent();
    aMetrics.height = fm->MaxHeight();
  } else {
    NS_WARNING("Cannot get font metrics - defaulting sizes to 0");
    aMetrics.ascent = aMetrics.height = 0;
  }
  aMetrics.ascent += aReflowState.mComputedBorderPadding.top;
  aMetrics.height += aReflowState.mComputedBorderPadding.top +
    aReflowState.mComputedBorderPadding.bottom;

  
  
  aMetrics.mOverflowAreas.Clear();

#ifdef NOISY_FINAL_SIZE
  ListTag(stdout);
  printf(": metrics=%d,%d ascent=%d\n",
         aMetrics.width, aMetrics.height, aMetrics.ascent);
#endif

  return rv;
}

nsresult
nsInlineFrame::ReflowInlineFrame(nsPresContext* aPresContext,
                                 const nsHTMLReflowState& aReflowState,
                                 InlineReflowState& irs,
                                 nsIFrame* aFrame,
                                 nsReflowStatus& aStatus)
{
  nsLineLayout* lineLayout = aReflowState.mLineLayout;
  bool reflowingFirstLetter = lineLayout->GetFirstLetterStyleOK();
  bool pushedFrame;
  nsresult rv =
    lineLayout->ReflowFrame(aFrame, aStatus, nullptr, pushedFrame);
  
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (NS_INLINE_IS_BREAK_BEFORE(aStatus)) {
    if (aFrame != mFrames.FirstChild()) {
      
      
      
      aStatus = NS_FRAME_NOT_COMPLETE |
        NS_INLINE_BREAK | NS_INLINE_BREAK_AFTER |
        (aStatus & NS_INLINE_BREAK_TYPE_MASK);
      PushFrames(aPresContext, aFrame, irs.mPrevFrame, irs);
    }
    else {
      
      
    }
    return NS_OK;
  }

  
  if (!NS_FRAME_IS_FULLY_COMPLETE(aStatus)) {
    nsIFrame* newFrame;
    rv = CreateNextInFlow(aPresContext, aFrame, newFrame);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  if (NS_INLINE_IS_BREAK_AFTER(aStatus)) {
    nsIFrame* nextFrame = aFrame->GetNextSibling();
    if (nextFrame) {
      NS_FRAME_SET_INCOMPLETE(aStatus);
      PushFrames(aPresContext, nextFrame, aFrame, irs);
    }
    else {
      
      
      nsInlineFrame* nextInFlow = static_cast<nsInlineFrame*>(GetNextInFlow());
      while (nextInFlow) {
        if (nextInFlow->mFrames.NotEmpty()) {
          NS_FRAME_SET_INCOMPLETE(aStatus);
          break;
        }
        nextInFlow = static_cast<nsInlineFrame*>(nextInFlow->GetNextInFlow());
      }
    }
    return NS_OK;
  }

  if (!NS_FRAME_IS_FULLY_COMPLETE(aStatus) && !reflowingFirstLetter) {
    nsIFrame* nextFrame = aFrame->GetNextSibling();
    if (nextFrame) {
      PushFrames(aPresContext, nextFrame, aFrame, irs);
    }
  }
  return NS_OK;
}

nsIFrame*
nsInlineFrame::PullOneFrame(nsPresContext* aPresContext,
                            InlineReflowState& irs,
                            bool* aIsComplete)
{
  bool isComplete = true;

  nsIFrame* frame = nullptr;
  nsInlineFrame* nextInFlow = irs.mNextInFlow;
  while (nullptr != nextInFlow) {
    frame = nextInFlow->mFrames.FirstChild();
    if (!frame) {
      
      nsFrameList* overflowFrames = nextInFlow->GetOverflowFrames();
      if (overflowFrames) {
        frame = overflowFrames->FirstChild();
        if (!frame->GetNextSibling()) {
          
          delete nextInFlow->StealOverflowFrames();
        } else {
          
          
          
          overflowFrames->RemoveFirstChild();
        }
        
        
        nextInFlow->mFrames.SetFrames(frame);
      }
    }

    if (nullptr != frame) {
      
      
      
      if (irs.mLineContainer && irs.mLineContainer->GetNextContinuation()) {
        
        
        
        ReparentFloatsForInlineChild(irs.mLineContainer, frame, false);
      }
      nextInFlow->mFrames.RemoveFirstChild();
      

      mFrames.InsertFrame(this, irs.mPrevFrame, frame);
      isComplete = false;
      if (irs.mLineLayout) {
        irs.mLineLayout->SetDirtyNextLine();
      }
      nsContainerFrame::ReparentFrameView(aPresContext, frame, nextInFlow, this);
      break;
    }
    nextInFlow = (nsInlineFrame*) nextInFlow->GetNextInFlow();
    irs.mNextInFlow = nextInFlow;
  }

  *aIsComplete = isComplete;
  return frame;
}

void
nsInlineFrame::PushFrames(nsPresContext* aPresContext,
                          nsIFrame* aFromChild,
                          nsIFrame* aPrevSibling,
                          InlineReflowState& aState)
{
  NS_PRECONDITION(aFromChild, "null pointer");
  NS_PRECONDITION(aPrevSibling, "pushing first child");
  NS_PRECONDITION(aPrevSibling->GetNextSibling() == aFromChild, "bad prev sibling");

#ifdef NOISY_PUSHING
  printf("%p pushing aFromChild %p, disconnecting from prev sib %p\n", 
         this, aFromChild, aPrevSibling);
#endif

  
  
  SetOverflowFrames(aPresContext, mFrames.RemoveFramesAfter(aPrevSibling));
  if (aState.mLineLayout) {
    aState.mLineLayout->SetDirtyNextLine();
  }
}




int
nsInlineFrame::GetSkipSides() const
{
  int skip = 0;
  if (!IsLeftMost()) {
    nsInlineFrame* prev = (nsInlineFrame*) GetPrevContinuation();
    if ((GetStateBits() & NS_INLINE_FRAME_BIDI_VISUAL_STATE_IS_SET) ||
        (prev && (prev->mRect.height || prev->mRect.width))) {
      
      
      skip |= 1 << NS_SIDE_LEFT;
    }
    else {
      
      
    }
  }
  if (!IsRightMost()) {
    nsInlineFrame* next = (nsInlineFrame*) GetNextContinuation();
    if ((GetStateBits() & NS_INLINE_FRAME_BIDI_VISUAL_STATE_IS_SET) ||
        (next && (next->mRect.height || next->mRect.width))) {
      
      
      skip |= 1 << NS_SIDE_RIGHT;
    }
    else {
      
      
    }
  }

  if (GetStateBits() & NS_FRAME_IS_SPECIAL) {
    
    
    
    
    
    bool ltr = (NS_STYLE_DIRECTION_LTR == GetStyleVisibility()->mDirection);
    int startBit = (1 << (ltr ? NS_SIDE_LEFT : NS_SIDE_RIGHT));
    int endBit = (1 << (ltr ? NS_SIDE_RIGHT : NS_SIDE_LEFT));
    if (((startBit | endBit) & skip) != (startBit | endBit)) {
      
      
      nsIFrame* firstContinuation = GetFirstContinuation();
      if (nsLayoutUtils::FrameIsNonLastInIBSplit(firstContinuation)) {
        skip |= endBit;
      }
      if (nsLayoutUtils::FrameIsNonFirstInIBSplit(firstContinuation)) {
        skip |= startBit;
      }
    }
  }

  return skip;
}

nscoord
nsInlineFrame::GetBaseline() const
{
  return mBaseline;
}

#ifdef ACCESSIBILITY
a11y::AccType
nsInlineFrame::AccessibleType()
{
  
  
  nsIAtom *tagAtom = mContent->Tag();
  if (tagAtom == nsGkAtoms::input)  
    return a11y::eHTMLButtonType;
  if (tagAtom == nsGkAtoms::img)  
    return a11y::eImageType;
  if (tagAtom == nsGkAtoms::label)  
    return a11y::eHTMLLabelType;

  return a11y::eNoType;
}
#endif





nsIFrame*
NS_NewFirstLineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsFirstLineFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsFirstLineFrame)

#ifdef DEBUG
NS_IMETHODIMP
nsFirstLineFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Line"), aResult);
}
#endif

nsIAtom*
nsFirstLineFrame::GetType() const
{
  return nsGkAtoms::lineFrame;
}

nsIFrame*
nsFirstLineFrame::PullOneFrame(nsPresContext* aPresContext, InlineReflowState& irs,
                               bool* aIsComplete)
{
  nsIFrame* frame = nsInlineFrame::PullOneFrame(aPresContext, irs, aIsComplete);
  if (frame && !GetPrevInFlow()) {
    
    
    NS_ASSERTION(frame->GetParent() == this, "Incorrect parent?");
    aPresContext->FrameManager()->ReparentStyleContext(frame);
  }
  return frame;
}

NS_IMETHODIMP
nsFirstLineFrame::Reflow(nsPresContext* aPresContext,
                         nsHTMLReflowMetrics& aMetrics,
                         const nsHTMLReflowState& aReflowState,
                         nsReflowStatus& aStatus)
{
  if (nullptr == aReflowState.mLineLayout) {
    return NS_ERROR_INVALID_ARG;
  }

  nsIFrame* lineContainer = aReflowState.mLineLayout->GetLineContainerFrame();

  
  nsFirstLineFrame* prevInFlow = (nsFirstLineFrame*)GetPrevInFlow();
  if (nullptr != prevInFlow) {
    nsAutoPtr<nsFrameList> prevOverflowFrames(prevInFlow->StealOverflowFrames());
    if (prevOverflowFrames) {
      
      if (lineContainer && lineContainer->GetPrevContinuation()) {
        ReparentFloatsForInlineChild(lineContainer,
                                     prevOverflowFrames->FirstChild(),
                                     true);
      }
      const nsFrameList::Slice& newFrames =
        mFrames.InsertFrames(this, nullptr, *prevOverflowFrames);
      ReparentChildListStyle(aPresContext, newFrames, this);
    }
  }

  
  nsAutoPtr<nsFrameList> overflowFrames(StealOverflowFrames());
  if (overflowFrames) {
    NS_ASSERTION(mFrames.NotEmpty(), "overflow list w/o frames");

    const nsFrameList::Slice& newFrames =
      mFrames.AppendFrames(nullptr, *overflowFrames);
    ReparentChildListStyle(aPresContext, newFrames, this);
  }

  
  
  InlineReflowState irs;
  irs.mPrevFrame = nullptr;
  irs.mLineContainer = lineContainer;
  irs.mLineLayout = aReflowState.mLineLayout;
  irs.mNextInFlow = (nsInlineFrame*) GetNextInFlow();

  nsresult rv;
  bool wasEmpty = mFrames.IsEmpty();
  if (wasEmpty) {
    
    
    bool complete;
    PullOneFrame(aPresContext, irs, &complete);
  }

  if (nullptr == GetPrevInFlow()) {
    
    
    
    
    
    
    irs.mPrevFrame = mFrames.LastChild();
    for (;;) {
      bool complete;
      nsIFrame* frame = PullOneFrame(aPresContext, irs, &complete);
      if (!frame) {
        break;
      }
      irs.mPrevFrame = frame;
    }
    irs.mPrevFrame = nullptr;
  }
  else {

    
    
    
    
    nsFirstLineFrame* first = (nsFirstLineFrame*) GetFirstInFlow();
    if (mStyleContext == first->mStyleContext) {
      
      
      nsStyleContext* parentContext = first->GetParent()->StyleContext();
      
      
      
      
      nsRefPtr<nsStyleContext> newSC;
      newSC = aPresContext->StyleSet()->
        ResolveAnonymousBoxStyle(nsCSSAnonBoxes::mozLineFrame, parentContext);
      if (newSC) {
        
        SetStyleContext(newSC);

        
        ReparentChildListStyle(aPresContext, mFrames, this);
      }
    }
  }

  NS_ASSERTION(!aReflowState.mLineLayout->GetInFirstLine(),
               "Nested first-line frames? BOGUS");
  aReflowState.mLineLayout->SetInFirstLine(true);
  rv = ReflowFrames(aPresContext, aReflowState, irs, aMetrics, aStatus);
  aReflowState.mLineLayout->SetInFirstLine(false);

  ReflowAbsoluteFrames(aPresContext, aMetrics, aReflowState, aStatus);

  

  return rv;
}

 void
nsFirstLineFrame::PullOverflowsFromPrevInFlow()
{
  nsFirstLineFrame* prevInFlow = static_cast<nsFirstLineFrame*>(GetPrevInFlow());
  if (prevInFlow) {
    nsAutoPtr<nsFrameList> prevOverflowFrames(prevInFlow->StealOverflowFrames());
    if (prevOverflowFrames) {
      
      const nsFrameList::Slice& newFrames =
        mFrames.InsertFrames(this, nullptr, *prevOverflowFrames);
      ReparentChildListStyle(PresContext(), newFrames, this);
    }
  }
}

