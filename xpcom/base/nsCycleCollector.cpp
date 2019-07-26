






















































































#if !defined(__MINGW32__)
#ifdef WIN32
#include <crtdbg.h>
#include <errno.h>
#endif
#endif

#include "base/process_util.h"


#include "mozilla/Util.h"

#include "nsCycleCollectionParticipant.h"
#include "nsCycleCollectorUtils.h"
#include "nsIProgrammingLanguage.h"
#include "nsBaseHashtable.h"
#include "nsHashKeys.h"
#include "nsDeque.h"
#include "nsCycleCollector.h"
#include "nsThreadUtils.h"
#include "prenv.h"
#include "prprf.h"
#include "plstr.h"
#include "nsPrintfCString.h"
#include "nsTArray.h"
#include "nsIObserverService.h"
#include "nsIConsoleService.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsTArray.h"
#include "mozilla/Services.h"
#include "mozilla/Attributes.h"
#include "nsICycleCollectorListener.h"
#include "nsIXPConnect.h"
#include "nsIJSRuntimeService.h"
#include "nsIMemoryReporter.h"
#include "xpcpublic.h"
#include "nsXPCOMPrivate.h"
#include "sampler.h"
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#include <process.h>
#endif

#ifdef XP_WIN
#include <windows.h>
#endif

#include "mozilla/Mutex.h"
#include "mozilla/CondVar.h"
#include "mozilla/StandardInteger.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;



#define DEFAULT_SHUTDOWN_COLLECTIONS 5
#ifdef DEBUG_CC
#define SHUTDOWN_COLLECTIONS(params) params.mShutdownCollections
#else
#define SHUTDOWN_COLLECTIONS(params) DEFAULT_SHUTDOWN_COLLECTIONS
#endif

#if defined(XP_WIN)

extern DWORD gTLSThreadIDIndex;
#elif defined(NS_TLS)

extern NS_TLS mozilla::threads::ID gTLSThreadID;
#else
PRThread* gCycleCollectorThread = nullptr;
#endif


const bool gAlwaysLogCCGraphs = false;

MOZ_NEVER_INLINE void
CC_AbortIfNull(void *ptr)
{
    if (!ptr)
        MOZ_CRASH();
}




struct nsCycleCollectorParams
{
    bool mDoNothing;
    bool mLogGraphs;
#ifdef DEBUG_CC
    bool mReportStats;
    bool mLogPointers;
    uint32_t mShutdownCollections;
#endif
    
    nsCycleCollectorParams() :
#ifdef DEBUG_CC
        mDoNothing     (PR_GetEnv("XPCOM_CC_DO_NOTHING") != NULL),
        mLogGraphs     (gAlwaysLogCCGraphs ||
                        PR_GetEnv("XPCOM_CC_DRAW_GRAPHS") != NULL),
        mReportStats   (PR_GetEnv("XPCOM_CC_REPORT_STATS") != NULL),
        mLogPointers   (PR_GetEnv("XPCOM_CC_LOG_POINTERS") != NULL),

        mShutdownCollections(DEFAULT_SHUTDOWN_COLLECTIONS)
#else
        mDoNothing     (false),
        mLogGraphs     (gAlwaysLogCCGraphs)
#endif
    {
#ifdef DEBUG_CC
        char *s = PR_GetEnv("XPCOM_CC_SHUTDOWN_COLLECTIONS");
        if (s)
            PR_sscanf(s, "%d", &mShutdownCollections);
#endif
    }
};

#ifdef DEBUG_CC



struct nsCycleCollectorStats
{
    uint32_t mFailedQI;
    uint32_t mSuccessfulQI;

    uint32_t mVisitedNode;
    uint32_t mWalkedGraph;
    uint32_t mFreedBytes;

    uint32_t mSetColorBlack;
    uint32_t mSetColorWhite;

    uint32_t mFailedUnlink;
    uint32_t mCollectedNode;

    uint32_t mSuspectNode;
    uint32_t mForgetNode;
  
    uint32_t mCollection;

    nsCycleCollectorStats()
    {
        memset(this, 0, sizeof(nsCycleCollectorStats));
    }
  
    void Dump()
    {
        fprintf(stderr, "\f\n");
#define DUMP(entry) fprintf(stderr, "%30.30s: %-20.20d\n", #entry, entry)
        DUMP(mFailedQI);
        DUMP(mSuccessfulQI);
    
        DUMP(mVisitedNode);
        DUMP(mWalkedGraph);
        DUMP(mFreedBytes);
    
        DUMP(mSetColorBlack);
        DUMP(mSetColorWhite);
    
        DUMP(mFailedUnlink);
        DUMP(mCollectedNode);
    
        DUMP(mSuspectNode);
        DUMP(mForgetNode);
    
        DUMP(mCollection);
#undef DUMP
    }
};
#endif

#ifdef DEBUG_CC
static bool nsCycleCollector_shouldSuppress(nsISupports *s);
#endif

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
        mNumBlocks = 0;
    }

    ~EdgePool()
    {
        NS_ASSERTION(!mSentinelAndBlocks[0].block &&
                     !mSentinelAndBlocks[1].block,
                     "Didn't call Clear()?");
    }

    void Clear()
    {
        Block *b = Blocks();
        while (b) {
            Block *next = b->Next();
            delete b;
            NS_ASSERTION(mNumBlocks > 0,
                         "Expected EdgePool mNumBlocks to be positive.");
            mNumBlocks--;
            b = next;
        }

        mSentinelAndBlocks[0].block = nullptr;
        mSentinelAndBlocks[1].block = nullptr;
    }

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
        Block*& Next()
            { return mPointers[BlockSize - 1].block; }
        PtrInfoOrBlock* Start()
            { return &mPointers[0]; }
        PtrInfoOrBlock* End()
            { return &mPointers[BlockSize - 2]; }
    };

    
    
    PtrInfoOrBlock mSentinelAndBlocks[2];
    uint32_t mNumBlocks;

    Block*& Blocks() { return mSentinelAndBlocks[1].block; }

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
              mNextBlockPtr(&aPool.Blocks()),
              mNumBlocks(aPool.mNumBlocks)
        {
        }

        Iterator Mark() { return Iterator(mCurrent); }

        void Add(PtrInfo* aEdge) {
            if (mCurrent == mBlockEnd) {
                Block *b = new Block();
                if (!b) {
                    
                    NS_NOTREACHED("out of memory, ignoring edges");
                    return;
                }
                *mNextBlockPtr = b;
                mCurrent = b->Start();
                mBlockEnd = b->End();
                mNextBlockPtr = &b->Next();
                mNumBlocks++;
            }
            (mCurrent++)->ptrInfo = aEdge;
        }
    private:
        
        PtrInfoOrBlock *mCurrent, *mBlockEnd;
        Block **mNextBlockPtr;
        uint32_t &mNumBlocks;
    };

    size_t BlocksSize() const {
        return sizeof(Block) * mNumBlocks;
    }

};

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
#ifdef DEBUG_CC
    size_t mBytes;
    char *mName;
#endif

    PtrInfo(void *aPointer, nsCycleCollectionParticipant *aParticipant)
        : mPointer(aPointer),
          mParticipant(aParticipant),
          mColor(grey),
          mInternalRefs(0),
          mRefCount(0),
          mFirstChild()
#ifdef DEBUG_CC
        , mBytes(0),
          mName(nullptr)
#endif
    {
        MOZ_ASSERT(aParticipant);
    }

#ifdef DEBUG_CC
    void Destroy() {
        PL_strfree(mName);
    }
#endif

    
    PtrInfo() {
        NS_NOTREACHED("should never be called");
    }

    EdgePool::Iterator FirstChild()
    {
        return mFirstChild;
    }

    
    EdgePool::Iterator LastChild()
    {
        return (this + 1)->mFirstChild;
    }

    void SetFirstChild(EdgePool::Iterator aFirstChild)
    {
        mFirstChild = aFirstChild;
    }

    
    void SetLastChild(EdgePool::Iterator aLastChild)
    {
        (this + 1)->mFirstChild = aLastChild;
    }
};





class NodePool
{
private:
    enum { BlockSize = 8 * 1024 }; 

    struct Block {
        
        
        
        Block() { NS_NOTREACHED("should never be called"); }
        ~Block() { NS_NOTREACHED("should never be called"); }

        Block* mNext;
        PtrInfo mEntries[BlockSize + 1]; 
    };

public:
    NodePool()
        : mBlocks(nullptr),
          mLast(nullptr),
          mNumBlocks(0)
    {
    }

    ~NodePool()
    {
        NS_ASSERTION(!mBlocks, "Didn't call Clear()?");
    }

    void Clear()
    {
#ifdef DEBUG_CC
        {
            Enumerator queue(*this);
            while (!queue.IsDone()) {
                queue.GetNext()->Destroy();
            }
        }
#endif
        Block *b = mBlocks;
        while (b) {
            Block *n = b->mNext;
            NS_Free(b);
            NS_ASSERTION(mNumBlocks > 0,
                         "Expected NodePool mNumBlocks to be positive.");
            mNumBlocks--;
            b = n;
        }

        mBlocks = nullptr;
        mLast = nullptr;
    }

    class Builder;
    friend class Builder;
    class Builder {
    public:
        Builder(NodePool& aPool)
            : mNextBlock(&aPool.mBlocks),
              mNext(aPool.mLast),
              mBlockEnd(nullptr),
              mNumBlocks(aPool.mNumBlocks)
        {
            NS_ASSERTION(aPool.mBlocks == nullptr && aPool.mLast == nullptr,
                         "pool not empty");
        }
        PtrInfo *Add(void *aPointer, nsCycleCollectionParticipant *aParticipant)
        {
            if (mNext == mBlockEnd) {
                Block *block;
                if (!(*mNextBlock = block =
                        static_cast<Block*>(NS_Alloc(sizeof(Block)))))
                    return nullptr;
                mNext = block->mEntries;
                mBlockEnd = block->mEntries + BlockSize;
                block->mNext = nullptr;
                mNextBlock = &block->mNext;
                mNumBlocks++;
            }
            return new (mNext++) PtrInfo(aPointer, aParticipant);
        }
    private:
        Block **mNextBlock;
        PtrInfo *&mNext;
        PtrInfo *mBlockEnd;
        uint32_t &mNumBlocks;
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
            NS_ASSERTION(!IsDone(), "calling GetNext when done");
            if (mNext == mBlockEnd) {
                Block *nextBlock = mCurBlock ? mCurBlock->mNext : mFirstBlock;
                mNext = nextBlock->mEntries;
                mBlockEnd = mNext + BlockSize;
                mCurBlock = nextBlock;
            }
            return mNext++;
        }
    private:
        Block *mFirstBlock, *mCurBlock;
        
        
        PtrInfo *mNext, *mBlockEnd, *&mLast;
    };

    size_t BlocksSize() const {
        return sizeof(Block) * mNumBlocks;
    }

private:
    Block *mBlocks;
    PtrInfo *mLast;
    uint32_t mNumBlocks;
};


struct WeakMapping
{
    
    PtrInfo *mMap;
    PtrInfo *mKey;
    PtrInfo *mVal;
};

class GCGraphBuilder;

struct GCGraph
{
    NodePool mNodes;
    EdgePool mEdges;
    nsTArray<WeakMapping> mWeakMaps;
    uint32_t mRootCount;

    GCGraph() : mRootCount(0) {
    }
    ~GCGraph() { 
    }

    size_t BlocksSize() const {
        return mNodes.BlocksSize() + mEdges.BlocksSize();
    }

};



typedef nsTHashtable<nsPtrHashKey<const void> > PointerSet;

static inline void
ToParticipant(nsISupports *s, nsXPCOMCycleCollectionParticipant **cp);

struct nsPurpleBuffer
{
private:
    struct Block {
        Block *mNext;
        nsPurpleBufferEntry mEntries[255];

        Block() : mNext(nullptr) {}
    };
public:
    
    

    nsCycleCollectorParams &mParams;
    uint32_t mNumBlocksAlloced;
    uint32_t mCount;
    Block mFirstBlock;
    nsPurpleBufferEntry *mFreeList;

    
    PointerSet mCompatObjects;
#ifdef DEBUG_CC
    PointerSet mNormalObjects; 
    nsCycleCollectorStats &mStats;
#endif
    
#ifdef DEBUG_CC
    nsPurpleBuffer(nsCycleCollectorParams &params,
                   nsCycleCollectorStats &stats) 
        : mParams(params),
          mStats(stats)
    {
        InitBlocks();
        mNormalObjects.Init();
        mCompatObjects.Init();
    }
#else
    nsPurpleBuffer(nsCycleCollectorParams &params) 
        : mParams(params)
    {
        InitBlocks();
        mCompatObjects.Init();
    }
#endif

    ~nsPurpleBuffer()
    {
        FreeBlocks();
    }

    void InitBlocks()
    {
        mNumBlocksAlloced = 0;
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
            NS_ASSERTION(mNumBlocksAlloced > 0,
                         "Expected positive mNumBlocksAlloced.");
            mNumBlocksAlloced--;
        }
        mFirstBlock.mNext = nullptr;
    }

    void UnmarkRemainingPurple(Block *b)
    {
        for (nsPurpleBufferEntry *e = b->mEntries,
                              *eEnd = ArrayEnd(b->mEntries);
             e != eEnd; ++e) {
            if (!(uintptr_t(e->mObject) & uintptr_t(1))) {
                
                
                if (e->mObject) {
                    nsXPCOMCycleCollectionParticipant *cp;
                    ToParticipant(e->mObject, &cp);

                    cp->UnmarkIfPurple(e->mObject);
                }

                if (--mCount == 0)
                    break;
            }
        }
    }

    void SelectPointers(GCGraphBuilder &builder);

    
    
    
    
    
    
    void RemoveSkippable(bool removeChildlessNodes);

#ifdef DEBUG_CC
    bool Exists(void *p) const
    {
        return mNormalObjects.GetEntry(p) || mCompatObjects.GetEntry(p);
    }
#endif

    nsPurpleBufferEntry* NewEntry()
    {
        if (!mFreeList) {
            Block *b = new Block;
            if (!b) {
                return nullptr;
            }
            mNumBlocksAlloced++;
            StartBlock(b);

            
            b->mNext = mFirstBlock.mNext;
            mFirstBlock.mNext = b;
        }

        nsPurpleBufferEntry *e = mFreeList;
        mFreeList = (nsPurpleBufferEntry*)
            (uintptr_t(mFreeList->mNextInFreeList) & ~uintptr_t(1));
        return e;
    }

    nsPurpleBufferEntry* Put(nsISupports *p)
    {
        nsPurpleBufferEntry *e = NewEntry();
        if (!e) {
            return nullptr;
        }

        ++mCount;

        e->mObject = p;

#ifdef DEBUG_CC
        mNormalObjects.PutEntry(p);
#endif

        
        return e;
    }

    void Remove(nsPurpleBufferEntry *e)
    {
        NS_ASSERTION(mCount != 0, "must have entries");

#ifdef DEBUG_CC
        mNormalObjects.RemoveEntry(e->mObject);
#endif

        e->mNextInFreeList =
            (nsPurpleBufferEntry*)(uintptr_t(mFreeList) | uintptr_t(1));
        mFreeList = e;

        --mCount;
    }

    bool PutCompatObject(nsISupports *p)
    {
        ++mCount;
        return !!mCompatObjects.PutEntry(p);
    }

    void RemoveCompatObject(nsISupports *p)
    {
        --mCount;
        mCompatObjects.RemoveEntry(p);
    }

    uint32_t Count() const
    {
        return mCount;
    }

    size_t BlocksSize() const
    {
        return sizeof(Block) * mNumBlocksAlloced;
    }

};

struct CallbackClosure
{
    CallbackClosure(nsPurpleBuffer *aPurpleBuffer, GCGraphBuilder &aBuilder)
        : mPurpleBuffer(aPurpleBuffer),
          mBuilder(aBuilder)
    {
    }
    nsPurpleBuffer *mPurpleBuffer;
    GCGraphBuilder &mBuilder;
};

static bool
AddPurpleRoot(GCGraphBuilder &builder, nsISupports *root);

static PLDHashOperator
selectionCallback(nsPtrHashKey<const void>* key, void* userArg)
{
    CallbackClosure *closure = static_cast<CallbackClosure*>(userArg);
    if (AddPurpleRoot(closure->mBuilder,
                      static_cast<nsISupports *>(
                        const_cast<void*>(key->GetKey()))))
        return PL_DHASH_REMOVE;

    return PL_DHASH_NEXT;
}

void
nsPurpleBuffer::SelectPointers(GCGraphBuilder &aBuilder)
{
#ifdef DEBUG_CC
    
    uint32_t realCount = 0;
    for (Block *b = &mFirstBlock; b; b = b->mNext) {
        for (nsPurpleBufferEntry *e = b->mEntries,
                              *eEnd = ArrayEnd(b->mEntries);
            e != eEnd; ++e) {
            if (!(uintptr_t(e->mObject) & uintptr_t(1))) {
                if (e->mObject) {
                    ++realCount;
                }
            }
        }
    }

    NS_ABORT_IF_FALSE(mCompatObjects.Count() + mNormalObjects.Count() ==
                          realCount,
                      "count out of sync");
#endif

    if (mCompatObjects.Count()) {
        mCount -= mCompatObjects.Count();
        CallbackClosure closure(this, aBuilder);
        mCompatObjects.EnumerateEntries(selectionCallback, &closure);
        mCount += mCompatObjects.Count(); 
    }

    
    for (Block *b = &mFirstBlock; b; b = b->mNext) {
        for (nsPurpleBufferEntry *e = b->mEntries,
                              *eEnd = ArrayEnd(b->mEntries);
            e != eEnd; ++e) {
            if (!(uintptr_t(e->mObject) & uintptr_t(1))) {
                
                
                if (!e->mObject || AddPurpleRoot(aBuilder, e->mObject)) {
                    Remove(e);
                }
            }
        }
    }

    NS_WARN_IF_FALSE(mCount == 0, "AddPurpleRoot failed");
    if (mCount == 0) {
        FreeBlocks();
        InitBlocks();
    }
}







struct nsCycleCollector
{
    bool mCollectionInProgress;
    bool mScanInProgress;
    bool mFollowupCollection;
    nsCycleCollectorResults *mResults;
    TimeStamp mCollectionStart;

    nsCycleCollectionJSRuntime *mJSRuntime;

    GCGraph mGraph;

    nsCycleCollectorParams mParams;

    nsTArray<PtrInfo*> *mWhiteNodes;
    uint32_t mWhiteNodeCount;

    
    uint32_t mVisitedRefCounted;
    uint32_t mVisitedGCed;

    CC_BeforeUnlinkCallback mBeforeUnlinkCB;
    CC_ForgetSkippableCallback mForgetSkippableCB;

    nsPurpleBuffer mPurpleBuf;

    void RegisterJSRuntime(nsCycleCollectionJSRuntime *aJSRuntime);
    void ForgetJSRuntime();

    void SelectPurple(GCGraphBuilder &builder);
    void MarkRoots(GCGraphBuilder &builder);
    void ScanRoots();
    void ScanWeakMaps();

    void ForgetSkippable(bool removeChildlessNodes);

    
    bool CollectWhite(nsICycleCollectorListener *aListener);

    nsCycleCollector();
    ~nsCycleCollector();

    
    
    bool Suspect(nsISupports *n);
    bool Forget(nsISupports *n);
    nsPurpleBufferEntry* Suspect2(nsISupports *n);
    bool Forget2(nsPurpleBufferEntry *e);

    void Collect(bool aMergeCompartments,
                 nsCycleCollectorResults *aResults,
                 uint32_t aTryCollections,
                 nsICycleCollectorListener *aListener);

    
    bool PrepareForCollection(nsCycleCollectorResults *aResults,
                              nsTArray<PtrInfo*> *aWhiteNodes);
    void GCIfNeeded(bool aForceGC);
    void CleanupAfterCollection();

    
    bool BeginCollection(bool aMergeCompartments, nsICycleCollectorListener *aListener);
    bool FinishCollection(nsICycleCollectorListener *aListener);

    uint32_t SuspectedCount();
    void Shutdown();

    void ClearGraph()
    {
        mGraph.mNodes.Clear();
        mGraph.mEdges.Clear();
        mGraph.mWeakMaps.Clear();
        mGraph.mRootCount = 0;
    }

#ifdef DEBUG_CC
    nsCycleCollectorStats mStats;
    FILE *mPtrLog;
    PointerSet mExpectedGarbage;

    void LogPurpleRemoval(void* aObject);

    void ShouldBeFreed(nsISupports *n);
    void WasFreed(nsISupports *n);
#endif
};









template <class Visitor>
class GraphWalker
{
private:
    Visitor mVisitor;

    void DoWalk(nsDeque &aQueue);

public:
    void Walk(PtrInfo *s0);
    void WalkFromRoots(GCGraph &aGraph);
    
    
    GraphWalker(const Visitor aVisitor) : mVisitor(aVisitor) {}
};







static nsCycleCollector *sCollector = nullptr;






MOZ_NEVER_INLINE static void
Fault(const char *msg, const void *ptr=nullptr)
{
    if (ptr)
        printf("Fault in cycle collector: %s (ptr: %p)\n", msg, ptr);
    else
        printf("Fault in cycle collector: %s\n", msg);

    NS_RUNTIMEABORT("cycle collector fault");
}

#ifdef DEBUG_CC
static void
Fault(const char *msg, PtrInfo *pi)
{
    printf("Fault in cycle collector: %s\n"
           "  while operating on pointer %p %s\n",
           msg, pi->mPointer, pi->mName);
    if (pi->mInternalRefs) {
        printf("  which has internal references from:\n");
        NodePool::Enumerator queue(sCollector->mGraph.mNodes);
        while (!queue.IsDone()) {
            PtrInfo *ppi = queue.GetNext();
            for (EdgePool::Iterator e = ppi->FirstChild(),
                                e_end = ppi->LastChild();
                 e != e_end; ++e) {
                if (*e == pi) {
                    printf("    %p %s\n", ppi->mPointer, ppi->mName);
                }
            }
        }
    }

    Fault(msg, pi->mPointer);
}
#else
static void
Fault(const char *msg, PtrInfo *pi)
{
    Fault(msg, pi->mPointer);
}
#endif

static inline void
AbortIfOffMainThreadIfCheckFast()
{
#if defined(XP_WIN) || defined(NS_TLS)
    if (!NS_IsMainThread() && !NS_IsCycleCollectorThread()) {
        NS_RUNTIMEABORT("Main-thread-only object used off the main thread");
    }
#endif
}

static nsISupports *
canonicalize(nsISupports *in)
{
    nsISupports* child;
    in->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                       reinterpret_cast<void**>(&child));
    return child;
}

static inline void
ToParticipant(nsISupports *s, nsXPCOMCycleCollectionParticipant **cp)
{
    
    
    
    
    CallQueryInterface(s, cp);
#ifdef DEBUG_CC
    if (cp)
        ++sCollector->mStats.mSuccessfulQI;
    else
        ++sCollector->mStats.mFailedQI;
#endif
}

template <class Visitor>
MOZ_NEVER_INLINE void
GraphWalker<Visitor>::Walk(PtrInfo *s0)
{
    nsDeque queue;
    CC_AbortIfNull(s0);
    queue.Push(s0);
    DoWalk(queue);
}

template <class Visitor>
MOZ_NEVER_INLINE void
GraphWalker<Visitor>::WalkFromRoots(GCGraph& aGraph)
{
    nsDeque queue;
    NodePool::Enumerator etor(aGraph.mNodes);
    for (uint32_t i = 0; i < aGraph.mRootCount; ++i) {
        PtrInfo *pi = etor.GetNext();
        CC_AbortIfNull(pi);
        queue.Push(pi);
    }
    DoWalk(queue);
}

template <class Visitor>
MOZ_NEVER_INLINE void
GraphWalker<Visitor>::DoWalk(nsDeque &aQueue)
{
    
    
    while (aQueue.GetSize() > 0) {
        PtrInfo *pi = static_cast<PtrInfo*>(aQueue.PopFront());
        CC_AbortIfNull(pi);

        if (mVisitor.ShouldVisitNode(pi)) {
            mVisitor.VisitNode(pi);
            for (EdgePool::Iterator child = pi->FirstChild(),
                                child_end = pi->LastChild();
                 child != child_end; ++child) {
                CC_AbortIfNull(*child);
                aQueue.Push(*child);
            }
        }
    };

#ifdef DEBUG_CC
    sCollector->mStats.mWalkedGraph++;
#endif
}

struct CCGraphDescriber
{
  CCGraphDescriber()
  : mAddress("0x"), mToAddress("0x"), mCnt(0), mType(eUnknown) {}

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
  nsCString mToAddress;
  nsCString mName;
  uint32_t mCnt;
  Type mType;
};

class nsCycleCollectorLogger MOZ_FINAL : public nsICycleCollectorListener
{
public:
    nsCycleCollectorLogger() :
      mStream(nullptr), mWantAllTraces(false),
      mDisableLog(false), mWantAfterProcessing(false),
      mNextIndex(0)
    {
    }
    ~nsCycleCollectorLogger()
    {
        if (mStream) {
            fclose(mStream);
        }
    }
    NS_DECL_ISUPPORTS

    NS_IMETHOD AllTraces(nsICycleCollectorListener** aListener)
    {
        mWantAllTraces = true;
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

    NS_IMETHOD Begin()
    {
        mCurrentAddress.AssignLiteral("0x");
        mDescribers.Clear();
        mNextIndex = 0;
        if (mDisableLog) {
            return NS_OK;
        }
        char basename[MAXPATHLEN] = {'\0'};
        char ccname[MAXPATHLEN] = {'\0'};
#ifdef XP_WIN
        
        
        GetTempPathA(mozilla::ArrayLength(basename), basename);
#else
        tmpnam(basename);
        char *lastSlash = strrchr(basename, XPCOM_FILE_PATH_SEPARATOR[0]);
        if (lastSlash) {
            *lastSlash = '\0';
        }
#endif

        ++gLogCounter;

        
        char gcname[MAXPATHLEN] = {'\0'};
        sprintf(gcname, "%s%sgc-edges-%d.%d.log", basename,
                XPCOM_FILE_PATH_SEPARATOR,
                gLogCounter, base::GetCurrentProcId());

        FILE* gcDumpFile = fopen(gcname, "w");
        if (!gcDumpFile)
            return NS_ERROR_FAILURE;
        xpc::DumpJSHeap(gcDumpFile);
        fclose(gcDumpFile);

        
        sprintf(ccname, "%s%scc-edges-%d.%d.log", basename,
                XPCOM_FILE_PATH_SEPARATOR,
                gLogCounter, base::GetCurrentProcId());
        mStream = fopen(ccname, "w");
        if (!mStream)
            return NS_ERROR_FAILURE;

        nsCOMPtr<nsIConsoleService> cs =
            do_GetService(NS_CONSOLESERVICE_CONTRACTID);
        if (cs) {
            cs->LogStringMessage(NS_ConvertUTF8toUTF16(ccname).get());
            cs->LogStringMessage(NS_ConvertUTF8toUTF16(gcname).get());
        }

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
            CCGraphDescriber* d = mDescribers.AppendElement();
            NS_ENSURE_TRUE(d, NS_ERROR_OUT_OF_MEMORY);
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
                              const char *aObjectDescription)
    {
        if (!mDisableLog) {
            fprintf(mStream, "%p [gc%s] %s\n", (void*)aAddress,
                    aMarked ? ".marked" : "", aObjectDescription);
        }
        if (mWantAfterProcessing) {
            CCGraphDescriber* d = mDescribers.AppendElement();
            NS_ENSURE_TRUE(d, NS_ERROR_OUT_OF_MEMORY);
            mCurrentAddress.AssignLiteral("0x");
            mCurrentAddress.AppendInt(aAddress, 16);
            d->mType = aMarked ? CCGraphDescriber::eGCMarkedObject :
                                 CCGraphDescriber::eGCedObject;
            d->mAddress = mCurrentAddress;
            d->mName.Append(aObjectDescription);
        }
        return NS_OK;
    }
    NS_IMETHOD NoteEdge(uint64_t aToAddress, const char *aEdgeName)
    {
        if (!mDisableLog) {
            fprintf(mStream, "> %p %s\n", (void*)aToAddress, aEdgeName);
        }
        if (mWantAfterProcessing) {
            CCGraphDescriber* d = mDescribers.AppendElement();
            NS_ENSURE_TRUE(d, NS_ERROR_OUT_OF_MEMORY);
            d->mType = CCGraphDescriber::eEdge;
            d->mAddress = mCurrentAddress;
            d->mToAddress.AppendInt(aToAddress, 16);
            d->mName.Append(aEdgeName);
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
            CCGraphDescriber* d = mDescribers.AppendElement();
            NS_ENSURE_TRUE(d, NS_ERROR_OUT_OF_MEMORY);
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
            CCGraphDescriber* d = mDescribers.AppendElement();
            NS_ENSURE_TRUE(d, NS_ERROR_OUT_OF_MEMORY);
            d->mType = CCGraphDescriber::eGarbage;
            d->mAddress.AppendInt(aAddress, 16);
        }
        return NS_OK;
    }
    NS_IMETHOD End()
    {
        if (!mDisableLog) {
            fclose(mStream);
            mStream = nullptr;
        }
        return NS_OK;
    }

    NS_IMETHOD ProcessNext(nsICycleCollectorHandler* aHandler,
                           bool* aCanContinue)
    {
        NS_ENSURE_STATE(aHandler && mWantAfterProcessing);
        if (mNextIndex < mDescribers.Length()) {
            CCGraphDescriber& d = mDescribers[mNextIndex++];
            switch (d.mType) {
                case CCGraphDescriber::eRefCountedObject:
                    aHandler->NoteRefCountedObject(d.mAddress,
                                                   d.mCnt,
                                                   d.mName);
                    break;
                case CCGraphDescriber::eGCedObject:
                case CCGraphDescriber::eGCMarkedObject:
                    aHandler->NoteGCedObject(d.mAddress,
                                             d.mType ==
                                               CCGraphDescriber::eGCMarkedObject,
                                             d.mName);
                    break;
                case CCGraphDescriber::eEdge:
                    aHandler->NoteEdge(d.mAddress,
                                       d.mToAddress,
                                       d.mName);
                    break;
                case CCGraphDescriber::eRoot:
                    aHandler->DescribeRoot(d.mAddress,
                                           d.mCnt);
                    break;
                case CCGraphDescriber::eGarbage:
                    aHandler->DescribeGarbage(d.mAddress);
                    break;
                case CCGraphDescriber::eUnknown:
                    NS_NOTREACHED("CCGraphDescriber::eUnknown");
                    break;
            }
        }
        if (!(*aCanContinue = mNextIndex < mDescribers.Length())) {
            mCurrentAddress.AssignLiteral("0x");
            mDescribers.Clear();
            mNextIndex = 0;
        }
        return NS_OK;
    }
private:
    FILE *mStream;
    bool mWantAllTraces;
    bool mDisableLog;
    bool mWantAfterProcessing;
    nsCString mCurrentAddress; 
    nsTArray<CCGraphDescriber> mDescribers;
    uint32_t mNextIndex;
    static uint32_t gLogCounter;
};

NS_IMPL_ISUPPORTS1(nsCycleCollectorLogger, nsICycleCollectorListener)

uint32_t nsCycleCollectorLogger::gLogCounter = 0;

nsresult
nsCycleCollectorLoggerConstructor(nsISupports* aOuter,
                                  const nsIID& aIID,
                                  void* *aInstancePtr)
{
    NS_ENSURE_TRUE(!aOuter, NS_ERROR_NO_AGGREGATION);

    nsISupports *logger = new nsCycleCollectorLogger();

    return logger->QueryInterface(aIID, aInstancePtr);
}





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

class GCGraphBuilder : public nsCycleCollectionTraversalCallback
{
private:
    NodePool::Builder mNodeBuilder;
    EdgePool::Builder mEdgeBuilder;
    nsTArray<WeakMapping> &mWeakMaps;
    PLDHashTable mPtrToNodeMap;
    PtrInfo *mCurrPi;
    nsCycleCollectionParticipant *mJSParticipant;
    nsCycleCollectionParticipant *mJSCompParticipant;
    nsCString mNextEdgeName;
    nsICycleCollectorListener *mListener;
    bool mMergeCompartments;

public:
    GCGraphBuilder(GCGraph &aGraph,
                   nsCycleCollectionJSRuntime *aJSRuntime,
                   nsICycleCollectorListener *aListener,
                   bool aMergeCompartments);
    ~GCGraphBuilder();
    bool Initialized();

    uint32_t Count() const { return mPtrToNodeMap.entryCount; }

    PtrInfo* AddNode(void *s, nsCycleCollectionParticipant *aParticipant);
    PtrInfo* AddWeakMapNode(void* node);
    void Traverse(PtrInfo* aPtrInfo);
    void SetLastChild();

private:
    void DescribeNode(uint32_t refCount,
                      size_t objSz,
                      const char *objName)
    {
        mCurrPi->mRefCount = refCount;
#ifdef DEBUG_CC
        mCurrPi->mBytes = objSz;
        mCurrPi->mName = PL_strdup(objName);
        sCollector->mStats.mVisitedNode++;
#endif
    }

public:
    
    NS_IMETHOD_(void) DescribeRefCountedNode(nsrefcnt refCount, size_t objSz,
                                             const char *objName);
    NS_IMETHOD_(void) DescribeGCedNode(bool isMarked, size_t objSz,
                                       const char *objName);

    NS_IMETHOD_(void) NoteXPCOMRoot(nsISupports *root);
    NS_IMETHOD_(void) NoteJSRoot(void *root);
    NS_IMETHOD_(void) NoteNativeRoot(void *root, nsCycleCollectionParticipant *participant);

    NS_IMETHOD_(void) NoteXPCOMChild(nsISupports *child);
    NS_IMETHOD_(void) NoteJSChild(void *child);
    NS_IMETHOD_(void) NoteNativeChild(void *child,
                                      nsCycleCollectionParticipant *participant);

    NS_IMETHOD_(void) NoteNextEdgeName(const char* name);
    NS_IMETHOD_(void) NoteWeakMapping(void *map, void *key, void *val);

private:
    NS_IMETHOD_(void) NoteRoot(void *root,
                               nsCycleCollectionParticipant *participant)
    {
        MOZ_ASSERT(root);
        MOZ_ASSERT(participant);

        if (!participant->CanSkipInCC(root) || NS_UNLIKELY(WantAllTraces())) {
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

    JSCompartment *MergeCompartment(void *gcthing) {
        if (!mMergeCompartments) {
            return nullptr;
        }
        JSCompartment *comp = js::GetGCThingCompartment(gcthing);
        if (js::IsSystemCompartment(comp)) {
            return nullptr;
        }
        return comp;
    }
};

GCGraphBuilder::GCGraphBuilder(GCGraph &aGraph,
                               nsCycleCollectionJSRuntime *aJSRuntime,
                               nsICycleCollectorListener *aListener,
                               bool aMergeCompartments)
    : mNodeBuilder(aGraph.mNodes),
      mEdgeBuilder(aGraph.mEdges),
      mWeakMaps(aGraph.mWeakMaps),
      mJSParticipant(nullptr),
      mJSCompParticipant(xpc_JSCompartmentParticipant()),
      mListener(aListener),
      mMergeCompartments(aMergeCompartments)
{
    if (!PL_DHashTableInit(&mPtrToNodeMap, &PtrNodeOps, nullptr,
                           sizeof(PtrToNodeEntry), 32768))
        mPtrToNodeMap.ops = nullptr;

    if (aJSRuntime) {
        mJSParticipant = aJSRuntime->GetParticipant();
    }

    uint32_t flags = 0;
#ifdef DEBUG_CC
    flags = nsCycleCollectionTraversalCallback::WANT_DEBUG_INFO |
            nsCycleCollectionTraversalCallback::WANT_ALL_TRACES;
#endif
    if (!flags && mListener) {
        flags = nsCycleCollectionTraversalCallback::WANT_DEBUG_INFO;
        bool all = false;
        mListener->GetWantAllTraces(&all);
        if (all) {
            flags |= nsCycleCollectionTraversalCallback::WANT_ALL_TRACES;
        }
    }

    mFlags |= flags;

    mMergeCompartments = mMergeCompartments && NS_LIKELY(!WantAllTraces());
}

GCGraphBuilder::~GCGraphBuilder()
{
    if (mPtrToNodeMap.ops)
        PL_DHashTableFinish(&mPtrToNodeMap);
}

bool
GCGraphBuilder::Initialized()
{
    return !!mPtrToNodeMap.ops;
}

PtrInfo*
GCGraphBuilder::AddNode(void *s, nsCycleCollectionParticipant *aParticipant)
{
    PtrToNodeEntry *e = static_cast<PtrToNodeEntry*>(PL_DHashTableOperate(&mPtrToNodeMap, s, PL_DHASH_ADD));
    if (!e)
        return nullptr;

    PtrInfo *result;
    if (!e->mNode) {
        
        result = mNodeBuilder.Add(s, aParticipant);
        if (!result) {
            PL_DHashTableRawRemove(&mPtrToNodeMap, e);
            return nullptr;
        }
        e->mNode = result;
    } else {
        result = e->mNode;
        NS_ASSERTION(result->mParticipant == aParticipant,
                     "nsCycleCollectionParticipant shouldn't change!");
    }
    return result;
}

MOZ_NEVER_INLINE void
GCGraphBuilder::Traverse(PtrInfo* aPtrInfo)
{
    mCurrPi = aPtrInfo;

#ifdef DEBUG_CC
    if (!mCurrPi->mParticipant) {
        Fault("unknown pointer during walk", aPtrInfo);
        return;
    }
#endif

    mCurrPi->SetFirstChild(mEdgeBuilder.Mark());

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
    root = canonicalize(root);
    NS_ASSERTION(root,
                 "Don't add objects that don't participate in collection!");

#ifdef DEBUG_CC
    if (nsCycleCollector_shouldSuppress(root))
        return;
#endif
    
    nsXPCOMCycleCollectionParticipant *cp;
    ToParticipant(root, &cp);

    NoteRoot(root, cp);
}

NS_IMETHODIMP_(void)
GCGraphBuilder::NoteJSRoot(void *root)
{
    if (JSCompartment *comp = MergeCompartment(root)) {
        NoteRoot(comp, mJSCompParticipant);
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
GCGraphBuilder::DescribeRefCountedNode(nsrefcnt refCount, size_t objSz,
                                       const char *objName)
{
    if (refCount == 0)
        Fault("zero refcount", mCurrPi);
    if (refCount == PR_UINT32_MAX)
        Fault("overflowing refcount", mCurrPi);
    sCollector->mVisitedRefCounted++;

    if (mListener) {
        mListener->NoteRefCountedObject((uint64_t)mCurrPi->mPointer, refCount,
                                        objName);
    }

    DescribeNode(refCount, objSz, objName);
}

NS_IMETHODIMP_(void)
GCGraphBuilder::DescribeGCedNode(bool isMarked, size_t objSz,
                                 const char *objName)
{
    uint32_t refCount = isMarked ? PR_UINT32_MAX : 0;
    sCollector->mVisitedGCed++;

    if (mListener) {
        mListener->NoteGCedObject((uint64_t)mCurrPi->mPointer, isMarked,
                                  objName);
    }

    DescribeNode(refCount, objSz, objName);
}

NS_IMETHODIMP_(void)
GCGraphBuilder::NoteXPCOMChild(nsISupports *child) 
{
    nsCString edgeName;
    if (WantDebugInfo()) {
        edgeName.Assign(mNextEdgeName);
        mNextEdgeName.Truncate();
    }
    if (!child || !(child = canonicalize(child)))
        return; 

#ifdef DEBUG_CC
    if (nsCycleCollector_shouldSuppress(child))
        return;
#endif
    
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

    NS_ASSERTION(participant, "Need a nsCycleCollectionParticipant!");
    NoteChild(child, participant, edgeName);
}

NS_IMETHODIMP_(void)
GCGraphBuilder::NoteJSChild(void *child) 
{
    if (!child) {
        return;
    }

    nsCString edgeName;
    if (NS_UNLIKELY(WantDebugInfo())) {
        edgeName.Assign(mNextEdgeName);
        mNextEdgeName.Truncate();
    }

    if (xpc_GCThingIsGrayCCThing(child) || NS_UNLIKELY(WantAllTraces())) {
        if (JSCompartment *comp = MergeCompartment(child)) {
            NoteChild(comp, mJSCompParticipant, edgeName);
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
    NS_ASSERTION(node, "Weak map node should be non-null.");

    if (!xpc_GCThingIsGrayCCThing(node) && !WantAllTraces())
        return nullptr;

    if (JSCompartment *comp = MergeCompartment(node)) {
        return AddNode(comp, mJSCompParticipant);
    } else {
        return AddNode(node, mJSParticipant);
    }
}

NS_IMETHODIMP_(void)
GCGraphBuilder::NoteWeakMapping(void *map, void *key, void *val)
{
    PtrInfo *valNode = AddWeakMapNode(val);

    if (!valNode)
        return;

    WeakMapping *mapping = mWeakMaps.AppendElement();
    mapping->mMap = map ? AddWeakMapNode(map) : nullptr;
    mapping->mKey = key ? AddWeakMapNode(key) : nullptr;
    mapping->mVal = valNode;
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
                                             size_t objsz,
                                             const char *objname) {}
    NS_IMETHOD_(void) DescribeGCedNode(bool ismarked,
                                       size_t objsz,
                                       const char *objname) {}
    NS_IMETHOD_(void) NoteXPCOMRoot(nsISupports *root) {}
    NS_IMETHOD_(void) NoteJSRoot(void *root) {}
    NS_IMETHOD_(void) NoteNativeRoot(void *root,
                                     nsCycleCollectionParticipant *helper) {}
    NS_IMETHOD_(void) NoteNextEdgeName(const char* name) {}
    NS_IMETHOD_(void) NoteWeakMapping(void *map, void *key, void *val) {}
    bool MayHaveChild() {
        return mMayHaveChild;
    }
private:
    bool mMayHaveChild;
};

NS_IMETHODIMP_(void)
ChildFinder::NoteXPCOMChild(nsISupports *child)
{
    if (!child || !(child = canonicalize(child)))
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
AddPurpleRoot(GCGraphBuilder &builder, nsISupports *root)
{
    root = canonicalize(root);
    NS_ASSERTION(root,
                 "Don't add objects that don't participate in collection!");

    nsXPCOMCycleCollectionParticipant *cp;
    ToParticipant(root, &cp);

    if (builder.WantAllTraces() || !cp->CanSkipInCC(root)) {
        PtrInfo *pinfo = builder.AddNode(root, cp);
        if (!pinfo) {
            return false;
        }
    }

    cp->UnmarkIfPurple(root);

    return true;
}

static bool
MayHaveChild(nsISupports *o, nsXPCOMCycleCollectionParticipant* cp)
{
    ChildFinder cf;
    cp->Traverse(o, cf);
    return cf.MayHaveChild();
}

void
nsPurpleBuffer::RemoveSkippable(bool removeChildlessNodes)
{
    
    for (Block *b = &mFirstBlock; b; b = b->mNext) {
        for (nsPurpleBufferEntry *e = b->mEntries,
                              *eEnd = ArrayEnd(b->mEntries);
            e != eEnd; ++e) {
            if (!(uintptr_t(e->mObject) & uintptr_t(1))) {
                
                
                if (e->mObject) {
                    nsISupports* o = canonicalize(e->mObject);
                    nsXPCOMCycleCollectionParticipant* cp;
                    ToParticipant(o, &cp);
                    if (!cp->CanSkip(o, false) &&
                        (!removeChildlessNodes || MayHaveChild(o, cp))) {
                        continue;
                    }
                    cp->UnmarkIfPurple(o);
                }
                Remove(e);
            }
        }
    }
}

void 
nsCycleCollector::SelectPurple(GCGraphBuilder &builder)
{
    mPurpleBuf.SelectPointers(builder);
}

void 
nsCycleCollector::ForgetSkippable(bool removeChildlessNodes)
{
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
        obs->NotifyObservers(nullptr, "cycle-collector-forget-skippable", nullptr);
    }
    mPurpleBuf.RemoveSkippable(removeChildlessNodes);
    if (mForgetSkippableCB) {
        mForgetSkippableCB();
    }
}

MOZ_NEVER_INLINE void
nsCycleCollector::MarkRoots(GCGraphBuilder &builder)
{
    mGraph.mRootCount = builder.Count();

    
    NodePool::Enumerator queue(mGraph.mNodes);
    while (!queue.IsDone()) {
        PtrInfo *pi = queue.GetNext();
        CC_AbortIfNull(pi);
        builder.Traverse(pi);
        if (queue.AtBlockEnd())
            builder.SetLastChild();
    }
    if (mGraph.mRootCount > 0)
        builder.SetLastChild();
}







struct ScanBlackVisitor
{
    ScanBlackVisitor(uint32_t &aWhiteNodeCount)
        : mWhiteNodeCount(aWhiteNodeCount)
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
#ifdef DEBUG_CC
        sCollector->mStats.mSetColorBlack++;
#endif
    }

    uint32_t &mWhiteNodeCount;
};


struct scanVisitor
{
    scanVisitor(uint32_t &aWhiteNodeCount) : mWhiteNodeCount(aWhiteNodeCount)
    {
    }

    bool ShouldVisitNode(PtrInfo const *pi)
    { 
        return pi->mColor == grey;
    }

    MOZ_NEVER_INLINE void VisitNode(PtrInfo *pi)
    {
        if (pi->mInternalRefs > pi->mRefCount && pi->mRefCount > 0)
            Fault("traversed refs exceed refcount", pi);

        if (pi->mInternalRefs == pi->mRefCount || pi->mRefCount == 0) {
            pi->mColor = white;
            ++mWhiteNodeCount;
#ifdef DEBUG_CC
            sCollector->mStats.mSetColorWhite++;
#endif
        } else {
            GraphWalker<ScanBlackVisitor>(ScanBlackVisitor(mWhiteNodeCount)).Walk(pi);
            NS_ASSERTION(pi->mColor == black,
                         "Why didn't ScanBlackVisitor make pi black?");
        }
    }

    uint32_t &mWhiteNodeCount;
};



void
nsCycleCollector::ScanWeakMaps()
{
    bool anyChanged;
    do {
        anyChanged = false;
        for (uint32_t i = 0; i < mGraph.mWeakMaps.Length(); i++) {
            WeakMapping *wm = &mGraph.mWeakMaps[i];

            
            uint32_t mColor = wm->mMap ? wm->mMap->mColor : black;
            uint32_t kColor = wm->mKey ? wm->mKey->mColor : black;
            PtrInfo *v = wm->mVal;

            
            
            
            
            NS_ASSERTION(mColor != grey, "Uncolored weak map");
            NS_ASSERTION(kColor != grey, "Uncolored weak map key");
            NS_ASSERTION(v->mColor != grey, "Uncolored weak map value");

            if (mColor == black && kColor == black && v->mColor != black) {
                GraphWalker<ScanBlackVisitor>(ScanBlackVisitor(mWhiteNodeCount)).Walk(v);
                anyChanged = true;
            }
        }
    } while (anyChanged);
}

void
nsCycleCollector::ScanRoots()
{
    mWhiteNodeCount = 0;

    
    
    
    GraphWalker<scanVisitor>(scanVisitor(mWhiteNodeCount)).WalkFromRoots(mGraph); 

    ScanWeakMaps();

#ifdef DEBUG_CC
    
    
    NodePool::Enumerator etor(mGraph.mNodes);
    while (!etor.IsDone())
    {
        PtrInfo *pinfo = etor.GetNext();
        if (pinfo->mColor == grey) {
            Fault("valid grey node after scanning", pinfo);
        }
    }
#endif
}






bool
nsCycleCollector::CollectWhite(nsICycleCollectorListener *aListener)
{
    
    
    
    
    
    
    
    
    
    
    
    
    

    nsresult rv;
    TimeLog timeLog;

    NS_ASSERTION(mWhiteNodes->IsEmpty(),
                 "FinishCollection wasn't called?");

    mWhiteNodes->SetCapacity(mWhiteNodeCount);
    uint32_t numWhiteGCed = 0;

    NodePool::Enumerator etor(mGraph.mNodes);
    while (!etor.IsDone())
    {
        PtrInfo *pinfo = etor.GetNext();
        if (pinfo->mColor == white && mWhiteNodes->AppendElement(pinfo)) {
            rv = pinfo->mParticipant->Root(pinfo->mPointer);
            if (NS_FAILED(rv)) {
                Fault("Failed root call while unlinking", pinfo);
                mWhiteNodes->RemoveElementAt(mWhiteNodes->Length() - 1);
            } else if (pinfo->mRefCount == 0) {
                
                ++numWhiteGCed;
            }
        }
    }

    uint32_t count = mWhiteNodes->Length();
    NS_ASSERTION(numWhiteGCed <= count,
                 "More freed GCed nodes than total freed nodes.");
    if (mResults) {
        mResults->mFreedRefCounted += count - numWhiteGCed;
        mResults->mFreedGCed += numWhiteGCed;
    }

    timeLog.Checkpoint("CollectWhite::Root");

    if (mBeforeUnlinkCB) {
        mBeforeUnlinkCB();
        timeLog.Checkpoint("CollectWhite::BeforeUnlinkCB");
    }
#if defined(DEBUG_CC) && !defined(__MINGW32__) && defined(WIN32)
    struct _CrtMemState ms1, ms2;
    _CrtMemCheckpoint(&ms1);
#endif

    if (aListener) {
        for (uint32_t i = 0; i < count; ++i) {
            PtrInfo *pinfo = mWhiteNodes->ElementAt(i);
            aListener->DescribeGarbage((uint64_t)pinfo->mPointer);
        }
        aListener->End();
    }

    for (uint32_t i = 0; i < count; ++i) {
        PtrInfo *pinfo = mWhiteNodes->ElementAt(i);
        rv = pinfo->mParticipant->Unlink(pinfo->mPointer);
        if (NS_FAILED(rv)) {
            Fault("Failed unlink call while unlinking", pinfo);
#ifdef DEBUG_CC
            mStats.mFailedUnlink++;
#endif
        }
        else {
#ifdef DEBUG_CC
            ++mStats.mCollectedNode;
#endif
        }
    }
    timeLog.Checkpoint("CollectWhite::Unlink");

    for (uint32_t i = 0; i < count; ++i) {
        PtrInfo *pinfo = mWhiteNodes->ElementAt(i);
        rv = pinfo->mParticipant->Unroot(pinfo->mPointer);
        if (NS_FAILED(rv))
            Fault("Failed unroot call while unlinking", pinfo);
    }
    timeLog.Checkpoint("CollectWhite::Unroot");

#if defined(DEBUG_CC) && !defined(__MINGW32__) && defined(WIN32)
    _CrtMemCheckpoint(&ms2);
    if (ms2.lTotalCount < ms1.lTotalCount)
        mStats.mFreedBytes += (ms1.lTotalCount - ms2.lTotalCount);
#endif

    return count > 0;
}






nsCycleCollector::nsCycleCollector() :
    mCollectionInProgress(false),
    mScanInProgress(false),
    mResults(nullptr),
    mJSRuntime(nullptr),
    mWhiteNodes(nullptr),
    mWhiteNodeCount(0),
    mVisitedRefCounted(0),
    mVisitedGCed(0),
    mBeforeUnlinkCB(nullptr),
    mForgetSkippableCB(nullptr),
#ifdef DEBUG_CC
    mPurpleBuf(mParams, mStats),
    mPtrLog(nullptr)
#else
    mPurpleBuf(mParams)
#endif
{
#ifdef DEBUG_CC
    mExpectedGarbage.Init();
#endif
}


nsCycleCollector::~nsCycleCollector()
{
}


void 
nsCycleCollector::RegisterJSRuntime(nsCycleCollectionJSRuntime *aJSRuntime)
{
    if (mParams.mDoNothing)
        return;

    if (mJSRuntime)
        Fault("multiple registrations of cycle collector JS runtime", aJSRuntime);

    mJSRuntime = aJSRuntime;
}

void 
nsCycleCollector::ForgetJSRuntime()
{
    if (mParams.mDoNothing)
        return;

    if (!mJSRuntime)
        Fault("forgetting non-registered cycle collector JS runtime");

    mJSRuntime = nullptr;
}

#ifdef DEBUG_CC

class Suppressor :
    public nsCycleCollectionTraversalCallback
{
protected:
    static char *sSuppressionList;
    static bool sInitialized;
    bool mSuppressThisNode;
public:
    Suppressor()
    {
    }

    bool shouldSuppress(nsISupports *s)
    {
        if (!sInitialized) {
            sSuppressionList = PR_GetEnv("XPCOM_CC_SUPPRESS");
            sInitialized = true;
        }
        if (sSuppressionList == nullptr) {
            mSuppressThisNode = false;
        } else {
            nsresult rv;
            nsXPCOMCycleCollectionParticipant *cp;
            rv = CallQueryInterface(s, &cp);
            if (NS_FAILED(rv)) {
                Fault("checking suppression on wrong type of pointer", s);
                return true;
            }
            cp->Traverse(s, *this);
        }
        return mSuppressThisNode;
    }

    NS_IMETHOD_(void) DescribeRefCountedNode(nsrefcnt refCount, size_t objSz,
                                             const char *objName)
    {
        mSuppressThisNode = (PL_strstr(sSuppressionList, objName) != nullptr);
    }

    NS_IMETHOD_(void) DescribeGCedNode(bool isMarked, size_t objSz,
                                       const char *objName)
    {
        mSuppressThisNode = (PL_strstr(sSuppressionList, objName) != nullptr);
    }

    NS_IMETHOD_(void) NoteXPCOMRoot(nsISupports *root) {}
    NS_IMETHOD_(void) NoteJSRoot(void *root) {}
    NS_IMETHOD_(void) NoteNativeRoot(void *root,
                                     nsCycleCollectionParticipant *participant) {}
    NS_IMETHOD_(void) NoteXPCOMChild(nsISupports *child) {}
    NS_IMETHOD_(void) NoteJSChild(void *child) {}
    NS_IMETHOD_(void) NoteNativeChild(void *child,
                                     nsCycleCollectionParticipant *participant) {}
    NS_IMETHOD_(void) NoteNextEdgeName(const char* name) {}
    NS_IMETHOD_(void) NoteWeakMapping(void *map, void *key, void *val) {}
};

char *Suppressor::sSuppressionList = nullptr;
bool Suppressor::sInitialized = false;

static bool
nsCycleCollector_shouldSuppress(nsISupports *s)
{
    Suppressor supp;
    return supp.shouldSuppress(s);
}
#endif

#ifdef DEBUG
static bool
nsCycleCollector_isScanSafe(nsISupports *s)
{
    if (!s)
        return false;

    nsXPCOMCycleCollectionParticipant *cp;
    ToParticipant(s, &cp);

    return cp != nullptr;
}
#endif

bool
nsCycleCollector::Suspect(nsISupports *n)
{
    AbortIfOffMainThreadIfCheckFast();

    
    
    

    if (mScanInProgress)
        return false;

    NS_ASSERTION(nsCycleCollector_isScanSafe(n),
                 "suspected a non-scansafe pointer");

    if (mParams.mDoNothing)
        return false;

#ifdef DEBUG_CC
    mStats.mSuspectNode++;

    if (nsCycleCollector_shouldSuppress(n))
        return false;

    if (mParams.mLogPointers) {
        if (!mPtrLog)
            mPtrLog = fopen("pointer_log", "w");
        fprintf(mPtrLog, "S %p\n", static_cast<void*>(n));
    }
#endif

    return mPurpleBuf.PutCompatObject(n);
}


bool
nsCycleCollector::Forget(nsISupports *n)
{
    AbortIfOffMainThreadIfCheckFast();

    
    
    

    if (mScanInProgress)
        return false;

    if (mParams.mDoNothing)
        return true; 

#ifdef DEBUG_CC
    mStats.mForgetNode++;

    if (mParams.mLogPointers) {
        if (!mPtrLog)
            mPtrLog = fopen("pointer_log", "w");
        fprintf(mPtrLog, "F %p\n", static_cast<void*>(n));
    }
#endif

    mPurpleBuf.RemoveCompatObject(n);
    return true;
}

nsPurpleBufferEntry*
nsCycleCollector::Suspect2(nsISupports *n)
{
    AbortIfOffMainThreadIfCheckFast();

    
    
    

    if (mScanInProgress)
        return nullptr;

    NS_ASSERTION(nsCycleCollector_isScanSafe(n),
                 "suspected a non-scansafe pointer");

    if (mParams.mDoNothing)
        return nullptr;

#ifdef DEBUG_CC
    mStats.mSuspectNode++;

    if (nsCycleCollector_shouldSuppress(n))
        return nullptr;

    if (mParams.mLogPointers) {
        if (!mPtrLog)
            mPtrLog = fopen("pointer_log", "w");
        fprintf(mPtrLog, "S %p\n", static_cast<void*>(n));
    }
#endif

    
    return mPurpleBuf.Put(n);
}


bool
nsCycleCollector::Forget2(nsPurpleBufferEntry *e)
{
    AbortIfOffMainThreadIfCheckFast();

    
    
    

    if (mScanInProgress)
        return false;

#ifdef DEBUG_CC
    LogPurpleRemoval(e->mObject);
#endif

    mPurpleBuf.Remove(e);
    return true;
}

#ifdef DEBUG_CC
void
nsCycleCollector_logPurpleRemoval(void* aObject)
{
    if (sCollector) {
        sCollector->LogPurpleRemoval(aObject);
    }
}

void
nsCycleCollector::LogPurpleRemoval(void* aObject)
{
    AbortIfOffMainThreadIfCheckFast();

    mStats.mForgetNode++;

    if (mParams.mLogPointers) {
        if (!mPtrLog)
            mPtrLog = fopen("pointer_log", "w");
        fprintf(mPtrLog, "F %p\n", aObject);
    }
    mPurpleBuf.mNormalObjects.RemoveEntry(aObject);
}
#endif







void
nsCycleCollector::GCIfNeeded(bool aForceGC)
{
    NS_ASSERTION(NS_IsMainThread(),
                 "nsCycleCollector::GCIfNeeded() must be called on the main thread.");

    if (mParams.mDoNothing)
        return;

    if (!mJSRuntime)
        return;

    if (!aForceGC) {
        bool needGC = mJSRuntime->NeedCollect();
        
        Telemetry::Accumulate(Telemetry::CYCLE_COLLECTOR_NEED_GC, needGC);
        if (!needGC)
            return;
        if (mResults)
            mResults->mForcedGC = true;
    }

    TimeLog timeLog;

    
    
    
    mJSRuntime->Collect(aForceGC ? js::gcreason::SHUTDOWN_CC : js::gcreason::CC_FORCED);
    timeLog.Checkpoint("GC()");
}

bool
nsCycleCollector::PrepareForCollection(nsCycleCollectorResults *aResults,
                                       nsTArray<PtrInfo*> *aWhiteNodes)
{
    
    if (mCollectionInProgress)
        return false;

    TimeLog timeLog;

    mCollectionStart = TimeStamp::Now();
    mVisitedRefCounted = 0;
    mVisitedGCed = 0;

    mCollectionInProgress = true;

    nsCOMPtr<nsIObserverService> obs =
        mozilla::services::GetObserverService();
    if (obs)
        obs->NotifyObservers(nullptr, "cycle-collector-begin", nullptr);

    mFollowupCollection = false;

    mResults = aResults;
    mWhiteNodes = aWhiteNodes;

    timeLog.Checkpoint("PrepareForCollection()");

    return true;
}

void
nsCycleCollector::CleanupAfterCollection()
{
    mWhiteNodes = nullptr;
    mCollectionInProgress = false;

#ifdef XP_OS2
    
    
    
    _heapmin();
#endif

    uint32_t interval = (uint32_t) ((TimeStamp::Now() - mCollectionStart).ToMilliseconds());
#ifdef COLLECT_TIME_DEBUG
    printf("cc: total cycle collector time was %ums\n", interval);
    if (mResults) {
        printf("cc: visited %u ref counted and %u GCed objects, freed %d ref counted and %d GCed objects.\n",
               mVisitedRefCounted, mVisitedGCed,
               mResults->mFreedRefCounted, mResults->mFreedGCed);
    } else {
        printf("cc: visited %u ref counted and %u GCed objects, freed %d.\n",
               mVisitedRefCounted, mVisitedGCed, mWhiteNodeCount);
    }
    printf("cc: \n");
#endif
    if (mResults) {
        mResults->mVisitedRefCounted = mVisitedRefCounted;
        mResults->mVisitedGCed = mVisitedGCed;
        mResults = nullptr;
    }
    Telemetry::Accumulate(Telemetry::CYCLE_COLLECTOR, interval);
    Telemetry::Accumulate(Telemetry::CYCLE_COLLECTOR_VISITED_REF_COUNTED, mVisitedRefCounted);
    Telemetry::Accumulate(Telemetry::CYCLE_COLLECTOR_VISITED_GCED, mVisitedGCed);
    Telemetry::Accumulate(Telemetry::CYCLE_COLLECTOR_COLLECTED, mWhiteNodeCount);
}

void
nsCycleCollector::Collect(bool aMergeCompartments,
                          nsCycleCollectorResults *aResults,
                          uint32_t aTryCollections,
                          nsICycleCollectorListener *aListener)
{
    nsAutoTArray<PtrInfo*, 4000> whiteNodes;

    if (!PrepareForCollection(aResults, &whiteNodes))
        return;

    uint32_t totalCollections = 0;
    while (aTryCollections > totalCollections) {
        
        GCIfNeeded(true);
        if (aListener && NS_FAILED(aListener->Begin()))
            aListener = nullptr;
        if (!(BeginCollection(aMergeCompartments, aListener) &&
              FinishCollection(aListener)))
            break;

        ++totalCollections;
    }

    CleanupAfterCollection();
}

bool
nsCycleCollector::BeginCollection(bool aMergeCompartments,
                                  nsICycleCollectorListener *aListener)
{
    
    TimeLog timeLog;

    if (mParams.mDoNothing)
        return false;

    GCGraphBuilder builder(mGraph, mJSRuntime, aListener, aMergeCompartments);
    if (!builder.Initialized())
        return false;

    if (mJSRuntime) {
        mJSRuntime->BeginCycleCollection(builder);
        timeLog.Checkpoint("mJSRuntime->BeginCycleCollection()");
    }

#ifdef DEBUG_CC
    uint32_t purpleStart = builder.Count();
#endif
    mScanInProgress = true;
    SelectPurple(builder);
#ifdef DEBUG_CC
    uint32_t purpleEnd = builder.Count();

    if (purpleStart != purpleEnd) {
        if (mParams.mLogPointers && !mPtrLog)
            mPtrLog = fopen("pointer_log", "w");

        uint32_t i = 0;
        NodePool::Enumerator queue(mGraph.mNodes);
        while (i++ < purpleStart) {
            queue.GetNext();
        }
        while (i++ < purpleEnd) {
            mStats.mForgetNode++;
            if (mParams.mLogPointers)
                fprintf(mPtrLog, "F %p\n", queue.GetNext()->mPointer);
        }
    }
#endif

    timeLog.Checkpoint("SelectPurple()");

    if (builder.Count() > 0) {
        

        MarkRoots(builder);
        timeLog.Checkpoint("MarkRoots()");

        ScanRoots();
        timeLog.Checkpoint("ScanRoots()");

        mScanInProgress = false;

        if (aListener) {
            aListener->BeginResults();

            NodePool::Enumerator etor(mGraph.mNodes);
            while (!etor.IsDone()) {
                PtrInfo *pi = etor.GetNext();
                if (pi->mColor == black &&
                    pi->mRefCount > 0 && pi->mRefCount < PR_UINT32_MAX &&
                    pi->mInternalRefs != pi->mRefCount) {
                    aListener->DescribeRoot((uint64_t)pi->mPointer,
                                            pi->mInternalRefs);
                }
            }
        }

#ifdef DEBUG_CC
        if (mFollowupCollection && purpleStart != purpleEnd) {
            uint32_t i = 0;
            NodePool::Enumerator queue(mGraph.mNodes);
            while (i++ < purpleStart) {
                queue.GetNext();
            }
            while (i++ < purpleEnd) {
                PtrInfo *pi = queue.GetNext();
                if (pi->mColor == white) {
                    printf("nsCycleCollector: a later shutdown collection collected the additional\n"
                           "  suspect %p %s\n"
                           "  (which could be fixed by improving traversal)\n",
                           pi->mPointer, pi->mName);
                }
            }
        }
#endif

        if (mJSRuntime) {
            mJSRuntime->FinishTraverse();
            timeLog.Checkpoint("mJSRuntime->FinishTraverse()");
        }
    } else {
        mScanInProgress = false;
    }

    return true;
}

bool
nsCycleCollector::FinishCollection(nsICycleCollectorListener *aListener)
{
    TimeLog timeLog;
    bool collected = CollectWhite(aListener);
    timeLog.Checkpoint("CollectWhite()");

#ifdef DEBUG_CC
    mStats.mCollection++;
    if (mParams.mReportStats)
        mStats.Dump();
#endif

    mFollowupCollection = true;

#ifdef DEBUG_CC
    uint32_t i, count = mWhiteNodes->Length();
    for (i = 0; i < count; ++i) {
        PtrInfo *pinfo = mWhiteNodes->ElementAt(i);
        if (mPurpleBuf.Exists(pinfo->mPointer)) {
            printf("nsCycleCollector: %s object @%p is still alive after\n"
                   "  calling RootAndUnlinkJSObjects, Unlink, and Unroot on"
                   " it!  This probably\n"
                   "  means the Unlink implementation was insufficient.\n",
                   pinfo->mName, pinfo->mPointer);
        }
    }
#endif

    mWhiteNodes->Clear();
    ClearGraph();
    timeLog.Checkpoint("ClearGraph()");

    mParams.mDoNothing = false;

    return collected;
}

uint32_t
nsCycleCollector::SuspectedCount()
{
    return mPurpleBuf.Count();
}

void
nsCycleCollector::Shutdown()
{
    
    

    nsCOMPtr<nsCycleCollectorLogger> listener;
    if (mParams.mLogGraphs) {
        listener = new nsCycleCollectorLogger();
    }
    Collect(false, nullptr, SHUTDOWN_COLLECTIONS(mParams), listener);

#ifdef DEBUG_CC
    GCGraphBuilder builder(mGraph, mJSRuntime, nullptr, false);
    mScanInProgress = true;
    SelectPurple(builder);
    mScanInProgress = false;
    if (builder.Count() != 0) {
        printf("Might have been able to release more cycles if the cycle collector would "
               "run once more at shutdown.\n");
    }
    ClearGraph();
#endif
    mParams.mDoNothing = true;
}

#ifdef DEBUG_CC
void
nsCycleCollector::ShouldBeFreed(nsISupports *n)
{
    if (n) {
        mExpectedGarbage.PutEntry(n);
    }
}

void
nsCycleCollector::WasFreed(nsISupports *n)
{
    if (n) {
        mExpectedGarbage.RemoveEntry(n);
    }
}
#endif






static int64_t
GetCycleCollectorSize()
{
    if (!sCollector)
        return 0;
    int64_t size = sizeof(nsCycleCollector) + 
        sCollector->mPurpleBuf.BlocksSize() +
        sCollector->mGraph.BlocksSize();
    if (sCollector->mWhiteNodes)
        size += sCollector->mWhiteNodes->Capacity() * sizeof(PtrInfo*);
    return size;
}

NS_MEMORY_REPORTER_IMPLEMENT(CycleCollector,
                             "explicit/cycle-collector",
                             KIND_HEAP,
                             UNITS_BYTES,
                             GetCycleCollectorSize,
                             "Memory used by the cycle collector.  This "
                             "includes the cycle collector structure, the "
                             "purple buffer, the graph, and the white nodes.  "
                             "The latter two are expected to be empty when the "
                             "cycle collector is idle.")







void
nsCycleCollector_registerJSRuntime(nsCycleCollectionJSRuntime *rt)
{
    static bool regMemReport = true;
    if (sCollector)
        sCollector->RegisterJSRuntime(rt);
    if (regMemReport) {
        regMemReport = false;
        NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(CycleCollector));
    }
}

void 
nsCycleCollector_forgetJSRuntime()
{
    if (sCollector)
        sCollector->ForgetJSRuntime();
}


bool
NS_CycleCollectorSuspect(nsISupports *n)
{
    if (sCollector)
        return sCollector->Suspect(n);
    return false;
}

bool
NS_CycleCollectorForget(nsISupports *n)
{
    return sCollector ? sCollector->Forget(n) : true;
}

nsPurpleBufferEntry*
NS_CycleCollectorSuspect2(nsISupports *n)
{
    if (sCollector)
        return sCollector->Suspect2(n);
    return nullptr;
}

bool
NS_CycleCollectorForget2(nsPurpleBufferEntry *e)
{
    return sCollector ? sCollector->Forget2(e) : true;
}

uint32_t
nsCycleCollector_suspectedCount()
{
    return sCollector ? sCollector->SuspectedCount() : 0;
}

#ifdef DEBUG
void
nsCycleCollector_DEBUG_shouldBeFreed(nsISupports *n)
{
#ifdef DEBUG_CC
    if (sCollector)
        sCollector->ShouldBeFreed(n);
#endif
}

void
nsCycleCollector_DEBUG_wasFreed(nsISupports *n)
{
#ifdef DEBUG_CC
    if (sCollector)
        sCollector->WasFreed(n);
#endif
}
#endif

class nsCycleCollectorRunner : public nsRunnable
{
    nsCycleCollector *mCollector;
    nsICycleCollectorListener *mListener;
    Mutex mLock;
    CondVar mRequest;
    CondVar mReply;
    bool mRunning;
    bool mShutdown;
    bool mCollected;
    bool mMergeCompartments;

public:
    NS_IMETHOD Run()
    {
        PR_SetCurrentThreadName("XPCOM CC");

#ifdef XP_WIN
        TlsSetValue(gTLSThreadIDIndex,
                    (void*) mozilla::threads::CycleCollector);
#elif defined(NS_TLS)
        gTLSThreadID = mozilla::threads::CycleCollector;
#else
        gCycleCollectorThread = PR_GetCurrentThread();
#endif

        NS_ASSERTION(NS_IsCycleCollectorThread() && !NS_IsMainThread(),
                     "Wrong thread!");

        MutexAutoLock autoLock(mLock);

        if (mShutdown)
            return NS_OK;

        mRunning = true;

        while (1) {
            mRequest.Wait();

            if (!mRunning) {
                mReply.Notify();
                return NS_OK;
            }

            mCollector->mJSRuntime->NotifyEnterCycleCollectionThread();
            mCollected = mCollector->BeginCollection(mMergeCompartments, mListener);
            mCollector->mJSRuntime->NotifyLeaveCycleCollectionThread();

            mReply.Notify();
        }

        return NS_OK;
    }

    nsCycleCollectorRunner(nsCycleCollector *collector)
        : mCollector(collector),
          mListener(nullptr),
          mLock("cycle collector lock"),
          mRequest(mLock, "cycle collector request condvar"),
          mReply(mLock, "cycle collector reply condvar"),
          mRunning(false),
          mShutdown(false),
          mCollected(false),
          mMergeCompartments(false)
    {
        NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    }

    void Collect(bool aMergeCompartments,
                 nsCycleCollectorResults *aResults,
                 nsICycleCollectorListener *aListener)
    {
        NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

        
        
        bool wantAllTraces = false;
        if (aListener) {
            aListener->GetWantAllTraces(&wantAllTraces);
        }
        mCollector->GCIfNeeded(wantAllTraces);

        MutexAutoLock autoLock(mLock);

        if (!mRunning)
            return;

        nsAutoTArray<PtrInfo*, 4000> whiteNodes;
        if (!mCollector->PrepareForCollection(aResults, &whiteNodes))
            return;

        NS_ASSERTION(!mListener, "Should have cleared this already!");
        if (aListener && NS_FAILED(aListener->Begin()))
            aListener = nullptr;
        mListener = aListener;
        mMergeCompartments = aMergeCompartments;

        if (mCollector->mJSRuntime->NotifyLeaveMainThread()) {
            mRequest.Notify();
            mReply.Wait();
            mCollector->mJSRuntime->NotifyEnterMainThread();
        } else {
            mCollected = mCollector->BeginCollection(aMergeCompartments, mListener);
        }

        mListener = nullptr;

        if (mCollected) {
            mCollector->FinishCollection(aListener);
            mCollector->CleanupAfterCollection();
        }
    }

    void Shutdown()
    {
        NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

        MutexAutoLock autoLock(mLock);

        mShutdown = true;

        if (!mRunning)
            return;

        mRunning = false;
        mRequest.Notify();
        mReply.Wait();
    }
};


static nsCycleCollectorRunner* sCollectorRunner;


static nsIThread* sCollectorThread;

nsresult
nsCycleCollector_startup()
{
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    NS_ASSERTION(!sCollector, "Forgot to call nsCycleCollector_shutdown?");

    sCollector = new nsCycleCollector();

    nsRefPtr<nsCycleCollectorRunner> runner =
        new nsCycleCollectorRunner(sCollector);

    nsCOMPtr<nsIThread> thread;
    nsresult rv = NS_NewThread(getter_AddRefs(thread), runner);
    NS_ENSURE_SUCCESS(rv, rv);

    runner.swap(sCollectorRunner);
    thread.swap(sCollectorThread);

    return rv;
}

void
nsCycleCollector_setBeforeUnlinkCallback(CC_BeforeUnlinkCallback aCB)
{
    if (sCollector) {
        sCollector->mBeforeUnlinkCB = aCB;
    }
}

void
nsCycleCollector_setForgetSkippableCallback(CC_ForgetSkippableCallback aCB)
{
    if (sCollector) {
        sCollector->mForgetSkippableCB = aCB;
    }
}

void
nsCycleCollector_forgetSkippable(bool aRemoveChildlessNodes)
{
    if (sCollector) {
        SAMPLE_LABEL("CC", "nsCycleCollector_forgetSkippable");
        TimeLog timeLog;
        sCollector->ForgetSkippable(aRemoveChildlessNodes);
        timeLog.Checkpoint("ForgetSkippable()");
    }
}

void
nsCycleCollector_collect(bool aMergeCompartments,
                         nsCycleCollectorResults *aResults,
                         nsICycleCollectorListener *aListener)
{
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    SAMPLE_LABEL("CC", "nsCycleCollector_collect");
    nsCOMPtr<nsICycleCollectorListener> listener(aListener);
    if (!aListener && sCollector && sCollector->mParams.mLogGraphs) {
        listener = new nsCycleCollectorLogger();
    }

    if (sCollectorRunner) {
        sCollectorRunner->Collect(aMergeCompartments, aResults, listener);
    } else if (sCollector) {
        sCollector->Collect(aMergeCompartments, aResults, 1, listener);
    }
}

void
nsCycleCollector_shutdownThreads()
{
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    if (sCollectorRunner) {
        nsRefPtr<nsCycleCollectorRunner> runner;
        runner.swap(sCollectorRunner);
        runner->Shutdown();
    }

    if (sCollectorThread) {
        nsCOMPtr<nsIThread> thread;
        thread.swap(sCollectorThread);
        thread->Shutdown();
    }
}

void
nsCycleCollector_shutdown()
{
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    NS_ASSERTION(!sCollectorRunner, "Should have finished before!");
    NS_ASSERTION(!sCollectorThread, "Should have finished before!");

    if (sCollector) {
        SAMPLE_LABEL("CC", "nsCycleCollector_shutdown");
        sCollector->Shutdown();
        delete sCollector;
        sCollector = nullptr;
    }
}
