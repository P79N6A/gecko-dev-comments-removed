





#ifndef mozilla_Selection_h__
#define mozilla_Selection_h__

#include "nsIWeakReference.h"

#include "mozilla/AutoRestore.h"
#include "mozilla/TextRange.h"
#include "nsISelection.h"
#include "nsISelectionController.h"
#include "nsISelectionPrivate.h"
#include "nsRange.h"
#include "nsThreadUtils.h"
#include "nsWrapperCache.h"

struct CachedOffsetForFrame;
class nsAutoScrollTimer;
class nsIContentIterator;
class nsIFrame;
class nsFrameSelection;
struct SelectionDetails;

namespace mozilla {
class ErrorResult;
struct AutoPrepareFocusRange;
}

struct RangeData
{
  explicit RangeData(nsRange* aRange)
    : mRange(aRange)
  {}

  nsRefPtr<nsRange> mRange;
  mozilla::TextRangeStyle mTextRangeStyle;
};






namespace mozilla {
namespace dom {

class Selection MOZ_FINAL : public nsISelectionPrivate,
                            public nsWrapperCache,
                            public nsSupportsWeakReference
{
protected:
  virtual ~Selection();

public:
  Selection();
  explicit Selection(nsFrameSelection *aList);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(Selection, nsISelectionPrivate)
  NS_DECL_NSISELECTION
  NS_DECL_NSISELECTIONPRIVATE

  nsIDocument* GetParentObject() const;

  
  nsPresContext* GetPresContext() const;
  nsIPresShell* GetPresShell() const;
  nsFrameSelection* GetFrameSelection() const { return mFrameSelection; }
  
  
  
  
  
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
  





  nsresult      AddItem(nsRange* aRange, int32_t* aOutIndex);
  nsresult      RemoveItem(nsRange* aRange);
  nsresult      RemoveCollapsedRanges();
  nsresult      Clear(nsPresContext* aPresContext);
  nsresult      Collapse(nsINode* aParentNode, int32_t aOffset);
  nsresult      Extend(nsINode* aParentNode, int32_t aOffset);
  nsRange*      GetRangeAt(int32_t aIndex);

  
  
  const nsRange* GetAnchorFocusRange() const {
    return mAnchorFocusRange;
  }

  nsDirection  GetDirection(){return mDirection;}
  void         SetDirection(nsDirection aDir){mDirection = aDir;}
  nsresult     SetAnchorFocusToRange(nsRange *aRange);
  void         ReplaceAnchorFocusRange(nsRange *aRange);
  void         AdjustAnchorFocusForMultiRange(nsDirection aDirection);

  
  NS_IMETHOD   GetPrimaryFrameForAnchorNode(nsIFrame **aResultFrame);
  NS_IMETHOD   GetPrimaryFrameForFocusNode(nsIFrame **aResultFrame, int32_t *aOffset, bool aVisual);
  NS_IMETHOD   LookUpSelection(nsIContent *aContent, int32_t aContentOffset, int32_t aContentLength,
                             SelectionDetails **aReturnDetails, SelectionType aType, bool aSlowCheck);
  NS_IMETHOD   Repaint(nsPresContext* aPresContext);

  
  nsresult     StartAutoScrollTimer(nsIFrame *aFrame,
                                    nsPoint& aPoint,
                                    uint32_t aDelay);

  nsresult     StopAutoScrollTimer();

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;

  
  nsINode*     GetAnchorNode();
  uint32_t     AnchorOffset();
  nsINode*     GetFocusNode();
  uint32_t     FocusOffset();

  bool IsCollapsed();
  void Collapse(nsINode& aNode, uint32_t aOffset, mozilla::ErrorResult& aRv);
  void CollapseToStart(mozilla::ErrorResult& aRv);
  void CollapseToEnd(mozilla::ErrorResult& aRv);

  void Extend(nsINode& aNode, uint32_t aOffset, mozilla::ErrorResult& aRv);

  void SelectAllChildren(nsINode& aNode, mozilla::ErrorResult& aRv);
  void DeleteFromDocument(mozilla::ErrorResult& aRv);

  uint32_t RangeCount() const
  {
    return mRanges.Length();
  }
  nsRange* GetRangeAt(uint32_t aIndex, mozilla::ErrorResult& aRv);
  void AddRange(nsRange& aRange, mozilla::ErrorResult& aRv);
  void RemoveRange(nsRange& aRange, mozilla::ErrorResult& aRv);
  void RemoveAllRanges(mozilla::ErrorResult& aRv);

  void Stringify(nsAString& aResult);

  bool ContainsNode(nsINode& aNode, bool aPartlyContained, mozilla::ErrorResult& aRv);

  void Modify(const nsAString& aAlter, const nsAString& aDirection,
              const nsAString& aGranularity, mozilla::ErrorResult& aRv);

  bool GetInterlinePosition(mozilla::ErrorResult& aRv);
  void SetInterlinePosition(bool aValue, mozilla::ErrorResult& aRv);

  void ToStringWithFormat(const nsAString& aFormatType,
                          uint32_t aFlags,
                          int32_t aWrapColumn,
                          nsAString& aReturn,
                          mozilla::ErrorResult& aRv);
  void AddSelectionListener(nsISelectionListener* aListener,
                            mozilla::ErrorResult& aRv);
  void RemoveSelectionListener(nsISelectionListener* aListener,
                               mozilla::ErrorResult& aRv);

  int16_t Type() const { return mType; }

  void GetRangesForInterval(nsINode& aBeginNode, int32_t aBeginOffset,
                            nsINode& aEndNode, int32_t aEndOffset,
                            bool aAllowAdjacent,
                            nsTArray<nsRefPtr<nsRange>>& aReturn,
                            mozilla::ErrorResult& aRv);

  void ScrollIntoView(int16_t aRegion, bool aIsSynchronous,
                      int16_t aVPercent, int16_t aHPercent,
                      mozilla::ErrorResult& aRv);

private:
  friend class ::nsAutoScrollTimer;

  
  nsresult DoAutoScroll(nsIFrame *aFrame, nsPoint& aPoint);

public:
  SelectionType GetType(){return mType;}
  void          SetType(SelectionType aType){mType = aType;}

  nsresult     NotifySelectionListeners();

  friend struct AutoApplyUserSelectStyle;
  struct MOZ_STACK_CLASS AutoApplyUserSelectStyle
  {
    explicit AutoApplyUserSelectStyle(Selection* aSelection
                             MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : mSavedValue(aSelection->mApplyUserSelectStyle)
    {
      MOZ_GUARD_OBJECT_NOTIFIER_INIT;
      aSelection->mApplyUserSelectStyle = true;
    }
    AutoRestore<bool> mSavedValue;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
  };
private:
  friend struct mozilla::AutoPrepareFocusRange;
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

  


  nsresult AddItemInternal(nsRange* aRange, int32_t* aOutIndex);

  
  
  
  
  
  
  
  
  
  
  
  
  
  nsTArray<RangeData> mRanges;

  nsRefPtr<nsRange> mAnchorFocusRange;
  nsRefPtr<nsFrameSelection> mFrameSelection;
  nsRefPtr<nsAutoScrollTimer> mAutoScrollTimer;
  nsCOMArray<nsISelectionListener> mSelectionListeners;
  nsRevocableEventPtr<ScrollSelectionIntoViewEvent> mScrollEvent;
  CachedOffsetForFrame *mCachedOffsetForFrame;
  nsDirection mDirection;
  SelectionType mType;
  



  bool mApplyUserSelectStyle;
};


class MOZ_STACK_CLASS SelectionBatcher MOZ_FINAL
{
private:
  nsRefPtr<Selection> mSelection;
public:
  explicit SelectionBatcher(Selection* aSelection)
  {
    mSelection = aSelection;
    if (mSelection) {
      mSelection->StartBatchChanges();
    }
  }

  ~SelectionBatcher()
  {
    if (mSelection) {
      mSelection->EndBatchChanges();
    }
  }
};

} 
} 

#endif
