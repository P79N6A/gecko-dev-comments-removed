










#include "nsBlockFrame.h"

#include "mozilla/DebugOnly.h"

#include "nsCOMPtr.h"
#include "nsAbsoluteContainingBlock.h"
#include "nsBlockReflowContext.h"
#include "nsBlockReflowState.h"
#include "nsBulletFrame.h"
#include "nsFontMetrics.h"
#include "nsLineBox.h"
#include "nsLineLayout.h"
#include "nsPlaceholderFrame.h"
#include "nsStyleConsts.h"
#include "nsFrameManager.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsStyleContext.h"
#include "nsHTMLParts.h"
#include "nsGkAtoms.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValueInlines.h"
#include "prprf.h"
#include "nsFloatManager.h"
#include "prenv.h"
#include "plstr.h"
#include "nsError.h"
#include "nsAutoPtr.h"
#include "nsIScrollableFrame.h"
#include <algorithm>
#ifdef ACCESSIBILITY
#include "nsIDOMHTMLDocument.h"
#endif
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"
#include "nsCSSAnonBoxes.h"
#include "nsCSSFrameConstructor.h"
#include "nsRenderingContext.h"
#include "TextOverflow.h"
#include "nsIFrameInlines.h"
#include "CounterStyleManager.h"
#include "nsISelection.h"

#include "nsBidiPresUtils.h"

static const int MIN_LINES_NEEDING_CURSOR = 20;

static const char16_t kDiscCharacter = 0x2022;

using namespace mozilla;
using namespace mozilla::css;
using namespace mozilla::layout;

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

int32_t nsBlockFrame::gNoiseIndent;

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
  nsStyleContext* sc = aFrame->StyleContext();
  while (nullptr != sc) {
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
  static uint32_t record[2];

  
  
  int index = 0;
  if (!aChildIsBlock) index |= 1;

  
  uint32_t newS = record[index];
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

NS_DECLARE_FRAME_PROPERTY(OverflowLinesProperty, DestroyOverflowLines)
NS_DECLARE_FRAME_PROPERTY_FRAMELIST(OverflowOutOfFlowsProperty)
NS_DECLARE_FRAME_PROPERTY_FRAMELIST(PushedFloatProperty)
NS_DECLARE_FRAME_PROPERTY_FRAMELIST(OutsideBulletProperty)
NS_DECLARE_FRAME_PROPERTY(InsideBulletProperty, nullptr)
NS_DECLARE_FRAME_PROPERTY(BlockEndEdgeOfChildrenProperty, nullptr)



nsBlockFrame*
NS_NewBlockFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, nsFrameState aFlags)
{
  nsBlockFrame* it = new (aPresShell) nsBlockFrame(aContext);
  it->SetFlags(aFlags);
  return it;
}

NS_IMPL_FRAMEARENA_HELPERS(nsBlockFrame)

nsBlockFrame::~nsBlockFrame()
{
}

void
nsBlockFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  ClearLineCursor();
  DestroyAbsoluteFrames(aDestructRoot);
  mFloats.DestroyFramesFrom(aDestructRoot);
  nsPresContext* presContext = PresContext();
  nsIPresShell* shell = presContext->PresShell();
  nsLineBox::DeleteLineList(presContext, mLines, aDestructRoot,
                            &mFrames);

  FramePropertyTable* props = presContext->PropertyTable();

  if (HasPushedFloats()) {
    SafelyDestroyFrameListProp(aDestructRoot, shell, props,
                               PushedFloatProperty());
    RemoveStateBits(NS_BLOCK_HAS_PUSHED_FLOATS);
  }

  
  FrameLines* overflowLines = RemoveOverflowLines();
  if (overflowLines) {
    nsLineBox::DeleteLineList(presContext, overflowLines->mLines,
                              aDestructRoot, &overflowLines->mFrames);
    delete overflowLines;
  }

  if (GetStateBits() & NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS) {
    SafelyDestroyFrameListProp(aDestructRoot, shell, props,
                               OverflowOutOfFlowsProperty());
    RemoveStateBits(NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS);
  }

  if (HasOutsideBullet()) {
    SafelyDestroyFrameListProp(aDestructRoot, shell, props,
                               OutsideBulletProperty());
    RemoveStateBits(NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET);
  }

  nsBlockFrameSuper::DestroyFrom(aDestructRoot);
}

 nsILineIterator*
nsBlockFrame::GetLineIterator()
{
  nsLineIterator* it = new nsLineIterator;
  if (!it)
    return nullptr;

  const nsStyleVisibility* visibility = StyleVisibility();
  nsresult rv = it->Init(mLines, visibility->mDirection == NS_STYLE_DIRECTION_RTL);
  if (NS_FAILED(rv)) {
    delete it;
    return nullptr;
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

#ifdef DEBUG_FRAME_DUMP
void
nsBlockFrame::List(FILE* out, const char* aPrefix, uint32_t aFlags) const
{
  nsCString str;
  ListGeneric(str, aPrefix, aFlags);

  fprintf_stderr(out, "%s<\n", str.get());

  nsCString pfx(aPrefix);
  pfx += "  ";

  
  if (!mLines.empty()) {
    const_line_iterator line = begin_lines(), line_end = end_lines();
    for ( ; line != line_end; ++line) {
      line->List(out, pfx.get(), aFlags);
    }
  }

  
  const FrameLines* overflowLines = GetOverflowLines();
  if (overflowLines && !overflowLines->mLines.empty()) {
    fprintf_stderr(out, "%sOverflow-lines %p/%p <\n", pfx.get(), overflowLines, &overflowLines->mFrames);
    nsCString nestedPfx(pfx);
    nestedPfx += "  ";
    const_line_iterator line = overflowLines->mLines.begin(),
                        line_end = overflowLines->mLines.end();
    for ( ; line != line_end; ++line) {
      line->List(out, nestedPfx.get(), aFlags);
    }
    fprintf_stderr(out, "%s>\n", pfx.get());
  }

  
  
  ChildListIterator lists(this);
  ChildListIDs skip(kPrincipalList | kOverflowList);
  for (; !lists.IsDone(); lists.Next()) {
    if (skip.Contains(lists.CurrentID())) {
      continue;
    }
    fprintf_stderr(out, "%s%s %p <\n", pfx.get(),
      mozilla::layout::ChildListName(lists.CurrentID()),
      &GetChildList(lists.CurrentID()));
    nsCString nestedPfx(pfx);
    nestedPfx += "  ";
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* kid = childFrames.get();
      kid->List(out, nestedPfx.get(), aFlags);
    }
    fprintf_stderr(out, "%s>\n", pfx.get());
  }

  fprintf_stderr(out, "%s>\n", aPrefix);
}

nsresult
nsBlockFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Block"), aResult);
}
#endif

#ifdef DEBUG
nsFrameState
nsBlockFrame::GetDebugStateBits() const
{
  
  
  return nsBlockFrameSuper::GetDebugStateBits() & ~NS_BLOCK_HAS_LINE_CURSOR;
}
#endif

nsIAtom*
nsBlockFrame::GetType() const
{
  return nsGkAtoms::blockFrame;
}

void
nsBlockFrame::InvalidateFrame(uint32_t aDisplayItemKey)
{
  if (IsSVGText()) {
    NS_ASSERTION(GetParent()->GetType() == nsGkAtoms::svgTextFrame,
                 "unexpected block frame in SVG text");
    GetParent()->InvalidateFrame();
    return;
  }
  nsBlockFrameSuper::InvalidateFrame(aDisplayItemKey);
}

void
nsBlockFrame::InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey)
{
  if (IsSVGText()) {
    NS_ASSERTION(GetParent()->GetType() == nsGkAtoms::svgTextFrame,
                 "unexpected block frame in SVG text");
    GetParent()->InvalidateFrame();
    return;
  }
  nsBlockFrameSuper::InvalidateFrameWithRect(aRect, aDisplayItemKey);
}

nscoord
nsBlockFrame::GetLogicalBaseline(WritingMode aWritingMode) const
{
  nscoord result;
  if (nsLayoutUtils::GetLastLineBaseline(aWritingMode, this, &result))
    return result;
  return nsFrame::GetLogicalBaseline(aWritingMode);
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
  float inflation = nsLayoutUtils::FontSizeInflationFor(this);
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm), inflation);
  nscoord lineHeight =
    nsHTMLReflowState::CalcLineHeight(GetContent(), StyleContext(),
                                      contentRect.height, inflation);
  const WritingMode wm = GetWritingMode();
  return nsLayoutUtils::GetCenteredFontBaseline(fm, lineHeight,
                                                wm.IsLineInverted()) + bp.top;
}




const nsFrameList&
nsBlockFrame::GetChildList(ChildListID aListID) const
{
  switch (aListID) {
    case kPrincipalList:
      return mFrames;
    case kOverflowList: {
      FrameLines* overflowLines = GetOverflowLines();
      return overflowLines ? overflowLines->mFrames : nsFrameList::EmptyList();
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
    case kBulletList: {
      const nsFrameList* list = GetOutsideBulletList();
      return list ? *list : nsFrameList::EmptyList();
    }
    default:
      return nsContainerFrame::GetChildList(aListID);
  }
}

void
nsBlockFrame::GetChildLists(nsTArray<ChildList>* aLists) const
{
  nsContainerFrame::GetChildLists(aLists);
  FrameLines* overflowLines = GetOverflowLines();
  if (overflowLines) {
    overflowLines->mFrames.AppendIfNonempty(aLists, kOverflowList);
  }
  const nsFrameList* list = GetOverflowOutOfFlows();
  if (list) {
    list->AppendIfNonempty(aLists, kOverflowOutOfFlowList);
  }
  mFloats.AppendIfNonempty(aLists, kFloatList);
  list = GetOutsideBulletList();
  if (list) {
    list->AppendIfNonempty(aLists, kBulletList);
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

static void
ReparentFrame(nsIFrame* aFrame, nsContainerFrame* aOldParent,
              nsContainerFrame* aNewParent)
{
  NS_ASSERTION(aOldParent == aFrame->GetParent(),
               "Parent not consistent with expectations");

  aFrame->SetParent(aNewParent);

  
  
  nsContainerFrame::ReparentFrameView(aFrame, aOldParent, aNewParent);
}
 
static void
ReparentFrames(nsFrameList& aFrameList, nsContainerFrame* aOldParent,
               nsContainerFrame* aNewParent)
{
  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    ReparentFrame(e.get(), aOldParent, aNewParent);
  }
}
 








static bool
RemoveFirstLine(nsLineList& aFromLines, nsFrameList& aFromFrames,
                nsLineBox** aOutLine, nsFrameList* aOutFrames)
{
  nsLineList_iterator removedLine = aFromLines.begin();
  *aOutLine = removedLine;
  nsLineList_iterator next = aFromLines.erase(removedLine);
  bool isLastLine = next == aFromLines.end();
  nsIFrame* lastFrame = isLastLine ? aFromFrames.LastChild()
                                   : next->mFirstChild->GetPrevSibling();
  nsFrameList::FrameLinkEnumerator linkToBreak(aFromFrames, lastFrame);
  *aOutFrames = aFromFrames.ExtractHead(linkToBreak);
  return isLastLine;
}




 void
nsBlockFrame::MarkIntrinsicISizesDirty()
{
  nsBlockFrame* dirtyBlock = static_cast<nsBlockFrame*>(FirstContinuation());
  dirtyBlock->mMinWidth = NS_INTRINSIC_WIDTH_UNKNOWN;
  dirtyBlock->mPrefWidth = NS_INTRINSIC_WIDTH_UNKNOWN;
  if (!(GetStateBits() & NS_BLOCK_NEEDS_BIDI_RESOLUTION)) {
    for (nsIFrame* frame = dirtyBlock; frame; 
         frame = frame->GetNextContinuation()) {
      frame->AddStateBits(NS_BLOCK_NEEDS_BIDI_RESOLUTION);
    }
  }

  nsBlockFrameSuper::MarkIntrinsicISizesDirty();
}

void
nsBlockFrame::CheckIntrinsicCacheAgainstShrinkWrapState()
{
  nsPresContext *presContext = PresContext();
  if (!nsLayoutUtils::FontSizeInflationEnabled(presContext)) {
    return;
  }
  bool inflationEnabled =
    !presContext->mInflationDisabledForShrinkWrap;
  if (inflationEnabled !=
      !!(GetStateBits() & NS_BLOCK_FRAME_INTRINSICS_INFLATED)) {
    mMinWidth = NS_INTRINSIC_WIDTH_UNKNOWN;
    mPrefWidth = NS_INTRINSIC_WIDTH_UNKNOWN;
    if (inflationEnabled) {
      AddStateBits(NS_BLOCK_FRAME_INTRINSICS_INFLATED);
    } else {
      RemoveStateBits(NS_BLOCK_FRAME_INTRINSICS_INFLATED);
    }
  }
}

 nscoord
nsBlockFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nsIFrame* firstInFlow = FirstContinuation();
  if (firstInFlow != this)
    return firstInFlow->GetMinISize(aRenderingContext);

  DISPLAY_MIN_WIDTH(this, mMinWidth);

  CheckIntrinsicCacheAgainstShrinkWrapState();

  if (mMinWidth != NS_INTRINSIC_WIDTH_UNKNOWN)
    return mMinWidth;

#ifdef DEBUG
  if (gNoisyIntrinsic) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": GetMinISize\n");
  }
  AutoNoisyIndenter indenter(gNoisyIntrinsic);
#endif

  for (nsBlockFrame* curFrame = this; curFrame;
       curFrame = static_cast<nsBlockFrame*>(curFrame->GetNextContinuation())) {
    curFrame->LazyMarkLinesDirty();
  }

  if (GetStateBits() & NS_BLOCK_NEEDS_BIDI_RESOLUTION)
    ResolveBidi();
  InlineMinISizeData data;
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
                        line->mFirstChild, nsLayoutUtils::MIN_ISIZE);
        data.ForceBreak(aRenderingContext);
      } else {
        if (!curFrame->GetPrevContinuation() &&
            line == curFrame->begin_lines()) {
          
          
          
          const nsStyleCoord &indent = StyleText()->mTextIndent;
          if (indent.ConvertsToLength())
            data.currentLine += nsRuleNode::ComputeCoordPercentCalc(indent, 0);
        }
        

        data.line = &line;
        data.lineContainer = curFrame;
        nsIFrame *kid = line->mFirstChild;
        for (int32_t i = 0, i_end = line->GetChildCount(); i != i_end;
             ++i, kid = kid->GetNextSibling()) {
          kid->AddInlineMinISize(aRenderingContext, &data);
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
nsBlockFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  nsIFrame* firstInFlow = FirstContinuation();
  if (firstInFlow != this)
    return firstInFlow->GetPrefISize(aRenderingContext);

  DISPLAY_PREF_WIDTH(this, mPrefWidth);

  CheckIntrinsicCacheAgainstShrinkWrapState();

  if (mPrefWidth != NS_INTRINSIC_WIDTH_UNKNOWN)
    return mPrefWidth;

#ifdef DEBUG
  if (gNoisyIntrinsic) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": GetPrefISize\n");
  }
  AutoNoisyIndenter indenter(gNoisyIntrinsic);
#endif

  for (nsBlockFrame* curFrame = this; curFrame;
       curFrame = static_cast<nsBlockFrame*>(curFrame->GetNextContinuation())) {
    curFrame->LazyMarkLinesDirty();
  }

  if (GetStateBits() & NS_BLOCK_NEEDS_BIDI_RESOLUTION)
    ResolveBidi();
  InlinePrefISizeData data;
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
                        line->mFirstChild, nsLayoutUtils::PREF_ISIZE);
        data.ForceBreak(aRenderingContext);
      } else {
        if (!curFrame->GetPrevContinuation() &&
            line == curFrame->begin_lines()) {
          
          
          
          const nsStyleCoord &indent = StyleText()->mTextIndent;
          if (indent.ConvertsToLength())
            data.currentLine += nsRuleNode::ComputeCoordPercentCalc(indent, 0);
        }
        

        data.line = &line;
        data.lineContainer = curFrame;
        nsIFrame *kid = line->mFirstChild;
        for (int32_t i = 0, i_end = line->GetChildCount(); i != i_end;
             ++i, kid = kid->GetNextSibling()) {
          kid->AddInlinePrefISize(aRenderingContext, &data);
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
  
  if (StyleContext()->HasTextDecorationLines()) {
    return GetVisualOverflowRect();
  }
  return ComputeSimpleTightBounds(aContext);
}

 nsresult
nsBlockFrame::GetPrefWidthTightBounds(nsRenderingContext* aRenderingContext,
                                      nscoord* aX,
                                      nscoord* aXMost)
{
  nsIFrame* firstInFlow = FirstContinuation();
  if (firstInFlow != this) {
    return firstInFlow->GetPrefWidthTightBounds(aRenderingContext, aX, aXMost);
  }

  *aX = 0;
  *aXMost = 0;

  nsresult rv;
  InlinePrefISizeData data;
  for (nsBlockFrame* curFrame = this; curFrame;
       curFrame = static_cast<nsBlockFrame*>(curFrame->GetNextContinuation())) {
    for (line_iterator line = curFrame->begin_lines(), line_end = curFrame->end_lines();
         line != line_end; ++line)
    {
      nscoord childX, childXMost;
      if (line->IsBlock()) {
        data.ForceBreak(aRenderingContext);
        rv = line->mFirstChild->GetPrefWidthTightBounds(aRenderingContext,
                                                        &childX, &childXMost);
        NS_ENSURE_SUCCESS(rv, rv);
        *aX = std::min(*aX, childX);
        *aXMost = std::max(*aXMost, childXMost);
      } else {
        if (!curFrame->GetPrevContinuation() &&
            line == curFrame->begin_lines()) {
          
          
          
          const nsStyleCoord &indent = StyleText()->mTextIndent;
          if (indent.ConvertsToLength()) {
            data.currentLine += nsRuleNode::ComputeCoordPercentCalc(indent, 0);
          }
        }
        

        data.line = &line;
        data.lineContainer = curFrame;
        nsIFrame *kid = line->mFirstChild;
        for (int32_t i = 0, i_end = line->GetChildCount(); i != i_end;
             ++i, kid = kid->GetNextSibling()) {
          rv = kid->GetPrefWidthTightBounds(aRenderingContext, &childX,
                                            &childXMost);
          NS_ENSURE_SUCCESS(rv, rv);
          *aX = std::min(*aX, data.currentLine + childX);
          *aXMost = std::max(*aXMost, data.currentLine + childXMost);
          kid->AddInlinePrefISize(aRenderingContext, &data);
        }
      }
    }
  }
  data.ForceBreak(aRenderingContext);

  return NS_OK;
}

static bool
AvailableSpaceShrunk(WritingMode aWM,
                     const LogicalRect& aOldAvailableSpace,
                     const LogicalRect& aNewAvailableSpace)
{
  if (aNewAvailableSpace.ISize(aWM) == 0) {
    
    return aOldAvailableSpace.ISize(aWM) != 0;
  }
  NS_ASSERTION(aOldAvailableSpace.IStart(aWM) <=
               aNewAvailableSpace.IStart(aWM) &&
               aOldAvailableSpace.IEnd(aWM) >=
               aNewAvailableSpace.IEnd(aWM),
               "available space should never grow");
  return aOldAvailableSpace.ISize(aWM) != aNewAvailableSpace.ISize(aWM);
}

static LogicalSize
CalculateContainingBlockSizeForAbsolutes(WritingMode aWM,
                                         const nsHTMLReflowState& aReflowState,
                                         LogicalSize aFrameSize)
{
  
  
  
  
  nsIFrame* frame = aReflowState.frame;

  LogicalSize cbSize(aFrameSize);
    
  const LogicalMargin& border =
    LogicalMargin(aWM, aReflowState.ComputedPhysicalBorderPadding() -
                       aReflowState.ComputedPhysicalPadding());
  cbSize.ISize(aWM) -= border.IStartEnd(aWM);
  cbSize.BSize(aWM) -= border.BStartEnd(aWM);

  if (frame->GetParent()->GetContent() == frame->GetContent() &&
      frame->GetParent()->GetType() != nsGkAtoms::canvasFrame) {
    
    
    
    
    
    
    
    

    
    
    
    const nsHTMLReflowState* aLastRS = &aReflowState;
    const nsHTMLReflowState* lastButOneRS = &aReflowState;
    while (aLastRS->parentReflowState &&
           aLastRS->parentReflowState->frame->GetContent() == frame->GetContent() &&
           aLastRS->parentReflowState->frame->GetType() != nsGkAtoms::fieldSetFrame) {
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
      
      
      WritingMode lastWM = aLastRS->GetWritingMode();
      LogicalSize lastRSSize =
        LogicalSize(lastWM,
                    aLastRS->ComputedISize(),
                    aLastRS->ComputedBSize()).ConvertTo(aWM, lastWM);
      LogicalMargin lastRSPadding =
        aLastRS->ComputedLogicalPadding().ConvertTo(aWM, lastWM);
      LogicalMargin logicalScrollbars(aWM, scrollbars);
      if (lastRSSize.ISize(aWM) != NS_UNCONSTRAINEDSIZE) {
        cbSize.ISize(aWM) = std::max(0, lastRSSize.ISize(aWM) +
                                        lastRSPadding.IStartEnd(aWM) -
                                        logicalScrollbars.IStartEnd(aWM));
      }
      if (lastRSSize.BSize(aWM) != NS_UNCONSTRAINEDSIZE) {
        cbSize.BSize(aWM) = std::max(0, lastRSSize.BSize(aWM) +
                                        lastRSPadding.BStartEnd(aWM) -
                                        logicalScrollbars.BStartEnd(aWM));
      }
    }
  }

  return cbSize;
}

void
nsBlockFrame::Reflow(nsPresContext*           aPresContext,
                     nsHTMLReflowMetrics&     aMetrics,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsBlockFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);
#ifdef DEBUG
  if (gNoisyReflow) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": begin reflow availSize=%d,%d computedSize=%d,%d\n",
           aReflowState.AvailableISize(), aReflowState.AvailableBSize(),
           aReflowState.ComputedISize(), aReflowState.ComputedBSize());
  }
  AutoNoisyIndenter indent(gNoisy);
  PRTime start = 0; 
  int32_t ctc = 0;        
  if (gLameReflowMetrics) {
    start = PR_Now();
    ctc = nsLineBox::GetCtorCount();
  }
#endif

  const nsHTMLReflowState *reflowState = &aReflowState;
  WritingMode wm = aReflowState.GetWritingMode();
  nscoord consumedBSize = GetConsumedBSize();
  nscoord effectiveComputedBSize = GetEffectiveComputedBSize(aReflowState,
                                                             consumedBSize);
  Maybe<nsHTMLReflowState> mutableReflowState;
  
  
  if (aReflowState.AvailableBSize() != NS_UNCONSTRAINEDSIZE &&
      aReflowState.ComputedBSize() != NS_AUTOHEIGHT &&
      ShouldApplyOverflowClipping(this, aReflowState.mStyleDisplay)) {
    LogicalMargin blockDirExtras = aReflowState.ComputedLogicalBorderPadding();
    if (GetLogicalSkipSides().BStart()) {
      blockDirExtras.BStart(wm) = 0;
    } else {
      
      
      blockDirExtras.BStart(wm) +=
        aReflowState.ComputedLogicalMargin().BStart(wm);
    }

    if (effectiveComputedBSize + blockDirExtras.BStartEnd(wm) <=
        aReflowState.AvailableBSize()) {
      mutableReflowState.emplace(aReflowState);
      mutableReflowState->AvailableBSize() = NS_UNCONSTRAINEDSIZE;
      reflowState = mutableReflowState.ptr();
    }
  }

  
  
  nsSize oldSize = GetSize();

  
  nsAutoFloatManager autoFloatManager(const_cast<nsHTMLReflowState&>(*reflowState));

  
  
  
  
  bool needFloatManager = nsBlockFrame::BlockNeedsFloatManager(this);
  if (needFloatManager)
    autoFloatManager.CreateFloatManager(aPresContext);

  
  
  
  
  ClearLineCursor();

  if (IsFrameTreeTooDeep(*reflowState, aMetrics, aStatus)) {
    return;
  }

  bool blockStartMarginRoot, blockEndMarginRoot;
  IsMarginRoot(&blockStartMarginRoot, &blockEndMarginRoot);

  
  
  nsBlockReflowState state(*reflowState, aPresContext, this,
                           blockStartMarginRoot, blockEndMarginRoot,
                           needFloatManager, consumedBSize);

  if (GetStateBits() & NS_BLOCK_NEEDS_BIDI_RESOLUTION)
    static_cast<nsBlockFrame*>(FirstContinuation())->ResolveBidi();

  if (RenumberLists(aPresContext)) {
    AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
  }

#ifdef DEBUG
  
  
  
  
  
  nsLayoutUtils::AssertNoDuplicateContinuations(this, mFloats);
#endif

  
  
  
  DrainOverflowLines();

  
  nsOverflowAreas ocBounds;
  nsReflowStatus ocStatus = NS_FRAME_COMPLETE;
  if (GetPrevInFlow()) {
    ReflowOverflowContainerChildren(aPresContext, *reflowState, ocBounds, 0,
                                    ocStatus);
  }

  
  
  nsOverflowContinuationTracker tracker(this, false);
  state.mOverflowTracker = &tracker;

  
  DrainPushedFloats();
  nsOverflowAreas fcBounds;
  nsReflowStatus fcStatus = NS_FRAME_COMPLETE;
  ReflowPushedFloats(state, fcBounds, fcStatus);

  
  
  
  if (!(GetStateBits() & NS_FRAME_IS_DIRTY) && reflowState->IsIResize()) {
    PrepareResizeReflow(state);
  }

  
  
  if (!(GetStateBits() & NS_FRAME_IS_DIRTY) &&
      reflowState->mCBReflowState &&
      reflowState->mCBReflowState->IsIResize() &&
      reflowState->mStyleText->mTextIndent.HasPercent() &&
      !mLines.empty()) {
    mLines.front()->MarkDirty();
  }

  LazyMarkLinesDirty();

  mState &= ~NS_FRAME_FIRST_REFLOW;

  
  ReflowDirtyLines(state);

  
  
  
  

  
  
  
  
  
  
  if (NS_FRAME_IS_FULLY_COMPLETE(state.mReflowStatus)) {
    nsBlockFrame* nif = static_cast<nsBlockFrame*>(GetNextInFlow());
    while (nif) {
      if (nif->HasPushedFloatsFromPrevContinuation()) {
        bool oc = nif->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER;
        NS_MergeReflowStatusInto(&state.mReflowStatus,
            oc ? NS_FRAME_OVERFLOW_INCOMPLETE : NS_FRAME_NOT_COMPLETE);
        break;
      }

      nif = static_cast<nsBlockFrame*>(nif->GetNextInFlow());
    }
  }

  NS_MergeReflowStatusInto(&state.mReflowStatus, ocStatus);
  NS_MergeReflowStatusInto(&state.mReflowStatus, fcStatus);

  
  
  if (NS_UNCONSTRAINEDSIZE != reflowState->AvailableBSize() &&
      NS_FRAME_IS_COMPLETE(state.mReflowStatus) &&
      state.mFloatManager->ClearContinues(FindTrailingClear())) {
    NS_FRAME_SET_INCOMPLETE(state.mReflowStatus);
  }

  if (!NS_FRAME_IS_FULLY_COMPLETE(state.mReflowStatus)) {
    if (HasOverflowLines() || HasPushedFloats()) {
      state.mReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
    }

#ifdef DEBUG_kipp
    ListTag(stdout); printf(": block is not fully complete\n");
#endif
  }

  
  
  
  
  
  
  
  
  
  
  
  
  if (HasOutsideBullet() && !mLines.empty() &&
      (mLines.front()->IsBlock() ||
       (0 == mLines.front()->BSize() &&
        mLines.front() != mLines.back() &&
        mLines.begin().next()->IsBlock()))) {
    
    nsHTMLReflowMetrics metrics(aReflowState);
    
    nsLayoutUtils::LinePosition position;
    WritingMode wm = aReflowState.GetWritingMode();
    bool havePosition = nsLayoutUtils::GetFirstLinePosition(wm, this,
                                                            &position);
    nscoord lineBStart = havePosition ?
      position.mBStart :
      reflowState->ComputedLogicalBorderPadding().BStart(wm);
    nsIFrame* bullet = GetOutsideBullet();
    ReflowBullet(bullet, state, metrics, lineBStart);
    NS_ASSERTION(!BulletIsEmpty() || metrics.BSize(wm) == 0,
                 "empty bullet took up space");

    if (havePosition && !BulletIsEmpty()) {
      

      
      
    
      
      LogicalRect bbox = bullet->GetLogicalRect(wm, metrics.Width());
      bbox.BStart(wm) = position.mBaseline - metrics.BlockStartAscent();
      bullet->SetRect(wm, bbox, metrics.Width());
    }
    
    
  }

  CheckFloats(state);

  
  nscoord blockEndEdgeOfChildren;
  ComputeFinalSize(*reflowState, state, aMetrics, &blockEndEdgeOfChildren);

  
  
  
  
  
  
  
  
  
  
  
  
  
  if (wm.IsVerticalRL()) {
    nscoord containerWidth = aMetrics.Width();
    nscoord deltaX = containerWidth - state.ContainerWidth();
    if (deltaX) {
      for (line_iterator line = begin_lines(), end = end_lines();
           line != end; line++) {
        UpdateLineContainerWidth(line, containerWidth);
      }
      for (nsIFrame* f = mFloats.FirstChild(); f; f = f->GetNextSibling()) {
        nsPoint physicalDelta(deltaX, 0);
        f->MovePositionBy(physicalDelta);
      }
      nsFrameList* bulletList = GetOutsideBulletList();
      if (bulletList) {
        nsPoint physicalDelta(deltaX, 0);
        for (nsIFrame* f = bulletList->FirstChild(); f;
             f = f->GetNextSibling()) {
          f->MovePositionBy(physicalDelta);
        }
      }
    }
  }

  nsRect areaBounds = nsRect(0, 0, aMetrics.Width(), aMetrics.Height());
  ComputeOverflowAreas(areaBounds, reflowState->mStyleDisplay,
                       blockEndEdgeOfChildren, aMetrics.mOverflowAreas);
  
  aMetrics.mOverflowAreas.UnionWith(ocBounds);
  
  aMetrics.mOverflowAreas.UnionWith(fcBounds);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  WritingMode parentWM = aMetrics.GetWritingMode();
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
      LogicalSize containingBlockSize =
        CalculateContainingBlockSizeForAbsolutes(parentWM, *reflowState,
                                                 aMetrics.Size(parentWM));

      
      
      
      
      
      

      
      
      bool cbWidthChanged = aMetrics.Width() != oldSize.width;
      bool isRoot = !GetContent()->GetParent();
      
      
      
      
      bool cbHeightChanged =
        !(isRoot && NS_UNCONSTRAINEDSIZE == reflowState->ComputedHeight()) &&
        aMetrics.Height() != oldSize.height;

      nsRect containingBlock(nsPoint(0, 0),
                             containingBlockSize.GetPhysicalSize(parentWM));
      absoluteContainer->Reflow(this, aPresContext, *reflowState,
                                state.mReflowStatus,
                                containingBlock, true,
                                cbWidthChanged, cbHeightChanged,
                                &aMetrics.mOverflowAreas);
    }
  }

  FinishAndStoreOverflow(&aMetrics);

  
  
  
  if (needFloatManager)
    state.mFloatManager = nullptr;

  aStatus = state.mReflowStatus;

#ifdef DEBUG
  
  
  
  
  
  nsLayoutUtils::AssertNoDuplicateContinuations(this, mFloats);

  if (gNoisyReflow) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": status=%x (%scomplete) metrics=%d,%d carriedMargin=%d",
           aStatus, NS_FRAME_IS_COMPLETE(aStatus) ? "" : "not ",
           aMetrics.ISize(parentWM), aMetrics.BSize(parentWM),
           aMetrics.mCarriedOutBEndMargin.get());
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

    int32_t ectc = nsLineBox::GetCtorCount();
    int32_t numLines = mLines.size();
    if (!numLines) numLines = 1;
    PRTime delta, perLineDelta, lines;
    lines = int64_t(numLines);
    delta = end - start;
    perLineDelta = delta / lines;

    ListTag(stdout);
    char buf[400];
    PR_snprintf(buf, sizeof(buf),
                ": %lld elapsed (%lld per line) (%d lines; %d new lines)",
                delta, perLineDelta, numLines, ectc - ctc);
    printf("%s\n", buf);
  }
#endif

  NS_FRAME_SET_TRUNCATION(aStatus, (*reflowState), aMetrics);
}

bool
nsBlockFrame::CheckForCollapsedBEndMarginFromClearanceLine()
{
  line_iterator begin = begin_lines();
  line_iterator line = end_lines();

  while (true) {
    if (begin == line) {
      return false;
    }
    --line;
    if (line->BSize() != 0 || !line->CachedIsEmpty()) {
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
                               nscoord*                 aBEndEdgeOfChildren)
{
  WritingMode wm = aState.mReflowState.GetWritingMode();
  const LogicalMargin& borderPadding = aState.BorderPadding();
#ifdef NOISY_FINAL_SIZE
  ListTag(stdout);
  printf(": mBCoord=%d mIsBEndMarginRoot=%s mPrevBEndMargin=%d bp=%d,%d\n",
         aState.mBCoord, aState.GetFlag(BRS_ISBENDMARGINROOT) ? "yes" : "no",
         aState.mPrevBEndMargin,
         borderPadding.BStart(wm), borderPadding.BEnd(wm));
#endif

  
  LogicalSize finalSize(wm);
  finalSize.ISize(wm) =
    NSCoordSaturatingAdd(NSCoordSaturatingAdd(borderPadding.IStart(wm),
                                              aReflowState.ComputedISize()),
                         borderPadding.IEnd(wm));

  
  
  
  
  
  nscoord nonCarriedOutBDirMargin = 0;
  if (!aState.GetFlag(BRS_ISBENDMARGINROOT)) {
    
    
    
    
    
    if (CheckForCollapsedBEndMarginFromClearanceLine()) {
      
      
      nonCarriedOutBDirMargin = aState.mPrevBEndMargin.get();
      aState.mPrevBEndMargin.Zero();
    }
    aMetrics.mCarriedOutBEndMargin = aState.mPrevBEndMargin;
  } else {
    aMetrics.mCarriedOutBEndMargin.Zero();
  }

  nscoord blockEndEdgeOfChildren = aState.mBCoord + nonCarriedOutBDirMargin;
  
  if (aState.GetFlag(BRS_ISBENDMARGINROOT) ||
      NS_UNCONSTRAINEDSIZE != aReflowState.ComputedBSize()) {
    
    
    
    
    
    
    if (blockEndEdgeOfChildren < aState.mReflowState.AvailableBSize())
    {
      
      blockEndEdgeOfChildren =
        std::min(blockEndEdgeOfChildren + aState.mPrevBEndMargin.get(),
               aState.mReflowState.AvailableBSize());
    }
  }
  if (aState.GetFlag(BRS_FLOAT_MGR)) {
    
    
    nscoord floatHeight =
      aState.ClearFloats(blockEndEdgeOfChildren, NS_STYLE_CLEAR_BOTH,
                         nullptr, nsFloatManager::DONT_CLEAR_PUSHED_FLOATS);
    blockEndEdgeOfChildren = std::max(blockEndEdgeOfChildren, floatHeight);
  }

  if (NS_UNCONSTRAINEDSIZE != aReflowState.ComputedBSize()
      && (GetParent()->GetType() != nsGkAtoms::columnSetFrame ||
          aReflowState.parentReflowState->AvailableBSize() == NS_UNCONSTRAINEDSIZE)) {
    ComputeFinalBSize(aReflowState, &aState.mReflowStatus,
                      aState.mBCoord + nonCarriedOutBDirMargin,
                      borderPadding, finalSize, aState.mConsumedBSize);
    if (!NS_FRAME_IS_COMPLETE(aState.mReflowStatus)) {
      
      
      
      
      finalSize.BSize(wm) = std::max(aReflowState.AvailableBSize(),
                                     aState.mBCoord + nonCarriedOutBDirMargin);
      
      nscoord effectiveComputedBSize =
        GetEffectiveComputedBSize(aReflowState, aState.GetConsumedBSize());
      finalSize.BSize(wm) =
        std::min(finalSize.BSize(wm),
                 borderPadding.BStart(wm) + effectiveComputedBSize);
      
      
      
      
    }

    
    aMetrics.mCarriedOutBEndMargin.Zero();
  }
  else if (NS_FRAME_IS_COMPLETE(aState.mReflowStatus)) {
    nscoord contentBSize = blockEndEdgeOfChildren - borderPadding.BStart(wm);
    nscoord autoBSize = aReflowState.ApplyMinMaxHeight(contentBSize);
    if (autoBSize != contentBSize) {
      
      
      aMetrics.mCarriedOutBEndMargin.Zero();
    }
    autoBSize += borderPadding.BStart(wm) + borderPadding.BEnd(wm);
    finalSize.BSize(wm) = autoBSize;
  }
  else {
    NS_ASSERTION(aReflowState.AvailableBSize() != NS_UNCONSTRAINEDSIZE,
      "Shouldn't be incomplete if availableBSize is UNCONSTRAINED.");
    finalSize.BSize(wm) = std::max(aState.mBCoord,
                                   aReflowState.AvailableBSize());
    if (aReflowState.AvailableBSize() == NS_UNCONSTRAINEDSIZE) {
      
      finalSize.BSize(wm) = aState.mBCoord;
    }
  }

  if (IS_TRUE_OVERFLOW_CONTAINER(this) &&
      NS_FRAME_IS_NOT_COMPLETE(aState.mReflowStatus)) {
    
    
    NS_ASSERTION(finalSize.BSize(wm) == 0,
                 "overflow containers must be zero-block-size");
    NS_FRAME_SET_OVERFLOW_INCOMPLETE(aState.mReflowStatus);
  }

  
  finalSize.BSize(wm) = std::max(0, finalSize.BSize(wm));
  *aBEndEdgeOfChildren = blockEndEdgeOfChildren;

  FrameProperties properties = Properties();
  if (blockEndEdgeOfChildren != finalSize.BSize(wm) - borderPadding.BEnd(wm)) {
    properties.Set(BlockEndEdgeOfChildrenProperty(),
                   NS_INT32_TO_PTR(blockEndEdgeOfChildren));
  } else {
    properties.Delete(BlockEndEdgeOfChildrenProperty());
  }

  aMetrics.SetSize(wm, finalSize);

#ifdef DEBUG_blocks
  if (CRAZY_SIZE(aMetrics.Width()) || CRAZY_SIZE(aMetrics.Height())) {
    ListTag(stdout);
    printf(": WARNING: desired:%d,%d\n", aMetrics.Width(), aMetrics.Height());
  }
#endif
}

static void
ConsiderBlockEndEdgeOfChildren(const WritingMode aWritingMode,
                               nscoord aBEndEdgeOfChildren,
                               nsOverflowAreas& aOverflowAreas)
{
  
  
  
  
  
  
  
  
  
  if (aWritingMode.IsVertical()) {
    if (aWritingMode.IsVerticalLR()) {
      NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
        nsRect& o = aOverflowAreas.Overflow(otype);
        o.width = std::max(o.XMost(), aBEndEdgeOfChildren) - o.x;
      }
    } else {
      NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
        nsRect& o = aOverflowAreas.Overflow(otype);
        nscoord xmost = o.XMost();
        o.x = std::min(o.x, xmost - aBEndEdgeOfChildren);
        o.width = xmost - o.x;
      }
    }
  } else {
    NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
      nsRect& o = aOverflowAreas.Overflow(otype);
      o.height = std::max(o.YMost(), aBEndEdgeOfChildren) - o.y;
    }
  }
}

void
nsBlockFrame::ComputeOverflowAreas(const nsRect&         aBounds,
                                   const nsStyleDisplay* aDisplay,
                                   nscoord               aBEndEdgeOfChildren,
                                   nsOverflowAreas&      aOverflowAreas)
{
  
  
  
  nsOverflowAreas areas(aBounds, aBounds);
  if (!ShouldApplyOverflowClipping(this, aDisplay)) {
    for (line_iterator line = begin_lines(), line_end = end_lines();
         line != line_end;
         ++line) {
      areas.UnionWith(line->GetOverflowAreas());
    }

    
    
    
    
    
    nsIFrame* outsideBullet = GetOutsideBullet();
    if (outsideBullet) {
      areas.UnionAllWith(outsideBullet->GetRect());
    }

    ConsiderBlockEndEdgeOfChildren(GetWritingMode(),
                                   aBEndEdgeOfChildren, areas);
  }

#ifdef NOISY_COMBINED_AREA
  ListTag(stdout);
  printf(": ca=%d,%d,%d,%d\n", area.x, area.y, area.width, area.height);
#endif

  aOverflowAreas = areas;
}

bool
nsBlockFrame::UpdateOverflow()
{
  nsRect rect(nsPoint(0, 0), GetSize());
  nsOverflowAreas overflowAreas(rect, rect);

  
  
  
  
  for (line_iterator line = begin_lines(), line_end = end_lines();
       line != line_end;
       ++line) {
    nsRect bounds = line->GetPhysicalBounds();
    nsOverflowAreas lineAreas(bounds, bounds);

    int32_t n = line->GetChildCount();
    for (nsIFrame* lineFrame = line->mFirstChild;
         n > 0; lineFrame = lineFrame->GetNextSibling(), --n) {
      ConsiderChildOverflow(lineAreas, lineFrame);
    }

    
    if (line->HasFloats()) {
      for (nsFloatCache* fc = line->GetFirstFloat(); fc; fc = fc->Next()) {
        ConsiderChildOverflow(lineAreas, fc->mFloat);
      }
    }

    line->SetOverflowAreas(lineAreas);
    overflowAreas.UnionWith(lineAreas);
  }

  
  
  ClearLineCursor();

  
  
  nsLayoutUtils::UnionChildOverflow(this, overflowAreas,
                                    kPrincipalList | kFloatList);

  bool found;
  nscoord blockEndEdgeOfChildren = NS_PTR_TO_INT32(
    Properties().Get(BlockEndEdgeOfChildrenProperty(), &found));
  if (found) {
    ConsiderBlockEndEdgeOfChildren(GetWritingMode(),
                                   blockEndEdgeOfChildren, overflowAreas);
  }

  return FinishAndStoreOverflow(overflowAreas, GetSize());
}

void
nsBlockFrame::LazyMarkLinesDirty()
{
  if (GetStateBits() & NS_BLOCK_LOOK_FOR_DIRTY_FRAMES) {
    for (line_iterator line = begin_lines(), line_end = end_lines();
         line != line_end; ++line) {
      int32_t n = line->GetChildCount();
      for (nsIFrame* lineFrame = line->mFirstChild;
           n > 0; lineFrame = lineFrame->GetNextSibling(), --n) {
        if (NS_SUBTREE_DIRTY(lineFrame)) {
          
          MarkLineDirty(line, &mLines);
          break;
        }
      }
    }
    RemoveStateBits(NS_BLOCK_LOOK_FOR_DIRTY_FRAMES);
  }
}

void
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

  
  
  
  if (aLine != aLineList->front() && aLine->IsInline() &&
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
}





static inline bool
IsAlignedLeft(uint8_t aAlignment,
              uint8_t aDirection,
              uint8_t aUnicodeBidi,
              nsIFrame* aFrame)
{
  return aFrame->IsSVGText() ||
         NS_STYLE_TEXT_ALIGN_LEFT == aAlignment ||
         (((NS_STYLE_TEXT_ALIGN_DEFAULT == aAlignment &&
           NS_STYLE_DIRECTION_LTR == aDirection) ||
          (NS_STYLE_TEXT_ALIGN_END == aAlignment &&
           NS_STYLE_DIRECTION_RTL == aDirection)) &&
         !(NS_STYLE_UNICODE_BIDI_PLAINTEXT & aUnicodeBidi));
}

void
nsBlockFrame::PrepareResizeReflow(nsBlockReflowState& aState)
{
  
  bool tryAndSkipLines =
    
    
    !StylePadding()->mPadding.GetLeft().HasPercent();

#ifdef DEBUG
  if (gDisableResizeOpt) {
    tryAndSkipLines = false;
  }
  if (gNoisyReflow) {
    if (!tryAndSkipLines) {
      IndentBy(stdout, gNoiseIndent);
      ListTag(stdout);
      printf(": marking all lines dirty: availISize=%d\n",
             aState.mReflowState.AvailableISize());
    }
  }
#endif

  if (tryAndSkipLines) {
    WritingMode wm = aState.mReflowState.GetWritingMode();
    nscoord newAvailISize =
      aState.mReflowState.ComputedLogicalBorderPadding().IStart(wm) +
      aState.mReflowState.ComputedISize();
    NS_ASSERTION(NS_UNCONSTRAINEDSIZE != aState.mReflowState.ComputedLogicalBorderPadding().IStart(wm) &&
                 NS_UNCONSTRAINEDSIZE != aState.mReflowState.ComputedISize(),
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
      
      
      bool isLastLine = line == mLines.back() && !GetNextInFlow();
      if (line->IsBlock() ||
          line->HasFloats() ||
          (!isLastLine && !line->HasBreakAfter()) ||
          ((isLastLine || !line->IsLineWrapped())) ||
          line->ResizeReflowOptimizationDisabled() ||
          line->IsImpactedByFloat() ||
          (line->IEnd() > newAvailISize)) {
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
           static_cast<void*>((line.next() != end_lines() ? line.next().get() : nullptr)),
           line->IsBlock() ? "block" : "inline",
           line->HasBreakAfter() ? "has-break-after " : "",
           line->HasFloats() ? "has-floats " : "",
           line->IsImpactedByFloat() ? "impacted " : "",
           line->GetBreakTypeBefore(), line->GetBreakTypeAfter(),
           line->IEnd());
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
}














void
nsBlockFrame::PropagateFloatDamage(nsBlockReflowState& aState,
                                   nsLineBox* aLine,
                                   nscoord aDeltaBCoord)
{
  nsFloatManager *floatManager = aState.mReflowState.mFloatManager;
  NS_ASSERTION((aState.mReflowState.parentReflowState &&
                aState.mReflowState.parentReflowState->mFloatManager == floatManager) ||
                aState.mReflowState.mBlockDelta == 0, "Bad block delta passed in");

  
  
  if (!floatManager->HasAnyFloats())
    return;

  
  if (floatManager->HasFloatDamage()) {
    
    
    nscoord lineBCoordBefore = aLine->BStart() + aDeltaBCoord;
    nscoord lineBCoordAfter = lineBCoordBefore + aLine->BSize();
    
    
    WritingMode wm = aState.mReflowState.GetWritingMode();
    nscoord containerWidth = aState.ContainerWidth();
    LogicalRect overflow = aLine->GetOverflowArea(eScrollableOverflow, wm,
                                                  containerWidth);
    nscoord lineBCoordCombinedBefore = overflow.BStart(wm) + aDeltaBCoord;
    nscoord lineBCoordCombinedAfter = lineBCoordCombinedBefore +
                                      overflow.BSize(wm);

    bool isDirty = floatManager->IntersectsDamage(wm, lineBCoordBefore,
                                                  lineBCoordAfter) ||
                   floatManager->IntersectsDamage(wm, lineBCoordCombinedBefore,
                                                  lineBCoordCombinedAfter);
    if (isDirty) {
      aLine->MarkDirty();
      return;
    }
  }

  
  if (aDeltaBCoord + aState.mReflowState.mBlockDelta != 0) {
    if (aLine->IsBlock()) {
      
      
      
      
      aLine->MarkDirty();
    } else {
      bool wasImpactedByFloat = aLine->IsImpactedByFloat();
      nsFlowAreaRect floatAvailableSpace =
        aState.GetFloatAvailableSpaceForBSize(aLine->BStart() + aDeltaBCoord,
                                              aLine->BSize(),
                                              nullptr);

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

static bool LineHasClear(nsLineBox* aLine) {
  return aLine->IsBlock()
    ? (aLine->GetBreakTypeBefore() ||
       (aLine->mFirstChild->GetStateBits() & NS_BLOCK_HAS_CLEAR_CHILDREN) ||
       !nsBlockFrame::BlockCanIntersectFloats(aLine->mFirstChild))
    : aLine->HasFloatBreakAfter();
}







void
nsBlockFrame::ReparentFloats(nsIFrame* aFirstFrame, nsBlockFrame* aOldParent,
                             bool aReparentSiblings) {
  nsFrameList list;
  aOldParent->CollectFloats(aFirstFrame, list, aReparentSiblings);
  if (list.NotEmpty()) {
    for (nsIFrame* f = list.FirstChild(); f; f = f->GetNextSibling()) {
      ReparentFrame(f, aOldParent, this);
    }
    mFloats.AppendFrames(nullptr, list);
  }
}

static void DumpLine(const nsBlockReflowState& aState, nsLineBox* aLine,
                     nscoord aDeltaBCoord, int32_t aDeltaIndent) {
#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsRect ovis(aLine->GetVisualOverflowArea());
    nsRect oscr(aLine->GetScrollableOverflowArea());
    nsBlockFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent + aDeltaIndent);
    printf("line=%p mBCoord=%d dirty=%s oldBounds={%d,%d,%d,%d} oldoverflow-vis={%d,%d,%d,%d} oldoverflow-scr={%d,%d,%d,%d} deltaBCoord=%d mPrevBEndMargin=%d childCount=%d\n",
           static_cast<void*>(aLine), aState.mBCoord,
           aLine->IsDirty() ? "yes" : "no",
           aLine->IStart(), aLine->BStart(),
           aLine->ISize(), aLine->BSize(),
           ovis.x, ovis.y, ovis.width, ovis.height,
           oscr.x, oscr.y, oscr.width, oscr.height,
           aDeltaBCoord, aState.mPrevBEndMargin.get(), aLine->GetChildCount());
  }
#endif
}

void
nsBlockFrame::ReflowDirtyLines(nsBlockReflowState& aState)
{
  bool keepGoing = true;
  bool repositionViews = false; 
  bool foundAnyClears = aState.mFloatBreakType != NS_STYLE_CLEAR_NONE;
  bool willReflowAgain = false;

#ifdef DEBUG
  if (gNoisyReflow) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": reflowing dirty lines");
    printf(" computedISize=%d\n", aState.mReflowState.ComputedISize());
  }
  AutoNoisyIndenter indent(gNoisyReflow);
#endif

  bool selfDirty = (GetStateBits() & NS_FRAME_IS_DIRTY) ||
                     (aState.mReflowState.IsBResize() &&
                      (GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT));

  
  
  if (aState.mReflowState.AvailableBSize() != NS_UNCONSTRAINEDSIZE
      && GetNextInFlow() && aState.mReflowState.AvailableBSize() >
        GetLogicalSize().BSize(aState.mReflowState.GetWritingMode())) {
    line_iterator lastLine = end_lines();
    if (lastLine != begin_lines()) {
      --lastLine;
      lastLine->MarkDirty();
    }
  }
    
    
  nscoord deltaBCoord = 0;

    
    
    
  bool needToRecoverState = false;
    
  bool reflowedFloat = mFloats.NotEmpty() &&
    (mFloats.FirstChild()->GetStateBits() & NS_FRAME_IS_PUSHED_FLOAT);
  bool lastLineMovedUp = false;
  
  uint8_t inlineFloatBreakType = aState.mFloatBreakType;

  line_iterator line = begin_lines(), line_end = end_lines();

  
  for ( ; line != line_end; ++line, aState.AdvanceToNextLine()) {
    DumpLine(aState, line, deltaBCoord, 0);
#ifdef DEBUG
    AutoNoisyIndenter indent2(gNoisyReflow);
#endif

    if (selfDirty)
      line->MarkDirty();

    
    
    
    if (!line->IsDirty() && line->IsBlock() &&
        (line->mFirstChild->GetStateBits() & NS_BLOCK_HAS_CLEAR_CHILDREN)) {
      line->MarkDirty();
    }

    nsIFrame *replacedBlock = nullptr;
    if (line->IsBlock() &&
        !nsBlockFrame::BlockCanIntersectFloats(line->mFirstChild)) {
      replacedBlock = line->mFirstChild;
    }

    
    
    if (!line->IsDirty() &&
        (line->GetBreakTypeBefore() != NS_STYLE_CLEAR_NONE ||
         replacedBlock)) {
      nscoord curBCoord = aState.mBCoord;
      
      
      if (inlineFloatBreakType != NS_STYLE_CLEAR_NONE) {
        curBCoord = aState.ClearFloats(curBCoord, inlineFloatBreakType);
      }

      nscoord newBCoord =
        aState.ClearFloats(curBCoord, line->GetBreakTypeBefore(), replacedBlock);

      if (line->HasClearance()) {
        
        if (newBCoord == curBCoord
            
            
            
            
            || newBCoord != line->BStart() + deltaBCoord) {
          line->MarkDirty();
        }
      } else {
        
        if (curBCoord != newBCoord) {
          line->MarkDirty();
        }
      }
    }

    
    if (inlineFloatBreakType != NS_STYLE_CLEAR_NONE) {
      aState.mBCoord = aState.ClearFloats(aState.mBCoord, inlineFloatBreakType);
      if (aState.mBCoord != line->BStart() + deltaBCoord) {
        
        
        line->MarkDirty();
      }
      inlineFloatBreakType = NS_STYLE_CLEAR_NONE;
    }

    bool previousMarginWasDirty = line->IsPreviousMarginDirty();
    if (previousMarginWasDirty) {
      
      line->MarkDirty();
      line->ClearPreviousMarginDirty();
    } else if (line->BEnd() + deltaBCoord > aState.mBEndEdge) {
      
      
      line->MarkDirty();
    }

    
    
    
    
    
    
    
    if (!line->IsDirty() &&
        aState.mReflowState.AvailableBSize() != NS_UNCONSTRAINEDSIZE &&
        (deltaBCoord != 0 || aState.mReflowState.IsBResize() ||
         aState.mReflowState.mFlags.mMustReflowPlaceholders) &&
        (line->IsBlock() || line->HasFloats() || line->HadFloatPushed())) {
      line->MarkDirty();
    }

    if (!line->IsDirty()) {
      
      
      PropagateFloatDamage(aState, line, deltaBCoord);
    }

    
    
    
    if (aState.ContainerWidth() != line->mContainerWidth) {
      line->mContainerWidth = aState.ContainerWidth();

      bool isLastLine = line == mLines.back() &&
                        !GetNextInFlow() &&
                        NS_STYLE_TEXT_ALIGN_AUTO == StyleText()->mTextAlignLast;
      uint8_t align = isLastLine ?
        StyleText()->mTextAlign : StyleText()->mTextAlignLast;

      if (line->mWritingMode.IsVertical() ||
          !line->mWritingMode.IsBidiLTR() ||
          !IsAlignedLeft(align,
                         aState.mReflowState.mStyleVisibility->mDirection,
                         StyleTextReset()->mUnicodeBidi, this)) {
        line->MarkDirty();
      }
    }

    if (needToRecoverState && line->IsDirty()) {
      
      
      
      aState.ReconstructMarginBefore(line);
    }

    bool reflowedPrevLine = !needToRecoverState;
    if (needToRecoverState) {
      needToRecoverState = false;

      
      
      if (line->IsDirty()) {
        NS_ASSERTION(line->mFirstChild->GetPrevSibling() ==
                     line.prev()->LastChild(), "unexpected line frames");
        aState.mPrevChild = line->mFirstChild->GetPrevSibling();
      }
    }

    
    
    
    
    
    
    
    
    
    if (line->IsDirty() && (line->HasFloats() || !willReflowAgain)) {
      lastLineMovedUp = true;

      bool maybeReflowingForFirstTime =
        line->IStart() == 0 && line->BStart() == 0 &&
        line->ISize() == 0 && line->BSize() == 0;

      
      
      
      nscoord oldB = line->BStart();
      nscoord oldBMost = line->BEnd();

      NS_ASSERTION(!willReflowAgain || !line->IsBlock(),
                   "Don't reflow blocks while willReflowAgain is true, reflow of block abs-pos children depends on this");

      
      
      ReflowLine(aState, line, &keepGoing);

      if (aState.mReflowState.WillReflowAgainForClearance()) {
        line->MarkDirty();
        willReflowAgain = true;
        
        
        
      }

      if (line->HasFloats()) {
        reflowedFloat = true;
      }

      if (!keepGoing) {
        DumpLine(aState, line, deltaBCoord, -1);
        if (0 == line->GetChildCount()) {
          DeleteLine(aState, line, line_end);
        }
        break;
      }

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      if (line.next() != end_lines()) {
        bool maybeWasEmpty = oldB == line.next()->BStart();
        bool isEmpty = line->CachedIsEmpty();
        if (maybeReflowingForFirstTime  ||
            (isEmpty || maybeWasEmpty) ) {
          line.next()->MarkPreviousMarginDirty();
          
        }
      }

      
      
      
      
      
      
      deltaBCoord = line->BEnd() - oldBMost;

      
      
      
      aState.mPresContext->CheckForInterrupt(this);
    } else {
      aState.mOverflowTracker->Skip(line->mFirstChild, aState.mReflowStatus);
        
        
        

      lastLineMovedUp = deltaBCoord < 0;

      if (deltaBCoord != 0)
        SlideLine(aState, line, deltaBCoord);
      else
        repositionViews = true;

      NS_ASSERTION(!line->IsDirty() || !line->HasFloats(),
                   "Possibly stale float cache here!");
      if (willReflowAgain && line->IsBlock()) {
        
        
        
        
        
        
        
        
        
        
      } else {
        
        aState.RecoverStateFrom(line, deltaBCoord);
      }

      
      
      
      
      
      if (line->IsBlock() || !line->CachedIsEmpty()) {
        aState.mBCoord = line->BEnd();
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

    DumpLine(aState, line, deltaBCoord, -1);

    if (aState.mPresContext->HasPendingInterrupt()) {
      willReflowAgain = true;
      
      
      
      
      
      
      
      MarkLineDirtyForInterrupt(line);
    }
  }

  
  if (inlineFloatBreakType != NS_STYLE_CLEAR_NONE) {
    aState.mBCoord = aState.ClearFloats(aState.mBCoord, inlineFloatBreakType);
  }

  if (needToRecoverState) {
    
    aState.ReconstructMarginBefore(line);

    
    
    NS_ASSERTION(line == line_end || line->mFirstChild->GetPrevSibling() ==
                 line.prev()->LastChild(), "unexpected line frames");
    aState.mPrevChild =
      line == line_end ? mFrames.LastChild() : line->mFirstChild->GetPrevSibling();
  }

  
  if (repositionViews)
    nsContainerFrame::PlaceFrameView(this);

  
  
  
  
  
  
  
  
  
  
  
  bool heightConstrained =
    aState.mReflowState.AvailableBSize() != NS_UNCONSTRAINEDSIZE;
  bool skipPull = willReflowAgain && heightConstrained;
  if (!skipPull && heightConstrained && aState.mNextInFlow &&
      (aState.mReflowState.mFlags.mNextInFlowUntouched &&
       !lastLineMovedUp && 
       !(GetStateBits() & NS_FRAME_IS_DIRTY) &&
       !reflowedFloat)) {
    
    
    
    
    
    line_iterator lineIter = this->end_lines();
    if (lineIter != this->begin_lines()) {
      lineIter--; 
      nsBlockInFlowLineIterator bifLineIter(this, lineIter);

      
      
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
    
    
    while (keepGoing && aState.mNextInFlow) {
      
      nsBlockFrame* nextInFlow = aState.mNextInFlow;
      nsLineBox* pulledLine;
      nsFrameList pulledFrames;
      if (!nextInFlow->mLines.empty()) {
        RemoveFirstLine(nextInFlow->mLines, nextInFlow->mFrames,
                        &pulledLine, &pulledFrames);
      } else {
        
        FrameLines* overflowLines = nextInFlow->GetOverflowLines();
        if (!overflowLines) {
          aState.mNextInFlow =
            static_cast<nsBlockFrame*>(nextInFlow->GetNextInFlow());
          continue;
        }
        bool last =
          RemoveFirstLine(overflowLines->mLines, overflowLines->mFrames,
                          &pulledLine, &pulledFrames);
        if (last) {
          nextInFlow->DestroyOverflowLines();
        }
      }

      if (pulledFrames.IsEmpty()) {
        
        NS_ASSERTION(pulledLine->GetChildCount() == 0 &&
                     !pulledLine->mFirstChild, "bad empty line");
        nextInFlow->FreeLineBox(pulledLine);
        continue;
      }

      if (pulledLine == nextInFlow->GetLineCursor()) {
        nextInFlow->ClearLineCursor();
      }
      ReparentFrames(pulledFrames, nextInFlow, this);

      NS_ASSERTION(pulledFrames.LastChild() == pulledLine->LastChild(),
                   "Unexpected last frame");
      NS_ASSERTION(aState.mPrevChild || mLines.empty(), "should have a prevchild here");
      NS_ASSERTION(aState.mPrevChild == mFrames.LastChild(),
                   "Incorrect aState.mPrevChild before inserting line at end");

      
      mFrames.AppendFrames(nullptr, pulledFrames);

      
      line = mLines.before_insert(end_lines(), pulledLine);
      aState.mPrevChild = mFrames.LastChild();

      
      ReparentFloats(pulledLine->mFirstChild, nextInFlow, true);

      DumpLine(aState, pulledLine, deltaBCoord, 0);
#ifdef DEBUG
      AutoNoisyIndenter indent2(gNoisyReflow);
#endif

      if (aState.mPresContext->HasPendingInterrupt()) {
        MarkLineDirtyForInterrupt(line);
      } else {
        
        
        
        
        while (line != end_lines()) {
          ReflowLine(aState, line, &keepGoing);

          if (aState.mReflowState.WillReflowAgainForClearance()) {
            line->MarkDirty();
            keepGoing = false;
            NS_FRAME_SET_INCOMPLETE(aState.mReflowStatus);
            break;
          }

          DumpLine(aState, line, deltaBCoord, -1);
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

  
  if (HasOutsideBullet() && mLines.empty()) {
    nsHTMLReflowMetrics metrics(aState.mReflowState);
    nsIFrame* bullet = GetOutsideBullet();
    WritingMode wm = aState.mReflowState.GetWritingMode();
    ReflowBullet(bullet, aState, metrics,
                 aState.mReflowState.ComputedPhysicalBorderPadding().top);
    NS_ASSERTION(!BulletIsEmpty() || metrics.BSize(wm) == 0,
                 "empty bullet took up space");

    if (!BulletIsEmpty()) {
      
      

      if (metrics.BlockStartAscent() == nsHTMLReflowMetrics::ASK_FOR_BASELINE) {
        nscoord ascent;
        WritingMode wm = aState.mReflowState.GetWritingMode();
        if (nsLayoutUtils::GetFirstLineBaseline(wm, bullet, &ascent)) {
          metrics.SetBlockStartAscent(ascent);
        } else {
          metrics.SetBlockStartAscent(metrics.BSize(wm));
        }
      }

      nsRefPtr<nsFontMetrics> fm;
      nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm),
        nsLayoutUtils::FontSizeInflationFor(this));

      nscoord minAscent =
        nsLayoutUtils::GetCenteredFontBaseline(fm, aState.mMinLineHeight,
                                               wm.IsLineInverted());
      nscoord minDescent = aState.mMinLineHeight - minAscent;

      aState.mBCoord += std::max(minAscent, metrics.BlockStartAscent()) +
                        std::max(minDescent, metrics.BSize(wm) -
                                             metrics.BlockStartAscent());

      nscoord offset = minAscent - metrics.BlockStartAscent();
      if (offset > 0) {
        bullet->SetRect(bullet->GetRect() + nsPoint(0, offset));
      }
    }
  }

  if (foundAnyClears) {
    AddStateBits(NS_BLOCK_HAS_CLEAR_CHILDREN);
  } else {
    RemoveStateBits(NS_BLOCK_HAS_CLEAR_CHILDREN);
  }

#ifdef DEBUG
  VerifyLines(true);
  VerifyOverflowSituation();
  if (gNoisyReflow) {
    IndentBy(stdout, gNoiseIndent - 1);
    ListTag(stdout);
    printf(": done reflowing dirty lines (status=%x)\n",
           aState.mReflowStatus);
  }
#endif
}

void
nsBlockFrame::MarkLineDirtyForInterrupt(nsLineBox* aLine)
{
  aLine->MarkDirty();

  
  
  
  
  if (GetStateBits() & NS_FRAME_IS_DIRTY) {
    
    
    int32_t n = aLine->GetChildCount();
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
    nsLineBox* line = aLine;
    aLine = mLines.erase(aLine);
    FreeLineBox(line);
    
    
    if (aLine != aLineEnd)
      aLine->MarkPreviousMarginDirty();
  }
}






void
nsBlockFrame::ReflowLine(nsBlockReflowState& aState,
                         line_iterator aLine,
                         bool* aKeepReflowGoing)
{
  MOZ_ASSERT(aLine->GetChildCount(), "reflowing empty line");

  
  aState.mCurrentLine = aLine;
  aLine->ClearDirty();
  aLine->InvalidateCachedIsEmpty();
  aLine->ClearHadFloatPushed();

  
  if (aLine->IsBlock()) {
    ReflowBlockFrame(aState, aLine, aKeepReflowGoing);
  } else {
    aLine->SetLineWrapped(false);
    ReflowInlineFrames(aState, aLine, aKeepReflowGoing);
  }
}

nsIFrame*
nsBlockFrame::PullFrame(nsBlockReflowState& aState,
                        line_iterator       aLine)
{
  
  if (end_lines() != aLine.next()) {
    return PullFrameFrom(aLine, this, aLine.next());
  }

  NS_ASSERTION(!GetOverflowLines(),
    "Our overflow lines should have been removed at the start of reflow");

  
  nsBlockFrame* nextInFlow = aState.mNextInFlow;
  while (nextInFlow) {
    if (nextInFlow->mLines.empty()) {
      nextInFlow->DrainSelfOverflowList();
    }
    if (!nextInFlow->mLines.empty()) {
      return PullFrameFrom(aLine, nextInFlow, nextInFlow->mLines.begin());
    }
    nextInFlow = static_cast<nsBlockFrame*>(nextInFlow->GetNextInFlow());
    aState.mNextInFlow = nextInFlow;
  }

  return nullptr;
}

nsIFrame*
nsBlockFrame::PullFrameFrom(nsLineBox*           aLine,
                            nsBlockFrame*        aFromContainer,
                            nsLineList::iterator aFromLine)
{
  nsLineBox* fromLine = aFromLine;
  MOZ_ASSERT(fromLine, "bad line to pull from");
  MOZ_ASSERT(fromLine->GetChildCount(), "empty line");
  MOZ_ASSERT(aLine->GetChildCount(), "empty line");

  NS_ASSERTION(fromLine->IsBlock() == fromLine->mFirstChild->IsBlockOutside(),
               "Disagreement about whether it's a block or not");

  if (fromLine->IsBlock()) {
    
    
    
    return nullptr;
  }
  
  nsIFrame* frame = fromLine->mFirstChild;
  nsIFrame* newFirstChild = frame->GetNextSibling();

  if (aFromContainer != this) {
    
    
    MOZ_ASSERT(aLine == mLines.back());
    MOZ_ASSERT(aFromLine == aFromContainer->mLines.begin(),
               "should only pull from first line");
    aFromContainer->mFrames.RemoveFrame(frame);

    
    
    ReparentFrame(frame, aFromContainer, this);
    mFrames.AppendFrame(nullptr, frame);

    
    
    ReparentFloats(frame, aFromContainer, false);
  } else {
    MOZ_ASSERT(aLine == aFromLine.prev());
  }

  aLine->NoteFrameAdded(frame);
  fromLine->NoteFrameRemoved(frame);

  if (fromLine->GetChildCount() > 0) {
    
    fromLine->MarkDirty();
    fromLine->mFirstChild = newFirstChild;
  } else {
    
    
    if (aFromLine.next() != aFromContainer->mLines.end()) {
      aFromLine.next()->MarkPreviousMarginDirty();
    }
    aFromContainer->mLines.erase(aFromLine);
    
    aFromContainer->FreeLineBox(fromLine);
  }

#ifdef DEBUG
  VerifyLines(true);
  VerifyOverflowSituation();
#endif

  return frame;
}

void
nsBlockFrame::SlideLine(nsBlockReflowState& aState,
                        nsLineBox* aLine, nscoord aDeltaBCoord)
{
  NS_PRECONDITION(aDeltaBCoord != 0, "why slide a line nowhere?");

  
  aLine->SlideBy(aDeltaBCoord, aState.ContainerWidth());

  
  MoveChildFramesOfLine(aLine, aDeltaBCoord);
}

void
nsBlockFrame::UpdateLineContainerWidth(nsLineBox* aLine,
                                       nscoord aNewContainerWidth)
{
  if (aNewContainerWidth == aLine->mContainerWidth) {
    return;
  }

  
  nscoord widthDelta = aLine->UpdateContainerWidth(aNewContainerWidth);

  
  if (GetWritingMode().IsVerticalRL()) {
    MoveChildFramesOfLine(aLine, widthDelta);
  }
}

void
nsBlockFrame::MoveChildFramesOfLine(nsLineBox* aLine, nscoord aDeltaBCoord)
{
  
  nsIFrame* kid = aLine->mFirstChild;
  if (!kid) {
    return;
  }

  WritingMode wm = GetWritingMode();
  LogicalPoint translation(wm, 0, aDeltaBCoord);

  if (aLine->IsBlock()) {
    if (aDeltaBCoord) {
      kid->MovePositionBy(wm, translation);
    }

    
    nsContainerFrame::PlaceFrameView(kid);
  }
  else {
    
    
    
    
    int32_t n = aLine->GetChildCount();
    while (--n >= 0) {
      if (aDeltaBCoord) {
        kid->MovePositionBy(wm, translation);
      }
      
      nsContainerFrame::PlaceFrameView(kid);
      kid = kid->GetNextSibling();
    }
  }
}

nsresult 
nsBlockFrame::AttributeChanged(int32_t         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               int32_t         aModType)
{
  nsresult rv = nsBlockFrameSuper::AttributeChanged(aNameSpaceID,
                                                    aAttribute, aModType);

  if (NS_FAILED(rv)) {
    return rv;
  }
  if (nsGkAtoms::start == aAttribute ||
      (nsGkAtoms::reversed == aAttribute &&
       mContent->IsHTMLElement(nsGkAtoms::ol))) {
    nsPresContext* presContext = PresContext();

    
    if (RenumberLists(presContext)) {
      presContext->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eStyleChange,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
    }
  }
  else if (nsGkAtoms::value == aAttribute) {
    const nsStyleDisplay* styleDisplay = StyleDisplay();
    if (NS_STYLE_DISPLAY_LIST_ITEM == styleDisplay->mDisplay) {
      
      
      
      
      nsBlockFrame* blockParent = nsLayoutUtils::FindNearestBlockAncestor(this);

      
      
      if (nullptr != blockParent) {
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
IsNonAutoNonZeroBSize(const nsStyleCoord& aCoord)
{
  nsStyleUnit unit = aCoord.GetUnit();
  if (unit == eStyleUnit_Auto ||
      
      
      
      
      
      
      
      
      
      unit == eStyleUnit_Enumerated) {
    return false;
  }
  if (aCoord.IsCoordPercentCalcUnit()) {
    
    
    
    
    return nsRuleNode::ComputeCoordPercentCalc(aCoord, nscoord_MAX) > 0 ||
           nsRuleNode::ComputeCoordPercentCalc(aCoord, 0) > 0;
  }
  MOZ_ASSERT(false, "unexpected unit for height or min-height");
  return true;
}

 bool
nsBlockFrame::IsSelfEmpty()
{
  
  
  
  if (GetStateBits() & NS_BLOCK_MARGIN_ROOT) {
    return false;
  }

  const nsStylePosition* position = StylePosition();
  bool vertical = GetWritingMode().IsVertical();

  if (vertical) {
    if (IsNonAutoNonZeroBSize(position->mMinWidth) ||
        IsNonAutoNonZeroBSize(position->mWidth)) {
      return false;
    }
  } else {
    if (IsNonAutoNonZeroBSize(position->mMinHeight) ||
        IsNonAutoNonZeroBSize(position->mHeight)) {
      return false;
    }
  }

  const nsStyleBorder* border = StyleBorder();
  const nsStylePadding* padding = StylePadding();

  if (vertical) {
    if (border->GetComputedBorderWidth(NS_SIDE_LEFT) != 0 ||
        border->GetComputedBorderWidth(NS_SIDE_RIGHT) != 0 ||
        !nsLayoutUtils::IsPaddingZero(padding->mPadding.GetLeft()) ||
        !nsLayoutUtils::IsPaddingZero(padding->mPadding.GetRight())) {
      return false;
    }
  } else {
    if (border->GetComputedBorderWidth(NS_SIDE_TOP) != 0 ||
        border->GetComputedBorderWidth(NS_SIDE_BOTTOM) != 0 ||
        !nsLayoutUtils::IsPaddingZero(padding->mPadding.GetTop()) ||
        !nsLayoutUtils::IsPaddingZero(padding->mPadding.GetBottom())) {
      return false;
    }
  }

  if (HasOutsideBullet() && !BulletIsEmpty()) {
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
nsBlockFrame::ShouldApplyBStartMargin(nsBlockReflowState& aState,
                                      nsLineBox* aLine,
                                      nsIFrame* aChildFrame)
{
  if (aState.GetFlag(BRS_APPLYBSTARTMARGIN)) {
    
    return true;
  }

  if (!aState.IsAdjacentWithTop() ||
      aChildFrame->StyleBorder()->mBoxDecorationBreak ==
        NS_STYLE_BOX_DECORATION_BREAK_CLONE) {
    
    
    
    aState.SetFlag(BRS_APPLYBSTARTMARGIN, true);
    return true;
  }

  
  line_iterator line = begin_lines();
  if (aState.GetFlag(BRS_HAVELINEADJACENTTOTOP)) {
    line = aState.mLineAdjacentToTop;
  }
  while (line != aLine) {
    if (!line->CachedIsEmpty() || line->HasClearance()) {
      
      
      aState.SetFlag(BRS_APPLYBSTARTMARGIN, true);
      return true;
    }
    
    
    ++line;
    aState.SetFlag(BRS_HAVELINEADJACENTTOTOP, true);
    aState.mLineAdjacentToTop = line;
  }

  
  
  
  return false;
}

void
nsBlockFrame::ReflowBlockFrame(nsBlockReflowState& aState,
                               line_iterator aLine,
                               bool* aKeepReflowGoing)
{
  NS_PRECONDITION(*aKeepReflowGoing, "bad caller");

  nsIFrame* frame = aLine->mFirstChild;
  if (!frame) {
    NS_ASSERTION(false, "program error - unexpected empty line"); 
    return; 
  }

  
  const nsStyleDisplay* display = frame->StyleDisplay();
  nsBlockReflowContext brc(aState.mPresContext, aState.mReflowState);

  uint8_t breakType = display->mBreakType;
  if (NS_STYLE_CLEAR_NONE != aState.mFloatBreakType) {
    breakType = nsLayoutUtils::CombineBreakType(breakType,
                                                aState.mFloatBreakType);
    aState.mFloatBreakType = NS_STYLE_CLEAR_NONE;
  }

  
  aLine->SetBreakTypeBefore(breakType);

  
  
  
  
  bool applyBStartMargin = (frame->StyleBorder()->mBoxDecorationBreak ==
                              NS_STYLE_BOX_DECORATION_BREAK_CLONE ||
                            !frame->GetPrevInFlow()) &&
                           ShouldApplyBStartMargin(aState, aLine, frame);
  if (applyBStartMargin) {
    
    
    
    
    aLine->ClearHasClearance();
  }
  bool treatWithClearance = aLine->HasClearance();

  bool mightClearFloats = breakType != NS_STYLE_CLEAR_NONE;
  nsIFrame *replacedBlock = nullptr;
  if (!nsBlockFrame::BlockCanIntersectFloats(frame)) {
    mightClearFloats = true;
    replacedBlock = frame;
  }

  
  
  
  
  if (!treatWithClearance && !applyBStartMargin && mightClearFloats &&
      aState.mReflowState.mDiscoveredClearance) {
    nscoord curBCoord = aState.mBCoord + aState.mPrevBEndMargin.get();
    nscoord clearBCoord = aState.ClearFloats(curBCoord, breakType, replacedBlock);
    if (clearBCoord != curBCoord) {
      
      
      
      
      treatWithClearance = true;
      
      if (!*aState.mReflowState.mDiscoveredClearance) {
        *aState.mReflowState.mDiscoveredClearance = frame;
      }
      aState.mPrevChild = frame;
      
      
      return;
    }
  }
  if (treatWithClearance) {
    applyBStartMargin = true;
  }

  nsIFrame* clearanceFrame = nullptr;
  nscoord startingBCoord = aState.mBCoord;
  nsCollapsingMargin incomingMargin = aState.mPrevBEndMargin;
  nscoord clearance;
  
  
  nsPoint originalPosition = frame->GetPosition();
  while (true) {
    clearance = 0;
    nscoord bStartMargin = 0;
    bool mayNeedRetry = false;
    bool clearedFloats = false;
    if (applyBStartMargin) {
      
      
      

      
      
      

      
      
      
      
      
      WritingMode wm = frame->GetWritingMode();
      LogicalSize availSpace = aState.ContentSize(wm);
      nsHTMLReflowState reflowState(aState.mPresContext, aState.mReflowState,
                                    frame, availSpace);

      if (treatWithClearance) {
        aState.mBCoord += aState.mPrevBEndMargin.get();
        aState.mPrevBEndMargin.Zero();
      }

      
      
      
      brc.ComputeCollapsedBStartMargin(reflowState,
                                       &aState.mPrevBEndMargin,
                                       clearanceFrame,
                                       &mayNeedRetry);

      
      

      if (clearanceFrame) {
        
        
        mayNeedRetry = false;
      }

      if (!treatWithClearance && !clearanceFrame && mightClearFloats) {
        
        
        
        
        
        
        nscoord curBCoord = aState.mBCoord + aState.mPrevBEndMargin.get();
        nscoord clearBCoord = aState.ClearFloats(curBCoord, breakType, replacedBlock);
        if (clearBCoord != curBCoord) {
          
          
          treatWithClearance = true;
          
          aLine->SetHasClearance();

          
          aState.mBCoord += aState.mPrevBEndMargin.get();
          aState.mPrevBEndMargin.Zero();

          
          mayNeedRetry = false;
          brc.ComputeCollapsedBStartMargin(reflowState,
                                           &aState.mPrevBEndMargin,
                                           clearanceFrame,
                                           &mayNeedRetry);
        }
      }

      
      
      
      
      bStartMargin = aState.mPrevBEndMargin.get();

      if (treatWithClearance) {
        nscoord currentBCoord = aState.mBCoord;
        
        aState.mBCoord = aState.ClearFloats(aState.mBCoord, breakType,
                                            replacedBlock);

        clearedFloats = aState.mBCoord != currentBCoord;

        
        
        
        
        
        
        
        clearance = aState.mBCoord - (currentBCoord + bStartMargin);

        
        
        bStartMargin += clearance;

        
        
      } else {
        
        aState.mBCoord += bStartMargin;
      }
    }

    
    
    nsFlowAreaRect floatAvailableSpace = aState.GetFloatAvailableSpace();
#ifdef REALLY_NOISY_REFLOW
    printf("setting line %p isImpacted to %s\n",
           aLine.get(), floatAvailableSpace.mHasFloats?"true":"false");
#endif
    aLine->SetLineIsImpactedByFloat(floatAvailableSpace.mHasFloats);
    WritingMode wm = aState.mReflowState.GetWritingMode();
    LogicalRect availSpace(wm);
    aState.ComputeBlockAvailSpace(frame, display, floatAvailableSpace,
                                  replacedBlock != nullptr, availSpace);

    
    
    
    
    
    if ((!aState.mReflowState.mFlags.mIsTopOfPage || clearedFloats) &&
        availSpace.BSize(wm) < 0) {
      
      
      
      
      
      
      
      
      
      
      aState.mBCoord = startingBCoord;
      aState.mPrevBEndMargin = incomingMargin;
      *aKeepReflowGoing = false;
      if (ShouldAvoidBreakInside(aState.mReflowState)) {
        aState.mReflowStatus = NS_INLINE_LINE_BREAK_BEFORE();
      } else {
        PushLines(aState, aLine.prev());
        NS_FRAME_SET_INCOMPLETE(aState.mReflowStatus);
      }
      return;
    }

    
    
    aState.mBCoord -= bStartMargin;
    availSpace.BStart(wm) -= bStartMargin;
    if (NS_UNCONSTRAINEDSIZE != availSpace.BSize(wm)) {
      availSpace.BSize(wm) += bStartMargin;
    }

    
    
    
    nsHTMLReflowState
      blockHtmlRS(aState.mPresContext, aState.mReflowState, frame,
                  availSpace.Size(wm).ConvertTo(frame->GetWritingMode(), wm));
    blockHtmlRS.mFlags.mHasClearance = aLine->HasClearance();

    nsFloatManager::SavedState floatManagerState;
    if (mayNeedRetry) {
      blockHtmlRS.mDiscoveredClearance = &clearanceFrame;
      aState.mFloatManager->PushState(&floatManagerState);
    } else if (!applyBStartMargin) {
      blockHtmlRS.mDiscoveredClearance = aState.mReflowState.mDiscoveredClearance;
    }

    nsReflowStatus frameReflowStatus = NS_FRAME_COMPLETE;
    brc.ReflowBlock(availSpace, applyBStartMargin, aState.mPrevBEndMargin,
                    clearance, aState.IsAdjacentWithTop(),
                    aLine.get(), blockHtmlRS, frameReflowStatus, aState);

    if (mayNeedRetry && clearanceFrame) {
      aState.mFloatManager->PopState(&floatManagerState);
      aState.mBCoord = startingBCoord;
      aState.mPrevBEndMargin = incomingMargin;
      continue;
    }

    aState.mPrevChild = frame;

    if (blockHtmlRS.WillReflowAgainForClearance()) {
      
      
      
      
      
      NS_ASSERTION(originalPosition == frame->GetPosition(),
                   "we need to call PositionChildViews");
      return;
    }

#if defined(REFLOW_STATUS_COVERAGE)
    RecordReflowStatus(true, frameReflowStatus);
#endif

    if (NS_INLINE_IS_BREAK_BEFORE(frameReflowStatus)) {
      
      *aKeepReflowGoing = false;
      if (ShouldAvoidBreakInside(aState.mReflowState)) {
        aState.mReflowStatus = NS_INLINE_LINE_BREAK_BEFORE();
      } else {
        PushLines(aState, aLine.prev());
        NS_FRAME_SET_INCOMPLETE(aState.mReflowStatus);
      }
    }
    else {
      

      
      
      
      
      
      
      bool forceFit = aState.IsAdjacentWithTop() && clearance <= 0 &&
        !floatAvailableSpace.mHasFloats;
      nsCollapsingMargin collapsedBEndMargin;
      nsOverflowAreas overflowAreas;
      *aKeepReflowGoing = brc.PlaceBlock(blockHtmlRS, forceFit, aLine.get(),
                                         collapsedBEndMargin,
                                         overflowAreas,
                                         frameReflowStatus);
      if (!NS_FRAME_IS_FULLY_COMPLETE(frameReflowStatus) &&
          ShouldAvoidBreakInside(aState.mReflowState)) {
        *aKeepReflowGoing = false;
      }

      if (aLine->SetCarriedOutBEndMargin(collapsedBEndMargin)) {
        line_iterator nextLine = aLine;
        ++nextLine;
        if (nextLine != end_lines()) {
          nextLine->MarkPreviousMarginDirty();
        }
      }

      aLine->SetOverflowAreas(overflowAreas);
      if (*aKeepReflowGoing) {
        
        
        
        nscoord newBCoord = aLine->BEnd();
        aState.mBCoord = newBCoord;

        
        
        
        if (!NS_FRAME_IS_FULLY_COMPLETE(frameReflowStatus)) {
          bool madeContinuation =
            CreateContinuationFor(aState, nullptr, frame);
          
          nsIFrame* nextFrame = frame->GetNextInFlow();
          NS_ASSERTION(nextFrame, "We're supposed to have a next-in-flow by now");
          
          if (NS_FRAME_IS_NOT_COMPLETE(frameReflowStatus)) {
            
            if (!madeContinuation &&
                (NS_FRAME_IS_OVERFLOW_CONTAINER & nextFrame->GetStateBits())) {
              nsOverflowContinuationTracker::AutoFinish fini(aState.mOverflowTracker, frame);
              nsContainerFrame* parent = nextFrame->GetParent();
              nsresult rv = parent->StealFrame(nextFrame);
              if (NS_FAILED(rv)) {
                return;
              }
              if (parent != this)
                ReparentFrame(nextFrame, parent, this);
              mFrames.InsertFrame(nullptr, frame, nextFrame);
              madeContinuation = true; 
              nextFrame->RemoveStateBits(NS_FRAME_IS_OVERFLOW_CONTAINER);
              frameReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
            }

            
            if (madeContinuation) {
              nsLineBox* line = NewLineBox(nextFrame, true);
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
            
            
            
            
            
#ifdef NOISY_BLOCK_DIR_MARGINS
            ListTag(stdout);
            printf(": reflow incomplete, frame=");
            nsFrame::ListTag(stdout, frame);
            printf(" prevBEndMargin=%d, setting to zero\n",
                   aState.mPrevBEndMargin);
#endif
            aState.mPrevBEndMargin.Zero();
          }
          else { 
            
            if (!madeContinuation &&
                !(NS_FRAME_IS_OVERFLOW_CONTAINER & nextFrame->GetStateBits())) {
              
              
              nsresult rv = nextFrame->GetParent()->StealFrame(nextFrame);
              if (NS_FAILED(rv)) {
                return;
              }
            }
            else if (madeContinuation) {
              mFrames.RemoveFrame(nextFrame);
            }

            
            aState.mOverflowTracker->Insert(nextFrame, frameReflowStatus);
            NS_MergeReflowStatusInto(&aState.mReflowStatus, frameReflowStatus);

#ifdef NOISY_BLOCK_DIR_MARGINS
            ListTag(stdout);
            printf(": reflow complete but overflow incomplete for ");
            nsFrame::ListTag(stdout, frame);
            printf(" prevBEndMargin=%d collapsedBEndMargin=%d\n",
                   aState.mPrevBEndMargin, collapsedBEndMargin.get());
#endif
            aState.mPrevBEndMargin = collapsedBEndMargin;
          }
        }
        else { 
#ifdef NOISY_BLOCK_DIR_MARGINS
          ListTag(stdout);
          printf(": reflow complete for ");
          nsFrame::ListTag(stdout, frame);
          printf(" prevBEndMargin=%d collapsedBEndMargin=%d\n",
                 aState.mPrevBEndMargin, collapsedBEndMargin.get());
#endif
          aState.mPrevBEndMargin = collapsedBEndMargin;
        }
#ifdef NOISY_BLOCK_DIR_MARGINS
        ListTag(stdout);
        printf(": frame=");
        nsFrame::ListTag(stdout, frame);
        printf(" carriedOutBEndMargin=%d collapsedBEndMargin=%d => %d\n",
               brc.GetCarriedOutBEndMargin(), collapsedBEndMargin.get(),
               aState.mPrevBEndMargin);
#endif
      } else {
        if ((aLine == mLines.front() && !GetPrevInFlow()) ||
            ShouldAvoidBreakInside(aState.mReflowState)) {
          
          
          
          aState.mReflowStatus = NS_INLINE_LINE_BREAK_BEFORE();
        } else {
          
          
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
}

void
nsBlockFrame::ReflowInlineFrames(nsBlockReflowState& aState,
                                 line_iterator aLine,
                                 bool* aKeepReflowGoing)
{
  *aKeepReflowGoing = true;

  aLine->SetLineIsImpactedByFloat(false);

  
  
  if (ShouldApplyBStartMargin(aState, aLine, aLine->mFirstChild)) {
    aState.mBCoord += aState.mPrevBEndMargin.get();
  }
  nsFlowAreaRect floatAvailableSpace = aState.GetFloatAvailableSpace();

  LineReflowStatus lineReflowStatus;
  do {
    nscoord availableSpaceHeight = 0;
    do {
      bool allowPullUp = true;
      nsIFrame* forceBreakInFrame = nullptr;
      int32_t forceBreakOffset = -1;
      gfxBreakPriority forceBreakPriority = gfxBreakPriority::eNoBreak;
      do {
        nsFloatManager::SavedState floatManagerState;
        aState.mReflowState.mFloatManager->PushState(&floatManagerState);

        
        
        
        
        
        
        
        nsLineLayout lineLayout(aState.mPresContext,
                                aState.mReflowState.mFloatManager,
                                &aState.mReflowState, &aLine, nullptr);
        lineLayout.Init(&aState, aState.mMinLineHeight, aState.mLineNumber);
        if (forceBreakInFrame) {
          lineLayout.ForceBreakAtPosition(forceBreakInFrame, forceBreakOffset);
        }
        DoReflowInlineFrames(aState, lineLayout, aLine,
                             floatAvailableSpace, availableSpaceHeight,
                             &floatManagerState, aKeepReflowGoing,
                             &lineReflowStatus, allowPullUp);
        lineLayout.EndLineReflow();

        if (LINE_REFLOW_REDO_NO_PULL == lineReflowStatus ||
            LINE_REFLOW_REDO_MORE_FLOATS == lineReflowStatus ||
            LINE_REFLOW_REDO_NEXT_BAND == lineReflowStatus) {
          if (lineLayout.NeedsBackup()) {
            NS_ASSERTION(!forceBreakInFrame, "Backing up twice; this should never be necessary");
            
            
            
            forceBreakInFrame =
              lineLayout.GetLastOptionalBreakPosition(&forceBreakOffset, &forceBreakPriority);
          } else {
            forceBreakInFrame = nullptr;
          }
          
          aState.mReflowState.mFloatManager->PopState(&floatManagerState);
          
          aState.mCurrentLineFloats.DeleteAll();
          aState.mBelowCurrentLineFloats.DeleteAll();
        }

        
        allowPullUp = false;
      } while (LINE_REFLOW_REDO_NO_PULL == lineReflowStatus);
    } while (LINE_REFLOW_REDO_MORE_FLOATS == lineReflowStatus);
  } while (LINE_REFLOW_REDO_NEXT_BAND == lineReflowStatus);
}

void
nsBlockFrame::PushTruncatedLine(nsBlockReflowState& aState,
                                line_iterator       aLine,
                                bool*               aKeepReflowGoing)
{
  PushLines(aState, aLine.prev());
  *aKeepReflowGoing = false;
  NS_FRAME_SET_INCOMPLETE(aState.mReflowStatus);
}

#ifdef DEBUG
static const char* LineReflowStatusNames[] = {
  "LINE_REFLOW_OK", "LINE_REFLOW_STOP", "LINE_REFLOW_REDO_NO_PULL",
  "LINE_REFLOW_REDO_MORE_FLOATS",
  "LINE_REFLOW_REDO_NEXT_BAND", "LINE_REFLOW_TRUNCATED"
};
#endif

void
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

  WritingMode outerWM = aState.mReflowState.GetWritingMode();
  WritingMode lineWM = GetWritingMode(aLine->mFirstChild);
  LogicalRect lineRect =
    aFloatAvailableSpace.mRect.ConvertTo(lineWM, outerWM,
                                         aState.ContainerWidth());

  nscoord iStart = lineRect.IStart(lineWM);
  nscoord availISize = lineRect.ISize(lineWM);
  nscoord availBSize;
  if (aState.GetFlag(BRS_UNCONSTRAINEDBSIZE)) {
    availBSize = NS_UNCONSTRAINEDSIZE;
  }
  else {
    
    availBSize = lineRect.BSize(lineWM);
  }

  
  
  aLine->EnableResizeReflowOptimization();

  aLineLayout.BeginLineReflow(iStart, aState.mBCoord,
                              availISize, availBSize,
                              aFloatAvailableSpace.mHasFloats,
                              false, 
                              lineWM, aState.mContainerSize);

  aState.SetFlag(BRS_LINE_LAYOUT_EMPTY, false);

  
  
  if ((0 == aLineLayout.GetLineNumber()) &&
      (NS_BLOCK_HAS_FIRST_LETTER_CHILD & mState) &&
      (NS_BLOCK_HAS_FIRST_LETTER_STYLE & mState)) {
    aLineLayout.SetFirstLetterStyleOK(true);
  }
  NS_ASSERTION(!((NS_BLOCK_HAS_FIRST_LETTER_CHILD & mState) &&
                 GetPrevContinuation()),
               "first letter child bit should only be on first continuation");

  
  LineReflowStatus lineReflowStatus = LINE_REFLOW_OK;
  int32_t i;
  nsIFrame* frame = aLine->mFirstChild;

  if (aFloatAvailableSpace.mHasFloats) {
    
    
    if (aLineLayout.NotifyOptionalBreakPosition(
            frame, 0, true, gfxBreakPriority::eNormalBreak)) {
      lineReflowStatus = LINE_REFLOW_REDO_NEXT_BAND;
    }
  }

  
  
  for (i = 0; LINE_REFLOW_OK == lineReflowStatus && i < aLine->GetChildCount();
       i++, frame = frame->GetNextSibling()) {
    ReflowInlineFrame(aState, aLineLayout, aLine, frame, &lineReflowStatus);
    if (LINE_REFLOW_OK != lineReflowStatus) {
      
      
      
      ++aLine;
      while ((aLine != end_lines()) && (0 == aLine->GetChildCount())) {
        
        
        nsLineBox *toremove = aLine;
        aLine = mLines.erase(aLine);
        NS_ASSERTION(nullptr == toremove->mFirstChild, "bad empty line");
        FreeLineBox(toremove);
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
        int32_t oldCount = aLine->GetChildCount();
        ReflowInlineFrame(aState, aLineLayout, aLine, frame, &lineReflowStatus);
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
    
    
    
    
    if (aLineLayout.HasOptionalBreakPosition()) {
      
      lineReflowStatus = LINE_REFLOW_REDO_NO_PULL;
    }
  } else {
    
    
    aLineLayout.ClearOptionalBreakPosition();
  }

  if (LINE_REFLOW_REDO_NEXT_BAND == lineReflowStatus) {
    
    
    
    
    
    
    NS_ASSERTION(NS_UNCONSTRAINEDSIZE !=
                 aFloatAvailableSpace.mRect.BSize(outerWM),
                 "unconstrained block size on totally empty line");

    
    if (aFloatAvailableSpace.mRect.BSize(outerWM) > 0) {
      NS_ASSERTION(aFloatAvailableSpace.mHasFloats,
                   "redo line on totally empty line with non-empty band...");
      
      
      
      
      aState.mFloatManager->AssertStateMatches(aFloatStateBeforeLine);
      aState.mBCoord += aFloatAvailableSpace.mRect.BSize(outerWM);
      aFloatAvailableSpace = aState.GetFloatAvailableSpace();
    } else {
      NS_ASSERTION(NS_UNCONSTRAINEDSIZE != aState.mReflowState.AvailableBSize(),
                   "We shouldn't be running out of height here");
      if (NS_UNCONSTRAINEDSIZE == aState.mReflowState.AvailableBSize()) {
        
        aState.mBCoord += 1;
        
        
        
        
        aState.mFloatManager->AssertStateMatches(aFloatStateBeforeLine);
        aFloatAvailableSpace = aState.GetFloatAvailableSpace();
      } else {
        
        
        
        lineReflowStatus = LINE_REFLOW_TRUNCATED;
        PushTruncatedLine(aState, aLine, aKeepReflowGoing);
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
    
    FrameLines* overflowLines = GetOverflowLines();
    
    
    bool pushedToOverflowLines = overflowLines &&
      overflowLines->mLines.front() == aLine.get();
    if (pushedToOverflowLines) {
      
      
      aLine = overflowLines->mLines.begin();
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
}









void
nsBlockFrame::ReflowInlineFrame(nsBlockReflowState& aState,
                                nsLineLayout& aLineLayout,
                                line_iterator aLine,
                                nsIFrame* aFrame,
                                LineReflowStatus* aLineReflowStatus)
{
  if (!aFrame) { 
    NS_ERROR("why call me?");
    return;
  }
  
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
  aLineLayout.ReflowFrame(aFrame, frameReflowStatus, nullptr, pushedFrame);

  if (frameReflowStatus & NS_FRAME_REFLOW_NEXTINFLOW) {
    aLineLayout.SetDirtyNextLine();
  }

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

    
    uint8_t breakType = NS_INLINE_GET_BREAK_TYPE(frameReflowStatus);
    NS_ASSERTION((NS_STYLE_CLEAR_NONE != breakType) || 
                 (NS_STYLE_CLEAR_NONE != aState.mFloatBreakType), "bad break type");
    NS_ASSERTION(NS_STYLE_CLEAR_MAX >= breakType, "invalid break type");

    if (NS_INLINE_IS_BREAK_BEFORE(frameReflowStatus)) {
      
      if (aFrame == aLine->mFirstChild) {
        
        
        
        
        *aLineReflowStatus = LINE_REFLOW_REDO_NEXT_BAND;
      }
      else {
        
        
        SplitLine(aState, aLineLayout, aLine, aFrame, aLineReflowStatus);

        
        
        
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
        
        SplitLine(aState, aLineLayout, aLine, aFrame->GetNextSibling(), aLineReflowStatus);

        if (NS_INLINE_IS_BREAK_AFTER(frameReflowStatus) &&
            !aLineLayout.GetLineEndsInBR()) {
          aLineLayout.SetDirtyNextLine();
        }
      }
    }
  }

  if (!NS_FRAME_IS_FULLY_COMPLETE(frameReflowStatus)) {
    
    
    CreateContinuationFor(aState, aLine, aFrame);

    
    if (!aLineLayout.GetLineEndsInBR()) {
      aLine->SetLineWrapped(true);
    }
    
    
    
    
    if ((!(frameReflowStatus & NS_INLINE_BREAK_FIRST_LETTER_COMPLETE) && 
         nsGkAtoms::placeholderFrame != aFrame->GetType()) ||
        *aLineReflowStatus == LINE_REFLOW_STOP) {
      
      *aLineReflowStatus = LINE_REFLOW_STOP;
      SplitLine(aState, aLineLayout, aLine, aFrame->GetNextSibling(), aLineReflowStatus);
    }
  }
}

bool
nsBlockFrame::CreateContinuationFor(nsBlockReflowState& aState,
                                    nsLineBox*          aLine,
                                    nsIFrame*           aFrame)
{
  nsIFrame* newFrame = nullptr;

  if (!aFrame->GetNextInFlow()) {
    newFrame = aState.mPresContext->PresShell()->FrameConstructor()->
      CreateContinuingFrame(aState.mPresContext, aFrame, this);

    mFrames.InsertFrame(nullptr, aFrame, newFrame);

    if (aLine) {
      aLine->NoteFrameAdded(newFrame);
    }
  }
#ifdef DEBUG
  VerifyLines(false);
#endif
  return !!newFrame;
}

nsresult
nsBlockFrame::SplitFloat(nsBlockReflowState& aState,
                         nsIFrame*           aFloat,
                         nsReflowStatus      aFloatStatus)
{
  MOZ_ASSERT(!NS_FRAME_IS_FULLY_COMPLETE(aFloatStatus),
             "why split the frame if it's fully complete?");
  MOZ_ASSERT(aState.mBlock == this);

  nsIFrame* nextInFlow = aFloat->GetNextInFlow();
  if (nextInFlow) {
    nsContainerFrame *oldParent = nextInFlow->GetParent();
    DebugOnly<nsresult> rv = oldParent->StealFrame(nextInFlow);
    NS_ASSERTION(NS_SUCCEEDED(rv), "StealFrame failed");
    if (oldParent != this) {
      ReparentFrame(nextInFlow, oldParent, this);
    }
    if (!NS_FRAME_OVERFLOW_IS_INCOMPLETE(aFloatStatus)) {
      nextInFlow->RemoveStateBits(NS_FRAME_IS_OVERFLOW_CONTAINER);
    }
  } else {
    nextInFlow = aState.mPresContext->PresShell()->FrameConstructor()->
      CreateContinuingFrame(aState.mPresContext, aFloat, this);
  }
  if (NS_FRAME_OVERFLOW_IS_INCOMPLETE(aFloatStatus)) {
    nextInFlow->AddStateBits(NS_FRAME_IS_OVERFLOW_CONTAINER);
  }

  if (aFloat->StyleDisplay()->mFloats == NS_STYLE_FLOAT_LEFT) {
    aState.mFloatManager->SetSplitLeftFloatAcrossBreak();
  } else {
    MOZ_ASSERT(aFloat->StyleDisplay()->mFloats == NS_STYLE_FLOAT_RIGHT,
               "unexpected float side");
    aState.mFloatManager->SetSplitRightFloatAcrossBreak();
  }

  aState.AppendPushedFloatChain(nextInFlow);
  NS_FRAME_SET_OVERFLOW_INCOMPLETE(aState.mReflowStatus);
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
                   GetPlaceholderFrameFor(aFC->mFloat->FirstInFlow());
  for (nsIFrame* f = ph; f; f = f->GetParent()) {
    if (f->GetParent() == aBlock)
      return aLine->Contains(f);
  }
  NS_ASSERTION(false, "aBlock is not an ancestor of aFrame!");
  return true;
}

void
nsBlockFrame::SplitLine(nsBlockReflowState& aState,
                        nsLineLayout& aLineLayout,
                        line_iterator aLine,
                        nsIFrame* aFrame,
                        LineReflowStatus* aLineReflowStatus)
{
  MOZ_ASSERT(aLine->IsInline(), "illegal SplitLine on block line");

  int32_t pushCount = aLine->GetChildCount() - aLineLayout.GetCurrentSpanCount();
  MOZ_ASSERT(pushCount >= 0, "bad push count"); 

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
    MOZ_ASSERT(aLine->GetChildCount() > pushCount, "bad push");
    MOZ_ASSERT(nullptr != aFrame, "whoops");
#ifdef DEBUG
    {
      nsIFrame *f = aFrame;
      int32_t count = pushCount;
      while (f && count > 0) {
        f = f->GetNextSibling();
        --count;
      }
      NS_ASSERTION(count == 0, "Not enough frames to push");
    }
#endif

    
    nsLineBox* newLine = NewLineBox(aLine, aFrame, pushCount);
    mLines.after_insert(aLine, newLine);
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
}

bool
nsBlockFrame::IsLastLine(nsBlockReflowState& aState,
                         line_iterator aLine)
{
  while (++aLine != end_lines()) {
    
    if (0 != aLine->GetChildCount()) {
      
      
      return aLine->IsBlock();
    }
    
  }

  
  
  nsBlockFrame* nextInFlow = (nsBlockFrame*) GetNextInFlow();
  while (nullptr != nextInFlow) {
    for (line_iterator line = nextInFlow->begin_lines(),
                   line_end = nextInFlow->end_lines();
         line != line_end;
         ++line)
    {
      if (0 != line->GetChildCount())
        return line->IsBlock();
    }
    nextInFlow = (nsBlockFrame*) nextInFlow->GetNextInFlow();
  }

  
  return true;
}

bool
nsBlockFrame::PlaceLine(nsBlockReflowState& aState,
                        nsLineLayout&       aLineLayout,
                        line_iterator       aLine,
                        nsFloatManager::SavedState *aFloatStateBeforeLine,
                        LogicalRect&        aFloatAvailableSpace,
                        nscoord&            aAvailableSpaceHeight,
                        bool*             aKeepReflowGoing)
{
  
  aLineLayout.TrimTrailingWhiteSpace();

  
  
  
  
  
  
  
  
  
  WritingMode wm = aState.mReflowState.GetWritingMode();
  bool addedBullet = false;
  if (HasOutsideBullet() &&
      ((aLine == mLines.front() &&
        (!aLineLayout.IsZeroBSize() || (aLine == mLines.back()))) ||
       (mLines.front() != mLines.back() &&
        0 == mLines.front()->BSize() &&
        aLine == mLines.begin().next()))) {
    nsHTMLReflowMetrics metrics(aState.mReflowState);
    nsIFrame* bullet = GetOutsideBullet();
    ReflowBullet(bullet, aState, metrics, aState.mBCoord);
    NS_ASSERTION(!BulletIsEmpty() || metrics.BSize(wm) == 0,
                 "empty bullet took up space");
    aLineLayout.AddBulletFrame(bullet, metrics);
    addedBullet = true;
  }
  aLineLayout.VerticalAlignLine();

  
  
  
  
  
  LogicalRect oldFloatAvailableSpace(aFloatAvailableSpace);
  
  
  aAvailableSpaceHeight = std::max(aAvailableSpaceHeight, aLine->BSize());
  aFloatAvailableSpace =
    aState.GetFloatAvailableSpaceForBSize(aLine->BStart(),
                                          aAvailableSpaceHeight,
                                          aFloatStateBeforeLine).mRect;
  NS_ASSERTION(aFloatAvailableSpace.BStart(wm) ==
               oldFloatAvailableSpace.BStart(wm), "yikes");
  
  aFloatAvailableSpace.BSize(wm) = oldFloatAvailableSpace.BSize(wm);
  
  
  
  if (AvailableSpaceShrunk(wm, oldFloatAvailableSpace, aFloatAvailableSpace)) {
    return false;
  }

#ifdef DEBUG
  {
    static nscoord lastHeight = 0;
    if (CRAZY_SIZE(aLine->BStart())) {
      lastHeight = aLine->BStart();
      if (abs(aLine->BStart() - lastHeight) > CRAZY_COORD/10) {
        nsFrame::ListTag(stdout);
        printf(": line=%p y=%d line.bounds.height=%d\n",
               static_cast<void*>(aLine.get()),
               aLine->BStart(), aLine->BSize());
      }
    }
    else {
      lastHeight = 0;
    }
  }
#endif

  
  
  
  const nsStyleText* styleText = StyleText();

  






  bool isLastLine =
    !IsSVGText() &&
    ((NS_STYLE_TEXT_ALIGN_AUTO != styleText->mTextAlignLast ||
      NS_STYLE_TEXT_ALIGN_JUSTIFY == styleText->mTextAlign) &&
     (aLineLayout.GetLineEndsInBR() ||
      IsLastLine(aState, aLine)));

  aLineLayout.TextAlignLine(aLine, isLastLine);

  
  
  nsOverflowAreas overflowAreas;
  aLineLayout.RelativePositionFrames(overflowAreas);
  aLine->SetOverflowAreas(overflowAreas);
  if (addedBullet) {
    aLineLayout.RemoveBulletFrame(GetOutsideBullet());
  }

  
  
  
  
  
  
  nscoord newBCoord;

  if (!aLine->CachedIsEmpty()) {
    
    
    aState.mPrevBEndMargin.Zero();
    newBCoord = aLine->BEnd();
  }
  else {
    
    
    
    
    
    nscoord dy = aState.GetFlag(BRS_APPLYBSTARTMARGIN)
                   ? -aState.mPrevBEndMargin.get() : 0;
    newBCoord = aState.mBCoord + dy;
  }

  if (!NS_FRAME_IS_FULLY_COMPLETE(aState.mReflowStatus) &&
      ShouldAvoidBreakInside(aState.mReflowState)) {
    aLine->AppendFloats(aState.mCurrentLineFloats);
    aState.mReflowStatus = NS_INLINE_LINE_BREAK_BEFORE();
    return true;
  }

  
  if (mLines.front() != aLine &&
      newBCoord > aState.mBEndEdge &&
      aState.mBEndEdge != NS_UNCONSTRAINEDSIZE) {
    NS_ASSERTION(aState.mCurrentLine == aLine, "oops");
    if (ShouldAvoidBreakInside(aState.mReflowState)) {
      
      aState.mReflowStatus = NS_INLINE_LINE_BREAK_BEFORE();
    } else {
      
      
      PushTruncatedLine(aState, aLine, aKeepReflowGoing);
    }
    return true;
  }

  aState.mBCoord = newBCoord;

  
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
    aState.mBCoord = aState.ClearFloats(aState.mBCoord, aLine->GetBreakTypeAfter());
  }
  return true;
}

void
nsBlockFrame::PushLines(nsBlockReflowState&  aState,
                        nsLineList::iterator aLineBefore)
{
  
  
  DebugOnly<bool> check = aLineBefore == mLines.begin();

  nsLineList::iterator overBegin(aLineBefore.next());

  
  bool firstLine = overBegin == begin_lines();

  if (overBegin != end_lines()) {
    
    nsFrameList floats;
    CollectFloats(overBegin->mFirstChild, floats, true);

    if (floats.NotEmpty()) {
      
      nsAutoOOFFrameList oofs(this);
      oofs.mList.InsertFrames(nullptr, nullptr, floats);
    }

    
    
    
    
    
    FrameLines* overflowLines = RemoveOverflowLines();
    if (!overflowLines) {
      
      overflowLines = new FrameLines();
    }
    if (overflowLines) {
      nsIFrame* lineBeforeLastFrame;
      if (firstLine) {
        lineBeforeLastFrame = nullptr; 
      } else {
        nsIFrame* f = overBegin->mFirstChild;
        lineBeforeLastFrame = f ? f->GetPrevSibling() : mFrames.LastChild();
        NS_ASSERTION(!f || lineBeforeLastFrame == aLineBefore->LastChild(),
                     "unexpected line frames");
      }
      nsFrameList pushedFrames = mFrames.RemoveFramesAfter(lineBeforeLastFrame);
      overflowLines->mFrames.InsertFrames(nullptr, nullptr, pushedFrames);

      overflowLines->mLines.splice(overflowLines->mLines.begin(), mLines,
                                    overBegin, end_lines());
      NS_ASSERTION(!overflowLines->mLines.empty(), "should not be empty");
      
      
      SetOverflowLines(overflowLines);
  
      
      

      
      for (line_iterator line = overflowLines->mLines.begin(),
             line_end = overflowLines->mLines.end();
           line != line_end;
           ++line)
      {
        line->MarkDirty();
        line->MarkPreviousMarginDirty();
        line->SetBoundsEmpty();
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
nsBlockFrame::DrainOverflowLines()
{
#ifdef DEBUG
  VerifyOverflowSituation();
#endif

  
  bool didFindOverflow = false;
  nsBlockFrame* prevBlock = static_cast<nsBlockFrame*>(GetPrevInFlow());
  if (prevBlock) {
    prevBlock->ClearLineCursor();
    FrameLines* overflowLines = prevBlock->RemoveOverflowLines();
    if (overflowLines) {
      
      ReparentFrames(overflowLines->mFrames, prevBlock, this);

      
      nsAutoOOFFrameList oofs(prevBlock);
      if (oofs.mList.NotEmpty()) {
        
        
        for (nsFrameList::Enumerator e(oofs.mList); !e.AtEnd(); e.Next()) {
          nsIFrame* nif = e.get()->GetNextInFlow();
          for (; nif && nif->GetParent() == this; nif = nif->GetNextInFlow()) {
            MOZ_ASSERT(mFloats.ContainsFrame(nif));
            nif->RemoveStateBits(NS_FRAME_IS_PUSHED_FLOAT);
          }
        }
        ReparentFrames(oofs.mList, prevBlock, this);
        mFloats.InsertFrames(nullptr, nullptr, oofs.mList);
      }

      if (!mLines.empty()) {
        
        
        mLines.front()->MarkPreviousMarginDirty();
      }
      
      
      
      
      mFrames.InsertFrames(nullptr, nullptr, overflowLines->mFrames);
      mLines.splice(mLines.begin(), overflowLines->mLines);
      NS_ASSERTION(overflowLines->mLines.empty(), "splice should empty list");
      delete overflowLines;
      didFindOverflow = true;
    }
  }

  
  return DrainSelfOverflowList() || didFindOverflow;
}

bool
nsBlockFrame::DrainSelfOverflowList()
{
  nsAutoPtr<FrameLines> ourOverflowLines(RemoveOverflowLines());
  if (!ourOverflowLines) {
    return false;
  }

  
  
  nsAutoOOFFrameList oofs(this);
  if (oofs.mList.NotEmpty()) {
    
    mFloats.AppendFrames(nullptr, oofs.mList);
  }

  if (!ourOverflowLines->mLines.empty()) {
    mFrames.AppendFrames(nullptr, ourOverflowLines->mFrames);
    mLines.splice(mLines.end(), ourOverflowLines->mLines);
  }

#ifdef DEBUG
  VerifyOverflowSituation();
#endif
  return true;
}


























void
nsBlockFrame::DrainSelfPushedFloats()
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsPresContext* presContext = PresContext();
  nsFrameList* ourPushedFloats = GetPushedFloats();
  if (ourPushedFloats) {
    
    
    
    
    nsIFrame *insertionPrevSibling = nullptr; 
    for (nsIFrame* f = mFloats.FirstChild();
         f && (f->GetStateBits() & NS_FRAME_IS_PUSHED_FLOAT);
         f = f->GetNextSibling()) {
      insertionPrevSibling = f;
    }

    for (nsIFrame *f = ourPushedFloats->LastChild(), *next; f; f = next) {
      next = f->GetPrevSibling();

      if (f->GetPrevContinuation()) {
        
      } else {
        nsPlaceholderFrame *placeholder =
          presContext->FrameManager()->GetPlaceholderFrameFor(f);
        nsIFrame *floatOriginalParent = presContext->PresShell()->
          FrameConstructor()->GetFloatContainingBlock(placeholder);
        if (floatOriginalParent != this) {
          
          
          
          
          ourPushedFloats->RemoveFrame(f);
          mFloats.InsertFrame(nullptr, insertionPrevSibling, f);
        }
      }
    }

    if (ourPushedFloats->IsEmpty()) {
      RemovePushedFloats()->Delete(presContext->PresShell());
    }
  }
}

void
nsBlockFrame::DrainPushedFloats()
{
  DrainSelfPushedFloats();

  
  
  nsBlockFrame* prevBlock = static_cast<nsBlockFrame*>(GetPrevInFlow());
  if (prevBlock) {
    AutoFrameListPtr list(PresContext(), prevBlock->RemovePushedFloats());
    if (list && list->NotEmpty()) {
      mFloats.InsertFrames(this, nullptr, *list);
    }
  }
}

nsBlockFrame::FrameLines*
nsBlockFrame::GetOverflowLines() const
{
  if (!HasOverflowLines()) {
    return nullptr;
  }
  FrameLines* prop =
    static_cast<FrameLines*>(Properties().Get(OverflowLinesProperty()));
  NS_ASSERTION(prop && !prop->mLines.empty() &&
               prop->mLines.front()->GetChildCount() == 0 ? prop->mFrames.IsEmpty() :
                 prop->mLines.front()->mFirstChild == prop->mFrames.FirstChild(),
               "value should always be stored and non-empty when state set");
  return prop;
}

nsBlockFrame::FrameLines*
nsBlockFrame::RemoveOverflowLines()
{
  if (!HasOverflowLines()) {
    return nullptr;
  }
  FrameLines* prop =
    static_cast<FrameLines*>(Properties().Remove(OverflowLinesProperty()));
  NS_ASSERTION(prop && !prop->mLines.empty() &&
               prop->mLines.front()->GetChildCount() == 0 ? prop->mFrames.IsEmpty() :
                 prop->mLines.front()->mFirstChild == prop->mFrames.FirstChild(),
               "value should always be stored and non-empty when state set");
  RemoveStateBits(NS_BLOCK_HAS_OVERFLOW_LINES);
  return prop;
}

void
nsBlockFrame::DestroyOverflowLines()
{
  NS_ASSERTION(HasOverflowLines(), "huh?");
  FrameLines* prop =
    static_cast<FrameLines*>(Properties().Remove(OverflowLinesProperty()));
  NS_ASSERTION(prop && prop->mLines.empty(),
               "value should always be stored but empty when destroying");
  RemoveStateBits(NS_BLOCK_HAS_OVERFLOW_LINES);
  delete prop;
}



void
nsBlockFrame::SetOverflowLines(FrameLines* aOverflowLines)
{
  NS_ASSERTION(aOverflowLines, "null lines");
  NS_ASSERTION(!aOverflowLines->mLines.empty(), "empty lines");
  NS_ASSERTION(aOverflowLines->mLines.front()->mFirstChild ==
               aOverflowLines->mFrames.FirstChild(),
               "invalid overflow lines / frames");
  NS_ASSERTION(!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_LINES),
               "Overwriting existing overflow lines");

  FrameProperties props = Properties();
  
  NS_ASSERTION(!props.Get(OverflowLinesProperty()), "existing overflow list");
  props.Set(OverflowLinesProperty(), aOverflowLines);
  AddStateBits(NS_BLOCK_HAS_OVERFLOW_LINES);
}

nsFrameList*
nsBlockFrame::GetOverflowOutOfFlows() const
{
  if (!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS)) {
    return nullptr;
  }
  nsFrameList* result =
    GetPropTableFrames(OverflowOutOfFlowsProperty());
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
    nsFrameList* list = RemovePropTableFrames(OverflowOutOfFlowsProperty());
    NS_ASSERTION(aPropValue == list, "prop value mismatch");
    list->Clear();
    list->Delete(PresContext()->PresShell());
    RemoveStateBits(NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS);
  }
  else if (GetStateBits() & NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS) {
    NS_ASSERTION(aPropValue == GetPropTableFrames(OverflowOutOfFlowsProperty()),
                 "prop value mismatch");
    *aPropValue = aList;
  }
  else {
    SetPropTableFrames(new (PresContext()->PresShell()) nsFrameList(aList),
                       OverflowOutOfFlowsProperty());
    AddStateBits(NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS);
  }
}

nsBulletFrame*
nsBlockFrame::GetInsideBullet() const
{
  if (!HasInsideBullet()) {
    return nullptr;
  }
  NS_ASSERTION(!HasOutsideBullet(), "invalid bullet state");
  nsBulletFrame* frame =
    static_cast<nsBulletFrame*>(Properties().Get(InsideBulletProperty()));
  NS_ASSERTION(frame && frame->GetType() == nsGkAtoms::bulletFrame,
               "bogus inside bullet frame");
  return frame;
}

nsBulletFrame*
nsBlockFrame::GetOutsideBullet() const
{
  nsFrameList* list = GetOutsideBulletList();
  return list ? static_cast<nsBulletFrame*>(list->FirstChild())
              : nullptr;
}

nsFrameList*
nsBlockFrame::GetOutsideBulletList() const
{
  if (!HasOutsideBullet()) {
    return nullptr;
  }
  NS_ASSERTION(!HasInsideBullet(), "invalid bullet state");
  nsFrameList* list =
    static_cast<nsFrameList*>(Properties().Get(OutsideBulletProperty()));
  NS_ASSERTION(list && list->GetLength() == 1 &&
               list->FirstChild()->GetType() == nsGkAtoms::bulletFrame,
               "bogus outside bullet list");
  return list;
}

nsFrameList*
nsBlockFrame::GetPushedFloats() const
{
  if (!HasPushedFloats()) {
    return nullptr;
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

  result = new (PresContext()->PresShell()) nsFrameList;
  Properties().Set(PushedFloatProperty(), result);
  AddStateBits(NS_BLOCK_HAS_PUSHED_FLOATS);

  return result;
}

nsFrameList*
nsBlockFrame::RemovePushedFloats()
{
  if (!HasPushedFloats()) {
    return nullptr;
  }
  nsFrameList *result =
    static_cast<nsFrameList*>(Properties().Remove(PushedFloatProperty()));
  RemoveStateBits(NS_BLOCK_HAS_PUSHED_FLOATS);
  NS_ASSERTION(result, "value should always be non-empty when state set");
  return result;
}




void
nsBlockFrame::AppendFrames(ChildListID  aListID,
                           nsFrameList& aFrameList)
{
  if (aFrameList.IsEmpty()) {
    return;
  }
  if (aListID != kPrincipalList) {
    if (kFloatList == aListID) {
      DrainSelfPushedFloats(); 
      mFloats.AppendFrames(nullptr, aFrameList);
      return;
    }
    MOZ_ASSERT(kNoReflowPrincipalList == aListID, "unexpected child list");
  }

  
  nsIFrame* lastKid = mFrames.LastChild();
  NS_ASSERTION((mLines.empty() ? nullptr : mLines.back()->LastChild()) ==
               lastKid, "out-of-sync mLines / mFrames");

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

  AddFrames(aFrameList, lastKid);
  if (aListID != kNoReflowPrincipalList) {
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN); 
  }
}

void
nsBlockFrame::InsertFrames(ChildListID aListID,
                           nsIFrame* aPrevFrame,
                           nsFrameList& aFrameList)
{
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  if (aListID != kPrincipalList) {
    if (kFloatList == aListID) {
      DrainSelfPushedFloats(); 
      mFloats.InsertFrames(this, aPrevFrame, aFrameList);
      return;
    }
    MOZ_ASSERT(kNoReflowPrincipalList == aListID, "unexpected child list");
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

  AddFrames(aFrameList, aPrevFrame);
  if (aListID != kNoReflowPrincipalList) {
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN); 
  }
}

void
nsBlockFrame::RemoveFrame(ChildListID aListID,
                          nsIFrame* aOldFrame)
{
#ifdef NOISY_REFLOW_REASON
  ListTag(stdout);
  printf(": remove ");
  nsFrame::ListTag(stdout, aOldFrame);
  printf("\n");
#endif

  if (aListID == kPrincipalList) {
    bool hasFloats = BlockHasAnyFloats(aOldFrame);
    DoRemoveFrame(aOldFrame, REMOVE_FIXED_CONTINUATIONS);
    if (hasFloats) {
      MarkSameFloatManagerLinesDirty(this);
    }
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
  else if (kNoReflowPrincipalList == aListID) {
    
    DoRemoveFrame(aOldFrame, REMOVE_FIXED_CONTINUATIONS);
    return;
  }
  else {
    MOZ_CRASH("unexpected child list");
  }

  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN); 
}

static bool
ShouldPutNextSiblingOnNewLine(nsIFrame* aLastFrame)
{
  nsIAtom* type = aLastFrame->GetType();
  if (type == nsGkAtoms::brFrame) {
    return true;
  }
  
  if (type == nsGkAtoms::textFrame &&
      !(aLastFrame->GetStateBits() & TEXT_OFFSETS_NEED_FIXING)) {
    return aLastFrame->HasSignificantTerminalNewline();
  }
  return false;
}

void
nsBlockFrame::AddFrames(nsFrameList& aFrameList, nsIFrame* aPrevSibling)
{
  
  ClearLineCursor();

  if (aFrameList.IsEmpty()) {
    return;
  }

  
  
  if (!aPrevSibling && HasInsideBullet()) {
    aPrevSibling = GetInsideBullet();
  }
  
  
  nsLineList* lineList = &mLines;
  nsFrameList* frames = &mFrames;
  nsLineList::iterator prevSibLine = lineList->end();
  int32_t prevSiblingIndex = -1;
  if (aPrevSibling) {
    
    
    

    
    if (!nsLineBox::RFindLineContaining(aPrevSibling, lineList->begin(),
                                        prevSibLine, mFrames.LastChild(),
                                        &prevSiblingIndex)) {
      
      FrameLines* overflowLines = GetOverflowLines();
      bool found = false;
      if (overflowLines) {
        prevSibLine = overflowLines->mLines.end();
        prevSiblingIndex = -1;
        found = nsLineBox::RFindLineContaining(aPrevSibling,
                                               overflowLines->mLines.begin(),
                                               prevSibLine,
                                               overflowLines->mFrames.LastChild(),
                                               &prevSiblingIndex);
      }
      if (MOZ_LIKELY(found)) {
        lineList = &overflowLines->mLines;
        frames = &overflowLines->mFrames;
      } else {
        
        
        MOZ_ASSERT_UNREACHABLE("prev sibling not in line list");
        aPrevSibling = nullptr;
        prevSibLine = lineList->end();
      }
    }
  }

  
  
  if (aPrevSibling) {
    
    
    int32_t rem = prevSibLine->GetChildCount() - prevSiblingIndex - 1;
    if (rem) {
      
      nsLineBox* line = NewLineBox(prevSibLine, aPrevSibling->GetNextSibling(), rem);
      lineList->after_insert(prevSibLine, line);
      
      
      
      MarkLineDirty(prevSibLine, lineList);
      
      
      line->MarkDirty();
      line->SetInvalidateTextRuns(true);
    }
  }
  else if (! lineList->empty()) {
    lineList->front()->MarkDirty();
    lineList->front()->SetInvalidateTextRuns(true);
  }
  const nsFrameList::Slice& newFrames =
    frames->InsertFrames(nullptr, aPrevSibling, aFrameList);

  
  
  for (nsFrameList::Enumerator e(newFrames); !e.AtEnd(); e.Next()) {
    nsIFrame* newFrame = e.get();
    NS_ASSERTION(!aPrevSibling || aPrevSibling->GetNextSibling() == newFrame,
                 "Unexpected aPrevSibling");
    NS_ASSERTION(newFrame->GetType() != nsGkAtoms::placeholderFrame ||
                 (!newFrame->IsAbsolutelyPositioned() &&
                  !newFrame->IsFloating()),
                 "Placeholders should not float or be positioned");

    bool isBlock = newFrame->IsBlockOutside();

    
    
    
    
    
    if (isBlock || prevSibLine == lineList->end() || prevSibLine->IsBlock() ||
        (aPrevSibling && ShouldPutNextSiblingOnNewLine(aPrevSibling))) {
      
      
      nsLineBox* line = NewLineBox(newFrame, isBlock);
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
      prevSibLine->NoteFrameAdded(newFrame);
      
      
      
      MarkLineDirty(prevSibLine, lineList);
    }

    aPrevSibling = newFrame;
  }

#ifdef DEBUG
  MOZ_ASSERT(aFrameList.IsEmpty());
  VerifyLines(true);
#endif
}

void
nsBlockFrame::RemoveFloatFromFloatCache(nsIFrame* aFloat)
{
  
  
  line_iterator line = begin_lines(), line_end = end_lines();
  for ( ; line != line_end; ++line) {
    if (line->IsInline() && line->RemoveFloat(aFloat)) {
      break;
    }
  }
}

void
nsBlockFrame::RemoveFloat(nsIFrame* aFloat)
{
#ifdef DEBUG
  
  
  if (!mFloats.ContainsFrame(aFloat)) {
    MOZ_ASSERT((GetOverflowOutOfFlows() &&
                GetOverflowOutOfFlows()->ContainsFrame(aFloat)) ||
               (GetPushedFloats() &&
                GetPushedFloats()->ContainsFrame(aFloat)),
               "aFloat is not our child or on an unexpected frame list");
  }
#endif

  if (mFloats.StartRemoveFrame(aFloat)) {
    return;
  }

  nsFrameList* list = GetPushedFloats();
  if (list && list->ContinueRemoveFrame(aFloat)) {
#if 0
    
    
    if (list->IsEmpty()) {
      delete RemovePushedFloats();
    }
#endif
    return;
  }

  {
    nsAutoOOFFrameList oofs(this);
    if (oofs.mList.ContinueRemoveFrame(aFloat)) {
      return;
    }
  }
}

void
nsBlockFrame::DoRemoveOutOfFlowFrame(nsIFrame* aFrame)
{
  
  nsBlockFrame* block = (nsBlockFrame*)aFrame->GetParent();

  
  if (aFrame->IsAbsolutelyPositioned()) {
    
    block->GetAbsoluteContainingBlock()->RemoveFrame(block,
                                                     kAbsoluteList,
                                                     aFrame);
  }
  else {
    
    nsIFrame* nif = aFrame->GetNextInFlow();
    if (nif) {
      nif->GetParent()->DeleteNextInFlowChild(nif, false);
    }
    
    block->RemoveFloatFromFloatCache(aFrame);
    block->RemoveFloat(aFrame);
    aFrame->Destroy();
  }
}




void
nsBlockFrame::TryAllLines(nsLineList::iterator* aIterator,
                          nsLineList::iterator* aStartIterator,
                          nsLineList::iterator* aEndIterator,
                          bool* aInOverflowLines,
                          FrameLines** aOverflowLines)
{
  if (*aIterator == *aEndIterator) {
    if (!*aInOverflowLines) {
      
      *aInOverflowLines = true;
      FrameLines* lines = GetOverflowLines();
      if (lines) {
        *aStartIterator = lines->mLines.begin();
        *aIterator = *aStartIterator;
        *aEndIterator = lines->mLines.end();
        *aOverflowLines = lines;
      }
    }
  }
}

nsBlockInFlowLineIterator::nsBlockInFlowLineIterator(nsBlockFrame* aFrame,
    line_iterator aLine)
  : mFrame(aFrame), mLine(aLine), mLineList(&aFrame->mLines)
{
  
  DebugOnly<bool> check = aLine == mFrame->begin_lines();
}

nsBlockInFlowLineIterator::nsBlockInFlowLineIterator(nsBlockFrame* aFrame,
    line_iterator aLine, bool aInOverflow)
  : mFrame(aFrame), mLine(aLine),
    mLineList(aInOverflow ? &aFrame->GetOverflowLines()->mLines
                          : &aFrame->mLines)
{
}

nsBlockInFlowLineIterator::nsBlockInFlowLineIterator(nsBlockFrame* aFrame,
    bool* aFoundValidLine)
  : mFrame(aFrame), mLineList(&aFrame->mLines)
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
      return nullptr;
    if (!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW))
      break;
    aFindFrame = aFrame->PresContext()->FrameManager()->GetPlaceholderFrameFor(child);
  }

  return child;
}

nsBlockInFlowLineIterator::nsBlockInFlowLineIterator(nsBlockFrame* aFrame,
    nsIFrame* aFindFrame, bool* aFoundValidLine)
  : mFrame(aFrame), mLineList(&aFrame->mLines)
{
  *aFoundValidLine = false;

  nsIFrame* child = FindChildContaining(aFrame, aFindFrame);
  if (!child)
    return;

  
  nsLineBox* cursor = aFrame->GetLineCursor();
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
  return mLineList->end();
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
  line_iterator begin = mLineList->begin();
  if (mLine != begin) {
    --mLine;
    return true;
  }
  bool currentlyInOverflowLines = GetInOverflow();
  while (true) {
    if (currentlyInOverflowLines) {
      mLineList = &mFrame->mLines;
      mLine = mLineList->end();
      if (mLine != mLineList->begin()) {
        --mLine;
        return true;
      }
    } else {
      mFrame = static_cast<nsBlockFrame*>(mFrame->GetPrevInFlow());
      if (!mFrame)
        return false;
      nsBlockFrame::FrameLines* overflowLines = mFrame->GetOverflowLines();
      if (overflowLines) {
        mLineList = &overflowLines->mLines;
        mLine = mLineList->end();
        NS_ASSERTION(mLine != mLineList->begin(), "empty overflow line list?");
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
  line_iterator end = mLineList->end();
  if (mLine != end)
    return true;
  bool currentlyInOverflowLines = GetInOverflow();
  while (true) {
    if (currentlyInOverflowLines) {
      mFrame = static_cast<nsBlockFrame*>(mFrame->GetNextInFlow());
      if (!mFrame)
        return false;
      mLineList = &mFrame->mLines;
      mLine = mLineList->begin();
      if (mLine != mLineList->end())
        return true;
    } else {
      nsBlockFrame::FrameLines* overflowLines = mFrame->GetOverflowLines();
      if (overflowLines) {
        mLineList = &overflowLines->mLines;
        mLine = mLineList->begin();
        NS_ASSERTION(mLine != mLineList->end(), "empty overflow line list?");
        return true;
      }
    }
    currentlyInOverflowLines = !currentlyInOverflowLines;
  }
}

static void RemoveBlockChild(nsIFrame* aFrame,
                             bool      aRemoveOnlyFluidContinuations)
{
  if (!aFrame) {
    return;
  }
  nsBlockFrame* nextBlock = nsLayoutUtils::GetAsBlock(aFrame->GetParent());
  NS_ASSERTION(nextBlock,
               "Our child's continuation's parent is not a block?");
  nextBlock->DoRemoveFrame(aFrame,
      (aRemoveOnlyFluidContinuations ? 0 : nsBlockFrame::REMOVE_FIXED_CONTINUATIONS));
}








void
nsBlockFrame::DoRemoveFrame(nsIFrame* aDeletedFrame, uint32_t aFlags)
{
  
  ClearLineCursor();

  if (aDeletedFrame->GetStateBits() &
      (NS_FRAME_OUT_OF_FLOW | NS_FRAME_IS_OVERFLOW_CONTAINER)) {
    if (!aDeletedFrame->GetPrevInFlow()) {
      NS_ASSERTION(aDeletedFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW,
                   "Expected out-of-flow frame");
      DoRemoveOutOfFlowFrame(aDeletedFrame);
    }
    else {
      nsContainerFrame::DeleteNextInFlowChild(aDeletedFrame,
                                              (aFlags & FRAMES_ARE_EMPTY) != 0);
    }
    return;
  }

  
  nsLineList::iterator line_start = mLines.begin(),
                       line_end = mLines.end();
  nsLineList::iterator line = line_start;
  FrameLines* overflowLines = nullptr;
  bool searchingOverflowList = false;
  
  
  TryAllLines(&line, &line_start, &line_end, &searchingOverflowList,
              &overflowLines);
  while (line != line_end) {
    if (line->Contains(aDeletedFrame)) {
      break;
    }
    ++line;
    TryAllLines(&line, &line_start, &line_end, &searchingOverflowList,
                &overflowLines);
  }

  if (line == line_end) {
    NS_ERROR("can't find deleted frame in lines");
    return;
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

  while (line != line_end && aDeletedFrame) {
    NS_ASSERTION(this == aDeletedFrame->GetParent(), "messed up delete code");
    NS_ASSERTION(line->Contains(aDeletedFrame), "frame not in line");

    if (!(aFlags & FRAMES_ARE_EMPTY)) {
      line->MarkDirty();
      line->SetInvalidateTextRuns(true);
    }

    
    
    bool isLastFrameOnLine = 1 == line->GetChildCount();
    if (!isLastFrameOnLine) {
      line_iterator next = line.next();
      nsIFrame* lastFrame = next != line_end ?
        next->mFirstChild->GetPrevSibling() :
        (searchingOverflowList ? overflowLines->mFrames.LastChild() : 
                                 mFrames.LastChild());
      NS_ASSERTION(next == line_end || lastFrame == line->LastChild(),
                   "unexpected line frames");
      isLastFrameOnLine = lastFrame == aDeletedFrame;
    }

    
    if (line->mFirstChild == aDeletedFrame) {
      
      
      
      line->mFirstChild = aDeletedFrame->GetNextSibling();
    }

    
    
    --line;
    if (line != line_end && !line->IsBlock()) {
      
      
      line->MarkDirty();
    }
    ++line;

    
    
    
    if (searchingOverflowList) {
      overflowLines->mFrames.RemoveFrame(aDeletedFrame);
    } else {
      mFrames.RemoveFrame(aDeletedFrame);
    }

    
    line->NoteFrameRemoved(aDeletedFrame);

    
    
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
      deletedNextContinuation->GetParent()->
        DeleteNextInFlowChild(deletedNextContinuation, false);
      deletedNextContinuation = nullptr;
    }

    aDeletedFrame->Destroy();
    aDeletedFrame = deletedNextContinuation;

    bool haveAdvancedToNextLine = false;
    
    if (0 == line->GetChildCount()) {
#ifdef NOISY_REMOVE_FRAME
        printf("DoRemoveFrame: %s line=%p became empty so it will be removed\n",
               searchingOverflowList?"overflow":"normal", line.get());
#endif
      nsLineBox *cur = line;
      if (!searchingOverflowList) {
        line = mLines.erase(line);
        
        
        
        
#ifdef NOISY_BLOCK_INVALIDATE
        nsRect visOverflow(cur->GetVisualOverflowArea());
        printf("%p invalidate 10 (%d, %d, %d, %d)\n",
               this, visOverflow.x, visOverflow.y,
               visOverflow.width, visOverflow.height);
#endif
      } else {
        line = overflowLines->mLines.erase(line);
        if (overflowLines->mLines.empty()) {
          DestroyOverflowLines();
          overflowLines = nullptr;
          
          
          
          line_start = mLines.begin();
          line_end = mLines.end();
          line = line_end;
        }
      }
      FreeLineBox(cur);

      
      
      
      
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

        TryAllLines(&line, &line_start, &line_end, &searchingOverflowList,
                    &overflowLines);
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
  VerifyOverflowSituation();
#endif

  
  RemoveBlockChild(aDeletedFrame, !(aFlags & REMOVE_FIXED_CONTINUATIONS));
}

static bool
FindBlockLineFor(nsIFrame*             aChild,
                 nsLineList::iterator  aBegin,
                 nsLineList::iterator  aEnd,
                 nsLineList::iterator* aResult)
{
  MOZ_ASSERT(aChild->IsBlockOutside());
  for (nsLineList::iterator line = aBegin; line != aEnd; ++line) {
    MOZ_ASSERT(line->GetChildCount() > 0);
    if (line->IsBlock() && line->mFirstChild == aChild) {
      MOZ_ASSERT(line->GetChildCount() == 1);
      *aResult = line;
      return true;
    }
  }
  return false;
}

static bool
FindInlineLineFor(nsIFrame*             aChild,
                  const nsFrameList&    aFrameList,
                  nsLineList::iterator  aBegin,
                  nsLineList::iterator  aEnd,
                  nsLineList::iterator* aResult)
{
  MOZ_ASSERT(!aChild->IsBlockOutside());
  for (nsLineList::iterator line = aBegin; line != aEnd; ++line) {
    MOZ_ASSERT(line->GetChildCount() > 0);
    if (!line->IsBlock()) {
      
      nsLineList::iterator next = line.next();
      if (aChild == (next == aEnd ? aFrameList.LastChild()
                                  : next->mFirstChild->GetPrevSibling()) ||
          line->Contains(aChild)) {
        *aResult = line;
        return true;
      }
    }
  }
  return false;
}

static bool
FindLineFor(nsIFrame*             aChild,
            const nsFrameList&    aFrameList,
            nsLineList::iterator  aBegin,
            nsLineList::iterator  aEnd,
            nsLineList::iterator* aResult)
{
  return aChild->IsBlockOutside() ?
    FindBlockLineFor(aChild, aBegin, aEnd, aResult) :
    FindInlineLineFor(aChild, aFrameList, aBegin, aEnd, aResult);
}

nsresult
nsBlockFrame::StealFrame(nsIFrame* aChild,
                         bool      aForceNormal)
{
  MOZ_ASSERT(aChild->GetParent() == this);

  if ((aChild->GetStateBits() & NS_FRAME_OUT_OF_FLOW) &&
      aChild->IsFloating()) {
    RemoveFloat(aChild);
    return NS_OK;
  }

  if ((aChild->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)
      && !aForceNormal) {
    return nsContainerFrame::StealFrame(aChild);
  }

  MOZ_ASSERT(!(aChild->GetStateBits() & NS_FRAME_OUT_OF_FLOW));

  nsLineList::iterator line;
  if (FindLineFor(aChild, mFrames, mLines.begin(), mLines.end(), &line)) {
    RemoveFrameFromLine(aChild, line, mFrames, mLines);
  } else {
    FrameLines* overflowLines = GetOverflowLines();
    DebugOnly<bool> found;
    found = FindLineFor(aChild, overflowLines->mFrames,
                        overflowLines->mLines.begin(),
                        overflowLines->mLines.end(), &line);
    MOZ_ASSERT(found);
    RemoveFrameFromLine(aChild, line, overflowLines->mFrames,
                        overflowLines->mLines);
    if (overflowLines->mLines.empty()) {
      DestroyOverflowLines();
    }
  }

  return NS_OK;
}

void
nsBlockFrame::RemoveFrameFromLine(nsIFrame* aChild, nsLineList::iterator aLine,
                                  nsFrameList& aFrameList, nsLineList& aLineList)
{
  aFrameList.RemoveFrame(aChild);
  if (aChild == aLine->mFirstChild) {
    aLine->mFirstChild = aChild->GetNextSibling();
  }
  aLine->NoteFrameRemoved(aChild);
  if (aLine->GetChildCount() > 0) {
    aLine->MarkDirty();
  } else {
    
    nsLineBox* lineBox = aLine;
    aLine = aLineList.erase(aLine);
    if (aLine != aLineList.end()) {
      aLine->MarkPreviousMarginDirty();
    }
    FreeLineBox(lineBox);
  }
}

void
nsBlockFrame::DeleteNextInFlowChild(nsIFrame* aNextInFlow,
                                    bool      aDeletingEmptyFrames)
{
  NS_PRECONDITION(aNextInFlow->GetPrevInFlow(), "bad next-in-flow");

  if (aNextInFlow->GetStateBits() &
      (NS_FRAME_OUT_OF_FLOW | NS_FRAME_IS_OVERFLOW_CONTAINER)) {
    nsContainerFrame::DeleteNextInFlowChild(aNextInFlow, aDeletingEmptyFrames);
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

const nsStyleText*
nsBlockFrame::StyleTextForLineLayout()
{
  
  return StyleText();
}




LogicalRect
nsBlockFrame::AdjustFloatAvailableSpace(nsBlockReflowState& aState,
                                        const LogicalRect& aFloatAvailableSpace,
                                        nsIFrame* aFloatFrame)
{
  
  
  nscoord availISize;
  const nsStyleDisplay* floatDisplay = aFloatFrame->StyleDisplay();
  WritingMode wm = aState.mReflowState.GetWritingMode();

  if (NS_STYLE_DISPLAY_TABLE != floatDisplay->mDisplay ||
      eCompatibility_NavQuirks != aState.mPresContext->CompatibilityMode() ) {
    availISize = aState.ContentISize();
  }
  else {
    
    
    
    
    availISize = aFloatAvailableSpace.ISize(wm);
  }

  nscoord availBSize = NS_UNCONSTRAINEDSIZE == aState.ContentBSize()
                       ? NS_UNCONSTRAINEDSIZE
                       : std::max(0, aState.ContentBEnd() - aState.mBCoord);

  if (availBSize != NS_UNCONSTRAINEDSIZE &&
      !aState.GetFlag(BRS_FLOAT_FRAGMENTS_INSIDE_COLUMN_ENABLED) &&
      nsLayoutUtils::GetClosestFrameOfType(this, nsGkAtoms::columnSetFrame)) {
    
    
    
    
    availBSize = NS_UNCONSTRAINEDSIZE;
  }

  return LogicalRect(wm, aState.ContentIStart(), aState.ContentBStart(),
                     availISize, availBSize);
}

nscoord
nsBlockFrame::ComputeFloatISize(nsBlockReflowState& aState,
                                const LogicalRect&  aFloatAvailableSpace,
                                nsIFrame*           aFloat)
{
  NS_PRECONDITION(aFloat->GetStateBits() & NS_FRAME_OUT_OF_FLOW,
                  "aFloat must be an out-of-flow frame");
  
  LogicalRect availSpace = AdjustFloatAvailableSpace(aState,
                                                     aFloatAvailableSpace,
                                                     aFloat);

  WritingMode blockWM = aState.mReflowState.GetWritingMode();
  WritingMode floatWM = aFloat->GetWritingMode();
  nsHTMLReflowState
    floatRS(aState.mPresContext, aState.mReflowState, aFloat,
            availSpace.Size(blockWM).ConvertTo(floatWM, blockWM));

  return floatRS.ComputedSizeWithMarginBorderPadding(blockWM).ISize(blockWM);
}

void
nsBlockFrame::ReflowFloat(nsBlockReflowState& aState,
                          const LogicalRect&  aAdjustedAvailableSpace,
                          nsIFrame*           aFloat,
                          LogicalMargin&      aFloatMargin,
                          LogicalMargin&      aFloatOffsets,
                          bool                aFloatPushedDown,
                          nsReflowStatus&     aReflowStatus)
{
  NS_PRECONDITION(aFloat->GetStateBits() & NS_FRAME_OUT_OF_FLOW,
                  "aFloat must be an out-of-flow frame");
  
  aReflowStatus = NS_FRAME_COMPLETE;

  WritingMode wm = aState.mReflowState.GetWritingMode();
#ifdef NOISY_FLOAT
  printf("Reflow Float %p in parent %p, availSpace(%d,%d,%d,%d)\n",
         aFloat, this,
         aFloatAvailableSpace.IStart(wm), aFloatAvailableSpace.BStart(wm),
         aFloatAvailableSpace.ISize(wm), aFloatAvailableSpace.BSize(wm)
  );
#endif

  nsHTMLReflowState
    floatRS(aState.mPresContext, aState.mReflowState, aFloat,
            aAdjustedAvailableSpace.Size(wm).ConvertTo(aFloat->GetWritingMode(),
                                                       wm));

  
  
  
  
  
  
  
  
  if (floatRS.mFlags.mIsTopOfPage &&
      (aFloatPushedDown ||
       aAdjustedAvailableSpace.ISize(wm) != aState.ContentISize())) {
    floatRS.mFlags.mIsTopOfPage = false;
  }

  
  nsBlockReflowContext brc(aState.mPresContext, aState.mReflowState);

  
  bool isAdjacentWithTop = aState.IsAdjacentWithTop();

  nsIFrame* clearanceFrame = nullptr;
  do {
    nsCollapsingMargin margin;
    bool mayNeedRetry = false;
    floatRS.mDiscoveredClearance = nullptr;
    
    if (!aFloat->GetPrevInFlow()) {
      brc.ComputeCollapsedBStartMargin(floatRS, &margin,
                                       clearanceFrame,
                                       &mayNeedRetry);

      if (mayNeedRetry && !clearanceFrame) {
        floatRS.mDiscoveredClearance = &clearanceFrame;
        
        
      }
    }

    brc.ReflowBlock(aAdjustedAvailableSpace, true, margin,
                    0, isAdjacentWithTop,
                    nullptr, floatRS,
                    aReflowStatus, aState);
  } while (clearanceFrame);

  if (!NS_FRAME_IS_FULLY_COMPLETE(aReflowStatus) &&
      ShouldAvoidBreakInside(floatRS)) {
    aReflowStatus = NS_INLINE_LINE_BREAK_BEFORE();
  } else if (NS_FRAME_IS_NOT_COMPLETE(aReflowStatus) &&
             (NS_UNCONSTRAINEDSIZE == aAdjustedAvailableSpace.BSize(wm))) {
    
    
    aReflowStatus = NS_FRAME_COMPLETE;
  }

  if (aReflowStatus & NS_FRAME_REFLOW_NEXTINFLOW) {
    aState.mReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
  }

  if (aFloat->GetType() == nsGkAtoms::letterFrame) {
    
    
    
    if (NS_FRAME_IS_NOT_COMPLETE(aReflowStatus)) 
      aReflowStatus = NS_FRAME_COMPLETE;
  }

  
  aFloatMargin =
    
    floatRS.ComputedLogicalMargin().ConvertTo(wm, floatRS.GetWritingMode());
  aFloatOffsets =
    floatRS.ComputedLogicalOffsets().ConvertTo(wm, floatRS.GetWritingMode());

  const nsHTMLReflowMetrics& metrics = brc.GetMetrics();

  
  
  
  
  
  
  WritingMode metricsWM = metrics.GetWritingMode();
  aFloat->SetSize(metricsWM, metrics.Size(metricsWM));
  if (aFloat->HasView()) {
    nsContainerFrame::SyncFrameViewAfterReflow(aState.mPresContext, aFloat,
                                               aFloat->GetView(),
                                               metrics.VisualOverflow(),
                                               NS_FRAME_NO_MOVE_VIEW);
  }
  
  aFloat->DidReflow(aState.mPresContext, &floatRS,
                    nsDidReflowStatus::FINISHED);

#ifdef NOISY_FLOAT
  printf("end ReflowFloat %p, sized to %d,%d\n",
         aFloat, metrics.Width(), metrics.Height());
#endif
}

uint8_t
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

void
nsBlockFrame::ReflowPushedFloats(nsBlockReflowState& aState,
                                 nsOverflowAreas&    aOverflowAreas,
                                 nsReflowStatus&     aStatus)
{
  
  
  nsIFrame* f = mFloats.FirstChild();
  nsIFrame* prev = nullptr;
  while (f && (f->GetStateBits() & NS_FRAME_IS_PUSHED_FLOAT)) {
    MOZ_ASSERT(prev == f->GetPrevSibling());
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    nsIFrame *prevContinuation = f->GetPrevContinuation();
    if (prevContinuation && prevContinuation->GetParent() == f->GetParent()) {
      mFloats.RemoveFrame(f);
      aState.AppendPushedFloatChain(f);
      f = !prev ? mFloats.FirstChild() : prev->GetNextSibling();
      continue;
    }

    
    
    aState.FlowAndPlaceFloat(f);
    ConsiderChildOverflow(aOverflowAreas, f);

    nsIFrame* next = !prev ? mFloats.FirstChild() : prev->GetNextSibling();
    if (next == f) {
      
      next = f->GetNextSibling();
      prev = f;
    } 
    f = next;
  }

  
  if (0 != aState.ClearFloats(0, NS_STYLE_CLEAR_BOTH)) {
    aState.mFloatBreakType = static_cast<nsBlockFrame*>(GetPrevInFlow())
                               ->FindTrailingClear();
  }
}

void
nsBlockFrame::RecoverFloats(nsFloatManager& aFloatManager, WritingMode aWM,
                            nscoord aContainerWidth)
{
  
  nsIFrame* stop = nullptr; 
                           
  for (nsIFrame* f = mFloats.FirstChild(); f && f != stop; f = f->GetNextSibling()) {
    LogicalRect region = nsFloatManager::GetRegionFor(aWM, f, aContainerWidth);
    aFloatManager.AddFloat(f, region, aWM, aContainerWidth);
    if (!stop && f->GetNextInFlow())
      stop = f->GetNextInFlow();
  }

  
  for (nsIFrame* oc = GetFirstChild(kOverflowContainersList);
       oc; oc = oc->GetNextSibling()) {
    RecoverFloatsFor(oc, aFloatManager, aWM, aContainerWidth);
  }

  
  for (nsBlockFrame::line_iterator line = begin_lines(); line != end_lines(); ++line) {
    if (line->IsBlock()) {
      RecoverFloatsFor(line->mFirstChild, aFloatManager, aWM, aContainerWidth);
    }
  }
}

void
nsBlockFrame::RecoverFloatsFor(nsIFrame*       aFrame,
                               nsFloatManager& aFloatManager,
                               WritingMode     aWM,
                               nscoord         aContainerWidth)
{
  NS_PRECONDITION(aFrame, "null frame");
  
  nsBlockFrame* block = nsLayoutUtils::GetAsBlock(aFrame);
  
  
  
  if (block && !nsBlockFrame::BlockNeedsFloatManager(block)) {
    
    
    

    LogicalRect rect(aWM, block->GetNormalRect(), aContainerWidth);
    nscoord lineLeft = rect.LineLeft(aWM, aContainerWidth);
    nscoord blockStart = rect.BStart(aWM);
    aFloatManager.Translate(lineLeft, blockStart);
    block->RecoverFloats(aFloatManager, aWM, aContainerWidth);
    aFloatManager.Translate(-lineLeft, -blockStart);
  }
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
  if (mContent->IsAnyOfHTMLElements(nsGkAtoms::html, nsGkAtoms::body))
    return true;

  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mContent));
  bool visible;
  nsresult rv = aSelection->ContainsNode(node, true, &visible);
  return NS_SUCCEEDED(rv) && visible;
}

#ifdef DEBUG
static void DebugOutputDrawLine(int32_t aDepth, nsLineBox* aLine, bool aDrawn) {
  if (nsBlockFrame::gNoisyDamageRepair) {
    nsFrame::IndentBy(stdout, aDepth+1);
    nsRect lineArea = aLine->GetVisualOverflowArea();
    printf("%s line=%p bounds=%d,%d,%d,%d ca=%d,%d,%d,%d\n",
           aDrawn ? "draw" : "skip",
           static_cast<void*>(aLine),
           aLine->IStart(), aLine->BStart(),
           aLine->ISize(), aLine->BSize(),
           lineArea.x, lineArea.y,
           lineArea.width, lineArea.height);
  }
}
#endif

static void
DisplayLine(nsDisplayListBuilder* aBuilder, const nsRect& aLineArea,
            const nsRect& aDirtyRect, nsBlockFrame::line_iterator& aLine,
            int32_t aDepth, int32_t& aDrawnLines, const nsDisplayListSet& aLists,
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
    return;

  
  
  
  nsDisplayListCollection collection;

  
  
  nsDisplayListSet childLists(collection,
    lineInline ? collection.Content() : collection.BlockBorderBackgrounds());

  uint32_t flags = lineInline ? nsIFrame::DISPLAY_CHILD_INLINE : 0;

  nsIFrame* kid = aLine->mFirstChild;
  int32_t n = aLine->GetChildCount();
  while (--n >= 0) {
    aFrame->BuildDisplayListForChild(aBuilder, kid, aDirtyRect,
                                     childLists, flags);
    kid = kid->GetNextSibling();
  }
  
  if (lineMayHaveTextOverflow) {
    aTextOverflow->ProcessLine(collection, aLine.get());
  }

  collection.MoveTo(aLists);
}

void
nsBlockFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                               const nsRect&           aDirtyRect,
                               const nsDisplayListSet& aLists)
{
  int32_t drawnLines; 
  int32_t depth = 0;
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
  PRTime start = 0; 
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
    TextOverflow::WillProcessLines(aBuilder, this));

  
  nsDisplayListCollection linesDisplayListCollection;

  
  
  
  
  
  
  
  nsLineBox* cursor = aBuilder->ShouldDescendIntoFrame(this) ?
    nullptr : GetFirstLineContaining(aDirtyRect.y);
  line_iterator line_end = end_lines();
  
  if (cursor) {
    for (line_iterator line = mLines.begin(cursor);
         line != line_end;
         ++line) {
      nsRect lineArea = line->GetVisualOverflowArea();
      if (!lineArea.IsEmpty()) {
        
        
        if (lineArea.y >= aDirtyRect.YMost()) {
          break;
        }
        DisplayLine(aBuilder, lineArea, aDirtyRect, line, depth, drawnLines,
                    linesDisplayListCollection, this, textOverflow);
      }
    }
  } else {
    bool nonDecreasingYs = true;
    int32_t lineCount = 0;
    nscoord lastY = INT32_MIN;
    nscoord lastYMost = INT32_MIN;
    for (line_iterator line = begin_lines();
         line != line_end;
         ++line) {
      nsRect lineArea = line->GetVisualOverflowArea();
      DisplayLine(aBuilder, lineArea, aDirtyRect, line, depth, drawnLines,
                  linesDisplayListCollection, this, textOverflow);
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

    if (nonDecreasingYs && lineCount >= MIN_LINES_NEEDING_CURSOR) {
      SetupLineCursor();
    }
  }

  
  
  
  
  if (textOverflow) {
    aLists.PositionedDescendants()->AppendToTop(&textOverflow->GetMarkers());
  }
  linesDisplayListCollection.MoveTo(aLists);

  if (HasOutsideBullet()) {
    
    nsIFrame* bullet = GetOutsideBullet();
    BuildDisplayListForChild(aBuilder, bullet, aDirtyRect, aLists);
  }

#ifdef DEBUG
  if (gLamePaintMetrics) {
    PRTime end = PR_Now();

    int32_t numLines = mLines.size();
    if (!numLines) numLines = 1;
    PRTime lines, deltaPerLine, delta;
    lines = int64_t(numLines);
    delta = end - start;
    deltaPerLine = delta / lines;

    ListTag(stdout);
    char buf[400];
    PR_snprintf(buf, sizeof(buf),
                ": %lld elapsed (%lld per line) lines=%d drawn=%d skip=%d",
                delta, deltaPerLine,
                numLines, drawnLines, numLines - drawnLines);
    printf("%s\n", buf);
  }
#endif
}

#ifdef ACCESSIBILITY
a11y::AccType
nsBlockFrame::AccessibleType()
{
  if (IsTableCaption()) {
    return GetRect().IsEmpty() ? a11y::eNoType : a11y::eHTMLCaptionType;
  }

  
  if (mContent->IsHTMLElement(nsGkAtoms::hr)) {
    return a11y::eHTMLHRType;
  }

  if (!HasBullet() || !PresContext()) {
    
    if (!mContent->GetParent()) {
      
      
      return a11y::eNoType;
    }
    
    nsCOMPtr<nsIDOMHTMLDocument> htmlDoc =
      do_QueryInterface(mContent->GetComposedDoc());
    if (htmlDoc) {
      nsCOMPtr<nsIDOMHTMLElement> body;
      htmlDoc->GetBody(getter_AddRefs(body));
      if (SameCOMIdentity(body, mContent)) {
        
        
        return a11y::eNoType;
      }
    }

    
    return a11y::eHyperTextType;
  }

  
  return a11y::eHTMLLiType;
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
    return nullptr;
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
      aChild->IsAbsolutelyPositioned()) {
    
  } else if (aChild == GetOutsideBullet()) {
    
    
    
    line_iterator bulletLine = begin_lines();
    if (bulletLine != end_lines() && bulletLine->BSize() == 0 &&
        bulletLine != mLines.back()) {
      bulletLine = bulletLine.next();
    }
    
    if (bulletLine != end_lines()) {
      MarkLineDirty(bulletLine, &mLines);
    }
    
    
  } else {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (!(aChild->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
      AddStateBits(NS_BLOCK_LOOK_FOR_DIRTY_FRAMES);
    } else {
      NS_ASSERTION(aChild->IsFloating(), "should be a float");
      nsIFrame *thisFC = FirstContinuation();
      nsIFrame *placeholderPath =
        PresContext()->FrameManager()->GetPlaceholderFrameFor(aChild);
      
      
      
      if (placeholderPath) {
        for (;;) {
          nsIFrame *parent = placeholderPath->GetParent();
          if (parent->GetContent() == mContent &&
              parent->FirstContinuation() == thisFC) {
            parent->AddStateBits(NS_BLOCK_LOOK_FOR_DIRTY_FRAMES);
            break;
          }
          placeholderPath = parent;
        }
        placeholderPath->AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
      }
    }
  }

  nsBlockFrameSuper::ChildIsDirty(aChild);
}

void
nsBlockFrame::Init(nsIContent*       aContent,
                   nsContainerFrame* aParent,
                   nsIFrame*         aPrevInFlow)
{
  if (aPrevInFlow) {
    
    SetFlags(aPrevInFlow->GetStateBits() &
             (NS_BLOCK_FLAGS_MASK & ~NS_BLOCK_FLAGS_NON_INHERITED_MASK));
  }

  nsBlockFrameSuper::Init(aContent, aParent, aPrevInFlow);

  if (!aPrevInFlow ||
      aPrevInFlow->GetStateBits() & NS_BLOCK_NEEDS_BIDI_RESOLUTION) {
    AddStateBits(NS_BLOCK_NEEDS_BIDI_RESOLUTION);
  }

  
  
  
  
  
  if (GetParent() && StyleVisibility()->mWritingMode !=
                     GetParent()->StyleVisibility()->mWritingMode) {
    AddStateBits(NS_BLOCK_FLOAT_MGR | NS_BLOCK_MARGIN_ROOT);
  }

  if ((GetStateBits() &
       (NS_FRAME_FONT_INFLATION_CONTAINER | NS_BLOCK_FLOAT_MGR)) ==
      (NS_FRAME_FONT_INFLATION_CONTAINER | NS_BLOCK_FLOAT_MGR)) {
    AddStateBits(NS_FRAME_FONT_INFLATION_FLOW_ROOT);
  }
}

void
nsBlockFrame::SetInitialChildList(ChildListID     aListID,
                                  nsFrameList&    aChildList)
{
  NS_ASSERTION(aListID != kPrincipalList ||
               (GetStateBits() & (NS_BLOCK_FRAME_HAS_INSIDE_BULLET |
                                  NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET)) == 0,
               "how can we have a bullet already?");

  if (kAbsoluteList == aListID) {
    nsContainerFrame::SetInitialChildList(aListID, aChildList);
  }
  else if (kFloatList == aListID) {
    mFloats.SetFrames(aChildList);
  }
  else {
    nsPresContext* presContext = PresContext();

#ifdef DEBUG
    
    
    
    
    
    
    
    nsIAtom *pseudo = StyleContext()->GetPseudo();
    bool haveFirstLetterStyle =
      (!pseudo ||
       (pseudo == nsCSSAnonBoxes::cellContent &&
        GetParent()->StyleContext()->GetPseudo() == nullptr) ||
       pseudo == nsCSSAnonBoxes::fieldsetContent ||
       pseudo == nsCSSAnonBoxes::buttonContent ||
       pseudo == nsCSSAnonBoxes::columnContent ||
       (pseudo == nsCSSAnonBoxes::scrolledContent &&
        GetParent()->GetType() != nsGkAtoms::listControlFrame) ||
       pseudo == nsCSSAnonBoxes::mozSVGText) &&
      GetType() != nsGkAtoms::comboboxControlFrame &&
      !IsFrameOfType(eMathML) &&
      nsRefPtr<nsStyleContext>(GetFirstLetterStyle(presContext)) != nullptr;
    NS_ASSERTION(haveFirstLetterStyle ==
                 ((mState & NS_BLOCK_HAS_FIRST_LETTER_STYLE) != 0),
                 "NS_BLOCK_HAS_FIRST_LETTER_STYLE state out of sync");
#endif
    
    AddFrames(aChildList, nullptr);

    
    
    
    
    
    
    
    
    
    
    
    nsIFrame* possibleListItem = this;
    while (1) {
      nsIFrame* parent = possibleListItem->GetParent();
      if (parent->GetContent() != GetContent()) {
        break;
      }
      possibleListItem = parent;
    }
    if (NS_STYLE_DISPLAY_LIST_ITEM ==
          possibleListItem->StyleDisplay()->mDisplay &&
        !GetPrevInFlow()) {
      
      const nsStyleList* styleList = StyleList();
      CounterStyle* style = styleList->GetCounterStyle();
      nsCSSPseudoElements::Type pseudoType = style->IsBullet() ?
        nsCSSPseudoElements::ePseudo_mozListBullet :
        nsCSSPseudoElements::ePseudo_mozListNumber;

      nsIPresShell *shell = presContext->PresShell();

      nsStyleContext* parentStyle =
        CorrectStyleParentFrame(this,
          nsCSSPseudoElements::GetPseudoAtom(pseudoType))->StyleContext();
      nsRefPtr<nsStyleContext> kidSC = shell->StyleSet()->
        ResolvePseudoElementStyle(mContent->AsElement(), pseudoType,
                                  parentStyle, nullptr);

      
      nsBulletFrame* bullet = new (shell) nsBulletFrame(kidSC);
      bullet->Init(mContent, this, nullptr);

      
      
      if (NS_STYLE_LIST_STYLE_POSITION_INSIDE ==
            styleList->mListStylePosition) {
        nsFrameList bulletList(bullet, bullet);
        AddFrames(bulletList, nullptr);
        Properties().Set(InsideBulletProperty(), bullet);
        AddStateBits(NS_BLOCK_FRAME_HAS_INSIDE_BULLET);
      } else {
        nsFrameList* bulletList = new (shell) nsFrameList(bullet, bullet);
        Properties().Set(OutsideBulletProperty(), bulletList);
        AddStateBits(NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET);
      }
    }
  }
}

bool
nsBlockFrame::BulletIsEmpty() const
{
  NS_ASSERTION(mContent->GetPrimaryFrame()->StyleDisplay()->mDisplay ==
                 NS_STYLE_DISPLAY_LIST_ITEM && HasOutsideBullet(),
               "should only care when we have an outside bullet");
  const nsStyleList* list = StyleList();
  return list->GetCounterStyle()->IsNone() &&
         !list->GetListStyleImage();
}

void
nsBlockFrame::GetSpokenBulletText(nsAString& aText) const
{
  const nsStyleList* myList = StyleList();
  if (myList->GetListStyleImage()) {
    aText.Assign(kDiscCharacter);
    aText.Append(' ');
  } else {
    nsBulletFrame* bullet = GetBullet();
    if (bullet) {
      bullet->GetSpokenText(aText);
    } else {
      aText.Truncate();
    }
  }
}


bool
nsBlockFrame::FrameStartsCounterScope(nsIFrame* aFrame)
{
  nsIContent* content = aFrame->GetContent();
  if (!content || !content->IsHTMLElement())
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

  MOZ_ASSERT(mContent->IsHTMLElement(),
             "FrameStartsCounterScope should only return true for HTML elements");

  
  
  int32_t ordinal = 1;
  int32_t increment;
  if (mContent->IsHTMLElement(nsGkAtoms::ol) &&
      mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::reversed)) {
    increment = -1;
  } else {
    increment = 1;
  }

  nsGenericHTMLElement *hc = nsGenericHTMLElement::FromContent(mContent);
  
  
  MOZ_ASSERT(hc, "How is mContent not HTML?");
  const nsAttrValue* attr = hc->GetParsedAttr(nsGkAtoms::start);
  if (attr && attr->Type() == nsAttrValue::eInteger) {
    ordinal = attr->GetIntegerValue();
  } else if (increment < 0) {
    
    
    ordinal = 0;
    for (nsIContent* kid = mContent->GetFirstChild(); kid;
         kid = kid->GetNextSibling()) {
      if (kid->IsHTMLElement(nsGkAtoms::li)) {
        
        
        
        ordinal -= increment;
      }
    }
  }

  
  nsBlockFrame* block = static_cast<nsBlockFrame*>(FirstInFlow());
  return RenumberListsInBlock(aPresContext, block, &ordinal, 0, increment);
}

bool
nsBlockFrame::RenumberListsInBlock(nsPresContext* aPresContext,
                                   nsBlockFrame* aBlockFrame,
                                   int32_t* aOrdinal,
                                   int32_t aDepth,
                                   int32_t aIncrement)
{
  
  bool foundValidLine;
  nsBlockInFlowLineIterator bifLineIter(aBlockFrame, &foundValidLine);
  
  if (!foundValidLine)
    return false;

  bool renumberedABullet = false;

  do {
    nsLineList::iterator line = bifLineIter.GetLine();
    nsIFrame* kid = line->mFirstChild;
    int32_t n = line->GetChildCount();
    while (--n >= 0) {
      bool kidRenumberedABullet = RenumberListsFor(aPresContext, kid, aOrdinal,
                                                   aDepth, aIncrement);
      if (kidRenumberedABullet) {
        line->MarkDirty();
        renumberedABullet = true;
      }
      kid = kid->GetNextSibling();
    }
  } while (bifLineIter.Next());

  
  
  
  
  
  if (renumberedABullet && aDepth != 0) {
    aBlockFrame->AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
  }

  return renumberedABullet;
}

bool
nsBlockFrame::RenumberListsFor(nsPresContext* aPresContext,
                               nsIFrame* aKid,
                               int32_t* aOrdinal,
                               int32_t aDepth,
                               int32_t aIncrement)
{
  NS_PRECONDITION(aPresContext && aKid && aOrdinal, "null params are immoral!");

  
  if (MAX_DEPTH_FOR_LIST_RENUMBERING < aDepth)
    return false;

  
  nsIFrame* kid = nsPlaceholderFrame::GetRealFrameFor(aKid);
  const nsStyleDisplay* display = kid->StyleDisplay();

  
  kid = kid->GetContentInsertionFrame();

  
  if (!kid)
    return false;

  bool kidRenumberedABullet = false;

  
  
  
  if (NS_STYLE_DISPLAY_LIST_ITEM == display->mDisplay) {
    
    
    nsBlockFrame* listItem = nsLayoutUtils::GetAsBlock(kid);
    if (listItem) {
      nsBulletFrame* bullet = listItem->GetBullet();
      if (bullet) {
        bool changed;
        *aOrdinal = bullet->SetListItemOrdinal(*aOrdinal, &changed, aIncrement);
        if (changed) {
          kidRenumberedABullet = true;

          
          
          
          
          
          bullet->AddStateBits(NS_FRAME_IS_DIRTY);
          nsIFrame *f = bullet;
          do {
            nsIFrame *parent = f->GetParent();
            parent->ChildIsDirty(f);
            f = parent;
          } while (f != listItem);
        }
      }

      
      
      
      bool meToo = RenumberListsInBlock(aPresContext, listItem, aOrdinal,
                                        aDepth + 1, aIncrement);
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
        kidRenumberedABullet = RenumberListsInBlock(aPresContext, kidBlock,
                                                    aOrdinal, aDepth + 1,
                                                    aIncrement);
      }
    }
  }
  return kidRenumberedABullet;
}

void
nsBlockFrame::ReflowBullet(nsIFrame* aBulletFrame,
                           nsBlockReflowState& aState,
                           nsHTMLReflowMetrics& aMetrics,
                           nscoord aLineTop)
{
  const nsHTMLReflowState &rs = aState.mReflowState;

  
  WritingMode bulletWM = aBulletFrame->GetWritingMode();
  LogicalSize availSize(bulletWM);
  
  availSize.ISize(bulletWM) = aState.ContentISize();
  availSize.BSize(bulletWM) = NS_UNCONSTRAINEDSIZE;

  
  
  
  nsHTMLReflowState reflowState(aState.mPresContext, rs,
                                aBulletFrame, availSize);
  nsReflowStatus  status;
  aBulletFrame->Reflow(aState.mPresContext, aMetrics, reflowState, status);

  
  
  
  
  
  LogicalRect floatAvailSpace =
    aState.GetFloatAvailableSpaceWithState(aLineTop,
                                           &aState.mFloatManagerStateBefore)
          .mRect;
  
  

  
  
  
  
  
  
  
  

  
  
  
  WritingMode wm = rs.GetWritingMode();
  
  
  LogicalMargin bulletMargin =
    reflowState.ComputedLogicalMargin().ConvertTo(wm, bulletWM);
  nscoord iStart = floatAvailSpace.IStart(wm) -
                   rs.ComputedLogicalBorderPadding().IStart(wm) -
                   bulletMargin.IEnd(wm) -
                   aMetrics.ISize(wm);

  
  
  
  nscoord bStart = floatAvailSpace.BStart(wm);
  aBulletFrame->SetRect(wm, LogicalRect(wm, iStart, bStart,
                                        aMetrics.ISize(wm),
                                        aMetrics.BSize(wm)),
                        aState.ContainerWidth());
  aBulletFrame->DidReflow(aState.mPresContext, &aState.mReflowState,
                          nsDidReflowStatus::FINISHED);
}






void
nsBlockFrame::DoCollectFloats(nsIFrame* aFrame, nsFrameList& aList,
                              bool aCollectSiblings)
{
  while (aFrame) {
    
    if (!aFrame->IsFloatContainingBlock()) {
      nsIFrame *outOfFlowFrame =
        aFrame->GetType() == nsGkAtoms::placeholderFrame ?
          nsLayoutUtils::GetFloatFromPlaceholder(aFrame) : nullptr;
      while (outOfFlowFrame && outOfFlowFrame->GetParent() == this) {
        RemoveFloat(outOfFlowFrame);
        aList.AppendFrame(nullptr, outOfFlowFrame);
        outOfFlowFrame = outOfFlowFrame->GetNextInFlow();
        
        
        
        
      }

      DoCollectFloats(aFrame->GetFirstPrincipalChild(), aList, true);
      DoCollectFloats(aFrame->GetFirstChild(kOverflowList), aList, true);
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
  uint32_t i = 0;
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

void
nsBlockFrame::IsMarginRoot(bool* aBStartMarginRoot, bool* aBEndMarginRoot)
{
  if (!(GetStateBits() & NS_BLOCK_MARGIN_ROOT)) {
    nsIFrame* parent = GetParent();
    if (!parent || parent->IsFloatContainingBlock()) {
      *aBStartMarginRoot = false;
      *aBEndMarginRoot = false;
      return;
    }
    if (parent->GetType() == nsGkAtoms::columnSetFrame) {
      *aBStartMarginRoot = GetPrevInFlow() == nullptr;
      *aBEndMarginRoot = GetNextInFlow() == nullptr;
      return;
    }
  }

  *aBStartMarginRoot = true;
  *aBEndMarginRoot = true;
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






nsBlockFrame::ReplacedElementISizeToClear
nsBlockFrame::ISizeToClearPastFloats(nsBlockReflowState& aState,
                                     const LogicalRect& aFloatAvailableSpace,
                                     nsIFrame* aFrame)
{
  nscoord inlineStartOffset, inlineEndOffset;
  WritingMode wm = aState.mReflowState.GetWritingMode();
  nsCSSOffsetState offsetState(aFrame, aState.mReflowState.rendContext,
                               aState.mContentArea.Width(wm));

  ReplacedElementISizeToClear result;
  aState.ComputeReplacedBlockOffsetsForFloats(aFrame, aFloatAvailableSpace,
                                              inlineStartOffset,
                                              inlineEndOffset);
  nscoord availISize = aState.mContentArea.ISize(wm) -
                       inlineStartOffset - inlineEndOffset;

  
  
  
  
  
  
  WritingMode frWM = aFrame->GetWritingMode();
  LogicalSize availSpace = LogicalSize(wm, availISize, NS_UNCONSTRAINEDSIZE).
                             ConvertTo(frWM, wm);
  nsHTMLReflowState reflowState(aState.mPresContext, aState.mReflowState,
                                aFrame, availSpace);
  result.borderBoxISize =
    reflowState.ComputedSizeWithBorderPadding().ConvertTo(wm, frWM).ISize(wm);
  
  
  LogicalMargin computedMargin =
    offsetState.ComputedLogicalMargin().ConvertTo(wm, frWM);
  result.marginIStart = computedMargin.IStart(wm);
  result.marginIEnd = computedMargin.IEnd(wm);
  return result;
}
 

nsBlockFrame*
nsBlockFrame::GetNearestAncestorBlock(nsIFrame* aCandidate)
{
  nsBlockFrame* block = nullptr;
  while(aCandidate) {
    block = nsLayoutUtils::GetAsBlock(aCandidate);
    if (block) { 
      
      return block;
    }
    
    aCandidate = aCandidate->GetParent();
  }
  NS_NOTREACHED("Fell off frame tree looking for ancestor block!");
  return nullptr;
}

void
nsBlockFrame::ComputeFinalBSize(const nsHTMLReflowState& aReflowState,
                                nsReflowStatus*          aStatus,
                                nscoord                  aContentBSize,
                                const LogicalMargin&     aBorderPadding,
                                LogicalSize&             aFinalSize,
                                nscoord                  aConsumed)
{
  WritingMode wm = aReflowState.GetWritingMode();
  
  
  nscoord computedBSizeLeftOver = GetEffectiveComputedBSize(aReflowState,
                                                            aConsumed);
  NS_ASSERTION(!( IS_TRUE_OVERFLOW_CONTAINER(this)
                  && computedBSizeLeftOver ),
               "overflow container must not have computedBSizeLeftOver");

  aFinalSize.BSize(wm) =
    NSCoordSaturatingAdd(NSCoordSaturatingAdd(aBorderPadding.BStart(wm),
                                              computedBSizeLeftOver),
                         aBorderPadding.BEnd(wm));

  if (NS_FRAME_IS_NOT_COMPLETE(*aStatus) &&
      aFinalSize.BSize(wm) < aReflowState.AvailableBSize()) {
    
    
    
    NS_FRAME_SET_OVERFLOW_INCOMPLETE(*aStatus);
  }

  if (NS_FRAME_IS_COMPLETE(*aStatus)) {
    if (computedBSizeLeftOver > 0 &&
        NS_UNCONSTRAINEDSIZE != aReflowState.AvailableBSize() &&
        aFinalSize.BSize(wm) > aReflowState.AvailableBSize()) {
      if (ShouldAvoidBreakInside(aReflowState)) {
        *aStatus = NS_INLINE_LINE_BREAK_BEFORE();
        return;
      }
      
      
      
      
      
      aFinalSize.BSize(wm) = std::max(aReflowState.AvailableBSize(),
                                      aContentBSize);
      NS_FRAME_SET_INCOMPLETE(*aStatus);
      if (!GetNextInFlow())
        *aStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
    }
  }
}

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

  nsLineBox* cursor = GetLineCursor();

  
  
  int32_t count = 0;
  line_iterator line, line_end;
  for (line = begin_lines(), line_end = end_lines();
       line != line_end;
       ++line) {
    if (line == cursor) {
      cursor = nullptr;
    }
    if (aFinalCheckOK) {
      MOZ_ASSERT(line->GetChildCount(), "empty line");
      if (line->IsBlock()) {
        NS_ASSERTION(1 == line->GetChildCount(), "bad first line");
      }
    }
    count += line->GetChildCount();
  }

  
  int32_t frameCount = 0;
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

  if (cursor) {
    FrameLines* overflowLines = GetOverflowLines();
    if (overflowLines) {
      line_iterator line = overflowLines->mLines.begin();
      line_iterator line_end = overflowLines->mLines.end();
      for (; line != line_end; ++line) {
        if (line == cursor) {
          cursor = nullptr;
          break;
        }
      }
    }
  }
  NS_ASSERTION(!cursor, "stale LineCursorProperty");
}

void
nsBlockFrame::VerifyOverflowSituation()
{
  
  nsFrameList* oofs = GetOverflowOutOfFlows() ;
  if (oofs) {
    for (nsFrameList::Enumerator e(*oofs); !e.AtEnd(); e.Next()) {
      nsIFrame* nif = e.get()->GetNextInFlow();
      MOZ_ASSERT(!nif || (!mFloats.ContainsFrame(nif) && !mFrames.ContainsFrame(nif)));
    }
  }

  
  oofs = GetPushedFloats();
  if (oofs) {
    for (nsFrameList::Enumerator e(*oofs); !e.AtEnd(); e.Next()) {
      nsIFrame* nif = e.get()->GetNextInFlow();
      MOZ_ASSERT(!nif || (!mFloats.ContainsFrame(nif) && !mFrames.ContainsFrame(nif)));
    }
  }

  
  
  nsIFrame::ChildListID childLists[] = { nsIFrame::kFloatList,
                                         nsIFrame::kPushedFloatsList };
  for (size_t i = 0; i < ArrayLength(childLists); ++i) {
    nsFrameList children(GetChildList(childLists[i]));
    for (nsFrameList::Enumerator e(children); !e.AtEnd(); e.Next()) {
      nsIFrame* parent = this;
      nsIFrame* nif = e.get()->GetNextInFlow();
      for (; nif; nif = nif->GetNextInFlow()) {
        bool found = false;
        for (nsIFrame* p = parent; p; p = p->GetNextInFlow()) {
          if (nif->GetParent() == p) {
            parent = p;
            found = true;
            break;
          }
        }
        MOZ_ASSERT(found, "next-in-flow is a child of parent earlier in the frame tree?");
      }
    }
  }

  nsBlockFrame* flow = static_cast<nsBlockFrame*>(FirstInFlow());
  while (flow) {
    FrameLines* overflowLines = flow->GetOverflowLines();
    if (overflowLines) {
      NS_ASSERTION(!overflowLines->mLines.empty(),
                   "should not be empty if present");
      NS_ASSERTION(overflowLines->mLines.front()->mFirstChild,
                   "bad overflow lines");
      NS_ASSERTION(overflowLines->mLines.front()->mFirstChild ==
                   overflowLines->mFrames.FirstChild(),
                   "bad overflow frames / lines");
    }
    nsLineBox* cursor = flow->GetLineCursor();
    if (cursor) {
      line_iterator line = flow->begin_lines();
      line_iterator line_end = flow->end_lines();
      for (; line != line_end && line != cursor; ++line)
        ;
      if (line == line_end && overflowLines) {
        line = overflowLines->mLines.begin();
        line_end = overflowLines->mLines.end();
        for (; line != line_end && line != cursor; ++line)
          ;
        }
      MOZ_ASSERT(line != line_end, "stale LineCursorProperty");
    }
    flow = static_cast<nsBlockFrame*>(flow->GetNextInFlow());
  }
}

int32_t
nsBlockFrame::GetDepth() const
{
  int32_t depth = 0;
  nsIFrame* parent = GetParent();
  while (parent) {
    parent = parent->GetParent();
    depth++;
  }
  return depth;
}
#endif
