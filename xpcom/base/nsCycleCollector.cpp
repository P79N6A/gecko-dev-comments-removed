























































































































#ifndef __MINGW32__
#ifdef WIN32
#include <crtdbg.h>
#include <errno.h>
#endif
#endif

#include "nsCycleCollectionParticipant.h"
#include "nsIProgrammingLanguage.h"
#include "nsBaseHashtable.h"
#include "nsHashKeys.h"
#include "nsDeque.h"
#include "nsCycleCollector.h"
#include "nsThreadUtils.h"
#include "prenv.h"
#include "prprf.h"
#include "plstr.h"
#include "prtime.h"
#include "nsPrintfCString.h"
#include "nsTArray.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"

#include <stdio.h>
#ifdef WIN32
#include <io.h>
#include <process.h>
#endif

#define DEFAULT_SHUTDOWN_COLLECTIONS 5
#ifdef DEBUG_CC
#define SHUTDOWN_COLLECTIONS(params) params.mShutdownCollections
#else
#define SHUTDOWN_COLLECTIONS(params) DEFAULT_SHUTDOWN_COLLECTIONS
#endif




struct nsCycleCollectorParams
{
    PRBool mDoNothing;
#ifdef DEBUG_CC
    PRBool mReportStats;
    PRBool mHookMalloc;
    PRBool mDrawGraphs;
    PRBool mFaultIsFatal;
    PRBool mLogPointers;

    PRUint32 mShutdownCollections;
#endif
    
    PRUint32 mScanDelay;
    
    nsCycleCollectorParams() :
#ifdef DEBUG_CC
        mDoNothing     (PR_GetEnv("XPCOM_CC_DO_NOTHING") != NULL),
        mReportStats   (PR_GetEnv("XPCOM_CC_REPORT_STATS") != NULL),
        mHookMalloc    (PR_GetEnv("XPCOM_CC_HOOK_MALLOC") != NULL),
        mDrawGraphs    (PR_GetEnv("XPCOM_CC_DRAW_GRAPHS") != NULL),
        mFaultIsFatal  (PR_GetEnv("XPCOM_CC_FAULT_IS_FATAL") != NULL),
        mLogPointers   (PR_GetEnv("XPCOM_CC_LOG_POINTERS") != NULL),

        mShutdownCollections(DEFAULT_SHUTDOWN_COLLECTIONS),
#else
        mDoNothing     (PR_FALSE),
#endif

        
        
        
        
        
        
        
        

        mScanDelay(10)
    {
#ifdef DEBUG_CC
        char *s = PR_GetEnv("XPCOM_CC_SCAN_DELAY");
        if (s)
            PR_sscanf(s, "%d", &mScanDelay);
        s = PR_GetEnv("XPCOM_CC_SHUTDOWN_COLLECTIONS");
        if (s)
            PR_sscanf(s, "%d", &mShutdownCollections);
#endif
    }
};

#ifdef DEBUG_CC



struct nsCycleCollectorStats
{
    PRUint32 mFailedQI;
    PRUint32 mSuccessfulQI;

    PRUint32 mVisitedNode;
    PRUint32 mWalkedGraph;
    PRUint32 mCollectedBytes;
    PRUint32 mFreeCalls;
    PRUint32 mFreedBytes;

    PRUint32 mSetColorGrey;
    PRUint32 mSetColorBlack;
    PRUint32 mSetColorWhite;

    PRUint32 mFailedUnlink;
    PRUint32 mCollectedNode;
    PRUint32 mBumpGeneration;
    PRUint32 mZeroGeneration;

    PRUint32 mSuspectNode;
    PRUint32 mSpills;    
    PRUint32 mForgetNode;
    PRUint32 mFreedWhilePurple;
  
    PRUint32 mCollection;

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
        DUMP(mCollectedBytes);
        DUMP(mFreeCalls);
        DUMP(mFreedBytes);
    
        DUMP(mSetColorGrey);
        DUMP(mSetColorBlack);
        DUMP(mSetColorWhite);
    
        DUMP(mFailedUnlink);
        DUMP(mCollectedNode);
        DUMP(mBumpGeneration);
        DUMP(mZeroGeneration);
    
        DUMP(mSuspectNode);
        DUMP(mSpills);
        DUMP(mForgetNode);
        DUMP(mFreedWhilePurple);
    
        DUMP(mCollection);
#undef DUMP
    }
};
#endif

#ifdef DEBUG_CC
static PRBool
nsCycleCollector_shouldSuppress(nsISupports *s);
#endif





struct PtrInfo;

class EdgePool
{
public:
    
    
    
    
    

    EdgePool()
    {
        mSentinelAndBlocks[0].block = nsnull;
        mSentinelAndBlocks[1].block = nsnull;
    }

    ~EdgePool()
    {
        Block *b = Blocks();
        while (b) {
            Block *next = b->Next();
            delete b;
            b = next;
        }
    }

private:
    struct Block;
    union PtrInfoOrBlock {
        
        
        PtrInfo *ptrInfo;
        Block *block;
    };
    struct Block {
        enum { BlockSize = 64 * 1024 };

        PtrInfoOrBlock mPointers[BlockSize];
        Block() {
            mPointers[BlockSize - 2].block = nsnull; 
            mPointers[BlockSize - 1].block = nsnull; 
        }
        Block*& Next()
            { return mPointers[BlockSize - 1].block; }
        PtrInfoOrBlock* Start()
            { return &mPointers[0]; }
        PtrInfoOrBlock* End()
            { return &mPointers[BlockSize - 2]; }
    };

    
    
    PtrInfoOrBlock mSentinelAndBlocks[2];

    Block*& Blocks() { return mSentinelAndBlocks[1].block; }

public:
    class Iterator
    {
    public:
        Iterator() : mPointer(nsnull) {}
        Iterator(PtrInfoOrBlock *aPointer) : mPointer(aPointer) {}
        Iterator(const Iterator& aOther) : mPointer(aOther.mPointer) {}

        Iterator& operator++()
        {
            if (mPointer->ptrInfo == nsnull) {
                
                mPointer = (mPointer + 1)->block->mPointers;
            }
            ++mPointer;
            return *this;
        }

        PtrInfo* operator*() const
        {
            if (mPointer->ptrInfo == nsnull) {
                
                return (mPointer + 1)->block->mPointers->ptrInfo;
            }
            return mPointer->ptrInfo;
        }
        PRBool operator==(const Iterator& aOther) const
            { return mPointer == aOther.mPointer; }
        PRBool operator!=(const Iterator& aOther) const
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

};


enum NodeColor { black, white, grey };





struct PtrInfo
{
    void *mPointer;
    nsCycleCollectionParticipant *mParticipant;
    PRUint32 mColor : 2;
    PRUint32 mInternalRefs : 30;
    PRUint32 mRefCount : 31;
    PRUint32 mWasPurple : 1;
    EdgePool::Iterator mFirstChild; 
    EdgePool::Iterator mLastChild; 

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
          mWasPurple(PR_FALSE),
          mFirstChild(),
          mLastChild()
#ifdef DEBUG_CC
        , mBytes(0),
          mName(nsnull)
#endif
    {
    }

    
    PtrInfo() {}
};





class NodePool
{
private:
    enum { BlockSize = 32 * 1024 }; 

    struct Block {
        Block* mNext;
        PtrInfo mEntries[BlockSize];

        Block() : mNext(nsnull) {}
    };

public:
    NodePool()
        : mBlocks(nsnull),
          mLast(nsnull)
    {
    }

    ~NodePool()
    {
#ifdef DEBUG_CC
        {
            Enumerator queue(*this);
            while (!queue.IsDone()) {
                PL_strfree(queue.GetNext()->mName);
            }
        }
#endif
        Block *b = mBlocks;
        while (b) {
            Block *n = b->mNext;
            delete b;
            b = n;
        }
    }

    class Builder;
    friend class Builder;
    class Builder {
    public:
        Builder(NodePool& aPool)
            : mNextBlock(&aPool.mBlocks),
              mNext(aPool.mLast),
              mBlockEnd(nsnull)
        {
            NS_ASSERTION(aPool.mBlocks == nsnull && aPool.mLast == nsnull,
                         "pool not empty");
        }
        PtrInfo *Add(void *aPointer, nsCycleCollectionParticipant *aParticipant)
        {
            if (mNext == mBlockEnd) {
                Block *block;
                if (!(*mNextBlock = block = new Block()))
                    return nsnull;
                mNext = block->mEntries;
                mBlockEnd = block->mEntries + BlockSize;
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
              mCurBlock(nsnull),
              mNext(nsnull),
              mBlockEnd(nsnull),
              mLast(aPool.mLast)
        {
        }

        PRBool IsDone() const
        {
            return mNext == mLast;
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

private:
    Block *mBlocks;
    PtrInfo *mLast;
};

struct GCGraph
{
    NodePool mNodes;
    EdgePool mEdges;
    PRUint32 mRootCount;

    GCGraph() : mRootCount(0) {}
};



typedef nsTHashtable<nsVoidPtrHashKey> PointerSet;
typedef nsBaseHashtable<nsVoidPtrHashKey, PRUint32, PRUint32>
    PointerSetWithGeneration;

static void
Fault(const char *msg, const void *ptr);

struct nsPurpleBuffer
{

#define ASSOCIATIVITY 2
#define INDEX_LOW_BIT 6
#define N_INDEX_BITS 13

#define N_ENTRIES (1 << N_INDEX_BITS)
#define N_POINTERS (N_ENTRIES * ASSOCIATIVITY)
#define TOTAL_BYTES (N_POINTERS * PR_BYTES_PER_WORD)
#define INDEX_MASK PR_BITMASK(N_INDEX_BITS)
#define POINTER_INDEX(P) ((((PRUword)P) >> INDEX_LOW_BIT) & (INDEX_MASK))

#if (INDEX_LOW_BIT + N_INDEX_BITS > (8 * PR_BYTES_PER_WORD))
#error "index bit overflow"
#endif

    
    
    
    
    
    

    nsCycleCollectorParams &mParams;
#ifdef DEBUG_CC
    nsCycleCollectorStats &mStats;
#endif
    void* mCache[N_POINTERS];
    PRUint32 mCurrGen;    
    PointerSetWithGeneration mBackingStore;
    nsDeque *mTransferBuffer;
    
#ifdef DEBUG_CC
    nsPurpleBuffer(nsCycleCollectorParams &params,
                   nsCycleCollectorStats &stats) 
        : mParams(params),
          mStats(stats),
          mCurrGen(0),
          mTransferBuffer(nsnull)
    {
        Init();
    }
#else
    nsPurpleBuffer(nsCycleCollectorParams &params) 
        : mParams(params),
          mCurrGen(0),
          mTransferBuffer(nsnull)
    {
        Init();
    }
#endif

    ~nsPurpleBuffer()
    {
        memset(mCache, 0, sizeof(mCache));
        mBackingStore.Clear();
    }

    void Init()
    {
        memset(mCache, 0, sizeof(mCache));
        mBackingStore.Init();
    }

    void BumpGeneration();
    void SelectAgedPointers(nsDeque *transferBuffer);

    PRBool Exists(void *p)
    {
        PRUint32 idx = POINTER_INDEX(p);
        for (PRUint32 i = 0; i < ASSOCIATIVITY; ++i) {
            if (mCache[idx+i] == p)
                return PR_TRUE;
        }
        PRUint32 gen;
        return mBackingStore.Get(p, &gen);
    }

    void Put(void *p)
    {
        PRUint32 idx = POINTER_INDEX(p);
        for (PRUint32 i = 0; i < ASSOCIATIVITY; ++i) {
            if (!mCache[idx+i]) {
                mCache[idx+i] = p;
                return;
            }
        }
#ifdef DEBUG_CC
        mStats.mSpills++;
#endif
        SpillOne(p);
    }

    void Remove(void *p)     
    {
        PRUint32 idx = POINTER_INDEX(p);
        for (PRUint32 i = 0; i < ASSOCIATIVITY; ++i) {
            if (mCache[idx+i] == p) {
                mCache[idx+i] = (void*)0;
                return;
            }
        }
        mBackingStore.Remove(p);
    }

    void SpillOne(void* &p)
    {
        mBackingStore.Put(p, mCurrGen);
        p = (void*)0;
    }

    void SpillAll()
    {
        for (PRUint32 i = 0; i < N_POINTERS; ++i) {
            if (mCache[i]) {
                SpillOne(mCache[i]);
            }
        }
    }
};

static PR_CALLBACK PLDHashOperator
zeroGenerationCallback(const void*  ptr,
                       PRUint32&    generation,
                       void*        userArg)
{
#ifdef DEBUG_CC
    nsPurpleBuffer *purp = NS_STATIC_CAST(nsPurpleBuffer*, userArg);
    purp->mStats.mZeroGeneration++;
#endif
    generation = 0;
    return PL_DHASH_NEXT;
}

void nsPurpleBuffer::BumpGeneration()
{
    SpillAll();
    if (mCurrGen == 0xffffffff) {
        mBackingStore.Enumerate(zeroGenerationCallback, this);
        mCurrGen = 0;
    } else {
        ++mCurrGen;
    }
#ifdef DEBUG_CC
    mStats.mBumpGeneration++;
#endif
}

static inline PRBool
SufficientlyAged(PRUint32 generation, nsPurpleBuffer *p)
{
    return generation + p->mParams.mScanDelay < p->mCurrGen;
}

static PR_CALLBACK PLDHashOperator
ageSelectionCallback(const void*  ptr,
                     PRUint32&    generation,
                     void*        userArg)
{
    nsPurpleBuffer *purp = NS_STATIC_CAST(nsPurpleBuffer*, userArg);
    if (SufficientlyAged(generation, purp)) {
        nsISupports *root = NS_STATIC_CAST(nsISupports *, 
                                           NS_CONST_CAST(void*, ptr));
        purp->mTransferBuffer->Push(root);
    }
    return PL_DHASH_NEXT;
}

void
nsPurpleBuffer::SelectAgedPointers(nsDeque *transferBuffer)
{
    mTransferBuffer = transferBuffer;
    mBackingStore.Enumerate(ageSelectionCallback, this);
    mTransferBuffer = nsnull;
}








struct nsCycleCollectionXPCOMRuntime : 
    public nsCycleCollectionLanguageRuntime 
{
    nsresult BeginCycleCollection() 
    {
        return NS_OK;
    }

    nsresult FinishCycleCollection() 
    {
        return NS_OK;
    }

    inline nsCycleCollectionParticipant *ToParticipant(void *p);
};

struct nsCycleCollector
{
    PRBool mCollectionInProgress;
    PRBool mScanInProgress;

    nsCycleCollectionLanguageRuntime *mRuntimes[nsIProgrammingLanguage::MAX+1];
    nsCycleCollectionXPCOMRuntime mXPCOMRuntime;

    
    
    

    nsDeque mBuf;
    
    nsCycleCollectorParams mParams;

    nsPurpleBuffer mPurpleBuf;

    void RegisterRuntime(PRUint32 langID, 
                         nsCycleCollectionLanguageRuntime *rt);
    void ForgetRuntime(PRUint32 langID);

    void CollectPurple(); 
    void MarkRoots(GCGraph &graph);
    void ScanRoots(GCGraph &graph);
    void CollectWhite(GCGraph &graph);

    nsCycleCollector();
    ~nsCycleCollector();

    void Suspect(nsISupports *n, PRBool current = PR_FALSE);
    void Forget(nsISupports *n);
    void Allocated(void *n, size_t sz);
    void Freed(void *n);
    void Collect(PRUint32 aTryCollections = 1);
    void Shutdown();

#ifdef DEBUG_CC
    nsCycleCollectorStats mStats;    

    FILE *mPtrLog;

    void MaybeDrawGraphs(GCGraph &graph);
    void ExplainLiveExpectedGarbage();
    void ShouldBeFreed(nsISupports *n);
    void WasFreed(nsISupports *n);
    PointerSet mExpectedGarbage;
#endif
};


class GraphWalker
{
private:
    void DoWalk(nsDeque &aQueue);

public:
    void Walk(PtrInfo *s0);
    void WalkFromRoots(GCGraph &aGraph);

    
    virtual PRBool ShouldVisitNode(PtrInfo const *pi) = 0;
    virtual void VisitNode(PtrInfo *pi) = 0;
};







static nsCycleCollector *sCollector = nsnull;






static void
Fault(const char *msg, const void *ptr=nsnull)
{
#ifdef DEBUG_CC
    
    if (!sCollector)
        return;

    if (sCollector->mParams.mFaultIsFatal) {

        if (ptr)
            printf("Fatal fault in cycle collector: %s (ptr: %p)\n", msg, ptr);
        else
            printf("Fatal fault in cycle collector: %s\n", msg);

        exit(1);

    } 
#endif

    NS_NOTREACHED(nsPrintfCString(256,
                  "Fault in cycle collector: %s (ptr: %p)\n",
                  msg, ptr).get());

    
    
    
    
    
    

    sCollector->mParams.mDoNothing = PR_TRUE;
}



static nsISupports *
canonicalize(nsISupports *in)
{
    nsCOMPtr<nsISupports> child;
    in->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                       getter_AddRefs(child));
    return child.get();
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

nsCycleCollectionParticipant *
nsCycleCollectionXPCOMRuntime::ToParticipant(void *p)
{
    nsXPCOMCycleCollectionParticipant *cp;
    ::ToParticipant(NS_STATIC_CAST(nsISupports*, p), &cp);
    return cp;
}


void
GraphWalker::Walk(PtrInfo *s0)
{
    nsDeque queue;
    queue.Push(s0);
    DoWalk(queue);
}

void
GraphWalker::WalkFromRoots(GCGraph& aGraph)
{
    nsDeque queue;
    NodePool::Enumerator etor(aGraph.mNodes);
    for (PRUint32 i = 0; i < aGraph.mRootCount; ++i) {
        queue.Push(etor.GetNext());
    }
    DoWalk(queue);
}

void
GraphWalker::DoWalk(nsDeque &aQueue)
{
    
    
    while (aQueue.GetSize() > 0) {
        PtrInfo *pi = NS_STATIC_CAST(PtrInfo*, aQueue.PopFront());

        if (this->ShouldVisitNode(pi)) {
            this->VisitNode(pi);
            for (EdgePool::Iterator child = pi->mFirstChild,
                                child_end = pi->mLastChild;
                 child != child_end; ++child) {
                aQueue.Push(*child);
            }
        }
    };

#ifdef DEBUG_CC
    sCollector->mStats.mWalkedGraph++;
#endif
}






struct PtrToNodeEntry : public PLDHashEntryHdr
{
    
    PtrInfo *mNode;
};

PR_STATIC_CALLBACK(PRBool)
PtrToNodeMatchEntry(PLDHashTable *table,
                    const PLDHashEntryHdr *entry,
                    const void *key)
{
    const PtrToNodeEntry *n = NS_STATIC_CAST(const PtrToNodeEntry*, entry);
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
    nsnull
};

class GCGraphBuilder : private nsCycleCollectionTraversalCallback
{
private:
    NodePool::Builder mNodeBuilder;
    EdgePool::Builder mEdgeBuilder;
    PLDHashTable mPtrToNodeMap;
    PtrInfo *mCurrPi;
    nsCycleCollectionLanguageRuntime **mRuntimes; 

public:
    GCGraphBuilder(GCGraph &aGraph,
                   nsCycleCollectionLanguageRuntime **aRuntimes);
    ~GCGraphBuilder();

    PRUint32 Count() const { return mPtrToNodeMap.entryCount; }

    PtrInfo* AddNode(void *s, nsCycleCollectionParticipant *aParticipant);
    void Traverse(PtrInfo* aPtrInfo);

private:
    
#ifdef DEBUG_CC
    void DescribeNode(size_t refCount, size_t objSz, const char *objName);
#else
    void DescribeNode(size_t refCount);
#endif
    void NoteXPCOMChild(nsISupports *child);
    void NoteNativeChild(void *child,
                         nsCycleCollectionParticipant *participant);
    void NoteScriptChild(PRUint32 langID, void *child);
};

GCGraphBuilder::GCGraphBuilder(GCGraph &aGraph,
                               nsCycleCollectionLanguageRuntime **aRuntimes)
    : mNodeBuilder(aGraph.mNodes),
      mEdgeBuilder(aGraph.mEdges),
      mRuntimes(aRuntimes)
{
    if (!PL_DHashTableInit(&mPtrToNodeMap, &PtrNodeOps, nsnull,
                           sizeof(PtrToNodeEntry), 32768))
        mPtrToNodeMap.ops = nsnull;
}

GCGraphBuilder::~GCGraphBuilder()
{
    if (mPtrToNodeMap.ops)
        PL_DHashTableFinish(&mPtrToNodeMap);
}

PtrInfo*
GCGraphBuilder::AddNode(void *s, nsCycleCollectionParticipant *aParticipant)
{
    PtrToNodeEntry *e = NS_STATIC_CAST(PtrToNodeEntry*, 
        PL_DHashTableOperate(&mPtrToNodeMap, s, PL_DHASH_ADD));
    PtrInfo *result;
    if (!e->mNode) {
        
        result = mNodeBuilder.Add(s, aParticipant);
        if (!result) {
            PL_DHashTableRawRemove(&mPtrToNodeMap, e);
            return nsnull;
        }
        e->mNode = result;
    } else {
        result = e->mNode;
        NS_ASSERTION(result->mParticipant == aParticipant,
                     "nsCycleCollectionParticipant shouldn't change!");
    }
    return result;
}

void
GCGraphBuilder::Traverse(PtrInfo* aPtrInfo)
{
    mCurrPi = aPtrInfo;

#ifdef DEBUG_CC
    if (!mCurrPi->mParticipant) {
        Fault("unknown pointer during walk");
        return;
    }
#endif

    mCurrPi->mFirstChild = mEdgeBuilder.Mark();
    
    nsresult rv = aPtrInfo->mParticipant->Traverse(aPtrInfo->mPointer, *this);
    if (NS_FAILED(rv)) {
        Fault("script pointer traversal failed", aPtrInfo->mPointer);
    }

    mCurrPi->mLastChild = mEdgeBuilder.Mark();
}

void 
#ifdef DEBUG_CC
GCGraphBuilder::DescribeNode(size_t refCount, size_t objSz, const char *objName)
#else
GCGraphBuilder::DescribeNode(size_t refCount)
#endif
{
    if (refCount == 0)
        Fault("zero refcount", mCurrPi->mPointer);

#ifdef DEBUG_CC
    mCurrPi->mBytes = objSz;
    mCurrPi->mName = PL_strdup(objName);
#endif

    mCurrPi->mRefCount = refCount;
#ifdef DEBUG_CC
    sCollector->mStats.mVisitedNode++;
#endif
}

void 
GCGraphBuilder::NoteXPCOMChild(nsISupports *child) 
{
    if (!child || !(child = canonicalize(child)))
        return; 

#ifdef DEBUG_CC
    if (nsCycleCollector_shouldSuppress(child))
        return;
#endif
    
    nsXPCOMCycleCollectionParticipant *cp;
    ToParticipant(child, &cp);
    if (cp) {
        PtrInfo *childPi = AddNode(child, cp);
        if (!childPi)
            return;
        mEdgeBuilder.Add(childPi);
        ++childPi->mInternalRefs;
    }
}

void
GCGraphBuilder::NoteNativeChild(void *child,
                                nsCycleCollectionParticipant *participant)
{
    if (!child)
        return;

    NS_ASSERTION(participant, "Need a nsCycleCollectionParticipant!");

    PtrInfo *childPi = AddNode(child, participant);
    if (!childPi)
        return;
    mEdgeBuilder.Add(childPi);
    ++childPi->mInternalRefs;
}

void
GCGraphBuilder::NoteScriptChild(PRUint32 langID, void *child) 
{
    if (!child)
        return;

    if (langID > nsIProgrammingLanguage::MAX || !mRuntimes[langID]) {
        Fault("traversing pointer for unregistered language", child);
        return;
    }

    nsCycleCollectionParticipant *cp = mRuntimes[langID]->ToParticipant(child);
    if (!cp)
        return;

    PtrInfo *childPi = AddNode(child, cp);
    if (!childPi)
        return;
    mEdgeBuilder.Add(childPi);
    ++childPi->mInternalRefs;
}


void 
nsCycleCollector::CollectPurple()
{
    mPurpleBuf.SelectAgedPointers(&mBuf);
}

void
nsCycleCollector::MarkRoots(GCGraph &graph)
{
    if (mBuf.GetSize() == 0)
        return;

    GCGraphBuilder builder(graph, mRuntimes);

    int i;
    for (i = 0; i < mBuf.GetSize(); ++i) {
        nsISupports *s = NS_STATIC_CAST(nsISupports *, mBuf.ObjectAt(i));
        nsXPCOMCycleCollectionParticipant *cp;
        ToParticipant(s, &cp);
        if (cp) {
            PtrInfo *pinfo = builder.AddNode(canonicalize(s), cp);
            if (pinfo)
                pinfo->mWasPurple = PR_TRUE;
        }
    }

    graph.mRootCount = builder.Count();

    
    NodePool::Enumerator queue(graph.mNodes);
    while (!queue.IsDone()) {
        PtrInfo *pi = queue.GetNext();
        builder.Traverse(pi);
    }
}







struct ScanBlackWalker : public GraphWalker
{
    PRBool ShouldVisitNode(PtrInfo const *pi)
    { 
        return pi->mColor != black;
    }

    void VisitNode(PtrInfo *pi)
    { 
        pi->mColor = black;
#ifdef DEBUG_CC
        sCollector->mStats.mSetColorBlack++;
#endif
    }
};


struct scanWalker : public GraphWalker
{
    PRBool ShouldVisitNode(PtrInfo const *pi)
    { 
        return pi->mColor == grey;
    }

    void VisitNode(PtrInfo *pi)
    {
        if (pi->mColor != grey)
            Fault("scanning non-grey node", pi->mPointer);

        if (pi->mInternalRefs > pi->mRefCount)
            Fault("traversed refs exceed refcount", pi->mPointer);

        if (pi->mInternalRefs == pi->mRefCount) {
            pi->mColor = white;
#ifdef DEBUG_CC
            sCollector->mStats.mSetColorWhite++;
#endif
        } else {
            ScanBlackWalker().Walk(pi);
            NS_ASSERTION(pi->mColor == black,
                         "Why didn't ScanBlackWalker make pi black?");
        }
    }
};

void
nsCycleCollector::ScanRoots(GCGraph &graph)
{
    
    
    
    scanWalker().WalkFromRoots(graph); 

#ifdef DEBUG_CC
    
    
    NodePool::Enumerator etor(graph.mNodes);
    while (!etor.IsDone())
    {
        PtrInfo *pinfo = etor.GetNext();
        if (pinfo->mColor == grey) {
            Fault("valid grey node after scanning", pinfo->mPointer);
        }
    }
#endif
}






void
nsCycleCollector::CollectWhite(GCGraph &graph)
{
    
    
    
    
    
    
    
    
    
    
    
    
    

    nsresult rv;

    mBuf.Empty();

#if defined(DEBUG_CC) && !defined(__MINGW32__) && defined(WIN32)
    struct _CrtMemState ms1, ms2;
    _CrtMemCheckpoint(&ms1);
#endif

    NodePool::Enumerator etor(graph.mNodes);
    while (!etor.IsDone())
    {
        PtrInfo *pinfo = etor.GetNext();
        void *p = pinfo->mPointer;

        if (pinfo->mColor == white) {
            mBuf.Push(pinfo);

            if (pinfo->mWasPurple) {
                nsISupports* s = NS_STATIC_CAST(nsISupports*, p);
                Forget(s);
            }
        }
        else if (pinfo->mWasPurple) {
            nsISupports* s = NS_STATIC_CAST(nsISupports*, p);
            nsXPCOMCycleCollectionParticipant* cp =
                NS_STATIC_CAST(nsXPCOMCycleCollectionParticipant*,
                               pinfo->mParticipant);
#ifdef DEBUG
            nsXPCOMCycleCollectionParticipant* checkcp;
            CallQueryInterface(s, &checkcp);
            NS_ASSERTION(checkcp == cp,
                         "QI should return the same participant!");
#endif
            cp->UnmarkPurple(s);
            Forget(s);
        }
    }

    PRUint32 i, count = mBuf.GetSize();
    for (i = 0; i < count; ++i) {
        PtrInfo *pinfo = NS_STATIC_CAST(PtrInfo*, mBuf.ObjectAt(i));
        rv = pinfo->mParticipant->Root(pinfo->mPointer);
        if (NS_FAILED(rv))
            Fault("Failed root call while unlinking");
    }

    for (i = 0; i < count; ++i) {
        PtrInfo *pinfo = NS_STATIC_CAST(PtrInfo*, mBuf.ObjectAt(i));
        rv = pinfo->mParticipant->Unlink(pinfo->mPointer);
        if (NS_FAILED(rv)) {
            Fault("Failed unlink call while unlinking");
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

    for (i = 0; i < count; ++i) {
        PtrInfo *pinfo = NS_STATIC_CAST(PtrInfo*, mBuf.ObjectAt(i));
        rv = pinfo->mParticipant->Unroot(pinfo->mPointer);
        if (NS_FAILED(rv))
            Fault("Failed unroot call while unlinking");
    }

    mBuf.Empty();

#if defined(DEBUG_CC) && !defined(__MINGW32__) && defined(WIN32)
    _CrtMemCheckpoint(&ms2);
    if (ms2.lTotalCount < ms1.lTotalCount)
        mStats.mFreedBytes += (ms1.lTotalCount - ms2.lTotalCount);
#endif
}


#ifdef DEBUG_CC






static PRBool hookedMalloc = PR_FALSE;

#ifdef __GLIBC__
#include <malloc.h>

static void* (*old_memalign_hook)(size_t, size_t, const void *);
static void* (*old_realloc_hook)(void *, size_t, const void *);
static void* (*old_malloc_hook)(size_t, const void *);
static void (*old_free_hook)(void *, const void *);

static void* my_memalign_hook(size_t, size_t, const void *);
static void* my_realloc_hook(void *, size_t, const void *);
static void* my_malloc_hook(size_t, const void *);
static void my_free_hook(void *, const void *);

static inline void 
install_old_hooks()
{
    __memalign_hook = old_memalign_hook;
    __realloc_hook = old_realloc_hook;
    __malloc_hook = old_malloc_hook;
    __free_hook = old_free_hook;
}

static inline void 
save_old_hooks()
{
    
    
    
    

    
    
    
    
}

static inline void 
install_new_hooks()
{
    __memalign_hook = my_memalign_hook;
    __realloc_hook = my_realloc_hook;
    __malloc_hook = my_malloc_hook;
    __free_hook = my_free_hook;
}

static void*
my_realloc_hook(void *ptr, size_t size, const void *caller)
{
    void *result;    

    install_old_hooks();
    result = realloc(ptr, size);
    save_old_hooks();

    if (sCollector) {
        sCollector->Freed(ptr);
        sCollector->Allocated(result, size);
    }

    install_new_hooks();

    return result;
}


static void* 
my_memalign_hook(size_t size, size_t alignment, const void *caller)
{
    void *result;    

    install_old_hooks();
    result = memalign(size, alignment);
    save_old_hooks();

    if (sCollector)
        sCollector->Allocated(result, size);

    install_new_hooks();

    return result;
}


static void 
my_free_hook (void *ptr, const void *caller)
{
    install_old_hooks();
    free(ptr);
    save_old_hooks();

    if (sCollector)
        sCollector->Freed(ptr);

    install_new_hooks();
}      


static void*
my_malloc_hook (size_t size, const void *caller)
{
    void *result;

    install_old_hooks();
    result = malloc (size);
    save_old_hooks();

    if (sCollector)
        sCollector->Allocated(result, size);

    install_new_hooks();

    return result;
}


static void 
InitMemHook(void)
{
    if (!hookedMalloc) {
        save_old_hooks();
        install_new_hooks();
        hookedMalloc = PR_TRUE;        
    }
}

#elif defined(WIN32)
#ifndef __MINGW32__

static int 
AllocHook(int allocType, void *userData, size_t size, int 
          blockType, long requestNumber, const unsigned char *filename, int 
          lineNumber)
{
    if (allocType == _HOOK_FREE)
        sCollector->Freed(userData);
    return 1;
}


static void InitMemHook(void)
{
    if (!hookedMalloc) {
        _CrtSetAllocHook (AllocHook);
        hookedMalloc = PR_TRUE;        
    }
}
#endif 

#elif 0 

#include <malloc/malloc.h>

static void (*old_free)(struct _malloc_zone_t *zone, void *ptr);

static void
freehook(struct _malloc_zone_t *zone, void *ptr)
{
    if (sCollector)
        sCollector->Freed(ptr);
    old_free(zone, ptr);
}


static void
InitMemHook(void)
{
    if (!hookedMalloc) {
        malloc_zone_t *default_zone = malloc_default_zone();
        old_free = default_zone->free;
        default_zone->free = freehook;
        hookedMalloc = PR_TRUE;
    }
}


#else

static void
InitMemHook(void)
{
}

#endif 
#endif 





nsCycleCollector::nsCycleCollector() : 
    mCollectionInProgress(PR_FALSE),
    mScanInProgress(PR_FALSE),
#ifdef DEBUG_CC
    mPurpleBuf(mParams, mStats),
    mPtrLog(nsnull)
#else
    mPurpleBuf(mParams)
#endif
{
#ifdef DEBUG_CC
    mExpectedGarbage.Init();
#endif

    memset(mRuntimes, 0, sizeof(mRuntimes));
    mRuntimes[nsIProgrammingLanguage::CPLUSPLUS] = &mXPCOMRuntime;
}


nsCycleCollector::~nsCycleCollector()
{
}


void 
nsCycleCollector::RegisterRuntime(PRUint32 langID, 
                                  nsCycleCollectionLanguageRuntime *rt)
{
    if (mParams.mDoNothing)
        return;

    if (langID > nsIProgrammingLanguage::MAX)
        Fault("unknown language runtime in registration");

    if (mRuntimes[langID])
        Fault("multiple registrations of language runtime", rt);

    mRuntimes[langID] = rt;
}


void 
nsCycleCollector::ForgetRuntime(PRUint32 langID)
{
    if (mParams.mDoNothing)
        return;

    if (langID > nsIProgrammingLanguage::MAX)
        Fault("unknown language runtime in deregistration");

    if (! mRuntimes[langID])
        Fault("forgetting non-registered language runtime");

    mRuntimes[langID] = nsnull;
}


#ifdef DEBUG_CC
void 
nsCycleCollector::MaybeDrawGraphs(GCGraph &graph)
{
    if (mParams.mDrawGraphs) {
        
        PRBool anyWhites = PR_FALSE;
        NodePool::Enumerator fwetor(graph.mNodes);
        while (!fwetor.IsDone())
        {
            PtrInfo *pinfo = fwetor.GetNext();
            if (pinfo->mColor == white) {
                anyWhites = PR_TRUE;
                break;
            }
        }

        if (anyWhites) {
            
            
            FILE *stream;
#ifdef WIN32
            stream = fopen("c:\\cycle-graph.dot", "w+");
#else
            stream = popen("dotty -", "w");
#endif
            fprintf(stream, 
                    "digraph collection {\n"
                    "rankdir=LR\n"
                    "node [fontname=fixed, fontsize=10, style=filled, shape=box]\n"
                    );

            NodePool::Enumerator etor(graph.mNodes);
            while (!etor.IsDone()) {
                PtrInfo *pi = etor.GetNext();
                const void *p = pi->mPointer;
                fprintf(stream, 
                        "n%p [label=\"%s\\n%p\\n%u/%u refs found\", "
                        "fillcolor=%s, fontcolor=%s]\n", 
                        p,
                        pi->mName,
                        p,
                        pi->mInternalRefs, pi->mRefCount,
                        (pi->mColor == black ? "black" : "white"),
                        (pi->mColor == black ? "white" : "black"));
                for (EdgePool::Iterator child = pi->mFirstChild,
                                    child_end = pi->mLastChild;
                     child != child_end; ++child) {
                    fprintf(stream, "n%p -> n%p\n", p, (*child)->mPointer);
                }
            }

            fprintf(stream, "\n}\n");
#ifdef WIN32
            fclose(stream);
            
            
            
            _spawnlp(_P_WAIT, 
                     "lefty", 
                     "lefty",
                     "-e",
                     "\"load('dotty.lefty');"
                     "dotty.simple('c:\\cycle-graph.dot');\"",
                     NULL);
            unlink("c:\\cycle-graph.dot");
#else
            pclose(stream);
#endif
        }
    }
}

class Suppressor :
    public nsCycleCollectionTraversalCallback
{
protected:
    static char *sSuppressionList;
    static PRBool sInitialized;
    PRBool mSuppressThisNode;
public:
    Suppressor()
    {
    }

    PRBool shouldSuppress(nsISupports *s)
    {
        if (!sInitialized) {
            sSuppressionList = PR_GetEnv("XPCOM_CC_SUPPRESS");
            sInitialized = PR_TRUE;
        }
        if (sSuppressionList == nsnull) {
            mSuppressThisNode = PR_FALSE;
        } else {
            nsresult rv;
            nsXPCOMCycleCollectionParticipant *cp;
            rv = CallQueryInterface(s, &cp);
            if (NS_FAILED(rv)) {
                Fault("checking suppression on wrong type of pointer", s);
                return PR_TRUE;
            }
            cp->Traverse(s, *this);
        }
        return mSuppressThisNode;
    }

    void DescribeNode(size_t refCount, size_t objSz, const char *objName)
    {
        mSuppressThisNode = (PL_strstr(sSuppressionList, objName) != nsnull);
    }

    void NoteXPCOMChild(nsISupports *child) {}
    void NoteScriptChild(PRUint32 langID, void *child) {}
    void NoteNativeChild(void *child,
                         nsCycleCollectionParticipant *participant) {}
};

char *Suppressor::sSuppressionList = nsnull;
PRBool Suppressor::sInitialized = PR_FALSE;

static PRBool
nsCycleCollector_shouldSuppress(nsISupports *s)
{
    Suppressor supp;
    return supp.shouldSuppress(s);
}
#endif

#ifdef DEBUG
static PRBool
nsCycleCollector_isScanSafe(nsISupports *s)
{
    if (!s)
        return PR_FALSE;

    nsXPCOMCycleCollectionParticipant *cp;
    ToParticipant(s, &cp);

    return cp != nsnull;
}
#endif

void 
nsCycleCollector::Suspect(nsISupports *n, PRBool current)
{
    
    
    

    if (mScanInProgress)
        return;

    NS_ASSERTION(nsCycleCollector_isScanSafe(n),
                 "suspected a non-scansafe pointer");
    NS_ASSERTION(NS_IsMainThread(), "trying to suspect from non-main thread");

    if (mParams.mDoNothing)
        return;

#ifdef DEBUG_CC
    mStats.mSuspectNode++;

    if (nsCycleCollector_shouldSuppress(n))
        return;

#ifndef __MINGW32__
    if (mParams.mHookMalloc)
        InitMemHook();
#endif

    if (mParams.mLogPointers) {
        if (!mPtrLog)
            mPtrLog = fopen("pointer_log", "w");
        fprintf(mPtrLog, "S %p\n", NS_STATIC_CAST(void*, n));
    }
#endif

    if (current)
        mBuf.Push(n);
    else
        mPurpleBuf.Put(n);
}


void 
nsCycleCollector::Forget(nsISupports *n)
{
    
    
    

    if (mScanInProgress)
        return;

    NS_ASSERTION(NS_IsMainThread(), "trying to forget from non-main thread");
    
    if (mParams.mDoNothing)
        return;

#ifdef DEBUG_CC
    mStats.mForgetNode++;

#ifndef __MINGW32__
    if (mParams.mHookMalloc)
        InitMemHook();
#endif

    if (mParams.mLogPointers) {
        if (!mPtrLog)
            mPtrLog = fopen("pointer_log", "w");
        fprintf(mPtrLog, "F %p\n", NS_STATIC_CAST(void*, n));
    }
#endif

    mPurpleBuf.Remove(n);
}

#ifdef DEBUG_CC
void 
nsCycleCollector::Allocated(void *n, size_t sz)
{
}

void 
nsCycleCollector::Freed(void *n)
{
    mStats.mFreeCalls++;

    if (!n) {
        
        return;
    }

    if (mPurpleBuf.Exists(n)) {
        mStats.mForgetNode++;
        mStats.mFreedWhilePurple++;
        Fault("freed while purple", n);
        mPurpleBuf.Remove(n);
        
        if (mParams.mLogPointers) {
            if (!mPtrLog)
                mPtrLog = fopen("pointer_log", "w");
            fprintf(mPtrLog, "R %p\n", n);
        }
    }
}
#endif

void
nsCycleCollector::Collect(PRUint32 aTryCollections)
{
#if defined(DEBUG_CC) && !defined(__MINGW32__)
    if (!mParams.mDoNothing && mParams.mHookMalloc)
        InitMemHook();
#endif

#ifdef COLLECT_TIME_DEBUG
    printf("cc: Starting nsCycleCollector::Collect(%d)\n", aTryCollections);
    PRTime start = PR_Now(), now;
#endif

    nsCOMPtr<nsIObserverService> obs =
      do_GetService("@mozilla.org/observer-service;1");
    if (obs) {
        obs->NotifyObservers(nsnull, "cycle-collector-begin", nsnull);
    }

#ifdef DEBUG_CC
    PRUint32 origTryCollections = aTryCollections;
#endif

    while (aTryCollections > 0) {
        
        
        
        
        
        
        
        
        mBuf.Empty();

#ifdef COLLECT_TIME_DEBUG
        now = PR_Now();
#endif
        for (PRUint32 i = 0; i <= nsIProgrammingLanguage::MAX; ++i) {
            if (mRuntimes[i])
                mRuntimes[i]->BeginCycleCollection();
        }

#ifdef COLLECT_TIME_DEBUG
        printf("cc: mRuntimes[*]->BeginCycleCollection() took %lldms\n",
               (PR_Now() - now) / PR_USEC_PER_MSEC);
#endif

        if (mParams.mDoNothing) {
            aTryCollections = 0;
        } else {
#ifdef COLLECT_TIME_DEBUG
            now = PR_Now();
#endif

#ifdef DEBUG_CC
            PRUint32 purpleStart = mBuf.GetSize();
#endif
            CollectPurple();
#ifdef DEBUG_CC
            PRUint32 purpleEnd = mBuf.GetSize();
#endif

#ifdef COLLECT_TIME_DEBUG
            printf("cc: CollectPurple() took %lldms\n",
                   (PR_Now() - now) / PR_USEC_PER_MSEC);
#endif

            if (mBuf.GetSize() == 0) {
                aTryCollections = 0;
            } else {
                if (mCollectionInProgress)
                    Fault("re-entered collection");

                mCollectionInProgress = PR_TRUE;

                mScanInProgress = PR_TRUE;

                GCGraph graph;

                

#ifdef COLLECT_TIME_DEBUG
                now = PR_Now();
#endif
                MarkRoots(graph);

#ifdef COLLECT_TIME_DEBUG
                {
                    PRTime then = PR_Now();
                    printf("cc: MarkRoots() took %lldms\n",
                           (then - now) / PR_USEC_PER_MSEC);
                    now = then;
                }
#endif

                ScanRoots(graph);

#ifdef COLLECT_TIME_DEBUG
                printf("cc: ScanRoots() took %lldms\n",
                       (PR_Now() - now) / PR_USEC_PER_MSEC);
#endif

#ifdef DEBUG_CC
                MaybeDrawGraphs(graph);
#endif

                mScanInProgress = PR_FALSE;

#ifdef DEBUG_CC
                if (aTryCollections != origTryCollections && purpleStart != purpleEnd) {
                    PRUint32 i = 0;
                    NodePool::Enumerator queue(graph.mNodes);
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

#ifdef COLLECT_TIME_DEBUG
                now = PR_Now();
#endif
                CollectWhite(graph);

#ifdef COLLECT_TIME_DEBUG
                printf("cc: CollectWhite() took %lldms\n",
                       (PR_Now() - now) / PR_USEC_PER_MSEC);
#endif

                

                --aTryCollections;
            }

            mPurpleBuf.BumpGeneration();

#ifdef DEBUG_CC
            mStats.mCollection++;
            if (mParams.mReportStats)
                mStats.Dump();
#endif

            mCollectionInProgress = PR_FALSE;
        }

        for (PRUint32 i = 0; i <= nsIProgrammingLanguage::MAX; ++i) {
            if (mRuntimes[i])
                mRuntimes[i]->FinishCycleCollection();
        }
    }

#ifdef COLLECT_TIME_DEBUG
    printf("cc: Collect() took %lldms\n",
           (PR_Now() - start) / PR_USEC_PER_MSEC);
#endif
}

void
nsCycleCollector::Shutdown()
{
    
    
    

    mPurpleBuf.BumpGeneration();
    mParams.mScanDelay = 0;
    Collect(SHUTDOWN_COLLECTIONS(mParams));

#ifdef DEBUG_CC
    CollectPurple();
    if (mBuf.GetSize() != 0) {
        printf("Might have been able to release more cycles if the cycle collector would "
               "run once more at shutdown.\n");
    }

    ExplainLiveExpectedGarbage();
#endif
    mParams.mDoNothing = PR_TRUE;
}

#ifdef DEBUG_CC

PR_STATIC_CALLBACK(PLDHashOperator)
AddExpectedGarbage(nsVoidPtrHashKey *p, void *arg)
{
    nsCycleCollector *c = NS_STATIC_CAST(nsCycleCollector*, arg);
    c->mBuf.Push(NS_CONST_CAST(void*, p->GetKey()));
    return PL_DHASH_NEXT;
}

void
nsCycleCollector::ExplainLiveExpectedGarbage()
{
    if (mScanInProgress || mCollectionInProgress)
        Fault("can't explain expected garbage during collection itself");

    if (mParams.mDoNothing) {
        printf("nsCycleCollector: not explaining expected garbage since\n"
               "  cycle collection disabled\n");
        return;
    }

    for (PRUint32 i = 0; i <= nsIProgrammingLanguage::MAX; ++i) {
        if (mRuntimes[i])
            mRuntimes[i]->BeginCycleCollection();
    }

    mCollectionInProgress = PR_TRUE;
    mScanInProgress = PR_TRUE;

    {
        GCGraph graph;
        mBuf.Empty();

        
        
        mExpectedGarbage.EnumerateEntries(&AddExpectedGarbage, this);

        MarkRoots(graph);
        ScanRoots(graph);

        mScanInProgress = PR_FALSE;

        PRBool giveKnownReferences = PR_FALSE;
        {
            NodePool::Enumerator queue(graph.mNodes);
            while (!queue.IsDone()) {
                PtrInfo *pi = queue.GetNext();
                if (pi->mColor == white) {
                    printf("nsCycleCollector: %s %p was not collected due to\n"
                           "  missing call to suspect or failure to unlink\n",
                           pi->mName, pi->mPointer);
                }

                if (pi->mInternalRefs != pi->mRefCount) {
                    
                    
                    
                    
                    printf("nsCycleCollector: %s %p was not collected due to %d\n"
                           "  external references\n",
                           pi->mName, pi->mPointer,
                           pi->mRefCount - pi->mInternalRefs);
                    giveKnownReferences = PR_TRUE;
                }
            }
        }

        if (giveKnownReferences) {
            printf("The known references to the objects above not collected\n"
                   "due to external references are:\n");
            NodePool::Enumerator queue(graph.mNodes);
            while (!queue.IsDone()) {
                PtrInfo *pi = queue.GetNext();
                for (EdgePool::Iterator child = pi->mFirstChild,
                                    child_end = pi->mLastChild;
                     child != child_end; ++child) {
                    if ((*child)->mInternalRefs != (*child)->mRefCount) {
                        printf("  to: %p from %s %p\n",
                               (*child)->mPointer, pi->mName, pi->mPointer);
                    }
                }
            }
        }
    }

    mCollectionInProgress = PR_FALSE;

    for (PRUint32 i = 0; i <= nsIProgrammingLanguage::MAX; ++i) {
        if (mRuntimes[i])
            mRuntimes[i]->FinishCycleCollection();
    }    
}

void
nsCycleCollector::ShouldBeFreed(nsISupports *n)
{
    mExpectedGarbage.PutEntry(n);
}

void
nsCycleCollector::WasFreed(nsISupports *n)
{
    mExpectedGarbage.RemoveEntry(n);
}
#endif






void 
nsCycleCollector_registerRuntime(PRUint32 langID, 
                                 nsCycleCollectionLanguageRuntime *rt)
{
    if (sCollector)
        sCollector->RegisterRuntime(langID, rt);
}


void 
nsCycleCollector_forgetRuntime(PRUint32 langID)
{
    if (sCollector)
        sCollector->ForgetRuntime(langID);
}


void 
nsCycleCollector_suspect(nsISupports *n)
{
    if (sCollector)
        sCollector->Suspect(n);
}


void 
nsCycleCollector_suspectCurrent(nsISupports *n)
{
    if (sCollector)
        sCollector->Suspect(n, PR_TRUE);
}


void 
nsCycleCollector_forget(nsISupports *n)
{
    if (sCollector)
        sCollector->Forget(n);
}


void 
nsCycleCollector_collect()
{
    if (sCollector)
        sCollector->Collect();
}

nsresult 
nsCycleCollector_startup()
{
    NS_ASSERTION(!sCollector, "Forgot to call nsCycleCollector_shutdown?");

    sCollector = new nsCycleCollector();
    return sCollector ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

void 
nsCycleCollector_shutdown()
{
    if (sCollector) {
        sCollector->Shutdown();
        delete sCollector;
        sCollector = nsnull;
    }
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
