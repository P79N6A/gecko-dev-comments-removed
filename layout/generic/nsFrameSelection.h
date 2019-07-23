




































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



#define NS_FRAME_SELECTION_IID      \
{ 0x6c2c1a4c, 0x47ec, 0x42be, \
  { 0xa7, 0x90, 0x00, 0x41, 0x7b, 0xf4, 0xc2, 0x41 } }

#ifdef IBMBIDI 
#define BIDI_LEVEL_UNDEFINED 0x80
#endif





struct SelectionDetails
{
  PRInt32 mStart;
  PRInt32 mEnd;
  SelectionType mType;
  SelectionDetails *mNext;
};

class nsIPresShell;

enum EWordMovementType { eStartWord, eEndWord, eDefaultBehavior };





struct nsPeekOffsetStruct
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

  
  
  PRBool mJumpLines;

  
  
  PRBool mScrollViewStop;

  
  
  PRBool mIsKeyboardSelect;

  
  
  PRBool mVisual;

  
  
  
  
  EWordMovementType mWordMovementType;

  

  
  nsCOMPtr<nsIContent> mResultContent;

  
  PRInt32 mContentOffset;

  
  
  nsIFrame *mResultFrame;

  
  
  
  
  
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
  
  
  NS_DECL_ISUPPORTS

  




  void Init(nsIPresShell *aShell, nsIContent *aLimiter);

  



  void SetScrollableViewProvider(nsIScrollableViewProvider* aProvider)
  {
    mScrollableViewProvider = aProvider;
  }

  


  nsIScrollableView* GetScrollableView()
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

  











  nsresult HandleTableSelection(nsIContent *aParentContent,
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
                                    PRBool aSlowCheck);

  



  void SetMouseDownState(PRBool aState);

  



  PRBool GetMouseDownState() { return mMouseDownState; }

  


  PRBool GetTableCellSelection() { return mSelectingTableCellMode != 0; }
  void ClearTableCellSelection(){ mSelectingTableCellMode = 0; }

  



  nsISelection* GetSelection(SelectionType aType);

  








  nsresult ScrollSelectionIntoView(SelectionType aType,
                                   SelectionRegion aRegion,
                                   PRBool aIsSynchronous);

  



  nsresult RepaintSelection(SelectionType aType);

  





  nsIFrame* GetFrameForNodeOffset(nsIContent *aNode,
                                  PRInt32     aOffset,
                                  HINT        aHint,
                                  PRInt32    *aReturnOffset);

  











  void CommonPageMove(PRBool aForward,
                      PRBool aExtend,
                      nsIScrollableView *aScrollableView);

  void SetHint(HINT aHintRight) { mHint = aHintRight; }
  HINT GetHint() { return mHint; }
  
#ifdef IBMBIDI
  



  virtual void SetCaretBidiLevel (PRUint8 aLevel);
  


  virtual PRUint8 GetCaretBidiLevel();
  


  virtual void UndefineCaretBidiLevel();
#endif

  




  nsresult CharacterMove(PRBool aForward, PRBool aExtend);

  




  nsresult WordMove(PRBool aForward, PRBool aExtend);

  



  nsresult WordExtendForDelete(PRBool aForward);
  
  




  nsresult LineMove(PRBool aForward, PRBool aExtend);

  




  nsresult IntraLineMove(PRBool aForward, PRBool aExtend); 

  


  nsresult SelectAll();

  

  void SetDisplaySelection(PRInt16 aState) { mDisplaySelection = aState; }
  PRInt16 GetDisplaySelection() { return mDisplaySelection; }

  





  void SetDelayedCaretData(nsMouseEvent *aMouseEvent);

  





  nsMouseEvent* GetDelayedCaretData();

  




  nsIContent* GetLimiter() { return mLimiter; }

  



  void SetMouseDoubleDown(PRBool aDoubleDown) { mMouseDoubleDownState = aDoubleDown; }
  
  


  PRBool GetMouseDoubleDown() { return mMouseDoubleDownState; }

  















  virtual nsPrevNextBidiLevels GetPrevNextBidiLevels(nsIContent *aNode,
                                                     PRUint32 aContentOffset,
                                                     PRBool aJumpLines);

  








  nsresult GetFrameFromLevel(nsIFrame *aFrameIn,
                             nsDirection aDirection,
                             PRUint8 aBidiLevel,
                             nsIFrame **aFrameOut);

  







  nsresult MaintainSelection(nsSelectionAmount aAmount = eSelectNoAmount);


  nsFrameSelection();
  virtual ~nsFrameSelection();

  void StartBatchChanges();
  void EndBatchChanges();
  nsresult DeleteFromDocument();

  nsIPresShell *GetShell() {return mShell;}

private:
  nsresult TakeFocus(nsIContent *aNewFocus,
                     PRUint32 aContentOffset,
                     PRUint32 aContentEndOffset, 
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
                                             PRBool aJumpLines);
#ifdef VISUALSELECTION
  NS_IMETHOD VisualSelectFrames(nsIFrame* aCurrentFrame,
                                nsPeekOffsetStruct aPos);
  NS_IMETHOD VisualSequence(nsIFrame* aSelectFrame,
                            nsIFrame* aCurrentFrame,
                            nsPeekOffsetStruct* aPos,
                            PRBool* aNeedVisualSelection);
  NS_IMETHOD SelectToEdge(nsIFrame *aFrame,
                          nsIContent *aContent,
                          PRInt32 aOffset,
                          PRInt32 aEdge,
                          PRBool aMultipleSelection);
  NS_IMETHOD SelectLines(nsDirection aSelectionDirection,
                         nsIDOMNode *aAnchorNode,
                         nsIFrame* aAnchorFrame,
                         PRInt32 aAnchorOffset,
                         nsIDOMNode *aCurrentNode,
                         nsIFrame* aCurrentFrame,
                         PRInt32 aCurrentOffset,
                         nsPeekOffsetStruct aPos);
#endif 

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

  PRUint32     GetBatching(){return mBatching;}
  PRBool       GetNotifyFrames(){return mNotifyFrames;}
  void         SetDirty(PRBool aDirty=PR_TRUE){if (mBatching) mChangesDuringBatching = aDirty;}

  nsresult     NotifySelectionListeners(SelectionType aType);     

  nsTypedSelection *mDomSelections[nsISelectionController::NUM_SELECTIONTYPES];

  
  
  nsITableLayout* GetTableLayout(nsIContent *aTableContent);
  nsITableCellLayout* GetCellLayout(nsIContent *aCellContent);

  nsresult SelectBlockOfCells(nsIContent *aStartNode, nsIContent *aEndNode);
  nsresult SelectRowOrColumn(nsIContent *aCellContent, PRUint32 aTarget);
  nsresult GetCellIndexes(nsIContent *aCell, PRInt32 &aRowIndex, PRInt32 &aColIndex);

  nsresult GetFirstSelectedCellAndRange(nsIDOMNode **aCell, nsIDOMRange **aRange);
  nsresult GetNextSelectedCellAndRange(nsIDOMNode **aCell, nsIDOMRange **aRange);
  nsresult GetFirstCellNodeInRange(nsIDOMRange *aRange, nsIDOMNode **aCellNode);
  
  PRBool   IsInSameTable(nsIContent *aContent1, nsIContent *aContent2, nsIContent **aTableNode);
  nsresult GetParentTable(nsIContent *aCellNode, nsIContent **aTableNode);
  nsresult SelectCellElement(nsIDOMElement* aCellElement);
  nsresult CreateAndAddRange(nsIDOMNode *aParentNode, PRInt32 aOffset);
  nsresult ClearNormalSelection();

  nsCOMPtr<nsIDOMNode> mCellParent; 
  nsCOMPtr<nsIContent> mStartSelectedCell;
  nsCOMPtr<nsIContent> mEndSelectedCell;
  nsCOMPtr<nsIContent> mAppendStartSelectedCell;
  nsCOMPtr<nsIContent> mUnselectCellOnMouseUp;
  PRInt32  mSelectingTableCellMode;
  PRInt32  mSelectedCellIndex;

  
  nsCOMPtr<nsIDOMRange> mMaintainRange;
  nsSelectionAmount mMaintainedAmount;

  
  PRInt32 mBatching;
    
  nsIContent *mLimiter;     
  nsIPresShell *mShell;

  PRInt16 mSelectionChangeReason; 
  PRInt16 mDisplaySelection; 

  HINT  mHint;   
#ifdef IBMBIDI
  PRInt8 mCaretBidiLevel;
#endif

  PRInt32 mDesiredX;
  nsIScrollableViewProvider* mScrollableViewProvider;

  nsMouseEvent mDelayedMouseEvent;

  PRPackedBool mDelayedMouseEventValid;

  PRPackedBool mChangesDuringBatching;
  PRPackedBool mNotifyFrames;
  PRPackedBool mIsEditor;
  PRPackedBool mDragSelectingCells;
  PRPackedBool mMouseDownState;   
  PRPackedBool mMouseDoubleDownState; 
  PRPackedBool mDesiredXSet;

  PRInt8 mCaretMovementStyle;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsFrameSelection, NS_FRAME_SELECTION_IID)

#endif 
