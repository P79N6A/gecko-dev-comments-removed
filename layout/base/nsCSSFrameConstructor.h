









































#ifndef nsCSSFrameConstructor_h___
#define nsCSSFrameConstructor_h___

#include "nsCOMPtr.h"
#include "nsILayoutHistoryState.h"
#include "nsIXBLService.h"
#include "nsQuoteList.h"
#include "nsCounterManager.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsThreadUtils.h"
#include "nsPageContentFrame.h"

class nsIDocument;
struct nsFrameItems;
struct nsAbsoluteItems;
class nsStyleContext;
struct nsStyleContent;
struct nsStyleDisplay;
class nsIPresShell;
class nsFrameManager;
class nsIDOMHTMLSelectElement;
class nsPresContext;
class nsStyleChangeList;
class nsIFrame;
struct nsGenConInitializer;
class ChildIterator;
class nsICSSAnonBoxPseudo;

struct nsFindFrameHint
{
  nsIFrame *mPrimaryFrameForPrevSibling;  
  nsFindFrameHint() : mPrimaryFrameForPrevSibling(nsnull) { }
};

typedef void (nsLazyFrameConstructionCallback)
             (nsIContent* aContent, nsIFrame* aFrame, void* aArg);

class nsFrameConstructorState;
class nsFrameConstructorSaveState;
  

struct nsFrameItems : public nsFrameList {
  nsIFrame* lastChild;
  
  nsFrameItems(nsIFrame* aFrame = nsnull);

  nsFrameItems(const nsFrameList& aList, nsIFrame* aLastChild) :
    nsFrameList(aList),
    lastChild(aLastChild)
  {
    NS_ASSERTION(LastChild() == lastChild, "Bogus aLastChild");
  }

  
  void AddChild(nsIFrame* aChild);

  void InsertFrame(nsIFrame* aParent, nsIFrame* aPrevSibling,
                   nsIFrame* aNewFrame) {
    nsFrameList::InsertFrame(aParent, aPrevSibling, aNewFrame);
    if (aPrevSibling == lastChild) {
      lastChild = aNewFrame;
    }
  }

  void InsertFrames(nsIFrame* aParent, nsIFrame* aPrevSibling,
                    nsFrameItems& aFrames) {
    nsFrameList::InsertFrames(aParent, aPrevSibling, aFrames);
    if (aPrevSibling == lastChild) {
      lastChild = aFrames.lastChild;
    }
  }

  void DestroyFrame(nsIFrame* aFrameToDestroy, nsIFrame* aPrevSibling) {
    NS_PRECONDITION((!aPrevSibling && aFrameToDestroy == FirstChild()) ||
                    aPrevSibling->GetNextSibling() == aFrameToDestroy,
                    "Unexpected prevsibling");
    nsFrameList::DestroyFrame(aFrameToDestroy, aPrevSibling);
    if (aFrameToDestroy == lastChild) {
      lastChild = aPrevSibling;
    }
  }

  PRBool RemoveFrame(nsIFrame* aFrameToRemove, nsIFrame* aPrevSibling) {
    NS_PRECONDITION(!aPrevSibling ||
                    aPrevSibling->GetNextSibling() == aFrameToRemove,
                    "Unexpected aPrevSibling");
    if (!aPrevSibling) {
      aPrevSibling = GetPrevSiblingFor(aFrameToRemove);
    }

    PRBool removed = nsFrameList::RemoveFrame(aFrameToRemove, aPrevSibling);

    if (aFrameToRemove == lastChild) {
      lastChild = aPrevSibling;
    }

    return removed;
  }

  nsFrameItems ExtractHead(FrameLinkEnumerator& aLink) {
    nsIFrame* newLastChild = aLink.PrevFrame();
    if (lastChild && aLink.NextFrame() == lastChild) {
      lastChild = nsnull;
    }
    return nsFrameItems(nsFrameList::ExtractHead(aLink),
                        newLastChild);
  }

  nsFrameItems ExtractTail(FrameLinkEnumerator& aLink) {
    nsIFrame* newLastChild = lastChild;
    lastChild = aLink.PrevFrame();
    return nsFrameItems(nsFrameList::ExtractTail(aLink),
                        newLastChild);
  }

  void Clear() {
    nsFrameList::Clear();
    lastChild = nsnull;
  }

private:
  
  void SetFrames(nsIFrame* aFrameList);
  void AppendFrames(nsIFrame* aParent, nsIFrame* aFrameList);
  Slice AppendFrames(nsIFrame* aParent, nsFrameList& aFrameList);
  void AppendFrame(nsIFrame* aParent, nsIFrame* aFrame);
  PRBool RemoveFirstChild();
  void InsertFrames(nsIFrame* aParent, nsIFrame* aPrevSibling,
                    nsIFrame* aFrameList);
  void SortByContentOrder();
};

class nsCSSFrameConstructor
{
public:
  nsCSSFrameConstructor(nsIDocument *aDocument, nsIPresShell* aPresShell);
  ~nsCSSFrameConstructor(void) {
    NS_ASSERTION(mUpdateCount == 0, "Dying in the middle of our own update?");
  }

  
  static nsIXBLService * GetXBLService();
  static void ReleaseGlobals() { NS_IF_RELEASE(gXBLService); }

  
  static void GetAlternateTextFor(nsIContent*    aContent,
                                  nsIAtom*       aTag,  
                                  nsXPIDLString& aAltText);
private: 
  
  nsCSSFrameConstructor(const nsCSSFrameConstructor& aCopy); 
  nsCSSFrameConstructor& operator=(const nsCSSFrameConstructor& aCopy); 

public:
  
  nsresult ConstructRootFrame(nsIFrame** aNewFrame);

  nsresult ReconstructDocElementHierarchy();

  nsresult ContentAppended(nsIContent*     aContainer,
                           PRInt32         aNewIndexInContainer);

  nsresult ContentInserted(nsIContent*            aContainer,
                           nsIContent*            aChild,
                           PRInt32                aIndexInContainer,
                           nsILayoutHistoryState* aFrameState);

  enum RemoveFlags { REMOVE_CONTENT, REMOVE_FOR_RECONSTRUCTION };
  nsresult ContentRemoved(nsIContent* aContainer,
                          nsIContent* aChild,
                          PRInt32     aIndexInContainer,
                          RemoveFlags aFlags,
                          PRBool*     aDidReconstruct);

  nsresult CharacterDataChanged(nsIContent* aContent,
                                CharacterDataChangeInfo* aInfo);

  nsresult ContentStatesChanged(nsIContent*     aContent1,
                                nsIContent*     aContent2,
                                PRInt32         aStateMask);

  
  
  
  
  
  
  
  
  
  nsresult AddLazyChildren(nsIContent* aContent,
                           nsLazyFrameConstructionCallback* aCallback,
                           void* aArg, PRBool aIsSynch = PR_FALSE);

  
  
  void NotifyDestroyingFrame(nsIFrame* aFrame);

  nsresult AttributeChanged(nsIContent* aContent,
                            PRInt32     aNameSpaceID,
                            nsIAtom*    aAttribute,
                            PRInt32     aModType,
                            PRUint32    aStateMask);

  void BeginUpdate();
  void EndUpdate();
  void RecalcQuotesAndCounters();

  
  
  void WillDestroyFrameTree();

  
  
  PRUint32 GetHoverGeneration() const { return mHoverGeneration; }

  
  
  
  
  
  nsresult ProcessRestyledFrames(nsStyleChangeList& aRestyleArray);

private:

  
  
  
  
  
  void ProcessOneRestyle(nsIContent* aContent, nsReStyleHint aRestyleHint,
                         nsChangeHint aChangeHint);

public:
  
  
  
  void RestyleForInsertOrChange(nsIContent* aContainer,
                                nsIContent* aChild);
  
  
  
  
  void RestyleForRemove(nsIContent* aContainer, nsIContent* aOldChild,
                        PRInt32 aIndexInContainer);
  
  
  void RestyleForAppend(nsIContent* aContainer,
                        PRInt32 aNewIndexInContainer);

  
  
  
  
  
  void ProcessPendingRestyles();
  
  
  
  
  void RebuildAllStyleData(nsChangeHint aExtraHint);

  void PostRestyleEvent(nsIContent* aContent, nsReStyleHint aRestyleHint,
                        nsChangeHint aMinChangeHint);
private:
  void PostRestyleEventInternal();
public:

  









  void PostRebuildAllStyleDataEvent(nsChangeHint aExtraHint);

  
  nsresult CreateContinuingFrame(nsPresContext* aPresContext,
                                 nsIFrame*       aFrame,
                                 nsIFrame*       aParentFrame,
                                 nsIFrame**      aContinuingFrame,
                                 PRBool          aIsFluid = PR_TRUE);

  
  nsresult ReplicateFixedFrames(nsPageContentFrame* aParentFrame);

  
  
  
  nsresult FindPrimaryFrameFor(nsFrameManager*  aFrameManager,
                               nsIContent*      aContent,
                               nsIFrame**       aFrame,
                               nsFindFrameHint* aHint);

  
  nsresult GetInsertionPoint(nsIFrame*     aParentFrame,
                             nsIContent*   aChildContent,
                             nsIFrame**    aInsertionPoint,
                             PRBool*       aMultiple = nsnull);

  nsresult CreateListBoxContent(nsPresContext* aPresContext,
                                nsIFrame*       aParentFrame,
                                nsIFrame*       aPrevFrame,
                                nsIContent*     aChild,
                                nsIFrame**      aResult,
                                PRBool          aIsAppend,
                                PRBool          aIsScrollbar,
                                nsILayoutHistoryState* aFrameState);

  nsresult RemoveMappingsForFrameSubtree(nsIFrame* aRemovedFrame);

  
  
  
  nsIFrame* GetRootElementFrame() { return mRootElementFrame; }
  
  
  nsIFrame* GetRootElementStyleFrame() { return mRootElementStyleFrame; }
  nsIFrame* GetPageSequenceFrame() { return mPageSequenceFrame; }

  
  nsIFrame* GetDocElementContainingBlock()
    { return mDocElementContainingBlock; }

private:
  struct FrameConstructionItem;
  class FrameConstructionItemList;

  nsresult ConstructPageFrame(nsIPresShell*  aPresShell, 
                              nsPresContext* aPresContext,
                              nsIFrame*      aParentFrame,
                              nsIFrame*      aPrevPageFrame,
                              nsIFrame*&     aPageFrame,
                              nsIFrame*&     aCanvasFrame);

  void DoContentStateChanged(nsIContent*     aContent,
                             PRInt32         aStateMask);

  
  void RestyleElement(nsIContent*     aContent,
                      nsIFrame*       aPrimaryFrame,
                      nsChangeHint    aMinHint);

  void RestyleLaterSiblings(nsIContent*     aContent);

  nsresult InitAndRestoreFrame (const nsFrameConstructorState& aState,
                                nsIContent*                    aContent,
                                nsIFrame*                      aParentFrame,
                                nsIFrame*                      aPrevInFlow,
                                nsIFrame*                      aNewFrame,
                                PRBool                         aAllowCounters = PR_TRUE);

  already_AddRefed<nsStyleContext>
  ResolveStyleContext(nsIFrame*         aParentFrame,
                      nsIContent*       aContent);
  already_AddRefed<nsStyleContext>
  ResolveStyleContext(nsStyleContext* aParentStyleContext,
                      nsIContent* aContent);

  
  
  
  nsresult ConstructFrame(nsFrameConstructorState& aState,
                          nsIContent*              aContent,
                          nsIFrame*                aParentFrame,
                          nsFrameItems&            aFrameItems);

  
  
  
  
  
  
  
  void AddFrameConstructionItems(nsFrameConstructorState& aState,
                                 nsIContent*              aContent,
                                 PRInt32                  aContentIndex,
                                 nsIFrame*                aParentFrame,
                                 FrameConstructionItemList& aItems);

  
  
  
  nsresult ConstructDocElementFrame(nsIContent*              aDocElement,
                                    nsILayoutHistoryState*   aFrameState,
                                    nsIFrame**               aNewFrame);

  
  
  nsresult SetUpDocElementContainingBlock(nsIContent* aDocElement);

  













  nsresult CreateAttributeContent(nsIContent* aParentContent,
                                  nsIFrame* aParentFrame,
                                  PRInt32 aAttrNamespace,
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
                                                      PRUint32        aContentIndex);

  
  void CreateGeneratedContentItem(nsFrameConstructorState& aState,
                                  nsIFrame*                aFrame,
                                  nsIContent*              aContent,
                                  nsStyleContext*          aStyleContext,
                                  nsIAtom*                 aPseudoElement,
                                  FrameConstructionItemList& aItems);

  
  
  
  
  
  nsresult AppendFrames(nsFrameConstructorState&       aState,
                        nsIFrame*                      aParentFrame,
                        nsFrameItems&                  aFrameList,
                        nsIFrame*                      aPrevSibling);

  
  



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
  (((PRUint32)(_type)) << FCDATA_PARENT_TYPE_OFFSET)

  
  static ParentType GetParentType(nsIFrame* aParentFrame) {
    return GetParentType(aParentFrame->GetType());
  }

  
  static ParentType GetParentType(nsIAtom* aFrameType);

  






  typedef nsIFrame* (* FrameCreationFunc)(nsIPresShell*, nsStyleContext*);

  





  struct FrameConstructionData;
  typedef const FrameConstructionData*
    (* FrameConstructionDataGetter)(nsIContent*, nsStyleContext*);

  


















  typedef nsresult
    (nsCSSFrameConstructor::* FrameFullConstructor)(nsFrameConstructorState& aState,
                                                    FrameConstructionItem& aItem,
                                                    nsIFrame* aParentFrame,
                                                    const nsStyleDisplay* aStyleDisplay,
                                                    nsFrameItems& aFrameItems,
                                                    nsIFrame** aFrame);

  

  



#define FCDATA_SKIP_FRAMEMAP 0x1
  


#define FCDATA_FUNC_IS_DATA_GETTER 0x2
  


#define FCDATA_FUNC_IS_FULL_CTOR 0x4
  



#define FCDATA_DISALLOW_OUT_OF_FLOW 0x8
  



#define FCDATA_FORCE_NULL_ABSPOS_CONTAINER 0x10
#ifdef MOZ_MATHML
  


#define FCDATA_WRAP_KIDS_IN_BLOCKS 0x20
#endif 
  


#define FCDATA_SUPPRESS_FRAME 0x40
  



#define FCDATA_MAY_NEED_SCROLLFRAME 0x80
#ifdef MOZ_XUL
  

#define FCDATA_IS_POPUP 0x100
#endif 
  

#define FCDATA_SKIP_ABSPOS_PUSH 0x200
  



#define FCDATA_FORCE_VIEW 0x400
  


#define FCDATA_DISALLOW_GENERATED_CONTENT 0x800
  


#define FCDATA_IS_TABLE_PART 0x1000
  

#define FCDATA_IS_INLINE 0x2000
  

#define FCDATA_IS_LINE_PARTICIPANT 0x4000
  

#define FCDATA_IS_LINE_BREAK 0x8000
  

#define FCDATA_ALLOW_BLOCK_STYLES 0x10000
  



#define FCDATA_USE_CHILD_ITEMS 0x20000

  

  struct FrameConstructionData {
    
    PRUint32 mBits;
    
    
    
    
    
    
    
    union Func {
      FrameCreationFunc mCreationFunc;
      FrameConstructionDataGetter mDataGetter;
    } mFunc;
    FrameFullConstructor mFullConstructor;
  };

  



  struct FrameConstructionDataByTag {
    
    
    const nsIAtom * const * const mTag;
    const FrameConstructionData mData;
  };

  

  struct FrameConstructionDataByInt {
    
    const PRInt32 mInt;
    const FrameConstructionData mData;
  };

  

  struct PseudoParentData {
    const FrameConstructionData mFCData;
    nsICSSAnonBoxPseudo * const * const mPseudoType;
  };
  

  static const PseudoParentData sPseudoParentData[eParentTypeCount];

  




  static const FrameConstructionData*
    FindDataByInt(PRInt32 aInt, nsIContent* aContent,
                  nsStyleContext* aStyleContext,
                  const FrameConstructionDataByInt* aDataPtr,
                  PRUint32 aDataLength);

  




  static const FrameConstructionData*
    FindDataByTag(nsIAtom* aTag, nsIContent* aContent,
                  nsStyleContext* aStyleContext,
                  const FrameConstructionDataByTag* aDataPtr,
                  PRUint32 aDataLength);

  
  class FrameConstructionItemList {
  public:
    FrameConstructionItemList() :
      mInlineCount(0),
      mLineParticipantCount(0),
      mItemCount(0),
      mLineBoundaryAtStart(PR_FALSE),
      mLineBoundaryAtEnd(PR_FALSE),
      mParentHasNoXBLChildren(PR_FALSE)
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

      
      
    }

    void SetLineBoundaryAtStart(PRBool aBoundary) { mLineBoundaryAtStart = aBoundary; }
    void SetLineBoundaryAtEnd(PRBool aBoundary) { mLineBoundaryAtEnd = aBoundary; }
    void SetParentHasNoXBLChildren(PRBool aHasNoXBLChildren) {
      mParentHasNoXBLChildren = aHasNoXBLChildren;
    }
    PRBool HasLineBoundaryAtStart() { return mLineBoundaryAtStart; }
    PRBool HasLineBoundaryAtEnd() { return mLineBoundaryAtEnd; }
    PRBool ParentHasNoXBLChildren() { return mParentHasNoXBLChildren; }
    PRBool IsEmpty() const { return PR_CLIST_IS_EMPTY(&mItems); }
    PRBool AnyItemsNeedBlockParent() const { return mLineParticipantCount != 0; }
    PRBool AreAllItemsInline() const { return mInlineCount == mItemCount; }
    PRBool IsStartInline() const {
      NS_ASSERTION(!IsEmpty(), "Someone forgot to check IsEmpty()");
      return ToItem(PR_LIST_HEAD(&mItems))->mHasInlineEnds;
    }
    PRBool IsEndInline() const {
      NS_ASSERTION(!IsEmpty(), "Someone forgot to check IsEmpty()");
      return ToItem(PR_LIST_TAIL(&mItems))->mHasInlineEnds;
    }
    PRBool AllWantParentType(ParentType aDesiredParentType) const {
      return mDesiredParentCounts[aDesiredParentType] == mItemCount;
    }

    
    
    
    FrameConstructionItem* AppendItem(const FrameConstructionData* aFCData,
                                      nsIContent* aContent,
                                      nsIAtom* aTag,
                                      PRInt32 aNameSpaceID,
                                      PRInt32 aContentIndex,
                                      already_AddRefed<nsStyleContext> aStyleContext)
    {
      FrameConstructionItem* item =
        new FrameConstructionItem(aFCData, aContent, aTag, aNameSpaceID,
                                  aContentIndex, aStyleContext);
      if (item) {
        PR_APPEND_LINK(item, &mItems);
        ++mItemCount;
        ++mDesiredParentCounts[item->DesiredParentType()];
      } else {
        
        nsRefPtr<nsStyleContext> sc(aStyleContext);
      }
      return item;
    }

    void InlineItemAdded() { ++mInlineCount; }
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

      PRBool operator==(const Iterator& aOther) const {
        NS_ASSERTION(mEnd == aOther.mEnd, "Iterators for different lists?");
        return mCurrent == aOther.mCurrent;
      }
      PRBool operator!=(const Iterator& aOther) const {
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
      PRBool IsDone() const { return mCurrent == mEnd; }
      PRBool AtStart() const { return mCurrent == PR_NEXT_LINK(mEnd); }
      void Next() {
        NS_ASSERTION(!IsDone(), "Should have checked IsDone()!");
        mCurrent = PR_NEXT_LINK(mCurrent);
      }
      void Prev() {
        NS_ASSERTION(!AtStart(), "Should have checked AtStart()!");
        mCurrent = PR_PREV_LINK(mCurrent);
      }
      void SetToEnd() { mCurrent = mEnd; }

      
      
      
      inline PRBool SkipItemsWantingParentType(ParentType aParentType);

      
      
      
      inline PRBool SkipWhitespace();

      
      
      
      
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

    
    
    void AdjustCountsForItem(FrameConstructionItem* aItem, PRInt32 aDelta);

    PRCList mItems;
    PRUint32 mInlineCount;
    PRUint32 mLineParticipantCount;
    PRUint32 mItemCount;
    PRUint32 mDesiredParentCounts[eParentTypeCount];
    
    
    PRPackedBool mLineBoundaryAtStart;
    
    
    PRPackedBool mLineBoundaryAtEnd;
    
    PRPackedBool mParentHasNoXBLChildren;
  };

  typedef FrameConstructionItemList::Iterator FCItemIterator;

  



  struct FrameConstructionItem : public PRCList {
    
    
    FrameConstructionItem(const FrameConstructionData* aFCData,
                          nsIContent* aContent,
                          nsIAtom* aTag,
                          PRInt32 aNameSpaceID,
                          PRInt32 aContentIndex,
                          already_AddRefed<nsStyleContext> aStyleContext) :
      mFCData(aFCData), mContent(aContent), mTag(aTag),
      mNameSpaceID(aNameSpaceID), mContentIndex(aContentIndex),
      mStyleContext(aStyleContext),
      mIsText(PR_FALSE), mIsGeneratedContent(PR_FALSE),
      mIsRootPopupgroup(PR_FALSE), mIsAllInline(PR_FALSE),
      mHasInlineEnds(PR_FALSE), mIsPopup(PR_FALSE),
      mIsLineParticipant(PR_FALSE)
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

    
    
    
    PRBool IsWhitespace() const;

    PRBool IsLineBoundary() const {
      return !mHasInlineEnds || (mFCData->mBits & FCDATA_IS_LINE_BREAK);
    }

    
    const FrameConstructionData* mFCData;
    
    nsIContent* mContent;
    
    nsIAtom* mTag;
    
    PRInt32 mNameSpaceID;
    
    
    PRInt32 mContentIndex;
    
    nsRefPtr<nsStyleContext> mStyleContext;
    
    PRPackedBool mIsText;
    
    
    PRPackedBool mIsGeneratedContent;
    
    PRPackedBool mIsRootPopupgroup;
    
    
    PRPackedBool mIsAllInline;
    
    
    
    
    PRPackedBool mHasInlineEnds;
    
    
    PRPackedBool mIsPopup;
    
    PRPackedBool mIsLineParticipant;

    
    FrameConstructionItemList mChildItems;

  private:
    FrameConstructionItem(const FrameConstructionItem& aOther); 
  };

  




  nsresult CreateNeededTablePseudos(FrameConstructionItemList& aItems,
                                    nsIFrame* aParentFrame);

  







  
  
  
  
  void AdjustParentFrame(nsIFrame* &                  aParentFrame,
                         const FrameConstructionData* aFCData,
                         nsStyleContext*              aStyleContext);

  

protected:
  static nsresult CreatePlaceholderFrameFor(nsIPresShell*    aPresShell, 
                                            nsIContent*      aContent,
                                            nsIFrame*        aFrame,
                                            nsStyleContext*  aStyleContext,
                                            nsIFrame*        aParentFrame,
                                            nsIFrame*        aPrevInFlow,
                                            nsIFrame**       aPlaceholderFrame);

private:
  
  
  nsresult ConstructButtonFrame(nsFrameConstructorState& aState,
                                FrameConstructionItem&    aItem,
                                nsIFrame*                aParentFrame,
                                const nsStyleDisplay*    aStyleDisplay,
                                nsFrameItems&            aFrameItems,
                                nsIFrame**               aNewFrame);

  
  
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

  nsresult ConstructTextFrame(const FrameConstructionData* aData,
                              nsFrameConstructorState& aState,
                              nsIContent*              aContent,
                              nsIFrame*                aParentFrame,
                              nsStyleContext*          aStyleContext,
                              nsFrameItems&            aFrameItems);

  
  
  void AddTextItemIfNeeded(nsFrameConstructorState& aState,
                           nsIFrame* aParentFrame,
                           nsIContent* aParentContent,
                           PRInt32 aContentIndex,
                           FrameConstructionItemList& aItems);

  
  
  void ReframeTextIfNeeded(nsIContent* aParentContent,
                           PRInt32 aContentIndex);

  void AddPageBreakItem(nsIContent* aContent,
                        nsStyleContext* aMainStyleContext,
                        FrameConstructionItemList& aItems);

  
  
  
  
  static const FrameConstructionData* FindHTMLData(nsIContent* aContent,
                                                   nsIAtom* aTag,
                                                   PRInt32 aNameSpaceID,
                                                   nsIFrame* aParentFrame,
                                                   nsStyleContext* aStyleContext);
  
  static const FrameConstructionData*
    FindImgData(nsIContent* aContent, nsStyleContext* aStyleContext);
  static const FrameConstructionData*
    FindImgControlData(nsIContent* aContent, nsStyleContext* aStyleContext);
  static const FrameConstructionData*
    FindInputData(nsIContent* aContent, nsStyleContext* aStyleContext);
  static const FrameConstructionData*
    FindObjectData(nsIContent* aContent, nsStyleContext* aStyleContext);

  










  nsresult ConstructFrameFromItemInternal(FrameConstructionItem& aItem,
                                          nsFrameConstructorState& aState,
                                          nsIFrame* aParentFrame,
                                          nsFrameItems& aFrameItems);

  
  
#define ITEM_ALLOW_XBL_BASE 0x1
  

#define ITEM_ALLOW_PAGE_BREAK 0x2
  
#define ITEM_IS_GENERATED_CONTENT 0x4
  
  
  
  void AddFrameConstructionItemsInternal(nsFrameConstructorState& aState,
                                         nsIContent*              aContent,
                                         nsIFrame*                aParentFrame,
                                         nsIAtom*                 aTag,
                                         PRInt32                  aNameSpaceID,
                                         PRInt32                  aContentIndex,
                                         nsStyleContext*          aStyleContext,
                                         PRUint32                 aFlags,
                                         FrameConstructionItemList& aItems);

  



  nsresult ConstructFramesFromItemList(nsFrameConstructorState& aState,
                                       FrameConstructionItemList& aItems,
                                       nsIFrame* aParentFrame,
                                       nsFrameItems& aFrameItems);
  nsresult ConstructFramesFromItem(nsFrameConstructorState& aState,
                                   FCItemIterator& aItem,
                                   nsIFrame* aParentFrame,
                                   nsFrameItems& aFrameItems);
  static PRBool AtLineBoundary(FCItemIterator& aIter);

  nsresult CreateAnonymousFrames(nsFrameConstructorState& aState,
                                 nsIContent*              aParent,
                                 nsIFrame*                aParentFrame,
                                 nsFrameItems&            aChildItems);

  nsresult GetAnonymousContent(nsIContent* aParent,
                               nsIFrame* aParentFrame,
                               nsTArray<nsIContent*>& aAnonContent);


#ifdef MOZ_MATHML
  




  nsresult FlushAccumulatedBlock(nsFrameConstructorState& aState,
                                 nsIContent* aContent,
                                 nsIFrame* aParentFrame,
                                 nsFrameItems* aBlockItems,
                                 nsFrameItems* aNewItems);

  
  
  static const FrameConstructionData* FindMathMLData(nsIContent* aContent,
                                                     nsIAtom* aTag,
                                                     PRInt32 aNameSpaceID,
                                                     nsStyleContext* aStyleContext);
#endif

  
  
  static const FrameConstructionData* FindXULTagData(nsIContent* aContent,
                                                     nsIAtom* aTag,
                                                     PRInt32 aNameSpaceID,
                                                     nsStyleContext* aStyleContext);
  
#ifdef MOZ_XUL
  static const FrameConstructionData*
    FindPopupGroupData(nsIContent* aContent, nsStyleContext* aStyleContext);
  
  static const FrameConstructionData sXULTextBoxData;
  static const FrameConstructionData*
    FindXULLabelData(nsIContent* aContent, nsStyleContext* aStyleContext);
  static const FrameConstructionData*
    FindXULDescriptionData(nsIContent* aContent, nsStyleContext* aStyleContext);
#ifdef XP_MACOSX
  static const FrameConstructionData*
    FindXULMenubarData(nsIContent* aContent, nsStyleContext* aStyleContext);
#endif 
  static const FrameConstructionData*
    FindXULListBoxBodyData(nsIContent* aContent, nsStyleContext* aStyleContext);
  static const FrameConstructionData*
    FindXULListItemData(nsIContent* aContent, nsStyleContext* aStyleContext);
#endif 

  
  
  
  
  
  static const FrameConstructionData*
    FindXULDisplayData(const nsStyleDisplay* aDisplay,
                       nsIContent* aContent,
                       nsStyleContext* aStyleContext);


#ifdef MOZ_SVG
  static const FrameConstructionData* FindSVGData(nsIContent* aContent,
                                                  nsIAtom* aTag,
                                                  PRInt32 aNameSpaceID,
                                                  nsIFrame* aParentFrame,
                                                  nsStyleContext* aStyleContext);

  nsresult ConstructSVGForeignObjectFrame(nsFrameConstructorState& aState,
                                          FrameConstructionItem&   aItem,
                                          nsIFrame* aParentFrame,
                                          const nsStyleDisplay* aStyleDisplay,
                                          nsFrameItems& aFrameItems,
                                          nsIFrame** aNewFrame);
#endif

  

  const FrameConstructionData*
    FindDisplayData(const nsStyleDisplay* aDisplay, nsIContent* aContent,
                    nsStyleContext* aStyleContext);

  


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
                           const PRBool             aCanHaveGeneratedContent,
                           nsFrameItems&            aFrameItems,
                           const PRBool             aAllowBlockStyles);

  nsIFrame* GetFrameFor(nsIContent* aContent);

  







public:
  nsIFrame* GetAbsoluteContainingBlock(nsIFrame* aFrame);
private:
  nsIFrame* GetFloatContainingBlock(nsIFrame* aFrame);

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
                           PRBool                   aIsRoot,
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
                        PRBool                   aBuildCombobox,
                        nsFrameItems&            aFrameItems);

  nsresult MaybeRecreateFramesForContent(nsIContent* aContent);

  
  
  nsresult RecreateFramesForContent(nsIContent* aContent, PRBool aAsyncInsert);

  
  
  
  
  
  
  
  
  PRBool MaybeRecreateContainerForFrameRemoval(nsIFrame* aFrame,
                                               nsresult* aResult);

  nsresult CreateContinuingOuterTableFrame(nsIPresShell*    aPresShell, 
                                           nsPresContext*  aPresContext,
                                           nsIFrame*        aFrame,
                                           nsIFrame*        aParentFrame,
                                           nsIContent*      aContent,
                                           nsStyleContext*  aStyleContext,
                                           nsIFrame**       aContinuingFrame);

  nsresult CreateContinuingTableFrame(nsIPresShell*    aPresShell, 
                                      nsPresContext*  aPresContext,
                                      nsIFrame*        aFrame,
                                      nsIFrame*        aParentFrame,
                                      nsIContent*      aContent,
                                      nsStyleContext*  aStyleContext,
                                      nsIFrame**       aContinuingFrame);

  

  

  already_AddRefed<nsStyleContext>
  GetFirstLetterStyle(nsIContent*      aContent,
                      nsStyleContext*  aStyleContext);

  already_AddRefed<nsStyleContext>
  GetFirstLineStyle(nsIContent*      aContent,
                    nsStyleContext*  aStyleContext);

  PRBool ShouldHaveFirstLetterStyle(nsIContent*      aContent,
                                    nsStyleContext*  aStyleContext);

  
  
  PRBool HasFirstLetterStyle(nsIFrame* aBlockFrame);

  PRBool ShouldHaveFirstLineStyle(nsIContent*      aContent,
                                  nsStyleContext*  aStyleContext);

  void ShouldHaveSpecialBlockStyle(nsIContent*      aContent,
                                   nsStyleContext*  aStyleContext,
                                   PRBool*          aHaveFirstLetterStyle,
                                   PRBool*          aHaveFirstLineStyle);

  
  
  
  
  
  
  
  
  
  
  
  nsresult ConstructBlock(nsFrameConstructorState& aState,
                          const nsStyleDisplay*    aDisplay,
                          nsIContent*              aContent,
                          nsIFrame*                aParentFrame,
                          nsIFrame*                aContentParentFrame,
                          nsStyleContext*          aStyleContext,
                          nsIFrame**               aNewFrame,
                          nsFrameItems&            aFrameItems,
                          PRBool                   aAbsPosContainer);

  nsresult ConstructInline(nsFrameConstructorState& aState,
                           FrameConstructionItem&   aItem,
                           nsIFrame*                aParentFrame,
                           const nsStyleDisplay*    aDisplay,
                           nsFrameItems&            aFrameItems,
                           nsIFrame**               aNewFrame);

  











  void MoveFramesToEndOfIBSplit(nsFrameConstructorState& aState,
                                nsIFrame* aExistingEndFrame,
                                nsFrameItems& aFramesToMove,
                                nsIFrame* aBlockPart,
                                nsFrameConstructorState* aTargetState);

  



  void BuildInlineChildItems(nsFrameConstructorState& aState,
                             FrameConstructionItem& aParentItem);

  
  
  
  
  
  
  
  
  
  
  PRBool WipeContainingBlock(nsFrameConstructorState& aState,
                             nsIFrame*                aContainingBlock,
                             nsIFrame*                aFrame,
                             FrameConstructionItemList& aItems,
                             PRBool                   aIsAppend,
                             nsIFrame*                aPrevSibling);

  nsresult ReframeContainingBlock(nsIFrame* aFrame);

  nsresult StyleChangeReflow(nsIFrame* aFrame, nsChangeHint aHint);

  








  nsIFrame* FindFrameWithContent(nsFrameManager*  aFrameManager,
                                 nsIFrame*        aParentFrame,
                                 nsIContent*      aParentContent,
                                 nsIContent*      aContent,
                                 nsFindFrameHint* aHint);

  

  

  void CreateFloatingLetterFrame(nsFrameConstructorState& aState,
                                 nsIFrame*                aBlockFrame,
                                 nsIContent*              aTextContent,
                                 nsIFrame*                aTextFrame,
                                 nsIContent*              aBlockContent,
                                 nsIFrame*                aParentFrame,
                                 nsStyleContext*          aStyleContext,
                                 nsFrameItems&            aResult);

  nsresult CreateLetterFrame(nsIFrame*                aBlockFrame,
                             nsIContent*              aTextContent,
                             nsIFrame*                aParentFrame,
                             nsFrameItems&            aResult);

  nsresult WrapFramesInFirstLetterFrame(nsIContent*   aBlockContent,
                                        nsIFrame*     aBlockFrame,
                                        nsFrameItems& aBlockFrames);

  nsresult WrapFramesInFirstLetterFrame(nsIFrame*     aBlockFrame,
                                        nsIFrame*     aParentFrame,
                                        nsIFrame*     aParentFrameList,
                                        nsIFrame**    aModifiedParent,
                                        nsIFrame**    aTextFrame,
                                        nsIFrame**    aPrevFrame,
                                        nsFrameItems& aLetterFrame,
                                        PRBool*       aStopLooking);

  nsresult RecoverLetterFrames(nsIFrame* aBlockFrame);

  
  nsresult RemoveLetterFrames(nsPresContext*  aPresContext,
                              nsIPresShell*    aPresShell,
                              nsFrameManager*  aFrameManager,
                              nsIFrame*        aBlockFrame);

  
  nsresult RemoveFirstLetterFrames(nsPresContext*  aPresContext,
                                   nsIPresShell*    aPresShell,
                                   nsFrameManager*  aFrameManager,
                                   nsIFrame*        aFrame,
                                   PRBool*          aStopLooking);

  
  nsresult RemoveFloatingFirstLetterFrames(nsPresContext*  aPresContext,
                                           nsIPresShell*    aPresShell,
                                           nsFrameManager*  aFrameManager,
                                           nsIFrame*        aBlockFrame,
                                           PRBool*          aStopLooking);

  
  
  nsresult CaptureStateForFramesOf(nsIContent* aContent,
                                   nsILayoutHistoryState* aHistoryState);

  
  nsresult CaptureStateFor(nsIFrame*              aFrame,
                           nsILayoutHistoryState* aHistoryState);

  

  

  
  
  
  
  
  
  
  nsresult WrapFramesInFirstLineFrame(nsFrameConstructorState& aState,
                                      nsIContent*              aBlockContent,
                                      nsIFrame*                aBlockFrame,
                                      nsIFrame*                aLineFrame,
                                      nsFrameItems&            aFrameItems);

  
  
  nsresult AppendFirstLineFrames(nsFrameConstructorState& aState,
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
                                       PRUint8& aTargetContentDisplay,
                                       PRBool aPrevSibling);

  
  
  
  nsIFrame* FindPreviousSibling(const ChildIterator& aFirst,
                                ChildIterator aIter);

  
  
  
  nsIFrame* FindNextSibling(ChildIterator aIter,
                            const ChildIterator& aLast);

  
  
  
  
  
  nsIFrame* GetInsertionPrevSibling(nsIFrame*& aParentFrame, 
                                    nsIContent* aContainer,
                                    nsIContent* aChild,
                                    PRInt32 aIndexInContainer,
                                    PRBool* aIsAppend);

  
  
  
  
  PRBool IsValidSibling(nsIFrame*              aSibling,
                        nsIContent*            aContent,
                        PRUint8&               aDisplay);
  
  void QuotesDirty() {
    NS_PRECONDITION(mUpdateCount != 0, "Instant quote updates are bad news");
    mQuotesDirty = PR_TRUE;
  }

  void CountersDirty() {
    NS_PRECONDITION(mUpdateCount != 0, "Instant counter updates are bad news");
    mCountersDirty = PR_TRUE;
  }

public:
  struct RestyleData;
  friend struct RestyleData;

  struct RestyleData {
    nsReStyleHint mRestyleHint;  
    nsChangeHint  mChangeHint;   
  };

  struct RestyleEnumerateData : public RestyleData {
    nsCOMPtr<nsIContent> mContent;
  };

  class RestyleEvent;
  friend class RestyleEvent;

  class RestyleEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    RestyleEvent(nsCSSFrameConstructor *aConstructor)
      : mConstructor(aConstructor) {
      NS_PRECONDITION(aConstructor, "Must have a constructor!");
    }
    void Revoke() { mConstructor = nsnull; }
  private:
    nsCSSFrameConstructor *mConstructor;
  };

  friend class nsFrameConstructorState;

private:

  class LazyGenerateChildrenEvent;
  friend class LazyGenerateChildrenEvent;

  
  class LazyGenerateChildrenEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    LazyGenerateChildrenEvent(nsIContent *aContent,
                              nsIPresShell *aPresShell,
                              nsLazyFrameConstructionCallback* aCallback,
                              void* aArg)
      : mContent(aContent), mPresShell(aPresShell), mCallback(aCallback), mArg(aArg)
    {}

  private:
    nsCOMPtr<nsIContent> mContent;
    nsCOMPtr<nsIPresShell> mPresShell;
    nsLazyFrameConstructionCallback* mCallback;
    void* mArg;
  };

  nsIDocument*        mDocument;  
  nsIPresShell*       mPresShell; 

  
  
  
  
  nsIFrame*           mRootElementFrame;
  
  nsIFrame*           mRootElementStyleFrame;
  
  
  nsIFrame*           mFixedContainingBlock;
  
  
  nsIFrame*           mDocElementContainingBlock;
  nsIFrame*           mGfxScrollFrame;
  nsIFrame*           mPageSequenceFrame;
  nsQuoteList         mQuoteList;
  nsCounterManager    mCounterManager;
  PRUint16            mUpdateCount;
  PRPackedBool        mQuotesDirty : 1;
  PRPackedBool        mCountersDirty : 1;
  PRPackedBool        mIsDestroyingFrameTree : 1;
  PRPackedBool        mRebuildAllStyleData : 1;
  
  PRPackedBool        mHasRootAbsPosContainingBlock : 1;
  PRUint32            mHoverGeneration;
  nsChangeHint        mRebuildAllExtraHint;

  nsRevocableEventPtr<RestyleEvent> mRestyleEvent;

  nsCOMPtr<nsILayoutHistoryState> mTempFrameTreeState;

  nsDataHashtable<nsISupportsHashKey, RestyleData> mPendingRestyles;

  static nsIXBLService * gXBLService;
};

#endif 
