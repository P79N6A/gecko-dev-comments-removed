









































#include "nsCOMPtr.h"

#include "nsITimer.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsFrameSelection.h"
#include "nsIFrame.h"
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
#include "nsIViewManager.h"
#include "nsPresContext.h"
#include "nsILookAndFeel.h"
#include "nsBlockFrame.h"
#include "nsISelectionController.h"
#include "nsDisplayList.h"
#include "nsCaret.h"




static const PRUint32 kMinBidiIndicatorPixels = 2;

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
, mLastContentOffset(0)
, mLastHint(nsFrameSelection::HINTLEFT)
#ifdef IBMBIDI
, mLastBidiLevel(0)
, mKeyboardRTL(PR_FALSE)
#endif
{
}


nsCaret::~nsCaret()
{
  KillTimer();
}


NS_IMETHODIMP nsCaret::Init(nsIPresShell *inPresShell)
{
  NS_ENSURE_ARG(inPresShell);
  
  mPresShell = do_GetWeakReference(inPresShell);    
  NS_ASSERTION(mPresShell, "Hey, pres shell should support weak refs");

  
  nsILookAndFeel *lookAndFeel = nsnull;
  nsPresContext *presContext = inPresShell->GetPresContext();
  
  PRInt32 caretPixelsWidth = 1;
  if (presContext && (lookAndFeel = presContext->LookAndFeel())) {
    PRInt32 tempInt;
    if (NS_SUCCEEDED(lookAndFeel->GetMetric(nsILookAndFeel::eMetric_CaretWidth, tempInt)))
      caretPixelsWidth = (nscoord)tempInt;
    if (NS_SUCCEEDED(lookAndFeel->GetMetric(nsILookAndFeel::eMetric_CaretBlinkTime, tempInt)))
      mBlinkRate = (PRUint32)tempInt;
    if (NS_SUCCEEDED(lookAndFeel->GetMetric(nsILookAndFeel::eMetric_ShowCaretDuringSelection, tempInt)))
      mShowDuringSelection = tempInt ? PR_TRUE : PR_FALSE;
  }
  
  mCaretWidth = presContext->DevPixelsToAppUnits(caretPixelsWidth);
  mBidiIndicatorSize = presContext->DevPixelsToAppUnits(kMinBidiIndicatorPixels);
  if (mBidiIndicatorSize < mCaretWidth) {
    mBidiIndicatorSize = mCaretWidth;
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
    rv = StartBlinking();
    if (NS_FAILED(rv))
      return rv;
  }

  return NS_OK;
}



NS_IMETHODIMP nsCaret::Terminate()
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
  
  return NS_OK;
}



NS_IMPL_ISUPPORTS2(nsCaret, nsICaret, nsISelectionListener)


NS_IMETHODIMP nsCaret::GetCaretDOMSelection(nsISelection **aDOMSel)
{
  nsCOMPtr<nsISelection> sel(do_QueryReferent(mDomSelectionWeak));
  
  NS_IF_ADDREF(*aDOMSel = sel);

  return NS_OK;
}



NS_IMETHODIMP nsCaret::SetCaretDOMSelection(nsISelection *aDOMSel)
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



NS_IMETHODIMP nsCaret::SetCaretVisible(PRBool inMakeVisible)
{
  mVisible = inMakeVisible;
  nsresult  err = NS_OK;
  if (mVisible)
    err = StartBlinking();
  else
    err = StopBlinking();
    
  return err;
}



NS_IMETHODIMP nsCaret::GetCaretVisible(PRBool *outMakeVisible)
{
  NS_ENSURE_ARG_POINTER(outMakeVisible);
  *outMakeVisible = mVisible;
  return NS_OK;
}



NS_IMETHODIMP nsCaret::SetCaretReadOnly(PRBool inMakeReadonly)
{
  mReadOnly = inMakeReadonly;
  return NS_OK;
}



NS_IMETHODIMP nsCaret::GetCaretCoordinates(EViewCoordinates aRelativeToType,
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

  nsFrameSelection* frameSelection = GetFrameSelection();
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
  
  
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell)
    return NS_ERROR_FAILURE;
  nsPresContext *presContext = presShell->GetPresContext();

  
  nsCOMPtr<nsIRenderingContext> rendContext;  
  err = presContext->DeviceContext()->
    CreateRenderingContext(drawingView, *getter_AddRefs(rendContext));
  if (NS_FAILED(err))
    return err;
  if (!rendContext)
    return NS_ERROR_UNEXPECTED;

  
  nsPoint   framePos(0, 0);
  err = theFrame->GetPointFromOffset(presContext, rendContext, theFrameOffset,
                                     &framePos);
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
  outCoordinates->width  = mCaretWidth;
  
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

NS_IMETHODIMP nsCaret::EraseCaret()
{
  if (mDrawn) {
    DrawCaret(PR_TRUE);
    if (mReadOnly) {
      
      
      
      DrawCaretAfterBriefDelay();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsCaret::SetVisibilityDuringSelection(PRBool aVisibility) 
{
  mShowDuringSelection = aVisibility;
  return NS_OK;
}

NS_IMETHODIMP nsCaret::DrawAtPosition(nsIDOMNode* aNode, PRInt32 aOffset)
{
  NS_ENSURE_ARG(aNode);

  PRUint8 bidiLevel;
  nsFrameSelection* frameSelection = GetFrameSelection();
  if (!frameSelection)
    return NS_ERROR_FAILURE;
  bidiLevel = frameSelection->GetCaretBidiLevel();
  
  
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
                         const nsPoint &aOffset,
                         nscolor aColor)
{
  NS_ASSERTION(mDrawn, "The caret shouldn't be drawing");

  aCtx->SetColor(aColor);
  aCtx->FillRect(mCaretRect + aOffset);
  if (!GetHookRect().IsEmpty())
    aCtx->FillRect(GetHookRect() + aOffset);
}

#ifdef XP_MAC
#pragma mark -
#endif


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

#ifdef XP_MAC
#pragma mark -
#endif


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



nsresult nsCaret::StartBlinking()
{
  if (mReadOnly) {
    
    
    DrawCaretAfterBriefDelay();
    return NS_OK;
  }
  PrimeTimer();

  
  
  
  
  
  if (mDrawn)
    DrawCaret(PR_TRUE);

  DrawCaret(PR_TRUE);    
  
  return NS_OK;
}



nsresult nsCaret::StopBlinking()
{
  if (mDrawn)     
    DrawCaret(PR_TRUE);

  NS_ASSERTION(!mDrawn, "We just erased ourselves");
  KillTimer();

  return NS_OK;
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
  if (
#ifdef SUPPORT_USER_MODIFY
        
      (userinterface->mUserModify == NS_STYLE_USER_MODIFY_READ_ONLY) ||
#endif          
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
      nsFrameSelection* frameSelection = GetFrameSelection();
      if (!frameSelection)
        return NS_ERROR_FAILURE;
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

NS_IMETHODIMP 
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

  nsFrameSelection* frameSelection = GetFrameSelection();
  if (!frameSelection)
    return NS_ERROR_FAILURE;

  nsIFrame* theFrame = nsnull;
  PRInt32   theFrameOffset = 0;

  theFrame = frameSelection->GetFrameForNodeOffset(aContentNode, aOffset,
                                                   aFrameHint, &theFrameOffset);
  if (!theFrame)
    return NS_ERROR_FAILURE;

  
  
  
  
  
  
  
  
  nsPresContext *presContext = presShell->GetPresContext();
  if (presContext && presContext->BidiEnabled())
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
          aBidiLevel = PR_MAX(aBidiLevel, PR_MIN(levelBefore, levelAfter));                                  
          aBidiLevel = PR_MIN(aBidiLevel, PR_MAX(levelBefore, levelAfter));                                  
          if (aBidiLevel == levelBefore                                                                      
              || aBidiLevel > levelBefore && aBidiLevel < levelAfter && !((aBidiLevel ^ levelBefore) & 1)    
              || aBidiLevel < levelBefore && aBidiLevel > levelAfter && !((aBidiLevel ^ levelBefore) & 1))   
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
                   || aBidiLevel > levelBefore && aBidiLevel < levelAfter && !((aBidiLevel ^ levelAfter) & 1)   
                   || aBidiLevel < levelBefore && aBidiLevel > levelAfter && !((aBidiLevel ^ levelAfter) & 1))  
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

  
  
  
  
  if (coordType == eIMECoordinates) {
#if defined(XP_MAC) || defined(XP_MACOSX) || defined(XP_WIN)
   
   
   coordType = eTopLevelWindowCoordinates; 
#else
   
   
   coordType = eRenderingViewCoordinates; 
#endif
  }

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
        nsIViewManager* vm = presShell->GetViewManager();
        if (vm) {
          vm->GetRootView(*outRelativeView);
        }
      }
    }
  }

  *outRenderingView = returnView;
}












PRBool nsCaret::MustDrawCaret()
{
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (presShell) {
    PRBool isPaintingSuppressed;
    presShell->IsPaintingSuppressed(&isPaintingSuppressed);
    if (isPaintingSuppressed)
      return PR_FALSE;
  }

  if (mDrawn)
    return PR_TRUE;

  nsCOMPtr<nsISelection> domSelection = do_QueryReferent(mDomSelectionWeak);
  if (!domSelection)
    return PR_FALSE;
  PRBool isCollapsed;

  if (NS_FAILED(domSelection->GetIsCollapsed(&isCollapsed)))
    return PR_FALSE;

  if (mShowDuringSelection)
    return PR_TRUE;      

  return isCollapsed;
}








void nsCaret::DrawCaret(PRBool aInvalidate)
{
  
  if (!MustDrawCaret())
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

    nsFrameSelection* frameSelection = GetFrameSelection();
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

nsresult nsCaret::UpdateCaretRects(nsIFrame* aFrame, PRInt32 aFrameOffset)
{
  NS_ASSERTION(aFrame, "Should have a frame here");

  nsRect frameRect = aFrame->GetRect();
  frameRect.x = 0;
  frameRect.y = 0;

  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) return NS_ERROR_FAILURE;

  nsPresContext *presContext = presShell->GetPresContext();

  
  
  
  if (frameRect.height == 0)
  {
    nsIWidget *widget = aFrame->GetWindow();
    if (!widget)
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsIRenderingContext> rendContext;
    nsresult rv = presContext->DeviceContext()->
      CreateRenderingContext(widget, *getter_AddRefs(rendContext));
    NS_ENSURE_SUCCESS(rv, rv);
    if (!rendContext)
      return NS_ERROR_UNEXPECTED;

    const nsStyleFont* fontStyle = aFrame->GetStyleFont();
    const nsStyleVisibility* vis = aFrame->GetStyleVisibility();
    rendContext->SetFont(fontStyle->mFont, vis->mLangGroup);

    nsCOMPtr<nsIFontMetrics> fm;
    rendContext->GetFontMetrics(*getter_AddRefs(fm));
    if (fm)
    {
      nscoord ascent, descent;
      fm->GetMaxAscent(ascent);
      fm->GetMaxDescent(descent);
      frameRect.height = ascent + descent;
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
  mCaretRect.width = mCaretWidth;

  
  const nsStyleVisibility* vis = aFrame->GetStyleVisibility();
  if (NS_STYLE_DIRECTION_RTL == vis->mDirection)
    mCaretRect.x -= mCaretRect.width;

  return UpdateHookRect(presContext);
}

nsresult nsCaret::UpdateHookRect(nsPresContext* aPresContext)
{
  mHookRect.Empty();

#ifdef IBMBIDI
  
  PRBool bidiEnabled;
  PRBool isCaretRTL=PR_FALSE;
  nsIBidiKeyboard* bidiKeyboard = nsContentUtils::GetBidiKeyboard();
  if (!bidiKeyboard || NS_FAILED(bidiKeyboard->IsLangRTL(&isCaretRTL)))
    
    
    
    return NS_OK;
  if (isCaretRTL)
  {
    bidiEnabled = PR_TRUE;
    aPresContext->SetBidiEnabled(bidiEnabled);
  }
  else
    bidiEnabled = aPresContext->BidiEnabled();
  if (bidiEnabled)
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
    
    
    
    mHookRect.SetRect(mCaretRect.x + ((isCaretRTL) ?
                      mBidiIndicatorSize * -1 :
                      mCaretRect.width),
                      mCaretRect.y + mBidiIndicatorSize,
                      mBidiIndicatorSize,
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
  aFrame->Invalidate(rect, PR_FALSE);
}

#ifdef XP_MAC
#pragma mark -
#endif



void nsCaret::CaretBlinkCallback(nsITimer *aTimer, void *aClosure)
{
  nsCaret   *theCaret = NS_REINTERPRET_CAST(nsCaret*, aClosure);
  if (!theCaret) return;
  
  theCaret->DrawCaret(PR_TRUE);
}



nsFrameSelection* nsCaret::GetFrameSelection() {
  nsCOMPtr<nsISelectionPrivate> privateSelection(do_QueryReferent(mDomSelectionWeak));
  if (!privateSelection)
    return nsnull;
  nsCOMPtr<nsFrameSelection> frameSelection;
  privateSelection->GetFrameSelection(getter_AddRefs(frameSelection));
  return frameSelection;
}



nsresult NS_NewCaret(nsICaret** aInstancePtrResult)
{
  NS_PRECONDITION(aInstancePtrResult, "null ptr");
  
  nsCaret* caret = new nsCaret();
  if (nsnull == caret)
      return NS_ERROR_OUT_OF_MEMORY;
      
  return caret->QueryInterface(NS_GET_IID(nsICaret), (void**) aInstancePtrResult);
}

