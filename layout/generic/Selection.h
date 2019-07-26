





#ifndef mozilla_Selection_h__
#define mozilla_Selection_h__

#include "nsIWeakReference.h"

#include "nsISelection.h"
#include "nsISelectionController.h"
#include "nsISelectionPrivate.h"
#include "nsRange.h"

struct CachedOffsetForFrame;
class nsAutoScrollTimer;
class nsIContentIterator;
class nsIFrame;
struct SelectionDetails;

struct RangeData
{
  RangeData(nsRange* aRange)
    : mRange(aRange)
  {}

  nsRefPtr<nsRange> mRange;
  nsTextRangeStyle mTextRangeStyle;
};






namespace mozilla {

class Selection : public nsISelectionPrivate,
                  public nsSupportsWeakReference
{
public:
  Selection();
  Selection(nsFrameSelection *aList);
  virtual ~Selection();
  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(Selection, nsISelectionPrivate)
  NS_DECL_NSISELECTION
  NS_DECL_NSISELECTIONPRIVATE

  
  nsPresContext* GetPresContext() const;
  nsIPresShell* GetPresShell() const;
  
  
  
  
  
  nsIFrame*     GetSelectionAnchorGeometry(SelectionRegion aRegion, nsRect *aRect);
  
  
  
  nsIFrame*     GetSelectionEndPointGeometry(SelectionRegion aRegion, nsRect *aRect);

  nsresult      PostScrollSelectionIntoViewEvent(
                                        SelectionRegion aRegion,
                                        int32_t aFlags,
                                        nsIPresShell::ScrollAxis aVertical,
                                        nsIPresShell::ScrollAxis aHorizontal);
  enum {
    SCROLL_SYNCHRONOUS = 1<<1,
    SCROLL_FIRST_ANCESTOR_ONLY = 1<<2,
    SCROLL_DO_FLUSH = 1<<3,
    SCROLL_OVERFLOW_HIDDEN = 1<<5
  };
  
  
  nsresult      ScrollIntoView(SelectionRegion aRegion,
                               nsIPresShell::ScrollAxis aVertical =
                                 nsIPresShell::ScrollAxis(),
                               nsIPresShell::ScrollAxis aHorizontal =
                                 nsIPresShell::ScrollAxis(),
                               int32_t aFlags = 0);
  nsresult      SubtractRange(RangeData* aRange, nsRange* aSubtract,
                              nsTArray<RangeData>* aOutput);
  nsresult      AddItem(nsRange *aRange, int32_t* aOutIndex);
  nsresult      RemoveItem(nsRange *aRange);
  nsresult      RemoveCollapsedRanges();
  nsresult      Clear(nsPresContext* aPresContext);
  nsresult      Collapse(nsINode* aParentNode, int32_t aOffset);
  nsresult      Extend(nsINode* aParentNode, int32_t aOffset);
  nsRange*      GetRangeAt(int32_t aIndex);
  int32_t GetRangeCount() { return mRanges.Length(); }

  
  nsINode*     GetAnchorNode();
  int32_t      GetAnchorOffset();

  nsINode*     GetFocusNode();
  int32_t      GetFocusOffset();

  bool IsCollapsed();

  
  
  const nsRange* GetAnchorFocusRange() const {
    return mAnchorFocusRange;
  }

  nsDirection  GetDirection(){return mDirection;}
  void         SetDirection(nsDirection aDir){mDirection = aDir;}
  nsresult     SetAnchorFocusToRange(nsRange *aRange);
  void         ReplaceAnchorFocusRange(nsRange *aRange);

  
  NS_IMETHOD   GetPrimaryFrameForAnchorNode(nsIFrame **aResultFrame);
  NS_IMETHOD   GetPrimaryFrameForFocusNode(nsIFrame **aResultFrame, int32_t *aOffset, bool aVisual);
  NS_IMETHOD   LookUpSelection(nsIContent *aContent, int32_t aContentOffset, int32_t aContentLength,
                             SelectionDetails **aReturnDetails, SelectionType aType, bool aSlowCheck);
  NS_IMETHOD   Repaint(nsPresContext* aPresContext);

  
  nsresult     StartAutoScrollTimer(nsIFrame *aFrame,
                                    nsPoint& aPoint,
                                    uint32_t aDelay);

  nsresult     StopAutoScrollTimer();

private:
  friend class ::nsAutoScrollTimer;

  
  nsresult DoAutoScroll(nsIFrame *aFrame, nsPoint& aPoint);

public:
  SelectionType GetType(){return mType;}
  void          SetType(SelectionType aType){mType = aType;}

  nsresult     NotifySelectionListeners();

private:

  class ScrollSelectionIntoViewEvent;
  friend class ScrollSelectionIntoViewEvent;

  class ScrollSelectionIntoViewEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    ScrollSelectionIntoViewEvent(Selection* aSelection,
                                 SelectionRegion aRegion,
                                 nsIPresShell::ScrollAxis aVertical,
                                 nsIPresShell::ScrollAxis aHorizontal,
                                 int32_t aFlags)
      : mSelection(aSelection),
        mRegion(aRegion),
        mVerticalScroll(aVertical),
        mHorizontalScroll(aHorizontal),
        mFlags(aFlags) {
      NS_ASSERTION(aSelection, "null parameter");
    }
    void Revoke() { mSelection = nullptr; }
  private:
    Selection *mSelection;
    SelectionRegion mRegion;
    nsIPresShell::ScrollAxis mVerticalScroll;
    nsIPresShell::ScrollAxis mHorizontalScroll;
    int32_t mFlags;
  };

  void setAnchorFocusRange(int32_t aIndex); 
                                            
                                            
  nsresult     SelectAllFramesForContent(nsIContentIterator *aInnerIter,
                               nsIContent *aContent,
                               bool aSelected);
  nsresult     selectFrames(nsPresContext* aPresContext, nsRange *aRange, bool aSelect);
  nsresult     getTableCellLocationFromRange(nsRange *aRange, int32_t *aSelectionType, int32_t *aRow, int32_t *aCol);
  nsresult     addTableCellRange(nsRange *aRange, bool *aDidAddRange, int32_t *aOutIndex);

  nsresult FindInsertionPoint(
      nsTArray<RangeData>* aElementArray,
      nsINode* aPointNode, int32_t aPointOffset,
      nsresult (*aComparator)(nsINode*,int32_t,nsRange*,int32_t*),
      int32_t* aPoint);
  bool EqualsRangeAtPoint(nsINode* aBeginNode, int32_t aBeginOffset,
                            nsINode* aEndNode, int32_t aEndOffset,
                            int32_t aRangeIndex);
  nsresult GetIndicesForInterval(nsINode* aBeginNode, int32_t aBeginOffset,
                                 nsINode* aEndNode, int32_t aEndOffset,
                                 bool aAllowAdjacent,
                                 int32_t* aStartIndex, int32_t* aEndIndex);
  RangeData* FindRangeData(nsIDOMRange* aRange);

  
  
  
  
  
  
  
  
  
  
  
  
  
  nsTArray<RangeData> mRanges;

  nsRefPtr<nsRange> mAnchorFocusRange;
  nsRefPtr<nsFrameSelection> mFrameSelection;
  nsRefPtr<nsAutoScrollTimer> mAutoScrollTimer;
  nsCOMArray<nsISelectionListener> mSelectionListeners;
  nsRevocableEventPtr<ScrollSelectionIntoViewEvent> mScrollEvent;
  CachedOffsetForFrame *mCachedOffsetForFrame;
  nsDirection mDirection;
  SelectionType mType;
};

} 

#endif 
