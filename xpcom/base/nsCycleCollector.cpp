






















































































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
#include "nsIFile.h"
#include "nsDirectoryServiceDefs.h"
#include "nsMemoryInfoDumper.h"
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

#include "mozilla/CondVar.h"
#include "mozilla/Likely.h"
#include "mozilla/Mutex.h"
#include "mozilla/StandardInteger.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;



#define DEFAULT_SHUTDOWN_COLLECTIONS 5
#define SHUTDOWN_COLLECTIONS(params) DEFAULT_SHUTDOWN_COLLECTIONS

#if defined(XP_WIN)

extern DWORD gTLSThreadIDIndex;
#elif defined(NS_TLS)

extern NS_TLS mozilla::threads::ID gTLSThreadID;
#else
PRThread* gCycleCollectorThread = nullptr;
#endif



















MOZ_NEVER_INLINE void
CC_AbortIfNull(void *ptr)
{
    if (!ptr)
        MOZ_CRASH();
}




struct nsCycleCollectorParams
{
    bool mLogAll;
    bool mLogShutdown;
    bool mAllTracesAtShutdown;
    bool mDoNothing;
    
    nsCycleCollectorParams() :
        mLogAll      (PR_GetEnv("XPCOM_CC_LOG_ALL") != NULL),
        mLogShutdown (PR_GetEnv("XPCOM_CC_LOG_SHUTDOWN") != NULL),
        mAllTracesAtShutdown (PR_GetEnv("XPCOM_CC_ALL_TRACES_AT_SHUTDOWN") != NULL),
        mDoNothing   (false)
    {
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
                if (!b) {
                    
                    NS_NOTREACHED("out of memory, ignoring edges");
                    return;
                }
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

    size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const {
        size_t n = 0;
        Block *b = Blocks();
        while (b) {
            n += aMallocSizeOf(b);
            b = b->Next();
        }
        return n;
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

    PtrInfo(void *aPointer, nsCycleCollectionParticipant *aParticipant)
        : mPointer(aPointer),
          mParticipant(aParticipant),
          mColor(grey),
          mInternalRefs(0),
          mRefCount(0),
          mFirstChild()
    {
        MOZ_ASSERT(aParticipant);
    }

    
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
                Block *block;
                if (!(*mNextBlock = block =
                        static_cast<Block*>(NS_Alloc(sizeof(Block)))))
                    return nullptr;
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
        Block *mFirstBlock, *mCurBlock;
        
        
        PtrInfo *mNext, *mBlockEnd, *&mLast;
    };

    size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const {
        
        
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

    GCGraph() : mRootCount(0) {
    }
    ~GCGraph() {
    }

    void SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                             size_t *aNodesSize, size_t *aEdgesSize) const {
        *aNodesSize = mNodes.SizeOfExcludingThis(aMallocSizeOf);
        *aEdgesSize = mEdges.SizeOfExcludingThis(aMallocSizeOf);

        
        
    }
};



typedef nsTHashtable<nsPtrHashKey<const void> > PointerSet;

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

struct nsPurpleBuffer
{
private:
    struct Block {
        Block *mNext;
        
        
        
        
        
        nsPurpleBufferEntry mEntries[1365];

        Block() : mNext(nullptr) {
            
            MOZ_STATIC_ASSERT(
                sizeof(Block) == 16384 ||       
                sizeof(Block) == 32768,         
                "ill-sized nsPurpleBuffer::Block"
            );
        }
        void StaticAsserts();
    };
public:
    
    

    nsCycleCollectorParams &mParams;
    uint32_t mCount;
    Block mFirstBlock;
    nsPurpleBufferEntry *mFreeList;

    nsPurpleBuffer(nsCycleCollectorParams &params)
        : mParams(params)
    {
        InitBlocks();
    }

    ~nsPurpleBuffer()
    {
        FreeBlocks();
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

    void UnmarkRemainingPurple(Block *b)
    {
        for (nsPurpleBufferEntry *e = b->mEntries,
                              *eEnd = ArrayEnd(b->mEntries);
             e != eEnd; ++e) {
            if (!(uintptr_t(e->mObject) & uintptr_t(1))) {
                
                
                if (e->mObject) {
                    void *obj = e->mObject;
                    nsCycleCollectionParticipant *cp = e->mParticipant;
                    CanonicalizeParticipant(&obj, &cp);
                    cp->UnmarkIfPurple(obj);
                }

                if (--mCount == 0)
                    break;
            }
        }
    }

    void SelectPointers(GCGraphBuilder &builder);

    
    
    
    
    
    
    
    void RemoveSkippable(bool removeChildlessNodes);

    nsPurpleBufferEntry* NewEntry()
    {
        if (!mFreeList) {
            Block *b = new Block;
            if (!b) {
                return nullptr;
            }
            StartBlock(b);

            
            b->mNext = mFirstBlock.mNext;
            mFirstBlock.mNext = b;
        }

        nsPurpleBufferEntry *e = mFreeList;
        mFreeList = (nsPurpleBufferEntry*)
            (uintptr_t(mFreeList->mNextInFreeList) & ~uintptr_t(1));
        return e;
    }

    nsPurpleBufferEntry* Put(void *p, nsCycleCollectionParticipant *cp)
    {
        nsPurpleBufferEntry *e = NewEntry();
        if (!e) {
            return nullptr;
        }

        ++mCount;

        e->mObject = p;
        e->mParticipant = cp;
        e->mNotPurple = false;

        
        return e;
    }

    void Remove(nsPurpleBufferEntry *e)
    {
        MOZ_ASSERT(mCount != 0, "must have entries");

        e->mNextInFreeList =
            (nsPurpleBufferEntry*)(uintptr_t(mFreeList) | uintptr_t(1));
        mFreeList = e;

        --mCount;
    }

    uint32_t Count() const
    {
        return mCount;
    }

    size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
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
AddPurpleRoot(GCGraphBuilder &builder, void *root, nsCycleCollectionParticipant *cp);

void
nsPurpleBuffer::SelectPointers(GCGraphBuilder &aBuilder)
{
    
    for (Block *b = &mFirstBlock; b; b = b->mNext) {
        for (nsPurpleBufferEntry *e = b->mEntries,
                              *eEnd = ArrayEnd(b->mEntries);
            e != eEnd; ++e) {
            if (!(uintptr_t(e->mObject) & uintptr_t(1))) {
                
                
                if (e->mObject && e->mNotPurple) {
                    void* o = e->mObject;
                    nsCycleCollectionParticipant* cp = e->mParticipant;
                    CanonicalizeParticipant(&o, &cp);
                    cp->UnmarkIfPurple(o);
                    Remove(e);
                } else if (!e->mObject || AddPurpleRoot(aBuilder, e->mObject,
                                                        e->mParticipant)) {
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

    nsCOMPtr<nsIMemoryMultiReporter> mReporter;

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

    nsPurpleBufferEntry* Suspect2(void *n, nsCycleCollectionParticipant *cp);
    bool Forget2(nsPurpleBufferEntry *e);

    void Collect(bool aMergeZones,
                 nsCycleCollectorResults *aResults,
                 uint32_t aTryCollections,
                 nsICycleCollectorListener *aListener);

    
    bool PrepareForCollection(nsCycleCollectorResults *aResults,
                              nsTArray<PtrInfo*> *aWhiteNodes);
    void FixGrayBits(bool aForceGC);
    void CleanupAfterCollection();

    
    bool BeginCollection(bool aMergeZones, nsICycleCollectorListener *aListener);
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

    void SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                             size_t *aObjectSize,
                             size_t *aGraphNodesSize,
                             size_t *aGraphEdgesSize,
                             size_t *aWhiteNodeSize,
                             size_t *aPurpleBufferSize) const;
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

static void
Fault(const char *msg, PtrInfo *pi)
{
    Fault(msg, pi->mPointer);
}

static inline void
AbortIfOffMainThreadIfCheckFast()
{
#if defined(XP_WIN) || defined(NS_TLS)
    if (!NS_IsMainThread() && !NS_IsCycleCollectorThread()) {
        NS_RUNTIMEABORT("Main-thread-only object used off the main thread");
    }
#endif
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

    NS_IMETHOD Begin()
    {
        mCurrentAddress.AssignLiteral("0x");
        mDescribers.Clear();
        mNextIndex = 0;
        if (mDisableLog) {
            return NS_OK;
        }

        
        
        
        
        
        nsCOMPtr<nsIFile> gcLogFile = CreateTempFile("incomplete-gc-edges");
        NS_ENSURE_STATE(gcLogFile);

        
        FILE* gcLogANSIFile = nullptr;
        gcLogFile->OpenANSIFileDesc("w", &gcLogANSIFile);
        NS_ENSURE_STATE(gcLogANSIFile);
        xpc::DumpJSHeap(gcLogANSIFile);
        fclose(gcLogANSIFile);

        
        nsCOMPtr<nsIFile> gcLogFileFinalDestination =
            CreateTempFile("gc-edges");
        NS_ENSURE_STATE(gcLogFileFinalDestination);

        nsAutoString gcLogFileFinalDestinationName;
        gcLogFileFinalDestination->GetLeafName(gcLogFileFinalDestinationName);
        NS_ENSURE_STATE(!gcLogFileFinalDestinationName.IsEmpty());

        gcLogFile->MoveTo( nullptr, gcLogFileFinalDestinationName);

        
        nsCOMPtr<nsIConsoleService> cs =
            do_GetService(NS_CONSOLESERVICE_CONTRACTID);
        if (cs) {
            nsAutoString gcLogPath;
            gcLogFileFinalDestination->GetPath(gcLogPath);

            nsString msg = NS_LITERAL_STRING("Garbage Collector log dumped to ") +
                           gcLogPath;
            cs->LogStringMessage(msg.get());
        }

        
        
        mOutFile = CreateTempFile("incomplete-cc-edges");
        NS_ENSURE_STATE(mOutFile);
        MOZ_ASSERT(!mStream);
        mOutFile->OpenANSIFileDesc("w", &mStream);
        NS_ENSURE_STATE(mStream);

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
            MOZ_ASSERT(mStream);
            MOZ_ASSERT(mOutFile);

            fclose(mStream);
            mStream = nullptr;

            
            nsCOMPtr<nsIFile> logFileFinalDestination =
                CreateTempFile("cc-edges");
            NS_ENSURE_STATE(logFileFinalDestination);

            nsAutoString logFileFinalDestinationName;
            logFileFinalDestination->GetLeafName(logFileFinalDestinationName);
            NS_ENSURE_STATE(!logFileFinalDestinationName.IsEmpty());

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
            }
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
    






    already_AddRefed<nsIFile>
    CreateTempFile(const char* aPrefix)
    {
        nsPrintfCString filename("%s.%d%s%s.log",
            aPrefix,
            base::GetCurrentProcId(),
            mFilenameIdentifier.IsEmpty() ? "" : ".",
            NS_ConvertUTF16toUTF8(mFilenameIdentifier).get());

        
        
        nsCOMPtr<nsIFile> logFile;
        if (char* env = PR_GetEnv("MOZ_CC_LOG_DIRECTORY")) {
            NS_NewNativeLocalFile(nsCString(env),  true,
                                  getter_AddRefs(logFile));
        }
        nsresult rv = nsMemoryInfoDumper::OpenTempFile(filename,
                                                       getter_AddRefs(logFile));
        NS_ENSURE_SUCCESS(rv, nullptr);

        return logFile.forget();
    }

    FILE *mStream;
    nsCOMPtr<nsIFile> mOutFile;
    bool mWantAllTraces;
    bool mDisableLog;
    bool mWantAfterProcessing;
    nsString mFilenameIdentifier;
    nsCString mCurrentAddress; 
    nsTArray<CCGraphDescriber> mDescribers;
    uint32_t mNextIndex;
};

NS_IMPL_ISUPPORTS1(nsCycleCollectorLogger, nsICycleCollectorListener)

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
    nsCycleCollectionParticipant *mJSZoneParticipant;
    nsCString mNextEdgeName;
    nsICycleCollectorListener *mListener;
    bool mMergeZones;

public:
    GCGraphBuilder(GCGraph &aGraph,
                   nsCycleCollectionJSRuntime *aJSRuntime,
                   nsICycleCollectorListener *aListener,
                   bool aMergeZones);
    ~GCGraphBuilder();
    bool Initialized();

    uint32_t Count() const { return mPtrToNodeMap.entryCount; }

    PtrInfo* AddNode(void *s, nsCycleCollectionParticipant *aParticipant);
    PtrInfo* AddWeakMapNode(void* node);
    void Traverse(PtrInfo* aPtrInfo);
    void SetLastChild();

private:
    void DescribeNode(uint32_t refCount, const char *objName)
    {
        mCurrPi->mRefCount = refCount;
    }

public:
    
    NS_IMETHOD_(void) DescribeRefCountedNode(nsrefcnt refCount,
                                             const char *objName);
    NS_IMETHOD_(void) DescribeGCedNode(bool isMarked, const char *objName);

    NS_IMETHOD_(void) NoteXPCOMRoot(nsISupports *root);
    NS_IMETHOD_(void) NoteJSRoot(void *root);
    NS_IMETHOD_(void) NoteNativeRoot(void *root, nsCycleCollectionParticipant *participant);

    NS_IMETHOD_(void) NoteXPCOMChild(nsISupports *child);
    NS_IMETHOD_(void) NoteJSChild(void *child);
    NS_IMETHOD_(void) NoteNativeChild(void *child,
                                      nsCycleCollectionParticipant *participant);

    NS_IMETHOD_(void) NoteNextEdgeName(const char* name);
    NS_IMETHOD_(void) NoteWeakMapping(void *map, void *key, void *kdelegate, void *val);

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
                               nsCycleCollectionJSRuntime *aJSRuntime,
                               nsICycleCollectorListener *aListener,
                               bool aMergeZones)
    : mNodeBuilder(aGraph.mNodes),
      mEdgeBuilder(aGraph.mEdges),
      mWeakMaps(aGraph.mWeakMaps),
      mJSParticipant(nullptr),
      mJSZoneParticipant(xpc_JSZoneParticipant()),
      mListener(aListener),
      mMergeZones(aMergeZones)
{
    if (!PL_DHashTableInit(&mPtrToNodeMap, &PtrNodeOps, nullptr,
                           sizeof(PtrToNodeEntry), 32768))
        mPtrToNodeMap.ops = nullptr;

    if (aJSRuntime) {
        mJSParticipant = aJSRuntime->GetParticipant();
    }

    uint32_t flags = 0;
    if (!flags && mListener) {
        flags = nsCycleCollectionTraversalCallback::WANT_DEBUG_INFO;
        bool all = false;
        mListener->GetWantAllTraces(&all);
        if (all) {
            flags |= nsCycleCollectionTraversalCallback::WANT_ALL_TRACES;
        }
    }

    mFlags |= flags;

    mMergeZones = mMergeZones && MOZ_LIKELY(!WantAllTraces());
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
    sCollector->mVisitedRefCounted++;

    if (mListener) {
        mListener->NoteRefCountedObject((uint64_t)mCurrPi->mPointer, refCount,
                                        objName);
    }

    DescribeNode(refCount, objName);
}

NS_IMETHODIMP_(void)
GCGraphBuilder::DescribeGCedNode(bool isMarked, const char *objName)
{
    uint32_t refCount = isMarked ? UINT32_MAX : 0;
    sCollector->mVisitedGCed++;

    if (mListener) {
        mListener->NoteGCedObject((uint64_t)mCurrPi->mPointer, isMarked,
                                  objName);
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
    
    
    WeakMapping *mapping = mWeakMaps.AppendElement();
    mapping->mMap = map ? AddWeakMapNode(map) : nullptr;
    mapping->mKey = key ? AddWeakMapNode(key) : nullptr;
    mapping->mKeyDelegate = kdelegate ? AddWeakMapNode(kdelegate) : mapping->mKey;
    mapping->mVal = val ? AddWeakMapNode(val) : nullptr;
}

static bool
AddPurpleRoot(GCGraphBuilder &builder, void *root, nsCycleCollectionParticipant *cp)
{
    CanonicalizeParticipant(&root, &cp);

    if (builder.WantAllTraces() || !cp->CanSkipInCC(root)) {
        PtrInfo *pinfo = builder.AddNode(root, cp);
        if (!pinfo) {
            return false;
        }
    }

    cp->UnmarkIfPurple(root);

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
                                       const char *objname) {}
    NS_IMETHOD_(void) NoteXPCOMRoot(nsISupports *root) {}
    NS_IMETHOD_(void) NoteJSRoot(void *root) {}
    NS_IMETHOD_(void) NoteNativeRoot(void *root,
                                     nsCycleCollectionParticipant *helper) {}
    NS_IMETHOD_(void) NoteNextEdgeName(const char* name) {}
    NS_IMETHOD_(void) NoteWeakMapping(void *map, void *key, void *kdelegate, void *val) {}
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

void
nsPurpleBuffer::RemoveSkippable(bool removeChildlessNodes)
{
    
    for (Block *b = &mFirstBlock; b; b = b->mNext) {
        for (nsPurpleBufferEntry *e = b->mEntries,
                              *eEnd = ArrayEnd(b->mEntries);
            e != eEnd; ++e) {
            if (!(uintptr_t(e->mObject) & uintptr_t(1))) {
                
                
                if (e->mObject) {
                    void *o = e->mObject;
                    nsCycleCollectionParticipant *cp = e->mParticipant;
                    CanonicalizeParticipant(&o, &cp);
                    if (!e->mNotPurple && !cp->CanSkip(o, false) &&
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
        } else {
            GraphWalker<ScanBlackVisitor>(ScanBlackVisitor(mWhiteNodeCount)).Walk(pi);
            MOZ_ASSERT(pi->mColor == black,
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
            uint32_t kdColor = wm->mKeyDelegate ? wm->mKeyDelegate->mColor : black;
            uint32_t vColor = wm->mVal ? wm->mVal->mColor : black;

            
            
            
            
            MOZ_ASSERT(mColor != grey, "Uncolored weak map");
            MOZ_ASSERT(kColor != grey, "Uncolored weak map key");
            MOZ_ASSERT(kdColor != grey, "Uncolored weak map key delegate");
            MOZ_ASSERT(vColor != grey, "Uncolored weak map value");

            if (mColor == black && kColor != black && kdColor == black) {
                GraphWalker<ScanBlackVisitor>(ScanBlackVisitor(mWhiteNodeCount)).Walk(wm->mKey);
                anyChanged = true;
            }

            if (mColor == black && kColor == black && vColor != black) {
                GraphWalker<ScanBlackVisitor>(ScanBlackVisitor(mWhiteNodeCount)).Walk(wm->mVal);
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
}






bool
nsCycleCollector::CollectWhite(nsICycleCollectorListener *aListener)
{
    
    
    
    
    
    
    
    
    
    
    
    
    

    nsresult rv;
    TimeLog timeLog;

    MOZ_ASSERT(mWhiteNodes->IsEmpty(),
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
    MOZ_ASSERT(numWhiteGCed <= count,
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

    if (aListener) {
        for (uint32_t i = 0; i < count; ++i) {
            PtrInfo *pinfo = mWhiteNodes->ElementAt(i);
            aListener->DescribeGarbage((uint64_t)pinfo->mPointer);
        }
        aListener->End();
    }

    for (uint32_t i = 0; i < count; ++i) {
        PtrInfo *pinfo = mWhiteNodes->ElementAt(i);
#ifdef DEBUG
        if (mJSRuntime) {
            mJSRuntime->SetObjectToUnlink(pinfo->mPointer);
        }
#endif
        rv = pinfo->mParticipant->Unlink(pinfo->mPointer);
#ifdef DEBUG
        if (mJSRuntime) {
            mJSRuntime->SetObjectToUnlink(nullptr);
            mJSRuntime->AssertNoObjectsToTrace(pinfo->mPointer);
        }
#endif
        if (NS_FAILED(rv)) {
            Fault("Failed unlink call while unlinking", pinfo);
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

    return count > 0;
}






class CycleCollectorMultiReporter MOZ_FINAL : public nsIMemoryMultiReporter
{
  public:
    CycleCollectorMultiReporter(nsCycleCollector* aCollector)
      : mCollector(aCollector)
    {}

    NS_DECL_ISUPPORTS

    NS_IMETHOD GetName(nsACString& name)
    {
        name.AssignLiteral("cycle-collector");
        return NS_OK;
    }

    NS_IMETHOD CollectReports(nsIMemoryMultiReporterCallback* aCb,
                              nsISupports* aClosure)
    {
        size_t objectSize, graphNodesSize, graphEdgesSize, whiteNodesSize,
               purpleBufferSize;
        mCollector->SizeOfIncludingThis(MallocSizeOf,
                                        &objectSize, &graphNodesSize,
                                        &graphEdgesSize, &whiteNodesSize,
                                        &purpleBufferSize);

    #define REPORT(_path, _amount, _desc)                                     \
        do {                                                                  \
            size_t amount = _amount;  /* evaluate |_amount| only once */      \
            if (amount > 0) {                                                 \
                nsresult rv;                                                  \
                rv = aCb->Callback(EmptyCString(), NS_LITERAL_CSTRING(_path), \
                                   nsIMemoryReporter::KIND_HEAP,              \
                                   nsIMemoryReporter::UNITS_BYTES, _amount,   \
                                   NS_LITERAL_CSTRING(_desc), aClosure);      \
                NS_ENSURE_SUCCESS(rv, rv);                                    \
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

        REPORT("explicit/cycle-collector/white-nodes", whiteNodesSize,
               "Memory used for the cycle collector's white nodes array. "
               "This should be zero when the collector is idle.");

        REPORT("explicit/cycle-collector/purple-buffer", purpleBufferSize,
               "Memory used for the cycle collector's purple buffer.");

    #undef REPORT

        return NS_OK;
    }

    NS_IMETHOD GetExplicitNonHeap(int64_t* n)
    {
        
        *n = 0;
        return NS_OK;
    }

  private:
    NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(MallocSizeOf)

    nsCycleCollector* mCollector;
};

NS_IMPL_ISUPPORTS1(CycleCollectorMultiReporter, nsIMemoryMultiReporter)






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
    mReporter(nullptr),
    mPurpleBuf(mParams)
{
}


nsCycleCollector::~nsCycleCollector()
{
    NS_UnregisterMemoryMultiReporter(mReporter);
}


void
nsCycleCollector::RegisterJSRuntime(nsCycleCollectionJSRuntime *aJSRuntime)
{
    if (mParams.mDoNothing)
        return;

    if (mJSRuntime)
        Fault("multiple registrations of cycle collector JS runtime", aJSRuntime);

    mJSRuntime = aJSRuntime;

    
    
    
    static bool registered = false;
    if (!registered) {
        NS_RegisterMemoryMultiReporter(new CycleCollectorMultiReporter(this));
        registered = true;
    }
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

#ifdef DEBUG
static bool
nsCycleCollector_isScanSafe(void *s, nsCycleCollectionParticipant *cp)
{
    if (!s)
        return false;

    if (cp)
        return true;

    nsXPCOMCycleCollectionParticipant *xcp;
    ToParticipant(static_cast<nsISupports*>(s), &xcp);

    return xcp != nullptr;
}
#endif

nsPurpleBufferEntry*
nsCycleCollector::Suspect2(void *n, nsCycleCollectionParticipant *cp)
{
    AbortIfOffMainThreadIfCheckFast();

    
    
    

    if (mScanInProgress)
        return nullptr;

    MOZ_ASSERT(nsCycleCollector_isScanSafe(n, cp),
               "suspected a non-scansafe pointer");

    if (mParams.mDoNothing)
        return nullptr;

    
    return mPurpleBuf.Put(n, cp);
}


bool
nsCycleCollector::Forget2(nsPurpleBufferEntry *e)
{
    AbortIfOffMainThreadIfCheckFast();

    
    
    

    if (mScanInProgress)
        return false;

    mPurpleBuf.Remove(e);
    return true;
}







void
nsCycleCollector::FixGrayBits(bool aForceGC)
{
    MOZ_ASSERT(NS_IsMainThread(),
               "nsCycleCollector::FixGrayBits() must be called on the main thread.");

    if (mParams.mDoNothing)
        return;

    if (!mJSRuntime)
        return;

    if (!aForceGC) {
        mJSRuntime->FixWeakMappingGrayBits();

        bool needGC = mJSRuntime->NeedCollect();
        
        Telemetry::Accumulate(Telemetry::CYCLE_COLLECTOR_NEED_GC, needGC);
        if (!needGC)
            return;
        if (mResults)
            mResults->mForcedGC = true;
    }

    TimeLog timeLog;

    
    
    
    mJSRuntime->Collect(aForceGC ? JS::gcreason::SHUTDOWN_CC : JS::gcreason::CC_FORCED);
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
nsCycleCollector::Collect(bool aMergeZones,
                          nsCycleCollectorResults *aResults,
                          uint32_t aTryCollections,
                          nsICycleCollectorListener *aListener)
{
    nsAutoTArray<PtrInfo*, 4000> whiteNodes;

    if (!PrepareForCollection(aResults, &whiteNodes))
        return;

    uint32_t totalCollections = 0;
    while (aTryCollections > totalCollections) {
        
        FixGrayBits(true);
        if (aListener && NS_FAILED(aListener->Begin()))
            aListener = nullptr;
        if (!(BeginCollection(aMergeZones, aListener) &&
              FinishCollection(aListener)))
            break;

        ++totalCollections;
    }

    CleanupAfterCollection();
}

bool
nsCycleCollector::BeginCollection(bool aMergeZones,
                                  nsICycleCollectorListener *aListener)
{
    
    TimeLog timeLog;

    if (mParams.mDoNothing)
        return false;

    GCGraphBuilder builder(mGraph, mJSRuntime, aListener, aMergeZones);
    if (!builder.Initialized())
        return false;

    if (mJSRuntime) {
        mJSRuntime->BeginCycleCollection(builder);
        timeLog.Checkpoint("mJSRuntime->BeginCycleCollection()");
    }

    mScanInProgress = true;
    SelectPurple(builder);

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
                    pi->mRefCount > 0 && pi->mRefCount < UINT32_MAX &&
                    pi->mInternalRefs != pi->mRefCount) {
                    aListener->DescribeRoot((uint64_t)pi->mPointer,
                                            pi->mInternalRefs);
                }
            }
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

    mFollowupCollection = true;

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
#ifndef DEBUG
    if (PR_GetEnv("XPCOM_CC_RUN_DURING_SHUTDOWN"))
#endif
    {
        nsCOMPtr<nsCycleCollectorLogger> listener;
        if (mParams.mLogAll || mParams.mLogShutdown) {
            listener = new nsCycleCollectorLogger();
            if (mParams.mAllTracesAtShutdown) {
                listener->SetAllTraces();
            }
        }
        Collect(false, nullptr,  SHUTDOWN_COLLECTIONS(mParams), listener);
    }

    mParams.mDoNothing = true;
}

void
nsCycleCollector::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                      size_t *aObjectSize,
                                      size_t *aGraphNodesSize,
                                      size_t *aGraphEdgesSize,
                                      size_t *aWhiteNodeSize,
                                      size_t *aPurpleBufferSize) const
{
    *aObjectSize = aMallocSizeOf(this);

    mGraph.SizeOfExcludingThis(aMallocSizeOf, aGraphNodesSize, aGraphEdgesSize);

    
    
    *aWhiteNodeSize = mWhiteNodes
                    ? mWhiteNodes->SizeOfIncludingThis(aMallocSizeOf)
                    : 0;

    *aPurpleBufferSize = mPurpleBuf.SizeOfExcludingThis(aMallocSizeOf);

    
    
    
    
}







void
nsCycleCollector_registerJSRuntime(nsCycleCollectionJSRuntime *rt)
{
    if (sCollector)
        sCollector->RegisterJSRuntime(rt);
}

void
nsCycleCollector_forgetJSRuntime()
{
    if (sCollector)
        sCollector->ForgetJSRuntime();
}

nsPurpleBufferEntry*
NS_CycleCollectorSuspect2(void *n, nsCycleCollectionParticipant *cp)
{
    if (sCollector)
        return sCollector->Suspect2(n, cp);
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
    bool mMergeZones;

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

        MOZ_ASSERT(NS_IsCycleCollectorThread() && !NS_IsMainThread(),
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
            mCollected = mCollector->BeginCollection(mMergeZones, mListener);
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
          mMergeZones(false)
    {
        MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
    }

    void Collect(bool aMergeZones,
                 nsCycleCollectorResults *aResults,
                 nsICycleCollectorListener *aListener)
    {
        MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");

        
        
        bool wantAllTraces = false;
        if (aListener) {
            aListener->GetWantAllTraces(&wantAllTraces);
        }
        mCollector->FixGrayBits(wantAllTraces);

        MutexAutoLock autoLock(mLock);

        if (!mRunning)
            return;

        nsAutoTArray<PtrInfo*, 4000> whiteNodes;
        if (!mCollector->PrepareForCollection(aResults, &whiteNodes))
            return;

        MOZ_ASSERT(!mListener, "Should have cleared this already!");
        if (aListener && NS_FAILED(aListener->Begin()))
            aListener = nullptr;
        mListener = aListener;
        mMergeZones = aMergeZones;

        if (mCollector->mJSRuntime->NotifyLeaveMainThread()) {
            mRequest.Notify();
            mReply.Wait();
            mCollector->mJSRuntime->NotifyEnterMainThread();
        } else {
            mCollected = mCollector->BeginCollection(aMergeZones, mListener);
        }

        mListener = nullptr;

        if (mCollected) {
            mCollector->FinishCollection(aListener);
            mCollector->CleanupAfterCollection();
        }
    }

    void Shutdown()
    {
        MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");

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
    MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
    MOZ_ASSERT(!sCollector, "Forgot to call nsCycleCollector_shutdown?");

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
        PROFILER_LABEL("CC", "nsCycleCollector_forgetSkippable");
        TimeLog timeLog;
        sCollector->ForgetSkippable(aRemoveChildlessNodes);
        timeLog.Checkpoint("ForgetSkippable()");
    }
}

void
nsCycleCollector_collect(bool aMergeZones,
                         nsCycleCollectorResults *aResults,
                         nsICycleCollectorListener *aListener)
{
    MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
    PROFILER_LABEL("CC", "nsCycleCollector_collect");
    nsCOMPtr<nsICycleCollectorListener> listener(aListener);
    if (!aListener && sCollector && sCollector->mParams.mLogAll) {
        listener = new nsCycleCollectorLogger();
    }

    if (sCollectorRunner) {
        sCollectorRunner->Collect(aMergeZones, aResults, listener);
    } else if (sCollector) {
        sCollector->Collect(aMergeZones, aResults, 1, listener);
    }
}

void
nsCycleCollector_shutdownThreads()
{
    MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
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
    MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
    MOZ_ASSERT(!sCollectorRunner, "Should have finished before!");
    MOZ_ASSERT(!sCollectorThread, "Should have finished before!");

    if (sCollector) {
        PROFILER_LABEL("CC", "nsCycleCollector_shutdown");
        sCollector->Shutdown();
        delete sCollector;
        sCollector = nullptr;
    }
}
