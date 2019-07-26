









#ifndef nsCSSFrameConstructor_h___
#define nsCSSFrameConstructor_h___

#include "mozilla/Attributes.h"

#include "nsCOMPtr.h"
#include "nsILayoutHistoryState.h"
#include "nsQuoteList.h"
#include "nsCounterManager.h"
#include "nsHashKeys.h"
#include "nsThreadUtils.h"
#include "nsPageContentFrame.h"
#include "nsCSSPseudoElements.h"
#include "RestyleTracker.h"
#include "nsIAnonymousContentCreator.h"
#include "nsFrameManager.h"

class nsIDocument;
struct nsFrameItems;
struct nsAbsoluteItems;
class nsStyleContext;
struct nsStyleContent;
struct nsStyleDisplay;
class nsIPresShell;
class nsIDOMHTMLSelectElement;
class nsPresContext;
class nsStyleChangeList;
class nsIFrame;
struct nsGenConInitializer;
class ChildIterator;
class nsICSSAnonBoxPseudo;
class nsPageContentFrame;
struct PendingBinding;
class nsRefreshDriver;

class nsFrameConstructorState;
class nsFrameConstructorSaveState;

class nsCSSFrameConstructor : public nsFrameManager
{
  friend class nsRefreshDriver;

public:
  typedef mozilla::dom::Element Element;
  typedef mozilla::css::RestyleTracker RestyleTracker;
  typedef mozilla::css::OverflowChangedTracker OverflowChangedTracker;

  nsCSSFrameConstructor(nsIDocument *aDocument, nsIPresShell* aPresShell);
  ~nsCSSFrameConstructor(void) {
    NS_ASSERTION(mUpdateCount == 0, "Dying in the middle of our own update?");
  }

  struct RestyleData;
  friend struct RestyleData;

  
  static void GetAlternateTextFor(nsIContent*    aContent,
                                  nsIAtom*       aTag,  
                                  nsXPIDLString& aAltText);

private:
  nsCSSFrameConstructor(const nsCSSFrameConstructor& aCopy) MOZ_DELETE;
  nsCSSFrameConstructor& operator=(const nsCSSFrameConstructor& aCopy) MOZ_DELETE;

public:
  
  nsresult ConstructRootFrame(nsIFrame** aNewFrame);

  nsresult ReconstructDocElementHierarchy();

  
  
  
  
  void CreateNeededFrames();

private:
  void CreateNeededFrames(nsIContent* aContent);

  enum Operation {
    CONTENTAPPEND,
    CONTENTINSERT
  };

  
  
  bool MaybeConstructLazily(Operation aOperation,
                              nsIContent* aContainer,
                              nsIContent* aChild);

  
  
  void IssueSingleInsertNofications(nsIContent* aContainer,
                                    nsIContent* aStartChild,
                                    nsIContent* aEndChild,
                                    bool aAllowLazyConstruction);
  
  
  
  
  
  
  nsIFrame* GetRangeInsertionPoint(nsIContent* aContainer,
                                   nsIFrame* aParentFrame,
                                   nsIContent* aStartChild,
                                   nsIContent* aEndChild,
                                   bool aAllowLazyConstruction);

  
  bool MaybeRecreateForFrameset(nsIFrame* aParentFrame,
                                  nsIContent* aStartChild,
                                  nsIContent* aEndChild);

public:
  











































  
  
  nsresult ContentAppended(nsIContent* aContainer,
                           nsIContent* aFirstNewContent,
                           bool        aAllowLazyConstruction);

  
  
  nsresult ContentInserted(nsIContent*            aContainer,
                           nsIContent*            aChild,
                           nsILayoutHistoryState* aFrameState,
                           bool                   aAllowLazyConstruction);

  
  
  
  
  
  
  nsresult ContentRangeInserted(nsIContent*            aContainer,
                                nsIContent*            aStartChild,
                                nsIContent*            aEndChild,
                                nsILayoutHistoryState* aFrameState,
                                bool                   aAllowLazyConstruction);

  enum RemoveFlags { REMOVE_CONTENT, REMOVE_FOR_RECONSTRUCTION };
  nsresult ContentRemoved(nsIContent* aContainer,
                          nsIContent* aChild,
                          nsIContent* aOldNextSibling,
                          RemoveFlags aFlags,
                          bool*     aDidReconstruct);

  nsresult CharacterDataChanged(nsIContent* aContent,
                                CharacterDataChangeInfo* aInfo);

  nsresult ContentStateChanged(nsIContent*   aContent,
                               nsEventStates aStateMask);

  
  nsresult GenerateChildFrames(nsIFrame* aFrame);

  
  
  void NotifyDestroyingFrame(nsIFrame* aFrame);

  void AttributeWillChange(Element* aElement,
                           int32_t  aNameSpaceID,
                           nsIAtom* aAttribute,
                           int32_t  aModType);
  void AttributeChanged(Element* aElement,
                        int32_t  aNameSpaceID,
                        nsIAtom* aAttribute,
                        int32_t  aModType);

  void BeginUpdate();
  void EndUpdate();
  void RecalcQuotesAndCounters();

  
  
  void WillDestroyFrameTree();

  
  
  uint32_t GetHoverGeneration() const { return mHoverGeneration; }

  
  
  uint64_t GetAnimationGeneration() const { return mAnimationGeneration; }

  
  
  
  
  
  nsresult ProcessRestyledFrames(nsStyleChangeList& aRestyleArray);

private:

  friend class mozilla::css::RestyleTracker;

  void RestyleForEmptyChange(Element* aContainer);

public:
  
  
  
  void RestyleForInsertOrChange(Element* aContainer, nsIContent* aChild);

  
  
  
  
  
  void RestyleForRemove(Element* aContainer,
                        nsIContent* aOldChild,
                        nsIContent* aFollowingSibling);
  
  
  void RestyleForAppend(Element* aContainer, nsIContent* aFirstNewContent);

  
  
  
  
  
  
  
  void ProcessPendingRestyles();
  
  
  
  
  void RebuildAllStyleData(nsChangeHint aExtraHint);

  
  
  void DoRebuildAllStyleData(RestyleTracker& aRestyleTracker,
                             nsChangeHint aExtraHint);

  
  void PostRestyleEvent(Element* aElement,
                        nsRestyleHint aRestyleHint,
                        nsChangeHint aMinChangeHint)
  {
    nsPresContext *presContext = mPresShell->GetPresContext();
    if (presContext) {
      PostRestyleEventCommon(aElement, aRestyleHint, aMinChangeHint,
                             presContext->IsProcessingAnimationStyleChange());
    }
  }

  
  void PostAnimationRestyleEvent(Element* aElement,
                                 nsRestyleHint aRestyleHint,
                                 nsChangeHint aMinChangeHint)
  {
    PostRestyleEventCommon(aElement, aRestyleHint, aMinChangeHint, true);
  }

  void FlushOverflowChangedTracker() 
  {
    mOverflowChangedTracker.Flush();
  }

private:
  














  void PostRestyleEventCommon(Element* aElement,
                              nsRestyleHint aRestyleHint,
                              nsChangeHint aMinChangeHint,
                              bool aForAnimation);
  void PostRestyleEventInternal(bool aForLazyConstruction);
public:

  









  void PostRebuildAllStyleDataEvent(nsChangeHint aExtraHint);

  
  nsIFrame* CreateContinuingFrame(nsPresContext* aPresContext,
                                  nsIFrame*       aFrame,
                                  nsIFrame*       aParentFrame,
                                  bool            aIsFluid = true);

  
  nsresult ReplicateFixedFrames(nsPageContentFrame* aParentFrame);

  
  nsresult GetInsertionPoint(nsIFrame*     aParentFrame,
                             nsIContent*   aChildContent,
                             nsIFrame**    aInsertionPoint,
                             bool*       aMultiple = nullptr);

  nsresult CreateListBoxContent(nsPresContext* aPresContext,
                                nsIFrame*       aParentFrame,
                                nsIFrame*       aPrevFrame,
                                nsIContent*     aChild,
                                nsIFrame**      aResult,
                                bool            aIsAppend,
                                bool            aIsScrollbar,
                                nsILayoutHistoryState* aFrameState);

  
  
  
  nsIFrame* GetRootElementFrame() { return mRootElementFrame; }
  
  
  nsIFrame* GetRootElementStyleFrame() { return mRootElementStyleFrame; }
  nsIFrame* GetPageSequenceFrame() { return mPageSequenceFrame; }

  
  nsIFrame* GetDocElementContainingBlock()
    { return mDocElementContainingBlock; }

private:
  struct FrameConstructionItem;
  class FrameConstructionItemList;

  nsIFrame* ConstructPageFrame(nsIPresShell*  aPresShell, 
                               nsPresContext* aPresContext,
                               nsIFrame*      aParentFrame,
                               nsIFrame*      aPrevPageFrame,
                               nsIFrame*&     aCanvasFrame);

  
  
  void RestyleElement(Element* aElement,
                      nsIFrame*       aPrimaryFrame,
                      nsChangeHint    aMinHint,
                      RestyleTracker& aRestyleTracker,
                      bool            aRestyleDescendants);

  void InitAndRestoreFrame (const nsFrameConstructorState& aState,
                            nsIContent*                    aContent,
                            nsIFrame*                      aParentFrame,
                            nsIFrame*                      aPrevInFlow,
                            nsIFrame*                      aNewFrame,
                            bool                           aAllowCounters = true);

  
  
  already_AddRefed<nsStyleContext>
  ResolveStyleContext(nsIFrame*         aParentFrame,
                      nsIContent*       aContent,
                      nsFrameConstructorState* aState);
  already_AddRefed<nsStyleContext>
  ResolveStyleContext(nsStyleContext* aParentStyleContext,
                      nsIContent* aContent,
                      nsFrameConstructorState* aState);

  
  
  
  nsresult ConstructFrame(nsFrameConstructorState& aState,
                          nsIContent*              aContent,
                          nsIFrame*                aParentFrame,
                          nsFrameItems&            aFrameItems);

  
  
  
  
  
  void AddFrameConstructionItems(nsFrameConstructorState& aState,
                                 nsIContent*              aContent,
                                 bool                     aSuppressWhiteSpaceOptimizations,
                                 nsIFrame*                aParentFrame,
                                 FrameConstructionItemList& aItems);

  
  
  
  nsresult ConstructDocElementFrame(Element*                 aDocElement,
                                    nsILayoutHistoryState*   aFrameState,
                                    nsIFrame**               aNewFrame);

  
  
  nsresult SetUpDocElementContainingBlock(nsIContent* aDocElement);

  













  nsresult CreateAttributeContent(nsIContent* aParentContent,
                                  nsIFrame* aParentFrame,
                                  int32_t aAttrNamespace,
                                  nsIAtom* aAttrName,
                                  nsStyleContext* aStyleContext,
                                  nsCOMArray<nsIContent>& aGeneratedContent,
                                  nsIContent** aNewContent,
                                  nsIFrame** aNewFrame);

  



  already_AddRefed<nsIContent> CreateGenConTextNode(nsFrameConstructorState& aState,
                                                    const nsString& aString,
                                                    nsCOMPtr<nsIDOMCharacterData>* aText,
                                                    nsGenConInitializer* aInitializer);

  








  already_AddRefed<nsIContent> CreateGeneratedContent(nsFrameConstructorState& aState,
                                                      nsIContent*     aParentContent,
                                                      nsStyleContext* aStyleContext,
                                                      uint32_t        aContentIndex);

  
  void CreateGeneratedContentItem(nsFrameConstructorState&   aState,
                                  nsIFrame*                  aFrame,
                                  nsIContent*                aContent,
                                  nsStyleContext*            aStyleContext,
                                  nsCSSPseudoElements::Type  aPseudoElement,
                                  FrameConstructionItemList& aItems);

  
  
  
  
  
  nsresult AppendFramesToParent(nsFrameConstructorState&       aState,
                                nsIFrame*                      aParentFrame,
                                nsFrameItems&                  aFrameList,
                                nsIFrame*                      aPrevSibling,
                                bool                           aIsRecursiveCall = false);

  
  



  nsresult ConstructTable(nsFrameConstructorState& aState,
                          FrameConstructionItem&   aItem,
                          nsIFrame*                aParentFrame,
                          const nsStyleDisplay*    aDisplay,
                          nsFrameItems&            aFrameItems,
                          nsIFrame**               aNewFrame);

  


  nsresult ConstructTableRow(nsFrameConstructorState& aState,
                             FrameConstructionItem&   aItem,
                             nsIFrame*                aParentFrame,
                             const nsStyleDisplay*    aStyleDisplay,
                             nsFrameItems&            aFrameItems,
                             nsIFrame**               aNewFrame);

  


  nsresult ConstructTableCol(nsFrameConstructorState& aState,
                             FrameConstructionItem&   aItem,
                             nsIFrame*                aParentFrame,
                             const nsStyleDisplay*    aStyleDisplay,
                             nsFrameItems&            aFrameItems,
                             nsIFrame**               aNewFrame);

  


  nsresult ConstructTableCell(nsFrameConstructorState& aState,
                              FrameConstructionItem&   aItem,
                              nsIFrame*                aParentFrame,
                              const nsStyleDisplay*    aStyleDisplay,
                              nsFrameItems&            aFrameItems,
                              nsIFrame**               aNewFrame);

private:
  
  enum ParentType {
    eTypeBlock = 0, 
    eTypeRow,
    eTypeRowGroup,
    eTypeColGroup,
    eTypeTable,
    eParentTypeCount
  };

  
#define FCDATA_PARENT_TYPE_OFFSET 29
  

#define FCDATA_DESIRED_PARENT_TYPE(_bits)           \
  ParentType((_bits) >> FCDATA_PARENT_TYPE_OFFSET)
  
#define FCDATA_DESIRED_PARENT_TYPE_TO_BITS(_type)     \
  (((uint32_t)(_type)) << FCDATA_PARENT_TYPE_OFFSET)

  
  static ParentType GetParentType(nsIFrame* aParentFrame) {
    return GetParentType(aParentFrame->GetType());
  }

  
  static ParentType GetParentType(nsIAtom* aFrameType);

  






  typedef nsIFrame* (* FrameCreationFunc)(nsIPresShell*, nsStyleContext*);

  





  struct FrameConstructionData;
  typedef const FrameConstructionData*
    (* FrameConstructionDataGetter)(Element*, nsStyleContext*);

  


















  typedef nsresult
    (nsCSSFrameConstructor::* FrameFullConstructor)(nsFrameConstructorState& aState,
                                                    FrameConstructionItem& aItem,
                                                    nsIFrame* aParentFrame,
                                                    const nsStyleDisplay* aStyleDisplay,
                                                    nsFrameItems& aFrameItems,
                                                    nsIFrame** aFrame);

  

  



#define FCDATA_SKIP_FRAMESET 0x1
  


#define FCDATA_FUNC_IS_DATA_GETTER 0x2
  


#define FCDATA_FUNC_IS_FULL_CTOR 0x4
  



#define FCDATA_DISALLOW_OUT_OF_FLOW 0x8
  



#define FCDATA_FORCE_NULL_ABSPOS_CONTAINER 0x10
  


#define FCDATA_WRAP_KIDS_IN_BLOCKS 0x20
  


#define FCDATA_SUPPRESS_FRAME 0x40
  

#define FCDATA_MAY_NEED_SCROLLFRAME 0x80
#ifdef MOZ_XUL
  

#define FCDATA_IS_POPUP 0x100
#endif 
  

#define FCDATA_SKIP_ABSPOS_PUSH 0x200
  


#define FCDATA_DISALLOW_GENERATED_CONTENT 0x400
  


#define FCDATA_IS_TABLE_PART 0x800
  

#define FCDATA_IS_INLINE 0x1000
  

#define FCDATA_IS_LINE_PARTICIPANT 0x2000
  

#define FCDATA_IS_LINE_BREAK 0x4000
  

#define FCDATA_ALLOW_BLOCK_STYLES 0x8000
  



#define FCDATA_USE_CHILD_ITEMS 0x10000
  


#define FCDATA_FORCED_NON_SCROLLABLE_BLOCK 0x20000
  



#define FCDATA_CREATE_BLOCK_WRAPPER_FOR_ALL_KIDS 0x40000
  

#define FCDATA_IS_SVG_TEXT 0x80000

  

  struct FrameConstructionData {
    
    uint32_t mBits;
    
    
    
    
    
    
    
    union Func {
      FrameCreationFunc mCreationFunc;
      FrameConstructionDataGetter mDataGetter;
    } mFunc;
    FrameFullConstructor mFullConstructor;
    
    
    nsICSSAnonBoxPseudo * const * const mAnonBoxPseudo;
  };

  



  struct FrameConstructionDataByTag {
    
    
    const nsIAtom * const * const mTag;
    const FrameConstructionData mData;
  };

  

  struct FrameConstructionDataByInt {
    
    const int32_t mInt;
    const FrameConstructionData mData;
  };

  

  struct PseudoParentData {
    const FrameConstructionData mFCData;
    nsICSSAnonBoxPseudo * const * const mPseudoType;
  };
  

  static const PseudoParentData sPseudoParentData[eParentTypeCount];

  




  static const FrameConstructionData*
    FindDataByInt(int32_t aInt, Element* aElement,
                  nsStyleContext* aStyleContext,
                  const FrameConstructionDataByInt* aDataPtr,
                  uint32_t aDataLength);

  




  static const FrameConstructionData*
    FindDataByTag(nsIAtom* aTag, Element* aElement,
                  nsStyleContext* aStyleContext,
                  const FrameConstructionDataByTag* aDataPtr,
                  uint32_t aDataLength);

  
  class FrameConstructionItemList {
  public:
    FrameConstructionItemList() :
      mInlineCount(0),
      mBlockCount(0),
      mLineParticipantCount(0),
      mItemCount(0),
      mLineBoundaryAtStart(false),
      mLineBoundaryAtEnd(false),
      mParentHasNoXBLChildren(false),
      mTriedConstructingFrames(false)
    {
      PR_INIT_CLIST(&mItems);
      memset(mDesiredParentCounts, 0, sizeof(mDesiredParentCounts));
    }

    ~FrameConstructionItemList() {
      PRCList* cur = PR_NEXT_LINK(&mItems);
      while (cur != &mItems) {
        PRCList* next = PR_NEXT_LINK(cur);
        delete ToItem(cur);
        cur = next;
      }

      
      

      
      
      
      if (!mUndisplayedItems.IsEmpty() && mTriedConstructingFrames) {
        
        
        nsFrameManager *mgr =
          mUndisplayedItems[0].mStyleContext->PresContext()->FrameManager();
        for (uint32_t i = 0; i < mUndisplayedItems.Length(); ++i) {
          UndisplayedItem& item = mUndisplayedItems[i];
          mgr->SetUndisplayedContent(item.mContent, item.mStyleContext);
        }
      }
    }

    void SetLineBoundaryAtStart(bool aBoundary) { mLineBoundaryAtStart = aBoundary; }
    void SetLineBoundaryAtEnd(bool aBoundary) { mLineBoundaryAtEnd = aBoundary; }
    void SetParentHasNoXBLChildren(bool aHasNoXBLChildren) {
      mParentHasNoXBLChildren = aHasNoXBLChildren;
    }
    void SetTriedConstructingFrames() { mTriedConstructingFrames = true; }
    bool HasLineBoundaryAtStart() { return mLineBoundaryAtStart; }
    bool HasLineBoundaryAtEnd() { return mLineBoundaryAtEnd; }
    bool ParentHasNoXBLChildren() { return mParentHasNoXBLChildren; }
    bool IsEmpty() const { return PR_CLIST_IS_EMPTY(&mItems); }
    bool AnyItemsNeedBlockParent() const { return mLineParticipantCount != 0; }
    bool AreAllItemsInline() const { return mInlineCount == mItemCount; }
    bool AreAllItemsBlock() const { return mBlockCount == mItemCount; }
    bool AllWantParentType(ParentType aDesiredParentType) const {
      return mDesiredParentCounts[aDesiredParentType] == mItemCount;
    }

    
    
    
    FrameConstructionItem* AppendItem(const FrameConstructionData* aFCData,
                                      nsIContent* aContent,
                                      nsIAtom* aTag,
                                      int32_t aNameSpaceID,
                                      PendingBinding* aPendingBinding,
                                      already_AddRefed<nsStyleContext> aStyleContext,
                                      bool aSuppressWhiteSpaceOptimizations)
    {
      FrameConstructionItem* item =
        new FrameConstructionItem(aFCData, aContent, aTag, aNameSpaceID,
                                  aPendingBinding, aStyleContext,
                                  aSuppressWhiteSpaceOptimizations);
      PR_APPEND_LINK(item, &mItems);
      ++mItemCount;
      ++mDesiredParentCounts[item->DesiredParentType()];
      return item;
    }

    void AppendUndisplayedItem(nsIContent* aContent,
                               nsStyleContext* aStyleContext) {
      mUndisplayedItems.AppendElement(UndisplayedItem(aContent, aStyleContext));
    }

    void InlineItemAdded() { ++mInlineCount; }
    void BlockItemAdded() { ++mBlockCount; }
    void LineParticipantItemAdded() { ++mLineParticipantCount; }

    class Iterator;
    friend class Iterator;

    class Iterator {
    public:
      Iterator(FrameConstructionItemList& list) :
        mCurrent(PR_NEXT_LINK(&list.mItems)),
        mEnd(&list.mItems),
        mList(list)
      {}
      Iterator(const Iterator& aOther) :
        mCurrent(aOther.mCurrent),
        mEnd(aOther.mEnd),
        mList(aOther.mList)
      {}

      bool operator==(const Iterator& aOther) const {
        NS_ASSERTION(mEnd == aOther.mEnd, "Iterators for different lists?");
        return mCurrent == aOther.mCurrent;
      }
      bool operator!=(const Iterator& aOther) const {
        return !(*this == aOther);
      }
      Iterator& operator=(const Iterator& aOther) {
        NS_ASSERTION(mEnd == aOther.mEnd, "Iterators for different lists?");
        mCurrent = aOther.mCurrent;
        return *this;
      }

      FrameConstructionItemList* List() {
        return &mList;
      }

      operator FrameConstructionItem& () {
        return item();
      }

      FrameConstructionItem& item() {
        return *FrameConstructionItemList::ToItem(mCurrent);
      }
      bool IsDone() const { return mCurrent == mEnd; }
      bool AtStart() const { return mCurrent == PR_NEXT_LINK(mEnd); }
      void Next() {
        NS_ASSERTION(!IsDone(), "Should have checked IsDone()!");
        mCurrent = PR_NEXT_LINK(mCurrent);
      }
      void Prev() {
        NS_ASSERTION(!AtStart(), "Should have checked AtStart()!");
        mCurrent = PR_PREV_LINK(mCurrent);
      }
      void SetToEnd() { mCurrent = mEnd; }

      
      
      
      inline bool SkipItemsWantingParentType(ParentType aParentType);

#ifdef MOZ_FLEXBOX
      
      
      
      inline bool SkipItemsThatNeedAnonFlexItem(
        const nsFrameConstructorState& aState);

      
      
      
      inline bool SkipItemsThatDontNeedAnonFlexItem(
        const nsFrameConstructorState& aState);
#endif 

      
      
      
      inline bool SkipWhitespace(nsFrameConstructorState& aState);

      
      
      
      
      void AppendItemToList(FrameConstructionItemList& aTargetList);

      
      
      
      
      
      
      void AppendItemsToList(const Iterator& aEnd,
                             FrameConstructionItemList& aTargetList);

      
      
      
      
      
      void InsertItem(FrameConstructionItem* aItem);

      
      
      
      
      
      void DeleteItemsTo(const Iterator& aEnd);

    private:
      PRCList* mCurrent;
      PRCList* mEnd;
      FrameConstructionItemList& mList;
    };

  private:
    static FrameConstructionItem* ToItem(PRCList* item) {
      return static_cast<FrameConstructionItem*>(item);
    }

    struct UndisplayedItem {
      UndisplayedItem(nsIContent* aContent, nsStyleContext* aStyleContext) :
        mContent(aContent), mStyleContext(aStyleContext)
      {}

      nsIContent * const mContent;
      nsRefPtr<nsStyleContext> mStyleContext;
    };

    
    
    void AdjustCountsForItem(FrameConstructionItem* aItem, int32_t aDelta);

    PRCList mItems;
    uint32_t mInlineCount;
    uint32_t mBlockCount;
    uint32_t mLineParticipantCount;
    uint32_t mItemCount;
    uint32_t mDesiredParentCounts[eParentTypeCount];
    
    
    bool mLineBoundaryAtStart;
    
    
    bool mLineBoundaryAtEnd;
    
    bool mParentHasNoXBLChildren;
    
    bool mTriedConstructingFrames;

    nsTArray<UndisplayedItem> mUndisplayedItems;
  };

  typedef FrameConstructionItemList::Iterator FCItemIterator;

  



  struct FrameConstructionItem : public PRCList {
    
    
    FrameConstructionItem(const FrameConstructionData* aFCData,
                          nsIContent* aContent,
                          nsIAtom* aTag,
                          int32_t aNameSpaceID,
                          PendingBinding* aPendingBinding,
                          already_AddRefed<nsStyleContext> aStyleContext,
                          bool aSuppressWhiteSpaceOptimizations) :
      mFCData(aFCData), mContent(aContent), mTag(aTag),
      mNameSpaceID(aNameSpaceID),
      mPendingBinding(aPendingBinding), mStyleContext(aStyleContext),
      mSuppressWhiteSpaceOptimizations(aSuppressWhiteSpaceOptimizations),
      mIsText(false), mIsGeneratedContent(false),
      mIsRootPopupgroup(false), mIsAllInline(false), mIsBlock(false),
      mHasInlineEnds(false), mIsPopup(false),
      mIsLineParticipant(false), mIsForSVGAElement(false)
    {}
    ~FrameConstructionItem() {
      if (mIsGeneratedContent) {
        mContent->UnbindFromTree();
        NS_RELEASE(mContent);
      }
    }

    ParentType DesiredParentType() {
      return FCDATA_DESIRED_PARENT_TYPE(mFCData->mBits);
    }

    
    
#ifdef MOZ_FLEXBOX
    bool NeedsAnonFlexItem(const nsFrameConstructorState& aState);
#endif 

    
    
    
    bool IsWhitespace(nsFrameConstructorState& aState) const;

    bool IsLineBoundary() const {
      return mIsBlock || (mFCData->mBits & FCDATA_IS_LINE_BREAK);
    }

    
    const FrameConstructionData* mFCData;
    
    nsIContent* mContent;
    
    nsIAtom* mTag;
    
    int32_t mNameSpaceID;
    
    
    
    
    
    
    
    
    PendingBinding* mPendingBinding;
    
    nsRefPtr<nsStyleContext> mStyleContext;
    
    
    bool mSuppressWhiteSpaceOptimizations;
    
    bool mIsText;
    
    
    bool mIsGeneratedContent;
    
    bool mIsRootPopupgroup;
    
    
    
    
    bool mIsAllInline;
    
    
    
    
    
    bool mIsBlock;
    
    
    
    
    bool mHasInlineEnds;
    
    
    bool mIsPopup;
    
    bool mIsLineParticipant;
    
    bool mIsForSVGAElement;

    
    FrameConstructionItemList mChildItems;

  private:
    FrameConstructionItem(const FrameConstructionItem& aOther) MOZ_DELETE; 
  };

  






#ifdef MOZ_FLEXBOX
  void CreateNeededAnonFlexItems(nsFrameConstructorState& aState,
                                    FrameConstructionItemList& aItems,
                                    nsIFrame* aParentFrame);
#endif 

  




  inline void CreateNeededTablePseudos(nsFrameConstructorState& aState,
                                       FrameConstructionItemList& aItems,
                                       nsIFrame* aParentFrame);

  







  
  
  
  
  void AdjustParentFrame(nsIFrame* &                  aParentFrame,
                         const FrameConstructionData* aFCData,
                         nsStyleContext*              aStyleContext);

  

protected:
  static nsIFrame* CreatePlaceholderFrameFor(nsIPresShell*    aPresShell, 
                                             nsIContent*      aContent,
                                             nsIFrame*        aFrame,
                                             nsStyleContext*  aStyleContext,
                                             nsIFrame*        aParentFrame,
                                             nsIFrame*        aPrevInFlow,
                                             nsFrameState     aTypeBit);

private:
  
  
  nsresult ConstructSelectFrame(nsFrameConstructorState& aState,
                                FrameConstructionItem&   aItem,
                                nsIFrame*                aParentFrame,
                                const nsStyleDisplay*    aStyleDisplay,
                                nsFrameItems&            aFrameItems,
                                nsIFrame**               aNewFrame);

  
  
  nsresult ConstructFieldSetFrame(nsFrameConstructorState& aState,
                                  FrameConstructionItem&   aItem,
                                  nsIFrame*                aParentFrame,
                                  const nsStyleDisplay*    aStyleDisplay,
                                  nsFrameItems&            aFrameItems,
                                  nsIFrame**               aNewFrame);

  
  
  static const FrameConstructionData* FindTextData(nsIFrame* aParentFrame);

  void ConstructTextFrame(const FrameConstructionData* aData,
                          nsFrameConstructorState& aState,
                          nsIContent*              aContent,
                          nsIFrame*                aParentFrame,
                          nsStyleContext*          aStyleContext,
                          nsFrameItems&            aFrameItems);

  
  
  void AddTextItemIfNeeded(nsFrameConstructorState& aState,
                           nsIFrame* aParentFrame,
                           nsIContent* aPossibleTextContent,
                           FrameConstructionItemList& aItems);

  
  
  void ReframeTextIfNeeded(nsIContent* aParentContent,
                           nsIContent* aContent);

  void AddPageBreakItem(nsIContent* aContent,
                        nsStyleContext* aMainStyleContext,
                        FrameConstructionItemList& aItems);

  
  
  
  
  static const FrameConstructionData* FindHTMLData(Element* aContent,
                                                   nsIAtom* aTag,
                                                   int32_t aNameSpaceID,
                                                   nsIFrame* aParentFrame,
                                                   nsStyleContext* aStyleContext);
  
  static const FrameConstructionData*
    FindImgData(Element* aElement, nsStyleContext* aStyleContext);
  static const FrameConstructionData*
    FindImgControlData(Element* aElement, nsStyleContext* aStyleContext);
  static const FrameConstructionData*
    FindInputData(Element* aElement, nsStyleContext* aStyleContext);
  static const FrameConstructionData*
    FindObjectData(Element* aElement, nsStyleContext* aStyleContext);
  static const FrameConstructionData*
    FindCanvasData(Element* aElement, nsStyleContext* aStyleContext);

  










  nsresult ConstructFrameFromItemInternal(FrameConstructionItem& aItem,
                                          nsFrameConstructorState& aState,
                                          nsIFrame* aParentFrame,
                                          nsFrameItems& aFrameItems);

  
  
#define ITEM_ALLOW_XBL_BASE 0x1
  

#define ITEM_ALLOW_PAGE_BREAK 0x2
  
#define ITEM_IS_GENERATED_CONTENT 0x4
  
#define ITEM_IS_WITHIN_SVG_TEXT 0x8
  
#define ITEM_ALLOWS_TEXT_PATH_CHILD 0x10
  
  
  
  void AddFrameConstructionItemsInternal(nsFrameConstructorState& aState,
                                         nsIContent*              aContent,
                                         nsIFrame*                aParentFrame,
                                         nsIAtom*                 aTag,
                                         int32_t                  aNameSpaceID,
                                         bool                     aSuppressWhiteSpaceOptimizations,
                                         nsStyleContext*          aStyleContext,
                                         uint32_t                 aFlags,
                                         FrameConstructionItemList& aItems);

  



  nsresult ConstructFramesFromItemList(nsFrameConstructorState& aState,
                                       FrameConstructionItemList& aItems,
                                       nsIFrame* aParentFrame,
                                       nsFrameItems& aFrameItems);
  nsresult ConstructFramesFromItem(nsFrameConstructorState& aState,
                                   FCItemIterator& aItem,
                                   nsIFrame* aParentFrame,
                                   nsFrameItems& aFrameItems);
  static bool AtLineBoundary(FCItemIterator& aIter);

  nsresult CreateAnonymousFrames(nsFrameConstructorState& aState,
                                 nsIContent*              aParent,
                                 nsIFrame*                aParentFrame,
                                 PendingBinding  *        aPendingBinding,
                                 nsFrameItems&            aChildItems);

  nsresult GetAnonymousContent(nsIContent* aParent,
                               nsIFrame* aParentFrame,
                               nsTArray<nsIAnonymousContentCreator::ContentInfo>& aAnonContent);


  




  nsresult FlushAccumulatedBlock(nsFrameConstructorState& aState,
                                 nsIContent* aContent,
                                 nsIFrame* aParentFrame,
                                 nsFrameItems& aBlockItems,
                                 nsFrameItems& aNewItems);

  
  
  static const FrameConstructionData* FindMathMLData(Element* aElement,
                                                     nsIAtom* aTag,
                                                     int32_t aNameSpaceID,
                                                     nsStyleContext* aStyleContext);

  
  
  static const FrameConstructionData* FindXULTagData(Element* aElement,
                                                     nsIAtom* aTag,
                                                     int32_t aNameSpaceID,
                                                     nsStyleContext* aStyleContext);
  
#ifdef MOZ_XUL
  static const FrameConstructionData*
    FindPopupGroupData(Element* aElement, nsStyleContext* aStyleContext);
  
  static const FrameConstructionData sXULTextBoxData;
  static const FrameConstructionData*
    FindXULLabelData(Element* aElement, nsStyleContext* aStyleContext);
  static const FrameConstructionData*
    FindXULDescriptionData(Element* aElement, nsStyleContext* aStyleContext);
#ifdef XP_MACOSX
  static const FrameConstructionData*
    FindXULMenubarData(Element* aElement, nsStyleContext* aStyleContext);
#endif 
  static const FrameConstructionData*
    FindXULListBoxBodyData(Element* aElement, nsStyleContext* aStyleContext);
  static const FrameConstructionData*
    FindXULListItemData(Element* aElement, nsStyleContext* aStyleContext);
#endif 

  
  
  
  
  
  static const FrameConstructionData*
    FindXULDisplayData(const nsStyleDisplay* aDisplay,
                       Element* aElement,
                       nsStyleContext* aStyleContext);


  




  nsresult ConstructOuterSVG(nsFrameConstructorState& aState,
                             FrameConstructionItem&   aItem,
                             nsIFrame*                aParentFrame,
                             const nsStyleDisplay*    aDisplay,
                             nsFrameItems&            aFrameItems,
                             nsIFrame**               aNewFrame);

  static const FrameConstructionData* FindSVGData(Element* aElement,
                                                  nsIAtom* aTag,
                                                  int32_t aNameSpaceID,
                                                  nsIFrame* aParentFrame,
                                                  bool aIsWithinSVGText,
                                                  bool aAllowsTextPathChild,
                                                  nsStyleContext* aStyleContext);

  

  const FrameConstructionData*
    FindDisplayData(const nsStyleDisplay* aDisplay, Element* aElement,
                    nsIFrame* aParentFrame, nsStyleContext* aStyleContext);

  


  nsresult ConstructScrollableBlock(nsFrameConstructorState& aState,
                                    FrameConstructionItem&   aItem,
                                    nsIFrame*                aParentFrame,
                                    const nsStyleDisplay*    aDisplay,
                                    nsFrameItems&            aFrameItems,
                                    nsIFrame**               aNewFrame);

  


  nsresult ConstructNonScrollableBlock(nsFrameConstructorState& aState,
                                       FrameConstructionItem&   aItem,
                                       nsIFrame*                aParentFrame,
                                       const nsStyleDisplay*    aDisplay,
                                       nsFrameItems&            aFrameItems,
                                       nsIFrame**               aNewFrame);

  






























  nsresult ProcessChildren(nsFrameConstructorState& aState,
                           nsIContent*              aContent,
                           nsStyleContext*          aStyleContext,
                           nsIFrame*                aFrame,
                           const bool               aCanHaveGeneratedContent,
                           nsFrameItems&            aFrameItems,
                           const bool               aAllowBlockStyles,
                           PendingBinding*          aPendingBinding,
                           nsIFrame*                aPossiblyLeafFrame = nullptr);

  nsIFrame* GetFrameFor(nsIContent* aContent);

  







public:
  enum ContainingBlockType {
    ABS_POS,
    FIXED_POS
  };
  nsIFrame* GetAbsoluteContainingBlock(nsIFrame* aFrame, ContainingBlockType aType);
  nsIFrame* GetFloatContainingBlock(nsIFrame* aFrame);

private:
  nsIContent* PropagateScrollToViewport();

  
  
  
  
  nsresult
  BuildScrollFrame(nsFrameConstructorState& aState,
                   nsIContent*              aContent,
                   nsStyleContext*          aContentStyle,
                   nsIFrame*                aScrolledFrame,
                   nsIFrame*                aParentFrame,
                   nsIFrame*&               aNewFrame);

  
  already_AddRefed<nsStyleContext>
  BeginBuildingScrollFrame(nsFrameConstructorState& aState,
                           nsIContent*              aContent,
                           nsStyleContext*          aContentStyle,
                           nsIFrame*                aParentFrame,
                           nsIAtom*                 aScrolledPseudo,
                           bool                     aIsRoot,
                           nsIFrame*&               aNewFrame);

  
  
  void
  FinishBuildingScrollFrame(nsIFrame* aScrollFrame,
                            nsIFrame* aScrolledFrame);

  
  
  
  nsresult
  InitializeSelectFrame(nsFrameConstructorState& aState,
                        nsIFrame*                scrollFrame,
                        nsIFrame*                scrolledFrame,
                        nsIContent*              aContent,
                        nsIFrame*                aParentFrame,
                        nsStyleContext*          aStyleContext,
                        bool                     aBuildCombobox,
                        PendingBinding*          aPendingBinding,
                        nsFrameItems&            aFrameItems);

  nsresult MaybeRecreateFramesForElement(Element* aElement);

  
  
  nsresult RecreateFramesForContent(nsIContent* aContent, bool aAsyncInsert);

  
  
  
  
  
  
  
  
  
  bool MaybeRecreateContainerForFrameRemoval(nsIFrame* aFrame,
                                               nsresult* aResult);

  nsIFrame* CreateContinuingOuterTableFrame(nsIPresShell*    aPresShell, 
                                            nsPresContext*  aPresContext,
                                            nsIFrame*        aFrame,
                                            nsIFrame*        aParentFrame,
                                            nsIContent*      aContent,
                                            nsStyleContext*  aStyleContext);

  nsIFrame* CreateContinuingTableFrame(nsIPresShell*    aPresShell, 
                                       nsPresContext*  aPresContext,
                                       nsIFrame*        aFrame,
                                       nsIFrame*        aParentFrame,
                                       nsIContent*      aContent,
                                       nsStyleContext*  aStyleContext);

  

  

  already_AddRefed<nsStyleContext>
  GetFirstLetterStyle(nsIContent*      aContent,
                      nsStyleContext*  aStyleContext);

  already_AddRefed<nsStyleContext>
  GetFirstLineStyle(nsIContent*      aContent,
                    nsStyleContext*  aStyleContext);

  bool ShouldHaveFirstLetterStyle(nsIContent*      aContent,
                                    nsStyleContext*  aStyleContext);

  
  
  bool HasFirstLetterStyle(nsIFrame* aBlockFrame);

  bool ShouldHaveFirstLineStyle(nsIContent*      aContent,
                                  nsStyleContext*  aStyleContext);

  void ShouldHaveSpecialBlockStyle(nsIContent*      aContent,
                                   nsStyleContext*  aStyleContext,
                                   bool*          aHaveFirstLetterStyle,
                                   bool*          aHaveFirstLineStyle);

  
  
  
  
  
  
  
  
  
  
  
  
  
  nsresult ConstructBlock(nsFrameConstructorState& aState,
                          const nsStyleDisplay*    aDisplay,
                          nsIContent*              aContent,
                          nsIFrame*                aParentFrame,
                          nsIFrame*                aContentParentFrame,
                          nsStyleContext*          aStyleContext,
                          nsIFrame**               aNewFrame,
                          nsFrameItems&            aFrameItems,
                          bool                     aAbsPosContainer,
                          PendingBinding*          aPendingBinding);

  nsresult ConstructInline(nsFrameConstructorState& aState,
                           FrameConstructionItem&   aItem,
                           nsIFrame*                aParentFrame,
                           const nsStyleDisplay*    aDisplay,
                           nsFrameItems&            aFrameItems,
                           nsIFrame**               aNewFrame);

  

















  void CreateIBSiblings(nsFrameConstructorState& aState,
                        nsIFrame* aInitialInline,
                        bool aIsPositioned,
                        nsFrameItems& aChildItems,
                        nsFrameItems& aSiblings);

  



  void BuildInlineChildItems(nsFrameConstructorState& aState,
                             FrameConstructionItem& aParentItem,
                             bool aItemIsWithinSVGText,
                             bool aItemAllowsTextPathChild);

  
  
  
  
  
  
  
  
  
  
  bool WipeContainingBlock(nsFrameConstructorState& aState,
                             nsIFrame*                aContainingBlock,
                             nsIFrame*                aFrame,
                             FrameConstructionItemList& aItems,
                             bool                     aIsAppend,
                             nsIFrame*                aPrevSibling);

  nsresult ReframeContainingBlock(nsIFrame* aFrame);

  nsresult StyleChangeReflow(nsIFrame* aFrame, nsChangeHint aHint);

  
  
  
  bool RecomputePosition(nsIFrame* aFrame);

  

  

  void CreateFloatingLetterFrame(nsFrameConstructorState& aState,
                                 nsIFrame*                aBlockFrame,
                                 nsIContent*              aTextContent,
                                 nsIFrame*                aTextFrame,
                                 nsIContent*              aBlockContent,
                                 nsIFrame*                aParentFrame,
                                 nsStyleContext*          aStyleContext,
                                 nsFrameItems&            aResult);

  nsresult CreateLetterFrame(nsIFrame*                aBlockFrame,
                             nsIFrame*                aBlockContinuation,
                             nsIContent*              aTextContent,
                             nsIFrame*                aParentFrame,
                             nsFrameItems&            aResult);

  nsresult WrapFramesInFirstLetterFrame(nsIContent*   aBlockContent,
                                        nsIFrame*     aBlockFrame,
                                        nsFrameItems& aBlockFrames);

  





















  nsresult WrapFramesInFirstLetterFrame(nsIFrame*     aBlockFrame,
                                        nsIFrame*     aBlockContinuation,
                                        nsIFrame*     aParentFrame,
                                        nsIFrame*     aParentFrameList,
                                        nsIFrame**    aModifiedParent,
                                        nsIFrame**    aTextFrame,
                                        nsIFrame**    aPrevFrame,
                                        nsFrameItems& aLetterFrames,
                                        bool*       aStopLooking);

  nsresult RecoverLetterFrames(nsIFrame* aBlockFrame);

  
  nsresult RemoveLetterFrames(nsPresContext*  aPresContext,
                              nsIPresShell*    aPresShell,
                              nsIFrame*        aBlockFrame);

  
  nsresult RemoveFirstLetterFrames(nsPresContext*  aPresContext,
                                   nsIPresShell*    aPresShell,
                                   nsIFrame*        aFrame,
                                   nsIFrame*        aBlockFrame,
                                   bool*          aStopLooking);

  
  nsresult RemoveFloatingFirstLetterFrames(nsPresContext*  aPresContext,
                                           nsIPresShell*    aPresShell,
                                           nsIFrame*        aBlockFrame,
                                           bool*          aStopLooking);

  
  
  void CaptureStateForFramesOf(nsIContent* aContent,
                               nsILayoutHistoryState* aHistoryState);

  

  

  
  
  
  
  
  
  
  void WrapFramesInFirstLineFrame(nsFrameConstructorState& aState,
                                  nsIContent*              aBlockContent,
                                  nsIFrame*                aBlockFrame,
                                  nsIFrame*                aLineFrame,
                                  nsFrameItems&            aFrameItems);

  
  
  void AppendFirstLineFrames(nsFrameConstructorState& aState,
                             nsIContent*              aContent,
                             nsIFrame*                aBlockFrame,
                             nsFrameItems&            aFrameItems);

  nsresult InsertFirstLineFrames(nsFrameConstructorState& aState,
                                 nsIContent*              aContent,
                                 nsIFrame*                aBlockFrame,
                                 nsIFrame**               aParentFrame,
                                 nsIFrame*                aPrevSibling,
                                 nsFrameItems&            aFrameItems);

  
  
  
  
  
  
  
  nsIFrame* FindFrameForContentSibling(nsIContent* aContent,
                                       nsIContent* aTargetContent,
                                       uint8_t& aTargetContentDisplay,
                                       bool aPrevSibling);

  
  
  
  nsIFrame* FindPreviousSibling(const ChildIterator& aFirst,
                                ChildIterator aIter,
                                uint8_t& aTargetContentDisplay);

  
  
  
  nsIFrame* FindNextSibling(ChildIterator aIter,
                            const ChildIterator& aLast,
                            uint8_t& aTargetContentDisplay);

  
  
  
  
  
  
  
  
  
  
  
  
  
  nsIFrame* GetInsertionPrevSibling(nsIFrame*& aParentFrame, 
                                    nsIContent* aContainer,
                                    nsIContent* aChild,
                                    bool* aIsAppend,
                                    bool* aIsRangeInsertSafe,
                                    nsIContent* aStartSkipChild = nullptr,
                                    nsIContent *aEndSkipChild = nullptr);

  
  
  
  
  bool IsValidSibling(nsIFrame*              aSibling,
                        nsIContent*            aContent,
                        uint8_t&               aDisplay);
  
  void QuotesDirty() {
    NS_PRECONDITION(mUpdateCount != 0, "Instant quote updates are bad news");
    mQuotesDirty = true;
    mDocument->SetNeedLayoutFlush();
  }

  void CountersDirty() {
    NS_PRECONDITION(mUpdateCount != 0, "Instant counter updates are bad news");
    mCountersDirty = true;
    mDocument->SetNeedLayoutFlush();
  }

  






  static void SetAsUndisplayedContent(FrameConstructionItemList& aList,
                                      nsIContent* aContent,
                                      nsStyleContext* aStyleContext,
                                      bool aIsGeneratedContent);

public:

  friend class nsFrameConstructorState;

private:

  nsIDocument*        mDocument;  

  
  
  
  
  nsIFrame*           mRootElementFrame;
  
  nsIFrame*           mRootElementStyleFrame;
  
  
  nsIFrame*           mFixedContainingBlock;
  
  
  nsIFrame*           mDocElementContainingBlock;
  nsIFrame*           mGfxScrollFrame;
  nsIFrame*           mPageSequenceFrame;
  nsQuoteList         mQuoteList;
  nsCounterManager    mCounterManager;
  uint16_t            mUpdateCount;
  bool                mQuotesDirty : 1;
  bool                mCountersDirty : 1;
  bool                mIsDestroyingFrameTree : 1;
  bool                mRebuildAllStyleData : 1;
  
  bool                mHasRootAbsPosContainingBlock : 1;
  
  bool                mObservingRefreshDriver : 1;
  
  bool                mInStyleRefresh : 1;
  uint32_t            mHoverGeneration;
  nsChangeHint        mRebuildAllExtraHint;

  nsCOMPtr<nsILayoutHistoryState> mTempFrameTreeState;

  OverflowChangedTracker mOverflowChangedTracker;

  
  
  uint64_t mAnimationGeneration;

  RestyleTracker mPendingRestyles;
  RestyleTracker mPendingAnimationRestyles;
};

#endif 
