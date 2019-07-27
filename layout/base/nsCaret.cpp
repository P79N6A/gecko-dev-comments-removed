







#include "nsCaret.h"

#include <algorithm>

#include "nsCOMPtr.h"
#include "nsITimer.h"
#include "nsFrameSelection.h"
#include "nsIFrame.h"
#include "nsIScrollableFrame.h"
#include "nsIDOMNode.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsIContent.h"
#include "nsIPresShell.h"
#include "nsRenderingContext.h"
#include "nsPresContext.h"
#include "nsBlockFrame.h"
#include "nsISelectionController.h"
#include "nsTextFrame.h"
#include "nsXULPopupManager.h"
#include "nsMenuPopupFrame.h"
#include "nsTextFragment.h"
#include "mozilla/Preferences.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/dom/Selection.h"
#include "nsIBidiKeyboard.h"
#include "nsContentUtils.h"

using namespace mozilla;
using namespace mozilla::dom;




static const int32_t kMinBidiIndicatorPixels = 2;







static nsIFrame*
CheckForTrailingTextFrameRecursive(nsIFrame* aFrame, nsIFrame* aStopAtFrame)
{
  if (aFrame == aStopAtFrame ||
      ((aFrame->GetType() == nsGkAtoms::textFrame &&
       (static_cast<nsTextFrame*>(aFrame))->IsAtEndOfLine())))
    return aFrame;
  if (!aFrame->IsFrameOfType(nsIFrame::eLineParticipant))
    return nullptr;

  for (nsIFrame* f = aFrame->GetFirstPrincipalChild(); f; f = f->GetNextSibling())
  {
    nsIFrame* r = CheckForTrailingTextFrameRecursive(f, aStopAtFrame);
    if (r)
      return r;
  }
  return nullptr;
}

static nsLineBox*
FindContainingLine(nsIFrame* aFrame)
{
  while (aFrame && aFrame->IsFrameOfType(nsIFrame::eLineParticipant))
  {
    nsIFrame* parent = aFrame->GetParent();
    nsBlockFrame* blockParent = nsLayoutUtils::GetAsBlock(parent);
    if (blockParent)
    {
      bool isValid;
      nsBlockInFlowLineIterator iter(blockParent, aFrame, &isValid);
      return isValid ? iter.GetLine().get() : nullptr;
    }
    aFrame = parent;
  }
  return nullptr;
}

static void
AdjustCaretFrameForLineEnd(nsIFrame** aFrame, int32_t* aOffset)
{
  nsLineBox* line = FindContainingLine(*aFrame);
  if (!line)
    return;
  int32_t count = line->GetChildCount();
  for (nsIFrame* f = line->mFirstChild; count > 0; --count, f = f->GetNextSibling())
  {
    nsIFrame* r = CheckForTrailingTextFrameRecursive(f, *aFrame);
    if (r == *aFrame)
      return;
    if (r)
    {
      *aFrame = r;
      NS_ASSERTION(r->GetType() == nsGkAtoms::textFrame, "Expected text frame");
      *aOffset = (static_cast<nsTextFrame*>(r))->GetContentEnd();
      return;
    }
  }
}

static bool
IsBidiUI()
{
  return Preferences::GetBool("bidi.browser.ui");
}



nsCaret::nsCaret()
: mOverrideOffset(0)
, mIsBlinkOn(false)
, mVisible(false)
, mReadOnly(false)
, mShowDuringSelection(false)
, mIgnoreUserModify(true)
{
}


nsCaret::~nsCaret()
{
  StopBlinking();
}


nsresult nsCaret::Init(nsIPresShell *inPresShell)
{
  NS_ENSURE_ARG(inPresShell);

  mPresShell = do_GetWeakReference(inPresShell);    
  NS_ASSERTION(mPresShell, "Hey, pres shell should support weak refs");

  mShowDuringSelection =
    LookAndFeel::GetInt(LookAndFeel::eIntID_ShowCaretDuringSelection,
                        mShowDuringSelection ? 1 : 0) != 0;

  
  

  nsCOMPtr<nsISelectionController> selCon = do_QueryReferent(mPresShell);
  if (!selCon)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsISelection> domSelection;
  nsresult rv = selCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                                     getter_AddRefs(domSelection));
  if (NS_FAILED(rv))
    return rv;
  if (!domSelection)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsISelectionPrivate> privateSelection = do_QueryInterface(domSelection);
  if (privateSelection)
    privateSelection->AddSelectionListener(this);
  mDomSelectionWeak = do_GetWeakReference(domSelection);
  
  return NS_OK;
}

static bool
DrawCJKCaret(nsIFrame* aFrame, int32_t aOffset)
{
  nsIContent* content = aFrame->GetContent();
  const nsTextFragment* frag = content->GetText();
  if (!frag)
    return false;
  if (aOffset < 0 || uint32_t(aOffset) >= frag->GetLength())
    return false;
  char16_t ch = frag->CharAt(aOffset);
  return 0x2e80 <= ch && ch <= 0xd7ff;
}

nsCaret::Metrics
nsCaret::ComputeMetrics(nsIFrame* aFrame, int32_t aOffset, nscoord aCaretHeight)
{
  
  nscoord caretWidth =
    (aCaretHeight * LookAndFeel::GetFloat(LookAndFeel::eFloatID_CaretAspectRatio, 0.0f)) +
    nsPresContext::CSSPixelsToAppUnits(
        LookAndFeel::GetInt(LookAndFeel::eIntID_CaretWidth, 1));

  if (DrawCJKCaret(aFrame, aOffset)) {
    caretWidth += nsPresContext::CSSPixelsToAppUnits(1);
  }
  nscoord bidiIndicatorSize = nsPresContext::CSSPixelsToAppUnits(kMinBidiIndicatorPixels);
  bidiIndicatorSize = std::max(caretWidth, bidiIndicatorSize);

  
  
  int32_t tpp = aFrame->PresContext()->AppUnitsPerDevPixel();
  Metrics result;
  result.mCaretWidth = NS_ROUND_BORDER_TO_PIXELS(caretWidth, tpp);
  result.mBidiIndicatorSize = NS_ROUND_BORDER_TO_PIXELS(bidiIndicatorSize, tpp);
  return result;
}


void nsCaret::Terminate()
{
  
  
  
  StopBlinking();
  mBlinkTimer = nullptr;

  
  nsCOMPtr<nsISelection> domSelection = do_QueryReferent(mDomSelectionWeak);
  nsCOMPtr<nsISelectionPrivate> privateSelection(do_QueryInterface(domSelection));
  if (privateSelection)
    privateSelection->RemoveSelectionListener(this);
  mDomSelectionWeak = nullptr;
  mPresShell = nullptr;

  mOverrideContent = nullptr;
}


NS_IMPL_ISUPPORTS(nsCaret, nsISelectionListener)


nsISelection* nsCaret::GetSelection()
{
  nsCOMPtr<nsISelection> sel(do_QueryReferent(mDomSelectionWeak));
  return sel;
}


void nsCaret::SetSelection(nsISelection *aDOMSel)
{
  MOZ_ASSERT(aDOMSel);
  mDomSelectionWeak = do_GetWeakReference(aDOMSel);   
  ResetBlinking();
  SchedulePaint();
}



void nsCaret::SetVisible(bool inMakeVisible)
{
  mVisible = inMakeVisible;
  mIgnoreUserModify = mVisible;
  ResetBlinking();
  SchedulePaint();
}



bool nsCaret::IsVisible()
{
  if (!mVisible) {
    return false;
  }

  if (!mShowDuringSelection) {
    Selection* selection = GetSelectionInternal();
    if (!selection) {
      return false;
    }
    bool isCollapsed;
    if (NS_FAILED(selection->GetIsCollapsed(&isCollapsed)) || !isCollapsed) {
      return false;
    }
  }

  if (IsMenuPopupHidingCaret()) {
    return false;
  }

  return true;
}



void nsCaret::SetCaretReadOnly(bool inMakeReadonly)
{
  mReadOnly = inMakeReadonly;
  ResetBlinking();
  SchedulePaint();
}

 nsresult
nsCaret::GetGeometryForFrame(nsIFrame* aFrame,
                             int32_t   aFrameOffset,
                             nsRect*   aRect,
                             nscoord*  aBidiIndicatorSize)
{
  nsPoint framePos(0, 0);
  nsresult rv = aFrame->GetPointFromOffset(aFrameOffset, &framePos);
  if (NS_FAILED(rv))
    return rv;

  nsIFrame* frame = aFrame->GetContentInsertionFrame();
  if (!frame) {
    frame = aFrame;
  }
  NS_ASSERTION(!(frame->GetStateBits() & NS_FRAME_IN_REFLOW),
               "We should not be in the middle of reflow");
  nscoord baseline = frame->GetCaretBaseline();
  nscoord ascent = 0, descent = 0;
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(aFrame, getter_AddRefs(fm),
    nsLayoutUtils::FontSizeInflationFor(aFrame));
  NS_ASSERTION(fm, "We should be able to get the font metrics");
  if (fm) {
    ascent = fm->MaxAscent();
    descent = fm->MaxDescent();
  }
  nscoord height = ascent + descent;
  framePos.y = baseline - ascent;
  Metrics caretMetrics = ComputeMetrics(aFrame, aFrameOffset, height);
  *aRect = nsRect(framePos, nsSize(caretMetrics.mCaretWidth, height));

  
  
  nsIFrame *scrollFrame =
    nsLayoutUtils::GetClosestFrameOfType(aFrame, nsGkAtoms::scrollFrame);
  if (scrollFrame) {
    
    nsIScrollableFrame *sf = do_QueryFrame(scrollFrame);
    nsIFrame *scrolled = sf->GetScrolledFrame();
    nsRect caretInScroll = *aRect + aFrame->GetOffsetTo(scrolled);

    
    
    nscoord overflow = caretInScroll.XMost() -
      scrolled->GetVisualOverflowRectRelativeToSelf().width;
    if (overflow > 0)
      aRect->x -= overflow;
  }

  if (aBidiIndicatorSize)
    *aBidiIndicatorSize = caretMetrics.mBidiIndicatorSize;

  return NS_OK;
}

static nsIFrame*
GetFrameAndOffset(Selection* aSelection,
                  nsINode* aOverrideNode, int32_t aOverrideOffset,
                  int32_t* aFrameOffset)
{
  nsINode* focusNode;
  int32_t focusOffset;

  if (aOverrideNode) {
    focusNode = aOverrideNode;
    focusOffset = aOverrideOffset;
  } else {
    focusNode = aSelection->GetFocusNode();
    aSelection->GetFocusOffset(&focusOffset);
  }

  if (!focusNode || !focusNode->IsContent()) {
    return nullptr;
  }

  nsIContent* contentNode = focusNode->AsContent();
  nsFrameSelection* frameSelection = aSelection->GetFrameSelection();
  uint8_t bidiLevel = frameSelection->GetCaretBidiLevel();
  nsIFrame* frame;
  nsresult rv = nsCaret::GetCaretFrameForNodeOffset(
      frameSelection, contentNode, focusOffset,
      frameSelection->GetHint(), bidiLevel, &frame, aFrameOffset);
  if (NS_FAILED(rv) || !frame) {
    return nullptr;
  }

  return frame;
}

 nsIFrame*
nsCaret::GetGeometry(nsISelection* aSelection, nsRect* aRect)
{
  int32_t frameOffset;
  nsIFrame* frame = GetFrameAndOffset(
      static_cast<Selection*>(aSelection), nullptr, 0, &frameOffset);
  if (frame) {
    GetGeometryForFrame(frame, frameOffset, aRect, nullptr);
  }
  return frame;
}

Selection*
nsCaret::GetSelectionInternal()
{
  return static_cast<Selection*>(GetSelection());
}

void nsCaret::SchedulePaint()
{
  nsINode* focusNode = mOverrideContent ?
    mOverrideContent.get() : GetSelectionInternal()->GetFocusNode();
  if (!focusNode || !focusNode->IsContent()) {
    return;
  }
  nsIFrame* f = focusNode->AsContent()->GetPrimaryFrame();
  if (!f) {
    return;
  }
  
  
  f->SchedulePaint();
}

void nsCaret::SetVisibilityDuringSelection(bool aVisibility) 
{
  mShowDuringSelection = aVisibility;
  SchedulePaint();
}

void
nsCaret::SetCaretPosition(nsIDOMNode* aNode, int32_t aOffset)
{
  mOverrideContent = do_QueryInterface(aNode);
  mOverrideOffset = aOffset;

  ResetBlinking();
  SchedulePaint();
}

void
nsCaret::CheckSelectionLanguageChange()
{
  
  bool isKeyboardRTL = false;
  nsIBidiKeyboard* bidiKeyboard = nsContentUtils::GetBidiKeyboard();
  
  
  
  if (bidiKeyboard && NS_SUCCEEDED(bidiKeyboard->IsLangRTL(&isKeyboardRTL)) &&
      IsBidiUI()) {
    
    
    
    Selection* selection = GetSelectionInternal();
    if (selection) {
      selection->SelectionLanguageChange(isKeyboardRTL);
    }
  }
}

nsIFrame*
nsCaret::GetPaintGeometry(nsRect* aRect)
{
  
  if (!IsVisible() || !mIsBlinkOn) {
    return nullptr;
  }

  
  
  CheckSelectionLanguageChange();

  int32_t frameOffset;
  nsIFrame *frame = GetFrameAndOffset(GetSelectionInternal(),
      mOverrideContent, mOverrideOffset, &frameOffset);
  if (!frame) {
    return nullptr;
  }

  
  const nsStyleUserInterface* userinterface = frame->StyleUserInterface();
  if ((!mIgnoreUserModify &&
       userinterface->mUserModify == NS_STYLE_USER_MODIFY_READ_ONLY) ||
      userinterface->mUserInput == NS_STYLE_USER_INPUT_NONE ||
      userinterface->mUserInput == NS_STYLE_USER_INPUT_DISABLED) {
    return nullptr;
  }

  
  int32_t startOffset, endOffset;
  if (frame->GetType() == nsGkAtoms::textFrame &&
      (NS_FAILED(frame->GetOffsets(startOffset, endOffset)) ||
      startOffset > frameOffset ||
      endOffset < frameOffset)) {
    return nullptr;
  }

  nsRect caretRect;
  nsRect hookRect;
  ComputeCaretRects(frame, frameOffset, &caretRect, &hookRect);

  aRect->UnionRect(caretRect, hookRect);
  return frame;
}

void nsCaret::PaintCaret(nsDisplayListBuilder *aBuilder,
                         nsRenderingContext *aCtx,
                         nsIFrame* aForFrame,
                         const nsPoint &aOffset)
{
  int32_t contentOffset;
  nsIFrame* frame = GetFrameAndOffset(GetSelectionInternal(),
    mOverrideContent, mOverrideOffset, &contentOffset);
  if (!frame) {
    return;
  }
  NS_ASSERTION(frame == aForFrame, "We're referring different frame");

  nscolor foregroundColor = aForFrame->GetCaretColorAt(contentOffset);
  aCtx->SetColor(foregroundColor);

  nsRect caretRect;
  nsRect hookRect;
  ComputeCaretRects(frame, contentOffset, &caretRect, &hookRect);
  aCtx->FillRect(caretRect + aOffset);
  if (!hookRect.IsEmpty()) {
    aCtx->FillRect(hookRect + aOffset);
  }
}



NS_IMETHODIMP
nsCaret::NotifySelectionChanged(nsIDOMDocument *, nsISelection *aDomSel,
                                int16_t aReason)
{
  if (aReason & nsISelectionListener::MOUSEUP_REASON)
    return NS_OK;

  nsCOMPtr<nsISelection> domSel(do_QueryReferent(mDomSelectionWeak));

  
  
  
  
  
  
  

  if (domSel != aDomSel)
    return NS_OK;

  ResetBlinking();
  SchedulePaint();

  return NS_OK;
}



void nsCaret::ResetBlinking()
{
  mIsBlinkOn = true;

  if (mReadOnly) {
    StopBlinking();
    return;
  }

  if (mBlinkTimer) {
    mBlinkTimer->Cancel();
  } else {
    nsresult  err;
    mBlinkTimer = do_CreateInstance("@mozilla.org/timer;1", &err);
    if (NS_FAILED(err))
      return;
  }

  uint32_t blinkRate = static_cast<uint32_t>(
    LookAndFeel::GetInt(LookAndFeel::eIntID_CaretBlinkTime, 500));
  if (blinkRate > 0) {
    mBlinkTimer->InitWithFuncCallback(CaretBlinkCallback, this, blinkRate,
                                      nsITimer::TYPE_REPEATING_SLACK);
  }
}


void nsCaret::StopBlinking()
{
  if (mBlinkTimer)
  {
    mBlinkTimer->Cancel();
  }
}

nsresult 
nsCaret::GetCaretFrameForNodeOffset(nsFrameSelection*    aFrameSelection,
                                    nsIContent*          aContentNode,
                                    int32_t              aOffset,
                                    CaretAssociationHint aFrameHint,
                                    uint8_t              aBidiLevel,
                                    nsIFrame**           aReturnFrame,
                                    int32_t*             aReturnOffset)
{
  if (!aFrameSelection)
    return NS_ERROR_FAILURE;
  nsIPresShell* presShell = aFrameSelection->GetShell();
  if (!presShell)
    return NS_ERROR_FAILURE;

  if (!aContentNode || !aContentNode->IsInComposedDoc() ||
      presShell->GetDocument() != aContentNode->GetComposedDoc())
    return NS_ERROR_FAILURE;

  nsIFrame* theFrame = nullptr;
  int32_t   theFrameOffset = 0;

  theFrame = aFrameSelection->GetFrameForNodeOffset(
      aContentNode, aOffset, aFrameHint, &theFrameOffset);
  if (!theFrame)
    return NS_ERROR_FAILURE;

  
  
  
  
  AdjustCaretFrameForLineEnd(&theFrame, &theFrameOffset);
  
  
  
  
  
  
  
  
  if (IsBidiUI())
  {
    
    if (aBidiLevel & BIDI_LEVEL_UNDEFINED)
      aBidiLevel = NS_GET_EMBEDDING_LEVEL(theFrame);

    int32_t start;
    int32_t end;
    nsIFrame* frameBefore;
    nsIFrame* frameAfter;
    uint8_t levelBefore;     
    uint8_t levelAfter;      

    theFrame->GetOffsets(start, end);
    if (start == 0 || end == 0 || start == theFrameOffset || end == theFrameOffset)
    {
      nsPrevNextBidiLevels levels = aFrameSelection->
        GetPrevNextBidiLevels(aContentNode, aOffset, false);
    
      
      if (levels.mFrameBefore || levels.mFrameAfter)
      {
        frameBefore = levels.mFrameBefore;
        frameAfter = levels.mFrameAfter;
        levelBefore = levels.mLevelBefore;
        levelAfter = levels.mLevelAfter;

        if ((levelBefore != levelAfter) || (aBidiLevel != levelBefore))
        {
          aBidiLevel = std::max(aBidiLevel, std::min(levelBefore, levelAfter));                                  
          aBidiLevel = std::min(aBidiLevel, std::max(levelBefore, levelAfter));                                  
          if (aBidiLevel == levelBefore                                                                      
              || (aBidiLevel > levelBefore && aBidiLevel < levelAfter && !((aBidiLevel ^ levelBefore) & 1))    
              || (aBidiLevel < levelBefore && aBidiLevel > levelAfter && !((aBidiLevel ^ levelBefore) & 1)))  
          {
            if (theFrame != frameBefore)
            {
              if (frameBefore) 
              {
                theFrame = frameBefore;
                theFrame->GetOffsets(start, end);
                theFrameOffset = end;
              }
              else 
              {
                
                
                
                
                uint8_t baseLevel = NS_GET_BASE_LEVEL(frameAfter);
                if (baseLevel != levelAfter)
                {
                  nsPeekOffsetStruct pos(eSelectBeginLine, eDirPrevious, 0, 0, false, true, false, true);
                  if (NS_SUCCEEDED(frameAfter->PeekOffset(&pos))) {
                    theFrame = pos.mResultFrame;
                    theFrameOffset = pos.mContentOffset;
                  }
                }
              }
            }
          }
          else if (aBidiLevel == levelAfter                                                                     
                   || (aBidiLevel > levelBefore && aBidiLevel < levelAfter && !((aBidiLevel ^ levelAfter) & 1))   
                   || (aBidiLevel < levelBefore && aBidiLevel > levelAfter && !((aBidiLevel ^ levelAfter) & 1)))  
          {
            if (theFrame != frameAfter)
            {
              if (frameAfter)
              {
                
                theFrame = frameAfter;
                theFrame->GetOffsets(start, end);
                theFrameOffset = start;
              }
              else 
              {
                
                
                
                
                uint8_t baseLevel = NS_GET_BASE_LEVEL(frameBefore);
                if (baseLevel != levelBefore)
                {
                  nsPeekOffsetStruct pos(eSelectEndLine, eDirNext, 0, 0, false, true, false, true);
                  if (NS_SUCCEEDED(frameBefore->PeekOffset(&pos))) {
                    theFrame = pos.mResultFrame;
                    theFrameOffset = pos.mContentOffset;
                  }
                }
              }
            }
          }
          else if (aBidiLevel > levelBefore && aBidiLevel < levelAfter  
                   && !((levelBefore ^ levelAfter) & 1)                 
                   && ((aBidiLevel ^ levelAfter) & 1))                  
          {
            if (NS_SUCCEEDED(aFrameSelection->GetFrameFromLevel(frameAfter, eDirNext, aBidiLevel, &theFrame)))
            {
              theFrame->GetOffsets(start, end);
              levelAfter = NS_GET_EMBEDDING_LEVEL(theFrame);
              if (aBidiLevel & 1) 
                theFrameOffset = (levelAfter & 1) ? start : end;
              else               
                theFrameOffset = (levelAfter & 1) ? end : start;
            }
          }
          else if (aBidiLevel < levelBefore && aBidiLevel > levelAfter  
                   && !((levelBefore ^ levelAfter) & 1)                 
                   && ((aBidiLevel ^ levelAfter) & 1))                  
          {
            if (NS_SUCCEEDED(aFrameSelection->GetFrameFromLevel(frameBefore, eDirPrevious, aBidiLevel, &theFrame)))
            {
              theFrame->GetOffsets(start, end);
              levelBefore = NS_GET_EMBEDDING_LEVEL(theFrame);
              if (aBidiLevel & 1) 
                theFrameOffset = (levelBefore & 1) ? end : start;
              else               
                theFrameOffset = (levelBefore & 1) ? start : end;
            }
          }   
        }
      }
    }
  }

  *aReturnFrame = theFrame;
  *aReturnOffset = theFrameOffset;
  return NS_OK;
}

size_t nsCaret::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  size_t total = aMallocSizeOf(this);
  if (mPresShell) {
    
    
    total += mPresShell->SizeOfOnlyThis(aMallocSizeOf);
  }
  if (mDomSelectionWeak) {
    
    
    total += mDomSelectionWeak->SizeOfOnlyThis(aMallocSizeOf);
  }
  if (mBlinkTimer) {
    total += mBlinkTimer->SizeOfIncludingThis(aMallocSizeOf);
  }
  return total;
}

bool nsCaret::IsMenuPopupHidingCaret()
{
#ifdef MOZ_XUL
  
  nsXULPopupManager *popMgr = nsXULPopupManager::GetInstance();
  nsTArray<nsIFrame*> popups;
  popMgr->GetVisiblePopups(popups);

  if (popups.Length() == 0)
    return false; 

  
  
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsISelection> domSelection = do_QueryReferent(mDomSelectionWeak);
  if (!domSelection)
    return true; 
  domSelection->GetFocusNode(getter_AddRefs(node));
  if (!node)
    return true; 
  nsCOMPtr<nsIContent> caretContent = do_QueryInterface(node);
  if (!caretContent)
    return true; 

  
  
  for (uint32_t i=0; i<popups.Length(); i++) {
    nsMenuPopupFrame* popupFrame = static_cast<nsMenuPopupFrame*>(popups[i]);
    nsIContent* popupContent = popupFrame->GetContent();

    if (nsContentUtils::ContentIsDescendantOf(caretContent, popupContent)) {
      
      
      return false;
    }

    if (popupFrame->PopupType() == ePopupTypeMenu && !popupFrame->IsContextMenu()) {
      
      
      
      return true;
    }
  }
#endif

  
  return false;
}

void
nsCaret::ComputeCaretRects(nsIFrame* aFrame, int32_t aFrameOffset,
                           nsRect* aCaretRect, nsRect* aHookRect)
{
  NS_ASSERTION(aFrame, "Should have a frame here");

  nscoord bidiIndicatorSize;
  nsresult rv =
    GetGeometryForFrame(aFrame, aFrameOffset, aCaretRect, &bidiIndicatorSize);
  if (NS_FAILED(rv)) {
    *aCaretRect = *aHookRect = nsRect();
    return;
  }

  
  const nsStyleVisibility* vis = aFrame->StyleVisibility();
  if (NS_STYLE_DIRECTION_RTL == vis->mDirection) {
    aCaretRect->x -= aCaretRect->width;
  }

  aHookRect->SetEmpty();

  bool isCaretRTL;
  nsIBidiKeyboard* bidiKeyboard = nsContentUtils::GetBidiKeyboard();
  if (bidiKeyboard && NS_SUCCEEDED(bidiKeyboard->IsLangRTL(&isCaretRTL)) &&
      IsBidiUI()) {
    
    
    
    aHookRect->SetRect(aCaretRect->x + (isCaretRTL ? bidiIndicatorSize * -1 : aCaretRect->width),
                       aCaretRect->y + bidiIndicatorSize,
                       bidiIndicatorSize,
                       aCaretRect->width);
  }
}



void nsCaret::CaretBlinkCallback(nsITimer* aTimer, void* aClosure)
{
  nsCaret* theCaret = reinterpret_cast<nsCaret*>(aClosure);
  if (!theCaret) {
    return;
  }
  theCaret->mIsBlinkOn = !theCaret->mIsBlinkOn;
  theCaret->SchedulePaint();
}



nsFrameSelection*
nsCaret::GetFrameSelection()
{
  return GetSelectionInternal()->GetFrameSelection();
}

void
nsCaret::SetIgnoreUserModify(bool aIgnoreUserModify)
{
  mIgnoreUserModify = aIgnoreUserModify;
  SchedulePaint();
}
