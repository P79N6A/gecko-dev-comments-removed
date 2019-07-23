















































#include "nsCOMPtr.h"
#include "nsBlockFrame.h"
#include "nsBlockReflowContext.h"
#include "nsBlockReflowState.h"
#include "nsBlockBandData.h"
#include "nsBulletFrame.h"
#include "nsLineBox.h"
#include "nsInlineFrame.h"
#include "nsLineLayout.h"
#include "nsPlaceholderFrame.h"
#include "nsStyleConsts.h"
#include "nsFrameManager.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsStyleContext.h"
#include "nsIView.h"
#include "nsIFontMetrics.h"
#include "nsHTMLParts.h"
#include "nsGkAtoms.h"
#include "nsIDOMEvent.h"
#include "nsGenericHTMLElement.h"
#include "prprf.h"
#include "nsStyleChangeList.h"
#include "nsFrameSelection.h"
#include "nsSpaceManager.h"
#include "nsIntervalSet.h"
#include "prenv.h"
#include "plstr.h"
#include "nsGUIEvent.h"
#include "nsLayoutErrors.h"
#include "nsAutoPtr.h"
#include "nsIServiceManager.h"
#include "nsIScrollableFrame.h"
#ifdef ACCESSIBILITY
#include "nsIDOMHTMLDocument.h"
#include "nsIAccessibilityService.h"
#endif
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"
#include "nsContentErrors.h"
#include "nsCSSAnonBoxes.h"
#include "nsCSSRendering.h"

#ifdef IBMBIDI
#include "nsBidiPresUtils.h"
#endif 

#include "nsIDOMHTMLBodyElement.h"
#include "nsIDOMHTMLHtmlElement.h"

static const int MIN_LINES_NEEDING_CURSOR = 20;

#define DISABLE_FLOAT_BREAKING_IN_COLUMNS

#ifdef DEBUG
#include "nsPrintfCString.h"
#include "nsBlockDebugFlags.h"

PRBool nsBlockFrame::gLamePaintMetrics;
PRBool nsBlockFrame::gLameReflowMetrics;
PRBool nsBlockFrame::gNoisy;
PRBool nsBlockFrame::gNoisyDamageRepair;
PRBool nsBlockFrame::gNoisyIntrinsic;
PRBool nsBlockFrame::gNoisyReflow;
PRBool nsBlockFrame::gReallyNoisyReflow;
PRBool nsBlockFrame::gNoisySpaceManager;
PRBool nsBlockFrame::gVerifyLines;
PRBool nsBlockFrame::gDisableResizeOpt;

PRInt32 nsBlockFrame::gNoiseIndent;

struct BlockDebugFlags {
  const char* name;
  PRBool* on;
};

static const BlockDebugFlags gFlags[] = {
  { "reflow", &nsBlockFrame::gNoisyReflow },
  { "really-noisy-reflow", &nsBlockFrame::gReallyNoisyReflow },
  { "intrinsic", &nsBlockFrame::gNoisyIntrinsic },
  { "space-manager", &nsBlockFrame::gNoisySpaceManager },
  { "verify-lines", &nsBlockFrame::gVerifyLines },
  { "damage-repair", &nsBlockFrame::gNoisyDamageRepair },
  { "lame-paint-metrics", &nsBlockFrame::gLamePaintMetrics },
  { "lame-reflow-metrics", &nsBlockFrame::gLameReflowMetrics },
  { "disable-resize-opt", &nsBlockFrame::gDisableResizeOpt },
};
#define NUM_DEBUG_FLAGS (sizeof(gFlags) / sizeof(gFlags[0]))

static void
ShowDebugFlags()
{
  printf("Here are the available GECKO_BLOCK_DEBUG_FLAGS:\n");
  const BlockDebugFlags* bdf = gFlags;
  const BlockDebugFlags* end = gFlags + NUM_DEBUG_FLAGS;
  for (; bdf < end; bdf++) {
    printf("  %s\n", bdf->name);
  }
  printf("Note: GECKO_BLOCK_DEBUG_FLAGS is a comma separated list of flag\n");
  printf("names (no whitespace)\n");
}

void
nsBlockFrame::InitDebugFlags()
{
  static PRBool firstTime = PR_TRUE;
  if (firstTime) {
    firstTime = PR_FALSE;
    char* flags = PR_GetEnv("GECKO_BLOCK_DEBUG_FLAGS");
    if (flags) {
      PRBool error = PR_FALSE;
      for (;;) {
        char* cm = PL_strchr(flags, ',');
        if (cm) *cm = '\0';

        PRBool found = PR_FALSE;
        const BlockDebugFlags* bdf = gFlags;
        const BlockDebugFlags* end = gFlags + NUM_DEBUG_FLAGS;
        for (; bdf < end; bdf++) {
          if (PL_strcasecmp(bdf->name, flags) == 0) {
            *(bdf->on) = PR_TRUE;
            printf("nsBlockFrame: setting %s debug flag on\n", bdf->name);
            gNoisy = PR_TRUE;
            found = PR_TRUE;
            break;
          }
        }
        if (!found) {
          error = PR_TRUE;
        }

        if (!cm) break;
        *cm = ',';
        flags = cm + 1;
      }
      if (error) {
        ShowDebugFlags();
      }
    }
  }
}

#endif



#define MAX_DEPTH_FOR_LIST_RENUMBERING 200  // 200 open displayable tags is pretty unrealistic





#ifdef DEBUG
const char* nsBlockFrame::kReflowCommandType[] = {
  "ContentChanged",
  "StyleChanged",
  "ReflowDirty",
  "Timeout",
  "UserDefined",
};
#endif

#ifdef REALLY_NOISY_FIRST_LINE
static void
DumpStyleGeneaology(nsIFrame* aFrame, const char* gap)
{
  fputs(gap, stdout);
  nsFrame::ListTag(stdout, aFrame);
  printf(": ");
  nsStyleContext* sc = aFrame->GetStyleContext();
  while (nsnull != sc) {
    nsStyleContext* psc;
    printf("%p ", sc);
    psc = sc->GetParent();
    sc = psc;
  }
  printf("\n");
}
#endif

#ifdef REFLOW_STATUS_COVERAGE
static void
RecordReflowStatus(PRBool aChildIsBlock, nsReflowStatus aFrameReflowStatus)
{
  static PRUint32 record[2];

  
  
  PRIntn index = 0;
  if (!aChildIsBlock) index |= 1;

  
  PRUint32 newS = record[index];
  if (NS_INLINE_IS_BREAK(aFrameReflowStatus)) {
    if (NS_INLINE_IS_BREAK_BEFORE(aFrameReflowStatus)) {
      newS |= 1;
    }
    else if (NS_FRAME_IS_NOT_COMPLETE(aFrameReflowStatus)) {
      newS |= 2;
    }
    else {
      newS |= 4;
    }
  }
  else if (NS_FRAME_IS_NOT_COMPLETE(aFrameReflowStatus)) {
    newS |= 8;
  }
  else {
    newS |= 16;
  }

  
  if (record[index] != newS) {
    record[index] = newS;
    printf("record(%d): %02x %02x\n", index, record[0], record[1]);
  }
}
#endif



const nsIID kBlockFrameCID = NS_BLOCK_FRAME_CID;

nsIFrame*
NS_NewBlockFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRUint32 aFlags)
{
  nsBlockFrame* it = new (aPresShell) nsBlockFrame(aContext);
  if (it) {
    it->SetFlags(aFlags);
  }
  return it;
}

nsBlockFrame::~nsBlockFrame()
{
}

void
nsBlockFrame::Destroy()
{
  mAbsoluteContainer.DestroyFrames(this);
  
  
  if (mBullet && HaveOutsideBullet()) {
    mBullet->Destroy();
    mBullet = nsnull;
  }

  mFloats.DestroyFrames();

  nsPresContext* presContext = PresContext();

  nsLineBox::DeleteLineList(presContext, mLines);

  
  nsLineList* overflowLines = RemoveOverflowLines();
  if (overflowLines) {
    nsLineBox::DeleteLineList(presContext, *overflowLines);
  }

  {
    nsAutoOOFFrameList oofs(this);
    oofs.mList.DestroyFrames();
    
  }

  nsBlockFrameSuper::Destroy();
}

 nsILineIterator*
nsBlockFrame::GetLineIterator()
{
  nsLineIterator* it = new nsLineIterator;
  if (!it)
    return nsnull;

  const nsStyleVisibility* visibility = GetStyleVisibility();
  nsresult rv = it->Init(mLines, visibility->mDirection == NS_STYLE_DIRECTION_RTL);
  if (NS_FAILED(rv)) {
    delete it;
    return nsnull;
  }
  return it;
}

NS_IMETHODIMP
nsBlockFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(aInstancePtr, "null out param");

  if (aIID.Equals(kBlockFrameCID)) {
    *aInstancePtr = static_cast<void*>(static_cast<nsBlockFrame*>(this));
    return NS_OK;
  }
  return nsBlockFrameSuper::QueryInterface(aIID, aInstancePtr);
}

nsSplittableType
nsBlockFrame::GetSplittableType() const
{
  return NS_FRAME_SPLITTABLE_NON_RECTANGULAR;
}

#ifdef DEBUG
NS_METHOD
nsBlockFrame::List(FILE* out, PRInt32 aIndent) const
{
  IndentBy(out, aIndent);
  ListTag(out);
#ifdef DEBUG_waterson
  fprintf(out, " [parent=%p]", mParent);
#endif
  if (HasView()) {
    fprintf(out, " [view=%p]", static_cast<void*>(GetView()));
  }
  if (nsnull != mNextSibling) {
    fprintf(out, " next=%p", static_cast<void*>(mNextSibling));
  }

  
  if (nsnull != GetPrevInFlow()) {
    fprintf(out, " prev-in-flow=%p", static_cast<void*>(GetPrevInFlow()));
  }
  if (nsnull != GetNextInFlow()) {
    fprintf(out, " next-in-flow=%p", static_cast<void*>(GetNextInFlow()));
  }

  
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    fprintf(out, " [state=%08x]", mState);
  }
  nsBlockFrame* f = const_cast<nsBlockFrame*>(this);
  if (f->GetStateBits() & NS_FRAME_OUTSIDE_CHILDREN) {
    nsRect overflowArea = f->GetOverflowRect();
    fprintf(out, " [overflow=%d,%d,%d,%d]", overflowArea.x, overflowArea.y,
            overflowArea.width, overflowArea.height);
  }
  PRInt32 numInlineLines = 0;
  PRInt32 numBlockLines = 0;
  if (!mLines.empty()) {
    const_line_iterator line = begin_lines(), line_end = end_lines();
    for ( ; line != line_end; ++line) {
      if (line->IsBlock())
        numBlockLines++;
      else
        numInlineLines++;
    }
  }
  fprintf(out, " sc=%p(i=%d,b=%d)",
          static_cast<void*>(mStyleContext), numInlineLines, numBlockLines);
  nsIAtom* pseudoTag = mStyleContext->GetPseudoType();
  if (pseudoTag) {
    nsAutoString atomString;
    pseudoTag->ToString(atomString);
    fprintf(out, " pst=%s",
            NS_LossyConvertUTF16toASCII(atomString).get());
  }
  fputs("<\n", out);

  aIndent++;

  
  if (!mLines.empty()) {
    const_line_iterator line = begin_lines(), line_end = end_lines();
    for ( ; line != line_end; ++line) {
      line->List(out, aIndent);
    }
  }

  
  const nsLineList* overflowLines = GetOverflowLines();
  if (overflowLines && !overflowLines->empty()) {
    IndentBy(out, aIndent);
    fputs("Overflow-lines<\n", out);
    const_line_iterator line = begin_lines(), line_end = end_lines();
    for ( ; line != line_end; ++line) {
      line->List(out, aIndent + 1);
    }
    IndentBy(out, aIndent);
    fputs(">\n", out);
  }

  nsIAtom* listName = nsnull;
  PRInt32 listIndex = 0;
  for (;;) {
    listName = GetAdditionalChildListName(listIndex++);
    if (nsGkAtoms::overflowList == listName) {
      continue; 
    }
    if (nsnull == listName) {
      break;
    }
    nsIFrame* kid = GetFirstChild(listName);
    if (kid) {
      IndentBy(out, aIndent);
      nsAutoString tmp;
      if (nsnull != listName) {
        listName->ToString(tmp);
        fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);
      }
      fputs("<\n", out);
      while (kid) {
        nsIFrameDebug*  frameDebug;

        if (NS_SUCCEEDED(CallQueryInterface(kid, &frameDebug))) {
          frameDebug->List(out, aIndent + 1);
        }
        kid = kid->GetNextSibling();
      }
      IndentBy(out, aIndent);
      fputs(">\n", out);
    }
  }

  aIndent--;
  IndentBy(out, aIndent);
  fputs(">\n", out);

  return NS_OK;
}

NS_IMETHODIMP_(nsFrameState)
nsBlockFrame::GetDebugStateBits() const
{
  
  
  return nsBlockFrameSuper::GetDebugStateBits() & ~NS_BLOCK_HAS_LINE_CURSOR;
}

NS_IMETHODIMP
nsBlockFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Block"), aResult);
}
#endif

nsIAtom*
nsBlockFrame::GetType() const
{
  return nsGkAtoms::blockFrame;
}

void
nsBlockFrame::InvalidateInternal(const nsRect& aDamageRect,
                                 nscoord aX, nscoord aY, nsIFrame* aForChild,
                                 PRUint32 aFlags)
{
  
  
  const nsStyleDisplay* disp = GetStyleDisplay();
  nsRect absPosClipRect;
  if (GetAbsPosClipRect(disp, &absPosClipRect, GetSize())) {
    
    
    nsRect r;
    if (r.IntersectRect(aDamageRect, absPosClipRect - nsPoint(aX, aY))) {
      nsBlockFrameSuper::InvalidateInternal(r, aX, aY, this, aFlags);
    }
    return;
  }

  nsBlockFrameSuper::InvalidateInternal(aDamageRect, aX, aY, this, aFlags);
}

nscoord
nsBlockFrame::GetBaseline() const
{
  NS_ASSERTION(!NS_SUBTREE_DIRTY(this), "frame must not be dirty");
  nscoord result;
  if (nsLayoutUtils::GetLastLineBaseline(this, &result))
    return result;
  return nsFrame::GetBaseline();
}




nsIFrame*
nsBlockFrame::GetFirstChild(nsIAtom* aListName) const
{
  if (nsGkAtoms::absoluteList == aListName) {
    return mAbsoluteContainer.GetFirstChild();
  }
  else if (nsnull == aListName) {
    return (mLines.empty()) ? nsnull : mLines.front()->mFirstChild;
  }
  else if (aListName == nsGkAtoms::overflowList) {
    nsLineList* overflowLines = GetOverflowLines();
    return overflowLines ? overflowLines->front()->mFirstChild : nsnull;
  }
  else if (aListName == nsGkAtoms::overflowOutOfFlowList) {
    return GetOverflowOutOfFlows().FirstChild();
  }
  else if (aListName == nsGkAtoms::floatList) {
    return mFloats.FirstChild();
  }
  else if (aListName == nsGkAtoms::bulletList) {
    return (HaveOutsideBullet()) ? mBullet : nsnull;
  }
  return nsContainerFrame::GetFirstChild(aListName);;
}

#define NS_BLOCK_FRAME_OVERFLOW_OOF_LIST_INDEX  (NS_CONTAINER_LIST_COUNT_INCL_OC + 0)
#define NS_BLOCK_FRAME_FLOAT_LIST_INDEX         (NS_CONTAINER_LIST_COUNT_INCL_OC + 1)
#define NS_BLOCK_FRAME_BULLET_LIST_INDEX        (NS_CONTAINER_LIST_COUNT_INCL_OC + 2)
#define NS_BLOCK_FRAME_ABSOLUTE_LIST_INDEX      (NS_CONTAINER_LIST_COUNT_INCL_OC + 3)


nsIAtom*
nsBlockFrame::GetAdditionalChildListName(PRInt32 aIndex) const
{
  if (aIndex < NS_CONTAINER_LIST_COUNT_INCL_OC)
    return nsContainerFrame::GetAdditionalChildListName(aIndex);

  switch (aIndex) {
  case NS_BLOCK_FRAME_FLOAT_LIST_INDEX:
    return nsGkAtoms::floatList;
  case NS_BLOCK_FRAME_BULLET_LIST_INDEX:
    return nsGkAtoms::bulletList;
  case NS_BLOCK_FRAME_OVERFLOW_OOF_LIST_INDEX:
    return nsGkAtoms::overflowOutOfFlowList;
  case NS_BLOCK_FRAME_ABSOLUTE_LIST_INDEX:
    return nsGkAtoms::absoluteList;
  default:
    return nsnull;
  }
}

 PRBool
nsBlockFrame::IsContainingBlock() const
{
  
  
  
  
  
  
  nsIAtom *pseudoType = GetStyleContext()->GetPseudoType();
  return pseudoType != nsCSSAnonBoxes::mozAnonymousBlock &&
         pseudoType != nsCSSAnonBoxes::mozAnonymousPositionedBlock;
}

 PRBool
nsBlockFrame::IsFloatContainingBlock() const
{
  return PR_TRUE;
}

static PRBool IsContinuationPlaceholder(nsIFrame* aFrame)
{
  return aFrame->GetPrevInFlow() &&
    nsGkAtoms::placeholderFrame == aFrame->GetType();
}

static void ReparentFrame(nsIFrame* aFrame, nsIFrame* aOldParent,
                          nsIFrame* aNewParent) {
  NS_ASSERTION(aOldParent == aFrame->GetParent(),
               "Parent not consistent with exepectations");

  aFrame->SetParent(aNewParent);

  
  
  nsHTMLContainerFrame::ReparentFrameView(aFrame->PresContext(), aFrame,
                                          aOldParent, aNewParent);
}
 






 void
nsBlockFrame::MarkIntrinsicWidthsDirty()
{
  nsBlockFrame* dirtyBlock = static_cast<nsBlockFrame*>(GetFirstContinuation());
  dirtyBlock->mMinWidth = NS_INTRINSIC_WIDTH_UNKNOWN;
  dirtyBlock->mPrefWidth = NS_INTRINSIC_WIDTH_UNKNOWN;
  if (!(GetStateBits() & NS_BLOCK_NEEDS_BIDI_RESOLUTION)) {
    for (nsIFrame* frame = dirtyBlock; frame; 
         frame = frame->GetNextContinuation()) {
      frame->AddStateBits(NS_BLOCK_NEEDS_BIDI_RESOLUTION);
    }
  }

  nsBlockFrameSuper::MarkIntrinsicWidthsDirty();
}

 nscoord
nsBlockFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nsIFrame* firstInFlow = GetFirstContinuation();
  if (firstInFlow != this)
    return firstInFlow->GetMinWidth(aRenderingContext);

  DISPLAY_MIN_WIDTH(this, mMinWidth);
  if (mMinWidth != NS_INTRINSIC_WIDTH_UNKNOWN)
    return mMinWidth;

#ifdef DEBUG
  if (gNoisyIntrinsic) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": GetMinWidth\n");
  }
  AutoNoisyIndenter indenter(gNoisyIntrinsic);
#endif

  if (GetStateBits() & NS_BLOCK_NEEDS_BIDI_RESOLUTION)
    ResolveBidi();
  InlineMinWidthData data;
  for (nsBlockFrame* curFrame = this; curFrame;
       curFrame = static_cast<nsBlockFrame*>(curFrame->GetNextContinuation())) {
    for (line_iterator line = curFrame->begin_lines(), line_end = curFrame->end_lines();
      line != line_end; ++line)
    {
#ifdef DEBUG
      if (gNoisyIntrinsic) {
        IndentBy(stdout, gNoiseIndent);
        printf("line (%s%s)\n",
               line->IsBlock() ? "block" : "inline",
               line->IsEmpty() ? ", empty" : "");
      }
      AutoNoisyIndenter lineindent(gNoisyIntrinsic);
#endif
      if (line->IsBlock()) {
        data.ForceBreak(aRenderingContext);
        data.currentLine = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                        line->mFirstChild, nsLayoutUtils::MIN_WIDTH);
        data.ForceBreak(aRenderingContext);
      } else {
        if (!curFrame->GetPrevContinuation() &&
            line == curFrame->begin_lines()) {
          const nsStyleCoord &indent = GetStyleText()->mTextIndent;
          if (indent.GetUnit() == eStyleUnit_Coord)
            data.currentLine += indent.GetCoordValue();
        }
        

        data.line = &line;
        nsIFrame *kid = line->mFirstChild;
        for (PRInt32 i = 0, i_end = line->GetChildCount(); i != i_end;
             ++i, kid = kid->GetNextSibling()) {
          kid->AddInlineMinWidth(aRenderingContext, &data);
        }
      }
#ifdef DEBUG
      if (gNoisyIntrinsic) {
        IndentBy(stdout, gNoiseIndent);
        printf("min: [prevLines=%d currentLine=%d]\n",
               data.prevLines, data.currentLine);
      }
#endif
    }
  }
  data.ForceBreak(aRenderingContext);

  mMinWidth = data.prevLines;
  return mMinWidth;
}

 nscoord
nsBlockFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  nsIFrame* firstInFlow = GetFirstContinuation();
  if (firstInFlow != this)
    return firstInFlow->GetPrefWidth(aRenderingContext);

  DISPLAY_PREF_WIDTH(this, mPrefWidth);

  if (mPrefWidth != NS_INTRINSIC_WIDTH_UNKNOWN)
    return mPrefWidth;

#ifdef DEBUG
  if (gNoisyIntrinsic) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": GetPrefWidth\n");
  }
  AutoNoisyIndenter indenter(gNoisyIntrinsic);
#endif

  if (GetStateBits() & NS_BLOCK_NEEDS_BIDI_RESOLUTION)
    ResolveBidi();
  InlinePrefWidthData data;
  for (nsBlockFrame* curFrame = this; curFrame;
       curFrame = static_cast<nsBlockFrame*>(curFrame->GetNextContinuation())) {
    for (line_iterator line = curFrame->begin_lines(), line_end = curFrame->end_lines();
         line != line_end; ++line)
    {
#ifdef DEBUG
      if (gNoisyIntrinsic) {
        IndentBy(stdout, gNoiseIndent);
        printf("line (%s%s)\n",
               line->IsBlock() ? "block" : "inline",
               line->IsEmpty() ? ", empty" : "");
      }
      AutoNoisyIndenter lineindent(gNoisyIntrinsic);
#endif
      if (line->IsBlock()) {
        data.ForceBreak(aRenderingContext);
        data.currentLine = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                        line->mFirstChild, nsLayoutUtils::PREF_WIDTH);
        data.ForceBreak(aRenderingContext);
      } else {
        if (!curFrame->GetPrevContinuation() &&
            line == curFrame->begin_lines()) {
          const nsStyleCoord &indent = GetStyleText()->mTextIndent;
          if (indent.GetUnit() == eStyleUnit_Coord)
            data.currentLine += indent.GetCoordValue();
        }
        

        data.line = &line;
        nsIFrame *kid = line->mFirstChild;
        for (PRInt32 i = 0, i_end = line->GetChildCount(); i != i_end;
             ++i, kid = kid->GetNextSibling()) {
          kid->AddInlinePrefWidth(aRenderingContext, &data);
        }
      }
#ifdef DEBUG
      if (gNoisyIntrinsic) {
        IndentBy(stdout, gNoiseIndent);
        printf("pref: [prevLines=%d currentLine=%d]\n",
               data.prevLines, data.currentLine);
      }
#endif
    }
  }
  data.ForceBreak(aRenderingContext);

  mPrefWidth = data.prevLines;
  return mPrefWidth;
}

nsRect
nsBlockFrame::ComputeTightBounds(gfxContext* aContext) const
{
  
  if (GetStyleContext()->HasTextDecorations())
    return GetOverflowRect();
  return ComputeSimpleTightBounds(aContext);
}

static nsSize
CalculateContainingBlockSizeForAbsolutes(const nsHTMLReflowState& aReflowState,
                                         nsSize aFrameSize)
{
  
  
  
  
  nsIFrame* frame = aReflowState.frame;

  nsSize cbSize(aFrameSize);
    
  const nsMargin& border =
    aReflowState.mComputedBorderPadding - aReflowState.mComputedPadding;
  cbSize.width -= border.LeftRight();
  cbSize.height -= border.TopBottom();

  if (frame->GetParent()->GetContent() == frame->GetContent() &&
      frame->GetParent()->GetType() != nsGkAtoms::canvasFrame) {
    
    
    
    
    
    
    
    

    
    
    const nsHTMLReflowState* aLastRS = &aReflowState;
    const nsHTMLReflowState* lastButOneRS = &aReflowState;
    while (aLastRS->parentReflowState &&
           aLastRS->parentReflowState->frame->GetContent() == frame->GetContent()) {
      lastButOneRS = aLastRS;
      aLastRS = aLastRS->parentReflowState;
    }
    if (aLastRS != &aReflowState) {
      
      
      nsIScrollableFrame* scrollFrame;
      CallQueryInterface(aLastRS->frame, &scrollFrame);
      nsMargin scrollbars(0,0,0,0);
      if (scrollFrame) {
        scrollbars =
          scrollFrame->GetDesiredScrollbarSizes(aLastRS->frame->PresContext(),
                                                aLastRS->rendContext);
        if (!lastButOneRS->mFlags.mAssumingHScrollbar) {
          scrollbars.top = scrollbars.bottom = 0;
        }
        if (!lastButOneRS->mFlags.mAssumingVScrollbar) {
          scrollbars.left = scrollbars.right = 0;
        }
      }
      
      
      if (aLastRS->ComputedWidth() != NS_UNCONSTRAINEDSIZE) {
        cbSize.width = PR_MAX(0,
          aLastRS->ComputedWidth() + aLastRS->mComputedPadding.LeftRight() - scrollbars.LeftRight());
      }
      if (aLastRS->ComputedHeight() != NS_UNCONSTRAINEDSIZE) {
        cbSize.height = PR_MAX(0,
          aLastRS->ComputedHeight() + aLastRS->mComputedPadding.TopBottom() - scrollbars.TopBottom());
      }
    }
  }

  return cbSize;
}

NS_IMETHODIMP
nsBlockFrame::Reflow(nsPresContext*           aPresContext,
                     nsHTMLReflowMetrics&     aMetrics,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsBlockFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);
#ifdef DEBUG
  if (gNoisyReflow) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": begin reflow availSize=%d,%d computedSize=%d,%d\n",
           aReflowState.availableWidth, aReflowState.availableHeight,
           aReflowState.ComputedWidth(), aReflowState.ComputedHeight());
  }
  AutoNoisyIndenter indent(gNoisy);
  PRTime start = LL_ZERO; 
  PRInt32 ctc = 0;        
  if (gLameReflowMetrics) {
    start = PR_Now();
    ctc = nsLineBox::GetCtorCount();
  }
#endif

  
  
  nsSize oldSize = GetSize();

  
  nsAutoSpaceManager autoSpaceManager(const_cast<nsHTMLReflowState &>(aReflowState));

  
  
  
  
  PRBool needSpaceManager = nsBlockFrame::BlockNeedsSpaceManager(this);
  if (needSpaceManager)
    autoSpaceManager.CreateSpaceManager(aPresContext);

  
  
  
  ClearLineCursor();

  if (IsFrameTreeTooDeep(aReflowState, aMetrics)) {
#ifdef DEBUG_kipp
    {
      extern char* nsPresShell_ReflowStackPointerTop;
      char marker;
      char* newsp = (char*) &marker;
      printf("XXX: frame tree is too deep; approx stack size = %d\n",
             nsPresShell_ReflowStackPointerTop - newsp);
    }
#endif
    aStatus = NS_FRAME_COMPLETE;
    return NS_OK;
  }

  PRBool marginRoot = BlockIsMarginRoot(this);
  nsBlockReflowState state(aReflowState, aPresContext, this, aMetrics,
                           marginRoot, marginRoot, needSpaceManager);

#ifdef IBMBIDI
  if (GetStateBits() & NS_BLOCK_NEEDS_BIDI_RESOLUTION)
    static_cast<nsBlockFrame*>(GetFirstContinuation())->ResolveBidi();
#endif 

  if (RenumberLists(aPresContext)) {
    AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
  }

  nsresult rv = NS_OK;

  
  
  
  DrainOverflowLines(state);
  state.SetupOverflowPlaceholdersProperty();
 
  
  
  
  if (aReflowState.mFlags.mHResize)
    PrepareResizeReflow(state);

  mState &= ~NS_FRAME_FIRST_REFLOW;

  
  rv = ReflowDirtyLines(state);
  NS_ASSERTION(NS_SUCCEEDED(rv), "reflow dirty lines failed");
  if (NS_FAILED(rv)) return rv;

  
  nsRect overflowContainerBounds;
  if (GetPrevInFlow()) {
    ReflowOverflowContainerChildren(aPresContext, aReflowState,
                                    overflowContainerBounds, 0,
                                    state.mReflowStatus);
  }

  
  
  
  
  
  if (state.mOverflowPlaceholders.NotEmpty()) {
    NS_ASSERTION(aReflowState.availableHeight != NS_UNCONSTRAINEDSIZE,
                 "Somehow we failed to fit all content, even though we have unlimited space!");
    if (NS_FRAME_IS_FULLY_COMPLETE(state.mReflowStatus)) {
      
      for (const nsHTMLReflowState* ancestorRS = aReflowState.parentReflowState; 
           ancestorRS; 
           ancestorRS = ancestorRS->parentReflowState) {
        nsIFrame* ancestor = ancestorRS->frame;
        if (nsLayoutUtils::GetAsBlock(ancestor) &&
            aReflowState.mSpaceManager == ancestorRS->mSpaceManager) {
          
          nsFrameList* ancestorPlace =
            ((nsBlockFrame*)ancestor)->GetOverflowPlaceholders();
          
          
          if (ancestorPlace) {
            for (nsIFrame* f = state.mOverflowPlaceholders.FirstChild();
                 f; f = f->GetNextSibling()) {
              NS_ASSERTION(IsContinuationPlaceholder(f),
                           "Overflow placeholders must be continuation placeholders");
              ReparentFrame(f, this, ancestorRS->frame);
              nsIFrame* oof = nsPlaceholderFrame::GetRealFrameForPlaceholder(f);
              mFloats.RemoveFrame(oof);
              ReparentFrame(oof, this, ancestorRS->frame);
              
              oof->SetNextSibling(nsnull);
              
              
              
            }
            ancestorPlace->AppendFrames(nsnull, state.mOverflowPlaceholders.FirstChild());
            state.mOverflowPlaceholders.SetFrames(nsnull);
            break;
          }
        }
      }
    }
    if (!state.mOverflowPlaceholders.IsEmpty()) {
      state.mOverflowPlaceholders.SortByContentOrder();
      PRInt32 numOverflowPlace = state.mOverflowPlaceholders.GetLength();
      nsLineBox* newLine =
        state.NewLineBox(state.mOverflowPlaceholders.FirstChild(),
                         numOverflowPlace, PR_FALSE);
      if (newLine) {
        nsLineList* overflowLines = GetOverflowLines();
        if (overflowLines) {
          
          
          
          
          nsFrameList floats;
          nsIFrame* lastFloat = nsnull;
          for (nsIFrame* f = state.mOverflowPlaceholders.FirstChild();
               f; f = f->GetNextSibling()) {
            NS_ASSERTION(IsContinuationPlaceholder(f),
                         "Overflow placeholders must be continuation placeholders");
            nsIFrame* oof = nsPlaceholderFrame::GetRealFrameForPlaceholder(f);
            
            floats.InsertFrames(nsnull, lastFloat, oof);
            lastFloat = oof;
          }

          
          
          nsIFrame* lastChild = overflowLines->back()->LastChild();
          lastChild->SetNextSibling(state.mOverflowPlaceholders.FirstChild());
          
          
          overflowLines->push_back(newLine);

          nsAutoOOFFrameList oofs(this);
          oofs.mList.AppendFrames(nsnull, floats.FirstChild());
        }
        else {
          mLines.push_back(newLine);
          nsLineList::iterator nextToLastLine = ----end_lines();
          PushLines(state, nextToLastLine);
        }
        state.mOverflowPlaceholders.SetFrames(nsnull);
      }
      state.mReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
      NS_FRAME_SET_INCOMPLETE(state.mReflowStatus);
    }
  }

  if (NS_FRAME_IS_NOT_COMPLETE(state.mReflowStatus)) {
    if (GetOverflowLines()) {
      state.mReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
    }

#ifdef DEBUG_kipp
    ListTag(stdout); printf(": block is not complete\n");
#endif
  }

  CheckFloats(state);

  
  
  
  
  
  
  
  
  
  
  
  
  if (mBullet && HaveOutsideBullet() && !mLines.empty() &&
      (mLines.front()->IsBlock() ||
       (0 == mLines.front()->mBounds.height &&
        mLines.front() != mLines.back() &&
        mLines.begin().next()->IsBlock()))) {
    
    nsHTMLReflowMetrics metrics;
    
    
    
    
    ReflowBullet(state, metrics, aReflowState.mComputedBorderPadding.top);

    nscoord baseline;
    if (nsLayoutUtils::GetFirstLineBaseline(this, &baseline)) {
      

      
      
    
      
      nsRect bbox = mBullet->GetRect();
      bbox.y = baseline - metrics.ascent;
      mBullet->SetRect(bbox);
    }
    
  }

  
  ComputeFinalSize(aReflowState, state, aMetrics);

  ComputeCombinedArea(aReflowState, aMetrics);
  
  aMetrics.mOverflowArea.UnionRect(aMetrics.mOverflowArea,
                                   overflowContainerBounds);

  
#ifdef DEBUG
  PRInt32 verifyReflowFlags = nsIPresShell::GetVerifyReflowFlags();
  if (VERIFY_REFLOW_INCLUDE_SPACE_MANAGER & verifyReflowFlags)
  {
    
    nsIPresShell *shell = aPresContext->GetPresShell();
    if (shell) {
      nsHTMLReflowState&  reflowState = (nsHTMLReflowState&)aReflowState;
      rv = SetProperty(nsGkAtoms::spaceManagerProperty,
                       reflowState.mSpaceManager,
                       nsnull );

      autoSpaceManager.DebugOrphanSpaceManager();
    }
  }
#endif

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (mAbsoluteContainer.HasAbsoluteFrames()) {
    if (aReflowState.WillReflowAgainForClearance()) {
      
      
      
      
      mAbsoluteContainer.MarkSizeDependentFramesDirty();
    } else {
      nsRect childBounds;
      nsSize containingBlockSize =
        CalculateContainingBlockSizeForAbsolutes(aReflowState,
                                                 nsSize(aMetrics.width,
                                                        aMetrics.height));

      
      
      
      
      
      
      PRBool cbWidthChanged = aMetrics.width != oldSize.width;
      PRBool isRoot = !GetContent()->GetParent();
      
      
      
      
      PRBool cbHeightChanged =
        !(isRoot && NS_UNCONSTRAINEDSIZE == aReflowState.ComputedHeight()) &&
        aMetrics.height != oldSize.height;

      rv = mAbsoluteContainer.Reflow(this, aPresContext, aReflowState,
                                     state.mReflowStatus,
                                     containingBlockSize.width,
                                     containingBlockSize.height, PR_TRUE,
                                     cbWidthChanged, cbHeightChanged,
                                     &childBounds);

      

      
      aMetrics.mOverflowArea.UnionRect(aMetrics.mOverflowArea, childBounds);
    }
  }

  
  CheckInvalidateSizeChange(aMetrics);

  FinishAndStoreOverflow(&aMetrics);

  
  
  
  if (needSpaceManager)
    state.mSpaceManager = nsnull;

  aStatus = state.mReflowStatus;

#ifdef DEBUG
  if (gNoisyReflow) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": status=%x (%scomplete) metrics=%d,%d carriedMargin=%d",
           aStatus, NS_FRAME_IS_COMPLETE(aStatus) ? "" : "not ",
           aMetrics.width, aMetrics.height,
           aMetrics.mCarriedOutBottomMargin.get());
    if (mState & NS_FRAME_OUTSIDE_CHILDREN) {
      printf(" combinedArea={%d,%d,%d,%d}",
             aMetrics.mOverflowArea.x,
             aMetrics.mOverflowArea.y,
             aMetrics.mOverflowArea.width,
             aMetrics.mOverflowArea.height);
    }
    printf("\n");
  }

  if (gLameReflowMetrics) {
    PRTime end = PR_Now();

    PRInt32 ectc = nsLineBox::GetCtorCount();
    PRInt32 numLines = mLines.size();
    if (!numLines) numLines = 1;
    PRTime delta, perLineDelta, lines;
    LL_I2L(lines, numLines);
    LL_SUB(delta, end, start);
    LL_DIV(perLineDelta, delta, lines);

    ListTag(stdout);
    char buf[400];
    PR_snprintf(buf, sizeof(buf),
                ": %lld elapsed (%lld per line) (%d lines; %d new lines)",
                delta, perLineDelta, numLines, ectc - ctc);
    printf("%s\n", buf);
  }
#endif

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
  return rv;
}

PRBool
nsBlockFrame::CheckForCollapsedBottomMarginFromClearanceLine()
{
  line_iterator begin = begin_lines();
  line_iterator line = end_lines();

  while (PR_TRUE) {
    if (begin == line) {
      return PR_FALSE;
    }
    --line;
    if (line->mBounds.height != 0 || !line->CachedIsEmpty()) {
      return PR_FALSE;
    }
    if (line->HasClearance()) {
      return PR_TRUE;
    }
  }
  
}

void
nsBlockFrame::ComputeFinalSize(const nsHTMLReflowState& aReflowState,
                               nsBlockReflowState&      aState,
                               nsHTMLReflowMetrics&     aMetrics)
{
  const nsMargin& borderPadding = aState.BorderPadding();
#ifdef NOISY_FINAL_SIZE
  ListTag(stdout);
  printf(": mY=%d mIsBottomMarginRoot=%s mPrevBottomMargin=%d bp=%d,%d\n",
         aState.mY, aState.GetFlag(BRS_ISBOTTOMMARGINROOT) ? "yes" : "no",
         aState.mPrevBottomMargin,
         borderPadding.top, borderPadding.bottom);
#endif

  
  aMetrics.width = borderPadding.left + aReflowState.ComputedWidth() +
    borderPadding.right;

  
  
  
  
  
  nscoord nonCarriedOutVerticalMargin = 0;
  if (!aState.GetFlag(BRS_ISBOTTOMMARGINROOT)) {
    
    
    
    
    
    if (CheckForCollapsedBottomMarginFromClearanceLine()) {
      
      
      nonCarriedOutVerticalMargin = aState.mPrevBottomMargin.get();
      aState.mPrevBottomMargin.Zero();
    }
    aMetrics.mCarriedOutBottomMargin = aState.mPrevBottomMargin;
  } else {
    aMetrics.mCarriedOutBottomMargin.Zero();
  }

  
  if (NS_UNCONSTRAINEDSIZE != aReflowState.ComputedHeight()) {
    
    
    nscoord computedHeightLeftOver = aReflowState.ComputedHeight();
    if (GetPrevInFlow()) {
      
      for (nsIFrame* prev = GetPrevInFlow(); prev; prev = prev->GetPrevInFlow()) {
        computedHeightLeftOver -= prev->GetRect().height;
      }
      
      
      computedHeightLeftOver += aReflowState.mComputedBorderPadding.top;
      
      computedHeightLeftOver = PR_MAX(0, computedHeightLeftOver);
    }
    NS_ASSERTION(!( IS_TRUE_OVERFLOW_CONTAINER(this)
                    && computedHeightLeftOver ),
                 "overflow container must not have computedHeightLeftOver");

    aMetrics.height = borderPadding.top + computedHeightLeftOver + borderPadding.bottom;
    if (NS_FRAME_IS_NOT_COMPLETE(aState.mReflowStatus)
        && aMetrics.height < aReflowState.availableHeight) {
      
      
      NS_FRAME_SET_OVERFLOW_INCOMPLETE(aState.mReflowStatus);
    }

    if (NS_FRAME_IS_COMPLETE(aState.mReflowStatus)) {
      if (computedHeightLeftOver > 0 &&
          NS_UNCONSTRAINEDSIZE != aReflowState.availableHeight &&
          aMetrics.height > aReflowState.availableHeight) {
        
        
        
        
        
        aMetrics.height = PR_MAX(aReflowState.availableHeight,
                                 aState.mY + nonCarriedOutVerticalMargin);
        NS_FRAME_SET_INCOMPLETE(aState.mReflowStatus);
        if (!GetNextInFlow())
          aState.mReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
      }
    }
    else {
      
      
      
      
      aMetrics.height = PR_MAX(aReflowState.availableHeight,
                               aState.mY + nonCarriedOutVerticalMargin);
      
      aMetrics.height = PR_MIN(aMetrics.height,
                               borderPadding.top + computedHeightLeftOver);
      
      
      
      
    }

    
    aMetrics.mCarriedOutBottomMargin.Zero();
  }
  else if (NS_FRAME_IS_COMPLETE(aState.mReflowStatus)) {
    nscoord autoHeight = aState.mY + nonCarriedOutVerticalMargin;

    
    if (aState.GetFlag(BRS_ISBOTTOMMARGINROOT)) {
      
      
      
      if (autoHeight < aState.mReflowState.availableHeight)
      {
        
        autoHeight = PR_MIN(autoHeight + aState.mPrevBottomMargin.get(), aState.mReflowState.availableHeight);
      }
    }

    if (aState.GetFlag(BRS_SPACE_MGR)) {
      
      
      nscoord floatHeight =
        aState.ClearFloats(autoHeight, NS_STYLE_CLEAR_LEFT_AND_RIGHT);
      autoHeight = PR_MAX(autoHeight, floatHeight);
    }

    
    autoHeight -= borderPadding.top;
    nscoord oldAutoHeight = autoHeight;
    aReflowState.ApplyMinMaxConstraints(nsnull, &autoHeight);
    if (autoHeight != oldAutoHeight) {
      
      
      aMetrics.mCarriedOutBottomMargin.Zero();
    }
    autoHeight += borderPadding.top + borderPadding.bottom;
    aMetrics.height = autoHeight;
  }
  else {
    NS_ASSERTION(aReflowState.availableHeight != NS_UNCONSTRAINEDSIZE,
      "Shouldn't be incomplete if availableHeight is UNCONSTRAINED.");
    aMetrics.height = PR_MAX(aState.mY, aReflowState.availableHeight);
    if (aReflowState.availableHeight == NS_UNCONSTRAINEDSIZE)
      
      aMetrics.height = aState.mY;
  }

  if (IS_TRUE_OVERFLOW_CONTAINER(this) &&
      NS_FRAME_IS_NOT_COMPLETE(aState.mReflowStatus)) {
    
    
    NS_ASSERTION(aMetrics.height == 0, "overflow containers must be zero-height");
    NS_FRAME_SET_OVERFLOW_INCOMPLETE(aState.mReflowStatus);
  }

  
  aMetrics.height = PR_MAX(0, aMetrics.height);

#ifdef DEBUG_blocks
  if (CRAZY_WIDTH(aMetrics.width) || CRAZY_HEIGHT(aMetrics.height)) {
    ListTag(stdout);
    printf(": WARNING: desired:%d,%d\n", aMetrics.width, aMetrics.height);
  }
#endif
}

void
nsBlockFrame::ComputeCombinedArea(const nsHTMLReflowState& aReflowState,
                                  nsHTMLReflowMetrics& aMetrics)
{
  
  
  
  nsRect area(0, 0, aMetrics.width, aMetrics.height);

  if (NS_STYLE_OVERFLOW_CLIP != aReflowState.mStyleDisplay->mOverflowX) {
    PRBool inQuirks = (PresContext()->CompatibilityMode() == eCompatibility_NavQuirks);
    for (line_iterator line = begin_lines(), line_end = end_lines();
         line != line_end;
         ++line) {

      
      if (!inQuirks && line->IsInline()) {
        nsRect shadowRect = nsLayoutUtils::GetTextShadowRectsUnion(line->GetCombinedArea(),
                                                                   this);
        area.UnionRect(area, shadowRect);
      }

      area.UnionRect(area, line->GetCombinedArea());
    }

    
    
    
    
    
    if (mBullet) {
      area.UnionRect(area, mBullet->GetRect());
    }
  }
#ifdef NOISY_COMBINED_AREA
  ListTag(stdout);
  printf(": ca=%d,%d,%d,%d\n", area.x, area.y, area.width, area.height);
#endif

  aMetrics.mOverflowArea = area;
}

nsresult
nsBlockFrame::MarkLineDirty(line_iterator aLine, const nsLineList* aLineList)
{
  
  aLine->MarkDirty();
  aLine->SetInvalidateTextRuns(PR_TRUE);
#ifdef DEBUG
  if (gNoisyReflow) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": mark line %p dirty\n", static_cast<void*>(aLine.get()));
  }
#endif

  
  
  
  if (aLine != (aLineList ? aLineList : &mLines)->front() &&
      aLine->IsInline() &&
      aLine.prev()->IsInline()) {
    aLine.prev()->MarkDirty();
    aLine.prev()->SetInvalidateTextRuns(PR_TRUE);
#ifdef DEBUG
    if (gNoisyReflow) {
      IndentBy(stdout, gNoiseIndent);
      ListTag(stdout);
      printf(": mark prev-line %p dirty\n",
             static_cast<void*>(aLine.prev().get()));
    }
#endif
  }

  return NS_OK;
}

nsresult
nsBlockFrame::PrepareResizeReflow(nsBlockReflowState& aState)
{
  
  
  aState.GetAvailableSpace();

  const nsStyleText* styleText = GetStyleText();
  
  PRBool tryAndSkipLines =
      
      !aState.IsImpactedByFloat() &&
      
      (NS_STYLE_TEXT_ALIGN_LEFT == styleText->mTextAlign ||
       (NS_STYLE_TEXT_ALIGN_DEFAULT == styleText->mTextAlign &&
        NS_STYLE_DIRECTION_LTR ==
          aState.mReflowState.mStyleVisibility->mDirection)) &&
      
      
      GetStylePadding()->mPadding.GetLeftUnit() != eStyleUnit_Percent;

#ifdef DEBUG
  if (gDisableResizeOpt) {
    tryAndSkipLines = PR_FALSE;
  }
  if (gNoisyReflow) {
    if (!tryAndSkipLines) {
      IndentBy(stdout, gNoiseIndent);
      ListTag(stdout);
      printf(": marking all lines dirty: availWidth=%d textAlign=%d\n",
             aState.mReflowState.availableWidth,
             styleText->mTextAlign);
    }
  }
#endif

  if (tryAndSkipLines) {
    nscoord newAvailWidth = aState.mReflowState.mComputedBorderPadding.left +
                            aState.mReflowState.ComputedWidth();
    NS_ASSERTION(NS_UNCONSTRAINEDSIZE != aState.mReflowState.mComputedBorderPadding.left &&
                 NS_UNCONSTRAINEDSIZE != aState.mReflowState.ComputedWidth(),
                 "math on NS_UNCONSTRAINEDSIZE");

#ifdef DEBUG
    if (gNoisyReflow) {
      IndentBy(stdout, gNoiseIndent);
      ListTag(stdout);
      printf(": trying to avoid marking all lines dirty\n");
    }
#endif

    for (line_iterator line = begin_lines(), line_end = end_lines();
         line != line_end;
         ++line)
    {
      
      
      if (line->IsBlock() ||
          line->HasFloats() ||
          (line != mLines.back() && !line->HasBreakAfter()) ||
          line->ResizeReflowOptimizationDisabled() ||
          line->IsImpactedByFloat() ||
          (line->mBounds.XMost() > newAvailWidth)) {
        line->MarkDirty();
      }

#ifdef REALLY_NOISY_REFLOW
      if (!line->IsBlock()) {
        printf("PrepareResizeReflow thinks line %p is %simpacted by floats\n", 
               line.get(), line->IsImpactedByFloat() ? "" : "not ");
      }
#endif
#ifdef DEBUG
      if (gNoisyReflow && !line->IsDirty()) {
        IndentBy(stdout, gNoiseIndent + 1);
        printf("skipped: line=%p next=%p %s %s%s%s breakTypeBefore/After=%d/%d xmost=%d\n",
           static_cast<void*>(line.get()),
           static_cast<void*>((line.next() != end_lines() ? line.next().get() : nsnull)),
           line->IsBlock() ? "block" : "inline",
           line->HasBreakAfter() ? "has-break-after " : "",
           line->HasFloats() ? "has-floats " : "",
           line->IsImpactedByFloat() ? "impacted " : "",
           line->GetBreakTypeBefore(), line->GetBreakTypeAfter(),
           line->mBounds.XMost());
      }
#endif
    }
  }
  else {
    
    for (line_iterator line = begin_lines(), line_end = end_lines();
         line != line_end;
         ++line)
    {
      line->MarkDirty();
    }
  }
  return NS_OK;
}














void
nsBlockFrame::PropagateFloatDamage(nsBlockReflowState& aState,
                                   nsLineBox* aLine,
                                   nscoord aDeltaY)
{
  nsSpaceManager *spaceManager = aState.mReflowState.mSpaceManager;
  NS_ASSERTION((aState.mReflowState.parentReflowState &&
                aState.mReflowState.parentReflowState->mSpaceManager == spaceManager) ||
                aState.mReflowState.mBlockDelta == 0, "Bad block delta passed in");

  
  
  if (!spaceManager->HasAnyFloats())
    return;

  
  if (spaceManager->HasFloatDamage()) {
    
    
    nscoord lineYA = aLine->mBounds.y + aDeltaY;
    nscoord lineYB = lineYA + aLine->mBounds.height;
    nscoord lineYCombinedA = aLine->GetCombinedArea().y + aDeltaY;
    nscoord lineYCombinedB = lineYCombinedA + aLine->GetCombinedArea().height;
    if (spaceManager->IntersectsDamage(lineYA, lineYB) ||
        spaceManager->IntersectsDamage(lineYCombinedA, lineYCombinedB)) {
      aLine->MarkDirty();
      return;
    }
  }

  
  if (aDeltaY + aState.mReflowState.mBlockDelta != 0) {
    if (aLine->IsBlock()) {
      
      
      
      
      aLine->MarkDirty();
    } else {
      
      
      aState.GetAvailableSpace(aLine->mBounds.y + aDeltaY, PR_FALSE);
      PRBool wasImpactedByFloat = aLine->IsImpactedByFloat();
      PRBool isImpactedByFloat = aState.IsImpactedByFloat();

#ifdef REALLY_NOISY_REFLOW
    printf("nsBlockFrame::PropagateFloatDamage %p was = %d, is=%d\n", 
       this, wasImpactedByFloat, isImpactedByFloat);
#endif

      
      
      
      if (wasImpactedByFloat || isImpactedByFloat) {
        aLine->MarkDirty();
      }
    }
  }
}

static void PlaceFrameView(nsIFrame* aFrame);

static PRBool LineHasClear(nsLineBox* aLine) {
  return aLine->IsBlock()
    ? (aLine->GetBreakTypeBefore() ||
       (aLine->mFirstChild->GetStateBits() & NS_BLOCK_HAS_CLEAR_CHILDREN) ||
       !nsBlockFrame::BlockCanIntersectFloats(aLine->mFirstChild))
    : aLine->HasFloatBreakAfter();
}







void
nsBlockFrame::ReparentFloats(nsIFrame* aFirstFrame,
                             nsBlockFrame* aOldParent, PRBool aFromOverflow,
                             PRBool aReparentSiblings) {
  nsFrameList list;
  nsIFrame* tail = nsnull;
  aOldParent->CollectFloats(aFirstFrame, list, &tail, aFromOverflow, aReparentSiblings);
  if (list.NotEmpty()) {
    for (nsIFrame* f = list.FirstChild(); f; f = f->GetNextSibling()) {
      ReparentFrame(f, aOldParent, this);
    }
    mFloats.AppendFrames(nsnull, list.FirstChild());
  }
}

static void DumpLine(const nsBlockReflowState& aState, nsLineBox* aLine,
                     nscoord aDeltaY, PRInt32 aDeltaIndent) {
#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsRect lca(aLine->GetCombinedArea());
    nsBlockFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent + aDeltaIndent);
    printf("line=%p mY=%d dirty=%s oldBounds={%d,%d,%d,%d} oldCombinedArea={%d,%d,%d,%d} deltaY=%d mPrevBottomMargin=%d childCount=%d\n",
           static_cast<void*>(aLine), aState.mY,
           aLine->IsDirty() ? "yes" : "no",
           aLine->mBounds.x, aLine->mBounds.y,
           aLine->mBounds.width, aLine->mBounds.height,
           lca.x, lca.y, lca.width, lca.height,
           aDeltaY, aState.mPrevBottomMargin.get(), aLine->GetChildCount());
  }
#endif
}




nsresult
nsBlockFrame::ReflowDirtyLines(nsBlockReflowState& aState)
{
  nsresult rv = NS_OK;
  PRBool keepGoing = PR_TRUE;
  PRBool repositionViews = PR_FALSE; 
  PRBool foundAnyClears = PR_FALSE;
  PRBool willReflowAgain = PR_FALSE;

#ifdef DEBUG
  if (gNoisyReflow) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": reflowing dirty lines");
    printf(" computedWidth=%d\n", aState.mReflowState.ComputedWidth());
  }
  AutoNoisyIndenter indent(gNoisyReflow);
#endif

  PRBool selfDirty = (GetStateBits() & NS_FRAME_IS_DIRTY) ||
                     (aState.mReflowState.mFlags.mVResize &&
                      (GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT));
  
    
    
  nscoord deltaY = 0;

    
    
    
  PRBool needToRecoverState = PR_FALSE;
  PRBool reflowedFloat = PR_FALSE;
  PRBool lastLineMovedUp = PR_FALSE;
  
  PRUint8 inlineFloatBreakType = NS_STYLE_CLEAR_NONE;

  line_iterator line = begin_lines(), line_end = end_lines();

  
  for ( ; line != line_end; ++line, aState.AdvanceToNextLine()) {
    DumpLine(aState, line, deltaY, 0);
#ifdef DEBUG
    AutoNoisyIndenter indent2(gNoisyReflow);
#endif

    if (selfDirty)
      line->MarkDirty();

    
    
    
    if (!line->IsDirty() && line->IsBlock() &&
        (line->mFirstChild->GetStateBits() & NS_BLOCK_HAS_CLEAR_CHILDREN)) {
      line->MarkDirty();
    }

    nsIFrame *replacedBlock = nsnull;
    if (line->IsBlock() &&
        !nsBlockFrame::BlockCanIntersectFloats(line->mFirstChild)) {
      replacedBlock = line->mFirstChild;
    }

    
    
    if (!line->IsDirty() &&
        (line->GetBreakTypeBefore() != NS_STYLE_CLEAR_NONE ||
         replacedBlock)) {
      nscoord curY = aState.mY;
      
      
      if (inlineFloatBreakType != NS_STYLE_CLEAR_NONE) {
        curY = aState.ClearFloats(curY, inlineFloatBreakType);
      }

      nscoord newY =
        aState.ClearFloats(curY, line->GetBreakTypeBefore(), replacedBlock);
      
      if (line->HasClearance()) {
        
        if (newY == curY
            
            
            
            
            || newY != line->mBounds.y + deltaY) {
          line->MarkDirty();
        }
      } else {
        
        if (curY != newY) {
          line->MarkDirty();
        }
      }
    }

    
    if (inlineFloatBreakType != NS_STYLE_CLEAR_NONE) {
      aState.mY = aState.ClearFloats(aState.mY, inlineFloatBreakType);
      if (aState.mY != line->mBounds.y + deltaY) {
        
        
        line->MarkDirty();
      }
      inlineFloatBreakType = NS_STYLE_CLEAR_NONE;
    }

    PRBool previousMarginWasDirty = line->IsPreviousMarginDirty();
    if (previousMarginWasDirty) {
      
      line->MarkDirty();
      line->ClearPreviousMarginDirty();
    } else if (line->mBounds.YMost() + deltaY > aState.mBottomEdge) {
      
      
      line->MarkDirty();
    }

    if (!line->IsDirty()) {
      
      
      PropagateFloatDamage(aState, line, deltaY);
    }

    if (needToRecoverState && line->IsDirty()) {
      
      
      
      aState.ReconstructMarginAbove(line);
    }

    if (needToRecoverState) {
      needToRecoverState = PR_FALSE;

      
      
      
      if (line->IsDirty())
        aState.mPrevChild = line.prev()->LastChild();
    }

    
    
    
    
    
    
    
    
    
    if (line->IsDirty() && (line->HasFloats() || !willReflowAgain)) {
      lastLineMovedUp = PR_TRUE;

      PRBool maybeReflowingForFirstTime =
        line->mBounds.x == 0 && line->mBounds.y == 0 &&
        line->mBounds.width == 0 && line->mBounds.height == 0;

      
      
      
      nscoord oldY = line->mBounds.y;
      nscoord oldYMost = line->mBounds.YMost();

      NS_ASSERTION(!willReflowAgain || !line->IsBlock(),
                   "Don't reflow blocks while willReflowAgain is true, reflow of block abs-pos children depends on this");

      
      
      rv = ReflowLine(aState, line, &keepGoing);
      NS_ENSURE_SUCCESS(rv, rv);

      if (aState.mReflowState.WillReflowAgainForClearance()) {
        line->MarkDirty();
        willReflowAgain = PR_TRUE;
        
        
        
      }

      if (line->HasFloats()) {
        reflowedFloat = PR_TRUE;
      }

      if (!keepGoing) {
        DumpLine(aState, line, deltaY, -1);
        if (0 == line->GetChildCount()) {
          DeleteLine(aState, line, line_end);
        }
        break;
      }

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      if (line.next() != end_lines()) {
        PRBool maybeWasEmpty = oldY == line.next()->mBounds.y;
        PRBool isEmpty = line->CachedIsEmpty();
        if (maybeReflowingForFirstTime  ||
            (isEmpty || maybeWasEmpty) ) {
          line.next()->MarkPreviousMarginDirty();
          
        }
      }

      
      
      
      
      
      
      deltaY = line->mBounds.YMost() - oldYMost;
    } else {
      aState.mOverflowTracker.Skip(line->mFirstChild, aState.mReflowStatus);
        
        
        

      lastLineMovedUp = deltaY < 0;

      if (deltaY != 0)
        SlideLine(aState, line, deltaY);
      else
        repositionViews = PR_TRUE;

      NS_ASSERTION(!line->IsDirty() || !line->HasFloats(),
                   "Possibly stale float cache here!");
      if (willReflowAgain && line->IsBlock()) {
        
        
        
        
        
        
        
        
        
        
      } else {
        
        aState.RecoverStateFrom(line, deltaY);
      }

      
      
      
      
      
      if (line->IsBlock() || !line->CachedIsEmpty()) {
        aState.mY = line->mBounds.YMost();
      }

      needToRecoverState = PR_TRUE;
    }

    
    
    
    
    if (line->HasFloatBreakAfter()) {
      inlineFloatBreakType = line->GetBreakTypeAfter();
    }

    if (LineHasClear(line.get())) {
      foundAnyClears = PR_TRUE;
    }

    DumpLine(aState, line, deltaY, -1);
  }

  
  if (inlineFloatBreakType != NS_STYLE_CLEAR_NONE) {
    aState.mY = aState.ClearFloats(aState.mY, inlineFloatBreakType);
  }

  if (needToRecoverState) {
    
    aState.ReconstructMarginAbove(line);

    
    
    
    aState.mPrevChild = line.prev()->LastChild();
  }

  
  if (repositionViews)
    ::PlaceFrameView(this);

  
  
  
  
  
  
  
  
  
  
  PRBool skipPull = willReflowAgain;
  if (aState.mNextInFlow &&
      (aState.mReflowState.mFlags.mNextInFlowUntouched &&
       !lastLineMovedUp && 
       !(GetStateBits() & NS_FRAME_IS_DIRTY) &&
       !reflowedFloat)) {
    
    
    
    
    
    line_iterator lineIter = this->end_lines();
    if (lineIter != this->begin_lines()) {
      lineIter--; 
      nsBlockInFlowLineIterator bifLineIter(this, lineIter, PR_FALSE);

      
      
      if (!bifLineIter.Next() ||                
          !bifLineIter.GetLine()->IsDirty()) {
        if (IS_TRUE_OVERFLOW_CONTAINER(aState.mNextInFlow))
          NS_FRAME_SET_OVERFLOW_INCOMPLETE(aState.mReflowStatus);
        else
          NS_FRAME_SET_INCOMPLETE(aState.mReflowStatus);
        skipPull=PR_TRUE;
      }
    }
  }
  
  if (!skipPull && aState.mNextInFlow) {
    
    
    while (keepGoing && (nsnull != aState.mNextInFlow)) {
      
      nsBlockFrame* nextInFlow = aState.mNextInFlow;
      line_iterator nifLine = nextInFlow->begin_lines();
      nsLineBox *toMove;
      PRBool collectOverflowFloats;
      if (nifLine != nextInFlow->end_lines()) {
        if (HandleOverflowPlaceholdersOnPulledLine(aState, nifLine)) {
          
          continue;
        }
        toMove = nifLine;
        nextInFlow->mLines.erase(nifLine);
        collectOverflowFloats = PR_FALSE;
      } else {
        
        nsLineList* overflowLines = nextInFlow->GetOverflowLines();
        if (overflowLines &&
            HandleOverflowPlaceholdersOnPulledLine(aState, overflowLines->front())) {
          
          continue;
        }
        if (!overflowLines) {
          aState.mNextInFlow =
            static_cast<nsBlockFrame*>(nextInFlow->GetNextInFlow());
          continue;
        }
        nifLine = overflowLines->begin();
        NS_ASSERTION(nifLine != overflowLines->end(),
                     "Stored overflow line list should not be empty");
        toMove = nifLine;
        nextInFlow->RemoveOverflowLines();
        nifLine = overflowLines->erase(nifLine);
        if (nifLine != overflowLines->end()) {
          
          
          
          
          nextInFlow->SetOverflowLines(overflowLines);
        }
        collectOverflowFloats = PR_TRUE;
      }

      if (0 == toMove->GetChildCount()) {
        
        NS_ASSERTION(nsnull == toMove->mFirstChild, "bad empty line");
        aState.FreeLineBox(toMove);
        continue;
      }

      
      
      nsIFrame* frame = toMove->mFirstChild;
      nsIFrame* lastFrame = nsnull;
      PRInt32 n = toMove->GetChildCount();
      while (--n >= 0) {
        ReparentFrame(frame, nextInFlow, this);
        lastFrame = frame;
        frame = frame->GetNextSibling();
      }
      lastFrame->SetNextSibling(nsnull);

      
      ReparentFloats(toMove->mFirstChild, nextInFlow, collectOverflowFloats, PR_TRUE);

      
      if (aState.mPrevChild) {
        aState.mPrevChild->SetNextSibling(toMove->mFirstChild);
      }
      aState.mPrevChild = toMove->LastChild();

      line = mLines.before_insert(end_lines(), toMove);

      DumpLine(aState, toMove, deltaY, 0);
#ifdef DEBUG
      AutoNoisyIndenter indent2(gNoisyReflow);
#endif

      
      
      
      
      while (line != end_lines()) {
        rv = ReflowLine(aState, line, &keepGoing);
        NS_ENSURE_SUCCESS(rv, rv);
        DumpLine(aState, line, deltaY, -1);
        if (!keepGoing) {
          if (0 == line->GetChildCount()) {
            DeleteLine(aState, line, line_end);
          }
          break;
        }

        if (LineHasClear(line.get())) {
          foundAnyClears = PR_TRUE;
        }

        
        ++line;
        aState.AdvanceToNextLine();
      }
    }

    if (NS_FRAME_IS_NOT_COMPLETE(aState.mReflowStatus)) {
      aState.mReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
    } 
  }

  
  if (mBullet && HaveOutsideBullet() && mLines.empty()) {
    nsHTMLReflowMetrics metrics;
    ReflowBullet(aState, metrics,
                 aState.mReflowState.mComputedBorderPadding.top);

    
    
    aState.mY += metrics.height;
  }

  if (foundAnyClears) {
    AddStateBits(NS_BLOCK_HAS_CLEAR_CHILDREN);
  } else {
    RemoveStateBits(NS_BLOCK_HAS_CLEAR_CHILDREN);
  }

#ifdef DEBUG
  if (gNoisyReflow) {
    IndentBy(stdout, gNoiseIndent - 1);
    ListTag(stdout);
    printf(": done reflowing dirty lines (status=%x)\n",
           aState.mReflowStatus);
  }
#endif

  return rv;
}

void
nsBlockFrame::DeleteLine(nsBlockReflowState& aState,
                         nsLineList::iterator aLine,
                         nsLineList::iterator aLineEnd)
{
  NS_PRECONDITION(0 == aLine->GetChildCount(), "can't delete !empty line");
  if (0 == aLine->GetChildCount()) {
    NS_ASSERTION(aState.mCurrentLine == aLine,
                 "using function more generally than designed, "
                 "but perhaps OK now");
    nsLineBox *line = aLine;
    aLine = mLines.erase(aLine);
    aState.FreeLineBox(line);
    
    
    if (aLine != aLineEnd)
      aLine->MarkPreviousMarginDirty();
  }
}






nsresult
nsBlockFrame::ReflowLine(nsBlockReflowState& aState,
                         line_iterator aLine,
                         PRBool* aKeepReflowGoing)
{
  nsresult rv = NS_OK;

  NS_ABORT_IF_FALSE(aLine->GetChildCount(), "reflowing empty line");

  
  aState.mCurrentLine = aLine;
  aLine->ClearDirty();
  aLine->InvalidateCachedIsEmpty();
  
  
  if (aLine->IsBlock()) {
    nsRect oldBounds = aLine->mFirstChild->GetRect();
    nsRect oldCombinedArea(aLine->GetCombinedArea());
    rv = ReflowBlockFrame(aState, aLine, aKeepReflowGoing);
    nsRect newBounds = aLine->mFirstChild->GetRect();

    
    
    
    
    
    
    
    
    
    nsRect lineCombinedArea(aLine->GetCombinedArea());
    if (oldCombinedArea.TopLeft() != lineCombinedArea.TopLeft() ||
        oldBounds.TopLeft() != newBounds.TopLeft()) {
      
      
      nsRect  dirtyRect;
      dirtyRect.UnionRect(oldCombinedArea, lineCombinedArea);
#ifdef NOISY_BLOCK_INVALIDATE
      printf("%p invalidate 6 (%d, %d, %d, %d)\n",
             this, dirtyRect.x, dirtyRect.y, dirtyRect.width, dirtyRect.height);
#endif
      Invalidate(dirtyRect);
    } else {
      nsRect combinedAreaHStrip, combinedAreaVStrip;
      nsRect boundsHStrip, boundsVStrip;
      nsLayoutUtils::GetRectDifferenceStrips(oldBounds, newBounds,
                                             &boundsHStrip, &boundsVStrip);
      nsLayoutUtils::GetRectDifferenceStrips(oldCombinedArea, lineCombinedArea,
                                             &combinedAreaHStrip,
                                             &combinedAreaVStrip);

#ifdef NOISY_BLOCK_INVALIDATE
      printf("%p invalidate boundsVStrip (%d, %d, %d, %d)\n",
             this, boundsVStrip.x, boundsVStrip.y, boundsVStrip.width, boundsVStrip.height);
      printf("%p invalidate boundsHStrip (%d, %d, %d, %d)\n",
             this, boundsHStrip.x, boundsHStrip.y, boundsHStrip.width, boundsHStrip.height);
      printf("%p invalidate combinedAreaVStrip (%d, %d, %d, %d)\n",
             this, combinedAreaVStrip.x, combinedAreaVStrip.y, combinedAreaVStrip.width, combinedAreaVStrip.height);
      printf("%p invalidate combinedAreaHStrip (%d, %d, %d, %d)\n",
             this, combinedAreaHStrip.x, combinedAreaHStrip.y, combinedAreaHStrip.width, combinedAreaHStrip.height);
#endif
      
      
      Invalidate(boundsVStrip);
      Invalidate(boundsHStrip);
      Invalidate(combinedAreaVStrip);
      Invalidate(combinedAreaHStrip);
    }
  }
  else {
    nsRect oldCombinedArea(aLine->GetCombinedArea());
    aLine->SetLineWrapped(PR_FALSE);

    rv = ReflowInlineFrames(aState, aLine, aKeepReflowGoing);

    
    
    nsRect dirtyRect;
    dirtyRect.UnionRect(oldCombinedArea, aLine->GetCombinedArea());
#ifdef NOISY_BLOCK_INVALIDATE
    printf("%p invalidate (%d, %d, %d, %d)\n",
           this, dirtyRect.x, dirtyRect.y, dirtyRect.width, dirtyRect.height);
    if (aLine->IsForceInvalidate())
      printf("  dirty line is %p\n", static_cast<void*>(aLine.get());
#endif
    Invalidate(dirtyRect);
  }

  return rv;
}





nsresult
nsBlockFrame::PullFrame(nsBlockReflowState& aState,
                        line_iterator aLine,
                        nsIFrame*& aFrameResult)
{
  aFrameResult = nsnull;

  
  if (end_lines() != aLine.next()) {
#ifdef DEBUG
    PRBool retry =
#endif
      PullFrameFrom(aState, aLine, this, PR_FALSE, aLine.next(), aFrameResult);
    NS_ASSERTION(!retry, "Shouldn't have to retry in the current block");
    return NS_OK;
  }

  NS_ASSERTION(!GetOverflowLines(),
    "Our overflow lines should have been removed at the start of reflow");

  
  nsBlockFrame* nextInFlow = aState.mNextInFlow;
  while (nextInFlow) {
    
    if (!nextInFlow->mLines.empty()) {
      if (PullFrameFrom(aState, aLine, nextInFlow, PR_FALSE,
                        nextInFlow->mLines.begin(), aFrameResult)) {
        
        continue;
      }
      break;
    }

    nsLineList* overflowLines = nextInFlow->GetOverflowLines();
    if (overflowLines) {
      if (PullFrameFrom(aState, aLine, nextInFlow, PR_TRUE,
                        overflowLines->begin(), aFrameResult)) {
        
        continue;
      }
      break;
    }

    nextInFlow = (nsBlockFrame*) nextInFlow->GetNextInFlow();
    aState.mNextInFlow = nextInFlow;
  }

  return NS_OK;
}















PRBool
nsBlockFrame::PullFrameFrom(nsBlockReflowState& aState,
                            nsLineBox* aLine,
                            nsBlockFrame* aFromContainer,
                            PRBool aFromOverflowLine,
                            nsLineList::iterator aFromLine,
                            nsIFrame*& aFrameResult)
{
  nsLineBox* fromLine = aFromLine;
  NS_ABORT_IF_FALSE(fromLine, "bad line to pull from");
  NS_ABORT_IF_FALSE(fromLine->GetChildCount(), "empty line");
  NS_ABORT_IF_FALSE(aLine->GetChildCount(), "empty line");

  NS_ASSERTION(fromLine->IsBlock() == fromLine->mFirstChild->GetStyleDisplay()->IsBlockOutside(),
               "Disagreement about whether it's a block or not");

  if (fromLine->IsBlock()) {
    
    
    
    aFrameResult = nsnull;
  }
  else {
    
    nsIFrame* frame = fromLine->mFirstChild;

    if (aFromContainer != this) {
      if (HandleOverflowPlaceholdersForPulledFrame(aState, frame)) {
        
        return PR_TRUE;
      }

      aLine->LastChild()->SetNextSibling(frame);
    }
    
    
    aLine->SetChildCount(aLine->GetChildCount() + 1);
      
    PRInt32 fromLineChildCount = fromLine->GetChildCount();
    if (0 != --fromLineChildCount) {
      
      fromLine->SetChildCount(fromLineChildCount);
      fromLine->MarkDirty();
      fromLine->mFirstChild = frame->GetNextSibling();
    }
    else {
      
      
      
      
      Invalidate(fromLine->mBounds);
      nsLineList* fromLineList = aFromOverflowLine
        ? aFromContainer->RemoveOverflowLines()
        : &aFromContainer->mLines;
      if (aFromLine.next() != fromLineList->end())
        aFromLine.next()->MarkPreviousMarginDirty();

      Invalidate(fromLine->GetCombinedArea());
      fromLineList->erase(aFromLine);
      
      aState.FreeLineBox(fromLine);

      
      if (aFromOverflowLine && !fromLineList->empty()) {
        aFromContainer->SetOverflowLines(fromLineList);
      }
    }

    
    if (aFromContainer != this) {
      
      
      NS_ASSERTION(frame->GetParent() == aFromContainer, "unexpected parent frame");

      ReparentFrame(frame, aFromContainer, this);

      
      
      frame->SetNextSibling(nsnull);
      if (nsnull != aState.mPrevChild) {
        aState.mPrevChild->SetNextSibling(frame);
      }

      
      
      ReparentFloats(frame, aFromContainer, aFromOverflowLine, PR_TRUE);
    }

    
    aFrameResult = frame;
#ifdef DEBUG
    VerifyLines(PR_TRUE);
#endif
  }
  return PR_FALSE;
}

static void
PlaceFrameView(nsIFrame* aFrame)
{
  if (aFrame->HasView())
    nsContainerFrame::PositionFrameView(aFrame);
  else
    nsContainerFrame::PositionChildViews(aFrame);
}

void
nsBlockFrame::SlideLine(nsBlockReflowState& aState,
                        nsLineBox* aLine, nscoord aDY)
{
  NS_PRECONDITION(aDY != 0, "why slide a line nowhere?");

  Invalidate(aLine->GetCombinedArea());
  
  aLine->SlideBy(aDY);
  Invalidate(aLine->GetCombinedArea());

  
  nsIFrame* kid = aLine->mFirstChild;
  if (!kid) {
    return;
  }

  if (aLine->IsBlock()) {
    if (aDY) {
      nsPoint p = kid->GetPosition();
      p.y += aDY;
      kid->SetPosition(p);
    }

    
    ::PlaceFrameView(kid);
  }
  else {
    
    
    
    
    PRInt32 n = aLine->GetChildCount();
    while (--n >= 0) {
      if (aDY) {
        nsPoint p = kid->GetPosition();
        p.y += aDY;
        kid->SetPosition(p);
      }
      
      ::PlaceFrameView(kid);
      kid = kid->GetNextSibling();
    }
  }
}

NS_IMETHODIMP 
nsBlockFrame::AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType)
{
  nsresult rv = nsBlockFrameSuper::AttributeChanged(aNameSpaceID,
                                                    aAttribute, aModType);

  if (NS_FAILED(rv)) {
    return rv;
  }
  if (nsGkAtoms::start == aAttribute) {
    nsPresContext* presContext = PresContext();

    
    if (RenumberLists(presContext)) {
      presContext->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eStyleChange,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
    }
  }
  else if (nsGkAtoms::value == aAttribute) {
    const nsStyleDisplay* styleDisplay = GetStyleDisplay();
    if (NS_STYLE_DISPLAY_LIST_ITEM == styleDisplay->mDisplay) {
      
      
      
      
      nsBlockFrame* blockParent = nsLayoutUtils::FindNearestBlockAncestor(this);

      
      
      if (nsnull != blockParent) {
        nsPresContext* presContext = PresContext();
        
        if (blockParent->RenumberLists(presContext)) {
          presContext->PresShell()->
            FrameNeedsReflow(blockParent, nsIPresShell::eStyleChange,
                             NS_FRAME_HAS_DIRTY_CHILDREN);
        }
      }
    }
  }

  return rv;
}

static inline PRBool
IsPaddingZero(nsStyleUnit aUnit, const nsStyleCoord &aCoord)
{
    return ((aUnit == eStyleUnit_Coord && aCoord.GetCoordValue() == 0) ||
            (aUnit == eStyleUnit_Percent && aCoord.GetPercentValue() == 0.0));
}

 PRBool
nsBlockFrame::IsSelfEmpty()
{
  
  
  
  if (GetStateBits() & NS_BLOCK_MARGIN_ROOT)
    return PR_FALSE;

  const nsStylePosition* position = GetStylePosition();

  switch (position->mMinHeight.GetUnit()) {
    case eStyleUnit_Coord:
      if (position->mMinHeight.GetCoordValue() != 0)
        return PR_FALSE;
      break;
    case eStyleUnit_Percent:
      if (position->mMinHeight.GetPercentValue() != 0.0f)
        return PR_FALSE;
      break;
    default:
      return PR_FALSE;
  }

  switch (position->mHeight.GetUnit()) {
    case eStyleUnit_Auto:
      break;
    case eStyleUnit_Coord:
      if (position->mHeight.GetCoordValue() != 0)
        return PR_FALSE;
      break;
    case eStyleUnit_Percent:
      if (position->mHeight.GetPercentValue() != 0.0f)
        return PR_FALSE;
      break;
    default:
      return PR_FALSE;
  }

  const nsStyleBorder* border = GetStyleBorder();
  const nsStylePadding* padding = GetStylePadding();
  if (border->GetActualBorderWidth(NS_SIDE_TOP) != 0 ||
      border->GetActualBorderWidth(NS_SIDE_BOTTOM) != 0 ||
      !IsPaddingZero(padding->mPadding.GetTopUnit(),
                     padding->mPadding.GetTop()) ||
      !IsPaddingZero(padding->mPadding.GetBottomUnit(),
                     padding->mPadding.GetBottom())) {
    return PR_FALSE;
  }

  return PR_TRUE;
}

PRBool
nsBlockFrame::CachedIsEmpty()
{
  if (!IsSelfEmpty()) {
    return PR_FALSE;
  }

  for (line_iterator line = begin_lines(), line_end = end_lines();
       line != line_end;
       ++line)
  {
    if (!line->CachedIsEmpty())
      return PR_FALSE;
  }

  return PR_TRUE;
}

PRBool
nsBlockFrame::IsEmpty()
{
  if (!IsSelfEmpty()) {
    return PR_FALSE;
  }

  for (line_iterator line = begin_lines(), line_end = end_lines();
       line != line_end;
       ++line)
  {
    if (!line->IsEmpty())
      return PR_FALSE;
  }

  return PR_TRUE;
}

PRBool
nsBlockFrame::ShouldApplyTopMargin(nsBlockReflowState& aState,
                                   nsLineBox* aLine)
{
  if (aState.GetFlag(BRS_APPLYTOPMARGIN)) {
    
    return PR_TRUE;
  }

  if (!aState.IsAdjacentWithTop()) {
    
    
    
    aState.SetFlag(BRS_APPLYTOPMARGIN, PR_TRUE);
    return PR_TRUE;
  }

  
  line_iterator line = begin_lines();
  if (aState.GetFlag(BRS_HAVELINEADJACENTTOTOP)) {
    line = aState.mLineAdjacentToTop;
  }
  while (line != aLine) {
    if (!line->CachedIsEmpty() || line->HasClearance()) {
      
      
      aState.SetFlag(BRS_APPLYTOPMARGIN, PR_TRUE);
      return PR_TRUE;
    }
    
    
    ++line;
    aState.SetFlag(BRS_HAVELINEADJACENTTOTOP, PR_TRUE);
    aState.mLineAdjacentToTop = line;
  }

  
  
  
  return PR_FALSE;
}

nsIFrame*
nsBlockFrame::GetTopBlockChild(nsPresContext* aPresContext)
{
  if (mLines.empty())
    return nsnull;

  nsLineBox *firstLine = mLines.front();
  if (firstLine->IsBlock())
    return firstLine->mFirstChild;

  if (!firstLine->CachedIsEmpty())
    return nsnull;

  line_iterator secondLine = begin_lines();
  ++secondLine;
  if (secondLine == end_lines() || !secondLine->IsBlock())
    return nsnull;

  return secondLine->mFirstChild;
}

nsresult
nsBlockFrame::ReflowBlockFrame(nsBlockReflowState& aState,
                               line_iterator aLine,
                               PRBool* aKeepReflowGoing)
{
  NS_PRECONDITION(*aKeepReflowGoing, "bad caller");

  nsresult rv = NS_OK;

  nsIFrame* frame = aLine->mFirstChild;
  if (!frame) {
    NS_ASSERTION(PR_FALSE, "program error - unexpected empty line"); 
    return NS_ERROR_NULL_POINTER; 
  }

  
  const nsStyleDisplay* display = frame->GetStyleDisplay();
  nsBlockReflowContext brc(aState.mPresContext, aState.mReflowState);

  PRUint8 breakType = display->mBreakType;
  
  
  
  if (NS_STYLE_CLEAR_NONE != aState.mFloatBreakType) {
    breakType = nsLayoutUtils::CombineBreakType(breakType,
                                                aState.mFloatBreakType);
    aState.mFloatBreakType = NS_STYLE_CLEAR_NONE;
  }

  
  aLine->SetBreakTypeBefore(breakType);

  
  
  
  
  PRBool applyTopMargin =
    !frame->GetPrevInFlow() && ShouldApplyTopMargin(aState, aLine);

  if (applyTopMargin) {
    
    
    
    
    aLine->ClearHasClearance();
  }
  PRBool treatWithClearance = aLine->HasClearance();

  PRBool mightClearFloats = breakType != NS_STYLE_CLEAR_NONE;
  nsIFrame *replacedBlock = nsnull;
  if (!nsBlockFrame::BlockCanIntersectFloats(frame)) {
    mightClearFloats = PR_TRUE;
    replacedBlock = frame;
  }

  
  
  
  
  if (!treatWithClearance && !applyTopMargin && mightClearFloats &&
      aState.mReflowState.mDiscoveredClearance) {
    nscoord curY = aState.mY + aState.mPrevBottomMargin.get();
    nscoord clearY = aState.ClearFloats(curY, breakType, replacedBlock);
    if (clearY != curY) {
      
      
      
      
      treatWithClearance = PR_TRUE;
      
      if (!*aState.mReflowState.mDiscoveredClearance) {
        *aState.mReflowState.mDiscoveredClearance = frame;
      }
      
      
      return NS_OK;
    }
  }
  if (treatWithClearance) {
    applyTopMargin = PR_TRUE;
  }

  nsIFrame* clearanceFrame = nsnull;
  nscoord startingY = aState.mY;
  nsCollapsingMargin incomingMargin = aState.mPrevBottomMargin;
  nscoord clearance;
  
  
  nsPoint originalPosition = frame->GetPosition();
  while (PR_TRUE) {
    
    nscoord passOriginalY = frame->GetRect().y;
    
    clearance = 0;
    nscoord topMargin = 0;
    PRBool mayNeedRetry = PR_FALSE;
    if (applyTopMargin) {
      
      
      

      
      
      
      
      
      
      
      
      
      nsSize availSpace(aState.mContentArea.width, NS_UNCONSTRAINEDSIZE);
      nsHTMLReflowState reflowState(aState.mPresContext, aState.mReflowState,
                                    frame, availSpace);
      
      if (treatWithClearance) {
        aState.mY += aState.mPrevBottomMargin.get();
        aState.mPrevBottomMargin.Zero();
      }
      
      
      
      nsBlockReflowContext::ComputeCollapsedTopMargin(reflowState,
                                                      &aState.mPrevBottomMargin, clearanceFrame, &mayNeedRetry);
      
      
      
      
      if (clearanceFrame) {
        
        
        mayNeedRetry = PR_FALSE;
      }
      
      if (!treatWithClearance && !clearanceFrame && mightClearFloats) {
        
        
        
        
        
        
        nscoord curY = aState.mY + aState.mPrevBottomMargin.get();
        nscoord clearY = aState.ClearFloats(curY, breakType, replacedBlock);
        if (clearY != curY) {
          
          
          treatWithClearance = PR_TRUE;
          
          aLine->SetHasClearance();
          
          
          aState.mY += aState.mPrevBottomMargin.get();
          aState.mPrevBottomMargin.Zero();
          
          
          mayNeedRetry = PR_FALSE;
          nsBlockReflowContext::ComputeCollapsedTopMargin(reflowState,
                                                          &aState.mPrevBottomMargin, clearanceFrame, &mayNeedRetry);
        }
      }
      
      
      
      
      
      topMargin = aState.mPrevBottomMargin.get();
      
      if (treatWithClearance) {
        nscoord currentY = aState.mY;
        
        aState.mY = aState.ClearFloats(aState.mY, breakType, replacedBlock);
        
        
        
        
        
        
        
        clearance = aState.mY - (currentY + topMargin);
        
        
        
        topMargin += clearance;
        
        
        
      } else {
        
        aState.mY += topMargin;
      }
    }
    
    
    
    aState.GetAvailableSpace();
#ifdef REALLY_NOISY_REFLOW
    printf("setting line %p isImpacted to %s\n", aLine.get(), aState.IsImpactedByFloat()?"true":"false");
#endif
    PRBool isImpacted = aState.IsImpactedByFloat() ? PR_TRUE : PR_FALSE;
    aLine->SetLineIsImpactedByFloat(isImpacted);
    nsRect availSpace;
    aState.ComputeBlockAvailSpace(frame, display, replacedBlock != nsnull,
                                  availSpace);
    
    
    
    aState.mY -= topMargin;
    availSpace.y -= topMargin;
    if (NS_UNCONSTRAINEDSIZE != availSpace.height) {
      availSpace.height += topMargin;
    }
    
    
    
    
    nsHTMLReflowState blockHtmlRS(aState.mPresContext, aState.mReflowState, frame, 
                                  nsSize(availSpace.width, availSpace.height));
    blockHtmlRS.mFlags.mHasClearance = aLine->HasClearance();
    
    nsSpaceManager::SavedState spaceManagerState;
    if (mayNeedRetry) {
      blockHtmlRS.mDiscoveredClearance = &clearanceFrame;
      aState.mSpaceManager->PushState(&spaceManagerState);
    } else if (!applyTopMargin) {
      blockHtmlRS.mDiscoveredClearance = aState.mReflowState.mDiscoveredClearance;
    }
    
    nsReflowStatus frameReflowStatus = NS_FRAME_COMPLETE;
    rv = brc.ReflowBlock(availSpace, applyTopMargin, aState.mPrevBottomMargin,
                         clearance, aState.IsAdjacentWithTop(),
                         aLine.get(), blockHtmlRS, frameReflowStatus, aState);

    
    
    
    if (!mayNeedRetry && clearanceFrame &&
        frame->GetRect().y != passOriginalY) {
      Invalidate(frame->GetOverflowRect() + frame->GetPosition());
    }
    
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (mayNeedRetry && clearanceFrame) {
      aState.mSpaceManager->PopState(&spaceManagerState);
      aState.mY = startingY;
      aState.mPrevBottomMargin = incomingMargin;
      continue;
    }
    
    aState.mPrevChild = frame;
    
#if defined(REFLOW_STATUS_COVERAGE)
    RecordReflowStatus(PR_TRUE, frameReflowStatus);
#endif
    
    if (NS_INLINE_IS_BREAK_BEFORE(frameReflowStatus)) {
      
      PushLines(aState, aLine.prev());
      *aKeepReflowGoing = PR_FALSE;
      NS_FRAME_SET_INCOMPLETE(aState.mReflowStatus);
    }
    else {
      
      
      
      
      
      
      
      
      PRBool forceFit = aState.IsAdjacentWithTop() && clearance <= 0 &&
        !isImpacted;
      nsCollapsingMargin collapsedBottomMargin;
      nsRect combinedArea(0,0,0,0);
      *aKeepReflowGoing = brc.PlaceBlock(blockHtmlRS, forceFit, aLine.get(),
                                         collapsedBottomMargin,
                                         aLine->mBounds, combinedArea, frameReflowStatus);
      if (aLine->SetCarriedOutBottomMargin(collapsedBottomMargin)) {
        line_iterator nextLine = aLine;
        ++nextLine;
        if (nextLine != end_lines()) {
          nextLine->MarkPreviousMarginDirty();
        }
      }
      
      aLine->SetCombinedArea(combinedArea);
      if (*aKeepReflowGoing) {
        
        
        
        nscoord newY = aLine->mBounds.YMost();
        aState.mY = newY;
        
        
        
        if (!NS_FRAME_IS_FULLY_COMPLETE(frameReflowStatus)) {
          PRBool madeContinuation;
          rv = CreateContinuationFor(aState, nsnull, frame, madeContinuation);
          NS_ENSURE_SUCCESS(rv, rv);
          
          nsIFrame* nextFrame = frame->GetNextInFlow();
          NS_ASSERTION(nextFrame, "We're supposed to have a next-in-flow by now");
          
          if (NS_FRAME_IS_NOT_COMPLETE(frameReflowStatus)) {
            
            if (!madeContinuation &&
                (NS_FRAME_IS_OVERFLOW_CONTAINER & nextFrame->GetStateBits())) {
              aState.mOverflowTracker.Finish(frame);
              nsContainerFrame* parent =
                static_cast<nsContainerFrame*>(nextFrame->GetParent());
              rv = parent->StealFrame(aState.mPresContext, nextFrame);
              NS_ENSURE_SUCCESS(rv, rv);
              if (parent != this)
                ReparentFrame(nextFrame, parent, this);
              nextFrame->SetNextSibling(frame->GetNextSibling());
              frame->SetNextSibling(nextFrame);
              madeContinuation = PR_TRUE; 
              nextFrame->RemoveStateBits(NS_FRAME_IS_OVERFLOW_CONTAINER);
              frameReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
            }

            
            if (madeContinuation) {
              nsLineBox* line = aState.NewLineBox(nextFrame, 1, PR_TRUE);
              NS_ENSURE_TRUE(line, NS_ERROR_OUT_OF_MEMORY);
              mLines.after_insert(aLine, line);
            }

            PushLines(aState, aLine);
            NS_FRAME_SET_INCOMPLETE(aState.mReflowStatus);

            
            
            if (frameReflowStatus & NS_FRAME_REFLOW_NEXTINFLOW) {
              aState.mReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
              
              
              
              
              
              
              
              if (!madeContinuation) {
                nsBlockFrame* nifBlock =
                  nsLayoutUtils::GetAsBlock(nextFrame->GetParent());
                NS_ASSERTION(nifBlock,
                             "A block's child's next in flow's parent must be a block!");
                for (line_iterator line = nifBlock->begin_lines(),
                     line_end = nifBlock->end_lines(); line != line_end; ++line) {
                  if (line->Contains(nextFrame)) {
                    line->MarkDirty();
                    break;
                  }
                }
              }
            }
            *aKeepReflowGoing = PR_FALSE;
            
            
            
            
            
#ifdef NOISY_VERTICAL_MARGINS
            ListTag(stdout);
            printf(": reflow incomplete, frame=");
            nsFrame::ListTag(stdout, frame);
            printf(" prevBottomMargin=%d, setting to zero\n",
                   aState.mPrevBottomMargin);
#endif
            aState.mPrevBottomMargin.Zero();
          }
          else { 
            
            if (!madeContinuation &&
                !(NS_FRAME_IS_OVERFLOW_CONTAINER & nextFrame->GetStateBits())) {
              
              
              nsContainerFrame* parent = static_cast<nsContainerFrame*>
                                           (nextFrame->GetParent());
              rv = parent->StealFrame(aState.mPresContext, nextFrame);
              NS_ENSURE_SUCCESS(rv, rv);
            }
            else if (madeContinuation) {
              frame->SetNextSibling(nextFrame->GetNextSibling());
              nextFrame->SetNextSibling(nsnull);
            }

            
            aState.mOverflowTracker.Insert(nextFrame, frameReflowStatus);
            NS_MergeReflowStatusInto(&aState.mReflowStatus, frameReflowStatus);

#ifdef NOISY_VERTICAL_MARGINS
            ListTag(stdout);
            printf(": reflow complete but overflow incomplete for ");
            nsFrame::ListTag(stdout, frame);
            printf(" prevBottomMargin=%d collapsedBottomMargin=%d\n",
                   aState.mPrevBottomMargin, collapsedBottomMargin.get());
#endif
            aState.mPrevBottomMargin = collapsedBottomMargin;
          }
        }
        else { 
#ifdef NOISY_VERTICAL_MARGINS
          ListTag(stdout);
          printf(": reflow complete for ");
          nsFrame::ListTag(stdout, frame);
          printf(" prevBottomMargin=%d collapsedBottomMargin=%d\n",
                 aState.mPrevBottomMargin, collapsedBottomMargin.get());
#endif
          aState.mPrevBottomMargin = collapsedBottomMargin;
        }
#ifdef NOISY_VERTICAL_MARGINS
        ListTag(stdout);
        printf(": frame=");
        nsFrame::ListTag(stdout, frame);
        printf(" carriedOutBottomMargin=%d collapsedBottomMargin=%d => %d\n",
               brc.GetCarriedOutBottomMargin(), collapsedBottomMargin.get(),
               aState.mPrevBottomMargin);
#endif
      }
      else {
        
        if (aLine == mLines.front()) {
          
          
          
          aState.mReflowStatus = NS_INLINE_LINE_BREAK_BEFORE();
        }
        else {
          
          
          PushLines(aState, aLine.prev());
          NS_FRAME_SET_INCOMPLETE(aState.mReflowStatus);
        }
      }
    }
    break; 
  }

  
  
  
  
  if (originalPosition != frame->GetPosition() && !frame->HasView()) {
    nsContainerFrame::PositionChildViews(frame);
  }
  
#ifdef DEBUG
  VerifyLines(PR_TRUE);
#endif
  return rv;
}

nsresult
nsBlockFrame::ReflowInlineFrames(nsBlockReflowState& aState,
                                 line_iterator aLine,
                                 PRBool* aKeepReflowGoing)
{
  nsresult rv = NS_OK;
  *aKeepReflowGoing = PR_TRUE;

#ifdef DEBUG
  PRInt32 spins = 0;
#endif
  LineReflowStatus lineReflowStatus = LINE_REFLOW_REDO_NEXT_BAND;
  PRBool movedPastFloat = PR_FALSE;
  do {
    PRBool allowPullUp = PR_TRUE;
    nsIContent* forceBreakInContent = nsnull;
    PRInt32 forceBreakOffset = -1;
    gfxBreakPriority forceBreakPriority = eNoBreak;
    do {
      nsSpaceManager::SavedState spaceManagerState;
      aState.mReflowState.mSpaceManager->PushState(&spaceManagerState);

      
      
      
      
      
      
      
      nsLineLayout lineLayout(aState.mPresContext,
                              aState.mReflowState.mSpaceManager,
                              &aState.mReflowState, &aLine);
      lineLayout.Init(&aState, aState.mMinLineHeight, aState.mLineNumber);
      if (forceBreakInContent) {
        lineLayout.ForceBreakAtPosition(forceBreakInContent, forceBreakOffset);
      }
      rv = DoReflowInlineFrames(aState, lineLayout, aLine,
                                aKeepReflowGoing, &lineReflowStatus,
                                allowPullUp);
      lineLayout.EndLineReflow();

      if (LINE_REFLOW_REDO_NO_PULL == lineReflowStatus ||
          LINE_REFLOW_REDO_NEXT_BAND == lineReflowStatus) {
        if (lineLayout.NeedsBackup()) {
          NS_ASSERTION(!forceBreakInContent, "Backing up twice; this should never be necessary");
          
          
          
          forceBreakInContent = lineLayout.GetLastOptionalBreakPosition(&forceBreakOffset, &forceBreakPriority);
        } else {
          forceBreakInContent = nsnull;
        }
        
        aState.mReflowState.mSpaceManager->PopState(&spaceManagerState);
        
        aState.mCurrentLineFloats.DeleteAll();
        aState.mBelowCurrentLineFloats.DeleteAll();
      }
      
#ifdef DEBUG
      spins++;
      if (1000 == spins) {
        ListTag(stdout);
        printf(": yikes! spinning on a line over 1000 times!\n");
        NS_ABORT();
      }
#endif

      
      allowPullUp = PR_FALSE;
    } while (NS_SUCCEEDED(rv) && LINE_REFLOW_REDO_NO_PULL == lineReflowStatus);

    if (LINE_REFLOW_REDO_NEXT_BAND == lineReflowStatus) {
      movedPastFloat = PR_TRUE;
    }
  } while (NS_SUCCEEDED(rv) && LINE_REFLOW_REDO_NEXT_BAND == lineReflowStatus);

  
  
  if (movedPastFloat) {
    aLine->SetLineIsImpactedByFloat(PR_TRUE);
  }

  return rv;
}





void
nsBlockFrame::PushTruncatedPlaceholderLine(nsBlockReflowState& aState,
                                           line_iterator       aLine,
                                           PRBool&             aKeepReflowGoing)
{
  line_iterator prevLine = aLine;
  --prevLine;
  PushLines(aState, prevLine);
  aKeepReflowGoing = PR_FALSE;
  NS_FRAME_SET_INCOMPLETE(aState.mReflowStatus);
}

#ifdef DEBUG
static const char* LineReflowStatusNames[] = {
  "LINE_REFLOW_OK", "LINE_REFLOW_STOP", "LINE_REFLOW_REDO_NO_PULL",
  "LINE_REFLOW_REDO_NEXT_BAND", "LINE_REFLOW_TRUNCATED"
};
#endif

nsresult
nsBlockFrame::DoReflowInlineFrames(nsBlockReflowState& aState,
                                   nsLineLayout& aLineLayout,
                                   line_iterator aLine,
                                   PRBool* aKeepReflowGoing,
                                   LineReflowStatus* aLineReflowStatus,
                                   PRBool aAllowPullUp)
{
  
  aLine->FreeFloats(aState.mFloatCacheFreeList);
  aState.mFloatCombinedArea.SetRect(0, 0, 0, 0);

  
  
  if (ShouldApplyTopMargin(aState, aLine)) {
    aState.mY += aState.mPrevBottomMargin.get();
  }
  aState.GetAvailableSpace();
  PRBool impactedByFloats = aState.IsImpactedByFloat() ? PR_TRUE : PR_FALSE;
  aLine->SetLineIsImpactedByFloat(impactedByFloats);
#ifdef REALLY_NOISY_REFLOW
  printf("nsBlockFrame::DoReflowInlineFrames %p impacted = %d\n",
         this, impactedByFloats);
#endif

  const nsMargin& borderPadding = aState.BorderPadding();
  nscoord x = aState.mAvailSpaceRect.x + borderPadding.left;
  nscoord availWidth = aState.mAvailSpaceRect.width;
  nscoord availHeight;
  if (aState.GetFlag(BRS_UNCONSTRAINEDHEIGHT)) {
    availHeight = NS_UNCONSTRAINEDSIZE;
  }
  else {
    
    availHeight = aState.mAvailSpaceRect.height;
  }

  
  
  aLine->EnableResizeReflowOptimization();

  aLineLayout.BeginLineReflow(x, aState.mY,
                              availWidth, availHeight,
                              impactedByFloats,
                              PR_FALSE );

  aState.SetFlag(BRS_LINE_LAYOUT_EMPTY, PR_FALSE);

  
  
  if ((0 == aLineLayout.GetLineNumber()) &&
      (NS_BLOCK_HAS_FIRST_LETTER_CHILD & mState) &&
      (NS_BLOCK_HAS_FIRST_LETTER_STYLE & mState)) {
    aLineLayout.SetFirstLetterStyleOK(PR_TRUE);
  }

  
  nsresult rv = NS_OK;
  LineReflowStatus lineReflowStatus = LINE_REFLOW_OK;
  PRInt32 i;
  nsIFrame* frame = aLine->mFirstChild;

  
  
  PRBool isContinuingPlaceholders = PR_FALSE;

  if (impactedByFloats) {
    
    
    if (aLineLayout.NotifyOptionalBreakPosition(frame->GetContent(), 0, PR_TRUE, eNormalBreak)) {
      lineReflowStatus = LINE_REFLOW_REDO_NEXT_BAND;
    }
  }

  
  
  for (i = 0; LINE_REFLOW_OK == lineReflowStatus && i < aLine->GetChildCount();
       i++, frame = frame->GetNextSibling()) {
    if (IsContinuationPlaceholder(frame)) {
      isContinuingPlaceholders = PR_TRUE;
    }
    rv = ReflowInlineFrame(aState, aLineLayout, aLine, frame,
                           &lineReflowStatus);
    NS_ENSURE_SUCCESS(rv, rv);
    if (LINE_REFLOW_OK != lineReflowStatus) {
      
      
      
      ++aLine;
      while ((aLine != end_lines()) && (0 == aLine->GetChildCount())) {
        
        
        nsLineBox *toremove = aLine;
        aLine = mLines.erase(aLine);
        NS_ASSERTION(nsnull == toremove->mFirstChild, "bad empty line");
        aState.FreeLineBox(toremove);
      }
      --aLine;

      if (LINE_REFLOW_TRUNCATED == lineReflowStatus) {
        
        PushTruncatedPlaceholderLine(aState, aLine, *aKeepReflowGoing);
      }
    }
  }

  
  if (!isContinuingPlaceholders && aAllowPullUp) {
    
    while (LINE_REFLOW_OK == lineReflowStatus) {
      rv = PullFrame(aState, aLine, frame);
      NS_ENSURE_SUCCESS(rv, rv);
      if (nsnull == frame) {
        break;
      }

      while (LINE_REFLOW_OK == lineReflowStatus) {
        PRInt32 oldCount = aLine->GetChildCount();
        rv = ReflowInlineFrame(aState, aLineLayout, aLine, frame,
                               &lineReflowStatus);
        NS_ENSURE_SUCCESS(rv, rv);
        if (aLine->GetChildCount() != oldCount) {
          
          
          
          
          frame = frame->GetNextSibling();
        }
        else {
          break;
        }
      }
    }
  }

  aState.SetFlag(BRS_LINE_LAYOUT_EMPTY, aLineLayout.LineIsEmpty());

  
  PRBool needsBackup = aLineLayout.NeedsBackup() &&
    (lineReflowStatus == LINE_REFLOW_STOP || lineReflowStatus == LINE_REFLOW_OK);
  if (needsBackup && aLineLayout.HaveForcedBreakPosition()) {
  	NS_WARNING("We shouldn't be backing up more than once! "
               "Someone must have set a break opportunity beyond the available width, "
               "even though there were better break opportunities before it");
    needsBackup = PR_FALSE;
  }
  if (needsBackup) {
    
    PRInt32 offset;
    gfxBreakPriority breakPriority;
    nsIContent* breakContent = aLineLayout.GetLastOptionalBreakPosition(&offset, &breakPriority);
    
    
    
    if (breakContent) {
      
      lineReflowStatus = LINE_REFLOW_REDO_NO_PULL;
    }
  } else {
    
    
    aLineLayout.ClearOptionalBreakPosition();
  }

  if (LINE_REFLOW_REDO_NEXT_BAND == lineReflowStatus) {
    
    
    
    
    
    
    NS_ASSERTION(NS_UNCONSTRAINEDSIZE != aState.mAvailSpaceRect.height,
                 "unconstrained height on totally empty line");

    
    if (aState.mAvailSpaceRect.height > 0) {
      NS_ASSERTION(aState.IsImpactedByFloat(),
                   "redo line on totally empty line with non-empty band...");
      aState.mY += aState.mAvailSpaceRect.height;
    } else {
      NS_ASSERTION(NS_UNCONSTRAINEDSIZE != aState.mReflowState.availableHeight,
                   "We shouldn't be running out of height here");
      if (NS_UNCONSTRAINEDSIZE == aState.mReflowState.availableHeight) {
        
        aState.mY += 1;
      } else {
        
        
        
        lineReflowStatus = LINE_REFLOW_TRUNCATED;
        
        PushTruncatedPlaceholderLine(aState, aLine, *aKeepReflowGoing);
      }
    }
      
    
    
    
    
    aState.mPrevBottomMargin.Zero();

    
    
    
    
  }
  else if (LINE_REFLOW_REDO_NO_PULL == lineReflowStatus) {
    
    
    
    
    aState.mPrevBottomMargin.Zero();
  }
  else if (LINE_REFLOW_TRUNCATED != lineReflowStatus) {
    
    
    if (!NS_INLINE_IS_BREAK_BEFORE(aState.mReflowStatus)) {
      PlaceLine(aState, aLineLayout, aLine, aKeepReflowGoing);
    }
  }
#ifdef DEBUG
  if (gNoisyReflow) {
    printf("Line reflow status = %s\n", LineReflowStatusNames[lineReflowStatus]);
  }
#endif
  *aLineReflowStatus = lineReflowStatus;

  return rv;
}









nsresult
nsBlockFrame::ReflowInlineFrame(nsBlockReflowState& aState,
                                nsLineLayout& aLineLayout,
                                line_iterator aLine,
                                nsIFrame* aFrame,
                                LineReflowStatus* aLineReflowStatus)
{
  NS_ENSURE_ARG_POINTER(aFrame);
  
  *aLineReflowStatus = LINE_REFLOW_OK;

#ifdef NOISY_FIRST_LETTER
  ListTag(stdout);
  printf(": reflowing ");
  nsFrame::ListTag(stdout, aFrame);
  printf(" reflowingFirstLetter=%s\n",
         aLineLayout.GetFirstLetterStyleOK() ? "on" : "off");
#endif

  
  nsReflowStatus frameReflowStatus;
  PRBool         pushedFrame;
  nsresult rv = aLineLayout.ReflowFrame(aFrame, frameReflowStatus,
                                        nsnull, pushedFrame);
  NS_ENSURE_SUCCESS(rv, rv);

  if (frameReflowStatus & NS_FRAME_REFLOW_NEXTINFLOW) {
    
    aState.mReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
    nsBlockFrame* ourNext = static_cast<nsBlockFrame*>(GetNextInFlow());
    if (ourNext && aFrame->GetNextInFlow()) {
      PRBool isValid;
      nsBlockInFlowLineIterator iter(ourNext, aFrame->GetNextInFlow(), &isValid);
      if (isValid) {
        iter.GetLine()->MarkDirty();
      }
    }
  }

  NS_ENSURE_SUCCESS(rv, rv);
#ifdef REALLY_NOISY_REFLOW_CHILD
  nsFrame::ListTag(stdout, aFrame);
  printf(": status=%x\n", frameReflowStatus);
#endif

#if defined(REFLOW_STATUS_COVERAGE)
  RecordReflowStatus(PR_FALSE, frameReflowStatus);
#endif

  
  aState.mPrevChild = aFrame;

   






  
  
  
  
  
  aLine->SetBreakTypeAfter(NS_STYLE_CLEAR_NONE);
  if (NS_INLINE_IS_BREAK(frameReflowStatus) || 
      (NS_STYLE_CLEAR_NONE != aState.mFloatBreakType)) {
    
    
    *aLineReflowStatus = LINE_REFLOW_STOP;

    
    PRUint8 breakType = NS_INLINE_GET_BREAK_TYPE(frameReflowStatus);
    NS_ASSERTION((NS_STYLE_CLEAR_NONE != breakType) || 
                 (NS_STYLE_CLEAR_NONE != aState.mFloatBreakType), "bad break type");
    NS_ASSERTION(NS_STYLE_CLEAR_PAGE != breakType, "no page breaks yet");

    if (NS_INLINE_IS_BREAK_BEFORE(frameReflowStatus)) {
      
      if (aFrame == aLine->mFirstChild) {
        
        
        
        
        *aLineReflowStatus = LINE_REFLOW_REDO_NEXT_BAND;
      }
      else {
        
        
        rv = SplitLine(aState, aLineLayout, aLine, aFrame, aLineReflowStatus);
        NS_ENSURE_SUCCESS(rv, rv);

        
        
        
        if (pushedFrame) {
          aLine->SetLineWrapped(PR_TRUE);
        }
      }
    }
    else {
      
      
      
      if (NS_STYLE_CLEAR_NONE != aState.mFloatBreakType) {
        breakType = nsLayoutUtils::CombineBreakType(breakType,
                                                    aState.mFloatBreakType);
        aState.mFloatBreakType = NS_STYLE_CLEAR_NONE;
      }
      
      if (breakType == NS_STYLE_CLEAR_LINE) {
        if (!aLineLayout.GetLineEndsInBR()) {
          breakType = NS_STYLE_CLEAR_NONE;
        }
      }
      aLine->SetBreakTypeAfter(breakType);
      if (NS_FRAME_IS_COMPLETE(frameReflowStatus)) {
        
        rv = SplitLine(aState, aLineLayout, aLine, aFrame->GetNextSibling(), aLineReflowStatus);
        NS_ENSURE_SUCCESS(rv, rv);

        if (NS_INLINE_IS_BREAK_AFTER(frameReflowStatus) &&
            !aLineLayout.GetLineEndsInBR()) {
          
          
          nsLineList_iterator next = aLine.next();
          if (next != end_lines() && !next->IsBlock()) {
            next->MarkDirty();
          }
        }
      }
    }
  }
  else if (NS_FRAME_IS_TRUNCATED(frameReflowStatus) &&
           nsGkAtoms::placeholderFrame == aFrame->GetType()) {
    
    
    *aLineReflowStatus = LINE_REFLOW_TRUNCATED;
  }

  if (NS_FRAME_IS_NOT_COMPLETE(frameReflowStatus)) {
    
    
    nsIAtom* frameType = aFrame->GetType();

    PRBool madeContinuation;
    rv = (nsGkAtoms::placeholderFrame == frameType)
         ? SplitPlaceholder(aState, aFrame)
         : CreateContinuationFor(aState, aLine, aFrame, madeContinuation);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (!aLineLayout.GetLineEndsInBR()) {
      aLine->SetLineWrapped(PR_TRUE);
    }
    
    
    
    if (!(frameReflowStatus & NS_INLINE_BREAK_FIRST_LETTER_COMPLETE) && 
        nsGkAtoms::placeholderFrame != frameType) {
      
      *aLineReflowStatus = LINE_REFLOW_STOP;
      rv = SplitLine(aState, aLineLayout, aLine, aFrame->GetNextSibling(), aLineReflowStatus);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      nsLineList_iterator next = aLine.next();
      if (next != end_lines() && !next->IsBlock()) {
        next->MarkDirty();
      }
    }
  }

  return NS_OK;
}





nsresult
nsBlockFrame::CreateContinuationFor(nsBlockReflowState& aState,
                                    nsLineBox*          aLine,
                                    nsIFrame*           aFrame,
                                    PRBool&             aMadeNewFrame)
{
  aMadeNewFrame = PR_FALSE;
  nsresult rv;
  nsIFrame* nextInFlow;
  rv = CreateNextInFlow(aState.mPresContext, this, aFrame, nextInFlow);
  NS_ENSURE_SUCCESS(rv, rv);
  if (nsnull != nextInFlow) {
    aMadeNewFrame = PR_TRUE;
    if (aLine) { 
      aLine->SetChildCount(aLine->GetChildCount() + 1);
    }
  }
#ifdef DEBUG
  VerifyLines(PR_FALSE);
#endif
  return rv;
}

nsresult
nsBlockFrame::SplitPlaceholder(nsBlockReflowState& aState,
                               nsIFrame*           aPlaceholder)
{
  nsIFrame* nextInFlow;
  nsresult rv = CreateNextInFlow(aState.mPresContext, this, aPlaceholder, nextInFlow);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!nextInFlow) {
    
    return NS_OK;
  }

  
  nsIFrame *contFrame = aPlaceholder->GetNextSibling();
  nsIFrame *next = contFrame->GetNextSibling();
  aPlaceholder->SetNextSibling(next);
  contFrame->SetNextSibling(nsnull);
  
  NS_ASSERTION(IsContinuationPlaceholder(contFrame),
               "Didn't create the right kind of frame");

  
  
  aState.mOverflowPlaceholders.AppendFrame(this, contFrame);
  return NS_OK;
}

static nsFloatCache*
GetLastFloat(nsLineBox* aLine)
{
  nsFloatCache* fc = aLine->GetFirstFloat();
  while (fc && fc->Next()) {
    fc = fc->Next();
  }
  return fc;
}

static PRBool
CheckPlaceholderInLine(nsIFrame* aBlock, nsLineBox* aLine, nsFloatCache* aFC)
{
  if (!aFC)
    return PR_TRUE;
  for (nsIFrame* f = aFC->mPlaceholder; f; f = f->GetParent()) {
    if (f->GetParent() == aBlock)
      return aLine->Contains(f);
  }
  NS_ASSERTION(PR_FALSE, "aBlock is not an ancestor of aFrame!");
  return PR_TRUE;
}

nsresult
nsBlockFrame::SplitLine(nsBlockReflowState& aState,
                        nsLineLayout& aLineLayout,
                        line_iterator aLine,
                        nsIFrame* aFrame,
                        LineReflowStatus* aLineReflowStatus)
{
  NS_ABORT_IF_FALSE(aLine->IsInline(), "illegal SplitLine on block line");

  PRInt32 pushCount = aLine->GetChildCount() - aLineLayout.GetCurrentSpanCount();
  NS_ABORT_IF_FALSE(pushCount >= 0, "bad push count"); 

#ifdef DEBUG
  if (gNoisyReflow) {
    nsFrame::IndentBy(stdout, gNoiseIndent);
    printf("split line: from line=%p pushCount=%d aFrame=",
           static_cast<void*>(aLine.get()), pushCount);
    if (aFrame) {
      nsFrame::ListTag(stdout, aFrame);
    }
    else {
      printf("(null)");
    }
    printf("\n");
    if (gReallyNoisyReflow) {
      aLine->List(stdout, gNoiseIndent+1);
    }
  }
#endif

  if (0 != pushCount) {
    NS_ABORT_IF_FALSE(aLine->GetChildCount() > pushCount, "bad push");
    NS_ABORT_IF_FALSE(nsnull != aFrame, "whoops");
    NS_ASSERTION(nsFrameList(aFrame).GetLength() >= pushCount,
                 "Not enough frames to push");

    
    nsLineBox* newLine = aState.NewLineBox(aFrame, pushCount, PR_FALSE);
    if (!newLine) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mLines.after_insert(aLine, newLine);
    aLine->SetChildCount(aLine->GetChildCount() - pushCount);
#ifdef DEBUG
    if (gReallyNoisyReflow) {
      newLine->List(stdout, gNoiseIndent+1);
    }
#endif

    
    
    aLineLayout.SplitLineTo(aLine->GetChildCount());

    
    
    
    
    
    if (!CheckPlaceholderInLine(this, aLine, GetLastFloat(aLine)) ||
        !CheckPlaceholderInLine(this, aLine, aState.mBelowCurrentLineFloats.Tail())) {
      *aLineReflowStatus = LINE_REFLOW_REDO_NO_PULL;
    }

#ifdef DEBUG
    VerifyLines(PR_TRUE);
#endif
  }
  return NS_OK;
}

PRBool
nsBlockFrame::ShouldJustifyLine(nsBlockReflowState& aState,
                                line_iterator aLine)
{
  while (++aLine != end_lines()) {
    
    if (0 != aLine->GetChildCount()) {
      
      
      
      return !aLine->IsBlock();
    }
    
  }

  
  
  nsBlockFrame* nextInFlow = (nsBlockFrame*) GetNextInFlow();
  while (nsnull != nextInFlow) {
    for (line_iterator line = nextInFlow->begin_lines(),
                   line_end = nextInFlow->end_lines();
         line != line_end;
         ++line)
    {
      if (0 != line->GetChildCount())
        return !line->IsBlock();
    }
    nextInFlow = (nsBlockFrame*) nextInFlow->GetNextInFlow();
  }

  
  return PR_FALSE;
}

void
nsBlockFrame::PlaceLine(nsBlockReflowState& aState,
                        nsLineLayout&       aLineLayout,
                        line_iterator       aLine,
                        PRBool*             aKeepReflowGoing)
{
  
  aLineLayout.TrimTrailingWhiteSpace();

  
  
  
  
  
  
  
  
  
  PRBool addedBullet = PR_FALSE;
  if (mBullet && HaveOutsideBullet() &&
      ((aLine == mLines.front() &&
        (!aLineLayout.IsZeroHeight() || (aLine == mLines.back()))) ||
       (mLines.front() != mLines.back() &&
        0 == mLines.front()->mBounds.height &&
        aLine == mLines.begin().next()))) {
    nsHTMLReflowMetrics metrics;
    ReflowBullet(aState, metrics, aState.mY);
    aLineLayout.AddBulletFrame(mBullet, metrics);
    addedBullet = PR_TRUE;
  }
  aLineLayout.VerticalAlignLine();

#ifdef DEBUG
  {
    static nscoord lastHeight = 0;
    if (CRAZY_HEIGHT(aLine->mBounds.y)) {
      lastHeight = aLine->mBounds.y;
      if (abs(aLine->mBounds.y - lastHeight) > CRAZY_H/10) {
        nsFrame::ListTag(stdout);
        printf(": line=%p y=%d line.bounds.height=%d\n",
               static_cast<void*>(aLine.get()),
               aLine->mBounds.y, aLine->mBounds.height);
      }
    }
    else {
      lastHeight = 0;
    }
  }
#endif

  
  
  
  const nsStyleText* styleText = GetStyleText();
  PRBool allowJustify = NS_STYLE_TEXT_ALIGN_JUSTIFY == styleText->mTextAlign &&
                        !aLineLayout.GetLineEndsInBR() &&
                        ShouldJustifyLine(aState, aLine);
  aLineLayout.HorizontalAlignFrames(aLine->mBounds, allowJustify);
  
  
  
  
#ifdef IBMBIDI
  
  if (aState.mPresContext->BidiEnabled()) {
    if (!aState.mPresContext->IsVisualMode()) {
      nsBidiPresUtils* bidiUtils = aState.mPresContext->GetBidiUtils();

      if (bidiUtils && bidiUtils->IsSuccessful() ) {
        bidiUtils->ReorderFrames(aLine->mFirstChild, aLine->GetChildCount());
      } 
    } 
  } 
#endif 

  
  
  nsRect combinedArea;
  aLineLayout.RelativePositionFrames(combinedArea);  
  aLine->SetCombinedArea(combinedArea);
  if (addedBullet) {
    aLineLayout.RemoveBulletFrame(mBullet);
  }

  
  
  
  
  
  
  nscoord newY;

  if (!aLine->CachedIsEmpty()) {
    
    
    aState.mPrevBottomMargin.Zero();
    newY = aLine->mBounds.YMost();
  }
  else {
    
    
    
    
    
    nscoord dy = aState.GetFlag(BRS_APPLYTOPMARGIN)
                   ? -aState.mPrevBottomMargin.get() : 0;
    newY = aState.mY + dy;
  }

  
  
  if (mLines.front() != aLine &&
      newY > aState.mBottomEdge &&
      aState.mBottomEdge != NS_UNCONSTRAINEDSIZE) {
    
    
    NS_ASSERTION((aState.mCurrentLine == aLine), "oops");
    PushLines(aState, aLine.prev());

    
    
    if (*aKeepReflowGoing) {
      NS_FRAME_SET_INCOMPLETE(aState.mReflowStatus);
      *aKeepReflowGoing = PR_FALSE;
    }
    return;
  }

  
  PRBool wasAdjacentWIthTop = aState.IsAdjacentWithTop();

  aState.mY = newY;
  
  
  aLine->AppendFloats(aState.mCurrentLineFloats);

  
  if (aState.mBelowCurrentLineFloats.NotEmpty()) {
    
    
    if (aState.PlaceBelowCurrentLineFloats(aState.mBelowCurrentLineFloats,
                                           wasAdjacentWIthTop)) {
      aLine->AppendFloats(aState.mBelowCurrentLineFloats);
    }
    else { 
      
      
      
      PushTruncatedPlaceholderLine(aState, aLine, *aKeepReflowGoing);
    }
  }

  
  
  if (aLine->HasFloats()) {
    
    
    nsRect lineCombinedArea(aLine->GetCombinedArea());
#ifdef NOISY_COMBINED_AREA
    ListTag(stdout);
    printf(": lineCA=%d,%d,%d,%d floatCA=%d,%d,%d,%d\n",
           lineCombinedArea.x, lineCombinedArea.y,
           lineCombinedArea.width, lineCombinedArea.height,
           aState.mFloatCombinedArea.x, aState.mFloatCombinedArea.y,
           aState.mFloatCombinedArea.width,
           aState.mFloatCombinedArea.height);
#endif
    lineCombinedArea.UnionRect(aState.mFloatCombinedArea, lineCombinedArea);

    aLine->SetCombinedArea(lineCombinedArea);
#ifdef NOISY_COMBINED_AREA
    printf("  ==> final lineCA=%d,%d,%d,%d\n",
           lineCombinedArea.x, lineCombinedArea.y,
           lineCombinedArea.width, lineCombinedArea.height);
#endif
  }

  
  
  if (aLine->HasFloatBreakAfter()) {
    aState.mY = aState.ClearFloats(aState.mY, aLine->GetBreakTypeAfter());
  }
}

void
nsBlockFrame::PushLines(nsBlockReflowState&  aState,
                        nsLineList::iterator aLineBefore)
{
  nsLineList::iterator overBegin(aLineBefore.next());

  
  PRBool firstLine = overBegin == begin_lines();

  if (overBegin != end_lines()) {
    
    nsFrameList floats;
    nsIFrame* tail = nsnull;
    CollectFloats(overBegin->mFirstChild, floats, &tail, PR_FALSE, PR_TRUE);

    if (floats.NotEmpty()) {
      
      nsFrameList oofs = GetOverflowOutOfFlows();
      if (oofs.NotEmpty()) {
        floats.InsertFrames(nsnull, tail, oofs.FirstChild());
      }
      SetOverflowOutOfFlows(floats);
    }

    
    
    
    
    
    nsLineList* overflowLines = RemoveOverflowLines();
    if (!overflowLines) {
      
      overflowLines = new nsLineList();
    }
    if (overflowLines) {
      if (!overflowLines->empty()) {
        mLines.back()->LastChild()->SetNextSibling(overflowLines->front()->mFirstChild);
      }
      overflowLines->splice(overflowLines->begin(), mLines, overBegin,
                            end_lines());
      NS_ASSERTION(!overflowLines->empty(), "should not be empty");
      
      
      SetOverflowLines(overflowLines);
  
      
      

      
      for (line_iterator line = overflowLines->begin(),
             line_end = overflowLines->end();
           line != line_end;
           ++line)
        {
          line->MarkDirty();
          line->MarkPreviousMarginDirty();
          line->mBounds.SetRect(0, 0, 0, 0);
          if (line->HasFloats()) {
            line->FreeFloats(aState.mFloatCacheFreeList);
          }
        }
    }
  }

  
  if (!firstLine)
    aLineBefore->LastChild()->SetNextSibling(nsnull);

#ifdef DEBUG
  VerifyOverflowSituation();
#endif
}
















PRBool
nsBlockFrame::HandleOverflowPlaceholdersForPulledFrame(
  nsBlockReflowState& aState, nsIFrame* aFrame)
{
  if (nsGkAtoms::placeholderFrame != aFrame->GetType()) {
    
    
    
    if (!aFrame->IsFloatContainingBlock()) {
      for (nsIFrame* f = aFrame->GetFirstChild(nsnull); f; f = f->GetNextSibling()) {
#ifdef DEBUG
        PRBool changed =
#endif
          HandleOverflowPlaceholdersForPulledFrame(aState, f);
        NS_ASSERTION(!changed, "Shouldn't find any continuation placeholders inside inlines");
      }
    }
    return PR_FALSE;
  }

  PRBool taken = PR_TRUE;
  nsIFrame* frame = aFrame;
  if (!aFrame->GetPrevInFlow()) {
    
    
    taken = PR_FALSE;
    frame = frame->GetNextInFlow();
    if (!frame)
      return PR_FALSE;
  }

  nsBlockFrame* parent = static_cast<nsBlockFrame*>(frame->GetParent());
  
  
#ifdef DEBUG
  nsresult rv =
#endif
    parent->DoRemoveFrame(frame, PERSERVE_REMOVED_FRAMES);
  NS_ASSERTION(NS_SUCCEEDED(rv), "frame should be in parent's lists");
  
  nsIFrame* lastOverflowPlace = aState.mOverflowPlaceholders.LastChild();
  while (frame) {
    NS_ASSERTION(IsContinuationPlaceholder(frame),
                 "Should only be dealing with continuation placeholders here");

    parent = static_cast<nsBlockFrame*>(frame->GetParent());
    ReparentFrame(frame, parent, this);

    
    nsIFrame* outOfFlow = nsPlaceholderFrame::GetRealFrameForPlaceholder(frame);

    if (!parent->mFloats.RemoveFrame(outOfFlow)) {
      nsAutoOOFFrameList oofs(parent);
#ifdef DEBUG
      PRBool found =
#endif
        oofs.mList.RemoveFrame(outOfFlow);
      NS_ASSERTION(found, "Must have the out of flow in some child list");
    }
    ReparentFrame(outOfFlow, parent, this);

    aState.mOverflowPlaceholders.InsertFrames(nsnull, lastOverflowPlace, frame);
    
    
    
    
    lastOverflowPlace = frame;

    frame = frame->GetNextInFlow();
  }

  return taken;
}







PRBool
nsBlockFrame::HandleOverflowPlaceholdersOnPulledLine(
  nsBlockReflowState& aState, nsLineBox* aLine)
{
  
  
  if (aLine->mFirstChild && IsContinuationPlaceholder(aLine->mFirstChild)) {
#ifdef DEBUG
    PRBool taken =
#endif
      HandleOverflowPlaceholdersForPulledFrame(aState, aLine->mFirstChild);
    NS_ASSERTION(taken, "We must have removed that frame");
    return PR_TRUE;
  }
 
  
  
  PRInt32 n = aLine->GetChildCount();
  for (nsIFrame* f = aLine->mFirstChild; n > 0; f = f->GetNextSibling(), --n) {
#ifdef DEBUG
    PRBool taken =
#endif
      HandleOverflowPlaceholdersForPulledFrame(aState, f);
    NS_ASSERTION(!taken, "Shouldn't be any continuation placeholders on this line");
  }

  return PR_FALSE;
}





PRBool
nsBlockFrame::DrainOverflowLines(nsBlockReflowState& aState)
{
#ifdef DEBUG
  VerifyOverflowSituation();
#endif
  nsLineList* overflowLines = nsnull;
  nsLineList* ourOverflowLines = nsnull;

  
  nsBlockFrame* prevBlock = (nsBlockFrame*) GetPrevInFlow();
  if (prevBlock) {
    overflowLines = prevBlock->RemoveOverflowLines();
    if (overflowLines) {
      NS_ASSERTION(! overflowLines->empty(),
                   "overflow lines should never be set and empty");
      
      nsIFrame* frame = overflowLines->front()->mFirstChild;
      while (nsnull != frame) {
        ReparentFrame(frame, prevBlock, this);

        
        frame = frame->GetNextSibling();
      }

      
      nsAutoOOFFrameList oofs(prevBlock);
      if (oofs.mList.NotEmpty()) {
        for (nsIFrame* f = oofs.mList.FirstChild(); f; f = f->GetNextSibling()) {
          ReparentFrame(f, prevBlock, this);
        }
        mFloats.InsertFrames(nsnull, nsnull, oofs.mList.FirstChild());
        oofs.mList.SetFrames(nsnull);
      }
    }
    
    
    
  }

  
  
  ourOverflowLines = RemoveOverflowLines();
  if (ourOverflowLines) {
    nsAutoOOFFrameList oofs(this);
    if (oofs.mList.NotEmpty()) {
      
      mFloats.AppendFrames(nsnull, oofs.mList.FirstChild());
      oofs.mList.SetFrames(nsnull);
    }
  }

  if (!overflowLines && !ourOverflowLines) {
    
    return PR_FALSE;
  }

  NS_ASSERTION(aState.mOverflowPlaceholders.IsEmpty(),
               "Should have no overflow placeholders yet");

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsIFrame* lastOP = nsnull;
  nsFrameList keepPlaceholders;
  nsFrameList keepOutOfFlows;
  nsIFrame* lastKP = nsnull;
  nsIFrame* lastKOOF = nsnull;
  nsLineList* lineLists[3] = { overflowLines, &mLines, ourOverflowLines };
  static const PRPackedBool searchFirstLinesOnly[3] = { PR_FALSE, PR_TRUE, PR_FALSE };
  for (PRInt32 i = 0; i < 3; ++i) {
    nsLineList* ll = lineLists[i];
    if (ll && !ll->empty()) {
      line_iterator iter = ll->begin();
      line_iterator iter_end = ll->end();
      nsIFrame* lastFrame = nsnull;
      while (iter != iter_end) {
        PRUint32 n = iter->GetChildCount();
        if (n == 0 || !IsContinuationPlaceholder(iter->mFirstChild)) {
          if (lastFrame) {
            lastFrame->SetNextSibling(iter->mFirstChild);
          }
          if (searchFirstLinesOnly[i]) {
            break;
          }
          lastFrame = iter->LastChild();
          ++iter;
        } else {
          nsLineBox* line = iter;
          iter = ll->erase(iter);
          nsIFrame* next;
          for (nsPlaceholderFrame* f = static_cast<nsPlaceholderFrame*>(line->mFirstChild);
               n > 0; --n, f = static_cast<nsPlaceholderFrame*>(next)) {
            NS_ASSERTION(IsContinuationPlaceholder(f),
                         "Line frames should all be continuation placeholders");
            next = f->GetNextSibling();
            nsIFrame* fpif = f->GetPrevInFlow();
            nsIFrame* oof = f->GetOutOfFlowFrame();
            
            
            
#ifdef DEBUG
            PRBool found =
#endif
              mFloats.RemoveFrame(oof);
            NS_ASSERTION(found, "Float should have been put in our mFloats list");

            PRBool isAncestor = nsLayoutUtils::IsProperAncestorFrame(this, fpif);
            if (isAncestor) {
              
              
              
              aState.mOverflowPlaceholders.InsertFrame(nsnull, lastOP, f);
              
              
              lastOP = f;
            } else {
              if (fpif->GetParent() == prevBlock) {
                keepPlaceholders.InsertFrame(nsnull, lastKP, f);
                keepOutOfFlows.InsertFrame(nsnull, lastKOOF, oof);
                lastKP = f;
                lastKOOF = oof;
              } else {
                
                
                
                
                
                NS_ASSERTION(nsLayoutUtils::IsProperAncestorFrame(prevBlock, fpif),
                             "bad prev-in-flow ancestor chain");
                
                
                
                nsIFrame* fpAncestor;
                for (fpAncestor = fpif->GetParent();
                     !fpAncestor->GetNextInFlow() || !fpAncestor->IsFloatContainingBlock();
                     fpAncestor = fpAncestor->GetParent())
                  ;
                if (fpAncestor == prevBlock) {
                  
                  keepPlaceholders.InsertFrame(nsnull, lastKP, f);
                  keepOutOfFlows.InsertFrame(nsnull, lastKOOF, oof);
                  lastKP = f;
                  lastKOOF = oof;
                } else {
                  
                  
                  nsLineBox* newLine = aState.NewLineBox(f, 1, PR_FALSE);
                  if (newLine) {
                    nsBlockFrame* target =
                      static_cast<nsBlockFrame*>(fpAncestor->GetNextInFlow());
                    if (!target->mLines.empty()) {
                      f->SetNextSibling(target->mLines.front()->mFirstChild);
                    } else {
                      f->SetNextSibling(nsnull);
                    }
                    target->mLines.push_front(newLine);
                    ReparentFrame(f, this, target);

                    target->mFloats.InsertFrame(nsnull, nsnull, oof);
                    ReparentFrame(oof, this, target);
                  }
                }
              }
            }
          }
          aState.FreeLineBox(line);
        }
      }
      if (lastFrame) {
        lastFrame->SetNextSibling(nsnull);
      }
    }
  }

  
  if (overflowLines) {
    if (!overflowLines->empty()) {
      
      if (! mLines.empty()) 
        {
          
          
          mLines.front()->MarkPreviousMarginDirty();
          
          nsIFrame* lastFrame = overflowLines->back()->LastChild();
          lastFrame->SetNextSibling(mLines.front()->mFirstChild);
        }
      
      mLines.splice(mLines.begin(), *overflowLines);
      NS_ASSERTION(overflowLines->empty(), "splice should empty list");
    }
    delete overflowLines;
  }
  if (ourOverflowLines) {
    if (!ourOverflowLines->empty()) {
      if (!mLines.empty()) {
        mLines.back()->LastChild()->
          SetNextSibling(ourOverflowLines->front()->mFirstChild);
      }
      
      mLines.splice(mLines.end(), *ourOverflowLines);
    }
    delete ourOverflowLines;
  }

  
  if (keepPlaceholders.NotEmpty()) {
    keepPlaceholders.SortByContentOrder();
    nsLineBox* newLine = aState.NewLineBox(keepPlaceholders.FirstChild(),
                                           keepPlaceholders.GetLength(), PR_FALSE);
    if (newLine) {
      if (!mLines.empty()) {
        keepPlaceholders.LastChild()->SetNextSibling(mLines.front()->mFirstChild);
      }
      mLines.push_front(newLine);
    }

    
    keepOutOfFlows.SortByContentOrder();
    mFloats.InsertFrames(nsnull, nsnull, keepOutOfFlows.FirstChild());
  }

  return PR_TRUE;
}

nsLineList*
nsBlockFrame::GetOverflowLines() const
{
  if (!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_LINES)) {
    return nsnull;
  }
  nsLineList* lines = static_cast<nsLineList*>
                                 (GetProperty(nsGkAtoms::overflowLinesProperty));
  NS_ASSERTION(lines && !lines->empty(),
               "value should always be stored and non-empty when state set");
  return lines;
}

nsLineList*
nsBlockFrame::RemoveOverflowLines()
{
  if (!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_LINES)) {
    return nsnull;
  }
  nsLineList* lines = static_cast<nsLineList*>
                                 (UnsetProperty(nsGkAtoms::overflowLinesProperty));
  NS_ASSERTION(lines && !lines->empty(),
               "value should always be stored and non-empty when state set");
  RemoveStateBits(NS_BLOCK_HAS_OVERFLOW_LINES);
  return lines;
}


static void
DestroyOverflowLines(void*           aFrame,
                     nsIAtom*        aPropertyName,
                     void*           aPropertyValue,
                     void*           aDtorData)
{
  if (aPropertyValue) {
    nsLineList* lines = static_cast<nsLineList*>(aPropertyValue);
    nsPresContext *context = static_cast<nsPresContext*>(aDtorData);
    nsLineBox::DeleteLineList(context, *lines);
    delete lines;
  }
}



nsresult
nsBlockFrame::SetOverflowLines(nsLineList* aOverflowLines)
{
  NS_ASSERTION(aOverflowLines, "null lines");
  NS_ASSERTION(!aOverflowLines->empty(), "empty lines");
  NS_ASSERTION(!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_LINES),
               "Overwriting existing overflow lines");

  nsPresContext *presContext = PresContext();
  nsresult rv = presContext->PropertyTable()->
    SetProperty(this, nsGkAtoms::overflowLinesProperty, aOverflowLines,
                DestroyOverflowLines, presContext);
  
  NS_ASSERTION(rv != NS_PROPTABLE_PROP_OVERWRITTEN, "existing overflow list");
  AddStateBits(NS_BLOCK_HAS_OVERFLOW_LINES);
  return rv;
}

nsFrameList
nsBlockFrame::GetOverflowOutOfFlows() const
{
  if (!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS)) {
    return nsFrameList();
  }
  nsIFrame* result = static_cast<nsIFrame*>
                                (GetProperty(nsGkAtoms::overflowOutOfFlowsProperty));
  NS_ASSERTION(result, "value should always be non-empty when state set");
  return nsFrameList(result);
}


void
nsBlockFrame::SetOverflowOutOfFlows(const nsFrameList& aList)
{
  if (aList.IsEmpty()) {
    if (!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS)) {
      return;
    }
#ifdef DEBUG
    nsIFrame* result = static_cast<nsIFrame*>
#endif
      (UnsetProperty(nsGkAtoms::overflowOutOfFlowsProperty));
    NS_ASSERTION(result, "value should always be non-empty when state set");
    RemoveStateBits(NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS);
  } else {
    SetProperty(nsGkAtoms::overflowOutOfFlowsProperty,
                aList.FirstChild(), nsnull);
    AddStateBits(NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS);
  }
}

nsFrameList*
nsBlockFrame::GetOverflowPlaceholders() const
{
  if (!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_PLACEHOLDERS)) {
    return nsnull;
  }
  nsFrameList* result = static_cast<nsFrameList*>
                                   (GetProperty(nsGkAtoms::overflowPlaceholdersProperty));
  NS_ASSERTION(result, "value should always be non-empty when state set");
  return result;
}




nsIFrame*
nsBlockFrame::LastChild()
{
  if (! mLines.empty()) {
    return mLines.back()->LastChild();
  }
  return nsnull;
}

NS_IMETHODIMP
nsBlockFrame::AppendFrames(nsIAtom*  aListName,
                           nsIFrame* aFrameList)
{
  if (nsnull == aFrameList) {
    return NS_OK;
  }
  if (aListName) {
    if (nsGkAtoms::absoluteList == aListName) {
      return mAbsoluteContainer.AppendFrames(this, aListName, aFrameList);
    }
    else if (nsGkAtoms::floatList == aListName) {
      mFloats.AppendFrames(nsnull, aFrameList);
      return NS_OK;
    }
    else {
      NS_ERROR("unexpected child list");
      return NS_ERROR_INVALID_ARG;
    }
  }

  
  nsIFrame* lastKid = nsnull;
  nsLineBox* lastLine = mLines.empty() ? nsnull : mLines.back();
  if (lastLine) {
    lastKid = lastLine->LastChild();
  }

  
#ifdef NOISY_REFLOW_REASON
  ListTag(stdout);
  printf(": append ");
  nsFrame::ListTag(stdout, aFrameList);
  if (lastKid) {
    printf(" after ");
    nsFrame::ListTag(stdout, lastKid);
  }
  printf("\n");
#endif
  nsresult rv = AddFrames(aFrameList, lastKid);
  if (NS_SUCCEEDED(rv)) {
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN); 
  }
  return rv;
}

NS_IMETHODIMP
nsBlockFrame::InsertFrames(nsIAtom*  aListName,
                           nsIFrame* aPrevFrame,
                           nsIFrame* aFrameList)
{
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  if (aListName) {
    if (nsGkAtoms::absoluteList == aListName) {
      return mAbsoluteContainer.InsertFrames(this, aListName, aPrevFrame,
                                             aFrameList);
    }
    else if (nsGkAtoms::floatList == aListName) {
      mFloats.InsertFrames(this, aPrevFrame, aFrameList);
      return NS_OK;
    }
#ifdef IBMBIDI
    else if (nsGkAtoms::nextBidi == aListName) {}
#endif 
    else {
      NS_ERROR("unexpected child list");
      return NS_ERROR_INVALID_ARG;
    }
  }

#ifdef NOISY_REFLOW_REASON
  ListTag(stdout);
  printf(": insert ");
  nsFrame::ListTag(stdout, aFrameList);
  if (aPrevFrame) {
    printf(" after ");
    nsFrame::ListTag(stdout, aPrevFrame);
  }
  printf("\n");
#endif
  nsresult rv = AddFrames(aFrameList, aPrevFrame);
#ifdef IBMBIDI
  if (aListName != nsGkAtoms::nextBidi)
#endif 
  if (NS_SUCCEEDED(rv)) {
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN); 
  }
  return rv;
}

static PRBool
ShouldPutNextSiblingOnNewLine(nsIFrame* aLastFrame)
{
  nsIAtom* type = aLastFrame->GetType();
  if (type == nsGkAtoms::brFrame)
    return PR_TRUE;
  if (type == nsGkAtoms::textFrame)
    return aLastFrame->HasTerminalNewline() &&
           aLastFrame->GetStyleText()->NewlineIsSignificant();
  if (type == nsGkAtoms::placeholderFrame)
    return IsContinuationPlaceholder(aLastFrame);
  return PR_FALSE;
}

nsresult
nsBlockFrame::AddFrames(nsIFrame* aFrameList,
                        nsIFrame* aPrevSibling)
{
  
  ClearLineCursor();

  if (nsnull == aFrameList) {
    return NS_OK;
  }

  
  
  if (!aPrevSibling && mBullet && !HaveOutsideBullet()) {
    NS_ASSERTION(!nsFrameList(aFrameList).ContainsFrame(mBullet),
                 "Trying to make mBullet prev sibling to itself");
    aPrevSibling = mBullet;
  }
  
  nsIPresShell *presShell = PresContext()->PresShell();

  
  nsLineList::iterator prevSibLine = end_lines();
  PRInt32 prevSiblingIndex = -1;
  if (aPrevSibling) {
    
    
    

    
    if (! nsLineBox::RFindLineContaining(aPrevSibling,
                                         begin_lines(), prevSibLine,
                                         &prevSiblingIndex)) {
      
      
      NS_NOTREACHED("prev sibling not in line list");
      aPrevSibling = nsnull;
      prevSibLine = end_lines();
    }
  }

  
  
  nsIFrame* prevSiblingNextFrame = nsnull;
  if (aPrevSibling) {
    prevSiblingNextFrame = aPrevSibling->GetNextSibling();

    
    
    PRInt32 rem = prevSibLine->GetChildCount() - prevSiblingIndex - 1;
    if (rem) {
      
      nsLineBox* line = NS_NewLineBox(presShell, prevSiblingNextFrame, rem, PR_FALSE);
      if (!line) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mLines.after_insert(prevSibLine, line);
      prevSibLine->SetChildCount(prevSibLine->GetChildCount() - rem);
      
      
      
      MarkLineDirty(prevSibLine);
      
      
      line->SetInvalidateTextRuns(PR_TRUE);
    }

    
    aPrevSibling->SetNextSibling(aFrameList);
  }
  else if (! mLines.empty()) {
    prevSiblingNextFrame = mLines.front()->mFirstChild;
    mLines.front()->SetInvalidateTextRuns(PR_TRUE);
  }

  
  
  nsIFrame* newFrame = aFrameList;
  while (newFrame) {
    NS_ASSERTION(newFrame->GetType() != nsGkAtoms::placeholderFrame ||
                 (!newFrame->GetStyleDisplay()->IsAbsolutelyPositioned() &&
                  !newFrame->GetStyleDisplay()->IsFloating()),
                 "Placeholders should not float or be positioned");

    PRBool isBlock = newFrame->GetStyleDisplay()->IsBlockOutside();

    
    
    
    
    
    if (isBlock || prevSibLine == end_lines() || prevSibLine->IsBlock() ||
        (aPrevSibling && ShouldPutNextSiblingOnNewLine(aPrevSibling))) {
      
      
      nsLineBox* line = NS_NewLineBox(presShell, newFrame, 1, isBlock);
      if (!line) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      if (prevSibLine != end_lines()) {
        
        mLines.after_insert(prevSibLine, line);
        ++prevSibLine;
      }
      else {
        
        mLines.push_front(line);
        prevSibLine = begin_lines();
      }
    }
    else {
      prevSibLine->SetChildCount(prevSibLine->GetChildCount() + 1);
      
      
      
      MarkLineDirty(prevSibLine);
    }

    aPrevSibling = newFrame;
    newFrame = newFrame->GetNextSibling();
  }
  if (prevSiblingNextFrame) {
    
    aPrevSibling->SetNextSibling(prevSiblingNextFrame);
  }

#ifdef DEBUG
  VerifyLines(PR_TRUE);
#endif
  return NS_OK;
}

nsBlockFrame::line_iterator
nsBlockFrame::RemoveFloat(nsIFrame* aFloat) {
  
  
  line_iterator line = begin_lines(), line_end = end_lines();
  for ( ; line != line_end; ++line) {
    if (line->IsInline() && line->RemoveFloat(aFloat)) {
      break;
    }
  }

  
  
  nsFrameManager* fm = PresContext()->GetPresShell()->FrameManager();
  nsPlaceholderFrame* placeholder = fm->GetPlaceholderFrameFor(aFloat);
  if (placeholder) {
    fm->UnregisterPlaceholderFrame(placeholder);
    placeholder->SetOutOfFlowFrame(nsnull);
  }

  
  if (mFloats.DestroyFrame(aFloat)) {
    return line;
  }

  
  {
    nsAutoOOFFrameList oofs(this);
    if (oofs.mList.DestroyFrame(aFloat)) {
      return line_end;
    }
  }
  
  
  
  
  aFloat->Destroy();
  return line_end;
}

static void MarkAllDescendantLinesDirty(nsBlockFrame* aBlock)
{
  nsLineList::iterator line = aBlock->begin_lines();
  nsLineList::iterator endLine = aBlock->end_lines();
  while (line != endLine) {
    if (line->IsBlock()) {
      nsIFrame* f = line->mFirstChild;
      nsBlockFrame* bf = nsLayoutUtils::GetAsBlock(f);
      if (bf) {
        MarkAllDescendantLinesDirty(bf);
      }
    }
    line->MarkDirty();
    ++line;
  }
}

static void MarkSameSpaceManagerLinesDirty(nsBlockFrame* aBlock)
{
  nsBlockFrame* blockWithSpaceMgr = aBlock;
  while (!(blockWithSpaceMgr->GetStateBits() & NS_BLOCK_SPACE_MGR)) {
    nsBlockFrame* bf = nsLayoutUtils::GetAsBlock(blockWithSpaceMgr->GetParent());
    if (!bf) {
      break;
    }
    blockWithSpaceMgr = bf;
  }
    
  
  
  
  
  MarkAllDescendantLinesDirty(blockWithSpaceMgr);
}




static PRBool BlockHasAnyFloats(nsIFrame* aFrame)
{
  nsBlockFrame* block = nsLayoutUtils::GetAsBlock(aFrame);
  if (!block)
    return PR_FALSE;
  if (block->GetFirstChild(nsGkAtoms::floatList))
    return PR_TRUE;
    
  nsLineList::iterator line = block->begin_lines();
  nsLineList::iterator endLine = block->end_lines();
  while (line != endLine) {
    if (line->IsBlock() && BlockHasAnyFloats(line->mFirstChild))
      return PR_TRUE;
    ++line;
  }
  return PR_FALSE;
}

NS_IMETHODIMP
nsBlockFrame::RemoveFrame(nsIAtom*  aListName,
                          nsIFrame* aOldFrame)
{
  nsresult rv = NS_OK;

#ifdef NOISY_REFLOW_REASON
  ListTag(stdout);
  printf(": remove ");
  nsFrame::ListTag(stdout, aOldFrame);
  printf("\n");
#endif

  if (nsnull == aListName) {
    PRBool hasFloats = BlockHasAnyFloats(aOldFrame);
    rv = DoRemoveFrame(aOldFrame, REMOVE_FIXED_CONTINUATIONS);
    if (hasFloats) {
      MarkSameSpaceManagerLinesDirty(this);
    }
  }
  else if (nsGkAtoms::absoluteList == aListName) {
    return mAbsoluteContainer.RemoveFrame(this, aListName, aOldFrame);
  }
  else if (nsGkAtoms::floatList == aListName) {
    nsIFrame* curFrame = aOldFrame;
    
    
    
    do {
      nsIFrame* continuation = curFrame->GetNextContinuation();
      nsBlockFrame* curParent = static_cast<nsBlockFrame*>(curFrame->GetParent());
      curParent->RemoveFloat(curFrame);
      MarkSameSpaceManagerLinesDirty(curParent);
      curFrame = continuation;
    } while (curFrame);
  }
#ifdef IBMBIDI
  else if (nsGkAtoms::nextBidi == aListName) {
    
    return DoRemoveFrame(aOldFrame, REMOVE_FIXED_CONTINUATIONS);
  }
#endif 
  else {
    NS_ERROR("unexpected child list");
    rv = NS_ERROR_INVALID_ARG;
  }

  if (NS_SUCCEEDED(rv)) {
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN); 
  }
  return rv;
}

void
nsBlockFrame::DoRemoveOutOfFlowFrame(nsIFrame* aFrame)
{
  
  nsBlockFrame* block = (nsBlockFrame*)aFrame->GetParent();

  
  const nsStyleDisplay* display = aFrame->GetStyleDisplay();
  if (display->IsAbsolutelyPositioned()) {
    
    block->mAbsoluteContainer.RemoveFrame(block,
                                          nsGkAtoms::absoluteList,
                                          aFrame);
  }
  else {
    
    nsIFrame* nextInFlow = aFrame->GetNextInFlow();
    if (nextInFlow) {
      nsBlockFrame::DoRemoveOutOfFlowFrame(nextInFlow);
    }
    
    
    block->RemoveFloat(aFrame);
  }
}




void
nsBlockFrame::TryAllLines(nsLineList::iterator* aIterator,
                          nsLineList::iterator* aStartIterator,
                          nsLineList::iterator* aEndIterator,
                          PRBool* aInOverflowLines) {
  if (*aIterator == *aEndIterator) {
    if (!*aInOverflowLines) {
      *aInOverflowLines = PR_TRUE;
      
      nsLineList* overflowLines = GetOverflowLines();
      if (overflowLines) {
        *aStartIterator = overflowLines->begin();
        *aIterator = *aStartIterator;
        *aEndIterator = overflowLines->end();
      }
    }
  }
}

nsBlockInFlowLineIterator::nsBlockInFlowLineIterator(nsBlockFrame* aFrame,
    line_iterator aLine, PRBool aInOverflow)
  : mFrame(aFrame), mLine(aLine), mInOverflowLines(nsnull)
{
  if (aInOverflow) {
    mInOverflowLines = aFrame->GetOverflowLines();
    NS_ASSERTION(mInOverflowLines, "How can we be in overflow if there isn't any?");
  }
}

nsBlockInFlowLineIterator::nsBlockInFlowLineIterator(nsBlockFrame* aFrame,
    PRBool* aFoundValidLine)
  : mFrame(aFrame), mInOverflowLines(nsnull)
{
  mLine = aFrame->begin_lines();
  *aFoundValidLine = FindValidLine();
}

static nsIFrame*
FindChildContaining(nsBlockFrame* aFrame, nsIFrame* aFindFrame)
{
  nsIFrame* child;
  while (PR_TRUE) {
    nsIFrame* block = aFrame;
    while (PR_TRUE) {
      child = nsLayoutUtils::FindChildContainingDescendant(block, aFindFrame);
      if (child)
        break;
      block = block->GetNextContinuation();
    }
    if (!child)
      return nsnull;
    if (!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW))
      break;
    aFindFrame = aFrame->PresContext()->FrameManager()->GetPlaceholderFrameFor(child);
  }

  return child;
}

nsBlockInFlowLineIterator::nsBlockInFlowLineIterator(nsBlockFrame* aFrame,
    nsIFrame* aFindFrame, PRBool* aFoundValidLine)
  : mFrame(aFrame), mInOverflowLines(nsnull)
{
  mLine = aFrame->begin_lines();

  *aFoundValidLine = PR_FALSE;

  nsIFrame* child = FindChildContaining(aFrame, aFindFrame);
  if (!child)
    return;

  if (!FindValidLine())
    return;

  do {
    if (mLine->Contains(child)) {
      *aFoundValidLine = PR_TRUE;
      return;
    }
  } while (Next());
}

nsBlockFrame::line_iterator
nsBlockInFlowLineIterator::End()
{
  return mInOverflowLines ? mInOverflowLines->end() : mFrame->end_lines();
}

PRBool
nsBlockInFlowLineIterator::IsLastLineInList()
{
  line_iterator end = End();
  return mLine != end && mLine.next() == end;
}

PRBool
nsBlockInFlowLineIterator::Next()
{
  ++mLine;
  return FindValidLine();
}

PRBool
nsBlockInFlowLineIterator::Prev()
{
  line_iterator begin = mInOverflowLines ? mInOverflowLines->begin() : mFrame->begin_lines();
  if (mLine != begin) {
    --mLine;
    return PR_TRUE;
  }
  PRBool currentlyInOverflowLines = mInOverflowLines != nsnull;
  while (PR_TRUE) {
    if (currentlyInOverflowLines) {
      mInOverflowLines = nsnull;
      mLine = mFrame->end_lines();
      if (mLine != mFrame->begin_lines()) {
        --mLine;
        return PR_TRUE;
      }
    } else {
      mFrame = static_cast<nsBlockFrame*>(mFrame->GetPrevInFlow());
      if (!mFrame)
        return PR_FALSE;
      mInOverflowLines = mFrame->GetOverflowLines();
      if (mInOverflowLines) {
        mLine = mInOverflowLines->end();
        NS_ASSERTION(mLine != mInOverflowLines->begin(), "empty overflow line list?");
        --mLine;
        return PR_TRUE;
      }
    }
    currentlyInOverflowLines = !currentlyInOverflowLines;
  }
}

PRBool
nsBlockInFlowLineIterator::FindValidLine()
{
  line_iterator end = mInOverflowLines ? mInOverflowLines->end() : mFrame->end_lines();
  if (mLine != end)
    return PR_TRUE; 
  PRBool currentlyInOverflowLines = mInOverflowLines != nsnull;
  while (PR_TRUE) {
    if (currentlyInOverflowLines) {
      mFrame = static_cast<nsBlockFrame*>(mFrame->GetNextInFlow());
      if (!mFrame)
        return PR_FALSE;
      mInOverflowLines = nsnull;
      mLine = mFrame->begin_lines();
      if (mLine != mFrame->end_lines())
        return PR_TRUE;
    } else {
      mInOverflowLines = mFrame->GetOverflowLines();
      if (mInOverflowLines) {
        mLine = mInOverflowLines->begin();
        NS_ASSERTION(mLine != mInOverflowLines->end(), "empty overflow line list?");
        return PR_TRUE;
      }
    }
    currentlyInOverflowLines = !currentlyInOverflowLines;
  }
}

static nsresult RemoveBlockChild(nsIFrame* aFrame, PRBool aDestroyFrames,
                                 PRBool aRemoveOnlyFluidContinuations)
{
  if (!aFrame)
    return NS_OK;

  nsBlockFrame* nextBlock = nsLayoutUtils::GetAsBlock(aFrame->GetParent());
  NS_ASSERTION(nextBlock,
               "Our child's continuation's parent is not a block?");
  return nextBlock->DoRemoveFrame(aFrame,
      (aDestroyFrames ? 0 : nsBlockFrame::PERSERVE_REMOVED_FRAMES) |
      (aRemoveOnlyFluidContinuations ? 0 : nsBlockFrame::REMOVE_FIXED_CONTINUATIONS));
}








nsresult
nsBlockFrame::DoRemoveFrame(nsIFrame* aDeletedFrame, PRUint32 aFlags)
{
  
  ClearLineCursor();

  nsPresContext* presContext = PresContext();
  if (NS_FRAME_IS_OVERFLOW_CONTAINER & aDeletedFrame->GetStateBits()) {
    if (!(aFlags & PERSERVE_REMOVED_FRAMES)) {
      nsIFrame* nif = aDeletedFrame->GetNextInFlow();
      if (nif)
        static_cast<nsContainerFrame*>(nif->GetParent())
          ->nsContainerFrame::DeleteNextInFlowChild(presContext, nif,
              (aFlags & FRAMES_ARE_EMPTY) != 0);
      nsresult rv = nsContainerFrame::StealFrame(presContext, aDeletedFrame);
      NS_ENSURE_SUCCESS(rv, rv);
      aDeletedFrame->Destroy();
    }
    else {
      PR_NOT_REACHED("We can't not destroy overflow containers");
      return NS_ERROR_NOT_IMPLEMENTED;
      
      
      
      
    }
    return NS_OK;
  }

  if (aDeletedFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
    NS_ASSERTION(!(aFlags & PERSERVE_REMOVED_FRAMES),
                 "We can't not destroy out of flows");
    DoRemoveOutOfFlowFrame(aDeletedFrame);
    return NS_OK;
  }
  
  nsIPresShell* presShell = presContext->PresShell();

  PRBool isPlaceholder = nsGkAtoms::placeholderFrame == aDeletedFrame->GetType();
  if (isPlaceholder) {
    nsFrameList* overflowPlaceholders = GetOverflowPlaceholders();
    if (overflowPlaceholders && overflowPlaceholders->RemoveFrame(aDeletedFrame)) {
      nsIFrame* nif = aDeletedFrame->GetNextInFlow();
      if (!(aFlags & PERSERVE_REMOVED_FRAMES)) {
        aDeletedFrame->Destroy();
      } else {
        aDeletedFrame->SetNextSibling(nsnull);
      }
      return RemoveBlockChild(nif, !(aFlags & PERSERVE_REMOVED_FRAMES),
                              !(aFlags & REMOVE_FIXED_CONTINUATIONS));
    }
  }
  
  
  
  nsLineList::iterator line_start = mLines.begin(),
                       line_end = mLines.end();
  nsLineList::iterator line = line_start;
  PRBool searchingOverflowList = PR_FALSE;
  nsIFrame* prevSibling = nsnull;
  
  
  TryAllLines(&line, &line_start, &line_end, &searchingOverflowList);
  while (line != line_end) {
    nsIFrame* frame = line->mFirstChild;
    PRInt32 n = line->GetChildCount();
    while (--n >= 0) {
      if (frame == aDeletedFrame) {
        goto found_frame;
      }
      prevSibling = frame;
      frame = frame->GetNextSibling();
    }
    ++line;
    TryAllLines(&line, &line_start, &line_end, &searchingOverflowList);
  }
found_frame:;
  if (line == line_end) {
    NS_ERROR("can't find deleted frame in lines");
    return NS_ERROR_FAILURE;
  }
  
  if (!(aFlags & FRAMES_ARE_EMPTY)) {
    if (line != line_start) {
      line.prev()->SetInvalidateTextRuns(PR_TRUE);
    }
    else if (searchingOverflowList && !mLines.empty()) {
      mLines.back()->SetInvalidateTextRuns(PR_TRUE);
    }
  }

  if (prevSibling && !prevSibling->GetNextSibling()) {
    
    
    prevSibling = nsnull;
  }
  NS_ASSERTION(!prevSibling || prevSibling->GetNextSibling() == aDeletedFrame, "bad prevSibling");

  while ((line != line_end) && (nsnull != aDeletedFrame)) {
    NS_ASSERTION(this == aDeletedFrame->GetParent(), "messed up delete code");
    NS_ASSERTION(line->Contains(aDeletedFrame), "frame not in line");

    if (!(aFlags & FRAMES_ARE_EMPTY)) {
      line->SetInvalidateTextRuns(PR_TRUE);
    }

    
    
    PRBool isLastFrameOnLine = (1 == line->GetChildCount() ||
                                line->LastChild() == aDeletedFrame);

    
    nsIFrame* nextFrame = aDeletedFrame->GetNextSibling();
    if (line->mFirstChild == aDeletedFrame) {
      
      
      
      line->mFirstChild = nextFrame;
    }

    
    
    --line;
    if (line != line_end && !line->IsBlock()) {
      
      
      line->MarkDirty();
    }
    ++line;

    
    
    
    if (prevSibling) {
      prevSibling->SetNextSibling(nextFrame);
    }

    
    PRInt32 lineChildCount = line->GetChildCount();
    lineChildCount--;
    line->SetChildCount(lineChildCount);

    
    
    nsIFrame* deletedNextContinuation = (aFlags & REMOVE_FIXED_CONTINUATIONS) ?
        aDeletedFrame->GetNextContinuation() : aDeletedFrame->GetNextInFlow();
#ifdef NOISY_REMOVE_FRAME
    printf("DoRemoveFrame: %s line=%p frame=",
           searchingOverflowList?"overflow":"normal", line.get());
    nsFrame::ListTag(stdout, aDeletedFrame);
    printf(" prevSibling=%p deletedNextContinuation=%p\n", prevSibling, deletedNextContinuation);
#endif

    if (aFlags & PERSERVE_REMOVED_FRAMES) {
      aDeletedFrame->SetNextSibling(nsnull);
    } else {
      aDeletedFrame->Destroy();
    }
    aDeletedFrame = deletedNextContinuation;

    PRBool haveAdvancedToNextLine = PR_FALSE;
    
    if (0 == lineChildCount) {
#ifdef NOISY_REMOVE_FRAME
        printf("DoRemoveFrame: %s line=%p became empty so it will be removed\n",
               searchingOverflowList?"overflow":"normal", line.get());
#endif
      nsLineBox *cur = line;
      if (!searchingOverflowList) {
        line = mLines.erase(line);
        
        
        
        
        nsRect lineCombinedArea(cur->GetCombinedArea());
#ifdef NOISY_BLOCK_INVALIDATE
        printf("%p invalidate 10 (%d, %d, %d, %d)\n",
               this, lineCombinedArea.x, lineCombinedArea.y,
               lineCombinedArea.width, lineCombinedArea.height);
#endif
        Invalidate(lineCombinedArea);
      } else {
        nsLineList* lineList = RemoveOverflowLines();
        line = lineList->erase(line);
        if (!lineList->empty()) {
          SetOverflowLines(lineList);
        }
      }
      cur->Destroy(presShell);

      
      
      
      
      if (line != line_end) {
        line->MarkPreviousMarginDirty();
      }
      haveAdvancedToNextLine = PR_TRUE;
    } else {
      
      
      if (!deletedNextContinuation || isLastFrameOnLine ||
          !line->Contains(deletedNextContinuation)) {
        line->MarkDirty();
        ++line;
        haveAdvancedToNextLine = PR_TRUE;
      }
    }

    if (deletedNextContinuation) {
      
      
      if (isPlaceholder) {
        return RemoveBlockChild(deletedNextContinuation,
                                !(aFlags & PERSERVE_REMOVED_FRAMES),
                                !(aFlags & REMOVE_FIXED_CONTINUATIONS));
      }

      
      if (deletedNextContinuation->GetParent() != this) {
        
        
        
        break;
      }

      
      
      if (haveAdvancedToNextLine) {
        if (line != line_end && !searchingOverflowList &&
            !line->Contains(deletedNextContinuation)) {
          
          
          line = line_end;
        }

        PRBool wasSearchingOverflowList = searchingOverflowList;
        TryAllLines(&line, &line_start, &line_end, &searchingOverflowList);
        if (NS_UNLIKELY(searchingOverflowList && !wasSearchingOverflowList &&
                        prevSibling)) {
          
          
          
          prevSibling->SetNextSibling(nsnull);
          prevSibling = nsnull;
        }
#ifdef NOISY_REMOVE_FRAME
        printf("DoRemoveFrame: now on %s line=%p prevSibling=%p\n",
               searchingOverflowList?"overflow":"normal", line.get(),
               prevSibling);
#endif
      }
    }
  }

  if (!(aFlags & FRAMES_ARE_EMPTY) && line.next() != line_end) {
    line.next()->SetInvalidateTextRuns(PR_TRUE);
  }

#ifdef DEBUG
  VerifyLines(PR_TRUE);
#endif

  
  return RemoveBlockChild(aDeletedFrame, !(aFlags & PERSERVE_REMOVED_FRAMES),
                          !(aFlags & REMOVE_FIXED_CONTINUATIONS));
}

nsresult
nsBlockFrame::StealFrame(nsPresContext* aPresContext,
                         nsIFrame*      aChild,
                         PRBool         aForceNormal)
{
  NS_PRECONDITION(aPresContext && aChild, "null pointer");

  if ((aChild->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)
      && !aForceNormal)
    return nsContainerFrame::StealFrame(aPresContext, aChild);

  
  
  nsLineList::iterator line = mLines.begin(),
                       line_start = line,
                       line_end = mLines.end();
  PRBool searchingOverflowList = PR_FALSE;
  nsIFrame* prevSibling = nsnull;
  
  
  TryAllLines(&line, &line_start, &line_end, &searchingOverflowList);
  while (line != line_end) {
    nsIFrame* frame = line->mFirstChild;
    PRInt32 n = line->GetChildCount();
    while (--n >= 0) {
      if (frame == aChild) {
        
        if (prevSibling)
          prevSibling->SetNextSibling(frame->GetNextSibling());
        else
          line->mFirstChild = frame->GetNextSibling();
        frame->SetNextSibling(nsnull);

        
        PRInt32 count = line->GetChildCount();
        line->SetChildCount(--count);
        if (count > 0) {
           line->MarkDirty();
        }
        else {
          
          nsLineBox* lineBox = line;
          if (searchingOverflowList) {
            
            nsLineList* lineList = RemoveOverflowLines();
            lineList->erase(line);
            if (!lineList->empty()) {
              nsresult rv = SetOverflowLines(lineList);
              NS_ENSURE_SUCCESS(rv, rv);
            }
          }
          else {
            mLines.erase(line);
          }
          lineBox->Destroy(aPresContext->PresShell());
          if (line != line_end) {
            
            line->MarkPreviousMarginDirty();
          }
        }

        
        return NS_OK;
      }
      prevSibling = frame;
      frame = frame->GetNextSibling();
    }
    ++line;
    TryAllLines(&line, &line_start, &line_end, &searchingOverflowList);
  }
  return NS_ERROR_UNEXPECTED;
}

void
nsBlockFrame::DeleteNextInFlowChild(nsPresContext* aPresContext,
                                    nsIFrame*      aNextInFlow,
                                    PRBool         aDeletingEmptyFrames)
{
  NS_PRECONDITION(aNextInFlow->GetPrevInFlow(), "bad next-in-flow");

  if (NS_FRAME_IS_OVERFLOW_CONTAINER & aNextInFlow->GetStateBits()) {
    nsContainerFrame::DeleteNextInFlowChild(aPresContext,
        aNextInFlow, aDeletingEmptyFrames);
  }
  else {
    DoRemoveFrame(aNextInFlow,
        aDeletingEmptyFrames ? FRAMES_ARE_EMPTY : 0);
  }
}




nsRect
nsBlockFrame::ComputeFloatAvailableSpace(nsBlockReflowState& aState,
                                         nsIFrame* aFloatFrame)
{
  
  
  nscoord availWidth;
  const nsStyleDisplay* floatDisplay = aFloatFrame->GetStyleDisplay();

  if (NS_STYLE_DISPLAY_TABLE != floatDisplay->mDisplay ||
      eCompatibility_NavQuirks != aState.mPresContext->CompatibilityMode() ) {
    availWidth = aState.mContentArea.width;
  }
  else {
    
    
    
    
    availWidth = aState.mAvailSpaceRect.width;
    
    
    
    
    nscoord twp = nsPresContext::CSSPixelsToAppUnits(1);
    availWidth -=  availWidth % twp;
  }

  
  nscoord contentYOffset = aState.mY - aState.BorderPadding().top;
  nscoord availHeight = NS_UNCONSTRAINEDSIZE == aState.mContentArea.height
                        ? NS_UNCONSTRAINEDSIZE
                        : PR_MAX(0, aState.mContentArea.height - contentYOffset);

#ifdef DISABLE_FLOAT_BREAKING_IN_COLUMNS
  if (availHeight != NS_UNCONSTRAINEDSIZE &&
      nsLayoutUtils::GetClosestFrameOfType(this, nsGkAtoms::columnSetFrame)) {
    
    
    
    
    availHeight = NS_UNCONSTRAINEDSIZE;
  }
#endif

  return nsRect(aState.BorderPadding().left,
                aState.BorderPadding().top,
                availWidth, availHeight);
}

nscoord
nsBlockFrame::ComputeFloatWidth(nsBlockReflowState& aState,
                                nsPlaceholderFrame* aPlaceholder)
{
  
  nsIFrame* floatFrame = aPlaceholder->GetOutOfFlowFrame();

  nsRect availSpace = ComputeFloatAvailableSpace(aState, floatFrame);

  nsHTMLReflowState floatRS(aState.mPresContext, aState.mReflowState,
                            floatFrame, 
                            nsSize(availSpace.width, availSpace.height));
  return floatRS.ComputedWidth() + floatRS.mComputedBorderPadding.LeftRight() +
    floatRS.mComputedMargin.LeftRight();
}

nsresult
nsBlockFrame::ReflowFloat(nsBlockReflowState& aState,
                          nsPlaceholderFrame* aPlaceholder,
                          nsMargin&           aFloatMargin,
                          nsReflowStatus&     aReflowStatus)
{
  
  nsIFrame* floatFrame = aPlaceholder->GetOutOfFlowFrame();
  aReflowStatus = NS_FRAME_COMPLETE;

#ifdef NOISY_FLOAT
  printf("Reflow Float %p in parent %p, availSpace(%d,%d,%d,%d)\n",
          aPlaceholder->GetOutOfFlowFrame(), this, 
          aState.mAvailSpaceRect.x, aState.mAvailSpaceRect.y, 
          aState.mAvailSpaceRect.width, aState.mAvailSpaceRect.height
  );
#endif

  nsRect availSpace = ComputeFloatAvailableSpace(aState, floatFrame);

  nsHTMLReflowState floatRS(aState.mPresContext, aState.mReflowState,
                            floatFrame, 
                            nsSize(availSpace.width, availSpace.height));

  
  nsBlockReflowContext brc(aState.mPresContext, aState.mReflowState);

  
  PRBool isAdjacentWithTop = aState.IsAdjacentWithTop();

  nsIFrame* clearanceFrame = nsnull;
  nsresult rv;
  do {
    nsCollapsingMargin margin;
    PRBool mayNeedRetry = PR_FALSE;
    floatRS.mDiscoveredClearance = nsnull;
    
    if (!floatFrame->GetPrevInFlow()) {
      nsBlockReflowContext::ComputeCollapsedTopMargin(floatRS, &margin,
                                                      clearanceFrame, &mayNeedRetry);

      if (mayNeedRetry && !clearanceFrame) {
        floatRS.mDiscoveredClearance = &clearanceFrame;
        
        
      }
    }

    rv = brc.ReflowBlock(availSpace, PR_TRUE, margin,
                         0, isAdjacentWithTop,
                         nsnull, floatRS,
                         aReflowStatus, aState);
  } while (NS_SUCCEEDED(rv) && clearanceFrame);

  
  
  if (NS_FRAME_IS_NOT_COMPLETE(aReflowStatus) &&
      (NS_UNCONSTRAINEDSIZE == availSpace.height))
    aReflowStatus = NS_FRAME_COMPLETE;

  
  if (NS_FRAME_OVERFLOW_IS_INCOMPLETE(aReflowStatus))
    NS_FRAME_SET_INCOMPLETE(aReflowStatus);
  
  if (NS_FRAME_IS_COMPLETE(aReflowStatus)) {
    
    
    
    
    nsIFrame* nextInFlow = aPlaceholder->GetNextInFlow();
    if (nextInFlow) {
      static_cast<nsHTMLContainerFrame*>(nextInFlow->GetParent())
        ->DeleteNextInFlowChild(aState.mPresContext, nextInFlow, PR_TRUE);
      
    }
  }
  if (aReflowStatus & NS_FRAME_REFLOW_NEXTINFLOW) {
    aState.mReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
  }

  if (floatFrame->GetType() == nsGkAtoms::letterFrame) {
    
    
    
    if (NS_FRAME_IS_NOT_COMPLETE(aReflowStatus)) 
      aReflowStatus = NS_FRAME_COMPLETE;
  }

  if (NS_FAILED(rv)) {
    return rv;
  }

  
  const nsMargin& m = floatRS.mComputedMargin;
  aFloatMargin.top = brc.GetTopMargin();
  aFloatMargin.right = m.right;
  
  if (NS_FRAME_IS_COMPLETE(aReflowStatus)) {
    brc.GetCarriedOutBottomMargin().Include(m.bottom);
  }
  aFloatMargin.bottom = brc.GetCarriedOutBottomMargin().get();
  aFloatMargin.left = m.left;

  const nsHTMLReflowMetrics& metrics = brc.GetMetrics();

  
  
  
  
  
  
  floatFrame->SetSize(nsSize(metrics.width, metrics.height));
  if (floatFrame->HasView()) {
    nsContainerFrame::SyncFrameViewAfterReflow(aState.mPresContext, floatFrame,
                                               floatFrame->GetView(),
                                               &metrics.mOverflowArea,
                                               NS_FRAME_NO_MOVE_VIEW);
  }
  
  floatFrame->DidReflow(aState.mPresContext, &floatRS,
                        NS_FRAME_REFLOW_FINISHED);

#ifdef NOISY_FLOAT
  printf("end ReflowFloat %p, sized to %d,%d\n",
         floatFrame, metrics.width, metrics.height);
#endif

  
  
  
  nsIFrame* prevPlaceholder = aPlaceholder->GetPrevInFlow();
  if (prevPlaceholder) {
    
    PRBool lastPlaceholder = PR_TRUE;
    nsIFrame* next = aPlaceholder->GetNextSibling();
    if (next) {
      if (nsGkAtoms::placeholderFrame == next->GetType()) {
        lastPlaceholder = PR_FALSE;
      }
    }
    if (lastPlaceholder) {
      
      if (GetPrevInFlow()) {
        
        nsBlockFrame* prevBlock = static_cast<nsBlockFrame*>(GetPrevInFlow());
        line_iterator endLine = prevBlock->end_lines();
        if (endLine != prevBlock->begin_lines()) {
          --endLine;
          if (endLine->HasFloatBreakAfter())
            aState.mFloatBreakType = endLine->GetBreakTypeAfter();
        }
      }
      else NS_ASSERTION(PR_FALSE, "no prev in flow");
    }
  }
  return NS_OK;
}




PRIntn
nsBlockFrame::GetSkipSides() const
{
  if (IS_TRUE_OVERFLOW_CONTAINER(this))
    return (1 << NS_SIDE_TOP) | (1 << NS_SIDE_BOTTOM);

  PRIntn skip = 0;
  if (GetPrevInFlow()) {
    skip |= 1 << NS_SIDE_TOP;
  }
  nsIFrame* nif = GetNextInFlow();
  if (nif && !IS_TRUE_OVERFLOW_CONTAINER(nif)) {
    skip |= 1 << NS_SIDE_BOTTOM;
  }
  return skip;
}

#ifdef DEBUG
static void ComputeCombinedArea(nsLineList& aLines,
                                nscoord aWidth, nscoord aHeight,
                                nsRect& aResult)
{
  nscoord xa = 0, ya = 0, xb = aWidth, yb = aHeight;
  for (nsLineList::iterator line = aLines.begin(), line_end = aLines.end();
       line != line_end;
       ++line) {
    
    
    nsRect lineCombinedArea(line->GetCombinedArea());
    nscoord x = lineCombinedArea.x;
    nscoord y = lineCombinedArea.y;
    nscoord xmost = x + lineCombinedArea.width;
    nscoord ymost = y + lineCombinedArea.height;
    if (x < xa) {
      xa = x;
    }
    if (xmost > xb) {
      xb = xmost;
    }
    if (y < ya) {
      ya = y;
    }
    if (ymost > yb) {
      yb = ymost;
    }
  }

  aResult.x = xa;
  aResult.y = ya;
  aResult.width = xb - xa;
  aResult.height = yb - ya;
}
#endif

PRBool
nsBlockFrame::IsVisibleInSelection(nsISelection* aSelection)
{
  nsCOMPtr<nsIDOMHTMLHtmlElement> html(do_QueryInterface(mContent));
  nsCOMPtr<nsIDOMHTMLBodyElement> body(do_QueryInterface(mContent));
  if (html || body)
    return PR_TRUE;

  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mContent));
  PRBool visible;
  nsresult rv = aSelection->ContainsNode(node, PR_TRUE, &visible);
  return NS_SUCCEEDED(rv) && visible;
}

 void
nsBlockFrame::PaintTextDecorationLine(gfxContext* aCtx, 
                                      const nsPoint& aPt,
                                      nsLineBox* aLine,
                                      nscolor aColor, 
                                      gfxFloat aOffset, 
                                      gfxFloat aAscent, 
                                      gfxFloat aSize,
                                      const PRUint8 aDecoration) 
{
  NS_ASSERTION(!aLine->IsBlock(), "Why did we ask for decorations on a block?");

  nscoord start = aLine->mBounds.x;
  nscoord width = aLine->mBounds.width;

  if (!GetPrevContinuation() && aLine == begin_lines().get()) {
    
    
    nscoord indent = 0;
    const nsStyleText* styleText = GetStyleText();
    nsStyleUnit unit = styleText->mTextIndent.GetUnit();
    if (eStyleUnit_Coord == unit) {
      indent = styleText->mTextIndent.GetCoordValue();
    } else if (eStyleUnit_Percent == unit) {
      
      nsIFrame* containingBlock =
        nsHTMLReflowState::GetContainingBlockFor(this);
      NS_ASSERTION(containingBlock, "Must have containing block!");
      indent = nscoord(styleText->mTextIndent.GetPercentValue() *
                       containingBlock->GetContentRect().width);
    }

    
    
    
    
    start += indent;
    width -= indent;
  }
      
  
  if (width > 0) {
    gfxPoint pt(PresContext()->AppUnitsToGfxUnits(start + aPt.x),
                PresContext()->AppUnitsToGfxUnits(aLine->mBounds.y + aPt.y));
    gfxSize size(PresContext()->AppUnitsToGfxUnits(width), aSize);
    nsCSSRendering::PaintDecorationLine(
      aCtx, aColor, pt, size,
      PresContext()->AppUnitsToGfxUnits(aLine->GetAscent()),
      aOffset, aDecoration, NS_STYLE_BORDER_STYLE_SOLID);
  }
}

#ifdef DEBUG
static void DebugOutputDrawLine(PRInt32 aDepth, nsLineBox* aLine, PRBool aDrawn) {
  if (nsBlockFrame::gNoisyDamageRepair) {
    nsFrame::IndentBy(stdout, aDepth+1);
    nsRect lineArea = aLine->GetCombinedArea();
    printf("%s line=%p bounds=%d,%d,%d,%d ca=%d,%d,%d,%d\n",
           aDrawn ? "draw" : "skip",
           static_cast<void*>(aLine),
           aLine->mBounds.x, aLine->mBounds.y,
           aLine->mBounds.width, aLine->mBounds.height,
           lineArea.x, lineArea.y,
           lineArea.width, lineArea.height);
  }
}
#endif

static nsresult
DisplayLine(nsDisplayListBuilder* aBuilder, const nsRect& aLineArea,
            const nsRect& aDirtyRect, nsBlockFrame::line_iterator& aLine,
            PRInt32 aDepth, PRInt32& aDrawnLines, const nsDisplayListSet& aLists,
            nsBlockFrame* aFrame) {
  
  
  
  PRBool intersect = aLineArea.Intersects(aDirtyRect);
#ifdef DEBUG
  if (nsBlockFrame::gLamePaintMetrics) {
    aDrawnLines++;
  }
  DebugOutputDrawLine(aDepth, aLine.get(), intersect);
#endif
  
  
  
  if (!intersect &&
      !(aFrame->GetStateBits() & NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO))
    return NS_OK;

  nsresult rv;
  nsDisplayList aboveTextDecorations;
  PRBool lineInline = aLine->IsInline();
  if (lineInline) {
    
    
    rv = aFrame->DisplayTextDecorations(aBuilder, aLists.Content(),
                                        &aboveTextDecorations, aLine);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  nsDisplayListSet childLists(aLists,
      lineInline ? aLists.Content() : aLists.BlockBorderBackgrounds());
  nsIFrame* kid = aLine->mFirstChild;
  PRInt32 n = aLine->GetChildCount();
  while (--n >= 0) {
    rv = aFrame->BuildDisplayListForChild(aBuilder, kid, aDirtyRect, childLists,
                                          lineInline ? nsIFrame::DISPLAY_CHILD_INLINE : 0);
    NS_ENSURE_SUCCESS(rv, rv);
    kid = kid->GetNextSibling();
  }
  
  aLists.Content()->AppendToTop(&aboveTextDecorations);
  return NS_OK;
}

NS_IMETHODIMP
nsBlockFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                               const nsRect&           aDirtyRect,
                               const nsDisplayListSet& aLists)
{
  PRInt32 drawnLines; 
  PRInt32 depth = 0;
#ifdef DEBUG
  if (gNoisyDamageRepair) {
      depth = GetDepth();
      nsRect ca;
      ::ComputeCombinedArea(mLines, mRect.width, mRect.height, ca);
      nsFrame::IndentBy(stdout, depth);
      ListTag(stdout);
      printf(": bounds=%d,%d,%d,%d dirty(absolute)=%d,%d,%d,%d ca=%d,%d,%d,%d\n",
             mRect.x, mRect.y, mRect.width, mRect.height,
             aDirtyRect.x, aDirtyRect.y, aDirtyRect.width, aDirtyRect.height,
             ca.x, ca.y, ca.width, ca.height);
  }
  PRTime start = LL_ZERO; 
  if (gLamePaintMetrics) {
    start = PR_Now();
    drawnLines = 0;
  }
#endif

  DisplayBorderBackgroundOutline(aBuilder, aLists);

  if (GetPrevInFlow()) {
    DisplayOverflowContainers(aBuilder, aDirtyRect, aLists);
  }

  aBuilder->MarkFramesForDisplayList(this, mFloats.FirstChild(), aDirtyRect);
  aBuilder->MarkFramesForDisplayList(this, mAbsoluteContainer.GetFirstChild(), aDirtyRect);

  
  
  
  nsLineBox* cursor = GetStateBits() & NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO
    ? nsnull : GetFirstLineContaining(aDirtyRect.y);
  line_iterator line_end = end_lines();
  nsresult rv = NS_OK;
  
  if (cursor) {
    for (line_iterator line = mLines.begin(cursor);
         line != line_end;
         ++line) {
      nsRect lineArea = line->GetCombinedArea();
      if (!lineArea.IsEmpty()) {
        
        
        if (lineArea.y >= aDirtyRect.YMost()) {
          break;
        }
        rv = DisplayLine(aBuilder, lineArea, aDirtyRect, line, depth, drawnLines,
                         aLists, this);
        if (NS_FAILED(rv))
          break;
      }
    }
  } else {
    PRBool nonDecreasingYs = PR_TRUE;
    PRInt32 lineCount = 0;
    nscoord lastY = PR_INT32_MIN;
    nscoord lastYMost = PR_INT32_MIN;
    for (line_iterator line = begin_lines();
         line != line_end;
         ++line) {
      nsRect lineArea = line->GetCombinedArea();
      rv = DisplayLine(aBuilder, lineArea, aDirtyRect, line, depth, drawnLines,
                       aLists, this);
      if (NS_FAILED(rv))
        break;
      if (!lineArea.IsEmpty()) {
        if (lineArea.y < lastY
            || lineArea.YMost() < lastYMost) {
          nonDecreasingYs = PR_FALSE;
        }
        lastY = lineArea.y;
        lastYMost = lineArea.YMost();
      }
      lineCount++;
    }

    if (NS_SUCCEEDED(rv) && nonDecreasingYs && lineCount >= MIN_LINES_NEEDING_CURSOR) {
      SetupLineCursor();
    }
  }

  if (NS_SUCCEEDED(rv) && (nsnull != mBullet) && HaveOutsideBullet()) {
    
    rv = BuildDisplayListForChild(aBuilder, mBullet, aDirtyRect, aLists);
  }

#ifdef DEBUG
  if (gLamePaintMetrics) {
    PRTime end = PR_Now();

    PRInt32 numLines = mLines.size();
    if (!numLines) numLines = 1;
    PRTime lines, deltaPerLine, delta;
    LL_I2L(lines, numLines);
    LL_SUB(delta, end, start);
    LL_DIV(deltaPerLine, delta, lines);

    ListTag(stdout);
    char buf[400];
    PR_snprintf(buf, sizeof(buf),
                ": %lld elapsed (%lld per line) lines=%d drawn=%d skip=%d",
                delta, deltaPerLine,
                numLines, drawnLines, numLines - drawnLines);
    printf("%s\n", buf);
  }
#endif

  return rv;
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsBlockFrame::GetAccessible(nsIAccessible** aAccessible)
{
  *aAccessible = nsnull;
  nsCOMPtr<nsIAccessibilityService> accService = 
    do_GetService("@mozilla.org/accessibilityService;1");
  NS_ENSURE_TRUE(accService, NS_ERROR_FAILURE);

  
  if (mContent->Tag() == nsGkAtoms::hr) {
    return accService->CreateHTMLHRAccessible(static_cast<nsIFrame*>(this), aAccessible);
  }

  nsPresContext *aPresContext = PresContext();
  if (!mBullet || !aPresContext) {
    if (!mContent->GetParent()) {
      
      
      return NS_ERROR_FAILURE;
    }
    
    nsCOMPtr<nsIDOMHTMLDocument> htmlDoc =
      do_QueryInterface(mContent->GetDocument());
    if (htmlDoc) {
      nsCOMPtr<nsIDOMHTMLElement> body;
      htmlDoc->GetBody(getter_AddRefs(body));
      if (SameCOMIdentity(body, mContent)) {
        
        
        return NS_ERROR_FAILURE;
      }
    }

    
    return accService->CreateHyperTextAccessible(static_cast<nsIFrame*>(this), aAccessible);
  }

  
  const nsStyleList* myList = GetStyleList();
  nsAutoString bulletText;
  if (myList->mListStyleImage || myList->mListStyleType == NS_STYLE_LIST_STYLE_DISC ||
      myList->mListStyleType == NS_STYLE_LIST_STYLE_CIRCLE ||
      myList->mListStyleType == NS_STYLE_LIST_STYLE_SQUARE) {
    bulletText.Assign(PRUnichar(0x2022));; 
  }
  else if (myList->mListStyleType != NS_STYLE_LIST_STYLE_NONE) {
    mBullet->GetListItemText(*myList, bulletText);
  }

  return accService->CreateHTMLLIAccessible(static_cast<nsIFrame*>(this), 
                                            static_cast<nsIFrame*>(mBullet), 
                                            bulletText,
                                            aAccessible);
}
#endif

void nsBlockFrame::ClearLineCursor() {
  if (!(GetStateBits() & NS_BLOCK_HAS_LINE_CURSOR)) {
    return;
  }

  UnsetProperty(nsGkAtoms::lineCursorProperty);
  RemoveStateBits(NS_BLOCK_HAS_LINE_CURSOR);
}

void nsBlockFrame::SetupLineCursor() {
  if (GetStateBits() & NS_BLOCK_HAS_LINE_CURSOR
      || mLines.empty()) {
    return;
  }
   
  SetProperty(nsGkAtoms::lineCursorProperty,
              mLines.front(), nsnull);
  AddStateBits(NS_BLOCK_HAS_LINE_CURSOR);
}

nsLineBox* nsBlockFrame::GetFirstLineContaining(nscoord y) {
  if (!(GetStateBits() & NS_BLOCK_HAS_LINE_CURSOR)) {
    return nsnull;
  }

  nsLineBox* property = static_cast<nsLineBox*>
                                   (GetProperty(nsGkAtoms::lineCursorProperty));
  line_iterator cursor = mLines.begin(property);
  nsRect cursorArea = cursor->GetCombinedArea();

  while ((cursorArea.IsEmpty() || cursorArea.YMost() > y)
         && cursor != mLines.front()) {
    cursor = cursor.prev();
    cursorArea = cursor->GetCombinedArea();
  }
  while ((cursorArea.IsEmpty() || cursorArea.YMost() <= y)
         && cursor != mLines.back()) {
    cursor = cursor.next();
    cursorArea = cursor->GetCombinedArea();
  }

  if (cursor.get() != property) {
    SetProperty(nsGkAtoms::lineCursorProperty,
                cursor.get(), nsnull);
  }

  return cursor.get();
}

 void
nsBlockFrame::ChildIsDirty(nsIFrame* aChild)
{
  
  if (aChild->GetStateBits() & NS_FRAME_OUT_OF_FLOW &&
      aChild->GetStyleDisplay()->IsAbsolutelyPositioned()) {
    
  } else if (aChild == mBullet && HaveOutsideBullet()) {
    
    
    
    line_iterator bulletLine = begin_lines();
    if (bulletLine != end_lines() && bulletLine->mBounds.height == 0 &&
        bulletLine != mLines.back()) {
      bulletLine = bulletLine.next();
    }
    
    if (bulletLine != end_lines()) {
      MarkLineDirty(bulletLine);
    }
    
    
  } else {
    
    
    
    PRBool isValid;
    nsBlockInFlowLineIterator iter(this, aChild, &isValid);
    if (isValid) {
      iter.GetContainer()->MarkLineDirty(iter.GetLine(), iter.GetLineList());
    }
  }

  nsBlockFrameSuper::ChildIsDirty(aChild);
}




#ifdef NS_DEBUG
static PRBool
InLineList(nsLineList& aLines, nsIFrame* aFrame)
{
  for (nsLineList::iterator line = aLines.begin(), line_end = aLines.end();
       line != line_end;
       ++line) {
    nsIFrame* frame = line->mFirstChild;
    PRInt32 n = line->GetChildCount();
    while (--n >= 0) {
      if (frame == aFrame) {
        return PR_TRUE;
      }
      frame = frame->GetNextSibling();
    }
  }
  return PR_FALSE;
}

static PRBool
InSiblingList(nsLineList& aLines, nsIFrame* aFrame)
{
  if (! aLines.empty()) {
    nsIFrame* frame = aLines.front()->mFirstChild;
    while (frame) {
      if (frame == aFrame) {
        return PR_TRUE;
      }
      frame = frame->GetNextSibling();
    }
  }
  return PR_FALSE;
}

NS_IMETHODIMP
nsBlockFrame::VerifyTree() const
{
  
  return NS_OK;
}
#endif




NS_IMETHODIMP
nsBlockFrame::Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        aPrevInFlow)
{
  if (aPrevInFlow) {
    
    nsBlockFrame*  blockFrame = (nsBlockFrame*)aPrevInFlow;

    SetFlags(blockFrame->mState &
             (NS_BLOCK_FLAGS_MASK & ~NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET));
  }

  nsresult rv = nsBlockFrameSuper::Init(aContent, aParent, aPrevInFlow);

  if (!aPrevInFlow ||
      aPrevInFlow->GetStateBits() & NS_BLOCK_NEEDS_BIDI_RESOLUTION)
    AddStateBits(NS_BLOCK_NEEDS_BIDI_RESOLUTION);

  return rv;
}

NS_IMETHODIMP
nsBlockFrame::SetInitialChildList(nsIAtom*        aListName,
                                  nsIFrame*       aChildList)
{
  nsresult rv = NS_OK;

  if (nsGkAtoms::absoluteList == aListName) {
    mAbsoluteContainer.SetInitialChildList(this, aListName, aChildList);
  }
  else if (nsGkAtoms::floatList == aListName) {
    mFloats.SetFrames(aChildList);
  }
  else {
    nsPresContext* presContext = PresContext();

#ifdef DEBUG
    
    
    
    
    
    
    
    nsIAtom *pseudo = GetStyleContext()->GetPseudoType();
    PRBool haveFirstLetterStyle =
      (!pseudo ||
       (pseudo == nsCSSAnonBoxes::cellContent &&
        mParent->GetStyleContext()->GetPseudoType() == nsnull) ||
       pseudo == nsCSSAnonBoxes::fieldsetContent ||
       pseudo == nsCSSAnonBoxes::scrolledContent ||
       pseudo == nsCSSAnonBoxes::columnContent) &&
      !IsFrameOfType(eMathML) &&
      nsRefPtr<nsStyleContext>(GetFirstLetterStyle(presContext)) != nsnull;
    NS_ASSERTION(haveFirstLetterStyle ==
                 ((mState & NS_BLOCK_HAS_FIRST_LETTER_STYLE) != 0),
                 "NS_BLOCK_HAS_FIRST_LETTER_STYLE state out of sync");
#endif
    
    rv = AddFrames(aChildList, nsnull);
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    
    
    const nsStyleDisplay* styleDisplay = GetStyleDisplay();
    if ((nsnull == GetPrevInFlow()) &&
        (NS_STYLE_DISPLAY_LIST_ITEM == styleDisplay->mDisplay) &&
        (nsnull == mBullet)) {
      
      const nsStyleList* styleList = GetStyleList();
      nsIAtom *pseudoElement;
      switch (styleList->mListStyleType) {
        case NS_STYLE_LIST_STYLE_DISC:
        case NS_STYLE_LIST_STYLE_CIRCLE:
        case NS_STYLE_LIST_STYLE_SQUARE:
          pseudoElement = nsCSSPseudoElements::mozListBullet;
          break;
        default:
          pseudoElement = nsCSSPseudoElements::mozListNumber;
          break;
      }

      nsIPresShell *shell = presContext->PresShell();

      nsStyleContext* parentStyle =
        CorrectStyleParentFrame(this, pseudoElement)->GetStyleContext();
      nsRefPtr<nsStyleContext> kidSC = shell->StyleSet()->
        ResolvePseudoStyleFor(mContent, pseudoElement, parentStyle);

      
      nsBulletFrame* bullet = new (shell) nsBulletFrame(kidSC);
      if (nsnull == bullet) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      bullet->Init(mContent, this, nsnull);

      
      
      if (NS_STYLE_LIST_STYLE_POSITION_INSIDE ==
          styleList->mListStylePosition) {
        AddFrames(bullet, nsnull);
        mState &= ~NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET;
      }
      else {
        mState |= NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET;
      }

      mBullet = bullet;
    }
  }

  return NS_OK;
}


PRBool
nsBlockFrame::FrameStartsCounterScope(nsIFrame* aFrame)
{
  nsIContent* content = aFrame->GetContent();
  if (!content || !content->IsNodeOfType(nsINode::eHTML))
    return PR_FALSE;

  nsIAtom *localName = content->NodeInfo()->NameAtom();
  return localName == nsGkAtoms::ol ||
         localName == nsGkAtoms::ul ||
         localName == nsGkAtoms::dir ||
         localName == nsGkAtoms::menu;
}

PRBool
nsBlockFrame::RenumberLists(nsPresContext* aPresContext)
{
  if (!FrameStartsCounterScope(this)) {
    
    
    return PR_FALSE;
  }

  
  
  PRInt32 ordinal = 1;

  nsGenericHTMLElement *hc = nsGenericHTMLElement::FromContent(mContent);

  if (hc) {
    const nsAttrValue* attr = hc->GetParsedAttr(nsGkAtoms::start);
    if (attr && attr->Type() == nsAttrValue::eInteger) {
      ordinal = attr->GetIntegerValue();
    }
  }

  
  nsBlockFrame* block = (nsBlockFrame*) GetFirstInFlow();
  return RenumberListsInBlock(aPresContext, block, &ordinal, 0);
}

PRBool
nsBlockFrame::RenumberListsInBlock(nsPresContext* aPresContext,
                                   nsBlockFrame* aBlockFrame,
                                   PRInt32* aOrdinal,
                                   PRInt32 aDepth)
{
  
  PRBool foundValidLine;
  nsBlockInFlowLineIterator bifLineIter(aBlockFrame, &foundValidLine);
  
  if (!foundValidLine)
    return PR_FALSE;

  PRBool renumberedABullet = PR_FALSE;

  do {
    nsLineList::iterator line = bifLineIter.GetLine();
    nsIFrame* kid = line->mFirstChild;
    PRInt32 n = line->GetChildCount();
    while (--n >= 0) {
      PRBool kidRenumberedABullet = RenumberListsFor(aPresContext, kid, aOrdinal, aDepth);
      if (kidRenumberedABullet) {
        line->MarkDirty();
        renumberedABullet = PR_TRUE;
      }
      kid = kid->GetNextSibling();
    }
  } while (bifLineIter.Next());

  return renumberedABullet;
}

PRBool
nsBlockFrame::RenumberListsFor(nsPresContext* aPresContext,
                               nsIFrame* aKid,
                               PRInt32* aOrdinal,
                               PRInt32 aDepth)
{
  NS_PRECONDITION(aPresContext && aKid && aOrdinal, "null params are immoral!");

  
  if (MAX_DEPTH_FOR_LIST_RENUMBERING < aDepth)
    return PR_FALSE;

  
  nsIFrame* kid = nsPlaceholderFrame::GetRealFrameFor(aKid);

  
  kid = kid->GetContentInsertionFrame();

  
  if (!kid)
    return PR_FALSE;

  PRBool kidRenumberedABullet = PR_FALSE;

  
  
  
  const nsStyleDisplay* display = kid->GetStyleDisplay();
  if (NS_STYLE_DISPLAY_LIST_ITEM == display->mDisplay) {
    
    
    nsBlockFrame* listItem = nsLayoutUtils::GetAsBlock(kid);
    if (listItem) {
      if (nsnull != listItem->mBullet) {
        PRBool changed;
        *aOrdinal = listItem->mBullet->SetListItemOrdinal(*aOrdinal,
                                                          &changed);
        if (changed) {
          kidRenumberedABullet = PR_TRUE;

          
          listItem->ChildIsDirty(listItem->mBullet);
        }
      }

      
      
      
      PRBool meToo = RenumberListsInBlock(aPresContext, listItem, aOrdinal, aDepth + 1);
      if (meToo) {
        kidRenumberedABullet = PR_TRUE;
      }
    }
  }
  else if (NS_STYLE_DISPLAY_BLOCK == display->mDisplay) {
    if (FrameStartsCounterScope(kid)) {
      
      
      
    }
    else {
      
      
      nsBlockFrame* kidBlock = nsLayoutUtils::GetAsBlock(kid);
      if (kidBlock) {
        kidRenumberedABullet = RenumberListsInBlock(aPresContext, kidBlock, aOrdinal, aDepth + 1);
      }
    }
  }
  return kidRenumberedABullet;
}

void
nsBlockFrame::ReflowBullet(nsBlockReflowState& aState,
                           nsHTMLReflowMetrics& aMetrics,
                           nscoord aLineTop)
{
  const nsHTMLReflowState &rs = aState.mReflowState;

  
  nsSize availSize;
  
  availSize.width = rs.ComputedWidth();
  availSize.height = NS_UNCONSTRAINEDSIZE;

  
  
  
  nsHTMLReflowState reflowState(aState.mPresContext, rs,
                                mBullet, availSize);
  nsReflowStatus  status;
  mBullet->WillReflow(aState.mPresContext);
  mBullet->Reflow(aState.mPresContext, aMetrics, reflowState, status);

  
  
  
  
  
  
  
  nscoord x = rs.mStyleVisibility->mDirection == NS_STYLE_DIRECTION_LTR ?
    PR_MIN(aState.mOutsideBulletX, aState.mAvailSpaceRect.x)
      - reflowState.mComputedMargin.right - aMetrics.width :
    PR_MAX(aState.mOutsideBulletX, aState.mAvailSpaceRect.XMost())
      + reflowState.mComputedMargin.left;

  
  
  aState.GetAvailableSpace();

  
  
  const nsMargin& bp = aState.BorderPadding();
  nscoord y = bp.top;
  mBullet->SetRect(nsRect(x, y, aMetrics.width, aMetrics.height));
  mBullet->DidReflow(aState.mPresContext, &aState.mReflowState, NS_FRAME_REFLOW_FINISHED);
}





void nsBlockFrame::CollectFloats(nsIFrame* aFrame, nsFrameList& aList, nsIFrame** aTail,
                                 PRBool aFromOverflow, PRBool aCollectSiblings) {
  while (aFrame) {
    
    if (!aFrame->IsFloatContainingBlock()) {
      nsIFrame *outOfFlowFrame = nsLayoutUtils::GetFloatFromPlaceholder(aFrame);
      if (outOfFlowFrame) {
        
        
        
        
        NS_ASSERTION(outOfFlowFrame->GetParent() == this,
                     "Out of flow frame doesn't have the expected parent");
        if (aFromOverflow) {
          nsAutoOOFFrameList oofs(this);
          oofs.mList.RemoveFrame(outOfFlowFrame);
        } else {
          mFloats.RemoveFrame(outOfFlowFrame);
        }
        aList.InsertFrame(nsnull, *aTail, outOfFlowFrame);
        *aTail = outOfFlowFrame;
      }

      CollectFloats(aFrame->GetFirstChild(nsnull), 
                    aList, aTail, aFromOverflow, PR_TRUE);
      
      
      
      CollectFloats(aFrame->GetFirstChild(nsGkAtoms::overflowList), 
                    aList, aTail, aFromOverflow, PR_TRUE);
    }
    if (!aCollectSiblings)
      break;
    aFrame = aFrame->GetNextSibling();
  }
}

void
nsBlockFrame::CheckFloats(nsBlockReflowState& aState)
{
#ifdef DEBUG
  
  
  
  PRBool anyLineDirty = PR_FALSE;

  
  nsAutoVoidArray lineFloats;
  for (line_iterator line = begin_lines(), line_end = end_lines();
       line != line_end; ++line) {
    if (line->HasFloats()) {
      nsFloatCache* fc = line->GetFirstFloat();
      while (fc) {
        nsIFrame* floatFrame = fc->mPlaceholder->GetOutOfFlowFrame();
        lineFloats.AppendElement(floatFrame);
        fc = fc->Next();
      }
    }
    if (line->IsDirty()) {
      anyLineDirty = PR_TRUE;
    }
  }
  
  nsAutoVoidArray storedFloats;
  PRBool equal = PR_TRUE;
  PRInt32 i = 0;
  for (nsIFrame* f = mFloats.FirstChild(); f; f = f->GetNextSibling()) {
    storedFloats.AppendElement(f);
    if (i < lineFloats.Count() && lineFloats.ElementAt(i) != f) {
      equal = PR_FALSE;
    }
    ++i;
  }

  if ((!equal || lineFloats.Count() != storedFloats.Count()) && !anyLineDirty) {
    NS_WARNING("nsBlockFrame::CheckFloats: Explicit float list is out of sync with float cache");
#if defined(DEBUG_roc)
    nsIFrameDebug::RootFrameList(PresContext(), stdout, 0);
    for (i = 0; i < lineFloats.Count(); ++i) {
      printf("Line float: %p\n", lineFloats.ElementAt(i));
    }
    for (i = 0; i < storedFloats.Count(); ++i) {
      printf("Stored float: %p\n", storedFloats.ElementAt(i));
    }
#endif
  }
#endif

  nsFrameList oofs = GetOverflowOutOfFlows();
  if (oofs.NotEmpty()) {
    
    
    
    
    
    
    
    
    
    
    aState.mSpaceManager->RemoveTrailingRegions(oofs.FirstChild());
  }
}


PRBool
nsBlockFrame::BlockIsMarginRoot(nsIFrame* aBlock)
{
  NS_PRECONDITION(aBlock, "Must have a frame");
  NS_ASSERTION(nsLayoutUtils::GetAsBlock(aBlock), "aBlock must be a block");

  nsIFrame* parent = aBlock->GetParent();
  return (aBlock->GetStateBits() & NS_BLOCK_MARGIN_ROOT) ||
    (parent && !parent->IsFloatContainingBlock() &&
     parent->GetType() != nsGkAtoms::columnSetFrame);
}


PRBool
nsBlockFrame::BlockNeedsSpaceManager(nsIFrame* aBlock)
{
  NS_PRECONDITION(aBlock, "Must have a frame");
  NS_ASSERTION(nsLayoutUtils::GetAsBlock(aBlock), "aBlock must be a block");

  nsIFrame* parent = aBlock->GetParent();
  return (aBlock->GetStateBits() & NS_BLOCK_SPACE_MGR) ||
    (parent && !parent->IsFloatContainingBlock());
}


PRBool
nsBlockFrame::BlockCanIntersectFloats(nsIFrame* aFrame)
{
  return aFrame->IsFrameOfType(nsIFrame::eBlockFrame) &&
         !aFrame->IsFrameOfType(nsIFrame::eReplaced) &&
         !(aFrame->GetStateBits() & NS_BLOCK_SPACE_MGR);
}






nsBlockFrame::ReplacedElementWidthToClear
nsBlockFrame::WidthToClearPastFloats(nsBlockReflowState& aState,
                                     nsIFrame* aFrame)
{
  nscoord leftOffset, rightOffset;
  nsCSSOffsetState offsetState(aFrame, aState.mReflowState.rendContext,
                               aState.mContentArea.width);

  ReplacedElementWidthToClear result;
  
  
  if (aFrame->GetType() == nsGkAtoms::tableOuterFrame) {
    nsIFrame *innerTable = aFrame->GetFirstChild(nsnull);
    nsIFrame *caption = aFrame->GetFirstChild(nsGkAtoms::captionList);

    nsMargin tableMargin, captionMargin;
    {
      nsCSSOffsetState tableOS(innerTable, aState.mReflowState.rendContext,
                               aState.mContentArea.width);
      tableMargin = tableOS.mComputedMargin;
    }

    if (caption) {
      nsCSSOffsetState captionOS(caption, aState.mReflowState.rendContext,
                                 aState.mContentArea.width);
      captionMargin = captionOS.mComputedMargin;
    }

    PRUint8 captionSide;
    if (!caption ||
        ((captionSide = caption->GetStyleTableBorder()->mCaptionSide)
           == NS_STYLE_CAPTION_SIDE_TOP ||
         captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM)) {
      result.marginLeft = tableMargin.left;
      result.marginRight = tableMargin.right;
    } else if (captionSide == NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE ||
               captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE) {
      
      
      
      result.marginLeft  = PR_MIN(tableMargin.left,  captionMargin.left);
      result.marginRight = PR_MIN(tableMargin.right, captionMargin.right);
    } else {
      NS_ASSERTION(captionSide == NS_STYLE_CAPTION_SIDE_LEFT ||
                   captionSide == NS_STYLE_CAPTION_SIDE_RIGHT,
                   "unexpected caption-side");
      if (captionSide == NS_STYLE_CAPTION_SIDE_LEFT) {
        result.marginLeft = captionMargin.left;
        result.marginRight = tableMargin.right;
      } else {
        result.marginLeft = tableMargin.left;
        result.marginRight = captionMargin.right;
      }
    }

    aState.ComputeReplacedBlockOffsetsForFloats(aFrame, leftOffset, rightOffset,
                                                &result);

    
    nscoord availWidth = aState.mContentArea.width - leftOffset - rightOffset;
    
    
    result.borderBoxWidth =
      aFrame->ComputeSize(aState.mReflowState.rendContext,
                          nsSize(aState.mContentArea.width,
                                 NS_UNCONSTRAINEDSIZE),
                          availWidth,
                          nsSize(offsetState.mComputedMargin.LeftRight(),
                                 offsetState.mComputedMargin.TopBottom()),
                          nsSize(offsetState.mComputedBorderPadding.LeftRight() -
                                   offsetState.mComputedPadding.LeftRight(),
                                 offsetState.mComputedBorderPadding.TopBottom() -
                                   offsetState.mComputedPadding.TopBottom()),
                          nsSize(offsetState.mComputedPadding.LeftRight(),
                                 offsetState.mComputedPadding.TopBottom()),
                          PR_TRUE).width +
      offsetState.mComputedBorderPadding.LeftRight() -
      (result.marginLeft + result.marginRight);
  } else {
    aState.ComputeReplacedBlockOffsetsForFloats(aFrame, leftOffset, rightOffset);
    nscoord availWidth = aState.mContentArea.width - leftOffset - rightOffset;

    
    
    
    
    
    
    nsSize availSpace(availWidth, NS_UNCONSTRAINEDSIZE);
    nsHTMLReflowState reflowState(aState.mPresContext, aState.mReflowState,
                                  aFrame, availSpace);
    result.borderBoxWidth = reflowState.ComputedWidth() +
                            reflowState.mComputedBorderPadding.LeftRight();
    
    
    result.marginLeft  = offsetState.mComputedMargin.left;
    result.marginRight = offsetState.mComputedMargin.right;
  }
  return result;
}
 

nsBlockFrame*
nsBlockFrame::GetNearestAncestorBlock(nsIFrame* aCandidate)
{
  nsBlockFrame* block = nsnull;
  while(aCandidate) {
    block = nsLayoutUtils::GetAsBlock(aCandidate);
    if (block) { 
      
      return block;
    }
    
    aCandidate = aCandidate->GetParent();
  }
  NS_NOTREACHED("Fell off frame tree looking for ancestor block!");
  return nsnull;
}

#ifdef IBMBIDI
nsresult
nsBlockFrame::ResolveBidi()
{
  NS_ASSERTION(!GetPrevInFlow(),
               "ResolveBidi called on non-first continuation");

  nsPresContext* presContext = PresContext();
  if (!presContext->BidiEnabled()) {
    return NS_OK;
  }

  nsBidiPresUtils* bidiUtils = presContext->GetBidiUtils();
  if (!bidiUtils)
    return NS_ERROR_NULL_POINTER;

  return bidiUtils->Resolve(this, IsVisualFormControl(presContext));
}

PRBool
nsBlockFrame::IsVisualFormControl(nsPresContext* aPresContext)
{
  
  
  
  
  
  if (!aPresContext->IsVisualMode()) {
    return PR_FALSE;
  }

  PRUint32 options = aPresContext->GetBidi();
  if (IBMBIDI_CONTROLSTEXTMODE_LOGICAL != GET_BIDI_OPTION_CONTROLSTEXTMODE(options)) {
    return PR_FALSE;
  }

  nsIContent* content = GetContent();
  for ( ; content; content = content->GetParent()) {
    if (content->IsNodeOfType(nsINode::eHTML_FORM_CONTROL)) {
      return PR_TRUE;
    }
  }
  
  return PR_FALSE;
}
#endif

#ifdef DEBUG
void
nsBlockFrame::VerifyLines(PRBool aFinalCheckOK)
{
  if (!gVerifyLines) {
    return;
  }
  if (mLines.empty()) {
    return;
  }

  
  
  PRInt32 count = 0;
  PRBool seenBlock = PR_FALSE;
  line_iterator line, line_end;
  for (line = begin_lines(), line_end = end_lines();
       line != line_end;
       ++line) {
    if (aFinalCheckOK) {
      NS_ABORT_IF_FALSE(line->GetChildCount(), "empty line");
      if (line->IsBlock()) {
        seenBlock = PR_TRUE;
      }
      if (line->IsBlock()) {
        NS_ASSERTION(1 == line->GetChildCount(), "bad first line");
      }
    }
    count += line->GetChildCount();
  }

  
  PRInt32 frameCount = 0;
  nsIFrame* frame = mLines.front()->mFirstChild;
  while (frame) {
    frameCount++;
    frame = frame->GetNextSibling();
  }
  NS_ASSERTION(count == frameCount, "bad line list");

  
  for (line = begin_lines(), line_end = end_lines();
       line != line_end;
        ) {
    count = line->GetChildCount();
    frame = line->mFirstChild;
    while (--count >= 0) {
      frame = frame->GetNextSibling();
    }
    ++line;
    if ((line != line_end) && (0 != line->GetChildCount())) {
      NS_ASSERTION(frame == line->mFirstChild, "bad line list");
    }
  }
}




void
nsBlockFrame::VerifyOverflowSituation()
{
  nsBlockFrame* flow = (nsBlockFrame*) GetFirstInFlow();
  while (nsnull != flow) {
    nsLineList* overflowLines = GetOverflowLines();
    if (nsnull != overflowLines) {
      NS_ASSERTION(! overflowLines->empty(), "should not be empty if present");
      NS_ASSERTION(overflowLines->front()->mFirstChild, "bad overflow list");
    }
    flow = (nsBlockFrame*) flow->GetNextInFlow();
  }
}

PRInt32
nsBlockFrame::GetDepth() const
{
  PRInt32 depth = 0;
  nsIFrame* parent = mParent;
  while (parent) {
    parent = parent->GetParent();
    depth++;
  }
  return depth;
}
#endif
