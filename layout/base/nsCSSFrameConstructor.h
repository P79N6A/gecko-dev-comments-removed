









































#ifndef nsCSSFrameConstructor_h___
#define nsCSSFrameConstructor_h___

#include "nsCOMPtr.h"
#include "nsILayoutHistoryState.h"
#include "nsIXBLService.h"
#include "nsQuoteList.h"
#include "nsCounterManager.h"
#include "nsHashKeys.h"
#include "nsThreadUtils.h"
#include "nsPageContentFrame.h"
#include "nsCSSPseudoElements.h"
#include "RestyleTracker.h"
#include "nsIAnonymousContentCreator.h"

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
class nsPageContentFrame;
struct PendingBinding;
class nsRefreshDriver;

class nsFrameConstructorState;
class nsFrameConstructorSaveState;

class nsCSSFrameConstructor
{
  friend class nsRefreshDriver;

public:
  typedef mozilla::dom::Element Element;
  typedef mozilla::css::RestyleTracker RestyleTracker;

  nsCSSFrameConstructor(nsIDocument *aDocument, nsIPresShell* aPresShell);
  ~nsCSSFrameConstructor(void) {
    NS_ASSERTION(mUpdateCount == 0, "Dying in the middle of our own update?");
  }

  struct RestyleData;
  friend struct RestyleData;

  
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

  
  
  
  
  void CreateNeededFrames();

private:
  void CreateNeededFrames(nsIContent* aContent);

  enum Operation {
    CONTENTAPPEND,
    CONTENTINSERT
  };

  
  
  PRBool MaybeConstructLazily(Operation aOperation,
                              nsIContent* aContainer,
                              nsIContent* aChild);

  
  
  void IssueSingleInsertNofications(nsIContent* aContainer,
                                    nsIContent* aStartChild,
                                    nsIContent* aEndChild,
                                    PRBool aAllowLazyConstruction);
  
  
  
  
  
  
  nsIFrame* GetRangeInsertionPoint(nsIContent* aContainer,
                                   nsIFrame* aParentFrame,
                                   nsIContent* aStartChild,
                                   nsIContent* aEndChild,
                                   PRBool aAllowLazyConstruction);

  
  PRBool MaybeRecreateForFrameset(nsIFrame* aParentFrame,
                                  nsIContent* aStartChild,
                                  nsIContent* aEndChild);

public:
  











































  
  
  nsresult ContentAppended(nsIContent* aContainer,
                           nsIContent* aFirstNewContent,
                           PRBool      aAllowLazyConstruction);

  
  
  nsresult ContentInserted(nsIContent*            aContainer,
                           nsIContent*            aChild,
                           nsILayoutHistoryState* aFrameState,
                           PRBool                 aAllowLazyConstruction);

  
  
  
  
  
  
  nsresult ContentRangeInserted(nsIContent*            aContainer,
                                nsIContent*            aStartChild,
                                nsIContent*            aEndChild,
                                nsILayoutHistoryState* aFrameState,
                                PRBool                 aAllowLazyConstruction);

  enum RemoveFlags { REMOVE_CONTENT, REMOVE_FOR_RECONSTRUCTION };
  nsresult ContentRemoved(nsIContent* aContainer,
                          nsIContent* aChild,
                          nsIContent* aOldNextSibling,
                          RemoveFlags aFlags,
                          PRBool*     aDidReconstruct);

  nsresult CharacterDataChanged(nsIContent* aContent,
                                CharacterDataChangeInfo* aInfo);

  nsresult ContentStateChanged(nsIContent*   aContent,
                               nsEventStates aStateMask);

  
  nsresult GenerateChildFrames(nsIFrame* aFrame);

  
  
  void NotifyDestroyingFrame(nsIFrame* aFrame);

  void AttributeWillChange(Element* aElement,
                           PRInt32  aNameSpaceID,
                           nsIAtom* aAttribute,
                           PRInt32  aModType);
  void AttributeChanged(Element* aElement,
                        PRInt32  aNameSpaceID,
                        nsIAtom* aAttribute,
                        PRInt32  aModType);

  void BeginUpdate();
  void EndUpdate();
  void RecalcQuotesAndCounters();

  
  
  void WillDestroyFrameTree();

  
  
  PRUint32 GetHoverGeneration() const { return mHoverGeneration; }

  
  
  
  
  
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
    PostRestyleEventCommon(aElement, aRestyleHint, aMinChangeHint, PR_TRUE);
  }
private:
  














  void PostRestyleEventCommon(Element* aElement,
                              nsRestyleHint aRestyleHint,
                              nsChangeHint aMinChangeHint,
                              PRBool aForAnimation);
  void PostRestyleEventInternal(PRBool aForLazyConstruction);
public:

  









  void PostRebuildAllStyleDataEvent(nsChangeHint aExtraHint);

  
  nsresult CreateContinuingFrame(nsPresContext* aPresContext,
                                 nsIFrame*       aFrame,
                                 nsIFrame*       aParentFrame,
                                 nsIFrame**      aContinuingFrame,
                                 PRBool          aIsFluid = PR_TRUE);

  
  nsresult ReplicateFixedFrames(nsPageContentFrame* aParentFrame);

  
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

  
  
  void RestyleElement(Element* aElement,
                      nsIFrame*       aPrimaryFrame,
                      nsChangeHint    aMinHint,
                      RestyleTracker& aRestyleTracker,
                      PRBool          aRestyleDescendants);

  nsresult InitAndRestoreFrame (const nsFrameConstructorState& aState,
                                nsIContent*                    aContent,
                                nsIFrame*                      aParentFrame,
                                nsIFrame*                      aPrevInFlow,
                                nsIFrame*                      aNewFrame,
                                PRBool                         aAllowCounters = PR_TRUE);

  
  
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
                                 PRBool                   aSuppressWhiteSpaceOptimizations,
                                 nsIFrame*                aParentFrame,
                                 FrameConstructionItemList& aItems);

  
  
  
  nsresult ConstructDocElementFrame(Element*                 aDocElement,
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

  
  void CreateGeneratedContentItem(nsFrameConstructorState&   aState,
                                  nsIFrame*                  aFrame,
                                  nsIContent*                aContent,
                                  nsStyleContext*            aStyleContext,
                                  nsCSSPseudoElements::Type  aPseudoElement,
                                  FrameConstructionItemList& aItems);

  
  
  
  
  
  nsresult AppendFrames(nsFrameConstructorState&       aState,
                        nsIFrame*                      aParentFrame,
                        nsFrameItems&                  aFrameList,
                        nsIFrame*                      aPrevSibling,
                        PRBool                         aIsRecursiveCall = PR_FALSE);

  
  



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
#ifdef MOZ_MATHML
  


#define FCDATA_WRAP_KIDS_IN_BLOCKS 0x20
#endif 
  


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
    FindDataByInt(PRInt32 aInt, Element* aElement,
                  nsStyleContext* aStyleContext,
                  const FrameConstructionDataByInt* aDataPtr,
                  PRUint32 aDataLength);

  




  static const FrameConstructionData*
    FindDataByTag(nsIAtom* aTag, Element* aElement,
                  nsStyleContext* aStyleContext,
                  const FrameConstructionDataByTag* aDataPtr,
                  PRUint32 aDataLength);

  
  class FrameConstructionItemList {
  public:
    FrameConstructionItemList() :
      mInlineCount(0),
      mBlockCount(0),
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
    PRBool AreAllItemsBlock() const { return mBlockCount == mItemCount; }
    PRBool AllWantParentType(ParentType aDesiredParentType) const {
      return mDesiredParentCounts[aDesiredParentType] == mItemCount;
    }

    
    
    
    FrameConstructionItem* AppendItem(const FrameConstructionData* aFCData,
                                      nsIContent* aContent,
                                      nsIAtom* aTag,
                                      PRInt32 aNameSpaceID,
                                      PendingBinding* aPendingBinding,
                                      already_AddRefed<nsStyleContext> aStyleContext,
                                      PRBool aSuppressWhiteSpaceOptimizations)
    {
      FrameConstructionItem* item =
        new FrameConstructionItem(aFCData, aContent, aTag, aNameSpaceID,
                                  aPendingBinding, aStyleContext,
                                  aSuppressWhiteSpaceOptimizations);
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

      
      
      
      inline PRBool SkipWhitespace(nsFrameConstructorState& aState);

      
      
      
      
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
    PRUint32 mBlockCount;
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
                          PendingBinding* aPendingBinding,
                          already_AddRefed<nsStyleContext> aStyleContext,
                          PRBool aSuppressWhiteSpaceOptimizations) :
      mFCData(aFCData), mContent(aContent), mTag(aTag),
      mNameSpaceID(aNameSpaceID),
      mPendingBinding(aPendingBinding), mStyleContext(aStyleContext),
      mSuppressWhiteSpaceOptimizations(aSuppressWhiteSpaceOptimizations),
      mIsText(PR_FALSE), mIsGeneratedContent(PR_FALSE),
      mIsRootPopupgroup(PR_FALSE), mIsAllInline(PR_FALSE), mIsBlock(PR_FALSE),
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

    
    
    
    PRBool IsWhitespace(nsFrameConstructorState& aState) const;

    PRBool IsLineBoundary() const {
      return mIsBlock || (mFCData->mBits & FCDATA_IS_LINE_BREAK);
    }

    
    const FrameConstructionData* mFCData;
    
    nsIContent* mContent;
    
    nsIAtom* mTag;
    
    PRInt32 mNameSpaceID;
    
    
    
    
    
    
    
    
    PendingBinding* mPendingBinding;
    
    nsRefPtr<nsStyleContext> mStyleContext;
    
    
    PRPackedBool mSuppressWhiteSpaceOptimizations;
    
    PRPackedBool mIsText;
    
    
    PRPackedBool mIsGeneratedContent;
    
    PRPackedBool mIsRootPopupgroup;
    
    
    
    
    PRPackedBool mIsAllInline;
    
    
    
    
    
    PRPackedBool mIsBlock;
    
    
    
    
    PRPackedBool mHasInlineEnds;
    
    
    PRPackedBool mIsPopup;
    
    PRPackedBool mIsLineParticipant;

    
    FrameConstructionItemList mChildItems;

  private:
    FrameConstructionItem(const FrameConstructionItem& aOther); 
  };

  




  inline nsresult CreateNeededTablePseudos(nsFrameConstructorState& aState,
                                           FrameConstructionItemList& aItems,
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
                                            nsFrameState     aTypeBit,
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
                           nsIContent* aPossibleTextContent,
                           FrameConstructionItemList& aItems);

  
  
  void ReframeTextIfNeeded(nsIContent* aParentContent,
                           nsIContent* aContent);

  void AddPageBreakItem(nsIContent* aContent,
                        nsStyleContext* aMainStyleContext,
                        FrameConstructionItemList& aItems);

  
  
  
  
  static const FrameConstructionData* FindHTMLData(Element* aContent,
                                                   nsIAtom* aTag,
                                                   PRInt32 aNameSpaceID,
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
                                         PRBool                   aSuppressWhiteSpaceOptimizations,
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
                                 PendingBinding  *        aPendingBinding,
                                 nsFrameItems&            aChildItems);

  nsresult GetAnonymousContent(nsIContent* aParent,
                               nsIFrame* aParentFrame,
                               nsTArray<nsIAnonymousContentCreator::ContentInfo>& aAnonContent);


#ifdef MOZ_MATHML
  




  nsresult FlushAccumulatedBlock(nsFrameConstructorState& aState,
                                 nsIContent* aContent,
                                 nsIFrame* aParentFrame,
                                 nsFrameItems* aBlockItems,
                                 nsFrameItems* aNewItems);

  
  
  static const FrameConstructionData* FindMathMLData(Element* aElement,
                                                     nsIAtom* aTag,
                                                     PRInt32 aNameSpaceID,
                                                     nsStyleContext* aStyleContext);
#endif

  
  
  static const FrameConstructionData* FindXULTagData(Element* aElement,
                                                     nsIAtom* aTag,
                                                     PRInt32 aNameSpaceID,
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


  static const FrameConstructionData* FindSVGData(Element* aElement,
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

  

  const FrameConstructionData*
    FindDisplayData(const nsStyleDisplay* aDisplay, Element* aElement,
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
                           const PRBool             aAllowBlockStyles,
                           PendingBinding*          aPendingBinding);

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
                        PendingBinding*          aPendingBinding,
                        nsFrameItems&            aFrameItems);

  nsresult MaybeRecreateFramesForElement(Element* aElement);

  
  
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
                          PRBool                   aAbsPosContainer,
                          PendingBinding*          aPendingBinding);

  nsresult ConstructInline(nsFrameConstructorState& aState,
                           FrameConstructionItem&   aItem,
                           nsIFrame*                aParentFrame,
                           const nsStyleDisplay*    aDisplay,
                           nsFrameItems&            aFrameItems,
                           nsIFrame**               aNewFrame);

  

















  void CreateIBSiblings(nsFrameConstructorState& aState,
                        nsIFrame* aInitialInline,
                        PRBool aIsPositioned,
                        nsFrameItems& aChildItems,
                        nsFrameItems& aSiblings);

  



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
                                   nsIFrame*        aBlockFrame,
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
                                ChildIterator aIter,
                                PRUint8& aTargetContentDisplay);

  
  
  
  nsIFrame* FindNextSibling(ChildIterator aIter,
                            const ChildIterator& aLast,
                            PRUint8& aTargetContentDisplay);

  
  
  
  
  
  
  
  
  
  
  
  
  
  nsIFrame* GetInsertionPrevSibling(nsIFrame*& aParentFrame, 
                                    nsIContent* aContainer,
                                    nsIContent* aChild,
                                    PRBool* aIsAppend,
                                    PRBool* aIsRangeInsertSafe,
                                    nsIContent* aStartSkipChild = nsnull,
                                    nsIContent *aEndSkipChild = nsnull);

  
  
  
  
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

  friend class nsFrameConstructorState;

private:

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
  
  PRPackedBool        mObservingRefreshDriver : 1;
  
  PRPackedBool        mInStyleRefresh : 1;
  PRUint32            mHoverGeneration;
  nsChangeHint        mRebuildAllExtraHint;

  nsCOMPtr<nsILayoutHistoryState> mTempFrameTreeState;

  RestyleTracker mPendingRestyles;
  RestyleTracker mPendingAnimationRestyles;

  static nsIXBLService * gXBLService;
};

#endif 
