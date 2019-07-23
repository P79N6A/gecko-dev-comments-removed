






































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

static NS_DEFINE_IID(kInlineFrameCID, NS_INLINE_FRAME_CID);

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
SplitInlineAncestors(nsPresContext* aPresContext,
                      nsIFrame*     aFrame)
{
  nsIPresShell *presShell = aPresContext->PresShell();
  nsIFrame* frame = aFrame;
  nsIFrame* parent = aFrame->GetParent();
  nsIFrame* newFrame = aFrame->GetNextSibling();
  nsIFrame* newParent;

  while (IsBidiSplittable(parent)) {
    nsIFrame* grandparent = parent->GetParent();
    NS_ASSERTION(grandparent, "Couldn't get parent's parent in nsBidiPresUtils::SplitInlineAncestors");
    
    nsresult rv = presShell->FrameConstructor()->
      CreateContinuingFrame(aPresContext, parent, grandparent, &newParent, PR_FALSE);
    if (NS_FAILED(rv)) {
      return rv;
    }
    
    
    frame->SetNextSibling(nsnull);
    rv = newParent->InsertFrames(nsGkAtoms::nextBidi, nsnull, newFrame);
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    rv = nsHTMLContainerFrame::ReparentFrameViewList(aPresContext, newFrame, parent, newParent);
    if (NS_FAILED(rv)) {
      return rv;
    }
    
    
    rv = grandparent->InsertFrames(nsGkAtoms::nextBidi, parent, newParent);
    if (NS_FAILED(rv)) {
      return rv;
    }
    
    frame = parent;
    newFrame = newParent;
    parent = grandparent;
  }
  
  return NS_OK;
}

static nsresult
CreateBidiContinuation(nsPresContext* aPresContext,
                       nsIFrame*       aFrame,
                       nsIFrame**      aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  NS_PRECONDITION(aFrame, "null ptr");

  *aNewFrame = nsnull;

  nsIPresShell *presShell = aPresContext->PresShell();
  NS_ASSERTION(presShell, "PresShell must be set on PresContext before calling nsBidiPresUtils::CreateBidiContinuation");

  nsIFrame* parent = aFrame->GetParent();
  NS_ASSERTION(parent, "Couldn't get frame parent in nsBidiPresUtils::CreateBidiContinuation");
  
  nsresult rv = presShell->FrameConstructor()->
    CreateContinuingFrame(aPresContext, aFrame, parent, aNewFrame, PR_FALSE);
  if (NS_FAILED(rv)) {
    return rv;
  }
  
  
  rv = parent->InsertFrames(nsGkAtoms::nextBidi, aFrame, *aNewFrame);
  if (NS_FAILED(rv)) {
    return rv;
  }
  
  
  rv = SplitInlineAncestors(aPresContext, aFrame);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}

static void
AdvanceLineIteratorToFrame(nsIFrame* aFrame,
                           nsIFrame* aBlockFrame,
                           nsBlockFrame::line_iterator& aLine,
                           nsIFrame*& aPrevFrame,
                           const nsBlockFrame::line_iterator& aEndLines)
{
  
  nsIFrame* child = aFrame;
  nsIFrame* parent = child->GetParent();
  while (parent && parent != aBlockFrame) {
    child = parent;
    parent = child->GetParent();
  }
  NS_ASSERTION (parent, "aFrame is not a descendent of aBlockFrame");
  while (aLine != aEndLines && !aLine->ContainsAfter(aPrevFrame, child, aLine, aEndLines)) {
    ++aLine;
    aPrevFrame = nsnull;
  }
  aPrevFrame = child;
  NS_ASSERTION (aLine != aEndLines, "frame not found on any line");
}
































nsresult
nsBidiPresUtils::Resolve(nsPresContext* aPresContext,
                         nsBlockFrame*   aBlockFrame,
                         nsIFrame*       aFirstChild,
                         PRBool          aIsVisualFormControl)
{
  mLogicalFrames.Clear();
  mContentToFrameIndex.Clear();
  
  nsIPresShell* shell = aPresContext->PresShell();
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
  mSuccess = InitLogicalArray(aPresContext, aFirstChild, nsnull, PR_TRUE);

  if (text->mUnicodeBidi == NS_STYLE_UNICODE_BIDI_OVERRIDE) {
    nsIFrame* directionalFrame = NS_NewDirectionalFrame(shell, styleContext, kPDF);
    if (directionalFrame) {
      mLogicalFrames.AppendElement(directionalFrame);
    }
  }
  if (NS_FAILED(mSuccess) ) {
    return mSuccess;
  }

  CreateBlockBuffer(aPresContext);

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
    isVisual = aPresContext->IsVisualMode();
  }
  mSuccess = mBidiEngine->CountRuns(&runCount);
  if (NS_FAILED(mSuccess) ) {
    return mSuccess;
  }
  PRInt32                  runLength      = 0;
  PRInt32                  fragmentLength = 0;
  PRInt32                  temp;
  PRInt32                  frameIndex     = -1;
  PRInt32                  frameCount     = mLogicalFrames.Count();
  PRInt32                  contentOffset  = 0;   
  PRInt32                  lineOffset     = 0;   
  PRInt32                  logicalLimit   = 0;
  PRInt32                  numRun         = -1;
  PRUint8                  charType;
  PRUint8                  prevType       = eCharType_LeftToRight;
  PRBool                   isTextFrame    = PR_FALSE;
  nsIFrame*                frame = nsnull;
  nsIFrame*                nextBidi;
  nsIContent*              content = nsnull;
  const nsTextFragment*    fragment;
  nsIAtom*                 frameType = nsnull;

  nsPropertyTable *propTable = aPresContext->PropertyTable();

  nsBlockFrame::line_iterator line = aBlockFrame->begin_lines();
  nsBlockFrame::line_iterator endLines = aBlockFrame->end_lines();
  nsIFrame* prevFrame = nsnull;
  PRBool lineNeedsUpdate = PR_FALSE;
  
  for (; ;) {
    if (fragmentLength <= 0) {
      if (++frameIndex >= frameCount) {
        break;
      }
      contentOffset = 0;
      
      frame = (nsIFrame*) (mLogicalFrames[frameIndex]);
      frameType = frame->GetType();
      lineNeedsUpdate = PR_TRUE;
      if (nsGkAtoms::textFrame == frameType) {
        content = frame->GetContent();
        if (!content) {
          mSuccess = NS_OK;
          break;
        }
        fragment = content->GetText();
        if (!fragment) {
          mSuccess = NS_ERROR_FAILURE;
          break;
        }
        fragmentLength = fragment->GetLength();
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
        PRInt32 typeLimit = PR_MIN(logicalLimit, lineOffset + fragmentLength);
        CalculateCharType(lineOffset, typeLimit, logicalLimit, runLength,
                           runCount, charType, prevType);
        
        propTable->SetProperty(frame, nsGkAtoms::charType,
                               NS_INT32_TO_PTR(charType), nsnull, nsnull);
        

        if ( (runLength > 0) && (runLength < fragmentLength) ) {
          if (!EnsureBidiContinuation(aPresContext, frame,
                                      &nextBidi, frameIndex) ) {
            break;
          }
          if (lineNeedsUpdate) {
            AdvanceLineIteratorToFrame(frame, aBlockFrame, line, prevFrame, endLines);
            lineNeedsUpdate = PR_FALSE;
          }
          line->MarkDirty();
          frame->AdjustOffsetsForBidi(contentOffset, contentOffset + runLength);
          frame = nextBidi;
          contentOffset += runLength;
        } 
        else {
          frame->AdjustOffsetsForBidi(contentOffset, contentOffset + fragmentLength);
          PRInt32 newIndex = 0;
          mContentToFrameIndex.Get(content, &newIndex);
          if (newIndex > frameIndex) {
            RemoveBidiContinuation(aPresContext, frame,
                                   frameIndex, newIndex, temp);
            if (lineNeedsUpdate) {
              AdvanceLineIteratorToFrame(frame, aBlockFrame, line, prevFrame, endLines);
              lineNeedsUpdate = PR_FALSE;
            }
            line->MarkDirty();
            runLength -= temp;
            fragmentLength -= temp;
            lineOffset += temp;
            frameIndex = newIndex;
          }
        }
      } 
      else {
        ++lineOffset;
      }
    } 
    temp = runLength;
    runLength -= fragmentLength;
    fragmentLength -= temp;

    
    if (frame && fragmentLength <= 0 && runLength <= 0) {
      
      nsIFrame* child = frame;
      nsIFrame* parent = frame->GetParent();
      while (parent &&
             IsBidiSplittable(parent) &&
             !child->GetNextSibling()) {
        child = parent;
        parent = child->GetParent();
      }
      if (parent && IsBidiSplittable(parent))
        SplitInlineAncestors(aPresContext, child);
    }
  } 
  return mSuccess;
}


PRBool IsBidiLeaf(nsIFrame* aFrame) {
  nsIFrame* kid = aFrame->GetFirstChild(nsnull);
  
  
  return !kid
    || !aFrame->IsFrameOfType(nsIFrame::eBidiInlineContainer)
    || aFrame->GetStyleDisplay()->IsBlockOutside();
}

nsresult
nsBidiPresUtils::InitLogicalArray(nsPresContext* aPresContext,
                                  nsIFrame*       aCurrentFrame,
                                  nsIFrame*       aNextInFlow,
                                  PRBool          aAddMarkers)
{
  nsIFrame*             frame;
  nsresult              res = NS_OK;

  nsIPresShell* shell = aPresContext->PresShell();
  nsStyleContext* styleContext;

  for (frame = aCurrentFrame;
       frame && frame != aNextInFlow;
       frame = frame->GetNextSibling()) {

    PRUnichar ch = 0;
    if (aAddMarkers &&
        frame->IsFrameOfType(nsIFrame::eBidiInlineContainer)) {
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
        mContentToFrameIndex.Put(content, mLogicalFrames.Count());
      }
      mLogicalFrames.AppendElement(frame);
    }
    else {
      nsIFrame* kid = frame->GetFirstChild(nsnull);
      res = InitLogicalArray(aPresContext, kid, aNextInFlow, aAddMarkers);
    }

    
    if (ch != 0 && !frame->GetNextContinuation()) {
      
      
      nsIFrame* dirFrame = NS_NewDirectionalFrame(shell, styleContext, kPDF);
      if (dirFrame) {
        mLogicalFrames.AppendElement(dirFrame);
      }
    }
  } 
  return res;       
}

void
nsBidiPresUtils::CreateBlockBuffer(nsPresContext* aPresContext)
{
  mBuffer.SetLength(0);

  nsIFrame*                 frame;
  nsIContent*               prevContent = nsnull;
  PRUint32                  i;
  PRUint32                  count = mLogicalFrames.Count();

  for (i = 0; i < count; i++) {
    frame = (nsIFrame*) (mLogicalFrames[i]);
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
nsBidiPresUtils::ReorderFrames(nsPresContext*       aPresContext,
                               nsIRenderingContext* aRendContext,
                               nsIFrame*            aFirstFrameOnLine,
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
  RepositionInlineFrames(aPresContext, aRendContext, aFirstFrameOnLine, isReordered);
}

nsresult
nsBidiPresUtils::Reorder(PRBool& aReordered, PRBool& aHasRTLFrames)
{
  aReordered = PR_FALSE;
  aHasRTLFrames = PR_FALSE;
  PRInt32 count = mLogicalFrames.Count();

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
    frame = (nsIFrame*) (mLogicalFrames[i]);
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

  nsIFrame* testFrame;
  aFrame->QueryInterface(kInlineFrameCID, (void**)&testFrame);

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
  nsMargin margin;
  aFrame->GetMargin(margin);
  if (isLeftMost)
    aLeft += margin.left;

  nscoord start = aLeft;

  if (!IsBidiLeaf(aFrame))
  {
    nscoord x = 0;
    nsMargin borderPadding;
    aFrame->GetBorderAndPadding(borderPadding);
    if (isLeftMost) {
      x += borderPadding.left;
    }

    
    
    
    nsVoidArray childList;
    nsIFrame *frame = aFrame->GetFirstChild(nsnull);
    if (frame && aIsOddLevel) {
      childList.AppendElement(nsnull);
      while (frame) {
        childList.AppendElement(frame);
        frame = frame->GetNextSibling();
      }
      frame = (nsIFrame*)childList[childList.Count() - 1];
    }

    
    PRInt32 index = 0;
    while (frame) {
      RepositionFrame(frame,
                      aIsOddLevel,
                      x,
                      aContinuationStates);
      index++;
      frame = aIsOddLevel ?
                (nsIFrame*)childList[childList.Count() - index - 1] :
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
nsBidiPresUtils::RepositionInlineFrames(nsPresContext*       aPresContext,
                                        nsIRenderingContext* aRendContext,
                                        nsIFrame*            aFirstChild,
                                        PRBool               aReordered) const
{
  nsMargin margin;
  const nsStyleVisibility* vis = aFirstChild->GetStyleVisibility();
  PRBool isLTR = (NS_STYLE_DIRECTION_LTR == vis->mDirection);
  nscoord leftSpace = 0;

  aFirstChild->GetMargin(margin);
  if (!aFirstChild->GetPrevContinuation())
    leftSpace = isLTR ? margin.left : margin.right;

  nscoord left = aFirstChild->GetPosition().x - leftSpace;
  nsIFrame* frame;
  PRInt32 count = mVisualFrames.Count();
  PRInt32 index;
  nsContinuationStates continuationStates;

  continuationStates.Init();

  
  
  for (index = 0; index < count; index++) {
    InitContinuationStates((nsIFrame*)mVisualFrames[index],
                           &continuationStates);
  }

  
  for (index = 0; index < count; index++) {
    frame = (nsIFrame*) (mVisualFrames[index]);
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
  PRInt32 count = mLogicalFrames.Count();
  
  if (aFirstVisual) {
    *aFirstVisual = (nsIFrame*)mVisualFrames[0];
  }
  if (aLastVisual) {
    *aLastVisual = (nsIFrame*)mVisualFrames[count-1];
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
  PRInt32 count = mVisualFrames.Count();

  if (aFrame == nsnull)
    return (nsIFrame*)mVisualFrames[0];
  
  for (PRInt32 i = 0; i < count - 1; i++) {
    if ((nsIFrame*)mVisualFrames[i] == aFrame) {
      return (nsIFrame*)mVisualFrames[i+1];
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
  PRInt32 count = mVisualFrames.Count();
  
  if (aFrame == nsnull)
    return (nsIFrame*)mVisualFrames[count-1];
  
  for (PRInt32 i = 1; i < count; i++) {
    if ((nsIFrame*)mVisualFrames[i] == aFrame) {
      return (nsIFrame*)mVisualFrames[i-1];
    }
  }
  
  return nsnull;
}

PRBool
nsBidiPresUtils::EnsureBidiContinuation(nsPresContext* aPresContext,
                                        nsIFrame*       aFrame,
                                        nsIFrame**      aNewFrame,
                                        PRInt32&        aFrameIndex)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  NS_PRECONDITION(aFrame, "aFrame is null");

  *aNewFrame = nsnull;
  nsBidiLevel embeddingLevel = NS_GET_EMBEDDING_LEVEL(aFrame);
  nsBidiLevel baseLevel = NS_GET_BASE_LEVEL(aFrame);
  nsCharType charType = (nsCharType)NS_PTR_TO_INT32(aFrame->GetProperty(nsGkAtoms::charType));
  
  
  while (aFrameIndex + 1 < mLogicalFrames.Count()) {
    nsIFrame* frame = (nsIFrame*)mLogicalFrames[aFrameIndex + 1];
    if (frame->GetPrevInFlow() != aFrame) {
      
      if (frame->GetPrevContinuation() == aFrame) {
        *aNewFrame = frame;
        aFrameIndex++;
      }
      break;
    }
    frame->SetProperty(nsGkAtoms::embeddingLevel, NS_INT32_TO_PTR(embeddingLevel));
    frame->SetProperty(nsGkAtoms::baseLevel, NS_INT32_TO_PTR(baseLevel));
    frame->SetProperty(nsGkAtoms::charType, NS_INT32_TO_PTR(charType));
    frame->AddStateBits(NS_FRAME_IS_BIDI);
    aFrameIndex++;
    aFrame = frame;
  }
  
  if (!*aNewFrame) {
    mSuccess = CreateBidiContinuation(aPresContext, aFrame, aNewFrame);
    if (NS_FAILED(mSuccess) ) {
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}

void
nsBidiPresUtils::RemoveBidiContinuation(nsPresContext* aPresContext,
                                        nsIFrame*       aFrame,
                                        PRInt32         aFirstIndex,
                                        PRInt32         aLastIndex,
                                        PRInt32&        aOffset) const
{
  aOffset = 0;

  nsresult rv;
  nsBidiLevel embeddingLevel = (nsCharType)NS_PTR_TO_INT32(aFrame->GetProperty(nsGkAtoms::embeddingLevel, &rv));
  NS_ASSERTION(NS_SUCCEEDED(rv), "embeddingLevel attribute missing from aFrame");
  nsBidiLevel baseLevel = (nsCharType)NS_PTR_TO_INT32(aFrame->GetProperty(nsGkAtoms::baseLevel, &rv));
  NS_ASSERTION(NS_SUCCEEDED(rv), "baseLevel attribute missing from aFrame");
  nsCharType charType = (nsCharType)NS_PTR_TO_INT32(aFrame->GetProperty(nsGkAtoms::charType, &rv));
  NS_ASSERTION(NS_SUCCEEDED(rv), "charType attribute missing from aFrame");
  
  for (PRInt32 index = aFirstIndex + 1; index <= aLastIndex; index++) {
    nsIFrame* frame = (nsIFrame*) mLogicalFrames[index];
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
                                   PRBool           aIsOddLevel,
                                   PRBool           aIsBidiSystem,
                                   PRBool           aIsNewTextRunSystem)
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

    case IBMBIDI_NUMERAL_NOMINAL:
    default:
      break;
  }

  PRBool doReverse = PR_FALSE;
  PRBool doShape = PR_FALSE;

  if (!aIsNewTextRunSystem) {
    if (aIsBidiSystem) {
      if ( (CHARTYPE_IS_RTL(aCharType)) ^ (aIsOddLevel) )
        doReverse = PR_TRUE;
    }
    else {
      if (aIsOddLevel)
        doReverse = PR_TRUE;
      if (eCharType_RightToLeftArabic == aCharType) 
        doShape = PR_TRUE;
    }
  }

  if (doReverse || doShape) {
    PRInt32    newLen;

    if (mBuffer.Length() < aTextLength) {
      if (!EnsureStringLength(mBuffer, aTextLength))
        return NS_ERROR_OUT_OF_MEMORY;
    }
    PRUnichar* buffer = mBuffer.BeginWriting();

    if (doReverse) {
      rv = mBidiEngine->WriteReverse(aText, aTextLength, buffer,
                                     NSBIDI_DO_MIRRORING, &newLen);
      if (NS_SUCCEEDED(rv) ) {
        aTextLength = newLen;
        memcpy(aText, buffer, aTextLength * sizeof(PRUnichar) );
      }
    }
    if (doShape) {
      rv = ArabicShaping(aText, aTextLength, buffer, (PRUint32 *)&newLen,
                         PR_FALSE, PR_FALSE);
      if (NS_SUCCEEDED(rv) ) {
        aTextLength = newLen;
        memcpy(aText, buffer, aTextLength * sizeof(PRUnichar) );
      }
    }
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
                                      nsIRenderingContext&   aRenderingContext,
                                      Mode                   aMode,
                                      nscoord                aX,
                                      nscoord                aY,
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

  nscoord width, xEndRun, xStartText = aX;
  PRBool isRTL = PR_FALSE;
  nscoord totalWidth = 0;
  PRInt32 i, start, limit, length;
  PRUint32 visualStart = 0;
  PRUint8 charType;
  PRUint8 prevType = eCharType_LeftToRight;
  nsBidiLevel level;

  PRUint32 hints = 0;
  aRenderingContext.GetHints(hints);
  PRBool isBidiSystem = (hints & NS_RENDERING_HINT_BIDI_REORDERING);
      
  for(int nPosResolve=0; nPosResolve < aPosResolveCount; ++nPosResolve)
  {
    aPosResolve[nPosResolve].visualIndex = kNotFound;
    aPosResolve[nPosResolve].visualLeftTwips = kNotFound;
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
    PRInt32 typeLimit = PR_MIN(limit, aLength);
    PRInt32 subRunCount = 1;
    PRInt32 subRunLimit = typeLimit;

    










    aRenderingContext.SetTextRunRTL(level & 1);

    if (level & 1) {
      aRenderingContext.GetWidth(aText + start, subRunLength, width, nsnull);
      aX += width;
      xEndRun = aX;
    }

    while (subRunCount > 0) {
      
      
      CalculateCharType(lineOffset, typeLimit, subRunLimit, subRunLength, subRunCount, charType, prevType);

      if (eCharType_RightToLeftArabic == charType) {
        isBidiSystem = (hints & NS_RENDERING_HINT_ARABIC_SHAPING);
      }
      if (isBidiSystem && (CHARTYPE_IS_RTL(charType) ^ isRTL) ) {
        
        isRTL = !isRTL;
        aRenderingContext.SetRightToLeftText(isRTL);
      }
      
      nsAutoString runVisualText;
      runVisualText.Assign(aText + start, subRunLength);
      if (runVisualText.Length() < subRunLength)
        return NS_ERROR_OUT_OF_MEMORY;
      FormatUnicodeText(aPresContext, runVisualText.BeginWriting(), subRunLength,
                        (nsCharType)charType, level & 1,
                        isBidiSystem, (hints & NS_RENDERING_HINT_NEW_TEXT_RUNS) != 0);

      aRenderingContext.GetWidth(runVisualText.get(), subRunLength, width, nsnull);
      totalWidth += width;
      if (level & 1) {
        aX -= width;
      }
      if (aMode == MODE_DRAW) {
        aRenderingContext.DrawString(runVisualText.get(), subRunLength, aX, aY);
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
            posResolve->visualLeftTwips = aX - xStartText;
          }
          








          else {
            nscoord subWidth;
            
            const PRUnichar* visualLeftPart;
            if (level & 1) {
              
              posResolve->visualIndex = visualStart + (subRunLength - (posResolve->logicalIndex + 1 - start));
              
              visualLeftPart = aText + posResolve->logicalIndex + 1;
            }
            else {
              posResolve->visualIndex = visualStart + (posResolve->logicalIndex - start);
              
              visualLeftPart = aText + start;
            }
            
            PRInt32 visualLeftLength = posResolve->visualIndex - visualStart;
            aRenderingContext.GetWidth(visualLeftPart,
                                       visualLeftLength,
                                       subWidth, nsnull);
            posResolve->visualLeftTwips = aX + subWidth - xStartText;
          }
        }
      }

      if (!(level & 1)) {
        aX += width;
      }

      --subRunCount;
      start = lineOffset;
      subRunLimit = typeLimit;
      subRunLength = typeLimit - lineOffset;
    } 
    if (level & 1) {
      aX = xEndRun;
    }
    
    visualStart += length;
  } 

  
  if (isRTL) {
    aRenderingContext.SetRightToLeftText(PR_FALSE);
  }
  if (aWidth) {
    *aWidth = totalWidth;
  }
  return NS_OK;
}
  
nsresult
nsBidiPresUtils::ReorderUnicodeText(PRUnichar*       aText,
                                    PRInt32&         aTextLength,
                                    nsCharType       aCharType,
                                    PRBool           aIsOddLevel,
                                    PRBool           aIsBidiSystem,
                                    PRBool           aIsNewTextRunSystem)
{
  NS_ASSERTION(aIsOddLevel == 0 || aIsOddLevel == 1, "aIsOddLevel should be 0 or 1");
  nsresult rv = NS_OK;
  PRBool doReverse = PR_FALSE;

  if (!aIsNewTextRunSystem) {
    if (aIsBidiSystem) {
      if ( (CHARTYPE_IS_RTL(aCharType)) ^ (aIsOddLevel) )
        doReverse = PR_TRUE;
    }
    else {
      if (aIsOddLevel)
        doReverse = PR_TRUE;
    }
  }

  if (doReverse) {
    PRInt32    newLen;

    if (mBuffer.Length() < aTextLength) {
      if (!EnsureStringLength(mBuffer, aTextLength))
        return NS_ERROR_OUT_OF_MEMORY;
    }
    PRUnichar* buffer = mBuffer.BeginWriting();

    if (doReverse) {
      rv = mBidiEngine->WriteReverse(aText, aTextLength, buffer,
                                     NSBIDI_DO_MIRRORING, &newLen);
      if (NS_SUCCEEDED(rv) ) {
        aTextLength = newLen;
        memcpy(aText, buffer, aTextLength * sizeof(PRUnichar) );
      }
    }
  }
  return rv;
}

#endif 
