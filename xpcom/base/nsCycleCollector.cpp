





















































































































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

#include <stdio.h>
#ifdef WIN32
#include <io.h>
#include <process.h>
#endif





struct nsCycleCollectorParams
{
    PRBool mDoNothing;
    PRBool mReportStats;
    PRBool mHookMalloc;
    PRBool mDrawGraphs;
    PRBool mFaultIsFatal;
    PRBool mLogPointers;
    
    PRUint32 mScanDelay;
    PRUint32 mShutdownCollections;
    
    nsCycleCollectorParams() :
        mDoNothing     (PR_GetEnv("XPCOM_CC_DO_NOTHING") != NULL),
        mReportStats   (PR_GetEnv("XPCOM_CC_REPORT_STATS") != NULL),
        mHookMalloc    (PR_GetEnv("XPCOM_CC_HOOK_MALLOC") != NULL),
        mDrawGraphs    (PR_GetEnv("XPCOM_CC_DRAW_GRAPHS") != NULL),
        mFaultIsFatal  (PR_GetEnv("XPCOM_CC_FAULT_IS_FATAL") != NULL),
        mLogPointers   (PR_GetEnv("XPCOM_CC_LOG_POINTERS") != NULL),

        
        
        
        
        
        
        
        

        mScanDelay(10),
        mShutdownCollections(5)
    {
        char *s = PR_GetEnv("XPCOM_CC_SCAN_DELAY");
        if (s)
            PR_sscanf(s, "%d", &mScanDelay);
        s = PR_GetEnv("XPCOM_CC_SHUTDOWN_COLLECTIONS");
        if (s)
            PR_sscanf(s, "%d", &mShutdownCollections);
    }
};




struct nsCycleCollectorStats
{
    PRUint32 mFailedQI;
    PRUint32 mSuccessfulQI;

    PRUint32 mVisitedNode;
    PRUint32 mVisitedJSNode;
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
        DUMP(mVisitedJSNode);
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

static void
ToParticipant(nsISupports *s, nsCycleCollectionParticipant **cp);

static PRBool
nsCycleCollector_shouldSuppress(nsISupports *s);





enum NodeColor { black, white, grey };

struct PtrInfo
{
    NodeColor mColor;
    PRUint32 mLang;
    size_t mInternalRefs;
    size_t mRefCount;
    size_t mBytes;
    const char *mName;

    PtrInfo() 
        : mColor(black),
          mLang(nsIProgrammingLanguage::CPLUSPLUS),
          mInternalRefs(0), 
          mRefCount(0),
          mBytes(0), 
          mName(nsnull)
    {}
    
    PtrInfo(PRUint32 gen, NodeColor col) 
        : mColor(col),
          mLang(nsIProgrammingLanguage::CPLUSPLUS),
          mInternalRefs(0), 
          mRefCount(0),
          mBytes(0), 
          mName(nsnull)
    {}
};




typedef nsTHashtable<nsClearingVoidPtrHashKey> PointerSet;
typedef nsBaseHashtable<nsClearingVoidPtrHashKey, PRUint32, PRUint32>
    PointerSetWithGeneration;
typedef nsBaseHashtable<nsClearingVoidPtrHashKey, PtrInfo, PtrInfo> GCTable;

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
    nsCycleCollectorStats &mStats;
    void* mCache[N_POINTERS];
    PRUint32 mCurrGen;    
    PointerSetWithGeneration mBackingStore;
    nsDeque *mTransferBuffer;
    
    nsPurpleBuffer(nsCycleCollectorParams &params,
                   nsCycleCollectorStats &stats) 
        : mParams(params),
          mStats(stats),
          mTransferBuffer(nsnull)
    {
        memset(mCache, 0, sizeof(mCache));
        mBackingStore.Init();
    }

    ~nsPurpleBuffer()
    {
        memset(mCache, 0, sizeof(mCache));
        mBackingStore.Clear();
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
        mStats.mSpills++;
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
    nsPurpleBuffer *purp = NS_STATIC_CAST(nsPurpleBuffer*, userArg);
    purp->mStats.mZeroGeneration++;
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
    mStats.mBumpGeneration++;
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

    nsresult Traverse(void *p, nsCycleCollectionTraversalCallback &cb) 
    {
        nsresult rv;

        nsISupports *s = NS_STATIC_CAST(nsISupports *, p);        
        nsCycleCollectionParticipant *cp;
        ToParticipant(s, &cp);
        if (!cp) {
            Fault("walking wrong type of pointer", s);
            return NS_ERROR_FAILURE;
        }

        rv = cp->Traverse(s, cb);
        if (NS_FAILED(rv)) {
            Fault("XPCOM pointer traversal failed", s);
            return NS_ERROR_FAILURE;
        }
        return NS_OK;
    }

    nsresult Root(const nsDeque &nodes)
    {
        for (PRInt32 i = 0; i < nodes.GetSize(); ++i) {
            void *p = nodes.ObjectAt(i);
            nsISupports *s = NS_STATIC_CAST(nsISupports *, p);
            NS_ADDREF(s);
        }
        return NS_OK;
    }

    nsresult Unlink(const nsDeque &nodes)
    {
        nsresult rv;

        for (PRInt32 i = 0; i < nodes.GetSize(); ++i) {
            void *p = nodes.ObjectAt(i);

            nsISupports *s = NS_STATIC_CAST(nsISupports *, p);
            nsCycleCollectionParticipant *cp;
            ToParticipant(s, &cp);
            if (!cp) {
                Fault("unlinking wrong kind of pointer", s);
                return NS_ERROR_FAILURE;
            }

            rv = cp->Unlink(s);

            if (NS_FAILED(rv)) {
                Fault("failed unlink", s);
                return NS_ERROR_FAILURE;
            }
        }
        return NS_OK;
    }

    nsresult Unroot(const nsDeque &nodes)
    {
        for (PRInt32 i = 0; i < nodes.GetSize(); ++i) {
            void *p = nodes.ObjectAt(i);
            nsISupports *s = NS_STATIC_CAST(nsISupports *, p);
            NS_RELEASE(s);
        }
        return NS_OK;
    }

    nsresult FinishCycleCollection() 
    {
        return NS_OK;
    }
};


struct nsCycleCollector
{
    PRBool mCollectionInProgress;
    PRBool mScanInProgress;

    GCTable mGraph;
    nsCycleCollectionLanguageRuntime *mRuntimes[nsIProgrammingLanguage::MAX+1];
    nsCycleCollectionXPCOMRuntime mXPCOMRuntime;

    
    
    
    
    
    

    nsDeque mBufs[nsIProgrammingLanguage::MAX + 1];
    
    nsCycleCollectorParams mParams;
    nsCycleCollectorStats mStats;    

    nsPurpleBuffer mPurpleBuf;
    FILE *mPtrLog;
    
    void MaybeDrawGraphs();
    void RegisterRuntime(PRUint32 langID, 
                         nsCycleCollectionLanguageRuntime *rt);
    void ForgetRuntime(PRUint32 langID);

    void CollectPurple(); 
    void MarkRoots();
    void ScanRoots();
    void CollectWhite();

    nsCycleCollector();
    ~nsCycleCollector();

    void Suspect(nsISupports *n, PRBool current=PR_FALSE);
    void Forget(nsISupports *n);
    void Allocated(void *n, size_t sz);
    void Freed(void *n);
    void Collect(PRUint32 aTryCollections = 1);
    void Shutdown();

#ifdef DEBUG
    void ExplainLiveExpectedGarbage();
    void ShouldBeFreed(nsISupports *n);
    void WasFreed(nsISupports *n);
    PointerSet mExpectedGarbage;
#endif
};


class GraphWalker :
    public nsCycleCollectionTraversalCallback
{
    nsDeque mQueue;
    void *mCurrPtr;
    PtrInfo mCurrPi;

protected:
    GCTable &mGraph;
    nsCycleCollectionLanguageRuntime **mRuntimes;

public:
    
    GraphWalker(GCTable & tab,
                nsCycleCollectionLanguageRuntime **runtimes) : 
        mQueue(nsnull),
        mCurrPtr(nsnull),
        mGraph(tab),
        mRuntimes(runtimes)
    {}

    virtual ~GraphWalker() 
    {}
   
    void Walk(void *s0);

    
    void DescribeNode(size_t refCount, size_t objSz, const char *objName);
    void NoteXPCOMChild(nsISupports *child);
    void NoteScriptChild(PRUint32 langID, void *child);

    
    virtual PRBool ShouldVisitNode(void *p, PtrInfo const & pi) = 0;
    virtual void VisitNode(void *p, PtrInfo & pi, size_t refcount) = 0;
    virtual void NoteChild(void *c, PtrInfo & childpi) = 0;
};







static nsCycleCollector *sCollector = nsnull;






#ifdef DEBUG_CC

struct safetyCallback :     
    public nsCycleCollectionTraversalCallback
{
    
    
    
    
    
    
    void DescribeNode(size_t refCount, size_t objSz, const char *objName) {}
    void NoteXPCOMChild(nsISupports *child) {}
    void NoteScriptChild(PRUint32 langID, void *child) {}
};

static safetyCallback sSafetyCallback;

#endif


static inline void
EnsurePtrInfo(GCTable & tab, void *n, PtrInfo & pi)
{
    if (!tab.Get(n, &pi))
        tab.Put(n, pi);
}


static void
Fault(const char *msg, const void *ptr=nsnull)
{
    
    if (!sCollector)
        return;

    if (sCollector->mParams.mFaultIsFatal) {

        if (ptr)
            printf("Fatal fault in cycle collector: %s (ptr: %p)\n", msg, ptr);
        else
            printf("Fatal fault in cycle collector: %s\n", msg);

        exit(1);

    } 

    NS_NOTREACHED(nsPrintfCString(256,
                  "Fault in cycle collector: %s (ptr: %p)\n",
                  msg, ptr).get());

    
    
    
    
    
    

    sCollector->mParams.mDoNothing = PR_TRUE;
}


void 
GraphWalker::DescribeNode(size_t refCount, size_t objSz, const char *objName)
{
    if (refCount == 0)
        Fault("zero refcount", mCurrPtr);

    mCurrPi.mBytes = objSz;
    mCurrPi.mName = objName;
    this->VisitNode(mCurrPtr, mCurrPi, refCount);
    sCollector->mStats.mVisitedNode++;
    if (mCurrPi.mLang == nsIProgrammingLanguage::JAVASCRIPT)
        sCollector->mStats.mVisitedJSNode++;
}


static nsISupports *
canonicalize(nsISupports *in)
{
    nsCOMPtr<nsISupports> child;
    in->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                       getter_AddRefs(child));
    return child.get();
}


void 
GraphWalker::NoteXPCOMChild(nsISupports *child) 
{
    if (!child)
        return; 
   
    child = canonicalize(child);

    if (nsCycleCollector_isScanSafe(child) && 
        !nsCycleCollector_shouldSuppress(child)) {
        PtrInfo childPi;
        EnsurePtrInfo(mGraph, child, childPi);
        this->NoteChild(child, childPi);
#ifdef DEBUG_CC
        mRuntimes[nsIProgrammingLanguage::CPLUSPLUS]->Traverse(child, sSafetyCallback);
#endif
        mQueue.Push(child);
    }
}


void
GraphWalker::NoteScriptChild(PRUint32 langID, void *child) 
{
    if (!child)
        return;

    if (!mRuntimes[langID])
        Fault("traversing pointer for unregistered language", child);

    PtrInfo childPi;
    childPi.mLang = langID;
    EnsurePtrInfo(mGraph, child, childPi);
    this->NoteChild(child, childPi);
#ifdef DEBUG_CC
    mRuntimes[langID]->Traverse(child, sSafetyCallback);
#endif
    mQueue.Push(child);
}


void
GraphWalker::Walk(void *s0)
{
    mQueue.Empty();
    mQueue.Push(s0);

    while (mQueue.GetSize() > 0) {

        mCurrPtr = mQueue.Pop();
        nsresult rv;

        if (!mGraph.Get(mCurrPtr, &mCurrPi)) {
            Fault("unknown pointer", mCurrPtr);
            continue;
        }

        if (mCurrPi.mLang >nsIProgrammingLanguage::MAX ) {
            Fault("unknown language during walk");
            continue;
        }

        if (!mRuntimes[mCurrPi.mLang]) {
            Fault("script pointer for unregistered language");
            continue;
        }
        
        if (this->ShouldVisitNode(mCurrPtr, mCurrPi)) {

            rv = mRuntimes[mCurrPi.mLang]->Traverse(mCurrPtr, *this);
            if (NS_FAILED(rv)) {
                Fault("script pointer traversal failed", mCurrPtr);
            }
        }
    }
    sCollector->mStats.mWalkedGraph++;
}







struct MarkGreyWalker : public GraphWalker
{
    MarkGreyWalker(GCTable &tab,
                   nsCycleCollectionLanguageRuntime **runtimes)
        : GraphWalker(tab, runtimes) 
    {}

    PRBool ShouldVisitNode(void *p, PtrInfo const & pi)  
    { 
        return pi.mColor != grey; 
    }

    void VisitNode(void *p, PtrInfo & pi, size_t refcount) 
    { 
        pi.mColor = grey; 
        pi.mRefCount = refcount;
        sCollector->mStats.mSetColorGrey++;
        mGraph.Put(p, pi);
    }

    void NoteChild(void *c, PtrInfo & childpi)
    { 
        childpi.mInternalRefs++; 
        mGraph.Put(c, childpi);
    }
};

void 
nsCycleCollector::CollectPurple()
{
    mPurpleBuf.SelectAgedPointers(&mBufs[0]);
}

void
nsCycleCollector::MarkRoots()
{
    int i;
    for (i = 0; i < mBufs[0].GetSize(); ++i) {
        PtrInfo pi;
        nsISupports *s = NS_STATIC_CAST(nsISupports *, mBufs[0].ObjectAt(i));
        s = canonicalize(s);
        EnsurePtrInfo(mGraph, s, pi);
        MarkGreyWalker(mGraph, mRuntimes).Walk(s);
    }
}







struct ScanBlackWalker : public GraphWalker
{
    ScanBlackWalker(GCTable &tab,
                   nsCycleCollectionLanguageRuntime **runtimes)
        : GraphWalker(tab, runtimes) 
    {}

    PRBool ShouldVisitNode(void *p, PtrInfo const & pi) 
    { 
        return pi.mColor != black; 
    }

    void VisitNode(void *p, PtrInfo & pi, size_t refcount) 
    { 
        pi.mColor = black; 
        sCollector->mStats.mSetColorBlack++;
        mGraph.Put(p, pi);
    }

    void NoteChild(void *c, PtrInfo & childpi) {}
};


struct scanWalker : public GraphWalker
{
    scanWalker(GCTable &tab,
               nsCycleCollectionLanguageRuntime **runtimes)
        : GraphWalker(tab, runtimes) 
    {}

    PRBool ShouldVisitNode(void *p, PtrInfo const & pi) 
    { 
        return pi.mColor == grey; 
    }

    void VisitNode(void *p, PtrInfo & pi, size_t refcount) 
    {
        if (pi.mColor != grey)
            Fault("scanning non-grey node", p);

        if (pi.mInternalRefs > refcount)
            Fault("traversed refs exceed refcount", p);

        if (pi.mInternalRefs == refcount) {
            pi.mColor = white;
            sCollector->mStats.mSetColorWhite++;
        } else {
            ScanBlackWalker(mGraph, mRuntimes).Walk(p);
            pi.mColor = black;
            sCollector->mStats.mSetColorBlack++;
        }
        mGraph.Put(p, pi);
    }
    void NoteChild(void *c, PtrInfo & childpi) {}
};


static PR_CALLBACK PLDHashOperator
NoGreyCallback(const void*  ptr,
               PtrInfo&     pinfo,
               void*        userArg)
{
    if (pinfo.mColor == grey)
        Fault("valid grey node after scanning", ptr);
    return PL_DHASH_NEXT;
}


void
nsCycleCollector::ScanRoots()
{
    int i;

    for (i = 0; i < mBufs[0].GetSize(); ++i) {
        nsISupports *s = NS_STATIC_CAST(nsISupports *, mBufs[0].ObjectAt(i));
        s = canonicalize(s);
        scanWalker(mGraph, mRuntimes).Walk(s); 
    }

    
    
    mGraph.Enumerate(NoGreyCallback, this);
}







static PR_CALLBACK PLDHashOperator
FindWhiteCallback(const void*  ptr,
                  PtrInfo&     pinfo,
                  void*        userArg)
{
    nsCycleCollector *collector = NS_STATIC_CAST(nsCycleCollector*, 
                                                 userArg);
    void* p = NS_CONST_CAST(void*, ptr);
    NS_ASSERTION(pinfo.mLang == nsIProgrammingLanguage::CPLUSPLUS ||
                 !collector->mPurpleBuf.Exists(p),
                 "Need to remove non-CPLUSPLUS objects from purple buffer!");
    if (pinfo.mColor == white) {
        if (pinfo.mLang  > nsIProgrammingLanguage::MAX)
            Fault("White node has bad language ID", p);
        else
            collector->mBufs[pinfo.mLang].Push(p);

        if (pinfo.mLang == nsIProgrammingLanguage::CPLUSPLUS) {
            nsISupports* s = NS_STATIC_CAST(nsISupports*, p);
            collector->Forget(s);
        }
    }
    else if (pinfo.mLang == nsIProgrammingLanguage::CPLUSPLUS) {
        nsISupports* s = NS_STATIC_CAST(nsISupports*, p);
        nsCycleCollectionParticipant* cp;
        CallQueryInterface(s, &cp);
        if (cp)
            cp->UnmarkPurple(s);
        collector->Forget(s);
    }
    return PL_DHASH_NEXT;
}


void
nsCycleCollector::CollectWhite()
{
    
    
    
    
    
    
    
    
    
    
    
    
    

    PRUint32 i;
    nsresult rv;

    for (i = 0; i < nsIProgrammingLanguage::MAX+1; ++i)
        mBufs[i].Empty();

#ifndef __MINGW32__
#ifdef WIN32
    struct _CrtMemState ms1, ms2;
    _CrtMemCheckpoint(&ms1);
#endif 
#endif 

    mGraph.Enumerate(FindWhiteCallback, this);

    for (i = 0; i < nsIProgrammingLanguage::MAX+1; ++i) {
        if (mRuntimes[i] &&
            mBufs[i].GetSize() > 0) {
            rv = mRuntimes[i]->Root(mBufs[i]);
            if (NS_FAILED(rv))
                Fault("Failed root call while unlinking");
        }
    }

    for (i = 0; i < nsIProgrammingLanguage::MAX+1; ++i) {
        if (mRuntimes[i] &&
            mBufs[i].GetSize() > 0) {
            rv = mRuntimes[i]->Unlink(mBufs[i]);
            if (NS_FAILED(rv)) {
                Fault("Failed unlink call while unlinking");
                mStats.mFailedUnlink++;
            } else {
                mStats.mCollectedNode += mBufs[i].GetSize();
            }
        }
    }

    for (i = 0; i < nsIProgrammingLanguage::MAX+1; ++i) {
        if (mRuntimes[i] &&
            mBufs[i].GetSize() > 0) {
            rv = mRuntimes[i]->Unroot(mBufs[i]);
            if (NS_FAILED(rv))
                Fault("Failed unroot call while unlinking");
        }
    }

    for (i = 0; i < nsIProgrammingLanguage::MAX+1; ++i)
        mBufs[i].Empty();

#ifndef __MINGW32__
#ifdef WIN32
    _CrtMemCheckpoint(&ms2);
    if (ms2.lTotalCount < ms1.lTotalCount)
        mStats.mFreedBytes += (ms1.lTotalCount - ms2.lTotalCount);
#endif 
#endif 
}







struct graphVizWalker : public GraphWalker
{
    
    
    PointerSet mVisited;
    void *mParent;
    FILE *mStream;

    graphVizWalker(GCTable &tab,
                   nsCycleCollectionLanguageRuntime **runtimes)
        : GraphWalker(tab, runtimes), 
          mParent(nsnull), 
          mStream(nsnull)        
    {
#ifdef WIN32
        mStream = fopen("c:\\cycle-graph.dot", "w+");
#else
        mStream = popen("dotty -", "w");
#endif
        mVisited.Init();
        fprintf(mStream, 
                "digraph collection {\n"
                "rankdir=LR\n"
                "node [fontname=fixed, fontsize=10, style=filled, shape=box]\n"
                );
    }

    ~graphVizWalker()
    {
        fprintf(mStream, "\n}\n");
#ifdef WIN32
        fclose(mStream);
        
        
        
        _spawnlp(_P_WAIT, 
                 "lefty", 
                 "lefty",
                 "-e",
                 "\"load('dotty.lefty');"
                 "dotty.simple('c:\\cycle-graph.dot');\"",
                 NULL);
        unlink("c:\\cycle-graph.dot");
#else
        pclose(mStream);
#endif
    }

    PRBool ShouldVisitNode(void *p, PtrInfo const & pi)  
    { 
        return ! mVisited.GetEntry(p);
    }

    void VisitNode(void *p, PtrInfo & pi, size_t refcount) 
    {
        mVisited.PutEntry(p);
        mParent = p;
        fprintf(mStream, 
                "n%p [label=\"%s\\n%p\\n%u/%u refs found\", "
                "fillcolor=%s, fontcolor=%s]\n", 
                p, pi.mName, 
                p,
                pi.mInternalRefs, pi.mRefCount, 
                (pi.mColor == black ? "black" : "white"),
                (pi.mColor == black ? "white" : "black"));
    }

    void NoteChild(void *c, PtrInfo & childpi)
    { 
        fprintf(mStream, "n%p -> n%p\n", mParent, c);
    }
};








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






nsCycleCollector::nsCycleCollector() : 
    mCollectionInProgress(PR_FALSE),
    mScanInProgress(PR_FALSE),
    mPurpleBuf(mParams, mStats),
    mPtrLog(nsnull)
{
    mGraph.Init();
#ifdef DEBUG
    mExpectedGarbage.Init();
#endif

    memset(mRuntimes, 0, sizeof(mRuntimes));
    mRuntimes[nsIProgrammingLanguage::CPLUSPLUS] = &mXPCOMRuntime;
}


nsCycleCollector::~nsCycleCollector()
{
    mGraph.Clear();    

    for (PRUint32 i = 0; i < nsIProgrammingLanguage::MAX+1; ++i) {
        mRuntimes[i] = NULL;
    }
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


void 
nsCycleCollector::MaybeDrawGraphs()
{
    if (mParams.mDrawGraphs) {

        PRUint32 i;
        nsDeque roots(nsnull);

        while (mBufs[0].GetSize() > 0)
            roots.Push(mBufs[0].Pop());

        for (i = 0; i < nsIProgrammingLanguage::MAX+1; ++i)
            mBufs[i].Empty();

        mGraph.Enumerate(FindWhiteCallback, this);

        
        PRBool anyWhites = PR_FALSE;

        for (i = 0; i < nsIProgrammingLanguage::MAX+1; ++i) {
            if (mBufs[i].GetSize() > 0) {
                anyWhites = PR_TRUE;
                break;
            }
        }

        if (anyWhites) {
            graphVizWalker gw(mGraph, mRuntimes);
            while (roots.GetSize() > 0) {
                nsISupports *s = NS_STATIC_CAST(nsISupports *, roots.Pop());
                s = canonicalize(s);
                gw.Walk(s);
            }
        }

        for (i = 0; i < nsIProgrammingLanguage::MAX+1; ++i)
            mBufs[i].Empty();
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
            nsCOMPtr<nsCycleCollectionParticipant> cp = do_QueryInterface(s, &rv);
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
};

char *Suppressor::sSuppressionList = nsnull;
PRBool Suppressor::sInitialized = PR_FALSE;

static PRBool
nsCycleCollector_shouldSuppress(nsISupports *s)
{
    Suppressor supp;
    return supp.shouldSuppress(s);
}

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
        mBufs[0].Push(n);
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

#ifdef DEBUC_CC
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


void
nsCycleCollector::Collect(PRUint32 aTryCollections)
{
#ifndef __MINGW32__
    if (!mParams.mDoNothing && mParams.mHookMalloc)
        InitMemHook();
#endif

#ifdef COLLECT_TIME_DEBUG
    printf("cc: Starting nsCycleCollector::Collect(%d)\n", aTryCollections);
    PRTime start = PR_Now(), now;
#endif

    while (aTryCollections > 0) {
        
        
        
        
        
        
        
        
        mBufs[0].Empty();

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

            CollectPurple();

#ifdef COLLECT_TIME_DEBUG
            printf("cc: CollectPurple() took %lldms\n",
                   (PR_Now() - now) / PR_USEC_PER_MSEC);
#endif

            if (mBufs[0].GetSize() == 0) {
                aTryCollections = 0;
            } else {
                if (mCollectionInProgress)
                    Fault("re-entered collection");

                mCollectionInProgress = PR_TRUE;

                mScanInProgress = PR_TRUE;

                mGraph.Clear();

                

#ifdef COLLECT_TIME_DEBUG
                now = PR_Now();
#endif
                MarkRoots();

#ifdef COLLECT_TIME_DEBUG
                {
                    PRTime then = PR_Now();
                    printf("cc: MarkRoots() took %lldms\n",
                           (then - now) / PR_USEC_PER_MSEC);
                    now = then;
                }
#endif

                ScanRoots();

#ifdef COLLECT_TIME_DEBUG
                printf("cc: ScanRoots() took %lldms\n",
                       (PR_Now() - now) / PR_USEC_PER_MSEC);
#endif

                MaybeDrawGraphs();

                mScanInProgress = PR_FALSE;


#ifdef COLLECT_TIME_DEBUG
                now = PR_Now();
#endif
                CollectWhite();

#ifdef COLLECT_TIME_DEBUG
                printf("cc: CollectWhite() took %lldms\n",
                       (PR_Now() - now) / PR_USEC_PER_MSEC);
#endif

                

                mGraph.Clear();

                --aTryCollections;
            }

            mPurpleBuf.BumpGeneration();
            mStats.mCollection++;
            if (mParams.mReportStats)
                mStats.Dump();
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
    Collect(mParams.mShutdownCollections);

#ifdef DEBUG
    CollectPurple();
    if (mBufs[0].GetSize() != 0) {
        printf("Might have been able to release more cycles if the cycle collector would "
               "run once more at shutdown.\n");
    }

    ExplainLiveExpectedGarbage();
#endif
    mParams.mDoNothing = PR_TRUE;
}

#ifdef DEBUG

PR_STATIC_CALLBACK(PLDHashOperator)
AddExpectedGarbage(nsClearingVoidPtrHashKey *p, void *arg)
{
    nsCycleCollector *c = NS_STATIC_CAST(nsCycleCollector*, arg);
    c->mBufs[0].Push(NS_CONST_CAST(void*, p->GetKey()));
    return PL_DHASH_NEXT;
}

struct explainWalker : public GraphWalker
{
    explainWalker(GCTable &tab,
                  nsCycleCollectionLanguageRuntime **runtimes)
        : GraphWalker(tab, runtimes) 
    {}

    PRBool ShouldVisitNode(void *p, PtrInfo const & pi) 
    { 
        
        return pi.mColor != grey; 
    }

    void VisitNode(void *p, PtrInfo & pi, size_t refcount) 
    {
        if (pi.mColor == grey)
            Fault("scanning grey node", p);

        if (pi.mColor == white) {
            printf("nsCycleCollector: %s %p was not collected due to\n"
                   "  missing call to suspect or failure to unlink\n",
                   pi.mName, p);
        }

        if (pi.mInternalRefs != refcount) {
            
            
            
            
            printf("nsCycleCollector: %s %p was not collected due to %d\n"
                   "  external references\n",
                   pi.mName, p, refcount - pi.mInternalRefs);
        }

        pi.mColor = grey;

        mGraph.Put(p, pi);
    }
    void NoteChild(void *c, PtrInfo & childpi) {}
};

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

    mGraph.Clear();
    mBufs[0].Empty();

    
    
    mExpectedGarbage.EnumerateEntries(&AddExpectedGarbage, this);

    MarkRoots();
    ScanRoots();

    mScanInProgress = PR_FALSE;

    for (int i = 0; i < mBufs[0].GetSize(); ++i) {
        nsISupports *s = NS_STATIC_CAST(nsISupports *, mBufs[0].ObjectAt(i));
        s = canonicalize(s);
        explainWalker(mGraph, mRuntimes).Walk(s); 
    }

    mGraph.Clear();

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
    if (sCollector)
        sCollector->ShouldBeFreed(n);
}

void
nsCycleCollector_DEBUG_wasFreed(nsISupports *n)
{
    if (sCollector)
        sCollector->WasFreed(n);
}
#endif

PRBool
nsCycleCollector_isScanSafe(nsISupports *s)
{
    if (!s)
        return PR_FALSE;

    nsCycleCollectionParticipant *cp;
    ToParticipant(s, &cp);

    return cp != nsnull;
}

static void
ToParticipant(nsISupports *s, nsCycleCollectionParticipant **cp)
{
    
    
    
    
    CallQueryInterface(s, cp);
    if (cp)
        ++sCollector->mStats.mSuccessfulQI;
    else
        ++sCollector->mStats.mFailedQI;
}
