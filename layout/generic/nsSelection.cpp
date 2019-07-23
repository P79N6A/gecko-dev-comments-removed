










































#include "nsCOMPtr.h"
#include "nsWeakReference.h"
#include "nsIFactory.h"
#include "nsIEnumerator.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsIDOMRange.h"
#include "nsFrameSelection.h"
#include "nsISelection.h"
#include "nsISelection2.h"
#include "nsISelectionPrivate.h"
#include "nsISelectionListener.h"
#include "nsIComponentManager.h"
#include "nsContentCID.h"
#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsIDOMNode.h"
#include "nsRange.h"
#include "nsCOMArray.h"
#include "nsGUIEvent.h"
#include "nsIDOMKeyEvent.h"
#include "nsITableLayout.h"
#include "nsITableCellLayout.h"
#include "nsIDOMNodeList.h"
#include "nsTArray.h"

#include "nsISelectionListener.h"
#include "nsIContentIterator.h"
#include "nsIDocumentEncoder.h"


#include "nsFrameTraversal.h"
#include "nsILineIterator.h"
#include "nsGkAtoms.h"
#include "nsIFrameTraversal.h"
#include "nsLayoutUtils.h"
#include "nsLayoutCID.h"
#include "nsBidiPresUtils.h"
static NS_DEFINE_CID(kFrameTraversalCID, NS_FRAMETRAVERSAL_CID);

#include "nsIDOMText.h"

#include "nsContentUtils.h"
#include "nsThreadUtils.h"


#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsICaret.h"



#include "nsIViewManager.h"
#include "nsIScrollableView.h"
#include "nsIDeviceContext.h"
#include "nsITimer.h"
#include "nsIServiceManager.h"
#include "nsFrameManager.h"

#include "nsIDOMDocument.h"
#include "nsIDocument.h"

#include "nsISelectionController.h"
#include "nsAutoCopyListener.h"
#include "nsCopySupport.h"
#include "nsIClipboard.h"

#ifdef IBMBIDI
#include "nsIBidiKeyboard.h"
#endif 

#define STATUS_CHECK_RETURN_MACRO() {if (!mShell) return NS_ERROR_FAILURE;}





static NS_DEFINE_IID(kCContentIteratorCID, NS_CONTENTITERATOR_CID);
static NS_DEFINE_IID(kCSubtreeIteratorCID, NS_SUBTREEITERATOR_CID);

#undef OLD_SELECTION
#undef OLD_TABLE_SELECTION



class nsSelectionIterator;
class nsFrameSelection;
class nsAutoScrollTimer;

PRBool  IsValidSelectionPoint(nsFrameSelection *aFrameSel, nsIContent *aContent);
PRBool  IsValidSelectionPoint(nsFrameSelection *aFrameSel, nsIDOMNode *aDomNode);

static nsIAtom *GetTag(nsIDOMNode *aNode);
static nsresult ParentOffset(nsIDOMNode *aNode, nsIDOMNode **aParent, PRInt32 *aChildOffset);
static nsIDOMNode *GetCellParent(nsIDOMNode *aDomNode);


#ifdef PRINT_RANGE
static void printRange(nsIDOMRange *aDomRange);
#define DEBUG_OUT_RANGE(x)  printRange(x)
#else
#define DEBUG_OUT_RANGE(x)  
#endif 









static PRInt32
CompareDOMPoints(nsIDOMNode* aParent1, PRInt32 aOffset1,
                 nsIDOMNode* aParent2, PRInt32 aOffset2)
{
  nsCOMPtr<nsINode> parent1 = do_QueryInterface(aParent1);
  nsCOMPtr<nsINode> parent2 = do_QueryInterface(aParent2);

  NS_ASSERTION(parent1 && parent2, "not real nodes?");

  return nsContentUtils::ComparePoints(parent1, aOffset1, parent2, aOffset2);
}

struct CachedOffsetForFrame {
  CachedOffsetForFrame()
  : mCachedFrameOffset(0, 0) 
  , mLastCaretFrame(nsnull)
  , mLastContentOffset(0)
  , mCanCacheFrameOffset(PR_FALSE)
  {}

  nsPoint      mCachedFrameOffset;      
  nsIFrame*    mLastCaretFrame;         
  PRInt32      mLastContentOffset;      
  PRPackedBool mCanCacheFrameOffset;    
};

struct RangeData
{
  RangeData(nsIDOMRange* aRange, PRInt32 aEndIndex) :
    mRange(aRange), mEndIndex(aEndIndex) {}

  nsCOMPtr<nsIDOMRange> mRange;
  PRInt32 mEndIndex; 
};

class nsTypedSelection : public nsISelection2,
                         public nsISelectionPrivate,
                         public nsSupportsWeakReference
{
public:
  nsTypedSelection();
  nsTypedSelection(nsFrameSelection *aList);
  virtual ~nsTypedSelection();
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSISELECTION
  NS_DECL_NSISELECTION2
  NS_DECL_NSISELECTIONPRIVATE

  
  nsresult      GetPresContext(nsPresContext **aPresContext);
  nsresult      GetPresShell(nsIPresShell **aPresShell);
  nsresult      GetRootScrollableView(nsIScrollableView **aScrollableView);
  nsresult      GetFrameToScrolledViewOffsets(nsIScrollableView *aScrollableView, nsIFrame *aFrame, nscoord *aXOffset, nscoord *aYOffset);
  nsresult      GetPointFromOffset(nsIFrame *aFrame, PRInt32 aContentOffset, nsPoint *aPoint);
  nsresult      GetSelectionRegionRectAndScrollableView(SelectionRegion aRegion, nsRect *aRect, nsIScrollableView **aScrollableView);
  nsresult      ScrollRectIntoView(nsIScrollableView *aScrollableView, nsRect& aRect, PRIntn  aVPercent, PRIntn  aHPercent, PRBool aScrollParentViews);

  nsresult      PostScrollSelectionIntoViewEvent(SelectionRegion aRegion);
  NS_IMETHOD    ScrollIntoView(SelectionRegion aRegion=nsISelectionController::SELECTION_FOCUS_REGION, PRBool aIsSynchronous=PR_TRUE);
  nsresult      AddItem(nsIDOMRange *aRange);
  nsresult      RemoveItem(nsIDOMRange *aRange);
  nsresult      Clear(nsPresContext* aPresContext);

  
  nsIDOMNode*  FetchAnchorNode();  
  PRInt32      FetchAnchorOffset();

  nsIDOMNode*  FetchOriginalAnchorNode();  
  PRInt32      FetchOriginalAnchorOffset();

  nsIDOMNode*  FetchFocusNode();   
  PRInt32      FetchFocusOffset();

  nsIDOMNode*  FetchStartParent(nsIDOMRange *aRange);   
  PRInt32      FetchStartOffset(nsIDOMRange *aRange);
  nsIDOMNode*  FetchEndParent(nsIDOMRange *aRange);     
  PRInt32      FetchEndOffset(nsIDOMRange *aRange);

  nsDirection  GetDirection(){return mDirection;}
  void         SetDirection(nsDirection aDir){mDirection = aDir;}
  PRBool       GetTrueDirection() {return mTrueDirection;}
  void         SetTrueDirection(PRBool aBool){mTrueDirection = aBool;}
  NS_IMETHOD   CopyRangeToAnchorFocus(nsIDOMRange *aRange);
  


  NS_IMETHOD   GetPrimaryFrameForAnchorNode(nsIFrame **aResultFrame);
  NS_IMETHOD   GetPrimaryFrameForFocusNode(nsIFrame **aResultFrame, PRInt32 *aOffset, PRBool aVisual);
  NS_IMETHOD   SetOriginalAnchorPoint(nsIDOMNode *aNode, PRInt32 aOffset);
  NS_IMETHOD   GetOriginalAnchorPoint(nsIDOMNode **aNode, PRInt32 *aOffset);
  NS_IMETHOD   LookUpSelection(nsIContent *aContent, PRInt32 aContentOffset, PRInt32 aContentLength,
                             SelectionDetails **aReturnDetails, SelectionType aType, PRBool aSlowCheck);
  NS_IMETHOD   Repaint(nsPresContext* aPresContext);

  nsresult     StartAutoScrollTimer(nsPresContext *aPresContext,
                                    nsIView *aView,
                                    nsPoint& aPoint,
                                    PRUint32 aDelay);

  nsresult     StopAutoScrollTimer();


private:
  friend class nsAutoScrollTimer;

  nsresult DoAutoScrollView(nsPresContext *aPresContext,
                            nsIView *aView,
                            nsPoint& aPoint,
                            PRBool aScrollParentViews);

  nsresult     ScrollPointIntoClipView(nsPresContext *aPresContext, nsIView *aView, nsPoint& aPoint, PRBool *aDidScroll);
  nsresult     ScrollPointIntoView(nsPresContext *aPresContext, nsIView *aView, nsPoint& aPoint, PRBool aScrollParentViews, PRBool *aDidScroll);
  nsresult     GetViewAncestorOffset(nsIView *aView, nsIView *aAncestorView, nscoord *aXOffset, nscoord *aYOffset);

public:
  SelectionType GetType(){return mType;}
  void          SetType(SelectionType aType){mType = aType;}

  nsresult     NotifySelectionListeners();

  void DetachFromPresentation();

private:
  friend class nsSelectionIterator;

  class ScrollSelectionIntoViewEvent;
  friend class ScrollSelectionIntoViewEvent;

  class ScrollSelectionIntoViewEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    ScrollSelectionIntoViewEvent(nsTypedSelection *aTypedSelection,
                                 SelectionRegion aRegion) 
      : mTypedSelection(aTypedSelection),
        mRegion(aRegion) {
      NS_ASSERTION(aTypedSelection, "null parameter");
    }
    void Revoke() { mTypedSelection = nsnull; }
  private:
    nsTypedSelection *mTypedSelection;
    SelectionRegion   mRegion;
  };

  void         setAnchorFocusRange(PRInt32 aIndex); 
  NS_IMETHOD   selectFrames(nsPresContext* aPresContext, nsIContentIterator *aInnerIter, nsIContent *aContent, nsIDOMRange *aRange, nsIPresShell *aPresShell, PRBool aFlags);
  NS_IMETHOD   selectFrames(nsPresContext* aPresContext, nsIDOMRange *aRange, PRBool aSelect);
  nsresult     getTableCellLocationFromRange(nsIDOMRange *aRange, PRInt32 *aSelectionType, PRInt32 *aRow, PRInt32 *aCol);
  nsresult     addTableCellRange(nsIDOMRange *aRange, PRBool *aDidAddRange);
  
#ifdef OLD_SELECTION
  NS_IMETHOD   FixupSelectionPoints(nsIDOMRange *aRange, nsDirection *aDir, PRBool *aFixupState);
#endif 

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsTArray<RangeData> mRanges;
  nsTArray<PRInt32> mRangeEndings;    
#ifdef DEBUG
  PRBool ValidateRanges();
#endif

  nsresult FindInsertionPoint(
      const nsTArray<PRInt32>* aRemappingArray,
      nsIDOMNode* aPointNode, PRInt32 aPointOffset,
      nsresult (*aComparator)(nsIDOMNode*,PRInt32,nsIDOMRange*,PRInt32*),
      PRInt32* aInsertionPoint);
  nsresult MoveIndexToFirstMatch(PRInt32* aIndex, nsIDOMNode* aNode,
                                 PRInt32 aOffset,
                                 const nsTArray<PRInt32>* aArray,
                                 PRBool aUseBeginning);
  nsresult MoveIndexToNextMismatch(PRInt32* aIndex, nsIDOMNode* aNode,
                                   PRInt32 aOffset,
                                   const nsTArray<PRInt32>* aRemappingArray,
                                   PRBool aUseBeginning);
  PRBool FindRangeGivenPoint(nsIDOMNode* aBeginNode, PRInt32 aBeginOffset,
                             nsIDOMNode* aEndNode, PRInt32 aEndOffset,
                             PRInt32 aStartSearchingHere);

  nsCOMPtr<nsIDOMRange> mAnchorFocusRange;
  nsCOMPtr<nsIDOMRange> mOriginalAnchorRange; 
  nsDirection mDirection; 
  PRBool mFixupState; 

  nsFrameSelection *mFrameSelection;
  nsWeakPtr mPresShellWeak; 
  SelectionType mType;
  nsRefPtr<nsAutoScrollTimer> mAutoScrollTimer; 
  nsCOMArray<nsISelectionListener> mSelectionListeners;
  PRPackedBool mTrueDirection;
  nsRevocableEventPtr<ScrollSelectionIntoViewEvent> mScrollEvent;
  CachedOffsetForFrame *mCachedOffsetForFrame;
};


class nsSelectionBatcher
{
private:
  nsCOMPtr<nsISelectionPrivate> mSelection;
public:
  nsSelectionBatcher(nsISelectionPrivate *aSelection) : mSelection(aSelection)
  {
    if (mSelection) mSelection->StartBatchChanges();
  }
  virtual ~nsSelectionBatcher() 
  { 
    if (mSelection) mSelection->EndBatchChanges();
  }
};

class nsSelectionIterator : public nsIBidirectionalEnumerator
{
public:



  NS_DECL_ISUPPORTS

  NS_DECL_NSIENUMERATOR

  NS_DECL_NSIBIDIRECTIONALENUMERATOR



  NS_IMETHOD CurrentItem(nsIDOMRange **aRange);

private:
  friend class nsTypedSelection;

  
  friend class nsFrameSelection;

  nsSelectionIterator(nsTypedSelection *);
  virtual ~nsSelectionIterator();
  PRInt32     mIndex;
  nsTypedSelection *mDomSelection;
  SelectionType mType;
};

class nsAutoScrollTimer : public nsITimerCallback
{
public:

  NS_DECL_ISUPPORTS

  nsAutoScrollTimer()
  : mFrameSelection(0), mSelection(0), mPresContext(0), mPoint(0,0), mDelay(30)
  {
  }

  virtual ~nsAutoScrollTimer()
  {
   if (mTimer)
       mTimer->Cancel();
  }

  nsresult Start(nsPresContext *aPresContext, nsIView *aView, nsPoint &aPoint)
  {
    mPoint = aPoint;

    
    
    mPresContext = aPresContext;

    
    
    nsIFrame* clientFrame = NS_STATIC_CAST(nsIFrame*, aView->GetClientData());
    NS_ASSERTION(clientFrame, "Missing client frame");

    nsIFrame* capturingFrame = nsFrame::GetNearestCapturingFrame(clientFrame);
    NS_ASSERTION(!capturingFrame || capturingFrame->GetMouseCapturer(),
                 "Capturing frame should have a mouse capturer" );

    NS_ASSERTION(!capturingFrame || mPresContext == capturingFrame->GetPresContext(),
                 "Shouldn't have different pres contexts");

    NS_ASSERTION(capturingFrame != mPresContext->PresShell()->FrameManager()->GetRootFrame(),
                 "Capturing frame should not be the root frame");

    if (capturingFrame)
    {
      mContent = capturingFrame->GetContent();
      NS_ASSERTION(mContent, "Need content");

      NS_ASSERTION(mContent != mPresContext->PresShell()->FrameManager()->GetRootFrame()->GetContent(),
                 "We didn't want the root content!");

      NS_ASSERTION(capturingFrame == nsFrame::GetNearestCapturingFrame(
                   mPresContext->PresShell()->GetPrimaryFrameFor(mContent)),
                   "Mapping of frame to content failed.");
    }

    
    NS_ASSERTION(capturingFrame || !mContent, "Content not cleared correctly.");

    if (!mTimer)
    {
      nsresult result;
      mTimer = do_CreateInstance("@mozilla.org/timer;1", &result);

      if (NS_FAILED(result))
        return result;
    }

    return mTimer->InitWithCallback(this, mDelay, nsITimer::TYPE_ONE_SHOT);
  }

  nsresult Stop()
  {
    if (mTimer)
    {
      mTimer->Cancel();
      mTimer = 0;
    }

    mContent = nsnull;
    return NS_OK;
  }

  nsresult Init(nsFrameSelection *aFrameSelection, nsTypedSelection *aSelection)
  {
    mFrameSelection = aFrameSelection;
    mSelection = aSelection;
    return NS_OK;
  }

  nsresult SetDelay(PRUint32 aDelay)
  {
    mDelay = aDelay;
    return NS_OK;
  }

  NS_IMETHOD Notify(nsITimer *timer)
  {
    if (mSelection && mPresContext)
    {
      
      nsIFrame* capturingFrame;
      if (mContent)
      {
        nsIFrame* contentFrame = mPresContext->PresShell()->GetPrimaryFrameFor(mContent);
        if (contentFrame)
        {
          capturingFrame = nsFrame::GetNearestCapturingFrame(contentFrame);
        }
        else 
        {
          capturingFrame = nsnull;
        }
        NS_ASSERTION(!capturingFrame || capturingFrame->GetMouseCapturer(),
                     "Capturing frame should have a mouse capturer" );
      }
      else
      {
        capturingFrame = mPresContext->PresShell()->FrameManager()->GetRootFrame();
      }

      
      mContent = nsnull;

      
      
      if (!capturingFrame) {
        NS_WARNING("Frame destroyed or set to display:none before scroll timer fired.");
        return NS_OK;
      }

      nsIView* captureView = capturingFrame->GetMouseCapturer();
    
      nsIFrame* viewFrame = NS_STATIC_CAST(nsIFrame*, captureView->GetClientData());
      NS_ASSERTION(viewFrame, "View must have a client frame");
      
      mFrameSelection->HandleDrag(viewFrame, mPoint);

      mSelection->DoAutoScrollView(mPresContext, captureView, mPoint, PR_TRUE);
    }
    return NS_OK;
  }
private:
  nsFrameSelection *mFrameSelection;
  nsTypedSelection *mSelection;
  nsPresContext *mPresContext;
  nsPoint mPoint;
  nsCOMPtr<nsITimer> mTimer;
  nsCOMPtr<nsIContent> mContent;
  PRUint32 mDelay;
};

NS_IMPL_ADDREF(nsAutoScrollTimer)
NS_IMPL_RELEASE(nsAutoScrollTimer)
NS_IMPL_QUERY_INTERFACE1(nsAutoScrollTimer, nsITimerCallback)

nsresult NS_NewSelection(nsFrameSelection **aFrameSelection);

nsresult NS_NewSelection(nsFrameSelection **aFrameSelection)
{
  nsFrameSelection *rlist = new nsFrameSelection;
  if (!rlist)
    return NS_ERROR_OUT_OF_MEMORY;
  *aFrameSelection = rlist;
  NS_ADDREF(rlist);
  return NS_OK;
}

nsresult NS_NewDomSelection(nsISelection **aDomSelection);

nsresult NS_NewDomSelection(nsISelection **aDomSelection)
{
  nsTypedSelection *rlist = new nsTypedSelection;
  if (!rlist)
    return NS_ERROR_OUT_OF_MEMORY;
  *aDomSelection = (nsISelection *)rlist;
  NS_ADDREF(rlist);
  return NS_OK;
}

static PRInt8
GetIndexFromSelectionType(SelectionType aType)
{
    switch (aType)
    {
    case nsISelectionController::SELECTION_NORMAL: return 0; break;
    case nsISelectionController::SELECTION_SPELLCHECK: return 1; break;
    case nsISelectionController::SELECTION_IME_RAWINPUT: return 2; break;
    case nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT: return 3; break;
    case nsISelectionController::SELECTION_IME_CONVERTEDTEXT: return 4; break;
    case nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT: return 5; break;
    case nsISelectionController::SELECTION_ACCESSIBILITY: return 6; break;
    default:return -1;break;
    }
    
    return 0;
}

static SelectionType 
GetSelectionTypeFromIndex(PRInt8 aIndex)
{
  switch (aIndex)
  {
    case 0: return nsISelectionController::SELECTION_NORMAL;break;
    case 1: return nsISelectionController::SELECTION_SPELLCHECK;break;
    case 2: return nsISelectionController::SELECTION_IME_RAWINPUT;break;
    case 3: return nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT;break;
    case 4: return nsISelectionController::SELECTION_IME_CONVERTEDTEXT;break;
    case 5: return nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT;break;
    case 6: return nsISelectionController::SELECTION_ACCESSIBILITY;break;
    default:
      return nsISelectionController::SELECTION_NORMAL;break;
  }
  
  return 0;
}


PRBool       
IsValidSelectionPoint(nsFrameSelection *aFrameSel, nsIDOMNode *aDomNode)
{
    nsCOMPtr<nsIContent> passedContent = do_QueryInterface(aDomNode);
    if (!passedContent)
      return PR_FALSE;
    return IsValidSelectionPoint(aFrameSel, passedContent);
}














PRBool       
IsValidSelectionPoint(nsFrameSelection *aFrameSel, nsIContent *aContent)
{
  if (!aFrameSel || !aContent)
    return PR_FALSE;
  if (aFrameSel)
  {
    nsCOMPtr<nsIContent> tLimiter = aFrameSel->GetLimiter();
    if (tLimiter && tLimiter != aContent)
    {
      if (tLimiter != aContent->GetParent()) 
        return PR_FALSE; 
    }
  }
  return PR_TRUE;
}


NS_IMPL_ADDREF(nsSelectionIterator)
NS_IMPL_RELEASE(nsSelectionIterator)

NS_INTERFACE_MAP_BEGIN(nsSelectionIterator)
  NS_INTERFACE_MAP_ENTRY(nsIEnumerator)
  NS_INTERFACE_MAP_ENTRY(nsIBidirectionalEnumerator)
NS_INTERFACE_MAP_END_AGGREGATED(mDomSelection)




nsSelectionIterator::nsSelectionIterator(nsTypedSelection *aList)
:mIndex(0)
{
  if (!aList)
  {
    NS_NOTREACHED("nsFrameSelection");
    return;
  }
  mDomSelection = aList;
}



nsSelectionIterator::~nsSelectionIterator()
{
}









NS_IMETHODIMP
nsSelectionIterator::Next()
{
  mIndex++;
  PRInt32 cnt = mDomSelection->mRanges.Length();
  if (mIndex < cnt)
    return NS_OK;
  return NS_ERROR_FAILURE;
}



NS_IMETHODIMP
nsSelectionIterator::Prev()
{
  mIndex--;
  if (mIndex >= 0 )
    return NS_OK;
  return NS_ERROR_FAILURE;
}



NS_IMETHODIMP
nsSelectionIterator::First()
{
  if (!mDomSelection)
    return NS_ERROR_NULL_POINTER;
  mIndex = 0;
  return NS_OK;
}



NS_IMETHODIMP
nsSelectionIterator::Last()
{
  if (!mDomSelection)
    return NS_ERROR_NULL_POINTER;
  mIndex = mDomSelection->mRanges.Length() - 1;
  return NS_OK;
}



NS_IMETHODIMP 
nsSelectionIterator::CurrentItem(nsISupports **aItem)
{
  if (!aItem)
    return NS_ERROR_NULL_POINTER;

  if (mIndex < 0 || mIndex >= (PRInt32)mDomSelection->mRanges.Length()) {
    return NS_ERROR_FAILURE;
  }

  return CallQueryInterface(mDomSelection->mRanges[mIndex].mRange,
                            aItem);
}


NS_IMETHODIMP 
nsSelectionIterator::CurrentItem(nsIDOMRange **aItem)
{
  if (!aItem)
    return NS_ERROR_NULL_POINTER;
  if (mIndex < 0 || mIndex >= (PRInt32)mDomSelection->mRanges.Length()) {
    return NS_ERROR_FAILURE;
  }

  *aItem = mDomSelection->mRanges[mIndex].mRange;
  NS_IF_ADDREF(*aItem);
  return NS_OK;
}



NS_IMETHODIMP
nsSelectionIterator::IsDone()
{
  PRInt32 cnt = mDomSelection->mRanges.Length();
  if (mIndex >= 0 && mIndex < cnt) {
    return NS_ENUMERATOR_FALSE;
  }
  return NS_OK;
}




#ifdef XP_MAC
#pragma mark -
#endif



nsFrameSelection::nsFrameSelection()
  : mScrollableViewProvider(nsnull),
    mDelayedMouseEvent(PR_FALSE, 0, nsnull, nsMouseEvent::eReal)
{
  PRInt32 i;
  for (i = 0;i<nsISelectionController::NUM_SELECTIONTYPES;i++){
    mDomSelections[i] = nsnull;
  }
  for (i = 0;i<nsISelectionController::NUM_SELECTIONTYPES;i++){
    mDomSelections[i] = new nsTypedSelection(this);
    if (!mDomSelections[i])
      return;
    NS_ADDREF(mDomSelections[i]);
    mDomSelections[i]->SetType(GetSelectionTypeFromIndex(i));
  }
  mBatching = 0;
  mChangesDuringBatching = PR_FALSE;
  mNotifyFrames = PR_TRUE;
  mLimiter = nsnull; 
  
  mMouseDoubleDownState = PR_FALSE;
  
  mHint = HINTLEFT;
#ifdef IBMBIDI
  mCaretBidiLevel = BIDI_LEVEL_UNDEFINED;
#endif
  mDragSelectingCells = PR_FALSE;
  mSelectingTableCellMode = 0;
  mSelectedCellIndex = 0;

  
  
  if (nsContentUtils::GetBoolPref("clipboard.autocopy")) {
    nsAutoCopyListener *autoCopy = nsAutoCopyListener::GetInstance();

    if (autoCopy) {
      PRInt8 index =
        GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
      if (mDomSelections[index]) {
        autoCopy->Listen(mDomSelections[index]);
      }
    }
  }

  mDisplaySelection = nsISelectionController::SELECTION_OFF;

  mDelayedMouseEventValid = PR_FALSE;
  mSelectionChangeReason = nsISelectionListener::NO_REASON;
}


nsFrameSelection::~nsFrameSelection()
{
  PRInt32 i;
  for (i = 0;i<nsISelectionController::NUM_SELECTIONTYPES;i++){
    if (mDomSelections[i]) {
      mDomSelections[i]->DetachFromPresentation();
      NS_RELEASE(mDomSelections[i]);
    }
  }
}


NS_IMPL_ISUPPORTS1(nsFrameSelection, nsFrameSelection)


nsresult
nsFrameSelection::FetchDesiredX(nscoord &aDesiredX) 
{
  if (!mShell)
  {
    NS_ASSERTION(0,"fetch desired X failed\n");
    return NS_ERROR_FAILURE;
  }
  if (mDesiredXSet)
  {
    aDesiredX = mDesiredX;
    return NS_OK;
  }

  nsCOMPtr<nsICaret> caret;
  nsresult result = mShell->GetCaret(getter_AddRefs(caret));
  if (NS_FAILED(result))
    return result;
  if (!caret)
    return NS_ERROR_NULL_POINTER;

  nsRect coord;
  PRBool  collapsed;
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  result = caret->SetCaretDOMSelection(mDomSelections[index]);
  if (NS_FAILED(result))
    return result;

  result = caret->GetCaretCoordinates(nsICaret::eClosestViewCoordinates, mDomSelections[index], &coord, &collapsed, nsnull);
  if (NS_FAILED(result))
    return result;
   
  aDesiredX = coord.x;
  return NS_OK;
}



void
nsFrameSelection::InvalidateDesiredX() 
{
  mDesiredXSet = PR_FALSE;
}



void
nsFrameSelection::SetDesiredX(nscoord aX) 
{
  mDesiredX = aX;
  mDesiredXSet = PR_TRUE;
}

nsresult
nsFrameSelection::GetRootForContentSubtree(nsIContent  *aContent,
                                           nsIContent **aParent)
{
  
  
  
  
  
  
  
  
  

  if (!aContent || !aParent)
    return NS_ERROR_NULL_POINTER;

  *aParent = 0;

  nsIContent* child = aContent;

  while (child)
  {
    nsIContent* parent = child->GetParent();

    if (!parent)
      break;

    PRUint32 childCount = parent->GetChildCount();

    if (childCount < 1)
      break;

    PRInt32 childIndex = parent->IndexOf(child);

    if (childIndex < 0 || ((PRUint32)childIndex) >= childCount)
      break;

    child = parent;
  }

  NS_IF_ADDREF(*aParent = child);

  return NS_OK;
}

nsresult
nsFrameSelection::ConstrainFrameAndPointToAnchorSubtree(nsIFrame  *aFrame,
                                                        nsPoint&   aPoint,
                                                        nsIFrame **aRetFrame,
                                                        nsPoint&   aRetPoint)
{
  
  
  
  
  
  
  
  
  
  
  
  

  if (!aFrame || !aRetFrame)
    return NS_ERROR_NULL_POINTER;

  *aRetFrame = aFrame;
  aRetPoint  = aPoint;

  
  
  

  nsresult result;
  nsCOMPtr<nsIDOMNode> anchorNode;
  PRInt32 anchorOffset = 0;
  PRInt32 anchorFrameOffset = 0;

  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  if (! mDomSelections[index])
    return NS_ERROR_NULL_POINTER;

  result = mDomSelections[index]->GetAnchorNode(getter_AddRefs(anchorNode));

  if (NS_FAILED(result))
    return result;

  if (!anchorNode)
    return NS_OK;

  result = mDomSelections[index]->GetAnchorOffset(&anchorOffset);

  if (NS_FAILED(result))
    return result;

  nsIFrame *anchorFrame = 0;
  nsCOMPtr<nsIContent> anchorContent = do_QueryInterface(anchorNode);

  if (!anchorContent)
    return NS_ERROR_FAILURE;
  
  anchorFrame = GetFrameForNodeOffset(anchorContent, anchorOffset,
                                      mHint, &anchorFrameOffset);

  
  
  

  nsCOMPtr<nsIContent> anchorRoot;
  result = GetRootForContentSubtree(anchorContent, getter_AddRefs(anchorRoot));

  if (NS_FAILED(result))
    return result;

  
  
  

  nsIContent* content = aFrame->GetContent();

  if (content)
  {
    nsCOMPtr<nsIContent> contentRoot;

    result = GetRootForContentSubtree(content, getter_AddRefs(contentRoot));

    if (anchorRoot == contentRoot)
    {
      
      
      
      
      *aRetFrame = aFrame;
      return NS_OK;
    }
  }

  
  
  
  
  
  

  *aRetFrame = mShell->GetPrimaryFrameFor(anchorRoot);

  if (! *aRetFrame)
    return NS_ERROR_FAILURE;

  
  
  
  

  aRetPoint = aPoint + aFrame->GetOffsetTo(*aRetFrame);

  return NS_OK;
}

#ifdef IBMBIDI
void
nsFrameSelection::SetCaretBidiLevel(PRUint8 aLevel)
{
  
  
  PRBool afterInsert = mCaretBidiLevel & BIDI_LEVEL_UNDEFINED;
  mCaretBidiLevel = aLevel;
  
  nsIBidiKeyboard* bidiKeyboard = nsContentUtils::GetBidiKeyboard();
  if (bidiKeyboard && !afterInsert)
    bidiKeyboard->SetLangFromBidiLevel(aLevel);
  return;
}

PRUint8
nsFrameSelection::GetCaretBidiLevel()
{
  return mCaretBidiLevel;
}

void
nsFrameSelection::UndefineCaretBidiLevel()
{
  mCaretBidiLevel |= BIDI_LEVEL_UNDEFINED;
}
#endif

#ifdef XP_MAC
#pragma mark -
#endif

#ifdef PRINT_RANGE
void printRange(nsIDOMRange *aDomRange)
{
  if (!aDomRange)
  {
    printf("NULL nsIDOMRange\n");
  }
  nsCOMPtr<nsIDOMNode> startNode;
  nsCOMPtr<nsIDOMNode> endNode;
  PRInt32 startOffset;
  PRInt32 endOffset;
  aDomRange->GetStartParent(getter_AddRefs(startNode));
  aDomRange->GetStartOffset(&startOffset);
  aDomRange->GetEndParent(getter_AddRefs(endNode));
  aDomRange->GetEndOffset(&endOffset);
  
  printf("range: 0x%lx\t start: 0x%lx %ld, \t end: 0x%lx,%ld\n",
         (unsigned long)aDomRange,
         (unsigned long)(nsIDOMNode*)startNode, (long)startOffset,
         (unsigned long)(nsIDOMNode*)endNode, (long)endOffset);
         
}
#endif 

static
nsIAtom *GetTag(nsIDOMNode *aNode)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  if (!content) 
  {
    NS_NOTREACHED("bad node passed to GetTag()");
    return nsnull;
  }
  
  return content->Tag();
}

nsresult
ParentOffset(nsIDOMNode *aNode, nsIDOMNode **aParent, PRInt32 *aChildOffset)
{
  if (!aNode || !aParent || !aChildOffset)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  if (content)
  {
    nsIContent* parent = content->GetParent();
    if (parent)
    {
      *aChildOffset = parent->IndexOf(content);

      return CallQueryInterface(parent, aParent);
    }
  }

  return NS_OK;
}

nsIDOMNode *
GetCellParent(nsIDOMNode *aDomNode)
{
    if (!aDomNode)
      return 0;
    nsCOMPtr<nsIDOMNode> parent(aDomNode);
    nsCOMPtr<nsIDOMNode> current(aDomNode);
    PRInt32 childOffset;
    nsIAtom *tag;
    
    while(current)
    {
      tag = GetTag(current);
      if (tag == nsGkAtoms::td || tag == nsGkAtoms::th)
        return current;
      if (NS_FAILED(ParentOffset(current,getter_AddRefs(parent),&childOffset)) || !parent)
        return 0;
      current = parent;
    }
    return 0;
}


void
nsFrameSelection::Init(nsIPresShell *aShell, nsIContent *aLimiter)
{
  mShell = aShell;
  mMouseDownState = PR_FALSE;
  mDesiredXSet = PR_FALSE;
  mLimiter = aLimiter;
  mCaretMovementStyle = nsContentUtils::GetIntPref("bidi.edit.caret_movement_style", 2);
}

nsresult
nsFrameSelection::MoveCaret(PRUint32          aKeycode,
                            PRBool            aContinueSelection,
                            nsSelectionAmount aAmount)
{
  nsPresContext *context = mShell->GetPresContext();
  if (!context)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> weakNodeUsed;
  PRInt32 offsetused = 0;

  PRBool isCollapsed;
  nscoord desiredX = 0; 

  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  nsresult result = mDomSelections[index]->GetIsCollapsed(&isCollapsed);
  if (NS_FAILED(result))
    return result;
  if (aKeycode == nsIDOMKeyEvent::DOM_VK_UP || aKeycode == nsIDOMKeyEvent::DOM_VK_DOWN)
  {
    result = FetchDesiredX(desiredX);
    if (NS_FAILED(result))
      return result;
    SetDesiredX(desiredX);
  }

  PRInt32 caretStyle = nsContentUtils::GetIntPref("layout.selection.caret_style", 0);
#ifdef XP_MACOSX
  if (caretStyle == 0) {
    caretStyle = 2; 
  }
#endif

  if (!isCollapsed && !aContinueSelection && caretStyle == 2) {
    switch (aKeycode){
      case nsIDOMKeyEvent::DOM_VK_LEFT  :
      case nsIDOMKeyEvent::DOM_VK_UP    :
          if (mDomSelections[index]->GetDirection() == eDirPrevious) { 
            offsetused = mDomSelections[index]->FetchFocusOffset();
            weakNodeUsed = mDomSelections[index]->FetchFocusNode();
          }
          else {
            offsetused = mDomSelections[index]->FetchAnchorOffset();
            weakNodeUsed = mDomSelections[index]->FetchAnchorNode();
          }
          result = mDomSelections[index]->Collapse(weakNodeUsed, offsetused);
          mHint = HINTRIGHT;
          mDomSelections[index]->ScrollIntoView();
          return NS_OK;

      case nsIDOMKeyEvent::DOM_VK_RIGHT :
      case nsIDOMKeyEvent::DOM_VK_DOWN  :
          if (mDomSelections[index]->GetDirection() == eDirPrevious) { 
            offsetused = mDomSelections[index]->FetchAnchorOffset();
            weakNodeUsed = mDomSelections[index]->FetchAnchorNode();
          }
          else {
            offsetused = mDomSelections[index]->FetchFocusOffset();
            weakNodeUsed = mDomSelections[index]->FetchFocusNode();
          }
          result = mDomSelections[index]->Collapse(weakNodeUsed, offsetused);
          mHint = HINTLEFT;
          mDomSelections[index]->ScrollIntoView();
          return NS_OK;
    }
  }

  PRBool visualMovement = 
    (aKeycode == nsIDOMKeyEvent::DOM_VK_BACK_SPACE || 
     aKeycode == nsIDOMKeyEvent::DOM_VK_DELETE ||
     aKeycode == nsIDOMKeyEvent::DOM_VK_HOME || 
     aKeycode == nsIDOMKeyEvent::DOM_VK_END) ?
    PR_FALSE : 
    mCaretMovementStyle == 1 || (mCaretMovementStyle == 2 && !aContinueSelection);

  nsIFrame *frame;
  result = mDomSelections[index]->GetPrimaryFrameForFocusNode(&frame, &offsetused, visualMovement);

  if (NS_FAILED(result) || !frame)
    return result?result:NS_ERROR_FAILURE;

  nsPeekOffsetStruct pos;
  
  
  pos.SetData(aAmount, eDirPrevious, offsetused, desiredX, 
              PR_TRUE, mLimiter != nsnull, PR_TRUE, visualMovement);

  nsBidiLevel baseLevel = nsBidiPresUtils::GetFrameBaseLevel(frame);
  
  HINT tHint(mHint); 
  switch (aKeycode){
    case nsIDOMKeyEvent::DOM_VK_RIGHT : 
        InvalidateDesiredX();
        pos.mDirection = (baseLevel & 1) ? eDirPrevious : eDirNext;
      break;
    case nsIDOMKeyEvent::DOM_VK_LEFT :
        InvalidateDesiredX();
        pos.mDirection = (baseLevel & 1) ? eDirNext : eDirPrevious;
      break;
    case nsIDOMKeyEvent::DOM_VK_DELETE :
        InvalidateDesiredX();
        pos.mDirection = eDirNext;
      break;
    case nsIDOMKeyEvent::DOM_VK_BACK_SPACE : 
        InvalidateDesiredX();
        pos.mDirection = eDirPrevious;
      break;
    case nsIDOMKeyEvent::DOM_VK_DOWN : 
        pos.mAmount = eSelectLine;
        pos.mDirection = eDirNext;
      break;
    case nsIDOMKeyEvent::DOM_VK_UP : 
        pos.mAmount = eSelectLine;
        pos.mDirection = eDirPrevious;
      break;
    case nsIDOMKeyEvent::DOM_VK_HOME :
        InvalidateDesiredX();
        pos.mAmount = eSelectBeginLine;
      break;
    case nsIDOMKeyEvent::DOM_VK_END :
        InvalidateDesiredX();
        pos.mAmount = eSelectEndLine;
      break;
  default :return NS_ERROR_FAILURE;
  }
  PostReason(nsISelectionListener::KEYPRESS_REASON);
  if (NS_SUCCEEDED(result = frame->PeekOffset(&pos)) && pos.mResultContent)
  {
    nsIFrame *theFrame;
    PRInt32 currentOffset, frameStart, frameEnd;

    if (aAmount == eSelectCharacter || aAmount == eSelectWord)
    {
      
      
      
      
      theFrame = pos.mResultFrame;
      theFrame->GetOffsets(frameStart, frameEnd);
      currentOffset = pos.mContentOffset;
      if (frameEnd == currentOffset && !(frameStart == 0 && frameEnd == 0))
        tHint = HINTLEFT;
      else
        tHint = HINTRIGHT;
    } else {
      
      
      tHint = (HINT)pos.mAttachForward;
      theFrame = GetFrameForNodeOffset(pos.mResultContent, pos.mContentOffset,
                                       tHint, &currentOffset);
      if (!theFrame)
        return NS_ERROR_FAILURE;

      theFrame->GetOffsets(frameStart, frameEnd);
    }

    if (context->BidiEnabled())
    {
      switch (aKeycode) {
        case nsIDOMKeyEvent::DOM_VK_HOME:
        case nsIDOMKeyEvent::DOM_VK_END:
          
          SetCaretBidiLevel(NS_GET_BASE_LEVEL(theFrame));
          break;

        default:
          
          if ((pos.mContentOffset != frameStart && pos.mContentOffset != frameEnd)
              || (eSelectLine == aAmount))
          {
            SetCaretBidiLevel(NS_GET_EMBEDDING_LEVEL(theFrame));
          }
          else
            BidiLevelFromMove(mShell, pos.mResultContent, pos.mContentOffset, aKeycode, tHint);
      }
#ifdef VISUALSELECTION
      
      if (aContinueSelection)
      {
        result = VisualSelectFrames(theFrame, pos);
        if (NS_FAILED(result)) 
          result = TakeFocus(pos.mResultContent, pos.mContentOffset, pos.mContentOffset, PR_FALSE, PR_FALSE);
      }    
      else
        result = TakeFocus(pos.mResultContent, pos.mContentOffset, pos.mContentOffset, aContinueSelection, PR_FALSE);
    }
    else
#else
    }
#endif
    result = TakeFocus(pos.mResultContent, pos.mContentOffset, pos.mContentOffset, aContinueSelection, PR_FALSE);
  } else if (aKeycode == nsIDOMKeyEvent::DOM_VK_RIGHT && !aContinueSelection) {
    
    
    
    weakNodeUsed = mDomSelections[index]->FetchFocusNode();
    offsetused = mDomSelections[index]->FetchFocusOffset();
    PRBool isBRFrame = frame->GetType() == nsGkAtoms::brFrame;
    mDomSelections[index]->Collapse(weakNodeUsed, offsetused);
    
    if (isBRFrame) {
      tHint = mHint;    
    }
    else {
      tHint = HINTLEFT; 
    }
    result = NS_OK;
  }
  if (NS_SUCCEEDED(result))
  {
    mHint = tHint; 
    result = mDomSelections[index]->ScrollIntoView();
  }

  return result;
}






NS_IMETHODIMP
nsTypedSelection::ToString(PRUnichar **aReturn)
{
  return ToStringWithFormat("text/plain", 0, 0, aReturn);
}


NS_IMETHODIMP
nsTypedSelection::ToStringWithFormat(const char * aFormatType, PRUint32 aFlags, 
                                   PRInt32 aWrapCol, PRUnichar **aReturn)
{
  nsresult rv = NS_OK;
  if (!aReturn)
    return NS_ERROR_NULL_POINTER;
  
  nsCAutoString formatType( NS_DOC_ENCODER_CONTRACTID_BASE );
  formatType.Append(aFormatType);
  nsCOMPtr<nsIDocumentEncoder> encoder =
           do_CreateInstance(formatType.get(), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPresShell> shell;
  rv = GetPresShell(getter_AddRefs(shell));
  if (NS_FAILED(rv) || !shell) {
    return NS_ERROR_FAILURE;
  }

  nsIDocument *doc = shell->GetDocument();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(doc);
  NS_ASSERTION(domDoc, "Need a document");

  
  aFlags |= nsIDocumentEncoder::OutputSelectionOnly;
  nsAutoString readstring;
  readstring.AssignASCII(aFormatType);
  rv = encoder->Init(domDoc, readstring, aFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  encoder->SetSelection(this);
  if (aWrapCol != 0)
    encoder->SetWrapColumn(aWrapCol);

  nsAutoString tmp;
  rv = encoder->EncodeToString(tmp);
  *aReturn = ToNewUnicode(tmp);
  return rv;
}

NS_IMETHODIMP
nsTypedSelection::SetInterlinePosition(PRBool aHintRight)
{
  if (!mFrameSelection)
    return NS_ERROR_NOT_INITIALIZED; 
  nsFrameSelection::HINT hint;
  if (aHintRight)
    hint = nsFrameSelection::HINTRIGHT;
  else
    hint = nsFrameSelection::HINTLEFT;
  mFrameSelection->SetHint(hint);
  
  return NS_OK;
}

NS_IMETHODIMP
nsTypedSelection::GetInterlinePosition(PRBool *aHintRight)
{
  if (!mFrameSelection)
    return NS_ERROR_NOT_INITIALIZED; 
  *aHintRight = (mFrameSelection->GetHint() == nsFrameSelection::HINTRIGHT);
  return NS_OK;
}

#ifdef VISUALSELECTION

static nsDirection
ReverseDirection(nsDirection aDirection)
{
  return (eDirNext == aDirection) ? eDirPrevious : eDirNext;
}

static nsresult
FindLineContaining(nsIFrame* aFrame, nsIFrame** aBlock, PRInt32* aLine)
{
  nsIFrame *blockFrame = aFrame;
  nsIFrame *thisBlock = nsnull;
  nsCOMPtr<nsILineIteratorNavigator> it; 
  nsresult result = NS_ERROR_FAILURE;
  while (NS_FAILED(result) && blockFrame)
  {
    thisBlock = blockFrame;
    blockFrame = blockFrame->GetParent();
    if (blockFrame) {
      it = do_QueryInterface(blockFrame, &result);
    }
  }
  if (!blockFrame || !it)
    return NS_ERROR_FAILURE;
  *aBlock = blockFrame;
  return it->FindLineContaining(thisBlock, aLine);  
}

NS_IMETHODIMP
nsFrameSelection::VisualSequence(nsIFrame*           aSelectFrame,
                                 nsIFrame*           aCurrentFrame,
                                 nsPeekOffsetStruct* aPos,
                                 PRBool*             aNeedVisualSelection)
{
  nsVoidArray frameArray;
  PRInt32 frameStart, frameEnd;
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  nsresult result = nsnull;
  
  PRUint8 currentLevel = NS_GET_EMBEDDING_LEVEL(aCurrentFrame);
  result = aSelectFrame->PeekOffset(aPos);
  while (aCurrentFrame != (aSelectFrame = aPos->mResultFrame))
  {
    if (NS_FAILED(result))
      return NS_OK; 
    if (!aSelectFrame)
      return NS_ERROR_FAILURE;
    if (frameArray.IndexOf(aSelectFrame) > -1)
      
      return NS_OK;
    else
      frameArray.AppendElement(aSelectFrame);

    aSelectFrame->GetOffsets(frameStart, frameEnd);
    PRUint8 bidiLevel = NS_GET_EMBEDDING_LEVEL(aSelectFrame);
    
    if (currentLevel != bidiLevel)
      *aNeedVisualSelection = PR_TRUE;
    if ((eDirNext == aPos->mDirection) == (bidiLevel & 1))
    {
      mDomSelections[index]->SetDirection(eDirPrevious);
      result = TakeFocus(aPos->mResultContent, frameEnd, frameStart, PR_FALSE, PR_TRUE);
    }
    else
    {
      mDomSelections[index]->SetDirection(eDirNext);
      result = TakeFocus(aPos->mResultContent, frameStart, frameEnd, PR_FALSE, PR_TRUE);
    }
    if (NS_FAILED(result))
      return result;

    aPos->mAmount = eSelectDir; 
    aPos->mContentOffset = 0;
    result = aSelectFrame->PeekOffset(aPos);
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsFrameSelection::SelectToEdge(nsIFrame   *aFrame,
                               nsIContent *aContent,
                               PRInt32     aOffset,
                               PRInt32     aEdge,
                               PRBool      aMultipleSelection)
{
  PRInt32 frameStart, frameEnd;
  
  aFrame->GetOffsets(frameStart, frameEnd);
  if (0 == aEdge)
    aEdge = frameStart;
  else if (-1 == aEdge)
    aEdge = frameEnd;
  if (0 == aOffset)
    aOffset = frameStart;
  else if (-1 == aOffset)
    aOffset = frameEnd;
  return TakeFocus(aContent, aOffset, aEdge, PR_FALSE, aMultipleSelection);
}

NS_IMETHODIMP
nsFrameSelection::SelectLines(nsDirection        aSelectionDirection,
                              nsIDOMNode        *aAnchorNode,
                              nsIFrame*          aAnchorFrame,
                              PRInt32            aAnchorOffset,
                              nsIDOMNode        *aCurrentNode,
                              nsIFrame*          aCurrentFrame,
                              PRInt32            aCurrentOffset,
                              nsPeekOffsetStruct aPos)
{
  nsIFrame *startFrame, *endFrame;
  PRInt32 startOffset, endOffset;
  PRInt32 relativePosition;
  nsCOMPtr<nsIDOMNode> startNode;
  nsCOMPtr<nsIDOMNode> endNode;
  nsIContent *startContent;
  nsIContent *endContent;
  nsresult result;

  
  relativePosition = CompareDOMPoints(aAnchorNode, aAnchorOffset,
                                      aCurrentNode, aCurrentOffset);
  if (0 == relativePosition)
    return NS_ERROR_FAILURE;
  else if (relativePosition < 0)
  {
    startNode = aAnchorNode;
    startFrame = aAnchorFrame;
    startOffset = aAnchorOffset;
    endNode = aCurrentNode;
    endFrame = aCurrentFrame;
    endOffset = aCurrentOffset;
  }
  else
  {
    startNode = aCurrentNode;
    startFrame = aCurrentFrame;
    startOffset = aCurrentOffset;
    endNode = aAnchorNode;
    endFrame = aAnchorFrame;
    endOffset = aAnchorOffset;
  }
  
  aPos.mStartOffset = startOffset;
  aPos.mDirection = eDirNext;
  aPos.mAmount = eSelectLine;
  result = startFrame->PeekOffset(&aPos);
  if (NS_FAILED(result))
    return result;
  startFrame = aPos.mResultFrame;
  
  aPos.mStartOffset = aPos.mContentOffset;
  aPos.mAmount = eSelectBeginLine;
  result = startFrame->PeekOffset(&aPos);
  if (NS_FAILED(result))
    return result;
  
  nsIFrame *theFrame;
  PRInt32 currentOffset, frameStart, frameEnd;
  
  theFrame = GetFrameForNodeOffset(aPos.mResultContent, aPos.mContentOffset,
                                   HINTLEFT, &currentOffset);
  if (!theFrame)
    return NS_ERROR_FAILURE;

  theFrame->GetOffsets(frameStart, frameEnd);
  startOffset = frameStart;
  startContent = aPos.mResultContent;
  startNode = do_QueryInterface(startContent);

  
  if (CompareDOMPoints(startNode, startOffset, endNode, endOffset) >= 0)
    return NS_ERROR_FAILURE;

  aPos.mStartOffset = endOffset;
  aPos.mDirection = eDirPrevious;
  aPos.mAmount = eSelectLine;
  result = endFrame->PeekOffset(&aPos);
  if (NS_FAILED(result))
    return result;
  endFrame = aPos.mResultFrame;

  aPos.mStartOffset = aPos.mContentOffset;
  aPos.mAmount = eSelectEndLine;
  result = endFrame->PeekOffset(&aPos);
  if (NS_FAILED(result))
    return result;

  theFrame = GetFrameForNodeOffset(aPos.mResultContent, aPos.mContentOffset,
                                   HINTRIGHT, &currentOffset);
  if (!theFrame)
    return NS_ERROR_FAILURE;

  theFrame->GetOffsets(frameStart, frameEnd);
  endOffset = frameEnd;
  endContent = aPos.mResultContent;
  endNode = do_QueryInterface(endContent);

  if (CompareDOMPoints(startNode, startOffset, endNode, endOffset) < 0)
  {
    TakeFocus(startContent, startOffset, startOffset, PR_FALSE, PR_TRUE);
    return TakeFocus(endContent, endOffset, endOffset, PR_TRUE, PR_TRUE);
  }
  else
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsFrameSelection::VisualSelectFrames(nsIFrame*          aCurrentFrame,
                                     nsPeekOffsetStruct aPos)
{
  nsCOMPtr<nsIContent> anchorContent;
  nsCOMPtr<nsIDOMNode> anchorNode;
  PRInt32 anchorOffset;
  nsIFrame* anchorFrame;
  nsCOMPtr<nsIContent> focusContent;
  nsCOMPtr<nsIDOMNode> focusNode;
  PRInt32 focusOffset;
  nsIFrame* focusFrame;
  nsCOMPtr<nsIContent> currentContent;
  nsCOMPtr<nsIDOMNode> currentNode;
  PRInt32 currentOffset;
  nsresult result;
  nsIFrame* startFrame;
  PRBool needVisualSelection = PR_FALSE;
  nsDirection selectionDirection;
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  
  result = mDomSelections[index]->GetOriginalAnchorPoint(getter_AddRefs(anchorNode), &anchorOffset);
  if (NS_FAILED(result))
    return result;
  anchorContent = do_QueryInterface(anchorNode);
  anchorFrame = GetFrameForNodeOffset(anchorContent, anchorOffset,
                                      mHint, &anchorOffset);
  if (!anchorFrame)
    return NS_ERROR_FAILURE;

  PRUint8 anchorLevel = NS_GET_EMBEDDING_LEVEL(anchorFrame);
  
  currentContent = aPos.mResultContent;
  currentNode = do_QueryInterface(currentContent);
  currentOffset = aPos.mContentOffset;
  PRUint8 currentLevel = NS_GET_EMBEDDING_LEVEL(aCurrentFrame);

  
  
  if (anchorFrame == aCurrentFrame) {
    mDomSelections[index]->SetTrueDirection(!(anchorLevel & 1));
    return TakeFocus(currentContent, anchorOffset, currentOffset, PR_FALSE, PR_FALSE);
  }

  focusOffset = mDomSelections[index]->FetchFocusOffset();
  focusNode = mDomSelections[index]->FetchFocusNode();
  focusContent = do_QueryInterface(focusNode);
  HINT hint;
  if ((HINTLEFT == mHint) == (currentLevel & 1))
    hint = HINTRIGHT;
  else
    hint = HINTLEFT;

  focusFrame = GetFrameForNodeOffset(focusContent, focusOffset,
                                     hint, &focusOffset);
  if (!focusFrame)
    return NS_ERROR_FAILURE;

  PRUint8 focusLevel = NS_GET_EMBEDDING_LEVEL(focusFrame);

  if (currentLevel != anchorLevel)
    needVisualSelection = PR_TRUE;

  
  selectionDirection = mDomSelections[index]->GetDirection();
  if (!mDomSelections[index]->GetTrueDirection()) {
    selectionDirection = ReverseDirection(selectionDirection);
    mDomSelections[index]->SetDirection(selectionDirection);
  }

  PRInt32 anchorLine, currentLine;
  nsIFrame* anchorBlock  = nsnull;
  nsIFrame* currentBlock = nsnull;
  FindLineContaining(anchorFrame, &anchorBlock, &anchorLine);
  FindLineContaining(aCurrentFrame, &currentBlock, &currentLine);

  if (anchorBlock==currentBlock && anchorLine==currentLine)
  {
    

    
    
    
    if ((eDirNext == selectionDirection) == (anchorLevel & 1))
      result = SelectToEdge(anchorFrame, anchorContent, anchorOffset, 0, PR_FALSE);
    else
      result = SelectToEdge(anchorFrame, anchorContent, anchorOffset, -1, PR_FALSE);
    if (NS_FAILED(result))
      return result;

    
    InvalidateDesiredX();
    aPos.mAmount = eSelectDir;
    aPos.mStartOffset = anchorOffset;
    aPos.mDirection = selectionDirection;

    result = anchorFrame->PeekOffset(&aPos);
    if (NS_FAILED(result))
      return result;
    
    startFrame = aPos.mResultFrame;
    result = VisualSequence(startFrame, aCurrentFrame, &aPos, &needVisualSelection);
    if (NS_FAILED(result))
      return result;

    if (!needVisualSelection)
    {
      if (currentLevel & 1)
        mDomSelections[index]->SetDirection(ReverseDirection(selectionDirection));
      
      result = TakeFocus(anchorContent, anchorOffset, anchorOffset, PR_FALSE, PR_FALSE);
      if (NS_FAILED(result))
        return result;
      result = TakeFocus(currentContent, currentOffset, currentOffset, PR_TRUE, PR_FALSE);
      if (NS_FAILED(result))
        return result;
    }
    else {
      if ((currentLevel & 1) != (focusLevel & 1))
        mDomSelections[index]->SetDirection(ReverseDirection(selectionDirection));
      
      if ((eDirNext == selectionDirection) == (currentLevel & 1))
        result = SelectToEdge(aCurrentFrame, currentContent, -1, currentOffset, PR_TRUE);
      else
        result = SelectToEdge(aCurrentFrame, currentContent, 0, currentOffset, PR_TRUE);
      if (NS_FAILED(result))
        return result;
    }
  }
  else {

    

    
    
    
    
    
    
    
    
    
    PRUint8 anchorBaseLevel = NS_GET_BASE_LEVEL(anchorFrame);
    if ((eDirNext == selectionDirection) != ((anchorLevel & 1) == (anchorBaseLevel & 1)))
      result = SelectToEdge(anchorFrame, anchorContent, anchorOffset, 0, PR_FALSE);
    else
      result = SelectToEdge(anchorFrame, anchorContent, anchorOffset, -1, PR_FALSE);
    if (NS_FAILED(result))
      return result;

    
    aPos.mJumpLines = PR_FALSE;
    aPos.mAmount = eSelectDir;
    aPos.mStartOffset = anchorOffset;
    aPos.mDirection = selectionDirection;
    if (anchorBaseLevel & 1)
      aPos.mDirection = ReverseDirection(aPos.mDirection);
    result = VisualSequence(anchorFrame, aCurrentFrame, &aPos, &needVisualSelection);
    if (NS_FAILED(result))
      return result;

    
    aPos.mJumpLines = PR_TRUE;
    result = SelectLines(selectionDirection, anchorNode, anchorFrame,
                         anchorOffset, currentNode, aCurrentFrame, currentOffset,
                         aPos);
    if (NS_FAILED(result))
      return result;

    
    PRUint8 currentBaseLevel = NS_GET_BASE_LEVEL(aCurrentFrame);
    
    aPos.mJumpLines = PR_FALSE;
    if ((currentBaseLevel & 1) == (anchorBaseLevel & 1))
      aPos.mDirection = ReverseDirection(aPos.mDirection);
    aPos.mStartOffset = currentOffset;
    result = VisualSequence(aCurrentFrame, anchorFrame, &aPos, &needVisualSelection);
    if (NS_FAILED(result))
      return result;

    
    if (currentLevel & 1)
      mDomSelections[index]->SetDirection(ReverseDirection(selectionDirection));

    if ((eDirPrevious == selectionDirection) != ((currentLevel & 1) == (currentBaseLevel & 1)))
      result = SelectToEdge(aCurrentFrame, currentContent, 0, currentOffset, PR_TRUE);
    else
      result = SelectToEdge(aCurrentFrame, currentContent, -1, currentOffset, PR_TRUE);
    if (NS_FAILED(result))
      return result;
    
    

  }

  
  mDomSelections[index]->SetTrueDirection(mDomSelections[index]->GetDirection() == selectionDirection);
  
  mDomSelections[index]->SetOriginalAnchorPoint(anchorNode, anchorOffset);
  NotifySelectionListeners(nsISelectionController::SELECTION_NORMAL);
  return NS_OK;
}
#endif 

nsPrevNextBidiLevels
nsFrameSelection::GetPrevNextBidiLevels(nsIContent *aNode,
                                        PRUint32    aContentOffset,
                                        PRBool      aJumpLines)
{
  return GetPrevNextBidiLevels(aNode, aContentOffset, mHint, aJumpLines);
}

nsPrevNextBidiLevels
nsFrameSelection::GetPrevNextBidiLevels(nsIContent *aNode,
                                        PRUint32    aContentOffset,
                                        HINT        aHint,
                                        PRBool      aJumpLines)
{
  
  nsIFrame    *currentFrame;
  PRInt32     currentOffset;
  PRInt32     frameStart, frameEnd;
  nsDirection direction;
  
  nsPrevNextBidiLevels levels;
  levels.SetData(nsnull, nsnull, 0, 0);

  currentFrame = GetFrameForNodeOffset(aNode, aContentOffset,
                                       aHint, &currentOffset);
  if (!currentFrame)
    return levels;

  currentFrame->GetOffsets(frameStart, frameEnd);

  if (0 == frameStart && 0 == frameEnd)
    direction = eDirPrevious;
  else if (frameStart == currentOffset)
    direction = eDirPrevious;
  else if (frameEnd == currentOffset)
    direction = eDirNext;
  else {
    
    levels.SetData(currentFrame, currentFrame,
                   NS_GET_EMBEDDING_LEVEL(currentFrame),
                   NS_GET_EMBEDDING_LEVEL(currentFrame));
    return levels;
  }

  nsIFrame *newFrame;
  PRInt32 offset;
  PRBool jumpedLine;
  nsresult rv = currentFrame->GetFrameFromDirection(direction, PR_FALSE,
                                                    aJumpLines, PR_TRUE,
                                                    &newFrame, &offset, &jumpedLine);
  if (NS_FAILED(rv))
    newFrame = nsnull;

  PRUint8 baseLevel = NS_GET_BASE_LEVEL(currentFrame);
  PRUint8 currentLevel = NS_GET_EMBEDDING_LEVEL(currentFrame);
  PRUint8 newLevel = newFrame ? NS_GET_EMBEDDING_LEVEL(newFrame) : baseLevel;
  
  
  
  if (!aJumpLines) {
    if (currentFrame->GetType() == nsGkAtoms::brFrame) {
      currentFrame = nsnull;
      currentLevel = baseLevel;
    }
    if (newFrame && newFrame->GetType() == nsGkAtoms::brFrame) {
      newFrame = nsnull;
      newLevel = baseLevel;
    }
  }
  
  if (direction == eDirNext)
    levels.SetData(currentFrame, newFrame, currentLevel, newLevel);
  else
    levels.SetData(newFrame, currentFrame, newLevel, currentLevel);

  return levels;
}

nsresult
nsFrameSelection::GetFrameFromLevel(nsIFrame    *aFrameIn,
                                    nsDirection  aDirection,
                                    PRUint8      aBidiLevel,
                                    nsIFrame   **aFrameOut)
{
  PRUint8 foundLevel = 0;
  nsIFrame *foundFrame = aFrameIn;

  nsCOMPtr<nsIBidirectionalEnumerator> frameTraversal;
  nsresult result;
  nsCOMPtr<nsIFrameTraversal> trav(do_CreateInstance(kFrameTraversalCID,&result));
  if (NS_FAILED(result))
      return result;

  result = trav->NewFrameTraversal(getter_AddRefs(frameTraversal),
                                   mShell->GetPresContext(), aFrameIn,
                                   eLeaf,
                                   PR_FALSE, 
                                   PR_FALSE, 
                                   PR_FALSE  
                                   );
  if (NS_FAILED(result))
    return result;
  nsISupports *isupports = nsnull;

  do {
    *aFrameOut = foundFrame;
    if (aDirection == eDirNext)
      result = frameTraversal->Next();
    else 
      result = frameTraversal->Prev();

    if (NS_FAILED(result))
      return result;
    result = frameTraversal->CurrentItem(&isupports);
    if (NS_FAILED(result))
      return result;
    if (!isupports)
      return NS_ERROR_NULL_POINTER;
    
    
    foundFrame = (nsIFrame *)isupports;
    foundLevel = NS_GET_EMBEDDING_LEVEL(foundFrame);

  } while (foundLevel > aBidiLevel);

  return NS_OK;
}


nsresult
nsFrameSelection::MaintainSelection(nsSelectionAmount aAmount)
{
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  nsCOMPtr<nsIDOMRange> range;
  nsresult rv = mDomSelections[index]->GetRangeAt(0, getter_AddRefs(range));
  if (NS_FAILED(rv))
    return rv;
  if (!range)
    return NS_ERROR_FAILURE;

  mMaintainedAmount = aAmount;
  
  nsCOMPtr<nsIDOMNode> startNode;
  nsCOMPtr<nsIDOMNode> endNode;
  PRInt32 startOffset;
  PRInt32 endOffset;
  range->GetStartContainer(getter_AddRefs(startNode));
  range->GetEndContainer(getter_AddRefs(endNode));
  range->GetStartOffset(&startOffset);
  range->GetEndOffset(&endOffset);

  mMaintainRange = nsnull;
  NS_NewRange(getter_AddRefs(mMaintainRange));
  if (!mMaintainRange)
    return NS_ERROR_OUT_OF_MEMORY;

  mMaintainRange->SetStart(startNode, startOffset);
  return mMaintainRange->SetEnd(endNode, endOffset);
}


















void nsFrameSelection::BidiLevelFromMove(nsIPresShell* aPresShell,
                                         nsIContent   *aNode,
                                         PRUint32      aContentOffset,
                                         PRUint32      aKeycode,
                                         HINT          aHint)
{
  switch (aKeycode) {

    
    case nsIDOMKeyEvent::DOM_VK_RIGHT:
    case nsIDOMKeyEvent::DOM_VK_LEFT:
    {
      nsPrevNextBidiLevels levels = GetPrevNextBidiLevels(aNode, aContentOffset,
                                                          aHint, PR_FALSE);

      if (HINTLEFT == aHint)
        SetCaretBidiLevel(levels.mLevelBefore);
      else
        SetCaretBidiLevel(levels.mLevelAfter);
      break;
    }
      








    default:
      UndefineCaretBidiLevel();
  }
}







void nsFrameSelection::BidiLevelFromClick(nsIContent *aNode,
                                          PRUint32    aContentOffset)
{
  nsIFrame* clickInFrame=nsnull;
  PRInt32 OffsetNotUsed;

  clickInFrame = GetFrameForNodeOffset(aNode, aContentOffset, mHint, &OffsetNotUsed);
  if (!clickInFrame)
    return;

  SetCaretBidiLevel(NS_GET_EMBEDDING_LEVEL(clickInFrame));
}


PRBool
nsFrameSelection::AdjustForMaintainedSelection(nsIContent *aContent,
                                               PRInt32     aOffset)
{
  
  
  
  
  
  if (!mMaintainRange)
    return PR_FALSE;

  nsCOMPtr<nsIDOMNode> rangenode;
  PRInt32 rangeOffset;
  mMaintainRange->GetStartContainer(getter_AddRefs(rangenode));
  mMaintainRange->GetStartOffset(&rangeOffset);

  nsCOMPtr<nsIDOMNode> domNode = do_QueryInterface(aContent);
  if (domNode)
  {
    PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
    nsCOMPtr<nsIDOMNSRange> nsrange = do_QueryInterface(mMaintainRange);
    if (nsrange)
    {
      PRBool insideSelection = PR_FALSE;
      nsrange->IsPointInRange(domNode, aOffset, &insideSelection);

      
      if (insideSelection)
      {
        mDomSelections[index]->Collapse(rangenode, rangeOffset);
        mMaintainRange->GetEndContainer(getter_AddRefs(rangenode));
        mMaintainRange->GetEndOffset(&rangeOffset);
        mDomSelections[index]->Extend(rangenode,rangeOffset);
        return PR_TRUE; 
      }
    }

    PRInt32 relativePosition = CompareDOMPoints(rangenode, rangeOffset,
                                                domNode, aOffset);
    
    if (relativePosition > 0
        && (mDomSelections[index]->GetDirection() == eDirNext))
    {
      mMaintainRange->GetEndContainer(getter_AddRefs(rangenode));
      mMaintainRange->GetEndOffset(&rangeOffset);
      mDomSelections[index]->Collapse(rangenode, rangeOffset);
    }
    else if (relativePosition < 0
             && (mDomSelections[index]->GetDirection() == eDirPrevious))
      mDomSelections[index]->Collapse(rangenode, rangeOffset);
  }

  return PR_FALSE;
}


nsresult
nsFrameSelection::HandleClick(nsIContent *aNewFocus,
                              PRUint32    aContentOffset,
                              PRUint32    aContentEndOffset,
                              PRBool      aContinueSelection, 
                              PRBool      aMultipleSelection,
                              PRBool      aHint) 
{
  if (!aNewFocus)
    return NS_ERROR_INVALID_ARG;

  InvalidateDesiredX();

  if (!aContinueSelection)
    mMaintainRange = nsnull;

  mHint = HINT(aHint);
  
  if (!mDragSelectingCells)
  {
    BidiLevelFromClick(aNewFocus, aContentOffset);
    PostReason(nsISelectionListener::MOUSEDOWN_REASON + nsISelectionListener::DRAG_REASON);
    if (aContinueSelection &&
        AdjustForMaintainedSelection(aNewFocus, aContentOffset))
      return NS_OK; 

    return TakeFocus(aNewFocus, aContentOffset, aContentEndOffset, aContinueSelection, aMultipleSelection);
  }
  
  return NS_OK;
}

void
nsFrameSelection::HandleDrag(nsIFrame *aFrame, nsPoint aPoint)
{
  if (!aFrame)
    return;

  nsresult result;
  nsIFrame *newFrame = 0;
  nsPoint   newPoint;

  result = ConstrainFrameAndPointToAnchorSubtree(aFrame, aPoint, &newFrame, newPoint);
  if (NS_FAILED(result))
    return;
  if (!newFrame)
    return;

  nsIFrame::ContentOffsets offsets =
      newFrame->GetContentOffsetsFromPoint(newPoint);
  if (!offsets.content)
    return;

  if ((newFrame->GetStateBits() & NS_FRAME_SELECTED_CONTENT) &&
       AdjustForMaintainedSelection(offsets.content, offsets.offset))
    return;

  
  if (mMaintainRange && 
      mMaintainedAmount != eSelectNoAmount) {    
    
    nsCOMPtr<nsIDOMNode> rangenode;
    PRInt32 rangeOffset;
    mMaintainRange->GetStartContainer(getter_AddRefs(rangenode));
    mMaintainRange->GetStartOffset(&rangeOffset);
    nsCOMPtr<nsIDOMNode> domNode = do_QueryInterface(offsets.content);
    PRInt32 relativePosition = CompareDOMPoints(rangenode, rangeOffset,
                                                domNode, offsets.offset);

    nsDirection direction = relativePosition > 0 ? eDirPrevious : eDirNext;
    nsSelectionAmount amount = mMaintainedAmount;
    if (amount == eSelectBeginLine && direction == eDirNext)
      amount = eSelectEndLine;

    PRInt32 offset;
    nsIFrame* frame = GetFrameForNodeOffset(offsets.content, offsets.offset, HINTRIGHT, &offset);
    nsPeekOffsetStruct pos;
    pos.SetData(amount, direction, offset, 0,
                PR_FALSE, mLimiter != nsnull, PR_FALSE, PR_FALSE);

    if (frame && NS_SUCCEEDED(frame->PeekOffset(&pos)) && pos.mResultContent) {
      offsets.content = pos.mResultContent;
      offsets.offset = pos.mContentOffset;
    }
  }
  
  
#ifdef VISUALSELECTION
  if (mShell->GetPresContext()->BidiEnabled()) {
    PRUint8 level;
    nsPeekOffsetStruct pos;
    
    
    pos.SetData(eSelectDir, eDirNext, startPos, 0,
                PR_TRUE, mLimiter != nsnull, PR_FALSE, PR_TRUE);
    mHint = HINT(beginOfContent);
    HINT saveHint = mHint;
    if (NS_GET_EMBEDDING_LEVEL(newFrame) & 1)
      mHint = (mHint==HINTLEFT) ? HINTRIGHT : HINTLEFT;
    pos.mResultContent = newContent;
    pos.mContentOffset = contentOffsetEnd;
    result = VisualSelectFrames(newFrame, pos);
    if (NS_FAILED(result))
      HandleClick(newContent, startPos, contentOffsetEnd, PR_TRUE,
                  PR_FALSE, beginOfContent);
    mHint = saveHint;
  }
  else
#endif 
    HandleClick(offsets.content, offsets.offset, offsets.offset,
                PR_TRUE, PR_FALSE, offsets.associateWithNext);
}

nsresult
nsFrameSelection::StartAutoScrollTimer(nsIView  *aView,
                                       nsPoint   aPoint,
                                       PRUint32  aDelay)
{
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  return mDomSelections[index]->StartAutoScrollTimer(mShell->GetPresContext(),
                                                     aView, aPoint, aDelay);
}

void
nsFrameSelection::StopAutoScrollTimer()
{
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  mDomSelections[index]->StopAutoScrollTimer();
}




nsresult
nsFrameSelection::TakeFocus(nsIContent *aNewFocus,
                            PRUint32    aContentOffset,
                            PRUint32    aContentEndOffset,
                            PRBool      aContinueSelection,
                            PRBool      aMultipleSelection)
{
  if (!aNewFocus)
    return NS_ERROR_NULL_POINTER;

  STATUS_CHECK_RETURN_MACRO();

  if (!IsValidSelectionPoint(this,aNewFocus))
    return NS_ERROR_FAILURE;

  
  mSelectingTableCellMode = 0;
  mDragSelectingCells = PR_FALSE;
  mStartSelectedCell = nsnull;
  mEndSelectedCell = nsnull;
  mAppendStartSelectedCell = nsnull;

  
  if (!aNewFocus->GetParent())
    return NS_ERROR_FAILURE;
  

  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  nsCOMPtr<nsIDOMNode> domNode = do_QueryInterface(aNewFocus);
  
  if (!aContinueSelection){ 
    PRUint32 batching = mBatching;
    PRBool changes = mChangesDuringBatching;
    mBatching = 1;

    if (aMultipleSelection){
      nsCOMPtr<nsIDOMRange> newRange;
      NS_NewRange(getter_AddRefs(newRange));

      newRange->SetStart(domNode,aContentOffset);
      newRange->SetEnd(domNode,aContentOffset);
      mDomSelections[index]->AddRange(newRange);
      mBatching = batching;
      mChangesDuringBatching = changes;
      mDomSelections[index]->SetOriginalAnchorPoint(domNode,aContentOffset);
    }
    else
    {
      PRBool oldDesiredXSet = mDesiredXSet; 
      mDomSelections[index]->Collapse(domNode, aContentOffset);
      mDesiredXSet = oldDesiredXSet; 
      mBatching = batching;
      mChangesDuringBatching = changes;
    }
    if (aContentEndOffset != aContentOffset)
      mDomSelections[index]->Extend(domNode,aContentEndOffset);

    
    
    
    

    PRInt16 displaySelection;
    nsresult result = mShell->GetSelectionFlags(&displaySelection);
    if (NS_FAILED(result))
      return result;

    
    if (displaySelection == nsISelectionDisplay::DISPLAY_ALL)
    {
      mCellParent = GetCellParent(domNode);
#ifdef DEBUG_TABLE_SELECTION
      if (mCellParent)
        printf(" * TakeFocus - Collapsing into new cell\n");
#endif
    }
  }
  else {
    
    if (aContinueSelection && domNode)
    {
      PRInt32 offset;
      nsIDOMNode *cellparent = GetCellParent(domNode);
      if (mCellParent && cellparent && cellparent != mCellParent) 
      {
#ifdef DEBUG_TABLE_SELECTION
printf(" * TakeFocus - moving into new cell\n");
#endif
        nsCOMPtr<nsIDOMNode> parent;
        nsCOMPtr<nsIContent> parentContent;
        nsMouseEvent event(PR_FALSE, 0, nsnull, nsMouseEvent::eReal);
        nsresult result;

        
        result = ParentOffset(mCellParent, getter_AddRefs(parent),&offset);
        parentContent = do_QueryInterface(parent);
        if (parentContent)
          result = HandleTableSelection(parentContent, offset, nsISelectionPrivate::TABLESELECTION_CELL, &event);

        
        result = ParentOffset(cellparent,getter_AddRefs(parent),&offset);
        parentContent = do_QueryInterface(parent);

        
        
        event.isShift = PR_FALSE; 
        if (parentContent)
        {
          mCellParent = cellparent;
          
          result = HandleTableSelection(parentContent, offset, nsISelectionPrivate::TABLESELECTION_CELL, &event);
        }
      }
      else
      {
        
        
        if (mDomSelections[index]->GetDirection() == eDirNext && aContentEndOffset > aContentOffset) 
        {
          mDomSelections[index]->Extend(domNode, aContentEndOffset);
        }
        else
          mDomSelections[index]->Extend(domNode, aContentOffset);
      }
    }
  }

  
  if (GetBatching())
    return NS_OK;
  return NotifySelectionListeners(nsISelectionController::SELECTION_NORMAL);
}



SelectionDetails*
nsFrameSelection::LookUpSelection(nsIContent *aContent, PRInt32 aContentOffset,
                                  PRInt32 aContentLength, PRBool aSlowCheck)
{
  if (!aContent || !mShell)
    return nsnull;

  SelectionDetails* details = nsnull;

  for (PRInt32 j = 0; j < nsISelectionController::NUM_SELECTIONTYPES; j++) {
    if (mDomSelections[j]) {
      mDomSelections[j]->LookUpSelection(aContent, aContentOffset,
          aContentLength, &details, (SelectionType)(1<<j), aSlowCheck);
    }
  }

  return details;
}

void
nsFrameSelection::SetMouseDownState(PRBool aState)
{
  if (mMouseDownState == aState)
    return;

  mMouseDownState = aState;
    
  if (!mMouseDownState)
  {
    PostReason(nsISelectionListener::MOUSEUP_REASON);
    NotifySelectionListeners(nsISelectionController::SELECTION_NORMAL); 
  }
}

nsISelection*
nsFrameSelection::GetSelection(SelectionType aType)
{
  PRInt8 index = GetIndexFromSelectionType(aType);
  if (index < 0)
    return nsnull;

  return NS_STATIC_CAST(nsISelection*, mDomSelections[index]);
}

nsresult
nsFrameSelection::ScrollSelectionIntoView(SelectionType   aType,
                                          SelectionRegion aRegion,
                                          PRBool          aIsSynchronous)
{
  PRInt8 index = GetIndexFromSelectionType(aType);
  if (index < 0)
    return NS_ERROR_INVALID_ARG;

  if (!mDomSelections[index])
    return NS_ERROR_NULL_POINTER;

  return mDomSelections[index]->ScrollIntoView(aRegion, aIsSynchronous);
}

nsresult
nsFrameSelection::RepaintSelection(SelectionType aType)
{
  PRInt8 index = GetIndexFromSelectionType(aType);
  if (index < 0)
    return NS_ERROR_INVALID_ARG;
  if (!mDomSelections[index])
    return NS_ERROR_NULL_POINTER;
  return mDomSelections[index]->Repaint(mShell->GetPresContext());
}
 
nsIFrame*
nsFrameSelection::GetFrameForNodeOffset(nsIContent *aNode,
                                        PRInt32     aOffset,
                                        HINT        aHint,
                                        PRInt32    *aReturnOffset)
{
  if (!aNode || !aReturnOffset)
    return nsnull;

  if (aOffset < 0)
    return nsnull;

  *aReturnOffset = aOffset;

  nsCOMPtr<nsIContent> theNode = aNode;

  if (aNode->IsNodeOfType(nsINode::eELEMENT))
  {
    PRInt32 childIndex  = 0;
    PRInt32 numChildren = theNode->GetChildCount();

    if (aHint == HINTLEFT)
    {
      if (aOffset > 0)
        childIndex = aOffset - 1;
      else
        childIndex = aOffset;
    }
    else 
    {
      if (aOffset >= numChildren)
      {
        if (numChildren > 0)
          childIndex = numChildren - 1;
        else
          childIndex = 0;
      }
      else
        childIndex = aOffset;
    }
    
    if (childIndex > 0 || numChildren > 0) {
      nsCOMPtr<nsIContent> childNode = theNode->GetChildAt(childIndex);

      if (!childNode)
        return nsnull;

      theNode = childNode;
    }

#ifdef DONT_DO_THIS_YET
    
    

    
    

    if (theNode->IsNodeOfType(nsINode::eELEMENT))
    {
      PRInt32 newOffset = 0;

      if (aOffset > childIndex)
      {
        numChildren = theNode->GetChildCount();

        newOffset = numChildren;
      }

      return GetFrameForNodeOffset(theNode, newOffset, aHint, aReturnOffset);
    }
    else
#endif 
    {
      
      

      nsCOMPtr<nsIDOMText> textNode = do_QueryInterface(theNode);

      if (textNode)
      {
        if (aOffset > childIndex)
        {
          PRUint32 textLength = 0;

          nsresult rv = textNode->GetLength(&textLength);
          if (NS_FAILED(rv))
            return nsnull;

          *aReturnOffset = (PRInt32)textLength;
        }
        else
          *aReturnOffset = 0;
      }
    }
  }
  
  nsIFrame* returnFrame = mShell->GetPrimaryFrameFor(theNode);
  if (!returnFrame)
    return nsnull;

  
  returnFrame->GetChildFrameContainingOffset(*aReturnOffset, aHint,
                                             &aOffset, &returnFrame);
  return returnFrame;
}

void
nsFrameSelection::CommonPageMove(PRBool aForward,
                                 PRBool aExtend,
                                 nsIScrollableView *aScrollableView)
{
  if (!aScrollableView)
    return;
  
  

  nsresult result;
  

  nsIFrame* mainframe = nsnull;

  
  nsIView *scrolledView;
  result = aScrollableView->GetScrolledView(scrolledView);

  if (NS_FAILED(result))
    return;

  if (scrolledView)
    mainframe = NS_STATIC_CAST(nsIFrame*, scrolledView->GetClientData());

  if (!mainframe)
    return;

  
  
  nsISelection* domSel = GetSelection(nsISelectionController::SELECTION_NORMAL);
  
  if (!domSel) 
    return;
  
  nsCOMPtr<nsICaret> caret;
  nsRect caretPos;
  PRBool isCollapsed;
  result = mShell->GetCaret(getter_AddRefs(caret));
  
  if (NS_FAILED(result)) 
    return;
  
  nsIView *caretView;
  result = caret->GetCaretCoordinates(nsICaret::eClosestViewCoordinates, domSel, &caretPos, &isCollapsed, &caretView);
  
  if (NS_FAILED(result)) 
    return;
  
  
  nsSize scrollDelta;
  aScrollableView->GetPageScrollDistances(&scrollDelta);

  if (aForward)
    caretPos.y += scrollDelta.height;
  else
    caretPos.y -= scrollDelta.height;

  
  if (caretView)
  {
    caretPos += caretView->GetOffsetTo(scrolledView);
  }
    
  
  nsPoint desiredPoint;
  desiredPoint.x = caretPos.x;
  desiredPoint.y = caretPos.y + caretPos.height/2;
  nsIFrame::ContentOffsets offsets =
      mainframe->GetContentOffsetsFromPoint(desiredPoint);

  if (!offsets.content)
    return;

  
  aScrollableView->ScrollByPages(0, aForward ? 1 : -1);

  
  HandleClick(offsets.content, offsets.offset,
              offsets.offset, aExtend, PR_FALSE, PR_TRUE);
}

nsresult
nsFrameSelection::CharacterMove(PRBool aForward, PRBool aExtend)
{
  if (aForward)
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_RIGHT,aExtend,eSelectCharacter);
  else
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_LEFT,aExtend,eSelectCharacter);
}

nsresult
nsFrameSelection::WordMove(PRBool aForward, PRBool aExtend)
{
  if (aForward)
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_RIGHT,aExtend,eSelectWord);
  else
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_LEFT,aExtend,eSelectWord);
}

nsresult
nsFrameSelection::WordExtendForDelete(PRBool aForward)
{
  if (aForward)
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_DELETE, PR_TRUE, eSelectWord);
  else
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_BACK_SPACE, PR_TRUE, eSelectWord);
}

nsresult
nsFrameSelection::LineMove(PRBool aForward, PRBool aExtend)
{
  if (aForward)
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_DOWN,aExtend,eSelectLine);
  else
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_UP,aExtend,eSelectLine);
}

nsresult
nsFrameSelection::IntraLineMove(PRBool aForward, PRBool aExtend)
{
  if (aForward)
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_END,aExtend,eSelectLine);
  else
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_HOME,aExtend,eSelectLine);
}

nsresult
nsFrameSelection::SelectAll()
{
  nsCOMPtr<nsIContent> rootContent;
  if (mLimiter)
  {
    rootContent = mLimiter;
  }
  else
  {
    nsIDocument *doc = mShell->GetDocument();
    if (!doc)
      return NS_ERROR_FAILURE;
    rootContent = doc->GetRootContent();
    if (!rootContent)
      return NS_ERROR_FAILURE;
  }
  PRInt32 numChildren = rootContent->GetChildCount();
  PostReason(nsISelectionListener::NO_REASON);
  return TakeFocus(mLimiter, 0, numChildren, PR_FALSE, PR_FALSE);
}



void
nsFrameSelection::StartBatchChanges()
{
  mBatching++;
}

void
nsFrameSelection::EndBatchChanges()
{
  mBatching--;
  NS_ASSERTION(mBatching >=0,"Bad mBatching");
  if (mBatching == 0 && mChangesDuringBatching){
    mChangesDuringBatching = PR_FALSE;
    NotifySelectionListeners(nsISelectionController::SELECTION_NORMAL);
  }
}


nsresult
nsFrameSelection::NotifySelectionListeners(SelectionType aType)
{
  PRInt8 index = GetIndexFromSelectionType(aType);
  if (index >=0)
  {
    return mDomSelections[index]->NotifySelectionListeners();
  }
  return NS_ERROR_FAILURE;
}



static PRBool IsCell(nsIContent *aContent)
{
  return ((aContent->Tag() == nsGkAtoms::td ||
           aContent->Tag() == nsGkAtoms::th) &&
          aContent->IsNodeOfType(nsINode::eHTML));
}

nsITableCellLayout* 
nsFrameSelection::GetCellLayout(nsIContent *aCellContent)
{
  
  nsIFrame *cellFrame = mShell->GetPrimaryFrameFor(aCellContent);
  if (!cellFrame)
    return nsnull;

  nsITableCellLayout *cellLayoutObject = nsnull;
  CallQueryInterface(cellFrame, &cellLayoutObject);

  return cellLayoutObject;
}

nsITableLayout* 
nsFrameSelection::GetTableLayout(nsIContent *aTableContent)
{
  
  nsIFrame *tableFrame = mShell->GetPrimaryFrameFor(aTableContent);
  if (!tableFrame)
    return nsnull;

  nsITableLayout *tableLayoutObject = nsnull;
  CallQueryInterface(tableFrame, &tableLayoutObject);

  return tableLayoutObject;
}

nsresult
nsFrameSelection::ClearNormalSelection()
{
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  return mDomSelections[index]->RemoveAllRanges();
}



nsresult
nsFrameSelection::HandleTableSelection(nsIContent *aParentContent, PRInt32 aContentOffset, PRInt32 aTarget, nsMouseEvent *aMouseEvent)
{
  NS_ENSURE_TRUE(aParentContent, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aMouseEvent, NS_ERROR_NULL_POINTER);

  if (mMouseDownState && mDragSelectingCells && (aTarget & nsISelectionPrivate::TABLESELECTION_TABLE))
  {
    
    
      return NS_OK;
  }

  nsCOMPtr<nsIDOMNode> parentNode = do_QueryInterface(aParentContent);
  if (!parentNode)
    return NS_ERROR_FAILURE;

  nsresult result = NS_OK;

  nsIContent *childContent = aParentContent->GetChildAt(aContentOffset);
  nsCOMPtr<nsIDOMNode> childNode = do_QueryInterface(childContent);
  if (!childNode)
    return NS_ERROR_FAILURE;

  
  
  
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  mDomSelections[index]->SetDirection(eDirNext);

  
  
  nsSelectionBatcher selectionBatcher(mDomSelections[index]);

  PRInt32 startRowIndex, startColIndex, curRowIndex, curColIndex;
  if (mMouseDownState && mDragSelectingCells)
  {
    
    if (aTarget != nsISelectionPrivate::TABLESELECTION_TABLE)
    {
      
      if (mEndSelectedCell == childContent)
        return NS_OK;

#ifdef DEBUG_TABLE_SELECTION
printf(" mStartSelectedCell = %x, mEndSelectedCell = %x, childContent = %x \n", mStartSelectedCell, mEndSelectedCell, childContent);
#endif
      
      
      
      
      

      if (mSelectingTableCellMode == nsISelectionPrivate::TABLESELECTION_ROW ||
          mSelectingTableCellMode == nsISelectionPrivate::TABLESELECTION_COLUMN)
      {
        if (mEndSelectedCell)
        {
          
          result = GetCellIndexes(mEndSelectedCell, startRowIndex, startColIndex);
          if (NS_FAILED(result)) return result;
          result = GetCellIndexes(childContent, curRowIndex, curColIndex);
          if (NS_FAILED(result)) return result;
        
#ifdef DEBUG_TABLE_SELECTION
printf(" curRowIndex = %d, startRowIndex = %d, curColIndex = %d, startColIndex = %d\n", curRowIndex, startRowIndex, curColIndex, startColIndex);
#endif
          if ((mSelectingTableCellMode == nsISelectionPrivate::TABLESELECTION_ROW && startRowIndex == curRowIndex) ||
              (mSelectingTableCellMode == nsISelectionPrivate::TABLESELECTION_COLUMN && startColIndex == curColIndex)) 
            return NS_OK;
        }
#ifdef DEBUG_TABLE_SELECTION
printf(" Dragged into a new column or row\n");
#endif
        
        return SelectRowOrColumn(childContent, mSelectingTableCellMode);
      }
      else if (mSelectingTableCellMode == nsISelectionPrivate::TABLESELECTION_CELL)
      {
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Dragged into a new cell\n");
#endif
        
        
        
        
        if (mStartSelectedCell && aMouseEvent->isShift)
        {
          result = GetCellIndexes(mStartSelectedCell, startRowIndex, startColIndex);
          if (NS_FAILED(result)) return result;
          result = GetCellIndexes(childContent, curRowIndex, curColIndex);
          if (NS_FAILED(result)) return result;
          
          if (startRowIndex == curRowIndex || 
              startColIndex == curColIndex)
          {
            
            mStartSelectedCell = nsnull;
            mDomSelections[index]->RemoveAllRanges();

            if (startRowIndex == curRowIndex)
              mSelectingTableCellMode = nsISelectionPrivate::TABLESELECTION_ROW;
            else
              mSelectingTableCellMode = nsISelectionPrivate::TABLESELECTION_COLUMN;

            return SelectRowOrColumn(childContent, mSelectingTableCellMode);
          }
        }
        
        
        return SelectBlockOfCells(mStartSelectedCell, childContent);
      }
    }
    
    return NS_OK;
  }
  else 
  {
    
    if (mMouseDownState)
    {
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Mouse down event\n");
#endif
      
      mUnselectCellOnMouseUp = nsnull;
      
      if (aTarget == nsISelectionPrivate::TABLESELECTION_CELL)
      {
        PRBool isSelected = PR_FALSE;

        
        nsCOMPtr<nsIDOMNode> previousCellNode;
        GetFirstSelectedCellAndRange(getter_AddRefs(previousCellNode), nsnull);
        if (previousCellNode)
        {
          

          
          nsIFrame  *cellFrame = mShell->GetPrimaryFrameFor(childContent);
          if (!cellFrame) return NS_ERROR_NULL_POINTER;
          result = cellFrame->GetSelected(&isSelected);
          if (NS_FAILED(result)) return result;
        }
        else
        {
          
          mDomSelections[index]->RemoveAllRanges();
        }
        mDragSelectingCells = PR_TRUE;    
        mSelectingTableCellMode = aTarget;
        
        mStartSelectedCell = childContent;
        
        mEndSelectedCell = childContent;
        
        if (isSelected)
        {
          
          mUnselectCellOnMouseUp = childContent;
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Saving mUnselectCellOnMouseUp\n");
#endif
        }
        else
        {
          
          
          nsCOMPtr<nsIContent> previousCellContent = do_QueryInterface(previousCellNode);
          if (previousCellContent && !IsInSameTable(previousCellContent, childContent, nsnull))
          {
            mDomSelections[index]->RemoveAllRanges();
            
            mSelectingTableCellMode = aTarget;
          }

          nsCOMPtr<nsIDOMElement> cellElement = do_QueryInterface(childContent);
          return SelectCellElement(cellElement);
        }

        return NS_OK;
      }
      else if (aTarget == nsISelectionPrivate::TABLESELECTION_TABLE)
      {
        
        
        
        mDragSelectingCells = PR_FALSE;
        mStartSelectedCell = nsnull;
        mEndSelectedCell = nsnull;

        
        mDomSelections[index]->RemoveAllRanges();
        return CreateAndAddRange(parentNode, aContentOffset);
      }
      else if (aTarget == nsISelectionPrivate::TABLESELECTION_ROW || aTarget == nsISelectionPrivate::TABLESELECTION_COLUMN)
      {
#ifdef DEBUG_TABLE_SELECTION
printf("aTarget == %d\n", aTarget);
#endif

        
        
        
        mDragSelectingCells = PR_TRUE;
      
        
        mStartSelectedCell = nsnull;
        mDomSelections[index]->RemoveAllRanges();
        
        mSelectingTableCellMode = aTarget;
        return SelectRowOrColumn(childContent, aTarget);
      }
    }
    else
    {
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Mouse UP event. mDragSelectingCells=%d, mStartSelectedCell=%d\n", mDragSelectingCells, mStartSelectedCell);
#endif
      
      PRInt32 rangeCount;
      result = mDomSelections[index]->GetRangeCount(&rangeCount);
      if (NS_FAILED(result)) 
        return result;

      if (rangeCount > 0 && aMouseEvent->isShift && 
          mAppendStartSelectedCell && mAppendStartSelectedCell != childContent)
      {
        
        mDragSelectingCells = PR_FALSE;
        return SelectBlockOfCells(mAppendStartSelectedCell, childContent);
      }

      if (mDragSelectingCells)
        mAppendStartSelectedCell = mStartSelectedCell;
        
      mDragSelectingCells = PR_FALSE;
      mStartSelectedCell = nsnull;
      mEndSelectedCell = nsnull;

      
      
      PRBool doMouseUpAction = PR_FALSE;
#if defined(XP_MAC) || defined(XP_MACOSX)
      doMouseUpAction = aMouseEvent->isMeta;
#else
      doMouseUpAction = aMouseEvent->isControl;
#endif
      if (!doMouseUpAction)
      {
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Ending cell selection on mouseup: mAppendStartSelectedCell=%d\n", mAppendStartSelectedCell);
#endif
        return NS_OK;
      }
      
      
      if( childContent == mUnselectCellOnMouseUp)
      {
        
        nsCOMPtr<nsIDOMNode> previousCellParent;
        nsCOMPtr<nsIDOMRange> range;
        PRInt32 offset;
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Unselecting mUnselectCellOnMouseUp; rangeCount=%d\n", rangeCount);
#endif
        for( PRInt32 i = 0; i < rangeCount; i++)
        {
          result = mDomSelections[index]->GetRangeAt(i, getter_AddRefs(range));
          if (NS_FAILED(result)) return result;
          if (!range) return NS_ERROR_NULL_POINTER;

          nsCOMPtr<nsIDOMNode> parent;
          result = range->GetStartContainer(getter_AddRefs(parent));
          if (NS_FAILED(result)) return result;
          if (!parent) return NS_ERROR_NULL_POINTER;

          range->GetStartOffset(&offset);
          
          nsCOMPtr<nsIContent> parentContent = do_QueryInterface(parent);
          nsCOMPtr<nsIContent> child = parentContent->GetChildAt(offset);
          if (child && IsCell(child))
            previousCellParent = parent;

          
          if (!previousCellParent) break;
        
          if (previousCellParent == parentNode && offset == aContentOffset)
          {
            
            if (rangeCount == 1)
            {
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Unselecting single selected cell\n");
#endif
              
              
              mStartSelectedCell = nsnull;
              mEndSelectedCell = nsnull;
              mAppendStartSelectedCell = nsnull;
              
              
              
              return mDomSelections[index]->Collapse(childNode, 0);
            }
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Removing cell from multi-cell selection\n");
#endif
            
            
            if (childContent == mAppendStartSelectedCell)
               mAppendStartSelectedCell = nsnull;

            
            return mDomSelections[index]->RemoveRange(range);
          }
        }
        mUnselectCellOnMouseUp = nsnull;
      }
    }
  }
  return result;
}

nsresult
nsFrameSelection::SelectBlockOfCells(nsIContent *aStartCell, nsIContent *aEndCell)
{
  NS_ENSURE_TRUE(aStartCell, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aEndCell, NS_ERROR_NULL_POINTER);
  mEndSelectedCell = aEndCell;

  nsCOMPtr<nsIContent> startCell;
  nsresult result = NS_OK;

  
  nsCOMPtr<nsIContent> table;
  if (!IsInSameTable(aStartCell, aEndCell, getter_AddRefs(table)))
    return NS_OK;

  
  PRInt32 startRowIndex, startColIndex, endRowIndex, endColIndex;
  result = GetCellIndexes(aStartCell, startRowIndex, startColIndex);
  if(NS_FAILED(result)) return result;
  result = GetCellIndexes(aEndCell, endRowIndex, endColIndex);
  if(NS_FAILED(result)) return result;

  
  
  nsITableLayout *tableLayoutObject = GetTableLayout(table);
  if (!tableLayoutObject) return NS_ERROR_FAILURE;

  PRInt32 curRowIndex, curColIndex;

  if (mDragSelectingCells)
  {
    

    PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);

    nsCOMPtr<nsIDOMNode> cellNode;
    nsCOMPtr<nsIDOMRange> range;
    result = GetFirstSelectedCellAndRange(getter_AddRefs(cellNode), getter_AddRefs(range));
    if (NS_FAILED(result)) return result;

    PRInt32 minRowIndex = PR_MIN(startRowIndex, endRowIndex);
    PRInt32 maxRowIndex = PR_MAX(startRowIndex, endRowIndex);
    PRInt32 minColIndex = PR_MIN(startColIndex, endColIndex);
    PRInt32 maxColIndex = PR_MAX(startColIndex, endColIndex);

    while (cellNode)
    {
      nsCOMPtr<nsIContent> childContent = do_QueryInterface(cellNode);
      result = GetCellIndexes(childContent, curRowIndex, curColIndex);
      if (NS_FAILED(result)) return result;

#ifdef DEBUG_TABLE_SELECTION
if (!range)
printf("SelectBlockOfCells -- range is null\n");
#endif
      if (range &&
          (curRowIndex < minRowIndex || curRowIndex > maxRowIndex || 
           curColIndex < minColIndex || curColIndex > maxColIndex))
      {
        mDomSelections[index]->RemoveRange(range);
        
        mSelectedCellIndex--;
      }    
      result = GetNextSelectedCellAndRange(getter_AddRefs(cellNode), getter_AddRefs(range));
      if (NS_FAILED(result)) return result;
    }
  }

  nsCOMPtr<nsIDOMElement> cellElement;
  PRInt32 rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;

  
  
  PRInt32 row = startRowIndex;
  while(PR_TRUE)
  {
    PRInt32 col = startColIndex;
    while(PR_TRUE)
    {
      result = tableLayoutObject->GetCellDataAt(row, col, *getter_AddRefs(cellElement),
                                                curRowIndex, curColIndex, rowSpan, colSpan, 
                                                actualRowSpan, actualColSpan, isSelected);
      if (NS_FAILED(result)) return result;

      NS_ASSERTION(actualColSpan, "!actualColSpan is 0!");

      
      if (!isSelected && cellElement && row == curRowIndex && col == curColIndex)
      {
        result = SelectCellElement(cellElement);
        if (NS_FAILED(result)) return result;
      }
      
      if (col == endColIndex) break;

      if (startColIndex < endColIndex)
        col ++;
      else
        col--;
    };
    if (row == endRowIndex) break;

    if (startRowIndex < endRowIndex)
      row++;
    else
      row--;
  };
  return result;
}

nsresult
nsFrameSelection::SelectRowOrColumn(nsIContent *aCellContent, PRUint32 aTarget)
{
  if (!aCellContent) return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIContent> table;
  nsresult result = GetParentTable(aCellContent, getter_AddRefs(table));
  if (NS_FAILED(result)) return PR_FALSE;
  if (!table) return NS_ERROR_NULL_POINTER;

  
  
  
  nsITableLayout *tableLayout = GetTableLayout(table);
  if (!tableLayout) return NS_ERROR_FAILURE;
  nsITableCellLayout *cellLayout = GetCellLayout(aCellContent);
  if (!cellLayout) return NS_ERROR_FAILURE;

  
  PRInt32 rowIndex, colIndex, curRowIndex, curColIndex;
  result = cellLayout->GetCellIndexes(rowIndex, colIndex);
  if (NS_FAILED(result)) return result;

  
  
  if (aTarget == nsISelectionPrivate::TABLESELECTION_ROW)
    colIndex = 0;
  if (aTarget == nsISelectionPrivate::TABLESELECTION_COLUMN)
    rowIndex = 0;

  nsCOMPtr<nsIDOMElement> cellElement;
  nsCOMPtr<nsIDOMElement> firstCell;
  nsCOMPtr<nsIDOMElement> lastCell;
  PRInt32 rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool isSelected;

  do {
    
    result = tableLayout->GetCellDataAt(rowIndex, colIndex, *getter_AddRefs(cellElement),
                                        curRowIndex, curColIndex, rowSpan, colSpan, 
                                        actualRowSpan, actualColSpan, isSelected);
    if (NS_FAILED(result)) return result;
    if (cellElement)
    {
      NS_ASSERTION(actualRowSpan > 0 && actualColSpan> 0, "SelectRowOrColumn: Bad rowspan or colspan\n");
      if (!firstCell)
        firstCell = cellElement;

      lastCell = cellElement;

      
      if (aTarget == nsISelectionPrivate::TABLESELECTION_ROW)
        colIndex += actualColSpan;
      else
        rowIndex += actualRowSpan;
    }
  }
  while (cellElement);

  
  
  
  if (firstCell && lastCell)
  {
    if (!mStartSelectedCell)
    {
      
      result = SelectCellElement(firstCell);
      if (NS_FAILED(result)) return result;
      mStartSelectedCell = do_QueryInterface(firstCell);
    }
    nsCOMPtr<nsIContent> lastCellContent = do_QueryInterface(lastCell);
    result = SelectBlockOfCells(mStartSelectedCell, lastCellContent);

    
    
    mEndSelectedCell = aCellContent;
    return result;
  }

#if 0


  do {
    
    result = tableLayout->GetCellDataAt(rowIndex, colIndex,
                                        getter_AddRefs(cellElement),
                                        curRowIndex, curColIndex,
                                        rowSpan, colSpan,
                                        actualRowSpan, actualColSpan,
                                        isSelected);
    if (NS_FAILED(result)) return result;
    
    if (!cellElement) break;


    
    NS_ASSERTION(actualColSpan, "actualColSpan is 0!");
    NS_ASSERTION(actualRowSpan, "actualRowSpan is 0!");
    
    
    if (!isSelected && rowIndex == curRowIndex && colIndex == curColIndex)
    {
      result = SelectCellElement(cellElement);
      if (NS_FAILED(result)) return result;
    }
    
    if (aTarget == nsISelectionPrivate::TABLESELECTION_ROW)
      colIndex += actualColSpan;
    else
      rowIndex += actualRowSpan;
  }
  while (cellElement);
#endif

  return NS_OK;
}

nsresult 
nsFrameSelection::GetFirstCellNodeInRange(nsIDOMRange *aRange,
                                          nsIDOMNode **aCellNode)
{
  if (!aRange || !aCellNode) return NS_ERROR_NULL_POINTER;

  *aCellNode = nsnull;

  nsCOMPtr<nsIDOMNode> startParent;
  nsresult result = aRange->GetStartContainer(getter_AddRefs(startParent));
  if (NS_FAILED(result))
    return result;
  if (!startParent)
    return NS_ERROR_FAILURE;

  PRInt32 offset;
  result = aRange->GetStartOffset(&offset);
  if (NS_FAILED(result))
    return result;

  nsCOMPtr<nsIContent> parentContent = do_QueryInterface(startParent);
  nsCOMPtr<nsIContent> childContent = parentContent->GetChildAt(offset);
  if (!childContent)
    return NS_ERROR_NULL_POINTER;
  
  if (!IsCell(childContent)) return NS_OK;

  nsCOMPtr<nsIDOMNode> childNode = do_QueryInterface(childContent);
  if (childNode)
  {
    *aCellNode = childNode;
    NS_ADDREF(*aCellNode);
  }
  return NS_OK;
}

nsresult 
nsFrameSelection::GetFirstSelectedCellAndRange(nsIDOMNode  **aCell,
                                               nsIDOMRange **aRange)
{
  if (!aCell) return NS_ERROR_NULL_POINTER;
  *aCell = nsnull;

  
  if (aRange)
    *aRange = nsnull;

  nsCOMPtr<nsIDOMRange> firstRange;
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  nsresult result = mDomSelections[index]->GetRangeAt(0, getter_AddRefs(firstRange));
  if (NS_FAILED(result)) return result;
  if (!firstRange) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> cellNode;
  result = GetFirstCellNodeInRange(firstRange, getter_AddRefs(cellNode));
  if (NS_FAILED(result)) return result;
  if (!cellNode) return NS_OK;

  *aCell = cellNode;
  NS_ADDREF(*aCell);
  if (aRange)
  {
    *aRange = firstRange;
    NS_ADDREF(*aRange);
  }

  
  mSelectedCellIndex = 1;

  return NS_OK;
}

nsresult
nsFrameSelection::GetNextSelectedCellAndRange(nsIDOMNode  **aCell,
                                              nsIDOMRange **aRange)
{
  if (!aCell) return NS_ERROR_NULL_POINTER;
  *aCell = nsnull;

  
  if (aRange)
    *aRange = nsnull;

  PRInt32 rangeCount;
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  nsresult result = mDomSelections[index]->GetRangeCount(&rangeCount);
  if (NS_FAILED(result)) return result;

  
  if (mSelectedCellIndex >= rangeCount) 
  {
    
    
    
    return NS_OK;
  }

  
  nsCOMPtr<nsIDOMRange> range;
  result = mDomSelections[index]->GetRangeAt(mSelectedCellIndex, getter_AddRefs(range));
  if (NS_FAILED(result)) return result;
  if (!range) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> cellNode;
  result = GetFirstCellNodeInRange(range, getter_AddRefs(cellNode));
  if (NS_FAILED(result)) return result;
  
  if (!cellNode) return NS_OK;

  *aCell = cellNode;
  NS_ADDREF(*aCell);
  if (aRange)
  {
    *aRange = range;
    NS_ADDREF(*aRange);
  }

  
  mSelectedCellIndex++;

  return NS_OK;
}

nsresult
nsFrameSelection::GetCellIndexes(nsIContent *aCell,
                                 PRInt32    &aRowIndex,
                                 PRInt32    &aColIndex)
{
  if (!aCell) return NS_ERROR_NULL_POINTER;

  aColIndex=0; 
  aRowIndex=0;

  nsITableCellLayout *cellLayoutObject = GetCellLayout(aCell);
  if (!cellLayoutObject)  return NS_ERROR_FAILURE;
  return cellLayoutObject->GetCellIndexes(aRowIndex, aColIndex);
}

PRBool 
nsFrameSelection::IsInSameTable(nsIContent  *aContent1,
                                nsIContent  *aContent2,
                                nsIContent **aTable)
{
  if (!aContent1 || !aContent2) return PR_FALSE;
  
  
  if(aTable) *aTable = nsnull;
  
  nsCOMPtr<nsIContent> tableNode1;
  nsCOMPtr<nsIContent> tableNode2;

  nsresult result = GetParentTable(aContent1, getter_AddRefs(tableNode1));
  if (NS_FAILED(result)) return PR_FALSE;
  result = GetParentTable(aContent2, getter_AddRefs(tableNode2));
  if (NS_FAILED(result)) return PR_FALSE;

  
  if (tableNode1 && (tableNode1 == tableNode2))
  {
    if (aTable)
    {
      *aTable = tableNode1;
      NS_ADDREF(*aTable);
    }
    return PR_TRUE;;
  }
  return PR_FALSE;
}

nsresult
nsFrameSelection::GetParentTable(nsIContent *aCell, nsIContent **aTable)
{
  if (!aCell || !aTable) {
    return NS_ERROR_NULL_POINTER;
  }

  for (nsIContent* parent = aCell->GetParent(); parent;
       parent = parent->GetParent()) {
    if (parent->Tag() == nsGkAtoms::table &&
        parent->IsNodeOfType(nsINode::eHTML)) {
      *aTable = parent;
      NS_ADDREF(*aTable);

      return NS_OK;
    }
  }

  return NS_OK;
}

nsresult
nsFrameSelection::SelectCellElement(nsIDOMElement *aCellElement)
{
  nsCOMPtr<nsIContent> cellContent = do_QueryInterface(aCellElement);

  if (!cellContent) {
    return NS_ERROR_FAILURE;
  }

  nsIContent *parent = cellContent->GetParent();
  nsCOMPtr<nsIDOMNode> parentNode(do_QueryInterface(parent));
  if (!parentNode) {
    return NS_ERROR_FAILURE;
  }

  
  PRInt32 offset = parent->IndexOf(cellContent);

  return CreateAndAddRange(parentNode, offset);
}

nsresult
nsTypedSelection::getTableCellLocationFromRange(nsIDOMRange *aRange, PRInt32 *aSelectionType, PRInt32 *aRow, PRInt32 *aCol)
{
  if (!aRange || !aSelectionType || !aRow || !aCol)
    return NS_ERROR_NULL_POINTER;

  *aSelectionType = nsISelectionPrivate::TABLESELECTION_NONE;
  *aRow = 0;
  *aCol = 0;

  
  if (!mFrameSelection) return NS_OK;

  nsresult result = GetTableSelectionType(aRange, aSelectionType);
  if (NS_FAILED(result)) return result;
  
  
  
  if (*aSelectionType  != nsISelectionPrivate::TABLESELECTION_CELL)
    return NS_OK;

  
  
  
  nsCOMPtr<nsIDOMNode> startNode;
  result = aRange->GetStartContainer(getter_AddRefs(startNode));
  if (NS_FAILED(result))
    return result;

  nsCOMPtr<nsIContent> content(do_QueryInterface(startNode));
  if (!content)
    return NS_ERROR_FAILURE;
  PRInt32 startOffset;
  result = aRange->GetStartOffset(&startOffset);
  if (NS_FAILED(result))
    return result;

  nsIContent *child = content->GetChildAt(startOffset);
  if (!child)
    return NS_ERROR_FAILURE;

  
  nsITableCellLayout *cellLayout = mFrameSelection->GetCellLayout(child);
  if (NS_FAILED(result))
    return result;
  if (!cellLayout)
    return NS_ERROR_FAILURE;

  return cellLayout->GetCellIndexes(*aRow, *aCol);
}

nsresult
nsTypedSelection::addTableCellRange(nsIDOMRange *aRange, PRBool *aDidAddRange)
{
  if (!aDidAddRange)
    return NS_ERROR_NULL_POINTER;

  *aDidAddRange = PR_FALSE;

  if (!mFrameSelection)
    return NS_OK;

  if (!aRange)
    return NS_ERROR_NULL_POINTER;

  nsresult result;

  
  PRInt32 newRow, newCol, tableMode;
  result = getTableCellLocationFromRange(aRange, &tableMode, &newRow, &newCol);
  if (NS_FAILED(result)) return result;
  
  
  if (tableMode != nsISelectionPrivate::TABLESELECTION_CELL)
  {
    mFrameSelection->mSelectingTableCellMode = tableMode;
    
    return NS_OK;
  }
  
  
  
  if (mFrameSelection->mSelectingTableCellMode == TABLESELECTION_NONE)
    mFrameSelection->mSelectingTableCellMode = tableMode;

  return AddItem(aRange);
}


NS_IMETHODIMP
nsTypedSelection::GetTableSelectionType(nsIDOMRange* aRange, PRInt32* aTableSelectionType)
{
  if (!aRange || !aTableSelectionType)
    return NS_ERROR_NULL_POINTER;
  
  *aTableSelectionType = nsISelectionPrivate::TABLESELECTION_NONE;
 
  
  if(!mFrameSelection) return NS_OK;

  nsCOMPtr<nsIDOMNode> startNode;
  nsresult result = aRange->GetStartContainer(getter_AddRefs(startNode));
  if (NS_FAILED(result)) return result;
  if (!startNode) return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIDOMNode> endNode;
  result = aRange->GetEndContainer(getter_AddRefs(endNode));
  if (NS_FAILED(result)) return result;
  if (!endNode) return NS_ERROR_FAILURE;

  
  if (startNode != endNode) return NS_OK;

  nsCOMPtr<nsINode> node = do_QueryInterface(startNode);
  if (!node) return NS_ERROR_FAILURE;

  
  
  if (!node->IsNodeOfType(nsINode::eELEMENT))
    return NS_OK; 

  nsCOMPtr<nsIContent> content = do_QueryInterface(startNode);
  NS_ASSERTION(content, "No content here?");
  
  PRInt32 startOffset;
  PRInt32 endOffset;
  result = aRange->GetEndOffset(&endOffset);
  if (NS_FAILED(result)) return result;
  result = aRange->GetStartOffset(&startOffset);
  if (NS_FAILED(result)) return result;

  
  if ((endOffset - startOffset) != 1)
    return NS_OK;

  if (!content->IsNodeOfType(nsINode::eHTML)) {
    return NS_OK;
  }

  nsIAtom *tag = content->Tag();

  if (tag == nsGkAtoms::tr)
  {
    *aTableSelectionType = nsISelectionPrivate::TABLESELECTION_CELL;
  }
  else 
  {
    nsIContent *child = content->GetChildAt(startOffset);
    if (!child)
      return NS_ERROR_FAILURE;

    tag = child->Tag();

    if (tag == nsGkAtoms::table)
      *aTableSelectionType = nsISelectionPrivate::TABLESELECTION_TABLE;
    else if (tag == nsGkAtoms::tr)
      *aTableSelectionType = nsISelectionPrivate::TABLESELECTION_ROW;
  }

  return result;
}

nsresult
nsFrameSelection::CreateAndAddRange(nsIDOMNode *aParentNode, PRInt32 aOffset)
{
  if (!aParentNode) return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIDOMRange> range;
  NS_NewRange(getter_AddRefs(range));
  if (!range) return NS_ERROR_OUT_OF_MEMORY;

  
  nsresult result = range->SetStart(aParentNode, aOffset);
  if (NS_FAILED(result)) return result;
  result = range->SetEnd(aParentNode, aOffset+1);
  if (NS_FAILED(result)) return result;
  
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  return mDomSelections[index]->AddRange(range);
}






#ifdef XP_MAC
#pragma mark -
#endif





nsresult
nsFrameSelection::DeleteFromDocument()
{
  nsresult res;

  
  
  
  PRBool isCollapsed;
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  mDomSelections[index]->GetIsCollapsed( &isCollapsed);
  if (isCollapsed)
  {
    
    if (mDomSelections[index]->FetchFocusOffset() > 0)
    {
      mDomSelections[index]->Extend(mDomSelections[index]->FetchFocusNode(), mDomSelections[index]->FetchFocusOffset() - 1);
    }
    else
    {
      
      printf("Sorry, don't know how to delete across frame boundaries yet\n");
      return NS_ERROR_NOT_IMPLEMENTED;
    }
  }

  
  nsSelectionIterator iter(mDomSelections[index]);
  res = iter.First();
  if (NS_FAILED(res))
    return res;

  nsCOMPtr<nsIDOMRange> range;
  while (iter.IsDone())
  {
    res = iter.CurrentItem(NS_STATIC_CAST(nsIDOMRange**, getter_AddRefs(range)));
    if (NS_FAILED(res))
      return res;
    res = range->DeleteContents();
    if (NS_FAILED(res))
      return res;
    iter.Next();
  }

  
  
  
  if (isCollapsed)
    mDomSelections[index]->Collapse(mDomSelections[index]->FetchAnchorNode(), mDomSelections[index]->FetchAnchorOffset()-1);
  else if (mDomSelections[index]->FetchAnchorOffset() > 0)
    mDomSelections[index]->Collapse(mDomSelections[index]->FetchAnchorNode(), mDomSelections[index]->FetchAnchorOffset());
#ifdef DEBUG
  else
    printf("Don't know how to set selection back past frame boundary\n");
#endif

  return NS_OK;
}

void
nsFrameSelection::SetDelayedCaretData(nsMouseEvent *aMouseEvent)
{
  if (aMouseEvent)
  {
    mDelayedMouseEventValid = PR_TRUE;
    mDelayedMouseEvent      = *aMouseEvent;

    
    mDelayedMouseEvent.widget = nsnull;
  }
  else
    mDelayedMouseEventValid = PR_FALSE;
}

nsMouseEvent*
nsFrameSelection::GetDelayedCaretData()
{
  if (mDelayedMouseEventValid)
    return &mDelayedMouseEvent;
  
  return nsnull;
}



#if 0
#pragma mark -
#endif





nsTypedSelection::nsTypedSelection(nsFrameSelection *aList)
{
  mFrameSelection = aList;
  mFixupState = PR_FALSE;
  mDirection = eDirNext;
  mCachedOffsetForFrame = nsnull;
}


nsTypedSelection::nsTypedSelection()
{
  mFrameSelection = nsnull;
  mFixupState = PR_FALSE;
  mDirection = eDirNext;
  mCachedOffsetForFrame = nsnull;
}



nsTypedSelection::~nsTypedSelection()
{
  DetachFromPresentation();
}
  
void nsTypedSelection::DetachFromPresentation() {
  setAnchorFocusRange(-1);

  if (mAutoScrollTimer) {
    mAutoScrollTimer->Stop();
    mAutoScrollTimer = nsnull;
  }

  mScrollEvent.Revoke();

  if (mCachedOffsetForFrame) {
    delete mCachedOffsetForFrame;
    mCachedOffsetForFrame = nsnull;
  }

  mFrameSelection = nsnull;
}



NS_INTERFACE_MAP_BEGIN(nsTypedSelection)
  NS_INTERFACE_MAP_ENTRY(nsISelection)
  NS_INTERFACE_MAP_ENTRY(nsISelection2)
  NS_INTERFACE_MAP_ENTRY(nsISelectionPrivate)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISelection)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(Selection)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsTypedSelection)
NS_IMPL_RELEASE(nsTypedSelection)


NS_IMETHODIMP
nsTypedSelection::SetPresShell(nsIPresShell *aPresShell)
{
  mPresShellWeak = do_GetWeakReference(aPresShell);
  return NS_OK;
}



NS_IMETHODIMP
nsTypedSelection::GetAnchorNode(nsIDOMNode** aAnchorNode)
{
  if (!aAnchorNode)
    return NS_ERROR_NULL_POINTER;
  *aAnchorNode = nsnull;
  if(!mAnchorFocusRange)
    return NS_OK;
   
  nsresult result;
  if (GetDirection() == eDirNext){
    result = mAnchorFocusRange->GetStartContainer(aAnchorNode);
  }
  else{
    result = mAnchorFocusRange->GetEndContainer(aAnchorNode);
  }
  return result;
}

NS_IMETHODIMP
nsTypedSelection::GetAnchorOffset(PRInt32* aAnchorOffset)
{
  if (!aAnchorOffset)
    return NS_ERROR_NULL_POINTER;
  *aAnchorOffset = nsnull;
  if(!mAnchorFocusRange)
    return NS_OK;

  nsresult result;
  if (GetDirection() == eDirNext){
    result = mAnchorFocusRange->GetStartOffset(aAnchorOffset);
  }
  else{
    result = mAnchorFocusRange->GetEndOffset(aAnchorOffset);
  }
  return result;
}


NS_IMETHODIMP
nsTypedSelection::GetFocusNode(nsIDOMNode** aFocusNode)
{
  if (!aFocusNode)
    return NS_ERROR_NULL_POINTER;
  *aFocusNode = nsnull;
  if(!mAnchorFocusRange)
    return NS_OK;

  nsresult result;
  if (GetDirection() == eDirNext){
    result = mAnchorFocusRange->GetEndContainer(aFocusNode);
  }
  else{
    result = mAnchorFocusRange->GetStartContainer(aFocusNode);
  }

  return result;
}

NS_IMETHODIMP nsTypedSelection::GetFocusOffset(PRInt32* aFocusOffset)
{
  if (!aFocusOffset)
    return NS_ERROR_NULL_POINTER;
  *aFocusOffset = nsnull;
  if(!mAnchorFocusRange)
    return NS_OK;

   nsresult result;
  if (GetDirection() == eDirNext){
    result = mAnchorFocusRange->GetEndOffset(aFocusOffset);
  }
  else{
    result = mAnchorFocusRange->GetStartOffset(aFocusOffset);
  }
  return result;
}


void nsTypedSelection::setAnchorFocusRange(PRInt32 indx)
{
  if (indx >= (PRInt32)mRanges.Length())
    return;
  if (indx < 0) 
  {
    mAnchorFocusRange = nsnull;
  }
  else{
    mAnchorFocusRange = mRanges[indx].mRange;
  }
}



nsIDOMNode*
nsTypedSelection::FetchAnchorNode()
{  
  nsCOMPtr<nsIDOMNode>returnval;
  GetAnchorNode(getter_AddRefs(returnval));
  return returnval;
}



PRInt32
nsTypedSelection::FetchAnchorOffset()
{
  PRInt32 returnval;
  if (NS_SUCCEEDED(GetAnchorOffset(&returnval)))
    return returnval;
  return 0;
}



nsIDOMNode*
nsTypedSelection::FetchOriginalAnchorNode()  
{
  nsCOMPtr<nsIDOMNode>returnval;
  PRInt32 unused;
  GetOriginalAnchorPoint(getter_AddRefs(returnval),  &unused);
  return returnval;
}



PRInt32
nsTypedSelection::FetchOriginalAnchorOffset()
{
  nsCOMPtr<nsIDOMNode>unused;
  PRInt32 returnval;
  if (NS_SUCCEEDED(GetOriginalAnchorPoint(getter_AddRefs(unused), &returnval)))
    return returnval;
  return NS_OK;
}



nsIDOMNode*
nsTypedSelection::FetchFocusNode()
{   
  nsCOMPtr<nsIDOMNode>returnval;
  GetFocusNode(getter_AddRefs(returnval));
  return returnval;
}



PRInt32
nsTypedSelection::FetchFocusOffset()
{
  PRInt32 returnval;
  if (NS_SUCCEEDED(GetFocusOffset(&returnval)))
    return returnval;
  return NS_OK;
}



nsIDOMNode*
nsTypedSelection::FetchStartParent(nsIDOMRange *aRange)   
{
  if (!aRange)
    return nsnull;
  nsCOMPtr<nsIDOMNode> returnval;
  aRange->GetStartContainer(getter_AddRefs(returnval));
  return returnval;
}



PRInt32
nsTypedSelection::FetchStartOffset(nsIDOMRange *aRange)
{
  if (!aRange)
    return nsnull;
  PRInt32 returnval;
  if (NS_SUCCEEDED(aRange->GetStartOffset(&returnval)))
    return returnval;
  return 0;
}



nsIDOMNode*
nsTypedSelection::FetchEndParent(nsIDOMRange *aRange)     
{
  if (!aRange)
    return nsnull;
  nsCOMPtr<nsIDOMNode> returnval;
  aRange->GetEndContainer(getter_AddRefs(returnval));
  return returnval;
}



PRInt32
nsTypedSelection::FetchEndOffset(nsIDOMRange *aRange)
{
  if (!aRange)
    return nsnull;
  PRInt32 returnval;
  if (NS_SUCCEEDED(aRange->GetEndOffset(&returnval)))
    return returnval;
  return 0;
}

static nsresult
CompareToRangeStart(nsIDOMNode* aCompareNode, PRInt32 aCompareOffset,
                    nsIDOMRange* aRange, PRInt32* cmp)
{
  nsCOMPtr<nsIDOMNode> startNode;
  nsresult rv = aRange->GetStartContainer(getter_AddRefs(startNode));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 startOffset;
  rv = aRange->GetStartOffset(&startOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  *cmp = CompareDOMPoints(aCompareNode, aCompareOffset,
                          startNode, startOffset);
  return NS_OK;
}

static nsresult
CompareToRangeEnd(nsIDOMNode* aCompareNode, PRInt32 aCompareOffset,
                  nsIDOMRange* aRange, PRInt32* cmp)
{
  nsCOMPtr<nsIDOMNode> endNode;
  nsresult rv = aRange->GetEndContainer(getter_AddRefs(endNode));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 endOffset;
  rv = aRange->GetEndOffset(&endOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  *cmp = CompareDOMPoints(aCompareNode, aCompareOffset,
                          endNode, endOffset);
  return NS_OK;
}

#ifdef DEBUG


PRBool
nsTypedSelection::ValidateRanges()
{
  if (mRanges.Length() != mRangeEndings.Length()) {
    NS_NOTREACHED("Lengths don't match");
    return PR_FALSE;
  }

  
  PRUint32 i;
  for (i = 0; i < mRanges.Length(); i ++) {
    if (mRanges[i].mEndIndex < 0 ||
        mRanges[i].mEndIndex >= (PRInt32)mRangeEndings.Length()) {
      NS_NOTREACHED("Sorted index is out of bounds");
      return PR_FALSE;
    }
    if (mRangeEndings[mRanges[i].mEndIndex] != (PRInt32)i) {
      NS_NOTREACHED("Beginning or ending isn't correct");
      return PR_FALSE;
    }
  }

  
  for (i = 0; i < mRangeEndings.Length(); i ++) {
    if (mRangeEndings[i] < 0 ||
        mRangeEndings[i] >= (PRInt32)mRanges.Length()) {
      NS_NOTREACHED("Ending refers to invalid index");
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}
#endif














nsresult
nsTypedSelection::FindInsertionPoint(
    const nsTArray<PRInt32>* aRemappingArray,
    nsIDOMNode* aPointNode, PRInt32 aPointOffset,
    nsresult (*aComparator)(nsIDOMNode*,PRInt32,nsIDOMRange*,PRInt32*),
    PRInt32* aInsertionPoint)
{
  nsresult rv;
  NS_ASSERTION(!aRemappingArray || aRemappingArray->Length() == mRanges.Length(),
               "Remapping array must have the same entries as the range array");

  PRInt32 beginSearch = 0;
  PRInt32 endSearch = mRanges.Length(); 
  while (endSearch - beginSearch > 0) {
    PRInt32 center = (endSearch - beginSearch) / 2 + beginSearch;

    nsIDOMRange* range;
    if (aRemappingArray)
      range = mRanges[(*aRemappingArray)[center]].mRange;
    else
      range = mRanges[center].mRange;

    PRInt32 cmp;
    rv = aComparator(aPointNode, aPointOffset, range, &cmp);
    NS_ENSURE_SUCCESS(rv, rv);

    if (cmp < 0) {        
      endSearch = center;
    } else if (cmp > 0) { 
      beginSearch = center + 1;
    } else {              
      beginSearch = center;
      break;
    }
  }
  *aInsertionPoint = beginSearch;
  return NS_OK;
}

nsresult
nsTypedSelection::AddItem(nsIDOMRange *aItem)
{
  nsresult rv;
  if (!aItem)
    return NS_ERROR_NULL_POINTER;

  NS_ASSERTION(ValidateRanges(), "Ranges out of sync");

  
  if (mRanges.Length() == 0) {
    if (! mRanges.AppendElement(RangeData(aItem, 0)))
      return NS_ERROR_OUT_OF_MEMORY;
    if (! mRangeEndings.AppendElement(0)) {
      mRanges.Clear();
      return NS_ERROR_OUT_OF_MEMORY;
    }
    return NS_OK;
  }

  nsCOMPtr<nsIDOMNode> beginNode;
  PRInt32 beginOffset;
  rv = aItem->GetStartContainer(getter_AddRefs(beginNode));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aItem->GetStartOffset(&beginOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 beginInsertionPoint;
  rv = FindInsertionPoint(nsnull, beginNode, beginOffset,
                          CompareToRangeStart, &beginInsertionPoint);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  

  nsCOMPtr<nsIDOMNode> endNode;
  PRInt32 endOffset;
  rv = aItem->GetEndContainer(getter_AddRefs(endNode));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aItem->GetEndOffset(&endOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (FindRangeGivenPoint(beginNode, beginOffset, endNode, endOffset,
                          beginInsertionPoint)) {
    
    return NS_OK;
  }

  PRInt32 endInsertionPoint;
  rv = FindInsertionPoint(&mRangeEndings, endNode, endOffset,
                          CompareToRangeEnd, &endInsertionPoint);
  NS_ENSURE_SUCCESS(rv, rv);


  
  
  if (! mRanges.InsertElementAt(beginInsertionPoint,
        RangeData(aItem, endInsertionPoint))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  if (! mRangeEndings.InsertElementAt(endInsertionPoint, beginInsertionPoint)) {
    mRanges.RemoveElementAt(beginInsertionPoint);
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  
  PRUint32 i;
  for (i = 0; i < mRangeEndings.Length(); i ++) {
    if (mRangeEndings[i] >= beginInsertionPoint)
      mRangeEndings[i] ++;
  }

  
  
  mRangeEndings[endInsertionPoint] = beginInsertionPoint;

  
  for (i = endInsertionPoint + 1; i < mRangeEndings.Length(); i ++)
    mRanges[mRangeEndings[i]].mEndIndex = i;

  NS_ASSERTION(ValidateRanges(), "Ranges out of sync");
  return NS_OK;
}


nsresult
nsTypedSelection::RemoveItem(nsIDOMRange *aItem)
{
  if (!aItem)
    return NS_ERROR_NULL_POINTER;
  NS_ASSERTION(ValidateRanges(), "Ranges out of sync");

  
  
  
  
  PRInt32 idx = -1;
  PRUint32 i;
  for (i = 0; i < mRanges.Length(); i ++) {
    if (mRanges[i].mRange == aItem) {
      idx = (PRInt32)i;
      break;
    }
  }
  if (idx < 0)
    return NS_ERROR_INVALID_ARG;
  mRanges.RemoveElementAt(idx);

  
  PRInt32 endingIndex = -1;
  for (i = 0; i < mRangeEndings.Length(); i ++) {
    if (mRangeEndings[i] == idx)
      endingIndex = i;
    if (mRangeEndings[i] > idx)
      mRangeEndings[i] --;
  }
  NS_ASSERTION(endingIndex >= 0 && endingIndex < (PRInt32)mRangeEndings.Length(),
               "Index not found in ending list");

  
  mRangeEndings.RemoveElementAt(endingIndex);

  
  for (i = endingIndex; i < mRangeEndings.Length(); i ++)
    mRanges[mRangeEndings[i]].mEndIndex = i;

  NS_ASSERTION(ValidateRanges(), "Ranges out of sync");
  return NS_OK;
}


nsresult
nsTypedSelection::Clear(nsPresContext* aPresContext)
{
  setAnchorFocusRange(-1);

  for (PRInt32 i = 0; i < (PRInt32)mRanges.Length(); i ++)
    selectFrames(aPresContext, mRanges[i].mRange, 0);
  mRanges.Clear();
  mRangeEndings.Clear();

  
  SetDirection(eDirNext);

  
  if (mFrameSelection->GetDisplaySelection() ==
      nsISelectionController::SELECTION_ATTENTION) {
    mFrameSelection->SetDisplaySelection(nsISelectionController::SELECTION_ON);
  }

  return NS_OK;
}






















nsresult
nsTypedSelection::MoveIndexToFirstMatch(PRInt32* aIndex, nsIDOMNode* aNode,
                                        PRInt32 aOffset,
                                        const nsTArray<PRInt32>* aRemappingArray,
                                        PRBool aUseBeginning)
{
  nsresult rv;
  nsCOMPtr<nsIDOMNode> curNode;
  PRInt32 curOffset;
  while (*aIndex > 0) {
    nsIDOMRange* range;
    if (aRemappingArray)
      range = mRanges[(*aRemappingArray)[(*aIndex) - 1]].mRange;
    else
      range = mRanges[(*aIndex) - 1].mRange;

    if (aUseBeginning) {
      rv = range->GetStartContainer(getter_AddRefs(curNode));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = range->GetStartOffset(&curOffset);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      rv = range->GetEndContainer(getter_AddRefs(curNode));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = range->GetEndOffset(&curOffset);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (curNode != aNode)
      break; 
    if (curOffset != aOffset)
      break; 

    
    (*aIndex) --;
  }
  return NS_OK;
}
























nsresult
nsTypedSelection::MoveIndexToNextMismatch(PRInt32* aIndex, nsIDOMNode* aNode,
                                          PRInt32 aOffset,
                                          const nsTArray<PRInt32>* aRemappingArray,
                                          PRBool aUseBeginning)
{
  nsresult rv;
  nsCOMPtr<nsIDOMNode> curNode;
  PRInt32 curOffset;
  while (*aIndex < (PRInt32)mRanges.Length()) {
    nsIDOMRange* range;
    if (aRemappingArray)
      range = mRanges[(*aRemappingArray)[*aIndex]].mRange;
    else
      range = mRanges[*aIndex].mRange;

    if (aUseBeginning) {
      rv = range->GetStartContainer(getter_AddRefs(curNode));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = range->GetStartOffset(&curOffset);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      rv = range->GetEndContainer(getter_AddRefs(curNode));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = range->GetEndOffset(&curOffset);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (curNode != aNode)
      break; 
    if (curOffset != aOffset)
      break; 

    
    (*aIndex) ++;
  }
  return NS_OK;
}





NS_IMETHODIMP
nsTypedSelection::GetRangesForInterval(nsIDOMNode* aBeginNode, PRInt32 aBeginOffset,
                                       nsIDOMNode* aEndNode, PRInt32 aEndOffset,
                                       PRBool aAllowAdjacent,
                                       PRUint32 *aResultCount,
                                       nsIDOMRange ***aResults)
{
  if (! aBeginNode || ! aEndNode || ! aResultCount || ! aResults)
    return NS_ERROR_NULL_POINTER;

  *aResultCount = 0;
  *aResults = nsnull;

  nsCOMArray<nsIDOMRange> results;
  nsresult rv = GetRangesForIntervalCOMArray(aBeginNode, aBeginOffset,
                                             aEndNode, aEndOffset,
                                             aAllowAdjacent,
                                             &results);
  NS_ENSURE_SUCCESS(rv, rv);
  if (results.Count() == 0)
    return NS_OK;

  *aResults = NS_STATIC_CAST(nsIDOMRange**,
      nsMemory::Alloc(sizeof(nsIDOMRange*) * results.Count()));
  NS_ENSURE_TRUE(*aResults, NS_ERROR_OUT_OF_MEMORY);

  *aResultCount = results.Count();
  for (PRInt32 i = 0; i < results.Count(); i ++)
    NS_ADDREF((*aResults)[i] = results[i]);
  return NS_OK;
}







NS_IMETHODIMP
nsTypedSelection::GetRangesForIntervalCOMArray(nsIDOMNode* aBeginNode, PRInt32 aBeginOffset,
                                               nsIDOMNode* aEndNode, PRInt32 aEndOffset,
                                               PRBool aAllowAdjacent,
                                               nsCOMArray<nsIDOMRange>* aRanges)
{
  nsresult rv;
  NS_ASSERTION(ValidateRanges(), "Ranges out of sync");
  aRanges->Clear();
  if (mRanges.Length() == 0)
    return NS_OK;

  
  
  

  
  
  PRInt32 beginningIndex;
  rv = FindInsertionPoint(nsnull, aEndNode, aEndOffset,
                          &CompareToRangeStart, &beginningIndex);
  NS_ENSURE_SUCCESS(rv, rv);
  if (beginningIndex == 0)
    return NS_OK; 

  
  PRInt32 endingIndex;
  rv = FindInsertionPoint(&mRangeEndings, aBeginNode, aBeginOffset,
                          &CompareToRangeEnd, &endingIndex);
  NS_ENSURE_SUCCESS(rv, rv);
  if (endingIndex == (PRInt32)mRangeEndings.Length())
    return NS_OK; 

  
  
  if (aAllowAdjacent) {
    
    
    
    
    
    
    
    
    
    
    rv = MoveIndexToFirstMatch(&endingIndex, aBeginNode, aBeginOffset,
                               &mRangeEndings, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = MoveIndexToNextMismatch(&beginningIndex, aEndNode, aEndOffset,
                                 nsnull, PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    
    
    
    
    
    rv = MoveIndexToNextMismatch(&endingIndex, aBeginNode, aBeginOffset,
                                 &mRangeEndings, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = MoveIndexToFirstMatch(&beginningIndex, aEndNode, aEndOffset,
                               nsnull, PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  if (beginningIndex > (PRInt32)mRangeEndings.Length() - endingIndex) {
    
    for (PRInt32 i = endingIndex; i < (PRInt32)mRangeEndings.Length(); i ++) {
      if (mRangeEndings[i] < beginningIndex) {
        if (! aRanges->AppendObject(mRanges[mRangeEndings[i]].mRange))
          return NS_ERROR_OUT_OF_MEMORY;
      }
    }
  } else {
    for (PRInt32 i = 0; i < beginningIndex; i ++) {
      if (mRanges[i].mEndIndex >= endingIndex) {
        if (! aRanges->AppendObject(mRanges[i].mRange))
          return NS_ERROR_OUT_OF_MEMORY;
      }
    }
  }
  return NS_OK;
}






static PRBool
RangeMatchesBeginPoint(nsIDOMRange* aRange, nsIDOMNode* aNode, PRInt32 aOffset)
{
  PRInt32 offset;
  nsresult rv = aRange->GetStartOffset(&offset);
  if (NS_FAILED(rv) || offset != aOffset)
    return PR_FALSE;

  nsCOMPtr<nsIDOMNode> node;
  rv = aRange->GetStartContainer(getter_AddRefs(node));
  if (NS_FAILED(rv) || node != aNode)
    return PR_FALSE;

  return PR_TRUE;
}

static PRBool
RangeMatchesEndPoint(nsIDOMRange* aRange, nsIDOMNode* aNode, PRInt32 aOffset)
{
  PRInt32 offset;
  nsresult rv = aRange->GetEndOffset(&offset);
  if (NS_FAILED(rv) || offset != aOffset)
    return PR_FALSE;

  nsCOMPtr<nsIDOMNode> node;
  rv = aRange->GetEndContainer(getter_AddRefs(node));
  if (NS_FAILED(rv) || node != aNode)
    return PR_FALSE;

  return PR_TRUE;
}













PRBool
nsTypedSelection::FindRangeGivenPoint(
    nsIDOMNode* aBeginNode, PRInt32 aBeginOffset,
    nsIDOMNode* aEndNode, PRInt32 aEndOffset,
    PRInt32 aStartSearchingHere)
{
  PRInt32 i;
  NS_ASSERTION(aStartSearchingHere >= 0 && aStartSearchingHere <= (PRInt32)mRanges.Length(),
               "Input searching seed is not in range.");

  
  for (i = aStartSearchingHere; i >= 0 && i < (PRInt32)mRanges.Length(); i --) {
    if (RangeMatchesBeginPoint(mRanges[i].mRange, aBeginNode, aBeginOffset)) {
      if (RangeMatchesEndPoint(mRanges[i].mRange, aEndNode, aEndOffset))
        return PR_TRUE;
    } else {
      
      break;
    }
  }

  
  for (i = aStartSearchingHere + 1; i < (PRInt32)mRanges.Length(); i ++) {
    if (RangeMatchesBeginPoint(mRanges[i].mRange, aBeginNode, aBeginOffset)) {
      if (RangeMatchesEndPoint(mRanges[i].mRange, aEndNode, aEndOffset))
        return PR_TRUE;
    } else {
      
      break;
    }
  }

  
  return PR_FALSE;
}



#if 0
NS_IMETHODIMP
nsTypedSelection::GetPrimaryFrameForRangeEndpoint(nsIDOMNode *aNode, PRInt32 aOffset, PRBool aIsEndNode, nsIFrame **aReturnFrame)
{
  if (!aNode || !aReturnFrame)
    return NS_ERROR_NULL_POINTER;
  
  if (aOffset < 0)
    return NS_ERROR_FAILURE;

  *aReturnFrame = 0;
  
  nsresult  result = NS_OK;
  
  nsCOMPtr<nsIDOMNode> node = aNode;

  if (!node)
    return NS_ERROR_NULL_POINTER;
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(node, &result);

  if (NS_FAILED(result))
    return result;

  if (!content)
    return NS_ERROR_NULL_POINTER;
  
  if (content->IsNodeOfType(nsINode::eELEMENT))
  {
    if (aIsEndNode)
      aOffset--;

    if (aOffset >= 0)
    {
      nsIContent *child = content->GetChildAt(aOffset);
      if (!child) 
        return NS_ERROR_FAILURE;

      content = child; 
    }
  }
  *aReturnFrame = mFrameSelection->GetShell()->GetPrimaryFrameFor(content);
  return NS_OK;
}
#endif


NS_IMETHODIMP
nsTypedSelection::GetPrimaryFrameForAnchorNode(nsIFrame **aReturnFrame)
{
  if (!aReturnFrame)
    return NS_ERROR_NULL_POINTER;
  
  PRInt32 frameOffset = 0;
  *aReturnFrame = 0;
  nsCOMPtr<nsIContent> content = do_QueryInterface(FetchAnchorNode());
  if (content && mFrameSelection)
  {
    *aReturnFrame = mFrameSelection->
      GetFrameForNodeOffset(content, FetchAnchorOffset(),
                            mFrameSelection->GetHint(), &frameOffset);
    if (*aReturnFrame)
      return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsTypedSelection::GetPrimaryFrameForFocusNode(nsIFrame **aReturnFrame, PRInt32 *aOffsetUsed,
                                              PRBool aVisual)
{
  if (!aReturnFrame)
    return NS_ERROR_NULL_POINTER;
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(FetchFocusNode());
  if (!content || !mFrameSelection)
    return NS_ERROR_FAILURE;
  
  nsIPresShell *presShell = mFrameSelection->GetShell();

  PRInt32 frameOffset = 0;
  *aReturnFrame = 0;
  if (!aOffsetUsed)
    aOffsetUsed = &frameOffset;
    
  nsFrameSelection::HINT hint = mFrameSelection->GetHint();

  if (aVisual) {
    nsCOMPtr<nsICaret> caret;
    nsresult result = presShell->GetCaret(getter_AddRefs(caret));
    if (NS_FAILED(result) || !caret)
      return NS_ERROR_FAILURE;
    
    PRUint8 caretBidiLevel = mFrameSelection->GetCaretBidiLevel();

    return caret->GetCaretFrameForNodeOffset(content, FetchFocusOffset(),
      hint, caretBidiLevel, aReturnFrame, aOffsetUsed);
  }
  
  *aReturnFrame = mFrameSelection->
    GetFrameForNodeOffset(content, FetchFocusOffset(),
                          hint, aOffsetUsed);
  if (!*aReturnFrame)
    return NS_ERROR_FAILURE;

  return NS_OK;
}




NS_IMETHODIMP
nsTypedSelection::selectFrames(nsPresContext* aPresContext,
                             nsIContentIterator *aInnerIter,
                             nsIContent *aContent,
                             nsIDOMRange *aRange,
                             nsIPresShell *aPresShell,
                             PRBool aFlags)
{
  if (!mFrameSelection)
    return NS_OK;
  nsresult result;
  if (!aInnerIter)
    return NS_ERROR_NULL_POINTER;
  result = aInnerIter->Init(aContent);
  nsIFrame *frame;
  if (NS_SUCCEEDED(result))
  {
    
    frame = mFrameSelection->GetShell()->GetPrimaryFrameFor(aContent);
    if (frame)
    {
      
      frame->SetSelected(aPresContext, nsnull, aFlags, eSpreadDown);
#ifndef OLD_TABLE_SELECTION
      if (mFrameSelection->GetTableCellSelection())
      {
        nsITableCellLayout *tcl = nsnull;
        CallQueryInterface(frame, &tcl);
        if (tcl)
        {
          return NS_OK;
        }
      }
#endif 
    }
    
    while (!aInnerIter->IsDone())
    {
      nsIContent *innercontent = aInnerIter->GetCurrentNode();

      frame = mFrameSelection->GetShell()->GetPrimaryFrameFor(innercontent);
      if (frame)
      {
        
        

        
        frame->SetSelected(aPresContext, nsnull,aFlags,eSpreadDown);
        nsRect frameRect = frame->GetRect();

        
        
        while (!frameRect.width || !frameRect.height)
        {
          
          frame = frame->GetNextInFlow();
          if (frame)
          {
            frameRect = frame->GetRect();
            frame->SetSelected(aPresContext, nsnull,aFlags,eSpreadDown);
          }
          else
            break;
        }
        
        
      }

      aInnerIter->Next();
    }

#if 0
    frame = mFrameSelection->GetShell()->GetPrimaryFrameFor(content);
    if (frame)
      frame->SetSelected(aRange,aFlags,eSpreadDown);
#endif

    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}




NS_IMETHODIMP
nsTypedSelection::selectFrames(nsPresContext* aPresContext, nsIDOMRange *aRange, PRBool aFlags)
{
  if (!mFrameSelection)
    return NS_OK;
  if (!aRange || !aPresContext) 
    return NS_ERROR_NULL_POINTER;

  nsresult result;
  nsCOMPtr<nsIContentIterator> iter = do_CreateInstance(
                                              kCSubtreeIteratorCID,
                                              &result);
  if (NS_FAILED(result))
    return result;

  nsCOMPtr<nsIContentIterator> inneriter = do_CreateInstance(
                                              kCContentIteratorCID,
                                              &result);

  if ((NS_SUCCEEDED(result)) && iter && inneriter)
  {
    nsIPresShell *presShell = aPresContext->GetPresShell();
    result = iter->Init(aRange);

    
    
    
    
    nsCOMPtr<nsIContent> content;
    nsIFrame *frame;

    content = do_QueryInterface(FetchStartParent(aRange), &result);
    if (NS_FAILED(result) || !content)
      return result;

    if (!content->IsNodeOfType(nsINode::eELEMENT))
    {
      frame = mFrameSelection->GetShell()->GetPrimaryFrameFor(content);
      if (frame)
        frame->SetSelected(aPresContext, aRange,aFlags,eSpreadDown);
    }

    iter->First();

    while (!iter->IsDone())
    {
      content = iter->GetCurrentNode();

      selectFrames(aPresContext, inneriter, content, aRange, presShell,aFlags);

      iter->Next();
    }

    if (FetchEndParent(aRange) != FetchStartParent(aRange))
    {
      content = do_QueryInterface(FetchEndParent(aRange), &result);
      if (NS_FAILED(result) || !content)
        return result;

      if (!content->IsNodeOfType(nsINode::eELEMENT))
      {
        frame = mFrameSelection->GetShell()->GetPrimaryFrameFor(content);
        if (frame)
           frame->SetSelected(aPresContext, aRange,aFlags,eSpreadDown);
      }
    }

  }
  return result;
}

























NS_IMETHODIMP
nsTypedSelection::LookUpSelection(nsIContent *aContent, PRInt32 aContentOffset,
                                  PRInt32 aContentLength,
                                  SelectionDetails **aReturnDetails,
                                  SelectionType aType, PRBool aSlowCheck)
{
  nsresult rv;
  if (! aContent || ! aReturnDetails)
    return NS_ERROR_NULL_POINTER;

  
  if (mRanges.Length() == 0)
    return NS_OK;

  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aContent, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMArray<nsIDOMRange> overlappingRanges;
  rv = GetRangesForIntervalCOMArray(node, aContentOffset,
                                    node, aContentOffset + aContentLength,
                                    PR_FALSE,
                                    &overlappingRanges);
  NS_ENSURE_SUCCESS(rv, rv);
  if (overlappingRanges.Count() == 0)
    return NS_OK;

  for (PRInt32 i = 0; i < overlappingRanges.Count(); i ++) {
    nsCOMPtr<nsIDOMNode> startNode, endNode;
    PRInt32 startOffset, endOffset;
    nsIDOMRange* range = overlappingRanges[i];
    range->GetStartContainer(getter_AddRefs(startNode));
    range->GetStartOffset(&startOffset);
    range->GetEndContainer(getter_AddRefs(endNode));
    range->GetEndOffset(&endOffset);

    PRInt32 start = -1, end = -1;
    if (startNode == node && endNode == node) {
      if (startOffset < (aContentOffset + aContentLength)  &&
          endOffset > aContentOffset) {
        
        start = PR_MAX(0, startOffset - aContentOffset);
        end = PR_MIN(aContentLength, endOffset - aContentOffset);
      }
      
      
    } else if (startNode == node) {
      if (startOffset < (aContentOffset + aContentLength)) {
        
        
        start = PR_MAX(0, startOffset - aContentOffset);
        end = aContentLength;
      }
    } else if (endNode == node) {
      if (endOffset > aContentOffset) {
        
        
        start = 0;
        end = PR_MIN(aContentLength, endOffset - aContentOffset);
      }
    } else {
      
      
      
      
      start = 0;
      end = aContentLength;
    }
    if (start < 0)
      continue; 

    SelectionDetails* details = new SelectionDetails;
    if (! details)
      return NS_ERROR_OUT_OF_MEMORY;

    details->mNext = *aReturnDetails;
    details->mStart = start;
    details->mEnd = end;
    details->mType = aType;
    *aReturnDetails = details;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsTypedSelection::Repaint(nsPresContext* aPresContext)
{
  PRInt32 arrCount = (PRInt32)mRanges.Length();

  if (arrCount < 1)
    return NS_OK;

  PRInt32 i;
  nsIDOMRange* range;
  
  for (i = 0; i < arrCount; i++)
  {
    range = mRanges[i].mRange;

    if (!range)
      return NS_ERROR_UNEXPECTED;

    nsresult rv = selectFrames(aPresContext, range, PR_TRUE);

    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsTypedSelection::GetCanCacheFrameOffset(PRBool *aCanCacheFrameOffset)
{ 
  NS_ENSURE_ARG_POINTER(aCanCacheFrameOffset);

  if (mCachedOffsetForFrame)
    *aCanCacheFrameOffset = mCachedOffsetForFrame->mCanCacheFrameOffset;
  else
    *aCanCacheFrameOffset = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP    
nsTypedSelection::SetCanCacheFrameOffset(PRBool aCanCacheFrameOffset)
{
  if (!mCachedOffsetForFrame) {
    mCachedOffsetForFrame = new CachedOffsetForFrame;
  }

  mCachedOffsetForFrame->mCanCacheFrameOffset = aCanCacheFrameOffset;

  
  
  if (!aCanCacheFrameOffset) {
    mCachedOffsetForFrame->mLastCaretFrame = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsTypedSelection::GetCachedFrameOffset(nsIFrame *aFrame, PRInt32 inOffset, nsPoint& aPoint)
{
  if (!mCachedOffsetForFrame) {
    mCachedOffsetForFrame = new CachedOffsetForFrame;
  }

  nsresult rv = NS_OK;
  if (mCachedOffsetForFrame->mCanCacheFrameOffset &&
      mCachedOffsetForFrame->mLastCaretFrame &&
      (aFrame == mCachedOffsetForFrame->mLastCaretFrame) &&
      (inOffset == mCachedOffsetForFrame->mLastContentOffset))
  {
     
     aPoint = mCachedOffsetForFrame->mCachedFrameOffset;
  } 
  else
  {
     
     
     rv = GetPointFromOffset(aFrame, inOffset, &aPoint);
     if (NS_SUCCEEDED(rv) && mCachedOffsetForFrame->mCanCacheFrameOffset) {
       mCachedOffsetForFrame->mCachedFrameOffset = aPoint;
       mCachedOffsetForFrame->mLastCaretFrame = aFrame;
       mCachedOffsetForFrame->mLastContentOffset = inOffset; 
     }
  }

  return rv;
}

NS_IMETHODIMP
nsTypedSelection::GetFrameSelection(nsFrameSelection **aFrameSelection) {
  NS_ENSURE_ARG_POINTER(aFrameSelection);
  *aFrameSelection = mFrameSelection;
  NS_IF_ADDREF(*aFrameSelection);
  return NS_OK;
}

nsresult
nsTypedSelection::StartAutoScrollTimer(nsPresContext *aPresContext,
                                       nsIView *aView,
                                       nsPoint& aPoint,
                                       PRUint32 aDelay)
{
  NS_PRECONDITION(aView, "Need a view");

  nsresult result;
  if (!mFrameSelection)
    return NS_OK;

  if (!mAutoScrollTimer)
  {
    mAutoScrollTimer = new nsAutoScrollTimer();

    if (!mAutoScrollTimer)
      return NS_ERROR_OUT_OF_MEMORY;

    result = mAutoScrollTimer->Init(mFrameSelection, this);

    if (NS_FAILED(result))
      return result;
  }

  result = mAutoScrollTimer->SetDelay(aDelay);

  if (NS_FAILED(result))
    return result;

  return DoAutoScrollView(aPresContext, aView, aPoint, PR_TRUE);
}

nsresult
nsTypedSelection::StopAutoScrollTimer()
{
  if (mAutoScrollTimer)
    return mAutoScrollTimer->Stop();

  return NS_OK; 
}

nsresult
nsTypedSelection::GetViewAncestorOffset(nsIView *aView, nsIView *aAncestorView, nscoord *aXOffset, nscoord *aYOffset)
{
  
  

  if (!aView || !aXOffset || !aYOffset)
    return NS_ERROR_FAILURE;

  nsPoint offset = aView->GetOffsetTo(aAncestorView);

  *aXOffset = offset.x;
  *aYOffset = offset.y;

  return NS_OK;
}

nsresult
nsTypedSelection::ScrollPointIntoClipView(nsPresContext *aPresContext, nsIView *aView, nsPoint& aPoint, PRBool *aDidScroll)
{
  nsresult result;

  if (!aPresContext || !aView || !aDidScroll)
    return NS_ERROR_NULL_POINTER;

  *aDidScroll = PR_FALSE;

  
  
  

  nsIScrollableView *scrollableView =
    nsLayoutUtils::GetNearestScrollingView(aView, nsLayoutUtils::eEither);

  if (!scrollableView)
    return NS_OK; 

  
  
  

  nsIView *scrolledView = 0;

  result = scrollableView->GetScrolledView(scrolledView);
  
  
  
  
  

  nsPoint viewOffset(0,0);

  result = GetViewAncestorOffset(aView, scrolledView, &viewOffset.x, &viewOffset.y);

  if (NS_FAILED(result))
    return result;

  
  
  
  

  nsRect bounds = scrollableView->View()->GetBounds();

  result = scrollableView->GetScrollPosition(bounds.x,bounds.y);

  if (NS_FAILED(result))
    return result;

  
  
  
  
  

  nscoord dx = 0, dy = 0;

  nsPresContext::ScrollbarStyles ss =
    nsLayoutUtils::ScrollbarStylesOfView(scrollableView);

  if (ss.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN) {
    nscoord e = aPoint.x + viewOffset.x;
  
    nscoord x1 = bounds.x;
    nscoord x2 = bounds.x + bounds.width;

    if (e < x1)
      dx = e - x1;
    else if (e > x2)
      dx = e - x2;
  }

  if (ss.mVertical != NS_STYLE_OVERFLOW_HIDDEN) {
    nscoord e = aPoint.y + viewOffset.y;

    nscoord y1 = bounds.y;
    nscoord y2 = bounds.y + bounds.height;

    if (e < y1)
      dy = e - y1;
    else if (e > y2)
      dy = e - y2;
  }

  
  
  

  if (dx != 0 || dy != 0)
  {
    
    aPresContext->GetViewManager()->Composite();

    

    result = scrollableView->ScrollTo(bounds.x + dx, bounds.y + dy,
                                      NS_VMREFRESH_NO_SYNC);

    if (NS_FAILED(result))
      return result;

    nsPoint newPos;

    result = scrollableView->GetScrollPosition(newPos.x, newPos.y);

    if (NS_FAILED(result))
      return result;

    *aDidScroll = (bounds.x != newPos.x || bounds.y != newPos.y);
  }

  return result;
}

nsresult
nsTypedSelection::ScrollPointIntoView(nsPresContext *aPresContext, nsIView *aView, nsPoint& aPoint, PRBool aScrollParentViews, PRBool *aDidScroll)
{
  if (!aPresContext || !aView || !aDidScroll)
    return NS_ERROR_NULL_POINTER;

  nsresult result;

  *aDidScroll = PR_FALSE;

  
  
  

  nsPoint globalOffset;

  result = GetViewAncestorOffset(aView, nsnull, &globalOffset.x, &globalOffset.y);

  if (NS_FAILED(result))
    return result;

  
  
  
  

  nsPoint globalPoint = aPoint + globalOffset;

  
  
  
  
  result = ScrollPointIntoClipView(aPresContext, aView, aPoint, aDidScroll);

  if (NS_FAILED(result))
    return result;

  
  
  

  if (aScrollParentViews)
  {
    
    
    

    nsIScrollableView *scrollableView =
      nsLayoutUtils::GetNearestScrollingView(aView, nsLayoutUtils::eEither);

    if (scrollableView)
    {
      
      
      

      nsIView *scrolledView = 0;
      nsIView *view = scrollableView->View();

      if (view)
      {
        
        
        
        

        view = view->GetParent();

        while (view)
        {
          scrollableView =
            nsLayoutUtils::GetNearestScrollingView(view,
                                                   nsLayoutUtils::eEither);

          if (!scrollableView)
            break;

          scrolledView = 0;
          result = scrollableView->GetScrolledView(scrolledView);
          
          if (NS_FAILED(result))
            return result;

          
          
          

          result = GetViewAncestorOffset(scrolledView, nsnull, &globalOffset.x, &globalOffset.y);

          if (NS_FAILED(result))
            return result;

          nsPoint newPoint = globalPoint - globalOffset;

          
          
          

          PRBool parentDidScroll = PR_FALSE;

          result = ScrollPointIntoClipView(aPresContext, scrolledView, newPoint, &parentDidScroll);

          if (NS_FAILED(result))
            return result;

          *aDidScroll = *aDidScroll || parentDidScroll;

          
          
          
          

          view = scrollableView->View()->GetParent();
        }
      }
    }
  }

  return NS_OK;
}

nsresult
nsTypedSelection::DoAutoScrollView(nsPresContext *aPresContext,
                                   nsIView *aView,
                                   nsPoint& aPoint,
                                   PRBool aScrollParentViews)
{
  if (!aPresContext || !aView)
    return NS_ERROR_NULL_POINTER;

  nsresult result;

  if (mAutoScrollTimer)
    result = mAutoScrollTimer->Stop();

  
  
  

  nsPoint globalOffset;
  result = GetViewAncestorOffset(aView, nsnull, &globalOffset.x, &globalOffset.y);
  NS_ENSURE_SUCCESS(result, result);

  
  
  
  
  nsPoint globalPoint = aPoint + globalOffset;
  
  
  

  PRBool didScroll = PR_FALSE;

  result = ScrollPointIntoView(aPresContext, aView, aPoint, aScrollParentViews, &didScroll);
  NS_ENSURE_SUCCESS(result, result);

  
  
  

  if (didScroll && mAutoScrollTimer)
  {
    
    
    
    
    
    result = GetViewAncestorOffset(aView, nsnull, &globalOffset.x, &globalOffset.y);
    NS_ENSURE_SUCCESS(result, result);

    nsPoint svPoint = globalPoint - globalOffset;
    mAutoScrollTimer->Start(aPresContext, aView, svPoint);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsTypedSelection::GetEnumerator(nsIEnumerator **aIterator)
{
  nsresult status = NS_ERROR_OUT_OF_MEMORY;
  nsSelectionIterator *iterator =  new nsSelectionIterator(this);
  if ( iterator && NS_FAILED(status = CallQueryInterface(iterator, aIterator)) )
    delete iterator;
  return status;
}





NS_IMETHODIMP
nsTypedSelection::RemoveAllRanges()
{
  if (!mFrameSelection)
    return NS_OK;
  nsCOMPtr<nsPresContext>  presContext;
  GetPresContext(getter_AddRefs(presContext));


  nsresult  result = Clear(presContext);
  if (NS_FAILED(result))
    return result;
  
  
  mFrameSelection->ClearTableCellSelection();

  return mFrameSelection->NotifySelectionListeners(GetType());
  
  
}




NS_IMETHODIMP
nsTypedSelection::AddRange(nsIDOMRange* aRange)
{
  if (!aRange) return NS_ERROR_NULL_POINTER;

  
  
  PRBool didAddRange;
  nsresult result = addTableCellRange(aRange, &didAddRange);
  if (NS_FAILED(result)) return result;

  if (!didAddRange)
  {
    result = AddItem(aRange);
    if (NS_FAILED(result)) return result;
  }

  PRInt32 count;
  result = GetRangeCount(&count);
  if (NS_FAILED(result)) return result;

  if (count <= 0)
  {
    NS_ASSERTION(0,"bad count after additem\n");
    return NS_ERROR_FAILURE;
  }
  setAnchorFocusRange(count -1);
  
  
  if (mType == nsISelectionController::SELECTION_NORMAL)
    SetInterlinePosition(PR_TRUE);

  nsCOMPtr<nsPresContext>  presContext;
  GetPresContext(getter_AddRefs(presContext));
  selectFrames(presContext, aRange, PR_TRUE);        

  
  if (!mFrameSelection)
    return NS_OK;

  return mFrameSelection->NotifySelectionListeners(GetType());
}













NS_IMETHODIMP
nsTypedSelection::RemoveRange(nsIDOMRange* aRange)
{
  if (!aRange)
    return NS_ERROR_INVALID_ARG;
  nsresult rv = RemoveItem(aRange);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIDOMNode> beginNode, endNode;
  rv = aRange->GetStartContainer(getter_AddRefs(beginNode));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aRange->GetEndContainer(getter_AddRefs(endNode));
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt32 beginOffset, endOffset;
  PRUint16 endNodeType = nsIDOMNode::ELEMENT_NODE;
  endNode->GetNodeType(&endNodeType);
  if (endNodeType == nsIDOMNode::TEXT_NODE) {
    
    
    
    beginOffset = 0;
    nsAutoString endNodeValue;
    endNode->GetNodeValue(endNodeValue);
    endOffset = endNodeValue.Length();
  } else {
    
    aRange->GetStartOffset(&beginOffset);
    aRange->GetEndOffset(&endOffset);
  }

  
  nsCOMPtr<nsPresContext>  presContext;
  GetPresContext(getter_AddRefs(presContext));
  selectFrames(presContext, aRange, PR_FALSE);

  
  nsCOMArray<nsIDOMRange> affectedRanges;
  rv = GetRangesForIntervalCOMArray(beginNode, beginOffset,
                                    endNode, endOffset,
                                    PR_TRUE, &affectedRanges);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRInt32 i = 0; i < affectedRanges.Count(); i ++)
    selectFrames(presContext, affectedRanges[i], PR_TRUE);

  
  
  
  
  if (mType != nsISelectionController::SELECTION_SPELLCHECK &&
      aRange == mAnchorFocusRange.get())
  {
    PRInt32 cnt = mRanges.Length();
    if (cnt > 0)
    {
      setAnchorFocusRange(cnt - 1);
      ScrollIntoView();
    }
  }
  if (!mFrameSelection)
    return NS_OK;
  return mFrameSelection->NotifySelectionListeners(GetType());
}






NS_IMETHODIMP
nsTypedSelection::Collapse(nsIDOMNode* aParentNode, PRInt32 aOffset)
{
  if (!aParentNode)
    return NS_ERROR_INVALID_ARG;
  if (!mFrameSelection)
    return NS_ERROR_NOT_INITIALIZED; 
  mFrameSelection->InvalidateDesiredX();
  if (!IsValidSelectionPoint(mFrameSelection, aParentNode))
    return NS_ERROR_FAILURE;
  nsresult result;
  
  if (NS_FAILED(SetOriginalAnchorPoint(aParentNode,aOffset)))
    return NS_ERROR_FAILURE; 
  nsCOMPtr<nsPresContext>  presContext;
  GetPresContext(getter_AddRefs(presContext));
  Clear(presContext);

  
  if (mFrameSelection)
    mFrameSelection->ClearTableCellSelection();

  nsCOMPtr<nsIDOMRange> range;
  NS_NewRange(getter_AddRefs(range));
  if (! range){
    NS_ASSERTION(PR_FALSE,"Couldn't make a range - nsFrameSelection::Collapse");
    return NS_ERROR_UNEXPECTED;
  }
  result = range->SetEnd(aParentNode, aOffset);
  if (NS_FAILED(result))
    return result;
  result = range->SetStart(aParentNode, aOffset);
  if (NS_FAILED(result))
    return result;

#ifdef DEBUG_SELECTION
  if (aParentNode)
  {
    nsCOMPtr<nsIContent>content;
    content = do_QueryInterface(aParentNode);
    if (!content)
      return NS_ERROR_FAILURE;

    const char *tagString;
    content->Tag()->GetUTF8String(&tagString);
    printf ("Sel. Collapse to %p %s %d\n", content.get(), tagString, aOffset);
  }
  else {
    printf ("Sel. Collapse set to null parent.\n");
  }
#endif


  result = AddItem(range);
  setAnchorFocusRange(0);
  selectFrames(presContext, range,PR_TRUE);
  if (NS_FAILED(result))
    return result;
  if (!mFrameSelection)
    return NS_OK;
  return mFrameSelection->NotifySelectionListeners(GetType());
}





NS_IMETHODIMP
nsTypedSelection::CollapseToStart()
{
  PRInt32 cnt;
  nsresult rv = GetRangeCount(&cnt);
  if (NS_FAILED(rv) || cnt <= 0)
    return NS_ERROR_FAILURE;

  
  nsIDOMRange* firstRange = mRanges[0].mRange;
  if (!firstRange)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> parent;
  rv = firstRange->GetStartContainer(getter_AddRefs(parent));
  if (NS_SUCCEEDED(rv))
  {
    if (parent)
    {
      PRInt32 startOffset;
      firstRange->GetStartOffset(&startOffset);
      rv = Collapse(parent, startOffset);
    } else {
      
      rv = NS_ERROR_FAILURE;
    }
  }
  return rv;
}





NS_IMETHODIMP
nsTypedSelection::CollapseToEnd()
{
  PRInt32 cnt;
  nsresult rv = GetRangeCount(&cnt);
  if (NS_FAILED(rv) || cnt <= 0)
    return NS_ERROR_FAILURE;

  
  nsIDOMRange* lastRange = mRanges[cnt-1].mRange;
  if (!lastRange)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> parent;
  rv = lastRange->GetEndContainer(getter_AddRefs(parent));
  if (NS_SUCCEEDED(rv))
  {
    if (parent)
    {
      PRInt32 endOffset;
      lastRange->GetEndOffset(&endOffset);
      rv = Collapse(parent, endOffset);
    } else {
      
      rv = NS_ERROR_FAILURE;
    }
  }
  return rv;
}




NS_IMETHODIMP
nsTypedSelection::GetIsCollapsed(PRBool* aIsCollapsed)
{
  if (!aIsCollapsed)
    return NS_ERROR_NULL_POINTER;

  PRInt32 cnt = (PRInt32)mRanges.Length();;
  if (cnt == 0)
  {
    *aIsCollapsed = PR_TRUE;
    return NS_OK;
  }
  
  if (cnt != 1)
  {
    *aIsCollapsed = PR_FALSE;
    return NS_OK;
  }
  
  return mRanges[0].mRange->GetCollapsed(aIsCollapsed);
}

NS_IMETHODIMP
nsTypedSelection::GetRangeCount(PRInt32* aRangeCount)
{
  if (!aRangeCount) 
    return NS_ERROR_NULL_POINTER;

  *aRangeCount = (PRInt32)mRanges.Length();
  NS_ASSERTION(ValidateRanges(), "Ranges out of sync");

  return NS_OK;
}

NS_IMETHODIMP
nsTypedSelection::GetRangeAt(PRInt32 aIndex, nsIDOMRange** aReturn)
{
  if (!aReturn)
    return NS_ERROR_NULL_POINTER;
  NS_ASSERTION(ValidateRanges(), "Ranges out of sync");

  PRInt32 cnt = (PRInt32)mRanges.Length();
  if (aIndex < 0 || aIndex >= cnt)
    return NS_ERROR_INVALID_ARG;

  *aReturn = mRanges[aIndex].mRange;
  NS_IF_ADDREF(*aReturn);

  return NS_OK;
}

#ifdef OLD_SELECTION



NS_IMETHODIMP
nsTypedSelection::FixupSelectionPoints(nsIDOMRange *aRange , nsDirection *aDir, PRBool *aFixupState)
{
  if (!aRange || !aFixupState)
    return NS_ERROR_NULL_POINTER;
  *aFixupState = PR_FALSE;
  nsresult res;

  
  
  nsCOMPtr<nsIDOMNode> startNode;
  nsCOMPtr<nsIDOMNode> endNode;
  PRInt32 startOffset;
  PRInt32 endOffset;
  nsresult result;
  if (*aDir == eDirNext)
  {
    if (NS_FAILED(GetOriginalAnchorPoint(getter_AddRefs(startNode), &startOffset)))
    {
      aRange->GetStartParent(getter_AddRefs(startNode));
      aRange->GetStartOffset(&startOffset);
    }
    aRange->GetEndParent(getter_AddRefs(endNode));
    aRange->GetEndOffset(&endOffset);
  }
  else
  {
    if (NS_FAILED(GetOriginalAnchorPoint(getter_AddRefs(startNode), &startOffset)))
    {
      aRange->GetEndParent(getter_AddRefs(startNode));
      aRange->GetEndOffset(&startOffset);
    }
    aRange->GetStartParent(getter_AddRefs(endNode));
    aRange->GetStartOffset(&endOffset);
  }
  if (!startNode || !endNode)
    return NS_ERROR_FAILURE;

  
  nsIAtom *atom = GetTag(endNode);
  if (atom == nsGkAtoms::tbody)
    return NS_ERROR_FAILURE; 

  
  nsCOMPtr<nsIDOMNode> parent;
  nsCOMPtr<nsIDOMRange> subRange;
  NS_NewRange(getter_AddRefs(subRange));
  if (!subRange) return NS_ERROR_OUT_OF_MEMORY

  result = subRange->SetStart(startNode,startOffset);
  if (NS_FAILED(result))
    return result;
  result = subRange->SetEnd(endNode,endOffset);
  if (NS_FAILED(result))
  {
    result = subRange->SetEnd(startNode,startOffset);
    if (NS_FAILED(result))
      return result;
    result = subRange->SetStart(endNode,endOffset);
    if (NS_FAILED(result))
      return result;
  }

  res = subRange->GetCommonParent(getter_AddRefs(parent));
  if (NS_FAILED(res) || !parent)
    return res;
 
  
  

  
  nsCOMPtr<nsIDOMNode> tempNode;
  nsCOMPtr<nsIDOMNode> tempNode2;
  PRBool cellMode = PR_FALSE;
  PRBool dirtystart = PR_FALSE;
  PRBool dirtyend = PR_FALSE;
  if (startNode != endNode)
  {
    if (parent != startNode)
    {
      result = startNode->GetParentNode(getter_AddRefs(tempNode));
      if (NS_FAILED(result) || !tempNode)
        return NS_ERROR_FAILURE;
      while (tempNode != parent)
      {
        atom = GetTag(tempNode);
        if (atom == nsGkAtoms::table) 
        {
          result = ParentOffset(tempNode, getter_AddRefs(startNode), &startOffset);
          if (NS_FAILED(result))
            return NS_ERROR_FAILURE;
          if (*aDir == eDirPrevious) 
            startOffset++;
          dirtystart = PR_TRUE;
          cellMode = PR_FALSE;
        }
        else if (atom == nsGkAtoms::td ||
                 atom == nsGkAtoms::th) 
        {
          cellMode = PR_TRUE;
          result = ParentOffset(tempNode, getter_AddRefs(startNode), &startOffset);
          if (NS_FAILED(result))
            return result;
          if (*aDir == eDirPrevious) 
            startOffset++;
          dirtystart = PR_TRUE;
        }
        result = tempNode->GetParentNode(getter_AddRefs(tempNode2));
        if (NS_FAILED(result) || !tempNode2)
          return NS_ERROR_FAILURE;
        tempNode = tempNode2;
      }
    }
  
  
    if (parent != endNode)
    {
      result = endNode->GetParentNode(getter_AddRefs(tempNode));
      PRBool found = !cellMode;
      if (NS_FAILED(result) || !tempNode)
        return NS_ERROR_FAILURE;
      while (tempNode != parent)
      {
        atom = GetTag(tempNode);
        if (atom == nsGkAtoms::table) 
        {
          if (!cellMode)
          {
            result = ParentOffset(tempNode, getter_AddRefs(endNode), &endOffset);
            if (NS_FAILED(result))
              return result;
            if (*aDir == eDirNext) 
              endOffset++;
            dirtyend = PR_TRUE;
          }
          else
            found = PR_FALSE; 
        }
        else if (atom == nsGkAtoms::td ||
                 atom == nsGkAtoms::th) 
        {
          result = ParentOffset(tempNode, getter_AddRefs(endNode), &endOffset);
          if (NS_FAILED(result))
            return result;
          if (*aDir == eDirNext) 
            endOffset++;
          found = PR_TRUE;
          dirtyend = PR_TRUE;
        }
        result = tempNode->GetParentNode(getter_AddRefs(tempNode2));
        if (NS_FAILED(result) || !tempNode2)
          return NS_ERROR_FAILURE;
        tempNode = tempNode2;
      }
      if (!found)
        return NS_ERROR_FAILURE;
    }
  }
  if (*aDir == eDirNext)
  {
    if (FetchAnchorNode() == startNode.get() && FetchFocusNode() == endNode.get() &&
      FetchAnchorOffset() == startOffset && FetchFocusOffset() == endOffset)
    {
      *aFixupState = PR_FALSE;
      return NS_ERROR_FAILURE;
    }
  }
  else
  {
    if (FetchAnchorNode() == endNode.get() && FetchFocusNode() == startNode.get() &&
      FetchAnchorOffset() == endOffset && FetchFocusOffset() == startOffset)
    {
      *aFixupState = PR_FALSE;
      return NS_ERROR_FAILURE;
    }
  }
  if (mFixupState && !dirtyend && !dirtystart)
  {
    dirtystart = PR_TRUE;
    dirtystart = PR_TRUE;
    *aFixupState = PR_TRUE;
    mFixupState = PR_FALSE;
  }
  else
  if ((dirtystart || dirtyend) && *aDir != mDirection) 
  {
    *aFixupState = PR_TRUE;
    
  }
  else
  if (dirtystart && (FetchAnchorNode() != startNode.get() || FetchAnchorOffset() != startOffset))
  {
    *aFixupState = PR_TRUE;
    mFixupState  = PR_TRUE;
  }
  else
  if (dirtyend && (FetchFocusNode() != endNode.get() || FetchFocusOffset() != endOffset))
  {
    *aFixupState = PR_TRUE;
    mFixupState  = PR_TRUE;
  }
  else
  {
    mFixupState = dirtystart || dirtyend;
    *aFixupState = PR_FALSE;
  }
  if (dirtystart || dirtyend){
    if (*aDir == eDirNext)
    {
      if (NS_FAILED(aRange->SetStart(startNode,startOffset)) || NS_FAILED(aRange->SetEnd(endNode, endOffset)))
      {
        *aDir = eDirPrevious;
        aRange->SetStart(endNode, endOffset);
        aRange->SetEnd(startNode, startOffset);
      }
    }
    else
    {
      if (NS_FAILED(aRange->SetStart(endNode,endOffset)) || NS_FAILED(aRange->SetEnd(startNode, startOffset)))
      {
        *aDir = eDirNext;
        aRange->SetStart(startNode, startOffset);
        aRange->SetEnd(endNode, endOffset);
      }
    }
  }
  return NS_OK;
}
#endif 




NS_IMETHODIMP
nsTypedSelection::SetOriginalAnchorPoint(nsIDOMNode *aNode, PRInt32 aOffset)
{
  if (!aNode){
    mOriginalAnchorRange = 0;
    return NS_OK;
  }
  nsCOMPtr<nsIDOMRange> newRange;
  nsresult result;
  NS_NewRange(getter_AddRefs(newRange));
  if (!newRange) return NS_ERROR_OUT_OF_MEMORY;

  result = newRange->SetStart(aNode,aOffset);
  if (NS_FAILED(result))
    return result;
  result = newRange->SetEnd(aNode,aOffset);
  if (NS_FAILED(result))
    return result;

  mOriginalAnchorRange = newRange;
  return result;
}



NS_IMETHODIMP
nsTypedSelection::GetOriginalAnchorPoint(nsIDOMNode **aNode, PRInt32 *aOffset)
{
  if (!aNode || !aOffset || !mOriginalAnchorRange)
    return NS_ERROR_NULL_POINTER;
  nsresult result;
  result = mOriginalAnchorRange->GetStartContainer(aNode);
  if (NS_FAILED(result))
    return result;
  result = mOriginalAnchorRange->GetStartOffset(aOffset);
  return result;
}





NS_IMETHODIMP
nsTypedSelection::CopyRangeToAnchorFocus(nsIDOMRange *aRange)
{
  nsCOMPtr<nsIDOMNode> startNode;
  nsCOMPtr<nsIDOMNode> endNode;
  PRInt32 startOffset;
  PRInt32 endOffset;
  aRange->GetStartContainer(getter_AddRefs(startNode));
  aRange->GetEndContainer(getter_AddRefs(endNode));
  aRange->GetStartOffset(&startOffset);
  aRange->GetEndOffset(&endOffset);
  if (NS_FAILED(mAnchorFocusRange->SetStart(startNode,startOffset)))
  {
    if (NS_FAILED(mAnchorFocusRange->SetEnd(endNode,endOffset)))
      return NS_ERROR_FAILURE;
    if (NS_FAILED(mAnchorFocusRange->SetStart(startNode,startOffset)))
      return NS_ERROR_FAILURE;
  }
  else if (NS_FAILED(mAnchorFocusRange->SetEnd(endNode,endOffset)))
          return NS_ERROR_FAILURE;
  return NS_OK;
}






























NS_IMETHODIMP
nsTypedSelection::Extend(nsIDOMNode* aParentNode, PRInt32 aOffset)
{
  if (!aParentNode)
    return NS_ERROR_INVALID_ARG;

  
  if (!mAnchorFocusRange)
    return NS_ERROR_NOT_INITIALIZED;

  if (!mFrameSelection)
    return NS_ERROR_NOT_INITIALIZED; 

  nsresult res;
  if (!IsValidSelectionPoint(mFrameSelection, aParentNode))
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIDOMRange> difRange;
  NS_NewRange(getter_AddRefs(difRange));
  nsCOMPtr<nsIDOMRange> range;

  if (FetchFocusNode() ==  aParentNode && FetchFocusOffset() == aOffset)
    return NS_ERROR_FAILURE;

  res = mAnchorFocusRange->CloneRange(getter_AddRefs(range));
  

  nsCOMPtr<nsIDOMNode> startNode;
  nsCOMPtr<nsIDOMNode> endNode;
  PRInt32 startOffset;
  PRInt32 endOffset;

  range->GetStartContainer(getter_AddRefs(startNode));
  range->GetEndContainer(getter_AddRefs(endNode));
  range->GetStartOffset(&startOffset);
  range->GetEndOffset(&endOffset);


  nsDirection dir = GetDirection();
  PRBool fixupState = PR_FALSE; 
  if (NS_FAILED(res))
    return res;

  NS_NewRange(getter_AddRefs(difRange));
  

  if (NS_FAILED(res))
    return res;
  PRInt32 result1 = CompareDOMPoints(FetchAnchorNode(),
                                     FetchAnchorOffset(),
                                     FetchFocusNode(),
                                     FetchFocusOffset());
  
  PRInt32 result2 = CompareDOMPoints(FetchFocusNode(),
                                     FetchFocusOffset(),
                                     aParentNode, aOffset);
  
  PRInt32 result3 = CompareDOMPoints(FetchAnchorNode(),
                                     FetchAnchorOffset(),
                                     aParentNode, aOffset);

  if (result2 == 0) 
    return NS_OK;

  nsCOMPtr<nsPresContext>  presContext;
  GetPresContext(getter_AddRefs(presContext));
  if ((result1 == 0 && result3 < 0) || (result1 <= 0 && result2 < 0)){
    
    res = range->SetEnd(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;
    dir = eDirNext;
    res = difRange->SetEnd(FetchEndParent(range), FetchEndOffset(range));
    res |= difRange->SetStart(FetchFocusNode(), FetchFocusOffset());
    if (NS_FAILED(res))
      return res;
#ifdef OLD_SELECTION
    res = FixupSelectionPoints(range, &dir, &fixupState);
#endif
    if (NS_FAILED(res))
      return res;
    if (fixupState) 
    {
#ifdef OLD_SELECTION
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
#endif
    }
    else{
      selectFrames(presContext, difRange , PR_TRUE);
    }
    res = CopyRangeToAnchorFocus(range);
    if (NS_FAILED(res))
      return res;
  }
  else if (result1 == 0 && result3 > 0){
    
    dir = eDirPrevious;
    res = range->SetStart(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;
#ifdef OLD_SELECTION
    res = FixupSelectionPoints(range, &dir, &fixupState);
    if (NS_FAILED(res))
      return res;
    if (fixupState) 
    {
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
    }
    else
#endif
      selectFrames(presContext, range, PR_TRUE);
    res = CopyRangeToAnchorFocus(range);
    if (NS_FAILED(res))
      return res;
  }
  else if (result3 <= 0 && result2 >= 0) {
    
    res = difRange->SetEnd(FetchFocusNode(), FetchFocusOffset());
    res |= difRange->SetStart(aParentNode, aOffset);
    if (NS_FAILED(res))
      return res;

    res = range->SetEnd(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;
#ifdef OLD_SELECTION    
    dir = eDirNext;
    res = FixupSelectionPoints(range, &dir, &fixupState);
#endif
    if (NS_FAILED(res))
      return res;
    if (fixupState) 
    {
#ifdef OLD_SELECTION    
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
#endif
    }
    else 
    {
      res = CopyRangeToAnchorFocus(range);
      if (NS_FAILED(res))
        return res;
      RemoveItem(mAnchorFocusRange);
      selectFrames(presContext, difRange, PR_FALSE);
      AddItem(mAnchorFocusRange);
      difRange->SetEnd(FetchEndParent(range),FetchEndOffset(range));
      selectFrames(presContext, difRange, PR_TRUE);
    }
  }
  else if (result1 >= 0 && result3 <= 0) {
    if (GetDirection() == eDirPrevious){
      res = range->SetStart(endNode,endOffset);
      if (NS_FAILED(res))
        return res;
    }
    dir = eDirNext;
    res = range->SetEnd(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;
#ifdef OLD_SELECTION
    res = FixupSelectionPoints(range, &dir, &fixupState);
    if (NS_FAILED(res))
      return res;

    if (fixupState) 
    {
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
    }
    else 
#endif
    {
      if (FetchFocusNode() != FetchAnchorNode() || FetchFocusOffset() != FetchAnchorOffset() ){
        res = difRange->SetStart(FetchFocusNode(), FetchFocusOffset());
        res |= difRange->SetEnd(FetchAnchorNode(), FetchAnchorOffset());
        if (NS_FAILED(res))
          return res;
        res = CopyRangeToAnchorFocus(range);
        if (NS_FAILED(res))
          return res;
        
        RemoveItem(mAnchorFocusRange);
        selectFrames(presContext, difRange , PR_FALSE);
        AddItem(mAnchorFocusRange);
      }
      else
      {
        res = CopyRangeToAnchorFocus(range);
        if (NS_FAILED(res))
          return res;
      }
      
      selectFrames(presContext, range , PR_TRUE);
    }
  }
  else if (result2 <= 0 && result3 >= 0) {
    
    res = difRange->SetEnd(aParentNode, aOffset);
    res |= difRange->SetStart(FetchFocusNode(), FetchFocusOffset());
    if (NS_FAILED(res))
      return res;
    dir = eDirPrevious;
    res = range->SetStart(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;

#ifdef OLD_SELECTION
    res = FixupSelectionPoints(range, &dir, &fixupState);
#endif
    if (NS_FAILED(res))
      return res;
    if (fixupState) 
    {
#ifdef OLD_SELECTION
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
#endif
    }
    else 
    {
      res = CopyRangeToAnchorFocus(range);
      if (NS_FAILED(res))
        return res;
      RemoveItem(mAnchorFocusRange);
      selectFrames(presContext, difRange , PR_FALSE);
      AddItem(mAnchorFocusRange);
      difRange->SetStart(FetchStartParent(range),FetchStartOffset(range));
      selectFrames(presContext, difRange, PR_TRUE);
    }
  }
  else if (result3 >= 0 && result1 <= 0) {
    if (GetDirection() == eDirNext){
      range->SetEnd(startNode,startOffset);
    }
    dir = eDirPrevious;
    res = range->SetStart(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;
#ifdef OLD_SELECTION
    res = FixupSelectionPoints(range, &dir, &fixupState);
    if (NS_FAILED(res))
      return res;
    if (fixupState) 
    {
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
    }
    else
#endif
    {
      
      if (FetchFocusNode() != FetchAnchorNode() || FetchFocusOffset() != FetchAnchorOffset() ){
        res = difRange->SetStart(FetchAnchorNode(), FetchAnchorOffset());
        res |= difRange->SetEnd(FetchFocusNode(), FetchFocusOffset());
        res = CopyRangeToAnchorFocus(range);
        if (NS_FAILED(res))
          return res;
        RemoveItem(mAnchorFocusRange);
        selectFrames(presContext, difRange, 0);
        AddItem(mAnchorFocusRange);
      }
      else
      {
        res = CopyRangeToAnchorFocus(range);
        if (NS_FAILED(res))
          return res;
      }
      
      selectFrames(presContext, range , PR_TRUE);
    }
  }
  else if (result2 >= 0 && result1 >= 0) {
    
    res = range->SetStart(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;
    dir = eDirPrevious;
    res = difRange->SetEnd(FetchFocusNode(), FetchFocusOffset());
    res |= difRange->SetStart(FetchStartParent(range), FetchStartOffset(range));
    if (NS_FAILED(res))
      return res;

#ifdef OLD_SELECTION
    res = FixupSelectionPoints(range, &dir, &fixupState);
#endif
    if (NS_FAILED(res))
      return res;
    if (fixupState) 
    {
#ifdef OLD_SELECTION
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
#endif
    }
    else {
      selectFrames(presContext, difRange, PR_TRUE);
    }
    res = CopyRangeToAnchorFocus(range);
    if (NS_FAILED(res))
      return res;
  }

  DEBUG_OUT_RANGE(range);
#if 0
  if (eDirNext == mDirection)
    printf("    direction = 1  LEFT TO RIGHT\n");
  else
    printf("    direction = 0  RIGHT TO LEFT\n");
#endif
  SetDirection(dir);
#ifdef DEBUG_SELECTION
  if (aParentNode)
  {
    nsCOMPtr<nsIContent>content;
    content = do_QueryInterface(aParentNode);

    const char *tagString;
    content->Tag()->GetUTF8String(&tagString);
    printf ("Sel. Extend to %p %s %d\n", content.get(), tagString, aOffset);
  }
  else {
    printf ("Sel. Extend set to null parent.\n");
  }
#endif
  if (!mFrameSelection)
    return NS_OK;
  return mFrameSelection->NotifySelectionListeners(GetType());
}

static nsresult
GetChildOffset(nsIDOMNode *aChild, nsIDOMNode *aParent, PRInt32 &aOffset)
{
  NS_ASSERTION((aChild && aParent), "bad args");
  nsCOMPtr<nsIContent> content = do_QueryInterface(aParent);
  nsCOMPtr<nsIContent> cChild = do_QueryInterface(aChild);

  if (!cChild || !content)
    return NS_ERROR_NULL_POINTER;

  aOffset = content->IndexOf(cChild);

  return NS_OK;
}

NS_IMETHODIMP
nsTypedSelection::SelectAllChildren(nsIDOMNode* aParentNode)
{
  NS_ENSURE_ARG_POINTER(aParentNode);
  
  if (mFrameSelection) 
  {
    mFrameSelection->PostReason(nsISelectionListener::SELECTALL_REASON);
  }
  nsresult result = Collapse(aParentNode, 0);
  if (NS_SUCCEEDED(result))
  {
    nsCOMPtr<nsIDOMNode>lastChild;
    result = aParentNode->GetLastChild(getter_AddRefs(lastChild));
    if ((NS_SUCCEEDED(result)) && lastChild)
    {
      PRInt32 numBodyChildren=0;
      GetChildOffset(lastChild, aParentNode, numBodyChildren);
      if (mFrameSelection) 
      {
        mFrameSelection->PostReason(nsISelectionListener::SELECTALL_REASON);
      }
      result = Extend(aParentNode, numBodyChildren+1);
    }
  }
  return result;
}

NS_IMETHODIMP
nsTypedSelection::ContainsNode(nsIDOMNode* aNode, PRBool aAllowPartial,
                               PRBool* aYes)
{
  nsresult rv;
  if (!aYes)
    return NS_ERROR_NULL_POINTER;
  NS_ASSERTION(ValidateRanges(), "Ranges out of sync");
  *aYes = PR_FALSE;

  if (mRanges.Length() == 0 || !aNode)
    return NS_OK;
  
  PRUint16 nodeType;
  aNode->GetNodeType(&nodeType);
  PRUint32 nodeLength;
  if (nodeType == nsIDOMNode::TEXT_NODE) {
    nsAutoString nodeValue;
    rv = aNode->GetNodeValue(nodeValue);
    NS_ENSURE_SUCCESS(rv, rv);
    nodeLength = nodeValue.Length();
  } else {
    nsCOMPtr<nsIDOMNodeList> aChildNodes;
    rv = aNode->GetChildNodes(getter_AddRefs(aChildNodes));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aChildNodes->GetLength(&nodeLength);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMArray<nsIDOMRange> overlappingRanges;
  rv = GetRangesForIntervalCOMArray(aNode, 0, aNode, nodeLength,
                                    PR_FALSE, &overlappingRanges);
  NS_ENSURE_SUCCESS(rv, rv);
  if (overlappingRanges.Count() == 0)
    return NS_OK; 
  
  
  if (aAllowPartial) {
    *aYes = PR_TRUE;
    return NS_OK;
  }

  
  if (nodeType == nsIDOMNode::TEXT_NODE) {
    *aYes = PR_TRUE;
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIContent> content (do_QueryInterface(aNode, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRInt32 i = 0; i < overlappingRanges.Count(); i ++) {
    PRBool nodeStartsBeforeRange, nodeEndsAfterRange;
    if (NS_SUCCEEDED(nsRange::CompareNodeToRange(content, overlappingRanges[i],
                                                 &nodeStartsBeforeRange,
                                                 &nodeEndsAfterRange))) {
      if (!nodeStartsBeforeRange && !nodeEndsAfterRange) {
        *aYes = PR_TRUE;
        return NS_OK;
      }
    }
  }
  return NS_OK;
}


nsresult
nsTypedSelection::GetPresContext(nsPresContext **aPresContext)
{
  if (!mFrameSelection)
    return NS_ERROR_FAILURE;
  nsIPresShell *shell = mFrameSelection->GetShell();

  if (!shell)
    return NS_ERROR_NULL_POINTER;

  NS_IF_ADDREF(*aPresContext = shell->GetPresContext());
  return NS_OK;
}

nsresult
nsTypedSelection::GetPresShell(nsIPresShell **aPresShell)
{
  if (mPresShellWeak)
  {
    nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShellWeak);
    if (presShell)
      NS_ADDREF(*aPresShell = presShell);
    return NS_OK;
  }
  nsresult rv = NS_OK;
  if (!mFrameSelection)
    return NS_ERROR_FAILURE;

  nsIPresShell *shell = mFrameSelection->GetShell();

  mPresShellWeak = do_GetWeakReference(shell);    
  if (mPresShellWeak)
    NS_ADDREF(*aPresShell = shell);
  return rv;
}

nsresult
nsTypedSelection::GetRootScrollableView(nsIScrollableView **aScrollableView)
{
  
  
  
  
  NS_ENSURE_ARG_POINTER(aScrollableView);

  if (!mFrameSelection)
    return NS_ERROR_FAILURE;

  nsIScrollableView *scrollView = mFrameSelection->GetScrollableView();
  if (!scrollView)
  {
    nsCOMPtr<nsIPresShell> presShell;

    nsresult rv = GetPresShell(getter_AddRefs(presShell));

    if (NS_FAILED(rv))
      return rv;

    if (!presShell)
      return NS_ERROR_NULL_POINTER;

    nsIViewManager* viewManager = presShell->GetViewManager();

    if (!viewManager)
      return NS_ERROR_NULL_POINTER;

    
    
    
    
    return viewManager->GetRootScrollableView(aScrollableView);
  }

  *aScrollableView = scrollView;
  return NS_OK;
}

nsresult
nsTypedSelection::GetFrameToScrolledViewOffsets(nsIScrollableView *aScrollableView, nsIFrame *aFrame, nscoord *aX, nscoord *aY)
{
  nsresult rv = NS_OK;
  if (!mFrameSelection)
    return NS_ERROR_FAILURE;

  if (!aScrollableView || !aFrame || !aX || !aY) {
    return NS_ERROR_NULL_POINTER;
  }

  *aX = 0;
  *aY = 0;

  nsIView*  scrolledView;
  nsPoint   offset;
  nsIView*  closestView;
          
  
  
  aScrollableView->GetScrolledView(scrolledView);
  nsIPresShell *shell = mFrameSelection->GetShell();

  if (!shell)
    return NS_ERROR_NULL_POINTER;

  aFrame->GetOffsetFromView(offset, &closestView);

  
  
  offset += closestView->GetOffsetTo(scrolledView);

  *aX = offset.x;
  *aY = offset.y;

  return rv;
}

nsresult
nsTypedSelection::GetPointFromOffset(nsIFrame *aFrame, PRInt32 aContentOffset, nsPoint *aPoint)
{
  nsresult rv = NS_OK;
  if (!mFrameSelection)
    return NS_ERROR_FAILURE;
  if (!aFrame || !aPoint)
    return NS_ERROR_NULL_POINTER;

  aPoint->x = 0;
  aPoint->y = 0;

  
  
  
  

  nsIPresShell *shell = mFrameSelection->GetShell();
  if (!shell)
    return NS_ERROR_NULL_POINTER;

  nsPresContext *presContext = shell->GetPresContext();
  if (!presContext)
    return NS_ERROR_NULL_POINTER;
  
  
  
  
  

  nsIWidget* widget = nsnull;
  nsIView *closestView = nsnull;
  nsPoint offset(0, 0);

  rv = aFrame->GetOffsetFromView(offset, &closestView);

  while (!widget && closestView)
  {
    widget = closestView->GetWidget();

    if (!widget)
    {
      closestView = closestView->GetParent();
    }
  }

  if (!closestView)
    return NS_ERROR_FAILURE;

  
  
  
  
  

  nsCOMPtr<nsIRenderingContext> rendContext;

  rv = presContext->DeviceContext()->
    CreateRenderingContext(closestView, *getter_AddRefs(rendContext));
  
  if (NS_FAILED(rv))
    return rv;

  if (!rendContext)
    return NS_ERROR_NULL_POINTER;

  
  
  

  rv = aFrame->GetPointFromOffset(presContext, rendContext, aContentOffset, aPoint);

  return rv;
}

nsresult
nsTypedSelection::GetSelectionRegionRectAndScrollableView(SelectionRegion aRegion, nsRect *aRect, nsIScrollableView **aScrollableView)
{
  if (!mFrameSelection)
    return NS_ERROR_FAILURE;  

  NS_ENSURE_TRUE(aRect && aScrollableView, NS_ERROR_NULL_POINTER);

  aRect->SetRect(0, 0, 0, 0);
  *aScrollableView = nsnull;

  nsIDOMNode *node       = nsnull;
  PRInt32     nodeOffset = 0;
  nsIFrame   *frame      = nsnull;

  switch (aRegion) {
    case nsISelectionController::SELECTION_ANCHOR_REGION:
      node       = FetchAnchorNode();
      nodeOffset = FetchAnchorOffset();
      break;
    case nsISelectionController::SELECTION_FOCUS_REGION:
      node       = FetchFocusNode();
      nodeOffset = FetchFocusOffset();
      break;
    default:
      return NS_ERROR_FAILURE;
  }

  if (!node)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIContent> content = do_QueryInterface(node);
  NS_ENSURE_TRUE(content.get(), NS_ERROR_FAILURE);
  PRInt32 frameOffset = 0;
  frame = mFrameSelection->GetFrameForNodeOffset(content, nodeOffset,
                                                 mFrameSelection->GetHint(),
                                                 &frameOffset);
  if (!frame)
    return NS_ERROR_FAILURE;

  
  nsIFrame* parentWithView = frame->GetAncestorWithView();
  if (!parentWithView)
    return NS_ERROR_FAILURE;
  nsIView* view = parentWithView->GetView();
  *aScrollableView =
    nsLayoutUtils::GetNearestScrollingView(view, nsLayoutUtils::eEither);
  if (!*aScrollableView)
    return NS_OK;

  
  
  PRUint16 nodeType = nsIDOMNode::ELEMENT_NODE;
  nsresult rv = node->GetNodeType(&nodeType);
  if (NS_FAILED(rv))
    return rv;

  nsPoint pt(0, 0);
  if (nodeType == nsIDOMNode::TEXT_NODE) {
    nsIFrame* childFrame = nsnull;
    frameOffset = 0;
    rv = frame->GetChildFrameContainingOffset(nodeOffset,
                                              mFrameSelection->GetHint(),
                                              &frameOffset, &childFrame);
    if (NS_FAILED(rv))
      return rv;
    if (!childFrame)
      return NS_ERROR_NULL_POINTER;

    frame = childFrame;

    
    rv = GetCachedFrameOffset(frame, nodeOffset, pt);
    if (NS_FAILED(rv))
      return rv;
  }

  
  *aRect = frame->GetRect();
  rv = GetFrameToScrolledViewOffsets(*aScrollableView, frame, &aRect->x,
                                     &aRect->y);
  NS_ENSURE_SUCCESS(rv, rv);

  if (nodeType == nsIDOMNode::TEXT_NODE) {
    aRect->x += pt.x;
  }
  else if (mFrameSelection->GetHint() == nsFrameSelection::HINTLEFT) {
    
    aRect->x += aRect->width;
  }

  nsRect clipRect = (*aScrollableView)->View()->GetBounds();
  rv = (*aScrollableView)->GetScrollPosition(clipRect.x, clipRect.y);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  PRInt32 pad = clipRect.width >> 2;

  if (pad <= 0)
    pad = 3; 

  if (aRect->x >= clipRect.XMost()) {
    aRect->width = pad;
  }
  else if (aRect->x <= clipRect.x) {
    aRect->x -= pad;
    aRect->width = pad;
  }
  else {
    aRect->width = 60; 
  }

  return rv;
}

static void
ClampPointInsideRect(nsPoint& aPoint, const nsRect& aRect)
{
  if (aPoint.x < aRect.x)
    aPoint.x = aRect.x;
  if (aPoint.x > aRect.XMost())
    aPoint.x = aRect.XMost();
  if (aPoint.y < aRect.y)
    aPoint.y = aRect.y;
  if (aPoint.y > aRect.YMost())
    aPoint.y = aRect.YMost();
}

nsresult
nsTypedSelection::ScrollRectIntoView(nsIScrollableView *aScrollableView,
                              nsRect& aRect,
                              PRIntn  aVPercent, 
                              PRIntn  aHPercent,
                              PRBool aScrollParentViews)
{
  nsresult rv = NS_OK;
  if (!mFrameSelection)
    return NS_OK;

  if (!aScrollableView)
    return NS_ERROR_NULL_POINTER;

  
  
  nsRect visibleRect = aScrollableView->View()->GetBounds();
  aScrollableView->GetScrollPosition(visibleRect.x, visibleRect.y);

  
  nscoord scrollOffsetX = visibleRect.x;
  nscoord scrollOffsetY = visibleRect.y;

  nsPresContext::ScrollbarStyles ss =
    nsLayoutUtils::ScrollbarStylesOfView(aScrollableView);

  
  if (ss.mVertical != NS_STYLE_OVERFLOW_HIDDEN) {
    if (NS_PRESSHELL_SCROLL_ANYWHERE == aVPercent) {
      
      
      if (aRect.y < visibleRect.y) {
        
        scrollOffsetY = aRect.y;
      } else if (aRect.YMost() > visibleRect.YMost()) {
        
        
        scrollOffsetY += aRect.YMost() - visibleRect.YMost();
        if (scrollOffsetY > aRect.y) {
          scrollOffsetY = aRect.y;
        }
      }
    } else {
      
      nscoord frameAlignY = aRect.y + (aRect.height * aVPercent) / 100;
      scrollOffsetY = frameAlignY - (visibleRect.height * aVPercent) / 100;
    }
  }

  
  if (ss.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN) {
    if (NS_PRESSHELL_SCROLL_ANYWHERE == aHPercent) {
      
      
      if (aRect.x < visibleRect.x) {
        
        scrollOffsetX = aRect.x;
      } else if (aRect.XMost() > visibleRect.XMost()) {
        
        
        scrollOffsetX += aRect.XMost() - visibleRect.XMost();
        if (scrollOffsetX > aRect.x) {
          scrollOffsetX = aRect.x;
        }
      }
        
    } else {
      
      nscoord frameAlignX = aRect.x + (aRect.width * aHPercent) / 100;
      scrollOffsetX = frameAlignX - (visibleRect.width * aHPercent) / 100;
    }
  }

  aScrollableView->ScrollTo(scrollOffsetX, scrollOffsetY, NS_VMREFRESH_IMMEDIATE);

  if (aScrollParentViews)
  {
    
    
    

    nsIView *scrolledView = 0;

    rv = aScrollableView->GetScrolledView(scrolledView);

    if (NS_FAILED(rv))
      return rv;

    if (!scrolledView)
      return NS_ERROR_FAILURE;

    
    
    

    nsIView *view = aScrollableView->View()->GetParent();

    if (view)
    {
      nsIScrollableView *parentSV =
        nsLayoutUtils::GetNearestScrollingView(view, nsLayoutUtils::eEither);

      if (parentSV)
      {
        
        
        
        
        
        
        nsRect svRect = scrolledView->GetBounds() - scrolledView->GetPosition();
        nsPoint topLeft = aRect.TopLeft();
        nsPoint bottomRight = aRect.BottomRight();
        ClampPointInsideRect(topLeft, svRect);
        ClampPointInsideRect(bottomRight, svRect);
        nsRect newRect(topLeft.x, topLeft.y, bottomRight.x - topLeft.x,
                       bottomRight.y - topLeft.y);

        
        
        
        

        rv = parentSV->GetScrolledView(view);

        if (NS_FAILED(rv))
          return rv;

        if (!view)
          return NS_ERROR_FAILURE;

        nscoord offsetX, offsetY;
        rv = GetViewAncestorOffset(scrolledView, view, &offsetX, &offsetY);

        if (NS_FAILED(rv))
          return rv;

        newRect.x     += offsetX;
        newRect.y     += offsetY;

        
        
        

        rv = ScrollRectIntoView(parentSV, newRect, aVPercent, aHPercent, aScrollParentViews);
      }
    }
  }

  return rv;
}

NS_IMETHODIMP
nsTypedSelection::ScrollSelectionIntoViewEvent::Run()
{
  if (!mTypedSelection)
    return NS_OK;  

  mTypedSelection->mScrollEvent.Forget();
  mTypedSelection->ScrollIntoView(mRegion, PR_TRUE);
  return NS_OK;
}

nsresult
nsTypedSelection::PostScrollSelectionIntoViewEvent(SelectionRegion aRegion)
{
  
  
  
  
  mScrollEvent.Revoke();

  nsRefPtr<ScrollSelectionIntoViewEvent> ev =
      new ScrollSelectionIntoViewEvent(this, aRegion);
  nsresult rv = NS_DispatchToCurrentThread(ev);
  NS_ENSURE_SUCCESS(rv, rv);

  mScrollEvent = ev;
  return NS_OK;
}

NS_IMETHODIMP
nsTypedSelection::ScrollIntoView(SelectionRegion aRegion, PRBool aIsSynchronous)
{
  nsresult result;
  if (!mFrameSelection)
    return NS_OK;

  if (mFrameSelection->GetBatching())
    return NS_OK;

  if (!aIsSynchronous)
    return PostScrollSelectionIntoViewEvent(aRegion);

  
  
  
  
  nsCOMPtr<nsIPresShell> presShell;
  result = GetPresShell(getter_AddRefs(presShell));
  if (NS_FAILED(result) || !presShell)
    return result;
  nsCOMPtr<nsICaret> caret;
  presShell->GetCaret(getter_AddRefs(caret));
  if (caret)
  {
    StCaretHider  caretHider(caret);      

    
    
    
    
    presShell->FlushPendingNotifications(Flush_OnlyReflow);

    
    
    

    nsRect rect;
    nsIScrollableView *scrollableView = 0;

    result = GetSelectionRegionRectAndScrollableView(aRegion, &rect, &scrollableView);

    if (NS_FAILED(result))
      return result;

    
    
    
    if (!scrollableView)
      return NS_OK;

    result = ScrollRectIntoView(scrollableView, rect, NS_PRESSHELL_SCROLL_ANYWHERE, NS_PRESSHELL_SCROLL_ANYWHERE, PR_TRUE);
  }
  return result;
}



NS_IMETHODIMP
nsTypedSelection::AddSelectionListener(nsISelectionListener* aNewListener)
{
  if (!aNewListener)
    return NS_ERROR_NULL_POINTER;
  return mSelectionListeners.AppendObject(aNewListener) ? NS_OK : NS_ERROR_FAILURE;      
}



NS_IMETHODIMP
nsTypedSelection::RemoveSelectionListener(nsISelectionListener* aListenerToRemove)
{
  if (!aListenerToRemove )
    return NS_ERROR_NULL_POINTER;
  return mSelectionListeners.RemoveObject(aListenerToRemove) ? NS_OK : NS_ERROR_FAILURE; 
}


nsresult
nsTypedSelection::NotifySelectionListeners()
{
  if (!mFrameSelection)
    return NS_OK;
 
  if (mFrameSelection->GetBatching()){
    mFrameSelection->SetDirty();
    return NS_OK;
  }
  PRInt32 cnt = mSelectionListeners.Count();
  nsCOMArray<nsISelectionListener> selectionListeners(mSelectionListeners);
  
  nsCOMPtr<nsIDOMDocument> domdoc;
  nsCOMPtr<nsIPresShell> shell;
  nsresult rv = GetPresShell(getter_AddRefs(shell));
  if (NS_SUCCEEDED(rv) && shell)
    domdoc = do_QueryInterface(shell->GetDocument());
  short reason = mFrameSelection->PopReason();
  for (PRInt32 i = 0; i < cnt; i++)
  {
    nsISelectionListener* thisListener = selectionListeners[i];
    if (thisListener)
      thisListener->NotifySelectionChanged(domdoc, this, reason);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsTypedSelection::StartBatchChanges()
{
  if (mFrameSelection)
    mFrameSelection->StartBatchChanges();

  return NS_OK;
}



NS_IMETHODIMP
nsTypedSelection::EndBatchChanges()
{
  if (mFrameSelection)
    mFrameSelection->EndBatchChanges();

  return NS_OK;
}



NS_IMETHODIMP
nsTypedSelection::DeleteFromDocument()
{
  if (!mFrameSelection)
    return NS_OK;
  return mFrameSelection->DeleteFromDocument();
}




NS_IMETHODIMP
nsTypedSelection::SelectionLanguageChange(PRBool aLangRTL)
{
  if (!mFrameSelection)
    return NS_ERROR_NOT_INITIALIZED; 
  nsresult result;
  nsCOMPtr<nsIDOMNode>  focusNode;
  nsCOMPtr<nsIContent> focusContent;
  PRInt32 focusOffset;
  nsIFrame *focusFrame = 0;

  focusOffset = FetchFocusOffset();
  focusNode = FetchFocusNode();
  result = GetPrimaryFrameForFocusNode(&focusFrame, nsnull, PR_FALSE);
  if (NS_FAILED(result) || !focusFrame)
    return result?result:NS_ERROR_FAILURE;

  PRInt32 frameStart, frameEnd;
  focusFrame->GetOffsets(frameStart, frameEnd);
  nsCOMPtr<nsPresContext> context;
  PRUint8 levelBefore, levelAfter;
  result = GetPresContext(getter_AddRefs(context));
  if (NS_FAILED(result) || !context)
    return result?result:NS_ERROR_FAILURE;

  PRUint8 level = NS_GET_EMBEDDING_LEVEL(focusFrame);
  if ((focusOffset != frameStart) && (focusOffset != frameEnd))
    
    
    levelBefore = levelAfter = level;
  else {
    
    
    focusContent = do_QueryInterface(focusNode);
    











    nsPrevNextBidiLevels levels = mFrameSelection->
      GetPrevNextBidiLevels(focusContent, focusOffset, PR_FALSE);
      
    levelBefore = levels.mLevelBefore;
    levelAfter = levels.mLevelAfter;
  }

  if ((levelBefore & 1) == (levelAfter & 1)) {
    
    
    
    
    if ((level != levelBefore) && (level != levelAfter))
      level = PR_MIN(levelBefore, levelAfter);
    if ((level & 1) == aLangRTL)
      mFrameSelection->SetCaretBidiLevel(level);
    else
      mFrameSelection->SetCaretBidiLevel(level + 1);
  }
  else {
    
    
    if ((levelBefore & 1) == aLangRTL)
      mFrameSelection->SetCaretBidiLevel(levelBefore);
    else
      mFrameSelection->SetCaretBidiLevel(levelAfter);
  }
  
  
  
  mFrameSelection->InvalidateDesiredX();
  
  return NS_OK;
}




nsAutoCopyListener* nsAutoCopyListener::sInstance = nsnull;

NS_IMPL_ISUPPORTS1(nsAutoCopyListener, nsISelectionListener)




























NS_IMETHODIMP
nsAutoCopyListener::NotifySelectionChanged(nsIDOMDocument *aDoc,
                                           nsISelection *aSel, PRInt16 aReason)
{
  if (!(aReason & nsISelectionListener::MOUSEUP_REASON   || 
        aReason & nsISelectionListener::SELECTALL_REASON ||
        aReason & nsISelectionListener::KEYPRESS_REASON))
    return NS_OK; 

  PRBool collapsed;
  if (!aDoc || !aSel ||
      NS_FAILED(aSel->GetIsCollapsed(&collapsed)) || collapsed) {
#ifdef DEBUG_CLIPBOARD
    fprintf(stderr, "CLIPBOARD: no selection/collapsed selection\n");
#endif
    
    return NS_OK;
  }

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDoc);
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  
  return nsCopySupport::HTMLCopy(aSel, doc, nsIClipboard::kSelectionClipboard);
}
