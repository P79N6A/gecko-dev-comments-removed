




#include "nsBidiPresUtils.h"
#include "nsFontMetrics.h"
#include "nsGkAtoms.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsBidiUtils.h"
#include "nsCSSFrameConstructor.h"
#include "nsContainerFrame.h"
#include "nsInlineFrame.h"
#include "nsPlaceholderFrame.h"
#include "nsFirstLetterFrame.h"
#include "nsUnicodeProperties.h"
#include "nsTextFrame.h"
#include "nsBlockFrame.h"
#include "nsIFrameInlines.h"
#include "nsStyleStructInlines.h"
#include <algorithm>

#undef NOISY_BIDI
#undef REALLY_NOISY_BIDI

using namespace mozilla;

static const char16_t kSpace            = 0x0020;
static const char16_t kZWSP             = 0x200B;
static const char16_t kLineSeparator    = 0x2028;
static const char16_t kObjectSubstitute = 0xFFFC;
static const char16_t kLRE              = 0x202A;
static const char16_t kRLE              = 0x202B;
static const char16_t kLRO              = 0x202D;
static const char16_t kRLO              = 0x202E;
static const char16_t kPDF              = 0x202C;
static const char16_t kSeparators[] = {
  
  char16_t('\t'),
  char16_t('\r'),
  char16_t('\n'),
  char16_t(0xb),
  char16_t(0x1c),
  char16_t(0x1d),
  char16_t(0x1e),
  char16_t(0x1f),
  char16_t(0x85),
  char16_t(0x2029),
  char16_t(0)
};

#define NS_BIDI_CONTROL_FRAME ((nsIFrame*)0xfffb1d1)

struct BidiParagraphData {
  nsString            mBuffer;
  nsAutoTArray<char16_t, 16> mEmbeddingStack;
  nsTArray<nsIFrame*> mLogicalFrames;
  nsTArray<nsLineBox*> mLinePerFrame;
  nsDataHashtable<nsISupportsHashKey, int32_t> mContentToFrameIndex;
  bool                mIsVisual;
  bool                mReset;
  nsBidiLevel         mParaLevel;
  nsIContent*         mPrevContent;
  nsAutoPtr<nsBidi>   mBidiEngine;
  nsIFrame*           mPrevFrame;
  nsAutoPtr<BidiParagraphData> mSubParagraph;
  uint8_t             mParagraphDepth;

  void Init(nsBlockFrame *aBlockFrame)
  {
    mBidiEngine = new nsBidi();
    mPrevContent = nullptr;
    mParagraphDepth = 0;

    mParaLevel = nsBidiPresUtils::BidiLevelFromStyle(aBlockFrame->StyleContext());

    mIsVisual = aBlockFrame->PresContext()->IsVisualMode();
    if (mIsVisual) {
      













      for (nsIContent* content = aBlockFrame->GetContent() ; content; 
           content = content->GetParent()) {
        if (content->IsNodeOfType(nsINode::eHTML_FORM_CONTROL) ||
            content->IsXULElement()) {
          mIsVisual = false;
          break;
        }
      }
    }
  }

  BidiParagraphData* GetSubParagraph()
  {
    if (!mSubParagraph) {
      mSubParagraph = new BidiParagraphData();
      mSubParagraph->Init(this);
    }

    return mSubParagraph;
  }

  
  void Init(BidiParagraphData *aBpd)
  {
    mBidiEngine = new nsBidi();
    mPrevContent = nullptr;
    mIsVisual = aBpd->mIsVisual;
    mReset = false;
  }

  void Reset(nsIFrame* aBDIFrame, BidiParagraphData *aBpd)
  {
    mReset = true;
    mLogicalFrames.Clear();
    mLinePerFrame.Clear();
    mContentToFrameIndex.Clear();
    mBuffer.SetLength(0);
    mPrevFrame = aBpd->mPrevFrame;
    mParagraphDepth = aBpd->mParagraphDepth + 1;

    const nsStyleTextReset* text = aBDIFrame->StyleTextReset();
    bool isRTL = (NS_STYLE_DIRECTION_RTL ==
                  aBDIFrame->StyleVisibility()->mDirection);

    if (text->mUnicodeBidi & NS_STYLE_UNICODE_BIDI_PLAINTEXT) {
      mParaLevel = NSBIDI_DEFAULT_LTR;
    } else {
      mParaLevel = mParagraphDepth * 2;
      if (isRTL) ++mParaLevel;
    }

    if (text->mUnicodeBidi & NS_STYLE_UNICODE_BIDI_OVERRIDE) {
      PushBidiControl(isRTL ? kRLO : kLRO);
    }
  }

  void EmptyBuffer()
  {
    mBuffer.SetLength(0);
  }

  nsresult SetPara()
  {
    return mBidiEngine->SetPara(mBuffer.get(), BufferLength(),
                                mParaLevel, nullptr);
  }

  




  nsBidiLevel GetParaLevel()
  {
    nsBidiLevel paraLevel = mParaLevel;
    if (IS_DEFAULT_LEVEL(paraLevel)) {
      mBidiEngine->GetParaLevel(&paraLevel);
    }
    return paraLevel;
  }

  nsBidiDirection GetDirection()
  {
    nsBidiDirection dir;
    mBidiEngine->GetDirection(&dir);
    return dir;
  }

  nsresult CountRuns(int32_t *runCount){ return mBidiEngine->CountRuns(runCount); }

  nsresult GetLogicalRun(int32_t aLogicalStart, 
                         int32_t* aLogicalLimit,
                         nsBidiLevel* aLevel)
  {
    nsresult rv = mBidiEngine->GetLogicalRun(aLogicalStart,
                                             aLogicalLimit, aLevel);
    if (mIsVisual || NS_FAILED(rv))
      *aLevel = GetParaLevel();
    return rv;
  }

  void ResetData()
  {
    mLogicalFrames.Clear();
    mLinePerFrame.Clear();
    mContentToFrameIndex.Clear();
    mBuffer.SetLength(0);
    mPrevContent = nullptr;
    for (uint32_t i = 0; i < mEmbeddingStack.Length(); ++i) {
      mBuffer.Append(mEmbeddingStack[i]);
      mLogicalFrames.AppendElement(NS_BIDI_CONTROL_FRAME);
      mLinePerFrame.AppendElement((nsLineBox*)nullptr);
    }
  }

  void ResetForNewBlock()
  {
    for (BidiParagraphData* bpd = this; bpd; bpd = bpd->mSubParagraph) {
      bpd->mPrevFrame = nullptr;
    }
  }

  void AppendFrame(nsIFrame* aFrame,
                   nsBlockInFlowLineIterator* aLineIter,
                   nsIContent* aContent = nullptr)
  {
    if (aContent) {
      mContentToFrameIndex.Put(aContent, FrameCount());
    }
    mLogicalFrames.AppendElement(aFrame);

    AdvanceLineIteratorToFrame(aFrame, aLineIter, mPrevFrame);
    mLinePerFrame.AppendElement(aLineIter->GetLine().get());
  }

  void AdvanceAndAppendFrame(nsIFrame** aFrame,
                             nsBlockInFlowLineIterator* aLineIter,
                             nsIFrame** aNextSibling)
  {
    nsIFrame* frame = *aFrame;
    nsIFrame* nextSibling = *aNextSibling;

    frame = frame->GetNextContinuation();
    if (frame) {
      AppendFrame(frame, aLineIter, nullptr);

      



      if (frame == nextSibling) {
        nextSibling = frame->GetNextSibling();
      }
    }

    *aFrame = frame;
    *aNextSibling = nextSibling;
  }

  int32_t GetLastFrameForContent(nsIContent *aContent)
  {
    int32_t index = 0;
    mContentToFrameIndex.Get(aContent, &index);
    return index;
  }

  int32_t FrameCount(){ return mLogicalFrames.Length(); }

  int32_t BufferLength(){ return mBuffer.Length(); }

  nsIFrame* FrameAt(int32_t aIndex){ return mLogicalFrames[aIndex]; }

  nsLineBox* GetLineForFrameAt(int32_t aIndex){ return mLinePerFrame[aIndex]; }

  void AppendUnichar(char16_t aCh){ mBuffer.Append(aCh); }

  void AppendString(const nsDependentSubstring& aString){ mBuffer.Append(aString); }

  void AppendControlChar(char16_t aCh)
  {
    mLogicalFrames.AppendElement(NS_BIDI_CONTROL_FRAME);
    mLinePerFrame.AppendElement((nsLineBox*)nullptr);
    AppendUnichar(aCh);
  }

  void PushBidiControl(char16_t aCh)
  {
    AppendControlChar(aCh);
    mEmbeddingStack.AppendElement(aCh);
  }

  void PopBidiControl()
  {
    AppendControlChar(kPDF);
    NS_ASSERTION(mEmbeddingStack.Length(), "embedding/override underflow");
    mEmbeddingStack.TruncateLength(mEmbeddingStack.Length() - 1);
  }

  void ClearBidiControls()
  {
    for (uint32_t i = 0; i < mEmbeddingStack.Length(); ++i) {
      AppendControlChar(kPDF);
    }
  }

  static bool
  IsFrameInCurrentLine(nsBlockInFlowLineIterator* aLineIter,
                       nsIFrame* aPrevFrame, nsIFrame* aFrame)
  {
    nsIFrame* endFrame = aLineIter->IsLastLineInList() ? nullptr :
      aLineIter->GetLine().next()->mFirstChild;
    nsIFrame* startFrame = aPrevFrame ? aPrevFrame : aLineIter->GetLine()->mFirstChild;
    for (nsIFrame* frame = startFrame; frame && frame != endFrame;
         frame = frame->GetNextSibling()) {
      if (frame == aFrame)
        return true;
    }
    return false;
  }

  static void
  AdvanceLineIteratorToFrame(nsIFrame* aFrame,
                             nsBlockInFlowLineIterator* aLineIter,
                             nsIFrame*& aPrevFrame)
  {
    
    nsIFrame* child = aFrame;
    nsIFrame* parent = nsLayoutUtils::GetParentOrPlaceholderFor(child);
    while (parent && !nsLayoutUtils::GetAsBlock(parent)) {
      child = parent;
      parent = nsLayoutUtils::GetParentOrPlaceholderFor(child);
    }
    NS_ASSERTION (parent, "aFrame is not a descendent of aBlockFrame");
    while (!IsFrameInCurrentLine(aLineIter, aPrevFrame, child)) {
#ifdef DEBUG
      bool hasNext =
#endif
        aLineIter->Next();
      NS_ASSERTION(hasNext, "Can't find frame in lines!");
      aPrevFrame = nullptr;
    }
    aPrevFrame = child;
  }

};

struct BidiLineData {
  nsTArray<nsIFrame*> mLogicalFrames;
  nsTArray<nsIFrame*> mVisualFrames;
  nsTArray<int32_t> mIndexMap;
  nsAutoTArray<uint8_t, 18> mLevels;
  bool mIsReordered;

  BidiLineData(nsIFrame* aFirstFrameOnLine, int32_t   aNumFramesOnLine)
  {
    



    mLogicalFrames.Clear();

    bool isReordered = false;
    bool hasRTLFrames = false;

    for (nsIFrame* frame = aFirstFrameOnLine;
         frame && aNumFramesOnLine--;
         frame = frame->GetNextSibling()) {
      AppendFrame(frame);
      nsBidiLevel level = nsBidiPresUtils::GetFrameEmbeddingLevel(frame);
      mLevels.AppendElement(level);
      mIndexMap.AppendElement(0);
      if (IS_LEVEL_RTL(level)) {
        hasRTLFrames = true;
      }
    }

    
    nsBidi::ReorderVisual(mLevels.Elements(), FrameCount(),
                          mIndexMap.Elements());

    for (int32_t i = 0; i < FrameCount(); i++) {
      mVisualFrames.AppendElement(LogicalFrameAt(mIndexMap[i]));
      if (i != mIndexMap[i]) {
        isReordered = true;
      }
    }

    
    mIsReordered = isReordered || hasRTLFrames;
  }

  void AppendFrame(nsIFrame* aFrame)
  {
    mLogicalFrames.AppendElement(aFrame); 
  }

  int32_t FrameCount(){ return mLogicalFrames.Length(); }

  nsIFrame* LogicalFrameAt(int32_t aIndex){ return mLogicalFrames[aIndex]; }

  nsIFrame* VisualFrameAt(int32_t aIndex){ return mVisualFrames[aIndex]; }
};




static bool
IsBidiSplittable(nsIFrame* aFrame)
{
  
  nsIAtom* frameType = aFrame->GetType();
  return (aFrame->IsFrameOfType(nsIFrame::eBidiInlineContainer) &&
          frameType != nsGkAtoms::lineFrame) ||
         frameType == nsGkAtoms::textFrame;
}


static bool
IsBidiLeaf(nsIFrame* aFrame)
{
  nsIFrame* kid = aFrame->GetFirstPrincipalChild();
  return !kid || !aFrame->IsFrameOfType(nsIFrame::eBidiInlineContainer);
}










static nsresult
SplitInlineAncestors(nsContainerFrame* aParent,
                     nsIFrame* aFrame)
{
  nsPresContext* presContext = aParent->PresContext();
  nsIPresShell* presShell = presContext->PresShell();
  nsIFrame* frame = aFrame;
  nsContainerFrame* parent = aParent;
  nsContainerFrame* newParent;

  while (IsBidiSplittable(parent)) {
    nsContainerFrame* grandparent = parent->GetParent();
    NS_ASSERTION(grandparent, "Couldn't get parent's parent in nsBidiPresUtils::SplitInlineAncestors");
    
    
    if (!frame || frame->GetNextSibling()) {
    
      newParent = static_cast<nsContainerFrame*>(presShell->FrameConstructor()->
        CreateContinuingFrame(presContext, parent, grandparent, false));

      nsFrameList tail = parent->StealFramesAfter(frame);

      
      nsresult rv;
      rv = nsContainerFrame::ReparentFrameViewList(tail, parent, newParent);
      if (NS_FAILED(rv)) {
        return rv;
      }

      
      newParent->InsertFrames(nsIFrame::kNoReflowPrincipalList, nullptr, tail);
    
      
      nsFrameList temp(newParent, newParent);
      grandparent->InsertFrames(nsIFrame::kNoReflowPrincipalList, parent, temp);
    }
    
    frame = parent;
    parent = grandparent;
  }
  
  return NS_OK;
}

static void
MakeContinuationFluid(nsIFrame* aFrame, nsIFrame* aNext)
{
  NS_ASSERTION (!aFrame->GetNextInFlow() || aFrame->GetNextInFlow() == aNext, 
                "next-in-flow is not next continuation!");
  aFrame->SetNextInFlow(aNext);

  NS_ASSERTION (!aNext->GetPrevInFlow() || aNext->GetPrevInFlow() == aFrame,
                "prev-in-flow is not prev continuation!");
  aNext->SetPrevInFlow(aFrame);
}

static void
MakeContinuationsNonFluidUpParentChain(nsIFrame* aFrame, nsIFrame* aNext)
{
  nsIFrame* frame;
  nsIFrame* next;

  for (frame = aFrame, next = aNext;
       frame && next &&
         next != frame && next == frame->GetNextInFlow() &&
         IsBidiSplittable(frame);
       frame = frame->GetParent(), next = next->GetParent()) {

    frame->SetNextContinuation(next);
    next->SetPrevContinuation(frame);
  }
}




static void
JoinInlineAncestors(nsIFrame* aFrame)
{
  nsIFrame* frame = aFrame;
  do {
    nsIFrame* next = frame->GetNextContinuation();
    if (next) {
      
      
      if (nsBidiPresUtils::GetParagraphDepth(frame) ==
          nsBidiPresUtils::GetParagraphDepth(next)) {
        MakeContinuationFluid(frame, next);
      }
    }
    
    if (frame->GetNextSibling())
      break;
    frame = frame->GetParent();
  } while (frame && IsBidiSplittable(frame));
}

static nsresult
CreateContinuation(nsIFrame*  aFrame,
                   nsIFrame** aNewFrame,
                   bool       aIsFluid)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  NS_PRECONDITION(aFrame, "null ptr");

  *aNewFrame = nullptr;

  nsPresContext *presContext = aFrame->PresContext();
  nsIPresShell *presShell = presContext->PresShell();
  NS_ASSERTION(presShell, "PresShell must be set on PresContext before calling nsBidiPresUtils::CreateContinuation");

  nsContainerFrame* parent = aFrame->GetParent();
  NS_ASSERTION(parent, "Couldn't get frame parent in nsBidiPresUtils::CreateContinuation");

  nsresult rv = NS_OK;
  
  
  
  
  if (parent->GetType() == nsGkAtoms::letterFrame &&
      parent->IsFloating()) {
    nsFirstLetterFrame* letterFrame = do_QueryFrame(parent);
    rv = letterFrame->CreateContinuationForFloatingParent(presContext, aFrame,
                                                          aNewFrame, aIsFluid);
    return rv;
  }

  *aNewFrame = presShell->FrameConstructor()->
    CreateContinuingFrame(presContext, aFrame, parent, aIsFluid);

  
  
  nsFrameList temp(*aNewFrame, *aNewFrame);
  parent->InsertFrames(nsIFrame::kNoReflowPrincipalList, aFrame, temp);

  if (!aIsFluid) {  
    
    rv = SplitInlineAncestors(parent, aFrame);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  return NS_OK;
}































nsresult
nsBidiPresUtils::Resolve(nsBlockFrame* aBlockFrame)
{
  BidiParagraphData bpd;
  bpd.Init(aBlockFrame);

  
  
  const nsStyleTextReset* text = aBlockFrame->StyleTextReset();
  char16_t ch = 0;
  if (text->mUnicodeBidi & NS_STYLE_UNICODE_BIDI_OVERRIDE) {
    const nsStyleVisibility* vis = aBlockFrame->StyleVisibility();
    if (NS_STYLE_DIRECTION_RTL == vis->mDirection) {
      ch = kRLO;
    }
    else if (NS_STYLE_DIRECTION_LTR == vis->mDirection) {
      ch = kLRO;
    }
    if (ch != 0) {
      bpd.PushBidiControl(ch);
    }
  }
  for (nsBlockFrame* block = aBlockFrame; block;
       block = static_cast<nsBlockFrame*>(block->GetNextContinuation())) {
    block->RemoveStateBits(NS_BLOCK_NEEDS_BIDI_RESOLUTION);
    nsBlockInFlowLineIterator lineIter(block, block->begin_lines());
    bpd.ResetForNewBlock();
    TraverseFrames(aBlockFrame, &lineIter, block->GetFirstPrincipalChild(), &bpd);
    
  }

  if (ch != 0) {
    bpd.PopBidiControl();
  }

  BidiParagraphData* subParagraph = bpd.GetSubParagraph();
  if (subParagraph->BufferLength()) {
    ResolveParagraph(aBlockFrame, subParagraph);
    subParagraph->EmptyBuffer();
  }
  return ResolveParagraph(aBlockFrame, &bpd);
}

nsresult
nsBidiPresUtils::ResolveParagraph(nsBlockFrame* aBlockFrame,
                                  BidiParagraphData* aBpd)
{
  nsPresContext *presContext = aBlockFrame->PresContext();

  if (aBpd->BufferLength() < 1) {
    return NS_OK;
  }
  aBpd->mBuffer.ReplaceChar(kSeparators, kSpace);

  int32_t runCount;

  nsresult rv = aBpd->SetPara();
  NS_ENSURE_SUCCESS(rv, rv);

  nsBidiLevel embeddingLevel = aBpd->GetParaLevel();

  rv = aBpd->CountRuns(&runCount);
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t     runLength      = 0;   
  int32_t     lineOffset     = 0;   
  int32_t     logicalLimit   = 0;   
  int32_t     numRun         = -1;
  int32_t     fragmentLength = 0;   
  int32_t     frameIndex     = -1;  
  int32_t     frameCount     = aBpd->FrameCount();
  int32_t     contentOffset  = 0;   
  bool        isTextFrame    = false;
  nsIFrame*   frame = nullptr;
  nsIContent* content = nullptr;
  int32_t     contentTextLength = 0;

  FramePropertyTable *propTable = presContext->PropertyTable();
  nsLineBox* currentLine = nullptr;
  
#ifdef DEBUG
#ifdef NOISY_BIDI
  printf("Before Resolve(), aBlockFrame=0x%p, mBuffer='%s', frameCount=%d, runCount=%d\n",
         (void*)aBlockFrame, NS_ConvertUTF16toUTF8(aBpd->mBuffer).get(), frameCount, runCount);
#ifdef REALLY_NOISY_BIDI
  printf(" block frame tree=:\n");
  aBlockFrame->List(stdout, 0);
#endif
#endif
#endif

  if (runCount == 1 && frameCount == 1 &&
      aBpd->mParagraphDepth == 0 && aBpd->GetDirection() == NSBIDI_LTR &&
      aBpd->GetParaLevel() == 0) {
    
    
    
    
    
    nsIFrame* frame = aBpd->FrameAt(0);
    if (frame != NS_BIDI_CONTROL_FRAME &&
        !frame->Properties().Get(nsIFrame::EmbeddingLevelProperty()) &&
        !frame->Properties().Get(nsIFrame::BaseLevelProperty())) {
#ifdef DEBUG
#ifdef NOISY_BIDI
      printf("early return for single direction frame %p\n", (void*)frame);
#endif
#endif
      frame->AddStateBits(NS_FRAME_IS_BIDI);
      return NS_OK;
    }
  }

  nsIFrame* firstFrame = nullptr;
  nsIFrame* lastFrame = nullptr;

  for (; ;) {
    if (fragmentLength <= 0) {
      
      if (++frameIndex >= frameCount) {
        break;
      }
      frame = aBpd->FrameAt(frameIndex);
      if (frame == NS_BIDI_CONTROL_FRAME ||
          nsGkAtoms::textFrame != frame->GetType()) {
        



        isTextFrame = false;
        fragmentLength = 1;
      }
      else {
        if (!firstFrame) {
          firstFrame = frame;
        }
        lastFrame = frame;
        currentLine = aBpd->GetLineForFrameAt(frameIndex);
        content = frame->GetContent();
        if (!content) {
          rv = NS_OK;
          break;
        }
        contentTextLength = content->TextLength();
        if (contentTextLength == 0) {
          frame->AdjustOffsetsForBidi(0, 0);
          
          
          propTable->Set(frame, nsIFrame::EmbeddingLevelProperty(),
                         NS_INT32_TO_PTR(embeddingLevel));
          propTable->Set(frame, nsIFrame::BaseLevelProperty(),
                         NS_INT32_TO_PTR(aBpd->GetParaLevel()));
          propTable->Set(frame, nsIFrame::ParagraphDepthProperty(),
                         NS_INT32_TO_PTR(aBpd->mParagraphDepth));
          continue;
        }
        int32_t start, end;
        frame->GetOffsets(start, end);
        NS_ASSERTION(!(contentTextLength < end - start),
                     "Frame offsets don't fit in content");
        fragmentLength = std::min(contentTextLength, end - start);
        contentOffset = start;
        isTextFrame = true;
      }
    } 

    if (runLength <= 0) {
      
      if (++numRun >= runCount) {
        break;
      }
      lineOffset = logicalLimit;
      if (NS_FAILED(aBpd->GetLogicalRun(
              lineOffset, &logicalLimit, &embeddingLevel) ) ) {
        break;
      }
      runLength = logicalLimit - lineOffset;
    } 

    if (frame == NS_BIDI_CONTROL_FRAME) {
      frame = nullptr;
      ++lineOffset;
    }
    else {
      propTable->Set(frame, nsIFrame::EmbeddingLevelProperty(),
                     NS_INT32_TO_PTR(embeddingLevel));
      propTable->Set(frame, nsIFrame::BaseLevelProperty(),
                     NS_INT32_TO_PTR(aBpd->GetParaLevel()));
      propTable->Set(frame, nsIFrame::ParagraphDepthProperty(),
                     NS_INT32_TO_PTR(aBpd->mParagraphDepth));
      if (isTextFrame) {
        if ( (runLength > 0) && (runLength < fragmentLength) ) {
          



          currentLine->MarkDirty();
          nsIFrame* nextBidi;
          int32_t runEnd = contentOffset + runLength;
          rv = EnsureBidiContinuation(frame, &nextBidi, frameIndex,
                                      contentOffset,
                                      runEnd);
          if (NS_FAILED(rv)) {
            break;
          }
          nextBidi->AdjustOffsetsForBidi(runEnd,
                                         contentOffset + fragmentLength);
          lastFrame = frame = nextBidi;
          contentOffset = runEnd;
        } 
        else {
          if (contentOffset + fragmentLength == contentTextLength) {
            




            int32_t newIndex = aBpd->GetLastFrameForContent(content);
            if (newIndex > frameIndex) {
              currentLine->MarkDirty();
              RemoveBidiContinuation(aBpd, frame,
                                     frameIndex, newIndex, lineOffset);
              frameIndex = newIndex;
              lastFrame = frame = aBpd->FrameAt(frameIndex);
            }
          } else if (fragmentLength > 0 && runLength > fragmentLength) {
            





            int32_t newIndex = frameIndex;
            do {
            } while (++newIndex < frameCount &&
                     aBpd->FrameAt(newIndex) == NS_BIDI_CONTROL_FRAME);
            if (newIndex < frameCount) {
              currentLine->MarkDirty();
              RemoveBidiContinuation(aBpd, frame,
                                     frameIndex, newIndex, lineOffset);
            }
          } else if (runLength == fragmentLength) {
            




            nsIFrame* next = frame->GetNextInFlow();
            if (next) {
              currentLine->MarkDirty();
              MakeContinuationsNonFluidUpParentChain(frame, next);
            }
          }
          frame->AdjustOffsetsForBidi(contentOffset, contentOffset + fragmentLength);
        }
      } 
      else {
        ++lineOffset;
      }
    } 
    int32_t temp = runLength;
    runLength -= fragmentLength;
    fragmentLength -= temp;

    if (frame && fragmentLength <= 0) {
      
      
      
      
      
      
      if (runLength <= 0 && !frame->GetNextInFlow()) {
        if (numRun + 1 < runCount) {
          nsIFrame* child = frame;
          nsContainerFrame* parent = frame->GetParent();
          
          
          
          
          
          
          while (parent &&
                 IsBidiSplittable(parent) &&
                 !child->GetNextSibling()) {
            nsIFrame* next = parent->GetNextInFlow();
            if (next) {
              parent->SetNextContinuation(next);
              next->SetPrevContinuation(parent);
            }
            child = parent;
            parent = child->GetParent();
          }
          if (parent && IsBidiSplittable(parent)) {
            SplitInlineAncestors(parent, child);
          }
        }
      }
      else {
        
        
        
        JoinInlineAncestors(frame);
      }
    }
  } 

  if (aBpd->mParagraphDepth > 0) {
    if (firstFrame) {
      nsContainerFrame* child = firstFrame->GetParent();
      if (child) {
        nsContainerFrame* parent = child->GetParent();
        if (parent && IsBidiSplittable(parent)) {
          nsIFrame* prev = child->GetPrevSibling();
          if (prev) {
            SplitInlineAncestors(parent, prev);
          }
        }
      }
    }
    if (lastFrame) {
      nsContainerFrame* child = lastFrame->GetParent();
      if (child) {
        nsContainerFrame* parent = child->GetParent();
        if (parent && IsBidiSplittable(parent)) {
          SplitInlineAncestors(parent, child);
        }
      }
    }
  }

#ifdef DEBUG
#ifdef REALLY_NOISY_BIDI
  printf("---\nAfter Resolve(), frameTree =:\n");
  aBlockFrame->List(stdout, 0);
  printf("===\n");
#endif
#endif

  return rv;
}

void
nsBidiPresUtils::TraverseFrames(nsBlockFrame*              aBlockFrame,
                                nsBlockInFlowLineIterator* aLineIter,
                                nsIFrame*                  aCurrentFrame,
                                BidiParagraphData*         aBpd)
{
  if (!aCurrentFrame)
    return;

#ifdef DEBUG
  nsBlockFrame* initialLineContainer = aLineIter->GetContainer();
#endif

  nsIFrame* childFrame = aCurrentFrame;
  do {
    







    nsIFrame* nextSibling = childFrame->GetNextSibling();
    bool isLastFrame = !childFrame->GetNextContinuation();
    bool isFirstFrame = !childFrame->GetPrevContinuation();

    
    
    
    nsIFrame* frame = childFrame;
    if (nsGkAtoms::placeholderFrame == childFrame->GetType()) {
      nsIFrame* realFrame =
        nsPlaceholderFrame::GetRealFrameForPlaceholder(childFrame);
      if (realFrame->GetType() == nsGkAtoms::letterFrame) {
        frame = realFrame;
      }
    }

    char16_t ch = 0;
    if (frame->IsFrameOfType(nsIFrame::eBidiInlineContainer)) {
      if (!(frame->GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
        nsContainerFrame* c = static_cast<nsContainerFrame*>(frame);
        MOZ_ASSERT(c = do_QueryFrame(frame),
                   "eBidiInlineContainer must be a nsContainerFrame subclass");
        c->DrainSelfOverflowList();
      }

      const nsStyleVisibility* vis = frame->StyleVisibility();
      const nsStyleTextReset* text = frame->StyleTextReset();
      if (text->mUnicodeBidi & NS_STYLE_UNICODE_BIDI_OVERRIDE) {
        if (NS_STYLE_DIRECTION_RTL == vis->mDirection) {
          ch = kRLO;
        }
        else if (NS_STYLE_DIRECTION_LTR == vis->mDirection) {
          ch = kLRO;
        }
      } else if (text->mUnicodeBidi & NS_STYLE_UNICODE_BIDI_EMBED) {
        if (NS_STYLE_DIRECTION_RTL == vis->mDirection) {
          ch = kRLE;
        }
        else if (NS_STYLE_DIRECTION_LTR == vis->mDirection) {
          ch = kLRE;
        }
      }

      
      
      if (ch != 0 && isFirstFrame) {
        aBpd->PushBidiControl(ch);
      }
    }

    if (IsBidiLeaf(frame)) {
      




      nsIContent* content = frame->GetContent();
      aBpd->AppendFrame(frame, aLineIter, content);

      
      nsIAtom* frameType = frame->GetType();
      if (nsGkAtoms::textFrame == frameType) {
        if (content != aBpd->mPrevContent) {
          aBpd->mPrevContent = content;
          if (!frame->StyleText()->NewlineIsSignificant(frame)) {
            content->AppendTextTo(aBpd->mBuffer);
          } else {
            



            nsAutoString text;
            content->AppendTextTo(text);
            nsIFrame* next;
            do {
              next = nullptr;

              int32_t start, end;
              frame->GetOffsets(start, end);
              int32_t endLine = text.FindChar('\n', start);
              if (endLine == -1) {
                




                aBpd->AppendString(Substring(text, start));
                while (frame && nextSibling) {
                  aBpd->AdvanceAndAppendFrame(&frame, aLineIter, &nextSibling);
                }
                break;
              }

              



              ++endLine;

              



              aBpd->AppendString(Substring(text, start,
                                           std::min(end, endLine) - start));
              while (end < endLine && nextSibling) { 
                aBpd->AdvanceAndAppendFrame(&frame, aLineIter, &nextSibling);
                NS_ASSERTION(frame, "Premature end of continuation chain");
                frame->GetOffsets(start, end);
                aBpd->AppendString(Substring(text, start,
                                             std::min(end, endLine) - start));
              }

              if (end < endLine) {
                aBpd->mPrevContent = nullptr;
                break;
              }

              bool createdContinuation = false;
              if (uint32_t(endLine) < text.Length()) {
                











                next = frame->GetNextInFlow();
                if (!next) {
                  
                  next = frame->GetNextContinuation();
                  if (next) {
                    MakeContinuationFluid(frame, next);
                    JoinInlineAncestors(frame);
                  }
                }

                nsTextFrame* textFrame = static_cast<nsTextFrame*>(frame);
                textFrame->SetLength(endLine - start, nullptr);

                if (!next) {
                  
                  CreateContinuation(frame, &next, true);
                  createdContinuation = true;
                }
                
                aBpd->GetLineForFrameAt(aBpd->FrameCount() - 1)->MarkDirty();
              }
              ResolveParagraphWithinBlock(aBlockFrame, aBpd);

              if (!nextSibling && !createdContinuation) {
                break;
              } else if (next) {
                frame = next;
                aBpd->AppendFrame(frame, aLineIter);
                
                aBpd->GetLineForFrameAt(aBpd->FrameCount() - 1)->MarkDirty();
              }

              



              if (frame && frame == nextSibling) {
                nextSibling = frame->GetNextSibling();
              }

            } while (next);
          }
        }
      } else if (nsGkAtoms::brFrame == frameType) {
        
        aBpd->AppendUnichar(kLineSeparator);
        ResolveParagraphWithinBlock(aBlockFrame, aBpd);
      } else { 
        
        
        
        
        
        aBpd->AppendUnichar(content->IsHTMLElement(nsGkAtoms::wbr) ?
                            kZWSP : kObjectSubstitute);
        if (!frame->IsInlineOutside()) {
          
          ResolveParagraphWithinBlock(aBlockFrame, aBpd);
        }
      }
    } else {
      
      nsIFrame* kid = frame->GetFirstPrincipalChild();
      MOZ_ASSERT(!frame->GetFirstChild(nsIFrame::kOverflowList),
                 "should have drained the overflow list above");
      if (kid) {
        const nsStyleTextReset* text = frame->StyleTextReset();
        if (text->mUnicodeBidi & NS_STYLE_UNICODE_BIDI_ISOLATE ||
            text->mUnicodeBidi & NS_STYLE_UNICODE_BIDI_PLAINTEXT) {
          
          
          BidiParagraphData* subParagraph = aBpd->GetSubParagraph();

          







          bool isLastContinuation = !frame->GetNextContinuation();
          if (!frame->GetPrevContinuation() || !subParagraph->mReset) {
            if (subParagraph->BufferLength()) {
              ResolveParagraph(aBlockFrame, subParagraph);
            }
            subParagraph->Reset(frame, aBpd);
          }
          TraverseFrames(aBlockFrame, aLineIter, kid, subParagraph);
          if (isLastContinuation) {
            ResolveParagraph(aBlockFrame, subParagraph);
            subParagraph->EmptyBuffer();
          }

          
          
          aBpd->AppendControlChar(kObjectSubstitute);
        } else {
          TraverseFrames(aBlockFrame, aLineIter, kid, aBpd);
        }
      }
    }

    
    if (isLastFrame) {
      if (ch) {
        
        
        aBpd->PopBidiControl();
      }
    }
    childFrame = nextSibling;
  } while (childFrame);

  MOZ_ASSERT(initialLineContainer == aLineIter->GetContainer());
}

void
nsBidiPresUtils::ResolveParagraphWithinBlock(nsBlockFrame* aBlockFrame,
                                             BidiParagraphData* aBpd)
{
  aBpd->ClearBidiControls();
  ResolveParagraph(aBlockFrame, aBpd);
  aBpd->ResetData();
}

void
nsBidiPresUtils::ReorderFrames(nsIFrame*     aFirstFrameOnLine,
                               int32_t       aNumFramesOnLine,
                               WritingMode   aLineWM,
                               const nsSize& aContainerSize,
                               nscoord       aStart)
{
  
  if (aFirstFrameOnLine->GetType() == nsGkAtoms::lineFrame) {
    aFirstFrameOnLine = aFirstFrameOnLine->GetFirstPrincipalChild();
    if (!aFirstFrameOnLine)
      return;
    
    
    aNumFramesOnLine = -1;
  }

  BidiLineData bld(aFirstFrameOnLine, aNumFramesOnLine);
  RepositionInlineFrames(&bld, aFirstFrameOnLine, aLineWM,
                         aContainerSize, aStart);
}

nsIFrame*
nsBidiPresUtils::GetFirstLeaf(nsIFrame* aFrame)
{
  nsIFrame* firstLeaf = aFrame;
  while (!IsBidiLeaf(firstLeaf)) {
    nsIFrame* firstChild = firstLeaf->GetFirstPrincipalChild();
    nsIFrame* realFrame = nsPlaceholderFrame::GetRealFrameFor(firstChild);
    firstLeaf = (realFrame->GetType() == nsGkAtoms::letterFrame) ?
                 realFrame : firstChild;
  }
  return firstLeaf;
}

nsBidiLevel
nsBidiPresUtils::GetFrameEmbeddingLevel(nsIFrame* aFrame)
{
  return NS_GET_EMBEDDING_LEVEL(nsBidiPresUtils::GetFirstLeaf(aFrame));
}

uint8_t
nsBidiPresUtils::GetParagraphDepth(nsIFrame* aFrame)
{
  return NS_GET_PARAGRAPH_DEPTH(nsBidiPresUtils::GetFirstLeaf(aFrame));
}


nsBidiLevel
nsBidiPresUtils::GetFrameBaseLevel(nsIFrame* aFrame)
{
  nsIFrame* firstLeaf = aFrame;
  while (!IsBidiLeaf(firstLeaf)) {
    firstLeaf = firstLeaf->GetFirstPrincipalChild();
  }
  return NS_GET_BASE_LEVEL(firstLeaf);
}

void
nsBidiPresUtils::IsFirstOrLast(nsIFrame*             aFrame,
                               nsContinuationStates* aContinuationStates,
                               bool                  aSpanDirMatchesLineDir,
                               bool&                 aIsFirst ,
                               bool&                 aIsLast )
{
  










  bool firstInLineOrder, lastInLineOrder;
  nsFrameContinuationState* frameState = aContinuationStates->GetEntry(aFrame);
  nsFrameContinuationState* firstFrameState;

  if (!frameState->mFirstVisualFrame) {
    
    nsFrameContinuationState* contState;
    nsIFrame* frame;

    frameState->mFrameCount = 1;
    frameState->mFirstVisualFrame = aFrame;

    




    
    for (frame = aFrame->GetPrevContinuation();
         frame && (contState = aContinuationStates->GetEntry(frame));
         frame = frame->GetPrevContinuation()) {
      frameState->mFrameCount++;
      contState->mFirstVisualFrame = aFrame;
    }
    frameState->mHasContOnPrevLines = (frame != nullptr);

    
    for (frame = aFrame->GetNextContinuation();
         frame && (contState = aContinuationStates->GetEntry(frame));
         frame = frame->GetNextContinuation()) {
      frameState->mFrameCount++;
      contState->mFirstVisualFrame = aFrame;
    }
    frameState->mHasContOnNextLines = (frame != nullptr);

    firstInLineOrder = true;
    firstFrameState = frameState;
  } else {
    
    firstInLineOrder = false;
    firstFrameState = aContinuationStates->GetEntry(frameState->mFirstVisualFrame);
  }

  lastInLineOrder = (firstFrameState->mFrameCount == 1);

  if (aSpanDirMatchesLineDir) {
    aIsFirst = firstInLineOrder;
    aIsLast = lastInLineOrder;
  } else {
    aIsFirst = lastInLineOrder;
    aIsLast = firstInLineOrder;
  }

  if (frameState->mHasContOnPrevLines) {
    aIsFirst = false;
  }
  if (firstFrameState->mHasContOnNextLines) {
    aIsLast = false;
  }

  if ((aIsFirst || aIsLast) &&
      (aFrame->GetStateBits() & NS_FRAME_PART_OF_IBSPLIT)) {
    
    
    
    nsIFrame* firstContinuation = aFrame->FirstContinuation();
    if (firstContinuation->FrameIsNonLastInIBSplit()) {
      
      aIsLast = false;
    }
    if (firstContinuation->FrameIsNonFirstInIBSplit()) {
      
      aIsFirst = false;
    }
  }

  
  firstFrameState->mFrameCount--;

  nsInlineFrame* testFrame = do_QueryFrame(aFrame);

  if (testFrame) {
    aFrame->AddStateBits(NS_INLINE_FRAME_BIDI_VISUAL_STATE_IS_SET);

    if (aIsFirst) {
      aFrame->AddStateBits(NS_INLINE_FRAME_BIDI_VISUAL_IS_FIRST);
    } else {
      aFrame->RemoveStateBits(NS_INLINE_FRAME_BIDI_VISUAL_IS_FIRST);
    }

    if (aIsLast) {
      aFrame->AddStateBits(NS_INLINE_FRAME_BIDI_VISUAL_IS_LAST);
    } else {
      aFrame->RemoveStateBits(NS_INLINE_FRAME_BIDI_VISUAL_IS_LAST);
    }
  }
}

void
nsBidiPresUtils::RepositionFrame(nsIFrame*             aFrame,
                                 bool                  aIsEvenLevel,
                                 nscoord&              aStart,
                                 nsContinuationStates* aContinuationStates,
                                 WritingMode           aContainerWM,
                                 const nsSize&         aContainerSize)
{
  if (!aFrame)
    return;

  bool isFirst, isLast;
  WritingMode frameWM = aFrame->GetWritingMode();
  IsFirstOrLast(aFrame,
                aContinuationStates,
                aContainerWM.IsBidiLTR() == frameWM.IsBidiLTR(),
                isFirst ,
                isLast );

  
  
  
  
  
  

  
  
  
  LogicalMargin frameMargin = aFrame->GetLogicalUsedMargin(frameWM);
  LogicalMargin borderPadding = aFrame->GetLogicalUsedBorderAndPadding(frameWM);
  if (!isFirst) {
    frameMargin.IStart(frameWM) = 0;
    borderPadding.IStart(frameWM) = 0;
  }
  if (!isLast) {
    frameMargin.IEnd(frameWM) = 0;
    borderPadding.IEnd(frameWM) = 0;
  }
  LogicalMargin margin = frameMargin.ConvertTo(aContainerWM, frameWM);
  aStart += margin.IStart(aContainerWM);

  nscoord start = aStart;

  if (!IsBidiLeaf(aFrame)) {
    
    
    
    
    bool reverseOrder = aIsEvenLevel != frameWM.IsBidiLTR();
    nsTArray<nsIFrame*> childList;
    nsIFrame *frame = aFrame->GetFirstPrincipalChild();
    if (frame && reverseOrder) {
      childList.AppendElement((nsIFrame*)nullptr);
      while (frame) {
        childList.AppendElement(frame);
        frame = frame->GetNextSibling();
      }
      frame = childList[childList.Length() - 1];
    }

    
    int32_t index = 0;
    nscoord iCoord = borderPadding.IStart(frameWM);

    while (frame) {
      RepositionFrame(frame,
                      aIsEvenLevel,
                      iCoord,
                      aContinuationStates,
                      frameWM,
                      aFrame->GetSize());
      index++;
      frame = reverseOrder ?
                childList[childList.Length() - index - 1] :
                frame->GetNextSibling();
    }

    aStart += iCoord + borderPadding.IEnd(frameWM);
  } else {
    aStart += aFrame->ISize(aContainerWM);
  }

  
  
  
  nsRect rect = aFrame->GetRect();
  nscoord lineSize = aContainerWM.IsVertical()
    ? aContainerSize.height : aContainerSize.width;
  NS_ASSERTION(aContainerWM.IsBidiLTR() || lineSize != NS_UNCONSTRAINEDSIZE,
               "Unconstrained inline line size in bidi frame reordering");

  nscoord frameIStart = aContainerWM.IsBidiLTR() ? start : lineSize - aStart;
  nscoord frameISize = aStart - start;

  (aContainerWM.IsVertical() ? rect.y : rect.x) = frameIStart;
  (aContainerWM.IsVertical() ? rect.height : rect.width) = frameISize;

  aFrame->SetRect(rect);

  aStart += margin.IEnd(aContainerWM);
}

void
nsBidiPresUtils::InitContinuationStates(nsIFrame*              aFrame,
                                        nsContinuationStates*  aContinuationStates)
{
  nsFrameContinuationState* state = aContinuationStates->PutEntry(aFrame);
  state->mFirstVisualFrame = nullptr;
  state->mFrameCount = 0;

  if (!IsBidiLeaf(aFrame)) {
    
    nsIFrame* frame;
    for (frame = aFrame->GetFirstPrincipalChild();
         frame;
         frame = frame->GetNextSibling()) {
      InitContinuationStates(frame,
                             aContinuationStates);
    }
  }
}

void
nsBidiPresUtils::RepositionInlineFrames(BidiLineData *aBld,
                                        nsIFrame* aFirstChild,
                                        WritingMode aLineWM,
                                        const nsSize& aContainerSize,
                                        nscoord aStart)
{
  nscoord start = aStart;
  nsIFrame* frame;
  int32_t count = aBld->mVisualFrames.Length();
  int32_t index;
  nsContinuationStates continuationStates;

  
  
  for (index = 0; index < count; index++) {
    InitContinuationStates(aBld->VisualFrameAt(index), &continuationStates);
  }

  
  int32_t step, limit;
  if (aLineWM.IsBidiLTR()) {
    index = 0;
    step = 1;
    limit = count;
  } else {
    index = count - 1;
    step = -1;
    limit = -1;
  }
  for (; index != limit; index += step) {
    frame = aBld->VisualFrameAt(index);
    RepositionFrame(frame,
                    !(IS_LEVEL_RTL(aBld->mLevels[aBld->mIndexMap[index]])),
                    start,
                    &continuationStates,
                    aLineWM,
                    aContainerSize);
  }
}

bool
nsBidiPresUtils::CheckLineOrder(nsIFrame*  aFirstFrameOnLine,
                                int32_t    aNumFramesOnLine,
                                nsIFrame** aFirstVisual,
                                nsIFrame** aLastVisual)
{
  BidiLineData bld(aFirstFrameOnLine, aNumFramesOnLine);
  int32_t count = bld.FrameCount();
  
  if (aFirstVisual) {
    *aFirstVisual = bld.VisualFrameAt(0);
  }
  if (aLastVisual) {
    *aLastVisual = bld.VisualFrameAt(count-1);
  }
  
  return bld.mIsReordered;
}

nsIFrame*
nsBidiPresUtils::GetFrameToRightOf(const nsIFrame*  aFrame,
                                   nsIFrame*        aFirstFrameOnLine,
                                   int32_t          aNumFramesOnLine)
{
  BidiLineData bld(aFirstFrameOnLine, aNumFramesOnLine);

  int32_t count = bld.mVisualFrames.Length();

  if (aFrame == nullptr && count)
    return bld.VisualFrameAt(0);
  
  for (int32_t i = 0; i < count - 1; i++) {
    if (bld.VisualFrameAt(i) == aFrame) {
      return bld.VisualFrameAt(i+1);
    }
  }
  
  return nullptr;
}

nsIFrame*
nsBidiPresUtils::GetFrameToLeftOf(const nsIFrame*  aFrame,
                                  nsIFrame*        aFirstFrameOnLine,
                                  int32_t          aNumFramesOnLine)
{
  BidiLineData bld(aFirstFrameOnLine, aNumFramesOnLine);

  int32_t count = bld.mVisualFrames.Length();
  
  if (aFrame == nullptr && count)
    return bld.VisualFrameAt(count-1);
  
  for (int32_t i = 1; i < count; i++) {
    if (bld.VisualFrameAt(i) == aFrame) {
      return bld.VisualFrameAt(i-1);
    }
  }
  
  return nullptr;
}

inline nsresult
nsBidiPresUtils::EnsureBidiContinuation(nsIFrame*       aFrame,
                                        nsIFrame**      aNewFrame,
                                        int32_t&        aFrameIndex,
                                        int32_t         aStart,
                                        int32_t         aEnd)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  NS_PRECONDITION(aFrame, "aFrame is null");

  aFrame->AdjustOffsetsForBidi(aStart, aEnd);
  return CreateContinuation(aFrame, aNewFrame, false);
}

void
nsBidiPresUtils::RemoveBidiContinuation(BidiParagraphData *aBpd,
                                        nsIFrame*       aFrame,
                                        int32_t         aFirstIndex,
                                        int32_t         aLastIndex,
                                        int32_t&        aOffset)
{
  FrameProperties props = aFrame->Properties();
  nsBidiLevel embeddingLevel =
    (nsBidiLevel)NS_PTR_TO_INT32(props.Get(nsIFrame::EmbeddingLevelProperty()));
  nsBidiLevel baseLevel =
    (nsBidiLevel)NS_PTR_TO_INT32(props.Get(nsIFrame::BaseLevelProperty()));
  uint8_t paragraphDepth = 
    NS_PTR_TO_INT32(props.Get(nsIFrame::ParagraphDepthProperty()));

  for (int32_t index = aFirstIndex + 1; index <= aLastIndex; index++) {
    nsIFrame* frame = aBpd->FrameAt(index);
    if (frame == NS_BIDI_CONTROL_FRAME) {
      ++aOffset;
    }
    else {
      
      
      FrameProperties frameProps = frame->Properties();
      frameProps.Set(nsIFrame::EmbeddingLevelProperty(),
                     NS_INT32_TO_PTR(embeddingLevel));
      frameProps.Set(nsIFrame::BaseLevelProperty(),
                     NS_INT32_TO_PTR(baseLevel));
      frameProps.Set(nsIFrame::ParagraphDepthProperty(),
                     NS_INT32_TO_PTR(paragraphDepth));
      frame->AddStateBits(NS_FRAME_IS_BIDI);
      while (frame) {
        nsIFrame* prev = frame->GetPrevContinuation();
        if (prev) {
          MakeContinuationFluid(prev, frame);
          frame = frame->GetParent();
        } else {
          break;
        }
      }
    }
  }

  
  
  
  nsIFrame* lastFrame = aBpd->FrameAt(aLastIndex);
  MakeContinuationsNonFluidUpParentChain(lastFrame, lastFrame->GetNextInFlow());
}

nsresult
nsBidiPresUtils::FormatUnicodeText(nsPresContext*  aPresContext,
                                   char16_t*       aText,
                                   int32_t&        aTextLength,
                                   nsCharType      aCharType,
                                   nsBidiDirection aDir)
{
  nsresult rv = NS_OK;
  
  
  uint32_t bidiOptions = aPresContext->GetBidi();
  switch (GET_BIDI_OPTION_NUMERAL(bidiOptions)) {

    case IBMBIDI_NUMERAL_HINDI:
      HandleNumbers(aText,aTextLength,IBMBIDI_NUMERAL_HINDI);
      break;

    case IBMBIDI_NUMERAL_ARABIC:
      HandleNumbers(aText,aTextLength,IBMBIDI_NUMERAL_ARABIC);
      break;

    case IBMBIDI_NUMERAL_PERSIAN:
      HandleNumbers(aText,aTextLength,IBMBIDI_NUMERAL_PERSIAN);
      break;

    case IBMBIDI_NUMERAL_REGULAR:

      switch (aCharType) {

        case eCharType_EuropeanNumber:
          HandleNumbers(aText,aTextLength,IBMBIDI_NUMERAL_ARABIC);
          break;

        case eCharType_ArabicNumber:
          HandleNumbers(aText,aTextLength,IBMBIDI_NUMERAL_HINDI);
          break;

        default:
          break;
      }
      break;

    case IBMBIDI_NUMERAL_HINDICONTEXT:
      if ( ( (GET_BIDI_OPTION_DIRECTION(bidiOptions)==IBMBIDI_TEXTDIRECTION_RTL) && (IS_ARABIC_DIGIT (aText[0])) ) || (eCharType_ArabicNumber == aCharType) )
        HandleNumbers(aText,aTextLength,IBMBIDI_NUMERAL_HINDI);
      else if (eCharType_EuropeanNumber == aCharType)
        HandleNumbers(aText,aTextLength,IBMBIDI_NUMERAL_ARABIC);
      break;

    case IBMBIDI_NUMERAL_PERSIANCONTEXT:
      if ( ( (GET_BIDI_OPTION_DIRECTION(bidiOptions)==IBMBIDI_TEXTDIRECTION_RTL) && (IS_ARABIC_DIGIT (aText[0])) ) || (eCharType_ArabicNumber == aCharType) )
        HandleNumbers(aText,aTextLength,IBMBIDI_NUMERAL_PERSIAN);
      else if (eCharType_EuropeanNumber == aCharType)
        HandleNumbers(aText,aTextLength,IBMBIDI_NUMERAL_ARABIC);
      break;

    case IBMBIDI_NUMERAL_NOMINAL:
    default:
      break;
  }

  StripBidiControlCharacters(aText, aTextLength);
  return rv;
}

void
nsBidiPresUtils::StripBidiControlCharacters(char16_t* aText,
                                            int32_t&   aTextLength)
{
  if ( (nullptr == aText) || (aTextLength < 1) ) {
    return;
  }

  int32_t stripLen = 0;

  for (int32_t i = 0; i < aTextLength; i++) {
    
    
    if (IsBidiControl((uint32_t)aText[i])) {
      ++stripLen;
    }
    else {
      aText[i - stripLen] = aText[i];
    }
  }
  aTextLength -= stripLen;
}
 
#if 0 
void
RemoveDiacritics(char16_t* aText,
                 int32_t&   aTextLength)
{
  if (aText && (aTextLength > 0) ) {
    int32_t offset = 0;

    for (int32_t i = 0; i < aTextLength && aText[i]; i++) {
      if (IS_BIDI_DIACRITIC(aText[i]) ) {
        ++offset;
        continue;
      }
      aText[i - offset] = aText[i];
    }
    aTextLength = i - offset;
    aText[aTextLength] = 0;
  }
}
#endif

void
nsBidiPresUtils::CalculateCharType(nsBidi* aBidiEngine,
                                   const char16_t* aText,
                                   int32_t& aOffset,
                                   int32_t  aCharTypeLimit,
                                   int32_t& aRunLimit,
                                   int32_t& aRunLength,
                                   int32_t& aRunCount,
                                   uint8_t& aCharType,
                                   uint8_t& aPrevCharType)

{
  bool       strongTypeFound = false;
  int32_t    offset;
  nsCharType charType;

  aCharType = eCharType_OtherNeutral;

  for (offset = aOffset; offset < aCharTypeLimit; offset++) {
    
    
    
    if (IS_HEBREW_CHAR(aText[offset]) ) {
      charType = eCharType_RightToLeft;
    }
    else if (IS_ARABIC_ALPHABETIC(aText[offset]) ) {
      charType = eCharType_RightToLeftArabic;
    }
    else {
      aBidiEngine->GetCharTypeAt(offset, &charType);
    }

    if (!CHARTYPE_IS_WEAK(charType) ) {

      if (strongTypeFound
          && (charType != aPrevCharType)
          && (CHARTYPE_IS_RTL(charType) || CHARTYPE_IS_RTL(aPrevCharType) ) ) {
        
        
        
        
        aRunLength = offset - aOffset;
        aRunLimit = offset;
        ++aRunCount;
        break;
      }

      if ( (eCharType_RightToLeftArabic == aPrevCharType
            || eCharType_ArabicNumber == aPrevCharType)
          && eCharType_EuropeanNumber == charType) {
        charType = eCharType_ArabicNumber;
      }

      
      
      aPrevCharType = charType;

      strongTypeFound = true;
      aCharType = charType;
    }
  }
  aOffset = offset;
}

nsresult nsBidiPresUtils::ProcessText(const char16_t*       aText,
                                      int32_t                aLength,
                                      nsBidiLevel            aBaseLevel,
                                      nsPresContext*         aPresContext,
                                      BidiProcessor&         aprocessor,
                                      Mode                   aMode,
                                      nsBidiPositionResolve* aPosResolve,
                                      int32_t                aPosResolveCount,
                                      nscoord*               aWidth,
                                      nsBidi*                aBidiEngine)
{
  NS_ASSERTION((aPosResolve == nullptr) != (aPosResolveCount > 0), "Incorrect aPosResolve / aPosResolveCount arguments");

  int32_t runCount;

  nsAutoString textBuffer(aText, aLength);

  nsresult rv = aBidiEngine->SetPara(aText, aLength, aBaseLevel, nullptr);
  if (NS_FAILED(rv))
    return rv;

  rv = aBidiEngine->CountRuns(&runCount);
  if (NS_FAILED(rv))
    return rv;

  nscoord xOffset = 0;
  nscoord width, xEndRun = 0;
  nscoord totalWidth = 0;
  int32_t i, start, limit, length;
  uint32_t visualStart = 0;
  uint8_t charType;
  uint8_t prevType = eCharType_LeftToRight;

  for(int nPosResolve=0; nPosResolve < aPosResolveCount; ++nPosResolve)
  {
    aPosResolve[nPosResolve].visualIndex = kNotFound;
    aPosResolve[nPosResolve].visualLeftTwips = kNotFound;
    aPosResolve[nPosResolve].visualWidth = kNotFound;
  }

  for (i = 0; i < runCount; i++) {
    nsBidiDirection dir;
    rv = aBidiEngine->GetVisualRun(i, &start, &length, &dir);
    if (NS_FAILED(rv))
      return rv;

    nsBidiLevel level;
    rv = aBidiEngine->GetLogicalRun(start, &limit, &level);
    if (NS_FAILED(rv))
      return rv;

    dir = DIRECTION_FROM_LEVEL(level);
    int32_t subRunLength = limit - start;
    int32_t lineOffset = start;
    int32_t typeLimit = std::min(limit, aLength);
    int32_t subRunCount = 1;
    int32_t subRunLimit = typeLimit;

    











    if (dir == NSBIDI_RTL) {
      aprocessor.SetText(aText + start, subRunLength, dir);
      width = aprocessor.GetWidth();
      xOffset += width;
      xEndRun = xOffset;
    }

    while (subRunCount > 0) {
      
      
      CalculateCharType(aBidiEngine, aText, lineOffset, typeLimit, subRunLimit, subRunLength, subRunCount, charType, prevType);

      nsAutoString runVisualText;
      runVisualText.Assign(aText + start, subRunLength);
      if (int32_t(runVisualText.Length()) < subRunLength)
        return NS_ERROR_OUT_OF_MEMORY;
      FormatUnicodeText(aPresContext, runVisualText.BeginWriting(),
                        subRunLength, (nsCharType)charType, dir);

      aprocessor.SetText(runVisualText.get(), subRunLength, dir);
      width = aprocessor.GetWidth();
      totalWidth += width;
      if (dir == NSBIDI_RTL) {
        xOffset -= width;
      }
      if (aMode == MODE_DRAW) {
        aprocessor.DrawText(xOffset, width);
      }

      



      for(int nPosResolve=0; nPosResolve<aPosResolveCount; ++nPosResolve)
      {
        nsBidiPositionResolve* posResolve = &aPosResolve[nPosResolve];
        


        if (posResolve->visualLeftTwips != kNotFound)
           continue;
           
        


        if (start <= posResolve->logicalIndex &&
            start + subRunLength > posResolve->logicalIndex) {
          




          if (subRunLength == 1) {
            posResolve->visualIndex = visualStart;
            posResolve->visualLeftTwips = xOffset;
            posResolve->visualWidth = width;
          }
          








          else {
            





















            nscoord subWidth;
            
            const char16_t* visualLeftPart;
            const char16_t* visualRightSide;
            if (dir == NSBIDI_RTL) {
              
              posResolve->visualIndex = visualStart + (subRunLength - (posResolve->logicalIndex + 1 - start));
              
              visualLeftPart = aText + posResolve->logicalIndex + 1;
              
              visualRightSide = visualLeftPart - 1;
            }
            else {
              posResolve->visualIndex = visualStart + (posResolve->logicalIndex - start);
              
              visualLeftPart = aText + start;
              
              visualRightSide = visualLeftPart;
            }
            
            int32_t visualLeftLength = posResolve->visualIndex - visualStart;
            aprocessor.SetText(visualLeftPart, visualLeftLength, dir);
            subWidth = aprocessor.GetWidth();
            aprocessor.SetText(visualRightSide, visualLeftLength + 1, dir);
            posResolve->visualLeftTwips = xOffset + subWidth;
            posResolve->visualWidth = aprocessor.GetWidth() - subWidth;
          }
        }
      }

      if (dir == NSBIDI_LTR) {
        xOffset += width;
      }

      --subRunCount;
      start = lineOffset;
      subRunLimit = typeLimit;
      subRunLength = typeLimit - lineOffset;
    } 
    if (dir == NSBIDI_RTL) {
      xOffset = xEndRun;
    }

    visualStart += length;
  } 

  if (aWidth) {
    *aWidth = totalWidth;
  }
  return NS_OK;
}

class MOZ_STACK_CLASS nsIRenderingContextBidiProcessor final
  : public nsBidiPresUtils::BidiProcessor
{
public:
  nsIRenderingContextBidiProcessor(nsRenderingContext* aCtx,
                                   nsRenderingContext* aTextRunConstructionContext,
                                   nsFontMetrics* aFontMetrics,
                                   const nsPoint&       aPt)
    : mCtx(aCtx)
    , mTextRunConstructionContext(aTextRunConstructionContext)
    , mFontMetrics(aFontMetrics)
    , mPt(aPt)
  {}

  ~nsIRenderingContextBidiProcessor()
  {
    mFontMetrics->SetTextRunRTL(false);
  }

  virtual void SetText(const char16_t* aText,
                       int32_t         aLength,
                       nsBidiDirection aDirection) override
  {
    mFontMetrics->SetTextRunRTL(aDirection==NSBIDI_RTL);
    mText = aText;
    mLength = aLength;
  }

  virtual nscoord GetWidth() override
  {
    return nsLayoutUtils::AppUnitWidthOfString(mText, mLength, *mFontMetrics,
                                               *mTextRunConstructionContext);
  }

  virtual void DrawText(nscoord aIOffset,
                        nscoord) override
  {
    nsPoint pt(mPt);
    if (mFontMetrics->GetVertical()) {
      pt.y += aIOffset;
    } else {
      pt.x += aIOffset;
    }
    mFontMetrics->DrawString(mText, mLength, pt.x, pt.y,
                             mCtx, mTextRunConstructionContext);
  }

private:
  nsRenderingContext* mCtx;
  nsRenderingContext* mTextRunConstructionContext;
  nsFontMetrics* mFontMetrics;
  nsPoint mPt;
  const char16_t* mText;
  int32_t mLength;
};

nsresult nsBidiPresUtils::ProcessTextForRenderingContext(const char16_t*       aText,
                                                         int32_t                aLength,
                                                         nsBidiLevel            aBaseLevel,
                                                         nsPresContext*         aPresContext,
                                                         nsRenderingContext&   aRenderingContext,
                                                         nsRenderingContext&   aTextRunConstructionContext,
                                                         nsFontMetrics&         aFontMetrics,
                                                         Mode                   aMode,
                                                         nscoord                aX,
                                                         nscoord                aY,
                                                         nsBidiPositionResolve* aPosResolve,
                                                         int32_t                aPosResolveCount,
                                                         nscoord*               aWidth)
{
  nsIRenderingContextBidiProcessor processor(&aRenderingContext,
                                             &aTextRunConstructionContext,
                                             &aFontMetrics,
                                             nsPoint(aX, aY));
  nsBidi bidiEngine;
  return ProcessText(aText, aLength, aBaseLevel, aPresContext, processor,
                     aMode, aPosResolve, aPosResolveCount, aWidth, &bidiEngine);
}


void nsBidiPresUtils::WriteReverse(const char16_t* aSrc,
                                   uint32_t aSrcLength,
                                   char16_t* aDest)
{
  char16_t* dest = aDest + aSrcLength;
  mozilla::unicode::ClusterIterator iter(aSrc, aSrcLength);

  while (!iter.AtEnd()) {
    iter.Next();
    for (const char16_t *cp = iter; cp > aSrc; ) {
      
      
      *--dest = mozilla::unicode::GetMirroredChar(*--cp);
    }
    aSrc = iter;
  }

  NS_ASSERTION(dest == aDest, "Whole string not copied");
}


bool nsBidiPresUtils::WriteLogicalToVisual(const char16_t* aSrc,
                                           uint32_t aSrcLength,
                                           char16_t* aDest,
                                           nsBidiLevel aBaseDirection,
                                           nsBidi* aBidiEngine)
{
  const char16_t* src = aSrc;
  nsresult rv = aBidiEngine->SetPara(src, aSrcLength, aBaseDirection, nullptr);
  if (NS_FAILED(rv)) {
    return false;
  }

  nsBidiDirection dir;
  rv = aBidiEngine->GetDirection(&dir);
  
  if (NS_FAILED(rv) || dir == NSBIDI_LTR) {
    return false;
  }

  int32_t runCount;
  rv = aBidiEngine->CountRuns(&runCount);
  if (NS_FAILED(rv)) {
    return false;
  }

  int32_t runIndex, start, length;
  char16_t* dest = aDest;

  for (runIndex = 0; runIndex < runCount; ++runIndex) {
    rv = aBidiEngine->GetVisualRun(runIndex, &start, &length, &dir);
    if (NS_FAILED(rv)) {
      return false;
    }

    src = aSrc + start;

    if (dir == NSBIDI_RTL) {
      WriteReverse(src, length, dest);
      dest += length;
    } else {
      do {
        NS_ASSERTION(src >= aSrc && src < aSrc + aSrcLength,
                     "logical index out of range");
        NS_ASSERTION(dest < aDest + aSrcLength, "visual index out of range");
        *(dest++) = *(src++);
      } while (--length);
    }
  }

  NS_ASSERTION(static_cast<uint32_t>(dest - aDest) == aSrcLength,
               "whole string not copied");
  return true;
}

void nsBidiPresUtils::CopyLogicalToVisual(const nsAString& aSource,
                                          nsAString& aDest,
                                          nsBidiLevel aBaseDirection,
                                          bool aOverride)
{
  aDest.SetLength(0);
  uint32_t srcLength = aSource.Length();
  if (srcLength == 0)
    return;
  if (!aDest.SetLength(srcLength, fallible)) {
    return;
  }
  nsAString::const_iterator fromBegin, fromEnd;
  nsAString::iterator toBegin;
  aSource.BeginReading(fromBegin);
  aSource.EndReading(fromEnd);
  aDest.BeginWriting(toBegin);

  if (aOverride) {
    if (aBaseDirection == NSBIDI_RTL) {
      
      WriteReverse(fromBegin.get(), srcLength, toBegin.get());
    } else {
      
      
      aDest.SetLength(0);
    }
  } else {
    nsBidi bidiEngine;
    if (!WriteLogicalToVisual(fromBegin.get(), srcLength, toBegin.get(),
                             aBaseDirection, &bidiEngine)) {
      aDest.SetLength(0);
    }
  }

  if (aDest.IsEmpty()) {
    
    
    CopyUnicodeTo(aSource.BeginReading(fromBegin), aSource.EndReading(fromEnd),
                  aDest);
  }
}


nsBidiLevel
nsBidiPresUtils::BidiLevelFromStyle(nsStyleContext* aStyleContext)
{
  if (aStyleContext->StyleTextReset()->mUnicodeBidi &
      NS_STYLE_UNICODE_BIDI_PLAINTEXT) {
    return NSBIDI_DEFAULT_LTR;
  }

  if (aStyleContext->StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
    return NSBIDI_RTL;
  }

  return NSBIDI_LTR;
}
