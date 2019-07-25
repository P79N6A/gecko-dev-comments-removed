

















































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
#ifdef NS_DEBUG
#include "nsIStyleRule.h"
#endif
#include "nsILayoutHistoryState.h"
#include "nsPresState.h"
#include "nsIContent.h"
#include "nsINameSpaceManager.h"
#include "nsIDocument.h"
#include "nsIScrollableFrame.h"

#include "nsIHTMLDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIFormControl.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIForm.h"
#include "nsContentUtils.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsPrintfCString.h"
#include "nsLayoutErrors.h"
#include "nsLayoutUtils.h"
#include "nsAutoPtr.h"
#include "imgIRequest.h"
#include "nsTransitionManager.h"
#include "RestyleTracker.h"
#include "nsAbsoluteContainingBlock.h"

#include "nsFrameManager.h"
#include "nsRuleProcessorData.h"

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

static PRBool
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
      mNext(nsnull)
  {
    MOZ_COUNT_CTOR(UndisplayedNode);
  }

  NS_HIDDEN ~UndisplayedNode()
  {
    MOZ_COUNT_DTOR(UndisplayedNode);

    
    UndisplayedNode *cur = mNext;
    while (cur) {
      UndisplayedNode *next = cur->mNext;
      cur->mNext = nsnull;
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
  UndisplayedMap(PRUint32 aNumBuckets = 16) NS_HIDDEN;
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



nsFrameManager::nsFrameManager()
{
}

nsFrameManager::~nsFrameManager()
{
  NS_ASSERTION(!mPresShell, "nsFrameManager::Destroy never called");
}

nsresult
nsFrameManager::Init(nsIPresShell* aPresShell,
                     nsStyleSet*  aStyleSet)
{
  if (!aPresShell) {
    NS_ERROR("null pres shell");
    return NS_ERROR_FAILURE;
  }

  if (!aStyleSet) {
    NS_ERROR("null style set");
    return NS_ERROR_FAILURE;
  }

  mPresShell = aPresShell;
  mStyleSet = aStyleSet;
  return NS_OK;
}

void
nsFrameManager::Destroy()
{
  NS_ASSERTION(mPresShell, "Frame manager already shut down.");

  
  mPresShell->SetIgnoreFrameDestruction(PR_TRUE);

  
  nsFrameManager::ClearPlaceholderFrameMap();

  if (mRootFrame) {
    mRootFrame->Destroy();
    mRootFrame = nsnull;
  }
  
  delete mUndisplayedMap;
  mUndisplayedMap = nsnull;

  mPresShell = nsnull;
}




nsPlaceholderFrame*
nsFrameManager::GetPlaceholderFrameFor(nsIFrame*  aFrame)
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

  return nsnull;
}

nsresult
nsFrameManager::RegisterPlaceholderFrame(nsPlaceholderFrame* aPlaceholderFrame)
{
  NS_PRECONDITION(aPlaceholderFrame, "null param unexpected");
  NS_PRECONDITION(nsGkAtoms::placeholderFrame == aPlaceholderFrame->GetType(),
                  "unexpected frame type");
  if (!mPlaceholderMap.ops) {
    if (!PL_DHashTableInit(&mPlaceholderMap, &PlaceholderMapOps, nsnull,
                           sizeof(PlaceholderMapEntry), 16)) {
      mPlaceholderMap.ops = nsnull;
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
                       PRUint32 number, void* arg)
{
  PlaceholderMapEntry* entry = static_cast<PlaceholderMapEntry*>(hdr);
  entry->placeholderFrame->SetOutOfFlowFrame(nsnull);
  return PL_DHASH_NEXT;
}

void
nsFrameManager::ClearPlaceholderFrameMap()
{
  if (mPlaceholderMap.ops) {
    PL_DHashTableEnumerate(&mPlaceholderMap, UnregisterPlaceholders, nsnull);
    PL_DHashTableFinish(&mPlaceholderMap);
    mPlaceholderMap.ops = nsnull;
  }
}



nsStyleContext*
nsFrameManager::GetUndisplayedContent(nsIContent* aContent)
{
  if (!aContent || !mUndisplayedMap)
    return nsnull;

  nsIContent* parent = aContent->GetParent();
  for (UndisplayedNode* node = mUndisplayedMap->GetFirstNode(parent);
         node; node = node->mNext) {
    if (node->mContent == aContent)
      return node->mStyle;
  }

  return nsnull;
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
        NS_ASSERTION(context == nsnull, "Found more undisplayed content data after removal");
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
    aParentContent->GetOwnerDoc()->BindingManager()->GetXBLChildNodesFor(aParentContent);
  if (list) {
    PRUint32 length;
    list->GetLength(&length);
    for (PRUint32 i = 0; i < length; ++i) {
      nsIContent* child = list->GetNodeAt(i);
      if (child->GetParent() != aParentContent) {
        ClearUndisplayedContentIn(child, child->GetParent());
      }
    }
  }
}


nsresult
nsFrameManager::AppendFrames(nsIFrame*       aParentFrame,
                             nsIAtom*        aListName,
                             nsFrameList&    aFrameList)
{
  if (aParentFrame->IsAbsoluteContainer() &&
      aListName == aParentFrame->GetAbsoluteListName()) {
    return aParentFrame->GetAbsoluteContainingBlock()->
           AppendFrames(aParentFrame, aListName, aFrameList);
  } else {
    return aParentFrame->AppendFrames(aListName, aFrameList);
  }
}

nsresult
nsFrameManager::InsertFrames(nsIFrame*       aParentFrame,
                             nsIAtom*        aListName,
                             nsIFrame*       aPrevFrame,
                             nsFrameList&    aFrameList)
{
  NS_PRECONDITION(!aPrevFrame || (!aPrevFrame->GetNextContinuation()
                  || IS_TRUE_OVERFLOW_CONTAINER(aPrevFrame->GetNextContinuation()))
                  && !IS_TRUE_OVERFLOW_CONTAINER(aPrevFrame),
                  "aPrevFrame must be the last continuation in its chain!");

  if (aParentFrame->IsAbsoluteContainer() &&
      aListName == aParentFrame->GetAbsoluteListName()) {
    return aParentFrame->GetAbsoluteContainingBlock()->
           InsertFrames(aParentFrame, aListName, aPrevFrame, aFrameList);
  } else {
    return aParentFrame->InsertFrames(aListName, aPrevFrame, aFrameList);
  }
}

nsresult
nsFrameManager::RemoveFrame(nsIAtom*        aListName,
                            nsIFrame*       aOldFrame)
{
  PRBool wasDestroyingFrames = mIsDestroyingFrames;
  mIsDestroyingFrames = PR_TRUE;

  
  
  
  
  
  
  aOldFrame->InvalidateFrameSubtree();

  NS_ASSERTION(!aOldFrame->GetPrevContinuation() ||
               
               aOldFrame->GetType() == nsGkAtoms::textFrame,
               "Must remove first continuation.");
  NS_ASSERTION(!(aOldFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW &&
                 GetPlaceholderFrameFor(aOldFrame)),
               "Must call RemoveFrame on placeholder for out-of-flows.");
  nsresult rv = NS_OK;
  nsIFrame* parentFrame = aOldFrame->GetParent();
  if (parentFrame->IsAbsoluteContainer() &&
      aListName == parentFrame->GetAbsoluteListName()) {
    parentFrame->GetAbsoluteContainingBlock()->
      RemoveFrame(parentFrame, aListName, aOldFrame);
  } else {
    rv = parentFrame->RemoveFrame(aListName, aOldFrame);
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

#ifdef NS_DEBUG
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
    aContext = aFrame->GetStyleContext();
  }

  if (!aParentContext) {
    
    
    

    
    nsIFrame* providerFrame = nsnull;
    PRBool providerIsChild;
    aFrame->GetParentStyleContextFrame(aPresContext,
                                       &providerFrame, &providerIsChild);
    if (providerFrame)
      aParentContext = providerFrame->GetStyleContext();
    
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
        DumpContext(nsnull, actualParentContext);
        fputs("should be using: ", stdout);
        DumpContext(nsnull, aParentContext);
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
      DumpContext(nsnull, actualParentContext);
      fputs("Should be null\n\n", stdout);
    }
  }

  nsStyleContext* childStyleIfVisited = aContext->GetStyleIfVisited();
  
  
  
  if (childStyleIfVisited &&
      !((childStyleIfVisited->GetRuleNode() != aContext->GetRuleNode() &&
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
  nsStyleContext*  context = aFrame->GetStyleContext();
  VerifyContextParent(aPresContext, aFrame, context, nsnull);

  PRInt32 listIndex = 0;
  nsIAtom* childList = nsnull;
  nsIFrame* child;

  do {
    child = aFrame->GetFirstChild(childList);
    while (child) {
      if (!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW)
          || (child->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)) {
        
        if (nsGkAtoms::placeholderFrame == child->GetType()) { 
          
          
          nsIFrame* outOfFlowFrame =
            nsPlaceholderFrame::GetRealFrameForPlaceholder(child);

          
          VerifyStyleTree(aPresContext, outOfFlowFrame, nsnull);

          
          
          VerifyContextParent(aPresContext, child, nsnull, nsnull);
        }
        else { 
          VerifyStyleTree(aPresContext, child, nsnull);
        }
      }
      child = child->GetNextSibling();
    }

    childList = aFrame->GetAdditionalChildListName(listIndex++);
  } while (childList);
  
  
  PRInt32 contextIndex = -1;
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
    nsStyleContext* context = aFrame->GetStyleContext();
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
    return nsnull;
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

  
  
  nsStyleContext* oldContext = aFrame->GetStyleContext();
  
  if (oldContext) {
    nsRefPtr<nsStyleContext> newContext;
    nsIFrame* providerFrame = nsnull;
    PRBool providerIsChild = PR_FALSE;
    nsIFrame* providerChild = nsnull;
    aFrame->GetParentStyleContextFrame(GetPresContext(), &providerFrame,
                                       &providerIsChild);
    nsStyleContext* newParentContext = nsnull;
    if (providerIsChild) {
      ReparentStyleContext(providerFrame);
      newParentContext = providerFrame->GetStyleContext();
      providerChild = providerFrame;
    } else if (providerFrame) {
      newParentContext = providerFrame->GetStyleContext();
    } else {
      NS_NOTREACHED("Reparenting something that has no usable parent? "
                    "Shouldn't happen!");
    }
    
    
    
    
    

#ifdef DEBUG
    {
      
      
      
      
      
      
      
      
      nsIFrame *nextContinuation = aFrame->GetNextContinuation();
      if (nextContinuation) {
        nsStyleContext *nextContinuationContext =
          nextContinuation->GetStyleContext();
        NS_ASSERTION(oldContext == nextContinuationContext ||
                     oldContext->GetPseudo() !=
                       nextContinuationContext->GetPseudo() ||
                     oldContext->GetParent() !=
                       nextContinuationContext->GetParent(),
                     "continuations should have the same style context");
      }
    }
#endif

    nsIFrame *prevContinuation = aFrame->GetPrevContinuation();
    nsStyleContext *prevContinuationContext;
    PRBool copyFromContinuation =
      prevContinuation &&
      (prevContinuationContext = prevContinuation->GetStyleContext())
        ->GetPseudo() == oldContext->GetPseudo() &&
       prevContinuationContext->GetParent() == newParentContext;
    if (copyFromContinuation) {
      
      
      
      
      newContext = prevContinuationContext;
    } else {
      nsIFrame* parentFrame = aFrame->GetParent();
      Element* element =
        ElementForStyleContext(parentFrame ? parentFrame->GetContent() : nsnull,
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

        
        
        nsChangeHint styleChange = oldContext->CalcStyleDifference(newContext);
        
        
        
        
        NS_ASSERTION(!(styleChange & nsChangeHint_ReconstructFrame),
                     "Our frame tree is likely to be bogus!");
        
        PRInt32 listIndex = 0;
        nsIAtom* childList = nsnull;
        nsIFrame* child;
          
        aFrame->SetStyleContext(newContext);

        do {
          child = aFrame->GetFirstChild(childList);
          while (child) {
            
            if ((!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW) ||
                 (child->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)) &&
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

            child = child->GetNextSibling();
          }

          childList = aFrame->GetAdditionalChildListName(listIndex++);
        } while (childList);

        
        
        
        
        
        
        if ((aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL) &&
            !aFrame->GetPrevContinuation()) {
          nsIFrame* sib = static_cast<nsIFrame*>
            (aFrame->Properties().Get(nsIFrame::IBSplitSpecialSibling()));
          if (sib) {
            ReparentStyleContext(sib);
          }
        }

        
        PRInt32 contextIndex = -1;
        while (1) {
          nsStyleContext* oldExtraContext =
            aFrame->GetAdditionalStyleContext(++contextIndex);
          if (oldExtraContext) {
            nsRefPtr<nsStyleContext> newExtraContext;
            newExtraContext = mStyleSet->ReparentStyleContext(oldExtraContext,
                                                              newContext,
                                                              nsnull);
            if (newExtraContext) {
              if (newExtraContext != oldExtraContext) {
                
                
                
                styleChange =
                  oldExtraContext->CalcStyleDifference(newExtraContext);
                
                
                
                
                
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

static nsChangeHint
CaptureChange(nsStyleContext* aOldContext, nsStyleContext* aNewContext,
              nsIFrame* aFrame, nsIContent* aContent,
              nsStyleChangeList* aChangeList, nsChangeHint aMinChange,
              nsChangeHint aChangeToAssume)
{
  nsChangeHint ourChange = aOldContext->CalcStyleDifference(aNewContext);
  NS_ASSERTION(!(ourChange & nsChangeHint_ReflowFrame) ||
               (ourChange & nsChangeHint_NeedReflow),
               "Reflow hint bits set without actually asking for a reflow");

  NS_UpdateHint(ourChange, aChangeToAssume);
  if (NS_UpdateHint(aMinChange, ourChange)) {
    if (!(ourChange & nsChangeHint_ReconstructFrame) || aContent) {
      aChangeList->AppendChange(aFrame, aContent, ourChange);
    }
  }
  return aMinChange;
}









nsChangeHint
nsFrameManager::ReResolveStyleContext(nsPresContext     *aPresContext,
                                      nsIFrame          *aFrame,
                                      nsIContent        *aParentContent,
                                      nsStyleChangeList *aChangeList, 
                                      nsChangeHint       aMinChange,
                                      nsRestyleHint      aRestyleHint,
                                      RestyleTracker&    aRestyleTracker,
                                      DesiredA11yNotifications aDesiredA11yNotifications,
                                      nsTArray<nsIContent*>& aVisibleKidsOfHiddenElement,
                                      TreeMatchContext &aTreeMatchContext)
{
  if (!NS_IsHintSubset(nsChangeHint_NeedDirtyReflow, aMinChange)) {
    
    
    
    
    
    aMinChange = NS_SubtractHint(aMinChange, nsChangeHint_ReflowFrame);
  } else if (!NS_IsHintSubset(nsChangeHint_ClearDescendantIntrinsics,
                              aMinChange)) {
    
    
    
    
    
    aMinChange =
      NS_SubtractHint(aMinChange, nsChangeHint_ClearAncestorIntrinsics);
  }

  
  
  
  
  
  
  
  NS_ASSERTION(aFrame->GetContent() || !aParentContent ||
               !aParentContent->GetParent(),
               "frame must have content (unless at the top of the tree)");
  
  
  

  
  
  

  nsChangeHint assumeDifferenceHint = NS_STYLE_HINT_NONE;
  
  nsStyleContext* oldContext = aFrame->GetStyleContext();
  nsStyleSet* styleSet = aPresContext->StyleSet();

  
  
  if (oldContext) {
    oldContext->AddRef();

#ifdef ACCESSIBILITY
    PRBool wasFrameVisible = mPresShell->IsAccessibilityActive() ?
      oldContext->GetStyleVisibility()->IsVisible() : PR_FALSE;
#endif

    nsIAtom* const pseudoTag = oldContext->GetPseudo();
    const nsCSSPseudoElements::Type pseudoType = oldContext->GetPseudoType();
    nsIContent* localContent = aFrame->GetContent();
    
    
    
    
    
    nsIContent* content = localContent ? localContent : aParentContent;

    if (content && content->IsElement()) {
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
    nsIFrame* resolvedChild = nsnull;
    
    
    nsIFrame* providerFrame = nsnull;
    PRBool providerIsChild = PR_FALSE;
    aFrame->GetParentStyleContextFrame(aPresContext,
                                       &providerFrame, &providerIsChild); 
    if (!providerIsChild) {
      if (providerFrame)
        parentContext = providerFrame->GetStyleContext();
      else
        parentContext = nsnull;
    }
    else {
      

      
      
      
      
      
      

      assumeDifferenceHint = ReResolveStyleContext(aPresContext, providerFrame,
                                                   aParentContent, aChangeList,
                                                   aMinChange, aRestyleHint,
                                                   aRestyleTracker,
                                                   aDesiredA11yNotifications,
                                                   aVisibleKidsOfHiddenElement,
                                                   aTreeMatchContext);

      
      
      parentContext = providerFrame->GetStyleContext();
      
      
      resolvedChild = providerFrame;
    }

#ifdef DEBUG
    {
      
      
      
      
      
      
      
      
      nsIFrame *nextContinuation = aFrame->GetNextContinuation();
      if (nextContinuation) {
        nsStyleContext *nextContinuationContext =
          nextContinuation->GetStyleContext();
        NS_ASSERTION(oldContext == nextContinuationContext ||
                     oldContext->GetPseudo() !=
                       nextContinuationContext->GetPseudo() ||
                     oldContext->GetParent() !=
                       nextContinuationContext->GetParent(),
                     "continuations should have the same style context");
      }
    }
#endif

    
    nsRefPtr<nsStyleContext> newContext;
    nsIFrame *prevContinuation = aFrame->GetPrevContinuation();
    nsStyleContext *prevContinuationContext;
    PRBool copyFromContinuation =
      prevContinuation &&
      (prevContinuationContext = prevContinuation->GetStyleContext())
        ->GetPseudo() == oldContext->GetPseudo() &&
       prevContinuationContext->GetParent() == parentContext;
    if (copyFromContinuation) {
      
      
      
      
      newContext = prevContinuationContext;
    }
    else if (pseudoTag == nsCSSAnonBoxes::mozNonElement) {
      NS_ASSERTION(localContent,
                   "non pseudo-element frame without content node");
      newContext = styleSet->ResolveStyleForNonElement(parentContext);
    }
    else if (!aRestyleHint) {
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
        newContext = styleSet->ResolveStyleFor(element, parentContext,
                                               aTreeMatchContext);
      }
    }

    NS_ASSERTION(newContext, "failed to get new style context");
    if (newContext) {
      if (!parentContext) {
        if (oldContext->GetRuleNode() == newContext->GetRuleNode() &&
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

        aMinChange = CaptureChange(oldContext, newContext, aFrame,
                                   content, aChangeList, aMinChange,
                                   assumeDifferenceHint);
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

    
    
    
    PRInt32 contextIndex = -1;
    while (1 == 1) {
      nsStyleContext* oldExtraContext = nsnull;
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
            aMinChange = CaptureChange(oldExtraContext, newExtraContext,
                                       aFrame, content, aChangeList,
                                       aMinChange, assumeDifferenceHint);
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

    

    
    
    
    PRBool checkUndisplayed;
    nsIContent *undisplayedParent;
    if (pseudoTag) {
      checkUndisplayed = aFrame == mPresShell->FrameConstructor()->
                                     GetDocElementContainingBlock();
      undisplayedParent = nsnull;
    } else {
      checkUndisplayed = !!localContent;
      undisplayedParent = localContent;
    }
    if (checkUndisplayed && mUndisplayedMap) {
      for (UndisplayedNode* undisplayed =
                              mUndisplayedMap->GetFirstNode(undisplayedParent);
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
          const nsStyleDisplay* display = undisplayedContext->GetStyleDisplay();
          if (display->mDisplay != NS_STYLE_DISPLAY_NONE) {
            NS_ASSERTION(undisplayed->mContent,
                         "Must have undisplayed content");
            aChangeList->AppendChange(nsnull, undisplayed->mContent, 
                                      NS_STYLE_HINT_FRAMECHANGE);
            
            
          } else {
            
            undisplayed->mStyle = undisplayedContext;
          }
        }
      }
    }

    
    
    
    if (!(aMinChange & nsChangeHint_ReconstructFrame) &&
        childRestyleHint) {
      
      
      if (!pseudoTag && localContent && localContent->IsElement() &&
          !aFrame->IsLeaf()) {
        
        
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
      
      
      if (!pseudoTag && localContent && localContent->IsElement() &&
          !aFrame->IsLeaf()) {
        
        
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
      
      
      
      if (mPresShell->IsAccessibilityActive() && !aFrame->GetPrevContinuation() &&
          !nsLayoutUtils::FrameIsNonFirstInIBSplit(aFrame)) {
        if (aDesiredA11yNotifications == eSendAllNotifications) {
          PRBool isFrameVisible = newContext->GetStyleVisibility()->IsVisible();
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
                   newContext->GetStyleVisibility()->IsVisible()) {
          
          
          aVisibleKidsOfHiddenElement.AppendElement(aFrame->GetContent());
          kidsDesiredA11yNotification = eSkipNotifications;
        }
      }
#endif

      
      
      
      

      
      PRInt32 listIndex = 0;
      nsIAtom* childList = nsnull;

      do {
        nsIFrame* child = aFrame->GetFirstChild(childList);
        while (child) {
          if (!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW)
              || (child->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)) {
            
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
                                                      nsChangeHint_ReflowFrame),
                                      childRestyleHint,
                                      aRestyleTracker,
                                      kidsDesiredA11yNotification,
                                      aVisibleKidsOfHiddenElement,
                                      aTreeMatchContext);
              } while ((outOfFlowFrame = outOfFlowFrame->GetNextContinuation()));

              
              
              ReResolveStyleContext(aPresContext, child, content,
                                    aChangeList, aMinChange,
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
          child = child->GetNextSibling();
        }

        childList = aFrame->GetAdditionalChildListName(listIndex++);
      } while (childList);
      

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

          
          PRUint32 visibleContentCount = aVisibleKidsOfHiddenElement.Length();
          for (PRUint32 idx = 0; idx < visibleContentCount; idx++) {
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
                                      PRBool             aRestyleDescendants)
{
  if (aMinChange) {
    aChangeList->AppendChange(aFrame, aFrame->GetContent(), aMinChange);
  }

  nsChangeHint topLevelChange = aMinChange;

  nsIFrame* frame = aFrame;
  nsIFrame* frame2 = aFrame;

  NS_ASSERTION(!frame->GetPrevContinuation(), "must start with the first in flow");

  
  
  

  FramePropertyTable *propTable = GetPresContext()->PropertyTable();

  TreeMatchContext treeMatchContext(PR_TRUE,
                                    nsRuleWalker::eRelevantLinkUnvisited,
                                    mPresShell->GetDocument());
  nsTArray<nsIContent*> visibleKidsOfHiddenElement;
  do {
    
    do {
      
      nsChangeHint frameChange =
        ReResolveStyleContext(GetPresContext(), frame, nsnull,
                              aChangeList, topLevelChange,
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
                                     nsILayoutHistoryState* aState,
                                     nsIStatefulFrame::SpecialStateID aID)
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
  nsresult rv = statefulFrame->SaveState(aID, getter_Transfers(frameState));
  if (!frameState) {
    return;
  }

  
  
  nsCAutoString stateKey;
  nsIContent* content = aFrame->GetContent();
  nsIDocument* doc = content ? content->GetCurrentDoc() : nsnull;
  rv = nsContentUtils::GenerateStateKey(content, doc, aID, stateKey);
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
  NS_PRECONDITION(nsnull != aFrame && nsnull != aState, "null parameters passed in");

  CaptureFrameStateFor(aFrame, aState);

  
  nsIAtom*  childListName = nsnull;
  PRInt32   childListIndex = 0;
  do {    
    nsIFrame* childFrame = aFrame->GetFirstChild(childListName);
    while (childFrame) {             
      CaptureFrameState(childFrame, aState);
      
      childFrame = childFrame->GetNextSibling();
    }
    childListName = aFrame->GetAdditionalChildListName(childListIndex++);
  } while (childListName);
}



void
nsFrameManager::RestoreFrameStateFor(nsIFrame* aFrame,
                                     nsILayoutHistoryState* aState,
                                     nsIStatefulFrame::SpecialStateID aID)
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

  nsCAutoString stateKey;
  nsIDocument* doc = content->GetCurrentDoc();
  nsresult rv = nsContentUtils::GenerateStateKey(content, doc, aID, stateKey);
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
  NS_PRECONDITION(nsnull != aFrame && nsnull != aState, "null parameters passed in");
  
  RestoreFrameStateFor(aFrame, aState);

  
  nsIAtom*  childListName = nsnull;
  PRInt32   childListIndex = 0;
  do {    
    nsIFrame* childFrame = aFrame->GetFirstChild(childListName);
    while (childFrame) {
      RestoreFrameState(childFrame, aState);
      
      childFrame = childFrame->GetNextSibling();
    }
    childListName = aFrame->GetAdditionalChildListName(childListIndex++);
  } while (childListName);
}



static PLHashNumber
HashKey(void* key)
{
  return NS_PTR_TO_INT32(key);
}

static PRIntn
CompareKeys(void* key1, void* key2)
{
  return key1 == key2;
}



nsFrameManagerBase::UndisplayedMap::UndisplayedMap(PRUint32 aNumBuckets)
{
  MOZ_COUNT_CTOR(nsFrameManagerBase::UndisplayedMap);
  mTable = PL_NewHashTable(aNumBuckets, (PLHashFunction)HashKey,
                           (PLHashComparator)CompareKeys,
                           (PLHashComparator)nsnull,
                           nsnull, nsnull);
  mLastLookup = nsnull;
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
  return nsnull;
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
    mLastLookup = nsnull; 
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
        aNode->mNext = nsnull;
      }
      else {
        PL_HashTableRawRemove(mTable, entry, *entry);
        mLastLookup = nsnull; 
      }
    }
    else {
      UndisplayedNode*  node = (UndisplayedNode*)((*entry)->value);
      while (node->mNext) {
        if (node->mNext == aNode) {
          node->mNext = aNode->mNext;
          aNode->mNext = nsnull;
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
    mLastLookup = nsnull; 
  }
}

static PRIntn
RemoveUndisplayedEntry(PLHashEntry* he, PRIntn i, void* arg)
{
  UndisplayedNode*  node = (UndisplayedNode*)(he->value);
  delete node;
  
  return HT_ENUMERATE_REMOVE | HT_ENUMERATE_NEXT;
}

void
nsFrameManagerBase::UndisplayedMap::Clear(void)
{
  mLastLookup = nsnull;
  PL_HashTableEnumerateEntries(mTable, RemoveUndisplayedEntry, 0);
}
