









#ifndef nsCSSFrameConstructor_h___
#define nsCSSFrameConstructor_h___

#include "mozilla/Attributes.h"

#include "nsCOMPtr.h"
#include "nsILayoutHistoryState.h"
#include "nsQuoteList.h"
#include "nsCounterManager.h"
#include "nsCSSPseudoElements.h"
#include "nsIAnonymousContentCreator.h"
#include "nsFrameManager.h"
#include "nsIDocument.h"
#include "ScrollbarStyles.h"

struct nsFrameItems;
class nsStyleContext;
struct nsStyleDisplay;
struct nsGenConInitializer;

class nsContainerFrame;
class nsFirstLineFrame;
class nsICSSAnonBoxPseudo;
class nsPageContentFrame;
struct PendingBinding;
class nsGenericDOMDataNode;

class nsFrameConstructorState;

namespace mozilla {

class RestyleManager;

namespace dom {

class FlattenedChildIterator;

}
}

class nsCSSFrameConstructor : public nsFrameManager
{
public:
  typedef mozilla::dom::Element Element;

  friend class mozilla::RestyleManager;

  nsCSSFrameConstructor(nsIDocument *aDocument, nsIPresShell* aPresShell,
                        nsStyleSet* aStyleSet);
  ~nsCSSFrameConstructor(void) {
    NS_ASSERTION(mUpdateCount == 0, "Dying in the middle of our own update?");
  }

  
  static void GetAlternateTextFor(nsIContent*    aContent,
                                  nsIAtom*       aTag,  
                                  nsXPIDLString& aAltText);

private:
  nsCSSFrameConstructor(const nsCSSFrameConstructor& aCopy) = delete;
  nsCSSFrameConstructor& operator=(const nsCSSFrameConstructor& aCopy) = delete;

public:
  mozilla::RestyleManager* RestyleManager() const
    { return mPresShell->GetPresContext()->RestyleManager(); }

  nsIFrame* ConstructRootFrame();

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

  


  struct InsertionPoint
  {
    InsertionPoint()
      : mParentFrame(nullptr), mContainer(nullptr), mMultiple(false) {}
    InsertionPoint(nsContainerFrame* aParentFrame, nsIContent* aContainer,
                   bool aMultiple = false)
      : mParentFrame(aParentFrame), mContainer(aContainer),
        mMultiple(aMultiple) {}
    







    nsContainerFrame* mParentFrame;
    



    nsIContent* mContainer;
    



    bool mMultiple;
  };
  






  InsertionPoint GetRangeInsertionPoint(nsIContent* aContainer,
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

  enum RemoveFlags {
    REMOVE_CONTENT, REMOVE_FOR_RECONSTRUCTION, REMOVE_DESTROY_FRAMES };
  















  nsresult ContentRemoved(nsIContent*  aContainer,
                          nsIContent*  aChild,
                          nsIContent*  aOldNextSibling,
                          RemoveFlags  aFlags,
                          bool*        aDidReconstruct,
                          nsIContent** aDestroyedFramesFor = nullptr);

  nsresult CharacterDataChanged(nsIContent* aContent,
                                CharacterDataChangeInfo* aInfo);

  
  
  
  
  
  nsIFrame* EnsureFrameForTextNode(nsGenericDOMDataNode* aContent);

  
  nsresult GenerateChildFrames(nsContainerFrame* aFrame);

  
  
  void NotifyDestroyingFrame(nsIFrame* aFrame);

  void BeginUpdate();
  void EndUpdate();
  void RecalcQuotesAndCounters();

  
  void NotifyCounterStylesAreDirty();

  
  
  void WillDestroyFrameTree();

  







  void DestroyFramesFor(nsIContent*  aContent,
                        nsIContent** aDestroyedFramesFor);

  
  nsIFrame* CreateContinuingFrame(nsPresContext*    aPresContext,
                                  nsIFrame*         aFrame,
                                  nsContainerFrame* aParentFrame,
                                  bool              aIsFluid = true);

  
  nsresult ReplicateFixedFrames(nsPageContentFrame* aParentFrame);

  


  InsertionPoint GetInsertionPoint(nsIContent* aContainer, nsIContent* aChild);

  nsresult CreateListBoxContent(nsPresContext*    aPresContext,
                                nsContainerFrame* aParentFrame,
                                nsIFrame*         aPrevFrame,
                                nsIContent*       aChild,
                                nsIFrame**        aResult,
                                bool              aIsAppend,
                                bool              aIsScrollbar,
                                nsILayoutHistoryState* aFrameState);

  
  
  
  nsContainerFrame* GetRootElementFrame() { return mRootElementFrame; }
  
  
  nsIFrame* GetRootElementStyleFrame() { return mRootElementStyleFrame; }
  nsIFrame* GetPageSequenceFrame() { return mPageSequenceFrame; }

  
  nsContainerFrame* GetDocElementContainingBlock()
    { return mDocElementContainingBlock; }

  



  nsILayoutHistoryState* GetLastCapturedLayoutHistoryState()
  {
    return mTempFrameTreeState;
  }

private:
  struct FrameConstructionItem;
  class FrameConstructionItemList;

  nsContainerFrame* ConstructPageFrame(nsIPresShell*      aPresShell,
                                       nsPresContext*     aPresContext,
                                       nsContainerFrame*  aParentFrame,
                                       nsIFrame*          aPrevPageFrame,
                                       nsContainerFrame*& aCanvasFrame);

  void InitAndRestoreFrame (const nsFrameConstructorState& aState,
                            nsIContent*                    aContent,
                            nsContainerFrame*              aParentFrame,
                            nsIFrame*                      aNewFrame,
                            bool                           aAllowCounters = true);

  
  
  already_AddRefed<nsStyleContext>
  ResolveStyleContext(nsIFrame*                aParentFrame,
                      nsIContent*              aContainer,
                      nsIContent*              aChild,
                      nsFrameConstructorState* aState);
  already_AddRefed<nsStyleContext>
  ResolveStyleContext(nsIFrame*                aParentFrame,
                      nsIContent*              aChild,
                      nsFrameConstructorState* aState);
  already_AddRefed<nsStyleContext>
  ResolveStyleContext(const InsertionPoint&    aInsertion,
                      nsIContent*              aChild,
                      nsFrameConstructorState* aState);
  already_AddRefed<nsStyleContext>
  ResolveStyleContext(nsStyleContext*          aParentStyleContext,
                      nsIContent*              aContent,
                      nsFrameConstructorState* aState);

  
  
  
  
  
  void AddFrameConstructionItems(nsFrameConstructorState& aState,
                                 nsIContent*              aContent,
                                 bool                     aSuppressWhiteSpaceOptimizations,
                                 const InsertionPoint&    aInsertion,
                                 FrameConstructionItemList& aItems);

  
  
  
  bool ShouldCreateItemsForChild(nsFrameConstructorState& aState,
                                 nsIContent* aContent,
                                 nsContainerFrame* aParentFrame);

  
  
  void DoAddFrameConstructionItems(nsFrameConstructorState& aState,
                                   nsIContent* aContent,
                                   nsStyleContext* aStyleContext,
                                   bool aSuppressWhiteSpaceOptimizations,
                                   nsContainerFrame* aParentFrame,
                                   nsTArray<nsIAnonymousContentCreator::ContentInfo>* aAnonChildren,
                                   FrameConstructionItemList& aItems);

  
  
  
  nsIFrame* ConstructDocElementFrame(Element*                 aDocElement,
                                     nsILayoutHistoryState*   aFrameState);

  
  
  void SetUpDocElementContainingBlock(nsIContent* aDocElement);

  













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
                                                    nsRefPtr<nsTextNode>* aText,
                                                    nsGenConInitializer* aInitializer);

  








  already_AddRefed<nsIContent> CreateGeneratedContent(nsFrameConstructorState& aState,
                                                      nsIContent*     aParentContent,
                                                      nsStyleContext* aStyleContext,
                                                      uint32_t        aContentIndex);

  
  void CreateGeneratedContentItem(nsFrameConstructorState&   aState,
                                  nsContainerFrame*          aFrame,
                                  nsIContent*                aContent,
                                  nsStyleContext*            aStyleContext,
                                  nsCSSPseudoElements::Type  aPseudoElement,
                                  FrameConstructionItemList& aItems);

  
  
  
  
  
  nsresult AppendFramesToParent(nsFrameConstructorState&       aState,
                                nsContainerFrame*              aParentFrame,
                                nsFrameItems&                  aFrameList,
                                nsIFrame*                      aPrevSibling,
                                bool                           aIsRecursiveCall = false);

  
  



  nsIFrame* ConstructTable(nsFrameConstructorState& aState,
                           FrameConstructionItem&   aItem,
                           nsContainerFrame*        aParentFrame,
                           const nsStyleDisplay*    aDisplay,
                           nsFrameItems&            aFrameItems);

  


  nsIFrame* ConstructTableRowOrRowGroup(nsFrameConstructorState& aState,
                                        FrameConstructionItem&   aItem,
                                        nsContainerFrame*        aParentFrame,
                                        const nsStyleDisplay*    aStyleDisplay,
                                        nsFrameItems&            aFrameItems);

  


  nsIFrame* ConstructTableCol(nsFrameConstructorState& aState,
                              FrameConstructionItem&   aItem,
                              nsContainerFrame*        aParentFrame,
                              const nsStyleDisplay*    aStyleDisplay,
                              nsFrameItems&            aFrameItems);

  


  nsIFrame* ConstructTableCell(nsFrameConstructorState& aState,
                               FrameConstructionItem&   aItem,
                               nsContainerFrame*        aParentFrame,
                               const nsStyleDisplay*    aStyleDisplay,
                               nsFrameItems&            aFrameItems);

private:
  

  enum ParentType {
    eTypeBlock = 0, 
    eTypeRow,
    eTypeRowGroup,
    eTypeColGroup,
    eTypeTable,
    eTypeRuby,
    eTypeRubyBase,
    eTypeRubyBaseContainer,
    eTypeRubyText,
    eTypeRubyTextContainer,
    eParentTypeCount
  };

  
#define FCDATA_PARENT_TYPE_OFFSET 28
  

#define FCDATA_DESIRED_PARENT_TYPE(_bits)           \
  ParentType((_bits) >> FCDATA_PARENT_TYPE_OFFSET)
  
#define FCDATA_DESIRED_PARENT_TYPE_TO_BITS(_type)     \
  (((uint32_t)(_type)) << FCDATA_PARENT_TYPE_OFFSET)

  
  static ParentType GetParentType(nsIFrame* aParentFrame) {
    return GetParentType(aParentFrame->GetType());
  }

  
  static ParentType GetParentType(nsIAtom* aFrameType);

  static bool IsRubyParentType(ParentType aParentType) {
    return (aParentType == eTypeRuby ||
            aParentType == eTypeRubyBase ||
            aParentType == eTypeRubyBaseContainer ||
            aParentType == eTypeRubyText ||
            aParentType == eTypeRubyTextContainer);
  }

  static bool IsTableParentType(ParentType aParentType) {
    return (aParentType == eTypeTable ||
            aParentType == eTypeRow ||
            aParentType == eTypeRowGroup ||
            aParentType == eTypeColGroup);
  }

  






  typedef nsIFrame* (* FrameCreationFunc)(nsIPresShell*, nsStyleContext*);
  typedef nsContainerFrame* (* ContainerFrameCreationFunc)(nsIPresShell*, nsStyleContext*);

  





  struct FrameConstructionData;
  typedef const FrameConstructionData*
    (* FrameConstructionDataGetter)(Element*, nsStyleContext*);

  



















  typedef nsIFrame*
    (nsCSSFrameConstructor::* FrameFullConstructor)(nsFrameConstructorState& aState,
                                                    FrameConstructionItem& aItem,
                                                    nsContainerFrame* aParentFrame,
                                                    const nsStyleDisplay* aStyleDisplay,
                                                    nsFrameItems& aFrameItems);

  

  



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
  


#define FCDATA_IS_CONTENTS 0x100000

  

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
                                      already_AddRefed<nsStyleContext>&& aStyleContext,
                                      bool aSuppressWhiteSpaceOptimizations,
                                      nsTArray<nsIAnonymousContentCreator::ContentInfo>* aAnonChildren)
    {
      FrameConstructionItem* item =
        new FrameConstructionItem(aFCData, aContent, aTag, aNameSpaceID,
                                  aPendingBinding, aStyleContext,
                                  aSuppressWhiteSpaceOptimizations,
                                  aAnonChildren);
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
      explicit Iterator(FrameConstructionItemList& list) :
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
        MOZ_ASSERT(!IsDone(), "Should have checked IsDone()!");
        return *FrameConstructionItemList::ToItem(mCurrent);
      }

      const FrameConstructionItem& item() const {
        MOZ_ASSERT(!IsDone(), "Should have checked IsDone()!");
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

      
      
      
      inline bool SkipItemsNotWantingParentType(ParentType aParentType);

      
      
      
      inline bool SkipItemsThatNeedAnonFlexOrGridItem(
        const nsFrameConstructorState& aState);

      
      
      
      inline bool SkipItemsThatDontNeedAnonFlexOrGridItem(
        const nsFrameConstructorState& aState);

      
      
      
      inline bool SkipItemsNotWantingRubyParent();

      
      
      
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
                          already_AddRefed<nsStyleContext>& aStyleContext,
                          bool aSuppressWhiteSpaceOptimizations,
                          nsTArray<nsIAnonymousContentCreator::ContentInfo>* aAnonChildren) :
      mFCData(aFCData), mContent(aContent), mTag(aTag),
      mPendingBinding(aPendingBinding), mStyleContext(aStyleContext),
      mNameSpaceID(aNameSpaceID),
      mSuppressWhiteSpaceOptimizations(aSuppressWhiteSpaceOptimizations),
      mIsText(false), mIsGeneratedContent(false),
      mIsAnonymousContentCreatorContent(false),
      mIsRootPopupgroup(false), mIsAllInline(false), mIsBlock(false),
      mHasInlineEnds(false), mIsPopup(false),
      mIsLineParticipant(false), mIsForSVGAElement(false)
    {
      if (aAnonChildren) {
        NS_ASSERTION(!(mFCData->mBits & FCDATA_FUNC_IS_FULL_CTOR) ||
                     mFCData->mFullConstructor ==
                       &nsCSSFrameConstructor::ConstructInline,
                     "This is going to fail");
        NS_ASSERTION(!(mFCData->mBits & FCDATA_USE_CHILD_ITEMS),
                     "nsIAnonymousContentCreator::CreateAnonymousContent "
                     "implementations should not output a list where the "
                     "items have children in this case");
        mAnonChildren.SwapElements(*aAnonChildren);
      }
    }
    ~FrameConstructionItem() {
      if (mIsGeneratedContent) {
        mContent->UnbindFromTree();
        NS_RELEASE(mContent);
      }
    }

    ParentType DesiredParentType() {
      return FCDATA_DESIRED_PARENT_TYPE(mFCData->mBits);
    }

    
    
    bool NeedsAnonFlexOrGridItem(const nsFrameConstructorState& aState);

    
    
    
    bool IsWhitespace(nsFrameConstructorState& aState) const;

    bool IsLineBoundary() const {
      return mIsBlock || (mFCData->mBits & FCDATA_IS_LINE_BREAK);
    }

    
    FrameConstructionItemList mChildItems;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    nsTArray<nsIAnonymousContentCreator::ContentInfo> mAnonChildren;

    
    const FrameConstructionData* mFCData;
    
    nsIContent* mContent;
    
    nsIAtom* mTag;
    
    
    
    
    
    
    
    
    PendingBinding* mPendingBinding;
    
    nsRefPtr<nsStyleContext> mStyleContext;
    
    int32_t mNameSpaceID;
    
    
    bool mSuppressWhiteSpaceOptimizations:1;
    
    bool mIsText:1;
    
    
    bool mIsGeneratedContent:1;
    
    bool mIsAnonymousContentCreatorContent:1;
    
    bool mIsRootPopupgroup:1;
    
    
    
    
    bool mIsAllInline:1;
    
    
    
    
    
    bool mIsBlock:1;
    
    
    
    
    bool mHasInlineEnds:1;
    
    
    bool mIsPopup:1;
    
    bool mIsLineParticipant:1;
    
    bool mIsForSVGAElement:1;

  private:
    FrameConstructionItem(const FrameConstructionItem& aOther) = delete; 
  };

  






  void CreateNeededAnonFlexOrGridItems(nsFrameConstructorState& aState,
                                       FrameConstructionItemList& aItems,
                                       nsIFrame* aParentFrame);

  enum RubyWhitespaceType
  {
    eRubyNotWhitespace,
    eRubyInterLevelWhitespace,
    
    eRubyInterLeafWhitespace,
    eRubyInterSegmentWhitespace
  };

  



  static inline RubyWhitespaceType ComputeRubyWhitespaceType(
    uint_fast8_t aPrevDisplay, uint_fast8_t aNextDisplay);

  



  static inline RubyWhitespaceType InterpretRubyWhitespace(
    nsFrameConstructorState& aState,
    const FCItemIterator& aStartIter, const FCItemIterator& aEndIter);

  



  void WrapItemsInPseudoRubyLeafBox(FCItemIterator& aIter,
                                    nsStyleContext* aParentStyle,
                                    nsIContent* aParentContent);

  



  inline void WrapItemsInPseudoRubyLevelContainer(
    nsFrameConstructorState& aState, FCItemIterator& aIter,
    nsStyleContext* aParentStyle, nsIContent* aParentContent);

  


  inline void TrimLeadingAndTrailingWhitespaces(
    nsFrameConstructorState& aState, FrameConstructionItemList& aItems);

  


  inline void CreateNeededPseudoInternalRubyBoxes(
    nsFrameConstructorState& aState,
    FrameConstructionItemList& aItems, nsIFrame* aParentFrame);

  




  inline void CreateNeededPseudoContainers(nsFrameConstructorState& aState,
                                           FrameConstructionItemList& aItems,
                                           nsIFrame* aParentFrame);

  


  inline void WrapItemsInPseudoParent(nsIContent* aParentContent,
                                      nsStyleContext* aParentStyle,
                                      ParentType aWrapperType,
                                      FCItemIterator& aIter,
                                      const FCItemIterator& aEndIter);

  


  inline void CreateNeededPseudoSiblings(nsFrameConstructorState& aState,
                                         FrameConstructionItemList& aItems,
                                         nsIFrame* aParentFrame);

  







  
  
  
  
  void AdjustParentFrame(nsContainerFrame**           aParentFrame,
                         const FrameConstructionData* aFCData,
                         nsStyleContext*              aStyleContext);

  

protected:
  static nsIFrame* CreatePlaceholderFrameFor(nsIPresShell*     aPresShell,
                                             nsIContent*       aContent,
                                             nsIFrame*         aFrame,
                                             nsStyleContext*   aStyleContext,
                                             nsContainerFrame* aParentFrame,
                                             nsIFrame*         aPrevInFlow,
                                             nsFrameState      aTypeBit);

private:
  
  
  nsIFrame* ConstructSelectFrame(nsFrameConstructorState& aState,
                                 FrameConstructionItem&   aItem,
                                 nsContainerFrame*        aParentFrame,
                                 const nsStyleDisplay*    aStyleDisplay,
                                 nsFrameItems&            aFrameItems);

  
  
  nsIFrame* ConstructFieldSetFrame(nsFrameConstructorState& aState,
                                   FrameConstructionItem&   aItem,
                                   nsContainerFrame*        aParentFrame,
                                   const nsStyleDisplay*    aStyleDisplay,
                                   nsFrameItems&            aFrameItems);

  
  
  static const FrameConstructionData* FindTextData(nsIFrame* aParentFrame);

  void ConstructTextFrame(const FrameConstructionData* aData,
                          nsFrameConstructorState& aState,
                          nsIContent*              aContent,
                          nsContainerFrame*        aParentFrame,
                          nsStyleContext*          aStyleContext,
                          nsFrameItems&            aFrameItems);

  
  
  void AddTextItemIfNeeded(nsFrameConstructorState& aState,
                           const InsertionPoint& aInsertion,
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

  










  void ConstructFrameFromItemInternal(FrameConstructionItem& aItem,
                                      nsFrameConstructorState& aState,
                                      nsContainerFrame* aParentFrame,
                                      nsFrameItems& aFrameItems);

  
  
#define ITEM_ALLOW_XBL_BASE 0x1
  

#define ITEM_ALLOW_PAGE_BREAK 0x2
  
#define ITEM_IS_GENERATED_CONTENT 0x4
  
#define ITEM_IS_WITHIN_SVG_TEXT 0x8
  
#define ITEM_ALLOWS_TEXT_PATH_CHILD 0x10
  
#define ITEM_IS_ANONYMOUSCONTENTCREATOR_CONTENT 0x20
  
  
  
  void AddFrameConstructionItemsInternal(nsFrameConstructorState& aState,
                                         nsIContent*              aContent,
                                         nsContainerFrame*        aParentFrame,
                                         nsIAtom*                 aTag,
                                         int32_t                  aNameSpaceID,
                                         bool                     aSuppressWhiteSpaceOptimizations,
                                         nsStyleContext*          aStyleContext,
                                         uint32_t                 aFlags,
                                         nsTArray<nsIAnonymousContentCreator::ContentInfo>* aAnonChildren,
                                         FrameConstructionItemList& aItems);

  



  void ConstructFramesFromItemList(nsFrameConstructorState& aState,
                                   FrameConstructionItemList& aItems,
                                   nsContainerFrame* aParentFrame,
                                   nsFrameItems& aFrameItems);
  void ConstructFramesFromItem(nsFrameConstructorState& aState,
                               FCItemIterator& aItem,
                               nsContainerFrame* aParentFrame,
                               nsFrameItems& aFrameItems);
  static bool AtLineBoundary(FCItemIterator& aIter);

  nsresult CreateAnonymousFrames(nsFrameConstructorState& aState,
                                 nsIContent*              aParent,
                                 nsContainerFrame*        aParentFrame,
                                 PendingBinding*          aPendingBinding,
                                 nsFrameItems&            aChildItems);

  nsresult GetAnonymousContent(nsIContent* aParent,
                               nsIFrame* aParentFrame,
                               nsTArray<nsIAnonymousContentCreator::ContentInfo>& aAnonContent);


  




  void FlushAccumulatedBlock(nsFrameConstructorState& aState,
                             nsIContent* aContent,
                             nsContainerFrame* aParentFrame,
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

  





  nsContainerFrame* ConstructFrameWithAnonymousChild(
                                  nsFrameConstructorState& aState,
                                  FrameConstructionItem&   aItem,
                                  nsContainerFrame*        aParentFrame,
                                  const nsStyleDisplay*    aDisplay,
                                  nsFrameItems&            aFrameItems,
                                  ContainerFrameCreationFunc aConstructor,
                                  ContainerFrameCreationFunc aInnerConstructor,
                                  nsICSSAnonBoxPseudo*     aInnerPseudo,
                                  bool                     aCandidateRootFrame);

  


  nsIFrame* ConstructOuterSVG(nsFrameConstructorState& aState,
                              FrameConstructionItem&   aItem,
                              nsContainerFrame*        aParentFrame,
                              const nsStyleDisplay*    aDisplay,
                              nsFrameItems&            aFrameItems);

  


  nsIFrame* ConstructMarker(nsFrameConstructorState& aState,
                            FrameConstructionItem&   aItem,
                            nsContainerFrame*        aParentFrame,
                            const nsStyleDisplay*    aDisplay,
                            nsFrameItems&            aFrameItems);

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

  


  nsIFrame* ConstructScrollableBlock(nsFrameConstructorState& aState,
                                     FrameConstructionItem&   aItem,
                                     nsContainerFrame*        aParentFrame,
                                     const nsStyleDisplay*    aDisplay,
                                     nsFrameItems&            aFrameItems);

  


  nsIFrame* ConstructNonScrollableBlock(nsFrameConstructorState& aState,
                                        FrameConstructionItem&   aItem,
                                        nsContainerFrame*        aParentFrame,
                                        const nsStyleDisplay*    aDisplay,
                                        nsFrameItems&            aFrameItems);

  




  void AddFCItemsForAnonymousContent(
            nsFrameConstructorState& aState,
            nsContainerFrame* aFrame,
            nsTArray<nsIAnonymousContentCreator::ContentInfo>& aAnonymousItems,
            FrameConstructionItemList& aItemsToConstruct,
            uint32_t aExtraFlags = 0);

  






























  void ProcessChildren(nsFrameConstructorState& aState,
                       nsIContent*              aContent,
                       nsStyleContext*          aStyleContext,
                       nsContainerFrame*        aParentFrame,
                       const bool               aCanHaveGeneratedContent,
                       nsFrameItems&            aFrameItems,
                       const bool               aAllowBlockStyles,
                       PendingBinding*          aPendingBinding,
                       nsIFrame*                aPossiblyLeafFrame = nullptr);

  







public:
  enum ContainingBlockType {
    ABS_POS,
    FIXED_POS
  };
  nsContainerFrame* GetAbsoluteContainingBlock(nsIFrame* aFrame,
                                               ContainingBlockType aType);
  nsContainerFrame* GetFloatContainingBlock(nsIFrame* aFrame);

private:
  nsIContent* PropagateScrollToViewport();

  
  
  
  
  void
  BuildScrollFrame(nsFrameConstructorState& aState,
                   nsIContent*              aContent,
                   nsStyleContext*          aContentStyle,
                   nsIFrame*                aScrolledFrame,
                   nsContainerFrame*        aParentFrame,
                   nsContainerFrame*&       aNewFrame);

  
  already_AddRefed<nsStyleContext>
  BeginBuildingScrollFrame(nsFrameConstructorState& aState,
                           nsIContent*              aContent,
                           nsStyleContext*          aContentStyle,
                           nsContainerFrame*        aParentFrame,
                           nsIAtom*                 aScrolledPseudo,
                           bool                     aIsRoot,
                           nsContainerFrame*&       aNewFrame);

  
  
  void
  FinishBuildingScrollFrame(nsContainerFrame* aScrollFrame,
                            nsIFrame* aScrolledFrame);

  
  
  
  nsresult
  InitializeSelectFrame(nsFrameConstructorState& aState,
                        nsContainerFrame*        aScrollFrame,
                        nsContainerFrame*        aScrolledFrame,
                        nsIContent*              aContent,
                        nsContainerFrame*        aParentFrame,
                        nsStyleContext*          aStyleContext,
                        bool                     aBuildCombobox,
                        PendingBinding*          aPendingBinding,
                        nsFrameItems&            aFrameItems);

  






  nsStyleContext* MaybeRecreateFramesForElement(Element* aElement);

  








  nsresult
  RecreateFramesForContent(nsIContent*  aContent,
                           bool         aAsyncInsert,
                           RemoveFlags  aFlags,
                           nsIContent** aDestroyedFramesFor);

  
  
  
  
  
  
  
  
  
  
  bool MaybeRecreateContainerForFrameRemoval(nsIFrame*    aFrame,
                                             RemoveFlags  aFlags,
                                             nsresult*    aResult,
                                             nsIContent** aDestroyedFramesFor);

  nsIFrame* CreateContinuingOuterTableFrame(nsIPresShell*     aPresShell,
                                            nsPresContext*    aPresContext,
                                            nsIFrame*         aFrame,
                                            nsContainerFrame* aParentFrame,
                                            nsIContent*       aContent,
                                            nsStyleContext*   aStyleContext);

  nsIFrame* CreateContinuingTableFrame(nsIPresShell*     aPresShell,
                                       nsPresContext*    aPresContext,
                                       nsIFrame*         aFrame,
                                       nsContainerFrame* aParentFrame,
                                       nsIContent*       aContent,
                                       nsStyleContext*   aStyleContext);

  

  

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

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void ConstructBlock(nsFrameConstructorState& aState,
                      const nsStyleDisplay*    aDisplay,
                      nsIContent*              aContent,
                      nsContainerFrame*        aParentFrame,
                      nsContainerFrame*        aContentParentFrame,
                      nsStyleContext*          aStyleContext,
                      nsContainerFrame**       aNewFrame,
                      nsFrameItems&            aFrameItems,
                      nsIFrame*                aPositionedFrameForAbsPosContainer,
                      PendingBinding*          aPendingBinding);

  nsIFrame* ConstructInline(nsFrameConstructorState& aState,
                            FrameConstructionItem&   aItem,
                            nsContainerFrame*        aParentFrame,
                            const nsStyleDisplay*    aDisplay,
                            nsFrameItems&            aFrameItems);

  

















  void CreateIBSiblings(nsFrameConstructorState& aState,
                        nsContainerFrame* aInitialInline,
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

  nsresult ReframeContainingBlock(nsIFrame*    aFrame,
                                  RemoveFlags  aFlags,
                                  nsIContent** aReframeContent);

  

  

  void CreateFloatingLetterFrame(nsFrameConstructorState& aState,
                                 nsContainerFrame*        aBlockFrame,
                                 nsIContent*              aTextContent,
                                 nsIFrame*                aTextFrame,
                                 nsContainerFrame*        aParentFrame,
                                 nsStyleContext*          aStyleContext,
                                 nsFrameItems&            aResult);

  void CreateLetterFrame(nsContainerFrame*        aBlockFrame,
                         nsContainerFrame*        aBlockContinuation,
                         nsIContent*              aTextContent,
                         nsContainerFrame*        aParentFrame,
                         nsFrameItems&            aResult);

  void WrapFramesInFirstLetterFrame(nsIContent*       aBlockContent,
                                    nsContainerFrame* aBlockFrame,
                                    nsFrameItems&     aBlockFrames);

  





















  void WrapFramesInFirstLetterFrame(nsContainerFrame*  aBlockFrame,
                                    nsContainerFrame*  aBlockContinuation,
                                    nsContainerFrame*  aParentFrame,
                                    nsIFrame*          aParentFrameList,
                                    nsContainerFrame** aModifiedParent,
                                    nsIFrame**         aTextFrame,
                                    nsIFrame**         aPrevFrame,
                                    nsFrameItems&      aLetterFrames,
                                    bool*              aStopLooking);

  void RecoverLetterFrames(nsContainerFrame* aBlockFrame);

  
  nsresult RemoveLetterFrames(nsPresContext*    aPresContext,
                              nsIPresShell*     aPresShell,
                              nsContainerFrame* aBlockFrame);

  
  nsresult RemoveFirstLetterFrames(nsPresContext*    aPresContext,
                                   nsIPresShell*     aPresShell,
                                   nsContainerFrame* aFrame,
                                   nsContainerFrame* aBlockFrame,
                                   bool*             aStopLooking);

  
  nsresult RemoveFloatingFirstLetterFrames(nsPresContext*  aPresContext,
                                           nsIPresShell*    aPresShell,
                                           nsIFrame*        aBlockFrame,
                                           bool*          aStopLooking);

  
  
  void CaptureStateForFramesOf(nsIContent* aContent,
                               nsILayoutHistoryState* aHistoryState);

  

  

  
  
  
  
  
  
  
  void WrapFramesInFirstLineFrame(nsFrameConstructorState& aState,
                                  nsIContent*              aBlockContent,
                                  nsContainerFrame*        aBlockFrame,
                                  nsFirstLineFrame*        aLineFrame,
                                  nsFrameItems&            aFrameItems);

  
  
  void AppendFirstLineFrames(nsFrameConstructorState& aState,
                             nsIContent*              aContent,
                             nsContainerFrame*        aBlockFrame,
                             nsFrameItems&            aFrameItems);

  nsresult InsertFirstLineFrames(nsFrameConstructorState& aState,
                                 nsIContent*              aContent,
                                 nsIFrame*                aBlockFrame,
                                 nsContainerFrame**       aParentFrame,
                                 nsIFrame*                aPrevSibling,
                                 nsFrameItems&            aFrameItems);

  














  nsIFrame* FindFrameForContentSibling(nsIContent* aContent,
                                       nsIContent* aTargetContent,
                                       uint8_t& aTargetContentDisplay,
                                       nsContainerFrame* aParentFrame,
                                       bool aPrevSibling);

  













  nsIFrame* FindPreviousSibling(mozilla::dom::FlattenedChildIterator aIter,
                                nsIContent* aTargetContent,
                                uint8_t& aTargetContentDisplay,
                                nsContainerFrame* aParentFrame);

  













  nsIFrame* FindNextSibling(mozilla::dom::FlattenedChildIterator aIter,
                            nsIContent* aTargetContent,
                            uint8_t& aTargetContentDisplay,
                            nsContainerFrame* aParentFrame);

  
  
  
  
  
  
  
  
  
  
  
  
  
  nsIFrame* GetInsertionPrevSibling(InsertionPoint* aInsertion, 
                                    nsIContent* aChild,
                                    bool* aIsAppend,
                                    bool* aIsRangeInsertSafe,
                                    nsIContent* aStartSkipChild = nullptr,
                                    nsIContent *aEndSkipChild = nullptr);

  



  nsContainerFrame* GetContentInsertionFrameFor(nsIContent* aContent);

  
  
  
  
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

  






  void SetAsUndisplayedContent(nsFrameConstructorState& aState,
                               FrameConstructionItemList& aList,
                               nsIContent* aContent,
                               nsStyleContext* aStyleContext,
                               bool aIsGeneratedContent);
  
  void ConstructAnonymousContentForCanvas(nsFrameConstructorState& aState,
                                          nsIFrame* aFrame,
                                          nsIContent* aDocElement);

public:

  friend class nsFrameConstructorState;

private:

  nsIDocument*        mDocument;  

  
  

  
  nsContainerFrame*   mRootElementFrame;
  
  nsIFrame*           mRootElementStyleFrame;
  
  
  nsContainerFrame*   mDocElementContainingBlock;
  nsIFrame*           mGfxScrollFrame;
  nsIFrame*           mPageSequenceFrame;
  nsQuoteList         mQuoteList;
  nsCounterManager    mCounterManager;
  
  uint16_t            mCurrentDepth;
  uint16_t            mUpdateCount;
  bool                mQuotesDirty : 1;
  bool                mCountersDirty : 1;
  bool                mIsDestroyingFrameTree : 1;
  
  bool                mHasRootAbsPosContainingBlock : 1;
  bool                mAlwaysCreateFramesForIgnorableWhitespace : 1;

  nsCOMPtr<nsILayoutHistoryState> mTempFrameTreeState;
};

#endif 
