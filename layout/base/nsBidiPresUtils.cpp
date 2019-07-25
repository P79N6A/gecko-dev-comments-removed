







































#ifdef IBMBIDI

#include "nsBidiPresUtils.h"
#include "nsTextFragment.h"
#include "nsGkAtoms.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsIServiceManager.h"
#include "nsFrameManager.h"
#include "nsBidiUtils.h"
#include "nsCSSFrameConstructor.h"
#include "nsHTMLContainerFrame.h"
#include "nsInlineFrame.h"
#include "nsPlaceholderFrame.h"
#include "nsContainerFrame.h"
#include "nsFirstLetterFrame.h"
#include "gfxUnicodeProperties.h"
#include "nsIThebesFontMetrics.h"
#include "nsTextFrame.h"

#undef NOISY_BIDI
#undef REALLY_NOISY_BIDI

using namespace mozilla;

static const PRUnichar kSpace            = 0x0020;
static const PRUnichar kLineSeparator    = 0x2028;
static const PRUnichar kObjectSubstitute = 0xFFFC;
static const PRUnichar kLRE              = 0x202A;
static const PRUnichar kRLE              = 0x202B;
static const PRUnichar kLRO              = 0x202D;
static const PRUnichar kRLO              = 0x202E;
static const PRUnichar kPDF              = 0x202C;

#define NS_BIDI_CONTROL_FRAME ((nsIFrame*)0xfffb1d1)

struct BidiParagraphData {
  nsString            mBuffer;
  nsAutoTArray<PRUnichar, 16> mEmbeddingStack;
  nsTArray<nsIFrame*> mLogicalFrames;
  nsDataHashtable<nsISupportsHashKey, PRInt32> mContentToFrameIndex;
  PRPackedBool        mIsVisual;
  nsBidiLevel         mParaLevel;
  nsIContent*         mPrevContent;
  nsAutoPtr<nsBidi>   mBidiEngine;

  void Init(nsBlockFrame *aBlockFrame)
  {
    mContentToFrameIndex.Init();
    mBidiEngine = new nsBidi();
    mPrevContent = nsnull;
    mParaLevel =
     (NS_STYLE_DIRECTION_RTL == aBlockFrame->GetStyleVisibility()->mDirection) ?
        NSBIDI_RTL : NSBIDI_LTR;

    mIsVisual = aBlockFrame->PresContext()->IsVisualMode();
    if (mIsVisual) {
      













      for (nsIContent* content = aBlockFrame->GetContent() ; content; 
           content = content->GetParent()) {
        if (content->IsNodeOfType(nsINode::eHTML_FORM_CONTROL) ||
            content->IsXUL()) {
          mIsVisual = PR_FALSE;
          break;
        }
      }
    }
  }

  nsresult SetPara()
  {
    return mBidiEngine->SetPara(mBuffer.get(), BufferLength(),
                                mParaLevel, nsnull);
  }

  nsresult CountRuns(PRInt32 *runCount){ return mBidiEngine->CountRuns(runCount); }

  nsresult GetLogicalRun(PRInt32 aLogicalStart, 
                         PRInt32* aLogicalLimit,
                         nsBidiLevel* aLevel)
  {
    nsresult rv = mBidiEngine->GetLogicalRun(aLogicalStart,
                                             aLogicalLimit, aLevel);
    if (mIsVisual || NS_FAILED(rv))
      *aLevel = mParaLevel;
    return rv;
  }

  void ResetData()
  {
    mLogicalFrames.Clear();
    mContentToFrameIndex.Clear();
    mBuffer.SetLength(0);
    for (PRUint32 i = 0; i < mEmbeddingStack.Length(); ++i) {
      mBuffer.Append(mEmbeddingStack[i]);
      mLogicalFrames.AppendElement(NS_BIDI_CONTROL_FRAME);
    }
  }

  void AppendFrame(nsIFrame* aFrame, nsIContent* aContent = nsnull)
  {
    if (aContent) {
      mContentToFrameIndex.Put(aContent, FrameCount());
    }
    mLogicalFrames.AppendElement(aFrame);
  }

  PRInt32 GetLastFrameForContent(nsIContent *aContent)
  {
    PRInt32 index = 0;
    mContentToFrameIndex.Get(aContent, &index);
    return index;
  }

  PRInt32 FrameCount(){ return mLogicalFrames.Length(); }

  PRInt32 BufferLength(){ return mBuffer.Length(); }

  nsIFrame* FrameAt(PRInt32 aIndex){ return mLogicalFrames[aIndex]; }

  void AppendUnichar(PRUnichar aCh){ mBuffer.Append(aCh); }

  void AppendString(const nsDependentSubstring& aString){ mBuffer.Append(aString); }

  void PushBidiControl(PRUnichar aCh)
  {
    mLogicalFrames.AppendElement(NS_BIDI_CONTROL_FRAME);
    mBuffer.Append(aCh);
    mEmbeddingStack.AppendElement(aCh);
  }

  void PopBidiControl()
  {
    mLogicalFrames.AppendElement(NS_BIDI_CONTROL_FRAME);
    mBuffer.Append(kPDF);
    NS_ASSERTION(mEmbeddingStack.Length(), "embedding/override underflow");
    mEmbeddingStack.TruncateLength(mEmbeddingStack.Length() - 1);
  }

  void ClearBidiControls()
  {
    for (PRUint32 i = 0; i < mEmbeddingStack.Length(); ++i) {
      mBuffer.Append(kPDF);
      mLogicalFrames.AppendElement(NS_BIDI_CONTROL_FRAME);
    }
  }
};

struct BidiLineData {
  nsTArray<nsIFrame*> mLogicalFrames;
  nsTArray<nsIFrame*> mVisualFrames;
  nsTArray<PRInt32> mIndexMap;
  nsAutoTArray<PRUint8, 18> mLevels;
  PRBool mIsReordered;

  BidiLineData(nsIFrame* aFirstFrameOnLine, PRInt32   aNumFramesOnLine)
  {
    



    mLogicalFrames.Clear();

    PRBool isReordered = PR_FALSE;
    PRBool hasRTLFrames = PR_FALSE;

    for (nsIFrame* frame = aFirstFrameOnLine;
         frame && aNumFramesOnLine--;
         frame = frame->GetNextSibling()) {
      AppendFrame(frame);
      PRUint8 level = nsBidiPresUtils::GetFrameEmbeddingLevel(frame);
      mLevels.AppendElement(level);
      mIndexMap.AppendElement(0);
      if (level & 1) {
        hasRTLFrames = PR_TRUE;
      }
    }

    
    nsBidi::ReorderVisual(mLevels.Elements(), FrameCount(),
                          mIndexMap.Elements());

    for (PRInt32 i = 0; i < FrameCount(); i++) {
      mVisualFrames.AppendElement(LogicalFrameAt(mIndexMap[i]));
      if (i != mIndexMap[i]) {
        isReordered = PR_TRUE;
      }
    }

    
    mIsReordered = isReordered || hasRTLFrames;
  }

  void AppendFrame(nsIFrame* aFrame){ mLogicalFrames.AppendElement(aFrame); }

  PRInt32 FrameCount(){ return mLogicalFrames.Length(); }

  nsIFrame* LogicalFrameAt(PRInt32 aIndex){ return mLogicalFrames[aIndex]; }

  nsIFrame* VisualFrameAt(PRInt32 aIndex){ return mVisualFrames[aIndex]; }
};




PRBool
IsBidiSplittable(nsIFrame* aFrame) {
  nsIAtom* frameType = aFrame->GetType();
  
  return aFrame->IsFrameOfType(nsIFrame::eBidiInlineContainer)
    && frameType != nsGkAtoms::lineFrame;
}

static nsresult
SplitInlineAncestors(nsIFrame*     aFrame)
{
  nsPresContext *presContext = aFrame->PresContext();
  nsIPresShell *presShell = presContext->PresShell();
  nsIFrame* frame = aFrame;
  nsIFrame* parent = aFrame->GetParent();
  nsIFrame* newParent;

  while (IsBidiSplittable(parent)) {
    nsIFrame* grandparent = parent->GetParent();
    NS_ASSERTION(grandparent, "Couldn't get parent's parent in nsBidiPresUtils::SplitInlineAncestors");
    
    nsresult rv = presShell->FrameConstructor()->
      CreateContinuingFrame(presContext, parent, grandparent, &newParent, PR_FALSE);
    if (NS_FAILED(rv)) {
      return rv;
    }
    
    
    nsContainerFrame* container = do_QueryFrame(parent);
    nsFrameList tail = container->StealFramesAfter(frame);

    
    rv = nsHTMLContainerFrame::ReparentFrameViewList(presContext, tail, parent, newParent);
    if (NS_FAILED(rv)) {
      return rv;
    }
    
    
    rv = newParent->InsertFrames(nsGkAtoms::nextBidi, nsnull, tail);
    if (NS_FAILED(rv)) {
      return rv;
    }
    
    nsFrameList temp(newParent, newParent);
    rv = grandparent->InsertFrames(nsGkAtoms::nextBidi, parent, temp);
    if (NS_FAILED(rv)) {
      return rv;
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
JoinInlineAncestors(nsIFrame* aFrame)
{
  if (aFrame->GetNextSibling()) {
    return;
  }
  nsIFrame* frame = aFrame->GetParent();
  while (frame && IsBidiSplittable(frame)) {
    nsIFrame* next = frame->GetNextContinuation();
    if (next) {
      MakeContinuationFluid(frame, next);
    }
    
    if (frame->GetNextSibling())
      break;
    frame = frame->GetParent();
  }
}

static nsresult
CreateContinuation(nsIFrame*       aFrame,
                   nsIFrame**      aNewFrame,
                   PRBool          aIsFluid)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  NS_PRECONDITION(aFrame, "null ptr");

  *aNewFrame = nsnull;

  nsPresContext *presContext = aFrame->PresContext();
  nsIPresShell *presShell = presContext->PresShell();
  NS_ASSERTION(presShell, "PresShell must be set on PresContext before calling nsBidiPresUtils::CreateContinuation");

  nsIFrame* parent = aFrame->GetParent();
  NS_ASSERTION(parent, "Couldn't get frame parent in nsBidiPresUtils::CreateContinuation");

  nsresult rv = NS_OK;
  
  
  
  
  if (parent->GetType() == nsGkAtoms::letterFrame &&
      parent->GetStyleDisplay()->IsFloating()) {
    nsFirstLetterFrame* letterFrame = do_QueryFrame(parent);
    rv = letterFrame->CreateContinuationForFloatingParent(presContext, aFrame,
                                                          aNewFrame, aIsFluid);
    return rv;
  }

  rv = presShell->FrameConstructor()->
    CreateContinuingFrame(presContext, aFrame, parent, aNewFrame, aIsFluid);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  nsFrameList temp(*aNewFrame, *aNewFrame);
  rv = parent->InsertFrames(nsGkAtoms::nextBidi, aFrame, temp);
  if (NS_FAILED(rv)) {
    return rv;
  }
  
  if (!aIsFluid) {
    
    rv = SplitInlineAncestors(aFrame);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  return NS_OK;
}

static PRBool
IsFrameInCurrentLine(nsBlockInFlowLineIterator* aLineIter,
                     nsIFrame* aPrevFrame, nsIFrame* aFrame)
{
  nsIFrame* endFrame = aLineIter->IsLastLineInList() ? nsnull :
    aLineIter->GetLine().next()->mFirstChild;
  nsIFrame* startFrame = aPrevFrame ? aPrevFrame : aLineIter->GetLine()->mFirstChild;
  for (nsIFrame* frame = startFrame; frame && frame != endFrame;
       frame = frame->GetNextSibling()) {
    if (frame == aFrame)
      return PR_TRUE;
  }
  return PR_FALSE;
}

static void
AdvanceLineIteratorToFrame(nsIFrame* aFrame,
                           nsBlockInFlowLineIterator* aLineIter,
                           nsIFrame*& aPrevFrame)
{
  
  nsIFrame* child = aFrame;
  nsFrameManager* frameManager = aFrame->PresContext()->FrameManager();
  nsIFrame* parent = nsLayoutUtils::GetParentOrPlaceholderFor(frameManager, child);
  while (parent && !nsLayoutUtils::GetAsBlock(parent)) {
    child = parent;
    parent = nsLayoutUtils::GetParentOrPlaceholderFor(frameManager, child);
  }
  NS_ASSERTION (parent, "aFrame is not a descendent of aBlockFrame");
  while (!IsFrameInCurrentLine(aLineIter, aPrevFrame, child)) {
#ifdef DEBUG
    PRBool hasNext =
#endif
      aLineIter->Next();
    NS_ASSERTION(hasNext, "Can't find frame in lines!");
    aPrevFrame = nsnull;
  }
  aPrevFrame = child;
}































nsresult
nsBidiPresUtils::Resolve(nsBlockFrame* aBlockFrame)
{
  BidiParagraphData bpd;
  bpd.Init(aBlockFrame);

  
  
  const nsStyleTextReset* text = aBlockFrame->GetStyleTextReset();
  PRUnichar ch = 0;
  if (text->mUnicodeBidi == NS_STYLE_UNICODE_BIDI_OVERRIDE) {
    const nsStyleVisibility* vis = aBlockFrame->GetStyleVisibility();
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
    TraverseFrames(aBlockFrame, block->GetFirstChild(nsnull), &bpd);
  }

  if (ch != 0) {
    bpd.PopBidiControl();
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
  aBpd->mBuffer.ReplaceChar("\t\r\n", kSpace);

  PRInt32 runCount;
  PRUint8 embeddingLevel = aBpd->mParaLevel;

  nsresult rv = aBpd->SetPara();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aBpd->CountRuns(&runCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32     runLength      = 0;   
  PRInt32     lineOffset     = 0;   
  PRInt32     logicalLimit   = 0;   
  PRInt32     numRun         = -1;
  PRInt32     fragmentLength = 0;   
  PRInt32     frameIndex     = -1;  
  PRInt32     frameCount     = aBpd->FrameCount();
  PRInt32     contentOffset  = 0;   
  PRBool      isTextFrame    = PR_FALSE;
  nsIFrame*   frame = nsnull;
  nsIContent* content = nsnull;
  PRInt32     contentTextLength = 0;

  FramePropertyTable *propTable = presContext->PropertyTable();
  
  nsBlockInFlowLineIterator lineIter(aBlockFrame,
                                     aBlockFrame->begin_lines(),
                                     PR_FALSE);
  if (lineIter.GetLine() == aBlockFrame->end_lines()) {
    
    lineIter.Next();
  }
  nsIFrame* prevFrame = nsnull;

  PRBool lineNeedsUpdate = PR_FALSE;
  
#ifdef DEBUG
#ifdef NOISY_BIDI
  printf("Before Resolve(), aBlockFrame=0x%p, mBuffer='%s', frameCount=%d\n",
         (void*)aBlockFrame, NS_ConvertUTF16toUTF8(aBpd->mBuffer).get(), frameCount);
#ifdef REALLY_NOISY_BIDI
  printf(" block frame tree=:\n");
  aBlockFrame->List(stdout, 0);
#endif
#endif
#endif

  for (; ;) {
    if (fragmentLength <= 0) {
      
      if (++frameIndex >= frameCount) {
        break;
      }
      frame = aBpd->FrameAt(frameIndex);
      lineNeedsUpdate = PR_TRUE;
      if (frame == NS_BIDI_CONTROL_FRAME ||
          nsGkAtoms::textFrame != frame->GetType()) {
        



        isTextFrame = PR_FALSE;
        fragmentLength = 1;
      }
      else {
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
                         NS_INT32_TO_PTR(aBpd->mParaLevel));
          continue;
        }
        PRInt32 start, end;
        frame->GetOffsets(start, end);
        NS_ASSERTION(!(contentTextLength < end - start),
                     "Frame offsets don't fit in content");
        fragmentLength = NS_MIN(contentTextLength, end - start);
        contentOffset = start;
        isTextFrame = PR_TRUE;
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
      frame = nsnull;
      ++lineOffset;
    }
    else {
      propTable->Set(frame, nsIFrame::EmbeddingLevelProperty(),
                     NS_INT32_TO_PTR(embeddingLevel));
      propTable->Set(frame, nsIFrame::BaseLevelProperty(),
                     NS_INT32_TO_PTR(aBpd->mParaLevel));
      if (isTextFrame) {
        if ( (runLength > 0) && (runLength < fragmentLength) ) {
          



          if (lineNeedsUpdate) {
            AdvanceLineIteratorToFrame(frame, &lineIter, prevFrame);
            lineNeedsUpdate = PR_FALSE;
          }
          lineIter.GetLine()->MarkDirty();
          nsIFrame* nextBidi;
          PRInt32 runEnd = contentOffset + runLength;
          rv = EnsureBidiContinuation(frame, &nextBidi, frameIndex,
                                      contentOffset,
                                      runEnd);
          if (NS_FAILED(rv)) {
            break;
          }
          nextBidi->AdjustOffsetsForBidi(runEnd,
                                         contentOffset + fragmentLength);
          frame = nextBidi;
          contentOffset = runEnd;
        } 
        else {
          if (contentOffset + fragmentLength == contentTextLength) {
            




            PRInt32 newIndex = aBpd->GetLastFrameForContent(content);
            if (newIndex > frameIndex) {
              RemoveBidiContinuation(aBpd, frame, 
                                     frameIndex, newIndex, lineOffset);
              frameIndex = newIndex;
            }
          } else if (fragmentLength > 0 && runLength > fragmentLength) {
            





            PRInt32 newIndex = frameIndex;
            do {
            } while (++newIndex < frameCount &&
                     aBpd->FrameAt(newIndex) == NS_BIDI_CONTROL_FRAME);
            if (newIndex < frameCount) {
              RemoveBidiContinuation(aBpd, frame, 
                                     frameIndex, newIndex, lineOffset);
            }
          } else if (runLength == fragmentLength &&
                     numRun + 1 < runCount) {
            




            nsIFrame* next = frame->GetNextInFlow();
            if (next) {
              frame->SetNextContinuation(next);
              next->SetPrevContinuation(frame);
            }
          }
          frame->AdjustOffsetsForBidi(contentOffset, contentOffset + fragmentLength);
          if (lineNeedsUpdate) {
            AdvanceLineIteratorToFrame(frame, &lineIter, prevFrame);
            lineNeedsUpdate = PR_FALSE;
          }
          lineIter.GetLine()->MarkDirty();
        }
      } 
      else {
        ++lineOffset;
      }
    } 
    PRInt32 temp = runLength;
    runLength -= fragmentLength;
    fragmentLength -= temp;

    if (frame && fragmentLength <= 0) {
      
      
      
      
      
      
      if (numRun + 1 < runCount && runLength <= 0 && !frame->GetNextInFlow()) {
        nsIFrame* child = frame;
        nsIFrame* parent = frame->GetParent();
        
        
        
        
        
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
        if (parent && IsBidiSplittable(parent))
          SplitInlineAncestors(child);
      }
      else {
        
        
        
        JoinInlineAncestors(frame);
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


PRBool IsBidiLeaf(nsIFrame* aFrame) {
  nsIFrame* kid = aFrame->GetFirstChild(nsnull);
  return !kid
    || !aFrame->IsFrameOfType(nsIFrame::eBidiInlineContainer);
}

void
nsBidiPresUtils::TraverseFrames(nsBlockFrame* aBlockFrame,
                                nsIFrame*     aCurrentFrame,
                                BidiParagraphData* aBpd)
{
  if (!aCurrentFrame)
    return;

  nsIFrame* childFrame = aCurrentFrame;
  do {
    







    nsIFrame* nextSibling = childFrame->GetNextSibling();
    PRBool isLastFrame = !childFrame->GetNextContinuation();

    
    
    
    nsIFrame* frame = childFrame;
    if (nsGkAtoms::placeholderFrame == childFrame->GetType()) {
      nsIFrame* realFrame =
        nsPlaceholderFrame::GetRealFrameForPlaceholder(childFrame);
      if (realFrame->GetType() == nsGkAtoms::letterFrame) {
        frame = realFrame;
      }
    }

    PRUnichar ch = 0;
    if (frame->IsFrameOfType(nsIFrame::eBidiInlineContainer)) {
      const nsStyleVisibility* vis = frame->GetStyleVisibility();
      const nsStyleTextReset* text = frame->GetStyleTextReset();
      switch (text->mUnicodeBidi) {
        case NS_STYLE_UNICODE_BIDI_NORMAL:
          break;
        case NS_STYLE_UNICODE_BIDI_EMBED:
          if (NS_STYLE_DIRECTION_RTL == vis->mDirection) {
            ch = kRLE;
          }
          else if (NS_STYLE_DIRECTION_LTR == vis->mDirection) {
            ch = kLRE;
          }
          break;
        case NS_STYLE_UNICODE_BIDI_OVERRIDE:
          if (NS_STYLE_DIRECTION_RTL == vis->mDirection) {
            ch = kRLO;
          }
          else if (NS_STYLE_DIRECTION_LTR == vis->mDirection) {
            ch = kLRO;
          }
          break;
      }

      
      
      if (ch != 0 && !frame->GetPrevContinuation()) {
        aBpd->PushBidiControl(ch);
      }
    }

    if (IsBidiLeaf(frame)) {
      




      nsIContent* content = frame->GetContent();
      aBpd->AppendFrame(frame, content);

      
      nsIAtom* frameType = frame->GetType();
      if (nsGkAtoms::textFrame == frameType) {
        if (content != aBpd->mPrevContent) {
          aBpd->mPrevContent = content;
          if (!frame->GetStyleContext()->GetStyleText()->NewlineIsSignificant()) {
            content->AppendTextTo(aBpd->mBuffer);
          } else {
            



            nsAutoString text;
            content->AppendTextTo(text);
            nsIFrame* next;
            do {
              next = nsnull;

              PRInt32 start, end;
              frame->GetOffsets(start, end);
              PRInt32 endLine = text.FindCharInSet(NS_LITERAL_STRING("\n\r"),
                                                   start);
              if (endLine == -1) {
                



                aBpd->AppendString(Substring(text, start));
                break;
              }

              




              ++endLine;

              



              while (end < endLine) {
                aBpd->AppendString(Substring(text, start, end - start));
                frame = frame->GetNextContinuation();
                NS_ASSERTION(frame, "Premature end of continuation chain");
                frame->GetOffsets(start, end);
                aBpd->AppendFrame(frame);

                



                if (frame == nextSibling) {
                  nextSibling = frame->GetNextSibling();
                }
              }

              aBpd->AppendString(Substring(text, start, endLine - start));

              if (PRUint32(endLine) < text.Length()) {
                next = frame->GetNextInFlow();
                if (!next) {
                  
                  next = frame->GetNextContinuation();
                  if (next) {
                    MakeContinuationFluid(frame, next);
                    JoinInlineAncestors(frame);
                  } else {
                    
                    CreateContinuation(frame, &next, PR_TRUE);
                  }
                }
                nsTextFrame* textFrame = static_cast<nsTextFrame*>(frame);
                textFrame->SetLength(endLine - start, nsnull);
              }
              ResolveParagraphWithinBlock(aBlockFrame, aBpd);

              if (next) {
                frame = next;
                aBpd->AppendFrame(frame);
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
        
        
        
        aBpd->AppendUnichar(kObjectSubstitute);
        if (!frame->GetStyleContext()->GetStyleDisplay()->IsInlineOutside()) {
          
          ResolveParagraphWithinBlock(aBlockFrame, aBpd);
        }
      }
    }
    else {
      
      nsIFrame* kid = frame->GetFirstChild(nsnull);
      TraverseFrames(aBlockFrame, kid, aBpd);
    }

    
    if (ch != 0 && isLastFrame) {
      
      
      aBpd->PopBidiControl();
    }
    childFrame = nextSibling;
  } while (childFrame);
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
nsBidiPresUtils::ReorderFrames(nsIFrame*            aFirstFrameOnLine,
                               PRInt32              aNumFramesOnLine)
{
  
  if (aFirstFrameOnLine->GetType() == nsGkAtoms::lineFrame) {
    aFirstFrameOnLine = aFirstFrameOnLine->GetFirstChild(nsnull);
    if (!aFirstFrameOnLine)
      return;
    
    
    aNumFramesOnLine = -1;
  }
  BidiLineData bld(aFirstFrameOnLine, aNumFramesOnLine);
  RepositionInlineFrames(&bld, aFirstFrameOnLine);
}

nsBidiLevel
nsBidiPresUtils::GetFrameEmbeddingLevel(nsIFrame* aFrame)
{
  nsIFrame* firstLeaf = aFrame;
  while (!IsBidiLeaf(firstLeaf)) {
    nsIFrame* firstChild = firstLeaf->GetFirstChild(nsnull);
    nsIFrame* realFrame = nsPlaceholderFrame::GetRealFrameFor(firstChild);
    firstLeaf = (realFrame->GetType() == nsGkAtoms::letterFrame) ?
                 realFrame : firstChild;
  }
  return NS_GET_EMBEDDING_LEVEL(firstLeaf);
}

nsBidiLevel
nsBidiPresUtils::GetFrameBaseLevel(nsIFrame* aFrame)
{
  nsIFrame* firstLeaf = aFrame;
  while (!IsBidiLeaf(firstLeaf)) {
    firstLeaf = firstLeaf->GetFirstChild(nsnull);
  }
  return NS_GET_BASE_LEVEL(firstLeaf);
}

void
nsBidiPresUtils::IsLeftOrRightMost(nsIFrame*              aFrame,
                                   nsContinuationStates*  aContinuationStates,
                                   PRBool&                aIsLeftMost ,
                                   PRBool&                aIsRightMost )
{
  const nsStyleVisibility* vis = aFrame->GetStyleVisibility();
  PRBool isLTR = (NS_STYLE_DIRECTION_LTR == vis->mDirection);

  









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
    frameState->mHasContOnPrevLines = (frame != nsnull);

    
    for (frame = aFrame->GetNextContinuation();
         frame && (contState = aContinuationStates->GetEntry(frame));
         frame = frame->GetNextContinuation()) {
      frameState->mFrameCount++;
      contState->mFirstVisualFrame = aFrame;
    }
    frameState->mHasContOnNextLines = (frame != nsnull);

    aIsLeftMost = isLTR ? !frameState->mHasContOnPrevLines
                        : !frameState->mHasContOnNextLines;
    firstFrameState = frameState;
  } else {
    
    aIsLeftMost = PR_FALSE;
    firstFrameState = aContinuationStates->GetEntry(frameState->mFirstVisualFrame);
  }

  aIsRightMost = (firstFrameState->mFrameCount == 1) &&
                 (isLTR ? !firstFrameState->mHasContOnNextLines
                        : !firstFrameState->mHasContOnPrevLines);

  if ((aIsLeftMost || aIsRightMost) &&
      (aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL)) {
    
    
    
    nsIFrame* firstContinuation = aFrame->GetFirstContinuation();
    if (nsLayoutUtils::FrameIsNonLastInIBSplit(firstContinuation)) {
      
      if (isLTR) {
        aIsRightMost = PR_FALSE;
      } else {
        aIsLeftMost = PR_FALSE;
      }
    }
    if (nsLayoutUtils::FrameIsNonFirstInIBSplit(firstContinuation)) {
      
      if (isLTR) {
        aIsLeftMost = PR_FALSE;
      } else {
        aIsRightMost = PR_FALSE;
      }
    }
  }

  
  firstFrameState->mFrameCount--;
}

void
nsBidiPresUtils::RepositionFrame(nsIFrame*              aFrame,
                                 PRBool                 aIsOddLevel,
                                 nscoord&               aLeft,
                                 nsContinuationStates*  aContinuationStates)
{
  if (!aFrame)
    return;

  PRBool isLeftMost, isRightMost;
  IsLeftOrRightMost(aFrame,
                    aContinuationStates,
                    isLeftMost ,
                    isRightMost );

  nsInlineFrame* testFrame = do_QueryFrame(aFrame);
  if (testFrame) {
    aFrame->AddStateBits(NS_INLINE_FRAME_BIDI_VISUAL_STATE_IS_SET);

    if (isLeftMost)
      aFrame->AddStateBits(NS_INLINE_FRAME_BIDI_VISUAL_IS_LEFT_MOST);
    else
      aFrame->RemoveStateBits(NS_INLINE_FRAME_BIDI_VISUAL_IS_LEFT_MOST);

    if (isRightMost)
      aFrame->AddStateBits(NS_INLINE_FRAME_BIDI_VISUAL_IS_RIGHT_MOST);
    else
      aFrame->RemoveStateBits(NS_INLINE_FRAME_BIDI_VISUAL_IS_RIGHT_MOST);
  }
  
  
  
  nsMargin margin = aFrame->GetUsedMargin();
  if (isLeftMost)
    aLeft += margin.left;

  nscoord start = aLeft;

  if (!IsBidiLeaf(aFrame))
  {
    nscoord x = 0;
    nsMargin borderPadding = aFrame->GetUsedBorderAndPadding();
    if (isLeftMost) {
      x += borderPadding.left;
    }

    
    
    
    nsTArray<nsIFrame*> childList;
    nsIFrame *frame = aFrame->GetFirstChild(nsnull);
    if (frame && aIsOddLevel) {
      childList.AppendElement((nsIFrame*)nsnull);
      while (frame) {
        childList.AppendElement(frame);
        frame = frame->GetNextSibling();
      }
      frame = childList[childList.Length() - 1];
    }

    
    PRInt32 index = 0;
    while (frame) {
      RepositionFrame(frame,
                      aIsOddLevel,
                      x,
                      aContinuationStates);
      index++;
      frame = aIsOddLevel ?
                childList[childList.Length() - index - 1] :
                frame->GetNextSibling();
    }

    if (isRightMost) {
      x += borderPadding.right;
    }
    aLeft += x;
  } else {
    aLeft += aFrame->GetSize().width;
  }
  nsRect rect = aFrame->GetRect();
  aFrame->SetRect(nsRect(start, rect.y, aLeft - start, rect.height));

  if (isRightMost)
    aLeft += margin.right;
}

void
nsBidiPresUtils::InitContinuationStates(nsIFrame*              aFrame,
                                        nsContinuationStates*  aContinuationStates)
{
  nsFrameContinuationState* state = aContinuationStates->PutEntry(aFrame);
  state->mFirstVisualFrame = nsnull;
  state->mFrameCount = 0;

  if (!IsBidiLeaf(aFrame)) {
    
    nsIFrame* frame;
    for (frame = aFrame->GetFirstChild(nsnull);
         frame;
         frame = frame->GetNextSibling()) {
      InitContinuationStates(frame,
                             aContinuationStates);
    }
  }
}

void
nsBidiPresUtils::RepositionInlineFrames(BidiLineData *aBld,
                                        nsIFrame* aFirstChild)
{
  const nsStyleVisibility* vis = aFirstChild->GetStyleVisibility();
  PRBool isLTR = (NS_STYLE_DIRECTION_LTR == vis->mDirection);
  nscoord leftSpace = 0;

  
  
  
  nsMargin margin = aFirstChild->GetUsedMargin();
  if (!aFirstChild->GetPrevContinuation() &&
      !nsLayoutUtils::FrameIsNonFirstInIBSplit(aFirstChild))
    leftSpace = isLTR ? margin.left : margin.right;

  nscoord left = aFirstChild->GetPosition().x - leftSpace;
  nsIFrame* frame;
  PRInt32 count = aBld->mVisualFrames.Length();
  PRInt32 index;
  nsContinuationStates continuationStates;

  continuationStates.Init();

  
  
  for (index = 0; index < count; index++) {
    InitContinuationStates(aBld->VisualFrameAt(index), &continuationStates);
  }

  
  for (index = 0; index < count; index++) {
    frame = aBld->VisualFrameAt(index);
    RepositionFrame(frame,
                    (aBld->mLevels[aBld->mIndexMap[index]] & 1),
                    left,
                    &continuationStates);
  } 
}

PRBool
nsBidiPresUtils::CheckLineOrder(nsIFrame*  aFirstFrameOnLine,
                                PRInt32    aNumFramesOnLine,
                                nsIFrame** aFirstVisual,
                                nsIFrame** aLastVisual)
{
  BidiLineData bld(aFirstFrameOnLine, aNumFramesOnLine);
  PRInt32 count = bld.FrameCount();
  
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
                                   PRInt32          aNumFramesOnLine)
{
  BidiLineData bld(aFirstFrameOnLine, aNumFramesOnLine);

  PRInt32 count = bld.mVisualFrames.Length();

  if (aFrame == nsnull)
    return bld.VisualFrameAt(0);
  
  for (PRInt32 i = 0; i < count - 1; i++) {
    if (bld.VisualFrameAt(i) == aFrame) {
      return bld.VisualFrameAt(i+1);
    }
  }
  
  return nsnull;
}

nsIFrame*
nsBidiPresUtils::GetFrameToLeftOf(const nsIFrame*  aFrame,
                                  nsIFrame*        aFirstFrameOnLine,
                                  PRInt32          aNumFramesOnLine)
{
  BidiLineData bld(aFirstFrameOnLine, aNumFramesOnLine);

  PRInt32 count = bld.mVisualFrames.Length();
  
  if (aFrame == nsnull)
    return bld.VisualFrameAt(count-1);
  
  for (PRInt32 i = 1; i < count; i++) {
    if (bld.VisualFrameAt(i) == aFrame) {
      return bld.VisualFrameAt(i-1);
    }
  }
  
  return nsnull;
}

inline nsresult
nsBidiPresUtils::EnsureBidiContinuation(nsIFrame*       aFrame,
                                        nsIFrame**      aNewFrame,
                                        PRInt32&        aFrameIndex,
                                        PRInt32         aStart,
                                        PRInt32         aEnd)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  NS_PRECONDITION(aFrame, "aFrame is null");

  aFrame->AdjustOffsetsForBidi(aStart, aEnd);
  return CreateContinuation(aFrame, aNewFrame, PR_FALSE);
}

void
nsBidiPresUtils::RemoveBidiContinuation(BidiParagraphData *aBpd,
                                        nsIFrame*       aFrame,
                                        PRInt32         aFirstIndex,
                                        PRInt32         aLastIndex,
                                        PRInt32&        aOffset)
{
  FrameProperties props = aFrame->Properties();
  nsBidiLevel embeddingLevel =
    (nsBidiLevel)NS_PTR_TO_INT32(props.Get(nsIFrame::EmbeddingLevelProperty()));
  nsBidiLevel baseLevel =
    (nsBidiLevel)NS_PTR_TO_INT32(props.Get(nsIFrame::BaseLevelProperty()));

  for (PRInt32 index = aFirstIndex + 1; index <= aLastIndex; index++) {
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
}

nsresult
nsBidiPresUtils::FormatUnicodeText(nsPresContext*  aPresContext,
                                   PRUnichar*       aText,
                                   PRInt32&         aTextLength,
                                   nsCharType       aCharType,
                                   PRBool           aIsOddLevel)
{
  NS_ASSERTION(aIsOddLevel == 0 || aIsOddLevel == 1, "aIsOddLevel should be 0 or 1");
  nsresult rv = NS_OK;
  
  
  PRUint32 bidiOptions = aPresContext->GetBidi();
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
nsBidiPresUtils::StripBidiControlCharacters(PRUnichar* aText,
                                            PRInt32&   aTextLength)
{
  if ( (nsnull == aText) || (aTextLength < 1) ) {
    return;
  }

  PRInt32 stripLen = 0;

  for (PRInt32 i = 0; i < aTextLength; i++) {
    
    
    if (IsBidiControl((PRUint32)aText[i])) {
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
RemoveDiacritics(PRUnichar* aText,
                 PRInt32&   aTextLength)
{
  if (aText && (aTextLength > 0) ) {
    PRInt32 offset = 0;

    for (PRInt32 i = 0; i < aTextLength && aText[i]; i++) {
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
                                   const PRUnichar* aText,
                                   PRInt32& aOffset,
                                   PRInt32  aCharTypeLimit,
                                   PRInt32& aRunLimit,
                                   PRInt32& aRunLength,
                                   PRInt32& aRunCount,
                                   PRUint8& aCharType,
                                   PRUint8& aPrevCharType)

{
  PRBool     strongTypeFound = PR_FALSE;
  PRInt32    offset;
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

      strongTypeFound = PR_TRUE;
      aCharType = charType;
    }
  }
  aOffset = offset;
}

nsresult nsBidiPresUtils::ProcessText(const PRUnichar*       aText,
                                      PRInt32                aLength,
                                      nsBidiDirection        aBaseDirection,
                                      nsPresContext*         aPresContext,
                                      BidiProcessor&         aprocessor,
                                      Mode                   aMode,
                                      nsBidiPositionResolve* aPosResolve,
                                      PRInt32                aPosResolveCount,
                                      nscoord*               aWidth,
                                      nsBidi*                aBidiEngine)
{
  NS_ASSERTION((aPosResolve == nsnull) != (aPosResolveCount > 0), "Incorrect aPosResolve / aPosResolveCount arguments");

  PRInt32 runCount;

  nsAutoString textBuffer(aText, aLength);

  nsresult rv = aBidiEngine->SetPara(aText, aLength, aBaseDirection, nsnull);
  if (NS_FAILED(rv))
    return rv;

  rv = aBidiEngine->CountRuns(&runCount);
  if (NS_FAILED(rv))
    return rv;

  nscoord xOffset = 0;
  nscoord width, xEndRun = 0;
  nscoord totalWidth = 0;
  PRInt32 i, start, limit, length;
  PRUint32 visualStart = 0;
  PRUint8 charType;
  PRUint8 prevType = eCharType_LeftToRight;
  nsBidiLevel level;
      
  for(int nPosResolve=0; nPosResolve < aPosResolveCount; ++nPosResolve)
  {
    aPosResolve[nPosResolve].visualIndex = kNotFound;
    aPosResolve[nPosResolve].visualLeftTwips = kNotFound;
    aPosResolve[nPosResolve].visualWidth = kNotFound;
  }

  for (i = 0; i < runCount; i++) {
    rv = aBidiEngine->GetVisualRun(i, &start, &length, &aBaseDirection);
    if (NS_FAILED(rv))
      return rv;

    rv = aBidiEngine->GetLogicalRun(start, &limit, &level);
    if (NS_FAILED(rv))
      return rv;

    PRInt32 subRunLength = limit - start;
    PRInt32 lineOffset = start;
    PRInt32 typeLimit = NS_MIN(limit, aLength);
    PRInt32 subRunCount = 1;
    PRInt32 subRunLimit = typeLimit;

    











    if (level & 1) {
      aprocessor.SetText(aText + start, subRunLength, nsBidiDirection(level & 1));
      width = aprocessor.GetWidth();
      xOffset += width;
      xEndRun = xOffset;
    }

    while (subRunCount > 0) {
      
      
      CalculateCharType(aBidiEngine, aText, lineOffset, typeLimit, subRunLimit, subRunLength, subRunCount, charType, prevType);
      
      nsAutoString runVisualText;
      runVisualText.Assign(aText + start, subRunLength);
      if (PRInt32(runVisualText.Length()) < subRunLength)
        return NS_ERROR_OUT_OF_MEMORY;
      FormatUnicodeText(aPresContext, runVisualText.BeginWriting(), subRunLength,
                        (nsCharType)charType, level & 1);

      aprocessor.SetText(runVisualText.get(), subRunLength, nsBidiDirection(level & 1));
      width = aprocessor.GetWidth();
      totalWidth += width;
      if (level & 1) {
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
            
            const PRUnichar* visualLeftPart, *visualRightSide;
            if (level & 1) {
              
              posResolve->visualIndex = visualStart + (subRunLength - (posResolve->logicalIndex + 1 - start));
              
              visualLeftPart = aText + posResolve->logicalIndex + 1;
              
              visualRightSide = visualLeftPart - 1;
            }
            else {
              posResolve->visualIndex = visualStart + (posResolve->logicalIndex - start);
              
              visualLeftPart = aText + start;
              
              visualRightSide = visualLeftPart;
            }
            
            PRInt32 visualLeftLength = posResolve->visualIndex - visualStart;
            aprocessor.SetText(visualLeftPart, visualLeftLength, nsBidiDirection(level & 1));
            subWidth = aprocessor.GetWidth();
            aprocessor.SetText(visualRightSide, visualLeftLength + 1, nsBidiDirection(level & 1));
            posResolve->visualLeftTwips = xOffset + subWidth;
            posResolve->visualWidth = aprocessor.GetWidth() - subWidth;
          }
        }
      }

      if (!(level & 1)) {
        xOffset += width;
      }

      --subRunCount;
      start = lineOffset;
      subRunLimit = typeLimit;
      subRunLength = typeLimit - lineOffset;
    } 
    if (level & 1) {
      xOffset = xEndRun;
    }
    
    visualStart += length;
  } 

  if (aWidth) {
    *aWidth = totalWidth;
  }
  return NS_OK;
}

class NS_STACK_CLASS nsIRenderingContextBidiProcessor : public nsBidiPresUtils::BidiProcessor {
public:
  nsIRenderingContextBidiProcessor(nsRenderingContext* aCtx,
                                   nsRenderingContext* aTextRunConstructionContext,
                                   const nsPoint&       aPt)
    : mCtx(aCtx), mTextRunConstructionContext(aTextRunConstructionContext), mPt(aPt) { }

  ~nsIRenderingContextBidiProcessor()
  {
    mCtx->SetRightToLeftText(PR_FALSE);
  }

  virtual void SetText(const PRUnichar* aText,
                       PRInt32          aLength,
                       nsBidiDirection  aDirection)
  {
    mTextRunConstructionContext->SetTextRunRTL(aDirection==NSBIDI_RTL);
    mText = aText;
    mLength = aLength;
  }

  virtual nscoord GetWidth()
  {
    return mTextRunConstructionContext->GetWidth(mText, mLength);
  }

  virtual void DrawText(nscoord aXOffset,
                        nscoord)
  {
    nsCOMPtr<nsIFontMetrics> metrics = mCtx->GetFontMetrics();
    nsIThebesFontMetrics* fm = static_cast<nsIThebesFontMetrics*>(metrics.get());
    fm->DrawString(mText, mLength, mPt.x + aXOffset, mPt.y,
                   mCtx, mTextRunConstructionContext);
  }

private:
  nsRenderingContext* mCtx;
  nsRenderingContext* mTextRunConstructionContext;
  nsPoint mPt;
  const PRUnichar* mText;
  PRInt32 mLength;
  nsBidiDirection mDirection;
};

nsresult nsBidiPresUtils::ProcessTextForRenderingContext(const PRUnichar*       aText,
                                                         PRInt32                aLength,
                                                         nsBidiDirection        aBaseDirection,
                                                         nsPresContext*         aPresContext,
                                                         nsRenderingContext&   aRenderingContext,
                                                         nsRenderingContext&   aTextRunConstructionContext,
                                                         Mode                   aMode,
                                                         nscoord                aX,
                                                         nscoord                aY,
                                                         nsBidiPositionResolve* aPosResolve,
                                                         PRInt32                aPosResolveCount,
                                                         nscoord*               aWidth)
{
  nsIRenderingContextBidiProcessor processor(&aRenderingContext, &aTextRunConstructionContext, nsPoint(aX, aY));
  nsBidi bidiEngine;
  return ProcessText(aText, aLength, aBaseDirection, aPresContext, processor,
                     aMode, aPosResolve, aPosResolveCount, aWidth, &bidiEngine);
}


void nsBidiPresUtils::WriteReverse(const PRUnichar* aSrc,
                                   PRUint32 aSrcLength,
                                   PRUnichar* aDest)
{
  const PRUnichar* src = aSrc + aSrcLength;
  PRUnichar* dest = aDest;
  PRUint32 UTF32Char;

  while (--src >= aSrc) {
    if (NS_IS_LOW_SURROGATE(*src)) {
      if (src > aSrc && NS_IS_HIGH_SURROGATE(*(src - 1))) {
        UTF32Char = SURROGATE_TO_UCS4(*(src - 1), *src);
        --src;
      } else {
        UTF32Char = UCS2_REPLACEMENT_CHAR;
      }
    } else if (NS_IS_HIGH_SURROGATE(*src)) {
      
      UTF32Char = UCS2_REPLACEMENT_CHAR;
    } else {
      UTF32Char = *src;
    }

    UTF32Char = gfxUnicodeProperties::GetMirroredChar(UTF32Char);

    if (IS_IN_BMP(UTF32Char)) {
      *(dest++) = UTF32Char;
    } else {
      *(dest++) = H_SURROGATE(UTF32Char);
      *(dest++) = L_SURROGATE(UTF32Char);
    }
  }

  NS_ASSERTION(dest - aDest == aSrcLength, "Whole string not copied");
}


PRBool nsBidiPresUtils::WriteLogicalToVisual(const PRUnichar* aSrc,
                                             PRUint32 aSrcLength,
                                             PRUnichar* aDest,
                                             nsBidiLevel aBaseDirection,
                                             nsBidi* aBidiEngine)
{
  const PRUnichar* src = aSrc;
  nsresult rv = aBidiEngine->SetPara(src, aSrcLength, aBaseDirection, nsnull);
  if (NS_FAILED(rv)) {
    return PR_FALSE;
  }

  nsBidiDirection dir;
  rv = aBidiEngine->GetDirection(&dir);
  
  if (NS_FAILED(rv) || dir == NSBIDI_LTR) {
    return PR_FALSE;
  }

  PRInt32 runCount;
  rv = aBidiEngine->CountRuns(&runCount);
  if (NS_FAILED(rv)) {
    return PR_FALSE;
  }

  PRInt32 runIndex, start, length;
  PRUnichar* dest = aDest;

  for (runIndex = 0; runIndex < runCount; ++runIndex) {
    rv = aBidiEngine->GetVisualRun(runIndex, &start, &length, &dir);
    if (NS_FAILED(rv)) {
      return PR_FALSE;
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

  NS_ASSERTION(dest - aDest == aSrcLength, "whole string not copied");
  return PR_TRUE;
}

void nsBidiPresUtils::CopyLogicalToVisual(const nsAString& aSource,
                                          nsAString& aDest,
                                          nsBidiLevel aBaseDirection,
                                          PRBool aOverride)
{
  aDest.SetLength(0);
  PRUint32 srcLength = aSource.Length();
  if (srcLength == 0)
    return;
  if (!EnsureStringLength(aDest, srcLength)) {
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
#endif 
