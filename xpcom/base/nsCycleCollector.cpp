
















































































































































#if !defined(__MINGW32__)
#ifdef WIN32
#include <crtdbg.h>
#include <errno.h>
#endif
#endif

#include "base/process_util.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/CycleCollectedJSRuntime.h"
#include "mozilla/HoldDropJSObjects.h"

#include "mozilla/MemoryReporting.h"
#include "mozilla/LinkedList.h"

#include "nsCycleCollectionParticipant.h"
#include "nsCycleCollectionNoteRootCallback.h"
#include "nsDeque.h"
#include "nsCycleCollector.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "prenv.h"
#include "nsPrintfCString.h"
#include "nsTArray.h"
#include "nsIConsoleService.h"
#include "mozilla/Attributes.h"
#include "nsICycleCollectorListener.h"
#include "nsIMemoryReporter.h"
#include "nsIFile.h"
#include "nsDumpUtils.h"
#include "xpcpublic.h"
#include "GeckoProfiler.h"
#include "js/SliceBudget.h"
#include <stdint.h>
#include <stdio.h>

#include "mozilla/Likely.h"
#include "mozilla/PoisonIOInterposer.h"
#include "mozilla/Telemetry.h"
#include "mozilla/ThreadLocal.h"

using namespace mozilla;






#define DEFAULT_SHUTDOWN_COLLECTIONS 5


#define NORMAL_SHUTDOWN_COLLECTIONS 2


































struct nsCycleCollectorParams
{
  bool mLogAll;
  bool mLogShutdown;
  bool mAllTracesAll;
  bool mAllTracesShutdown;
  bool mLogThisThread;

  nsCycleCollectorParams() :
    mLogAll      (PR_GetEnv("MOZ_CC_LOG_ALL") != nullptr),
    mLogShutdown (PR_GetEnv("MOZ_CC_LOG_SHUTDOWN") != nullptr),
    mAllTracesAll(false),
    mAllTracesShutdown(false)
  {
    const char* logThreadEnv = PR_GetEnv("MOZ_CC_LOG_THREAD");
    bool threadLogging = true;
    if (logThreadEnv && !!strcmp(logThreadEnv, "all")) {
      if (NS_IsMainThread()) {
        threadLogging = !strcmp(logThreadEnv, "main");
      } else {
        threadLogging = !strcmp(logThreadEnv, "worker");
      }
    }

    const char* logProcessEnv = PR_GetEnv("MOZ_CC_LOG_PROCESS");
    bool processLogging = true;
    if (logProcessEnv && !!strcmp(logProcessEnv, "all")) {
      switch (XRE_GetProcessType()) {
        case GeckoProcessType_Default:
          processLogging = !strcmp(logProcessEnv, "main");
          break;
        case GeckoProcessType_Plugin:
          processLogging = !strcmp(logProcessEnv, "plugins");
          break;
        case GeckoProcessType_Content:
          processLogging = !strcmp(logProcessEnv, "content");
          break;
        default:
          processLogging = false;
          break;
      }
    }
    mLogThisThread = threadLogging && processLogging;

    const char* allTracesEnv = PR_GetEnv("MOZ_CC_ALL_TRACES");
    if (allTracesEnv) {
      if (!strcmp(allTracesEnv, "all")) {
        mAllTracesAll = true;
      } else if (!strcmp(allTracesEnv, "shutdown")) {
        mAllTracesShutdown = true;
      }
    }
  }

  bool LogThisCC(bool aIsShutdown)
  {
    return (mLogAll || (aIsShutdown && mLogShutdown)) && mLogThisThread;
  }

  bool AllTracesThisCC(bool aIsShutdown)
  {
    return mAllTracesAll || (aIsShutdown && mAllTracesShutdown);
  }
};

#ifdef COLLECT_TIME_DEBUG
class TimeLog
{
public:
  TimeLog() : mLastCheckpoint(TimeStamp::Now()) {}

  void
  Checkpoint(const char* aEvent)
  {
    TimeStamp now = TimeStamp::Now();
    uint32_t dur = (uint32_t) ((now - mLastCheckpoint).ToMilliseconds());
    if (dur > 0) {
      printf("cc: %s took %dms\n", aEvent, dur);
    }
    mLastCheckpoint = now;
  }

private:
  TimeStamp mLastCheckpoint;
};
#else
class TimeLog
{
public:
  TimeLog() {}
  void Checkpoint(const char* aEvent) {}
};
#endif






struct PtrInfo;

class EdgePool
{
public:
  
  
  
  
  

  EdgePool()
  {
    mSentinelAndBlocks[0].block = nullptr;
    mSentinelAndBlocks[1].block = nullptr;
  }

  ~EdgePool()
  {
    MOZ_ASSERT(!mSentinelAndBlocks[0].block &&
               !mSentinelAndBlocks[1].block,
               "Didn't call Clear()?");
  }

  void Clear()
  {
    Block *b = Blocks();
    while (b) {
      Block *next = b->Next();
      delete b;
      b = next;
    }

    mSentinelAndBlocks[0].block = nullptr;
    mSentinelAndBlocks[1].block = nullptr;
  }

#ifdef DEBUG
  bool IsEmpty()
  {
    return !mSentinelAndBlocks[0].block &&
           !mSentinelAndBlocks[1].block;
  }
#endif

private:
  struct Block;
  union PtrInfoOrBlock {
    
    
    PtrInfo *ptrInfo;
    Block *block;
  };
  struct Block {
    enum { BlockSize = 16 * 1024 };

    PtrInfoOrBlock mPointers[BlockSize];
    Block() {
      mPointers[BlockSize - 2].block = nullptr; 
      mPointers[BlockSize - 1].block = nullptr; 
    }
    Block*& Next()          { return mPointers[BlockSize - 1].block; }
    PtrInfoOrBlock* Start() { return &mPointers[0]; }
    PtrInfoOrBlock* End()   { return &mPointers[BlockSize - 2]; }
  };

  
  
  PtrInfoOrBlock mSentinelAndBlocks[2];

  Block*& Blocks()       { return mSentinelAndBlocks[1].block; }
  Block*  Blocks() const { return mSentinelAndBlocks[1].block; }

public:
  class Iterator
  {
  public:
    Iterator() : mPointer(nullptr) {}
    Iterator(PtrInfoOrBlock *aPointer) : mPointer(aPointer) {}
    Iterator(const Iterator& aOther) : mPointer(aOther.mPointer) {}

    Iterator& operator++()
    {
      if (mPointer->ptrInfo == nullptr) {
        
        mPointer = (mPointer + 1)->block->mPointers;
      }
      ++mPointer;
      return *this;
    }

    PtrInfo* operator*() const
    {
      if (mPointer->ptrInfo == nullptr) {
        
        return (mPointer + 1)->block->mPointers->ptrInfo;
      }
      return mPointer->ptrInfo;
    }
    bool operator==(const Iterator& aOther) const
    { return mPointer == aOther.mPointer; }
    bool operator!=(const Iterator& aOther) const
    { return mPointer != aOther.mPointer; }

#ifdef DEBUG_CC_GRAPH
    bool Initialized() const
    {
      return mPointer != nullptr;
    }
#endif

  private:
    PtrInfoOrBlock *mPointer;
  };

  class Builder;
  friend class Builder;
  class Builder {
  public:
    Builder(EdgePool &aPool)
      : mCurrent(&aPool.mSentinelAndBlocks[0]),
        mBlockEnd(&aPool.mSentinelAndBlocks[0]),
        mNextBlockPtr(&aPool.Blocks())
    {
    }

    Iterator Mark() { return Iterator(mCurrent); }

    void Add(PtrInfo* aEdge) {
      if (mCurrent == mBlockEnd) {
        Block *b = new Block();
        *mNextBlockPtr = b;
        mCurrent = b->Start();
        mBlockEnd = b->End();
        mNextBlockPtr = &b->Next();
      }
      (mCurrent++)->ptrInfo = aEdge;
    }
  private:
    
    PtrInfoOrBlock *mCurrent, *mBlockEnd;
    Block **mNextBlockPtr;
  };

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const {
    size_t n = 0;
    Block *b = Blocks();
    while (b) {
      n += aMallocSizeOf(b);
      b = b->Next();
    }
    return n;
  }
};

#ifdef DEBUG_CC_GRAPH
#define CC_GRAPH_ASSERT(b) MOZ_ASSERT(b)
#else
#define CC_GRAPH_ASSERT(b)
#endif

#define CC_TELEMETRY(_name, _value)                                            \
    PR_BEGIN_MACRO                                                             \
    if (NS_IsMainThread()) {                                                   \
      Telemetry::Accumulate(Telemetry::CYCLE_COLLECTOR##_name, _value);        \
    } else {                                                                   \
      Telemetry::Accumulate(Telemetry::CYCLE_COLLECTOR_WORKER##_name, _value); \
    }                                                                          \
    PR_END_MACRO

enum NodeColor { black, white, grey };





struct PtrInfo
{
  void *mPointer;
  nsCycleCollectionParticipant *mParticipant;
  uint32_t mColor : 2;
  uint32_t mInternalRefs : 30;
  uint32_t mRefCount;
private:
  EdgePool::Iterator mFirstChild;

public:

  PtrInfo(void *aPointer, nsCycleCollectionParticipant *aParticipant)
    : mPointer(aPointer),
      mParticipant(aParticipant),
      mColor(grey),
      mInternalRefs(0),
      mRefCount(UINT32_MAX - 1),
      mFirstChild()
  {
    
    
    

    MOZ_ASSERT(aParticipant);
  }

  
  PtrInfo() {
    NS_NOTREACHED("should never be called");
  }

  EdgePool::Iterator FirstChild()
  {
    CC_GRAPH_ASSERT(mFirstChild.Initialized());
    return mFirstChild;
  }

  
  EdgePool::Iterator LastChild()
  {
    CC_GRAPH_ASSERT((this + 1)->mFirstChild.Initialized());
    return (this + 1)->mFirstChild;
  }

  void SetFirstChild(EdgePool::Iterator aFirstChild)
  {
    CC_GRAPH_ASSERT(aFirstChild.Initialized());
    mFirstChild = aFirstChild;
  }

  
  void SetLastChild(EdgePool::Iterator aLastChild)
  {
    CC_GRAPH_ASSERT(aLastChild.Initialized());
    (this + 1)->mFirstChild = aLastChild;
  }
};





class NodePool
{
private:
  enum { BlockSize = 8 * 1024 }; 

  struct Block {
    
    
    
    Block()  { NS_NOTREACHED("should never be called"); }
    ~Block() { NS_NOTREACHED("should never be called"); }

    Block* mNext;
    PtrInfo mEntries[BlockSize + 1]; 
  };

public:
  NodePool()
    : mBlocks(nullptr),
      mLast(nullptr)
  {
  }

  ~NodePool()
  {
    MOZ_ASSERT(!mBlocks, "Didn't call Clear()?");
  }

  void Clear()
  {
    Block *b = mBlocks;
    while (b) {
      Block *n = b->mNext;
      NS_Free(b);
      b = n;
    }

    mBlocks = nullptr;
    mLast = nullptr;
  }

#ifdef DEBUG
  bool IsEmpty()
  {
    return !mBlocks && !mLast;
  }
#endif

  class Builder;
  friend class Builder;
  class Builder {
  public:
    Builder(NodePool& aPool)
      : mNextBlock(&aPool.mBlocks),
        mNext(aPool.mLast),
        mBlockEnd(nullptr)
    {
      MOZ_ASSERT(aPool.mBlocks == nullptr && aPool.mLast == nullptr,
                 "pool not empty");
    }
    PtrInfo *Add(void *aPointer, nsCycleCollectionParticipant *aParticipant)
    {
      if (mNext == mBlockEnd) {
        Block *block = static_cast<Block*>(NS_Alloc(sizeof(Block)));
        *mNextBlock = block;
        mNext = block->mEntries;
        mBlockEnd = block->mEntries + BlockSize;
        block->mNext = nullptr;
        mNextBlock = &block->mNext;
      }
      return new (mNext++) PtrInfo(aPointer, aParticipant);
    }
  private:
    Block **mNextBlock;
    PtrInfo *&mNext;
    PtrInfo *mBlockEnd;
  };

  class Enumerator;
  friend class Enumerator;
  class Enumerator {
  public:
    Enumerator(NodePool& aPool)
      : mFirstBlock(aPool.mBlocks),
        mCurBlock(nullptr),
        mNext(nullptr),
        mBlockEnd(nullptr),
        mLast(aPool.mLast)
    {
    }

    bool IsDone() const
    {
      return mNext == mLast;
    }

    bool AtBlockEnd() const
    {
      return mNext == mBlockEnd;
    }

    PtrInfo* GetNext()
    {
      MOZ_ASSERT(!IsDone(), "calling GetNext when done");
      if (mNext == mBlockEnd) {
        Block *nextBlock = mCurBlock ? mCurBlock->mNext : mFirstBlock;
        mNext = nextBlock->mEntries;
        mBlockEnd = mNext + BlockSize;
        mCurBlock = nextBlock;
      }
      return mNext++;
    }
  private:
    
    
    Block *&mFirstBlock;
    Block *mCurBlock;
    
    
    PtrInfo *mNext, *mBlockEnd, *&mLast;
  };

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const {
    
    
    size_t n = 0;
    Block *b = mBlocks;
    while (b) {
      n += aMallocSizeOf(b);
      b = b->mNext;
    }
    return n;
  }

private:
  Block *mBlocks;
  PtrInfo *mLast;
};




struct PtrToNodeEntry : public PLDHashEntryHdr
{
  
  PtrInfo *mNode;
};

static bool
PtrToNodeMatchEntry(PLDHashTable *table,
                    const PLDHashEntryHdr *entry,
                    const void *key)
{
  const PtrToNodeEntry *n = static_cast<const PtrToNodeEntry*>(entry);
  return n->mNode->mPointer == key;
}

static PLDHashTableOps PtrNodeOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashVoidPtrKeyStub,
  PtrToNodeMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  nullptr
};


struct WeakMapping
{
  
  PtrInfo *mMap;
  PtrInfo *mKey;
  PtrInfo *mKeyDelegate;
  PtrInfo *mVal;
};

class GCGraphBuilder;

struct GCGraph
{
  NodePool mNodes;
  EdgePool mEdges;
  nsTArray<WeakMapping> mWeakMaps;
  uint32_t mRootCount;

private:
  PLDHashTable mPtrToNodeMap;

public:
  GCGraph() : mRootCount(0)
  {
    mPtrToNodeMap.ops = nullptr;
  }

  ~GCGraph()
  {
    if (mPtrToNodeMap.ops) {
      PL_DHashTableFinish(&mPtrToNodeMap);
    }
  }

  void Init()
  {
    MOZ_ASSERT(IsEmpty(), "Failed to call GCGraph::Clear");
    PL_DHashTableInit(&mPtrToNodeMap, &PtrNodeOps, nullptr,
                      sizeof(PtrToNodeEntry), 32768);
  }

  void Clear()
  {
    mNodes.Clear();
    mEdges.Clear();
    mWeakMaps.Clear();
    mRootCount = 0;
    PL_DHashTableFinish(&mPtrToNodeMap);
    mPtrToNodeMap.ops = nullptr;
  }

#ifdef DEBUG
  bool IsEmpty()
  {
    return mNodes.IsEmpty() && mEdges.IsEmpty() &&
           mWeakMaps.IsEmpty() && mRootCount == 0 &&
           !mPtrToNodeMap.ops;
  }
#endif

  PtrInfo* FindNode(void *aPtr);
  PtrToNodeEntry* AddNodeToMap(void *aPtr);
  void RemoveNodeFromMap(void *aPtr);

  uint32_t MapCount() const
  {
    return mPtrToNodeMap.entryCount;
  }

  void SizeOfExcludingThis(MallocSizeOf aMallocSizeOf,
                           size_t *aNodesSize, size_t *aEdgesSize,
                           size_t *aWeakMapsSize) const {
    *aNodesSize = mNodes.SizeOfExcludingThis(aMallocSizeOf);
    *aEdgesSize = mEdges.SizeOfExcludingThis(aMallocSizeOf);

    
    
    *aWeakMapsSize = mWeakMaps.SizeOfExcludingThis(aMallocSizeOf);
  }
};

PtrInfo*
GCGraph::FindNode(void *aPtr)
{
  PtrToNodeEntry *e = static_cast<PtrToNodeEntry*>(PL_DHashTableOperate(&mPtrToNodeMap, aPtr, PL_DHASH_LOOKUP));
  if (!PL_DHASH_ENTRY_IS_BUSY(e)) {
    return nullptr;
  }
  return e->mNode;
}

PtrToNodeEntry*
GCGraph::AddNodeToMap(void *aPtr)
{
  PtrToNodeEntry *e = static_cast<PtrToNodeEntry*>(PL_DHashTableOperate(&mPtrToNodeMap, aPtr, PL_DHASH_ADD));
  if (!e) {
    
    return nullptr;
  }
  return e;
}

void
GCGraph::RemoveNodeFromMap(void *aPtr)
{
  PL_DHashTableOperate(&mPtrToNodeMap, aPtr, PL_DHASH_REMOVE);
}


static nsISupports *
CanonicalizeXPCOMParticipant(nsISupports *in)
{
  nsISupports* out;
  in->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                     reinterpret_cast<void**>(&out));
  return out;
}

static inline void
ToParticipant(nsISupports *s, nsXPCOMCycleCollectionParticipant **cp);

static void
CanonicalizeParticipant(void **parti, nsCycleCollectionParticipant **cp)
{
  
  

  if (!*cp) {
    nsISupports *nsparti = static_cast<nsISupports*>(*parti);
    nsparti = CanonicalizeXPCOMParticipant(nsparti);
    NS_ASSERTION(nsparti,
                 "Don't add objects that don't participate in collection!");
    nsXPCOMCycleCollectionParticipant *xcp;
    ToParticipant(nsparti, &xcp);
    *parti = nsparti;
    *cp = xcp;
  }
}

struct nsPurpleBufferEntry {
  union {
    void *mObject;                        
    nsPurpleBufferEntry *mNextInFreeList; 
  };

  nsCycleCollectingAutoRefCnt *mRefCnt;

  nsCycleCollectionParticipant *mParticipant; 
};

class nsCycleCollector;

struct nsPurpleBuffer
{
private:
  struct Block {
    Block *mNext;
    
    
    
    
    
    nsPurpleBufferEntry mEntries[1365];

    Block() : mNext(nullptr) {
      
      static_assert(
        sizeof(Block) == 16384 ||       
        sizeof(Block) == 32768,         
        "ill-sized nsPurpleBuffer::Block"
      );
    }

    template <class PurpleVisitor>
    void VisitEntries(nsPurpleBuffer &aBuffer, PurpleVisitor &aVisitor)
    {
      nsPurpleBufferEntry *eEnd = ArrayEnd(mEntries);
      for (nsPurpleBufferEntry *e = mEntries; e != eEnd; ++e) {
        if (!(uintptr_t(e->mObject) & uintptr_t(1))) {
          aVisitor.Visit(aBuffer, e);
        }
      }
    }
  };
  
  

  uint32_t mCount;
  Block mFirstBlock;
  nsPurpleBufferEntry *mFreeList;

public:
  nsPurpleBuffer()
  {
    InitBlocks();
  }

  ~nsPurpleBuffer()
  {
    FreeBlocks();
  }

  template <class PurpleVisitor>
  void VisitEntries(PurpleVisitor &aVisitor)
  {
    for (Block *b = &mFirstBlock; b; b = b->mNext) {
      b->VisitEntries(*this, aVisitor);
    }
  }

  void InitBlocks()
  {
    mCount = 0;
    mFreeList = nullptr;
    StartBlock(&mFirstBlock);
  }

  void StartBlock(Block *aBlock)
  {
    NS_ABORT_IF_FALSE(!mFreeList, "should not have free list");

    
    nsPurpleBufferEntry *entries = aBlock->mEntries;
    mFreeList = entries;
    for (uint32_t i = 1; i < ArrayLength(aBlock->mEntries); ++i) {
      entries[i - 1].mNextInFreeList =
        (nsPurpleBufferEntry*)(uintptr_t(entries + i) | 1);
    }
    entries[ArrayLength(aBlock->mEntries) - 1].mNextInFreeList =
      (nsPurpleBufferEntry*)1;
  }

  void FreeBlocks()
  {
    if (mCount > 0)
      UnmarkRemainingPurple(&mFirstBlock);
    Block *b = mFirstBlock.mNext;
    while (b) {
      if (mCount > 0)
        UnmarkRemainingPurple(b);
      Block *next = b->mNext;
      delete b;
      b = next;
    }
    mFirstBlock.mNext = nullptr;
  }

  struct UnmarkRemainingPurpleVisitor
  {
    void
    Visit(nsPurpleBuffer &aBuffer, nsPurpleBufferEntry *aEntry)
    {
      if (aEntry->mRefCnt) {
        aEntry->mRefCnt->RemoveFromPurpleBuffer();
        aEntry->mRefCnt = nullptr;
      }
      aEntry->mObject = nullptr;
      --aBuffer.mCount;
    }
  };

  void UnmarkRemainingPurple(Block *b)
  {
    UnmarkRemainingPurpleVisitor visitor;
    b->VisitEntries(*this, visitor);
  }

  void SelectPointers(GCGraphBuilder &builder);

  
  
  
  
  
  
  
  void RemoveSkippable(nsCycleCollector* aCollector,
                       bool removeChildlessNodes,
                       bool aAsyncSnowWhiteFreeing,
                       CC_ForgetSkippableCallback aCb);

  MOZ_ALWAYS_INLINE nsPurpleBufferEntry* NewEntry()
  {
    if (MOZ_UNLIKELY(!mFreeList)) {
      Block *b = new Block;
      StartBlock(b);

      
      b->mNext = mFirstBlock.mNext;
      mFirstBlock.mNext = b;
    }

    nsPurpleBufferEntry *e = mFreeList;
    mFreeList = (nsPurpleBufferEntry*)
      (uintptr_t(mFreeList->mNextInFreeList) & ~uintptr_t(1));
    return e;
  }

  MOZ_ALWAYS_INLINE void Put(void *p, nsCycleCollectionParticipant *cp,
                             nsCycleCollectingAutoRefCnt *aRefCnt)
  {
    nsPurpleBufferEntry *e = NewEntry();

    ++mCount;

    e->mObject = p;
    e->mRefCnt = aRefCnt;
    e->mParticipant = cp;
  }

  void Remove(nsPurpleBufferEntry *e)
  {
    MOZ_ASSERT(mCount != 0, "must have entries");

    if (e->mRefCnt) {
      e->mRefCnt->RemoveFromPurpleBuffer();
      e->mRefCnt = nullptr;
    }
    e->mNextInFreeList =
      (nsPurpleBufferEntry*)(uintptr_t(mFreeList) | uintptr_t(1));
    mFreeList = e;

    --mCount;
  }

  uint32_t Count() const
  {
    return mCount;
  }

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
  {
    size_t n = 0;

    
    const Block *block = mFirstBlock.mNext;
    while (block) {
      n += aMallocSizeOf(block);
      block = block->mNext;
    }

    
    
    
    
    

    return n;
  }
};

static bool
AddPurpleRoot(GCGraphBuilder &aBuilder, void *aRoot, nsCycleCollectionParticipant *aParti);

struct SelectPointersVisitor
{
  SelectPointersVisitor(GCGraphBuilder &aBuilder)
    : mBuilder(aBuilder)
  {}

  void
  Visit(nsPurpleBuffer &aBuffer, nsPurpleBufferEntry *aEntry)
  {
    MOZ_ASSERT(aEntry->mObject, "Null object in purple buffer");
    MOZ_ASSERT(aEntry->mRefCnt->get() != 0,
               "SelectPointersVisitor: snow-white object in the purple buffer");
    if (!aEntry->mRefCnt->IsPurple() ||
        AddPurpleRoot(mBuilder, aEntry->mObject, aEntry->mParticipant)) {
      aBuffer.Remove(aEntry);
    }
  }

private:
  GCGraphBuilder &mBuilder;
};

void
nsPurpleBuffer::SelectPointers(GCGraphBuilder &aBuilder)
{
  SelectPointersVisitor visitor(aBuilder);
  VisitEntries(visitor);

  NS_ASSERTION(mCount == 0, "AddPurpleRoot failed");
  if (mCount == 0) {
    FreeBlocks();
    InitBlocks();
  }
}

enum ccPhase {
  IdlePhase,
  GraphBuildingPhase,
  ScanAndCollectWhitePhase,
  CleanupPhase
};

enum ccType {
  SliceCC,     
  ManualCC,    
  ShutdownCC   
};

#ifdef MOZ_NUWA_PROCESS
#include "ipc/Nuwa.h"
#endif





typedef js::SliceBudget SliceBudget;

class JSPurpleBuffer;

class nsCycleCollector : public nsIMemoryReporter
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMEMORYREPORTER

  bool mActivelyCollecting;
  bool mFreeingSnowWhite;
  
  bool mScanInProgress;
  CycleCollectorResults mResults;
  TimeStamp mCollectionStart;

  CycleCollectedJSRuntime *mJSRuntime;

  ccPhase mIncrementalPhase;
  GCGraph mGraph;
  nsAutoPtr<GCGraphBuilder> mBuilder;
  nsAutoPtr<NodePool::Enumerator> mCurrNode;
  nsCOMPtr<nsICycleCollectorListener> mListener;

  nsIThread* mThread;

  nsCycleCollectorParams mParams;

  uint32_t mWhiteNodeCount;

  CC_BeforeUnlinkCallback mBeforeUnlinkCB;
  CC_ForgetSkippableCallback mForgetSkippableCB;

  nsPurpleBuffer mPurpleBuf;

  uint32_t mUnmergedNeeded;
  uint32_t mMergedInARow;

  JSPurpleBuffer* mJSPurpleBuffer;

public:
  nsCycleCollector();
  virtual ~nsCycleCollector();

  void RegisterJSRuntime(CycleCollectedJSRuntime *aJSRuntime);
  void ForgetJSRuntime();

  void SetBeforeUnlinkCallback(CC_BeforeUnlinkCallback aBeforeUnlinkCB)
  {
    CheckThreadSafety();
    mBeforeUnlinkCB = aBeforeUnlinkCB;
  }

  void SetForgetSkippableCallback(CC_ForgetSkippableCallback aForgetSkippableCB)
  {
    CheckThreadSafety();
    mForgetSkippableCB = aForgetSkippableCB;
  }

  void Suspect(void *n, nsCycleCollectionParticipant *cp,
               nsCycleCollectingAutoRefCnt *aRefCnt);
  uint32_t SuspectedCount();
  void ForgetSkippable(bool aRemoveChildlessNodes, bool aAsyncSnowWhiteFreeing);
  bool FreeSnowWhite(bool aUntilNoSWInPurpleBuffer);

  
  void RemoveObjectFromGraph(void *aPtr);

  void PrepareForGarbageCollection();
  void FinishAnyCurrentCollection();

  bool Collect(ccType aCCType,
               SliceBudget &aBudget,
               nsICycleCollectorListener *aManualListener);
  void Shutdown();

  void SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                           size_t *aObjectSize,
                           size_t *aGraphNodesSize,
                           size_t *aGraphEdgesSize,
                           size_t *aWeakMapsSize,
                           size_t *aPurpleBufferSize) const;

  JSPurpleBuffer* GetJSPurpleBuffer();
private:
  void CheckThreadSafety();
  void ShutdownCollect();

  void FixGrayBits(bool aForceGC);
  bool ShouldMergeZones(ccType aCCType);

  void BeginCollection(ccType aCCType, nsICycleCollectorListener *aManualListener);
  void MarkRoots(SliceBudget &aBudget);
  void ScanRoots(bool aFullySynchGraphBuild);
  void ScanIncrementalRoots();
  void ScanWeakMaps();

  
  bool CollectWhite();

  void CleanupAfterCollection();
};

NS_IMPL_ISUPPORTS(nsCycleCollector, nsIMemoryReporter)








template <class Visitor>
class GraphWalker
{
private:
  Visitor mVisitor;

  void DoWalk(nsDeque &aQueue);

  void CheckedPush(nsDeque &aQueue, PtrInfo *pi)
  {
    if (!pi) {
      MOZ_CRASH();
    }
    if (!aQueue.Push(pi, fallible_t())) {
      mVisitor.Failed();
    }
  }

public:
  void Walk(PtrInfo *s0);
  void WalkFromRoots(GCGraph &aGraph);
  
  
  GraphWalker(const Visitor aVisitor) : mVisitor(aVisitor) {}
};






struct CollectorData {
  nsRefPtr<nsCycleCollector> mCollector;
  CycleCollectedJSRuntime* mRuntime;
};

static mozilla::ThreadLocal<CollectorData*> sCollectorData;





MOZ_NEVER_INLINE static void
Fault(const char *msg, const void *ptr=nullptr)
{
  if (ptr)
    printf("Fault in cycle collector: %s (ptr: %p)\n", msg, ptr);
  else
    printf("Fault in cycle collector: %s\n", msg);

  NS_RUNTIMEABORT("cycle collector fault");
}

static void
Fault(const char *msg, PtrInfo *pi)
{
  Fault(msg, pi->mPointer);
}

static inline void
ToParticipant(nsISupports *s, nsXPCOMCycleCollectionParticipant **cp)
{
  
  
  
  
  CallQueryInterface(s, cp);
}

template <class Visitor>
MOZ_NEVER_INLINE void
GraphWalker<Visitor>::Walk(PtrInfo *s0)
{
  nsDeque queue;
  CheckedPush(queue, s0);
  DoWalk(queue);
}

template <class Visitor>
MOZ_NEVER_INLINE void
GraphWalker<Visitor>::WalkFromRoots(GCGraph& aGraph)
{
  nsDeque queue;
  NodePool::Enumerator etor(aGraph.mNodes);
  for (uint32_t i = 0; i < aGraph.mRootCount; ++i) {
    CheckedPush(queue, etor.GetNext());
  }
  DoWalk(queue);
}

template <class Visitor>
MOZ_NEVER_INLINE void
GraphWalker<Visitor>::DoWalk(nsDeque &aQueue)
{
  
  
  while (aQueue.GetSize() > 0) {
    PtrInfo *pi = static_cast<PtrInfo*>(aQueue.PopFront());

    if (pi->mParticipant && mVisitor.ShouldVisitNode(pi)) {
      mVisitor.VisitNode(pi);
      for (EdgePool::Iterator child = pi->FirstChild(),
           child_end = pi->LastChild();
           child != child_end; ++child) {
        CheckedPush(aQueue, *child);
      }
    }
  }
}

struct CCGraphDescriber : public LinkedListElement<CCGraphDescriber>
{
  CCGraphDescriber()
    : mAddress("0x"), mCnt(0), mType(eUnknown) {}

  enum Type
  {
    eRefCountedObject,
    eGCedObject,
    eGCMarkedObject,
    eEdge,
    eRoot,
    eGarbage,
    eUnknown
  };

  nsCString mAddress;
  nsCString mName;
  nsCString mCompartmentOrToAddress;
  uint32_t mCnt;
  Type mType;
};

class nsCycleCollectorLogger MOZ_FINAL : public nsICycleCollectorListener
{
public:
  nsCycleCollectorLogger() :
    mStream(nullptr), mWantAllTraces(false),
    mDisableLog(false), mWantAfterProcessing(false)
  {
  }
  ~nsCycleCollectorLogger()
  {
    ClearDescribers();
    if (mStream) {
      MozillaUnRegisterDebugFILE(mStream);
      fclose(mStream);
    }
  }
  NS_DECL_ISUPPORTS

  void SetAllTraces()
  {
    mWantAllTraces = true;
  }

  NS_IMETHOD AllTraces(nsICycleCollectorListener** aListener)
  {
    SetAllTraces();
    NS_ADDREF(*aListener = this);
    return NS_OK;
  }

  NS_IMETHOD GetWantAllTraces(bool* aAllTraces)
  {
    *aAllTraces = mWantAllTraces;
    return NS_OK;
  }

  NS_IMETHOD GetDisableLog(bool* aDisableLog)
  {
    *aDisableLog = mDisableLog;
    return NS_OK;
  }

  NS_IMETHOD SetDisableLog(bool aDisableLog)
  {
    mDisableLog = aDisableLog;
    return NS_OK;
  }

  NS_IMETHOD GetWantAfterProcessing(bool* aWantAfterProcessing)
  {
    *aWantAfterProcessing = mWantAfterProcessing;
    return NS_OK;
  }

  NS_IMETHOD SetWantAfterProcessing(bool aWantAfterProcessing)
  {
    mWantAfterProcessing = aWantAfterProcessing;
    return NS_OK;
  }

  NS_IMETHOD GetFilenameIdentifier(nsAString& aIdentifier)
  {
    aIdentifier = mFilenameIdentifier;
    return NS_OK;
  }

  NS_IMETHOD SetFilenameIdentifier(const nsAString& aIdentifier)
  {
    mFilenameIdentifier = aIdentifier;
    return NS_OK;
  }

  NS_IMETHOD GetGcLogPath(nsAString &aPath)
  {
    aPath = mGCLogPath;
    return NS_OK;
  }

  NS_IMETHOD GetCcLogPath(nsAString &aPath)
  {
    aPath = mCCLogPath;
    return NS_OK;
  }

  NS_IMETHOD Begin()
  {
    mCurrentAddress.AssignLiteral("0x");
    ClearDescribers();
    if (mDisableLog) {
      return NS_OK;
    }

    
    
    
    
    
    nsCOMPtr<nsIFile> gcLogFile = CreateTempFile("incomplete-gc-edges");
    if (NS_WARN_IF(!gcLogFile))
      return NS_ERROR_UNEXPECTED;

    
    FILE* gcLogANSIFile = nullptr;
    gcLogFile->OpenANSIFileDesc("w", &gcLogANSIFile);
    if (NS_WARN_IF(!gcLogANSIFile))
      return NS_ERROR_UNEXPECTED;
    MozillaRegisterDebugFILE(gcLogANSIFile);
    CollectorData *data = sCollectorData.get();
    if (data && data->mRuntime)
      data->mRuntime->DumpJSHeap(gcLogANSIFile);
    MozillaUnRegisterDebugFILE(gcLogANSIFile);
    fclose(gcLogANSIFile);

    
    nsCOMPtr<nsIFile> gcLogFileFinalDestination =
      CreateTempFile("gc-edges");
    if (NS_WARN_IF(!gcLogFileFinalDestination))
      return NS_ERROR_UNEXPECTED;

    nsAutoString gcLogFileFinalDestinationName;
    gcLogFileFinalDestination->GetLeafName(gcLogFileFinalDestinationName);
    if (NS_WARN_IF(gcLogFileFinalDestinationName.IsEmpty()))
      return NS_ERROR_UNEXPECTED;

    gcLogFile->MoveTo( nullptr, gcLogFileFinalDestinationName);

    
    nsCOMPtr<nsIConsoleService> cs =
      do_GetService(NS_CONSOLESERVICE_CONTRACTID);
    if (cs) {
      nsAutoString gcLogPath;
      gcLogFileFinalDestination->GetPath(gcLogPath);

      nsString msg = NS_LITERAL_STRING("Garbage Collector log dumped to ") +
                     gcLogPath;
      cs->LogStringMessage(msg.get());

      mGCLogPath = gcLogPath;
    }

    
    
    mOutFile = CreateTempFile("incomplete-cc-edges");
    if (NS_WARN_IF(!mOutFile))
      return NS_ERROR_UNEXPECTED;
    MOZ_ASSERT(!mStream);
    mOutFile->OpenANSIFileDesc("w", &mStream);
    if (NS_WARN_IF(!mStream))
      return NS_ERROR_UNEXPECTED;
    MozillaRegisterDebugFILE(mStream);

    fprintf(mStream, "# WantAllTraces=%s\n", mWantAllTraces ? "true" : "false");

    return NS_OK;
  }
  NS_IMETHOD NoteRefCountedObject(uint64_t aAddress, uint32_t refCount,
                                  const char *aObjectDescription)
  {
    if (!mDisableLog) {
      fprintf(mStream, "%p [rc=%u] %s\n", (void*)aAddress, refCount,
              aObjectDescription);
    }
    if (mWantAfterProcessing) {
      CCGraphDescriber* d =  new CCGraphDescriber();
      mDescribers.insertBack(d);
      mCurrentAddress.AssignLiteral("0x");
      mCurrentAddress.AppendInt(aAddress, 16);
      d->mType = CCGraphDescriber::eRefCountedObject;
      d->mAddress = mCurrentAddress;
      d->mCnt = refCount;
      d->mName.Append(aObjectDescription);
    }
    return NS_OK;
  }
  NS_IMETHOD NoteGCedObject(uint64_t aAddress, bool aMarked,
                            const char *aObjectDescription,
                            uint64_t aCompartmentAddress)
  {
    if (!mDisableLog) {
      fprintf(mStream, "%p [gc%s] %s\n", (void*)aAddress,
              aMarked ? ".marked" : "", aObjectDescription);
    }
    if (mWantAfterProcessing) {
      CCGraphDescriber* d =  new CCGraphDescriber();
      mDescribers.insertBack(d);
      mCurrentAddress.AssignLiteral("0x");
      mCurrentAddress.AppendInt(aAddress, 16);
      d->mType = aMarked ? CCGraphDescriber::eGCMarkedObject :
                           CCGraphDescriber::eGCedObject;
      d->mAddress = mCurrentAddress;
      d->mName.Append(aObjectDescription);
      if (aCompartmentAddress) {
        d->mCompartmentOrToAddress.AssignLiteral("0x");
        d->mCompartmentOrToAddress.AppendInt(aCompartmentAddress, 16);
      } else {
        d->mCompartmentOrToAddress.SetIsVoid(true);
      }
    }
    return NS_OK;
  }
  NS_IMETHOD NoteEdge(uint64_t aToAddress, const char *aEdgeName)
  {
    if (!mDisableLog) {
      fprintf(mStream, "> %p %s\n", (void*)aToAddress, aEdgeName);
    }
    if (mWantAfterProcessing) {
      CCGraphDescriber* d =  new CCGraphDescriber();
      mDescribers.insertBack(d);
      d->mType = CCGraphDescriber::eEdge;
      d->mAddress = mCurrentAddress;
      d->mCompartmentOrToAddress.AssignLiteral("0x");
      d->mCompartmentOrToAddress.AppendInt(aToAddress, 16);
      d->mName.Append(aEdgeName);
    }
    return NS_OK;
  }
  NS_IMETHOD NoteWeakMapEntry(uint64_t aMap, uint64_t aKey,
                              uint64_t aKeyDelegate, uint64_t aValue)
  {
    if (!mDisableLog) {
      fprintf(mStream, "WeakMapEntry map=%p key=%p keyDelegate=%p value=%p\n",
              (void*)aMap, (void*)aKey, (void*)aKeyDelegate, (void*)aValue);
    }
    
    return NS_OK;
  }
  NS_IMETHOD NoteIncrementalRoot(uint64_t aAddress)
  {
    if (!mDisableLog) {
      fprintf(mStream, "IncrementalRoot %p\n", (void*)aAddress);
    }
    
    return NS_OK;
  }
  NS_IMETHOD BeginResults()
  {
    if (!mDisableLog) {
      fputs("==========\n", mStream);
    }
    return NS_OK;
  }
  NS_IMETHOD DescribeRoot(uint64_t aAddress, uint32_t aKnownEdges)
  {
    if (!mDisableLog) {
      fprintf(mStream, "%p [known=%u]\n", (void*)aAddress, aKnownEdges);
    }
    if (mWantAfterProcessing) {
      CCGraphDescriber* d =  new CCGraphDescriber();
      mDescribers.insertBack(d);
      d->mType = CCGraphDescriber::eRoot;
      d->mAddress.AppendInt(aAddress, 16);
      d->mCnt = aKnownEdges;
    }
    return NS_OK;
  }
  NS_IMETHOD DescribeGarbage(uint64_t aAddress)
  {
    if (!mDisableLog) {
      fprintf(mStream, "%p [garbage]\n", (void*)aAddress);
    }
    if (mWantAfterProcessing) {
      CCGraphDescriber* d =  new CCGraphDescriber();
      mDescribers.insertBack(d);
      d->mType = CCGraphDescriber::eGarbage;
      d->mAddress.AppendInt(aAddress, 16);
    }
    return NS_OK;
  }
  NS_IMETHOD End()
  {
    if (!mDisableLog) {
      MOZ_ASSERT(mStream);
      MOZ_ASSERT(mOutFile);

      MozillaUnRegisterDebugFILE(mStream);
      fclose(mStream);
      mStream = nullptr;

      
      nsCOMPtr<nsIFile> logFileFinalDestination =
        CreateTempFile("cc-edges");
      if (NS_WARN_IF(!logFileFinalDestination))
        return NS_ERROR_UNEXPECTED;

      nsAutoString logFileFinalDestinationName;
      logFileFinalDestination->GetLeafName(logFileFinalDestinationName);
      if (NS_WARN_IF(logFileFinalDestinationName.IsEmpty()))
        return NS_ERROR_UNEXPECTED;

      mOutFile->MoveTo( nullptr,
                       logFileFinalDestinationName);
      mOutFile = nullptr;

      
      nsCOMPtr<nsIConsoleService> cs =
        do_GetService(NS_CONSOLESERVICE_CONTRACTID);
      if (cs) {
        nsAutoString ccLogPath;
        logFileFinalDestination->GetPath(ccLogPath);

        nsString msg = NS_LITERAL_STRING("Cycle Collector log dumped to ") +
                       ccLogPath;
        cs->LogStringMessage(msg.get());

        mCCLogPath = ccLogPath;
      }
    }
    return NS_OK;
  }
  NS_IMETHOD ProcessNext(nsICycleCollectorHandler* aHandler,
                         bool* aCanContinue)
  {
    if (NS_WARN_IF(!aHandler) || NS_WARN_IF(!mWantAfterProcessing))
      return NS_ERROR_UNEXPECTED;
    CCGraphDescriber* d = mDescribers.popFirst();
    if (d) {
      switch (d->mType) {
        case CCGraphDescriber::eRefCountedObject:
          aHandler->NoteRefCountedObject(d->mAddress,
                                         d->mCnt,
                                         d->mName);
          break;
        case CCGraphDescriber::eGCedObject:
        case CCGraphDescriber::eGCMarkedObject:
          aHandler->NoteGCedObject(d->mAddress,
                                   d->mType ==
                                     CCGraphDescriber::eGCMarkedObject,
                                   d->mName,
                                   d->mCompartmentOrToAddress);
          break;
        case CCGraphDescriber::eEdge:
          aHandler->NoteEdge(d->mAddress,
                             d->mCompartmentOrToAddress,
                             d->mName);
          break;
        case CCGraphDescriber::eRoot:
          aHandler->DescribeRoot(d->mAddress,
                                 d->mCnt);
          break;
        case CCGraphDescriber::eGarbage:
          aHandler->DescribeGarbage(d->mAddress);
          break;
        case CCGraphDescriber::eUnknown:
          NS_NOTREACHED("CCGraphDescriber::eUnknown");
          break;
      }
      delete d;
    }
    if (!(*aCanContinue = !mDescribers.isEmpty())) {
      mCurrentAddress.AssignLiteral("0x");
    }
    return NS_OK;
  }
private:
  






  already_AddRefed<nsIFile>
  CreateTempFile(const char* aPrefix)
  {
    nsPrintfCString filename("%s.%d%s%s.log",
                             aPrefix,
                             base::GetCurrentProcId(),
                             mFilenameIdentifier.IsEmpty() ? "" : ".",
                             NS_ConvertUTF16toUTF8(mFilenameIdentifier).get());

    
    
    
    
    nsIFile* logFile = nullptr;
    if (char* env = PR_GetEnv("MOZ_CC_LOG_DIRECTORY")) {
      NS_NewNativeLocalFile(nsCString(env),  true,
                            &logFile);
    }

    
    
    
    nsresult rv = nsDumpUtils::OpenTempFile(
      filename,
      &logFile,
      NS_LITERAL_CSTRING("memory-reports"));
    if (NS_FAILED(rv)) {
      NS_IF_RELEASE(logFile);
      return nullptr;
    }

    return dont_AddRef(logFile);
  }

  void ClearDescribers()
  {
    CCGraphDescriber* d;
    while((d = mDescribers.popFirst())) {
      delete d;
    }
  }

  FILE *mStream;
  nsCOMPtr<nsIFile> mOutFile;
  bool mWantAllTraces;
  bool mDisableLog;
  bool mWantAfterProcessing;
  nsString mFilenameIdentifier;
  nsString mGCLogPath;
  nsString mCCLogPath;
  nsCString mCurrentAddress;
  mozilla::LinkedList<CCGraphDescriber> mDescribers;
};

NS_IMPL_ISUPPORTS(nsCycleCollectorLogger, nsICycleCollectorListener)

nsresult
nsCycleCollectorLoggerConstructor(nsISupports* aOuter,
                                  const nsIID& aIID,
                                  void* *aInstancePtr)
{
  if (NS_WARN_IF(aOuter))
    return NS_ERROR_NO_AGGREGATION;

  nsISupports *logger = new nsCycleCollectorLogger();

  return logger->QueryInterface(aIID, aInstancePtr);
}





class GCGraphBuilder : public nsCycleCollectionTraversalCallback,
                       public nsCycleCollectionNoteRootCallback
{
private:
  GCGraph &mGraph;
  CycleCollectorResults &mResults;
  NodePool::Builder mNodeBuilder;
  EdgePool::Builder mEdgeBuilder;
  PtrInfo *mCurrPi;
  nsCycleCollectionParticipant *mJSParticipant;
  nsCycleCollectionParticipant *mJSZoneParticipant;
  nsCString mNextEdgeName;
  nsICycleCollectorListener *mListener;
  bool mMergeZones;
  bool mRanOutOfMemory;

public:
  GCGraphBuilder(GCGraph &aGraph,
                 CycleCollectorResults &aResults,
                 CycleCollectedJSRuntime *aJSRuntime,
                 nsICycleCollectorListener *aListener,
                 bool aMergeZones);
  virtual ~GCGraphBuilder();

  bool WantAllTraces() const
  {
    return nsCycleCollectionNoteRootCallback::WantAllTraces();
  }

  PtrInfo* AddNode(void *aPtr, nsCycleCollectionParticipant *aParticipant);
  PtrInfo* AddWeakMapNode(void* node);
  void Traverse(PtrInfo* aPtrInfo);
  void SetLastChild();

  bool RanOutOfMemory() const { return mRanOutOfMemory; }

private:
  void DescribeNode(uint32_t refCount, const char *objName)
  {
    mCurrPi->mRefCount = refCount;
  }

public:
  
  NS_IMETHOD_(void) NoteXPCOMRoot(nsISupports *root);
  NS_IMETHOD_(void) NoteJSRoot(void *root);
  NS_IMETHOD_(void) NoteNativeRoot(void *root, nsCycleCollectionParticipant *participant);
  NS_IMETHOD_(void) NoteWeakMapping(void *map, void *key, void *kdelegate, void *val);

  
  NS_IMETHOD_(void) DescribeRefCountedNode(nsrefcnt refCount,
                                           const char *objName);
  NS_IMETHOD_(void) DescribeGCedNode(bool isMarked, const char *objName,
                                     uint64_t aCompartmentAddress);

  NS_IMETHOD_(void) NoteXPCOMChild(nsISupports *child);
  NS_IMETHOD_(void) NoteJSChild(void *child);
  NS_IMETHOD_(void) NoteNativeChild(void *child,
                                    nsCycleCollectionParticipant *participant);
  NS_IMETHOD_(void) NoteNextEdgeName(const char* name);

private:
  NS_IMETHOD_(void) NoteRoot(void *root,
                             nsCycleCollectionParticipant *participant)
  {
    MOZ_ASSERT(root);
    MOZ_ASSERT(participant);

    if (!participant->CanSkipInCC(root) || MOZ_UNLIKELY(WantAllTraces())) {
      AddNode(root, participant);
    }
  }

  NS_IMETHOD_(void) NoteChild(void *child, nsCycleCollectionParticipant *cp,
                              nsCString edgeName)
  {
    PtrInfo *childPi = AddNode(child, cp);
    if (!childPi)
      return;
    mEdgeBuilder.Add(childPi);
    if (mListener) {
      mListener->NoteEdge((uint64_t)child, edgeName.get());
    }
    ++childPi->mInternalRefs;
  }

  JS::Zone *MergeZone(void *gcthing) {
    if (!mMergeZones) {
      return nullptr;
    }
    JS::Zone *zone = JS::GetGCThingZone(gcthing);
    if (js::IsSystemZone(zone)) {
      return nullptr;
    }
    return zone;
  }
};

GCGraphBuilder::GCGraphBuilder(GCGraph &aGraph,
                               CycleCollectorResults &aResults,
                               CycleCollectedJSRuntime *aJSRuntime,
                               nsICycleCollectorListener *aListener,
                               bool aMergeZones)
  : mGraph(aGraph),
    mResults(aResults),
    mNodeBuilder(aGraph.mNodes),
    mEdgeBuilder(aGraph.mEdges),
    mJSParticipant(nullptr),
    mJSZoneParticipant(nullptr),
    mListener(aListener),
    mMergeZones(aMergeZones),
    mRanOutOfMemory(false)
{
  if (aJSRuntime) {
    mJSParticipant = aJSRuntime->GCThingParticipant();
    mJSZoneParticipant = aJSRuntime->ZoneParticipant();
  }

  uint32_t flags = 0;
  if (!flags && mListener) {
    flags = nsCycleCollectionTraversalCallback::WANT_DEBUG_INFO;
    bool all = false;
    mListener->GetWantAllTraces(&all);
    if (all) {
      flags |= nsCycleCollectionTraversalCallback::WANT_ALL_TRACES;
      mWantAllTraces = true; 
    }
  }

  mFlags |= flags;

  mMergeZones = mMergeZones && MOZ_LIKELY(!WantAllTraces());

  MOZ_ASSERT(nsCycleCollectionNoteRootCallback::WantAllTraces() ==
             nsCycleCollectionTraversalCallback::WantAllTraces());
}

GCGraphBuilder::~GCGraphBuilder()
{
}

PtrInfo*
GCGraphBuilder::AddNode(void *aPtr, nsCycleCollectionParticipant *aParticipant)
{
  PtrToNodeEntry *e = mGraph.AddNodeToMap(aPtr);
  if (!e) {
    mRanOutOfMemory = true;
    return nullptr;
  }

  PtrInfo *result;
  if (!e->mNode) {
    
    result = mNodeBuilder.Add(aPtr, aParticipant);
    e->mNode = result;
    NS_ASSERTION(result, "mNodeBuilder.Add returned null");
  } else {
    result = e->mNode;
    MOZ_ASSERT(result->mParticipant == aParticipant,
               "nsCycleCollectionParticipant shouldn't change!");
  }
  return result;
}

MOZ_NEVER_INLINE void
GCGraphBuilder::Traverse(PtrInfo* aPtrInfo)
{
  mCurrPi = aPtrInfo;

  mCurrPi->SetFirstChild(mEdgeBuilder.Mark());

  if (!aPtrInfo->mParticipant) {
    return;
  }

  nsresult rv = aPtrInfo->mParticipant->Traverse(aPtrInfo->mPointer, *this);
  if (NS_FAILED(rv)) {
    Fault("script pointer traversal failed", aPtrInfo);
  }
}

void
GCGraphBuilder::SetLastChild()
{
  mCurrPi->SetLastChild(mEdgeBuilder.Mark());
}

NS_IMETHODIMP_(void)
GCGraphBuilder::NoteXPCOMRoot(nsISupports *root)
{
  root = CanonicalizeXPCOMParticipant(root);
  NS_ASSERTION(root,
               "Don't add objects that don't participate in collection!");

  nsXPCOMCycleCollectionParticipant *cp;
  ToParticipant(root, &cp);

  NoteRoot(root, cp);
}

NS_IMETHODIMP_(void)
GCGraphBuilder::NoteJSRoot(void *root)
{
  if (JS::Zone *zone = MergeZone(root)) {
    NoteRoot(zone, mJSZoneParticipant);
  } else {
    NoteRoot(root, mJSParticipant);
  }
}

NS_IMETHODIMP_(void)
GCGraphBuilder::NoteNativeRoot(void *root, nsCycleCollectionParticipant *participant)
{
  NoteRoot(root, participant);
}

NS_IMETHODIMP_(void)
GCGraphBuilder::DescribeRefCountedNode(nsrefcnt refCount, const char *objName)
{
  if (refCount == 0)
    Fault("zero refcount", mCurrPi);
  if (refCount == UINT32_MAX)
    Fault("overflowing refcount", mCurrPi);
  mResults.mVisitedRefCounted++;

  if (mListener) {
    mListener->NoteRefCountedObject((uint64_t)mCurrPi->mPointer, refCount,
                                    objName);
  }

  DescribeNode(refCount, objName);
}

NS_IMETHODIMP_(void)
GCGraphBuilder::DescribeGCedNode(bool isMarked, const char *objName,
                                 uint64_t aCompartmentAddress)
{
  uint32_t refCount = isMarked ? UINT32_MAX : 0;
  mResults.mVisitedGCed++;

  if (mListener) {
    mListener->NoteGCedObject((uint64_t)mCurrPi->mPointer, isMarked,
                              objName, aCompartmentAddress);
  }

  DescribeNode(refCount, objName);
}

NS_IMETHODIMP_(void)
GCGraphBuilder::NoteXPCOMChild(nsISupports *child)
{
  nsCString edgeName;
  if (WantDebugInfo()) {
    edgeName.Assign(mNextEdgeName);
    mNextEdgeName.Truncate();
  }
  if (!child || !(child = CanonicalizeXPCOMParticipant(child)))
    return;

  nsXPCOMCycleCollectionParticipant *cp;
  ToParticipant(child, &cp);
  if (cp && (!cp->CanSkipThis(child) || WantAllTraces())) {
    NoteChild(child, cp, edgeName);
  }
}

NS_IMETHODIMP_(void)
GCGraphBuilder::NoteNativeChild(void *child,
                                nsCycleCollectionParticipant *participant)
{
  nsCString edgeName;
  if (WantDebugInfo()) {
    edgeName.Assign(mNextEdgeName);
    mNextEdgeName.Truncate();
  }
  if (!child)
    return;

  MOZ_ASSERT(participant, "Need a nsCycleCollectionParticipant!");
  NoteChild(child, participant, edgeName);
}

NS_IMETHODIMP_(void)
GCGraphBuilder::NoteJSChild(void *child)
{
  if (!child) {
    return;
  }

  nsCString edgeName;
  if (MOZ_UNLIKELY(WantDebugInfo())) {
    edgeName.Assign(mNextEdgeName);
    mNextEdgeName.Truncate();
  }

  if (xpc_GCThingIsGrayCCThing(child) || MOZ_UNLIKELY(WantAllTraces())) {
    if (JS::Zone *zone = MergeZone(child)) {
      NoteChild(zone, mJSZoneParticipant, edgeName);
    } else {
      NoteChild(child, mJSParticipant, edgeName);
    }
  }
}

NS_IMETHODIMP_(void)
GCGraphBuilder::NoteNextEdgeName(const char* name)
{
  if (WantDebugInfo()) {
    mNextEdgeName = name;
  }
}

PtrInfo*
GCGraphBuilder::AddWeakMapNode(void *node)
{
  MOZ_ASSERT(node, "Weak map node should be non-null.");

  if (!xpc_GCThingIsGrayCCThing(node) && !WantAllTraces())
    return nullptr;

  if (JS::Zone *zone = MergeZone(node)) {
    return AddNode(zone, mJSZoneParticipant);
  } else {
    return AddNode(node, mJSParticipant);
  }
}

NS_IMETHODIMP_(void)
GCGraphBuilder::NoteWeakMapping(void *map, void *key, void *kdelegate, void *val)
{
  
  
  WeakMapping *mapping = mGraph.mWeakMaps.AppendElement();
  mapping->mMap = map ? AddWeakMapNode(map) : nullptr;
  mapping->mKey = key ? AddWeakMapNode(key) : nullptr;
  mapping->mKeyDelegate = kdelegate ? AddWeakMapNode(kdelegate) : mapping->mKey;
  mapping->mVal = val ? AddWeakMapNode(val) : nullptr;

  if (mListener) {
    mListener->NoteWeakMapEntry((uint64_t)map, (uint64_t)key,
                                (uint64_t)kdelegate, (uint64_t)val);
  }
}

static bool
AddPurpleRoot(GCGraphBuilder &aBuilder, void *aRoot, nsCycleCollectionParticipant *aParti)
{
  CanonicalizeParticipant(&aRoot, &aParti);

  if (aBuilder.WantAllTraces() || !aParti->CanSkipInCC(aRoot)) {
    PtrInfo *pinfo = aBuilder.AddNode(aRoot, aParti);
    if (!pinfo) {
      return false;
    }
  }

  return true;
}



class ChildFinder : public nsCycleCollectionTraversalCallback
{
public:
  ChildFinder() : mMayHaveChild(false) {}

  
  
  NS_IMETHOD_(void) NoteXPCOMChild(nsISupports *child);
  NS_IMETHOD_(void) NoteNativeChild(void *child,
                                    nsCycleCollectionParticipant *helper);
  NS_IMETHOD_(void) NoteJSChild(void *child);

  NS_IMETHOD_(void) DescribeRefCountedNode(nsrefcnt refcount,
                                           const char *objname) {}
  NS_IMETHOD_(void) DescribeGCedNode(bool ismarked,
                                     const char *objname,
                                     uint64_t aCompartmentAddress) {}
  NS_IMETHOD_(void) NoteNextEdgeName(const char* name) {}
  bool MayHaveChild() {
    return mMayHaveChild;
  }
private:
  bool mMayHaveChild;
};

NS_IMETHODIMP_(void)
ChildFinder::NoteXPCOMChild(nsISupports *child)
{
  if (!child || !(child = CanonicalizeXPCOMParticipant(child)))
    return;
  nsXPCOMCycleCollectionParticipant *cp;
  ToParticipant(child, &cp);
  if (cp && !cp->CanSkip(child, true))
    mMayHaveChild = true;
}

NS_IMETHODIMP_(void)
ChildFinder::NoteNativeChild(void *child,
                             nsCycleCollectionParticipant *helper)
{
  if (child)
    mMayHaveChild = true;
}

NS_IMETHODIMP_(void)
ChildFinder::NoteJSChild(void *child)
{
  if (child && xpc_GCThingIsGrayCCThing(child)) {
    mMayHaveChild = true;
  }
}

static bool
MayHaveChild(void *o, nsCycleCollectionParticipant* cp)
{
  ChildFinder cf;
  cp->Traverse(o, cf);
  return cf.MayHaveChild();
}

template<class T>
class SegmentedArrayElement : public LinkedListElement<SegmentedArrayElement<T>>
                            , public AutoFallibleTArray<T, 60>
{
};

template<class T>
class SegmentedArray
{
public:
  ~SegmentedArray()
  {
    MOZ_ASSERT(IsEmpty());
  }

  void AppendElement(T& aElement)
  {
    SegmentedArrayElement<T>* last = mSegments.getLast();
    if (!last || last->Length() == last->Capacity()) {
      last = new SegmentedArrayElement<T>();
      mSegments.insertBack(last);
    }
    last->AppendElement(aElement);
  }

  void Clear()
  {
    SegmentedArrayElement<T>* first;
    while ((first = mSegments.popFirst())) {
      delete first;
    }
  }

  SegmentedArrayElement<T>* GetFirstSegment()
  {
    return mSegments.getFirst();
  }

  bool IsEmpty()
  {
    return !GetFirstSegment();
  }

private:
  mozilla::LinkedList<SegmentedArrayElement<T>> mSegments;
};






class JSPurpleBuffer
{
public:
  JSPurpleBuffer(JSPurpleBuffer*& aReferenceToThis)
    : mReferenceToThis(aReferenceToThis)
  {
    mReferenceToThis = this;
    NS_ADDREF_THIS();
    mozilla::HoldJSObjects(this);
  }

  ~JSPurpleBuffer()
  {
    MOZ_ASSERT(mValues.IsEmpty());
    MOZ_ASSERT(mObjects.IsEmpty());
    MOZ_ASSERT(mTenuredObjects.IsEmpty());
  }

  void Destroy()
  {
    mReferenceToThis = nullptr;
    mValues.Clear();
    mObjects.Clear();
    mTenuredObjects.Clear();
    mozilla::DropJSObjects(this);
    NS_RELEASE_THIS();
  }

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(JSPurpleBuffer)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(JSPurpleBuffer)

  JSPurpleBuffer*& mReferenceToThis;
  SegmentedArray<JS::Heap<JS::Value>> mValues;
  SegmentedArray<JS::Heap<JSObject*>> mObjects;
  SegmentedArray<JS::TenuredHeap<JSObject*>> mTenuredObjects;
};

NS_IMPL_CYCLE_COLLECTION_CLASS(JSPurpleBuffer)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(JSPurpleBuffer)
  tmp->Destroy();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(JSPurpleBuffer)
  CycleCollectionNoteChild(cb, tmp, "self");
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

#define NS_TRACE_SEGMENTED_ARRAY(_field)                                       \
    {                                                                          \
        auto segment = tmp->_field.GetFirstSegment();                          \
        while (segment) {                                                      \
            for (uint32_t i = segment->Length(); i > 0;) {                     \
                aCallbacks.Trace(&segment->ElementAt(--i), #_field, aClosure); \
            }                                                                  \
            segment = segment->getNext();                                      \
        }                                                                      \
    }

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(JSPurpleBuffer)
  NS_TRACE_SEGMENTED_ARRAY(mValues)
  NS_TRACE_SEGMENTED_ARRAY(mObjects)
  NS_TRACE_SEGMENTED_ARRAY(mTenuredObjects)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(JSPurpleBuffer, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(JSPurpleBuffer, Release)

struct SnowWhiteObject
{
  void* mPointer;
  nsCycleCollectionParticipant* mParticipant;
  nsCycleCollectingAutoRefCnt* mRefCnt;
};

class SnowWhiteKiller : public TraceCallbacks
{
public:
  SnowWhiteKiller(nsCycleCollector *aCollector, uint32_t aMaxCount)
    : mCollector(aCollector)
  {
    MOZ_ASSERT(mCollector, "Calling SnowWhiteKiller after nsCC went away");
    while (true) {
      if (mObjects.SetCapacity(aMaxCount)) {
        break;
      }
      if (aMaxCount == 1) {
        NS_RUNTIMEABORT("Not enough memory to even delete objects!");
      }
      aMaxCount /= 2;
    }
  }

  ~SnowWhiteKiller()
  {
    for (uint32_t i = 0; i < mObjects.Length(); ++i) {
      SnowWhiteObject& o = mObjects[i];
      if (!o.mRefCnt->get() && !o.mRefCnt->IsInPurpleBuffer()) {
        mCollector->RemoveObjectFromGraph(o.mPointer);
        o.mRefCnt->stabilizeForDeletion();
        o.mParticipant->Trace(o.mPointer, *this, nullptr);
        o.mParticipant->DeleteCycleCollectable(o.mPointer);
      }
    }
  }

  void
  Visit(nsPurpleBuffer& aBuffer, nsPurpleBufferEntry* aEntry)
  {
    MOZ_ASSERT(aEntry->mObject, "Null object in purple buffer");
    if (!aEntry->mRefCnt->get()) {
      void *o = aEntry->mObject;
      nsCycleCollectionParticipant *cp = aEntry->mParticipant;
      CanonicalizeParticipant(&o, &cp);
      SnowWhiteObject swo = { o, cp, aEntry->mRefCnt };
      if (mObjects.AppendElement(swo)) {
        aBuffer.Remove(aEntry);
      }
    }
  }

  bool HasSnowWhiteObjects() const
  {
    return mObjects.Length() > 0;
  }

  virtual void Trace(JS::Heap<JS::Value>* aValue, const char* aName,
                     void* aClosure) const
  {
    if (aValue->isMarkable()) {
      void* thing = aValue->toGCThing();
      if (thing && xpc_GCThingIsGrayCCThing(thing)) {
        mCollector->GetJSPurpleBuffer()->mValues.AppendElement(*aValue);
      }
    }
  }

  virtual void Trace(JS::Heap<jsid>* aId, const char* aName,
                     void* aClosure) const
  {
  }

  virtual void Trace(JS::Heap<JSObject*>* aObject, const char* aName,
                     void* aClosure) const
  {
    if (*aObject && xpc_GCThingIsGrayCCThing(*aObject)) {
      mCollector->GetJSPurpleBuffer()->mObjects.AppendElement(*aObject);
    }
  }

  virtual void Trace(JS::TenuredHeap<JSObject*>* aObject, const char* aName,
                     void* aClosure) const
  {
    if (*aObject && xpc_GCThingIsGrayCCThing(*aObject)) {
      mCollector->GetJSPurpleBuffer()->mTenuredObjects.AppendElement(*aObject);
    }
  }

  virtual void Trace(JS::Heap<JSString*>* aString, const char* aName,
                     void* aClosure) const
  {
  }

  virtual void Trace(JS::Heap<JSScript*>* aScript, const char* aName,
                     void* aClosure) const
  {
  }

  virtual void Trace(JS::Heap<JSFunction*>* aFunction, const char* aName,
                     void* aClosure) const
  {
  }

private:
  nsCycleCollector *mCollector;
  FallibleTArray<SnowWhiteObject> mObjects;
};

class RemoveSkippableVisitor : public SnowWhiteKiller
{
public:
  RemoveSkippableVisitor(nsCycleCollector* aCollector,
                         uint32_t aMaxCount, bool aRemoveChildlessNodes,
                         bool aAsyncSnowWhiteFreeing,
                         CC_ForgetSkippableCallback aCb)
    : SnowWhiteKiller(aCollector, aAsyncSnowWhiteFreeing ? 0 : aMaxCount),
      mRemoveChildlessNodes(aRemoveChildlessNodes),
      mAsyncSnowWhiteFreeing(aAsyncSnowWhiteFreeing),
      mDispatchedDeferredDeletion(false),
      mCallback(aCb)
  {}

  ~RemoveSkippableVisitor()
  {
    
    
    if (mCallback) {
      mCallback();
    }
    if (HasSnowWhiteObjects()) {
      
      nsCycleCollector_dispatchDeferredDeletion(true);
    }
  }

  void
  Visit(nsPurpleBuffer &aBuffer, nsPurpleBufferEntry *aEntry)
  {
    MOZ_ASSERT(aEntry->mObject, "null mObject in purple buffer");
    if (!aEntry->mRefCnt->get()) {
      if (!mAsyncSnowWhiteFreeing) {
        SnowWhiteKiller::Visit(aBuffer, aEntry);
      } else if (!mDispatchedDeferredDeletion) {
        mDispatchedDeferredDeletion = true;
        nsCycleCollector_dispatchDeferredDeletion(false);
      }
      return;
    }
    void *o = aEntry->mObject;
    nsCycleCollectionParticipant *cp = aEntry->mParticipant;
    CanonicalizeParticipant(&o, &cp);
    if (aEntry->mRefCnt->IsPurple() && !cp->CanSkip(o, false) &&
        (!mRemoveChildlessNodes || MayHaveChild(o, cp))) {
      return;
    }
    aBuffer.Remove(aEntry);
  }

private:
  bool mRemoveChildlessNodes;
  bool mAsyncSnowWhiteFreeing;
  bool mDispatchedDeferredDeletion;
  CC_ForgetSkippableCallback mCallback;
};

void
nsPurpleBuffer::RemoveSkippable(nsCycleCollector* aCollector,
                                bool aRemoveChildlessNodes,
                                bool aAsyncSnowWhiteFreeing,
                                CC_ForgetSkippableCallback aCb)
{
  RemoveSkippableVisitor visitor(aCollector, Count(), aRemoveChildlessNodes,
                                 aAsyncSnowWhiteFreeing, aCb);
  VisitEntries(visitor);
}

bool
nsCycleCollector::FreeSnowWhite(bool aUntilNoSWInPurpleBuffer)
{
  CheckThreadSafety();

  if (mFreeingSnowWhite) {
    return false;
  }

  AutoRestore<bool> ar(mFreeingSnowWhite);
  mFreeingSnowWhite = true;

  bool hadSnowWhiteObjects = false;
  do {
    SnowWhiteKiller visitor(this, mPurpleBuf.Count());
    mPurpleBuf.VisitEntries(visitor);
    hadSnowWhiteObjects = hadSnowWhiteObjects ||
                          visitor.HasSnowWhiteObjects();
    if (!visitor.HasSnowWhiteObjects()) {
      break;
    }
  } while (aUntilNoSWInPurpleBuffer);
  return hadSnowWhiteObjects;
}

void
nsCycleCollector::ForgetSkippable(bool aRemoveChildlessNodes,
                                  bool aAsyncSnowWhiteFreeing)
{
  CheckThreadSafety();

  
  
  MOZ_ASSERT(mIncrementalPhase == IdlePhase);

  if (mJSRuntime) {
    mJSRuntime->PrepareForForgetSkippable();
  }
  MOZ_ASSERT(!mScanInProgress, "Don't forget skippable or free snow-white while scan is in progress.");
  mPurpleBuf.RemoveSkippable(this, aRemoveChildlessNodes,
                             aAsyncSnowWhiteFreeing, mForgetSkippableCB);
}

MOZ_NEVER_INLINE void
nsCycleCollector::MarkRoots(SliceBudget &aBudget)
{
  const intptr_t kNumNodesBetweenTimeChecks = 1000;
  const intptr_t kStep = SliceBudget::CounterReset / kNumNodesBetweenTimeChecks;

  TimeLog timeLog;
  AutoRestore<bool> ar(mScanInProgress);
  MOZ_ASSERT(!mScanInProgress);
  mScanInProgress = true;
  MOZ_ASSERT(mIncrementalPhase == GraphBuildingPhase);
  MOZ_ASSERT(mCurrNode);

  while (!aBudget.isOverBudget() && !mCurrNode->IsDone()) {
    PtrInfo *pi = mCurrNode->GetNext();
    if (!pi) {
      MOZ_CRASH();
    }

    
    
    
    mBuilder->Traverse(pi);
    if (mCurrNode->AtBlockEnd()) {
      mBuilder->SetLastChild();
    }
    aBudget.step(kStep);
  }

  if (!mCurrNode->IsDone()) {
    timeLog.Checkpoint("MarkRoots()");
    return;
  }

  if (mGraph.mRootCount > 0) {
    mBuilder->SetLastChild();
  }

  if (mBuilder->RanOutOfMemory()) {
    MOZ_ASSERT(false, "Ran out of memory while building cycle collector graph");
    CC_TELEMETRY(_OOM, true);
  }

  mBuilder = nullptr;
  mCurrNode = nullptr;
  mIncrementalPhase = ScanAndCollectWhitePhase;
  timeLog.Checkpoint("MarkRoots()");
}







struct ScanBlackVisitor
{
  ScanBlackVisitor(uint32_t &aWhiteNodeCount, bool &aFailed)
    : mWhiteNodeCount(aWhiteNodeCount), mFailed(aFailed)
  {
  }

  bool ShouldVisitNode(PtrInfo const *pi)
  {
    return pi->mColor != black;
  }

  MOZ_NEVER_INLINE void VisitNode(PtrInfo *pi)
  {
    if (pi->mColor == white)
      --mWhiteNodeCount;
    pi->mColor = black;
  }

  void Failed()
  {
    mFailed = true;
  }

private:
  uint32_t &mWhiteNodeCount;
  bool &mFailed;
};


struct scanVisitor
{
  scanVisitor(uint32_t &aWhiteNodeCount, bool &aFailed, bool aWasIncremental)
    : mWhiteNodeCount(aWhiteNodeCount), mFailed(aFailed),
      mWasIncremental(aWasIncremental)
  {
  }

  bool ShouldVisitNode(PtrInfo const *pi)
  {
    return pi->mColor == grey;
  }

  MOZ_NEVER_INLINE void VisitNode(PtrInfo *pi)
  {
    if (pi->mInternalRefs > pi->mRefCount && pi->mRefCount > 0) {
      
      
      
      
      
      if (!mWasIncremental || pi->mColor != black) {
        Fault("traversed refs exceed refcount", pi);
      }
    }

    if (pi->mInternalRefs == pi->mRefCount || pi->mRefCount == 0) {
      pi->mColor = white;
      ++mWhiteNodeCount;
    } else {
      GraphWalker<ScanBlackVisitor>(ScanBlackVisitor(mWhiteNodeCount, mFailed)).Walk(pi);
      MOZ_ASSERT(pi->mColor == black,
                 "Why didn't ScanBlackVisitor make pi black?");
    }
  }

  void Failed() {
    mFailed = true;
  }

private:
  uint32_t &mWhiteNodeCount;
  bool &mFailed;
  bool mWasIncremental;
};



void
nsCycleCollector::ScanWeakMaps()
{
  bool anyChanged;
  bool failed = false;
  do {
    anyChanged = false;
    for (uint32_t i = 0; i < mGraph.mWeakMaps.Length(); i++) {
      WeakMapping *wm = &mGraph.mWeakMaps[i];

      
      uint32_t mColor = wm->mMap ? wm->mMap->mColor : black;
      uint32_t kColor = wm->mKey ? wm->mKey->mColor : black;
      uint32_t kdColor = wm->mKeyDelegate ? wm->mKeyDelegate->mColor : black;
      uint32_t vColor = wm->mVal ? wm->mVal->mColor : black;

      
      
      
      
      MOZ_ASSERT(mColor != grey, "Uncolored weak map");
      MOZ_ASSERT(kColor != grey, "Uncolored weak map key");
      MOZ_ASSERT(kdColor != grey, "Uncolored weak map key delegate");
      MOZ_ASSERT(vColor != grey, "Uncolored weak map value");

      if (mColor == black && kColor != black && kdColor == black) {
        GraphWalker<ScanBlackVisitor>(ScanBlackVisitor(mWhiteNodeCount, failed)).Walk(wm->mKey);
        anyChanged = true;
      }

      if (mColor == black && kColor == black && vColor != black) {
        GraphWalker<ScanBlackVisitor>(ScanBlackVisitor(mWhiteNodeCount, failed)).Walk(wm->mVal);
        anyChanged = true;
      }
    }
  } while (anyChanged);

  if (failed) {
    MOZ_ASSERT(false, "Ran out of memory in ScanWeakMaps");
    CC_TELEMETRY(_OOM, true);
  }
}


class PurpleScanBlackVisitor
{
public:
  PurpleScanBlackVisitor(GCGraph &aGraph, nsICycleCollectorListener *aListener,
                         uint32_t &aCount, bool &aFailed)
    : mGraph(aGraph), mListener(aListener), mCount(aCount), mFailed(aFailed)
  {
  }

  void
  Visit(nsPurpleBuffer &aBuffer, nsPurpleBufferEntry *aEntry)
  {
    MOZ_ASSERT(aEntry->mObject, "Entries with null mObject shouldn't be in the purple buffer.");
    MOZ_ASSERT(aEntry->mRefCnt->get() != 0, "Snow-white objects shouldn't be in the purple buffer.");

    void *obj = aEntry->mObject;
    if (!aEntry->mParticipant) {
      obj = CanonicalizeXPCOMParticipant(static_cast<nsISupports*>(obj));
      MOZ_ASSERT(obj, "Don't add objects that don't participate in collection!");
    }

    PtrInfo *pi = mGraph.FindNode(obj);
    if (!pi) {
      return;
    }
    MOZ_ASSERT(pi->mParticipant, "No dead objects should be in the purple buffer.");
    if (MOZ_UNLIKELY(mListener)) {
      mListener->NoteIncrementalRoot((uint64_t)pi->mPointer);
    }
    if (pi->mColor == black) {
      return;
    }
    GraphWalker<ScanBlackVisitor>(ScanBlackVisitor(mCount, mFailed)).Walk(pi);
  }

private:
  GCGraph &mGraph;
  nsICycleCollectorListener *mListener;
  uint32_t &mCount;
  bool &mFailed;
};




void
nsCycleCollector::ScanIncrementalRoots()
{
  TimeLog timeLog;

  
  
  
  
  
  
  
  
  
  bool failed = false;
  PurpleScanBlackVisitor purpleScanBlackVisitor(mGraph, mListener, mWhiteNodeCount, failed);
  mPurpleBuf.VisitEntries(purpleScanBlackVisitor);
  timeLog.Checkpoint("ScanIncrementalRoots::fix purple");

  
  
  
  
  
  if (mJSRuntime) {
    nsCycleCollectionParticipant *jsParticipant = mJSRuntime->GCThingParticipant();
    nsCycleCollectionParticipant *zoneParticipant = mJSRuntime->ZoneParticipant();
    NodePool::Enumerator etor(mGraph.mNodes);

    while (!etor.IsDone()) {
      PtrInfo *pi = etor.GetNext();

      
      if (pi->mRefCount != 0) {
        continue;
      }

      
      
      
      if (pi->mColor == black && MOZ_LIKELY(!mListener)) {
        continue;
      }

      
      
      if (pi->mParticipant == jsParticipant) {
        if (xpc_GCThingIsGrayCCThing(pi->mPointer)) {
          continue;
        }
      } else if (pi->mParticipant == zoneParticipant) {
        JS::Zone *zone = static_cast<JS::Zone*>(pi->mPointer);
        if (js::ZoneGlobalsAreAllGray(zone)) {
          continue;
        }
      } else {
        MOZ_ASSERT(false, "Non-JS thing with 0 refcount? Treating as live.");
      }

      

      
      
      
      if (MOZ_UNLIKELY(mListener)) {
        mListener->NoteIncrementalRoot((uint64_t)pi->mPointer);
      }

      GraphWalker<ScanBlackVisitor>(ScanBlackVisitor(mWhiteNodeCount, failed)).Walk(pi);
    }

    timeLog.Checkpoint("ScanIncrementalRoots::fix JS");
  }

  if (failed) {
    NS_ASSERTION(false, "Ran out of memory in ScanIncrementalRoots");
    CC_TELEMETRY(_OOM, true);
  }
}

void
nsCycleCollector::ScanRoots(bool aFullySynchGraphBuild)
{
  AutoRestore<bool> ar(mScanInProgress);
  MOZ_ASSERT(!mScanInProgress);
  mScanInProgress = true;
  mWhiteNodeCount = 0;
  MOZ_ASSERT(mIncrementalPhase == ScanAndCollectWhitePhase);

  if (!aFullySynchGraphBuild) {
    ScanIncrementalRoots();
  }

  TimeLog timeLog;

  
  
  
  bool failed = false;
  scanVisitor sv(mWhiteNodeCount, failed, !aFullySynchGraphBuild);
  GraphWalker<scanVisitor>(sv).WalkFromRoots(mGraph);
  timeLog.Checkpoint("ScanRoots::WalkFromRoots");

  if (failed) {
    NS_ASSERTION(false, "Ran out of memory in ScanRoots");
    CC_TELEMETRY(_OOM, true);
  }

  
  ScanWeakMaps();
  timeLog.Checkpoint("ScanRoots::ScanWeakMaps");

  if (mListener) {
    mListener->BeginResults();

    NodePool::Enumerator etor(mGraph.mNodes);
    while (!etor.IsDone()) {
      PtrInfo *pi = etor.GetNext();
      if (!pi->mParticipant) {
        continue;
      }
      switch (pi->mColor) {
        case black:
          if (pi->mRefCount > 0 && pi->mRefCount < UINT32_MAX &&
              pi->mInternalRefs != pi->mRefCount) {
            mListener->DescribeRoot((uint64_t)pi->mPointer,
                                    pi->mInternalRefs);
          }
          break;
        case white:
          mListener->DescribeGarbage((uint64_t)pi->mPointer);
          break;
        case grey:
          
          
          break;
      }
    }

    mListener->End();
    mListener = nullptr;
    timeLog.Checkpoint("ScanRoots::listener");
  }
}






bool
nsCycleCollector::CollectWhite()
{
  
  
  
  
  
  
  
  
  
  
  
  
  

  TimeLog timeLog;
  nsAutoTArray<PtrInfo*, 4000> whiteNodes;

  MOZ_ASSERT(mIncrementalPhase == ScanAndCollectWhitePhase);

  whiteNodes.SetCapacity(mWhiteNodeCount);
  uint32_t numWhiteGCed = 0;

  NodePool::Enumerator etor(mGraph.mNodes);
  while (!etor.IsDone())
  {
    PtrInfo *pinfo = etor.GetNext();
    if (pinfo->mColor == white && pinfo->mParticipant) {
      whiteNodes.AppendElement(pinfo);
      pinfo->mParticipant->Root(pinfo->mPointer);
      if (pinfo->mRefCount == 0) {
        
        ++numWhiteGCed;
      }
    }
  }

  uint32_t count = whiteNodes.Length();
  MOZ_ASSERT(numWhiteGCed <= count,
             "More freed GCed nodes than total freed nodes.");
  mResults.mFreedRefCounted += count - numWhiteGCed;
  mResults.mFreedGCed += numWhiteGCed;

  timeLog.Checkpoint("CollectWhite::Root");

  if (mBeforeUnlinkCB) {
    mBeforeUnlinkCB();
    timeLog.Checkpoint("CollectWhite::BeforeUnlinkCB");
  }

  for (uint32_t i = 0; i < count; ++i) {
    PtrInfo *pinfo = whiteNodes.ElementAt(i);
    MOZ_ASSERT(pinfo->mParticipant, "Unlink shouldn't see objects removed from graph.");
    pinfo->mParticipant->Unlink(pinfo->mPointer);
#ifdef DEBUG
    if (mJSRuntime) {
      mJSRuntime->AssertNoObjectsToTrace(pinfo->mPointer);
    }
#endif
  }
  timeLog.Checkpoint("CollectWhite::Unlink");

  for (uint32_t i = 0; i < count; ++i) {
    PtrInfo *pinfo = whiteNodes.ElementAt(i);
    MOZ_ASSERT(pinfo->mParticipant, "Unroot shouldn't see objects removed from graph.");
    pinfo->mParticipant->Unroot(pinfo->mPointer);
  }
  timeLog.Checkpoint("CollectWhite::Unroot");

  nsCycleCollector_dispatchDeferredDeletion(false);
  mIncrementalPhase = CleanupPhase;

  return count > 0;
}






MOZ_DEFINE_MALLOC_SIZE_OF(CycleCollectorMallocSizeOf)

NS_IMETHODIMP
nsCycleCollector::CollectReports(nsIHandleReportCallback* aHandleReport,
                                 nsISupports* aData)
{
  size_t objectSize, graphNodesSize, graphEdgesSize, weakMapsSize,
         purpleBufferSize;
  SizeOfIncludingThis(CycleCollectorMallocSizeOf,
                      &objectSize,
                      &graphNodesSize, &graphEdgesSize,
                      &weakMapsSize,
                      &purpleBufferSize);

#define REPORT(_path, _amount, _desc)                                     \
    do {                                                                  \
        size_t amount = _amount;  /* evaluate |_amount| only once */      \
        if (amount > 0) {                                                 \
            nsresult rv;                                                  \
            rv = aHandleReport->Callback(EmptyCString(),                  \
                                         NS_LITERAL_CSTRING(_path),       \
                                         KIND_HEAP, UNITS_BYTES, _amount, \
                                         NS_LITERAL_CSTRING(_desc),       \
                                         aData);                          \
            if (NS_WARN_IF(NS_FAILED(rv)))                                \
                return rv;                                                \
        }                                                                 \
    } while (0)

  REPORT("explicit/cycle-collector/collector-object", objectSize,
         "Memory used for the cycle collector object itself.");

  REPORT("explicit/cycle-collector/graph-nodes", graphNodesSize,
         "Memory used for the nodes of the cycle collector's graph. "
         "This should be zero when the collector is idle.");

  REPORT("explicit/cycle-collector/graph-edges", graphEdgesSize,
         "Memory used for the edges of the cycle collector's graph. "
         "This should be zero when the collector is idle.");

  REPORT("explicit/cycle-collector/weak-maps", weakMapsSize,
         "Memory used for the representation of weak maps in the "
         "cycle collector's graph. "
         "This should be zero when the collector is idle.");

  REPORT("explicit/cycle-collector/purple-buffer", purpleBufferSize,
         "Memory used for the cycle collector's purple buffer.");

#undef REPORT

  return NS_OK;
};






nsCycleCollector::nsCycleCollector() :
  mActivelyCollecting(false),
  mFreeingSnowWhite(false),
  mScanInProgress(false),
  mJSRuntime(nullptr),
  mIncrementalPhase(IdlePhase),
  mThread(NS_GetCurrentThread()),
  mWhiteNodeCount(0),
  mBeforeUnlinkCB(nullptr),
  mForgetSkippableCB(nullptr),
  mUnmergedNeeded(0),
  mMergedInARow(0),
  mJSPurpleBuffer(nullptr)
{
}

nsCycleCollector::~nsCycleCollector()
{
  UnregisterWeakMemoryReporter(this);
}

void
nsCycleCollector::RegisterJSRuntime(CycleCollectedJSRuntime *aJSRuntime)
{
  if (mJSRuntime)
    Fault("multiple registrations of cycle collector JS runtime", aJSRuntime);

  mJSRuntime = aJSRuntime;

  
  
  
  static bool registered = false;
  if (!registered) {
    RegisterWeakMemoryReporter(this);
    registered = true;
  }
}

void
nsCycleCollector::ForgetJSRuntime()
{
  if (!mJSRuntime)
    Fault("forgetting non-registered cycle collector JS runtime");

  mJSRuntime = nullptr;
}

#ifdef DEBUG
static bool
HasParticipant(void *aPtr, nsCycleCollectionParticipant *aParti)
{
  if (aParti) {
    return true;
  }

  nsXPCOMCycleCollectionParticipant *xcp;
  ToParticipant(static_cast<nsISupports*>(aPtr), &xcp);
  return xcp != nullptr;
}
#endif

MOZ_ALWAYS_INLINE void
nsCycleCollector::Suspect(void *aPtr, nsCycleCollectionParticipant *aParti,
                          nsCycleCollectingAutoRefCnt *aRefCnt)
{
  CheckThreadSafety();

  
  
  

  if (MOZ_UNLIKELY(mScanInProgress)) {
    return;
  }

  MOZ_ASSERT(aPtr, "Don't suspect null pointers");

  MOZ_ASSERT(HasParticipant(aPtr, aParti),
             "Suspected nsISupports pointer must QI to nsXPCOMCycleCollectionParticipant");

  mPurpleBuf.Put(aPtr, aParti, aRefCnt);
}

void
nsCycleCollector::CheckThreadSafety()
{
#ifdef DEBUG
  nsIThread* currentThread = NS_GetCurrentThread();
  
  
  MOZ_ASSERT(mThread == currentThread || !currentThread);
#endif
}







void
nsCycleCollector::FixGrayBits(bool aForceGC)
{
  CheckThreadSafety();

  if (!mJSRuntime)
    return;

  if (!aForceGC) {
    mJSRuntime->FixWeakMappingGrayBits();

    bool needGC = !mJSRuntime->AreGCGrayBitsValid();
    
    CC_TELEMETRY(_NEED_GC, needGC);
    if (!needGC)
      return;
    mResults.mForcedGC = true;
  }

  TimeLog timeLog;
  mJSRuntime->GarbageCollect(aForceGC ? JS::gcreason::SHUTDOWN_CC : JS::gcreason::CC_FORCED);
  timeLog.Checkpoint("GC()");
}

void
nsCycleCollector::CleanupAfterCollection()
{
  MOZ_ASSERT(mIncrementalPhase == CleanupPhase);
  mGraph.Clear();

  uint32_t interval = (uint32_t) ((TimeStamp::Now() - mCollectionStart).ToMilliseconds());
#ifdef COLLECT_TIME_DEBUG
  printf("cc: total cycle collector time was %ums in %u slices\n", interval, mResults.mNumSlices);
  printf("cc: visited %u ref counted and %u GCed objects, freed %d ref counted and %d GCed objects",
         mResults.mVisitedRefCounted, mResults.mVisitedGCed,
         mResults.mFreedRefCounted, mResults.mFreedGCed);
  uint32_t numVisited = mResults.mVisitedRefCounted + mResults.mVisitedGCed;
  if (numVisited > 1000) {
    uint32_t numFreed = mResults.mFreedRefCounted + mResults.mFreedGCed;
    printf(" (%d%%)", 100 * numFreed / numVisited);
  }
  printf(".\ncc: \n");
#endif
  CC_TELEMETRY( , interval);
  CC_TELEMETRY(_VISITED_REF_COUNTED, mResults.mVisitedRefCounted);
  CC_TELEMETRY(_VISITED_GCED, mResults.mVisitedGCed);
  CC_TELEMETRY(_COLLECTED, mWhiteNodeCount);

  if (mJSRuntime) {
    mJSRuntime->EndCycleCollectionCallback(mResults);
  }
  mIncrementalPhase = IdlePhase;
}

void
nsCycleCollector::ShutdownCollect()
{
  SliceBudget unlimitedBudget;
  uint32_t i;
  for (i = 0; i < DEFAULT_SHUTDOWN_COLLECTIONS; ++i) {
    if (!Collect(ShutdownCC, unlimitedBudget, nullptr)) {
      break;
    }
  }
  NS_WARN_IF_FALSE(i < NORMAL_SHUTDOWN_COLLECTIONS, "Extra shutdown CC");
}

static void
PrintPhase(const char *aPhase)
{
#ifdef DEBUG_PHASES
  printf("cc: begin %s on %s\n", aPhase,
         NS_IsMainThread() ? "mainthread" : "worker");
#endif
}

bool
nsCycleCollector::Collect(ccType aCCType,
                          SliceBudget &aBudget,
                          nsICycleCollectorListener *aManualListener)
{
  CheckThreadSafety();

  
  if (mActivelyCollecting || mFreeingSnowWhite) {
    return false;
  }
  mActivelyCollecting = true;

  bool startedIdle = (mIncrementalPhase == IdlePhase);
  bool collectedAny = false;

  
  
  if (!startedIdle) {
    FreeSnowWhite(true);
  }

  ++mResults.mNumSlices;

  bool finished = false;
  do {
    switch (mIncrementalPhase) {
      case IdlePhase:
        PrintPhase("BeginCollection");
        BeginCollection(aCCType, aManualListener);
        break;
      case GraphBuildingPhase:
        PrintPhase("MarkRoots");
        MarkRoots(aBudget);
        break;
      case ScanAndCollectWhitePhase:
        
        
        
        
        PrintPhase("ScanRoots");
        ScanRoots(startedIdle);
        PrintPhase("CollectWhite");
        collectedAny = CollectWhite();
        break;
      case CleanupPhase:
        PrintPhase("CleanupAfterCollection");
        CleanupAfterCollection();
        finished = true;
        break;
    }
  } while (!aBudget.checkOverBudget() && !finished);

  
  
  mActivelyCollecting = false;

  if (aCCType != SliceCC && !startedIdle) {
    
    
    
    MOZ_ASSERT(mIncrementalPhase == IdlePhase);
    if (Collect(aCCType, aBudget, aManualListener)) {
      collectedAny = true;
    }
  }

  MOZ_ASSERT_IF(aCCType != SliceCC, mIncrementalPhase == IdlePhase);

  return collectedAny;
}





void
nsCycleCollector::PrepareForGarbageCollection()
{
  if (mIncrementalPhase == IdlePhase) {
    MOZ_ASSERT(mGraph.IsEmpty(), "Non-empty graph when idle");
    MOZ_ASSERT(!mBuilder, "Non-null builder when idle");
    if (mJSPurpleBuffer) {
      mJSPurpleBuffer->Destroy();
    }
    return;
  }

  FinishAnyCurrentCollection();
}

void
nsCycleCollector::FinishAnyCurrentCollection()
{
  if (mIncrementalPhase == IdlePhase) {
    return;
  }

  SliceBudget unlimitedBudget;
  PrintPhase("FinishAnyCurrentCollection");
  
  Collect(SliceCC, unlimitedBudget, nullptr);
  MOZ_ASSERT(mIncrementalPhase == IdlePhase);
}



static const uint32_t kMinConsecutiveUnmerged = 3;
static const uint32_t kMaxConsecutiveMerged = 3;

bool
nsCycleCollector::ShouldMergeZones(ccType aCCType)
{
  if (!mJSRuntime) {
    return false;
  }

  MOZ_ASSERT(mUnmergedNeeded <= kMinConsecutiveUnmerged);
  MOZ_ASSERT(mMergedInARow <= kMaxConsecutiveMerged);

  if (mMergedInARow == kMaxConsecutiveMerged) {
    MOZ_ASSERT(mUnmergedNeeded == 0);
    mUnmergedNeeded = kMinConsecutiveUnmerged;
  }

  if (mUnmergedNeeded > 0) {
    mUnmergedNeeded--;
    mMergedInARow = 0;
    return false;
  }

  if (aCCType == SliceCC && mJSRuntime->UsefulToMergeZones()) {
    mMergedInARow++;
    return true;
  } else {
    mMergedInARow = 0;
    return false;
  }
}

void
nsCycleCollector::BeginCollection(ccType aCCType,
                                  nsICycleCollectorListener *aManualListener)
{
  TimeLog timeLog;
  MOZ_ASSERT(mIncrementalPhase == IdlePhase);

  mCollectionStart = TimeStamp::Now();

  if (mJSRuntime) {
    mJSRuntime->BeginCycleCollectionCallback();
    timeLog.Checkpoint("BeginCycleCollectionCallback()");
  }

  bool isShutdown = (aCCType == ShutdownCC);

  
  MOZ_ASSERT_IF(isShutdown, !aManualListener);
  MOZ_ASSERT(!mListener, "Forgot to clear a previous listener?");
  mListener = aManualListener;
  aManualListener = nullptr;
  if (!mListener && mParams.LogThisCC(isShutdown)) {
    nsRefPtr<nsCycleCollectorLogger> logger = new nsCycleCollectorLogger();
    if (mParams.AllTracesThisCC(isShutdown)) {
      logger->SetAllTraces();
    }
    mListener = logger.forget();
  }

  bool forceGC = isShutdown;
  if (!forceGC && mListener) {
    
    
    mListener->GetWantAllTraces(&forceGC);
  }
  FixGrayBits(forceGC);

  FreeSnowWhite(true);

  if (mListener && NS_FAILED(mListener->Begin())) {
    mListener = nullptr;
  }

  
  mGraph.Init();
  mResults.Init();
  bool mergeZones = ShouldMergeZones(aCCType);
  mResults.mMergedZones = mergeZones;

  MOZ_ASSERT(!mBuilder, "Forgot to clear mBuilder");
  mBuilder = new GCGraphBuilder(mGraph, mResults, mJSRuntime, mListener, mergeZones);

  if (mJSRuntime) {
    mJSRuntime->TraverseRoots(*mBuilder);
    timeLog.Checkpoint("mJSRuntime->TraverseRoots()");
  }

  AutoRestore<bool> ar(mScanInProgress);
  MOZ_ASSERT(!mScanInProgress);
  mScanInProgress = true;
  mPurpleBuf.SelectPointers(*mBuilder);
  timeLog.Checkpoint("SelectPointers()");

  
  mGraph.mRootCount = mGraph.MapCount();

  mCurrNode = new NodePool::Enumerator(mGraph.mNodes);
  mIncrementalPhase = GraphBuildingPhase;
}

uint32_t
nsCycleCollector::SuspectedCount()
{
  CheckThreadSafety();
  return mPurpleBuf.Count();
}

void
nsCycleCollector::Shutdown()
{
  CheckThreadSafety();

  
  FreeSnowWhite(true);

#ifndef DEBUG
  if (PR_GetEnv("MOZ_CC_RUN_DURING_SHUTDOWN"))
#endif
  {
    ShutdownCollect();
  }
}

void
nsCycleCollector::RemoveObjectFromGraph(void *aObj)
{
  if (mIncrementalPhase == IdlePhase) {
    return;
  }

  if (PtrInfo *pinfo = mGraph.FindNode(aObj)) {
    mGraph.RemoveNodeFromMap(aObj);

    pinfo->mPointer = nullptr;
    pinfo->mParticipant = nullptr;
  }
}

void
nsCycleCollector::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                      size_t *aObjectSize,
                                      size_t *aGraphNodesSize,
                                      size_t *aGraphEdgesSize,
                                      size_t *aWeakMapsSize,
                                      size_t *aPurpleBufferSize) const
{
  *aObjectSize = aMallocSizeOf(this);

  mGraph.SizeOfExcludingThis(aMallocSizeOf, aGraphNodesSize, aGraphEdgesSize,
                             aWeakMapsSize);

  *aPurpleBufferSize = mPurpleBuf.SizeOfExcludingThis(aMallocSizeOf);

  
  
  
}

JSPurpleBuffer*
nsCycleCollector::GetJSPurpleBuffer()
{
  if (!mJSPurpleBuffer) {
    
    
    
    nsRefPtr<JSPurpleBuffer> pb = new JSPurpleBuffer(mJSPurpleBuffer);
  }
  return mJSPurpleBuffer;
}






void
nsCycleCollector_registerJSRuntime(CycleCollectedJSRuntime *rt)
{
  CollectorData *data = sCollectorData.get();

  
  MOZ_ASSERT(data);
  MOZ_ASSERT(data->mCollector);
  
  MOZ_ASSERT(!data->mRuntime);

  data->mRuntime = rt;
  data->mCollector->RegisterJSRuntime(rt);
}

void
nsCycleCollector_forgetJSRuntime()
{
  CollectorData *data = sCollectorData.get();

  
  MOZ_ASSERT(data);
  
  MOZ_ASSERT(data->mRuntime);

  
  if (data->mCollector) {
    data->mCollector->ForgetJSRuntime();
    data->mRuntime = nullptr;
  } else {
    data->mRuntime = nullptr;
    delete data;
    sCollectorData.set(nullptr);
  }
}

 CycleCollectedJSRuntime*
CycleCollectedJSRuntime::Get()
{
  CollectorData* data = sCollectorData.get();
  if (data) {
    return data->mRuntime;
  }
  return nullptr;
}


namespace mozilla {
namespace cyclecollector {

void
HoldJSObjectsImpl(void* aHolder, nsScriptObjectTracer* aTracer)
{
  CollectorData* data = sCollectorData.get();

  
  MOZ_ASSERT(data);
  MOZ_ASSERT(data->mCollector);
  
  MOZ_ASSERT(data->mRuntime);

  data->mRuntime->AddJSHolder(aHolder, aTracer);
}

void
HoldJSObjectsImpl(nsISupports* aHolder)
{
  nsXPCOMCycleCollectionParticipant* participant;
  CallQueryInterface(aHolder, &participant);
  MOZ_ASSERT(participant, "Failed to QI to nsXPCOMCycleCollectionParticipant!");
  MOZ_ASSERT(participant->CheckForRightISupports(aHolder),
             "The result of QIing a JS holder should be the same as ToSupports");

  HoldJSObjectsImpl(aHolder, participant);
}

void
DropJSObjectsImpl(void* aHolder)
{
  CollectorData* data = sCollectorData.get();

  
  
  MOZ_ASSERT(data);
  
  MOZ_ASSERT(data->mRuntime);

  data->mRuntime->RemoveJSHolder(aHolder);
}

void
DropJSObjectsImpl(nsISupports* aHolder)
{
#ifdef DEBUG
  nsXPCOMCycleCollectionParticipant* participant;
  CallQueryInterface(aHolder, &participant);
  MOZ_ASSERT(participant, "Failed to QI to nsXPCOMCycleCollectionParticipant!");
  MOZ_ASSERT(participant->CheckForRightISupports(aHolder),
             "The result of QIing a JS holder should be the same as ToSupports");
#endif
  DropJSObjectsImpl(static_cast<void*>(aHolder));
}

#ifdef DEBUG
bool
IsJSHolder(void* aHolder)
{
  CollectorData *data = sCollectorData.get();

  
  
  MOZ_ASSERT(data);
  
  MOZ_ASSERT(data->mRuntime);

  return data->mRuntime->IsJSHolder(aHolder);
}
#endif

void
DeferredFinalize(nsISupports* aSupports)
{
  CollectorData *data = sCollectorData.get();

  
  
  MOZ_ASSERT(data);
  
  MOZ_ASSERT(data->mRuntime);

  data->mRuntime->DeferredFinalize(aSupports);
}

void
DeferredFinalize(DeferredFinalizeAppendFunction aAppendFunc,
                 DeferredFinalizeFunction aFunc,
                 void* aThing)
{
  CollectorData *data = sCollectorData.get();

  
  
  MOZ_ASSERT(data);
  
  MOZ_ASSERT(data->mRuntime);

  data->mRuntime->DeferredFinalize(aAppendFunc, aFunc, aThing);
}

} 
} 


MOZ_NEVER_INLINE static void
SuspectAfterShutdown(void* n, nsCycleCollectionParticipant* cp,
                     nsCycleCollectingAutoRefCnt* aRefCnt,
                     bool* aShouldDelete)
{
  if (aRefCnt->get() == 0) {
    if (!aShouldDelete) {
      
      CanonicalizeParticipant(&n, &cp);
      aRefCnt->stabilizeForDeletion();
      cp->DeleteCycleCollectable(n);
    } else {
      *aShouldDelete = true;
    }
  } else {
    
    aRefCnt->RemoveFromPurpleBuffer();
  }
}

void
NS_CycleCollectorSuspect3(void *n, nsCycleCollectionParticipant *cp,
                          nsCycleCollectingAutoRefCnt *aRefCnt,
                          bool* aShouldDelete)
{
  CollectorData *data = sCollectorData.get();

  
  MOZ_ASSERT(data);

  if (MOZ_LIKELY(data->mCollector)) {
    data->mCollector->Suspect(n, cp, aRefCnt);
    return;
  }
  SuspectAfterShutdown(n, cp, aRefCnt, aShouldDelete);
}

uint32_t
nsCycleCollector_suspectedCount()
{
  CollectorData *data = sCollectorData.get();

  
  MOZ_ASSERT(data);

  if (!data->mCollector) {
    return 0;
  }

  return data->mCollector->SuspectedCount();
}

bool
nsCycleCollector_init()
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
  MOZ_ASSERT(!sCollectorData.initialized(), "Called twice!?");

  return sCollectorData.init();
}

void
nsCycleCollector_startup()
{
  MOZ_ASSERT(sCollectorData.initialized(),
             "Forgot to call nsCycleCollector_init!");
  if (sCollectorData.get()) {
    MOZ_CRASH();
  }

  CollectorData* data = new CollectorData;
  data->mCollector = new nsCycleCollector();
  data->mRuntime = nullptr;

  sCollectorData.set(data);
}

void
nsCycleCollector_setBeforeUnlinkCallback(CC_BeforeUnlinkCallback aCB)
{
  CollectorData *data = sCollectorData.get();

  
  MOZ_ASSERT(data);
  MOZ_ASSERT(data->mCollector);

  data->mCollector->SetBeforeUnlinkCallback(aCB);
}

void
nsCycleCollector_setForgetSkippableCallback(CC_ForgetSkippableCallback aCB)
{
  CollectorData *data = sCollectorData.get();

  
  MOZ_ASSERT(data);
  MOZ_ASSERT(data->mCollector);

  data->mCollector->SetForgetSkippableCallback(aCB);
}

void
nsCycleCollector_forgetSkippable(bool aRemoveChildlessNodes,
                                 bool aAsyncSnowWhiteFreeing)
{
  CollectorData *data = sCollectorData.get();

  
  MOZ_ASSERT(data);
  MOZ_ASSERT(data->mCollector);

  PROFILER_LABEL("CC", "nsCycleCollector_forgetSkippable");
  TimeLog timeLog;
  data->mCollector->ForgetSkippable(aRemoveChildlessNodes,
                                    aAsyncSnowWhiteFreeing);
  timeLog.Checkpoint("ForgetSkippable()");
}

void
nsCycleCollector_dispatchDeferredDeletion(bool aContinuation)
{
  CollectorData *data = sCollectorData.get();

  if (!data || !data->mRuntime) {
    return;
  }

  data->mRuntime->DispatchDeferredDeletion(aContinuation);
}

bool
nsCycleCollector_doDeferredDeletion()
{
  CollectorData *data = sCollectorData.get();

  
  MOZ_ASSERT(data);
  MOZ_ASSERT(data->mCollector);
  MOZ_ASSERT(data->mRuntime);

  return data->mCollector->FreeSnowWhite(false);
}

void
nsCycleCollector_collect(nsICycleCollectorListener *aManualListener)
{
  CollectorData *data = sCollectorData.get();

  
  MOZ_ASSERT(data);
  MOZ_ASSERT(data->mCollector);

  PROFILER_LABEL("CC", "nsCycleCollector_collect");
  SliceBudget unlimitedBudget;
  data->mCollector->Collect(ManualCC, unlimitedBudget, aManualListener);
}

void
nsCycleCollector_collectSlice(int64_t aSliceTime)
{
  CollectorData *data = sCollectorData.get();

  
  MOZ_ASSERT(data);
  MOZ_ASSERT(data->mCollector);

  PROFILER_LABEL("CC", "nsCycleCollector_collectSlice");
  SliceBudget budget;
  if (aSliceTime >= 0) {
    budget = SliceBudget::TimeBudget(aSliceTime);
  }
  data->mCollector->Collect(SliceCC, budget, nullptr);
}

void
nsCycleCollector_collectSliceWork(int64_t aSliceWork)
{
  CollectorData *data = sCollectorData.get();

  
  MOZ_ASSERT(data);
  MOZ_ASSERT(data->mCollector);

  PROFILER_LABEL("CC", "nsCycleCollector_collectSliceWork");
  SliceBudget budget;
  if (aSliceWork >= 0) {
    budget = SliceBudget::WorkBudget(aSliceWork);
  }
  data->mCollector->Collect(SliceCC, budget, nullptr);
}

void
nsCycleCollector_prepareForGarbageCollection()
{
  CollectorData *data = sCollectorData.get();

  MOZ_ASSERT(data);

  if (!data->mCollector) {
    return;
  }

  data->mCollector->PrepareForGarbageCollection();
}

void
nsCycleCollector_finishAnyCurrentCollection()
{
    CollectorData *data = sCollectorData.get();

    MOZ_ASSERT(data);

    if (!data->mCollector) {
        return;
    }

    data->mCollector->FinishAnyCurrentCollection();
}

void
nsCycleCollector_shutdown()
{
  CollectorData *data = sCollectorData.get();

  if (data) {
    MOZ_ASSERT(data->mCollector);
    PROFILER_LABEL("CC", "nsCycleCollector_shutdown");
    data->mCollector->Shutdown();
    data->mCollector = nullptr;
    if (!data->mRuntime) {
      delete data;
      sCollectorData.set(nullptr);
    }
  }
}
