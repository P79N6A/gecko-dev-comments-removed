



#ifndef nsFrameSelection_h___
#define nsFrameSelection_h___

#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "mozilla/dom/Selection.h"
#include "mozilla/TextRange.h"
#include "nsIFrame.h"
#include "nsIContent.h"
#include "nsISelectionController.h"
#include "nsISelectionListener.h"
#include "nsITableCellLayout.h"
#include "nsIDOMElement.h"
#include "WordMovementType.h"
#include "CaretAssociationHint.h"
#include "nsBidiPresUtils.h"

class nsRange;



#define NS_FRAME_SELECTION_IID      \
{ 0x3c6ae2d0, 0x4cf1, 0x44a1, \
  { 0x9e, 0x9d, 0x24, 0x11, 0x86, 0x7f, 0x19, 0xc6 } }

#define BIDI_LEVEL_UNDEFINED 0x80





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
  int32_t mStart;
  int32_t mEnd;
  SelectionType mType;
  mozilla::TextRangeStyle mTextRangeStyle;
  SelectionDetails *mNext;
};

class nsIPresShell;
class nsIScrollableFrame;





struct MOZ_STACK_CLASS nsPeekOffsetStruct
{
  nsPeekOffsetStruct(nsSelectionAmount aAmount,
                     nsDirection aDirection,
                     int32_t aStartOffset,
                     nsPoint aDesiredPos,
                     bool aJumpLines,
                     bool aScrollViewStop,
                     bool aIsKeyboardSelect,
                     bool aVisual,
                     bool aExtend,
                     mozilla::EWordMovementType aWordMovementType = mozilla::eDefaultBehavior);

  
  
  

  
  

  
  nsSelectionAmount mAmount;

  
  
  
  
  
  
  
  
  
  
  nsDirection mDirection;

  
  
  int32_t mStartOffset;
  
  
  
  
  nsPoint mDesiredPos;

  
  
  
  
  mozilla::EWordMovementType mWordMovementType;

  
  
  bool mJumpLines;

  
  
  bool mScrollViewStop;

  
  
  bool mIsKeyboardSelect;

  
  
  bool mVisual;

  
  bool mExtend;

  

  
  nsCOMPtr<nsIContent> mResultContent;

  
  
  nsIFrame *mResultFrame;

  
  int32_t mContentOffset;

  
  
  
  
  
  mozilla::CaretAssociationHint mAttach;
};

struct nsPrevNextBidiLevels
{
  void SetData(nsIFrame* aFrameBefore,
               nsIFrame* aFrameAfter,
               nsBidiLevel aLevelBefore,
               nsBidiLevel aLevelAfter)
  {
    mFrameBefore = aFrameBefore;
    mFrameAfter = aFrameAfter;
    mLevelBefore = aLevelBefore;
    mLevelAfter = aLevelAfter;
  }
  nsIFrame* mFrameBefore;
  nsIFrame* mFrameAfter;
  nsBidiLevel mLevelBefore;
  nsBidiLevel mLevelAfter;
};

namespace mozilla {
namespace dom {
class Selection;
}
}
class nsIScrollableFrame;







class nsFrameSelection final {
public:
  typedef mozilla::CaretAssociationHint CaretAssociateHint;

  
  
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(nsFrameSelection)
  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsFrameSelection)

  




  void Init(nsIPresShell *aShell, nsIContent *aLimiter);

  












  
  nsresult HandleClick(nsIContent *aNewFocus,
                       uint32_t aContentOffset,
                       uint32_t aContentEndOffset,
                       bool aContinueSelection,
                       bool aMultipleSelection,
                       CaretAssociateHint aHint);

  




  
  void HandleDrag(nsIFrame *aFrame, nsPoint aPoint);

  











  
  nsresult HandleTableSelection(nsINode* aParentContent,
                                int32_t aContentOffset,
                                int32_t aTarget,
                                mozilla::WidgetMouseEvent* aMouseEvent);

  




  virtual nsresult SelectCellElement(nsIContent *aCell);

  








  virtual nsresult AddCellsToSelection(nsIContent *aTable,
                                       int32_t aStartRowIndex,
                                       int32_t aStartColumnIndex,
                                       int32_t aEndRowIndex,
                                       int32_t aEndColumnIndex);

  








  virtual nsresult RemoveCellsFromSelection(nsIContent *aTable,
                                            int32_t aStartRowIndex,
                                            int32_t aStartColumnIndex,
                                            int32_t aEndRowIndex,
                                            int32_t aEndColumnIndex);

  








  virtual nsresult RestrictCellsToSelection(nsIContent *aTable,
                                            int32_t aStartRowIndex,
                                            int32_t aStartColumnIndex,
                                            int32_t aEndRowIndex,
                                            int32_t aEndColumnIndex);

  









  
  nsresult StartAutoScrollTimer(nsIFrame *aFrame,
                                nsPoint aPoint,
                                uint32_t aDelay);

  

  void StopAutoScrollTimer();

  







  SelectionDetails* LookUpSelection(nsIContent *aContent,
                                    int32_t aContentOffset,
                                    int32_t aContentLength,
                                    bool aSlowCheck) const;

  



  
  void SetDragState(bool aState);

  



  bool GetDragState() const { return mDragState; }

  


  bool GetTableCellSelection() const { return mSelectingTableCellMode != 0; }
  void ClearTableCellSelection() { mSelectingTableCellMode = 0; }

  



  mozilla::dom::Selection* GetSelection(SelectionType aType) const;

  












  
  nsresult ScrollSelectionIntoView(SelectionType aType,
                                   SelectionRegion aRegion,
                                   int16_t aFlags) const;

  



  nsresult RepaintSelection(SelectionType aType) const;

  





  virtual nsIFrame* GetFrameForNodeOffset(nsIContent*        aNode,
                                          int32_t            aOffset,
                                          CaretAssociateHint aHint,
                                          int32_t*           aReturnOffset) const;

  











  
  void CommonPageMove(bool aForward,
                      bool aExtend,
                      nsIScrollableFrame* aScrollableFrame);

  void SetHint(CaretAssociateHint aHintRight) { mHint = aHintRight; }
  CaretAssociateHint GetHint() const { return mHint; }

  



  virtual void SetCaretBidiLevel(nsBidiLevel aLevel);
  


  virtual nsBidiLevel GetCaretBidiLevel() const;
  


  virtual void UndefineCaretBidiLevel();

  






  
  nsresult PhysicalMove(int16_t aDirection, int16_t aAmount, bool aExtend);

  




  
  nsresult CharacterMove(bool aForward, bool aExtend);

  


  
  nsresult CharacterExtendForDelete();

  


  
  nsresult CharacterExtendForBackspace();

  




  
  nsresult WordMove(bool aForward, bool aExtend);

  



  
  nsresult WordExtendForDelete(bool aForward);
  
  




  
  nsresult LineMove(bool aForward, bool aExtend);

  




  
  nsresult IntraLineMove(bool aForward, bool aExtend); 

  


  
  nsresult SelectAll();

  

  void SetDisplaySelection(int16_t aState) { mDisplaySelection = aState; }
  int16_t GetDisplaySelection() const { return mDisplaySelection; }

  





  void SetDelayedCaretData(mozilla::WidgetMouseEvent* aMouseEvent);

  





  bool HasDelayedCaretData() { return mDelayedMouseEventValid; }
  bool IsShiftDownInDelayedCaretData()
  {
    NS_ASSERTION(mDelayedMouseEventValid, "No valid delayed caret data");
    return mDelayedMouseEventIsShift;
  }
  uint32_t GetClickCountInDelayedCaretData()
  {
    NS_ASSERTION(mDelayedMouseEventValid, "No valid delayed caret data");
    return mDelayedMouseEventClickCount;
  }

  bool MouseDownRecorded()
  {
    return !GetDragState() &&
           HasDelayedCaretData() &&
           GetClickCountInDelayedCaretData() < 2;
  }

  




  nsIContent* GetLimiter() const { return mLimiter; }

  nsIContent* GetAncestorLimiter() const { return mAncestorLimiter; }
  
  void SetAncestorLimiter(nsIContent *aLimiter);

  



  void SetMouseDoubleDown(bool aDoubleDown) { mMouseDoubleDownState = aDoubleDown; }
  
  


  bool GetMouseDoubleDown() const { return mMouseDoubleDownState; }

  















  virtual nsPrevNextBidiLevels GetPrevNextBidiLevels(nsIContent *aNode,
                                                     uint32_t aContentOffset,
                                                     bool aJumpLines) const;

  








  nsresult GetFrameFromLevel(nsIFrame *aFrameIn,
                             nsDirection aDirection,
                             nsBidiLevel aBidiLevel,
                             nsIFrame **aFrameOut) const;

  







  nsresult MaintainSelection(nsSelectionAmount aAmount = eSelectNoAmount);

  nsresult ConstrainFrameAndPointToAnchorSubtree(nsIFrame *aFrame,
                                                 nsPoint& aPoint,
                                                 nsIFrame **aRetFrame,
                                                 nsPoint& aRetPoint);

  nsFrameSelection();

  void StartBatchChanges();
  void EndBatchChanges();
  
  nsresult DeleteFromDocument();

  nsIPresShell *GetShell()const  { return mShell; }

  void DisconnectFromPresShell();
  nsresult ClearNormalSelection();

private:
  ~nsFrameSelection();

  nsresult TakeFocus(nsIContent *aNewFocus,
                     uint32_t aContentOffset,
                     uint32_t aContentEndOffset,
                     CaretAssociateHint aHint,
                     bool aContinueSelection,
                     bool aMultipleSelection);

  void BidiLevelFromMove(nsIPresShell* aPresShell,
                         nsIContent *aNode,
                         uint32_t aContentOffset,
                         nsSelectionAmount aAmount,
                         CaretAssociateHint aHint);
  void BidiLevelFromClick(nsIContent *aNewFocus, uint32_t aContentOffset);
  nsPrevNextBidiLevels GetPrevNextBidiLevels(nsIContent *aNode,
                                             uint32_t aContentOffset,
                                             CaretAssociateHint aHint,
                                             bool aJumpLines) const;

  bool AdjustForMaintainedSelection(nsIContent *aContent, int32_t aOffset);


  void    PostReason(int16_t aReason) { mSelectionChangeReason = aReason; }
  int16_t PopReason()
  {
    int16_t retval = mSelectionChangeReason;
    mSelectionChangeReason = nsISelectionListener::NO_REASON;
    return retval;
  }
  bool IsUserSelectionReason() const
  {
    return (mSelectionChangeReason &
            (nsISelectionListener::DRAG_REASON |
             nsISelectionListener::MOUSEDOWN_REASON |
             nsISelectionListener::MOUSEUP_REASON |
             nsISelectionListener::KEYPRESS_REASON)) !=
           nsISelectionListener::NO_REASON;
  }

  friend class mozilla::dom::Selection;
  friend struct mozilla::AutoPrepareFocusRange;
#ifdef DEBUG
  void printSelection();       
#endif 

  void ResizeBuffer(uint32_t aNewBufSize);


  
  
  enum CaretMovementStyle {
    eLogical,
    eVisual,
    eUsePrefStyle
  };
  nsresult     MoveCaret(nsDirection aDirection, bool aContinueSelection,
                         nsSelectionAmount aAmount,
                         CaretMovementStyle aMovementStyle);

  nsresult     FetchDesiredPos(nsPoint &aDesiredPos); 
  void         InvalidateDesiredPos(); 
  void         SetDesiredPos(nsPoint aPos); 

  uint32_t     GetBatching() const {return mBatching; }
  bool         GetNotifyFrames() const { return mNotifyFrames; }
  void         SetDirty(bool aDirty=true){if (mBatching) mChangesDuringBatching = aDirty;}

  
  
  nsresult     NotifySelectionListeners(SelectionType aType);     

  nsRefPtr<mozilla::dom::Selection> mDomSelections[nsISelectionController::NUM_SELECTIONTYPES];

  
  nsITableCellLayout* GetCellLayout(nsIContent *aCellContent) const;

  nsresult SelectBlockOfCells(nsIContent *aStartNode, nsIContent *aEndNode);
  nsresult SelectRowOrColumn(nsIContent *aCellContent, uint32_t aTarget);
  nsresult UnselectCells(nsIContent *aTable,
                         int32_t aStartRowIndex, int32_t aStartColumnIndex,
                         int32_t aEndRowIndex, int32_t aEndColumnIndex,
                         bool aRemoveOutsideOfCellRange);

  nsresult GetCellIndexes(nsIContent *aCell, int32_t &aRowIndex, int32_t &aColIndex);

  
  
  
  nsRange* GetFirstCellRange();
  
  
  
  nsRange* GetNextCellRange();
  nsIContent* GetFirstCellNodeInRange(nsRange *aRange) const;
  
  nsIContent* IsInSameTable(nsIContent *aContent1, nsIContent *aContent2) const;
  
  nsIContent* GetParentTable(nsIContent *aCellNode) const;
  nsresult CreateAndAddRange(nsINode *aParentNode, int32_t aOffset);

  nsCOMPtr<nsINode> mCellParent; 
  nsCOMPtr<nsIContent> mStartSelectedCell;
  nsCOMPtr<nsIContent> mEndSelectedCell;
  nsCOMPtr<nsIContent> mAppendStartSelectedCell;
  nsCOMPtr<nsIContent> mUnselectCellOnMouseUp;
  int32_t  mSelectingTableCellMode;
  int32_t  mSelectedCellIndex;

  
  nsRefPtr<nsRange> mMaintainRange;
  nsSelectionAmount mMaintainedAmount;

  
  int32_t mBatching;
    
  
  nsCOMPtr<nsIContent> mLimiter;
  
  nsCOMPtr<nsIContent> mAncestorLimiter;

  nsIPresShell *mShell;

  int16_t mSelectionChangeReason; 
  int16_t mDisplaySelection; 

  CaretAssociateHint mHint;   
  nsBidiLevel mCaretBidiLevel;
  nsBidiLevel mKbdBidiLevel;

  nsPoint mDesiredPos;
  uint32_t mDelayedMouseEventClickCount;
  bool mDelayedMouseEventIsShift;
  bool mDelayedMouseEventValid;

  bool mChangesDuringBatching;
  bool mNotifyFrames;
  bool mDragSelectingCells;
  bool mDragState;   
  bool mMouseDoubleDownState; 
  bool mDesiredPosSet;

  int8_t mCaretMovementStyle;
};

#endif 
