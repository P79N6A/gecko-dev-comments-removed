
















#include "mozilla/DebugOnly.h"

#include "nscore.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsStyleSet.h"
#include "nsCSSFrameConstructor.h"
#include "nsStyleContext.h"
#include "nsStyleChangeList.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "prthread.h"
#include "plhash.h"
#include "nsPlaceholderFrame.h"
#include "nsContainerFrame.h"
#include "nsBlockFrame.h"
#include "nsGkAtoms.h"
#include "nsCSSAnonBoxes.h"
#include "nsCSSPseudoElements.h"
#ifdef DEBUG
#include "nsIStyleRule.h"
#endif
#include "nsILayoutHistoryState.h"
#include "nsPresState.h"
#include "nsIContent.h"
#include "nsINameSpaceManager.h"
#include "nsIDocument.h"
#include "nsIScrollableFrame.h"

#include "nsIDOMNodeList.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIFormControl.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIForm.h"
#include "nsContentUtils.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsError.h"
#include "nsLayoutUtils.h"
#include "nsAutoPtr.h"
#include "imgIRequest.h"
#include "nsTransitionManager.h"
#include "RestyleTracker.h"
#include "nsAbsoluteContainingBlock.h"

#include "nsFrameManager.h"
#include "nsRuleProcessorData.h"
#include "sampler.h"

#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

  #ifdef DEBUG
    
    
  #else
    #undef NOISY_DEBUG
    #undef DEBUG_UNDISPLAYED_MAP
  #endif

  #ifdef NOISY_DEBUG
    #define NOISY_TRACE(_msg) \
      printf("%s",_msg);
    #define NOISY_TRACE_FRAME(_msg,_frame) \
      printf("%s ",_msg); nsFrame::ListTag(stdout,_frame); printf("\n");
  #else
    #define NOISY_TRACE(_msg);
    #define NOISY_TRACE_FRAME(_msg,_frame);
  #endif

using namespace mozilla;
using namespace mozilla::dom;



struct PlaceholderMapEntry : public PLDHashEntryHdr {
  
  nsPlaceholderFrame *placeholderFrame;
};

static bool
PlaceholderMapMatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                         const void *key)
{
  const PlaceholderMapEntry *entry =
    static_cast<const PlaceholderMapEntry*>(hdr);
  NS_ASSERTION(entry->placeholderFrame->GetOutOfFlowFrame() !=
               (void*)0xdddddddd,
               "Dead placeholder in placeholder map");
  return entry->placeholderFrame->GetOutOfFlowFrame() == key;
}

static PLDHashTableOps PlaceholderMapOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashVoidPtrKeyStub,
  PlaceholderMapMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  NULL
};






class UndisplayedNode {
public:
  UndisplayedNode(nsIContent* aContent, nsStyleContext* aStyle)
    : mContent(aContent),
      mStyle(aStyle),
      mNext(nullptr)
  {
    MOZ_COUNT_CTOR(UndisplayedNode);
  }

  NS_HIDDEN ~UndisplayedNode()
  {
    MOZ_COUNT_DTOR(UndisplayedNode);

    
    UndisplayedNode *cur = mNext;
    while (cur) {
      UndisplayedNode *next = cur->mNext;
      cur->mNext = nullptr;
      delete cur;
      cur = next;
    }
  }

  nsCOMPtr<nsIContent>      mContent;
  nsRefPtr<nsStyleContext>  mStyle;
  UndisplayedNode*          mNext;
};

class nsFrameManagerBase::UndisplayedMap {
public:
  UndisplayedMap(uint32_t aNumBuckets = 16) NS_HIDDEN;
  ~UndisplayedMap(void) NS_HIDDEN;

  NS_HIDDEN_(UndisplayedNode*) GetFirstNode(nsIContent* aParentContent);

  NS_HIDDEN_(nsresult) AddNodeFor(nsIContent* aParentContent,
                                  nsIContent* aChild, nsStyleContext* aStyle);

  NS_HIDDEN_(void) RemoveNodeFor(nsIContent* aParentContent,
                                 UndisplayedNode* aNode);

  NS_HIDDEN_(void) RemoveNodesFor(nsIContent* aParentContent);

  
  NS_HIDDEN_(void)  Clear(void);

protected:
  NS_HIDDEN_(PLHashEntry**) GetEntryFor(nsIContent* aParentContent);
  NS_HIDDEN_(void)          AppendNodeFor(UndisplayedNode* aNode,
                                          nsIContent* aParentContent);

  PLHashTable*  mTable;
  PLHashEntry** mLastLookup;
};



nsFrameManager::~nsFrameManager()
{
  NS_ASSERTION(!mPresShell, "nsFrameManager::Destroy never called");
}

nsresult
nsFrameManager::Init(nsStyleSet* aStyleSet)
{
  if (!mPresShell) {
    NS_ERROR("null pres shell");
    return NS_ERROR_FAILURE;
  }

  if (!aStyleSet) {
    NS_ERROR("null style set");
    return NS_ERROR_FAILURE;
  }

  mStyleSet = aStyleSet;
  return NS_OK;
}

void
nsFrameManager::Destroy()
{
  NS_ASSERTION(mPresShell, "Frame manager already shut down.");

  
  mPresShell->SetIgnoreFrameDestruction(true);

  
  nsFrameManager::ClearPlaceholderFrameMap();

  if (mRootFrame) {
    mRootFrame->Destroy();
    mRootFrame = nullptr;
  }
  
  delete mUndisplayedMap;
  mUndisplayedMap = nullptr;

  mPresShell = nullptr;
}




nsPlaceholderFrame*
nsFrameManager::GetPlaceholderFrameFor(const nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "null param unexpected");

  if (mPlaceholderMap.ops) {
    PlaceholderMapEntry *entry = static_cast<PlaceholderMapEntry*>
                                            (PL_DHashTableOperate(const_cast<PLDHashTable*>(&mPlaceholderMap),
                                aFrame, PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      return entry->placeholderFrame;
    }
  }

  return nullptr;
}

nsresult
nsFrameManager::RegisterPlaceholderFrame(nsPlaceholderFrame* aPlaceholderFrame)
{
  NS_PRECONDITION(aPlaceholderFrame, "null param unexpected");
  NS_PRECONDITION(nsGkAtoms::placeholderFrame == aPlaceholderFrame->GetType(),
                  "unexpected frame type");
  if (!mPlaceholderMap.ops) {
    if (!PL_DHashTableInit(&mPlaceholderMap, &PlaceholderMapOps, nullptr,
                           sizeof(PlaceholderMapEntry), 16)) {
      mPlaceholderMap.ops = nullptr;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  PlaceholderMapEntry *entry = static_cast<PlaceholderMapEntry*>(PL_DHashTableOperate(&mPlaceholderMap,
                              aPlaceholderFrame->GetOutOfFlowFrame(),
                              PL_DHASH_ADD));
  if (!entry)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ASSERTION(!entry->placeholderFrame, "Registering a placeholder for a frame that already has a placeholder!");
  entry->placeholderFrame = aPlaceholderFrame;

  return NS_OK;
}

void
nsFrameManager::UnregisterPlaceholderFrame(nsPlaceholderFrame* aPlaceholderFrame)
{
  NS_PRECONDITION(aPlaceholderFrame, "null param unexpected");
  NS_PRECONDITION(nsGkAtoms::placeholderFrame == aPlaceholderFrame->GetType(),
                  "unexpected frame type");

  if (mPlaceholderMap.ops) {
    PL_DHashTableOperate(&mPlaceholderMap,
                         aPlaceholderFrame->GetOutOfFlowFrame(),
                         PL_DHASH_REMOVE);
  }
}

static PLDHashOperator
UnregisterPlaceholders(PLDHashTable* table, PLDHashEntryHdr* hdr,
                       uint32_t number, void* arg)
{
  PlaceholderMapEntry* entry = static_cast<PlaceholderMapEntry*>(hdr);
  entry->placeholderFrame->SetOutOfFlowFrame(nullptr);
  return PL_DHASH_NEXT;
}

void
nsFrameManager::ClearPlaceholderFrameMap()
{
  if (mPlaceholderMap.ops) {
    PL_DHashTableEnumerate(&mPlaceholderMap, UnregisterPlaceholders, nullptr);
    PL_DHashTableFinish(&mPlaceholderMap);
    mPlaceholderMap.ops = nullptr;
  }
}



nsStyleContext*
nsFrameManager::GetUndisplayedContent(nsIContent* aContent)
{
  if (!aContent || !mUndisplayedMap)
    return nullptr;

  nsIContent* parent = aContent->GetParent();
  for (UndisplayedNode* node = mUndisplayedMap->GetFirstNode(parent);
         node; node = node->mNext) {
    if (node->mContent == aContent)
      return node->mStyle;
  }

  return nullptr;
}
  
void
nsFrameManager::SetUndisplayedContent(nsIContent* aContent, 
                                      nsStyleContext* aStyleContext)
{
  NS_PRECONDITION(!aStyleContext->GetPseudo(),
                  "Should only have actual elements here");

#ifdef DEBUG_UNDISPLAYED_MAP
  static int i = 0;
  printf("SetUndisplayedContent(%d): p=%p \n", i++, (void *)aContent);
#endif

  NS_ASSERTION(!GetUndisplayedContent(aContent),
               "Already have an undisplayed context entry for aContent");

  if (! mUndisplayedMap) {
    mUndisplayedMap = new UndisplayedMap;
  }
  nsIContent* parent = aContent->GetParent();
  NS_ASSERTION(parent || (mPresShell && mPresShell->GetDocument() &&
               mPresShell->GetDocument()->GetRootElement() == aContent),
               "undisplayed content must have a parent, unless it's the root "
               "element");
  mUndisplayedMap->AddNodeFor(parent, aContent, aStyleContext);
}

void
nsFrameManager::ChangeUndisplayedContent(nsIContent* aContent, 
                                         nsStyleContext* aStyleContext)
{
  NS_ASSERTION(mUndisplayedMap, "no existing undisplayed content");
  
#ifdef DEBUG_UNDISPLAYED_MAP
   static int i = 0;
   printf("ChangeUndisplayedContent(%d): p=%p \n", i++, (void *)aContent);
#endif

  for (UndisplayedNode* node = mUndisplayedMap->GetFirstNode(aContent->GetParent());
         node; node = node->mNext) {
    if (node->mContent == aContent) {
      node->mStyle = aStyleContext;
      return;
    }
  }

  NS_NOTREACHED("no existing undisplayed content");
}

void
nsFrameManager::ClearUndisplayedContentIn(nsIContent* aContent,
                                          nsIContent* aParentContent)
{
#ifdef DEBUG_UNDISPLAYED_MAP
  static int i = 0;
  printf("ClearUndisplayedContent(%d): content=%p parent=%p --> ", i++, (void *)aContent, (void*)aParentContent);
#endif
  
  if (mUndisplayedMap) {
    UndisplayedNode* node = mUndisplayedMap->GetFirstNode(aParentContent);
    while (node) {
      if (node->mContent == aContent) {
        mUndisplayedMap->RemoveNodeFor(aParentContent, node);

#ifdef DEBUG_UNDISPLAYED_MAP
        printf( "REMOVED!\n");
#endif
#ifdef DEBUG
        
        nsStyleContext *context = GetUndisplayedContent(aContent);
        NS_ASSERTION(context == nullptr, "Found more undisplayed content data after removal");
#endif
        return;
      }
      node = node->mNext;
    }
  }
}

void
nsFrameManager::ClearAllUndisplayedContentIn(nsIContent* aParentContent)
{
#ifdef DEBUG_UNDISPLAYED_MAP
  static int i = 0;
  printf("ClearAllUndisplayedContentIn(%d): parent=%p \n", i++, (void*)aParentContent);
#endif

  if (mUndisplayedMap) {
    mUndisplayedMap->RemoveNodesFor(aParentContent);
  }

  
  
  
  
  
  nsINodeList* list =
    aParentContent->OwnerDoc()->BindingManager()->GetXBLChildNodesFor(aParentContent);
  if (list) {
    uint32_t length;
    list->GetLength(&length);
    for (uint32_t i = 0; i < length; ++i) {
      nsIContent* child = list->Item(i);
      if (child->GetParent() != aParentContent) {
        ClearUndisplayedContentIn(child, child->GetParent());
      }
    }
  }
}


nsresult
nsFrameManager::AppendFrames(nsIFrame*       aParentFrame,
                             ChildListID     aListID,
                             nsFrameList&    aFrameList)
{
  if (aParentFrame->IsAbsoluteContainer() &&
      aListID == aParentFrame->GetAbsoluteListID()) {
    return aParentFrame->GetAbsoluteContainingBlock()->
           AppendFrames(aParentFrame, aListID, aFrameList);
  } else {
    return aParentFrame->AppendFrames(aListID, aFrameList);
  }
}

nsresult
nsFrameManager::InsertFrames(nsIFrame*       aParentFrame,
                             ChildListID     aListID,
                             nsIFrame*       aPrevFrame,
                             nsFrameList&    aFrameList)
{
  NS_PRECONDITION(!aPrevFrame || (!aPrevFrame->GetNextContinuation()
                  || (((aPrevFrame->GetNextContinuation()->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER))
                  && !(aPrevFrame->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER))),
                  "aPrevFrame must be the last continuation in its chain!");

  if (aParentFrame->IsAbsoluteContainer() &&
      aListID == aParentFrame->GetAbsoluteListID()) {
    return aParentFrame->GetAbsoluteContainingBlock()->
           InsertFrames(aParentFrame, aListID, aPrevFrame, aFrameList);
  } else {
    return aParentFrame->InsertFrames(aListID, aPrevFrame, aFrameList);
  }
}

nsresult
nsFrameManager::RemoveFrame(ChildListID     aListID,
                            nsIFrame*       aOldFrame)
{
  bool wasDestroyingFrames = mIsDestroyingFrames;
  mIsDestroyingFrames = true;

  
  
  
  
  
  
  aOldFrame->InvalidateFrameForRemoval();

  NS_ASSERTION(!aOldFrame->GetPrevContinuation() ||
               
               aOldFrame->GetType() == nsGkAtoms::textFrame,
               "Must remove first continuation.");
  NS_ASSERTION(!(aOldFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW &&
                 GetPlaceholderFrameFor(aOldFrame)),
               "Must call RemoveFrame on placeholder for out-of-flows.");
  nsresult rv = NS_OK;
  nsIFrame* parentFrame = aOldFrame->GetParent();
  if (parentFrame->IsAbsoluteContainer() &&
      aListID == parentFrame->GetAbsoluteListID()) {
    parentFrame->GetAbsoluteContainingBlock()->
      RemoveFrame(parentFrame, aListID, aOldFrame);
  } else {
    rv = parentFrame->RemoveFrame(aListID, aOldFrame);
  }

  mIsDestroyingFrames = wasDestroyingFrames;

  return rv;
}



void
nsFrameManager::NotifyDestroyingFrame(nsIFrame* aFrame)
{
  nsIContent* content = aFrame->GetContent();
  if (content && content->GetPrimaryFrame() == aFrame) {
    ClearAllUndisplayedContentIn(content);
  }
}

#ifdef DEBUG
static void
DumpContext(nsIFrame* aFrame, nsStyleContext* aContext)
{
  if (aFrame) {
    fputs("frame: ", stdout);
    nsAutoString  name;
    aFrame->GetFrameName(name);
    fputs(NS_LossyConvertUTF16toASCII(name).get(), stdout);
    fprintf(stdout, " (%p)", static_cast<void*>(aFrame));
  }
  if (aContext) {
    fprintf(stdout, " style: %p ", static_cast<void*>(aContext));

    nsIAtom* pseudoTag = aContext->GetPseudo();
    if (pseudoTag) {
      nsAutoString  buffer;
      pseudoTag->ToString(buffer);
      fputs(NS_LossyConvertUTF16toASCII(buffer).get(), stdout);
      fputs(" ", stdout);
    }
    fputs("{}\n", stdout);
  }
}

static void
VerifySameTree(nsStyleContext* aContext1, nsStyleContext* aContext2)
{
  nsStyleContext* top1 = aContext1;
  nsStyleContext* top2 = aContext2;
  nsStyleContext* parent;
  for (;;) {
    parent = top1->GetParent();
    if (!parent)
      break;
    top1 = parent;
  }
  for (;;) {
    parent = top2->GetParent();
    if (!parent)
      break;
    top2 = parent;
  }
  NS_ASSERTION(top1 == top2,
               "Style contexts are not in the same style context tree");
}

static void
VerifyContextParent(nsPresContext* aPresContext, nsIFrame* aFrame, 
                    nsStyleContext* aContext, nsStyleContext* aParentContext)
{
  
  if (!aContext) {
    aContext = aFrame->StyleContext();
  }

  if (!aParentContext) {
    
    
    

    
    nsIFrame* providerFrame = aFrame->GetParentStyleContextFrame();
    if (providerFrame)
      aParentContext = providerFrame->StyleContext();
    
  }

  NS_ASSERTION(aContext, "Failure to get required contexts");
  nsStyleContext* actualParentContext = aContext->GetParent();

  if (aParentContext) {
    if (aParentContext != actualParentContext) {
      DumpContext(aFrame, aContext);
      if (aContext == aParentContext) {
        NS_ERROR("Using parent's style context");
      }
      else {
        NS_ERROR("Wrong parent style context");
        fputs("Wrong parent style context: ", stdout);
        DumpContext(nullptr, actualParentContext);
        fputs("should be using: ", stdout);
        DumpContext(nullptr, aParentContext);
        VerifySameTree(actualParentContext, aParentContext);
        fputs("\n", stdout);
      }
    }

  }
  else {
    if (actualParentContext) {
      NS_ERROR("Have parent context and shouldn't");
      DumpContext(aFrame, aContext);
      fputs("Has parent context: ", stdout);
      DumpContext(nullptr, actualParentContext);
      fputs("Should be null\n\n", stdout);
    }
  }

  nsStyleContext* childStyleIfVisited = aContext->GetStyleIfVisited();
  
  
  
  if (childStyleIfVisited &&
      !((childStyleIfVisited->RuleNode() != aContext->RuleNode() &&
         childStyleIfVisited->GetParent() == aContext->GetParent()) ||
        childStyleIfVisited->GetParent() ==
          aContext->GetParent()->GetStyleIfVisited())) {
    NS_ERROR("Visited style has wrong parent");
    DumpContext(aFrame, aContext);
    fputs("\n", stdout);
  }
}

static void
VerifyStyleTree(nsPresContext* aPresContext, nsIFrame* aFrame,
                nsStyleContext* aParentContext)
{
  nsStyleContext*  context = aFrame->StyleContext();
  VerifyContextParent(aPresContext, aFrame, context, nullptr);

  nsIFrame::ChildListIterator lists(aFrame);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* child = childFrames.get();
      if (!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
        
        if (nsGkAtoms::placeholderFrame == child->GetType()) { 
          
          
          nsIFrame* outOfFlowFrame =
            nsPlaceholderFrame::GetRealFrameForPlaceholder(child);

          
          do {
            VerifyStyleTree(aPresContext, outOfFlowFrame, nullptr);
          } while ((outOfFlowFrame = outOfFlowFrame->GetNextContinuation()));

          
          
          VerifyContextParent(aPresContext, child, nullptr, nullptr);
        }
        else { 
          VerifyStyleTree(aPresContext, child, nullptr);
        }
      }
    }
  }
  
  
  int32_t contextIndex = -1;
  while (1) {
    nsStyleContext* extraContext = aFrame->GetAdditionalStyleContext(++contextIndex);
    if (extraContext) {
      VerifyContextParent(aPresContext, aFrame, extraContext, context);
    }
    else {
      break;
    }
  }
}

void
nsFrameManager::DebugVerifyStyleTree(nsIFrame* aFrame)
{
  if (aFrame) {
    nsStyleContext* context = aFrame->StyleContext();
    nsStyleContext* parentContext = context->GetParent();
    VerifyStyleTree(GetPresContext(), aFrame, parentContext);
  }
}

#endif 



static void
TryStartingTransition(nsPresContext *aPresContext, nsIContent *aContent,
                      nsStyleContext *aOldStyleContext,
                      nsRefPtr<nsStyleContext> *aNewStyleContext )
{
  if (!aContent || !aContent->IsElement()) {
    return;
  }

  
  
  
  
  
  
  nsCOMPtr<nsIStyleRule> coverRule = 
    aPresContext->TransitionManager()->StyleContextChanged(
      aContent->AsElement(), aOldStyleContext, *aNewStyleContext);
  if (coverRule) {
    nsCOMArray<nsIStyleRule> rules;
    rules.AppendObject(coverRule);
    *aNewStyleContext = aPresContext->StyleSet()->
                          ResolveStyleByAddingRules(*aNewStyleContext, rules);
  }
}

static inline Element*
ElementForStyleContext(nsIContent* aParentContent,
                       nsIFrame* aFrame,
                       nsCSSPseudoElements::Type aPseudoType)
{
  
  NS_PRECONDITION(aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement ||
                  aPseudoType == nsCSSPseudoElements::ePseudo_AnonBox ||
                  aPseudoType < nsCSSPseudoElements::ePseudo_PseudoElementCount,
                  "Unexpected pseudo");
  
  
  if (aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement) {
    return aFrame->GetContent()->AsElement();
  }

  if (aPseudoType == nsCSSPseudoElements::ePseudo_AnonBox) {
    return nullptr;
  }

  if (aPseudoType == nsCSSPseudoElements::ePseudo_firstLetter) {
    NS_ASSERTION(aFrame->GetType() == nsGkAtoms::letterFrame,
                 "firstLetter pseudoTag without a nsFirstLetterFrame");
    nsBlockFrame* block = nsBlockFrame::GetNearestAncestorBlock(aFrame);
    return block->GetContent()->AsElement();
  }

  nsIContent* content = aParentContent ? aParentContent : aFrame->GetContent();
  return content->AsElement();
}

static nsIFrame*
GetPrevContinuationWithPossiblySameStyle(nsIFrame* aFrame)
{
  
  
  
  
  
  
  
  
  
  
  
  nsIFrame *prevContinuation = aFrame->GetPrevContinuation();
  if (!prevContinuation && (aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL)) {
    
    
    prevContinuation = static_cast<nsIFrame*>(
      aFrame->Properties().Get(nsIFrame::IBSplitSpecialPrevSibling()));
    if (prevContinuation) {
      prevContinuation = static_cast<nsIFrame*>(
        prevContinuation->Properties().Get(nsIFrame::IBSplitSpecialPrevSibling()));
    }
  }
  return prevContinuation;
}

nsresult
nsFrameManager::ReparentStyleContext(nsIFrame* aFrame)
{
  if (nsGkAtoms::placeholderFrame == aFrame->GetType()) {
    
    nsIFrame* outOfFlow =
      nsPlaceholderFrame::GetRealFrameForPlaceholder(aFrame);
    NS_ASSERTION(outOfFlow, "no out-of-flow frame");
    do {
      ReparentStyleContext(outOfFlow);
    } while ((outOfFlow = outOfFlow->GetNextContinuation()));
  }

  
  
  nsStyleContext* oldContext = aFrame->StyleContext();
  
  if (oldContext) {
    nsRefPtr<nsStyleContext> newContext;
    nsIFrame* providerFrame = aFrame->GetParentStyleContextFrame();
    bool isChild = providerFrame && providerFrame->GetParent() == aFrame;
    nsStyleContext* newParentContext = nullptr;
    nsIFrame* providerChild = nullptr;
    if (isChild) {
      ReparentStyleContext(providerFrame);
      newParentContext = providerFrame->StyleContext();
      providerChild = providerFrame;
    } else if (providerFrame) {
      newParentContext = providerFrame->StyleContext();
    } else {
      NS_NOTREACHED("Reparenting something that has no usable parent? "
                    "Shouldn't happen!");
    }
    
    
    
    
    

#ifdef DEBUG
    {
      
      
      
      
      
      
      
      
      nsIFrame *nextContinuation = aFrame->GetNextContinuation();
      if (nextContinuation) {
        nsStyleContext *nextContinuationContext =
          nextContinuation->StyleContext();
        NS_ASSERTION(oldContext == nextContinuationContext ||
                     oldContext->GetPseudo() !=
                       nextContinuationContext->GetPseudo() ||
                     oldContext->GetParent() !=
                       nextContinuationContext->GetParent(),
                     "continuations should have the same style context");
      }
    }
#endif

    nsIFrame *prevContinuation =
      GetPrevContinuationWithPossiblySameStyle(aFrame);
    nsStyleContext *prevContinuationContext;
    bool copyFromContinuation =
      prevContinuation &&
      (prevContinuationContext = prevContinuation->StyleContext())
        ->GetPseudo() == oldContext->GetPseudo() &&
       prevContinuationContext->GetParent() == newParentContext;
    if (copyFromContinuation) {
      
      
      
      
      newContext = prevContinuationContext;
    } else {
      nsIFrame* parentFrame = aFrame->GetParent();
      Element* element =
        ElementForStyleContext(parentFrame ? parentFrame->GetContent() : nullptr,
                               aFrame,
                               oldContext->GetPseudoType());
      newContext = mStyleSet->ReparentStyleContext(oldContext,
                                                   newParentContext,
                                                   element);
    }

    if (newContext) {
      if (newContext != oldContext) {
        
        
        
        
        
#if 0
        if (!copyFromContinuation) {
          TryStartingTransition(GetPresContext(), aFrame->GetContent(),
                                oldContext, &newContext);
        }
#endif

        
        
        DebugOnly<nsChangeHint> styleChange =
          oldContext->CalcStyleDifference(newContext, nsChangeHint(0));
        
        
        
        
        NS_ASSERTION(!(styleChange & nsChangeHint_ReconstructFrame),
                     "Our frame tree is likely to be bogus!");
        
        aFrame->SetStyleContext(newContext);

        nsIFrame::ChildListIterator lists(aFrame);
        for (; !lists.IsDone(); lists.Next()) {
          nsFrameList::Enumerator childFrames(lists.CurrentList());
          for (; !childFrames.AtEnd(); childFrames.Next()) {
            nsIFrame* child = childFrames.get();
            
            if (!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW) &&
                child != providerChild) {
#ifdef DEBUG
              if (nsGkAtoms::placeholderFrame == child->GetType()) {
                nsIFrame* outOfFlowFrame =
                  nsPlaceholderFrame::GetRealFrameForPlaceholder(child);
                NS_ASSERTION(outOfFlowFrame, "no out-of-flow frame");

                NS_ASSERTION(outOfFlowFrame != providerChild,
                             "Out of flow provider?");
              }
#endif
              ReparentStyleContext(child);
            }
          }
        }

        
        
        
        
        
        
        if ((aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL) &&
            !aFrame->GetPrevContinuation()) {
          nsIFrame* sib = static_cast<nsIFrame*>
            (aFrame->Properties().Get(nsIFrame::IBSplitSpecialSibling()));
          if (sib) {
            ReparentStyleContext(sib);
          }
        }

        
        int32_t contextIndex = -1;
        while (1) {
          nsStyleContext* oldExtraContext =
            aFrame->GetAdditionalStyleContext(++contextIndex);
          if (oldExtraContext) {
            nsRefPtr<nsStyleContext> newExtraContext;
            newExtraContext = mStyleSet->ReparentStyleContext(oldExtraContext,
                                                              newContext,
                                                              nullptr);
            if (newExtraContext) {
              if (newExtraContext != oldExtraContext) {
                
                
                
                styleChange =
                  oldExtraContext->CalcStyleDifference(newExtraContext,
                                                       nsChangeHint(0));
                
                
                
                
                
                NS_ASSERTION(!(styleChange & nsChangeHint_ReconstructFrame),
                             "Our frame tree is likely to be bogus!");
              }
              
              aFrame->SetAdditionalStyleContext(contextIndex, newExtraContext);
            }
          }
          else {
            break;
          }
        }
#ifdef DEBUG
        VerifyStyleTree(GetPresContext(), aFrame, newParentContext);
#endif
      }
    }
  }
  return NS_OK;
}

static void
CaptureChange(nsStyleContext* aOldContext, nsStyleContext* aNewContext,
              nsIFrame* aFrame, nsIContent* aContent,
              nsStyleChangeList* aChangeList,
              nsChangeHint &aMinChange,
              nsChangeHint aParentHintsNotHandledForDescendants,
              nsChangeHint &aHintsNotHandledForDescendants,
              nsChangeHint aChangeToAssume)
{
  nsChangeHint ourChange = aOldContext->CalcStyleDifference(aNewContext,
                             aParentHintsNotHandledForDescendants);
  NS_ASSERTION(!(ourChange & nsChangeHint_AllReflowHints) ||
               (ourChange & nsChangeHint_NeedReflow),
               "Reflow hint bits set without actually asking for a reflow");

  
  
  
  if ((ourChange & nsChangeHint_UpdateEffects) &&
      aContent && !aContent->IsElement()) {
    ourChange = NS_SubtractHint(ourChange, nsChangeHint_UpdateEffects);
  }

  NS_UpdateHint(ourChange, aChangeToAssume);
  if (NS_UpdateHint(aMinChange, ourChange)) {
    if (!(ourChange & nsChangeHint_ReconstructFrame) || aContent) {
      aChangeList->AppendChange(aFrame, aContent, ourChange);
    }
  }
  aHintsNotHandledForDescendants = NS_HintsNotHandledForDescendantsIn(ourChange);
}












nsChangeHint
nsFrameManager::ReResolveStyleContext(nsPresContext     *aPresContext,
                                      nsIFrame          *aFrame,
                                      nsIContent        *aParentContent,
                                      nsStyleChangeList *aChangeList, 
                                      nsChangeHint       aMinChange,
                                      nsChangeHint       aParentFrameHintsNotHandledForDescendants,
                                      nsRestyleHint      aRestyleHint,
                                      RestyleTracker&    aRestyleTracker,
                                      DesiredA11yNotifications aDesiredA11yNotifications,
                                      nsTArray<nsIContent*>& aVisibleKidsOfHiddenElement,
                                      TreeMatchContext &aTreeMatchContext)
{
  
  
  
  aMinChange = NS_SubtractHint(aMinChange, NS_HintsNotHandledForDescendantsIn(aMinChange));

  
  
  
  
  
  
  
  NS_ASSERTION(aFrame->GetContent() || !aParentContent ||
               !aParentContent->GetParent(),
               "frame must have content (unless at the top of the tree)");
  
  
  

  
  
  

  nsChangeHint assumeDifferenceHint = NS_STYLE_HINT_NONE;
  
  nsStyleContext* oldContext = aFrame->StyleContext();
  nsStyleSet* styleSet = aPresContext->StyleSet();

  
  
  if (oldContext) {
    oldContext->AddRef();

#ifdef ACCESSIBILITY
    bool wasFrameVisible = nsIPresShell::IsAccessibilityActive() ?
      oldContext->StyleVisibility()->IsVisible() : false;
#endif

    nsIAtom* const pseudoTag = oldContext->GetPseudo();
    const nsCSSPseudoElements::Type pseudoType = oldContext->GetPseudoType();
    nsIContent* localContent = aFrame->GetContent();
    
    
    
    
    
    nsIContent* content = localContent ? localContent : aParentContent;

    if (content && content->IsElement()) {
      content->OwnerDoc()->FlushPendingLinkUpdates();
      RestyleTracker::RestyleData restyleData;
      if (aRestyleTracker.GetRestyleData(content->AsElement(), &restyleData)) {
        if (NS_UpdateHint(aMinChange, restyleData.mChangeHint)) {
          aChangeList->AppendChange(aFrame, content, restyleData.mChangeHint);
        }
        aRestyleHint = nsRestyleHint(aRestyleHint | restyleData.mRestyleHint);
      }
    }

    nsRestyleHint childRestyleHint = aRestyleHint;

    if (childRestyleHint == eRestyle_Self) {
      childRestyleHint = nsRestyleHint(0);
    }

    nsStyleContext* parentContext;
    nsIFrame* resolvedChild = nullptr;
    
    
    nsIFrame* providerFrame = aFrame->GetParentStyleContextFrame();
    bool isChild = providerFrame && providerFrame->GetParent() == aFrame;
    if (!isChild) {
      if (providerFrame)
        parentContext = providerFrame->StyleContext();
      else
        parentContext = nullptr;
    }
    else {
      MOZ_ASSERT(providerFrame->GetContent() == aFrame->GetContent(),
                 "Postcondition for GetParentStyleContextFrame() violated. "
                 "That means we need to add the current element to the "
                 "ancestor filter.");

      

      
      
      
      
      
      

      assumeDifferenceHint = ReResolveStyleContext(aPresContext, providerFrame,
                                                   aParentContent, aChangeList,
                                                   aMinChange,
                                                   nsChangeHint_Hints_NotHandledForDescendants,
                                                   aRestyleHint,
                                                   aRestyleTracker,
                                                   aDesiredA11yNotifications,
                                                   aVisibleKidsOfHiddenElement,
                                                   aTreeMatchContext);

      
      
      parentContext = providerFrame->StyleContext();
      
      
      resolvedChild = providerFrame;
    }

    if (providerFrame != aFrame->GetParent()) {
      
      
      aParentFrameHintsNotHandledForDescendants =
        nsChangeHint_Hints_NotHandledForDescendants;
    }

#ifdef DEBUG
    {
      
      
      
      
      
      
      
      
      nsIFrame *nextContinuation = aFrame->GetNextContinuation();
      if (nextContinuation) {
        nsStyleContext *nextContinuationContext =
          nextContinuation->StyleContext();
        NS_ASSERTION(oldContext == nextContinuationContext ||
                     oldContext->GetPseudo() !=
                       nextContinuationContext->GetPseudo() ||
                     oldContext->GetParent() !=
                       nextContinuationContext->GetParent(),
                     "continuations should have the same style context");
      }
      
      
      
      if ((aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL) &&
          !aFrame->GetPrevContinuation()) {
        nsIFrame *nextIBSibling = static_cast<nsIFrame*>(
          aFrame->Properties().Get(nsIFrame::IBSplitSpecialSibling()));
        if (nextIBSibling) {
          nextIBSibling = static_cast<nsIFrame*>(
            nextIBSibling->Properties().Get(nsIFrame::IBSplitSpecialSibling()));
        }
        if (nextIBSibling) {
          nsStyleContext *nextIBSiblingContext =
            nextIBSibling->StyleContext();
          NS_ASSERTION(oldContext == nextIBSiblingContext ||
                       oldContext->GetPseudo() !=
                         nextIBSiblingContext->GetPseudo() ||
                       oldContext->GetParent() !=
                         nextIBSiblingContext->GetParent(),
                       "continuations should have the same style context");
        }
      }
    }
#endif

    
    nsRefPtr<nsStyleContext> newContext;
    nsChangeHint nonInheritedHints = nsChangeHint(0);
    nsIFrame *prevContinuation =
      GetPrevContinuationWithPossiblySameStyle(aFrame);
    nsStyleContext *prevContinuationContext;
    bool copyFromContinuation =
      prevContinuation &&
      (prevContinuationContext = prevContinuation->StyleContext())
        ->GetPseudo() == oldContext->GetPseudo() &&
       prevContinuationContext->GetParent() == parentContext;
    if (copyFromContinuation) {
      
      
      
      
      newContext = prevContinuationContext;
      
      
      nonInheritedHints = nsChangeHint_Hints_NotHandledForDescendants;
    }
    else if (pseudoTag == nsCSSAnonBoxes::mozNonElement) {
      NS_ASSERTION(localContent,
                   "non pseudo-element frame without content node");
      newContext = styleSet->ResolveStyleForNonElement(parentContext);
    }
    else if (!aRestyleHint && !prevContinuation) {
      
      
      
      
      
      
      
      
      newContext =
        styleSet->ReparentStyleContext(oldContext, parentContext,
                                       ElementForStyleContext(aParentContent,
                                                              aFrame,
                                                              pseudoType));
    } else if (pseudoType == nsCSSPseudoElements::ePseudo_AnonBox) {
      newContext = styleSet->ResolveAnonymousBoxStyle(pseudoTag,
                                                      parentContext);
    }
    else {
      Element* element = ElementForStyleContext(aParentContent,
                                                aFrame,
                                                pseudoType);
      if (pseudoTag) {
        if (pseudoTag == nsCSSPseudoElements::before ||
            pseudoTag == nsCSSPseudoElements::after) {
          
          newContext = styleSet->ProbePseudoElementStyle(element,
                                                         pseudoType,
                                                         parentContext,
                                                         aTreeMatchContext);
          if (!newContext) {
            
            NS_UpdateHint(aMinChange, nsChangeHint_ReconstructFrame);
            aChangeList->AppendChange(aFrame, element,
                                      nsChangeHint_ReconstructFrame);
            
            newContext = oldContext;
          }
        } else {
          
          
          NS_ASSERTION(pseudoType <
                         nsCSSPseudoElements::ePseudo_PseudoElementCount,
                       "Unexpected pseudo type");
          newContext = styleSet->ResolvePseudoElementStyle(element,
                                                           pseudoType,
                                                           parentContext);
        }
      }
      else {
        NS_ASSERTION(localContent,
                     "non pseudo-element frame without content node");
        
        TreeMatchContext::AutoFlexItemStyleFixupSkipper
          flexFixupSkipper(aTreeMatchContext,
                           element->IsRootOfNativeAnonymousSubtree());
        newContext = styleSet->ResolveStyleFor(element, parentContext,
                                               aTreeMatchContext);
      }
    }

    NS_ASSERTION(newContext, "failed to get new style context");
    if (newContext) {
      if (!parentContext) {
        if (oldContext->RuleNode() == newContext->RuleNode() &&
            oldContext->IsLinkContext() == newContext->IsLinkContext() &&
            oldContext->RelevantLinkVisited() ==
              newContext->RelevantLinkVisited()) {
          
          
          
          
          
          newContext = oldContext;
        }
      }

      if (newContext != oldContext) {
        if (!copyFromContinuation) {
          TryStartingTransition(aPresContext, aFrame->GetContent(),
                                oldContext, &newContext);
        }

        CaptureChange(oldContext, newContext, aFrame, content, aChangeList,
                      aMinChange, aParentFrameHintsNotHandledForDescendants,
                      nonInheritedHints, assumeDifferenceHint);
        if (!(aMinChange & nsChangeHint_ReconstructFrame)) {
          
          aFrame->SetStyleContext(newContext);
        }
      }
      oldContext->Release();
    }
    else {
      NS_ERROR("resolve style context failed");
      newContext = oldContext;  
    }

    
    
    
    int32_t contextIndex = -1;
    while (1 == 1) {
      nsStyleContext* oldExtraContext = nullptr;
      oldExtraContext = aFrame->GetAdditionalStyleContext(++contextIndex);
      if (oldExtraContext) {
        nsRefPtr<nsStyleContext> newExtraContext;
        nsIAtom* const extraPseudoTag = oldExtraContext->GetPseudo();
        const nsCSSPseudoElements::Type extraPseudoType =
          oldExtraContext->GetPseudoType();
        NS_ASSERTION(extraPseudoTag &&
                     extraPseudoTag != nsCSSAnonBoxes::mozNonElement,
                     "extra style context is not pseudo element");
        if (extraPseudoType == nsCSSPseudoElements::ePseudo_AnonBox) {
          newExtraContext = styleSet->ResolveAnonymousBoxStyle(extraPseudoTag,
                                                               newContext);
        }
        else {
          
          
          NS_ASSERTION(extraPseudoType <
                         nsCSSPseudoElements::ePseudo_PseudoElementCount,
                       "Unexpected type");
          newExtraContext = styleSet->ResolvePseudoElementStyle(content->AsElement(),
                                                                extraPseudoType,
                                                                newContext);
        }
        if (newExtraContext) {
          if (oldExtraContext != newExtraContext) {
            nsChangeHint extraHintsNotHandledForDescendants = nsChangeHint(0);
            CaptureChange(oldExtraContext, newExtraContext, aFrame, content,
                          aChangeList, aMinChange,
                          aParentFrameHintsNotHandledForDescendants,
                          extraHintsNotHandledForDescendants,
                          assumeDifferenceHint);
            NS_UpdateHint(nonInheritedHints, extraHintsNotHandledForDescendants);
            if (!(aMinChange & nsChangeHint_ReconstructFrame)) {
              aFrame->SetAdditionalStyleContext(contextIndex, newExtraContext);
            }
          }
        }
      }
      else {
        break;
      }
    }

    

    
    
    
    bool checkUndisplayed;
    nsIContent *undisplayedParent;
    if (pseudoTag) {
      checkUndisplayed = aFrame == mPresShell->FrameConstructor()->
                                     GetDocElementContainingBlock();
      undisplayedParent = nullptr;
    } else {
      checkUndisplayed = !!localContent;
      undisplayedParent = localContent;
    }
    if (checkUndisplayed && mUndisplayedMap) {
      UndisplayedNode* undisplayed =
        mUndisplayedMap->GetFirstNode(undisplayedParent);
      for (TreeMatchContext::AutoAncestorPusher
             pushAncestor(undisplayed, aTreeMatchContext,
                          undisplayedParent ? undisplayedParent->AsElement()
                                            : nullptr);
           undisplayed; undisplayed = undisplayed->mNext) {
        NS_ASSERTION(undisplayedParent ||
                     undisplayed->mContent ==
                       mPresShell->GetDocument()->GetRootElement(),
                     "undisplayed node child of null must be root");
        NS_ASSERTION(!undisplayed->mStyle->GetPseudo(),
                     "Shouldn't have random pseudo style contexts in the "
                     "undisplayed map");
        nsRestyleHint thisChildHint = childRestyleHint;
        RestyleTracker::RestyleData undisplayedRestyleData;
        if (aRestyleTracker.GetRestyleData(undisplayed->mContent->AsElement(),
                                           &undisplayedRestyleData)) {
          thisChildHint =
            nsRestyleHint(thisChildHint | undisplayedRestyleData.mRestyleHint);
        }
        nsRefPtr<nsStyleContext> undisplayedContext;
        if (thisChildHint) {
          undisplayedContext =
            styleSet->ResolveStyleFor(undisplayed->mContent->AsElement(),
                                      newContext,
                                      aTreeMatchContext);
        } else {
          undisplayedContext =
            styleSet->ReparentStyleContext(undisplayed->mStyle, newContext,
                                           undisplayed->mContent->AsElement());
        }
        if (undisplayedContext) {
          const nsStyleDisplay* display = undisplayedContext->StyleDisplay();
          if (display->mDisplay != NS_STYLE_DISPLAY_NONE) {
            NS_ASSERTION(undisplayed->mContent,
                         "Must have undisplayed content");
            aChangeList->AppendChange(nullptr, undisplayed->mContent, 
                                      NS_STYLE_HINT_FRAMECHANGE);
            
            
          } else {
            
            undisplayed->mStyle = undisplayedContext;
          }
        }
      }
    }

    
    
    
    if (!(aMinChange & nsChangeHint_ReconstructFrame) &&
        childRestyleHint) {
      
      
      if (!pseudoTag &&
          ((aFrame->GetStateBits() & NS_FRAME_MAY_HAVE_GENERATED_CONTENT) ||
           
           (aFrame->GetContentInsertionFrame()->GetStateBits() &
            NS_FRAME_MAY_HAVE_GENERATED_CONTENT))) {
        
        
        nsIFrame* prevContinuation = aFrame->GetPrevContinuation();
        if (!prevContinuation) {
          
          
          if (!nsLayoutUtils::GetBeforeFrame(aFrame) &&
              nsLayoutUtils::HasPseudoStyle(localContent, newContext,
                                            nsCSSPseudoElements::ePseudo_before,
                                            aPresContext)) {
            
            NS_UpdateHint(aMinChange, nsChangeHint_ReconstructFrame);
            aChangeList->AppendChange(aFrame, content,
                                      nsChangeHint_ReconstructFrame);
          }
        }
      }
    }

    
    
    
    if (!(aMinChange & nsChangeHint_ReconstructFrame) &&
        childRestyleHint) {
      
      
      if (!pseudoTag &&
          ((aFrame->GetStateBits() & NS_FRAME_MAY_HAVE_GENERATED_CONTENT) ||
           
           (aFrame->GetContentInsertionFrame()->GetStateBits() &
            NS_FRAME_MAY_HAVE_GENERATED_CONTENT))) {
        
        
        nsIFrame* nextContinuation = aFrame->GetNextContinuation();

        if (!nextContinuation) {
          
          
          if (nsLayoutUtils::HasPseudoStyle(localContent, newContext,
                                            nsCSSPseudoElements::ePseudo_after,
                                            aPresContext) &&
              !nsLayoutUtils::GetAfterFrame(aFrame)) {
            
            NS_UpdateHint(aMinChange, nsChangeHint_ReconstructFrame);
            aChangeList->AppendChange(aFrame, content,
                                      nsChangeHint_ReconstructFrame);
          }
        }      
      }
    }

    if (!(aMinChange & nsChangeHint_ReconstructFrame)) {
      DesiredA11yNotifications kidsDesiredA11yNotification =
        aDesiredA11yNotifications;
#ifdef ACCESSIBILITY
      A11yNotificationType ourA11yNotification = eDontNotify;
      
      
      
      if (nsIPresShell::IsAccessibilityActive() &&
          !aFrame->GetPrevContinuation() &&
          !nsLayoutUtils::FrameIsNonFirstInIBSplit(aFrame)) {
        if (aDesiredA11yNotifications == eSendAllNotifications) {
          bool isFrameVisible = newContext->StyleVisibility()->IsVisible();
          if (isFrameVisible != wasFrameVisible) {
            if (isFrameVisible) {
              
              
              
              kidsDesiredA11yNotification = eSkipNotifications;
              ourA11yNotification = eNotifyShown;
            } else {
              
              
              
              
              
              kidsDesiredA11yNotification = eNotifyIfShown;
              ourA11yNotification = eNotifyHidden;
            }
          }
        } else if (aDesiredA11yNotifications == eNotifyIfShown &&
                   newContext->StyleVisibility()->IsVisible()) {
          
          
          aVisibleKidsOfHiddenElement.AppendElement(aFrame->GetContent());
          kidsDesiredA11yNotification = eSkipNotifications;
        }
      }
#endif

      
      
      
      

      
      nsIFrame::ChildListIterator lists(aFrame);
      for (TreeMatchContext::AutoAncestorPusher
             pushAncestor(!lists.IsDone(),
                          aTreeMatchContext,
                          content && content->IsElement() ? content->AsElement()
                                                          : nullptr);
           !lists.IsDone(); lists.Next()) {
        nsFrameList::Enumerator childFrames(lists.CurrentList());
        for (; !childFrames.AtEnd(); childFrames.Next()) {
          nsIFrame* child = childFrames.get();
          if (!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
            
            if (nsGkAtoms::placeholderFrame == child->GetType()) { 
              
              nsIFrame* outOfFlowFrame =
                nsPlaceholderFrame::GetRealFrameForPlaceholder(child);
              NS_ASSERTION(outOfFlowFrame, "no out-of-flow frame");
              NS_ASSERTION(outOfFlowFrame != resolvedChild,
                           "out-of-flow frame not a true descendant");

              
              
              
              
              
              
              
              
              
              

              
              
              do {
                ReResolveStyleContext(aPresContext, outOfFlowFrame,
                                      content, aChangeList,
                                      NS_SubtractHint(aMinChange,
                                                      nsChangeHint_AllReflowHints),
                                      nonInheritedHints,
                                      childRestyleHint,
                                      aRestyleTracker,
                                      kidsDesiredA11yNotification,
                                      aVisibleKidsOfHiddenElement,
                                      aTreeMatchContext);
              } while ((outOfFlowFrame = outOfFlowFrame->GetNextContinuation()));

              
              
              ReResolveStyleContext(aPresContext, child, content,
                                    aChangeList, aMinChange,
                                    nonInheritedHints,
                                    childRestyleHint,
                                    aRestyleTracker,
                                    kidsDesiredA11yNotification,
                                    aVisibleKidsOfHiddenElement,
                                    aTreeMatchContext);
            }
            else {  
              if (child != resolvedChild) {
                ReResolveStyleContext(aPresContext, child, content,
                                      aChangeList, aMinChange,
                                      nonInheritedHints,
                                      childRestyleHint,
                                      aRestyleTracker,
                                      kidsDesiredA11yNotification,
                                      aVisibleKidsOfHiddenElement,
                                      aTreeMatchContext);
              } else {
                NOISY_TRACE_FRAME("child frame already resolved as descendant, skipping",aFrame);
              }
            }
          }
        }
      }
      

#ifdef ACCESSIBILITY
      
      if (ourA11yNotification == eNotifyShown) {
        nsAccessibilityService* accService = nsIPresShell::AccService();
        if (accService) {
          nsIPresShell* presShell = aFrame->PresContext()->GetPresShell();
          nsIContent* content = aFrame->GetContent();

          accService->ContentRangeInserted(presShell, content->GetParent(),
                                           content,
                                           content->GetNextSibling());
        }
      } else if (ourA11yNotification == eNotifyHidden) {
        nsAccessibilityService* accService = nsIPresShell::AccService();
        if (accService) {
          nsIPresShell* presShell = aFrame->PresContext()->GetPresShell();
          nsIContent* content = aFrame->GetContent();
          accService->ContentRemoved(presShell, content->GetParent(), content);

          
          uint32_t visibleContentCount = aVisibleKidsOfHiddenElement.Length();
          for (uint32_t idx = 0; idx < visibleContentCount; idx++) {
            nsIContent* content = aVisibleKidsOfHiddenElement[idx];
            accService->ContentRangeInserted(presShell, content->GetParent(),
                                             content, content->GetNextSibling());
          }
          aVisibleKidsOfHiddenElement.Clear();
        }
      }
#endif
    }
  }

  return aMinChange;
}

void
nsFrameManager::ComputeStyleChangeFor(nsIFrame          *aFrame, 
                                      nsStyleChangeList *aChangeList,
                                      nsChangeHint       aMinChange,
                                      RestyleTracker&    aRestyleTracker,
                                      bool               aRestyleDescendants)
{
  PROFILER_LABEL("CSS", "ComputeStyleChangeFor");

  nsIContent *content = aFrame->GetContent();
  if (aMinChange) {
    aChangeList->AppendChange(aFrame, content, aMinChange);
  }

  nsChangeHint topLevelChange = aMinChange;

  nsIFrame* frame = aFrame;
  nsIFrame* frame2 = aFrame;

  NS_ASSERTION(!frame->GetPrevContinuation(), "must start with the first in flow");

  
  
  

  FramePropertyTable *propTable = GetPresContext()->PropertyTable();

  TreeMatchContext treeMatchContext(true,
                                    nsRuleWalker::eRelevantLinkUnvisited,
                                    mPresShell->GetDocument());
  nsIContent *parent = content ? content->GetParent() : nullptr;
  Element *parentElement =
    parent && parent->IsElement() ? parent->AsElement() : nullptr;
  treeMatchContext.InitAncestors(parentElement);
  nsTArray<nsIContent*> visibleKidsOfHiddenElement;
  do {
    
    do {
      
      nsChangeHint frameChange =
        ReResolveStyleContext(GetPresContext(), frame, nullptr,
                              aChangeList, topLevelChange, nsChangeHint(0),
                              aRestyleDescendants ?
                                eRestyle_Subtree : eRestyle_Self,
                              aRestyleTracker,
                              eSendAllNotifications,
                              visibleKidsOfHiddenElement,
                              treeMatchContext);
      NS_UpdateHint(topLevelChange, frameChange);

      if (topLevelChange & nsChangeHint_ReconstructFrame) {
        
        
        
        NS_ASSERTION(!frame->GetPrevContinuation(),
                     "continuing frame had more severe impact than first-in-flow");
        return;
      }

      frame = frame->GetNextContinuation();
    } while (frame);

    
    if (!(frame2->GetStateBits() & NS_FRAME_IS_SPECIAL)) {
      
      return;
    }
    
    frame2 = static_cast<nsIFrame*>
      (propTable->Get(frame2, nsIFrame::IBSplitSpecialSibling()));
    frame = frame2;
  } while (frame2);
}



void
nsFrameManager::CaptureFrameStateFor(nsIFrame* aFrame,
                                     nsILayoutHistoryState* aState)
{
  if (!aFrame || !aState) {
    NS_WARNING("null frame, or state");
    return;
  }

  
  nsIStatefulFrame* statefulFrame = do_QueryFrame(aFrame);
  if (!statefulFrame) {
    return;
  }

  
  nsAutoPtr<nsPresState> frameState;
  nsresult rv = statefulFrame->SaveState(getter_Transfers(frameState));
  if (!frameState) {
    return;
  }

  
  
  nsAutoCString stateKey;
  nsIContent* content = aFrame->GetContent();
  nsIDocument* doc = content ? content->GetCurrentDoc() : nullptr;
  rv = nsContentUtils::GenerateStateKey(content, doc, stateKey);
  if(NS_FAILED(rv) || stateKey.IsEmpty()) {
    return;
  }

  
  rv = aState->AddState(stateKey, frameState);
  if (NS_SUCCEEDED(rv)) {
    
    frameState.forget();
  }
}

void
nsFrameManager::CaptureFrameState(nsIFrame* aFrame,
                                  nsILayoutHistoryState* aState)
{
  NS_PRECONDITION(nullptr != aFrame && nullptr != aState, "null parameters passed in");

  CaptureFrameStateFor(aFrame, aState);

  
  nsIFrame::ChildListIterator lists(aFrame);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* child = childFrames.get();
      if (child->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
        
        continue;
      }
      
      
      
      CaptureFrameState(nsPlaceholderFrame::GetRealFrameFor(child), aState);
    }
  }
}



void
nsFrameManager::RestoreFrameStateFor(nsIFrame* aFrame,
                                     nsILayoutHistoryState* aState)
{
  if (!aFrame || !aState) {
    NS_WARNING("null frame or state");
    return;
  }

  
  nsIStatefulFrame* statefulFrame = do_QueryFrame(aFrame);
  if (!statefulFrame) {
    return;
  }

  
  
  nsIContent* content = aFrame->GetContent();
  
  
  if (!content) {
    return;
  }

  nsAutoCString stateKey;
  nsIDocument* doc = content->GetCurrentDoc();
  nsresult rv = nsContentUtils::GenerateStateKey(content, doc, stateKey);
  if (NS_FAILED(rv) || stateKey.IsEmpty()) {
    return;
  }

  
  nsPresState *frameState;
  rv = aState->GetState(stateKey, &frameState);
  if (!frameState) {
    return;
  }

  
  rv = statefulFrame->RestoreState(frameState);
  if (NS_FAILED(rv)) {
    return;
  }

  
  aState->RemoveState(stateKey);
}

void
nsFrameManager::RestoreFrameState(nsIFrame* aFrame,
                                  nsILayoutHistoryState* aState)
{
  NS_PRECONDITION(nullptr != aFrame && nullptr != aState, "null parameters passed in");
  
  RestoreFrameStateFor(aFrame, aState);

  
  nsIFrame::ChildListIterator lists(aFrame);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      RestoreFrameState(childFrames.get(), aState);
    }
  }
}



static PLHashNumber
HashKey(void* key)
{
  return NS_PTR_TO_INT32(key);
}

static int
CompareKeys(void* key1, void* key2)
{
  return key1 == key2;
}



nsFrameManagerBase::UndisplayedMap::UndisplayedMap(uint32_t aNumBuckets)
{
  MOZ_COUNT_CTOR(nsFrameManagerBase::UndisplayedMap);
  mTable = PL_NewHashTable(aNumBuckets, (PLHashFunction)HashKey,
                           (PLHashComparator)CompareKeys,
                           (PLHashComparator)nullptr,
                           nullptr, nullptr);
  mLastLookup = nullptr;
}

nsFrameManagerBase::UndisplayedMap::~UndisplayedMap(void)
{
  MOZ_COUNT_DTOR(nsFrameManagerBase::UndisplayedMap);
  Clear();
  PL_HashTableDestroy(mTable);
}

PLHashEntry**  
nsFrameManagerBase::UndisplayedMap::GetEntryFor(nsIContent* aParentContent)
{
  if (mLastLookup && (aParentContent == (*mLastLookup)->key)) {
    return mLastLookup;
  }
  PLHashNumber hashCode = NS_PTR_TO_INT32(aParentContent);
  PLHashEntry** entry = PL_HashTableRawLookup(mTable, hashCode, aParentContent);
  if (*entry) {
    mLastLookup = entry;
  }
  return entry;
}

UndisplayedNode* 
nsFrameManagerBase::UndisplayedMap::GetFirstNode(nsIContent* aParentContent)
{
  PLHashEntry** entry = GetEntryFor(aParentContent);
  if (*entry) {
    return (UndisplayedNode*)((*entry)->value);
  }
  return nullptr;
}

void
nsFrameManagerBase::UndisplayedMap::AppendNodeFor(UndisplayedNode* aNode,
                                                  nsIContent* aParentContent)
{
  PLHashEntry** entry = GetEntryFor(aParentContent);
  if (*entry) {
    UndisplayedNode*  node = (UndisplayedNode*)((*entry)->value);
    while (node->mNext) {
      if (node->mContent == aNode->mContent) {
        
        
        
        NS_NOTREACHED("node in map twice");
        delete aNode;
        return;
      }
      node = node->mNext;
    }
    node->mNext = aNode;
  }
  else {
    PLHashNumber hashCode = NS_PTR_TO_INT32(aParentContent);
    PL_HashTableRawAdd(mTable, entry, hashCode, aParentContent, aNode);
    mLastLookup = nullptr; 
  }
}

nsresult 
nsFrameManagerBase::UndisplayedMap::AddNodeFor(nsIContent* aParentContent,
                                               nsIContent* aChild, 
                                               nsStyleContext* aStyle)
{
  UndisplayedNode*  node = new UndisplayedNode(aChild, aStyle);

  AppendNodeFor(node, aParentContent);
  return NS_OK;
}

void
nsFrameManagerBase::UndisplayedMap::RemoveNodeFor(nsIContent* aParentContent,
                                                  UndisplayedNode* aNode)
{
  PLHashEntry** entry = GetEntryFor(aParentContent);
  NS_ASSERTION(*entry, "content not in map");
  if (*entry) {
    if ((UndisplayedNode*)((*entry)->value) == aNode) {  
      if (aNode->mNext) {
        (*entry)->value = aNode->mNext;
        aNode->mNext = nullptr;
      }
      else {
        PL_HashTableRawRemove(mTable, entry, *entry);
        mLastLookup = nullptr; 
      }
    }
    else {
      UndisplayedNode*  node = (UndisplayedNode*)((*entry)->value);
      while (node->mNext) {
        if (node->mNext == aNode) {
          node->mNext = aNode->mNext;
          aNode->mNext = nullptr;
          break;
        }
        node = node->mNext;
      }
    }
  }
  delete aNode;
}

void
nsFrameManagerBase::UndisplayedMap::RemoveNodesFor(nsIContent* aParentContent)
{
  PLHashEntry** entry = GetEntryFor(aParentContent);
  NS_ASSERTION(entry, "content not in map");
  if (*entry) {
    UndisplayedNode*  node = (UndisplayedNode*)((*entry)->value);
    NS_ASSERTION(node, "null node for non-null entry in UndisplayedMap");
    delete node;
    PL_HashTableRawRemove(mTable, entry, *entry);
    mLastLookup = nullptr; 
  }
}

static int
RemoveUndisplayedEntry(PLHashEntry* he, int i, void* arg)
{
  UndisplayedNode*  node = (UndisplayedNode*)(he->value);
  delete node;
  
  return HT_ENUMERATE_REMOVE | HT_ENUMERATE_NEXT;
}

void
nsFrameManagerBase::UndisplayedMap::Clear(void)
{
  mLastLookup = nullptr;
  PL_HashTableEnumerateEntries(mTable, RemoveUndisplayedEntry, 0);
}

uint32_t nsFrameManagerBase::sGlobalGenerationNumber;
