
















































































































































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
#include "mozilla/DebugOnly.h"
#include "mozilla/HoldDropJSObjects.h"

#include "mozilla/LinkedList.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/SegmentedVector.h"

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
    mLogAll(PR_GetEnv("MOZ_CC_LOG_ALL") != nullptr),
    mLogShutdown(PR_GetEnv("MOZ_CC_LOG_SHUTDOWN") != nullptr),
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
  TimeLog() : mLastCheckpoint(TimeStamp::Now())
  {
  }

  void
  Checkpoint(const char* aEvent)
  {
    TimeStamp now = TimeStamp::Now();
    double dur = (now - mLastCheckpoint).ToMilliseconds();
    if (dur >= 0.5) {
      printf("cc: %s took %.1fms\n", aEvent, dur);
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
  TimeLog()
  {
  }
  void Checkpoint(const char* aEvent)
  {
  }
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
    Block* b = Blocks();
    while (b) {
      Block* next = b->Next();
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
  union PtrInfoOrBlock
  {
    
    
    PtrInfo* ptrInfo;
    Block* block;
  };
  struct Block
  {
    enum { BlockSize = 16 * 1024 };

    PtrInfoOrBlock mPointers[BlockSize];
    Block()
    {
      mPointers[BlockSize - 2].block = nullptr; 
      mPointers[BlockSize - 1].block = nullptr; 
    }
    Block*& Next()
    {
      return mPointers[BlockSize - 1].block;
    }
    PtrInfoOrBlock* Start()
    {
      return &mPointers[0];
    }
    PtrInfoOrBlock* End()
    {
      return &mPointers[BlockSize - 2];
    }
  };

  
  
  PtrInfoOrBlock mSentinelAndBlocks[2];

  Block*& Blocks()
  {
    return mSentinelAndBlocks[1].block;
  }
  Block* Blocks() const
  {
    return mSentinelAndBlocks[1].block;
  }

public:
  class Iterator
  {
  public:
    Iterator() : mPointer(nullptr) {}
    explicit Iterator(PtrInfoOrBlock* aPointer) : mPointer(aPointer) {}
    Iterator(const Iterator& aOther) : mPointer(aOther.mPointer) {}

    Iterator& operator++()
    {
      if (!mPointer->ptrInfo) {
        
        mPointer = (mPointer + 1)->block->mPointers;
      }
      ++mPointer;
      return *this;
    }

    PtrInfo* operator*() const
    {
      if (!mPointer->ptrInfo) {
        
        return (mPointer + 1)->block->mPointers->ptrInfo;
      }
      return mPointer->ptrInfo;
    }
    bool operator==(const Iterator& aOther) const
    {
      return mPointer == aOther.mPointer;
    }
    bool operator!=(const Iterator& aOther) const
    {
      return mPointer != aOther.mPointer;
    }

#ifdef DEBUG_CC_GRAPH
    bool Initialized() const
    {
      return mPointer != nullptr;
    }
#endif

  private:
    PtrInfoOrBlock* mPointer;
  };

  class Builder;
  friend class Builder;
  class Builder
  {
  public:
    explicit Builder(EdgePool& aPool)
      : mCurrent(&aPool.mSentinelAndBlocks[0])
      , mBlockEnd(&aPool.mSentinelAndBlocks[0])
      , mNextBlockPtr(&aPool.Blocks())
    {
    }

    Iterator Mark()
    {
      return Iterator(mCurrent);
    }

    void Add(PtrInfo* aEdge)
    {
      if (mCurrent == mBlockEnd) {
        Block* b = new Block();
        *mNextBlockPtr = b;
        mCurrent = b->Start();
        mBlockEnd = b->End();
        mNextBlockPtr = &b->Next();
      }
      (mCurrent++)->ptrInfo = aEdge;
    }
  private:
    
    PtrInfoOrBlock* mCurrent;
    PtrInfoOrBlock* mBlockEnd;
    Block** mNextBlockPtr;
  };

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
  {
    size_t n = 0;
    Block* b = Blocks();
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
  void* mPointer;
  nsCycleCollectionParticipant* mParticipant;
  uint32_t mColor : 2;
  uint32_t mInternalRefs : 30;
  uint32_t mRefCount;
private:
  EdgePool::Iterator mFirstChild;

  static const uint32_t kInitialRefCount = UINT32_MAX - 1;

public:

  PtrInfo(void* aPointer, nsCycleCollectionParticipant* aParticipant)
    : mPointer(aPointer),
      mParticipant(aParticipant),
      mColor(grey),
      mInternalRefs(0),
      mRefCount(kInitialRefCount),
      mFirstChild()
  {
    MOZ_ASSERT(aParticipant);

    
    
    
    MOZ_ASSERT(!IsGrayJS() && !IsBlackJS());
  }

  
  PtrInfo()
  {
    NS_NOTREACHED("should never be called");
  }

  bool IsGrayJS() const
  {
    return mRefCount == 0;
  }

  bool IsBlackJS() const
  {
    return mRefCount == UINT32_MAX;
  }

  bool WasTraversed() const
  {
    return mRefCount != kInitialRefCount;
  }

  EdgePool::Iterator FirstChild() const
  {
    CC_GRAPH_ASSERT(mFirstChild.Initialized());
    return mFirstChild;
  }

  
  EdgePool::Iterator LastChild() const
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
  
  
  enum { BlockSize = 8 * 1024 - 2 };

  struct Block
  {
    
    
    
    Block()
    {
      NS_NOTREACHED("should never be called");

      
      static_assert(
        sizeof(Block) == 163824 ||      
        sizeof(Block) == 262120,        
        "ill-sized NodePool::Block"
      );
    }
    ~Block()
    {
      NS_NOTREACHED("should never be called");
    }

    Block* mNext;
    PtrInfo mEntries[BlockSize + 1]; 
  };

public:
  NodePool()
    : mBlocks(nullptr)
    , mLast(nullptr)
  {
  }

  ~NodePool()
  {
    MOZ_ASSERT(!mBlocks, "Didn't call Clear()?");
  }

  void Clear()
  {
    Block* b = mBlocks;
    while (b) {
      Block* n = b->mNext;
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
  class Builder
  {
  public:
    explicit Builder(NodePool& aPool)
      : mNextBlock(&aPool.mBlocks)
      , mNext(aPool.mLast)
      , mBlockEnd(nullptr)
    {
      MOZ_ASSERT(!aPool.mBlocks && !aPool.mLast, "pool not empty");
    }
    PtrInfo* Add(void* aPointer, nsCycleCollectionParticipant* aParticipant)
    {
      if (mNext == mBlockEnd) {
        Block* block = static_cast<Block*>(NS_Alloc(sizeof(Block)));
        *mNextBlock = block;
        mNext = block->mEntries;
        mBlockEnd = block->mEntries + BlockSize;
        block->mNext = nullptr;
        mNextBlock = &block->mNext;
      }
      return new (mNext++) PtrInfo(aPointer, aParticipant);
    }
  private:
    Block** mNextBlock;
    PtrInfo*& mNext;
    PtrInfo* mBlockEnd;
  };

  class Enumerator;
  friend class Enumerator;
  class Enumerator
  {
  public:
    explicit Enumerator(NodePool& aPool)
      : mFirstBlock(aPool.mBlocks)
      , mCurBlock(nullptr)
      , mNext(nullptr)
      , mBlockEnd(nullptr)
      , mLast(aPool.mLast)
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
        Block* nextBlock = mCurBlock ? mCurBlock->mNext : mFirstBlock;
        mNext = nextBlock->mEntries;
        mBlockEnd = mNext + BlockSize;
        mCurBlock = nextBlock;
      }
      return mNext++;
    }
  private:
    
    
    Block*& mFirstBlock;
    Block* mCurBlock;
    
    
    PtrInfo* mNext;
    PtrInfo* mBlockEnd;
    PtrInfo*& mLast;
  };

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
  {
    
    
    size_t n = 0;
    Block* b = mBlocks;
    while (b) {
      n += aMallocSizeOf(b);
      b = b->mNext;
    }
    return n;
  }

private:
  Block* mBlocks;
  PtrInfo* mLast;
};




struct PtrToNodeEntry : public PLDHashEntryHdr
{
  
  PtrInfo* mNode;
};

static bool
PtrToNodeMatchEntry(PLDHashTable* aTable,
                    const PLDHashEntryHdr* aEntry,
                    const void* aKey)
{
  const PtrToNodeEntry* n = static_cast<const PtrToNodeEntry*>(aEntry);
  return n->mNode->mPointer == aKey;
}

static PLDHashTableOps PtrNodeOps = {
  PL_DHashVoidPtrKeyStub,
  PtrToNodeMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  nullptr
};


struct WeakMapping
{
  
  PtrInfo* mMap;
  PtrInfo* mKey;
  PtrInfo* mKeyDelegate;
  PtrInfo* mVal;
};

class CCGraphBuilder;

struct CCGraph
{
  NodePool mNodes;
  EdgePool mEdges;
  nsTArray<WeakMapping> mWeakMaps;
  uint32_t mRootCount;

private:
  PLDHashTable mPtrToNodeMap;
  bool mOutOfMemory;

public:
  CCGraph() : mRootCount(0), mOutOfMemory(false) {}

  ~CCGraph()
  {
    if (mPtrToNodeMap.IsInitialized()) {
      PL_DHashTableFinish(&mPtrToNodeMap);
    }
  }

  void Init()
  {
    MOZ_ASSERT(IsEmpty(), "Failed to call CCGraph::Clear");
    PL_DHashTableInit(&mPtrToNodeMap, &PtrNodeOps,
                      sizeof(PtrToNodeEntry), 16384);
  }

  void Clear()
  {
    mNodes.Clear();
    mEdges.Clear();
    mWeakMaps.Clear();
    mRootCount = 0;
    PL_DHashTableFinish(&mPtrToNodeMap);
    mOutOfMemory = false;
  }

#ifdef DEBUG
  bool IsEmpty()
  {
    return mNodes.IsEmpty() && mEdges.IsEmpty() &&
           mWeakMaps.IsEmpty() && mRootCount == 0 &&
           !mPtrToNodeMap.IsInitialized();
  }
#endif

  PtrInfo* FindNode(void* aPtr);
  PtrToNodeEntry* AddNodeToMap(void* aPtr);
  void RemoveNodeFromMap(void* aPtr);

  uint32_t MapCount() const
  {
    return mPtrToNodeMap.EntryCount();
  }

  void SizeOfExcludingThis(MallocSizeOf aMallocSizeOf,
                           size_t* aNodesSize, size_t* aEdgesSize,
                           size_t* aWeakMapsSize) const
  {
    *aNodesSize = mNodes.SizeOfExcludingThis(aMallocSizeOf);
    *aEdgesSize = mEdges.SizeOfExcludingThis(aMallocSizeOf);

    
    
    *aWeakMapsSize = mWeakMaps.SizeOfExcludingThis(aMallocSizeOf);
  }
};

PtrInfo*
CCGraph::FindNode(void* aPtr)
{
  PtrToNodeEntry* e =
    static_cast<PtrToNodeEntry*>(PL_DHashTableSearch(&mPtrToNodeMap, aPtr));
  return e ? e->mNode : nullptr;
}

PtrToNodeEntry*
CCGraph::AddNodeToMap(void* aPtr)
{
  JS::AutoSuppressGCAnalysis suppress;
  if (mOutOfMemory) {
    return nullptr;
  }

  PtrToNodeEntry* e = static_cast<PtrToNodeEntry*>
    (PL_DHashTableAdd(&mPtrToNodeMap, aPtr, fallible));
  if (!e) {
    mOutOfMemory = true;
    MOZ_ASSERT(false, "Ran out of memory while building cycle collector graph");
    return nullptr;
  }
  return e;
}

void
CCGraph::RemoveNodeFromMap(void* aPtr)
{
  PL_DHashTableRemove(&mPtrToNodeMap, aPtr);
}


static nsISupports*
CanonicalizeXPCOMParticipant(nsISupports* aIn)
{
  nsISupports* out = nullptr;
  aIn->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                      reinterpret_cast<void**>(&out));
  return out;
}

static inline void
ToParticipant(nsISupports* aPtr, nsXPCOMCycleCollectionParticipant** aCp);

static void
CanonicalizeParticipant(void** aParti, nsCycleCollectionParticipant** aCp)
{
  
  

  if (!*aCp) {
    nsISupports* nsparti = static_cast<nsISupports*>(*aParti);
    nsparti = CanonicalizeXPCOMParticipant(nsparti);
    NS_ASSERTION(nsparti,
                 "Don't add objects that don't participate in collection!");
    nsXPCOMCycleCollectionParticipant* xcp;
    ToParticipant(nsparti, &xcp);
    *aParti = nsparti;
    *aCp = xcp;
  }
}

struct nsPurpleBufferEntry
{
  union
  {
    void* mObject;                        
    nsPurpleBufferEntry* mNextInFreeList; 
  };

  nsCycleCollectingAutoRefCnt* mRefCnt;

  nsCycleCollectionParticipant* mParticipant; 
};

class nsCycleCollector;

struct nsPurpleBuffer
{
private:
  struct Block
  {
    Block* mNext;
    
    
    
    
    
    nsPurpleBufferEntry mEntries[1365];

    Block() : mNext(nullptr)
    {
      
      static_assert(
        sizeof(Block) == 16384 ||       
        sizeof(Block) == 32768,         
        "ill-sized nsPurpleBuffer::Block"
      );
    }

    template<class PurpleVisitor>
    void VisitEntries(nsPurpleBuffer& aBuffer, PurpleVisitor& aVisitor)
    {
      nsPurpleBufferEntry* eEnd = ArrayEnd(mEntries);
      for (nsPurpleBufferEntry* e = mEntries; e != eEnd; ++e) {
        MOZ_ASSERT(e->mObject, "There should be no null mObject when we iterate over the purple buffer");
        if (!(uintptr_t(e->mObject) & uintptr_t(1)) && e->mObject) {
          aVisitor.Visit(aBuffer, e);
        }
      }
    }
  };
  
  

  uint32_t mCount;
  Block mFirstBlock;
  nsPurpleBufferEntry* mFreeList;

public:
  nsPurpleBuffer()
  {
    InitBlocks();
  }

  ~nsPurpleBuffer()
  {
    FreeBlocks();
  }

  template<class PurpleVisitor>
  void VisitEntries(PurpleVisitor& aVisitor)
  {
    for (Block* b = &mFirstBlock; b; b = b->mNext) {
      b->VisitEntries(*this, aVisitor);
    }
  }

  void InitBlocks()
  {
    mCount = 0;
    mFreeList = nullptr;
    StartBlock(&mFirstBlock);
  }

  void StartBlock(Block* aBlock)
  {
    MOZ_ASSERT(!mFreeList, "should not have free list");

    
    nsPurpleBufferEntry* entries = aBlock->mEntries;
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
    if (mCount > 0) {
      UnmarkRemainingPurple(&mFirstBlock);
    }
    Block* b = mFirstBlock.mNext;
    while (b) {
      if (mCount > 0) {
        UnmarkRemainingPurple(b);
      }
      Block* next = b->mNext;
      delete b;
      b = next;
    }
    mFirstBlock.mNext = nullptr;
  }

  struct UnmarkRemainingPurpleVisitor
  {
    void
    Visit(nsPurpleBuffer& aBuffer, nsPurpleBufferEntry* aEntry)
    {
      if (aEntry->mRefCnt) {
        aEntry->mRefCnt->RemoveFromPurpleBuffer();
        aEntry->mRefCnt = nullptr;
      }
      aEntry->mObject = nullptr;
      --aBuffer.mCount;
    }
  };

  void UnmarkRemainingPurple(Block* aBlock)
  {
    UnmarkRemainingPurpleVisitor visitor;
    aBlock->VisitEntries(*this, visitor);
  }

  void SelectPointers(CCGraphBuilder& aBuilder);

  
  
  
  
  
  
  
  void RemoveSkippable(nsCycleCollector* aCollector,
                       bool aRemoveChildlessNodes,
                       bool aAsyncSnowWhiteFreeing,
                       CC_ForgetSkippableCallback aCb);

  MOZ_ALWAYS_INLINE nsPurpleBufferEntry* NewEntry()
  {
    if (MOZ_UNLIKELY(!mFreeList)) {
      Block* b = new Block;
      StartBlock(b);

      
      b->mNext = mFirstBlock.mNext;
      mFirstBlock.mNext = b;
    }

    nsPurpleBufferEntry* e = mFreeList;
    mFreeList = (nsPurpleBufferEntry*)
      (uintptr_t(mFreeList->mNextInFreeList) & ~uintptr_t(1));
    return e;
  }

  MOZ_ALWAYS_INLINE void Put(void* aObject, nsCycleCollectionParticipant* aCp,
                             nsCycleCollectingAutoRefCnt* aRefCnt)
  {
    nsPurpleBufferEntry* e = NewEntry();

    ++mCount;

    e->mObject = aObject;
    e->mRefCnt = aRefCnt;
    e->mParticipant = aCp;
  }

  void Remove(nsPurpleBufferEntry* aEntry)
  {
    MOZ_ASSERT(mCount != 0, "must have entries");

    if (aEntry->mRefCnt) {
      aEntry->mRefCnt->RemoveFromPurpleBuffer();
      aEntry->mRefCnt = nullptr;
    }
    aEntry->mNextInFreeList =
      (nsPurpleBufferEntry*)(uintptr_t(mFreeList) | uintptr_t(1));
    mFreeList = aEntry;

    --mCount;
  }

  uint32_t Count() const
  {
    return mCount;
  }

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
  {
    size_t n = 0;

    
    const Block* block = mFirstBlock.mNext;
    while (block) {
      n += aMallocSizeOf(block);
      block = block->mNext;
    }

    
    
    
    
    

    return n;
  }
};

static bool
AddPurpleRoot(CCGraphBuilder& aBuilder, void* aRoot,
              nsCycleCollectionParticipant* aParti);

struct SelectPointersVisitor
{
  explicit SelectPointersVisitor(CCGraphBuilder& aBuilder)
    : mBuilder(aBuilder)
  {
  }

  void
  Visit(nsPurpleBuffer& aBuffer, nsPurpleBufferEntry* aEntry)
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
  CCGraphBuilder& mBuilder;
};

void
nsPurpleBuffer::SelectPointers(CCGraphBuilder& aBuilder)
{
  SelectPointersVisitor visitor(aBuilder);
  VisitEntries(visitor);

  NS_ASSERTION(mCount == 0, "AddPurpleRoot failed");
  if (mCount == 0) {
    FreeBlocks();
    InitBlocks();
  }
}

enum ccPhase
{
  IdlePhase,
  GraphBuildingPhase,
  ScanAndCollectWhitePhase,
  CleanupPhase
};

enum ccType
{
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

  CycleCollectedJSRuntime* mJSRuntime;

  ccPhase mIncrementalPhase;
  CCGraph mGraph;
  nsAutoPtr<CCGraphBuilder> mBuilder;
  nsCOMPtr<nsICycleCollectorListener> mListener;

  DebugOnly<void*> mThread;

  nsCycleCollectorParams mParams;

  uint32_t mWhiteNodeCount;

  CC_BeforeUnlinkCallback mBeforeUnlinkCB;
  CC_ForgetSkippableCallback mForgetSkippableCB;

  nsPurpleBuffer mPurpleBuf;

  uint32_t mUnmergedNeeded;
  uint32_t mMergedInARow;

  nsRefPtr<JSPurpleBuffer> mJSPurpleBuffer;

private:
  virtual ~nsCycleCollector();

public:
  nsCycleCollector();

  void RegisterJSRuntime(CycleCollectedJSRuntime* aJSRuntime);
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

  void Suspect(void* aPtr, nsCycleCollectionParticipant* aCp,
               nsCycleCollectingAutoRefCnt* aRefCnt);
  uint32_t SuspectedCount();
  void ForgetSkippable(bool aRemoveChildlessNodes, bool aAsyncSnowWhiteFreeing);
  bool FreeSnowWhite(bool aUntilNoSWInPurpleBuffer);

  
  void RemoveObjectFromGraph(void* aPtr);

  void PrepareForGarbageCollection();
  void FinishAnyCurrentCollection();

  bool Collect(ccType aCCType,
               SliceBudget& aBudget,
               nsICycleCollectorListener* aManualListener,
               bool aPreferShorterSlices = false);
  void Shutdown();

  void SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                           size_t* aObjectSize,
                           size_t* aGraphNodesSize,
                           size_t* aGraphEdgesSize,
                           size_t* aWeakMapsSize,
                           size_t* aPurpleBufferSize) const;

  JSPurpleBuffer* GetJSPurpleBuffer();
private:
  void CheckThreadSafety();
  void ShutdownCollect();

  void FixGrayBits(bool aForceGC);
  bool ShouldMergeZones(ccType aCCType);

  void BeginCollection(ccType aCCType, nsICycleCollectorListener* aManualListener);
  void MarkRoots(SliceBudget& aBudget);
  void ScanRoots(bool aFullySynchGraphBuild);
  void ScanIncrementalRoots();
  void ScanWhiteNodes(bool aFullySynchGraphBuild);
  void ScanBlackNodes();
  void ScanWeakMaps();

  
  bool CollectWhite();

  void CleanupAfterCollection();
};

NS_IMPL_ISUPPORTS(nsCycleCollector, nsIMemoryReporter)








template<class Visitor>
class GraphWalker
{
private:
  Visitor mVisitor;

  void DoWalk(nsDeque& aQueue);

  void CheckedPush(nsDeque& aQueue, PtrInfo* aPi)
  {
    if (!aPi) {
      MOZ_CRASH();
    }
    if (!aQueue.Push(aPi, fallible)) {
      mVisitor.Failed();
    }
  }

public:
  void Walk(PtrInfo* aPi);
  void WalkFromRoots(CCGraph& aGraph);
  
  
  explicit GraphWalker(const Visitor aVisitor) : mVisitor(aVisitor)
  {
  }
};






struct CollectorData
{
  nsRefPtr<nsCycleCollector> mCollector;
  CycleCollectedJSRuntime* mRuntime;
};

static mozilla::ThreadLocal<CollectorData*> sCollectorData;





MOZ_NEVER_INLINE static void
Fault(const char* aMsg, const void* aPtr = nullptr)
{
  if (aPtr) {
    printf("Fault in cycle collector: %s (ptr: %p)\n", aMsg, aPtr);
  } else {
    printf("Fault in cycle collector: %s\n", aMsg);
  }

  NS_RUNTIMEABORT("cycle collector fault");
}

static void
Fault(const char* aMsg, PtrInfo* aPi)
{
  Fault(aMsg, aPi->mPointer);
}

static inline void
ToParticipant(nsISupports* aPtr, nsXPCOMCycleCollectionParticipant** aCp)
{
  
  
  
  
  *aCp = nullptr;
  CallQueryInterface(aPtr, aCp);
}

template<class Visitor>
MOZ_NEVER_INLINE void
GraphWalker<Visitor>::Walk(PtrInfo* aPi)
{
  nsDeque queue;
  CheckedPush(queue, aPi);
  DoWalk(queue);
}

template<class Visitor>
MOZ_NEVER_INLINE void
GraphWalker<Visitor>::WalkFromRoots(CCGraph& aGraph)
{
  nsDeque queue;
  NodePool::Enumerator etor(aGraph.mNodes);
  for (uint32_t i = 0; i < aGraph.mRootCount; ++i) {
    CheckedPush(queue, etor.GetNext());
  }
  DoWalk(queue);
}

template<class Visitor>
MOZ_NEVER_INLINE void
GraphWalker<Visitor>::DoWalk(nsDeque& aQueue)
{
  
  
  while (aQueue.GetSize() > 0) {
    PtrInfo* pi = static_cast<PtrInfo*>(aQueue.PopFront());

    if (pi->WasTraversed() && mVisitor.ShouldVisitNode(pi)) {
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
    : mAddress("0x"), mCnt(0), mType(eUnknown)
  {
  }

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

class nsCycleCollectorLogSinkToFile final : public nsICycleCollectorLogSink
{
public:
  NS_DECL_ISUPPORTS

  nsCycleCollectorLogSinkToFile() :
    mProcessIdentifier(base::GetCurrentProcId()),
    mGCLog("gc-edges"), mCCLog("cc-edges")
  {
  }

  NS_IMETHOD GetFilenameIdentifier(nsAString& aIdentifier) override
  {
    aIdentifier = mFilenameIdentifier;
    return NS_OK;
  }

  NS_IMETHOD SetFilenameIdentifier(const nsAString& aIdentifier) override
  {
    mFilenameIdentifier = aIdentifier;
    return NS_OK;
  }

  NS_IMETHOD GetProcessIdentifier(int32_t* aIdentifier) override
  {
    *aIdentifier = mProcessIdentifier;
    return NS_OK;
  }

  NS_IMETHOD SetProcessIdentifier(int32_t aIdentifier) override
  {
    mProcessIdentifier = aIdentifier;
    return NS_OK;
  }

  NS_IMETHOD GetGcLog(nsIFile** aPath) override
  {
    NS_IF_ADDREF(*aPath = mGCLog.mFile);
    return NS_OK;
  }

  NS_IMETHOD GetCcLog(nsIFile** aPath) override
  {
    NS_IF_ADDREF(*aPath = mCCLog.mFile);
    return NS_OK;
  }

  NS_IMETHOD Open(FILE** aGCLog, FILE** aCCLog) override
  {
    nsresult rv;

    if (mGCLog.mStream || mCCLog.mStream) {
      return NS_ERROR_UNEXPECTED;
    }

    rv = OpenLog(&mGCLog);
    NS_ENSURE_SUCCESS(rv, rv);
    *aGCLog = mGCLog.mStream;

    rv = OpenLog(&mCCLog);
    NS_ENSURE_SUCCESS(rv, rv);
    *aCCLog = mCCLog.mStream;

    return NS_OK;
  }

  NS_IMETHOD CloseGCLog() override
  {
    if (!mGCLog.mStream) {
      return NS_ERROR_UNEXPECTED;
    }
    CloseLog(&mGCLog, NS_LITERAL_STRING("Garbage"));
    return NS_OK;
  }

  NS_IMETHOD CloseCCLog() override
  {
    if (!mCCLog.mStream) {
      return NS_ERROR_UNEXPECTED;
    }
    CloseLog(&mCCLog, NS_LITERAL_STRING("Cycle"));
    return NS_OK;
  }

private:
  ~nsCycleCollectorLogSinkToFile()
  {
    if (mGCLog.mStream) {
      MozillaUnRegisterDebugFILE(mGCLog.mStream);
      fclose(mGCLog.mStream);
    }
    if (mCCLog.mStream) {
      MozillaUnRegisterDebugFILE(mCCLog.mStream);
      fclose(mCCLog.mStream);
    }
  }

  struct FileInfo
  {
    const char* const mPrefix;
    nsCOMPtr<nsIFile> mFile;
    FILE* mStream;

    explicit FileInfo(const char* aPrefix) : mPrefix(aPrefix), mStream(nullptr) { }
  };

  






  already_AddRefed<nsIFile> CreateTempFile(const char* aPrefix)
  {
    nsPrintfCString filename("%s.%d%s%s.log",
                             aPrefix,
                             mProcessIdentifier,
                             mFilenameIdentifier.IsEmpty() ? "" : ".",
                             NS_ConvertUTF16toUTF8(mFilenameIdentifier).get());

    
    
    
    
    nsIFile* logFile = nullptr;
    if (char* env = PR_GetEnv("MOZ_CC_LOG_DIRECTORY")) {
      NS_NewNativeLocalFile(nsCString(env),  true,
                            &logFile);
    }

    
    
    
    
    nsresult rv = nsDumpUtils::OpenTempFile(filename, &logFile,
                                            NS_LITERAL_CSTRING("memory-reports"));
    if (NS_FAILED(rv)) {
      NS_IF_RELEASE(logFile);
      return nullptr;
    }

    return dont_AddRef(logFile);
  }

  nsresult OpenLog(FileInfo* aLog)
  {
    
    
    
    
    
    nsAutoCString incomplete;
    incomplete += "incomplete-";
    incomplete += aLog->mPrefix;
    MOZ_ASSERT(!aLog->mFile);
    aLog->mFile = CreateTempFile(incomplete.get());
    if (NS_WARN_IF(!aLog->mFile)) {
      return NS_ERROR_UNEXPECTED;
    }

    MOZ_ASSERT(!aLog->mStream);
    aLog->mFile->OpenANSIFileDesc("w", &aLog->mStream);
    if (NS_WARN_IF(!aLog->mStream)) {
      return NS_ERROR_UNEXPECTED;
    }
    MozillaRegisterDebugFILE(aLog->mStream);
    return NS_OK;
  }

  nsresult CloseLog(FileInfo* aLog, const nsAString& aCollectorKind)
  {
    MOZ_ASSERT(aLog->mStream);
    MOZ_ASSERT(aLog->mFile);

    MozillaUnRegisterDebugFILE(aLog->mStream);
    fclose(aLog->mStream);
    aLog->mStream = nullptr;

    
    nsCOMPtr<nsIFile> logFileFinalDestination =
      CreateTempFile(aLog->mPrefix);
    if (NS_WARN_IF(!logFileFinalDestination)) {
      return NS_ERROR_UNEXPECTED;
    }

    nsAutoString logFileFinalDestinationName;
    logFileFinalDestination->GetLeafName(logFileFinalDestinationName);
    if (NS_WARN_IF(logFileFinalDestinationName.IsEmpty())) {
      return NS_ERROR_UNEXPECTED;
    }

    aLog->mFile->MoveTo( nullptr, logFileFinalDestinationName);

    
    aLog->mFile = logFileFinalDestination;

    
    nsCOMPtr<nsIConsoleService> cs =
      do_GetService(NS_CONSOLESERVICE_CONTRACTID);
    if (cs) {
      
      nsAutoString logPath;
      logFileFinalDestination->GetPath(logPath);

      nsString msg = aCollectorKind +
        NS_LITERAL_STRING(" Collector log dumped to ") + logPath;
      cs->LogStringMessage(msg.get());
    }
    return NS_OK;
  }

  int32_t mProcessIdentifier;
  nsString mFilenameIdentifier;
  FileInfo mGCLog;
  FileInfo mCCLog;
};

NS_IMPL_ISUPPORTS(nsCycleCollectorLogSinkToFile, nsICycleCollectorLogSink)


class nsCycleCollectorLogger final : public nsICycleCollectorListener
{
  ~nsCycleCollectorLogger()
  {
    ClearDescribers();
  }

public:
  nsCycleCollectorLogger()
    : mLogSink(nsCycleCollector_createLogSink())
    , mWantAllTraces(false)
    , mDisableLog(false)
    , mWantAfterProcessing(false)
    , mCCLog(nullptr)
  {
  }

  NS_DECL_ISUPPORTS

  void SetAllTraces()
  {
    mWantAllTraces = true;
  }

  NS_IMETHOD AllTraces(nsICycleCollectorListener** aListener) override
  {
    SetAllTraces();
    NS_ADDREF(*aListener = this);
    return NS_OK;
  }

  NS_IMETHOD GetWantAllTraces(bool* aAllTraces) override
  {
    *aAllTraces = mWantAllTraces;
    return NS_OK;
  }

  NS_IMETHOD GetDisableLog(bool* aDisableLog) override
  {
    *aDisableLog = mDisableLog;
    return NS_OK;
  }

  NS_IMETHOD SetDisableLog(bool aDisableLog) override
  {
    mDisableLog = aDisableLog;
    return NS_OK;
  }

  NS_IMETHOD GetWantAfterProcessing(bool* aWantAfterProcessing) override
  {
    *aWantAfterProcessing = mWantAfterProcessing;
    return NS_OK;
  }

  NS_IMETHOD SetWantAfterProcessing(bool aWantAfterProcessing) override
  {
    mWantAfterProcessing = aWantAfterProcessing;
    return NS_OK;
  }

  NS_IMETHOD GetLogSink(nsICycleCollectorLogSink** aLogSink) override
  {
    NS_ADDREF(*aLogSink = mLogSink);
    return NS_OK;
  }

  NS_IMETHOD SetLogSink(nsICycleCollectorLogSink* aLogSink) override
  {
    if (!aLogSink) {
      return NS_ERROR_INVALID_ARG;
    }
    mLogSink = aLogSink;
    return NS_OK;
  }

  NS_IMETHOD Begin() override
  {
    nsresult rv;

    mCurrentAddress.AssignLiteral("0x");
    ClearDescribers();
    if (mDisableLog) {
      return NS_OK;
    }

    FILE* gcLog;
    rv = mLogSink->Open(&gcLog, &mCCLog);
    NS_ENSURE_SUCCESS(rv, rv);
    
    CollectorData* data = sCollectorData.get();
    if (data && data->mRuntime) {
      data->mRuntime->DumpJSHeap(gcLog);
    }
    rv = mLogSink->CloseGCLog();
    NS_ENSURE_SUCCESS(rv, rv);

    fprintf(mCCLog, "# WantAllTraces=%s\n", mWantAllTraces ? "true" : "false");
    return NS_OK;
  }
  NS_IMETHOD NoteRefCountedObject(uint64_t aAddress, uint32_t aRefCount,
                                  const char* aObjectDescription) override
  {
    if (!mDisableLog) {
      fprintf(mCCLog, "%p [rc=%u] %s\n", (void*)aAddress, aRefCount,
              aObjectDescription);
    }
    if (mWantAfterProcessing) {
      CCGraphDescriber* d =  new CCGraphDescriber();
      mDescribers.insertBack(d);
      mCurrentAddress.AssignLiteral("0x");
      mCurrentAddress.AppendInt(aAddress, 16);
      d->mType = CCGraphDescriber::eRefCountedObject;
      d->mAddress = mCurrentAddress;
      d->mCnt = aRefCount;
      d->mName.Append(aObjectDescription);
    }
    return NS_OK;
  }
  NS_IMETHOD NoteGCedObject(uint64_t aAddress, bool aMarked,
                            const char* aObjectDescription,
                            uint64_t aCompartmentAddress) override
  {
    if (!mDisableLog) {
      fprintf(mCCLog, "%p [gc%s] %s\n", (void*)aAddress,
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
  NS_IMETHOD NoteEdge(uint64_t aToAddress, const char* aEdgeName) override
  {
    if (!mDisableLog) {
      fprintf(mCCLog, "> %p %s\n", (void*)aToAddress, aEdgeName);
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
                              uint64_t aKeyDelegate, uint64_t aValue) override
  {
    if (!mDisableLog) {
      fprintf(mCCLog, "WeakMapEntry map=%p key=%p keyDelegate=%p value=%p\n",
              (void*)aMap, (void*)aKey, (void*)aKeyDelegate, (void*)aValue);
    }
    
    return NS_OK;
  }
  NS_IMETHOD NoteIncrementalRoot(uint64_t aAddress) override
  {
    if (!mDisableLog) {
      fprintf(mCCLog, "IncrementalRoot %p\n", (void*)aAddress);
    }
    
    return NS_OK;
  }
  NS_IMETHOD BeginResults() override
  {
    if (!mDisableLog) {
      fputs("==========\n", mCCLog);
    }
    return NS_OK;
  }
  NS_IMETHOD DescribeRoot(uint64_t aAddress, uint32_t aKnownEdges) override
  {
    if (!mDisableLog) {
      fprintf(mCCLog, "%p [known=%u]\n", (void*)aAddress, aKnownEdges);
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
  NS_IMETHOD DescribeGarbage(uint64_t aAddress) override
  {
    if (!mDisableLog) {
      fprintf(mCCLog, "%p [garbage]\n", (void*)aAddress);
    }
    if (mWantAfterProcessing) {
      CCGraphDescriber* d =  new CCGraphDescriber();
      mDescribers.insertBack(d);
      d->mType = CCGraphDescriber::eGarbage;
      d->mAddress.AppendInt(aAddress, 16);
    }
    return NS_OK;
  }
  NS_IMETHOD End() override
  {
    if (!mDisableLog) {
      mCCLog = nullptr;
      nsresult rv = mLogSink->CloseCCLog();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return NS_OK;
  }
  NS_IMETHOD ProcessNext(nsICycleCollectorHandler* aHandler,
                         bool* aCanContinue) override
  {
    if (NS_WARN_IF(!aHandler) || NS_WARN_IF(!mWantAfterProcessing)) {
      return NS_ERROR_UNEXPECTED;
    }
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
  void ClearDescribers()
  {
    CCGraphDescriber* d;
    while ((d = mDescribers.popFirst())) {
      delete d;
    }
  }

  nsCOMPtr<nsICycleCollectorLogSink> mLogSink;
  bool mWantAllTraces;
  bool mDisableLog;
  bool mWantAfterProcessing;
  nsCString mCurrentAddress;
  mozilla::LinkedList<CCGraphDescriber> mDescribers;
  FILE* mCCLog;
};

NS_IMPL_ISUPPORTS(nsCycleCollectorLogger, nsICycleCollectorListener)

nsresult
nsCycleCollectorLoggerConstructor(nsISupports* aOuter,
                                  const nsIID& aIID,
                                  void** aInstancePtr)
{
  if (NS_WARN_IF(aOuter)) {
    return NS_ERROR_NO_AGGREGATION;
  }

  nsISupports* logger = new nsCycleCollectorLogger();

  return logger->QueryInterface(aIID, aInstancePtr);
}

static bool
GCThingIsGrayCCThing(JS::GCCellPtr thing)
{
    return AddToCCKind(thing.kind()) &&
           JS::GCThingIsMarkedGray(thing);
}

static bool
ValueIsGrayCCThing(const JS::Value& value)
{
    return AddToCCKind(value.gcKind()) &&
           JS::GCThingIsMarkedGray(value.toGCCellPtr());
}





class CCGraphBuilder final : public nsCycleCollectionTraversalCallback,
  public nsCycleCollectionNoteRootCallback
{
private:
  CCGraph& mGraph;
  CycleCollectorResults& mResults;
  NodePool::Builder mNodeBuilder;
  EdgePool::Builder mEdgeBuilder;
  PtrInfo* mCurrPi;
  nsCycleCollectionParticipant* mJSParticipant;
  nsCycleCollectionParticipant* mJSZoneParticipant;
  nsCString mNextEdgeName;
  nsCOMPtr<nsICycleCollectorListener> mListener;
  bool mMergeZones;
  nsAutoPtr<NodePool::Enumerator> mCurrNode;

public:
  CCGraphBuilder(CCGraph& aGraph,
                 CycleCollectorResults& aResults,
                 CycleCollectedJSRuntime* aJSRuntime,
                 nsICycleCollectorListener* aListener,
                 bool aMergeZones);
  virtual ~CCGraphBuilder();

  bool WantAllTraces() const
  {
    return nsCycleCollectionNoteRootCallback::WantAllTraces();
  }

  bool AddPurpleRoot(void* aRoot, nsCycleCollectionParticipant* aParti);

  
  void DoneAddingRoots();

  
  bool BuildGraph(SliceBudget& aBudget);

private:
  PtrInfo* AddNode(void* aPtr, nsCycleCollectionParticipant* aParticipant);
  PtrInfo* AddWeakMapNode(JS::GCCellPtr aThing);
  PtrInfo* AddWeakMapNode(JSObject* aObject);

  void SetFirstChild()
  {
    mCurrPi->SetFirstChild(mEdgeBuilder.Mark());
  }

  void SetLastChild()
  {
    mCurrPi->SetLastChild(mEdgeBuilder.Mark());
  }

public:
  
  NS_IMETHOD_(void) NoteXPCOMRoot(nsISupports* aRoot);
  NS_IMETHOD_(void) NoteJSRoot(void* aRoot);
  NS_IMETHOD_(void) NoteNativeRoot(void* aRoot,
                                   nsCycleCollectionParticipant* aParticipant);
  NS_IMETHOD_(void) NoteWeakMapping(JSObject* aMap, JS::GCCellPtr aKey,
                                    JSObject* aKdelegate, JS::GCCellPtr aVal);

  
  NS_IMETHOD_(void) DescribeRefCountedNode(nsrefcnt aRefCount,
                                           const char* aObjName);
  NS_IMETHOD_(void) DescribeGCedNode(bool aIsMarked, const char* aObjName,
                                     uint64_t aCompartmentAddress);

  NS_IMETHOD_(void) NoteXPCOMChild(nsISupports* aChild);
  NS_IMETHOD_(void) NoteJSObject(JSObject* aChild);
  NS_IMETHOD_(void) NoteJSScript(JSScript* aChild);
  NS_IMETHOD_(void) NoteNativeChild(void* aChild,
                                    nsCycleCollectionParticipant* aParticipant);
  NS_IMETHOD_(void) NoteNextEdgeName(const char* aName);

private:
  void NoteJSChild(JS::GCCellPtr aChild);

  NS_IMETHOD_(void) NoteRoot(void* aRoot,
                             nsCycleCollectionParticipant* aParticipant)
  {
    MOZ_ASSERT(aRoot);
    MOZ_ASSERT(aParticipant);

    if (!aParticipant->CanSkipInCC(aRoot) || MOZ_UNLIKELY(WantAllTraces())) {
      AddNode(aRoot, aParticipant);
    }
  }

  NS_IMETHOD_(void) NoteChild(void* aChild, nsCycleCollectionParticipant* aCp,
                              nsCString& aEdgeName)
  {
    PtrInfo* childPi = AddNode(aChild, aCp);
    if (!childPi) {
      return;
    }
    mEdgeBuilder.Add(childPi);
    if (mListener) {
      mListener->NoteEdge((uint64_t)aChild, aEdgeName.get());
    }
    ++childPi->mInternalRefs;
  }

  JS::Zone* MergeZone(void* aGcthing)
  {
    if (!mMergeZones) {
      return nullptr;
    }
    JS::Zone* zone = JS::GetTenuredGCThingZone(aGcthing);
    if (js::IsSystemZone(zone)) {
      return nullptr;
    }
    return zone;
  }
};

CCGraphBuilder::CCGraphBuilder(CCGraph& aGraph,
                               CycleCollectorResults& aResults,
                               CycleCollectedJSRuntime* aJSRuntime,
                               nsICycleCollectorListener* aListener,
                               bool aMergeZones)
  : mGraph(aGraph)
  , mResults(aResults)
  , mNodeBuilder(aGraph.mNodes)
  , mEdgeBuilder(aGraph.mEdges)
  , mJSParticipant(nullptr)
  , mJSZoneParticipant(nullptr)
  , mListener(aListener)
  , mMergeZones(aMergeZones)
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

CCGraphBuilder::~CCGraphBuilder()
{
}

PtrInfo*
CCGraphBuilder::AddNode(void* aPtr, nsCycleCollectionParticipant* aParticipant)
{
  PtrToNodeEntry* e = mGraph.AddNodeToMap(aPtr);
  if (!e) {
    return nullptr;
  }

  PtrInfo* result;
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

bool
CCGraphBuilder::AddPurpleRoot(void* aRoot, nsCycleCollectionParticipant* aParti)
{
  CanonicalizeParticipant(&aRoot, &aParti);

  if (WantAllTraces() || !aParti->CanSkipInCC(aRoot)) {
    PtrInfo* pinfo = AddNode(aRoot, aParti);
    if (!pinfo) {
      return false;
    }
  }

  return true;
}

void
CCGraphBuilder::DoneAddingRoots()
{
  
  mGraph.mRootCount = mGraph.MapCount();

  mCurrNode = new NodePool::Enumerator(mGraph.mNodes);
}

MOZ_NEVER_INLINE bool
CCGraphBuilder::BuildGraph(SliceBudget& aBudget)
{
  const intptr_t kNumNodesBetweenTimeChecks = 1000;
  const intptr_t kStep = SliceBudget::CounterReset / kNumNodesBetweenTimeChecks;

  MOZ_ASSERT(mCurrNode);

  while (!aBudget.isOverBudget() && !mCurrNode->IsDone()) {
    PtrInfo* pi = mCurrNode->GetNext();
    if (!pi) {
      MOZ_CRASH();
    }

    mCurrPi = pi;

    
    
    SetFirstChild();

    if (pi->mParticipant) {
      nsresult rv = pi->mParticipant->Traverse(pi->mPointer, *this);
      if (NS_FAILED(rv)) {
        Fault("script pointer traversal failed", pi);
      }
    }

    if (mCurrNode->AtBlockEnd()) {
      SetLastChild();
    }

    aBudget.step(kStep);
  }

  if (!mCurrNode->IsDone()) {
    return false;
  }

  if (mGraph.mRootCount > 0) {
    SetLastChild();
  }

  mCurrNode = nullptr;

  return true;
}

NS_IMETHODIMP_(void)
CCGraphBuilder::NoteXPCOMRoot(nsISupports* aRoot)
{
  aRoot = CanonicalizeXPCOMParticipant(aRoot);
  NS_ASSERTION(aRoot,
               "Don't add objects that don't participate in collection!");

  nsXPCOMCycleCollectionParticipant* cp;
  ToParticipant(aRoot, &cp);

  NoteRoot(aRoot, cp);
}

NS_IMETHODIMP_(void)
CCGraphBuilder::NoteJSRoot(void* aRoot)
{
  if (JS::Zone* zone = MergeZone(aRoot)) {
    NoteRoot(zone, mJSZoneParticipant);
  } else {
    NoteRoot(aRoot, mJSParticipant);
  }
}

NS_IMETHODIMP_(void)
CCGraphBuilder::NoteNativeRoot(void* aRoot,
                               nsCycleCollectionParticipant* aParticipant)
{
  NoteRoot(aRoot, aParticipant);
}

NS_IMETHODIMP_(void)
CCGraphBuilder::DescribeRefCountedNode(nsrefcnt aRefCount, const char* aObjName)
{
  if (aRefCount == 0) {
    Fault("zero refcount", mCurrPi);
  }
  if (aRefCount == UINT32_MAX) {
    Fault("overflowing refcount", mCurrPi);
  }
  mResults.mVisitedRefCounted++;

  if (mListener) {
    mListener->NoteRefCountedObject((uint64_t)mCurrPi->mPointer, aRefCount,
                                    aObjName);
  }

  mCurrPi->mRefCount = aRefCount;
}

NS_IMETHODIMP_(void)
CCGraphBuilder::DescribeGCedNode(bool aIsMarked, const char* aObjName,
                                 uint64_t aCompartmentAddress)
{
  uint32_t refCount = aIsMarked ? UINT32_MAX : 0;
  mResults.mVisitedGCed++;

  if (mListener) {
    mListener->NoteGCedObject((uint64_t)mCurrPi->mPointer, aIsMarked,
                              aObjName, aCompartmentAddress);
  }

  mCurrPi->mRefCount = refCount;
}

NS_IMETHODIMP_(void)
CCGraphBuilder::NoteXPCOMChild(nsISupports* aChild)
{
  nsCString edgeName;
  if (WantDebugInfo()) {
    edgeName.Assign(mNextEdgeName);
    mNextEdgeName.Truncate();
  }
  if (!aChild || !(aChild = CanonicalizeXPCOMParticipant(aChild))) {
    return;
  }

  nsXPCOMCycleCollectionParticipant* cp;
  ToParticipant(aChild, &cp);
  if (cp && (!cp->CanSkipThis(aChild) || WantAllTraces())) {
    NoteChild(aChild, cp, edgeName);
  }
}

NS_IMETHODIMP_(void)
CCGraphBuilder::NoteNativeChild(void* aChild,
                                nsCycleCollectionParticipant* aParticipant)
{
  nsCString edgeName;
  if (WantDebugInfo()) {
    edgeName.Assign(mNextEdgeName);
    mNextEdgeName.Truncate();
  }
  if (!aChild) {
    return;
  }

  MOZ_ASSERT(aParticipant, "Need a nsCycleCollectionParticipant!");
  if (!aParticipant->CanSkipThis(aChild) || WantAllTraces()) {
    NoteChild(aChild, aParticipant, edgeName);
  }
}

NS_IMETHODIMP_(void)
CCGraphBuilder::NoteJSObject(JSObject* aChild)
{
  return NoteJSChild(JS::GCCellPtr(aChild));
}

NS_IMETHODIMP_(void)
CCGraphBuilder::NoteJSScript(JSScript* aChild)
{
  return NoteJSChild(JS::GCCellPtr(aChild));
}

void
CCGraphBuilder::NoteJSChild(JS::GCCellPtr aChild)
{
  if (!aChild) {
    return;
  }

  nsCString edgeName;
  if (MOZ_UNLIKELY(WantDebugInfo())) {
    edgeName.Assign(mNextEdgeName);
    mNextEdgeName.Truncate();
  }

  if (GCThingIsGrayCCThing(aChild) || MOZ_UNLIKELY(WantAllTraces())) {
    if (JS::Zone* zone = MergeZone(aChild.asCell())) {
      NoteChild(zone, mJSZoneParticipant, edgeName);
    } else {
      NoteChild(aChild.asCell(), mJSParticipant, edgeName);
    }
  }
}

NS_IMETHODIMP_(void)
CCGraphBuilder::NoteNextEdgeName(const char* aName)
{
  if (WantDebugInfo()) {
    mNextEdgeName = aName;
  }
}

PtrInfo*
CCGraphBuilder::AddWeakMapNode(JS::GCCellPtr aNode)
{
  MOZ_ASSERT(aNode, "Weak map node should be non-null.");

  if (!GCThingIsGrayCCThing(aNode) && !WantAllTraces()) {
    return nullptr;
  }

  if (JS::Zone* zone = MergeZone(aNode.asCell())) {
    return AddNode(zone, mJSZoneParticipant);
  }
  return AddNode(aNode.asCell(), mJSParticipant);
}

PtrInfo*
CCGraphBuilder::AddWeakMapNode(JSObject* aObject)
{
  return AddWeakMapNode(JS::GCCellPtr(aObject));
}

NS_IMETHODIMP_(void)
CCGraphBuilder::NoteWeakMapping(JSObject* aMap, JS::GCCellPtr aKey,
                                JSObject* aKdelegate, JS::GCCellPtr aVal)
{
  
  
  WeakMapping* mapping = mGraph.mWeakMaps.AppendElement();
  mapping->mMap = aMap ? AddWeakMapNode(aMap) : nullptr;
  mapping->mKey = aKey ? AddWeakMapNode(aKey) : nullptr;
  mapping->mKeyDelegate = aKdelegate ? AddWeakMapNode(aKdelegate) : mapping->mKey;
  mapping->mVal = aVal ? AddWeakMapNode(aVal) : nullptr;

  if (mListener) {
    mListener->NoteWeakMapEntry((uint64_t)aMap, aKey.unsafeAsInteger(),
                                (uint64_t)aKdelegate, aVal.unsafeAsInteger());
  }
}

static bool
AddPurpleRoot(CCGraphBuilder& aBuilder, void* aRoot,
              nsCycleCollectionParticipant* aParti)
{
  return aBuilder.AddPurpleRoot(aRoot, aParti);
}



class ChildFinder : public nsCycleCollectionTraversalCallback
{
public:
  ChildFinder() : mMayHaveChild(false)
  {
  }

  
  
  NS_IMETHOD_(void) NoteXPCOMChild(nsISupports* aChild);
  NS_IMETHOD_(void) NoteNativeChild(void* aChild,
                                    nsCycleCollectionParticipant* aHelper);
  NS_IMETHOD_(void) NoteJSObject(JSObject* aChild);
  NS_IMETHOD_(void) NoteJSScript(JSScript* aChild);

  NS_IMETHOD_(void) DescribeRefCountedNode(nsrefcnt aRefcount,
                                           const char* aObjname)
  {
  }
  NS_IMETHOD_(void) DescribeGCedNode(bool aIsMarked,
                                     const char* aObjname,
                                     uint64_t aCompartmentAddress)
  {
  }
  NS_IMETHOD_(void) NoteNextEdgeName(const char* aName)
  {
  }
  bool MayHaveChild()
  {
    return mMayHaveChild;
  }
private:
  bool mMayHaveChild;
};

NS_IMETHODIMP_(void)
ChildFinder::NoteXPCOMChild(nsISupports* aChild)
{
  if (!aChild || !(aChild = CanonicalizeXPCOMParticipant(aChild))) {
    return;
  }
  nsXPCOMCycleCollectionParticipant* cp;
  ToParticipant(aChild, &cp);
  if (cp && !cp->CanSkip(aChild, true)) {
    mMayHaveChild = true;
  }
}

NS_IMETHODIMP_(void)
ChildFinder::NoteNativeChild(void* aChild,
                             nsCycleCollectionParticipant* aHelper)
{
  if (!aChild) {
    return;
  }
  MOZ_ASSERT(aHelper, "Native child must have a participant");
  if (!aHelper->CanSkip(aChild, true)) {
    mMayHaveChild = true;
  }
}

NS_IMETHODIMP_(void)
ChildFinder::NoteJSObject(JSObject* aChild)
{
  if (aChild && JS::ObjectIsMarkedGray(aChild)) {
    mMayHaveChild = true;
  }
}

NS_IMETHODIMP_(void)
ChildFinder::NoteJSScript(JSScript* aChild)
{
  if (aChild && JS::ScriptIsMarkedGray(aChild)) {
    mMayHaveChild = true;
  }
}

static bool
MayHaveChild(void* aObj, nsCycleCollectionParticipant* aCp)
{
  ChildFinder cf;
  aCp->Traverse(aObj, cf);
  return cf.MayHaveChild();
}






class JSPurpleBuffer
{
  ~JSPurpleBuffer()
  {
    MOZ_ASSERT(mValues.IsEmpty());
    MOZ_ASSERT(mObjects.IsEmpty());
  }

public:
  explicit JSPurpleBuffer(nsRefPtr<JSPurpleBuffer>& aReferenceToThis)
    : mReferenceToThis(aReferenceToThis)
    , mValues(kSegmentSize)
    , mObjects(kSegmentSize)
  {
    mReferenceToThis = this;
    mozilla::HoldJSObjects(this);
  }

  void Destroy()
  {
    mReferenceToThis = nullptr;
    mValues.Clear();
    mObjects.Clear();
    mozilla::DropJSObjects(this);
  }

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(JSPurpleBuffer)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(JSPurpleBuffer)

  nsRefPtr<JSPurpleBuffer>& mReferenceToThis;

  
  
  
  
  static const size_t kSegmentSize = 512;
  SegmentedVector<JS::Value, kSegmentSize, InfallibleAllocPolicy> mValues;
  SegmentedVector<JSObject*, kSegmentSize, InfallibleAllocPolicy> mObjects;
};

NS_IMPL_CYCLE_COLLECTION_CLASS(JSPurpleBuffer)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(JSPurpleBuffer)
  tmp->Destroy();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(JSPurpleBuffer)
  CycleCollectionNoteChild(cb, tmp, "self");
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

#define NS_TRACE_SEGMENTED_ARRAY(_field, _type)                               \
  {                                                                           \
    for (auto iter = tmp->_field.Iter(); !iter.Done(); iter.Next()) {         \
      js::gc::CallTraceCallbackOnNonHeap<_type, TraceCallbacks>(              \
          &iter.Get(), aCallbacks, #_field, aClosure);                        \
    }                                                                         \
  }

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(JSPurpleBuffer)
  NS_TRACE_SEGMENTED_ARRAY(mValues, JS::Value)
  NS_TRACE_SEGMENTED_ARRAY(mObjects, JSObject*)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(JSPurpleBuffer, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(JSPurpleBuffer, Release)

class SnowWhiteKiller : public TraceCallbacks
{
  struct SnowWhiteObject
  {
    void* mPointer;
    nsCycleCollectionParticipant* mParticipant;
    nsCycleCollectingAutoRefCnt* mRefCnt;
  };

  
  static const size_t kSegmentSize = sizeof(void*) * 1024;
  typedef SegmentedVector<SnowWhiteObject, kSegmentSize, InfallibleAllocPolicy>
    ObjectsVector;

public:
  SnowWhiteKiller(nsCycleCollector* aCollector, uint32_t aMaxCount)
    : mCollector(aCollector)
    , mObjects(kSegmentSize)
  {
    MOZ_ASSERT(mCollector, "Calling SnowWhiteKiller after nsCC went away");
  }

  ~SnowWhiteKiller()
  {
    for (auto iter = mObjects.Iter(); !iter.Done(); iter.Next()) {
      SnowWhiteObject& o = iter.Get();
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
      void* o = aEntry->mObject;
      nsCycleCollectionParticipant* cp = aEntry->mParticipant;
      CanonicalizeParticipant(&o, &cp);
      SnowWhiteObject swo = { o, cp, aEntry->mRefCnt };
      mObjects.InfallibleAppend(swo);
      aBuffer.Remove(aEntry);
    }
  }

  bool HasSnowWhiteObjects() const
  {
    return !mObjects.IsEmpty();
  }

  virtual void Trace(JS::Heap<JS::Value>* aValue, const char* aName,
                     void* aClosure) const
  {
    if (aValue->isMarkable() && ValueIsGrayCCThing(*aValue)) {
      MOZ_ASSERT(!js::gc::IsInsideNursery(aValue->toGCThing()));
      mCollector->GetJSPurpleBuffer()->mValues.InfallibleAppend(*aValue);
    }
  }

  virtual void Trace(JS::Heap<jsid>* aId, const char* aName,
                     void* aClosure) const
  {
  }

  void AppendJSObjectToPurpleBuffer(JSObject* obj) const
  {
    if (obj && JS::ObjectIsMarkedGray(obj)) {
      MOZ_ASSERT(JS::ObjectIsTenured(obj));
      mCollector->GetJSPurpleBuffer()->mObjects.InfallibleAppend(obj);
    }
  }

  virtual void Trace(JS::Heap<JSObject*>* aObject, const char* aName,
                     void* aClosure) const
  {
    AppendJSObjectToPurpleBuffer(*aObject);
  }

  virtual void Trace(JS::TenuredHeap<JSObject*>* aObject, const char* aName,
                     void* aClosure) const
  {
    AppendJSObjectToPurpleBuffer(*aObject);
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
  nsRefPtr<nsCycleCollector> mCollector;
  ObjectsVector mObjects;
};

class RemoveSkippableVisitor : public SnowWhiteKiller
{
public:
  RemoveSkippableVisitor(nsCycleCollector* aCollector,
                         uint32_t aMaxCount, bool aRemoveChildlessNodes,
                         bool aAsyncSnowWhiteFreeing,
                         CC_ForgetSkippableCallback aCb)
    : SnowWhiteKiller(aCollector, aAsyncSnowWhiteFreeing ? 0 : aMaxCount)
    , mRemoveChildlessNodes(aRemoveChildlessNodes)
    , mAsyncSnowWhiteFreeing(aAsyncSnowWhiteFreeing)
    , mDispatchedDeferredDeletion(false)
    , mCallback(aCb)
  {
  }

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
  Visit(nsPurpleBuffer& aBuffer, nsPurpleBufferEntry* aEntry)
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
    void* o = aEntry->mObject;
    nsCycleCollectionParticipant* cp = aEntry->mParticipant;
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
  MOZ_ASSERT(!mScanInProgress,
             "Don't forget skippable or free snow-white while scan is in progress.");
  mPurpleBuf.RemoveSkippable(this, aRemoveChildlessNodes,
                             aAsyncSnowWhiteFreeing, mForgetSkippableCB);
}

MOZ_NEVER_INLINE void
nsCycleCollector::MarkRoots(SliceBudget& aBudget)
{
  TimeLog timeLog;
  AutoRestore<bool> ar(mScanInProgress);
  MOZ_ASSERT(!mScanInProgress);
  mScanInProgress = true;
  MOZ_ASSERT(mIncrementalPhase == GraphBuildingPhase);

  bool doneBuilding = mBuilder->BuildGraph(aBudget);

  if (!doneBuilding) {
    timeLog.Checkpoint("MarkRoots()");
    return;
  }

  mBuilder = nullptr;
  mIncrementalPhase = ScanAndCollectWhitePhase;
  timeLog.Checkpoint("MarkRoots()");
}







struct ScanBlackVisitor
{
  ScanBlackVisitor(uint32_t& aWhiteNodeCount, bool& aFailed)
    : mWhiteNodeCount(aWhiteNodeCount), mFailed(aFailed)
  {
  }

  bool ShouldVisitNode(PtrInfo const* aPi)
  {
    return aPi->mColor != black;
  }

  MOZ_NEVER_INLINE void VisitNode(PtrInfo* aPi)
  {
    if (aPi->mColor == white) {
      --mWhiteNodeCount;
    }
    aPi->mColor = black;
  }

  void Failed()
  {
    mFailed = true;
  }

private:
  uint32_t& mWhiteNodeCount;
  bool& mFailed;
};

static void
FloodBlackNode(uint32_t& aWhiteNodeCount, bool& aFailed, PtrInfo* aPi)
{
  GraphWalker<ScanBlackVisitor>(ScanBlackVisitor(aWhiteNodeCount,
                                                 aFailed)).Walk(aPi);
  MOZ_ASSERT(aPi->mColor == black || !aPi->WasTraversed(),
             "FloodBlackNode should make aPi black");
}



void
nsCycleCollector::ScanWeakMaps()
{
  bool anyChanged;
  bool failed = false;
  do {
    anyChanged = false;
    for (uint32_t i = 0; i < mGraph.mWeakMaps.Length(); i++) {
      WeakMapping* wm = &mGraph.mWeakMaps[i];

      
      uint32_t mColor = wm->mMap ? wm->mMap->mColor : black;
      uint32_t kColor = wm->mKey ? wm->mKey->mColor : black;
      uint32_t kdColor = wm->mKeyDelegate ? wm->mKeyDelegate->mColor : black;
      uint32_t vColor = wm->mVal ? wm->mVal->mColor : black;

      MOZ_ASSERT(mColor != grey, "Uncolored weak map");
      MOZ_ASSERT(kColor != grey, "Uncolored weak map key");
      MOZ_ASSERT(kdColor != grey, "Uncolored weak map key delegate");
      MOZ_ASSERT(vColor != grey, "Uncolored weak map value");

      if (mColor == black && kColor != black && kdColor == black) {
        FloodBlackNode(mWhiteNodeCount, failed, wm->mKey);
        anyChanged = true;
      }

      if (mColor == black && kColor == black && vColor != black) {
        FloodBlackNode(mWhiteNodeCount, failed, wm->mVal);
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
  PurpleScanBlackVisitor(CCGraph& aGraph, nsICycleCollectorListener* aListener,
                         uint32_t& aCount, bool& aFailed)
    : mGraph(aGraph), mListener(aListener), mCount(aCount), mFailed(aFailed)
  {
  }

  void
  Visit(nsPurpleBuffer& aBuffer, nsPurpleBufferEntry* aEntry)
  {
    MOZ_ASSERT(aEntry->mObject,
               "Entries with null mObject shouldn't be in the purple buffer.");
    MOZ_ASSERT(aEntry->mRefCnt->get() != 0,
               "Snow-white objects shouldn't be in the purple buffer.");

    void* obj = aEntry->mObject;
    if (!aEntry->mParticipant) {
      obj = CanonicalizeXPCOMParticipant(static_cast<nsISupports*>(obj));
      MOZ_ASSERT(obj, "Don't add objects that don't participate in collection!");
    }

    PtrInfo* pi = mGraph.FindNode(obj);
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
    FloodBlackNode(mCount, mFailed, pi);
  }

private:
  CCGraph& mGraph;
  nsCOMPtr<nsICycleCollectorListener> mListener;
  uint32_t& mCount;
  bool& mFailed;
};




void
nsCycleCollector::ScanIncrementalRoots()
{
  TimeLog timeLog;

  
  
  
  
  
  
  
  
  
  bool failed = false;
  PurpleScanBlackVisitor purpleScanBlackVisitor(mGraph, mListener,
                                                mWhiteNodeCount, failed);
  mPurpleBuf.VisitEntries(purpleScanBlackVisitor);
  timeLog.Checkpoint("ScanIncrementalRoots::fix purple");

  bool hasJSRuntime = !!mJSRuntime;
  nsCycleCollectionParticipant* jsParticipant =
    hasJSRuntime ? mJSRuntime->GCThingParticipant() : nullptr;
  nsCycleCollectionParticipant* zoneParticipant =
    hasJSRuntime ? mJSRuntime->ZoneParticipant() : nullptr;
  bool hasListener = !!mListener;

  NodePool::Enumerator etor(mGraph.mNodes);
  while (!etor.IsDone()) {
    PtrInfo* pi = etor.GetNext();

    
    
    
    if (pi->mColor == black && MOZ_LIKELY(!hasListener)) {
      continue;
    }

    
    
    
    
    
    if (pi->IsGrayJS() && MOZ_LIKELY(hasJSRuntime)) {
      
      
      if (pi->mParticipant == jsParticipant) {
        JS::GCCellPtr ptr(pi->mPointer, js::GCThingTraceKind(pi->mPointer));
        if (GCThingIsGrayCCThing(ptr)) {
          continue;
        }
      } else if (pi->mParticipant == zoneParticipant) {
        JS::Zone* zone = static_cast<JS::Zone*>(pi->mPointer);
        if (js::ZoneGlobalsAreAllGray(zone)) {
          continue;
        }
      } else {
        MOZ_ASSERT(false, "Non-JS thing with 0 refcount? Treating as live.");
      }
    } else if (!pi->mParticipant && pi->WasTraversed()) {
      
      
      
      
      
      
      
      
      
      
    } else {
      continue;
    }

    

    
    
    
    if (MOZ_UNLIKELY(hasListener) && pi->mPointer) {
      
      mListener->NoteIncrementalRoot((uint64_t)pi->mPointer);
    }

    FloodBlackNode(mWhiteNodeCount, failed, pi);
  }

  timeLog.Checkpoint("ScanIncrementalRoots::fix nodes");

  if (failed) {
    NS_ASSERTION(false, "Ran out of memory in ScanIncrementalRoots");
    CC_TELEMETRY(_OOM, true);
  }
}




void
nsCycleCollector::ScanWhiteNodes(bool aFullySynchGraphBuild)
{
  NodePool::Enumerator nodeEnum(mGraph.mNodes);
  while (!nodeEnum.IsDone()) {
    PtrInfo* pi = nodeEnum.GetNext();
    if (pi->mColor == black) {
      
      
      
      MOZ_ASSERT(!aFullySynchGraphBuild,
                 "In a synch CC, no nodes should be marked black early on.");
      continue;
    }
    MOZ_ASSERT(pi->mColor == grey);

    if (!pi->WasTraversed()) {
      
      
      MOZ_ASSERT(!pi->mParticipant, "Live nodes should all have been traversed");
      continue;
    }

    if (pi->mInternalRefs == pi->mRefCount || pi->IsGrayJS()) {
      pi->mColor = white;
      ++mWhiteNodeCount;
      continue;
    }

    if (MOZ_LIKELY(pi->mInternalRefs < pi->mRefCount)) {
      
      continue;
    }

    Fault("Traversed refs exceed refcount", pi);
  }
}





void
nsCycleCollector::ScanBlackNodes()
{
  bool failed = false;
  NodePool::Enumerator nodeEnum(mGraph.mNodes);
  while (!nodeEnum.IsDone()) {
    PtrInfo* pi = nodeEnum.GetNext();
    if (pi->mColor == grey && pi->WasTraversed()) {
      FloodBlackNode(mWhiteNodeCount, failed, pi);
    }
  }

  if (failed) {
    NS_ASSERTION(false, "Ran out of memory in ScanBlackNodes");
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
  ScanWhiteNodes(aFullySynchGraphBuild);
  timeLog.Checkpoint("ScanRoots::ScanWhiteNodes");

  ScanBlackNodes();
  timeLog.Checkpoint("ScanRoots::ScanBlackNodes");

  
  ScanWeakMaps();
  timeLog.Checkpoint("ScanRoots::ScanWeakMaps");

  if (mListener) {
    mListener->BeginResults();

    NodePool::Enumerator etor(mGraph.mNodes);
    while (!etor.IsDone()) {
      PtrInfo* pi = etor.GetNext();
      if (!pi->WasTraversed()) {
        continue;
      }
      switch (pi->mColor) {
        case black:
          if (!pi->IsGrayJS() && !pi->IsBlackJS() &&
              pi->mInternalRefs != pi->mRefCount) {
            mListener->DescribeRoot((uint64_t)pi->mPointer,
                                    pi->mInternalRefs);
          }
          break;
        case white:
          mListener->DescribeGarbage((uint64_t)pi->mPointer);
          break;
        case grey:
          MOZ_ASSERT(false, "All traversed objects should be black or white");
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
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  static const size_t kSegmentSize = sizeof(void*) * 1024;
  SegmentedVector<PtrInfo*, kSegmentSize, InfallibleAllocPolicy>
    whiteNodes(kSegmentSize);
  TimeLog timeLog;

  MOZ_ASSERT(mIncrementalPhase == ScanAndCollectWhitePhase);

  uint32_t numWhiteNodes = 0;
  uint32_t numWhiteGCed = 0;
  uint32_t numWhiteJSZones = 0;

  bool hasJSRuntime = !!mJSRuntime;
  nsCycleCollectionParticipant* zoneParticipant =
    hasJSRuntime ? mJSRuntime->ZoneParticipant() : nullptr;

  NodePool::Enumerator etor(mGraph.mNodes);
  while (!etor.IsDone()) {
    PtrInfo* pinfo = etor.GetNext();
    if (pinfo->mColor == white && pinfo->mParticipant) {
      if (pinfo->IsGrayJS()) {
        ++numWhiteGCed;
        if (MOZ_UNLIKELY(pinfo->mParticipant == zoneParticipant)) {
          ++numWhiteJSZones;
        }
      } else {
        whiteNodes.InfallibleAppend(pinfo);
        pinfo->mParticipant->Root(pinfo->mPointer);
        ++numWhiteNodes;
      }
    }
  }

  mResults.mFreedRefCounted += numWhiteNodes;
  mResults.mFreedGCed += numWhiteGCed;
  mResults.mFreedJSZones += numWhiteJSZones;

  timeLog.Checkpoint("CollectWhite::Root");

  if (mBeforeUnlinkCB) {
    mBeforeUnlinkCB();
    timeLog.Checkpoint("CollectWhite::BeforeUnlinkCB");
  }

  
  

  for (auto iter = whiteNodes.Iter(); !iter.Done(); iter.Next()) {
    PtrInfo* pinfo = iter.Get();
    MOZ_ASSERT(pinfo->mParticipant,
               "Unlink shouldn't see objects removed from graph.");
    pinfo->mParticipant->Unlink(pinfo->mPointer);
#ifdef DEBUG
    if (mJSRuntime) {
      mJSRuntime->AssertNoObjectsToTrace(pinfo->mPointer);
    }
#endif
  }
  timeLog.Checkpoint("CollectWhite::Unlink");

  for (auto iter = whiteNodes.Iter(); !iter.Done(); iter.Next()) {
    PtrInfo* pinfo = iter.Get();
    MOZ_ASSERT(pinfo->mParticipant,
               "Unroot shouldn't see objects removed from graph.");
    pinfo->mParticipant->Unroot(pinfo->mPointer);
  }
  timeLog.Checkpoint("CollectWhite::Unroot");

  nsCycleCollector_dispatchDeferredDeletion(false);
  timeLog.Checkpoint("CollectWhite::dispatchDeferredDeletion");

  mIncrementalPhase = CleanupPhase;

  return numWhiteNodes > 0 || numWhiteGCed > 0 || numWhiteJSZones > 0;
}






MOZ_DEFINE_MALLOC_SIZE_OF(CycleCollectorMallocSizeOf)

NS_IMETHODIMP
nsCycleCollector::CollectReports(nsIHandleReportCallback* aHandleReport,
                                 nsISupports* aData, bool aAnonymize)
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
  mMergedInARow(0)
{
}

nsCycleCollector::~nsCycleCollector()
{
  UnregisterWeakMemoryReporter(this);
}

void
nsCycleCollector::RegisterJSRuntime(CycleCollectedJSRuntime* aJSRuntime)
{
  if (mJSRuntime) {
    Fault("multiple registrations of cycle collector JS runtime", aJSRuntime);
  }

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
  if (!mJSRuntime) {
    Fault("forgetting non-registered cycle collector JS runtime");
  }

  mJSRuntime = nullptr;
}

#ifdef DEBUG
static bool
HasParticipant(void* aPtr, nsCycleCollectionParticipant* aParti)
{
  if (aParti) {
    return true;
  }

  nsXPCOMCycleCollectionParticipant* xcp;
  ToParticipant(static_cast<nsISupports*>(aPtr), &xcp);
  return xcp != nullptr;
}
#endif

MOZ_ALWAYS_INLINE void
nsCycleCollector::Suspect(void* aPtr, nsCycleCollectionParticipant* aParti,
                          nsCycleCollectingAutoRefCnt* aRefCnt)
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

  if (!mJSRuntime) {
    return;
  }

  if (!aForceGC) {
    mJSRuntime->FixWeakMappingGrayBits();

    bool needGC = !mJSRuntime->AreGCGrayBitsValid();
    
    CC_TELEMETRY(_NEED_GC, needGC);
    if (!needGC) {
      return;
    }
    mResults.mForcedGC = true;
  }

  TimeLog timeLog;
  mJSRuntime->GarbageCollect(aForceGC ? JS::gcreason::SHUTDOWN_CC :
                                        JS::gcreason::CC_FORCED);
  timeLog.Checkpoint("GC()");
}

void
nsCycleCollector::CleanupAfterCollection()
{
  TimeLog timeLog;
  MOZ_ASSERT(mIncrementalPhase == CleanupPhase);
  mGraph.Clear();
  timeLog.Checkpoint("CleanupAfterCollection::mGraph.Clear()");

  uint32_t interval =
    (uint32_t)((TimeStamp::Now() - mCollectionStart).ToMilliseconds());
#ifdef COLLECT_TIME_DEBUG
  printf("cc: total cycle collector time was %ums in %u slices\n", interval,
         mResults.mNumSlices);
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
  timeLog.Checkpoint("CleanupAfterCollection::telemetry");

  if (mJSRuntime) {
    mJSRuntime->EndCycleCollectionCallback(mResults);
    timeLog.Checkpoint("CleanupAfterCollection::EndCycleCollectionCallback()");
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
PrintPhase(const char* aPhase)
{
#ifdef DEBUG_PHASES
  printf("cc: begin %s on %s\n", aPhase,
         NS_IsMainThread() ? "mainthread" : "worker");
#endif
}

bool
nsCycleCollector::Collect(ccType aCCType,
                          SliceBudget& aBudget,
                          nsICycleCollectorListener* aManualListener,
                          bool aPreferShorterSlices)
{
  CheckThreadSafety();

  
  if (mActivelyCollecting || mFreeingSnowWhite) {
    return false;
  }
  mActivelyCollecting = true;

  bool startedIdle = (mIncrementalPhase == IdlePhase);
  bool collectedAny = false;

  
  
  if (!startedIdle) {
    TimeLog timeLog;
    FreeSnowWhite(true);
    timeLog.Checkpoint("Collect::FreeSnowWhite");
  }

  ++mResults.mNumSlices;

  bool continueSlice = aBudget.isUnlimited() || !aPreferShorterSlices;
  do {
    switch (mIncrementalPhase) {
      case IdlePhase:
        PrintPhase("BeginCollection");
        BeginCollection(aCCType, aManualListener);
        break;
      case GraphBuildingPhase:
        PrintPhase("MarkRoots");
        MarkRoots(aBudget);

        
        
        
        
        
        
        continueSlice = aBudget.isUnlimited() ||
          (mResults.mNumSlices < 3 && !aPreferShorterSlices);
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
        continueSlice = false;
        break;
    }
    if (continueSlice) {
      
      aBudget.step(SliceBudget::CounterReset);
      continueSlice = !aBudget.isOverBudget();
    }
  } while (continueSlice);

  
  
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

  
  
  
  
  MOZ_ASSERT(mIncrementalPhase == IdlePhase ||
             (mActivelyCollecting && mIncrementalPhase != GraphBuildingPhase),
             "Reentered CC during graph building");
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
                                  nsICycleCollectorListener* aManualListener)
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
  mBuilder = new CCGraphBuilder(mGraph, mResults, mJSRuntime, mListener,
                                mergeZones);

  if (mJSRuntime) {
    mJSRuntime->TraverseRoots(*mBuilder);
    timeLog.Checkpoint("mJSRuntime->TraverseRoots()");
  }

  AutoRestore<bool> ar(mScanInProgress);
  MOZ_ASSERT(!mScanInProgress);
  mScanInProgress = true;
  mPurpleBuf.SelectPointers(*mBuilder);
  timeLog.Checkpoint("SelectPointers()");

  mBuilder->DoneAddingRoots();
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
nsCycleCollector::RemoveObjectFromGraph(void* aObj)
{
  if (mIncrementalPhase == IdlePhase) {
    return;
  }

  if (PtrInfo* pinfo = mGraph.FindNode(aObj)) {
    mGraph.RemoveNodeFromMap(aObj);

    pinfo->mPointer = nullptr;
    pinfo->mParticipant = nullptr;
  }
}

void
nsCycleCollector::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                      size_t* aObjectSize,
                                      size_t* aGraphNodesSize,
                                      size_t* aGraphEdgesSize,
                                      size_t* aWeakMapsSize,
                                      size_t* aPurpleBufferSize) const
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
    
    JS::AutoSuppressGCAnalysis nogc;
    
    
    
    nsRefPtr<JSPurpleBuffer> pb = new JSPurpleBuffer(mJSPurpleBuffer);
  }
  return mJSPurpleBuffer;
}






void
nsCycleCollector_registerJSRuntime(CycleCollectedJSRuntime* aRt)
{
  CollectorData* data = sCollectorData.get();

  
  MOZ_ASSERT(data);
  MOZ_ASSERT(data->mCollector);
  
  MOZ_ASSERT(!data->mRuntime);

  data->mRuntime = aRt;
  data->mCollector->RegisterJSRuntime(aRt);
}

void
nsCycleCollector_forgetJSRuntime()
{
  CollectorData* data = sCollectorData.get();

  
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

MOZ_NEVER_INLINE static void
SuspectAfterShutdown(void* aPtr, nsCycleCollectionParticipant* aCp,
                     nsCycleCollectingAutoRefCnt* aRefCnt,
                     bool* aShouldDelete)
{
  if (aRefCnt->get() == 0) {
    if (!aShouldDelete) {
      
      CanonicalizeParticipant(&aPtr, &aCp);
      aRefCnt->stabilizeForDeletion();
      aCp->DeleteCycleCollectable(aPtr);
    } else {
      *aShouldDelete = true;
    }
  } else {
    
    aRefCnt->RemoveFromPurpleBuffer();
  }
}

void
NS_CycleCollectorSuspect3(void* aPtr, nsCycleCollectionParticipant* aCp,
                          nsCycleCollectingAutoRefCnt* aRefCnt,
                          bool* aShouldDelete)
{
  CollectorData* data = sCollectorData.get();

  
  MOZ_ASSERT(data);

  if (MOZ_LIKELY(data->mCollector)) {
    data->mCollector->Suspect(aPtr, aCp, aRefCnt);
    return;
  }
  SuspectAfterShutdown(aPtr, aCp, aRefCnt, aShouldDelete);
}

uint32_t
nsCycleCollector_suspectedCount()
{
  CollectorData* data = sCollectorData.get();

  
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
  CollectorData* data = sCollectorData.get();

  
  MOZ_ASSERT(data);
  MOZ_ASSERT(data->mCollector);

  data->mCollector->SetBeforeUnlinkCallback(aCB);
}

void
nsCycleCollector_setForgetSkippableCallback(CC_ForgetSkippableCallback aCB)
{
  CollectorData* data = sCollectorData.get();

  
  MOZ_ASSERT(data);
  MOZ_ASSERT(data->mCollector);

  data->mCollector->SetForgetSkippableCallback(aCB);
}

void
nsCycleCollector_forgetSkippable(bool aRemoveChildlessNodes,
                                 bool aAsyncSnowWhiteFreeing)
{
  CollectorData* data = sCollectorData.get();

  
  MOZ_ASSERT(data);
  MOZ_ASSERT(data->mCollector);

  PROFILER_LABEL("nsCycleCollector", "forgetSkippable",
                 js::ProfileEntry::Category::CC);

  TimeLog timeLog;
  data->mCollector->ForgetSkippable(aRemoveChildlessNodes,
                                    aAsyncSnowWhiteFreeing);
  timeLog.Checkpoint("ForgetSkippable()");
}

void
nsCycleCollector_dispatchDeferredDeletion(bool aContinuation)
{
  CycleCollectedJSRuntime* rt = CycleCollectedJSRuntime::Get();
  if (rt) {
    rt->DispatchDeferredDeletion(aContinuation);
  }
}

bool
nsCycleCollector_doDeferredDeletion()
{
  CollectorData* data = sCollectorData.get();

  
  MOZ_ASSERT(data);
  MOZ_ASSERT(data->mCollector);
  MOZ_ASSERT(data->mRuntime);

  return data->mCollector->FreeSnowWhite(false);
}

already_AddRefed<nsICycleCollectorLogSink>
nsCycleCollector_createLogSink()
{
  nsCOMPtr<nsICycleCollectorLogSink> sink = new nsCycleCollectorLogSinkToFile();
  return sink.forget();
}

void
nsCycleCollector_collect(nsICycleCollectorListener* aManualListener)
{
  CollectorData* data = sCollectorData.get();

  
  MOZ_ASSERT(data);
  MOZ_ASSERT(data->mCollector);

  PROFILER_LABEL("nsCycleCollector", "collect",
                 js::ProfileEntry::Category::CC);

  SliceBudget unlimitedBudget;
  data->mCollector->Collect(ManualCC, unlimitedBudget, aManualListener);
}

void
nsCycleCollector_collectSlice(SliceBudget& budget,
                              bool aPreferShorterSlices)
{
  CollectorData* data = sCollectorData.get();

  
  MOZ_ASSERT(data);
  MOZ_ASSERT(data->mCollector);

  PROFILER_LABEL("nsCycleCollector", "collectSlice",
                 js::ProfileEntry::Category::CC);

  data->mCollector->Collect(SliceCC, budget, nullptr, aPreferShorterSlices);
}

void
nsCycleCollector_prepareForGarbageCollection()
{
  CollectorData* data = sCollectorData.get();

  MOZ_ASSERT(data);

  if (!data->mCollector) {
    return;
  }

  data->mCollector->PrepareForGarbageCollection();
}

void
nsCycleCollector_finishAnyCurrentCollection()
{
  CollectorData* data = sCollectorData.get();

  MOZ_ASSERT(data);

  if (!data->mCollector) {
    return;
  }

  data->mCollector->FinishAnyCurrentCollection();
}

void
nsCycleCollector_shutdown()
{
  CollectorData* data = sCollectorData.get();

  if (data) {
    MOZ_ASSERT(data->mCollector);
    PROFILER_LABEL("nsCycleCollector", "shutdown",
                   js::ProfileEntry::Category::CC);

    data->mCollector->Shutdown();
    data->mCollector = nullptr;
    if (!data->mRuntime) {
      delete data;
      sCollectorData.set(nullptr);
    }
  }
}
