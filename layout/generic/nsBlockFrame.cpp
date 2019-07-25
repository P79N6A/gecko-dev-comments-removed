















































#include "nsCOMPtr.h"
#include "nsBlockFrame.h"
#include "nsBlockReflowContext.h"
#include "nsBlockReflowState.h"
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
#include "nsHTMLParts.h"
#include "nsGkAtoms.h"
#include "nsIDOMEvent.h"
#include "nsGenericHTMLElement.h"
#include "prprf.h"
#include "nsStyleChangeList.h"
#include "nsFrameSelection.h"
#include "nsFloatManager.h"
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
#include "nsAccessibilityService.h"
#endif
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"
#include "nsContentErrors.h"
#include "nsCSSAnonBoxes.h"
#include "nsCSSFrameConstructor.h"
#include "nsCSSRendering.h"
#include "FrameLayerBuilder.h"
#include "nsRenderingContext.h"
#include "TextOverflow.h"
#include "mozilla/Util.h" 

#ifdef IBMBIDI
#include "nsBidiPresUtils.h"
#endif 

#include "nsIDOMHTMLBodyElement.h"
#include "nsIDOMHTMLHtmlElement.h"

static const int MIN_LINES_NEEDING_CURSOR = 20;

static const PRUnichar kDiscCharacter = 0x2022;
static const PRUnichar kCircleCharacter = 0x25e6;
static const PRUnichar kSquareCharacter = 0x25aa;

#define DISABLE_FLOAT_BREAKING_IN_COLUMNS

using namespace mozilla;
using namespace mozilla::css;

#ifdef DEBUG
#include "nsBlockDebugFlags.h"

bool nsBlockFrame::gLamePaintMetrics;
bool nsBlockFrame::gLameReflowMetrics;
bool nsBlockFrame::gNoisy;
bool nsBlockFrame::gNoisyDamageRepair;
bool nsBlockFrame::gNoisyIntrinsic;
bool nsBlockFrame::gNoisyReflow;
bool nsBlockFrame::gReallyNoisyReflow;
bool nsBlockFrame::gNoisyFloatManager;
bool nsBlockFrame::gVerifyLines;
bool nsBlockFrame::gDisableResizeOpt;

PRInt32 nsBlockFrame::gNoiseIndent;

struct BlockDebugFlags {
  const char* name;
  bool* on;
};

static const BlockDebugFlags gFlags[] = {
  { "reflow", &nsBlockFrame::gNoisyReflow },
  { "really-noisy-reflow", &nsBlockFrame::gReallyNoisyReflow },
  { "intrinsic", &nsBlockFrame::gNoisyIntrinsic },
  { "float-manager", &nsBlockFrame::gNoisyFloatManager },
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
  static bool firstTime = true;
  if (firstTime) {
    firstTime = false;
    char* flags = PR_GetEnv("GECKO_BLOCK_DEBUG_FLAGS");
    if (flags) {
      bool error = false;
      for (;;) {
        char* cm = PL_strchr(flags, ',');
        if (cm) *cm = '\0';

        bool found = false;
        const BlockDebugFlags* bdf = gFlags;
        const BlockDebugFlags* end = gFlags + NUM_DEBUG_FLAGS;
        for (; bdf < end; bdf++) {
          if (PL_strcasecmp(bdf->name, flags) == 0) {
            *(bdf->on) = true;
            printf("nsBlockFrame: setting %s debug flag on\n", bdf->name);
            gNoisy = true;
            found = true;
            break;
          }
        }
        if (!found) {
          error = true;
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
RecordReflowStatus(bool aChildIsBlock, nsReflowStatus aFrameReflowStatus)
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


static void
DestroyOverflowLines(void* aPropertyValue)
{
  NS_ERROR("Overflow lines should never be destroyed by the FramePropertyTable");
}

NS_DECLARE_FRAME_PROPERTY(LineCursorProperty, nsnull)
NS_DECLARE_FRAME_PROPERTY(OverflowLinesProperty, DestroyOverflowLines)
NS_DECLARE_FRAME_PROPERTY(OverflowOutOfFlowsProperty,
                          nsContainerFrame::DestroyFrameList)



nsIFrame*
NS_NewBlockFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRUint32 aFlags)
{
  nsBlockFrame* it = new (aPresShell) nsBlockFrame(aContext);
  if (it) {
    it->SetFlags(aFlags);
  }
  return it;
}

NS_IMPL_FRAMEARENA_HELPERS(nsBlockFrame)

nsBlockFrame::~nsBlockFrame()
{
}

void
nsBlockFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  DestroyAbsoluteFrames(aDestructRoot);
  
  
  if (mBullet && HaveOutsideBullet()) {
    mBullet->DestroyFrom(aDestructRoot);
    mBullet = nsnull;
  }

  mFloats.DestroyFramesFrom(aDestructRoot);

  nsPresContext* presContext = PresContext();

  nsLineBox::DeleteLineList(presContext, mLines, aDestructRoot);
  
  mFrames.Clear();

  nsFrameList* pushedFloats = RemovePushedFloats();
  if (pushedFloats) {
    pushedFloats->DestroyFrom(aDestructRoot);
  }

  
  nsLineList* overflowLines = RemoveOverflowLines();
  if (overflowLines) {
    nsLineBox::DeleteLineList(presContext, *overflowLines, aDestructRoot);
    delete overflowLines;
  }

  {
    nsAutoOOFFrameList oofs(this);
    oofs.mList.DestroyFramesFrom(aDestructRoot);
    
  }

  nsBlockFrameSuper::DestroyFrom(aDestructRoot);
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

NS_QUERYFRAME_HEAD(nsBlockFrame)
  NS_QUERYFRAME_ENTRY(nsBlockFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBlockFrameSuper)

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
  if (GetNextSibling()) {
    fprintf(out, " next=%p", static_cast<void*>(GetNextSibling()));
  }

  
  if (nsnull != GetPrevInFlow()) {
    fprintf(out, " prev-in-flow=%p", static_cast<void*>(GetPrevInFlow()));
  }
  if (nsnull != GetNextInFlow()) {
    fprintf(out, " next-in-flow=%p", static_cast<void*>(GetNextInFlow()));
  }

  void* IBsibling = Properties().Get(IBSplitSpecialSibling());
  if (IBsibling) {
    fprintf(out, " IBSplitSpecialSibling=%p", IBsibling);
  }
  void* IBprevsibling = Properties().Get(IBSplitSpecialPrevSibling());
  if (IBprevsibling) {
    fprintf(out, " IBSplitSpecialPrevSibling=%p", IBprevsibling);
  }

  if (nsnull != mContent) {
    fprintf(out, " [content=%p]", static_cast<void*>(mContent));
  }

  
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    fprintf(out, " [state=%016llx]", mState);
  }
  nsBlockFrame* f = const_cast<nsBlockFrame*>(this);
  if (f->HasOverflowAreas()) {
    nsRect overflowArea = f->GetVisualOverflowRect();
    fprintf(out, " [vis-overflow=%d,%d,%d,%d]", overflowArea.x, overflowArea.y,
            overflowArea.width, overflowArea.height);
    overflowArea = f->GetScrollableOverflowRect();
    fprintf(out, " [scr-overflow=%d,%d,%d,%d]", overflowArea.x, overflowArea.y,
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
  nsIAtom* pseudoTag = mStyleContext->GetPseudo();
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
    const_line_iterator line = overflowLines->begin(),
                        line_end = overflowLines->end();
    for ( ; line != line_end; ++line) {
      line->List(out, aIndent + 1);
    }
    IndentBy(out, aIndent);
    fputs(">\n", out);
  }

  
  
  ChildListIterator lists(this);
  ChildListIDs skip(kPrincipalList | kOverflowList);
  for (; !lists.IsDone(); lists.Next()) {
    if (skip.Contains(lists.CurrentID())) {
      continue;
    }
    IndentBy(out, aIndent);
    fprintf(out, "%s<\n", mozilla::layout::ChildListName(lists.CurrentID()));
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* kid = childFrames.get();
      kid->List(out, aIndent + 1);
    }
    IndentBy(out, aIndent);
    fputs(">\n", out);
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
  
  
  
  
  if (aForChild) {
    const nsStyleDisplay* disp = GetStyleDisplay();
    nsRect absPosClipRect;
    if (GetAbsPosClipRect(disp, &absPosClipRect, GetSize())) {
      
      
      nsRect r;
      if (r.IntersectRect(aDamageRect, absPosClipRect - nsPoint(aX, aY))) {
        nsBlockFrameSuper::InvalidateInternal(r, aX, aY, this, aFlags);
      }
      return;
    }
  }

  nsBlockFrameSuper::InvalidateInternal(aDamageRect, aX, aY, this, aFlags);
}

nscoord
nsBlockFrame::GetBaseline() const
{
  nscoord result;
  if (nsLayoutUtils::GetLastLineBaseline(this, &result))
    return result;
  return nsFrame::GetBaseline();
}

nscoord
nsBlockFrame::GetCaretBaseline() const
{
  nsRect contentRect = GetContentRect();
  nsMargin bp = GetUsedBorderAndPadding();

  if (!mLines.empty()) {
    const_line_iterator line = begin_lines();
    const nsLineBox* firstLine = line;
    if (firstLine->GetChildCount()) {
      return bp.top + firstLine->mFirstChild->GetCaretBaseline();
    }
  }
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
  return nsLayoutUtils::GetCenteredFontBaseline(fm, nsHTMLReflowState::
      CalcLineHeight(GetStyleContext(), contentRect.height)) +
    bp.top;
}




nsFrameList
nsBlockFrame::GetChildList(ChildListID aListID) const
{
  switch (aListID) {
    case kPrincipalList:
      return mFrames;
    case kOverflowList: {
      
      
      nsLineList* overflowLines = GetOverflowLines();
      return overflowLines ? nsFrameList(overflowLines->front()->mFirstChild,
                                         overflowLines->back()->LastChild())
                           : nsFrameList::EmptyList();
    }
    case kFloatList:
      return mFloats;
    case kOverflowOutOfFlowList: {
      const nsFrameList* list = GetOverflowOutOfFlows();
      return list ? *list : nsFrameList::EmptyList();
    }
    case kPushedFloatsList: {
      const nsFrameList* list = GetPushedFloats();
      return list ? *list : nsFrameList::EmptyList();
    }
    case kBulletList:
      return HaveOutsideBullet() ? nsFrameList(mBullet, mBullet)
                                 : nsFrameList::EmptyList();
    default:
      return nsContainerFrame::GetChildList(aListID);
  }
}

void
nsBlockFrame::GetChildLists(nsTArray<ChildList>* aLists) const
{
  nsContainerFrame::GetChildLists(aLists);
  nsLineList* overflowLines = GetOverflowLines();
  if (overflowLines && overflowLines->front()->mFirstChild) {
    nsFrameList overflowList(overflowLines->front()->mFirstChild,
                             overflowLines->back()->LastChild());
    overflowList.AppendIfNonempty(aLists, kOverflowList);
  }
  const nsFrameList* list = GetOverflowOutOfFlows();
  if (list) {
    list->AppendIfNonempty(aLists, kOverflowOutOfFlowList);
  }
  mFloats.AppendIfNonempty(aLists, kFloatList);
  if (HaveOutsideBullet()) {
    nsFrameList bullet(mBullet, mBullet);
    bullet.AppendIfNonempty(aLists, kBulletList);
  }
  list = GetPushedFloats();
  if (list) {
    list->AppendIfNonempty(aLists, kPushedFloatsList);
  }
}

 bool
nsBlockFrame::IsFloatContainingBlock() const
{
  return true;
}

static void ReparentFrame(nsIFrame* aFrame, nsIFrame* aOldParent,
                          nsIFrame* aNewParent) {
  NS_ASSERTION(aOldParent == aFrame->GetParent(),
               "Parent not consistent with expectations");

  aFrame->SetParent(aNewParent);

  
  
  nsContainerFrame::ReparentFrameView(aFrame->PresContext(), aFrame,
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
nsBlockFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
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
          if (indent.ConvertsToLength())
            data.currentLine += nsRuleNode::ComputeCoordPercentCalc(indent, 0);
        }
        

        data.line = &line;
        data.lineContainer = curFrame;
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
nsBlockFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
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
          if (indent.ConvertsToLength())
            data.currentLine += nsRuleNode::ComputeCoordPercentCalc(indent, 0);
        }
        

        data.line = &line;
        data.lineContainer = curFrame;
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
  
  if (GetStyleContext()->HasTextDecorationLines()) {
    return GetVisualOverflowRect();
  }
  return ComputeSimpleTightBounds(aContext);
}

static bool
AvailableSpaceShrunk(const nsRect& aOldAvailableSpace,
                     const nsRect& aNewAvailableSpace)
{
  if (aNewAvailableSpace.width == 0) {
    
    return aOldAvailableSpace.width != 0;
  }
  NS_ASSERTION(aOldAvailableSpace.x <= aNewAvailableSpace.x &&
               aOldAvailableSpace.XMost() >= aNewAvailableSpace.XMost(),
               "available space should never grow");
  return aOldAvailableSpace.width != aNewAvailableSpace.width;
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
      
      
      nsIScrollableFrame* scrollFrame = do_QueryFrame(aLastRS->frame);
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
        cbSize.width = NS_MAX(0,
          aLastRS->ComputedWidth() + aLastRS->mComputedPadding.LeftRight() - scrollbars.LeftRight());
      }
      if (aLastRS->ComputedHeight() != NS_UNCONSTRAINEDSIZE) {
        cbSize.height = NS_MAX(0,
          aLastRS->ComputedHeight() + aLastRS->mComputedPadding.TopBottom() - scrollbars.TopBottom());
      }
    }
  }

  return cbSize;
}

static inline bool IsClippingChildren(nsIFrame* aFrame,
                                        const nsHTMLReflowState& aReflowState)
{
  return aReflowState.mStyleDisplay->mOverflowX == NS_STYLE_OVERFLOW_CLIP ||
    nsFrame::ApplyPaginatedOverflowClipping(aFrame);
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

  const nsHTMLReflowState *reflowState = &aReflowState;
  nsAutoPtr<nsHTMLReflowState> mutableReflowState;
  
  
  if (aReflowState.availableHeight != NS_UNCONSTRAINEDSIZE &&
      aReflowState.ComputedHeight() != NS_AUTOHEIGHT &&
      IsClippingChildren(this, aReflowState)) {
    nsMargin heightExtras = aReflowState.mComputedBorderPadding;
    if (GetSkipSides() & NS_SIDE_TOP) {
      heightExtras.top = 0;
    } else {
      
      
      heightExtras.top += aReflowState.mComputedMargin.top;
    }

    if (GetEffectiveComputedHeight(aReflowState) + heightExtras.TopBottom() <=
        aReflowState.availableHeight) {
      mutableReflowState = new nsHTMLReflowState(aReflowState);
      mutableReflowState->availableHeight = NS_UNCONSTRAINEDSIZE;
      reflowState = mutableReflowState;
    }
  }

  
  
  nsSize oldSize = GetSize();

  
  nsAutoFloatManager autoFloatManager(const_cast<nsHTMLReflowState&>(*reflowState));

  
  
  
  
  bool needFloatManager = nsBlockFrame::BlockNeedsFloatManager(this);
  if (needFloatManager)
    autoFloatManager.CreateFloatManager(aPresContext);

  
  
  
  
  ClearLineCursor();

  if (IsFrameTreeTooDeep(*reflowState, aMetrics, aStatus)) {
    return NS_OK;
  }

  bool marginRoot = BlockIsMarginRoot(this);
  nsBlockReflowState state(*reflowState, aPresContext, this, aMetrics,
                           marginRoot, marginRoot, needFloatManager);

#ifdef IBMBIDI
  if (GetStateBits() & NS_BLOCK_NEEDS_BIDI_RESOLUTION)
    static_cast<nsBlockFrame*>(GetFirstContinuation())->ResolveBidi();
#endif 

  if (RenumberLists(aPresContext)) {
    AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
  }

  nsresult rv = NS_OK;

  
  
  
  DrainOverflowLines(state);

  
  nsOverflowAreas ocBounds;
  nsReflowStatus ocStatus = NS_FRAME_COMPLETE;
  if (GetPrevInFlow()) {
    ReflowOverflowContainerChildren(aPresContext, *reflowState, ocBounds, 0,
                                    ocStatus);
  }

  
  
  nsOverflowContinuationTracker tracker(aPresContext, this, false);
  state.mOverflowTracker = &tracker;

  
  DrainPushedFloats(state);
  nsOverflowAreas fcBounds;
  nsReflowStatus fcStatus = NS_FRAME_COMPLETE;
  rv = ReflowPushedFloats(state, fcBounds, fcStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  if (reflowState->mFlags.mHResize)
    PrepareResizeReflow(state);

  mState &= ~NS_FRAME_FIRST_REFLOW;

  
  rv = ReflowDirtyLines(state);
  NS_ASSERTION(NS_SUCCEEDED(rv), "reflow dirty lines failed");
  if (NS_FAILED(rv)) return rv;

  NS_MergeReflowStatusInto(&state.mReflowStatus, ocStatus);
  NS_MergeReflowStatusInto(&state.mReflowStatus, fcStatus);

  
  
  if (NS_UNCONSTRAINEDSIZE != reflowState->availableHeight &&
      NS_FRAME_IS_COMPLETE(state.mReflowStatus) &&
      state.mFloatManager->ClearContinues(FindTrailingClear())) {
    NS_FRAME_SET_INCOMPLETE(state.mReflowStatus);
  }

  if (!NS_FRAME_IS_FULLY_COMPLETE(state.mReflowStatus)) {
    if (GetOverflowLines() || GetPushedFloats()) {
      state.mReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
    }

#ifdef DEBUG_kipp
    ListTag(stdout); printf(": block is not fully complete\n");
#endif
  }

  CheckFloats(state);

  
  
  
  
  
  
  
  
  
  
  
  
  if (mBullet && HaveOutsideBullet() && !mLines.empty() &&
      (mLines.front()->IsBlock() ||
       (0 == mLines.front()->mBounds.height &&
        mLines.front() != mLines.back() &&
        mLines.begin().next()->IsBlock()))) {
    
    nsHTMLReflowMetrics metrics;
    
    nsLayoutUtils::LinePosition position;
    bool havePosition = nsLayoutUtils::GetFirstLinePosition(this, &position);
    nscoord lineTop = havePosition ? position.mTop
                                   : reflowState->mComputedBorderPadding.top;
    ReflowBullet(state, metrics, lineTop);
    NS_ASSERTION(!BulletIsEmpty() || metrics.height == 0,
                 "empty bullet took up space");

    if (havePosition && !BulletIsEmpty()) {
      

      
      
    
      
      nsRect bbox = mBullet->GetRect();
      bbox.y = position.mBaseline - metrics.ascent;
      mBullet->SetRect(bbox);
    }
    
  }

  
  nscoord bottomEdgeOfChildren;
  ComputeFinalSize(*reflowState, state, aMetrics, &bottomEdgeOfChildren);
  ComputeOverflowAreas(*reflowState, aMetrics, bottomEdgeOfChildren);
  
  aMetrics.mOverflowAreas.UnionWith(ocBounds);
  
  aMetrics.mOverflowAreas.UnionWith(fcBounds);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (HasAbsolutelyPositionedChildren()) {
    nsAbsoluteContainingBlock* absoluteContainer = GetAbsoluteContainingBlock();
    bool haveInterrupt = aPresContext->HasPendingInterrupt();
    if (reflowState->WillReflowAgainForClearance() ||
        haveInterrupt) {
      
      
      
      
      
      
      if (haveInterrupt && (GetStateBits() & NS_FRAME_IS_DIRTY)) {
        absoluteContainer->MarkAllFramesDirty();
      } else {
        absoluteContainer->MarkSizeDependentFramesDirty();
      }
    } else {
      nsSize containingBlockSize =
        CalculateContainingBlockSizeForAbsolutes(*reflowState,
                                                 nsSize(aMetrics.width,
                                                        aMetrics.height));

      
      
      
      
      
      
      bool cbWidthChanged = aMetrics.width != oldSize.width;
      bool isRoot = !GetContent()->GetParent();
      
      
      
      
      bool cbHeightChanged =
        !(isRoot && NS_UNCONSTRAINEDSIZE == reflowState->ComputedHeight()) &&
        aMetrics.height != oldSize.height;

      absoluteContainer->Reflow(this, aPresContext, *reflowState,
                                state.mReflowStatus,
                                containingBlockSize.width,
                                containingBlockSize.height, true,
                                cbWidthChanged, cbHeightChanged,
                                &aMetrics.mOverflowAreas);

      
    }
  }

  
  CheckInvalidateSizeChange(aMetrics);

  FinishAndStoreOverflow(&aMetrics);

  
  
  
  if (needFloatManager)
    state.mFloatManager = nsnull;

  aStatus = state.mReflowStatus;

#ifdef DEBUG
  
  
  
  
  
  nsLayoutUtils::AssertNoDuplicateContinuations(this, mFloats);

  if (gNoisyReflow) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": status=%x (%scomplete) metrics=%d,%d carriedMargin=%d",
           aStatus, NS_FRAME_IS_COMPLETE(aStatus) ? "" : "not ",
           aMetrics.width, aMetrics.height,
           aMetrics.mCarriedOutBottomMargin.get());
    if (HasOverflowAreas()) {
      printf(" overflow-vis={%d,%d,%d,%d}",
             aMetrics.VisualOverflow().x,
             aMetrics.VisualOverflow().y,
             aMetrics.VisualOverflow().width,
             aMetrics.VisualOverflow().height);
      printf(" overflow-scr={%d,%d,%d,%d}",
             aMetrics.ScrollableOverflow().x,
             aMetrics.ScrollableOverflow().y,
             aMetrics.ScrollableOverflow().width,
             aMetrics.ScrollableOverflow().height);
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

  NS_FRAME_SET_TRUNCATION(aStatus, (*reflowState), aMetrics);
  return rv;
}

bool
nsBlockFrame::CheckForCollapsedBottomMarginFromClearanceLine()
{
  line_iterator begin = begin_lines();
  line_iterator line = end_lines();

  while (true) {
    if (begin == line) {
      return false;
    }
    --line;
    if (line->mBounds.height != 0 || !line->CachedIsEmpty()) {
      return false;
    }
    if (line->HasClearance()) {
      return true;
    }
  }
  
}

void
nsBlockFrame::ComputeFinalSize(const nsHTMLReflowState& aReflowState,
                               nsBlockReflowState&      aState,
                               nsHTMLReflowMetrics&     aMetrics,
                               nscoord*                 aBottomEdgeOfChildren)
{
  const nsMargin& borderPadding = aState.BorderPadding();
#ifdef NOISY_FINAL_SIZE
  ListTag(stdout);
  printf(": mY=%d mIsBottomMarginRoot=%s mPrevBottomMargin=%d bp=%d,%d\n",
         aState.mY, aState.GetFlag(BRS_ISBOTTOMMARGINROOT) ? "yes" : "no",
         aState.mPrevBottomMargin,
         borderPadding.top, borderPadding.bottom);
#endif

  
  aMetrics.width =
    NSCoordSaturatingAdd(NSCoordSaturatingAdd(borderPadding.left,
                                              aReflowState.ComputedWidth()), 
                         borderPadding.right);

  
  
  
  
  
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

  nscoord bottomEdgeOfChildren = aState.mY + nonCarriedOutVerticalMargin;
  
  if (aState.GetFlag(BRS_ISBOTTOMMARGINROOT) ||
      NS_UNCONSTRAINEDSIZE != aReflowState.ComputedHeight()) {
    
    
    
    
    
    
    if (bottomEdgeOfChildren < aState.mReflowState.availableHeight)
    {
      
      bottomEdgeOfChildren =
        NS_MIN(bottomEdgeOfChildren + aState.mPrevBottomMargin.get(),
               aState.mReflowState.availableHeight);
    }
  }
  if (aState.GetFlag(BRS_FLOAT_MGR)) {
    
    
    nscoord floatHeight =
      aState.ClearFloats(bottomEdgeOfChildren, NS_STYLE_CLEAR_LEFT_AND_RIGHT,
                         nsnull, nsFloatManager::DONT_CLEAR_PUSHED_FLOATS);
    bottomEdgeOfChildren = NS_MAX(bottomEdgeOfChildren, floatHeight);
  }

  
  if (NS_UNCONSTRAINEDSIZE != aReflowState.ComputedHeight()) {
    
    
    nscoord computedHeightLeftOver = GetEffectiveComputedHeight(aReflowState);
    NS_ASSERTION(!( IS_TRUE_OVERFLOW_CONTAINER(this)
                    && computedHeightLeftOver ),
                 "overflow container must not have computedHeightLeftOver");

    aMetrics.height =
      NSCoordSaturatingAdd(NSCoordSaturatingAdd(borderPadding.top,
                                                computedHeightLeftOver),
                           borderPadding.bottom);

    if (NS_FRAME_IS_NOT_COMPLETE(aState.mReflowStatus)
        && aMetrics.height < aReflowState.availableHeight) {
      
      
      NS_FRAME_SET_OVERFLOW_INCOMPLETE(aState.mReflowStatus);
    }

    if (NS_FRAME_IS_COMPLETE(aState.mReflowStatus)) {
      if (computedHeightLeftOver > 0 &&
          NS_UNCONSTRAINEDSIZE != aReflowState.availableHeight &&
          aMetrics.height > aReflowState.availableHeight) {
        
        
        
        
        
        aMetrics.height = NS_MAX(aReflowState.availableHeight,
                                 aState.mY + nonCarriedOutVerticalMargin);
        NS_FRAME_SET_INCOMPLETE(aState.mReflowStatus);
        if (!GetNextInFlow())
          aState.mReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
      }
    }
    else {
      
      
      
      
      aMetrics.height = NS_MAX(aReflowState.availableHeight,
                               aState.mY + nonCarriedOutVerticalMargin);
      
      aMetrics.height = NS_MIN(aMetrics.height,
                               borderPadding.top + computedHeightLeftOver);
      
      
      
      
    }

    
    aMetrics.mCarriedOutBottomMargin.Zero();
  }
  else if (NS_FRAME_IS_COMPLETE(aState.mReflowStatus)) {
    nscoord autoHeight = bottomEdgeOfChildren;
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
    aMetrics.height = NS_MAX(aState.mY, aReflowState.availableHeight);
    if (aReflowState.availableHeight == NS_UNCONSTRAINEDSIZE)
      
      aMetrics.height = aState.mY;
  }

  if (IS_TRUE_OVERFLOW_CONTAINER(this) &&
      NS_FRAME_IS_NOT_COMPLETE(aState.mReflowStatus)) {
    
    
    NS_ASSERTION(aMetrics.height == 0, "overflow containers must be zero-height");
    NS_FRAME_SET_OVERFLOW_INCOMPLETE(aState.mReflowStatus);
  }

  
  aMetrics.height = NS_MAX(0, aMetrics.height);
  *aBottomEdgeOfChildren = bottomEdgeOfChildren;

#ifdef DEBUG_blocks
  if (CRAZY_WIDTH(aMetrics.width) || CRAZY_HEIGHT(aMetrics.height)) {
    ListTag(stdout);
    printf(": WARNING: desired:%d,%d\n", aMetrics.width, aMetrics.height);
  }
#endif
}

void
nsBlockFrame::ComputeOverflowAreas(const nsHTMLReflowState& aReflowState,
                                   nsHTMLReflowMetrics&     aMetrics,
                                   nscoord                  aBottomEdgeOfChildren)
{
  
  
  
  nsRect bounds(0, 0, aMetrics.width, aMetrics.height);
  nsOverflowAreas areas(bounds, bounds);

  if (!IsClippingChildren(this, aReflowState)) {
    for (line_iterator line = begin_lines(), line_end = end_lines();
         line != line_end;
         ++line) {
      areas.UnionWith(line->GetOverflowAreas());
    }

    
    
    
    
    
    if (mBullet) {
      areas.UnionAllWith(mBullet->GetRect());
    }

    
    
    
    
    
    
    nscoord bottomEdgeOfContents = aBottomEdgeOfChildren;
    if (GetStyleContext()->GetPseudo() == nsCSSAnonBoxes::scrolledContent) {
      
      
      bottomEdgeOfContents += aReflowState.mComputedPadding.bottom;
    }
    
    
    
    NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
      nsRect& o = areas.Overflow(otype);
      o.height = NS_MAX(o.YMost(), bottomEdgeOfContents) - o.y;
    }
  }
#ifdef NOISY_COMBINED_AREA
  ListTag(stdout);
  printf(": ca=%d,%d,%d,%d\n", area.x, area.y, area.width, area.height);
#endif

  aMetrics.mOverflowAreas = areas;
}

nsresult
nsBlockFrame::MarkLineDirty(line_iterator aLine, const nsLineList* aLineList)
{
  
  aLine->MarkDirty();
  aLine->SetInvalidateTextRuns(true);
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
    aLine.prev()->SetInvalidateTextRuns(true);
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
  const nsStyleText* styleText = GetStyleText();
  
  bool tryAndSkipLines =
      
      (NS_STYLE_TEXT_ALIGN_LEFT == styleText->mTextAlign ||
       (NS_STYLE_TEXT_ALIGN_DEFAULT == styleText->mTextAlign &&
        NS_STYLE_DIRECTION_LTR ==
          aState.mReflowState.mStyleVisibility->mDirection) ||
       (NS_STYLE_TEXT_ALIGN_END == styleText->mTextAlign &&
        NS_STYLE_DIRECTION_RTL ==
          aState.mReflowState.mStyleVisibility->mDirection)) &&
      
      
      !GetStylePadding()->mPadding.GetLeft().HasPercent();

#ifdef DEBUG
  if (gDisableResizeOpt) {
    tryAndSkipLines = false;
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
          ((line != mLines.back() || GetNextInFlow()) 
           && !line->HasBreakAfter()) ||
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
  nsFloatManager *floatManager = aState.mReflowState.mFloatManager;
  NS_ASSERTION((aState.mReflowState.parentReflowState &&
                aState.mReflowState.parentReflowState->mFloatManager == floatManager) ||
                aState.mReflowState.mBlockDelta == 0, "Bad block delta passed in");

  
  
  if (!floatManager->HasAnyFloats())
    return;

  
  if (floatManager->HasFloatDamage()) {
    
    
    nscoord lineYA = aLine->mBounds.y + aDeltaY;
    nscoord lineYB = lineYA + aLine->mBounds.height;
    
    
    nsRect overflow = aLine->GetOverflowArea(eScrollableOverflow);
    nscoord lineYCombinedA = overflow.y + aDeltaY;
    nscoord lineYCombinedB = lineYCombinedA + overflow.height;
    if (floatManager->IntersectsDamage(lineYA, lineYB) ||
        floatManager->IntersectsDamage(lineYCombinedA, lineYCombinedB)) {
      aLine->MarkDirty();
      return;
    }
  }

  
  if (aDeltaY + aState.mReflowState.mBlockDelta != 0) {
    if (aLine->IsBlock()) {
      
      
      
      
      aLine->MarkDirty();
    } else {
      bool wasImpactedByFloat = aLine->IsImpactedByFloat();
      nsFlowAreaRect floatAvailableSpace =
        aState.GetFloatAvailableSpaceForHeight(aLine->mBounds.y + aDeltaY,
                                               aLine->mBounds.height,
                                               nsnull);

#ifdef REALLY_NOISY_REFLOW
    printf("nsBlockFrame::PropagateFloatDamage %p was = %d, is=%d\n", 
           this, wasImpactedByFloat, floatAvailableSpace.mHasFloats);
#endif

      
      
      
      if (wasImpactedByFloat || floatAvailableSpace.mHasFloats) {
        aLine->MarkDirty();
      }
    }
  }
}

static void PlaceFrameView(nsIFrame* aFrame);

static bool LineHasClear(nsLineBox* aLine) {
  return aLine->IsBlock()
    ? (aLine->GetBreakTypeBefore() ||
       (aLine->mFirstChild->GetStateBits() & NS_BLOCK_HAS_CLEAR_CHILDREN) ||
       !nsBlockFrame::BlockCanIntersectFloats(aLine->mFirstChild))
    : aLine->HasFloatBreakAfter();
}







void
nsBlockFrame::ReparentFloats(nsIFrame* aFirstFrame,
                             nsBlockFrame* aOldParent, bool aFromOverflow,
                             bool aReparentSiblings) {
  nsFrameList list;
  aOldParent->CollectFloats(aFirstFrame, list, aFromOverflow, aReparentSiblings);
  if (list.NotEmpty()) {
    for (nsIFrame* f = list.FirstChild(); f; f = f->GetNextSibling()) {
      ReparentFrame(f, aOldParent, this);
    }
    mFloats.AppendFrames(nsnull, list);
  }
}

static void DumpLine(const nsBlockReflowState& aState, nsLineBox* aLine,
                     nscoord aDeltaY, PRInt32 aDeltaIndent) {
#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsRect ovis(aLine->GetVisualOverflowArea());
    nsRect oscr(aLine->GetScrollableOverflowArea());
    nsBlockFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent + aDeltaIndent);
    printf("line=%p mY=%d dirty=%s oldBounds={%d,%d,%d,%d} oldoverflow-vis={%d,%d,%d,%d} oldoverflow-scr={%d,%d,%d,%d} deltaY=%d mPrevBottomMargin=%d childCount=%d\n",
           static_cast<void*>(aLine), aState.mY,
           aLine->IsDirty() ? "yes" : "no",
           aLine->mBounds.x, aLine->mBounds.y,
           aLine->mBounds.width, aLine->mBounds.height,
           ovis.x, ovis.y, ovis.width, ovis.height,
           oscr.x, oscr.y, oscr.width, oscr.height,
           aDeltaY, aState.mPrevBottomMargin.get(), aLine->GetChildCount());
  }
#endif
}




nsresult
nsBlockFrame::ReflowDirtyLines(nsBlockReflowState& aState)
{
  nsresult rv = NS_OK;
  bool keepGoing = true;
  bool repositionViews = false; 
  bool foundAnyClears = aState.mFloatBreakType != NS_STYLE_CLEAR_NONE;
  bool willReflowAgain = false;

#ifdef DEBUG
  if (gNoisyReflow) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": reflowing dirty lines");
    printf(" computedWidth=%d\n", aState.mReflowState.ComputedWidth());
  }
  AutoNoisyIndenter indent(gNoisyReflow);
#endif

  bool selfDirty = (GetStateBits() & NS_FRAME_IS_DIRTY) ||
                     (aState.mReflowState.mFlags.mVResize &&
                      (GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT));

  
  
  if (aState.mReflowState.availableHeight != NS_UNCONSTRAINEDSIZE
      && GetNextInFlow() && aState.mReflowState.availableHeight > mRect.height) {
    line_iterator lastLine = end_lines();
    if (lastLine != begin_lines()) {
      --lastLine;
      lastLine->MarkDirty();
    }
  }
    
    
  nscoord deltaY = 0;

    
    
    
  bool needToRecoverState = false;
    
  bool reflowedFloat = mFloats.NotEmpty() &&
    (mFloats.FirstChild()->GetStateBits() & NS_FRAME_IS_PUSHED_FLOAT);
  bool lastLineMovedUp = false;
  
  PRUint8 inlineFloatBreakType = aState.mFloatBreakType;

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

    bool previousMarginWasDirty = line->IsPreviousMarginDirty();
    if (previousMarginWasDirty) {
      
      line->MarkDirty();
      line->ClearPreviousMarginDirty();
    } else if (line->mBounds.YMost() + deltaY > aState.mBottomEdge) {
      
      
      line->MarkDirty();
    }

    
    
    
    
    
    
    
    if (!line->IsDirty() &&
        aState.mReflowState.availableHeight != NS_UNCONSTRAINEDSIZE &&
        (deltaY != 0 || aState.mReflowState.mFlags.mVResize) &&
        (line->IsBlock() || line->HasFloats() || line->HadFloatPushed())) {
      line->MarkDirty();
    }

    if (!line->IsDirty()) {
      
      
      PropagateFloatDamage(aState, line, deltaY);
    }

    if (needToRecoverState && line->IsDirty()) {
      
      
      
      aState.ReconstructMarginAbove(line);
    }

    bool reflowedPrevLine = !needToRecoverState;
    if (needToRecoverState) {
      needToRecoverState = false;

      
      
      
      if (line->IsDirty())
        aState.mPrevChild = line.prev()->LastChild();
    }

    
    
    
    
    
    
    
    
    
    if (line->IsDirty() && (line->HasFloats() || !willReflowAgain)) {
      lastLineMovedUp = true;

      bool maybeReflowingForFirstTime =
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
        willReflowAgain = true;
        
        
        
      }

      if (line->HasFloats()) {
        reflowedFloat = true;
      }

      if (!keepGoing) {
        DumpLine(aState, line, deltaY, -1);
        if (0 == line->GetChildCount()) {
          DeleteLine(aState, line, line_end);
        }
        break;
      }

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      if (line.next() != end_lines()) {
        bool maybeWasEmpty = oldY == line.next()->mBounds.y;
        bool isEmpty = line->CachedIsEmpty();
        if (maybeReflowingForFirstTime  ||
            (isEmpty || maybeWasEmpty) ) {
          line.next()->MarkPreviousMarginDirty();
          
        }
      }

      
      
      
      
      
      
      deltaY = line->mBounds.YMost() - oldYMost;

      
      
      
      aState.mPresContext->CheckForInterrupt(this);
    } else {
      aState.mOverflowTracker->Skip(line->mFirstChild, aState.mReflowStatus);
        
        
        

      lastLineMovedUp = deltaY < 0;

      if (deltaY != 0)
        SlideLine(aState, line, deltaY);
      else
        repositionViews = true;

      NS_ASSERTION(!line->IsDirty() || !line->HasFloats(),
                   "Possibly stale float cache here!");
      if (willReflowAgain && line->IsBlock()) {
        
        
        
        
        
        
        
        
        
        
      } else {
        
        aState.RecoverStateFrom(line, deltaY);
      }

      
      
      
      
      
      if (line->IsBlock() || !line->CachedIsEmpty()) {
        aState.mY = line->mBounds.YMost();
      }

      needToRecoverState = true;

      if (reflowedPrevLine && !line->IsBlock() &&
          aState.mPresContext->HasPendingInterrupt()) {
        
        for (nsIFrame* inlineKid = line->mFirstChild; inlineKid;
             inlineKid = inlineKid->GetFirstPrincipalChild()) {
          inlineKid->PullOverflowsFromPrevInFlow();
        }
      }
    }

    
    
    
    
    if (line->HasFloatBreakAfter()) {
      inlineFloatBreakType = line->GetBreakTypeAfter();
    }

    if (LineHasClear(line.get())) {
      foundAnyClears = true;
    }

    DumpLine(aState, line, deltaY, -1);

    if (aState.mPresContext->HasPendingInterrupt()) {
      willReflowAgain = true;
      
      
      
      
      
      
      
      MarkLineDirtyForInterrupt(line);
    }
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

  
  
  
  
  
  
  
  
  
  
  
  bool heightConstrained =
    aState.mReflowState.availableHeight != NS_UNCONSTRAINEDSIZE;
  bool skipPull = willReflowAgain && heightConstrained;
  if (!skipPull && heightConstrained && aState.mNextInFlow &&
      (aState.mReflowState.mFlags.mNextInFlowUntouched &&
       !lastLineMovedUp && 
       !(GetStateBits() & NS_FRAME_IS_DIRTY) &&
       !reflowedFloat)) {
    
    
    
    
    
    line_iterator lineIter = this->end_lines();
    if (lineIter != this->begin_lines()) {
      lineIter--; 
      nsBlockInFlowLineIterator bifLineIter(this, lineIter, false);

      
      
      if (!bifLineIter.Next() ||                
          !bifLineIter.GetLine()->IsDirty()) {
        skipPull=true;
      }
    }
  }

  if (skipPull && aState.mNextInFlow) {
    NS_ASSERTION(heightConstrained, "Height should be constrained here\n");
    if (IS_TRUE_OVERFLOW_CONTAINER(aState.mNextInFlow))
      NS_FRAME_SET_OVERFLOW_INCOMPLETE(aState.mReflowStatus);
    else
      NS_FRAME_SET_INCOMPLETE(aState.mReflowStatus);
  }
  
  if (!skipPull && aState.mNextInFlow) {
    
    
    while (keepGoing && (nsnull != aState.mNextInFlow)) {
      
      nsBlockFrame* nextInFlow = aState.mNextInFlow;
      line_iterator nifLine = nextInFlow->begin_lines();
      nsLineBox *toMove;
      bool toMoveIsOverflowLine;
      if (nifLine != nextInFlow->end_lines()) {
        toMove = nifLine;
        nextInFlow->mLines.erase(nifLine);
        toMoveIsOverflowLine = false;
      } else {
        
        nsLineList* overflowLines = nextInFlow->GetOverflowLines();
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
        } else {
          delete overflowLines;
        }
        toMoveIsOverflowLine = true;
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

      NS_ASSERTION(lastFrame == toMove->LastChild(), "Unexpected lastFrame");

      NS_ASSERTION(aState.mPrevChild || mLines.empty(), "should have a prevchild here");

      NS_ASSERTION(aState.mPrevChild == mFrames.LastChild(),
                   "Incorrect aState.mPrevChild before inserting line at end");

      
      if (toMoveIsOverflowLine) {
        
        
        
        lastFrame->SetNextSibling(nsnull);
      } else {
        
        nsFrameList::FrameLinkEnumerator linkToBreak(nextInFlow->mFrames, lastFrame);
        nextInFlow->mFrames.ExtractHead(linkToBreak);
      }
      nsFrameList newFrames(toMove->mFirstChild, lastFrame);
      mFrames.AppendFrames(nsnull, newFrames);

      
      line = mLines.before_insert(end_lines(), toMove);
      aState.mPrevChild = lastFrame;

      NS_ASSERTION(aState.mPrevChild == mFrames.LastChild(),
                   "Incorrect aState.mPrevChild after inserting line at end");

      
      ReparentFloats(toMove->mFirstChild, nextInFlow, toMoveIsOverflowLine, true);

      DumpLine(aState, toMove, deltaY, 0);
#ifdef DEBUG
      AutoNoisyIndenter indent2(gNoisyReflow);
#endif

      if (aState.mPresContext->HasPendingInterrupt()) {
        MarkLineDirtyForInterrupt(line);
      } else {
        
        
        
        
        while (line != end_lines()) {
          rv = ReflowLine(aState, line, &keepGoing);
          NS_ENSURE_SUCCESS(rv, rv);

          if (aState.mReflowState.WillReflowAgainForClearance()) {
            line->MarkDirty();
            keepGoing = false;
            NS_FRAME_SET_INCOMPLETE(aState.mReflowStatus);
            break;
          }

          DumpLine(aState, line, deltaY, -1);
          if (!keepGoing) {
            if (0 == line->GetChildCount()) {
              DeleteLine(aState, line, line_end);
            }
            break;
          }

          if (LineHasClear(line.get())) {
            foundAnyClears = true;
          }

          if (aState.mPresContext->CheckForInterrupt(this)) {
            MarkLineDirtyForInterrupt(line);
            break;
          }

          
          ++line;
          aState.AdvanceToNextLine();
        }
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
    NS_ASSERTION(!BulletIsEmpty() || metrics.height == 0,
                 "empty bullet took up space");

    if (!BulletIsEmpty()) {
      
      

      if (metrics.ascent == nsHTMLReflowMetrics::ASK_FOR_BASELINE &&
          !nsLayoutUtils::GetFirstLineBaseline(mBullet, &metrics.ascent)) {
        metrics.ascent = metrics.height;
      }

      nsRefPtr<nsFontMetrics> fm;
      nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
      aState.mReflowState.rendContext->SetFont(fm); 

      nscoord minAscent =
        nsLayoutUtils::GetCenteredFontBaseline(fm, aState.mMinLineHeight);
      nscoord minDescent = aState.mMinLineHeight - minAscent;

      aState.mY += NS_MAX(minAscent, metrics.ascent) +
                   NS_MAX(minDescent, metrics.height - metrics.ascent);

      nscoord offset = minAscent - metrics.ascent;
      if (offset > 0) {
        mBullet->SetRect(mBullet->GetRect() + nsPoint(0, offset));
      }
    }
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

void
nsBlockFrame::MarkLineDirtyForInterrupt(nsLineBox* aLine)
{
  aLine->MarkDirty();

  
  
  
  
  if (GetStateBits() & NS_FRAME_IS_DIRTY) {
    
    
    PRInt32 n = aLine->GetChildCount();
    for (nsIFrame* f = aLine->mFirstChild; n > 0;
         f = f->GetNextSibling(), --n) {
      f->AddStateBits(NS_FRAME_IS_DIRTY);
    }
    
    if (aLine->HasFloats()) {
      for (nsFloatCache* fc = aLine->GetFirstFloat(); fc; fc = fc->Next()) {
        fc->mFloat->AddStateBits(NS_FRAME_IS_DIRTY);
      }
    }
  } else {
    
    
    
    
    
    
    nsBlockFrame* bf = nsLayoutUtils::GetAsBlock(aLine->mFirstChild);
    if (bf) {
      MarkAllDescendantLinesDirty(bf);
    }
  }
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

static void
InvalidateThebesLayersInLineBox(nsIFrame* aBlock, nsLineBox* aLine)
{
  if (aBlock->GetStateBits() & NS_FRAME_HAS_CONTAINER_LAYER_DESCENDANT) {
    PRInt32 childCount = aLine->GetChildCount();
    for (nsIFrame* f = aLine->mFirstChild; childCount;
         --childCount, f = f->GetNextSibling()) {
      FrameLayerBuilder::InvalidateThebesLayersInSubtree(f);
    }
  }
}






nsresult
nsBlockFrame::ReflowLine(nsBlockReflowState& aState,
                         line_iterator aLine,
                         bool* aKeepReflowGoing)
{
  nsresult rv = NS_OK;

  NS_ABORT_IF_FALSE(aLine->GetChildCount(), "reflowing empty line");

  
  aState.mCurrentLine = aLine;
  aLine->ClearDirty();
  aLine->InvalidateCachedIsEmpty();
  aLine->ClearHadFloatPushed();

  
  if (aLine->IsBlock()) {
    nsRect oldBounds = aLine->mFirstChild->GetRect();
    nsRect oldVisOverflow(aLine->GetVisualOverflowArea());
    rv = ReflowBlockFrame(aState, aLine, aKeepReflowGoing);
    nsRect newBounds = aLine->mFirstChild->GetRect();

    
    
    
    
    
    
    
    
    
    nsRect visOverflow(aLine->GetVisualOverflowArea());
    if (oldVisOverflow.TopLeft() != visOverflow.TopLeft() ||
        oldBounds.TopLeft() != newBounds.TopLeft()) {
      
      
      nsRect  dirtyRect;
      dirtyRect.UnionRect(oldVisOverflow, visOverflow);
#ifdef NOISY_BLOCK_INVALIDATE
      printf("%p invalidate 6 (%d, %d, %d, %d)\n",
             this, dirtyRect.x, dirtyRect.y, dirtyRect.width, dirtyRect.height);
#endif
      Invalidate(dirtyRect);
      FrameLayerBuilder::InvalidateThebesLayersInSubtree(aLine->mFirstChild);
    } else {
      nsRect combinedAreaHStrip, combinedAreaVStrip;
      nsRect boundsHStrip, boundsVStrip;
      nsLayoutUtils::GetRectDifferenceStrips(oldBounds, newBounds,
                                             &boundsHStrip, &boundsVStrip);
      nsLayoutUtils::GetRectDifferenceStrips(oldVisOverflow, visOverflow,
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
    nsRect oldVisOverflow(aLine->GetVisualOverflowArea());
    aLine->SetLineWrapped(false);

    rv = ReflowInlineFrames(aState, aLine, aKeepReflowGoing);

    
    
    nsRect dirtyRect;
    dirtyRect.UnionRect(oldVisOverflow, aLine->GetVisualOverflowArea());
#ifdef NOISY_BLOCK_INVALIDATE
    printf("%p invalidate (%d, %d, %d, %d)\n",
           this, dirtyRect.x, dirtyRect.y, dirtyRect.width, dirtyRect.height);
    if (aLine->IsForceInvalidate())
      printf("  dirty line is %p\n", static_cast<void*>(aLine.get()));
#endif
    Invalidate(dirtyRect);
    InvalidateThebesLayersInLineBox(this, aLine);
  }

  return rv;
}

nsIFrame*
nsBlockFrame::PullFrame(nsBlockReflowState& aState,
                        line_iterator       aLine)
{
  
  if (end_lines() != aLine.next()) {
    return PullFrameFrom(aState, aLine, this, false, aLine.next());
  }

  NS_ASSERTION(!GetOverflowLines(),
    "Our overflow lines should have been removed at the start of reflow");

  
  nsBlockFrame* nextInFlow = aState.mNextInFlow;
  while (nextInFlow) {
    
    if (!nextInFlow->mLines.empty()) {
      return PullFrameFrom(aState, aLine, nextInFlow, false,
                           nextInFlow->mLines.begin());
    }

    nsLineList* overflowLines = nextInFlow->GetOverflowLines();
    if (overflowLines) {
      return PullFrameFrom(aState, aLine, nextInFlow, true,
                           overflowLines->begin());
    }

    nextInFlow = static_cast<nsBlockFrame*>(nextInFlow->GetNextInFlow());
    aState.mNextInFlow = nextInFlow;
  }

  return nsnull;
}

nsIFrame*
nsBlockFrame::PullFrameFrom(nsBlockReflowState&  aState,
                            nsLineBox*           aLine,
                            nsBlockFrame*        aFromContainer,
                            bool                 aFromOverflowLine,
                            nsLineList::iterator aFromLine)
{
  nsLineBox* fromLine = aFromLine;
  NS_ABORT_IF_FALSE(fromLine, "bad line to pull from");
  NS_ABORT_IF_FALSE(fromLine->GetChildCount(), "empty line");
  NS_ABORT_IF_FALSE(aLine->GetChildCount(), "empty line");

  NS_ASSERTION(fromLine->IsBlock() == fromLine->mFirstChild->GetStyleDisplay()->IsBlockOutside(),
               "Disagreement about whether it's a block or not");

  if (fromLine->IsBlock()) {
    
    
    
    return nsnull;
  }
  
  nsIFrame* frame = fromLine->mFirstChild;
  nsIFrame* newFirstChild = frame->GetNextSibling();

  if (aFromContainer != this) {
    NS_ASSERTION(aState.mPrevChild == aLine->LastChild(),
      "mPrevChild should be the LastChild of the line we are adding to");
    
    
    if (NS_LIKELY(!aFromOverflowLine)) {
      NS_ASSERTION(aFromLine == aFromContainer->mLines.begin(),
                   "should only pull from first line");
      
      aFromContainer->mFrames.RemoveFrame(frame);
    } else {
      
      
      
      frame->SetNextSibling(nsnull);
    }

    
    
    NS_ASSERTION(frame->GetParent() == aFromContainer, "unexpected parent frame");

    ReparentFrame(frame, aFromContainer, this);
    mFrames.InsertFrame(nsnull, aState.mPrevChild, frame);

    
    
    ReparentFloats(frame, aFromContainer, aFromOverflowLine, true);
  }
  
  
  aLine->SetChildCount(aLine->GetChildCount() + 1);
    
  PRInt32 fromLineChildCount = fromLine->GetChildCount();
  if (0 != --fromLineChildCount) {
    
    fromLine->SetChildCount(fromLineChildCount);
    fromLine->MarkDirty();
    fromLine->mFirstChild = newFirstChild;
  }
  else {
    
    
    
    
    Invalidate(fromLine->mBounds);
    nsLineList* fromLineList = aFromOverflowLine
      ? aFromContainer->RemoveOverflowLines()
      : &aFromContainer->mLines;
    if (aFromLine.next() != fromLineList->end())
      aFromLine.next()->MarkPreviousMarginDirty();

    Invalidate(fromLine->GetVisualOverflowArea());
    fromLineList->erase(aFromLine);
    
    aState.FreeLineBox(fromLine);

    
    if (aFromOverflowLine) {
      if (!fromLineList->empty()) {
        aFromContainer->SetOverflowLines(fromLineList);
      } else {
        delete fromLineList;
        
        
      }
    }
  }

#ifdef DEBUG
  VerifyLines(true);
#endif

  return frame;
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

  Invalidate(aLine->GetVisualOverflowArea());
  
  aLine->SlideBy(aDY);
  Invalidate(aLine->GetVisualOverflowArea());
  InvalidateThebesLayersInLineBox(this, aLine);

  
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

static inline bool
IsNonAutoNonZeroHeight(const nsStyleCoord& aCoord)
{
  if (aCoord.GetUnit() == eStyleUnit_Auto)
    return false;
  if (aCoord.IsCoordPercentCalcUnit()) {
    
    
    
    
    return nsRuleNode::ComputeCoordPercentCalc(aCoord, nscoord_MAX) > 0 ||
           nsRuleNode::ComputeCoordPercentCalc(aCoord, 0) > 0;
  }
  NS_ABORT_IF_FALSE(false, "unexpected unit for height or min-height");
  return true;
}

 bool
nsBlockFrame::IsSelfEmpty()
{
  
  
  
  if (GetStateBits() & NS_BLOCK_MARGIN_ROOT)
    return false;

  const nsStylePosition* position = GetStylePosition();

  if (IsNonAutoNonZeroHeight(position->mMinHeight) ||
      IsNonAutoNonZeroHeight(position->mHeight))
    return false;

  const nsStyleBorder* border = GetStyleBorder();
  const nsStylePadding* padding = GetStylePadding();
  if (border->GetActualBorderWidth(NS_SIDE_TOP) != 0 ||
      border->GetActualBorderWidth(NS_SIDE_BOTTOM) != 0 ||
      !nsLayoutUtils::IsPaddingZero(padding->mPadding.GetTop()) ||
      !nsLayoutUtils::IsPaddingZero(padding->mPadding.GetBottom())) {
    return false;
  }

  if (HaveOutsideBullet() && !BulletIsEmpty()) {
    return false;
  }

  return true;
}

bool
nsBlockFrame::CachedIsEmpty()
{
  if (!IsSelfEmpty()) {
    return false;
  }

  for (line_iterator line = begin_lines(), line_end = end_lines();
       line != line_end;
       ++line)
  {
    if (!line->CachedIsEmpty())
      return false;
  }

  return true;
}

bool
nsBlockFrame::IsEmpty()
{
  if (!IsSelfEmpty()) {
    return false;
  }

  for (line_iterator line = begin_lines(), line_end = end_lines();
       line != line_end;
       ++line)
  {
    if (!line->IsEmpty())
      return false;
  }

  return true;
}

bool
nsBlockFrame::ShouldApplyTopMargin(nsBlockReflowState& aState,
                                   nsLineBox* aLine)
{
  if (aState.GetFlag(BRS_APPLYTOPMARGIN)) {
    
    return true;
  }

  if (!aState.IsAdjacentWithTop()) {
    
    
    
    aState.SetFlag(BRS_APPLYTOPMARGIN, true);
    return true;
  }

  
  line_iterator line = begin_lines();
  if (aState.GetFlag(BRS_HAVELINEADJACENTTOTOP)) {
    line = aState.mLineAdjacentToTop;
  }
  while (line != aLine) {
    if (!line->CachedIsEmpty() || line->HasClearance()) {
      
      
      aState.SetFlag(BRS_APPLYTOPMARGIN, true);
      return true;
    }
    
    
    ++line;
    aState.SetFlag(BRS_HAVELINEADJACENTTOTOP, true);
    aState.mLineAdjacentToTop = line;
  }

  
  
  
  return false;
}

nsresult
nsBlockFrame::ReflowBlockFrame(nsBlockReflowState& aState,
                               line_iterator aLine,
                               bool* aKeepReflowGoing)
{
  NS_PRECONDITION(*aKeepReflowGoing, "bad caller");

  nsresult rv = NS_OK;

  nsIFrame* frame = aLine->mFirstChild;
  if (!frame) {
    NS_ASSERTION(false, "program error - unexpected empty line"); 
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

  
  
  
  
  bool applyTopMargin =
    !frame->GetPrevInFlow() && ShouldApplyTopMargin(aState, aLine);

  if (applyTopMargin) {
    
    
    
    
    aLine->ClearHasClearance();
  }
  bool treatWithClearance = aLine->HasClearance();

  bool mightClearFloats = breakType != NS_STYLE_CLEAR_NONE;
  nsIFrame *replacedBlock = nsnull;
  if (!nsBlockFrame::BlockCanIntersectFloats(frame)) {
    mightClearFloats = true;
    replacedBlock = frame;
  }

  
  
  
  
  if (!treatWithClearance && !applyTopMargin && mightClearFloats &&
      aState.mReflowState.mDiscoveredClearance) {
    nscoord curY = aState.mY + aState.mPrevBottomMargin.get();
    nscoord clearY = aState.ClearFloats(curY, breakType, replacedBlock);
    if (clearY != curY) {
      
      
      
      
      treatWithClearance = true;
      
      if (!*aState.mReflowState.mDiscoveredClearance) {
        *aState.mReflowState.mDiscoveredClearance = frame;
      }
      aState.mPrevChild = frame;
      
      
      return NS_OK;
    }
  }
  if (treatWithClearance) {
    applyTopMargin = true;
  }

  nsIFrame* clearanceFrame = nsnull;
  nscoord startingY = aState.mY;
  nsCollapsingMargin incomingMargin = aState.mPrevBottomMargin;
  nscoord clearance;
  
  
  nsPoint originalPosition = frame->GetPosition();
  while (true) {
    
    nscoord passOriginalY = frame->GetRect().y;
    
    clearance = 0;
    nscoord topMargin = 0;
    bool mayNeedRetry = false;
    bool clearedFloats = false;
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
        
        
        mayNeedRetry = false;
      }
      
      if (!treatWithClearance && !clearanceFrame && mightClearFloats) {
        
        
        
        
        
        
        nscoord curY = aState.mY + aState.mPrevBottomMargin.get();
        nscoord clearY = aState.ClearFloats(curY, breakType, replacedBlock);
        if (clearY != curY) {
          
          
          treatWithClearance = true;
          
          aLine->SetHasClearance();
          
          
          aState.mY += aState.mPrevBottomMargin.get();
          aState.mPrevBottomMargin.Zero();
          
          
          mayNeedRetry = false;
          nsBlockReflowContext::ComputeCollapsedTopMargin(reflowState,
                                                          &aState.mPrevBottomMargin, clearanceFrame, &mayNeedRetry);
        }
      }
      
      
      
      
      
      topMargin = aState.mPrevBottomMargin.get();
      
      if (treatWithClearance) {
        nscoord currentY = aState.mY;
        
        aState.mY = aState.ClearFloats(aState.mY, breakType, replacedBlock);

        clearedFloats = aState.mY != currentY;

        
        
        
        
        
        
        clearance = aState.mY - (currentY + topMargin);
        
        
        
        topMargin += clearance;
        
        
        
      } else {
        
        aState.mY += topMargin;
      }
    }
    
    
    
    nsFlowAreaRect floatAvailableSpace = aState.GetFloatAvailableSpace();
#ifdef REALLY_NOISY_REFLOW
    printf("setting line %p isImpacted to %s\n",
           aLine.get(), floatAvailableSpace.mHasFloats?"true":"false");
#endif
    aLine->SetLineIsImpactedByFloat(floatAvailableSpace.mHasFloats);
    nsRect availSpace;
    aState.ComputeBlockAvailSpace(frame, display, floatAvailableSpace,
                                  replacedBlock != nsnull, availSpace);

    
    
    
    
    
    if ((!aState.mReflowState.mFlags.mIsTopOfPage || clearedFloats) &&
        availSpace.height < 0) {
      
      
      
      
      
      
      
      
      
      
      aState.mY = startingY;
      aState.mPrevBottomMargin = incomingMargin;
      PushLines(aState, aLine.prev());
      NS_FRAME_SET_INCOMPLETE(aState.mReflowStatus);
      *aKeepReflowGoing = false;
      return NS_OK;
    }

    
    
    aState.mY -= topMargin;
    availSpace.y -= topMargin;
    if (NS_UNCONSTRAINEDSIZE != availSpace.height) {
      availSpace.height += topMargin;
    }
    
    
    
    
    nsHTMLReflowState blockHtmlRS(aState.mPresContext, aState.mReflowState, frame, 
                                  nsSize(availSpace.width, availSpace.height));
    blockHtmlRS.mFlags.mHasClearance = aLine->HasClearance();
    
    nsFloatManager::SavedState floatManagerState;
    if (mayNeedRetry) {
      blockHtmlRS.mDiscoveredClearance = &clearanceFrame;
      aState.mFloatManager->PushState(&floatManagerState);
    } else if (!applyTopMargin) {
      blockHtmlRS.mDiscoveredClearance = aState.mReflowState.mDiscoveredClearance;
    }
    
    nsReflowStatus frameReflowStatus = NS_FRAME_COMPLETE;
    rv = brc.ReflowBlock(availSpace, applyTopMargin, aState.mPrevBottomMargin,
                         clearance, aState.IsAdjacentWithTop(),
                         aLine.get(), blockHtmlRS, frameReflowStatus, aState);

    
    
    
    if (!mayNeedRetry && clearanceFrame &&
        frame->GetRect().y != passOriginalY) {
      Invalidate(frame->GetVisualOverflowRect() + frame->GetPosition());
    }
    
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (mayNeedRetry && clearanceFrame) {
      aState.mFloatManager->PopState(&floatManagerState);
      aState.mY = startingY;
      aState.mPrevBottomMargin = incomingMargin;
      continue;
    }

    aState.mPrevChild = frame;

    if (blockHtmlRS.WillReflowAgainForClearance()) {
      
      
      
      
      
      NS_ASSERTION(originalPosition == frame->GetPosition(),
                   "we need to call PositionChildViews");
      return NS_OK;
    }

#if defined(REFLOW_STATUS_COVERAGE)
    RecordReflowStatus(true, frameReflowStatus);
#endif
    
    if (NS_INLINE_IS_BREAK_BEFORE(frameReflowStatus)) {
      
      PushLines(aState, aLine.prev());
      *aKeepReflowGoing = false;
      NS_FRAME_SET_INCOMPLETE(aState.mReflowStatus);
    }
    else {
      
      
      
      
      
      
      
      
      bool forceFit = aState.IsAdjacentWithTop() && clearance <= 0 &&
        !floatAvailableSpace.mHasFloats;
      nsCollapsingMargin collapsedBottomMargin;
      nsOverflowAreas overflowAreas;
      *aKeepReflowGoing = brc.PlaceBlock(blockHtmlRS, forceFit, aLine.get(),
                                         collapsedBottomMargin,
                                         aLine->mBounds, overflowAreas,
                                         frameReflowStatus);
      if (aLine->SetCarriedOutBottomMargin(collapsedBottomMargin)) {
        line_iterator nextLine = aLine;
        ++nextLine;
        if (nextLine != end_lines()) {
          nextLine->MarkPreviousMarginDirty();
        }
      }

      aLine->SetOverflowAreas(overflowAreas);
      if (*aKeepReflowGoing) {
        
        
        
        nscoord newY = aLine->mBounds.YMost();
        aState.mY = newY;
        
        
        
        if (!NS_FRAME_IS_FULLY_COMPLETE(frameReflowStatus)) {
          bool madeContinuation;
          rv = CreateContinuationFor(aState, nsnull, frame, madeContinuation);
          NS_ENSURE_SUCCESS(rv, rv);
          
          nsIFrame* nextFrame = frame->GetNextInFlow();
          NS_ASSERTION(nextFrame, "We're supposed to have a next-in-flow by now");
          
          if (NS_FRAME_IS_NOT_COMPLETE(frameReflowStatus)) {
            
            if (!madeContinuation &&
                (NS_FRAME_IS_OVERFLOW_CONTAINER & nextFrame->GetStateBits())) {
              aState.mOverflowTracker->Finish(frame);
              nsContainerFrame* parent =
                static_cast<nsContainerFrame*>(nextFrame->GetParent());
              rv = parent->StealFrame(aState.mPresContext, nextFrame);
              NS_ENSURE_SUCCESS(rv, rv);
              if (parent != this)
                ReparentFrame(nextFrame, parent, this);
              mFrames.InsertFrame(nsnull, frame, nextFrame);
              madeContinuation = true; 
              nextFrame->RemoveStateBits(NS_FRAME_IS_OVERFLOW_CONTAINER);
              frameReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
            }

            
            if (madeContinuation) {
              nsLineBox* line = aState.NewLineBox(nextFrame, 1, true);
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
            *aKeepReflowGoing = false;
            
            
            
            
            
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
              mFrames.RemoveFrame(nextFrame);
            }

            
            aState.mOverflowTracker->Insert(nextFrame, frameReflowStatus);
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
        
        if (aLine == mLines.front() && !GetPrevInFlow()) {
          
          
          
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
  VerifyLines(true);
#endif
  return rv;
}

nsresult
nsBlockFrame::ReflowInlineFrames(nsBlockReflowState& aState,
                                 line_iterator aLine,
                                 bool* aKeepReflowGoing)
{
  nsresult rv = NS_OK;
  *aKeepReflowGoing = true;

  aLine->SetLineIsImpactedByFloat(false);

  
  
  if (ShouldApplyTopMargin(aState, aLine)) {
    aState.mY += aState.mPrevBottomMargin.get();
  }
  nsFlowAreaRect floatAvailableSpace = aState.GetFloatAvailableSpace();

  LineReflowStatus lineReflowStatus;
  do {
    nscoord availableSpaceHeight = 0;
    do {
      bool allowPullUp = true;
      nsIContent* forceBreakInContent = nsnull;
      PRInt32 forceBreakOffset = -1;
      gfxBreakPriority forceBreakPriority = eNoBreak;
      do {
        nsFloatManager::SavedState floatManagerState;
        aState.mReflowState.mFloatManager->PushState(&floatManagerState);

        
        
        
        
        
        
        
        nsLineLayout lineLayout(aState.mPresContext,
                                aState.mReflowState.mFloatManager,
                                &aState.mReflowState, &aLine);
        lineLayout.Init(&aState, aState.mMinLineHeight, aState.mLineNumber);
        if (forceBreakInContent) {
          lineLayout.ForceBreakAtPosition(forceBreakInContent, forceBreakOffset);
        }
        rv = DoReflowInlineFrames(aState, lineLayout, aLine,
                                  floatAvailableSpace, availableSpaceHeight,
                                  &floatManagerState, aKeepReflowGoing,
                                  &lineReflowStatus, allowPullUp);
        lineLayout.EndLineReflow();

        if (NS_FAILED(rv)) {
          return rv;
        }

        if (LINE_REFLOW_REDO_NO_PULL == lineReflowStatus ||
            LINE_REFLOW_REDO_MORE_FLOATS == lineReflowStatus ||
            LINE_REFLOW_REDO_NEXT_BAND == lineReflowStatus) {
          if (lineLayout.NeedsBackup()) {
            NS_ASSERTION(!forceBreakInContent, "Backing up twice; this should never be necessary");
            
            
            
            forceBreakInContent = lineLayout.GetLastOptionalBreakPosition(&forceBreakOffset, &forceBreakPriority);
          } else {
            forceBreakInContent = nsnull;
          }
          
          aState.mReflowState.mFloatManager->PopState(&floatManagerState);
          
          aState.mCurrentLineFloats.DeleteAll();
          aState.mBelowCurrentLineFloats.DeleteAll();
        }

        
        allowPullUp = false;
      } while (LINE_REFLOW_REDO_NO_PULL == lineReflowStatus);
    } while (LINE_REFLOW_REDO_MORE_FLOATS == lineReflowStatus);
  } while (LINE_REFLOW_REDO_NEXT_BAND == lineReflowStatus);

  return rv;
}

void
nsBlockFrame::PushTruncatedLine(nsBlockReflowState& aState,
                                line_iterator       aLine,
                                bool&             aKeepReflowGoing)
{
  line_iterator prevLine = aLine;
  --prevLine;
  PushLines(aState, prevLine);
  aKeepReflowGoing = false;
  NS_FRAME_SET_INCOMPLETE(aState.mReflowStatus);
}

#ifdef DEBUG
static const char* LineReflowStatusNames[] = {
  "LINE_REFLOW_OK", "LINE_REFLOW_STOP", "LINE_REFLOW_REDO_NO_PULL",
  "LINE_REFLOW_REDO_MORE_FLOATS",
  "LINE_REFLOW_REDO_NEXT_BAND", "LINE_REFLOW_TRUNCATED"
};
#endif

nsresult
nsBlockFrame::DoReflowInlineFrames(nsBlockReflowState& aState,
                                   nsLineLayout& aLineLayout,
                                   line_iterator aLine,
                                   nsFlowAreaRect& aFloatAvailableSpace,
                                   nscoord& aAvailableSpaceHeight,
                                   nsFloatManager::SavedState*
                                     aFloatStateBeforeLine,
                                   bool* aKeepReflowGoing,
                                   LineReflowStatus* aLineReflowStatus,
                                   bool aAllowPullUp)
{
  
  aLine->FreeFloats(aState.mFloatCacheFreeList);
  aState.mFloatOverflowAreas.Clear();

  
  
  if (aFloatAvailableSpace.mHasFloats)
    aLine->SetLineIsImpactedByFloat(true);
#ifdef REALLY_NOISY_REFLOW
  printf("nsBlockFrame::DoReflowInlineFrames %p impacted = %d\n",
         this, aFloatAvailableSpace.mHasFloats);
#endif

  nscoord x = aFloatAvailableSpace.mRect.x;
  nscoord availWidth = aFloatAvailableSpace.mRect.width;
  nscoord availHeight;
  if (aState.GetFlag(BRS_UNCONSTRAINEDHEIGHT)) {
    availHeight = NS_UNCONSTRAINEDSIZE;
  }
  else {
    
    availHeight = aFloatAvailableSpace.mRect.height;
  }

  
  
  aLine->EnableResizeReflowOptimization();

  aLineLayout.BeginLineReflow(x, aState.mY,
                              availWidth, availHeight,
                              aFloatAvailableSpace.mHasFloats,
                              false );

  aState.SetFlag(BRS_LINE_LAYOUT_EMPTY, false);

  
  
  if ((0 == aLineLayout.GetLineNumber()) &&
      (NS_BLOCK_HAS_FIRST_LETTER_CHILD & mState) &&
      (NS_BLOCK_HAS_FIRST_LETTER_STYLE & mState)) {
    aLineLayout.SetFirstLetterStyleOK(true);
  }
  NS_ASSERTION(!((NS_BLOCK_HAS_FIRST_LETTER_CHILD & mState) &&
                 GetPrevContinuation()),
               "first letter child bit should only be on first continuation");

  
  nsresult rv = NS_OK;
  LineReflowStatus lineReflowStatus = LINE_REFLOW_OK;
  PRInt32 i;
  nsIFrame* frame = aLine->mFirstChild;

  if (aFloatAvailableSpace.mHasFloats) {
    
    
    if (aLineLayout.NotifyOptionalBreakPosition(frame->GetContent(), 0, true, eNormalBreak)) {
      lineReflowStatus = LINE_REFLOW_REDO_NEXT_BAND;
    }
  }

  
  
  for (i = 0; LINE_REFLOW_OK == lineReflowStatus && i < aLine->GetChildCount();
       i++, frame = frame->GetNextSibling()) {
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

      NS_ASSERTION(lineReflowStatus != LINE_REFLOW_TRUNCATED,
                   "ReflowInlineFrame should never determine that a line "
                   "needs to go to the next page/column");
    }
  }

  
  if (aAllowPullUp) {
    
    while (LINE_REFLOW_OK == lineReflowStatus) {
      frame = PullFrame(aState, aLine);
      if (!frame) {
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

  
  bool needsBackup = aLineLayout.NeedsBackup() &&
    (lineReflowStatus == LINE_REFLOW_STOP || lineReflowStatus == LINE_REFLOW_OK);
  if (needsBackup && aLineLayout.HaveForcedBreakPosition()) {
  	NS_WARNING("We shouldn't be backing up more than once! "
               "Someone must have set a break opportunity beyond the available width, "
               "even though there were better break opportunities before it");
    needsBackup = false;
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
    
    
    
    
    
    
    NS_ASSERTION(NS_UNCONSTRAINEDSIZE != aFloatAvailableSpace.mRect.height,
                 "unconstrained height on totally empty line");

    
    if (aFloatAvailableSpace.mRect.height > 0) {
      NS_ASSERTION(aFloatAvailableSpace.mHasFloats,
                   "redo line on totally empty line with non-empty band...");
      
      
      
      
      aState.mFloatManager->AssertStateMatches(aFloatStateBeforeLine);
      aState.mY += aFloatAvailableSpace.mRect.height;
      aFloatAvailableSpace = aState.GetFloatAvailableSpace();
    } else {
      NS_ASSERTION(NS_UNCONSTRAINEDSIZE != aState.mReflowState.availableHeight,
                   "We shouldn't be running out of height here");
      if (NS_UNCONSTRAINEDSIZE == aState.mReflowState.availableHeight) {
        
        aState.mY += 1;
        
        
        
        
        aState.mFloatManager->AssertStateMatches(aFloatStateBeforeLine);
        aFloatAvailableSpace = aState.GetFloatAvailableSpace();
      } else {
        
        
        
        lineReflowStatus = LINE_REFLOW_TRUNCATED;
        
        PushTruncatedLine(aState, aLine, *aKeepReflowGoing);
      }
    }

    
    
    
    
  }
  else if (LINE_REFLOW_TRUNCATED != lineReflowStatus &&
           LINE_REFLOW_REDO_NO_PULL != lineReflowStatus) {
    
    
    if (!NS_INLINE_IS_BREAK_BEFORE(aState.mReflowStatus)) {
      if (!PlaceLine(aState, aLineLayout, aLine, aFloatStateBeforeLine,
                     aFloatAvailableSpace.mRect, aAvailableSpaceHeight,
                     aKeepReflowGoing)) {
        lineReflowStatus = LINE_REFLOW_REDO_MORE_FLOATS;
        
      }
    }
  }
#ifdef DEBUG
  if (gNoisyReflow) {
    printf("Line reflow status = %s\n", LineReflowStatusNames[lineReflowStatus]);
  }
#endif

  if (aLineLayout.GetDirtyNextLine()) {
    
    nsLineList* overflowLines = GetOverflowLines();
    
    
    bool pushedToOverflowLines = overflowLines &&
      overflowLines->front() == aLine.get();
    if (pushedToOverflowLines) {
      
      
      aLine = overflowLines->begin();
    }
    nsBlockInFlowLineIterator iter(this, aLine, pushedToOverflowLines);
    if (iter.Next() && iter.GetLine()->IsInline()) {
      iter.GetLine()->MarkDirty();
      if (iter.GetContainer() != this) {
        aState.mReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
      }
    }
  }

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
  bool           pushedFrame;
  nsresult rv = aLineLayout.ReflowFrame(aFrame, frameReflowStatus,
                                        nsnull, pushedFrame);
  NS_ENSURE_SUCCESS(rv, rv);

  if (frameReflowStatus & NS_FRAME_REFLOW_NEXTINFLOW) {
    aLineLayout.SetDirtyNextLine();
  }

  NS_ENSURE_SUCCESS(rv, rv);
#ifdef REALLY_NOISY_REFLOW_CHILD
  nsFrame::ListTag(stdout, aFrame);
  printf(": status=%x\n", frameReflowStatus);
#endif

#if defined(REFLOW_STATUS_COVERAGE)
  RecordReflowStatus(false, frameReflowStatus);
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
          aLine->SetLineWrapped(true);
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
          aLineLayout.SetDirtyNextLine();
        }
      }
    }
  }

  if (!NS_FRAME_IS_FULLY_COMPLETE(frameReflowStatus)) {
    
    
    nsIAtom* frameType = aFrame->GetType();

    bool madeContinuation;
    rv = CreateContinuationFor(aState, aLine, aFrame, madeContinuation);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (!aLineLayout.GetLineEndsInBR()) {
      aLine->SetLineWrapped(true);
    }
    
    
    
    
    if ((!(frameReflowStatus & NS_INLINE_BREAK_FIRST_LETTER_COMPLETE) && 
         nsGkAtoms::placeholderFrame != frameType) ||
        *aLineReflowStatus == LINE_REFLOW_STOP) {
      
      *aLineReflowStatus = LINE_REFLOW_STOP;
      rv = SplitLine(aState, aLineLayout, aLine, aFrame->GetNextSibling(), aLineReflowStatus);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
nsBlockFrame::CreateContinuationFor(nsBlockReflowState& aState,
                                    nsLineBox*          aLine,
                                    nsIFrame*           aFrame,
                                    bool&             aMadeNewFrame)
{
  aMadeNewFrame = false;

  if (!aFrame->GetNextInFlow()) {
    nsIFrame* newFrame;
    nsresult rv = aState.mPresContext->PresShell()->FrameConstructor()->
      CreateContinuingFrame(aState.mPresContext, aFrame, this, &newFrame);
    if (NS_FAILED(rv)) {
      return rv;
    }

    mFrames.InsertFrame(nsnull, aFrame, newFrame);

    if (aLine) { 
      aLine->SetChildCount(aLine->GetChildCount() + 1);
    }

    aMadeNewFrame = true;
  }
#ifdef DEBUG
  VerifyLines(false);
#endif
  return NS_OK;
}

nsresult
nsBlockFrame::SplitFloat(nsBlockReflowState& aState,
                         nsIFrame*           aFloat,
                         nsReflowStatus      aFloatStatus)
{
  nsIFrame* nextInFlow = aFloat->GetNextInFlow();
  if (nextInFlow) {
    nsContainerFrame *oldParent =
      static_cast<nsContainerFrame*>(nextInFlow->GetParent());
    DebugOnly<nsresult> rv = oldParent->StealFrame(aState.mPresContext, nextInFlow);
    NS_ASSERTION(NS_SUCCEEDED(rv), "StealFrame failed");
    if (oldParent != this) {
      ReparentFrame(nextInFlow, oldParent, this);
    }
  } else {
    nsresult rv = aState.mPresContext->PresShell()->FrameConstructor()->
      CreateContinuingFrame(aState.mPresContext, aFloat, this, &nextInFlow);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (NS_FRAME_OVERFLOW_IS_INCOMPLETE(aFloatStatus))
    aFloat->GetNextInFlow()->AddStateBits(NS_FRAME_IS_OVERFLOW_CONTAINER);

  
  NS_FRAME_SET_OVERFLOW_INCOMPLETE(aState.mReflowStatus);

  if (aFloat->GetStyleDisplay()->mFloats == NS_STYLE_FLOAT_LEFT) {
    aState.mFloatManager->SetSplitLeftFloatAcrossBreak();
  } else {
    NS_ABORT_IF_FALSE(aFloat->GetStyleDisplay()->mFloats ==
                        NS_STYLE_FLOAT_RIGHT, "unexpected float side");
    aState.mFloatManager->SetSplitRightFloatAcrossBreak();
  }

  aState.AppendPushedFloat(nextInFlow);
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

static bool
CheckPlaceholderInLine(nsIFrame* aBlock, nsLineBox* aLine, nsFloatCache* aFC)
{
  if (!aFC)
    return true;
  NS_ASSERTION(!aFC->mFloat->GetPrevContinuation(),
               "float in a line should never be a continuation");
  NS_ASSERTION(!(aFC->mFloat->GetStateBits() & NS_FRAME_IS_PUSHED_FLOAT),
               "float in a line should never be a pushed float");
  nsIFrame* ph = aBlock->PresContext()->FrameManager()->
                   GetPlaceholderFrameFor(aFC->mFloat->GetFirstInFlow());
  for (nsIFrame* f = ph; f; f = f->GetParent()) {
    if (f->GetParent() == aBlock)
      return aLine->Contains(f);
  }
  NS_ASSERTION(false, "aBlock is not an ancestor of aFrame!");
  return true;
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
#ifdef DEBUG
    {
      nsIFrame *f = aFrame;
      PRInt32 count = pushCount;
      while (f && count > 0) {
        f = f->GetNextSibling();
        --count;
      }
      NS_ASSERTION(count == 0, "Not enough frames to push");
    }
#endif

    
    nsLineBox* newLine = aState.NewLineBox(aFrame, pushCount, false);
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
    VerifyLines(true);
#endif
  }
  return NS_OK;
}

bool
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

  
  return false;
}

bool
nsBlockFrame::PlaceLine(nsBlockReflowState& aState,
                        nsLineLayout&       aLineLayout,
                        line_iterator       aLine,
                        nsFloatManager::SavedState *aFloatStateBeforeLine,
                        nsRect&             aFloatAvailableSpace,
                        nscoord&            aAvailableSpaceHeight,
                        bool*             aKeepReflowGoing)
{
  
  aLineLayout.TrimTrailingWhiteSpace();

  
  
  
  
  
  
  
  
  
  bool addedBullet = false;
  if (mBullet && HaveOutsideBullet() &&
      ((aLine == mLines.front() &&
        (!aLineLayout.IsZeroHeight() || (aLine == mLines.back()))) ||
       (mLines.front() != mLines.back() &&
        0 == mLines.front()->mBounds.height &&
        aLine == mLines.begin().next()))) {
    nsHTMLReflowMetrics metrics;
    ReflowBullet(aState, metrics, aState.mY);
    NS_ASSERTION(!BulletIsEmpty() || metrics.height == 0,
                 "empty bullet took up space");
    aLineLayout.AddBulletFrame(mBullet, metrics);
    addedBullet = true;
  }
  aLineLayout.VerticalAlignLine();

  
  
  
  
  
  nsRect oldFloatAvailableSpace(aFloatAvailableSpace);
  
  
  aAvailableSpaceHeight = NS_MAX(aAvailableSpaceHeight, aLine->mBounds.height);
  aFloatAvailableSpace = 
    aState.GetFloatAvailableSpaceForHeight(aLine->mBounds.y,
                                           aAvailableSpaceHeight,
                                           aFloatStateBeforeLine).mRect;
  NS_ASSERTION(aFloatAvailableSpace.y == oldFloatAvailableSpace.y, "yikes");
  
  aFloatAvailableSpace.height = oldFloatAvailableSpace.height;
  
  
  
  if (AvailableSpaceShrunk(oldFloatAvailableSpace, aFloatAvailableSpace)) {
    return false;
  }

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
  bool allowJustify = NS_STYLE_TEXT_ALIGN_JUSTIFY == styleText->mTextAlign &&
                        !aLineLayout.GetLineEndsInBR() &&
                        ShouldJustifyLine(aState, aLine);
  aLineLayout.HorizontalAlignFrames(aLine->mBounds, allowJustify);
  
  
  
  
#ifdef IBMBIDI
  
  if (aState.mPresContext->BidiEnabled()) {
    if (!aState.mPresContext->IsVisualMode() ||
        GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
      nsBidiPresUtils::ReorderFrames(aLine->mFirstChild, aLine->GetChildCount());
    } 
  } 
#endif 

  
  
  nsOverflowAreas overflowAreas;
  aLineLayout.RelativePositionFrames(overflowAreas);
  aLine->SetOverflowAreas(overflowAreas);
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
      *aKeepReflowGoing = false;
    }
    return true;
  }

  aState.mY = newY;
  
  
  aLine->AppendFloats(aState.mCurrentLineFloats);

  
  if (aState.mBelowCurrentLineFloats.NotEmpty()) {
    
    
    aState.PlaceBelowCurrentLineFloats(aState.mBelowCurrentLineFloats, aLine);
    aLine->AppendFloats(aState.mBelowCurrentLineFloats);
  }

  
  
  if (aLine->HasFloats()) {
    
    
    nsOverflowAreas lineOverflowAreas;
    NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
      nsRect &o = lineOverflowAreas.Overflow(otype);
      o = aLine->GetOverflowArea(otype);
#ifdef NOISY_COMBINED_AREA
      ListTag(stdout);
      printf(": overflow %d lineCA=%d,%d,%d,%d floatCA=%d,%d,%d,%d\n",
             otype,
             o.x, o.y, o.width, o.height,
             aState.mFloatOverflowAreas.Overflow(otype).x,
             aState.mFloatOverflowAreas.Overflow(otype).y,
             aState.mFloatOverflowAreas.Overflow(otype).width,
             aState.mFloatOverflowAreas.Overflow(otype).height);
#endif
      o.UnionRect(aState.mFloatOverflowAreas.Overflow(otype), o);

#ifdef NOISY_COMBINED_AREA
      printf("  ==> final lineCA=%d,%d,%d,%d\n",
             o.x, o.y, o.width, o.height);
#endif
    }
    aLine->SetOverflowAreas(lineOverflowAreas);
  }

  
  
  if (aLine->HasFloatBreakAfter()) {
    aState.mY = aState.ClearFloats(aState.mY, aLine->GetBreakTypeAfter());
  }
  return true;
}

void
nsBlockFrame::PushLines(nsBlockReflowState&  aState,
                        nsLineList::iterator aLineBefore)
{
  nsLineList::iterator overBegin(aLineBefore.next());

  
  bool firstLine = overBegin == begin_lines();

  if (overBegin != end_lines()) {
    
    nsFrameList floats;
    CollectFloats(overBegin->mFirstChild, floats, false, true);

    if (floats.NotEmpty()) {
      
      nsAutoOOFFrameList oofs(this);
      oofs.mList.InsertFrames(nsnull, nsnull, floats);
    }

    
    
    
    
    
    nsLineList* overflowLines = RemoveOverflowLines();
    if (!overflowLines) {
      
      overflowLines = new nsLineList();
    }
    if (overflowLines) {
      
      nsIFrame* oldLastChild = mFrames.LastChild();
      if (firstLine) {
        mFrames.Clear();
      } else {
        mFrames.RemoveFramesAfter(aLineBefore->LastChild());
      }
      if (!overflowLines->empty()) {
        
        
        oldLastChild->SetNextSibling(overflowLines->front()->mFirstChild);
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

#ifdef DEBUG
  VerifyOverflowSituation();
#endif
}





bool
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
        mFloats.InsertFrames(nsnull, nsnull, oofs.mList);
      }
    }
    
    
    
  }

  
  
  ourOverflowLines = RemoveOverflowLines();
  if (ourOverflowLines) {
    nsAutoOOFFrameList oofs(this);
    if (oofs.mList.NotEmpty()) {
      
      mFloats.AppendFrames(nsnull, oofs.mList);
    }
  }

  if (!overflowLines && !ourOverflowLines) {
    
    return false;
  }

  
  if (overflowLines) {
    if (!overflowLines->empty()) {
      
      if (!mLines.empty()) {
          
          
          mLines.front()->MarkPreviousMarginDirty();
      }
      
      
      nsIFrame* firstFrame = overflowLines->front()->mFirstChild;
      nsIFrame* lastFrame = overflowLines->back()->LastChild();
      nsFrameList framesToInsert(firstFrame, lastFrame);
      mFrames.InsertFrames(nsnull, nsnull, framesToInsert);

      
      mLines.splice(mLines.begin(), *overflowLines);
      NS_ASSERTION(overflowLines->empty(), "splice should empty list");
    }
    delete overflowLines;
  }
  if (ourOverflowLines) {
    if (!ourOverflowLines->empty()) {
      nsIFrame* firstFrame = ourOverflowLines->front()->mFirstChild;
      nsIFrame* lastFrame = ourOverflowLines->back()->LastChild();
      nsFrameList framesToAppend(firstFrame, lastFrame);
      mFrames.AppendFrames(nsnull, framesToAppend);

      
      mLines.splice(mLines.end(), *ourOverflowLines);
    }
    delete ourOverflowLines;
  }

  return true;
}




void
nsBlockFrame::DrainPushedFloats(nsBlockReflowState& aState)
{
#ifdef DEBUG
  
  
  
  
  
  nsLayoutUtils::AssertNoDuplicateContinuations(this, mFloats);
#endif

  
  nsBlockFrame* prevBlock = static_cast<nsBlockFrame*>(GetPrevInFlow());
  if (!prevBlock)
    return;
  nsFrameList *list = prevBlock->RemovePushedFloats();
  if (list) {
    if (list->NotEmpty()) {
      mFloats.InsertFrames(this, nsnull, *list);
    }
    delete list;
  }
}

nsLineList*
nsBlockFrame::GetOverflowLines() const
{
  if (!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_LINES)) {
    return nsnull;
  }
  nsLineList* lines = static_cast<nsLineList*>
    (Properties().Get(OverflowLinesProperty()));
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
    (Properties().Remove(OverflowLinesProperty()));
  NS_ASSERTION(lines && !lines->empty(),
               "value should always be stored and non-empty when state set");
  RemoveStateBits(NS_BLOCK_HAS_OVERFLOW_LINES);
  return lines;
}



nsresult
nsBlockFrame::SetOverflowLines(nsLineList* aOverflowLines)
{
  NS_ASSERTION(aOverflowLines, "null lines");
  NS_ASSERTION(!aOverflowLines->empty(), "empty lines");
  NS_ASSERTION(!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_LINES),
               "Overwriting existing overflow lines");

  FrameProperties props = Properties();
  
  NS_ASSERTION(!props.Get(OverflowLinesProperty()), "existing overflow list");
  props.Set(OverflowLinesProperty(), aOverflowLines);
  AddStateBits(NS_BLOCK_HAS_OVERFLOW_LINES);
  return NS_OK;
}

nsFrameList*
nsBlockFrame::GetOverflowOutOfFlows() const
{
  if (!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS)) {
    return nsnull;
  }
  nsFrameList* result =
    GetPropTableFrames(PresContext(), OverflowOutOfFlowsProperty());
  NS_ASSERTION(result, "value should always be non-empty when state set");
  return result;
}


void
nsBlockFrame::SetOverflowOutOfFlows(const nsFrameList& aList,
                                    nsFrameList* aPropValue)
{
  NS_PRECONDITION(!!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS) ==
                  !!aPropValue, "state does not match value");

  if (aList.IsEmpty()) {
    if (!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS)) {
      return;
    }
    nsFrameList* list =
      RemovePropTableFrames(PresContext(),
                            OverflowOutOfFlowsProperty());
    NS_ASSERTION(aPropValue == list, "prop value mismatch");
    delete list;
    RemoveStateBits(NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS);
  }
  else if (GetStateBits() & NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS) {
    NS_ASSERTION(aPropValue == GetPropTableFrames(PresContext(),
                                 OverflowOutOfFlowsProperty()),
                 "prop value mismatch");
    *aPropValue = aList;
  }
  else {
    SetPropTableFrames(PresContext(), new nsFrameList(aList),
                       OverflowOutOfFlowsProperty());
    AddStateBits(NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS);
  }
}

nsFrameList*
nsBlockFrame::GetPushedFloats() const
{
  if (!(GetStateBits() & NS_BLOCK_HAS_PUSHED_FLOATS)) {
    return nsnull;
  }
  nsFrameList* result =
    static_cast<nsFrameList*>(Properties().Get(PushedFloatProperty()));
  NS_ASSERTION(result, "value should always be non-empty when state set");
  return result;
}

nsFrameList*
nsBlockFrame::EnsurePushedFloats()
{
  nsFrameList *result = GetPushedFloats();
  if (result)
    return result;

  result = new nsFrameList;
  Properties().Set(PushedFloatProperty(), result);
  AddStateBits(NS_BLOCK_HAS_PUSHED_FLOATS);

  return result;
}

nsFrameList*
nsBlockFrame::RemovePushedFloats()
{
  if (!(GetStateBits() & NS_BLOCK_HAS_PUSHED_FLOATS)) {
    return nsnull;
  }

  nsFrameList *result =
    static_cast<nsFrameList*>(Properties().Remove(PushedFloatProperty()));
  RemoveStateBits(NS_BLOCK_HAS_PUSHED_FLOATS);
  NS_ASSERTION(result, "value should always be non-empty when state set");
  return result;
}




NS_IMETHODIMP
nsBlockFrame::AppendFrames(ChildListID  aListID,
                           nsFrameList& aFrameList)
{
  if (aFrameList.IsEmpty()) {
    return NS_OK;
  }
  if (aListID != kPrincipalList) {
    if (kAbsoluteList == aListID) {
      return nsContainerFrame::AppendFrames(aListID, aFrameList);
    }
    else if (kFloatList == aListID) {
      mFloats.AppendFrames(nsnull, aFrameList);
      return NS_OK;
    }
    else {
      NS_ERROR("unexpected child list");
      return NS_ERROR_INVALID_ARG;
    }
  }

  
  nsIFrame* lastKid = mLines.empty() ? nsnull : mLines.back()->LastChild();

  
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
  if (NS_FAILED(rv)) {
    return rv;
  }
  aFrameList.Clear();

  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN); 
  return NS_OK;
}

NS_IMETHODIMP
nsBlockFrame::InsertFrames(ChildListID aListID,
                           nsIFrame* aPrevFrame,
                           nsFrameList& aFrameList)
{
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  if (aListID != kPrincipalList) {
    if (kAbsoluteList == aListID) {
      return nsContainerFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
    }
    else if (kFloatList == aListID) {
      mFloats.InsertFrames(this, aPrevFrame, aFrameList);
      return NS_OK;
    }
#ifdef IBMBIDI
    else if (kNoReflowPrincipalList == aListID) {}
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
  if (NS_FAILED(rv)) {
    return rv;
  }
#ifdef IBMBIDI
  if (aListID != kNoReflowPrincipalList)
#endif 
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN); 
  return NS_OK;
}

static bool
ShouldPutNextSiblingOnNewLine(nsIFrame* aLastFrame)
{
  nsIAtom* type = aLastFrame->GetType();
  if (type == nsGkAtoms::brFrame)
    return true;
  if (type == nsGkAtoms::textFrame)
    return aLastFrame->HasTerminalNewline() &&
           aLastFrame->GetStyleText()->NewlineIsSignificant();
  return false;
}

nsresult
nsBlockFrame::AddFrames(nsFrameList& aFrameList, nsIFrame* aPrevSibling)
{
  
  ClearLineCursor();

  if (aFrameList.IsEmpty()) {
    return NS_OK;
  }

  
  
  if (!aPrevSibling && mBullet && !HaveOutsideBullet()) {
    NS_ASSERTION(!aFrameList.ContainsFrame(mBullet),
                 "Trying to make mBullet prev sibling to itself");
    aPrevSibling = mBullet;
  }
  
  nsIPresShell *presShell = PresContext()->PresShell();

  
  nsFrameList overflowFrames;
  nsLineList* lineList = &mLines;
  nsLineList::iterator prevSibLine = lineList->end();
  PRInt32 prevSiblingIndex = -1;
  if (aPrevSibling) {
    
    
    

    
    if (!nsLineBox::RFindLineContaining(aPrevSibling, lineList->begin(),
                                        prevSibLine, mFrames.LastChild(),
                                        &prevSiblingIndex)) {
      
      lineList = GetOverflowLines();
      if (lineList) {
        prevSibLine = lineList->end();
        prevSiblingIndex = -1;
        overflowFrames = nsFrameList(lineList->front()->mFirstChild,
                                     lineList->back()->LastChild());
        if (!nsLineBox::RFindLineContaining(aPrevSibling, lineList->begin(),
                                            prevSibLine,
                                            overflowFrames.LastChild(),
                                            &prevSiblingIndex)) {
          lineList = nsnull;
        }
      }
      if (!lineList) {
        
        
        NS_NOTREACHED("prev sibling not in line list");
        lineList = &mLines;
        aPrevSibling = nsnull;
        prevSibLine = lineList->end();
      }
    }
  }

  
  
  if (aPrevSibling) {
    
    
    PRInt32 rem = prevSibLine->GetChildCount() - prevSiblingIndex - 1;
    if (rem) {
      
      nsLineBox* line = NS_NewLineBox(presShell, aPrevSibling->GetNextSibling(), rem, false);
      if (!line) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      lineList->after_insert(prevSibLine, line);
      prevSibLine->SetChildCount(prevSibLine->GetChildCount() - rem);
      
      
      
      MarkLineDirty(prevSibLine);
      
      
      line->MarkDirty();
      line->SetInvalidateTextRuns(true);
    }
  }
  else if (! lineList->empty()) {
    lineList->front()->MarkDirty();
    lineList->front()->SetInvalidateTextRuns(true);
  }
  nsFrameList& frames = lineList == &mLines ? mFrames : overflowFrames;
  const nsFrameList::Slice& newFrames =
    frames.InsertFrames(nsnull, aPrevSibling, aFrameList);

  
  
  for (nsFrameList::Enumerator e(newFrames); !e.AtEnd(); e.Next()) {
    nsIFrame* newFrame = e.get();
    NS_ASSERTION(!aPrevSibling || aPrevSibling->GetNextSibling() == newFrame,
                 "Unexpected aPrevSibling");
    NS_ASSERTION(newFrame->GetType() != nsGkAtoms::placeholderFrame ||
                 (!newFrame->GetStyleDisplay()->IsAbsolutelyPositioned() &&
                  !newFrame->GetStyleDisplay()->IsFloating()),
                 "Placeholders should not float or be positioned");

    bool isBlock = newFrame->GetStyleDisplay()->IsBlockOutside();

    
    
    
    
    
    if (isBlock || prevSibLine == lineList->end() || prevSibLine->IsBlock() ||
        (aPrevSibling && ShouldPutNextSiblingOnNewLine(aPrevSibling))) {
      
      
      nsLineBox* line = NS_NewLineBox(presShell, newFrame, 1, isBlock);
      if (!line) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      if (prevSibLine != lineList->end()) {
        
        lineList->after_insert(prevSibLine, line);
        ++prevSibLine;
      }
      else {
        
        lineList->push_front(line);
        prevSibLine = lineList->begin();
      }
    }
    else {
      prevSibLine->SetChildCount(prevSibLine->GetChildCount() + 1);
      
      
      
      MarkLineDirty(prevSibLine);
    }

    aPrevSibling = newFrame;
  }

#ifdef DEBUG
  VerifyLines(true);
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

  
  if (mFloats.DestroyFrameIfPresent(aFloat)) {
    return line;
  }

  
  {
    nsAutoOOFFrameList oofs(this);
    if (oofs.mList.DestroyFrameIfPresent(aFloat)) {
      return line_end;
    }
  }

  NS_ERROR("Destroying float without removing from a child list.");
  return line_end;
}

static void MarkSameFloatManagerLinesDirty(nsBlockFrame* aBlock)
{
  nsBlockFrame* blockWithFloatMgr = aBlock;
  while (!(blockWithFloatMgr->GetStateBits() & NS_BLOCK_FLOAT_MGR)) {
    nsBlockFrame* bf = nsLayoutUtils::GetAsBlock(blockWithFloatMgr->GetParent());
    if (!bf) {
      break;
    }
    blockWithFloatMgr = bf;
  }
    
  
  
  
  
  MarkAllDescendantLinesDirty(blockWithFloatMgr);
}




static bool BlockHasAnyFloats(nsIFrame* aFrame)
{
  nsBlockFrame* block = nsLayoutUtils::GetAsBlock(aFrame);
  if (!block)
    return false;
  if (block->GetFirstChild(nsIFrame::kFloatList))
    return true;
    
  nsLineList::iterator line = block->begin_lines();
  nsLineList::iterator endLine = block->end_lines();
  while (line != endLine) {
    if (line->IsBlock() && BlockHasAnyFloats(line->mFirstChild))
      return true;
    ++line;
  }
  return false;
}

NS_IMETHODIMP
nsBlockFrame::RemoveFrame(ChildListID aListID,
                          nsIFrame* aOldFrame)
{
  nsresult rv = NS_OK;

#ifdef NOISY_REFLOW_REASON
  ListTag(stdout);
  printf(": remove ");
  nsFrame::ListTag(stdout, aOldFrame);
  printf("\n");
#endif

  if (aListID == kPrincipalList) {
    bool hasFloats = BlockHasAnyFloats(aOldFrame);
    rv = DoRemoveFrame(aOldFrame, REMOVE_FIXED_CONTINUATIONS);
    if (hasFloats) {
      MarkSameFloatManagerLinesDirty(this);
    }
  }
  else if (kAbsoluteList == aListID) {
    nsContainerFrame::RemoveFrame(aListID, aOldFrame);
    return NS_OK;
  }
  else if (kFloatList == aListID) {
    
    
    
    NS_ASSERTION(!aOldFrame->GetPrevContinuation(),
                 "RemoveFrame should not be called on pushed floats.");
    for (nsIFrame* f = aOldFrame;
         f && !(f->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER);
         f = f->GetNextContinuation()) {
      MarkSameFloatManagerLinesDirty(static_cast<nsBlockFrame*>(f->GetParent()));
    }
    DoRemoveOutOfFlowFrame(aOldFrame);
  }
#ifdef IBMBIDI
  else if (kNoReflowPrincipalList == aListID) {
    
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
    
    block->GetAbsoluteContainingBlock()->RemoveFrame(block,
                                                     kAbsoluteList,
                                                     aFrame);
  }
  else {
    
    nsIFrame* nif = aFrame->GetNextInFlow();
    if (nif) {
      static_cast<nsContainerFrame*>(nif->GetParent())
        ->DeleteNextInFlowChild(aFrame->PresContext(), nif, false);
    }
    
    
    block->RemoveFloat(aFrame);
  }
}




void
nsBlockFrame::TryAllLines(nsLineList::iterator* aIterator,
                          nsLineList::iterator* aStartIterator,
                          nsLineList::iterator* aEndIterator,
                          bool* aInOverflowLines) {
  if (*aIterator == *aEndIterator) {
    if (!*aInOverflowLines) {
      *aInOverflowLines = true;
      
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
    line_iterator aLine, bool aInOverflow)
  : mFrame(aFrame), mLine(aLine), mInOverflowLines(nsnull)
{
  if (aInOverflow) {
    mInOverflowLines = aFrame->GetOverflowLines();
    NS_ASSERTION(mInOverflowLines, "How can we be in overflow if there isn't any?");
  }
}

nsBlockInFlowLineIterator::nsBlockInFlowLineIterator(nsBlockFrame* aFrame,
    bool* aFoundValidLine)
  : mFrame(aFrame), mInOverflowLines(nsnull)
{
  mLine = aFrame->begin_lines();
  *aFoundValidLine = FindValidLine();
}

static nsIFrame*
FindChildContaining(nsBlockFrame* aFrame, nsIFrame* aFindFrame)
{
  NS_ASSERTION(aFrame, "must have frame");
  nsIFrame* child;
  while (true) {
    nsIFrame* block = aFrame;
    do {
      child = nsLayoutUtils::FindChildContainingDescendant(block, aFindFrame);
      if (child)
        break;
      block = block->GetNextContinuation();
    } while (block);
    if (!child)
      return nsnull;
    if (!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW))
      break;
    aFindFrame = aFrame->PresContext()->FrameManager()->GetPlaceholderFrameFor(child);
  }

  return child;
}

nsBlockInFlowLineIterator::nsBlockInFlowLineIterator(nsBlockFrame* aFrame,
    nsIFrame* aFindFrame, bool* aFoundValidLine)
  : mFrame(aFrame), mInOverflowLines(nsnull)
{
  *aFoundValidLine = false;

  nsIFrame* child = FindChildContaining(aFrame, aFindFrame);
  if (!child)
    return;

  
  nsLineBox* cursor = static_cast<nsLineBox*>
    (aFrame->Properties().Get(LineCursorProperty()));
  if (!cursor) {
    line_iterator iter = aFrame->begin_lines();
    if (iter != aFrame->end_lines()) {
      cursor = iter;
    }
  }

  if (cursor) {
    
    
    nsBlockFrame::line_iterator line = aFrame->line(cursor);
    nsBlockFrame::reverse_line_iterator rline = aFrame->rline(cursor);
    nsBlockFrame::line_iterator line_end = aFrame->end_lines();
    nsBlockFrame::reverse_line_iterator rline_end = aFrame->rend_lines();
    
    
    
    ++rline;
    while (line != line_end || rline != rline_end) {
      if (line != line_end) {
        if (line->Contains(child)) {
          *aFoundValidLine = true;
          mLine = line;
          return;
        }
        ++line;
      }
      if (rline != rline_end) {
        if (rline->Contains(child)) {
          *aFoundValidLine = true;
          mLine = rline;
          return;
        }
        ++rline;
      }
    }
    
  }

  
  
  
  
  

  mLine = aFrame->end_lines();

  if (!FindValidLine())
    return;

  do {
    if (mLine->Contains(child)) {
      *aFoundValidLine = true;
      return;
    }
  } while (Next());
}

nsBlockFrame::line_iterator
nsBlockInFlowLineIterator::End()
{
  return mInOverflowLines ? mInOverflowLines->end() : mFrame->end_lines();
}

bool
nsBlockInFlowLineIterator::IsLastLineInList()
{
  line_iterator end = End();
  return mLine != end && mLine.next() == end;
}

bool
nsBlockInFlowLineIterator::Next()
{
  ++mLine;
  return FindValidLine();
}

bool
nsBlockInFlowLineIterator::Prev()
{
  line_iterator begin = mInOverflowLines ? mInOverflowLines->begin() : mFrame->begin_lines();
  if (mLine != begin) {
    --mLine;
    return true;
  }
  bool currentlyInOverflowLines = mInOverflowLines != nsnull;
  while (true) {
    if (currentlyInOverflowLines) {
      mInOverflowLines = nsnull;
      mLine = mFrame->end_lines();
      if (mLine != mFrame->begin_lines()) {
        --mLine;
        return true;
      }
    } else {
      mFrame = static_cast<nsBlockFrame*>(mFrame->GetPrevInFlow());
      if (!mFrame)
        return false;
      mInOverflowLines = mFrame->GetOverflowLines();
      if (mInOverflowLines) {
        mLine = mInOverflowLines->end();
        NS_ASSERTION(mLine != mInOverflowLines->begin(), "empty overflow line list?");
        --mLine;
        return true;
      }
    }
    currentlyInOverflowLines = !currentlyInOverflowLines;
  }
}

bool
nsBlockInFlowLineIterator::FindValidLine()
{
  line_iterator end = mInOverflowLines ? mInOverflowLines->end() : mFrame->end_lines();
  if (mLine != end)
    return true;
  bool currentlyInOverflowLines = mInOverflowLines != nsnull;
  while (true) {
    if (currentlyInOverflowLines) {
      mFrame = static_cast<nsBlockFrame*>(mFrame->GetNextInFlow());
      if (!mFrame)
        return false;
      mInOverflowLines = nsnull;
      mLine = mFrame->begin_lines();
      if (mLine != mFrame->end_lines())
        return true;
    } else {
      mInOverflowLines = mFrame->GetOverflowLines();
      if (mInOverflowLines) {
        mLine = mInOverflowLines->begin();
        NS_ASSERTION(mLine != mInOverflowLines->end(), "empty overflow line list?");
        return true;
      }
    }
    currentlyInOverflowLines = !currentlyInOverflowLines;
  }
}

static nsresult RemoveBlockChild(nsIFrame* aFrame,
                                 bool      aRemoveOnlyFluidContinuations)
{
  if (!aFrame)
    return NS_OK;

  nsBlockFrame* nextBlock = nsLayoutUtils::GetAsBlock(aFrame->GetParent());
  NS_ASSERTION(nextBlock,
               "Our child's continuation's parent is not a block?");
  return nextBlock->DoRemoveFrame(aFrame,
      (aRemoveOnlyFluidContinuations ? 0 : nsBlockFrame::REMOVE_FIXED_CONTINUATIONS));
}








nsresult
nsBlockFrame::DoRemoveFrame(nsIFrame* aDeletedFrame, PRUint32 aFlags)
{
  
  ClearLineCursor();

  nsPresContext* presContext = PresContext();
  if (aDeletedFrame->GetStateBits() &
      (NS_FRAME_OUT_OF_FLOW | NS_FRAME_IS_OVERFLOW_CONTAINER)) {
    if (!aDeletedFrame->GetPrevInFlow()) {
      NS_ASSERTION(aDeletedFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW,
                   "Expected out-of-flow frame");
      DoRemoveOutOfFlowFrame(aDeletedFrame);
    }
    else {
      nsContainerFrame::DeleteNextInFlowChild(presContext, aDeletedFrame,
                                              (aFlags & FRAMES_ARE_EMPTY) != 0);
    }
    return NS_OK;
  }

  nsIPresShell* presShell = presContext->PresShell();

  
  nsLineList::iterator line_start = mLines.begin(),
                       line_end = mLines.end();
  nsLineList::iterator line = line_start;
  bool searchingOverflowList = false;
  
  
  TryAllLines(&line, &line_start, &line_end, &searchingOverflowList);
  while (line != line_end) {
    if (line->Contains(aDeletedFrame)) {
      break;
    }
    ++line;
    TryAllLines(&line, &line_start, &line_end, &searchingOverflowList);
  }

  if (line == line_end) {
    NS_ERROR("can't find deleted frame in lines");
    return NS_ERROR_FAILURE;
  }
  
  if (!(aFlags & FRAMES_ARE_EMPTY)) {
    if (line != line_start) {
      line.prev()->MarkDirty();
      line.prev()->SetInvalidateTextRuns(true);
    }
    else if (searchingOverflowList && !mLines.empty()) {
      mLines.back()->MarkDirty();
      mLines.back()->SetInvalidateTextRuns(true);
    }
  }

  while ((line != line_end) && (nsnull != aDeletedFrame)) {
    NS_ASSERTION(this == aDeletedFrame->GetParent(), "messed up delete code");
    NS_ASSERTION(line->Contains(aDeletedFrame), "frame not in line");

    if (!(aFlags & FRAMES_ARE_EMPTY)) {
      line->MarkDirty();
      line->SetInvalidateTextRuns(true);
    }

    
    
    bool isLastFrameOnLine = (1 == line->GetChildCount() ||
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

    
    
    
    if (searchingOverflowList) {
      nsIFrame* prevSibling = aDeletedFrame->GetPrevSibling();
      if (prevSibling) {
        
        
        prevSibling->SetNextSibling(nextFrame);
      }
      aDeletedFrame->SetNextSibling(nsnull);
    } else {
      mFrames.RemoveFrame(aDeletedFrame);
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
    printf(" prevSibling=%p deletedNextContinuation=%p\n",
           aDeletedFrame->GetPrevSibling(), deletedNextContinuation);
#endif

    
    if (deletedNextContinuation &&
        deletedNextContinuation->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER) {
      static_cast<nsContainerFrame*>(deletedNextContinuation->GetParent())
        ->DeleteNextInFlowChild(presContext, deletedNextContinuation, false);
      deletedNextContinuation = nsnull;
    }

    aDeletedFrame->Destroy();
    aDeletedFrame = deletedNextContinuation;

    bool haveAdvancedToNextLine = false;
    
    if (0 == lineChildCount) {
#ifdef NOISY_REMOVE_FRAME
        printf("DoRemoveFrame: %s line=%p became empty so it will be removed\n",
               searchingOverflowList?"overflow":"normal", line.get());
#endif
      nsLineBox *cur = line;
      if (!searchingOverflowList) {
        line = mLines.erase(line);
        
        
        
        
        nsRect visOverflow(cur->GetVisualOverflowArea());
#ifdef NOISY_BLOCK_INVALIDATE
        printf("%p invalidate 10 (%d, %d, %d, %d)\n",
               this, visOverflow.x, visOverflow.y,
               visOverflow.width, visOverflow.height);
#endif
        Invalidate(visOverflow);
      } else {
        nsLineList* lineList = RemoveOverflowLines();
        line = lineList->erase(line);
        if (!lineList->empty()) {
          SetOverflowLines(lineList);
        } else {
          delete lineList;
          
          
          
          line_start = mLines.begin();
          line_end = mLines.end();
          line = line_end;
        }
      }
      cur->Destroy(presShell);

      
      
      
      
      if (line != line_end) {
        line->MarkPreviousMarginDirty();
      }
      haveAdvancedToNextLine = true;
    } else {
      
      
      if (!deletedNextContinuation || isLastFrameOnLine ||
          !line->Contains(deletedNextContinuation)) {
        line->MarkDirty();
        ++line;
        haveAdvancedToNextLine = true;
      }
    }

    if (deletedNextContinuation) {
      
      if (deletedNextContinuation->GetParent() != this) {
        
        
        
        
        
        
        
        aFlags &= ~FRAMES_ARE_EMPTY;
        break;
      }

      
      
      if (haveAdvancedToNextLine) {
        if (line != line_end && !searchingOverflowList &&
            !line->Contains(deletedNextContinuation)) {
          
          
          line = line_end;
        }

        TryAllLines(&line, &line_start, &line_end, &searchingOverflowList);
#ifdef NOISY_REMOVE_FRAME
        printf("DoRemoveFrame: now on %s line=%p\n",
               searchingOverflowList?"overflow":"normal", line.get());
#endif
      }
    }
  }

  if (!(aFlags & FRAMES_ARE_EMPTY) && line.next() != line_end) {
    line.next()->MarkDirty();
    line.next()->SetInvalidateTextRuns(true);
  }

#ifdef DEBUG
  VerifyLines(true);
#endif

  
  return RemoveBlockChild(aDeletedFrame, !(aFlags & REMOVE_FIXED_CONTINUATIONS));
}

nsresult
nsBlockFrame::StealFrame(nsPresContext* aPresContext,
                         nsIFrame*      aChild,
                         bool           aForceNormal)
{
  NS_PRECONDITION(aPresContext && aChild, "null pointer");

  if ((aChild->GetStateBits() & NS_FRAME_OUT_OF_FLOW) &&
      aChild->GetStyleDisplay()->IsFloating()) {
    bool removed = mFloats.RemoveFrameIfPresent(aChild);
    if (!removed) {
      nsFrameList* list = GetPushedFloats();
      if (list) {
        removed = list->RemoveFrameIfPresent(aChild);
      }
    }
    return removed ? NS_OK : NS_ERROR_UNEXPECTED;
  }

  if ((aChild->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)
      && !aForceNormal)
    return nsContainerFrame::StealFrame(aPresContext, aChild);

  
  
  nsLineList::iterator line = mLines.begin(),
                       line_start = line,
                       line_end = mLines.end();
  bool searchingOverflowList = false;
  nsIFrame* prevSibling = nsnull;
  
  
  TryAllLines(&line, &line_start, &line_end, &searchingOverflowList);
  while (line != line_end) {
    nsIFrame* frame = line->mFirstChild;
    PRInt32 n = line->GetChildCount();
    while (--n >= 0) {
      if (frame == aChild) {
        
        if (frame == line->mFirstChild) {
          line->mFirstChild = frame->GetNextSibling();
        }
        if (searchingOverflowList) {
          
          
          if (prevSibling)
            prevSibling->SetNextSibling(frame->GetNextSibling());
          frame->SetNextSibling(nsnull);
        } else {
          mFrames.RemoveFrame(frame);
        }

        
        PRInt32 count = line->GetChildCount();
        line->SetChildCount(--count);
        if (count > 0) {
           line->MarkDirty();
        }
        else {
          
          nsLineBox* lineBox = line;
          if (searchingOverflowList) {
            
            nsLineList* lineList = RemoveOverflowLines();
            line = lineList->erase(line);
            if (!lineList->empty()) {
              nsresult rv = SetOverflowLines(lineList);
              NS_ENSURE_SUCCESS(rv, rv);
            } else {
              delete lineList;
              
              
              
              line_start = mLines.begin();
              line_end = mLines.end();
              line = line_end;
            }
          }
          else {
            line = mLines.erase(line);
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
    if (prevSibling && !prevSibling->GetNextSibling()) {
      
      prevSibling = nsnull;
    }
  }
  return NS_ERROR_UNEXPECTED;
}

void
nsBlockFrame::DeleteNextInFlowChild(nsPresContext* aPresContext,
                                    nsIFrame*      aNextInFlow,
                                    bool           aDeletingEmptyFrames)
{
  NS_PRECONDITION(aNextInFlow->GetPrevInFlow(), "bad next-in-flow");

  if (aNextInFlow->GetStateBits() &
      (NS_FRAME_OUT_OF_FLOW | NS_FRAME_IS_OVERFLOW_CONTAINER)) {
    nsContainerFrame::DeleteNextInFlowChild(aPresContext,
        aNextInFlow, aDeletingEmptyFrames);
  }
  else {
#ifdef DEBUG
    if (aDeletingEmptyFrames) {
      nsLayoutUtils::AssertTreeOnlyEmptyNextInFlows(aNextInFlow);
    }
#endif
    DoRemoveFrame(aNextInFlow,
        aDeletingEmptyFrames ? FRAMES_ARE_EMPTY : 0);
  }
}




nsRect
nsBlockFrame::AdjustFloatAvailableSpace(nsBlockReflowState& aState,
                                        const nsRect& aFloatAvailableSpace,
                                        nsIFrame* aFloatFrame)
{
  
  
  nscoord availWidth;
  const nsStyleDisplay* floatDisplay = aFloatFrame->GetStyleDisplay();

  if (NS_STYLE_DISPLAY_TABLE != floatDisplay->mDisplay ||
      eCompatibility_NavQuirks != aState.mPresContext->CompatibilityMode() ) {
    availWidth = aState.mContentArea.width;
  }
  else {
    
    
    
    
    availWidth = aFloatAvailableSpace.width;
  }

  nscoord availHeight = NS_UNCONSTRAINEDSIZE == aState.mContentArea.height
                        ? NS_UNCONSTRAINEDSIZE
                        : NS_MAX(0, aState.mContentArea.YMost() - aState.mY);

#ifdef DISABLE_FLOAT_BREAKING_IN_COLUMNS
  if (availHeight != NS_UNCONSTRAINEDSIZE &&
      nsLayoutUtils::GetClosestFrameOfType(this, nsGkAtoms::columnSetFrame)) {
    
    
    
    
    availHeight = NS_UNCONSTRAINEDSIZE;
  }
#endif

  return nsRect(aState.mContentArea.x,
                aState.mContentArea.y,
                availWidth, availHeight);
}

nscoord
nsBlockFrame::ComputeFloatWidth(nsBlockReflowState& aState,
                                const nsRect&       aFloatAvailableSpace,
                                nsIFrame*           aFloat)
{
  NS_PRECONDITION(aFloat->GetStateBits() & NS_FRAME_OUT_OF_FLOW,
                  "aFloat must be an out-of-flow frame");
  
  nsRect availSpace = AdjustFloatAvailableSpace(aState, aFloatAvailableSpace,
                                                aFloat);

  nsHTMLReflowState floatRS(aState.mPresContext, aState.mReflowState, aFloat, 
                            nsSize(availSpace.width, availSpace.height));
  return floatRS.ComputedWidth() + floatRS.mComputedBorderPadding.LeftRight() +
    floatRS.mComputedMargin.LeftRight();
}

nsresult
nsBlockFrame::ReflowFloat(nsBlockReflowState& aState,
                          const nsRect&       aAdjustedAvailableSpace,
                          nsIFrame*           aFloat,
                          nsMargin&           aFloatMargin,
                          bool                aFloatPushedDown,
                          nsReflowStatus&     aReflowStatus)
{
  NS_PRECONDITION(aFloat->GetStateBits() & NS_FRAME_OUT_OF_FLOW,
                  "aFloat must be an out-of-flow frame");
  
  aReflowStatus = NS_FRAME_COMPLETE;

#ifdef NOISY_FLOAT
  printf("Reflow Float %p in parent %p, availSpace(%d,%d,%d,%d)\n",
          aFloat, this, 
          aFloatAvailableSpace.x, aFloatAvailableSpace.y, 
          aFloatAvailableSpace.width, aFloatAvailableSpace.height
  );
#endif

  nsHTMLReflowState floatRS(aState.mPresContext, aState.mReflowState, aFloat,
                            nsSize(aAdjustedAvailableSpace.width,
                                   aAdjustedAvailableSpace.height));

  
  
  
  
  
  
  
  
  if (floatRS.mFlags.mIsTopOfPage &&
      (aFloatPushedDown ||
       aAdjustedAvailableSpace.width != aState.mContentArea.width)) {
    floatRS.mFlags.mIsTopOfPage = false;
  }

  
  nsBlockReflowContext brc(aState.mPresContext, aState.mReflowState);

  
  bool isAdjacentWithTop = aState.IsAdjacentWithTop();

  nsIFrame* clearanceFrame = nsnull;
  nsresult rv;
  do {
    nsCollapsingMargin margin;
    bool mayNeedRetry = false;
    floatRS.mDiscoveredClearance = nsnull;
    
    if (!aFloat->GetPrevInFlow()) {
      nsBlockReflowContext::ComputeCollapsedTopMargin(floatRS, &margin,
                                                      clearanceFrame, &mayNeedRetry);

      if (mayNeedRetry && !clearanceFrame) {
        floatRS.mDiscoveredClearance = &clearanceFrame;
        
        
      }
    }

    rv = brc.ReflowBlock(aAdjustedAvailableSpace, true, margin,
                         0, isAdjacentWithTop,
                         nsnull, floatRS,
                         aReflowStatus, aState);
  } while (NS_SUCCEEDED(rv) && clearanceFrame);

  
  
  if (NS_FRAME_IS_NOT_COMPLETE(aReflowStatus) &&
      (NS_UNCONSTRAINEDSIZE == aAdjustedAvailableSpace.height))
    aReflowStatus = NS_FRAME_COMPLETE;

  if (aReflowStatus & NS_FRAME_REFLOW_NEXTINFLOW) {
    aState.mReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
  }

  if (aFloat->GetType() == nsGkAtoms::letterFrame) {
    
    
    
    if (NS_FRAME_IS_NOT_COMPLETE(aReflowStatus)) 
      aReflowStatus = NS_FRAME_COMPLETE;
  }

  if (NS_FAILED(rv)) {
    return rv;
  }

  
  aFloatMargin = floatRS.mComputedMargin; 

  const nsHTMLReflowMetrics& metrics = brc.GetMetrics();

  
  
  
  
  
  
  aFloat->SetSize(nsSize(metrics.width, metrics.height));
  if (aFloat->HasView()) {
    nsContainerFrame::SyncFrameViewAfterReflow(aState.mPresContext, aFloat,
                                               aFloat->GetView(),
                                               metrics.VisualOverflow(),
                                               NS_FRAME_NO_MOVE_VIEW);
  }
  
  aFloat->DidReflow(aState.mPresContext, &floatRS,
                        NS_FRAME_REFLOW_FINISHED);

#ifdef NOISY_FLOAT
  printf("end ReflowFloat %p, sized to %d,%d\n",
         aFloat, metrics.width, metrics.height);
#endif

  return NS_OK;
}

PRUint8
nsBlockFrame::FindTrailingClear()
{
  
  for (nsIFrame* b = this; b; b = b->GetPrevInFlow()) {
    nsBlockFrame* block = static_cast<nsBlockFrame*>(b);
    line_iterator endLine = block->end_lines();
    if (endLine != block->begin_lines()) {
      --endLine;
      return endLine->GetBreakTypeAfter();
    }
  }
  return NS_STYLE_CLEAR_NONE;
}

nsresult
nsBlockFrame::ReflowPushedFloats(nsBlockReflowState& aState,
                                 nsOverflowAreas&    aOverflowAreas,
                                 nsReflowStatus&     aStatus)
{
  nsresult rv = NS_OK;
  for (nsIFrame* f = mFloats.FirstChild(), *next;
       f && (f->GetStateBits() & NS_FRAME_IS_PUSHED_FLOAT);
       f = next) {
    
    
    next = f->GetNextSibling();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    nsIFrame *prevContinuation = f->GetPrevContinuation();
    if (prevContinuation && prevContinuation->GetParent() == f->GetParent()) {
      mFloats.RemoveFrame(f);
      aState.AppendPushedFloat(f);
      continue;
    }

    if (NS_SUBTREE_DIRTY(f) || aState.mReflowState.ShouldReflowAllKids()) {
      
      nsRect oldRect = f->GetRect();
      nsRect oldOverflow = f->GetVisualOverflowRect();

      
      aState.FlowAndPlaceFloat(f);

      
      nsRect rect = f->GetRect();
      if (!rect.IsEqualInterior(oldRect)) {
        nsRect dirtyRect = oldOverflow;
        dirtyRect.MoveBy(oldRect.x, oldRect.y);
        Invalidate(dirtyRect);

        dirtyRect = f->GetVisualOverflowRect();
        dirtyRect.MoveBy(rect.x, rect.y);
        Invalidate(dirtyRect);
      }
    }
    else {
      
      nsRect region = nsFloatManager::GetRegionFor(f);
      aState.mFloatManager->AddFloat(f, region);
      if (f->GetNextInFlow())
        NS_MergeReflowStatusInto(&aStatus, NS_FRAME_OVERFLOW_INCOMPLETE);
    }

    ConsiderChildOverflow(aOverflowAreas, f);
  }

  
  if (0 != aState.ClearFloats(0, NS_STYLE_CLEAR_LEFT_AND_RIGHT)) {
    aState.mFloatBreakType = static_cast<nsBlockFrame*>(GetPrevInFlow())
                               ->FindTrailingClear();
  }

  return rv;
}

void
nsBlockFrame::RecoverFloats(nsFloatManager& aFloatManager)
{
  
  nsIFrame* stop = nsnull; 
                           
  for (nsIFrame* f = mFloats.FirstChild(); f && f != stop; f = f->GetNextSibling()) {
    nsRect region = nsFloatManager::GetRegionFor(f);
    aFloatManager.AddFloat(f, region);
    if (!stop && f->GetNextInFlow())
      stop = f->GetNextInFlow();
  }

  
  for (nsIFrame* oc = GetFirstChild(kOverflowContainersList);
       oc; oc = oc->GetNextSibling()) {
    RecoverFloatsFor(oc, aFloatManager);
  }

  
  for (nsBlockFrame::line_iterator line = begin_lines(); line != end_lines(); ++line) {
    if (line->IsBlock()) {
      RecoverFloatsFor(line->mFirstChild, aFloatManager);
    }
  }
}

void
nsBlockFrame::RecoverFloatsFor(nsIFrame*       aFrame,
                               nsFloatManager& aFloatManager)
{
  NS_PRECONDITION(aFrame, "null frame");
  
  nsBlockFrame* block = nsLayoutUtils::GetAsBlock(aFrame);
  
  
  
  if (block && !nsBlockFrame::BlockNeedsFloatManager(block)) {
    
    
    
    nsPoint pos = block->GetPosition() - block->GetRelativeOffset();
    aFloatManager.Translate(pos.x, pos.y);
    block->RecoverFloats(aFloatManager);
    aFloatManager.Translate(-pos.x, -pos.y);
  }
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
static void ComputeVisualOverflowArea(nsLineList& aLines,
                                      nscoord aWidth, nscoord aHeight,
                                      nsRect& aResult)
{
  nscoord xa = 0, ya = 0, xb = aWidth, yb = aHeight;
  for (nsLineList::iterator line = aLines.begin(), line_end = aLines.end();
       line != line_end;
       ++line) {
    
    
    nsRect visOverflow(line->GetVisualOverflowArea());
    nscoord x = visOverflow.x;
    nscoord y = visOverflow.y;
    nscoord xmost = x + visOverflow.width;
    nscoord ymost = y + visOverflow.height;
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

bool
nsBlockFrame::IsVisibleInSelection(nsISelection* aSelection)
{
  nsCOMPtr<nsIDOMHTMLHtmlElement> html(do_QueryInterface(mContent));
  nsCOMPtr<nsIDOMHTMLBodyElement> body(do_QueryInterface(mContent));
  if (html || body)
    return true;

  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mContent));
  bool visible;
  nsresult rv = aSelection->ContainsNode(node, true, &visible);
  return NS_SUCCEEDED(rv) && visible;
}

#ifdef DEBUG
static void DebugOutputDrawLine(PRInt32 aDepth, nsLineBox* aLine, bool aDrawn) {
  if (nsBlockFrame::gNoisyDamageRepair) {
    nsFrame::IndentBy(stdout, aDepth+1);
    nsRect lineArea = aLine->GetVisualOverflowArea();
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
            nsBlockFrame* aFrame, TextOverflow* aTextOverflow) {
  
  
  
  bool intersect = aLineArea.Intersects(aDirtyRect);
#ifdef DEBUG
  if (nsBlockFrame::gLamePaintMetrics) {
    aDrawnLines++;
  }
  DebugOutputDrawLine(aDepth, aLine.get(), intersect);
#endif
  
  
  
  
  
  
  
  bool lineInline = aLine->IsInline();
  bool lineMayHaveTextOverflow = aTextOverflow && lineInline;
  if (!intersect && !aBuilder->ShouldDescendIntoFrame(aFrame) &&
      !lineMayHaveTextOverflow)
    return NS_OK;

  nsDisplayListCollection collection;
  nsresult rv;
  nsDisplayList aboveTextDecorations;

  
  
  nsDisplayListSet childLists(collection,
    lineInline ? collection.Content() : collection.BlockBorderBackgrounds());
  nsIFrame* kid = aLine->mFirstChild;
  PRInt32 n = aLine->GetChildCount();
  while (--n >= 0) {
    rv = aFrame->BuildDisplayListForChild(aBuilder, kid, aDirtyRect, childLists,
                                          lineInline ? nsIFrame::DISPLAY_CHILD_INLINE : 0);
    NS_ENSURE_SUCCESS(rv, rv);
    kid = kid->GetNextSibling();
  }
  
  collection.Content()->AppendToTop(&aboveTextDecorations);

  if (lineMayHaveTextOverflow) {
    aTextOverflow->ProcessLine(collection, aLine.get());
  }

  collection.MoveTo(aLists);
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
      ::ComputeVisualOverflowArea(mLines, mRect.width, mRect.height, ca);
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
    for (nsIFrame* f = mFloats.FirstChild(); f; f = f->GetNextSibling()) {
      if (f->GetStateBits() & NS_FRAME_IS_PUSHED_FLOAT)
         BuildDisplayListForChild(aBuilder, f, aDirtyRect, aLists);
    }
  }

  aBuilder->MarkFramesForDisplayList(this, mFloats, aDirtyRect);

  
  nsAutoPtr<TextOverflow> textOverflow(
    TextOverflow::WillProcessLines(aBuilder, aLists, this));

  
  
  
  
  
  
  
  nsLineBox* cursor = aBuilder->ShouldDescendIntoFrame(this) ?
    nsnull : GetFirstLineContaining(aDirtyRect.y);
  line_iterator line_end = end_lines();
  nsresult rv = NS_OK;
  
  if (cursor) {
    for (line_iterator line = mLines.begin(cursor);
         line != line_end;
         ++line) {
      nsRect lineArea = line->GetVisualOverflowArea();
      if (!lineArea.IsEmpty()) {
        
        
        if (lineArea.y >= aDirtyRect.YMost()) {
          break;
        }
        rv = DisplayLine(aBuilder, lineArea, aDirtyRect, line, depth, drawnLines,
                         aLists, this, textOverflow);
        if (NS_FAILED(rv))
          break;
      }
    }
  } else {
    bool nonDecreasingYs = true;
    PRInt32 lineCount = 0;
    nscoord lastY = PR_INT32_MIN;
    nscoord lastYMost = PR_INT32_MIN;
    for (line_iterator line = begin_lines();
         line != line_end;
         ++line) {
      nsRect lineArea = line->GetVisualOverflowArea();
      rv = DisplayLine(aBuilder, lineArea, aDirtyRect, line, depth, drawnLines,
                       aLists, this, textOverflow);
      if (NS_FAILED(rv))
        break;
      if (!lineArea.IsEmpty()) {
        if (lineArea.y < lastY
            || lineArea.YMost() < lastYMost) {
          nonDecreasingYs = false;
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

  
  if (textOverflow) {
    textOverflow->DidProcessLines();
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
already_AddRefed<nsAccessible>
nsBlockFrame::CreateAccessible()
{
  nsAccessibilityService* accService = nsIPresShell::AccService();
  if (!accService) {
    return nsnull;
  }

  nsPresContext* presContext = PresContext();

  
  if (mContent->Tag() == nsGkAtoms::hr) {
    return accService->CreateHTMLHRAccessible(mContent,
                                              presContext->PresShell());
  }

  if (!mBullet || !presContext) {
    if (!mContent->GetParent()) {
      
      
      return nsnull;
    }
    
    nsCOMPtr<nsIDOMHTMLDocument> htmlDoc =
      do_QueryInterface(mContent->GetDocument());
    if (htmlDoc) {
      nsCOMPtr<nsIDOMHTMLElement> body;
      htmlDoc->GetBody(getter_AddRefs(body));
      if (SameCOMIdentity(body, mContent)) {
        
        
        return nsnull;
      }
    }

    
    return accService->CreateHyperTextAccessible(mContent,
                                                 presContext->PresShell());
  }

  
  return accService->CreateHTMLLIAccessible(mContent, presContext->PresShell());
}
#endif

void nsBlockFrame::ClearLineCursor()
{
  if (!(GetStateBits() & NS_BLOCK_HAS_LINE_CURSOR)) {
    return;
  }

  Properties().Delete(LineCursorProperty());
  RemoveStateBits(NS_BLOCK_HAS_LINE_CURSOR);
}

void nsBlockFrame::SetupLineCursor()
{
  if (GetStateBits() & NS_BLOCK_HAS_LINE_CURSOR
      || mLines.empty()) {
    return;
  }
   
  Properties().Set(LineCursorProperty(), mLines.front());
  AddStateBits(NS_BLOCK_HAS_LINE_CURSOR);
}

nsLineBox* nsBlockFrame::GetFirstLineContaining(nscoord y)
{
  if (!(GetStateBits() & NS_BLOCK_HAS_LINE_CURSOR)) {
    return nsnull;
  }

  FrameProperties props = Properties();
  
  nsLineBox* property = static_cast<nsLineBox*>
    (props.Get(LineCursorProperty()));
  line_iterator cursor = mLines.begin(property);
  nsRect cursorArea = cursor->GetVisualOverflowArea();

  while ((cursorArea.IsEmpty() || cursorArea.YMost() > y)
         && cursor != mLines.front()) {
    cursor = cursor.prev();
    cursorArea = cursor->GetVisualOverflowArea();
  }
  while ((cursorArea.IsEmpty() || cursorArea.YMost() <= y)
         && cursor != mLines.back()) {
    cursor = cursor.next();
    cursorArea = cursor->GetVisualOverflowArea();
  }

  if (cursor.get() != property) {
    props.Set(LineCursorProperty(), cursor.get());
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
    
    
    
    bool isValid;
    nsBlockInFlowLineIterator iter(this, aChild, &isValid);
    if (isValid) {
      iter.GetContainer()->MarkLineDirty(iter.GetLine(), iter.GetLineList());
    }
  }

  nsBlockFrameSuper::ChildIsDirty(aChild);
}

NS_IMETHODIMP
nsBlockFrame::Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        aPrevInFlow)
{
  if (aPrevInFlow) {
    
    nsBlockFrame*  blockFrame = (nsBlockFrame*)aPrevInFlow;

    
    
    SetFlags(blockFrame->mState &
             (NS_BLOCK_FLAGS_MASK &
               (~NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET &
                ~NS_BLOCK_HAS_FIRST_LETTER_CHILD)));
  }

  nsresult rv = nsBlockFrameSuper::Init(aContent, aParent, aPrevInFlow);

  if (!aPrevInFlow ||
      aPrevInFlow->GetStateBits() & NS_BLOCK_NEEDS_BIDI_RESOLUTION)
    AddStateBits(NS_BLOCK_NEEDS_BIDI_RESOLUTION);

  return rv;
}

NS_IMETHODIMP
nsBlockFrame::SetInitialChildList(ChildListID     aListID,
                                  nsFrameList&    aChildList)
{
  nsresult rv = NS_OK;

  if (kAbsoluteList == aListID) {
    nsContainerFrame::SetInitialChildList(aListID, aChildList);
  }
  else if (kFloatList == aListID) {
    mFloats.SetFrames(aChildList);
  }
  else {
    nsPresContext* presContext = PresContext();

#ifdef DEBUG
    
    
    
    
    
    
    
    nsIAtom *pseudo = GetStyleContext()->GetPseudo();
    bool haveFirstLetterStyle =
      (!pseudo ||
       (pseudo == nsCSSAnonBoxes::cellContent &&
        mParent->GetStyleContext()->GetPseudo() == nsnull) ||
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

    
    
    
    
    
    
    
    
    
    
    
    nsIFrame* possibleListItem = this;
    while (1) {
      nsIFrame* parent = possibleListItem->GetParent();
      if (parent->GetContent() != GetContent()) {
        break;
      }
      possibleListItem = parent;
    }
    if ((nsnull == GetPrevInFlow()) &&
        (NS_STYLE_DISPLAY_LIST_ITEM ==
           possibleListItem->GetStyleDisplay()->mDisplay) &&
        (nsnull == mBullet)) {
      
      const nsStyleList* styleList = GetStyleList();
      nsCSSPseudoElements::Type pseudoType;
      switch (styleList->mListStyleType) {
        case NS_STYLE_LIST_STYLE_DISC:
        case NS_STYLE_LIST_STYLE_CIRCLE:
        case NS_STYLE_LIST_STYLE_SQUARE:
          pseudoType = nsCSSPseudoElements::ePseudo_mozListBullet;
          break;
        default:
          pseudoType = nsCSSPseudoElements::ePseudo_mozListNumber;
          break;
      }

      nsIPresShell *shell = presContext->PresShell();

      nsStyleContext* parentStyle =
        CorrectStyleParentFrame(this,
          nsCSSPseudoElements::GetPseudoAtom(pseudoType))->GetStyleContext();
      nsRefPtr<nsStyleContext> kidSC = shell->StyleSet()->
        ResolvePseudoElementStyle(mContent->AsElement(), pseudoType,
                                  parentStyle);

      
      nsBulletFrame* bullet = new (shell) nsBulletFrame(kidSC);
      if (nsnull == bullet) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      bullet->Init(mContent, this, nsnull);

      
      
      if (NS_STYLE_LIST_STYLE_POSITION_INSIDE ==
          styleList->mListStylePosition) {
        nsFrameList bulletList(bullet, bullet);
        AddFrames(bulletList, nsnull);
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

bool
nsBlockFrame::BulletIsEmpty() const
{
  NS_ASSERTION(mContent->GetPrimaryFrame()->GetStyleDisplay()->mDisplay ==
                 NS_STYLE_DISPLAY_LIST_ITEM &&
               HaveOutsideBullet(),
               "should only care when we have an outside bullet");
  const nsStyleList* list = GetStyleList();
  return list->mListStyleType == NS_STYLE_LIST_STYLE_NONE &&
         !list->GetListStyleImage();
}

void
nsBlockFrame::GetBulletText(nsAString& aText) const
{
  aText.Truncate();

  const nsStyleList* myList = GetStyleList();
  if (myList->GetListStyleImage() ||
      myList->mListStyleType == NS_STYLE_LIST_STYLE_DISC) {
    aText.Assign(kDiscCharacter);
  }
  else if (myList->mListStyleType == NS_STYLE_LIST_STYLE_CIRCLE) {
    aText.Assign(kCircleCharacter);
  }
  else if (myList->mListStyleType == NS_STYLE_LIST_STYLE_SQUARE) {
    aText.Assign(kSquareCharacter);
  }
  else if (myList->mListStyleType != NS_STYLE_LIST_STYLE_NONE) {
    nsAutoString text;
    mBullet->GetListItemText(*myList, text);
    aText = text;
  }
}

bool
nsBlockFrame::HasBullet() const
{
  if (mBullet) {
    const nsStyleList* styleList = GetStyleList();
    return styleList->GetListStyleImage() ||
      styleList->mListStyleType != NS_STYLE_LIST_STYLE_NONE;
  }

  return false;
}


bool
nsBlockFrame::FrameStartsCounterScope(nsIFrame* aFrame)
{
  nsIContent* content = aFrame->GetContent();
  if (!content || !content->IsHTML())
    return false;

  nsIAtom *localName = content->NodeInfo()->NameAtom();
  return localName == nsGkAtoms::ol ||
         localName == nsGkAtoms::ul ||
         localName == nsGkAtoms::dir ||
         localName == nsGkAtoms::menu;
}

bool
nsBlockFrame::RenumberLists(nsPresContext* aPresContext)
{
  if (!FrameStartsCounterScope(this)) {
    
    
    return false;
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

bool
nsBlockFrame::RenumberListsInBlock(nsPresContext* aPresContext,
                                   nsBlockFrame* aBlockFrame,
                                   PRInt32* aOrdinal,
                                   PRInt32 aDepth)
{
  
  bool foundValidLine;
  nsBlockInFlowLineIterator bifLineIter(aBlockFrame, &foundValidLine);
  
  if (!foundValidLine)
    return false;

  bool renumberedABullet = false;

  do {
    nsLineList::iterator line = bifLineIter.GetLine();
    nsIFrame* kid = line->mFirstChild;
    PRInt32 n = line->GetChildCount();
    while (--n >= 0) {
      bool kidRenumberedABullet = RenumberListsFor(aPresContext, kid, aOrdinal, aDepth);
      if (kidRenumberedABullet) {
        line->MarkDirty();
        renumberedABullet = true;
      }
      kid = kid->GetNextSibling();
    }
  } while (bifLineIter.Next());

  return renumberedABullet;
}

bool
nsBlockFrame::RenumberListsFor(nsPresContext* aPresContext,
                               nsIFrame* aKid,
                               PRInt32* aOrdinal,
                               PRInt32 aDepth)
{
  NS_PRECONDITION(aPresContext && aKid && aOrdinal, "null params are immoral!");

  
  if (MAX_DEPTH_FOR_LIST_RENUMBERING < aDepth)
    return false;

  
  nsIFrame* kid = nsPlaceholderFrame::GetRealFrameFor(aKid);
  const nsStyleDisplay* display = kid->GetStyleDisplay();

  
  kid = kid->GetContentInsertionFrame();

  
  if (!kid)
    return false;

  bool kidRenumberedABullet = false;

  
  
  
  if (NS_STYLE_DISPLAY_LIST_ITEM == display->mDisplay) {
    
    
    nsBlockFrame* listItem = nsLayoutUtils::GetAsBlock(kid);
    if (listItem) {
      if (nsnull != listItem->mBullet) {
        bool changed;
        *aOrdinal = listItem->mBullet->SetListItemOrdinal(*aOrdinal,
                                                          &changed);
        if (changed) {
          kidRenumberedABullet = true;

          
          listItem->ChildIsDirty(listItem->mBullet);
        }
      }

      
      
      
      bool meToo = RenumberListsInBlock(aPresContext, listItem, aOrdinal, aDepth + 1);
      if (meToo) {
        kidRenumberedABullet = true;
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
  
  availSize.width = aState.mContentArea.width;
  availSize.height = NS_UNCONSTRAINEDSIZE;

  
  
  
  nsHTMLReflowState reflowState(aState.mPresContext, rs,
                                mBullet, availSize);
  nsReflowStatus  status;
  mBullet->WillReflow(aState.mPresContext);
  mBullet->Reflow(aState.mPresContext, aMetrics, reflowState, status);

  
  
  
  
  
  nsRect floatAvailSpace =
    aState.GetFloatAvailableSpaceWithState(aLineTop,
                                           &aState.mFloatManagerStateBefore)
          .mRect;
  
  

  
  
  
  
  
  
  
  
  nscoord x;
  if (rs.mStyleVisibility->mDirection == NS_STYLE_DIRECTION_LTR) {
    
    
    
    x = floatAvailSpace.x - rs.mComputedBorderPadding.left
        - reflowState.mComputedMargin.right - aMetrics.width;
  } else {
    
    
    
    x = floatAvailSpace.XMost() + rs.mComputedBorderPadding.right
        + reflowState.mComputedMargin.left;
  }

  
  
  nscoord y = aState.mContentArea.y;
  mBullet->SetRect(nsRect(x, y, aMetrics.width, aMetrics.height));
  mBullet->DidReflow(aState.mPresContext, &aState.mReflowState, NS_FRAME_REFLOW_FINISHED);
}





void nsBlockFrame::CollectFloats(nsIFrame* aFrame, nsFrameList& aList,
                                 bool aFromOverflow, bool aCollectSiblings) {
  while (aFrame) {
    
    if (!aFrame->IsFloatContainingBlock()) {
      nsIFrame *outOfFlowFrame =
        aFrame->GetType() == nsGkAtoms::placeholderFrame ?
          nsLayoutUtils::GetFloatFromPlaceholder(aFrame) : nsnull;
      if (outOfFlowFrame) {
        if (outOfFlowFrame->GetStateBits() & NS_FRAME_IS_PUSHED_FLOAT) {
          if (outOfFlowFrame->GetParent() == this) {
            nsFrameList* list = GetPushedFloats();
            if (!list || !list->RemoveFrameIfPresent(outOfFlowFrame)) {
              if (aFromOverflow) {
                nsAutoOOFFrameList oofs(this);
                oofs.mList.RemoveFrame(outOfFlowFrame);
              } else {
                mFloats.RemoveFrame(outOfFlowFrame);
              }
            }
            aList.AppendFrame(nsnull, outOfFlowFrame);
          }
          
          
          
        } else {
          
          
          
          
          NS_ASSERTION(outOfFlowFrame->GetParent() == this,
                       "Out of flow frame doesn't have the expected parent");
          if (aFromOverflow) {
            nsAutoOOFFrameList oofs(this);
            oofs.mList.RemoveFrame(outOfFlowFrame);
          } else {
            mFloats.RemoveFrame(outOfFlowFrame);
          }
          aList.AppendFrame(nsnull, outOfFlowFrame);
        }
      }

      CollectFloats(aFrame->GetFirstPrincipalChild(), 
                    aList, aFromOverflow, true);
      
      
      
      CollectFloats(aFrame->GetFirstChild(kOverflowList), 
                    aList, aFromOverflow, true);
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
  
  
  
  bool anyLineDirty = false;

  
  nsAutoTArray<nsIFrame*, 8> lineFloats;
  for (line_iterator line = begin_lines(), line_end = end_lines();
       line != line_end; ++line) {
    if (line->HasFloats()) {
      nsFloatCache* fc = line->GetFirstFloat();
      while (fc) {
        lineFloats.AppendElement(fc->mFloat);
        fc = fc->Next();
      }
    }
    if (line->IsDirty()) {
      anyLineDirty = true;
    }
  }
  
  nsAutoTArray<nsIFrame*, 8> storedFloats;
  bool equal = true;
  PRUint32 i = 0;
  for (nsIFrame* f = mFloats.FirstChild(); f; f = f->GetNextSibling()) {
    if (f->GetStateBits() & NS_FRAME_IS_PUSHED_FLOAT)
      continue;
    storedFloats.AppendElement(f);
    if (i < lineFloats.Length() && lineFloats.ElementAt(i) != f) {
      equal = false;
    }
    ++i;
  }

  if ((!equal || lineFloats.Length() != storedFloats.Length()) && !anyLineDirty) {
    NS_WARNING("nsBlockFrame::CheckFloats: Explicit float list is out of sync with float cache");
#if defined(DEBUG_roc)
    nsFrame::RootFrameList(PresContext(), stdout, 0);
    for (i = 0; i < lineFloats.Length(); ++i) {
      printf("Line float: %p\n", lineFloats.ElementAt(i));
    }
    for (i = 0; i < storedFloats.Length(); ++i) {
      printf("Stored float: %p\n", storedFloats.ElementAt(i));
    }
#endif
  }
#endif

  const nsFrameList* oofs = GetOverflowOutOfFlows();
  if (oofs && oofs->NotEmpty()) {
    
    
    
    
    
    
    
    
    
    
    aState.mFloatManager->RemoveTrailingRegions(oofs->FirstChild());
  }
}


bool
nsBlockFrame::BlockIsMarginRoot(nsIFrame* aBlock)
{
  NS_PRECONDITION(aBlock, "Must have a frame");
  NS_ASSERTION(nsLayoutUtils::GetAsBlock(aBlock), "aBlock must be a block");

  nsIFrame* parent = aBlock->GetParent();
  return (aBlock->GetStateBits() & NS_BLOCK_MARGIN_ROOT) ||
    (parent && !parent->IsFloatContainingBlock() &&
     parent->GetType() != nsGkAtoms::columnSetFrame);
}


bool
nsBlockFrame::BlockNeedsFloatManager(nsIFrame* aBlock)
{
  NS_PRECONDITION(aBlock, "Must have a frame");
  NS_ASSERTION(nsLayoutUtils::GetAsBlock(aBlock), "aBlock must be a block");

  nsIFrame* parent = aBlock->GetParent();
  return (aBlock->GetStateBits() & NS_BLOCK_FLOAT_MGR) ||
    (parent && !parent->IsFloatContainingBlock());
}


bool
nsBlockFrame::BlockCanIntersectFloats(nsIFrame* aFrame)
{
  return aFrame->IsFrameOfType(nsIFrame::eBlockFrame) &&
         !aFrame->IsFrameOfType(nsIFrame::eReplaced) &&
         !(aFrame->GetStateBits() & NS_BLOCK_FLOAT_MGR);
}






nsBlockFrame::ReplacedElementWidthToClear
nsBlockFrame::WidthToClearPastFloats(nsBlockReflowState& aState,
                                     const nsRect& aFloatAvailableSpace,
                                     nsIFrame* aFrame)
{
  nscoord leftOffset, rightOffset;
  nsCSSOffsetState offsetState(aFrame, aState.mReflowState.rendContext,
                               aState.mContentArea.width);

  ReplacedElementWidthToClear result;
  aState.ComputeReplacedBlockOffsetsForFloats(aFrame, aFloatAvailableSpace,
                                              leftOffset, rightOffset);
  nscoord availWidth = aState.mContentArea.width - leftOffset - rightOffset;

  
  
  
  
  
  
  nsSize availSpace(availWidth, NS_UNCONSTRAINEDSIZE);
  nsHTMLReflowState reflowState(aState.mPresContext, aState.mReflowState,
                                aFrame, availSpace);
  result.borderBoxWidth = reflowState.ComputedWidth() +
                          reflowState.mComputedBorderPadding.LeftRight();
  
  
  result.marginLeft  = offsetState.mComputedMargin.left;
  result.marginRight = offsetState.mComputedMargin.right;
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

nscoord
nsBlockFrame::GetEffectiveComputedHeight(const nsHTMLReflowState& aReflowState) const
{
  nscoord height = aReflowState.ComputedHeight();
  NS_ABORT_IF_FALSE(height != NS_UNCONSTRAINEDSIZE, "Don't call me!");

  if (GetPrevInFlow()) {
    
    for (nsIFrame* prev = GetPrevInFlow(); prev; prev = prev->GetPrevInFlow()) {
      height -= prev->GetRect().height;
    }
    
    
    height += aReflowState.mComputedBorderPadding.top;
    
    height = NS_MAX(0, height);
  }
  return height;
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

  return nsBidiPresUtils::Resolve(this);
}
#endif

#ifdef DEBUG
void
nsBlockFrame::VerifyLines(bool aFinalCheckOK)
{
  if (!gVerifyLines) {
    return;
  }
  if (mLines.empty()) {
    return;
  }

  
  
  PRInt32 count = 0;
  bool seenBlock = false;
  line_iterator line, line_end;
  for (line = begin_lines(), line_end = end_lines();
       line != line_end;
       ++line) {
    if (aFinalCheckOK) {
      NS_ABORT_IF_FALSE(line->GetChildCount(), "empty line");
      if (line->IsBlock()) {
        seenBlock = true;
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
