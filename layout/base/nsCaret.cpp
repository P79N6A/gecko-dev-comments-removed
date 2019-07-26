







#include "nsCOMPtr.h"

#include "nsITimer.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsFrameSelection.h"
#include "nsIFrame.h"
#include "nsIScrollableFrame.h"
#include "nsIDOMNode.h"
#include "nsIDOMRange.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsIDOMCharacterData.h"
#include "nsIContent.h"
#include "nsIPresShell.h"
#include "nsRenderingContext.h"
#include "nsPresContext.h"
#include "nsBlockFrame.h"
#include "nsISelectionController.h"
#include "nsDisplayList.h"
#include "nsCaret.h"
#include "nsTextFrame.h"
#include "nsXULPopupManager.h"
#include "nsMenuPopupFrame.h"
#include "nsTextFragment.h"
#include "nsThemeConstants.h"
#include "mozilla/Preferences.h"
#include "mozilla/LookAndFeel.h"
#include <algorithm>




static const int32_t kMinBidiIndicatorPixels = 2;

#ifdef IBMBIDI
#include "nsIBidiKeyboard.h"
#include "nsContentUtils.h"
#endif 

using namespace mozilla;







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



nsCaret::nsCaret()
: mPresShell(nullptr)
, mBlinkRate(500)
, mVisible(false)
, mDrawn(false)
, mPendingDraw(false)
, mReadOnly(false)
, mShowDuringSelection(false)
, mIgnoreUserModify(true)
#ifdef IBMBIDI
, mKeyboardRTL(false)
, mLastBidiLevel(0)
#endif
, mLastContentOffset(0)
, mLastHint(nsFrameSelection::HINTLEFT)
{
}


nsCaret::~nsCaret()
{
  KillTimer();
}


nsresult nsCaret::Init(nsIPresShell *inPresShell)
{
  NS_ENSURE_ARG(inPresShell);

  mPresShell = do_GetWeakReference(inPresShell);    
  NS_ASSERTION(mPresShell, "Hey, pres shell should support weak refs");

  
  
  mCaretWidthCSSPx = LookAndFeel::GetInt(LookAndFeel::eIntID_CaretWidth, 1);
  mCaretAspectRatio =
    LookAndFeel::GetFloat(LookAndFeel::eFloatID_CaretAspectRatio, 0.0f);

  mBlinkRate = static_cast<uint32_t>(
    LookAndFeel::GetInt(LookAndFeel::eIntID_CaretBlinkTime, mBlinkRate));
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
  
  
  if (mVisible)
  {
    StartBlinking();
  }
#ifdef IBMBIDI
  mBidiUI = Preferences::GetBool("bidi.browser.ui");
#endif

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
  PRUnichar ch = frag->CharAt(aOffset);
  return 0x2e80 <= ch && ch <= 0xd7ff;
}

nsCaret::Metrics nsCaret::ComputeMetrics(nsIFrame* aFrame, int32_t aOffset, nscoord aCaretHeight)
{
  
  nscoord caretWidth = (aCaretHeight * mCaretAspectRatio) +
                       nsPresContext::CSSPixelsToAppUnits(mCaretWidthCSSPx);

  if (DrawCJKCaret(aFrame, aOffset)) {
    caretWidth += nsPresContext::CSSPixelsToAppUnits(1);
  }
  nscoord bidiIndicatorSize = nsPresContext::CSSPixelsToAppUnits(kMinBidiIndicatorPixels);
  bidiIndicatorSize = std::max(caretWidth, bidiIndicatorSize);

  
  
  uint32_t tpp = aFrame->PresContext()->AppUnitsPerDevPixel();
  Metrics result;
  result.mCaretWidth = NS_ROUND_BORDER_TO_PIXELS(caretWidth, tpp);
  result.mBidiIndicatorSize = NS_ROUND_BORDER_TO_PIXELS(bidiIndicatorSize, tpp);
  return result;
}


void nsCaret::Terminate()
{
  
  
  
  KillTimer();
  mBlinkTimer = nullptr;

  
  nsCOMPtr<nsISelection> domSelection = do_QueryReferent(mDomSelectionWeak);
  nsCOMPtr<nsISelectionPrivate> privateSelection(do_QueryInterface(domSelection));
  if (privateSelection)
    privateSelection->RemoveSelectionListener(this);
  mDomSelectionWeak = nullptr;
  mPresShell = nullptr;

  mLastContent = nullptr;
}


NS_IMPL_ISUPPORTS1(nsCaret, nsISelectionListener)


nsISelection* nsCaret::GetCaretDOMSelection()
{
  nsCOMPtr<nsISelection> sel(do_QueryReferent(mDomSelectionWeak));
  return sel;  
}


nsresult nsCaret::SetCaretDOMSelection(nsISelection *aDOMSel)
{
  NS_ENSURE_ARG_POINTER(aDOMSel);
  mDomSelectionWeak = do_GetWeakReference(aDOMSel);   
  if (mVisible)
  {
    
    StopBlinking();
    
    StartBlinking();
  }
  return NS_OK;
}



void nsCaret::SetCaretVisible(bool inMakeVisible)
{
  mVisible = inMakeVisible;
  if (mVisible) {
    SetIgnoreUserModify(true);
    StartBlinking();
  } else {
    StopBlinking();
    SetIgnoreUserModify(false);
  }
}



nsresult nsCaret::GetCaretVisible(bool *outMakeVisible)
{
  NS_ENSURE_ARG_POINTER(outMakeVisible);
  *outMakeVisible = (mVisible && MustDrawCaret(true));
  return NS_OK;
}



void nsCaret::SetCaretReadOnly(bool inMakeReadonly)
{
  mReadOnly = inMakeReadonly;
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

  nsIFrame *frame = aFrame->GetContentInsertionFrame();
  NS_ASSERTION(frame, "We should not be in the middle of reflow");
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

nsIFrame* nsCaret::GetGeometry(nsISelection* aSelection, nsRect* aRect,
                               nscoord* aBidiIndicatorSize)
{
  nsCOMPtr<nsIDOMNode> focusNode;
  nsresult rv = aSelection->GetFocusNode(getter_AddRefs(focusNode));
  if (NS_FAILED(rv) || !focusNode)
    return nullptr;

  int32_t focusOffset;
  rv = aSelection->GetFocusOffset(&focusOffset);
  if (NS_FAILED(rv))
    return nullptr;
    
  nsCOMPtr<nsIContent> contentNode = do_QueryInterface(focusNode);
  if (!contentNode)
    return nullptr;

  nsRefPtr<nsFrameSelection> frameSelection = GetFrameSelection();
  if (!frameSelection)
    return nullptr;
  uint8_t bidiLevel = frameSelection->GetCaretBidiLevel();
  nsIFrame* frame;
  int32_t frameOffset;
  rv = GetCaretFrameForNodeOffset(contentNode, focusOffset,
                                  frameSelection->GetHint(), bidiLevel,
                                  &frame, &frameOffset);
  if (NS_FAILED(rv) || !frame)
    return nullptr;

  GetGeometryForFrame(frame, frameOffset, aRect, aBidiIndicatorSize);
  return frame;
}

void nsCaret::DrawCaretAfterBriefDelay()
{
  
  if (!mBlinkTimer) {
    nsresult  err;
    mBlinkTimer = do_CreateInstance("@mozilla.org/timer;1", &err);    
    if (NS_FAILED(err))
      return;
  }    

  mBlinkTimer->InitWithFuncCallback(CaretBlinkCallback, this, 0,
                                    nsITimer::TYPE_ONE_SHOT);
}

void nsCaret::EraseCaret()
{
  if (mDrawn) {
    DrawCaret(true);
    if (mReadOnly && mBlinkRate) {
      
      
      
      DrawCaretAfterBriefDelay();
    }
  }
}

void nsCaret::SetVisibilityDuringSelection(bool aVisibility) 
{
  mShowDuringSelection = aVisibility;
}

static
nsFrameSelection::HINT GetHintForPosition(nsIDOMNode* aNode, int32_t aOffset)
{
  nsFrameSelection::HINT hint = nsFrameSelection::HINTLEFT;
  nsCOMPtr<nsIContent> node = do_QueryInterface(aNode);
  if (!node || aOffset < 1) {
    return hint;
  }
  const nsTextFragment* text = node->GetText();
  if (text && text->CharAt(aOffset - 1) == '\n') {
    
    hint = nsFrameSelection::HINTRIGHT;
  }
  return hint;
}

nsresult nsCaret::DrawAtPosition(nsIDOMNode* aNode, int32_t aOffset)
{
  NS_ENSURE_ARG(aNode);

  uint8_t bidiLevel;
  nsRefPtr<nsFrameSelection> frameSelection = GetFrameSelection();
  if (!frameSelection)
    return NS_ERROR_FAILURE;
  bidiLevel = frameSelection->GetCaretBidiLevel();

  
  
  
  mBlinkRate = 0;

  nsresult rv = DrawAtPositionWithHint(aNode, aOffset,
                                       GetHintForPosition(aNode, aOffset),
                                       bidiLevel, true)
    ?  NS_OK : NS_ERROR_FAILURE;
  ToggleDrawnStatus();
  return rv;
}

nsIFrame * nsCaret::GetCaretFrame(int32_t *aOffset)
{
  
  if (!mDrawn)
    return nullptr;

  
  
  int32_t offset;
  nsIFrame *frame = nullptr;
  nsresult rv = GetCaretFrameForNodeOffset(mLastContent, mLastContentOffset,
                                           mLastHint, mLastBidiLevel, &frame,
                                           &offset);
  if (NS_FAILED(rv))
    return nullptr;

  if (aOffset) {
    *aOffset = offset;
  }
  return frame;
}

void nsCaret::InvalidateOutsideCaret()
{
  nsIFrame *frame = GetCaretFrame();

  
  if (frame && !frame->GetVisualOverflowRect().Contains(GetCaretRect())) {
    frame->SchedulePaint();
  }
}

void nsCaret::UpdateCaretPosition()
{
  
  if (!mDrawn)
    return;

  
  
  mDrawn = false;
  DrawCaret(false);
}

void nsCaret::PaintCaret(nsDisplayListBuilder *aBuilder,
                         nsRenderingContext *aCtx,
                         nsIFrame* aForFrame,
                         const nsPoint &aOffset)
{
  NS_ASSERTION(mDrawn, "The caret shouldn't be drawing");

  const nsRect drawCaretRect = mCaretRect + aOffset;
  int32_t contentOffset;

#ifdef DEBUG
  nsIFrame* frame =
#endif
    GetCaretFrame(&contentOffset);
  NS_ASSERTION(frame == aForFrame, "We're referring different frame");
  
  int32_t startOffset, endOffset;
  if (aForFrame->GetType() == nsGkAtoms::textFrame &&
      (NS_FAILED(aForFrame->GetOffsets(startOffset, endOffset)) ||
      startOffset > contentOffset ||
      endOffset < contentOffset)) {
    return;
  }
  nscolor foregroundColor = aForFrame->GetCaretColorAt(contentOffset);

  
  
  
  
  nsPresContext* presContext = aForFrame->PresContext();

  if (GetHookRect().IsEmpty() && presContext) {
    nsITheme *theme = presContext->GetTheme();
    if (theme && theme->ThemeSupportsWidget(presContext, aForFrame, NS_THEME_TEXTFIELD_CARET)) {
      nscolor fieldText;
      nsresult rv = LookAndFeel::GetColor(LookAndFeel::eColorID__moz_fieldtext,
                                          &fieldText);
      if (NS_SUCCEEDED(rv) && fieldText == foregroundColor) {
        theme->DrawWidgetBackground(aCtx, aForFrame, NS_THEME_TEXTFIELD_CARET,
                                    drawCaretRect, drawCaretRect);
        return;
      }
    }
  }

  aCtx->SetColor(foregroundColor);
  aCtx->FillRect(drawCaretRect);
  if (!GetHookRect().IsEmpty())
    aCtx->FillRect(GetHookRect() + aOffset);
}



NS_IMETHODIMP nsCaret::NotifySelectionChanged(nsIDOMDocument *, nsISelection *aDomSel, int16_t aReason)
{
  if (aReason & nsISelectionListener::MOUSEUP_REASON)
    return NS_OK;

  nsCOMPtr<nsISelection> domSel(do_QueryReferent(mDomSelectionWeak));

  
  
  
  
  
  
  

  if (domSel != aDomSel)
    return NS_OK;

  if (mVisible)
  {
    
    StopBlinking();

    
    StartBlinking();
  }

  return NS_OK;
}



void nsCaret::KillTimer()
{
  if (mBlinkTimer)
  {
    mBlinkTimer->Cancel();
  }
}



nsresult nsCaret::PrimeTimer()
{
  
  if (!mReadOnly && mBlinkRate > 0)
  {
    if (!mBlinkTimer) {
      nsresult  err;
      mBlinkTimer = do_CreateInstance("@mozilla.org/timer;1", &err);    
      if (NS_FAILED(err))
        return err;
    }    

    mBlinkTimer->InitWithFuncCallback(CaretBlinkCallback, this, mBlinkRate,
                                      nsITimer::TYPE_REPEATING_SLACK);
  }

  return NS_OK;
}


void nsCaret::StartBlinking()
{
  if (mReadOnly) {
    
    
    DrawCaretAfterBriefDelay();
    return;
  }
  PrimeTimer();

  
  
  
  
  
  if (mDrawn)
    DrawCaret(true);

  DrawCaret(true);    
}



void nsCaret::StopBlinking()
{
  if (mDrawn)     
    DrawCaret(true);

  NS_ASSERTION(!mDrawn, "Caret still drawn after StopBlinking().");
  KillTimer();
}

bool
nsCaret::DrawAtPositionWithHint(nsIDOMNode*             aNode,
                                int32_t                 aOffset,
                                nsFrameSelection::HINT  aFrameHint,
                                uint8_t                 aBidiLevel,
                                bool                    aInvalidate)
{
  nsCOMPtr<nsIContent> contentNode = do_QueryInterface(aNode);
  if (!contentNode)
    return false;

  nsIFrame* theFrame = nullptr;
  int32_t   theFrameOffset = 0;

  nsresult rv = GetCaretFrameForNodeOffset(contentNode, aOffset, aFrameHint, aBidiLevel,
                                           &theFrame, &theFrameOffset);
  if (NS_FAILED(rv) || !theFrame)
    return false;

  
  const nsStyleUserInterface* userinterface = theFrame->StyleUserInterface();
  if ((!mIgnoreUserModify &&
       userinterface->mUserModify == NS_STYLE_USER_MODIFY_READ_ONLY) ||
      (userinterface->mUserInput == NS_STYLE_USER_INPUT_NONE) ||
      (userinterface->mUserInput == NS_STYLE_USER_INPUT_DISABLED))
  {
    return false;
  }  

  if (!mDrawn)
  {
    
    mLastContent = contentNode;
    mLastContentOffset = aOffset;
    mLastHint = aFrameHint;
    mLastBidiLevel = aBidiLevel;

    
    if (aBidiLevel & BIDI_LEVEL_UNDEFINED) {
      nsRefPtr<nsFrameSelection> frameSelection = GetFrameSelection();
      if (!frameSelection)
        return false;
      frameSelection->SetCaretBidiLevel(NS_GET_EMBEDDING_LEVEL(theFrame));
    }

    
    if (!UpdateCaretRects(theFrame, theFrameOffset))
      return false;
  }

  if (aInvalidate)
    theFrame->SchedulePaint();

  return true;
}

nsresult 
nsCaret::GetCaretFrameForNodeOffset(nsIContent*             aContentNode,
                                    int32_t                 aOffset,
                                    nsFrameSelection::HINT aFrameHint,
                                    uint8_t                 aBidiLevel,
                                    nsIFrame**              aReturnFrame,
                                    int32_t*                aReturnOffset)
{

  
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell)
    return NS_ERROR_FAILURE;

  if (!aContentNode || !aContentNode->IsInDoc() ||
      presShell->GetDocument() != aContentNode->GetCurrentDoc())
    return NS_ERROR_FAILURE;

  nsRefPtr<nsFrameSelection> frameSelection = GetFrameSelection();
  if (!frameSelection)
    return NS_ERROR_FAILURE;

  nsIFrame* theFrame = nullptr;
  int32_t   theFrameOffset = 0;

  theFrame = frameSelection->GetFrameForNodeOffset(aContentNode, aOffset,
                                                   aFrameHint, &theFrameOffset);
  if (!theFrame)
    return NS_ERROR_FAILURE;

  
  
  
  
  AdjustCaretFrameForLineEnd(&theFrame, &theFrameOffset);
  
  
  
  
  
  
  
  
  if (mBidiUI)
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
      nsPrevNextBidiLevels levels = frameSelection->
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
            if (NS_SUCCEEDED(frameSelection->GetFrameFromLevel(frameAfter, eDirNext, aBidiLevel, &theFrame)))
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
            if (NS_SUCCEEDED(frameSelection->GetFrameFromLevel(frameBefore, eDirPrevious, aBidiLevel, &theFrame)))
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

  NS_ASSERTION(!theFrame || theFrame->PresContext()->PresShell() == presShell,
               "caret frame is in wrong document");
  *aReturnFrame = theFrame;
  *aReturnOffset = theFrameOffset;
  return NS_OK;
}

nsresult nsCaret::CheckCaretDrawingState()
{
  if (mDrawn) {
    
    if (!mVisible || !MustDrawCaret(true))
      EraseCaret();
  }
  else
  {
    
    if (mPendingDraw && (mVisible && MustDrawCaret(true)))
      DrawCaret(true);
  }
  return NS_OK;
}













bool nsCaret::MustDrawCaret(bool aIgnoreDrawnState)
{
  if (!aIgnoreDrawnState && mDrawn)
    return true;

  nsCOMPtr<nsISelection> domSelection = do_QueryReferent(mDomSelectionWeak);
  if (!domSelection)
    return false;

  bool isCollapsed;
  if (NS_FAILED(domSelection->GetIsCollapsed(&isCollapsed)))
    return false;

  if (mShowDuringSelection)
    return true;      

  if (IsMenuPopupHidingCaret())
    return false;

  return isCollapsed;
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

void nsCaret::DrawCaret(bool aInvalidate)
{
  
  if (!MustDrawCaret(false))
    return;
  
  
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  NS_ENSURE_TRUE_VOID(presShell);
  {
    if (presShell->IsPaintingSuppressed())
    {
      if (!mDrawn)
        mPendingDraw = true;

      
      
      return;
    }
  }

  nsCOMPtr<nsIDOMNode> node;
  int32_t offset;
  nsFrameSelection::HINT hint;
  uint8_t bidiLevel;

  if (!mDrawn)
  {
    nsCOMPtr<nsISelection> domSelection = do_QueryReferent(mDomSelectionWeak);
    nsCOMPtr<nsISelectionPrivate> privateSelection(do_QueryInterface(domSelection));
    if (!privateSelection) return;
    
    bool isCollapsed = false;
    domSelection->GetIsCollapsed(&isCollapsed);
    if (!mShowDuringSelection && !isCollapsed)
      return;

    bool hintRight;
    privateSelection->GetInterlinePosition(&hintRight);
    hint = hintRight ? nsFrameSelection::HINTRIGHT : nsFrameSelection::HINTLEFT;

    
    domSelection->GetFocusNode(getter_AddRefs(node));
    if (!node)
      return;
    
    if (NS_FAILED(domSelection->GetFocusOffset(&offset)))
      return;

    nsRefPtr<nsFrameSelection> frameSelection = GetFrameSelection();
    if (!frameSelection)
      return;

    bidiLevel = frameSelection->GetCaretBidiLevel();
    mPendingDraw = false;
  }
  else
  {
    if (!mLastContent)
    {
      mDrawn = false;
      return;
    }
    if (!mLastContent->IsInDoc() ||
        presShell->GetDocument() != mLastContent->GetCurrentDoc())
    {
      mLastContent = nullptr;
      mDrawn = false;
      return;
    }
    node = do_QueryInterface(mLastContent);
    offset = mLastContentOffset;
    hint = mLastHint;
    bidiLevel = mLastBidiLevel;
  }

  DrawAtPositionWithHint(node, offset, hint, bidiLevel, aInvalidate);
  ToggleDrawnStatus();
}

bool
nsCaret::UpdateCaretRects(nsIFrame* aFrame, int32_t aFrameOffset)
{
  NS_ASSERTION(aFrame, "Should have a frame here");

  nscoord bidiIndicatorSize;
  nsresult rv =
    GetGeometryForFrame(aFrame, aFrameOffset, &mCaretRect, &bidiIndicatorSize);
  if (NS_FAILED(rv)) {
    return false;
  }

  
  const nsStyleVisibility* vis = aFrame->StyleVisibility();
  if (NS_STYLE_DIRECTION_RTL == vis->mDirection)
    mCaretRect.x -= mCaretRect.width;

#ifdef IBMBIDI
  mHookRect.SetEmpty();

  
  bool isCaretRTL = false;
  nsIBidiKeyboard* bidiKeyboard = nsContentUtils::GetBidiKeyboard();
  
  
  
  if (bidiKeyboard && NS_SUCCEEDED(bidiKeyboard->IsLangRTL(&isCaretRTL)) &&
      mBidiUI) {
    if (isCaretRTL != mKeyboardRTL) {
      






 
      mKeyboardRTL = isCaretRTL;
      nsCOMPtr<nsISelection> domSelection = do_QueryReferent(mDomSelectionWeak);
      if (!domSelection ||
          NS_SUCCEEDED(domSelection->SelectionLanguageChange(mKeyboardRTL)))
        return false;
    }
    
    
    
    mHookRect.SetRect(mCaretRect.x + ((isCaretRTL) ?
                      bidiIndicatorSize * -1 :
                      mCaretRect.width),
                      mCaretRect.y + bidiIndicatorSize,
                      bidiIndicatorSize,
                      mCaretRect.width);
  }
#endif 
  return true;
}



void nsCaret::CaretBlinkCallback(nsITimer *aTimer, void *aClosure)
{
  nsCaret   *theCaret = reinterpret_cast<nsCaret*>(aClosure);
  if (!theCaret) return;
  
  theCaret->DrawCaret(true);
}



already_AddRefed<nsFrameSelection>
nsCaret::GetFrameSelection()
{
  nsCOMPtr<nsISelectionPrivate> privateSelection(do_QueryReferent(mDomSelectionWeak));
  if (!privateSelection)
    return nullptr;
  nsFrameSelection* frameSelection = nullptr;
  privateSelection->GetFrameSelection(&frameSelection);
  return frameSelection;
}

void
nsCaret::SetIgnoreUserModify(bool aIgnoreUserModify)
{
  if (!aIgnoreUserModify && mIgnoreUserModify && mDrawn) {
    
    
    
    
    nsIFrame *frame = GetCaretFrame();
    if (frame) {
      const nsStyleUserInterface* userinterface = frame->StyleUserInterface();
      if (userinterface->mUserModify == NS_STYLE_USER_MODIFY_READ_ONLY) {
        StopBlinking();
      }
    }
  }
  mIgnoreUserModify = aIgnoreUserModify;
}
