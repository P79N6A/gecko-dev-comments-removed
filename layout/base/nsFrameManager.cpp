
















#include "nscore.h"
#include "nsIPresShell.h"
#include "nsStyleContext.h"
#include "nsCOMPtr.h"
#include "plhash.h"
#include "nsPlaceholderFrame.h"
#include "nsGkAtoms.h"
#include "nsILayoutHistoryState.h"
#include "nsPresState.h"
#include "mozilla/dom/Element.h"
#include "nsIDocument.h"

#include "nsContentUtils.h"
#include "nsError.h"
#include "nsAutoPtr.h"
#include "nsAbsoluteContainingBlock.h"
#include "ChildIterator.h"

#include "nsFrameManager.h"
#include "GeckoProfiler.h"
#include "nsIStatefulFrame.h"
#include "nsContainerFrame.h"

  #ifdef DEBUG
    
    
  #else
    #undef DEBUG_UNDISPLAYED_MAP
    #undef DEBUG_DISPLAY_CONTENTS_MAP
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

static const PLDHashTableOps PlaceholderMapOps = {
  PL_DHashVoidPtrKeyStub,
  PlaceholderMapMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  nullptr
};






class nsFrameManagerBase::UndisplayedMap {
public:
  explicit UndisplayedMap(uint32_t aNumBuckets = 16);
  ~UndisplayedMap(void);

  UndisplayedNode* GetFirstNode(nsIContent* aParentContent);

  nsresult AddNodeFor(nsIContent* aParentContent,
                                  nsIContent* aChild, nsStyleContext* aStyle);

  void RemoveNodeFor(nsIContent* aParentContent,
                                 UndisplayedNode* aNode);

  void RemoveNodesFor(nsIContent* aParentContent);
  UndisplayedNode* UnlinkNodesFor(nsIContent* aParentContent);

  
  void  Clear(void);

protected:
  




  PLHashEntry** GetEntryFor(nsIContent** aParentContent);
  void          AppendNodeFor(UndisplayedNode* aNode,
                                          nsIContent* aParentContent);

  PLHashTable*  mTable;
  PLHashEntry** mLastLookup;
};



nsFrameManager::~nsFrameManager()
{
  NS_ASSERTION(!mPresShell, "nsFrameManager::Destroy never called");
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
  delete mDisplayContentsMap;
  mDisplayContentsMap = nullptr;

  mPresShell = nullptr;
}




nsPlaceholderFrame*
nsFrameManager::GetPlaceholderFrameFor(const nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "null param unexpected");

  if (mPlaceholderMap.IsInitialized()) {
    PlaceholderMapEntry *entry = static_cast<PlaceholderMapEntry*>
                                            (PL_DHashTableSearch(const_cast<PLDHashTable*>(&mPlaceholderMap),
                                aFrame));
    if (entry) {
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
  if (!mPlaceholderMap.IsInitialized()) {
    PL_DHashTableInit(&mPlaceholderMap, &PlaceholderMapOps,
                      sizeof(PlaceholderMapEntry));
  }
  PlaceholderMapEntry *entry = static_cast<PlaceholderMapEntry*>(PL_DHashTableAdd(&mPlaceholderMap,
                              aPlaceholderFrame->GetOutOfFlowFrame()));
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

  if (mPlaceholderMap.IsInitialized()) {
    PL_DHashTableRemove(&mPlaceholderMap,
                        aPlaceholderFrame->GetOutOfFlowFrame());
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
  if (mPlaceholderMap.IsInitialized()) {
    PL_DHashTableEnumerate(&mPlaceholderMap, UnregisterPlaceholders, nullptr);
    PL_DHashTableFinish(&mPlaceholderMap);
  }
}



 nsStyleContext*
nsFrameManager::GetStyleContextInMap(UndisplayedMap* aMap, nsIContent* aContent)
{
  if (!aContent) {
    return nullptr;
  }
  nsIContent* parent = aContent->GetParent();
  for (UndisplayedNode* node = aMap->GetFirstNode(parent);
         node; node = node->mNext) {
    if (node->mContent == aContent)
      return node->mStyle;
  }

  return nullptr;
}

 UndisplayedNode*
nsFrameManager::GetAllUndisplayedNodesInMapFor(UndisplayedMap* aMap,
                                               nsIContent* aParentContent)
{
  return aMap ? aMap->GetFirstNode(aParentContent) : nullptr;
}

UndisplayedNode*
nsFrameManager::GetAllUndisplayedContentIn(nsIContent* aParentContent)
{
  return GetAllUndisplayedNodesInMapFor(mUndisplayedMap, aParentContent);
}

 void
nsFrameManager::SetStyleContextInMap(UndisplayedMap* aMap,
                                     nsIContent* aContent,
                                     nsStyleContext* aStyleContext)
{
  NS_PRECONDITION(!aStyleContext->GetPseudo(),
                  "Should only have actual elements here");

#if defined(DEBUG_UNDISPLAYED_MAP) || defined(DEBUG_DISPLAY_BOX_CONTENTS_MAP)
  static int i = 0;
  printf("SetStyleContextInMap(%d): p=%p \n", i++, (void *)aContent);
#endif

  NS_ASSERTION(!GetStyleContextInMap(aMap, aContent),
               "Already have an entry for aContent");

  nsIContent* parent = aContent->GetParent();
#ifdef DEBUG
  nsIPresShell* shell = aStyleContext->PresContext()->PresShell();
  NS_ASSERTION(parent || (shell && shell->GetDocument() &&
                          shell->GetDocument()->GetRootElement() == aContent),
               "undisplayed content must have a parent, unless it's the root "
               "element");
#endif
  aMap->AddNodeFor(parent, aContent, aStyleContext);
}

void
nsFrameManager::SetUndisplayedContent(nsIContent* aContent,
                                      nsStyleContext* aStyleContext)
{
  if (!mUndisplayedMap) {
    mUndisplayedMap = new UndisplayedMap;
  }
  SetStyleContextInMap(mUndisplayedMap, aContent, aStyleContext);
}

 void
nsFrameManager::ChangeStyleContextInMap(UndisplayedMap* aMap,
                                        nsIContent* aContent,
                                        nsStyleContext* aStyleContext)
{
  MOZ_ASSERT(aMap, "expecting a map");

#if defined(DEBUG_UNDISPLAYED_MAP) || defined(DEBUG_DISPLAY_BOX_CONTENTS_MAP)
   static int i = 0;
   printf("ChangeStyleContextInMap(%d): p=%p \n", i++, (void *)aContent);
#endif

  for (UndisplayedNode* node = aMap->GetFirstNode(aContent->GetParent());
         node; node = node->mNext) {
    if (node->mContent == aContent) {
      node->mStyle = aStyleContext;
      return;
    }
  }

  MOZ_CRASH("couldn't find the entry to change");
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
#ifdef DEBUG_UNDISPLAYED_MAP
  printf( "not found.\n");
#endif
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

  
  
  
  
  FlattenedChildIterator iter(aParentContent);
  for (nsIContent* child = iter.GetNextChild(); child; child = iter.GetNextChild()) {
    if (child->GetParent() != aParentContent) {
      ClearUndisplayedContentIn(child, child->GetParent());
    }
  }
}



void
nsFrameManager::SetDisplayContents(nsIContent* aContent,
                                   nsStyleContext* aStyleContext)
{
  if (!mDisplayContentsMap) {
    mDisplayContentsMap = new UndisplayedMap;
  }
  SetStyleContextInMap(mDisplayContentsMap, aContent, aStyleContext);
}

UndisplayedNode*
nsFrameManager::GetAllDisplayContentsIn(nsIContent* aParentContent)
{
  return GetAllUndisplayedNodesInMapFor(mDisplayContentsMap, aParentContent);
}

void
nsFrameManager::ClearDisplayContentsIn(nsIContent* aContent,
                                       nsIContent* aParentContent)
{
#ifdef DEBUG_DISPLAY_CONTENTS_MAP
  static int i = 0;
  printf("ClearDisplayContents(%d): content=%p parent=%p --> ", i++, (void *)aContent, (void*)aParentContent);
#endif
  
  if (mDisplayContentsMap) {
    UndisplayedNode* node = mDisplayContentsMap->GetFirstNode(aParentContent);
    while (node) {
      if (node->mContent == aContent) {
        mDisplayContentsMap->RemoveNodeFor(aParentContent, node);

#ifdef DEBUG_DISPLAY_CONTENTS_MAP
        printf( "REMOVED!\n");
#endif
#ifdef DEBUG
        
        nsStyleContext* context = GetDisplayContentsStyleFor(aContent);
        NS_ASSERTION(context == nullptr, "Found more entries for aContent after removal");
#endif
        ClearAllDisplayContentsIn(aContent);
        ClearAllUndisplayedContentIn(aContent);
        return;
      }
      node = node->mNext;
    }
  }
#ifdef DEBUG_DISPLAY_CONTENTS_MAP
  printf( "not found.\n");
#endif
}

void
nsFrameManager::ClearAllDisplayContentsIn(nsIContent* aParentContent)
{
#ifdef DEBUG_DISPLAY_CONTENTS_MAP
  static int i = 0;
  printf("ClearAllDisplayContentsIn(%d): parent=%p \n", i++, (void*)aParentContent);
#endif

  if (mDisplayContentsMap) {
    UndisplayedNode* cur = mDisplayContentsMap->UnlinkNodesFor(aParentContent);
    while (cur) {
      UndisplayedNode* next = cur->mNext;
      cur->mNext = nullptr;
      ClearAllDisplayContentsIn(cur->mContent);
      ClearAllUndisplayedContentIn(cur->mContent);
      delete cur;
      cur = next;
    }
  }

  
  
  
  
  FlattenedChildIterator iter(aParentContent);
  for (nsIContent* child = iter.GetNextChild(); child; child = iter.GetNextChild()) {
    if (child->GetParent() != aParentContent) {
      ClearDisplayContentsIn(child, child->GetParent());
      ClearUndisplayedContentIn(child, child->GetParent());
    }
  }
}


void
nsFrameManager::AppendFrames(nsContainerFrame* aParentFrame,
                             ChildListID       aListID,
                             nsFrameList&      aFrameList)
{
  if (aParentFrame->IsAbsoluteContainer() &&
      aListID == aParentFrame->GetAbsoluteListID()) {
    aParentFrame->GetAbsoluteContainingBlock()->
      AppendFrames(aParentFrame, aListID, aFrameList);
  } else {
    aParentFrame->AppendFrames(aListID, aFrameList);
  }
}

void
nsFrameManager::InsertFrames(nsContainerFrame* aParentFrame,
                             ChildListID       aListID,
                             nsIFrame*         aPrevFrame,
                             nsFrameList&      aFrameList)
{
  NS_PRECONDITION(!aPrevFrame || (!aPrevFrame->GetNextContinuation()
                  || (((aPrevFrame->GetNextContinuation()->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER))
                  && !(aPrevFrame->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER))),
                  "aPrevFrame must be the last continuation in its chain!");

  if (aParentFrame->IsAbsoluteContainer() &&
      aListID == aParentFrame->GetAbsoluteListID()) {
    aParentFrame->GetAbsoluteContainingBlock()->
      InsertFrames(aParentFrame, aListID, aPrevFrame, aFrameList);
  } else {
    aParentFrame->InsertFrames(aListID, aPrevFrame, aFrameList);
  }
}

void
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
  nsContainerFrame* parentFrame = aOldFrame->GetParent();
  if (parentFrame->IsAbsoluteContainer() &&
      aListID == parentFrame->GetAbsoluteListID()) {
    parentFrame->GetAbsoluteContainingBlock()->
      RemoveFrame(parentFrame, aListID, aOldFrame);
  } else {
    parentFrame->RemoveFrame(aListID, aOldFrame);
  }

  mIsDestroyingFrames = wasDestroyingFrames;
}



void
nsFrameManager::NotifyDestroyingFrame(nsIFrame* aFrame)
{
  nsIContent* content = aFrame->GetContent();
  if (content && content->GetPrimaryFrame() == aFrame) {
    ClearAllUndisplayedContentIn(content);
    ClearAllDisplayContentsIn(content);
  }
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

  
  aState->AddState(stateKey, frameState.forget());
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

  
  nsPresState* frameState = aState->GetState(stateKey);
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
nsFrameManagerBase::UndisplayedMap::GetEntryFor(nsIContent** aParentContent)
{
  nsIContent* parentContent = *aParentContent;

  if (mLastLookup && (parentContent == (*mLastLookup)->key)) {
    return mLastLookup;
  }

  
  
  
  
  
  
  if (parentContent && nsContentUtils::IsContentInsertionPoint(parentContent)) {
    parentContent = parentContent->GetParent();
    
    *aParentContent = parentContent;
  }

  PLHashNumber hashCode = NS_PTR_TO_INT32(parentContent);
  PLHashEntry** entry = PL_HashTableRawLookup(mTable, hashCode, parentContent);
  if (*entry) {
    mLastLookup = entry;
  }
  return entry;
}

UndisplayedNode* 
nsFrameManagerBase::UndisplayedMap::GetFirstNode(nsIContent* aParentContent)
{
  PLHashEntry** entry = GetEntryFor(&aParentContent);
  if (*entry) {
    return (UndisplayedNode*)((*entry)->value);
  }
  return nullptr;
}

void
nsFrameManagerBase::UndisplayedMap::AppendNodeFor(UndisplayedNode* aNode,
                                                  nsIContent* aParentContent)
{
  PLHashEntry** entry = GetEntryFor(&aParentContent);
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
  PLHashEntry** entry = GetEntryFor(&aParentContent);
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


UndisplayedNode*
nsFrameManagerBase::UndisplayedMap::UnlinkNodesFor(nsIContent* aParentContent)
{
  PLHashEntry** entry = GetEntryFor(&aParentContent);
  NS_ASSERTION(entry, "content not in map");
  if (*entry) {
    UndisplayedNode* node = (UndisplayedNode*)((*entry)->value);
    NS_ASSERTION(node, "null node for non-null entry in UndisplayedMap");
    PL_HashTableRawRemove(mTable, entry, *entry);
    mLastLookup = nullptr; 
    return node;
  }
  return nullptr;
}

void
nsFrameManagerBase::UndisplayedMap::RemoveNodesFor(nsIContent* aParentContent)
{
  delete UnlinkNodesFor(aParentContent);
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
