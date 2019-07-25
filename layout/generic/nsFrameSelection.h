




































#ifndef nsFrameSelection_h___
#define nsFrameSelection_h___

#include "nsIFrame.h"
#include "nsIContent.h"
#include "nsISelectionController.h"
#include "nsITableLayout.h"
#include "nsITableCellLayout.h"
#include "nsIDOMElement.h"
#include "nsGUIEvent.h"
#include "nsIRange.h"



#define NS_FRAME_SELECTION_IID      \
{ 0x3c6ae2d0, 0x4cf1, 0x44a1, \
  { 0x9e, 0x9d, 0x24, 0x11, 0x86, 0x7f, 0x19, 0xc6 } }

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
class nsIScrollableFrame;

enum EWordMovementType { eStartWord, eEndWord, eDefaultBehavior };





struct NS_STACK_CLASS nsPeekOffsetStruct
{
  void SetData(nsSelectionAmount aAmount,
               nsDirection aDirection,
               PRInt32 aStartOffset,
               nscoord aDesiredX,
               bool aJumpLines,
               bool aScrollViewStop,
               bool aIsKeyboardSelect,
               bool aVisual,
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

  
  
  bool mJumpLines;

  
  
  bool mScrollViewStop;

  
  
  bool mIsKeyboardSelect;

  
  
  bool mVisual;

  

  
  nsCOMPtr<nsIContent> mResultContent;

  
  
  nsIFrame *mResultFrame;

  
  PRInt32 mContentOffset;

  
  
  
  
  
  bool mAttachForward;
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
class nsIScrollableFrame;







class nsFrameSelection : public nsISupports {
public:
  enum HINT { HINTLEFT = 0, HINTRIGHT = 1};  
  
  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsFrameSelection)

  




  void Init(nsIPresShell *aShell, nsIContent *aLimiter);

  












  
  nsresult HandleClick(nsIContent *aNewFocus,
                       PRUint32 aContentOffset,
                       PRUint32 aContentEndOffset,
                       bool aContinueSelection,
                       bool aMultipleSelection,
                       bool aHint);

  




  
  void HandleDrag(nsIFrame *aFrame, nsPoint aPoint);

  











  
  nsresult HandleTableSelection(nsINode *aParentContent,
                                PRInt32 aContentOffset,
                                PRInt32 aTarget,
                                nsMouseEvent *aMouseEvent);

  




  virtual nsresult SelectCellElement(nsIContent *aCell);

  








  virtual nsresult AddCellsToSelection(nsIContent *aTable,
                                       PRInt32 aStartRowIndex,
                                       PRInt32 aStartColumnIndex,
                                       PRInt32 aEndRowIndex,
                                       PRInt32 aEndColumnIndex);

  








  virtual nsresult RemoveCellsFromSelection(nsIContent *aTable,
                                            PRInt32 aStartRowIndex,
                                            PRInt32 aStartColumnIndex,
                                            PRInt32 aEndRowIndex,
                                            PRInt32 aEndColumnIndex);

  








  virtual nsresult RestrictCellsToSelection(nsIContent *aTable,
                                            PRInt32 aStartRowIndex,
                                            PRInt32 aStartColumnIndex,
                                            PRInt32 aEndRowIndex,
                                            PRInt32 aEndColumnIndex);

  









  
  nsresult StartAutoScrollTimer(nsIFrame *aFrame,
                                nsPoint aPoint,
                                PRUint32 aDelay);

  

  void StopAutoScrollTimer();

  







  SelectionDetails* LookUpSelection(nsIContent *aContent,
                                    PRInt32 aContentOffset,
                                    PRInt32 aContentLength,
                                    bool aSlowCheck) const;

  



  
  void SetMouseDownState(bool aState);

  



  bool GetMouseDownState() const { return mMouseDownState; }

  


  bool GetTableCellSelection() const { return mSelectingTableCellMode != 0; }
  void ClearTableCellSelection() { mSelectingTableCellMode = 0; }

  



  nsISelection* GetSelection(SelectionType aType) const;

  












  
  nsresult ScrollSelectionIntoView(SelectionType aType,
                                   SelectionRegion aRegion,
                                   PRInt16 aFlags) const;

  



  nsresult RepaintSelection(SelectionType aType) const;

  





  virtual nsIFrame* GetFrameForNodeOffset(nsIContent *aNode,
                                          PRInt32     aOffset,
                                          HINT        aHint,
                                          PRInt32    *aReturnOffset) const;

  











  
  void CommonPageMove(bool aForward,
                      bool aExtend,
                      nsIScrollableFrame* aScrollableFrame);

  void SetHint(HINT aHintRight) { mHint = aHintRight; }
  HINT GetHint() const { return mHint; }
  
#ifdef IBMBIDI
  



  virtual void SetCaretBidiLevel (PRUint8 aLevel);
  


  virtual PRUint8 GetCaretBidiLevel() const;
  


  virtual void UndefineCaretBidiLevel();
#endif

  




  
  nsresult CharacterMove(bool aForward, bool aExtend);

  


  
  nsresult CharacterExtendForDelete();

  


  
  nsresult CharacterExtendForBackspace();

  




  
  nsresult WordMove(bool aForward, bool aExtend);

  



  
  nsresult WordExtendForDelete(bool aForward);
  
  




  
  nsresult LineMove(bool aForward, bool aExtend);

  




  
  nsresult IntraLineMove(bool aForward, bool aExtend); 

  


  
  nsresult SelectAll();

  

  void SetDisplaySelection(PRInt16 aState) { mDisplaySelection = aState; }
  PRInt16 GetDisplaySelection() const { return mDisplaySelection; }

  





  void SetDelayedCaretData(nsMouseEvent *aMouseEvent);

  





  nsMouseEvent* GetDelayedCaretData();

  




  nsIContent* GetLimiter() const { return mLimiter; }

  nsIContent* GetAncestorLimiter() const { return mAncestorLimiter; }
  
  void SetAncestorLimiter(nsIContent *aLimiter);

  



  void SetMouseDoubleDown(bool aDoubleDown) { mMouseDoubleDownState = aDoubleDown; }
  
  


  bool GetMouseDoubleDown() const { return mMouseDoubleDownState; }

  















  virtual nsPrevNextBidiLevels GetPrevNextBidiLevels(nsIContent *aNode,
                                                     PRUint32 aContentOffset,
                                                     bool aJumpLines) const;

  








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
                     bool aContinueSelection,
                     bool aMultipleSelection);

  void BidiLevelFromMove(nsIPresShell* aPresShell,
                         nsIContent *aNode,
                         PRUint32 aContentOffset,
                         PRUint32 aKeycode,
                         HINT aHint);
  void BidiLevelFromClick(nsIContent *aNewFocus, PRUint32 aContentOffset);
  nsPrevNextBidiLevels GetPrevNextBidiLevels(nsIContent *aNode,
                                             PRUint32 aContentOffset,
                                             HINT aHint,
                                             bool aJumpLines) const;

  bool AdjustForMaintainedSelection(nsIContent *aContent, PRInt32 aOffset);


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

  nsresult     MoveCaret(PRUint32 aKeycode, bool aContinueSelection,
                         nsSelectionAmount aAmount);
  nsresult     MoveCaret(PRUint32 aKeycode, bool aContinueSelection,
                         nsSelectionAmount aAmount,
                         bool aVisualMovement);

  nsresult     FetchDesiredX(nscoord &aDesiredX); 
  void         InvalidateDesiredX(); 
  void         SetDesiredX(nscoord aX); 

  nsresult     ConstrainFrameAndPointToAnchorSubtree(nsIFrame *aFrame, nsPoint& aPoint, nsIFrame **aRetFrame, nsPoint& aRetPoint);

  PRUint32     GetBatching() const {return mBatching; }
  bool         GetNotifyFrames() const { return mNotifyFrames; }
  void         SetDirty(bool aDirty=true){if (mBatching) mChangesDuringBatching = aDirty;}

  
  
  nsresult     NotifySelectionListeners(SelectionType aType);     

  nsRefPtr<nsTypedSelection> mDomSelections[nsISelectionController::NUM_SELECTIONTYPES];

  
  
  nsITableLayout* GetTableLayout(nsIContent *aTableContent) const;
  nsITableCellLayout* GetCellLayout(nsIContent *aCellContent) const;

  nsresult SelectBlockOfCells(nsIContent *aStartNode, nsIContent *aEndNode);
  nsresult SelectRowOrColumn(nsIContent *aCellContent, PRUint32 aTarget);
  nsresult UnselectCells(nsIContent *aTable,
                         PRInt32 aStartRowIndex, PRInt32 aStartColumnIndex,
                         PRInt32 aEndRowIndex, PRInt32 aEndColumnIndex,
                         bool aRemoveOutsideOfCellRange);

  nsresult GetCellIndexes(nsIContent *aCell, PRInt32 &aRowIndex, PRInt32 &aColIndex);

  
  
  
  nsIRange* GetFirstCellRange();
  
  
  
  nsIRange* GetNextCellRange();
  nsIContent* GetFirstCellNodeInRange(nsIRange *aRange) const;
  
  nsIContent* IsInSameTable(nsIContent *aContent1, nsIContent *aContent2) const;
  
  nsIContent* GetParentTable(nsIContent *aCellNode) const;
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

  nsMouseEvent mDelayedMouseEvent;

  bool mDelayedMouseEventValid;

  bool mChangesDuringBatching;
  bool mNotifyFrames;
  bool mDragSelectingCells;
  bool mMouseDownState;   
  bool mMouseDoubleDownState; 
  bool mDesiredXSet;

  PRInt8 mCaretMovementStyle;
};

#endif 
