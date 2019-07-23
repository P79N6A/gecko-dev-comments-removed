




































#ifndef nsFrameSelection_h___
#define nsFrameSelection_h___
 
#include "nsIFrame.h"
#include "nsIContent.h"
#include "nsISelectionController.h"
#include "nsIScrollableViewProvider.h"
#include "nsITableLayout.h"
#include "nsITableCellLayout.h"
#include "nsIDOMElement.h"
#include "nsGUIEvent.h"
#include "nsIRange.h"



#define NS_FRAME_SELECTION_IID      \
{ 0xea74459, 0xe3f9, 0x48b0, \
  { 0x8a, 0xa4, 0x5d, 0xfe, 0xf5, 0x3b, 0xf1, 0xf7 } }

#ifdef IBMBIDI 
#define BIDI_LEVEL_UNDEFINED 0x80
#endif





struct SelectionDetails
{
#ifdef NS_BUILD_REFCNT_LOGGING
  SelectionDetails() {
    MOZ_COUNT_CTOR(SelectionDetails);
  }
  ~SelectionDetails() {
    MOZ_COUNT_DTOR(SelectionDetails);
  }
#endif
  PRInt32 mStart;
  PRInt32 mEnd;
  SelectionType mType;
  nsTextRangeStyle mTextRangeStyle;
  SelectionDetails *mNext;
};

class nsIPresShell;

enum EWordMovementType { eStartWord, eEndWord, eDefaultBehavior };





struct NS_STACK_CLASS nsPeekOffsetStruct
{
  void SetData(nsSelectionAmount aAmount,
               nsDirection aDirection,
               PRInt32 aStartOffset,
               nscoord aDesiredX,
               PRBool aJumpLines,
               PRBool aScrollViewStop,
               PRBool aIsKeyboardSelect,
               PRBool aVisual,
               EWordMovementType aWordMovementType = eDefaultBehavior)

  {
    mAmount = aAmount;
    mDirection = aDirection;
    mStartOffset = aStartOffset;
    mDesiredX = aDesiredX;
    mJumpLines = aJumpLines;
    mScrollViewStop = aScrollViewStop;
    mIsKeyboardSelect = aIsKeyboardSelect;
    mVisual = aVisual;
    mWordMovementType = aWordMovementType;
  }

  
  
  

  
  

  
  nsSelectionAmount mAmount;

  
  
  
  
  
  
  
  
  
  
  nsDirection mDirection;

  
  
  PRInt32 mStartOffset;
  
  
  
  nscoord mDesiredX;

  
  
  
  
  EWordMovementType mWordMovementType;

  
  
  PRPackedBool mJumpLines;

  
  
  PRPackedBool mScrollViewStop;

  
  
  PRPackedBool mIsKeyboardSelect;

  
  
  PRPackedBool mVisual;

  

  
  nsCOMPtr<nsIContent> mResultContent;

  
  
  nsIFrame *mResultFrame;

  
  PRInt32 mContentOffset;

  
  
  
  
  
  PRBool mAttachForward;
};

struct nsPrevNextBidiLevels
{
  void SetData(nsIFrame* aFrameBefore,
               nsIFrame* aFrameAfter,
               PRUint8 aLevelBefore,
               PRUint8 aLevelAfter)
  {
    mFrameBefore = aFrameBefore;
    mFrameAfter = aFrameAfter;
    mLevelBefore = aLevelBefore;
    mLevelAfter = aLevelAfter;
  }
  nsIFrame* mFrameBefore;
  nsIFrame* mFrameAfter;
  PRUint8 mLevelBefore;
  PRUint8 mLevelAfter;
};

class nsTypedSelection;
class nsIScrollableView;







class nsFrameSelection : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_FRAME_SELECTION_IID)
  enum HINT { HINTLEFT = 0, HINTRIGHT = 1};  
  
  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsFrameSelection)

  




  void Init(nsIPresShell *aShell, nsIContent *aLimiter);

  



  void SetScrollableViewProvider(nsIScrollableViewProvider* aProvider)
  {
    mScrollableViewProvider = aProvider;
  }

  


  nsIScrollableView* GetScrollableView() const
  {
    return mScrollableViewProvider
      ? mScrollableViewProvider->GetScrollableView()
      : nsnull;
  }

  












  
  nsresult HandleClick(nsIContent *aNewFocus,
                       PRUint32 aContentOffset,
                       PRUint32 aContentEndOffset,
                       PRBool aContinueSelection,
                       PRBool aMultipleSelection,
                       PRBool aHint);

  




  
  void HandleDrag(nsIFrame *aFrame, nsPoint aPoint);

  











  
  nsresult HandleTableSelection(nsINode *aParentContent,
                                PRInt32 aContentOffset,
                                PRInt32 aTarget,
                                nsMouseEvent *aMouseEvent);

  







  
  nsresult StartAutoScrollTimer(nsIView *aView,
                                nsPoint aPoint,
                                PRUint32 aDelay);

  

  void StopAutoScrollTimer();

  







  SelectionDetails* LookUpSelection(nsIContent *aContent,
                                    PRInt32 aContentOffset,
                                    PRInt32 aContentLength,
                                    PRBool aSlowCheck) const;

  



  
  void SetMouseDownState(PRBool aState);

  



  PRBool GetMouseDownState() const { return mMouseDownState; }

  


  PRBool GetTableCellSelection() const { return mSelectingTableCellMode != 0; }
  void ClearTableCellSelection() { mSelectingTableCellMode = 0; }

  



  nsISelection* GetSelection(SelectionType aType) const;

  








  
  nsresult ScrollSelectionIntoView(SelectionType aType,
                                   SelectionRegion aRegion,
                                   PRBool aIsSynchronous) const;

  



  nsresult RepaintSelection(SelectionType aType) const;

  





  virtual nsIFrame* GetFrameForNodeOffset(nsIContent *aNode,
                                          PRInt32     aOffset,
                                          HINT        aHint,
                                          PRInt32    *aReturnOffset) const;

  











  
  void CommonPageMove(PRBool aForward,
                      PRBool aExtend,
                      nsIScrollableView *aScrollableView);

  void SetHint(HINT aHintRight) { mHint = aHintRight; }
  HINT GetHint() const { return mHint; }
  
#ifdef IBMBIDI
  



  virtual void SetCaretBidiLevel (PRUint8 aLevel);
  


  virtual PRUint8 GetCaretBidiLevel() const;
  


  virtual void UndefineCaretBidiLevel();
#endif

  




  
  nsresult CharacterMove(PRBool aForward, PRBool aExtend);

  


  
  nsresult CharacterExtendForDelete();

  




  
  nsresult WordMove(PRBool aForward, PRBool aExtend);

  



  
  nsresult WordExtendForDelete(PRBool aForward);
  
  




  
  nsresult LineMove(PRBool aForward, PRBool aExtend);

  




  
  nsresult IntraLineMove(PRBool aForward, PRBool aExtend); 

  


  
  nsresult SelectAll();

  

  void SetDisplaySelection(PRInt16 aState) { mDisplaySelection = aState; }
  PRInt16 GetDisplaySelection() const { return mDisplaySelection; }

  





  void SetDelayedCaretData(nsMouseEvent *aMouseEvent);

  





  nsMouseEvent* GetDelayedCaretData();

  




  nsIContent* GetLimiter() const { return mLimiter; }

  nsIContent* GetAncestorLimiter() const { return mAncestorLimiter; }
  
  void SetAncestorLimiter(nsIContent *aLimiter);

  



  void SetMouseDoubleDown(PRBool aDoubleDown) { mMouseDoubleDownState = aDoubleDown; }
  
  


  PRBool GetMouseDoubleDown() const { return mMouseDoubleDownState; }

  















  virtual nsPrevNextBidiLevels GetPrevNextBidiLevels(nsIContent *aNode,
                                                     PRUint32 aContentOffset,
                                                     PRBool aJumpLines) const;

  








  nsresult GetFrameFromLevel(nsIFrame *aFrameIn,
                             nsDirection aDirection,
                             PRUint8 aBidiLevel,
                             nsIFrame **aFrameOut) const;

  







  nsresult MaintainSelection(nsSelectionAmount aAmount = eSelectNoAmount);


  nsFrameSelection();

  void StartBatchChanges();
  void EndBatchChanges();
  
  nsresult DeleteFromDocument();

  nsIPresShell *GetShell()const  { return mShell; }

  void DisconnectFromPresShell() { StopAutoScrollTimer(); mShell = nsnull; }
private:
  nsresult TakeFocus(nsIContent *aNewFocus,
                     PRUint32 aContentOffset,
                     PRUint32 aContentEndOffset,
                     HINT aHint,
                     PRBool aContinueSelection,
                     PRBool aMultipleSelection);

  void BidiLevelFromMove(nsIPresShell* aPresShell,
                         nsIContent *aNode,
                         PRUint32 aContentOffset,
                         PRUint32 aKeycode,
                         HINT aHint);
  void BidiLevelFromClick(nsIContent *aNewFocus, PRUint32 aContentOffset);
  nsPrevNextBidiLevels GetPrevNextBidiLevels(nsIContent *aNode,
                                             PRUint32 aContentOffset,
                                             HINT aHint,
                                             PRBool aJumpLines) const;

  PRBool AdjustForMaintainedSelection(nsIContent *aContent, PRInt32 aOffset);


  void    PostReason(PRInt16 aReason) { mSelectionChangeReason = aReason; }
  PRInt16 PopReason()
  {
    PRInt16 retval = mSelectionChangeReason;
    mSelectionChangeReason = 0;
    return retval;
  }

  friend class nsTypedSelection; 
#ifdef DEBUG
  void printSelection();       
#endif 

  void ResizeBuffer(PRUint32 aNewBufSize);

  nsresult     MoveCaret(PRUint32 aKeycode, PRBool aContinueSelection, nsSelectionAmount aAmount);

  nsresult     FetchDesiredX(nscoord &aDesiredX); 
  void         InvalidateDesiredX(); 
  void         SetDesiredX(nscoord aX); 

  nsresult     GetRootForContentSubtree(nsIContent *aContent, nsIContent **aParent);
  nsresult     ConstrainFrameAndPointToAnchorSubtree(nsIFrame *aFrame, nsPoint& aPoint, nsIFrame **aRetFrame, nsPoint& aRetPoint);

  PRUint32     GetBatching() const {return mBatching; }
  PRBool       GetNotifyFrames() const { return mNotifyFrames; }
  void         SetDirty(PRBool aDirty=PR_TRUE){if (mBatching) mChangesDuringBatching = aDirty;}

  
  
  nsresult     NotifySelectionListeners(SelectionType aType);     

  nsRefPtr<nsTypedSelection> mDomSelections[nsISelectionController::NUM_SELECTIONTYPES];

  
  
  nsITableLayout* GetTableLayout(nsIContent *aTableContent) const;
  nsITableCellLayout* GetCellLayout(nsIContent *aCellContent) const;

  nsresult SelectBlockOfCells(nsIContent *aStartNode, nsIContent *aEndNode);
  nsresult SelectRowOrColumn(nsIContent *aCellContent, PRUint32 aTarget);
  nsresult GetCellIndexes(nsIContent *aCell, PRInt32 &aRowIndex, PRInt32 &aColIndex);

  
  
  
  nsIRange* GetFirstCellRange();
  
  
  
  nsIRange* GetNextCellRange();
  nsIContent* GetFirstCellNodeInRange(nsIRange *aRange) const;
  
  nsIContent* IsInSameTable(nsIContent *aContent1, nsIContent *aContent2) const;
  
  nsIContent* GetParentTable(nsIContent *aCellNode) const;
  nsresult SelectCellElement(nsIContent* aCellElement);
  nsresult CreateAndAddRange(nsINode *aParentNode, PRInt32 aOffset);
  nsresult ClearNormalSelection();

  nsCOMPtr<nsINode> mCellParent; 
  nsCOMPtr<nsIContent> mStartSelectedCell;
  nsCOMPtr<nsIContent> mEndSelectedCell;
  nsCOMPtr<nsIContent> mAppendStartSelectedCell;
  nsCOMPtr<nsIContent> mUnselectCellOnMouseUp;
  PRInt32  mSelectingTableCellMode;
  PRInt32  mSelectedCellIndex;

  
  nsCOMPtr<nsIRange> mMaintainRange;
  nsSelectionAmount mMaintainedAmount;

  
  PRInt32 mBatching;
    
  nsIContent *mLimiter;     
  nsIContent *mAncestorLimiter; 
                                
  nsIPresShell *mShell;

  PRInt16 mSelectionChangeReason; 
  PRInt16 mDisplaySelection; 

  HINT  mHint;   
#ifdef IBMBIDI
  PRUint8 mCaretBidiLevel;
#endif

  PRInt32 mDesiredX;
  nsIScrollableViewProvider* mScrollableViewProvider;

  nsMouseEvent mDelayedMouseEvent;

  PRPackedBool mDelayedMouseEventValid;

  PRPackedBool mChangesDuringBatching;
  PRPackedBool mNotifyFrames;
  PRPackedBool mDragSelectingCells;
  PRPackedBool mMouseDownState;   
  PRPackedBool mMouseDoubleDownState; 
  PRPackedBool mDesiredXSet;

  PRInt8 mCaretMovementStyle;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsFrameSelection, NS_FRAME_SELECTION_IID)

#endif 
