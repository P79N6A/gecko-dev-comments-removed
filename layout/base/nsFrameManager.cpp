
















































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

#include "nsFrameManager.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#include "nsIAccessibleEvent.h"
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



struct PrimaryFrameMapEntry : public PLDHashEntryHdr {
  
  
  
  
  nsIContent *content;
  nsIFrame *frame;
};

  
  
#if 0
static PRBool
PrimaryFrameMapMatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                         const void *key)
{
  const PrimaryFrameMapEntry *entry =
    static_cast<const PrimaryFrameMapEntry*>(hdr);
  return entry->frame->GetContent() == key;
}

static PLDHashTableOps PrimaryFrameMapOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashVoidPtrKeyStub,
  PrimaryFrameMapMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  NULL
};
#endif 






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

  mIsDestroying = PR_TRUE;  

  
  nsFrameManager::ClearPlaceholderFrameMap();

  if (mRootFrame) {
    mRootFrame->Destroy();
    mRootFrame = nsnull;
  }
  
  nsFrameManager::ClearPrimaryFrameMap();
  delete mUndisplayedMap;
  mUndisplayedMap = nsnull;

  mPresShell = nsnull;
}

nsIFrame*
nsFrameManager::GetCanvasFrame()
{
  if (mRootFrame) {
    
    
    nsIFrame* childFrame = mRootFrame;
    while (childFrame) {
      
      nsIFrame *siblingFrame = childFrame;
      while (siblingFrame) {
        if (siblingFrame->GetType() == nsGkAtoms::canvasFrame) {
          
          return siblingFrame;
        } else {
          siblingFrame = siblingFrame->GetNextSibling();
        }
      }
      
      childFrame = childFrame->GetFirstChild(nsnull);
    }
  }
  return nsnull;
}




nsIFrame*
nsFrameManager::GetPrimaryFrameFor(nsIContent* aContent,
                                   PRInt32 aIndexHint)
{
  NS_ASSERTION(!mIsDestroyingFrames,
               "GetPrimaryFrameFor() called while frames are being destroyed!");
  NS_ENSURE_TRUE(aContent, nsnull);

  if (mIsDestroying) {
    NS_ERROR("GetPrimaryFrameFor() called while nsFrameManager is being destroyed!");
    return nsnull;
  }

  if (!aContent->MayHaveFrame()) {
    return nsnull;
  }

  if (mPrimaryFrameMap.ops) {
    PrimaryFrameMapEntry *entry = static_cast<PrimaryFrameMapEntry*>
                                             (PL_DHashTableOperate(&mPrimaryFrameMap, aContent, PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      return entry->frame;
    }

    
    
    
    
    
    
    
    
    
    

    
    

    
    
    
    
    nsFindFrameHint hint;
    nsIContent* parent = aContent->GetParent();
    if (parent)
    {
      PRInt32 index = aIndexHint >= 0 ? aIndexHint : parent->IndexOf(aContent);
      if (index > 0)  
      {
        nsIContent *prevSibling;
        do {
          prevSibling = parent->GetChildAt(--index);
        } while (index &&
                 (prevSibling->IsNodeOfType(nsINode::eTEXT) ||
                  prevSibling->IsNodeOfType(nsINode::eCOMMENT) ||
                  prevSibling->IsNodeOfType(nsINode::ePROCESSING_INSTRUCTION)));
        if (prevSibling) {
          entry = static_cast<PrimaryFrameMapEntry*>
                             (PL_DHashTableOperate(&mPrimaryFrameMap, prevSibling,
                                               PL_DHASH_LOOKUP));
          
          
          if (PL_DHASH_ENTRY_IS_BUSY(entry) && entry->frame &&
              entry->frame->GetContent() == prevSibling)
            hint.mPrimaryFrameForPrevSibling = entry->frame;
        }
      }
    }

    
    
    nsIFrame *result;

    mPresShell->FrameConstructor()->
      FindPrimaryFrameFor(this, aContent, &result, 
                          hint.mPrimaryFrameForPrevSibling ? &hint : nsnull);

    return result;
  }

  return nsnull;
}

nsresult
nsFrameManager::SetPrimaryFrameFor(nsIContent* aContent,
                                   nsIFrame*   aPrimaryFrame)
{
  NS_ENSURE_ARG_POINTER(aContent);
  NS_ASSERTION(aPrimaryFrame && aPrimaryFrame->GetParent(),
               "BOGUS!");
#ifdef DEBUG
  {
    nsIFrame *docElementCB = 
      mPresShell->FrameConstructor()->GetDocElementContainingBlock();
    NS_ASSERTION(aPrimaryFrame != docElementCB &&
                 !nsLayoutUtils::IsProperAncestorFrame(aPrimaryFrame,
                                                       docElementCB),
                 "too high in the frame tree to be a primary frame");
  }
#endif

  
  
#if 0
  NS_PRECONDITION(aPrimaryFrame->GetContent() == aContent, "wrong content");
#endif

  
  if (!mPrimaryFrameMap.ops) {
    if (!PL_DHashTableInit(&mPrimaryFrameMap, PL_DHashGetStubOps(), nsnull,
                           sizeof(PrimaryFrameMapEntry), 16)) {
      mPrimaryFrameMap.ops = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  
  PrimaryFrameMapEntry *entry = static_cast<PrimaryFrameMapEntry*>
                                           (PL_DHashTableOperate(&mPrimaryFrameMap, aContent, PL_DHASH_ADD));
#ifdef DEBUG_dbaron
  if (entry->frame) {
    NS_WARNING("already have primary frame for content");
  }
#endif
  entry->frame = aPrimaryFrame;
  entry->content = aContent;
    
  return NS_OK;
}

void
nsFrameManager::RemoveAsPrimaryFrame(nsIContent* aContent,
                                     nsIFrame* aPrimaryFrame)
{
  NS_PRECONDITION(aPrimaryFrame, "Must have a frame");
  if (aContent && mPrimaryFrameMap.ops) {
    PrimaryFrameMapEntry *entry = static_cast<PrimaryFrameMapEntry*>
                                             (PL_DHashTableOperate(&mPrimaryFrameMap, aContent, PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry) && entry->frame == aPrimaryFrame) {
      
      
      PL_DHashTableOperate(&mPrimaryFrameMap, aContent, PL_DHASH_REMOVE);
    }
  }

  aPrimaryFrame->RemovedAsPrimaryFrame();
}

void
nsFrameManager::ClearPrimaryFrameMap()
{
  if (mPrimaryFrameMap.ops) {
    PL_DHashTableFinish(&mPrimaryFrameMap);
    mPrimaryFrameMap.ops = nsnull;
  }
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
  NS_PRECONDITION(!aStyleContext->GetPseudoType(),
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
  if (mUndisplayedMap) {
    nsIContent* parent = aContent->GetParent();
    NS_ASSERTION(parent || (mPresShell && mPresShell->GetDocument() &&
                 mPresShell->GetDocument()->GetRootContent() == aContent),
                 "undisplayed content must have a parent, unless it's the root content");
    mUndisplayedMap->AddNodeFor(parent, aContent, aStyleContext);
  }
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

void
nsFrameManager::ClearUndisplayedContentMap()
{
#ifdef DEBUG_UNDISPLAYED_MAP
  static int i = 0;
  printf("ClearUndisplayedContentMap(%d)\n", i++);
#endif

  if (mUndisplayedMap) {
    mUndisplayedMap->Clear();
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

  return aParentFrame->InsertFrames(aListName, aPrevFrame, aFrameList);
}

nsresult
nsFrameManager::RemoveFrame(nsIFrame*       aParentFrame,
                            nsIAtom*        aListName,
                            nsIFrame*       aOldFrame)
{
  PRBool wasDestroyingFrames = mIsDestroyingFrames;
  mIsDestroyingFrames = PR_TRUE;

  
  
  
  
  
  
  aOldFrame->Invalidate(aOldFrame->GetOverflowRect());

  nsresult rv = aParentFrame->RemoveFrame(aListName, aOldFrame);

  mIsDestroyingFrames = wasDestroyingFrames;

  return rv;
}



void
nsFrameManager::NotifyDestroyingFrame(nsIFrame* aFrame)
{
  
  
  
  
  if (mPrimaryFrameMap.ops) {
    PrimaryFrameMapEntry *entry = static_cast<PrimaryFrameMapEntry*>
                                             (PL_DHashTableOperate(&mPrimaryFrameMap, aFrame->GetContent(), PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry) && entry->frame == aFrame) {
      NS_NOTREACHED("frame was not removed from primary frame map before "
                    "destruction or was readded to map after being removed");
      PL_DHashTableRawRemove(&mPrimaryFrameMap, entry);
    }
  }
}

#ifdef NS_DEBUG
static void
DumpContext(nsIFrame* aFrame, nsStyleContext* aContext)
{
  if (aFrame) {
    fputs("frame: ", stdout);
    nsAutoString  name;
    nsIFrameDebug *frameDebug = do_QueryFrame(aFrame);
    if (frameDebug) {
      frameDebug->GetFrameName(name);
      fputs(NS_LossyConvertUTF16toASCII(name).get(), stdout);
    }
    fprintf(stdout, " (%p)", static_cast<void*>(aFrame));
  }
  if (aContext) {
    fprintf(stdout, " style: %p ", static_cast<void*>(aContext));

    nsIAtom* pseudoTag = aContext->GetPseudoType();
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

nsresult
nsFrameManager::ReParentStyleContext(nsIFrame* aFrame)
{
  if (nsGkAtoms::placeholderFrame == aFrame->GetType()) {
    
    nsIFrame* outOfFlow =
      nsPlaceholderFrame::GetRealFrameForPlaceholder(aFrame);
    NS_ASSERTION(outOfFlow, "no out-of-flow frame");

    ReParentStyleContext(outOfFlow);
  }

  
  
  nsStyleContext* oldContext = aFrame->GetStyleContext();
  
  if (oldContext) {
    nsPresContext *presContext = GetPresContext();
    nsRefPtr<nsStyleContext> newContext;
    nsIFrame* providerFrame = nsnull;
    PRBool providerIsChild = PR_FALSE;
    nsIFrame* providerChild = nsnull;
    aFrame->GetParentStyleContextFrame(presContext, &providerFrame,
                                       &providerIsChild);
    nsStyleContext* newParentContext = nsnull;
    if (providerIsChild) {
      ReParentStyleContext(providerFrame);
      newParentContext = providerFrame->GetStyleContext();
      providerChild = providerFrame;
    } else if (providerFrame) {
      newParentContext = providerFrame->GetStyleContext();
    } else {
      NS_NOTREACHED("Reparenting something that has no usable parent? "
                    "Shouldn't happen!");
    }
    
    
    
    
    

    newContext = mStyleSet->ReParentStyleContext(presContext, oldContext,
                                                 newParentContext);
    if (newContext) {
      if (newContext != oldContext) {
        
        
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

              ReParentStyleContext(child);
            }

            child = child->GetNextSibling();
          }

          childList = aFrame->GetAdditionalChildListName(listIndex++);
        } while (childList);

        
        
        
        
        
        
        if ((aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL) &&
            !aFrame->GetPrevInFlow()) {
          nsIFrame* sib = static_cast<nsIFrame*>(aFrame->GetProperty(nsGkAtoms::IBSplitSpecialSibling));
          if (sib) {
            ReParentStyleContext(sib);
          }
        }

        
        PRInt32 contextIndex = -1;
        while (1) {
          nsStyleContext* oldExtraContext =
            aFrame->GetAdditionalStyleContext(++contextIndex);
          if (oldExtraContext) {
            nsRefPtr<nsStyleContext> newExtraContext;
            newExtraContext = mStyleSet->ReParentStyleContext(presContext,
                                                              oldExtraContext,
                                                              newContext);
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
  NS_UpdateHint(ourChange, aChangeToAssume);
  if (NS_UpdateHint(aMinChange, ourChange)) {
    aChangeList->AppendChange(aFrame, aContent, ourChange);
  }
  return aMinChange;
}









nsChangeHint
nsFrameManager::ReResolveStyleContext(nsPresContext     *aPresContext,
                                      nsIFrame          *aFrame,
                                      nsIContent        *aParentContent,
                                      nsStyleChangeList *aChangeList, 
                                      nsChangeHint       aMinChange,
                                      PRBool             aFireAccessibilityEvents)
{
  
  
  
  
  
  
  if (!NS_IsHintSubset(nsChangeHint_NeedDirtyReflow, aMinChange)) {
    aMinChange = NS_SubtractHint(aMinChange, nsChangeHint_NeedReflow);
  }
  
  
  
  
  
  
  
  
  NS_ASSERTION(aFrame->GetContent() || !aParentContent ||
               !aParentContent->GetParent(),
               "frame must have content (unless at the top of the tree)");
  
  
  

  
  
  

  nsChangeHint assumeDifferenceHint = NS_STYLE_HINT_NONE;
  
  nsStyleContext* oldContext = aFrame->GetStyleContext();
  nsStyleSet* styleSet = aPresContext->StyleSet();
#ifdef ACCESSIBILITY
  PRBool isVisible = aFrame->GetStyleVisibility()->IsVisible();
#endif

  
  
  if (oldContext) {
    oldContext->AddRef();
    nsIAtom* const pseudoTag = oldContext->GetPseudoType();
    nsIContent* localContent = aFrame->GetContent();
    
    
    
    
    
    nsIContent* content = localContent ? localContent : aParentContent;

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
                                                   aMinChange, PR_FALSE);

      
      
      parentContext = providerFrame->GetStyleContext();
      
      
      resolvedChild = providerFrame;
    }
    
    
    
    nsStyleContext* newContext = nsnull;
    if (pseudoTag == nsCSSAnonBoxes::mozNonElement) {
      NS_ASSERTION(localContent,
                   "non pseudo-element frame without content node");
      newContext = styleSet->ResolveStyleForNonElement(parentContext).get();
    }
    else if (pseudoTag) {
      
      
      
      
      nsIContent* pseudoContent =
          aParentContent ? aParentContent : localContent;
      if (pseudoTag == nsCSSPseudoElements::before ||
          pseudoTag == nsCSSPseudoElements::after) {
        
        newContext = styleSet->ProbePseudoStyleFor(pseudoContent,
                                                   pseudoTag,
                                                   parentContext).get();
        if (!newContext) {
          
          NS_UpdateHint(aMinChange, nsChangeHint_ReconstructFrame);
          aChangeList->AppendChange(aFrame, pseudoContent,
                                    nsChangeHint_ReconstructFrame);
          
          newContext = oldContext;
          newContext->AddRef();
        }
      } else {
        if (pseudoTag == nsCSSPseudoElements::firstLetter) {
          NS_ASSERTION(aFrame->GetType() == nsGkAtoms::letterFrame, 
                       "firstLetter pseudoTag without a nsFirstLetterFrame");
          nsBlockFrame* block = nsBlockFrame::GetNearestAncestorBlock(aFrame);
          pseudoContent = block->GetContent();
        } else if (pseudoTag == nsCSSAnonBoxes::pageBreak) {
          pseudoContent = nsnull;
        }
        newContext = styleSet->ResolvePseudoStyleFor(pseudoContent,
                                                     pseudoTag,
                                                     parentContext).get();
      }
    }
    else {
      NS_ASSERTION(localContent,
                   "non pseudo-element frame without content node");
      newContext = styleSet->ResolveStyleFor(content, parentContext).get();
    }
    NS_ASSERTION(newContext, "failed to get new style context");
    if (newContext) {
      if (!parentContext) {
        if (oldContext->GetRuleNode() == newContext->GetRuleNode()) {
          
          
          
          
          
          newContext->Release();
          newContext = oldContext;
          newContext->AddRef();
        }
      }

      if (newContext != oldContext) {
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
      oldContext = nsnull;
    }

    
    PRInt32 contextIndex = -1;
    while (1 == 1) {
      nsStyleContext* oldExtraContext = nsnull;
      oldExtraContext = aFrame->GetAdditionalStyleContext(++contextIndex);
      if (oldExtraContext) {
        nsStyleContext* newExtraContext = nsnull;
        nsIAtom* const extraPseudoTag = oldExtraContext->GetPseudoType();
        NS_ASSERTION(extraPseudoTag &&
                     extraPseudoTag != nsCSSAnonBoxes::mozNonElement,
                     "extra style context is not pseudo element");
        newExtraContext = styleSet->ResolvePseudoStyleFor(content,
                                                          extraPseudoTag,
                                                          newContext).get();
        if (newExtraContext) {
          if (oldExtraContext != newExtraContext) {
            aMinChange = CaptureChange(oldExtraContext, newExtraContext,
                                       aFrame, content, aChangeList,
                                       aMinChange, assumeDifferenceHint);
            if (!(aMinChange & nsChangeHint_ReconstructFrame)) {
              aFrame->SetAdditionalStyleContext(contextIndex, newExtraContext);
            }
          }
          newExtraContext->Release();
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
                       mPresShell->GetDocument()->GetRootContent(),
                     "undisplayed node child of null must be root");
        NS_ASSERTION(!undisplayed->mStyle->GetPseudoType(),
                     "Shouldn't have random pseudo style contexts in the "
                     "undisplayed map");
        nsRefPtr<nsStyleContext> undisplayedContext =
          styleSet->ResolveStyleFor(undisplayed->mContent, newContext);
        if (undisplayedContext) {
          const nsStyleDisplay* display = undisplayedContext->GetStyleDisplay();
          if (display->mDisplay != NS_STYLE_DISPLAY_NONE) {
            aChangeList->AppendChange(nsnull,
                                      undisplayed->mContent
                                      ? static_cast<nsIContent*>
                                                   (undisplayed->mContent)
                                      : localContent, 
                                      NS_STYLE_HINT_FRAMECHANGE);
            
            
          } else {
            
            undisplayed->mStyle = undisplayedContext;
          }
        }
      }
    }

    if (!(aMinChange & nsChangeHint_ReconstructFrame)) {
      
      
      if (!pseudoTag && localContent &&
          localContent->IsNodeOfType(nsINode::eELEMENT) &&
          !aFrame->IsLeaf()) {
        
        
        nsIFrame* prevContinuation = aFrame->GetPrevContinuation();
        if (!prevContinuation) {
          
          
          if (!nsLayoutUtils::GetBeforeFrame(aFrame) &&
              nsLayoutUtils::HasPseudoStyle(localContent, newContext,
                                            nsCSSPseudoElements::before,
                                            aPresContext)) {
            
            NS_UpdateHint(aMinChange, nsChangeHint_ReconstructFrame);
            aChangeList->AppendChange(aFrame, content,
                                      nsChangeHint_ReconstructFrame);
          }
        }
      }
    }

    
    if (!(aMinChange & nsChangeHint_ReconstructFrame)) {
      
      
      if (!pseudoTag && localContent &&
          localContent->IsNodeOfType(nsINode::eELEMENT) &&
          !aFrame->IsLeaf()) {
        
        
        nsIFrame* nextContinuation = aFrame->GetNextContinuation();

        if (!nextContinuation) {
          
          
          if (nsLayoutUtils::HasPseudoStyle(localContent, newContext,
                                            nsCSSPseudoElements::after,
                                            aPresContext) &&
              !nsLayoutUtils::GetAfterFrame(aFrame)) {
            
            NS_UpdateHint(aMinChange, nsChangeHint_ReconstructFrame);
            aChangeList->AppendChange(aFrame, content,
                                      nsChangeHint_ReconstructFrame);
          }
        }      
      }
    }

    PRBool fireAccessibilityEvents = aFireAccessibilityEvents;
#ifdef ACCESSIBILITY
    if (fireAccessibilityEvents && mPresShell->IsAccessibilityActive() &&
        aFrame->GetStyleVisibility()->IsVisible() != isVisible &&
        !aFrame->GetPrevContinuation()) {
      
      
      

      
      
      
      nsCOMPtr<nsIAccessibilityService> accService = 
        do_GetService("@mozilla.org/accessibilityService;1");
      if (accService) {
        PRUint32 event = isVisible ?
          nsIAccessibleEvent::EVENT_ASYNCH_HIDE :
          nsIAccessibleEvent::EVENT_ASYNCH_SHOW;
        accService->InvalidateSubtreeFor(mPresShell, aFrame->GetContent(),
                                         event);
        fireAccessibilityEvents = PR_FALSE;
      }
    }
#endif

    if (!(aMinChange & nsChangeHint_ReconstructFrame)) {
      
      
      
      
      

      
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

              
              
              
              
              
              
              
              
              
              

              
              
              ReResolveStyleContext(aPresContext, outOfFlowFrame,
                                    content, aChangeList,
                                    NS_SubtractHint(aMinChange,
                                                    nsChangeHint_ReflowFrame),
                                    fireAccessibilityEvents);

              
              
              ReResolveStyleContext(aPresContext, child, content,
                                    aChangeList, aMinChange,
                                    fireAccessibilityEvents);
            }
            else {  
              if (child != resolvedChild) {
                ReResolveStyleContext(aPresContext, child, content,
                                      aChangeList, aMinChange,
                                      fireAccessibilityEvents);
              } else {
                NOISY_TRACE_FRAME("child frame already resolved as descendant, skipping",aFrame);
              }
            }
          }
          child = child->GetNextSibling();
        }

        childList = aFrame->GetAdditionalChildListName(listIndex++);
      } while (childList);
      
    }

    newContext->Release();
  }

  return aMinChange;
}

void
nsFrameManager::ComputeStyleChangeFor(nsIFrame          *aFrame, 
                                      nsStyleChangeList *aChangeList,
                                      nsChangeHint       aMinChange)
{
  if (aMinChange) {
    aChangeList->AppendChange(aFrame, aFrame->GetContent(), aMinChange);
  }

  nsChangeHint topLevelChange = aMinChange;

  nsIFrame* frame = aFrame;
  nsIFrame* frame2 = aFrame;

  NS_ASSERTION(!frame->GetPrevContinuation(), "must start with the first in flow");

  
  
  

  nsPropertyTable *propTable = GetPresContext()->PropertyTable();

  do {
    
    do {
      
      nsChangeHint frameChange =
        ReResolveStyleContext(GetPresContext(), frame, nsnull,
                              aChangeList, topLevelChange, PR_TRUE);
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
                        (propTable->GetProperty(frame2, nsGkAtoms::IBSplitSpecialSibling));
    frame = frame2;
  } while (frame2);
}


nsReStyleHint
nsFrameManager::HasAttributeDependentStyle(nsIContent *aContent,
                                           nsIAtom *aAttribute,
                                           PRInt32 aModType,
                                           PRUint32 aStateMask)
{
  nsReStyleHint hint = mStyleSet->HasAttributeDependentStyle(GetPresContext(),
                                                             aContent,
                                                             aAttribute,
                                                             aModType,
                                                             aStateMask);

  if (aAttribute == nsGkAtoms::style) {
    
    
    
    hint = nsReStyleHint(hint | eReStyle_Self);
  }

  return hint;
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
  if (! node) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

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
