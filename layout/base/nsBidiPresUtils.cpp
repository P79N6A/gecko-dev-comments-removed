







































#ifdef IBMBIDI

#include "nsBidiPresUtils.h"
#include "nsTextFragment.h"
#include "nsGkAtoms.h"
#include "nsPresContext.h"
#include "nsIRenderingContext.h"
#include "nsIServiceManager.h"
#include "nsFrameManager.h"
#include "nsBidiFrames.h"
#include "nsBidiUtils.h"
#include "nsCSSFrameConstructor.h"
#include "nsHTMLContainerFrame.h"
#include "nsInlineFrame.h"
#include "nsPlaceholderFrame.h"
#include "nsContainerFrame.h"

static const PRUnichar kSpace            = 0x0020;
static const PRUnichar kLineSeparator    = 0x2028;
static const PRUnichar kObjectSubstitute = 0xFFFC;
static const PRUnichar kLRE              = 0x202A;
static const PRUnichar kRLE              = 0x202B;
static const PRUnichar kLRO              = 0x202D;
static const PRUnichar kRLO              = 0x202E;
static const PRUnichar kPDF              = 0x202C;
static const PRUnichar ALEF              = 0x05D0;

#define CHAR_IS_HEBREW(c) ((0x0590 <= (c)) && ((c)<= 0x05FF))


nsIFrame*
NS_NewDirectionalFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRUnichar aChar);

nsBidiPresUtils::nsBidiPresUtils() : mArraySize(8),
                                     mIndexMap(nsnull),
                                     mLevels(nsnull),
                                     mSuccess(NS_ERROR_FAILURE),
                                     mBidiEngine(nsnull)
{
  mBidiEngine = new nsBidi();
  if (mBidiEngine && mContentToFrameIndex.Init()) {
    mSuccess = NS_OK;
  }
}

nsBidiPresUtils::~nsBidiPresUtils()
{
  if (mLevels) {
    delete[] mLevels;
  }
  if (mIndexMap) {
    delete[] mIndexMap;
  }
  delete mBidiEngine;
}

PRBool
nsBidiPresUtils::IsSuccessful() const
{ 
  return NS_SUCCEEDED(mSuccess); 
}




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
JoinInlineAncestors(nsIFrame* aFrame)
{
  nsIFrame* frame = aFrame;
  while (frame && IsBidiSplittable(frame)) {
    nsIFrame* next = frame->GetNextContinuation();
    if (next) {
      NS_ASSERTION (!frame->GetNextInFlow() || frame->GetNextInFlow() == next, 
                    "next-in-flow is not next continuation!");
      frame->SetNextInFlow(next);

      NS_ASSERTION (!next->GetPrevInFlow() || next->GetPrevInFlow() == frame,
                    "prev-in-flow is not prev continuation!");
      next->SetPrevInFlow(frame);
    }
    
    if (frame->GetNextSibling())
      break;
    frame = frame->GetParent();
  }
}

static nsresult
CreateBidiContinuation(nsIFrame*       aFrame,
                       nsIFrame**      aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  NS_PRECONDITION(aFrame, "null ptr");

  *aNewFrame = nsnull;

  nsPresContext *presContext = aFrame->PresContext();
  nsIPresShell *presShell = presContext->PresShell();
  NS_ASSERTION(presShell, "PresShell must be set on PresContext before calling nsBidiPresUtils::CreateBidiContinuation");

  nsIFrame* parent = aFrame->GetParent();
  NS_ASSERTION(parent, "Couldn't get frame parent in nsBidiPresUtils::CreateBidiContinuation");
  
  nsresult rv = presShell->FrameConstructor()->
    CreateContinuingFrame(presContext, aFrame, parent, aNewFrame, PR_FALSE);
  if (NS_FAILED(rv)) {
    return rv;
  }
  
  
  
  nsFrameList temp(*aNewFrame, *aNewFrame);
  rv = parent->InsertFrames(nsGkAtoms::nextBidi, aFrame, temp);
  if (NS_FAILED(rv)) {
    return rv;
  }
  
  
  rv = SplitInlineAncestors(aFrame);
  if (NS_FAILED(rv)) {
    return rv;
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
nsBidiPresUtils::Resolve(nsBlockFrame*   aBlockFrame,
                         PRBool          aIsVisualFormControl)
{
  mLogicalFrames.Clear();
  mContentToFrameIndex.Clear();
  
  nsPresContext *presContext = aBlockFrame->PresContext();
  nsIPresShell* shell = presContext->PresShell();
  nsStyleContext* styleContext = aBlockFrame->GetStyleContext();

  
  
  const nsStyleVisibility* vis = aBlockFrame->GetStyleVisibility();
  const nsStyleTextReset* text = aBlockFrame->GetStyleTextReset();

  if (text->mUnicodeBidi == NS_STYLE_UNICODE_BIDI_OVERRIDE) {
    nsIFrame *directionalFrame = nsnull;

    if (NS_STYLE_DIRECTION_RTL == vis->mDirection) {
      directionalFrame = NS_NewDirectionalFrame(shell, styleContext, kRLO);
    }
    else if (NS_STYLE_DIRECTION_LTR == vis->mDirection) {
      directionalFrame = NS_NewDirectionalFrame(shell, styleContext, kLRO);
    }

    if (directionalFrame) {
      mLogicalFrames.AppendElement(directionalFrame);
    }
  }
  for (nsBlockFrame* block = aBlockFrame; block;
       block = static_cast<nsBlockFrame*>(block->GetNextContinuation())) {
    block->RemoveStateBits(NS_BLOCK_NEEDS_BIDI_RESOLUTION);
    InitLogicalArray(block->GetFirstChild(nsnull));
  }

  if (text->mUnicodeBidi == NS_STYLE_UNICODE_BIDI_OVERRIDE) {
    nsIFrame* directionalFrame = NS_NewDirectionalFrame(shell, styleContext, kPDF);
    if (directionalFrame) {
      mLogicalFrames.AppendElement(directionalFrame);
    }
  }

  CreateBlockBuffer();

  PRInt32 bufferLength = mBuffer.Length();

  if (bufferLength < 1) {
    mSuccess = NS_OK;
    return mSuccess;
  }
  PRInt32 runCount;
  PRUint8 embeddingLevel;

  nsBidiLevel paraLevel = embeddingLevel =
    (NS_STYLE_DIRECTION_RTL == vis->mDirection)
    ? NSBIDI_RTL : NSBIDI_LTR;

  mSuccess = mBidiEngine->SetPara(mBuffer.get(), bufferLength, paraLevel, nsnull);
  if (NS_FAILED(mSuccess) ) {
      return mSuccess;
  }

  PRBool isVisual;
  if (aIsVisualFormControl) {
    isVisual = PR_FALSE;
  } else {
    isVisual = presContext->IsVisualMode();
  }
  mSuccess = mBidiEngine->CountRuns(&runCount);
  if (NS_FAILED(mSuccess) ) {
    return mSuccess;
  }
  PRInt32     runLength      = 0;   
  PRInt32     lineOffset     = 0;   
  PRInt32     logicalLimit   = 0;   
  PRInt32     numRun         = -1;
  PRInt32     fragmentLength = 0;   
  PRInt32     frameIndex     = -1;  
  PRInt32     frameCount     = mLogicalFrames.Length();
  PRInt32     contentOffset  = 0;   
  PRUint8     charType;
  PRUint8     prevType       = eCharType_LeftToRight;
  PRBool      isTextFrame    = PR_FALSE;
  nsIFrame*   frame = nsnull;
  nsIContent* content = nsnull;
  PRInt32     contentTextLength;
  nsIAtom*    frameType = nsnull;

  nsPropertyTable *propTable = presContext->PropertyTable();

  nsBlockInFlowLineIterator lineIter(aBlockFrame, aBlockFrame->begin_lines(), PR_FALSE);
  if (lineIter.GetLine() == aBlockFrame->end_lines()) {
    
    lineIter.Next();
  }
  nsIFrame* prevFrame = nsnull;
  PRBool lineNeedsUpdate = PR_FALSE;
  
  for (; ;) {
    if (fragmentLength <= 0) {
      
      if (++frameIndex >= frameCount) {
        break;
      }
      frame = mLogicalFrames[frameIndex];
      frameType = frame->GetType();
      lineNeedsUpdate = PR_TRUE;
      if (nsGkAtoms::textFrame == frameType) {
        content = frame->GetContent();
        if (!content) {
          mSuccess = NS_OK;
          break;
        }
        contentTextLength = content->TextLength();
        if (contentTextLength == 0) {
          frame->AdjustOffsetsForBidi(0, 0);
          
          
          propTable->SetProperty(frame, nsGkAtoms::embeddingLevel,
                                 NS_INT32_TO_PTR(embeddingLevel),
                                 nsnull, nsnull);
          propTable->SetProperty(frame, nsGkAtoms::baseLevel,
                                 NS_INT32_TO_PTR(paraLevel), nsnull, nsnull);
          continue;
        }
        PRInt32 start, end;
        frame->GetOffsets(start, end);
        fragmentLength = end - start;
        contentOffset = start;
        isTextFrame = PR_TRUE;
      }
      else {
        



        isTextFrame = PR_FALSE;
        fragmentLength = 1;
      }
    } 

    if (runLength <= 0) {
      
      if (++numRun >= runCount) {
        break;
      }
      lineOffset = logicalLimit;
      if (NS_FAILED(mBidiEngine->GetLogicalRun(
              lineOffset, &logicalLimit, &embeddingLevel) ) ) {
        break;
      }
      runLength = logicalLimit - lineOffset;
      if (isVisual) {
        embeddingLevel = paraLevel;
      }
    } 

    if (nsGkAtoms::directionalFrame == frameType) {
      frame->Destroy();
      frame = nsnull;
      ++lineOffset;
    }
    else {
      propTable->SetProperty(frame, nsGkAtoms::embeddingLevel,
                             NS_INT32_TO_PTR(embeddingLevel), nsnull, nsnull);
      propTable->SetProperty(frame, nsGkAtoms::baseLevel,
                             NS_INT32_TO_PTR(paraLevel), nsnull, nsnull);
      if (isTextFrame) {
        PRInt32 typeLimit = NS_MIN(logicalLimit, lineOffset + fragmentLength);
        CalculateCharType(lineOffset, typeLimit, logicalLimit, runLength,
                           runCount, charType, prevType);
        
        propTable->SetProperty(frame, nsGkAtoms::charType,
                               NS_INT32_TO_PTR(charType), nsnull, nsnull);
        

        if ( (runLength > 0) && (runLength < fragmentLength) ) {
          



          if (lineNeedsUpdate) {
            AdvanceLineIteratorToFrame(frame, &lineIter, prevFrame);
            lineNeedsUpdate = PR_FALSE;
          }
          lineIter.GetLine()->MarkDirty();
          nsIFrame* nextBidi;
          EnsureBidiContinuation(frame, &nextBidi, frameIndex,
                                 contentOffset,
                                 contentOffset + runLength);
          if (NS_FAILED(mSuccess)) {
            break;
          }
          frame = nextBidi;
          contentOffset += runLength;
        } 
        else {
          if (contentOffset + fragmentLength == contentTextLength) {
            




            PRInt32 newIndex = 0;
            mContentToFrameIndex.Get(content, &newIndex);
            if (newIndex > frameIndex) {
              RemoveBidiContinuation(frame, frameIndex, newIndex, lineOffset);
              frameIndex = newIndex;
            }
          } else if (fragmentLength > 0 && runLength > fragmentLength) {
            





            PRInt32 newIndex = frameIndex;
            do {
            } while (mLogicalFrames[++newIndex]->GetType() == nsGkAtoms::directionalFrame);
            RemoveBidiContinuation(frame, frameIndex, newIndex, lineOffset);
          } else if (runLength == fragmentLength) {
            



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
      if (runLength <= 0) {
        
        nsIFrame* child = frame;
        nsIFrame* parent = frame->GetParent();
        
        while (parent &&
               IsBidiSplittable(parent) &&
               !child->GetNextSibling()) {
          child = parent;
          parent = child->GetParent();
        }
        if (parent && IsBidiSplittable(parent))
          SplitInlineAncestors(child);
      }
      else if (!frame->GetNextSibling()) {
        
        
        
        nsIFrame* parent = frame->GetParent();
        JoinInlineAncestors(parent);
      }
    }
  } 
  return mSuccess;
}


PRBool IsBidiLeaf(nsIFrame* aFrame) {
  nsIFrame* kid = aFrame->GetFirstChild(nsnull);
  return !kid
    || !aFrame->IsFrameOfType(nsIFrame::eBidiInlineContainer);
}

void
nsBidiPresUtils::InitLogicalArray(nsIFrame*       aCurrentFrame)
{
  if (!aCurrentFrame)
    return;

  nsIPresShell* shell = aCurrentFrame->PresContext()->PresShell();
  nsStyleContext* styleContext;

  for (nsIFrame* childFrame = aCurrentFrame; childFrame;
       childFrame = childFrame->GetNextSibling()) {

    
    
    
    nsIFrame* frame = childFrame;
    if (nsGkAtoms::placeholderFrame == childFrame->GetType()) {
      nsIFrame* realFrame =
        nsPlaceholderFrame::GetRealFrameForPlaceholder(childFrame);
      if (realFrame->IsFrameOfType(nsIFrame::eBidiInlineContainer)) {
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
          styleContext = frame->GetStyleContext();

          if (NS_STYLE_DIRECTION_RTL == vis->mDirection) {
            ch = kRLE;
          }
          else if (NS_STYLE_DIRECTION_LTR == vis->mDirection) {
            ch = kLRE;
          }
          break;
        case NS_STYLE_UNICODE_BIDI_OVERRIDE:
          styleContext = frame->GetStyleContext();

          if (NS_STYLE_DIRECTION_RTL == vis->mDirection) {
            ch = kRLO;
          }
          else if (NS_STYLE_DIRECTION_LTR == vis->mDirection) {
            ch = kLRO;
          }
          break;
      }

      
      
      if (ch != 0 && !frame->GetPrevContinuation()) {
        nsIFrame* dirFrame = NS_NewDirectionalFrame(shell, styleContext, ch);
        if (dirFrame) {
          mLogicalFrames.AppendElement(dirFrame);
        }
      }
    }

    if (IsBidiLeaf(frame)) {
      




      nsIContent* content = frame->GetContent();
      if (content) {
        mContentToFrameIndex.Put(content, mLogicalFrames.Length());
      }
      mLogicalFrames.AppendElement(frame);
    }
    else {
      nsIFrame* kid = frame->GetFirstChild(nsnull);
      InitLogicalArray(kid);
    }

    
    if (ch != 0 && !frame->GetNextContinuation()) {
      
      
      nsIFrame* dirFrame = NS_NewDirectionalFrame(shell, styleContext, kPDF);
      if (dirFrame) {
        mLogicalFrames.AppendElement(dirFrame);
      }
    }
  } 
}

void
nsBidiPresUtils::CreateBlockBuffer()
{
  mBuffer.SetLength(0);

  nsIFrame*                 frame;
  nsIContent*               prevContent = nsnull;
  PRUint32                  i;
  PRUint32                  count = mLogicalFrames.Length();

  for (i = 0; i < count; i++) {
    frame = mLogicalFrames[i];
    nsIAtom* frameType = frame->GetType();

    if (nsGkAtoms::textFrame == frameType) {
      nsIContent* content = frame->GetContent();
      if (!content) {
        mSuccess = NS_OK;
        break;
      }
      if (content == prevContent) {
        continue;
      }
      prevContent = content;
      content->AppendTextTo(mBuffer);
    }
    else if (nsGkAtoms::brFrame == frameType) { 
      
      mBuffer.Append(kLineSeparator);
    }
    else if (nsGkAtoms::directionalFrame == frameType) {
      nsDirectionalFrame* dirFrame = static_cast<nsDirectionalFrame*>(frame);
      mBuffer.Append(dirFrame->GetChar());
    }
    else { 
      
      
      mBuffer.Append(kObjectSubstitute);
    }
  }
  
  mBuffer.ReplaceChar("\t\r\n", kSpace);
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

  InitLogicalArrayFromLine(aFirstFrameOnLine, aNumFramesOnLine);

  PRBool isReordered;
  PRBool hasRTLFrames;
  Reorder(isReordered, hasRTLFrames);
  RepositionInlineFrames(aFirstFrameOnLine);
}

nsresult
nsBidiPresUtils::Reorder(PRBool& aReordered, PRBool& aHasRTLFrames)
{
  aReordered = PR_FALSE;
  aHasRTLFrames = PR_FALSE;
  PRInt32 count = mLogicalFrames.Length();

  if (mArraySize < count) {
    mArraySize = count << 1;
    if (mLevels) {
      delete[] mLevels;
      mLevels = nsnull;
    }
    if (mIndexMap) {
      delete[] mIndexMap;
      mIndexMap = nsnull;
    }
  }
  if (!mLevels) {
    mLevels = new PRUint8[mArraySize];
    if (!mLevels) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  memset(mLevels, 0, sizeof(PRUint8) * mArraySize);

  nsIFrame* frame;
  PRInt32   i;

  for (i = 0; i < count; i++) {
    frame = mLogicalFrames[i];
    mLevels[i] = GetFrameEmbeddingLevel(frame);
    if (mLevels[i] & 1) {
      aHasRTLFrames = PR_TRUE;
    }      
  }
  if (!mIndexMap) {
    mIndexMap = new PRInt32[mArraySize];
  }
  if (!mIndexMap) {
    mSuccess = NS_ERROR_OUT_OF_MEMORY;
  }
  else {
    memset(mIndexMap, 0, sizeof(PRUint32) * mArraySize);

    mSuccess = mBidiEngine->ReorderVisual(mLevels, count, mIndexMap);

    if (NS_SUCCEEDED(mSuccess) ) {
      mVisualFrames.Clear();

      for (i = 0; i < count; i++) {
        mVisualFrames.AppendElement(mLogicalFrames[mIndexMap[i]]);
        if (i != mIndexMap[i]) {
          aReordered = PR_TRUE;
        }
      }
    } 
  } 

  if (NS_FAILED(mSuccess) ) {
    aReordered = PR_FALSE;
  }
  return mSuccess;
}

nsBidiLevel
nsBidiPresUtils::GetFrameEmbeddingLevel(nsIFrame* aFrame)
{
  nsIFrame* firstLeaf = aFrame;
  while (!IsBidiLeaf(firstLeaf)) {
    firstLeaf = firstLeaf->GetFirstChild(nsnull);
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
                                   PRBool&                aIsRightMost ) const
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
    
    
    if (nsLayoutUtils::FrameIsInFirstPartOfIBSplit(aFrame)) {
      
      if (isLTR) {
        aIsRightMost = PR_FALSE;
      } else {
        aIsLeftMost = PR_FALSE;
      }
    } else {
      NS_ASSERTION(nsLayoutUtils::FrameIsInLastPartOfIBSplit(aFrame),
                   "How did that happen?");
      
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
                                 nsContinuationStates*  aContinuationStates) const
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
                                        nsContinuationStates*  aContinuationStates) const
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
nsBidiPresUtils::RepositionInlineFrames(nsIFrame* aFirstChild) const
{
  const nsStyleVisibility* vis = aFirstChild->GetStyleVisibility();
  PRBool isLTR = (NS_STYLE_DIRECTION_LTR == vis->mDirection);
  nscoord leftSpace = 0;

  
  
  
  nsMargin margin = aFirstChild->GetUsedMargin();
  if (!aFirstChild->GetPrevContinuation() &&
      !nsLayoutUtils::FrameIsInLastPartOfIBSplit(aFirstChild))
    leftSpace = isLTR ? margin.left : margin.right;

  nscoord left = aFirstChild->GetPosition().x - leftSpace;
  nsIFrame* frame;
  PRInt32 count = mVisualFrames.Length();
  PRInt32 index;
  nsContinuationStates continuationStates;

  continuationStates.Init();

  
  
  for (index = 0; index < count; index++) {
    InitContinuationStates(mVisualFrames[index], &continuationStates);
  }

  
  for (index = 0; index < count; index++) {
    frame = mVisualFrames[index];
    RepositionFrame(frame,
                    (mLevels[mIndexMap[index]] & 1),
                    left,
                    &continuationStates);
  } 
}

void 
nsBidiPresUtils::InitLogicalArrayFromLine(nsIFrame* aFirstFrameOnLine,
                                          PRInt32   aNumFramesOnLine) {
  mLogicalFrames.Clear();
  for (nsIFrame* frame = aFirstFrameOnLine;
       frame && aNumFramesOnLine--;
       frame = frame->GetNextSibling()) {
    mLogicalFrames.AppendElement(frame);
  }
}

PRBool
nsBidiPresUtils::CheckLineOrder(nsIFrame*  aFirstFrameOnLine,
                                PRInt32    aNumFramesOnLine,
                                nsIFrame** aFirstVisual,
                                nsIFrame** aLastVisual)
{
  InitLogicalArrayFromLine(aFirstFrameOnLine, aNumFramesOnLine);
  
  PRBool isReordered;
  PRBool hasRTLFrames;
  Reorder(isReordered, hasRTLFrames);
  PRInt32 count = mLogicalFrames.Length();
  
  if (aFirstVisual) {
    *aFirstVisual = mVisualFrames[0];
  }
  if (aLastVisual) {
    *aLastVisual = mVisualFrames[count-1];
  }
  
  
  return isReordered || hasRTLFrames;
}

nsIFrame*
nsBidiPresUtils::GetFrameToRightOf(const nsIFrame*  aFrame,
                                   nsIFrame*        aFirstFrameOnLine,
                                   PRInt32          aNumFramesOnLine)
{
  InitLogicalArrayFromLine(aFirstFrameOnLine, aNumFramesOnLine);
  
  PRBool isReordered;
  PRBool hasRTLFrames;
  Reorder(isReordered, hasRTLFrames);
  PRInt32 count = mVisualFrames.Length();

  if (aFrame == nsnull)
    return mVisualFrames[0];
  
  for (PRInt32 i = 0; i < count - 1; i++) {
    if (mVisualFrames[i] == aFrame) {
      return mVisualFrames[i+1];
    }
  }
  
  return nsnull;
}

nsIFrame*
nsBidiPresUtils::GetFrameToLeftOf(const nsIFrame*  aFrame,
                                  nsIFrame*        aFirstFrameOnLine,
                                  PRInt32          aNumFramesOnLine)
{
  InitLogicalArrayFromLine(aFirstFrameOnLine, aNumFramesOnLine);
  
  PRBool isReordered;
  PRBool hasRTLFrames;
  Reorder(isReordered, hasRTLFrames);
  PRInt32 count = mVisualFrames.Length();
  
  if (aFrame == nsnull)
    return mVisualFrames[count-1];
  
  for (PRInt32 i = 1; i < count; i++) {
    if (mVisualFrames[i] == aFrame) {
      return mVisualFrames[i-1];
    }
  }
  
  return nsnull;
}

inline void
nsBidiPresUtils::EnsureBidiContinuation(nsIFrame*       aFrame,
                                        nsIFrame**      aNewFrame,
                                        PRInt32&        aFrameIndex,
                                        PRInt32         aStart,
                                        PRInt32         aEnd)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  NS_PRECONDITION(aFrame, "aFrame is null");

  aFrame->AdjustOffsetsForBidi(aStart, aEnd);
  mSuccess = CreateBidiContinuation(aFrame, aNewFrame);
}

void
nsBidiPresUtils::RemoveBidiContinuation(nsIFrame*       aFrame,
                                        PRInt32         aFirstIndex,
                                        PRInt32         aLastIndex,
                                        PRInt32&        aOffset) const
{
  nsresult rv;
  nsBidiLevel embeddingLevel = (nsCharType)NS_PTR_TO_INT32(aFrame->GetProperty(nsGkAtoms::embeddingLevel, &rv));
  NS_ASSERTION(NS_SUCCEEDED(rv), "embeddingLevel attribute missing from aFrame");
  nsBidiLevel baseLevel = (nsCharType)NS_PTR_TO_INT32(aFrame->GetProperty(nsGkAtoms::baseLevel, &rv));
  NS_ASSERTION(NS_SUCCEEDED(rv), "baseLevel attribute missing from aFrame");
  nsCharType charType = (nsCharType)NS_PTR_TO_INT32(aFrame->GetProperty(nsGkAtoms::charType, &rv));
  NS_ASSERTION(NS_SUCCEEDED(rv), "charType attribute missing from aFrame");
  
  for (PRInt32 index = aFirstIndex + 1; index <= aLastIndex; index++) {
    nsIFrame* frame = mLogicalFrames[index];
    if (nsGkAtoms::directionalFrame == frame->GetType()) {
      frame->Destroy();
      ++aOffset;
    }
    else {
      
      
      frame->SetProperty(nsGkAtoms::embeddingLevel, NS_INT32_TO_PTR(embeddingLevel));
      frame->SetProperty(nsGkAtoms::baseLevel, NS_INT32_TO_PTR(baseLevel));
      frame->SetProperty(nsGkAtoms::charType, NS_INT32_TO_PTR(charType));
      frame->AddStateBits(NS_FRAME_IS_BIDI);
      while (frame) {
        nsIFrame* prev = frame->GetPrevContinuation();
        if (prev) {
          NS_ASSERTION (!frame->GetPrevInFlow() || frame->GetPrevInFlow() == prev, 
                        "prev-in-flow is not prev continuation!");
          frame->SetPrevInFlow(prev);

          NS_ASSERTION (!prev->GetNextInFlow() || prev->GetNextInFlow() == frame,
                        "next-in-flow is not next continuation!");
          prev->SetNextInFlow(frame);

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
                                            PRInt32&   aTextLength) const
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
nsBidiPresUtils::CalculateCharType(PRInt32& aOffset,
                                   PRInt32  aCharTypeLimit,
                                   PRInt32& aRunLimit,
                                   PRInt32& aRunLength,
                                   PRInt32& aRunCount,
                                   PRUint8& aCharType,
                                   PRUint8& aPrevCharType) const

{
  PRBool     strongTypeFound = PR_FALSE;
  PRInt32    offset;
  nsCharType charType;

  aCharType = eCharType_OtherNeutral;

  for (offset = aOffset; offset < aCharTypeLimit; offset++) {
    
    
    
    if (IS_HEBREW_CHAR(mBuffer[offset]) ) {
      charType = eCharType_RightToLeft;
    }
    else if (IS_ARABIC_ALPHABETIC(mBuffer[offset]) ) {
      charType = eCharType_RightToLeftArabic;
    }
    else {
      mBidiEngine->GetCharTypeAt(offset, &charType);
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

nsresult nsBidiPresUtils::GetBidiEngine(nsBidi** aBidiEngine)
{
  nsresult rv = NS_ERROR_FAILURE;
  if (mBidiEngine) {
    *aBidiEngine = mBidiEngine;
    rv = NS_OK;
  }
  return rv; 
}

nsresult nsBidiPresUtils::ProcessText(const PRUnichar*       aText,
                                      PRInt32                aLength,
                                      nsBidiDirection        aBaseDirection,
                                      nsPresContext*         aPresContext,
                                      BidiProcessor&         aprocessor,
                                      Mode                   aMode,
                                      nsBidiPositionResolve* aPosResolve,
                                      PRInt32                aPosResolveCount,
                                      nscoord*               aWidth)
{
  NS_ASSERTION((aPosResolve == nsnull) != (aPosResolveCount > 0), "Incorrect aPosResolve / aPosResolveCount arguments");

  PRInt32 runCount;

  mBuffer.Assign(aText, aLength);

  nsresult rv = mBidiEngine->SetPara(mBuffer.get(), aLength, aBaseDirection, nsnull);
  if (NS_FAILED(rv))
    return rv;

  rv = mBidiEngine->CountRuns(&runCount);
  if (NS_FAILED(rv))
    return rv;

  nscoord xOffset = 0;
  nscoord width, xEndRun;
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
    rv = mBidiEngine->GetVisualRun(i, &start, &length, &aBaseDirection);
    if (NS_FAILED(rv))
      return rv;

    rv = mBidiEngine->GetLogicalRun(start, &limit, &level);
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
      
      
      CalculateCharType(lineOffset, typeLimit, subRunLimit, subRunLength, subRunCount, charType, prevType);
      
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
  nsIRenderingContextBidiProcessor(nsIRenderingContext* aCtx,
                                   const nsPoint&       aPt)
                                   : mCtx(aCtx), mPt(aPt) { }

  ~nsIRenderingContextBidiProcessor()
  {
    mCtx->SetRightToLeftText(PR_FALSE);
  }

  virtual void SetText(const PRUnichar* aText,
                       PRInt32          aLength,
                       nsBidiDirection  aDirection)
  {
    mCtx->SetTextRunRTL(aDirection==NSBIDI_RTL);
    mText = aText;
    mLength = aLength;
  }

  virtual nscoord GetWidth()
  {
    nscoord width;
    mCtx->GetWidth(mText, mLength, width, nsnull);
    return width;
  }

  virtual void DrawText(nscoord aXOffset,
                        nscoord)
  {
    mCtx->DrawString(mText, mLength, mPt.x + aXOffset, mPt.y);
  }

private:
  nsIRenderingContext* mCtx;
  nsPoint mPt;
  const PRUnichar* mText;
  PRInt32 mLength;
  nsBidiDirection mDirection;
};

nsresult nsBidiPresUtils::ProcessTextForRenderingContext(const PRUnichar*       aText,
                                                         PRInt32                aLength,
                                                         nsBidiDirection        aBaseDirection,
                                                         nsPresContext*         aPresContext,
                                                         nsIRenderingContext&   aRenderingContext,
                                                         Mode                   aMode,
                                                         nscoord                aX,
                                                         nscoord                aY,
                                                         nsBidiPositionResolve* aPosResolve,
                                                         PRInt32                aPosResolveCount,
                                                         nscoord*               aWidth)
{
  nsIRenderingContextBidiProcessor processor(&aRenderingContext, nsPoint(aX, aY));

  return ProcessText(aText, aLength, aBaseDirection, aPresContext, processor,
                     aMode, aPosResolve, aPosResolveCount, aWidth);
}
#endif 
