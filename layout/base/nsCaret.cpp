









































#include "nsCOMPtr.h"

#include "nsITimer.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsFrameSelection.h"
#include "nsIFrame.h"
#include "nsIScrollableFrame.h"
#include "nsIDOMNode.h"
#include "nsIDOMRange.h"
#include "nsIFontMetrics.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsIDOMCharacterData.h"
#include "nsIContent.h"
#include "nsIPresShell.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsIView.h"
#include "nsIScrollableView.h"
#include "nsIViewManager.h"
#include "nsPresContext.h"
#include "nsILookAndFeel.h"
#include "nsBlockFrame.h"
#include "nsISelectionController.h"
#include "nsDisplayList.h"
#include "nsCaret.h"
#include "nsTextFrame.h"
#include "nsXULPopupManager.h"
#include "nsMenuPopupFrame.h"
#include "nsTextFragment.h"
#include "nsThemeConstants.h"




static const PRInt32 kMinBidiIndicatorPixels = 2;

#ifdef IBMBIDI
#include "nsIBidiKeyboard.h"
#include "nsContentUtils.h"
#endif 



nsCaret::nsCaret()
: mPresShell(nsnull)
, mBlinkRate(500)
, mVisible(PR_FALSE)
, mDrawn(PR_FALSE)
, mReadOnly(PR_FALSE)
, mShowDuringSelection(PR_FALSE)
, mIgnoreUserModify(PR_TRUE)
#ifdef IBMBIDI
, mKeyboardRTL(PR_FALSE)
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

  
  nsILookAndFeel *lookAndFeel = nsnull;
  nsPresContext *presContext = inPresShell->GetPresContext();
  
  
  
  mCaretWidthCSSPx = 1;
  mCaretAspectRatio = 0;
  if (presContext && (lookAndFeel = presContext->LookAndFeel())) {
    PRInt32 tempInt;
    float tempFloat;
    if (NS_SUCCEEDED(lookAndFeel->GetMetric(nsILookAndFeel::eMetric_CaretWidth, tempInt)))
      mCaretWidthCSSPx = (nscoord)tempInt;
    if (NS_SUCCEEDED(lookAndFeel->GetMetric(nsILookAndFeel::eMetricFloat_CaretAspectRatio, tempFloat)))
      mCaretAspectRatio = tempFloat;
    if (NS_SUCCEEDED(lookAndFeel->GetMetric(nsILookAndFeel::eMetric_CaretBlinkTime, tempInt)))
      mBlinkRate = (PRUint32)tempInt;
    if (NS_SUCCEEDED(lookAndFeel->GetMetric(nsILookAndFeel::eMetric_ShowCaretDuringSelection, tempInt)))
      mShowDuringSelection = tempInt ? PR_TRUE : PR_FALSE;
  }
  
  
  

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
  mBidiUI = nsContentUtils::GetBoolPref("bidi.browser.ui");
#endif

  return NS_OK;
}

static PRBool
DrawCJKCaret(nsIFrame* aFrame, PRInt32 aOffset)
{
  nsIContent* content = aFrame->GetContent();
  const nsTextFragment* frag = content->GetText();
  if (!frag)
    return PR_FALSE;
  if (aOffset < 0 || PRUint32(aOffset) >= frag->GetLength())
    return PR_FALSE;
  PRUnichar ch = frag->CharAt(aOffset);
  return 0x2e80 <= ch && ch <= 0xd7ff;
}

nsCaret::Metrics nsCaret::ComputeMetrics(nsIFrame* aFrame, PRInt32 aOffset, nscoord aCaretHeight)
{
  
  nscoord caretWidth = (aCaretHeight * mCaretAspectRatio) +
                       nsPresContext::CSSPixelsToAppUnits(mCaretWidthCSSPx);

  if (DrawCJKCaret(aFrame, aOffset)) {
    caretWidth += nsPresContext::CSSPixelsToAppUnits(1);
  }
  nscoord bidiIndicatorSize = nsPresContext::CSSPixelsToAppUnits(kMinBidiIndicatorPixels);
  bidiIndicatorSize = NS_MAX(caretWidth, bidiIndicatorSize);

  
  
  PRUint32 tpp = aFrame->PresContext()->AppUnitsPerDevPixel();
  Metrics result;
  result.mCaretWidth = NS_ROUND_BORDER_TO_PIXELS(caretWidth, tpp);
  result.mBidiIndicatorSize = NS_ROUND_BORDER_TO_PIXELS(bidiIndicatorSize, tpp);
  return result;
}


void nsCaret::Terminate()
{
  
  
  
  KillTimer();
  mBlinkTimer = nsnull;

  
  nsCOMPtr<nsISelection> domSelection = do_QueryReferent(mDomSelectionWeak);
  nsCOMPtr<nsISelectionPrivate> privateSelection(do_QueryInterface(domSelection));
  if (privateSelection)
    privateSelection->RemoveSelectionListener(this);
  mDomSelectionWeak = nsnull;
  mPresShell = nsnull;

  mLastContent = nsnull;
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



void nsCaret::SetCaretVisible(PRBool inMakeVisible)
{
  mVisible = inMakeVisible;
  if (mVisible) {
    StartBlinking();
    SetIgnoreUserModify(PR_TRUE);
  } else {
    StopBlinking();
    SetIgnoreUserModify(PR_FALSE);
  }
}



nsresult nsCaret::GetCaretVisible(PRBool *outMakeVisible)
{
  NS_ENSURE_ARG_POINTER(outMakeVisible);
  *outMakeVisible = (mVisible && MustDrawCaret(PR_TRUE));
  return NS_OK;
}



void nsCaret::SetCaretReadOnly(PRBool inMakeReadonly)
{
  mReadOnly = inMakeReadonly;
}



nsresult nsCaret::GetCaretCoordinates(EViewCoordinates aRelativeToType,
                                      nsISelection *aDOMSel,
                                      nsRect *outCoordinates,
                                      PRBool *outIsCollapsed,
                                      nsIView **outView)
{
  if (!mPresShell)
    return NS_ERROR_NOT_INITIALIZED;
  if (!outCoordinates || !outIsCollapsed)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsISelection> domSelection = aDOMSel;

  if (outView)
    *outView = nsnull;

  
  outCoordinates->x = -1;
  outCoordinates->y = -1;
  outCoordinates->width = -1;
  outCoordinates->height = -1;
  *outIsCollapsed = PR_FALSE;
  
  nsresult err = domSelection->GetIsCollapsed(outIsCollapsed);
  if (NS_FAILED(err)) 
    return err;
    
  nsCOMPtr<nsIDOMNode>  focusNode;
  
  err = domSelection->GetFocusNode(getter_AddRefs(focusNode));
  if (NS_FAILED(err))
    return err;
  if (!focusNode)
    return NS_ERROR_FAILURE;
  
  PRInt32 focusOffset;
  err = domSelection->GetFocusOffset(&focusOffset);
  if (NS_FAILED(err))
    return err;
    
  nsCOMPtr<nsIContent> contentNode = do_QueryInterface(focusNode);
  if (!contentNode)
    return NS_ERROR_FAILURE;

  
  nsIFrame*       theFrame = nsnull;
  PRInt32         theFrameOffset = 0;

  nsCOMPtr<nsFrameSelection> frameSelection = GetFrameSelection();
  if (!frameSelection)
    return NS_ERROR_FAILURE;
  PRUint8 bidiLevel = frameSelection->GetCaretBidiLevel();
  
  err = GetCaretFrameForNodeOffset(contentNode, focusOffset,
                                   frameSelection->GetHint(), bidiLevel,
                                   &theFrame, &theFrameOffset);
  if (NS_FAILED(err) || !theFrame)
    return err;
  
  nsPoint   viewOffset(0, 0);
  nsIView   *drawingView;     

  GetViewForRendering(theFrame, aRelativeToType, viewOffset, &drawingView, outView);
  if (!drawingView)
    return NS_ERROR_UNEXPECTED;
 
  nsPoint   framePos(0, 0);
  err = theFrame->GetPointFromOffset(theFrameOffset, &framePos);
  if (NS_FAILED(err))
    return err;

  
  if (aRelativeToType == eClosestViewCoordinates)
  {
    theFrame->GetOffsetFromView(viewOffset, &drawingView);
    if (outView)
      *outView = drawingView;
  }
  
  viewOffset += framePos;
  outCoordinates->x = viewOffset.x;
  outCoordinates->y = viewOffset.y;
  outCoordinates->height = theFrame->GetSize().height;
  outCoordinates->width = ComputeMetrics(theFrame, theFrameOffset, outCoordinates->height).mCaretWidth;
  
  return NS_OK;
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
    DrawCaret(PR_TRUE);
    if (mReadOnly && mBlinkRate) {
      
      
      
      DrawCaretAfterBriefDelay();
    }
  }
}

void nsCaret::SetVisibilityDuringSelection(PRBool aVisibility) 
{
  mShowDuringSelection = aVisibility;
}

nsresult nsCaret::DrawAtPosition(nsIDOMNode* aNode, PRInt32 aOffset)
{
  NS_ENSURE_ARG(aNode);

  PRUint8 bidiLevel;
  nsCOMPtr<nsFrameSelection> frameSelection = GetFrameSelection();
  if (!frameSelection)
    return NS_ERROR_FAILURE;
  bidiLevel = frameSelection->GetCaretBidiLevel();

  
  
  
  mBlinkRate = 0;

  
  nsresult rv = DrawAtPositionWithHint(aNode, aOffset,
                                       nsFrameSelection::HINTLEFT,
                                       bidiLevel, PR_TRUE)
    ?  NS_OK : NS_ERROR_FAILURE;
  ToggleDrawnStatus();
  return rv;
}

nsIFrame * nsCaret::GetCaretFrame()
{
  
  if (!mDrawn)
    return nsnull;

  
  
  PRInt32 unused;
  nsIFrame *frame = nsnull;
  nsresult rv = GetCaretFrameForNodeOffset(mLastContent, mLastContentOffset,
                                           mLastHint, mLastBidiLevel, &frame,
                                           &unused);
  if (NS_FAILED(rv))
    return nsnull;

  return frame;
}

void nsCaret::InvalidateOutsideCaret()
{
  nsIFrame *frame = GetCaretFrame();

  
  if (frame && !frame->GetOverflowRect().Contains(GetCaretRect()))
    InvalidateRects(mCaretRect, GetHookRect(), frame);
}

void nsCaret::UpdateCaretPosition()
{
  
  if (!mDrawn)
    return;

  
  
  mDrawn = PR_FALSE;
  DrawCaret(PR_FALSE);
}

void nsCaret::PaintCaret(nsDisplayListBuilder *aBuilder,
                         nsIRenderingContext *aCtx,
                         nsIFrame* aForFrame,
                         const nsPoint &aOffset)
{
  NS_ASSERTION(mDrawn, "The caret shouldn't be drawing");

  const nsRect drawCaretRect = mCaretRect + aOffset;
  nscolor cssColor = aForFrame->GetStyleColor()->mColor;

  
  
  
  
  nsPresContext* presContext = aForFrame->PresContext();

  if (GetHookRect().IsEmpty() && presContext) {
    nsITheme *theme = presContext->GetTheme();
    if (theme && theme->ThemeSupportsWidget(presContext, aForFrame, NS_THEME_TEXTFIELD_CARET)) {
      nsILookAndFeel* lookAndFeel = presContext->LookAndFeel();
      nscolor fieldText;
      if (NS_SUCCEEDED(lookAndFeel->GetColor(nsILookAndFeel::eColor__moz_fieldtext, fieldText)) &&
          fieldText == cssColor) {
        theme->DrawWidgetBackground(aCtx, aForFrame, NS_THEME_TEXTFIELD_CARET,
                                    drawCaretRect, drawCaretRect);
        return;
      }
    }
  }

  aCtx->SetColor(cssColor);
  aCtx->FillRect(drawCaretRect);
  if (!GetHookRect().IsEmpty())
    aCtx->FillRect(GetHookRect() + aOffset);
}



NS_IMETHODIMP nsCaret::NotifySelectionChanged(nsIDOMDocument *, nsISelection *aDomSel, PRInt16 aReason)
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
    DrawCaret(PR_TRUE);

  DrawCaret(PR_TRUE);    
}



void nsCaret::StopBlinking()
{
  if (mDrawn)     
    DrawCaret(PR_TRUE);

  NS_ASSERTION(!mDrawn, "Caret still drawn after StopBlinking().");
  KillTimer();
}

PRBool
nsCaret::DrawAtPositionWithHint(nsIDOMNode*             aNode,
                                PRInt32                 aOffset,
                                nsFrameSelection::HINT  aFrameHint,
                                PRUint8                 aBidiLevel,
                                PRBool                  aInvalidate)
{
  nsCOMPtr<nsIContent> contentNode = do_QueryInterface(aNode);
  if (!contentNode)
    return PR_FALSE;

  nsIFrame* theFrame = nsnull;
  PRInt32   theFrameOffset = 0;

  nsresult rv = GetCaretFrameForNodeOffset(contentNode, aOffset, aFrameHint, aBidiLevel,
                                           &theFrame, &theFrameOffset);
  if (NS_FAILED(rv) || !theFrame)
    return PR_FALSE;

  
  const nsStyleUserInterface* userinterface = theFrame->GetStyleUserInterface();
  if ((!mIgnoreUserModify &&
       userinterface->mUserModify == NS_STYLE_USER_MODIFY_READ_ONLY) ||
      (userinterface->mUserInput == NS_STYLE_USER_INPUT_NONE) ||
      (userinterface->mUserInput == NS_STYLE_USER_INPUT_DISABLED))
  {
    return PR_FALSE;
  }  

  if (!mDrawn)
  {
    
    mLastContent = contentNode;
    mLastContentOffset = aOffset;
    mLastHint = aFrameHint;
    mLastBidiLevel = aBidiLevel;

    
    if (aBidiLevel & BIDI_LEVEL_UNDEFINED) {
      nsCOMPtr<nsFrameSelection> frameSelection = GetFrameSelection();
      if (!frameSelection)
        return PR_FALSE;
      frameSelection->SetCaretBidiLevel(NS_GET_EMBEDDING_LEVEL(theFrame));
    }

    
    rv = UpdateCaretRects(theFrame, theFrameOffset);
    if (NS_FAILED(rv))
      return PR_FALSE;
  }

  if (aInvalidate)
    InvalidateRects(mCaretRect, mHookRect, theFrame);

  return PR_TRUE;
}







static nsIFrame*
CheckForTrailingTextFrameRecursive(nsIFrame* aFrame, nsIFrame* aStopAtFrame)
{
  if (aFrame == aStopAtFrame ||
      ((aFrame->GetType() == nsGkAtoms::textFrame &&
       (static_cast<nsTextFrame*>(aFrame))->IsAtEndOfLine())))
    return aFrame;
  if (!aFrame->IsFrameOfType(nsIFrame::eLineParticipant))
    return nsnull;

  for (nsIFrame* f = aFrame->GetFirstChild(nsnull); f; f = f->GetNextSibling())
  {
    nsIFrame* r = CheckForTrailingTextFrameRecursive(f, aStopAtFrame);
    if (r)
      return r;
  }
  return nsnull;
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
      PRBool isValid;
      nsBlockInFlowLineIterator iter(blockParent, aFrame, &isValid);
      return isValid ? iter.GetLine().get() : nsnull;
    }
    aFrame = parent;
  }
  return nsnull;
}

static void
AdjustCaretFrameForLineEnd(nsIFrame** aFrame, PRInt32* aOffset)
{
  nsLineBox* line = FindContainingLine(*aFrame);
  if (!line)
    return;
  PRInt32 count = line->GetChildCount();
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

nsresult 
nsCaret::GetCaretFrameForNodeOffset(nsIContent*             aContentNode,
                                    PRInt32                 aOffset,
                                    nsFrameSelection::HINT aFrameHint,
                                    PRUint8                 aBidiLevel,
                                    nsIFrame**              aReturnFrame,
                                    PRInt32*                aReturnOffset)
{

  
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsFrameSelection> frameSelection = GetFrameSelection();
  if (!frameSelection)
    return NS_ERROR_FAILURE;

  nsIFrame* theFrame = nsnull;
  PRInt32   theFrameOffset = 0;

  theFrame = frameSelection->GetFrameForNodeOffset(aContentNode, aOffset,
                                                   aFrameHint, &theFrameOffset);
  if (!theFrame)
    return NS_ERROR_FAILURE;

  
  
  
  
  AdjustCaretFrameForLineEnd(&theFrame, &theFrameOffset);
  
  
  
  
  
  
  
  
  
  if (mBidiUI)
  {
    
    if (aBidiLevel & BIDI_LEVEL_UNDEFINED)
      aBidiLevel = NS_GET_EMBEDDING_LEVEL(theFrame);

    PRInt32 start;
    PRInt32 end;
    nsIFrame* frameBefore;
    nsIFrame* frameAfter;
    PRUint8 levelBefore;     
    PRUint8 levelAfter;      

    theFrame->GetOffsets(start, end);
    if (start == 0 || end == 0 || start == theFrameOffset || end == theFrameOffset)
    {
      nsPrevNextBidiLevels levels = frameSelection->
        GetPrevNextBidiLevels(aContentNode, aOffset, PR_FALSE);
    
      
      if (levels.mFrameBefore || levels.mFrameAfter)
      {
        frameBefore = levels.mFrameBefore;
        frameAfter = levels.mFrameAfter;
        levelBefore = levels.mLevelBefore;
        levelAfter = levels.mLevelAfter;

        if ((levelBefore != levelAfter) || (aBidiLevel != levelBefore))
        {
          aBidiLevel = NS_MAX(aBidiLevel, NS_MIN(levelBefore, levelAfter));                                  
          aBidiLevel = NS_MIN(aBidiLevel, NS_MAX(levelBefore, levelAfter));                                  
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
                
                
                
                
                PRUint8 baseLevel = NS_GET_BASE_LEVEL(frameAfter);
                if (baseLevel != levelAfter)
                {
                  nsPeekOffsetStruct pos;
                  pos.SetData(eSelectBeginLine, eDirPrevious, 0, 0, PR_FALSE, PR_TRUE, PR_FALSE, PR_TRUE);
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
                
                
                
                
                PRUint8 baseLevel = NS_GET_BASE_LEVEL(frameBefore);
                if (baseLevel != levelBefore)
                {
                  nsPeekOffsetStruct pos;
                  pos.SetData(eSelectEndLine, eDirNext, 0, 0, PR_FALSE, PR_TRUE, PR_FALSE, PR_TRUE);
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
  *aReturnFrame = theFrame;
  *aReturnOffset = theFrameOffset;
  return NS_OK;
}



void nsCaret::GetViewForRendering(nsIFrame *caretFrame,
                                  EViewCoordinates coordType,
                                  nsPoint &viewOffset,
                                  nsIView **outRenderingView,
                                  nsIView **outRelativeView)
{
  if (!caretFrame || !outRenderingView)
    return;

  *outRenderingView = nsnull;
  if (outRelativeView)
    *outRelativeView = nsnull;
  
  NS_ASSERTION(caretFrame, "Should have a frame here");
 
  viewOffset.x = 0;
  viewOffset.y = 0;
  
  nsPoint withinViewOffset(0, 0);
  
  nsIView* theView = nsnull;
  caretFrame->GetOffsetFromView(withinViewOffset, &theView);
  if (!theView)
      return;

  if (outRelativeView && coordType == eClosestViewCoordinates)
    *outRelativeView = theView;

  
  nsIView* returnView = nsIView::GetViewFor(theView->GetNearestWidget(nsnull));
  
  
  if (coordType == eRenderingViewCoordinates) {
    if (returnView) {
      
      withinViewOffset += theView->GetOffsetTo(returnView);
      
      
      
      withinViewOffset += returnView->GetPosition() -
                          returnView->GetBounds().TopLeft();
      viewOffset = withinViewOffset;

      if (outRelativeView)
        *outRelativeView = returnView;
    }
  }
  else {
    
    withinViewOffset += theView->GetOffsetTo(nsnull);
    viewOffset = withinViewOffset;

    
    if (outRelativeView && coordType == eTopLevelWindowCoordinates) {
      nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
      if (presShell) {
        nsIViewManager* vm =
          presShell->GetPresContext()->RootPresContext()->PresShell()->GetViewManager();
        if (vm) {
          vm->GetRootView(*outRelativeView);
        }
      }
    }
  }

  *outRenderingView = returnView;
}

nsresult nsCaret::CheckCaretDrawingState() 
{
  
  if (mDrawn && (!mVisible || !MustDrawCaret(PR_TRUE)))
    EraseCaret();
  return NS_OK;
}













PRBool nsCaret::MustDrawCaret(PRBool aIgnoreDrawnState)
{
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (presShell) {
    PRBool isPaintingSuppressed;
    presShell->IsPaintingSuppressed(&isPaintingSuppressed);
    if (isPaintingSuppressed)
      return PR_FALSE;
  }

  if (!aIgnoreDrawnState && mDrawn)
    return PR_TRUE;

  nsCOMPtr<nsISelection> domSelection = do_QueryReferent(mDomSelectionWeak);
  if (!domSelection)
    return PR_FALSE;
  PRBool isCollapsed;

  if (NS_FAILED(domSelection->GetIsCollapsed(&isCollapsed)))
    return PR_FALSE;

  if (mShowDuringSelection)
    return PR_TRUE;      

  if (IsMenuPopupHidingCaret())
    return PR_FALSE;

  return isCollapsed;
}

PRBool nsCaret::IsMenuPopupHidingCaret()
{
#ifdef MOZ_XUL
  
  nsXULPopupManager *popMgr = nsXULPopupManager::GetInstance();
  nsTArray<nsIFrame*> popups = popMgr->GetVisiblePopups();

  if (popups.Length() == 0)
    return PR_FALSE; 

  
  
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsISelection> domSelection = do_QueryReferent(mDomSelectionWeak);
  if (!domSelection)
    return PR_TRUE; 
  domSelection->GetFocusNode(getter_AddRefs(node));
  if (!node)
    return PR_TRUE; 
  nsCOMPtr<nsIContent> caretContent = do_QueryInterface(node);
  if (!caretContent)
    return PR_TRUE; 

  
  
  for (PRUint32 i=0; i<popups.Length(); i++) {
    nsMenuPopupFrame* popupFrame = static_cast<nsMenuPopupFrame*>(popups[i]);
    nsIContent* popupContent = popupFrame->GetContent();

    if (nsContentUtils::ContentIsDescendantOf(caretContent, popupContent)) {
      
      
      return PR_FALSE;
    }

    if (popupFrame->PopupType() == ePopupTypeMenu && !popupFrame->IsContextMenu()) {
      
      
      
      return PR_TRUE;
    }
  }
#endif

  
  return PR_FALSE;
}







void nsCaret::DrawCaret(PRBool aInvalidate)
{
  
  if (!MustDrawCaret(PR_FALSE))
    return;
  
  nsCOMPtr<nsIDOMNode> node;
  PRInt32 offset;
  nsFrameSelection::HINT hint;
  PRUint8 bidiLevel;

  if (!mDrawn)
  {
    nsCOMPtr<nsISelection> domSelection = do_QueryReferent(mDomSelectionWeak);
    nsCOMPtr<nsISelectionPrivate> privateSelection(do_QueryInterface(domSelection));
    if (!privateSelection) return;
    
    PRBool isCollapsed = PR_FALSE;
    domSelection->GetIsCollapsed(&isCollapsed);
    if (!mShowDuringSelection && !isCollapsed)
      return;

    PRBool hintRight;
    privateSelection->GetInterlinePosition(&hintRight);
    hint = hintRight ? nsFrameSelection::HINTRIGHT : nsFrameSelection::HINTLEFT;

    
    domSelection->GetFocusNode(getter_AddRefs(node));
    if (!node)
      return;
    
    if (NS_FAILED(domSelection->GetFocusOffset(&offset)))
      return;

    nsCOMPtr<nsFrameSelection> frameSelection = GetFrameSelection();
    if (!frameSelection)
      return;
    bidiLevel = frameSelection->GetCaretBidiLevel();
  }
  else
  {
    if (!mLastContent)
    {
      mDrawn = PR_FALSE;
      return;
    }
    if (!mLastContent->IsInDoc())
    {
      mLastContent = nsnull;
      mDrawn = PR_FALSE;
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

static PRBool
FramesOnSameLineHaveZeroHeight(nsIFrame* aFrame)
{
  nsLineBox* line = FindContainingLine(aFrame);
  if (!line)
    return aFrame->GetRect().height == 0;
  PRInt32 count = line->GetChildCount();
  for (nsIFrame* f = line->mFirstChild; count > 0; --count, f = f->GetNextSibling())
  {
   if (f->GetRect().height != 0)
     return PR_FALSE;
  }
  return PR_TRUE;
}

nsresult nsCaret::UpdateCaretRects(nsIFrame* aFrame, PRInt32 aFrameOffset)
{
  NS_ASSERTION(aFrame, "Should have a frame here");

  nsRect frameRect = aFrame->GetRect();
  frameRect.x = 0;
  frameRect.y = 0;

  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) return NS_ERROR_FAILURE;

  
  
  if (frameRect.height == 0)
  {
    nsCOMPtr<nsIFontMetrics> fm;
    nsLayoutUtils::GetFontMetricsForFrame(aFrame, getter_AddRefs(fm));

    if (fm)
    {
      nscoord ascent, descent;
      fm->GetMaxAscent(ascent);
      fm->GetMaxDescent(descent);
      frameRect.height = ascent + descent;

      
      
      
      if (aFrame->GetStyleDisplay()->IsInlineOutside() &&
          !FramesOnSameLineHaveZeroHeight(aFrame))
        frameRect.y -= ascent;
    }
  }

  mCaretRect = frameRect;
  nsCOMPtr<nsISelection> domSelection = do_QueryReferent(mDomSelectionWeak);
  nsCOMPtr<nsISelectionPrivate> privateSelection = do_QueryInterface(domSelection);

  nsPoint framePos;

  
  nsresult rv = privateSelection->GetCachedFrameOffset(aFrame, aFrameOffset,
                                                       framePos);
  if (NS_FAILED(rv))
  {
    mCaretRect.Empty();
    return rv;
  }

  mCaretRect += framePos;
  Metrics metrics = ComputeMetrics(aFrame, aFrameOffset, mCaretRect.height);
  mCaretRect.width = metrics.mCaretWidth;

  
  
  nsIFrame *scrollFrame =
    nsLayoutUtils::GetClosestFrameOfType(aFrame, nsGkAtoms::scrollFrame);
  if (scrollFrame)
  {
    
    nsIScrollableFrame *scrollable = do_QueryFrame(scrollFrame);
    nsIScrollableView *scrollView = scrollable->GetScrollableView();
    nsIView *view;
    scrollView->GetScrolledView(view);

    
    
    
    
    nsPoint toScroll = aFrame->GetOffsetTo(scrollFrame) -
      view->GetOffsetTo(scrollFrame->GetView());
    nsRect caretInScroll = mCaretRect + toScroll;

    
    
    nscoord overflow = caretInScroll.XMost() - view->GetBounds().width;
    if (overflow > 0)
      mCaretRect.x -= overflow;
  }

  
  const nsStyleVisibility* vis = aFrame->GetStyleVisibility();
  if (NS_STYLE_DIRECTION_RTL == vis->mDirection)
    mCaretRect.x -= mCaretRect.width;

  return UpdateHookRect(presShell->GetPresContext(), metrics);
}

nsresult nsCaret::UpdateHookRect(nsPresContext* aPresContext,
                                 const Metrics& aMetrics)
{
  mHookRect.Empty();

#ifdef IBMBIDI
  
  PRBool isCaretRTL=PR_FALSE;
  nsIBidiKeyboard* bidiKeyboard = nsContentUtils::GetBidiKeyboard();
  if (!bidiKeyboard || NS_FAILED(bidiKeyboard->IsLangRTL(&isCaretRTL)))
    
    
    
    return NS_OK;
  if (mBidiUI)
  {
    if (isCaretRTL != mKeyboardRTL)
    {
      






 
      mKeyboardRTL = isCaretRTL;
      nsCOMPtr<nsISelection> domSelection = do_QueryReferent(mDomSelectionWeak);
      if (domSelection)
      {
        if (NS_SUCCEEDED(domSelection->SelectionLanguageChange(mKeyboardRTL)))
        {
          return NS_ERROR_FAILURE;
        }
      }
    }
    
    
    
    nscoord bidiIndicatorSize = aMetrics.mBidiIndicatorSize;
    mHookRect.SetRect(mCaretRect.x + ((isCaretRTL) ?
                      bidiIndicatorSize * -1 :
                      mCaretRect.width),
                      mCaretRect.y + bidiIndicatorSize,
                      bidiIndicatorSize,
                      mCaretRect.width);
  }
#endif 

  return NS_OK;
}


void nsCaret::InvalidateRects(const nsRect &aRect, const nsRect &aHook,
                              nsIFrame *aFrame)
{
  NS_ASSERTION(aFrame, "Must have a frame to invalidate");
  nsRect rect;
  rect.UnionRect(aRect, aHook);
  aFrame->Invalidate(rect);
}



void nsCaret::CaretBlinkCallback(nsITimer *aTimer, void *aClosure)
{
  nsCaret   *theCaret = reinterpret_cast<nsCaret*>(aClosure);
  if (!theCaret) return;
  
  theCaret->DrawCaret(PR_TRUE);
}



already_AddRefed<nsFrameSelection>
nsCaret::GetFrameSelection()
{
  nsCOMPtr<nsISelectionPrivate> privateSelection(do_QueryReferent(mDomSelectionWeak));
  if (!privateSelection)
    return nsnull;
  nsFrameSelection* frameSelection = nsnull;
  privateSelection->GetFrameSelection(&frameSelection);
  return frameSelection;
}

void
nsCaret::SetIgnoreUserModify(PRBool aIgnoreUserModify)
{
  if (!aIgnoreUserModify && mIgnoreUserModify && mDrawn) {
    
    
    
    
    nsIFrame *frame = GetCaretFrame();
    if (frame) {
      const nsStyleUserInterface* userinterface = frame->GetStyleUserInterface();
      if (userinterface->mUserModify == NS_STYLE_USER_MODIFY_READ_ONLY) {
        StopBlinking();
      }
    }
  }
  mIgnoreUserModify = aIgnoreUserModify;
}


nsresult NS_NewCaret(nsCaret** aInstancePtrResult)
{
  NS_PRECONDITION(aInstancePtrResult, "null ptr");
  
  nsCaret* caret = new nsCaret();
  if (nsnull == caret)
      return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(caret);
  *aInstancePtrResult = caret;
  return NS_OK;
}

