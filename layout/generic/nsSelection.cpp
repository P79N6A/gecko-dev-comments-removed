









#include "mozilla/Selection.h"

#include "mozilla/Attributes.h"

#include "nsCOMPtr.h"
#include "nsWeakReference.h"
#include "nsIFactory.h"
#include "nsIEnumerator.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsFrameSelection.h"
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
#include "nsIScrollableFrame.h"
#include "nsCCUncollectableMarker.h"
#include "nsIContentIterator.h"
#include "nsIDocumentEncoder.h"
#include "nsTextFragment.h"


#include "nsFrameTraversal.h"
#include "nsILineIterator.h"
#include "nsGkAtoms.h"
#include "nsIFrameTraversal.h"
#include "nsLayoutUtils.h"
#include "nsLayoutCID.h"
#include "nsBidiPresUtils.h"
static NS_DEFINE_CID(kFrameTraversalCID, NS_FRAMETRAVERSAL_CID);
#include "nsTextFrame.h"

#include "nsIDOMText.h"

#include "nsContentUtils.h"
#include "nsThreadUtils.h"
#include "mozilla/Preferences.h"


#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsCaret.h"


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

#include "nsDOMError.h"
#include "mozilla/dom/Element.h"

using namespace mozilla;



static NS_DEFINE_IID(kCContentIteratorCID, NS_CONTENTITERATOR_CID);


class nsFrameSelection;
class nsAutoScrollTimer;

static bool IsValidSelectionPoint(nsFrameSelection *aFrameSel, nsINode *aNode);

static nsIAtom *GetTag(nsINode *aNode);

static nsINode* ParentOffset(nsINode *aNode, PRInt32 *aChildOffset);
static nsINode* GetCellParent(nsINode *aDomNode);

#ifdef PRINT_RANGE
static void printRange(nsRange *aDomRange);
#define DEBUG_OUT_RANGE(x)  printRange(x)
#else
#define DEBUG_OUT_RANGE(x)  
#endif 









struct CachedOffsetForFrame {
  CachedOffsetForFrame()
  : mCachedFrameOffset(0, 0) 
  , mLastCaretFrame(nsnull)
  , mLastContentOffset(0)
  , mCanCacheFrameOffset(false)
  {}

  nsPoint      mCachedFrameOffset;      
  nsIFrame*    mLastCaretFrame;         
  PRInt32      mLastContentOffset;      
  bool mCanCacheFrameOffset;    
};

static RangeData sEmptyData(nsnull);


class NS_STACK_CLASS nsSelectionBatcher MOZ_FINAL
{
private:
  nsCOMPtr<nsISelectionPrivate> mSelection;
public:
  nsSelectionBatcher(nsISelectionPrivate *aSelection) : mSelection(aSelection)
  {
    if (mSelection) mSelection->StartBatchChanges();
  }
  ~nsSelectionBatcher() 
  { 
    if (mSelection) mSelection->EndBatchChanges();
  }
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

  
  nsresult Start(nsPresContext *aPresContext, nsPoint &aPoint)
  {
    mPoint = aPoint;

    
    
    mPresContext = aPresContext;

    mContent = nsIPresShell::GetCapturingContent();

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

  nsresult Init(nsFrameSelection* aFrameSelection, Selection* aSelection)
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
      nsWeakFrame frame =
        mContent ? mPresContext->GetPrimaryFrameFor(mContent) : nsnull;
      if (!frame)
        return NS_OK;
      mContent = nsnull;

      nsPoint pt = mPoint -
        frame->GetOffsetTo(mPresContext->PresShell()->FrameManager()->GetRootFrame());
      mFrameSelection->HandleDrag(frame, pt);
      if (!frame.IsAlive())
        return NS_OK;

      NS_ASSERTION(frame->PresContext() == mPresContext, "document mismatch?");
      mSelection->DoAutoScroll(frame, pt);
    }
    return NS_OK;
  }
private:
  nsFrameSelection *mFrameSelection;
  Selection* mSelection;
  nsPresContext *mPresContext;
  
  nsPoint mPoint;
  nsCOMPtr<nsITimer> mTimer;
  nsCOMPtr<nsIContent> mContent;
  PRUint32 mDelay;
};

NS_IMPL_ISUPPORTS1(nsAutoScrollTimer, nsITimerCallback)

nsresult NS_NewDomSelection(nsISelection **aDomSelection)
{
  Selection* rlist = new Selection;
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
    case nsISelectionController::SELECTION_FIND: return 7; break;
    case nsISelectionController::SELECTION_URLSECONDARY: return 8; break;
    default:
      return -1; break;
    }
    
    return 0;
}

static SelectionType 
GetSelectionTypeFromIndex(PRInt8 aIndex)
{
  switch (aIndex)
  {
    case 0: return nsISelectionController::SELECTION_NORMAL; break;
    case 1: return nsISelectionController::SELECTION_SPELLCHECK; break;
    case 2: return nsISelectionController::SELECTION_IME_RAWINPUT; break;
    case 3: return nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT; break;
    case 4: return nsISelectionController::SELECTION_IME_CONVERTEDTEXT; break;
    case 5: return nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT; break;
    case 6: return nsISelectionController::SELECTION_ACCESSIBILITY; break;
    case 7: return nsISelectionController::SELECTION_FIND; break;
    case 8: return nsISelectionController::SELECTION_URLSECONDARY; break;
    default:
      return nsISelectionController::SELECTION_NORMAL; break;
  }
  
  return 0;
}














bool         
IsValidSelectionPoint(nsFrameSelection *aFrameSel, nsINode *aNode)
{
  if (!aFrameSel || !aNode)
    return false;

  nsIContent *limiter = aFrameSel->GetLimiter();
  if (limiter && limiter != aNode && limiter != aNode->GetParent()) {
    
    return false; 
  }

  limiter = aFrameSel->GetAncestorLimiter();
  return !limiter || nsContentUtils::ContentIsDescendantOf(aNode, limiter);
}


NS_IMPL_ADDREF(nsSelectionIterator)
NS_IMPL_RELEASE(nsSelectionIterator)

NS_INTERFACE_MAP_BEGIN(nsSelectionIterator)
  NS_INTERFACE_MAP_ENTRY(nsIEnumerator)
  NS_INTERFACE_MAP_ENTRY(nsIBidirectionalEnumerator)
NS_INTERFACE_MAP_END_AGGREGATED(mDomSelection)




nsSelectionIterator::nsSelectionIterator(Selection* aList)
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
  *aItem = static_cast<nsIDOMRange*>(CurrentItem());
  if (!*aItem) {
    return NS_ERROR_FAILURE;
  }

  NS_ADDREF(*aItem);
  return NS_OK;
}

nsRange*
nsSelectionIterator::CurrentItem()
{
  return mDomSelection->mRanges.SafeElementAt(mIndex, sEmptyData).mRange;
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







nsFrameSelection::nsFrameSelection()
{
  PRInt32 i;
  for (i = 0;i<nsISelectionController::NUM_SELECTIONTYPES;i++){
    mDomSelections[i] = new Selection(this);
    mDomSelections[i]->SetType(GetSelectionTypeFromIndex(i));
  }
  mBatching = 0;
  mChangesDuringBatching = false;
  mNotifyFrames = true;
  mLimiter = nsnull; 
  mAncestorLimiter = nsnull;
  
  mMouseDoubleDownState = false;
  
  mHint = HINTLEFT;
#ifdef IBMBIDI
  mCaretBidiLevel = BIDI_LEVEL_UNDEFINED;
#endif
  mDragSelectingCells = false;
  mSelectingTableCellMode = 0;
  mSelectedCellIndex = 0;

  
  
  if (Preferences::GetBool("clipboard.autocopy")) {
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
  mSelectionChangeReason = nsISelectionListener::NO_REASON;

  mDelayedMouseEventValid = false;
  
  
  
  mDelayedMouseEventIsShift = false;
  mDelayedMouseEventClickCount = 0;
}


NS_IMPL_CYCLE_COLLECTION_CLASS(nsFrameSelection)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsFrameSelection)
  PRInt32 i;
  for (i = 0; i < nsISelectionController::NUM_SELECTIONTYPES; ++i) {
    tmp->mDomSelections[i] = nsnull;
  }

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCellParent)
  tmp->mSelectingTableCellMode = 0;
  tmp->mDragSelectingCells = false;
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mStartSelectedCell)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mEndSelectedCell)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mAppendStartSelectedCell)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mUnselectCellOnMouseUp)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mMaintainRange)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsFrameSelection)
  if (tmp->mShell && tmp->mShell->GetDocument() &&
      nsCCUncollectableMarker::InGeneration(cb,
                                            tmp->mShell->GetDocument()->
                                              GetMarkedCCGeneration())) {
    return NS_SUCCESS_INTERRUPTED_TRAVERSE;
  }
  PRInt32 i;
  for (i = 0; i < nsISelectionController::NUM_SELECTIONTYPES; ++i) {
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mDomSelections[i],
                                                         nsISelection)
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCellParent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mStartSelectedCell)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mEndSelectedCell)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mAppendStartSelectedCell)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mUnselectCellOnMouseUp)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mMaintainRange, nsIDOMRange)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsFrameSelection)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsFrameSelection)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsFrameSelection)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END


nsresult
nsFrameSelection::FetchDesiredX(nscoord &aDesiredX) 
{
  if (!mShell)
  {
    NS_ERROR("fetch desired X failed");
    return NS_ERROR_FAILURE;
  }
  if (mDesiredXSet)
  {
    aDesiredX = mDesiredX;
    return NS_OK;
  }

  nsRefPtr<nsCaret> caret = mShell->GetCaret();
  if (!caret)
    return NS_ERROR_NULL_POINTER;

  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  nsresult result = caret->SetCaretDOMSelection(mDomSelections[index]);
  if (NS_FAILED(result))
    return result;

  nsRect coord;
  nsIFrame* caretFrame = caret->GetGeometry(mDomSelections[index], &coord);
  if (!caretFrame)
    return NS_ERROR_FAILURE;
  nsPoint viewOffset(0, 0);
  nsIView* view = nsnull;
  caretFrame->GetOffsetFromView(viewOffset, &view);
  if (view)
    coord.x += viewOffset.x;

  aDesiredX = coord.x;
  return NS_OK;
}



void
nsFrameSelection::InvalidateDesiredX() 
{
  mDesiredXSet = false;
}



void
nsFrameSelection::SetDesiredX(nscoord aX) 
{
  mDesiredX = aX;
  mDesiredXSet = true;
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

  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  if (!mDomSelections[index])
    return NS_ERROR_NULL_POINTER;

  result = mDomSelections[index]->GetAnchorNode(getter_AddRefs(anchorNode));

  if (NS_FAILED(result))
    return result;

  if (!anchorNode)
    return NS_OK;

  result = mDomSelections[index]->GetAnchorOffset(&anchorOffset);

  if (NS_FAILED(result))
    return result;

  nsCOMPtr<nsIContent> anchorContent = do_QueryInterface(anchorNode);

  if (!anchorContent)
    return NS_ERROR_FAILURE;
  
  
  
  

  NS_ENSURE_STATE(mShell);
  nsIContent* anchorRoot = anchorContent->GetSelectionRootContent(mShell);
  NS_ENSURE_TRUE(anchorRoot, NS_ERROR_UNEXPECTED);

  
  
  

  nsIContent* content = aFrame->GetContent();

  if (content)
  {
    nsIContent* contentRoot = content->GetSelectionRootContent(mShell);
    NS_ENSURE_TRUE(contentRoot, NS_ERROR_UNEXPECTED);

    if (anchorRoot == contentRoot)
    {
      
      
      nsIContent* capturedContent = nsIPresShell::GetCapturingContent();
      if (capturedContent != content)
      {
        return NS_OK;
      }

      
      
      
      nsIFrame* rootFrame = mShell->FrameManager()->GetRootFrame();
      nsPoint ptInRoot = aPoint + aFrame->GetOffsetTo(rootFrame);
      nsIFrame* cursorFrame =
        nsLayoutUtils::GetFrameForPoint(rootFrame, ptInRoot);

      
      
      if (cursorFrame && cursorFrame->PresContext()->PresShell() == mShell)
      {
        nsIContent* cursorContent = cursorFrame->GetContent();
        NS_ENSURE_TRUE(cursorContent, NS_ERROR_FAILURE);
        nsIContent* cursorContentRoot =
          cursorContent->GetSelectionRootContent(mShell);
        NS_ENSURE_TRUE(cursorContentRoot, NS_ERROR_UNEXPECTED);
        if (cursorContentRoot == anchorRoot)
        {
          *aRetFrame = cursorFrame;
          aRetPoint = aPoint + aFrame->GetOffsetTo(cursorFrame);
          return NS_OK;
        }
      }
      
      
      
    }
  }

  
  
  
  
  

  *aRetFrame = anchorRoot->GetPrimaryFrame();

  if (!*aRetFrame)
    return NS_ERROR_FAILURE;

  
  
  
  

  aRetPoint = aPoint + aFrame->GetOffsetTo(*aRetFrame);

  return NS_OK;
}

#ifdef IBMBIDI
void
nsFrameSelection::SetCaretBidiLevel(PRUint8 aLevel)
{
  
  
  bool afterInsert = !!(mCaretBidiLevel & BIDI_LEVEL_UNDEFINED);
  mCaretBidiLevel = aLevel;
  
  nsIBidiKeyboard* bidiKeyboard = nsContentUtils::GetBidiKeyboard();
  if (bidiKeyboard && !afterInsert)
    bidiKeyboard->SetLangFromBidiLevel(aLevel);
  return;
}

PRUint8
nsFrameSelection::GetCaretBidiLevel() const
{
  return mCaretBidiLevel;
}

void
nsFrameSelection::UndefineCaretBidiLevel()
{
  mCaretBidiLevel |= BIDI_LEVEL_UNDEFINED;
}
#endif


#ifdef PRINT_RANGE
void printRange(nsRange *aDomRange)
{
  if (!aDomRange)
  {
    printf("NULL nsIDOMRange\n");
  }
  nsINode* startNode = aDomRange->GetStartParent();
  nsINode* endNode = aDomRange->GetEndParent();
  PRInt32 startOffset = aDomRange->StartOffset();
  PRInt32 endOffset = aDomRange->EndOffset();
  
  printf("range: 0x%lx\t start: 0x%lx %ld, \t end: 0x%lx,%ld\n",
         (unsigned long)aDomRange,
         (unsigned long)startNode, (long)startOffset,
         (unsigned long)endNode, (long)endOffset);
         
}
#endif 

static
nsIAtom *GetTag(nsINode *aNode)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  if (!content) 
  {
    NS_NOTREACHED("bad node passed to GetTag()");
    return nsnull;
  }
  
  return content->Tag();
}


nsINode*
ParentOffset(nsINode *aNode, PRInt32 *aChildOffset)
{
  if (!aNode || !aChildOffset)
    return nsnull;

  nsIContent* parent = aNode->GetParent();
  if (parent)
  {
    *aChildOffset = parent->IndexOf(aNode);

    return parent;
  }

  return nsnull;
}

static nsINode*
GetCellParent(nsINode *aDomNode)
{
    if (!aDomNode)
      return nsnull;
    nsINode* current = aDomNode;
    
    while (current)
    {
      nsIAtom* tag = GetTag(current);
      if (tag == nsGkAtoms::td || tag == nsGkAtoms::th)
        return current;
      current = current->GetParent();
    }
    return nsnull;
}


void
nsFrameSelection::Init(nsIPresShell *aShell, nsIContent *aLimiter)
{
  mShell = aShell;
  mMouseDownState = false;
  mDesiredXSet = false;
  mLimiter = aLimiter;
  mCaretMovementStyle =
    Preferences::GetInt("bidi.edit.caret_movement_style", 2);
}

nsresult
nsFrameSelection::MoveCaret(PRUint32          aKeycode,
                            bool              aContinueSelection,
                            nsSelectionAmount aAmount)
{
  bool visualMovement =
      (aKeycode == nsIDOMKeyEvent::DOM_VK_BACK_SPACE ||
       aKeycode == nsIDOMKeyEvent::DOM_VK_DELETE ||
       aKeycode == nsIDOMKeyEvent::DOM_VK_HOME ||
       aKeycode == nsIDOMKeyEvent::DOM_VK_END) ?
      false : 
      mCaretMovementStyle == 1 ||
        (mCaretMovementStyle == 2 && !aContinueSelection);

  return MoveCaret(aKeycode, aContinueSelection, aAmount, visualMovement);
}

nsresult
nsFrameSelection::MoveCaret(PRUint32          aKeycode,
                            bool              aContinueSelection,
                            nsSelectionAmount aAmount,
                            bool              aVisualMovement)
{
  NS_ENSURE_STATE(mShell);
  
  
  mShell->FlushPendingNotifications(Flush_Layout);

  if (!mShell) {
    return NS_OK;
  }

  nsPresContext *context = mShell->GetPresContext();
  if (!context)
    return NS_ERROR_FAILURE;

  bool isCollapsed;
  nscoord desiredX = 0; 

  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  nsRefPtr<Selection> sel = mDomSelections[index];
  if (!sel)
    return NS_ERROR_NULL_POINTER;

  nsresult result = sel->GetIsCollapsed(&isCollapsed);
  if (NS_FAILED(result))
    return result;
  if (aKeycode == nsIDOMKeyEvent::DOM_VK_UP ||
      aKeycode == nsIDOMKeyEvent::DOM_VK_DOWN)
  {
    result = FetchDesiredX(desiredX);
    if (NS_FAILED(result))
      return result;
    SetDesiredX(desiredX);
  }

  PRInt32 caretStyle =
    Preferences::GetInt("layout.selection.caret_style", 0);
#ifdef XP_MACOSX
  if (caretStyle == 0) {
    caretStyle = 2; 
  }
#endif

  if (!isCollapsed && !aContinueSelection && caretStyle == 2) {
    switch (aKeycode){
      case nsIDOMKeyEvent::DOM_VK_LEFT  :
      case nsIDOMKeyEvent::DOM_VK_UP    :
        {
          const nsRange* anchorFocusRange = sel->GetAnchorFocusRange();
          if (anchorFocusRange) {
            sel->Collapse(anchorFocusRange->GetStartParent(),
                          anchorFocusRange->StartOffset());
          }
          mHint = HINTRIGHT;
          sel->ScrollIntoView(nsISelectionController::SELECTION_FOCUS_REGION);
          return NS_OK;
        }

      case nsIDOMKeyEvent::DOM_VK_RIGHT :
      case nsIDOMKeyEvent::DOM_VK_DOWN  :
        {
          const nsRange* anchorFocusRange = sel->GetAnchorFocusRange();
          if (anchorFocusRange) {
            sel->Collapse(anchorFocusRange->GetEndParent(),
                          anchorFocusRange->EndOffset());
          }
          mHint = HINTLEFT;
          sel->ScrollIntoView(nsISelectionController::SELECTION_FOCUS_REGION);
          return NS_OK;
        }
    }
  }

  nsIFrame *frame;
  PRInt32 offsetused = 0;
  result = sel->GetPrimaryFrameForFocusNode(&frame, &offsetused,
                                            aVisualMovement);

  if (NS_FAILED(result) || !frame)
    return result?result:NS_ERROR_FAILURE;

  
  
  nsPeekOffsetStruct pos(aAmount, eDirPrevious, offsetused, desiredX,
                         true, mLimiter != nsnull, true, aVisualMovement);

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

    if (aAmount >= eSelectCharacter && aAmount <= eSelectWord)
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
    }
    result = TakeFocus(pos.mResultContent, pos.mContentOffset, pos.mContentOffset,
                       tHint, aContinueSelection, false);
  } else if (aKeycode == nsIDOMKeyEvent::DOM_VK_RIGHT && !aContinueSelection) {
    
    
    
    bool isBRFrame = frame->GetType() == nsGkAtoms::brFrame;
    sel->Collapse(sel->GetFocusNode(), sel->GetFocusOffset());
    
    if (!isBRFrame) {
      mHint = HINTLEFT; 
    }
    result = NS_OK;
  }
  if (NS_SUCCEEDED(result))
  {
    result = mDomSelections[index]->
      ScrollIntoView(nsISelectionController::SELECTION_FOCUS_REGION);
  }

  return result;
}






NS_IMETHODIMP
Selection::ToString(nsAString& aReturn)
{
  
  
  
  nsCOMPtr<nsIPresShell> shell =
    mFrameSelection ? mFrameSelection->GetShell() : nsnull;
  if (!shell) {
    aReturn.Truncate();
    return NS_OK;
  }
  shell->FlushPendingNotifications(Flush_Style);

  return ToStringWithFormat("text/plain",
                            nsIDocumentEncoder::SkipInvisibleContent,
                            0, aReturn);
}

NS_IMETHODIMP
Selection::ToStringWithFormat(const char* aFormatType, PRUint32 aFlags,
                              PRInt32 aWrapCol, nsAString& aReturn)
{
  nsresult rv = NS_OK;
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

  return encoder->EncodeToString(aReturn);
}

NS_IMETHODIMP
Selection::SetInterlinePosition(bool aHintRight)
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
Selection::GetInterlinePosition(bool* aHintRight)
{
  if (!mFrameSelection)
    return NS_ERROR_NOT_INITIALIZED; 
  *aHintRight = (mFrameSelection->GetHint() == nsFrameSelection::HINTRIGHT);
  return NS_OK;
}

nsPrevNextBidiLevels
nsFrameSelection::GetPrevNextBidiLevels(nsIContent *aNode,
                                        PRUint32    aContentOffset,
                                        bool        aJumpLines) const
{
  return GetPrevNextBidiLevels(aNode, aContentOffset, mHint, aJumpLines);
}

nsPrevNextBidiLevels
nsFrameSelection::GetPrevNextBidiLevels(nsIContent *aNode,
                                        PRUint32    aContentOffset,
                                        HINT        aHint,
                                        bool        aJumpLines) const
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
  bool jumpedLine;
  nsresult rv = currentFrame->GetFrameFromDirection(direction, false,
                                                    aJumpLines, true,
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
                                    nsIFrame   **aFrameOut) const
{
  NS_ENSURE_STATE(mShell);
  PRUint8 foundLevel = 0;
  nsIFrame *foundFrame = aFrameIn;

  nsCOMPtr<nsIFrameEnumerator> frameTraversal;
  nsresult result;
  nsCOMPtr<nsIFrameTraversal> trav(do_CreateInstance(kFrameTraversalCID,&result));
  if (NS_FAILED(result))
      return result;

  result = trav->NewFrameTraversal(getter_AddRefs(frameTraversal),
                                   mShell->GetPresContext(), aFrameIn,
                                   eLeaf,
                                   false, 
                                   false, 
                                   false     
                                   );
  if (NS_FAILED(result))
    return result;

  do {
    *aFrameOut = foundFrame;
    if (aDirection == eDirNext)
      frameTraversal->Next();
    else 
      frameTraversal->Prev();

    foundFrame = frameTraversal->CurrentItem();
    if (!foundFrame)
      return NS_ERROR_FAILURE;
    foundLevel = NS_GET_EMBEDDING_LEVEL(foundFrame);

  } while (foundLevel > aBidiLevel);

  return NS_OK;
}


nsresult
nsFrameSelection::MaintainSelection(nsSelectionAmount aAmount)
{
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  if (!mDomSelections[index])
    return NS_ERROR_NULL_POINTER;

  mMaintainedAmount = aAmount;

  const nsRange* anchorFocusRange =
    mDomSelections[index]->GetAnchorFocusRange();
  if (anchorFocusRange) {
    mMaintainRange = anchorFocusRange->CloneRange();
    return NS_OK;
  }

  mMaintainRange = nsnull;
  return NS_OK;
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
                                                          aHint, false);

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


bool
nsFrameSelection::AdjustForMaintainedSelection(nsIContent *aContent,
                                               PRInt32     aOffset)
{
  if (!mMaintainRange)
    return false;

  if (!aContent) {
    return false;
  }

  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  if (!mDomSelections[index])
    return false;

  nsINode* rangeStartNode = mMaintainRange->GetStartParent();
  nsINode* rangeEndNode = mMaintainRange->GetEndParent();
  PRInt32 rangeStartOffset = mMaintainRange->StartOffset();
  PRInt32 rangeEndOffset = mMaintainRange->EndOffset();

  PRInt32 relToStart =
    nsContentUtils::ComparePoints(rangeStartNode, rangeStartOffset,
                                  aContent, aOffset);
  PRInt32 relToEnd =
    nsContentUtils::ComparePoints(rangeEndNode, rangeEndOffset,
                                  aContent, aOffset);

  
  
  if ((relToStart < 0 && relToEnd > 0) ||
      (relToStart > 0 &&
       mDomSelections[index]->GetDirection() == eDirNext) ||
      (relToEnd < 0 &&
       mDomSelections[index]->GetDirection() == eDirPrevious)) {
    
    mDomSelections[index]->ReplaceAnchorFocusRange(mMaintainRange);
    if (relToStart < 0 && relToEnd > 0) {
      
      return true;
    }
    
    
    mDomSelections[index]->SetDirection(relToStart > 0 ? eDirPrevious : eDirNext);
  }
  return false;
}


nsresult
nsFrameSelection::HandleClick(nsIContent *aNewFocus,
                              PRUint32    aContentOffset,
                              PRUint32    aContentEndOffset,
                              bool        aContinueSelection, 
                              bool        aMultipleSelection,
                              bool        aHint) 
{
  if (!aNewFocus)
    return NS_ERROR_INVALID_ARG;

  InvalidateDesiredX();

  if (!aContinueSelection) {
    mMaintainRange = nsnull;
    if (!IsValidSelectionPoint(this, aNewFocus)) {
      mAncestorLimiter = nsnull;
    }
  }

  
  if (!mDragSelectingCells)
  {
    BidiLevelFromClick(aNewFocus, aContentOffset);
    PostReason(nsISelectionListener::MOUSEDOWN_REASON + nsISelectionListener::DRAG_REASON);
    if (aContinueSelection &&
        AdjustForMaintainedSelection(aNewFocus, aContentOffset))
      return NS_OK; 

    return TakeFocus(aNewFocus, aContentOffset, aContentEndOffset, HINT(aHint),
                     aContinueSelection, aMultipleSelection);
  }
  
  return NS_OK;
}

void
nsFrameSelection::HandleDrag(nsIFrame *aFrame, nsPoint aPoint)
{
  if (!aFrame || !mShell)
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

  if (newFrame->IsSelected() &&
      AdjustForMaintainedSelection(offsets.content, offsets.offset))
    return;

  
  if (mMaintainRange && 
      mMaintainedAmount != eSelectNoAmount) {    
    
    nsINode* rangenode = mMaintainRange->GetStartParent();
    PRInt32 rangeOffset = mMaintainRange->StartOffset();
    PRInt32 relativePosition =
      nsContentUtils::ComparePoints(rangenode, rangeOffset,
                                    offsets.content, offsets.offset);

    nsDirection direction = relativePosition > 0 ? eDirPrevious : eDirNext;
    nsSelectionAmount amount = mMaintainedAmount;
    if (amount == eSelectBeginLine && direction == eDirNext)
      amount = eSelectEndLine;

    PRInt32 offset;
    nsIFrame* frame = GetFrameForNodeOffset(offsets.content, offsets.offset, HINTRIGHT, &offset);

    if (frame && amount == eSelectWord && direction == eDirPrevious) {
      
      
      nsPeekOffsetStruct charPos(eSelectCharacter, eDirNext, offset, 0,
                                 false, mLimiter != nsnull, false, false);
      if (NS_SUCCEEDED(frame->PeekOffset(&charPos))) {
        frame = charPos.mResultFrame;
        offset = charPos.mContentOffset;
      }
    }

    nsPeekOffsetStruct pos(amount, direction, offset, 0,
                           false, mLimiter != nsnull, false, false);

    if (frame && NS_SUCCEEDED(frame->PeekOffset(&pos)) && pos.mResultContent) {
      offsets.content = pos.mResultContent;
      offsets.offset = pos.mContentOffset;
    }
  }
  
  HandleClick(offsets.content, offsets.offset, offsets.offset,
              true, false, offsets.associateWithNext);
}

nsresult
nsFrameSelection::StartAutoScrollTimer(nsIFrame *aFrame,
                                       nsPoint   aPoint,
                                       PRUint32  aDelay)
{
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  if (!mDomSelections[index])
    return NS_ERROR_NULL_POINTER;

  return mDomSelections[index]->StartAutoScrollTimer(aFrame, aPoint, aDelay);
}

void
nsFrameSelection::StopAutoScrollTimer()
{
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  if (!mDomSelections[index])
    return;

  mDomSelections[index]->StopAutoScrollTimer();
}




nsresult
nsFrameSelection::TakeFocus(nsIContent *aNewFocus,
                            PRUint32    aContentOffset,
                            PRUint32    aContentEndOffset,
                            HINT        aHint,
                            bool        aContinueSelection,
                            bool        aMultipleSelection)
{
  if (!aNewFocus)
    return NS_ERROR_NULL_POINTER;

  NS_ENSURE_STATE(mShell);

  if (!IsValidSelectionPoint(this,aNewFocus))
    return NS_ERROR_FAILURE;

  
  mSelectingTableCellMode = 0;
  mDragSelectingCells = false;
  mStartSelectedCell = nsnull;
  mEndSelectedCell = nsnull;
  mAppendStartSelectedCell = nsnull;
  mHint = aHint;
  
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  if (!mDomSelections[index])
    return NS_ERROR_NULL_POINTER;

  
  if (!aContinueSelection) {
    PRUint32 batching = mBatching;
    bool changes = mChangesDuringBatching;
    mBatching = 1;

    if (aMultipleSelection) {
      
      
      mDomSelections[index]->RemoveCollapsedRanges();

      nsRefPtr<nsRange> newRange = new nsRange();

      newRange->SetStart(aNewFocus, aContentOffset);
      newRange->SetEnd(aNewFocus, aContentOffset);
      mDomSelections[index]->AddRange(newRange);
      mBatching = batching;
      mChangesDuringBatching = changes;
    }
    else
    {
      bool oldDesiredXSet = mDesiredXSet; 
      mDomSelections[index]->Collapse(aNewFocus, aContentOffset);
      mDesiredXSet = oldDesiredXSet; 
      mBatching = batching;
      mChangesDuringBatching = changes;
    }
    if (aContentEndOffset != aContentOffset)
      mDomSelections[index]->Extend(aNewFocus, aContentEndOffset);

    
    
    
    

    NS_ENSURE_STATE(mShell);
    PRInt16 displaySelection = mShell->GetSelectionFlags();

    
    if (displaySelection == nsISelectionDisplay::DISPLAY_ALL)
    {
      mCellParent = GetCellParent(aNewFocus);
#ifdef DEBUG_TABLE_SELECTION
      if (mCellParent)
        printf(" * TakeFocus - Collapsing into new cell\n");
#endif
    }
  }
  else {
    
    if (aContinueSelection && aNewFocus)
    {
      PRInt32 offset;
      nsINode *cellparent = GetCellParent(aNewFocus);
      if (mCellParent && cellparent && cellparent != mCellParent) 
      {
#ifdef DEBUG_TABLE_SELECTION
printf(" * TakeFocus - moving into new cell\n");
#endif
        nsMouseEvent event(false, 0, nsnull, nsMouseEvent::eReal);

        
        nsINode* parent = ParentOffset(mCellParent, &offset);
        if (parent)
          HandleTableSelection(parent, offset,
                               nsISelectionPrivate::TABLESELECTION_CELL, &event);

        
        parent = ParentOffset(cellparent, &offset);

        
        
        event.modifiers &= ~widget::MODIFIER_SHIFT; 
        if (parent)
        {
          mCellParent = cellparent;
          
          HandleTableSelection(parent, offset,
                               nsISelectionPrivate::TABLESELECTION_CELL, &event);
        }
      }
      else
      {
        
        
        if (mDomSelections[index]->GetDirection() == eDirNext && aContentEndOffset > aContentOffset) 
        {
          mDomSelections[index]->Extend(aNewFocus, aContentEndOffset);
        }
        else
          mDomSelections[index]->Extend(aNewFocus, aContentOffset);
      }
    }
  }

  
  if (GetBatching())
    return NS_OK;
  return NotifySelectionListeners(nsISelectionController::SELECTION_NORMAL);
}


SelectionDetails*
nsFrameSelection::LookUpSelection(nsIContent *aContent,
                                  PRInt32 aContentOffset,
                                  PRInt32 aContentLength,
                                  bool aSlowCheck) const
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
nsFrameSelection::SetMouseDownState(bool aState)
{
  if (mMouseDownState == aState)
    return;

  mMouseDownState = aState;
    
  if (!mMouseDownState)
  {
    mDragSelectingCells = false;
    PostReason(nsISelectionListener::MOUSEUP_REASON);
    NotifySelectionListeners(nsISelectionController::SELECTION_NORMAL); 
  }
}

Selection*
nsFrameSelection::GetSelection(SelectionType aType) const
{
  PRInt8 index = GetIndexFromSelectionType(aType);
  if (index < 0)
    return nsnull;

  return mDomSelections[index];
}

nsresult
nsFrameSelection::ScrollSelectionIntoView(SelectionType   aType,
                                          SelectionRegion aRegion,
                                          PRInt16         aFlags) const
{
  PRInt8 index = GetIndexFromSelectionType(aType);
  if (index < 0)
    return NS_ERROR_INVALID_ARG;

  if (!mDomSelections[index])
    return NS_ERROR_NULL_POINTER;

  nsIPresShell::ScrollAxis verticalScroll = nsIPresShell::ScrollAxis();
  PRInt32 flags = Selection::SCROLL_DO_FLUSH;
  if (aFlags & nsISelectionController::SCROLL_SYNCHRONOUS) {
    flags |= Selection::SCROLL_SYNCHRONOUS;
  } else if (aFlags & nsISelectionController::SCROLL_FIRST_ANCESTOR_ONLY) {
    flags |= Selection::SCROLL_FIRST_ANCESTOR_ONLY;
  }
  if (aFlags & nsISelectionController::SCROLL_CENTER_VERTICALLY) {
    verticalScroll = nsIPresShell::ScrollAxis(
      nsIPresShell::SCROLL_CENTER, nsIPresShell::SCROLL_IF_NOT_FULLY_VISIBLE);
  }

  
  
  return mDomSelections[index]->ScrollIntoView(aRegion,
                                               verticalScroll,
                                               nsIPresShell::ScrollAxis(),
                                               flags);
}

nsresult
nsFrameSelection::RepaintSelection(SelectionType aType) const
{
  PRInt8 index = GetIndexFromSelectionType(aType);
  if (index < 0)
    return NS_ERROR_INVALID_ARG;
  if (!mDomSelections[index])
    return NS_ERROR_NULL_POINTER;
  NS_ENSURE_STATE(mShell);
  return mDomSelections[index]->Repaint(mShell->GetPresContext());
}
 
nsIFrame*
nsFrameSelection::GetFrameForNodeOffset(nsIContent *aNode,
                                        PRInt32     aOffset,
                                        HINT        aHint,
                                        PRInt32    *aReturnOffset) const
{
  if (!aNode || !aReturnOffset || !mShell)
    return nsnull;

  if (aOffset < 0)
    return nsnull;

  *aReturnOffset = aOffset;

  nsCOMPtr<nsIContent> theNode = aNode;

  if (aNode->IsElement())
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
    
    

    
    

    if (theNode->IsElement())
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
        if (theNode->GetPrimaryFrame())
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
        else
        {
          
          
          
          theNode = aNode;
        }
      }
    }
  }
  
  nsIFrame* returnFrame = theNode->GetPrimaryFrame();
  if (!returnFrame)
    return nsnull;

  
  returnFrame->GetChildFrameContainingOffset(*aReturnOffset, aHint == HINTRIGHT,
                                             &aOffset, &returnFrame);
  return returnFrame;
}

void
nsFrameSelection::CommonPageMove(bool aForward,
                                 bool aExtend,
                                 nsIScrollableFrame* aScrollableFrame)
{
  
  

  

  nsIFrame* scrolledFrame = aScrollableFrame->GetScrolledFrame();
  if (!scrolledFrame)
    return;

  
  
  nsISelection* domSel = GetSelection(nsISelectionController::SELECTION_NORMAL);
  if (!domSel) 
    return;

  nsRefPtr<nsCaret> caret = mShell->GetCaret();

  nsRect caretPos;
  nsIFrame* caretFrame = caret->GetGeometry(domSel, &caretPos);
  if (!caretFrame) 
    return;
  
  
  nsSize scrollDelta = aScrollableFrame->GetPageScrollAmount();

  if (aForward)
    caretPos.y += scrollDelta.height;
  else
    caretPos.y -= scrollDelta.height;

  caretPos += caretFrame->GetOffsetTo(scrolledFrame);
    
  
  nsPoint desiredPoint;
  desiredPoint.x = caretPos.x;
  desiredPoint.y = caretPos.y + caretPos.height/2;
  nsIFrame::ContentOffsets offsets =
      scrolledFrame->GetContentOffsetsFromPoint(desiredPoint);

  if (!offsets.content)
    return;

  
  aScrollableFrame->ScrollBy(nsIntPoint(0, aForward ? 1 : -1),
                             nsIScrollableFrame::PAGES,
                             nsIScrollableFrame::SMOOTH);

  
  HandleClick(offsets.content, offsets.offset,
              offsets.offset, aExtend, false, true);
}

nsresult
nsFrameSelection::CharacterMove(bool aForward, bool aExtend)
{
  if (aForward)
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_RIGHT, aExtend, eSelectCluster);
  else
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_LEFT, aExtend, eSelectCluster);
}

nsresult
nsFrameSelection::CharacterExtendForDelete()
{
  return MoveCaret(nsIDOMKeyEvent::DOM_VK_DELETE, true, eSelectCluster);
}

nsresult
nsFrameSelection::CharacterExtendForBackspace()
{
  return MoveCaret(nsIDOMKeyEvent::DOM_VK_BACK_SPACE, true, eSelectCharacter);
}

nsresult
nsFrameSelection::WordMove(bool aForward, bool aExtend)
{
  if (aForward)
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_RIGHT,aExtend,eSelectWord);
  else
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_LEFT,aExtend,eSelectWord);
}

nsresult
nsFrameSelection::WordExtendForDelete(bool aForward)
{
  if (aForward)
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_DELETE, true, eSelectWord);
  else
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_BACK_SPACE, true, eSelectWord);
}

nsresult
nsFrameSelection::LineMove(bool aForward, bool aExtend)
{
  if (aForward)
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_DOWN,aExtend,eSelectLine);
  else
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_UP,aExtend,eSelectLine);
}

nsresult
nsFrameSelection::IntraLineMove(bool aForward, bool aExtend)
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
  else if (mAncestorLimiter) {
    rootContent = mAncestorLimiter;
  }
  else
  {
    NS_ENSURE_STATE(mShell);
    nsIDocument *doc = mShell->GetDocument();
    if (!doc)
      return NS_ERROR_FAILURE;
    rootContent = doc->GetRootElement();
    if (!rootContent)
      return NS_ERROR_FAILURE;
  }
  PRInt32 numChildren = rootContent->GetChildCount();
  PostReason(nsISelectionListener::NO_REASON);
  return TakeFocus(rootContent, 0, numChildren, HINTLEFT, false, false);
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
    mChangesDuringBatching = false;
    NotifySelectionListeners(nsISelectionController::SELECTION_NORMAL);
  }
}


nsresult
nsFrameSelection::NotifySelectionListeners(SelectionType aType)
{
  PRInt8 index = GetIndexFromSelectionType(aType);
  if (index >=0 && mDomSelections[index])
  {
    return mDomSelections[index]->NotifySelectionListeners();
  }
  return NS_ERROR_FAILURE;
}



static bool IsCell(nsIContent *aContent)
{
  return ((aContent->Tag() == nsGkAtoms::td ||
           aContent->Tag() == nsGkAtoms::th) &&
          aContent->IsHTML());
}

nsITableCellLayout* 
nsFrameSelection::GetCellLayout(nsIContent *aCellContent) const
{
  NS_ENSURE_TRUE(mShell, nsnull);
  nsITableCellLayout *cellLayoutObject =
    do_QueryFrame(aCellContent->GetPrimaryFrame());
  return cellLayoutObject;
}

nsITableLayout* 
nsFrameSelection::GetTableLayout(nsIContent *aTableContent) const
{
  NS_ENSURE_TRUE(mShell, nsnull);
  nsITableLayout *tableLayoutObject =
    do_QueryFrame(aTableContent->GetPrimaryFrame());
  return tableLayoutObject;
}

nsresult
nsFrameSelection::ClearNormalSelection()
{
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  if (!mDomSelections[index])
    return NS_ERROR_NULL_POINTER;

  return mDomSelections[index]->RemoveAllRanges();
}

static nsIContent*
GetFirstSelectedContent(nsRange* aRange)
{
  if (!aRange) {
    return nsnull;
  }

  NS_PRECONDITION(aRange->GetStartParent(), "Must have start parent!");
  NS_PRECONDITION(aRange->GetStartParent()->IsElement(),
                  "Unexpected parent");

  return aRange->GetStartParent()->GetChildAt(aRange->StartOffset());
}



nsresult
nsFrameSelection::HandleTableSelection(nsINode *aParentContent,
                                       PRInt32 aContentOffset,
                                       PRInt32 aTarget,
                                       nsMouseEvent *aMouseEvent)
{
  NS_ENSURE_TRUE(aParentContent, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aMouseEvent, NS_ERROR_NULL_POINTER);

  if (mMouseDownState && mDragSelectingCells && (aTarget & nsISelectionPrivate::TABLESELECTION_TABLE))
  {
    
    
      return NS_OK;
  }

  nsresult result = NS_OK;

  nsIContent *childContent = aParentContent->GetChildAt(aContentOffset);

  
  
  
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  if (!mDomSelections[index])
    return NS_ERROR_NULL_POINTER;

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
        
        
        
        
        if (mStartSelectedCell && aMouseEvent->IsShift())
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
        bool isSelected = false;

        
        nsIContent* previousCellNode =
          GetFirstSelectedContent(GetFirstCellRange());
        if (previousCellNode)
        {
          

          
          nsIFrame  *cellFrame = childContent->GetPrimaryFrame();
          if (!cellFrame) return NS_ERROR_NULL_POINTER;
          isSelected = cellFrame->IsSelected();
        }
        else
        {
          
          mDomSelections[index]->RemoveAllRanges();
        }
        mDragSelectingCells = true;    
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
          
          
          if (previousCellNode &&
              !IsInSameTable(previousCellNode, childContent))
          {
            mDomSelections[index]->RemoveAllRanges();
            
            mSelectingTableCellMode = aTarget;
          }

          return SelectCellElement(childContent);
        }

        return NS_OK;
      }
      else if (aTarget == nsISelectionPrivate::TABLESELECTION_TABLE)
      {
        
        
        
        mDragSelectingCells = false;
        mStartSelectedCell = nsnull;
        mEndSelectedCell = nsnull;

        
        mDomSelections[index]->RemoveAllRanges();
        return CreateAndAddRange(aParentContent, aContentOffset);
      }
      else if (aTarget == nsISelectionPrivate::TABLESELECTION_ROW || aTarget == nsISelectionPrivate::TABLESELECTION_COLUMN)
      {
#ifdef DEBUG_TABLE_SELECTION
printf("aTarget == %d\n", aTarget);
#endif

        
        
        
        mDragSelectingCells = true;
      
        
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

      if (rangeCount > 0 && aMouseEvent->IsShift() && 
          mAppendStartSelectedCell && mAppendStartSelectedCell != childContent)
      {
        
        mDragSelectingCells = false;
        return SelectBlockOfCells(mAppendStartSelectedCell, childContent);
      }

      if (mDragSelectingCells)
        mAppendStartSelectedCell = mStartSelectedCell;
        
      mDragSelectingCells = false;
      mStartSelectedCell = nsnull;
      mEndSelectedCell = nsnull;

      
      
      bool doMouseUpAction = false;
#ifdef XP_MACOSX
      doMouseUpAction = aMouseEvent->IsMeta();
#else
      doMouseUpAction = aMouseEvent->IsControl();
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
        
        
        
        nsINode* previousCellParent = nsnull;
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Unselecting mUnselectCellOnMouseUp; rangeCount=%d\n", rangeCount);
#endif
        for( PRInt32 i = 0; i < rangeCount; i++)
        {
          
          
          nsRefPtr<nsRange> range = mDomSelections[index]->GetRangeAt(i);
          if (!range) return NS_ERROR_NULL_POINTER;

          nsINode* parent = range->GetStartParent();
          if (!parent) return NS_ERROR_NULL_POINTER;

          PRInt32 offset = range->StartOffset();
          
          nsIContent* child = parent->GetChildAt(offset);
          if (child && IsCell(child))
            previousCellParent = parent;

          
          if (!previousCellParent) break;
        
          if (previousCellParent == aParentContent && offset == aContentOffset)
          {
            
            if (rangeCount == 1)
            {
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Unselecting single selected cell\n");
#endif
              
              
              mStartSelectedCell = nsnull;
              mEndSelectedCell = nsnull;
              mAppendStartSelectedCell = nsnull;
              
              
              
              return mDomSelections[index]->Collapse(childContent, 0);
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

  
  nsIContent* table = IsInSameTable(aStartCell, aEndCell);
  if (!table) {
    return NS_OK;
  }

  
  PRInt32 startRowIndex, startColIndex, endRowIndex, endColIndex;
  result = GetCellIndexes(aStartCell, startRowIndex, startColIndex);
  if(NS_FAILED(result)) return result;
  result = GetCellIndexes(aEndCell, endRowIndex, endColIndex);
  if(NS_FAILED(result)) return result;

  if (mDragSelectingCells)
  {
    
    UnselectCells(table, startRowIndex, startColIndex, endRowIndex, endColIndex,
                  true);
  }

  
  
  return AddCellsToSelection(table, startRowIndex, startColIndex,
                             endRowIndex, endColIndex);
}

nsresult
nsFrameSelection::UnselectCells(nsIContent *aTableContent,
                                PRInt32 aStartRowIndex,
                                PRInt32 aStartColumnIndex,
                                PRInt32 aEndRowIndex,
                                PRInt32 aEndColumnIndex,
                                bool aRemoveOutsideOfCellRange)
{
  PRInt8 index =
    GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  if (!mDomSelections[index])
    return NS_ERROR_NULL_POINTER;

  nsITableLayout *tableLayout = GetTableLayout(aTableContent);
  if (!tableLayout)
    return NS_ERROR_FAILURE;

  PRInt32 minRowIndex = NS_MIN(aStartRowIndex, aEndRowIndex);
  PRInt32 maxRowIndex = NS_MAX(aStartRowIndex, aEndRowIndex);
  PRInt32 minColIndex = NS_MIN(aStartColumnIndex, aEndColumnIndex);
  PRInt32 maxColIndex = NS_MAX(aStartColumnIndex, aEndColumnIndex);

  
  nsRefPtr<nsRange> range = GetFirstCellRange();
  nsIContent* cellNode = GetFirstSelectedContent(range);
  NS_PRECONDITION(!range || cellNode, "Must have cellNode if had a range");

  PRInt32 curRowIndex, curColIndex;
  while (cellNode)
  {
    nsresult result = GetCellIndexes(cellNode, curRowIndex, curColIndex);
    if (NS_FAILED(result))
      return result;

#ifdef DEBUG_TABLE_SELECTION
    if (!range)
      printf("RemoveCellsToSelection -- range is null\n");
#endif

    if (range) {
      if (aRemoveOutsideOfCellRange) {
        if (curRowIndex < minRowIndex || curRowIndex > maxRowIndex || 
            curColIndex < minColIndex || curColIndex > maxColIndex) {

          mDomSelections[index]->RemoveRange(range);
          
          mSelectedCellIndex--;
        }

      } else {
        
        
        nsCOMPtr<nsIDOMElement> cellElement;
        PRInt32 origRowIndex, origColIndex, rowSpan, colSpan,
          actualRowSpan, actualColSpan;
        bool isSelected;

        result = tableLayout->GetCellDataAt(curRowIndex, curColIndex,
                                            *getter_AddRefs(cellElement),
                                            origRowIndex, origColIndex,
                                            rowSpan, colSpan, 
                                            actualRowSpan, actualColSpan,
                                            isSelected);
        if (NS_FAILED(result))
          return result;

        if (origRowIndex <= maxRowIndex &&
            origRowIndex + actualRowSpan - 1 >= minRowIndex &&
            origColIndex <= maxColIndex &&
            origColIndex + actualColSpan - 1 >= minColIndex) {

          mDomSelections[index]->RemoveRange(range);
          
          mSelectedCellIndex--;
        }
      }
    }

    range = GetNextCellRange();
    cellNode = GetFirstSelectedContent(range);
    NS_PRECONDITION(!range || cellNode, "Must have cellNode if had a range");
  }

  return NS_OK;
}

nsresult
nsFrameSelection::AddCellsToSelection(nsIContent *aTableContent,
                                      PRInt32 aStartRowIndex,
                                      PRInt32 aStartColumnIndex,
                                      PRInt32 aEndRowIndex,
                                      PRInt32 aEndColumnIndex)
{
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  if (!mDomSelections[index])
    return NS_ERROR_NULL_POINTER;

  
  
  nsITableLayout *tableLayoutObject = GetTableLayout(aTableContent);
  if (!tableLayoutObject) 
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMElement> cellElement;
  PRInt32 rowSpan, colSpan, actualRowSpan, actualColSpan,
    curRowIndex, curColIndex;
  bool isSelected;
  nsresult result = NS_OK;

  PRInt32 row = aStartRowIndex;
  while(true)
  {
    PRInt32 col = aStartColumnIndex;
    while(true)
    {
      result = tableLayoutObject->GetCellDataAt(row, col, *getter_AddRefs(cellElement),
                                                curRowIndex, curColIndex, rowSpan, colSpan, 
                                                actualRowSpan, actualColSpan, isSelected);
      if (NS_FAILED(result)) return result;

      NS_ASSERTION(actualColSpan, "!actualColSpan is 0!");

      
      if (!isSelected && cellElement && row == curRowIndex && col == curColIndex)
      {
        nsCOMPtr<nsIContent> cellContent = do_QueryInterface(cellElement);
        result = SelectCellElement(cellContent);
        if (NS_FAILED(result)) return result;
      }
      
      if (col == aEndColumnIndex) break;

      if (aStartColumnIndex < aEndColumnIndex)
        col ++;
      else
        col--;
    };
    if (row == aEndRowIndex) break;

    if (aStartRowIndex < aEndRowIndex)
      row++;
    else
      row--;
  };
  return result;
}

nsresult
nsFrameSelection::RemoveCellsFromSelection(nsIContent *aTable,
                                           PRInt32 aStartRowIndex,
                                           PRInt32 aStartColumnIndex,
                                           PRInt32 aEndRowIndex,
                                           PRInt32 aEndColumnIndex)
{
  return UnselectCells(aTable, aStartRowIndex, aStartColumnIndex,
                       aEndRowIndex, aEndColumnIndex, false);
}

nsresult
nsFrameSelection::RestrictCellsToSelection(nsIContent *aTable,
                                           PRInt32 aStartRowIndex,
                                           PRInt32 aStartColumnIndex,
                                           PRInt32 aEndRowIndex,
                                           PRInt32 aEndColumnIndex)
{
  return UnselectCells(aTable, aStartRowIndex, aStartColumnIndex,
                       aEndRowIndex, aEndColumnIndex, true);
}

nsresult
nsFrameSelection::SelectRowOrColumn(nsIContent *aCellContent, PRUint32 aTarget)
{
  if (!aCellContent) return NS_ERROR_NULL_POINTER;

  nsIContent* table = GetParentTable(aCellContent);
  if (!table) return NS_ERROR_NULL_POINTER;

  
  
  
  nsITableLayout *tableLayout = GetTableLayout(table);
  if (!tableLayout) return NS_ERROR_FAILURE;
  nsITableCellLayout *cellLayout = GetCellLayout(aCellContent);
  if (!cellLayout) return NS_ERROR_FAILURE;

  
  PRInt32 rowIndex, colIndex, curRowIndex, curColIndex;
  nsresult result = cellLayout->GetCellIndexes(rowIndex, colIndex);
  if (NS_FAILED(result)) return result;

  
  
  if (aTarget == nsISelectionPrivate::TABLESELECTION_ROW)
    colIndex = 0;
  if (aTarget == nsISelectionPrivate::TABLESELECTION_COLUMN)
    rowIndex = 0;

  nsCOMPtr<nsIDOMElement> cellElement;
  nsCOMPtr<nsIContent> firstCell;
  nsCOMPtr<nsIDOMElement> lastCell;
  PRInt32 rowSpan, colSpan, actualRowSpan, actualColSpan;
  bool isSelected;

  do {
    
    result = tableLayout->GetCellDataAt(rowIndex, colIndex, *getter_AddRefs(cellElement),
                                        curRowIndex, curColIndex, rowSpan, colSpan, 
                                        actualRowSpan, actualColSpan, isSelected);
    if (NS_FAILED(result)) return result;
    if (cellElement)
    {
      NS_ASSERTION(actualRowSpan > 0 && actualColSpan> 0, "SelectRowOrColumn: Bad rowspan or colspan\n");
      if (!firstCell)
        firstCell = do_QueryInterface(cellElement);

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
      mStartSelectedCell = firstCell;
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

nsIContent*
nsFrameSelection::GetFirstCellNodeInRange(nsRange *aRange) const
{
  if (!aRange) return nsnull;

  nsINode* startParent = aRange->GetStartParent();
  if (!startParent)
    return nsnull;

  PRInt32 offset = aRange->StartOffset();

  nsIContent* childContent = startParent->GetChildAt(offset);
  if (!childContent)
    return nsnull;
  
  if (!IsCell(childContent))
    return nsnull;

  return childContent;
}

nsRange*
nsFrameSelection::GetFirstCellRange()
{
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  if (!mDomSelections[index])
    return nsnull;

  nsRange* firstRange = mDomSelections[index]->GetRangeAt(0);
  if (!GetFirstCellNodeInRange(firstRange)) {
    return nsnull;
  }

  
  mSelectedCellIndex = 1;

  return firstRange;
}

nsRange*
nsFrameSelection::GetNextCellRange()
{
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  if (!mDomSelections[index])
    return nsnull;

  nsRange* range = mDomSelections[index]->GetRangeAt(mSelectedCellIndex);

  
  if (!GetFirstCellNodeInRange(range)) {
    return nsnull;
  }

  
  mSelectedCellIndex++;

  return range;
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

nsIContent*
nsFrameSelection::IsInSameTable(nsIContent  *aContent1,
                                nsIContent  *aContent2) const
{
  if (!aContent1 || !aContent2) return nsnull;
  
  nsIContent* tableNode1 = GetParentTable(aContent1);
  nsIContent* tableNode2 = GetParentTable(aContent2);

  
  
  return (tableNode1 == tableNode2) ? tableNode1 : nsnull;
}

nsIContent*
nsFrameSelection::GetParentTable(nsIContent *aCell) const
{
  if (!aCell) {
    return nsnull;
  }

  for (nsIContent* parent = aCell->GetParent(); parent;
       parent = parent->GetParent()) {
    if (parent->Tag() == nsGkAtoms::table &&
        parent->IsHTML()) {
      return parent;
    }
  }

  return nsnull;
}

nsresult
nsFrameSelection::SelectCellElement(nsIContent *aCellElement)
{
  nsIContent *parent = aCellElement->GetParent();

  
  PRInt32 offset = parent->IndexOf(aCellElement);

  return CreateAndAddRange(parent, offset);
}

nsresult
Selection::getTableCellLocationFromRange(nsRange* aRange,
                                         PRInt32* aSelectionType,
                                         PRInt32* aRow, PRInt32* aCol)
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

  
  
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(aRange->GetStartParent());
  if (!content)
    return NS_ERROR_FAILURE;

  nsIContent *child = content->GetChildAt(aRange->StartOffset());
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
Selection::addTableCellRange(nsRange* aRange, bool* aDidAddRange,
                             PRInt32* aOutIndex)
{  
  if (!aDidAddRange || !aOutIndex)
    return NS_ERROR_NULL_POINTER;

  *aDidAddRange = false;
  *aOutIndex = -1;

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

  *aDidAddRange = true;
  return AddItem(aRange, aOutIndex);
}


nsresult
Selection::GetTableSelectionType(nsIDOMRange* aDOMRange,
                                 PRInt32* aTableSelectionType)
{
  if (!aDOMRange || !aTableSelectionType)
    return NS_ERROR_NULL_POINTER;
  nsRange* range = static_cast<nsRange*>(aDOMRange);
  
  *aTableSelectionType = nsISelectionPrivate::TABLESELECTION_NONE;
 
  
  if(!mFrameSelection) return NS_OK;

  nsINode* startNode = range->GetStartParent();
  if (!startNode) return NS_ERROR_FAILURE;
  
  nsINode* endNode = range->GetEndParent();
  if (!endNode) return NS_ERROR_FAILURE;

  
  if (startNode != endNode) return NS_OK;

  PRInt32 startOffset = range->StartOffset();
  PRInt32 endOffset = range->EndOffset();

  
  if ((endOffset - startOffset) != 1)
    return NS_OK;

  nsIContent* startContent = static_cast<nsIContent*>(startNode);
  if (!(startNode->IsElement() && startContent->IsHTML())) {
    
    
    return NS_OK;
  }

  nsIAtom *tag = startContent->Tag();

  if (tag == nsGkAtoms::tr)
  {
    *aTableSelectionType = nsISelectionPrivate::TABLESELECTION_CELL;
  }
  else 
  {
    nsIContent *child = startNode->GetChildAt(startOffset);
    if (!child)
      return NS_ERROR_FAILURE;

    tag = child->Tag();

    if (tag == nsGkAtoms::table)
      *aTableSelectionType = nsISelectionPrivate::TABLESELECTION_TABLE;
    else if (tag == nsGkAtoms::tr)
      *aTableSelectionType = nsISelectionPrivate::TABLESELECTION_ROW;
  }

  return NS_OK;
}

nsresult
nsFrameSelection::CreateAndAddRange(nsINode *aParentNode, PRInt32 aOffset)
{
  if (!aParentNode) return NS_ERROR_NULL_POINTER;

  nsRefPtr<nsRange> range = new nsRange();

  
  nsresult result = range->SetStart(aParentNode, aOffset);
  if (NS_FAILED(result)) return result;
  result = range->SetEnd(aParentNode, aOffset+1);
  if (NS_FAILED(result)) return result;
  
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  if (!mDomSelections[index])
    return NS_ERROR_NULL_POINTER;

  return mDomSelections[index]->AddRange(range);
}



void
nsFrameSelection::SetAncestorLimiter(nsIContent *aLimiter)
{
  if (mAncestorLimiter != aLimiter) {
    mAncestorLimiter = aLimiter;
    PRInt8 index =
      GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
    if (!mDomSelections[index])
      return;

    if (!IsValidSelectionPoint(this, mDomSelections[index]->GetFocusNode())) {
      ClearNormalSelection();
      if (mAncestorLimiter) {
        PostReason(nsISelectionListener::NO_REASON);
        TakeFocus(mAncestorLimiter, 0, 0, HINTLEFT, false, false);
      }
    }
  }
}








nsresult
nsFrameSelection::DeleteFromDocument()
{
  nsresult res;

  
  bool isCollapsed;
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  if (!mDomSelections[index])
    return NS_ERROR_NULL_POINTER;

  mDomSelections[index]->GetIsCollapsed( &isCollapsed);
  if (isCollapsed)
  {
    return NS_OK;
  }

  
  nsSelectionIterator iter(mDomSelections[index]);
  res = iter.First();
  if (NS_FAILED(res))
    return res;

  while (iter.IsDone())
  {
    nsRefPtr<nsRange> range = iter.CurrentItem();
    res = range->DeleteContents();
    if (NS_FAILED(res))
      return res;
    iter.Next();
  }

  
  
  
  if (isCollapsed)
    mDomSelections[index]->Collapse(mDomSelections[index]->GetAnchorNode(), mDomSelections[index]->GetAnchorOffset()-1);
  else if (mDomSelections[index]->GetAnchorOffset() > 0)
    mDomSelections[index]->Collapse(mDomSelections[index]->GetAnchorNode(), mDomSelections[index]->GetAnchorOffset());
#ifdef DEBUG
  else
    printf("Don't know how to set selection back past frame boundary\n");
#endif

  return NS_OK;
}

void
nsFrameSelection::SetDelayedCaretData(nsMouseEvent *aMouseEvent)
{
  if (aMouseEvent) {
    mDelayedMouseEventValid = true;
    mDelayedMouseEventIsShift = aMouseEvent->IsShift();
    mDelayedMouseEventClickCount = aMouseEvent->clickCount;
  } else {
    mDelayedMouseEventValid = false;
  }
}

void
nsFrameSelection::DisconnectFromPresShell()
{
  StopAutoScrollTimer();
  for (PRInt32 i = 0; i < nsISelectionController::NUM_SELECTIONTYPES; i++) {
    mDomSelections[i]->Clear(nsnull);
  }
  mShell = nsnull;
}



#if 0
#pragma mark -
#endif





Selection::Selection()
  : mCachedOffsetForFrame(nsnull)
  , mDirection(eDirNext)
  , mType(nsISelectionController::SELECTION_NORMAL)
{
}

Selection::Selection(nsFrameSelection* aList)
  : mFrameSelection(aList)
  , mCachedOffsetForFrame(nsnull)
  , mDirection(eDirNext)
  , mType(nsISelectionController::SELECTION_NORMAL)
{
}

Selection::~Selection()
{
  setAnchorFocusRange(-1);

  PRUint32 count = mRanges.Length();
  for (PRUint32 i = 0; i < count; ++i) {
    mRanges[i].mRange->SetInSelection(false);
  }

  if (mAutoScrollTimer) {
    mAutoScrollTimer->Stop();
    mAutoScrollTimer = nsnull;
  }

  mScrollEvent.Revoke();

  if (mCachedOffsetForFrame) {
    delete mCachedOffsetForFrame;
    mCachedOffsetForFrame = nsnull;
  }
}


NS_IMPL_CYCLE_COLLECTION_CLASS(Selection)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(Selection)
  
  
  
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mSelectionListeners)
  tmp->RemoveAllRanges();
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFrameSelection)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(Selection)
  {
    PRUint32 i, count = tmp->mRanges.Length();
    for (i = 0; i < count; ++i) {
      NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mRanges[i].mRange, nsIDOMRange)
    }
  }
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mAnchorFocusRange, nsIDOMRange)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFrameSelection)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mSelectionListeners)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

DOMCI_DATA(Selection, Selection)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Selection)
  NS_INTERFACE_MAP_ENTRY(nsISelection)
  NS_INTERFACE_MAP_ENTRY(nsISelectionPrivate)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISelection)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Selection)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(Selection)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Selection)

NS_IMETHODIMP
Selection::SetPresShell(nsIPresShell* aPresShell)
{
  mPresShellWeak = do_GetWeakReference(aPresShell);
  return NS_OK;
}



NS_IMETHODIMP
Selection::GetAnchorNode(nsIDOMNode** aAnchorNode)
{
  nsINode* anchorNode = GetAnchorNode();
  if (anchorNode) {
    return CallQueryInterface(anchorNode, aAnchorNode);
  }

  *aAnchorNode = nsnull;
  return NS_OK;
}

nsINode*
Selection::GetAnchorNode()
{
  if (!mAnchorFocusRange)
    return nsnull;
   
  if (GetDirection() == eDirNext) {
    return mAnchorFocusRange->GetStartParent();
  }

  return mAnchorFocusRange->GetEndParent();
}

NS_IMETHODIMP
Selection::GetAnchorOffset(PRInt32* aAnchorOffset)
{
  *aAnchorOffset = GetAnchorOffset();
  return NS_OK;
}


NS_IMETHODIMP
Selection::GetFocusNode(nsIDOMNode** aFocusNode)
{
  nsINode* focusNode = GetFocusNode();
  if (focusNode) {
    return CallQueryInterface(focusNode, aFocusNode);
  }

  *aFocusNode = nsnull;
  return NS_OK;
}

nsINode*
Selection::GetFocusNode()
{
  if (!mAnchorFocusRange)
    return nsnull;

  if (GetDirection() == eDirNext){
    return mAnchorFocusRange->GetEndParent();
  }

  return mAnchorFocusRange->GetStartParent();
}

NS_IMETHODIMP
Selection::GetFocusOffset(PRInt32* aFocusOffset)
{
  *aFocusOffset = GetFocusOffset();
  return NS_OK;
}

void
Selection::setAnchorFocusRange(PRInt32 indx)
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

PRInt32
Selection::GetAnchorOffset()
{
  if (!mAnchorFocusRange)
    return 0;

  if (GetDirection() == eDirNext){
    return mAnchorFocusRange->StartOffset();
  }

  return mAnchorFocusRange->EndOffset();
}

PRInt32
Selection::GetFocusOffset()
{
  if (!mAnchorFocusRange)
    return 0;

  if (GetDirection() == eDirNext){
    return mAnchorFocusRange->EndOffset();
  }

  return mAnchorFocusRange->StartOffset();
}

static nsresult
CompareToRangeStart(nsINode* aCompareNode, PRInt32 aCompareOffset,
                    nsRange* aRange, PRInt32* aCmp)
{
  nsINode* start = aRange->GetStartParent();
  NS_ENSURE_STATE(aCompareNode && start);
  
  
  if (aCompareNode->GetCurrentDoc() != start->GetCurrentDoc() ||
      !start->GetCurrentDoc()) {
    *aCmp = 1;
  } else {
    *aCmp = nsContentUtils::ComparePoints(aCompareNode, aCompareOffset,
                                          start, aRange->StartOffset());
  }
  return NS_OK;
}

static nsresult
CompareToRangeEnd(nsINode* aCompareNode, PRInt32 aCompareOffset,
                  nsRange* aRange, PRInt32* aCmp)
{
  nsINode* end = aRange->GetEndParent();
  NS_ENSURE_STATE(aCompareNode && end);
  
  
  if (aCompareNode->GetCurrentDoc() != end->GetCurrentDoc() ||
      !end->GetCurrentDoc()) {
    *aCmp = 1;
  } else {
    *aCmp = nsContentUtils::ComparePoints(aCompareNode, aCompareOffset,
                                          end, aRange->EndOffset());
  }
  return NS_OK;
}










nsresult
Selection::FindInsertionPoint(
    nsTArray<RangeData>* aElementArray,
    nsINode* aPointNode, PRInt32 aPointOffset,
    nsresult (*aComparator)(nsINode*,PRInt32,nsRange*,PRInt32*),
    PRInt32* aPoint)
{
  *aPoint = 0;
  PRInt32 beginSearch = 0;
  PRInt32 endSearch = aElementArray->Length(); 
  while (endSearch - beginSearch > 0) {
    PRInt32 center = (endSearch - beginSearch) / 2 + beginSearch;

    nsRange* range = (*aElementArray)[center].mRange;

    PRInt32 cmp;
    nsresult rv = aComparator(aPointNode, aPointOffset, range, &cmp);
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
  *aPoint = beginSearch;
  return NS_OK;
}








nsresult
Selection::SubtractRange(RangeData* aRange, nsRange* aSubtract,
                         nsTArray<RangeData>* aOutput)
{
  nsRange* range = aRange->mRange;

  
  PRInt32 cmp;
  nsresult rv = CompareToRangeStart(range->GetStartParent(),
                                    range->StartOffset(),
                                    aSubtract, &cmp);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt32 cmp2;
  rv = CompareToRangeEnd(range->GetEndParent(),
                         range->EndOffset(),
                         aSubtract, &cmp2);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  

  if (cmp2 > 0) {
    
    
    nsRange* postOverlap = new nsRange();

    rv =
      postOverlap->SetStart(aSubtract->GetEndParent(), aSubtract->EndOffset());
    NS_ENSURE_SUCCESS(rv, rv);
    rv =
     postOverlap->SetEnd(range->GetEndParent(), range->EndOffset());
    NS_ENSURE_SUCCESS(rv, rv);
    if (!postOverlap->Collapsed()) {
      if (!aOutput->InsertElementAt(0, RangeData(postOverlap)))
        return NS_ERROR_OUT_OF_MEMORY;
      (*aOutput)[0].mTextRangeStyle = aRange->mTextRangeStyle;
    }
  }

  if (cmp < 0) {
    
    
    nsRange* preOverlap = new nsRange();

    nsresult rv =
     preOverlap->SetStart(range->GetStartParent(), range->StartOffset());
    NS_ENSURE_SUCCESS(rv, rv);
    rv =
     preOverlap->SetEnd(aSubtract->GetStartParent(), aSubtract->StartOffset());
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (!preOverlap->Collapsed()) {
      if (!aOutput->InsertElementAt(0, RangeData(preOverlap)))
        return NS_ERROR_OUT_OF_MEMORY;
      (*aOutput)[0].mTextRangeStyle = aRange->mTextRangeStyle;
    }
  }

  return NS_OK;
}

nsresult
Selection::AddItem(nsRange* aItem, PRInt32* aOutIndex)
{
  if (!aItem)
    return NS_ERROR_NULL_POINTER;
  if (!aItem->IsPositioned())
    return NS_ERROR_UNEXPECTED;
  if (aOutIndex)
    *aOutIndex = -1;

  
  if (mRanges.Length() == 0) {
    if (!mRanges.AppendElement(RangeData(aItem)))
      return NS_ERROR_OUT_OF_MEMORY;
    aItem->SetInSelection(true);

    if (aOutIndex)
      *aOutIndex = 0;
    return NS_OK;
  }

  PRInt32 startIndex, endIndex;
  GetIndicesForInterval(aItem->GetStartParent(), aItem->StartOffset(),
                        aItem->GetEndParent(), aItem->EndOffset(),
                        false, &startIndex, &endIndex);

  if (endIndex == -1) {
    
    
    startIndex = 0;
    endIndex = 0;
  } else if (startIndex == -1) {
    
    
    startIndex = mRanges.Length();
    endIndex = startIndex;
  }

  
  bool sameRange = EqualsRangeAtPoint(aItem->GetStartParent(),
                                        aItem->StartOffset(),
                                        aItem->GetEndParent(),
                                        aItem->EndOffset(), startIndex);
  if (sameRange) {
    if (aOutIndex)
      *aOutIndex = startIndex;
    return NS_OK;
  }

  if (startIndex == endIndex) {
    
    if (!mRanges.InsertElementAt(startIndex, RangeData(aItem)))
      return NS_ERROR_OUT_OF_MEMORY;
    aItem->SetInSelection(true);
    if (aOutIndex)
      *aOutIndex = startIndex;
    return NS_OK;
  }

  
  
  
  
  
  
  nsTArray<RangeData> overlaps;
  if (!overlaps.InsertElementAt(0, mRanges[startIndex]))
    return NS_ERROR_OUT_OF_MEMORY;

  if (endIndex - 1 != startIndex) {
    if (!overlaps.InsertElementAt(1, mRanges[endIndex - 1]))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  
  for (PRInt32 i = startIndex; i < endIndex; ++i) {
    mRanges[i].mRange->SetInSelection(false);
  }
  mRanges.RemoveElementsAt(startIndex, endIndex - startIndex);

  nsTArray<RangeData> temp;
  for (PRInt32 i = overlaps.Length() - 1; i >= 0; i--) {
    nsresult rv = SubtractRange(&overlaps[i], aItem, &temp);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  PRInt32 insertionPoint;
  nsresult rv = FindInsertionPoint(&temp, aItem->GetStartParent(),
                                   aItem->StartOffset(),
                                   CompareToRangeStart,
                                   &insertionPoint);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!temp.InsertElementAt(insertionPoint, RangeData(aItem)))
    return NS_ERROR_OUT_OF_MEMORY;

  
  if (!mRanges.InsertElementsAt(startIndex, temp))
    return NS_ERROR_OUT_OF_MEMORY;

  for (PRUint32 i = 0; i < temp.Length(); ++i) {
    temp[i].mRange->SetInSelection(true);
  }

  *aOutIndex = startIndex + insertionPoint;
  return NS_OK;
}

nsresult
Selection::RemoveItem(nsRange* aItem)
{
  if (!aItem)
    return NS_ERROR_NULL_POINTER;

  
  
  
  
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
  aItem->SetInSelection(false);
  return NS_OK;
}

nsresult
Selection::RemoveCollapsedRanges()
{
  PRUint32 i = 0;
  while (i < mRanges.Length()) {
    if (mRanges[i].mRange->Collapsed()) {
      nsresult rv = RemoveItem(mRanges[i].mRange);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      ++i;
    }
  }
  return NS_OK;
}

nsresult
Selection::Clear(nsPresContext* aPresContext)
{
  setAnchorFocusRange(-1);

  for (PRUint32 i = 0; i < mRanges.Length(); ++i) {
    mRanges[i].mRange->SetInSelection(false);
    selectFrames(aPresContext, mRanges[i].mRange, false);
  }
  mRanges.Clear();

  
  SetDirection(eDirNext);

  
  if (mFrameSelection &&
      mFrameSelection->GetDisplaySelection() ==
      nsISelectionController::SELECTION_ATTENTION) {
    mFrameSelection->SetDisplaySelection(nsISelectionController::SELECTION_ON);
  }

  return NS_OK;
}

NS_IMETHODIMP
Selection::GetType(PRInt16* aType)
{
  NS_ENSURE_ARG_POINTER(aType);
  *aType = mType;

  return NS_OK;
}






static inline bool
RangeMatchesBeginPoint(nsRange* aRange, nsINode* aNode, PRInt32 aOffset)
{
  return aRange->GetStartParent() == aNode && aRange->StartOffset() == aOffset;
}

static inline bool
RangeMatchesEndPoint(nsRange* aRange, nsINode* aNode, PRInt32 aOffset)
{
  return aRange->GetEndParent() == aNode && aRange->EndOffset() == aOffset;
}





bool
Selection::EqualsRangeAtPoint(
    nsINode* aBeginNode, PRInt32 aBeginOffset,
    nsINode* aEndNode, PRInt32 aEndOffset,
    PRInt32 aRangeIndex)
{
  if (aRangeIndex >=0 && aRangeIndex < (PRInt32) mRanges.Length()) {
    nsRange* range = mRanges[aRangeIndex].mRange;
    if (RangeMatchesBeginPoint(range, aBeginNode, aBeginOffset) &&
        RangeMatchesEndPoint(range, aEndNode, aEndOffset))
      return true;
  }
  return false;
}





NS_IMETHODIMP
Selection::GetRangesForInterval(nsIDOMNode* aBeginNode, PRInt32 aBeginOffset,
                                nsIDOMNode* aEndNode, PRInt32 aEndOffset,
                                bool aAllowAdjacent,
                                PRUint32* aResultCount,
                                nsIDOMRange*** aResults)
{
  if (!aBeginNode || ! aEndNode || ! aResultCount || ! aResults)
    return NS_ERROR_NULL_POINTER;

  *aResultCount = 0;
  *aResults = nsnull;
  
  nsCOMPtr<nsINode> beginNode = do_QueryInterface(aBeginNode);
  nsCOMPtr<nsINode> endNode = do_QueryInterface(aEndNode);

  nsTArray<nsRange*> results;
  nsresult rv = GetRangesForIntervalArray(beginNode, aBeginOffset,
                                          endNode, aEndOffset,
                                          aAllowAdjacent, &results);
  NS_ENSURE_SUCCESS(rv, rv);
  *aResultCount = results.Length();
  if (*aResultCount == 0) {
    return NS_OK;
  }

  *aResults = static_cast<nsIDOMRange**>
                         (nsMemory::Alloc(sizeof(nsIDOMRange*) * *aResultCount));
  NS_ENSURE_TRUE(*aResults, NS_ERROR_OUT_OF_MEMORY);

  for (PRUint32 i = 0; i < *aResultCount; i++)
    NS_ADDREF((*aResults)[i] = results[i]);
  return NS_OK;
}





















nsresult
Selection::GetRangesForIntervalArray(nsINode* aBeginNode, PRInt32 aBeginOffset,
                                     nsINode* aEndNode, PRInt32 aEndOffset,
                                     bool aAllowAdjacent,
                                     nsTArray<nsRange*>* aRanges)
{
  aRanges->Clear();
  PRInt32 startIndex, endIndex;
  GetIndicesForInterval(aBeginNode, aBeginOffset, aEndNode, aEndOffset,
                        aAllowAdjacent, &startIndex, &endIndex);
  if (startIndex == -1 || endIndex == -1)
    return NS_OK;

  for (PRInt32 i = startIndex; i < endIndex; i++) {
    if (!aRanges->AppendElement(mRanges[i].mRange))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}







void
Selection::GetIndicesForInterval(nsINode* aBeginNode, PRInt32 aBeginOffset,
                                 nsINode* aEndNode, PRInt32 aEndOffset,
                                 bool aAllowAdjacent,
                                 PRInt32* aStartIndex, PRInt32* aEndIndex)
{
  PRInt32 startIndex;
  PRInt32 endIndex;

  if (!aStartIndex)
    aStartIndex = &startIndex;
  if (!aEndIndex)
    aEndIndex = &endIndex;

  *aStartIndex = -1;
  *aEndIndex = -1;

  if (mRanges.Length() == 0)
    return;

  bool intervalIsCollapsed = aBeginNode == aEndNode &&
    aBeginOffset == aEndOffset;

  bool doSearches = true;

  
  
  PRInt32 endsBeforeIndex;
  PRInt32 beginsAfterIndex;

  
  PRInt32 cmp;
  if (NS_FAILED(CompareToRangeEnd(aBeginNode, aBeginOffset,
                                  mRanges[mRanges.Length() - 1].mRange,
                                  &cmp))) {
    return;
  } else if (cmp == 1) {
    *aEndIndex = mRanges.Length();
    return;
  } else if (cmp == 0) {
    beginsAfterIndex = mRanges.Length() - 1;
    if (!intervalIsCollapsed) {
      endsBeforeIndex = beginsAfterIndex + 1;
    } else {
      endsBeforeIndex = beginsAfterIndex;
    }
    doSearches = false;
  }

  if (doSearches &&
      NS_FAILED(FindInsertionPoint(&mRanges, aEndNode, aEndOffset,
                                   &CompareToRangeStart,
                                   &endsBeforeIndex))) {
    return;
  }

  if (endsBeforeIndex == 0) {
    nsRange* endRange = mRanges[endsBeforeIndex].mRange;

    
    
    if (!RangeMatchesBeginPoint(endRange, aEndNode, aEndOffset))
      return;

    
    
    
    
    
    if (!aAllowAdjacent && !(endRange->Collapsed() && intervalIsCollapsed))
      return;
  }
  *aEndIndex = endsBeforeIndex;

  if (doSearches &&
      NS_FAILED(FindInsertionPoint(&mRanges, aBeginNode, aBeginOffset,
                                   &CompareToRangeEnd,
                                   &beginsAfterIndex))) {
    return;
  }

  if (aAllowAdjacent) {
    
    
    
    
    
    
    
    
    
    
    while (endsBeforeIndex < (PRInt32) mRanges.Length()) {
      nsRange* endRange = mRanges[endsBeforeIndex].mRange;
      if (!RangeMatchesBeginPoint(endRange, aEndNode, aEndOffset))
        break;
      endsBeforeIndex++;
    }

    
    
    
    
    
    
    
    
    
    
    
    nsRange* beginRange = mRanges[beginsAfterIndex].mRange;
    if (beginsAfterIndex > 0 && beginRange->Collapsed() &&
        RangeMatchesEndPoint(beginRange, aBeginNode, aBeginOffset)) {
      beginRange = mRanges[beginsAfterIndex - 1].mRange;
      if (RangeMatchesEndPoint(beginRange, aBeginNode, aBeginOffset))
        beginsAfterIndex--;
    }
  } else {
    
    
    
    
    nsRange* beginRange = mRanges[beginsAfterIndex].mRange;
    if (RangeMatchesEndPoint(beginRange, aBeginNode, aBeginOffset) &&
        !beginRange->Collapsed())
      beginsAfterIndex++;

    
    
    
    
    if (endsBeforeIndex < (PRInt32) mRanges.Length()) {
      nsRange* endRange = mRanges[endsBeforeIndex].mRange;
      if (RangeMatchesBeginPoint(endRange, aEndNode, aEndOffset) &&
          endRange->Collapsed())
        endsBeforeIndex++;
     }
  }

  *aStartIndex = beginsAfterIndex;
  *aEndIndex = endsBeforeIndex;
  return;
}

NS_IMETHODIMP
Selection::GetPrimaryFrameForAnchorNode(nsIFrame** aReturnFrame)
{
  if (!aReturnFrame)
    return NS_ERROR_NULL_POINTER;
  
  PRInt32 frameOffset = 0;
  *aReturnFrame = 0;
  nsCOMPtr<nsIContent> content = do_QueryInterface(GetAnchorNode());
  if (content && mFrameSelection)
  {
    *aReturnFrame = mFrameSelection->
      GetFrameForNodeOffset(content, GetAnchorOffset(),
                            mFrameSelection->GetHint(), &frameOffset);
    if (*aReturnFrame)
      return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
Selection::GetPrimaryFrameForFocusNode(nsIFrame** aReturnFrame,
                                       PRInt32* aOffsetUsed,
                                       bool aVisual)
{
  if (!aReturnFrame)
    return NS_ERROR_NULL_POINTER;
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(GetFocusNode());
  if (!content || !mFrameSelection)
    return NS_ERROR_FAILURE;
  
  nsIPresShell *presShell = mFrameSelection->GetShell();

  PRInt32 frameOffset = 0;
  *aReturnFrame = 0;
  if (!aOffsetUsed)
    aOffsetUsed = &frameOffset;
    
  nsFrameSelection::HINT hint = mFrameSelection->GetHint();

  if (aVisual) {
    nsRefPtr<nsCaret> caret = presShell->GetCaret();
    if (!caret)
      return NS_ERROR_FAILURE;
    
    PRUint8 caretBidiLevel = mFrameSelection->GetCaretBidiLevel();

    return caret->GetCaretFrameForNodeOffset(content, GetFocusOffset(),
      hint, caretBidiLevel, aReturnFrame, aOffsetUsed);
  }
  
  *aReturnFrame = mFrameSelection->
    GetFrameForNodeOffset(content, GetFocusOffset(),
                          hint, aOffsetUsed);
  if (!*aReturnFrame)
    return NS_ERROR_FAILURE;

  return NS_OK;
}


nsresult
Selection::SelectAllFramesForContent(nsIContentIterator* aInnerIter,
                                     nsIContent* aContent,
                                     bool aSelected)
{
  nsresult result = aInnerIter->Init(aContent);
  nsIFrame *frame;
  if (NS_SUCCEEDED(result))
  {
    
    frame = aContent->GetPrimaryFrame();
    if (frame && frame->GetType() == nsGkAtoms::textFrame) {
      nsTextFrame* textFrame = static_cast<nsTextFrame*>(frame);
      textFrame->SetSelectedRange(0, aContent->GetText()->GetLength(), aSelected, mType);
    }
    
    while (!aInnerIter->IsDone()) {
      nsCOMPtr<nsIContent> innercontent =
        do_QueryInterface(aInnerIter->GetCurrentNode());

      frame = innercontent->GetPrimaryFrame();
      if (frame) {
        if (frame->GetType() == nsGkAtoms::textFrame) {
          nsTextFrame* textFrame = static_cast<nsTextFrame*>(frame);
          textFrame->SetSelectedRange(0, innercontent->GetText()->GetLength(), aSelected, mType);
        } else {
          frame->InvalidateFrameSubtree();  
        }
      }

      aInnerIter->Next();
    }

    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}





nsresult
Selection::selectFrames(nsPresContext* aPresContext, nsRange* aRange,
                        bool aFlags)
{
  if (!mFrameSelection || !aPresContext || !aPresContext->GetPresShell()) {
    
    return NS_OK;
  }
  MOZ_ASSERT(aRange);

  if (mFrameSelection->GetTableCellSelection()) {
    nsINode* node = aRange->GetCommonAncestor();
    nsIFrame* frame = node->IsContent() ? node->AsContent()->GetPrimaryFrame()
                                : aPresContext->FrameManager()->GetRootFrame();
    if (frame) {
      frame->InvalidateFrameSubtree();
    }
    return NS_OK;
  }

  nsCOMPtr<nsIContentIterator> iter = NS_NewContentSubtreeIterator();
  iter->Init(aRange);

  
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(aRange->GetStartParent());
  NS_ENSURE_STATE(content);

  
  if (content->IsNodeOfType(nsINode::eTEXT)) {
    nsIFrame* frame = content->GetPrimaryFrame();
    
    if (frame && frame->GetType() == nsGkAtoms::textFrame) {
      nsTextFrame* textFrame = static_cast<nsTextFrame*>(frame);
      PRUint32 startOffset = aRange->StartOffset();
      PRUint32 endOffset;
      if (aRange->GetEndParent() == content) {
        endOffset = aRange->EndOffset();
      } else {
        endOffset = content->Length();
      }
      textFrame->SetSelectedRange(startOffset, endOffset, aFlags, mType);
    }
  }

  iter->First();
  nsCOMPtr<nsIContentIterator> inneriter = NS_NewContentIterator();
  for (iter->First(); !iter->IsDone(); iter->Next()) {
    content = do_QueryInterface(iter->GetCurrentNode());
    SelectAllFramesForContent(inneriter, content, aFlags);
  }

  
  if (aRange->GetEndParent() != aRange->GetStartParent()) {
    nsresult res;
    content = do_QueryInterface(aRange->GetEndParent(), &res);
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(content, res);

    if (content->IsNodeOfType(nsINode::eTEXT)) {
      nsIFrame* frame = content->GetPrimaryFrame();
      
      if (frame && frame->GetType() == nsGkAtoms::textFrame) {
        nsTextFrame* textFrame = static_cast<nsTextFrame*>(frame);
        textFrame->SetSelectedRange(0, aRange->EndOffset(), aFlags, mType);
      }
    }
  }
  return NS_OK;
}


























NS_IMETHODIMP
Selection::LookUpSelection(nsIContent* aContent, PRInt32 aContentOffset,
                           PRInt32 aContentLength,
                           SelectionDetails** aReturnDetails,
                           SelectionType aType, bool aSlowCheck)
{
  nsresult rv;
  if (!aContent || ! aReturnDetails)
    return NS_ERROR_NULL_POINTER;

  
  if (mRanges.Length() == 0)
    return NS_OK;

  nsTArray<nsRange*> overlappingRanges;
  rv = GetRangesForIntervalArray(aContent, aContentOffset,
                                 aContent, aContentOffset + aContentLength,
                                 false,
                                 &overlappingRanges);
  NS_ENSURE_SUCCESS(rv, rv);
  if (overlappingRanges.Length() == 0)
    return NS_OK;

  for (PRUint32 i = 0; i < overlappingRanges.Length(); i++) {
    nsRange* range = overlappingRanges[i];
    nsINode* startNode = range->GetStartParent();
    nsINode* endNode = range->GetEndParent();
    PRInt32 startOffset = range->StartOffset();
    PRInt32 endOffset = range->EndOffset();

    PRInt32 start = -1, end = -1;
    if (startNode == aContent && endNode == aContent) {
      if (startOffset < (aContentOffset + aContentLength)  &&
          endOffset > aContentOffset) {
        
        start = NS_MAX(0, startOffset - aContentOffset);
        end = NS_MIN(aContentLength, endOffset - aContentOffset);
      }
      
      
    } else if (startNode == aContent) {
      if (startOffset < (aContentOffset + aContentLength)) {
        
        
        start = NS_MAX(0, startOffset - aContentOffset);
        end = aContentLength;
      }
    } else if (endNode == aContent) {
      if (endOffset > aContentOffset) {
        
        
        start = 0;
        end = NS_MIN(aContentLength, endOffset - aContentOffset);
      }
    } else {
      
      
      
      
      start = 0;
      end = aContentLength;
    }
    if (start < 0)
      continue; 

    SelectionDetails* details = new SelectionDetails;

    details->mNext = *aReturnDetails;
    details->mStart = start;
    details->mEnd = end;
    details->mType = aType;
    RangeData *rd = FindRangeData(range);
    if (rd) {
      details->mTextRangeStyle = rd->mTextRangeStyle;
    }
    *aReturnDetails = details;
  }
  return NS_OK;
}

NS_IMETHODIMP
Selection::Repaint(nsPresContext* aPresContext)
{
  PRInt32 arrCount = (PRInt32)mRanges.Length();

  if (arrCount < 1)
    return NS_OK;

  PRInt32 i;
  
  for (i = 0; i < arrCount; i++)
  {
    nsresult rv = selectFrames(aPresContext, mRanges[i].mRange, true);

    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
Selection::GetCanCacheFrameOffset(bool* aCanCacheFrameOffset)
{ 
  NS_ENSURE_ARG_POINTER(aCanCacheFrameOffset);

  if (mCachedOffsetForFrame)
    *aCanCacheFrameOffset = mCachedOffsetForFrame->mCanCacheFrameOffset;
  else
    *aCanCacheFrameOffset = false;

  return NS_OK;
}

NS_IMETHODIMP    
Selection::SetCanCacheFrameOffset(bool aCanCacheFrameOffset)
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
Selection::GetCachedFrameOffset(nsIFrame* aFrame, PRInt32 inOffset,
                                nsPoint& aPoint)
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
     
     
     rv = aFrame->GetPointFromOffset(inOffset, &aPoint);
     if (NS_SUCCEEDED(rv) && mCachedOffsetForFrame->mCanCacheFrameOffset) {
       mCachedOffsetForFrame->mCachedFrameOffset = aPoint;
       mCachedOffsetForFrame->mLastCaretFrame = aFrame;
       mCachedOffsetForFrame->mLastContentOffset = inOffset; 
     }
  }

  return rv;
}

NS_IMETHODIMP
Selection::GetFrameSelection(nsFrameSelection** aFrameSelection) {
  NS_ENSURE_ARG_POINTER(aFrameSelection);
  *aFrameSelection = mFrameSelection;
  NS_IF_ADDREF(*aFrameSelection);
  return NS_OK;
}

NS_IMETHODIMP
Selection::SetAncestorLimiter(nsIContent* aContent)
{
  if (mFrameSelection)
    mFrameSelection->SetAncestorLimiter(aContent);
  return NS_OK;
}

RangeData*
Selection::FindRangeData(nsIDOMRange* aRange)
{
  NS_ENSURE_TRUE(aRange, nsnull);
  for (PRUint32 i = 0; i < mRanges.Length(); i++) {
    if (mRanges[i].mRange == aRange)
      return &mRanges[i];
  }
  return nsnull;
}

NS_IMETHODIMP
Selection::SetTextRangeStyle(nsIDOMRange* aRange,
                             const nsTextRangeStyle& aTextRangeStyle)
{
  NS_ENSURE_ARG_POINTER(aRange);
  RangeData *rd = FindRangeData(aRange);
  if (rd) {
    rd->mTextRangeStyle = aTextRangeStyle;
  }
  return NS_OK;
}

nsresult
Selection::StartAutoScrollTimer(nsIFrame* aFrame, nsPoint& aPoint,
                                PRUint32 aDelay)
{
  NS_PRECONDITION(aFrame, "Need a frame");

  nsresult result;
  if (!mFrameSelection)
    return NS_OK;

  if (!mAutoScrollTimer)
  {
    mAutoScrollTimer = new nsAutoScrollTimer();

    result = mAutoScrollTimer->Init(mFrameSelection, this);

    if (NS_FAILED(result))
      return result;
  }

  result = mAutoScrollTimer->SetDelay(aDelay);

  if (NS_FAILED(result))
    return result;

  return DoAutoScroll(aFrame, aPoint);
}

nsresult
Selection::StopAutoScrollTimer()
{
  if (mAutoScrollTimer)
    return mAutoScrollTimer->Stop();

  return NS_OK; 
}

nsresult
Selection::DoAutoScroll(nsIFrame* aFrame, nsPoint& aPoint)
{
  NS_PRECONDITION(aFrame, "Need a frame");

  if (mAutoScrollTimer)
    (void)mAutoScrollTimer->Stop();

  nsPresContext* presContext = aFrame->PresContext();
  nsRootPresContext* rootPC = presContext->GetRootPresContext();
  if (!rootPC)
    return NS_OK;
  nsIFrame* rootmostFrame = rootPC->PresShell()->FrameManager()->GetRootFrame();
  
  
  nsPoint globalPoint = aPoint + aFrame->GetOffsetToCrossDoc(rootmostFrame);

  bool didScroll = presContext->PresShell()->ScrollFrameRectIntoView(
    aFrame, 
    nsRect(aPoint, nsSize(0, 0)),
    nsIPresShell::ScrollAxis(),
    nsIPresShell::ScrollAxis(),
    0);

  
  
  

  if (didScroll && mAutoScrollTimer)
  {
    nsPoint presContextPoint = globalPoint -
      presContext->PresShell()->FrameManager()->GetRootFrame()->GetOffsetToCrossDoc(rootmostFrame);
    mAutoScrollTimer->Start(presContext, presContextPoint);
  }

  return NS_OK;
}

NS_IMETHODIMP
Selection::GetEnumerator(nsIEnumerator** aIterator)
{
  NS_ADDREF(*aIterator = new nsSelectionIterator(this));
  return NS_OK;
}





NS_IMETHODIMP
Selection::RemoveAllRanges()
{
  if (!mFrameSelection)
    return NS_OK;
  nsRefPtr<nsPresContext>  presContext;
  GetPresContext(getter_AddRefs(presContext));


  nsresult  result = Clear(presContext);
  if (NS_FAILED(result))
    return result;
  
  
  mFrameSelection->ClearTableCellSelection();

  return mFrameSelection->NotifySelectionListeners(GetType());
  
  
}




NS_IMETHODIMP
Selection::AddRange(nsIDOMRange* aDOMRange)
{
  if (!aDOMRange) {
    return NS_ERROR_NULL_POINTER;
  }
  nsRange* range = static_cast<nsRange*>(aDOMRange);

  
  
  bool didAddRange;
  PRInt32 rangeIndex;
  nsresult result = addTableCellRange(range, &didAddRange, &rangeIndex);
  if (NS_FAILED(result)) return result;

  if (!didAddRange)
  {
    result = AddItem(range, &rangeIndex);
    if (NS_FAILED(result)) return result;
  }

  NS_ASSERTION(rangeIndex >= 0, "Range index not returned");
  setAnchorFocusRange(rangeIndex);
  
  
  if (mType == nsISelectionController::SELECTION_NORMAL)
    SetInterlinePosition(true);

  nsRefPtr<nsPresContext>  presContext;
  GetPresContext(getter_AddRefs(presContext));
  selectFrames(presContext, range, true);

  if (!mFrameSelection)
    return NS_OK;

  return mFrameSelection->NotifySelectionListeners(GetType());
}













nsresult
Selection::RemoveRange(nsIDOMRange* aDOMRange)
{
  if (!aDOMRange) {
    return NS_ERROR_INVALID_ARG;
  }
  nsRefPtr<nsRange> range = static_cast<nsRange*>(aDOMRange);
  
  nsresult rv = RemoveItem(range);
  if (NS_FAILED(rv))
    return rv;

  nsINode* beginNode = range->GetStartParent();
  nsINode* endNode = range->GetEndParent();

  if (!beginNode || !endNode) {
    
    return NS_OK;
  }
  
  
  PRInt32 beginOffset, endOffset;
  if (endNode->IsNodeOfType(nsINode::eTEXT)) {
    
    
    
    beginOffset = 0;
    endOffset = static_cast<nsIContent*>(endNode)->TextLength();
  } else {
    
    beginOffset = range->StartOffset();
    endOffset = range->EndOffset();
  }

  
  nsRefPtr<nsPresContext>  presContext;
  GetPresContext(getter_AddRefs(presContext));
  selectFrames(presContext, range, false);

  
  nsTArray<nsRange*> affectedRanges;
  rv = GetRangesForIntervalArray(beginNode, beginOffset,
                                 endNode, endOffset,
                                 true, &affectedRanges);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 i = 0; i < affectedRanges.Length(); i++) {
    selectFrames(presContext, affectedRanges[i], true);
  }

  PRInt32 cnt = mRanges.Length();
  if (range == mAnchorFocusRange) {
    
    setAnchorFocusRange(cnt - 1);

    
    
    
    
    if (mType != nsISelectionController::SELECTION_SPELLCHECK && cnt > 0)
      ScrollIntoView(nsISelectionController::SELECTION_FOCUS_REGION);
  }

  if (!mFrameSelection)
    return NS_OK;
  return mFrameSelection->NotifySelectionListeners(GetType());
}






NS_IMETHODIMP
Selection::Collapse(nsIDOMNode* aParentNode, PRInt32 aOffset)
{
  nsCOMPtr<nsINode> parentNode = do_QueryInterface(aParentNode);
  return Collapse(parentNode, aOffset);
}

NS_IMETHODIMP
Selection::CollapseNative(nsINode* aParentNode, PRInt32 aOffset)
{
  return Collapse(aParentNode, aOffset);
}

nsresult
Selection::Collapse(nsINode* aParentNode, PRInt32 aOffset)
{
  if (!aParentNode)
    return NS_ERROR_INVALID_ARG;
  if (!mFrameSelection)
    return NS_ERROR_NOT_INITIALIZED; 

  nsCOMPtr<nsINode> kungfuDeathGrip = aParentNode;

  mFrameSelection->InvalidateDesiredX();
  if (!IsValidSelectionPoint(mFrameSelection, aParentNode))
    return NS_ERROR_FAILURE;
  nsresult result;
  
  nsRefPtr<nsPresContext>  presContext;
  GetPresContext(getter_AddRefs(presContext));
  Clear(presContext);

  
  mFrameSelection->ClearTableCellSelection();

  nsRefPtr<nsRange> range = new nsRange();
  result = range->SetEnd(aParentNode, aOffset);
  if (NS_FAILED(result))
    return result;
  result = range->SetStart(aParentNode, aOffset);
  if (NS_FAILED(result))
    return result;

#ifdef DEBUG_SELECTION
  if (aParentNode) {
    nsCOMPtr<nsIContent> content = do_QueryInterface(aParentNode);
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(aParentNode);
    printf ("Sel. Collapse to %p %s %d\n", aParentNode,
            content ? nsAtomCString(content->Tag()).get()
                    : (doc ? "DOCUMENT" : "???"),
            aOffset);
  }
#endif

  result = AddItem(range);
  if (NS_FAILED(result))
    return result;
  setAnchorFocusRange(0);
  selectFrames(presContext, range, true);
  return mFrameSelection->NotifySelectionListeners(GetType());
}





NS_IMETHODIMP
Selection::CollapseToStart()
{
  PRInt32 cnt;
  nsresult rv = GetRangeCount(&cnt);
  if (NS_FAILED(rv) || cnt <= 0)
    return NS_ERROR_DOM_INVALID_STATE_ERR;

  
  nsRange* firstRange = mRanges[0].mRange;
  if (!firstRange)
    return NS_ERROR_FAILURE;

  return Collapse(firstRange->GetStartParent(), firstRange->StartOffset());
}





NS_IMETHODIMP
Selection::CollapseToEnd()
{
  PRInt32 cnt;
  nsresult rv = GetRangeCount(&cnt);
  if (NS_FAILED(rv) || cnt <= 0)
    return NS_ERROR_DOM_INVALID_STATE_ERR;

  
  nsRange* lastRange = mRanges[cnt - 1].mRange;
  if (!lastRange)
    return NS_ERROR_FAILURE;

  return Collapse(lastRange->GetEndParent(), lastRange->EndOffset());
}




bool
Selection::IsCollapsed()
{
  PRUint32 cnt = mRanges.Length();
  if (cnt == 0) {
    return true;
  }

  if (cnt != 1) {
    return false;
  }

  return mRanges[0].mRange->Collapsed();
}


bool
Selection::Collapsed()
{
  return IsCollapsed();
}

NS_IMETHODIMP
Selection::GetIsCollapsed(bool* aIsCollapsed)
{
  NS_ENSURE_TRUE(aIsCollapsed, NS_ERROR_NULL_POINTER);

  *aIsCollapsed = IsCollapsed();
  return NS_OK;
}

NS_IMETHODIMP
Selection::GetRangeCount(PRInt32* aRangeCount)
{
  *aRangeCount = (PRInt32)mRanges.Length();

  return NS_OK;
}

NS_IMETHODIMP
Selection::GetRangeAt(PRInt32 aIndex, nsIDOMRange** aReturn)
{
  *aReturn = mRanges.SafeElementAt(aIndex, sEmptyData).mRange;
  if (!*aReturn) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  NS_ADDREF(*aReturn);

  return NS_OK;
}

nsRange*
Selection::GetRangeAt(PRInt32 aIndex)
{
  return mRanges.SafeElementAt(aIndex, sEmptyData).mRange;
}




nsresult
Selection::SetAnchorFocusToRange(nsRange* aRange)
{
  NS_ENSURE_STATE(mAnchorFocusRange);

  nsresult res = RemoveItem(mAnchorFocusRange);
  if (NS_FAILED(res))
    return res;

  PRInt32 aOutIndex = -1;
  res = AddItem(aRange, &aOutIndex);
  if (NS_FAILED(res))
    return res;
  setAnchorFocusRange(aOutIndex);

  return NS_OK;
}

void
Selection::ReplaceAnchorFocusRange(nsRange* aRange)
{
  nsRefPtr<nsPresContext> presContext;
  GetPresContext(getter_AddRefs(presContext));
  if (presContext) {
    selectFrames(presContext, mAnchorFocusRange, false);
    SetAnchorFocusToRange(aRange);
    selectFrames(presContext, mAnchorFocusRange, true);
  }
}






























NS_IMETHODIMP
Selection::Extend(nsIDOMNode* aParentNode, PRInt32 aOffset)
{
  nsCOMPtr<nsINode> parentNode = do_QueryInterface(aParentNode);
  return Extend(parentNode, aOffset);
}

NS_IMETHODIMP
Selection::ExtendNative(nsINode* aParentNode, PRInt32 aOffset)
{
  return Extend(aParentNode, aOffset);
}

nsresult
Selection::Extend(nsINode* aParentNode, PRInt32 aOffset)
{
  if (!aParentNode)
    return NS_ERROR_INVALID_ARG;

  
  if (!mAnchorFocusRange)
    return NS_ERROR_DOM_INVALID_STATE_ERR;

  if (!mFrameSelection)
    return NS_ERROR_NOT_INITIALIZED; 

  nsresult res;
  if (!IsValidSelectionPoint(mFrameSelection, aParentNode))
    return NS_ERROR_FAILURE;

  

  nsINode* anchorNode = GetAnchorNode();
  nsINode* focusNode = GetFocusNode();
  PRInt32 anchorOffset = GetAnchorOffset();
  PRInt32 focusOffset = GetFocusOffset();

  nsRefPtr<nsRange> range = mAnchorFocusRange->CloneRange();

  nsINode* startNode = range->GetStartParent();
  nsINode* endNode = range->GetEndParent();
  PRInt32 startOffset = range->StartOffset();
  PRInt32 endOffset = range->EndOffset();

  nsDirection dir = GetDirection();

  

  
  
  
  bool disconnected = false;
  PRInt32 result1 = nsContentUtils::ComparePoints(anchorNode, anchorOffset,
                                                  focusNode, focusOffset,
                                                  &disconnected);
  
  PRInt32 result2 = nsContentUtils::ComparePoints(focusNode, focusOffset,
                                                  aParentNode, aOffset,
                                                  &disconnected);
  
  PRInt32 result3 = nsContentUtils::ComparePoints(anchorNode, anchorOffset,
                                                  aParentNode, aOffset,
                                                  &disconnected);

  nsRefPtr<nsPresContext>  presContext;
  GetPresContext(getter_AddRefs(presContext));
  nsRefPtr<nsRange> difRange = new nsRange();
  if ((result1 == 0 && result3 < 0) || (result1 <= 0 && result2 < 0)){
    
    res = range->SetEnd(aParentNode, aOffset);
    if (NS_FAILED(res))
      return res;
    dir = eDirNext;
    res = difRange->SetEnd(range->GetEndParent(), range->EndOffset());
    res |= difRange->SetStart(focusNode, focusOffset);
    if (NS_FAILED(res))
      return res;
    selectFrames(presContext, difRange , true);
    res = SetAnchorFocusToRange(range);
    if (NS_FAILED(res))
      return res;
  }
  else if (result1 == 0 && result3 > 0){
    
    dir = eDirPrevious;
    res = range->SetStart(aParentNode, aOffset);
    if (NS_FAILED(res))
      return res;
    selectFrames(presContext, range, true);
    res = SetAnchorFocusToRange(range);
    if (NS_FAILED(res))
      return res;
  }
  else if (result3 <= 0 && result2 >= 0) {
    
    res = difRange->SetEnd(focusNode, focusOffset);
    res |= difRange->SetStart(aParentNode, aOffset);
    if (NS_FAILED(res))
      return res;

    res = range->SetEnd(aParentNode, aOffset);
    if (NS_FAILED(res))
      return res;
    res = SetAnchorFocusToRange(range);
    if (NS_FAILED(res))
      return res;
    selectFrames(presContext, difRange, false); 
    difRange->SetEnd(range->GetEndParent(), range->EndOffset());
    selectFrames(presContext, difRange, true); 
  }
  else if (result1 >= 0 && result3 <= 0) {
    if (GetDirection() == eDirPrevious){
      res = range->SetStart(endNode, endOffset);
      if (NS_FAILED(res))
        return res;
    }
    dir = eDirNext;
    res = range->SetEnd(aParentNode, aOffset);
    if (NS_FAILED(res))
      return res;
    if (focusNode != anchorNode || focusOffset != anchorOffset) {
      res = difRange->SetStart(focusNode, focusOffset);
      res |= difRange->SetEnd(anchorNode, anchorOffset);
      if (NS_FAILED(res))
        return res;
      res = SetAnchorFocusToRange(range);
      if (NS_FAILED(res))
        return res;
      
      selectFrames(presContext, difRange , false);
    }
    else
    {
      res = SetAnchorFocusToRange(range);
      if (NS_FAILED(res))
        return res;
    }
    
    selectFrames(presContext, range , true);
  }
  else if (result2 <= 0 && result3 >= 0) {
    
    res = difRange->SetEnd(aParentNode, aOffset);
    res |= difRange->SetStart(focusNode, focusOffset);
    if (NS_FAILED(res))
      return res;
    dir = eDirPrevious;
    res = range->SetStart(aParentNode, aOffset);
    if (NS_FAILED(res))
      return res;

    res = SetAnchorFocusToRange(range);
    if (NS_FAILED(res))
      return res;
    selectFrames(presContext, difRange , false);
    difRange->SetStart(range->GetStartParent(), range->StartOffset());
    selectFrames(presContext, difRange, true);
  }
  else if (result3 >= 0 && result1 <= 0) {
    if (GetDirection() == eDirNext){
      range->SetEnd(startNode, startOffset);
    }
    dir = eDirPrevious;
    res = range->SetStart(aParentNode, aOffset);
    if (NS_FAILED(res))
      return res;
    
    if (focusNode != anchorNode || focusOffset!= anchorOffset) {
      res = difRange->SetStart(anchorNode, anchorOffset);
      res |= difRange->SetEnd(focusNode, focusOffset);
      res |= SetAnchorFocusToRange(range);
      if (NS_FAILED(res))
        return res;
      selectFrames(presContext, difRange, false);
    }
    else
    {
      res = SetAnchorFocusToRange(range);
      if (NS_FAILED(res))
        return res;
    }
    
    selectFrames(presContext, range , true);
  }
  else if (result2 >= 0 && result1 >= 0) {
    
    res = range->SetStart(aParentNode, aOffset);
    if (NS_FAILED(res))
      return res;
    dir = eDirPrevious;
    res = difRange->SetEnd(focusNode, focusOffset);
    res |= difRange->SetStart(range->GetStartParent(), range->StartOffset());
    if (NS_FAILED(res))
      return res;

    selectFrames(presContext, difRange, true);
    res = SetAnchorFocusToRange(range);
    if (NS_FAILED(res))
      return res;
  }

  DEBUG_OUT_RANGE(range);
#ifdef DEBUG_SELECTION
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

    printf ("Sel. Extend to %p %s %d\n", content.get(),
            nsAtomCString(content->Tag()).get(), aOffset);
  }
  else {
    printf ("Sel. Extend set to null parent.\n");
  }
#endif
  return mFrameSelection->NotifySelectionListeners(GetType());
}

NS_IMETHODIMP
Selection::SelectAllChildren(nsIDOMNode* aParentNode)
{
  NS_ENSURE_ARG_POINTER(aParentNode);
  nsCOMPtr<nsINode> node = do_QueryInterface(aParentNode);

  if (mFrameSelection)
  {
    mFrameSelection->PostReason(nsISelectionListener::SELECTALL_REASON);
  }
  nsresult result = Collapse(node, 0);
  if (NS_FAILED(result))
    return result;

  if (mFrameSelection)
  {
    mFrameSelection->PostReason(nsISelectionListener::SELECTALL_REASON);
  }
  return Extend(node, node->GetChildCount());
}

NS_IMETHODIMP
Selection::ContainsNode(nsIDOMNode* aNode, bool aAllowPartial, bool* aYes)
{
  nsresult rv;
  if (!aYes)
    return NS_ERROR_NULL_POINTER;
  *aYes = false;

  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  if (mRanges.Length() == 0 || !node)
    return NS_OK;

  
  PRUint32 nodeLength;
  bool isData = node->IsNodeOfType(nsINode::eDATA_NODE);
  if (isData) {
    nodeLength = static_cast<nsIContent*>(node.get())->TextLength();
  } else {
    nodeLength = node->GetChildCount();
  }

  nsTArray<nsRange*> overlappingRanges;
  rv = GetRangesForIntervalArray(node, 0, node, nodeLength,
                                 false, &overlappingRanges);
  NS_ENSURE_SUCCESS(rv, rv);
  if (overlappingRanges.Length() == 0)
    return NS_OK; 
  
  
  if (aAllowPartial) {
    *aYes = true;
    return NS_OK;
  }

  
  if (isData) {
    *aYes = true;
    return NS_OK;
  }

  
  
  for (PRUint32 i = 0; i < overlappingRanges.Length(); i++) {
    bool nodeStartsBeforeRange, nodeEndsAfterRange;
    if (NS_SUCCEEDED(nsRange::CompareNodeToRange(node, overlappingRanges[i],
                                                 &nodeStartsBeforeRange,
                                                 &nodeEndsAfterRange))) {
      if (!nodeStartsBeforeRange && !nodeEndsAfterRange) {
        *aYes = true;
        return NS_OK;
      }
    }
  }
  return NS_OK;
}


nsresult
Selection::GetPresContext(nsPresContext** aPresContext)
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
Selection::GetPresShell(nsIPresShell** aPresShell)
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

nsIFrame *
Selection::GetSelectionAnchorGeometry(SelectionRegion aRegion, nsRect* aRect)
{
  if (!mFrameSelection)
    return nsnull;  

  NS_ENSURE_TRUE(aRect, nsnull);

  aRect->SetRect(0, 0, 0, 0);

  switch (aRegion) {
    case nsISelectionController::SELECTION_ANCHOR_REGION:
    case nsISelectionController::SELECTION_FOCUS_REGION:
      return GetSelectionEndPointGeometry(aRegion, aRect);
      break;
    case nsISelectionController::SELECTION_WHOLE_SELECTION:
      break;
    default:
      return nsnull;
  }

  NS_ASSERTION(aRegion == nsISelectionController::SELECTION_WHOLE_SELECTION,
    "should only be SELECTION_WHOLE_SELECTION here");

  nsRect anchorRect;
  nsIFrame* anchorFrame = GetSelectionEndPointGeometry(
    nsISelectionController::SELECTION_ANCHOR_REGION, &anchorRect);
  if (!anchorFrame)
    return nsnull;

  nsRect focusRect;
  nsIFrame* focusFrame = GetSelectionEndPointGeometry(
    nsISelectionController::SELECTION_FOCUS_REGION, &focusRect);
  if (!focusFrame)
    return nsnull;

  NS_ASSERTION(anchorFrame->PresContext() == focusFrame->PresContext(),
    "points of selection in different documents?");
  
  focusRect += focusFrame->GetOffsetTo(anchorFrame);

  aRect->UnionRectEdges(anchorRect, focusRect);
  return anchorFrame;
}

nsIFrame *
Selection::GetSelectionEndPointGeometry(SelectionRegion aRegion, nsRect* aRect)
{
  if (!mFrameSelection)
    return nsnull;  

  NS_ENSURE_TRUE(aRect, nsnull);

  aRect->SetRect(0, 0, 0, 0);

  nsINode    *node       = nsnull;
  PRInt32     nodeOffset = 0;
  nsIFrame   *frame      = nsnull;

  switch (aRegion) {
    case nsISelectionController::SELECTION_ANCHOR_REGION:
      node       = GetAnchorNode();
      nodeOffset = GetAnchorOffset();
      break;
    case nsISelectionController::SELECTION_FOCUS_REGION:
      node       = GetFocusNode();
      nodeOffset = GetFocusOffset();
      break;
    default:
      return nsnull;
  }

  if (!node)
    return nsnull;

  nsCOMPtr<nsIContent> content = do_QueryInterface(node);
  NS_ENSURE_TRUE(content.get(), nsnull);
  PRInt32 frameOffset = 0;
  frame = mFrameSelection->GetFrameForNodeOffset(content, nodeOffset,
                                                 mFrameSelection->GetHint(),
                                                 &frameOffset);
  if (!frame)
    return nsnull;

  
  
  bool isText = node->IsNodeOfType(nsINode::eTEXT);

  nsPoint pt(0, 0);
  if (isText) {
    nsIFrame* childFrame = nsnull;
    frameOffset = 0;
    nsresult rv =
      frame->GetChildFrameContainingOffset(nodeOffset,
                                           mFrameSelection->GetHint(),
                                           &frameOffset, &childFrame);
    if (NS_FAILED(rv))
      return nsnull;
    if (!childFrame)
      return nsnull;

    frame = childFrame;

    
    rv = GetCachedFrameOffset(frame, nodeOffset, pt);
    if (NS_FAILED(rv))
      return nsnull;
  }

  
  if (isText) {
    aRect->x = pt.x;
  } else if (mFrameSelection->GetHint() == nsFrameSelection::HINTLEFT) {
    
    aRect->x = frame->GetRect().width;
  }
  aRect->height = frame->GetRect().height;

  return frame;
}

NS_IMETHODIMP
Selection::ScrollSelectionIntoViewEvent::Run()
{
  if (!mSelection)
    return NS_OK;  

  PRInt32 flags = Selection::SCROLL_DO_FLUSH |
                  Selection::SCROLL_SYNCHRONOUS;
  if (mFirstAncestorOnly) {
    flags |= Selection::SCROLL_FIRST_ANCESTOR_ONLY;
  }

  mSelection->mScrollEvent.Forget();
  mSelection->ScrollIntoView(mRegion, mVerticalScroll,
                             mHorizontalScroll, flags);
  return NS_OK;
}

nsresult
Selection::PostScrollSelectionIntoViewEvent(
                                         SelectionRegion aRegion,
                                         bool aFirstAncestorOnly,
                                         nsIPresShell::ScrollAxis aVertical,
                                         nsIPresShell::ScrollAxis aHorizontal)
{
  
  
  
  
  mScrollEvent.Revoke();

  nsRefPtr<ScrollSelectionIntoViewEvent> ev =
      new ScrollSelectionIntoViewEvent(this, aRegion, aVertical, aHorizontal,
                                       aFirstAncestorOnly);
  nsresult rv = NS_DispatchToCurrentThread(ev);
  NS_ENSURE_SUCCESS(rv, rv);

  mScrollEvent = ev;
  return NS_OK;
}

NS_IMETHODIMP
Selection::ScrollIntoView(SelectionRegion aRegion, bool aIsSynchronous,
                          PRInt16 aVPercent, PRInt16 aHPercent)
{
  return ScrollIntoViewInternal(aRegion,
                                aIsSynchronous,
                                nsIPresShell::ScrollAxis(aVPercent),
                                nsIPresShell::ScrollAxis(aHPercent));
}

NS_IMETHODIMP
Selection::ScrollIntoViewInternal(SelectionRegion aRegion, bool aIsSynchronous,
                                  nsIPresShell::ScrollAxis aVertical,
                                  nsIPresShell::ScrollAxis aHorizontal)
{
  return ScrollIntoView(aRegion, aVertical, aHorizontal,
                        aIsSynchronous ? Selection::SCROLL_SYNCHRONOUS : 0);
}

nsresult
Selection::ScrollIntoView(SelectionRegion aRegion,
                          nsIPresShell::ScrollAxis aVertical,
                          nsIPresShell::ScrollAxis aHorizontal,
                          PRInt32 aFlags)
{
  nsresult result;
  if (!mFrameSelection)
    return NS_OK;

  if (mFrameSelection->GetBatching())
    return NS_OK;

  if (!(aFlags & Selection::SCROLL_SYNCHRONOUS))
    return PostScrollSelectionIntoViewEvent(aRegion,
      !!(aFlags & Selection::SCROLL_FIRST_ANCESTOR_ONLY),
      aVertical, aHorizontal);

  
  
  
  
  nsCOMPtr<nsIPresShell> presShell;
  result = GetPresShell(getter_AddRefs(presShell));
  if (NS_FAILED(result) || !presShell)
    return result;
  nsRefPtr<nsCaret> caret = presShell->GetCaret();
  if (caret)
  {
    
    
    
    
    
    if (aFlags & Selection::SCROLL_DO_FLUSH) {
      presShell->FlushPendingNotifications(Flush_Layout);

      
      result = GetPresShell(getter_AddRefs(presShell));
      if (NS_FAILED(result) || !presShell)
        return result;
    }

    StCaretHider  caretHider(caret);      

    
    
    

    nsRect rect;
    nsIFrame* frame = GetSelectionAnchorGeometry(aRegion, &rect);
    if (!frame)
      return NS_ERROR_FAILURE;

    presShell->ScrollFrameRectIntoView(frame, rect, aVertical, aHorizontal,
      (aFlags & Selection::SCROLL_FIRST_ANCESTOR_ONLY) ?
       nsIPresShell::SCROLL_FIRST_ANCESTOR_ONLY : 0);
    return NS_OK;
  }
  return result;
}



NS_IMETHODIMP
Selection::AddSelectionListener(nsISelectionListener* aNewListener)
{
  if (!aNewListener)
    return NS_ERROR_NULL_POINTER;
  return mSelectionListeners.AppendObject(aNewListener) ? NS_OK : NS_ERROR_FAILURE;      
}



NS_IMETHODIMP
Selection::RemoveSelectionListener(nsISelectionListener* aListenerToRemove)
{
  if (!aListenerToRemove )
    return NS_ERROR_NULL_POINTER;
  return mSelectionListeners.RemoveObject(aListenerToRemove) ? NS_OK : NS_ERROR_FAILURE; 
}


nsresult
Selection::NotifySelectionListeners()
{
  if (!mFrameSelection)
    return NS_OK;
 
  if (mFrameSelection->GetBatching()) {
    mFrameSelection->SetDirty();
    return NS_OK;
  }
  nsCOMArray<nsISelectionListener> selectionListeners(mSelectionListeners);
  PRInt32 cnt = selectionListeners.Count();
  if (cnt != mSelectionListeners.Count()) {
    return NS_ERROR_OUT_OF_MEMORY;  
  }
  nsCOMPtr<nsIDOMDocument> domdoc;
  nsCOMPtr<nsIPresShell> shell;
  nsresult rv = GetPresShell(getter_AddRefs(shell));
  if (NS_SUCCEEDED(rv) && shell)
    domdoc = do_QueryInterface(shell->GetDocument());
  short reason = mFrameSelection->PopReason();
  for (PRInt32 i = 0; i < cnt; i++) {
    selectionListeners[i]->NotifySelectionChanged(domdoc, this, reason);
  }
  return NS_OK;
}

NS_IMETHODIMP
Selection::StartBatchChanges()
{
  if (mFrameSelection)
    mFrameSelection->StartBatchChanges();

  return NS_OK;
}



NS_IMETHODIMP
Selection::EndBatchChanges()
{
  if (mFrameSelection)
    mFrameSelection->EndBatchChanges();

  return NS_OK;
}



NS_IMETHODIMP
Selection::DeleteFromDocument()
{
  if (!mFrameSelection)
    return NS_OK;
  return mFrameSelection->DeleteFromDocument();
}

NS_IMETHODIMP
Selection::Modify(const nsAString& aAlter, const nsAString& aDirection,
                  const nsAString& aGranularity)
{
  
  if (!mFrameSelection || !GetAnchorFocusRange() || !GetFocusNode()) {
    return NS_OK;
  }

  if (!aAlter.LowerCaseEqualsLiteral("move") &&
      !aAlter.LowerCaseEqualsLiteral("extend")) {
    return NS_ERROR_INVALID_ARG;
  }

  if (!aDirection.LowerCaseEqualsLiteral("forward") &&
      !aDirection.LowerCaseEqualsLiteral("backward") &&
      !aDirection.LowerCaseEqualsLiteral("left") &&
      !aDirection.LowerCaseEqualsLiteral("right")) {
    return NS_ERROR_INVALID_ARG;
  }

  
  bool visual  = aDirection.LowerCaseEqualsLiteral("left") ||
                   aDirection.LowerCaseEqualsLiteral("right") ||
                   aGranularity.LowerCaseEqualsLiteral("line");

  bool forward = aDirection.LowerCaseEqualsLiteral("forward") ||
                   aDirection.LowerCaseEqualsLiteral("right");

  bool extend  = aAlter.LowerCaseEqualsLiteral("extend");

  
  nsSelectionAmount amount;
  PRUint32 keycode;
  if (aGranularity.LowerCaseEqualsLiteral("character")) {
    amount = eSelectCluster;
    keycode = forward ? (PRUint32) nsIDOMKeyEvent::DOM_VK_RIGHT :
                        (PRUint32) nsIDOMKeyEvent::DOM_VK_LEFT;
  }
  else if (aGranularity.LowerCaseEqualsLiteral("word")) {
    amount = eSelectWordNoSpace;
    keycode = forward ? (PRUint32) nsIDOMKeyEvent::DOM_VK_RIGHT :
                        (PRUint32) nsIDOMKeyEvent::DOM_VK_LEFT;
  }
  else if (aGranularity.LowerCaseEqualsLiteral("line")) {
    amount = eSelectLine;
    keycode = forward ? (PRUint32) nsIDOMKeyEvent::DOM_VK_DOWN :
                        (PRUint32) nsIDOMKeyEvent::DOM_VK_UP;
  }
  else if (aGranularity.LowerCaseEqualsLiteral("lineboundary")) {
    amount = eSelectLine;
    keycode = forward ? (PRUint32) nsIDOMKeyEvent::DOM_VK_END :
                        (PRUint32) nsIDOMKeyEvent::DOM_VK_HOME;
  }
  else if (aGranularity.LowerCaseEqualsLiteral("sentence") ||
           aGranularity.LowerCaseEqualsLiteral("sentenceboundary") ||
           aGranularity.LowerCaseEqualsLiteral("paragraph") ||
           aGranularity.LowerCaseEqualsLiteral("paragraphboundary") ||
           aGranularity.LowerCaseEqualsLiteral("documentboundary")) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  else {
    return NS_ERROR_INVALID_ARG;
  }

  
  
  
  nsresult rv = NS_OK;
  if (!extend) {
    nsINode* focusNode = GetFocusNode();
    
    NS_ENSURE_TRUE(focusNode, NS_ERROR_UNEXPECTED);
    PRInt32 focusOffset = GetFocusOffset();
    Collapse(focusNode, focusOffset);
  }

  
  
  nsIFrame *frame;
  PRInt32 offset;
  rv = GetPrimaryFrameForFocusNode(&frame, &offset, visual);
  if (NS_SUCCEEDED(rv) && frame) {
    nsBidiLevel baseLevel = nsBidiPresUtils::GetFrameBaseLevel(frame);

    if (baseLevel & 1) {
      if (!visual && keycode == nsIDOMKeyEvent::DOM_VK_RIGHT) {
        keycode = nsIDOMKeyEvent::DOM_VK_LEFT;
      }
      else if (!visual && keycode == nsIDOMKeyEvent::DOM_VK_LEFT) {
        keycode = nsIDOMKeyEvent::DOM_VK_RIGHT;
      }
      else if (visual && keycode == nsIDOMKeyEvent::DOM_VK_HOME) {
        keycode = nsIDOMKeyEvent::DOM_VK_END;
      }
      else if (visual && keycode == nsIDOMKeyEvent::DOM_VK_END) {
        keycode = nsIDOMKeyEvent::DOM_VK_HOME;
      }
    }
  }

  
  
  
  
  rv = mFrameSelection->MoveCaret(keycode, extend, amount, visual);

  if (aGranularity.LowerCaseEqualsLiteral("line") && NS_FAILED(rv)) {
    nsCOMPtr<nsISelectionController> shell =
      do_QueryInterface(mFrameSelection->GetShell());
    if (!shell)
      return NS_OK;
    shell->CompleteMove(forward, extend);
  }
  return NS_OK;
}




NS_IMETHODIMP
Selection::SelectionLanguageChange(bool aLangRTL)
{
  if (!mFrameSelection)
    return NS_ERROR_NOT_INITIALIZED; 
  nsresult result;
  nsIFrame *focusFrame = 0;

  result = GetPrimaryFrameForFocusNode(&focusFrame, nsnull, false);
  if (NS_FAILED(result)) {
    return result;
  }
  if (!focusFrame) {
    return NS_ERROR_FAILURE;
  }

  PRInt32 frameStart, frameEnd;
  focusFrame->GetOffsets(frameStart, frameEnd);
  nsRefPtr<nsPresContext> context;
  PRUint8 levelBefore, levelAfter;
  result = GetPresContext(getter_AddRefs(context));
  if (NS_FAILED(result) || !context)
    return result?result:NS_ERROR_FAILURE;

  PRUint8 level = NS_GET_EMBEDDING_LEVEL(focusFrame);
  PRInt32 focusOffset = GetFocusOffset();
  if ((focusOffset != frameStart) && (focusOffset != frameEnd))
    
    
    levelBefore = levelAfter = level;
  else {
    
    
    nsCOMPtr<nsIContent> focusContent = do_QueryInterface(GetFocusNode());
    











    nsPrevNextBidiLevels levels = mFrameSelection->
      GetPrevNextBidiLevels(focusContent, focusOffset, false);
      
    levelBefore = levels.mLevelBefore;
    levelAfter = levels.mLevelAfter;
  }

  if ((levelBefore & 1) == (levelAfter & 1)) {
    
    
    
    
    if ((level != levelBefore) && (level != levelAfter))
      level = NS_MIN(levelBefore, levelAfter);
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

NS_IMETHODIMP_(nsDirection)
Selection::GetSelectionDirection() {
  return mDirection;
}

NS_IMETHODIMP_(void)
Selection::SetSelectionDirection(nsDirection aDirection) {
  mDirection = aDirection;
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

  bool collapsed;
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
