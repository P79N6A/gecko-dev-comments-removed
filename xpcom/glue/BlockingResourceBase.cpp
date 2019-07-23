





































#ifdef DEBUG

#include "plhash.h"
#include "prprf.h"
#include "prlock.h"
#include "prthread.h"
#include "nsDebug.h"
#include "nsVoidArray.h"

#include "mozilla/BlockingResourceBase.h"
#include "mozilla/CondVar.h"
#include "mozilla/Monitor.h"
#include "mozilla/Mutex.h"

#ifdef NS_TRACE_MALLOC
# include <stdio.h>
# include "nsTraceMalloc.h"
#endif

static PRUintn      ResourceAcqnChainFrontTPI = (PRUintn)-1;
static PLHashTable* OrderTable = 0;
static PRLock*      OrderTableLock = 0;
static PRLock*      ResourceMutex = 0;


static char const *const kResourceTypeNames[] = {
    "Mutex", "Monitor", "CondVar"
};

struct nsNamedVector : public nsVoidArray {
    const char* mName;

#ifdef NS_TRACE_MALLOC
    
    
    nsVoidArray mInnerSites;
#endif

    nsNamedVector(const char* name = 0, PRUint32 initialSize = 0)
        : nsVoidArray(initialSize),
          mName(name)
    {
    }
};

static void *
_hash_alloc_table(void *pool, PRSize size)
{
    return operator new(size);
}

static void 
_hash_free_table(void *pool, void *item)
{
    operator delete(item);
}

static PLHashEntry *
_hash_alloc_entry(void *pool, const void *key)
{
    return new PLHashEntry;
}



















static void 
_hash_free_entry(void *pool, PLHashEntry *entry, PRUintn flag)
{
    nsNamedVector* vec = (nsNamedVector*) entry->value;
    if (vec) {
        entry->value = 0;
        delete vec;
    }
    if (flag == HT_FREE_ENTRY)
        delete entry;
}

static const PLHashAllocOps _hash_alloc_ops = {
    _hash_alloc_table, _hash_free_table,
    _hash_alloc_entry, _hash_free_entry
};

static PRIntn
_purge_one(PLHashEntry* he, PRIntn cnt, void* arg)
{
    nsNamedVector* vec = (nsNamedVector*) he->value;

    if (he->key == arg)
        return HT_ENUMERATE_REMOVE;
    vec->RemoveElement(arg);
    return HT_ENUMERATE_NEXT;
}

static void
OnResourceRecycle(void* aResource)
{
    NS_PRECONDITION(OrderTable, "should be inited!");

    PR_Lock(OrderTableLock);
    PL_HashTableEnumerateEntries(OrderTable, _purge_one, aResource);
    PR_Unlock(OrderTableLock);
}

static PLHashNumber
_hash_pointer(const void* key)
{
    return PLHashNumber(NS_PTR_TO_INT32(key)) >> 2;
}


#include "prcmon.h"


static void InitAutoLockStatics()
{
    (void) PR_NewThreadPrivateIndex(&ResourceAcqnChainFrontTPI, 0);
    OrderTable = PL_NewHashTable(64, _hash_pointer,
                                 PL_CompareValues, PL_CompareValues,
                                 &_hash_alloc_ops, 0);
    if (OrderTable && !(OrderTableLock = PR_NewLock())) {
        PL_HashTableDestroy(OrderTable);
        OrderTable = 0;
    }

    if (OrderTable && !(ResourceMutex = PR_NewLock())) {
        PL_HashTableDestroy(OrderTable);
        OrderTable = 0;
    }

    
    PR_CSetOnMonitorRecycle(OnResourceRecycle);
}





#if 0
void _FreeAutoLockStatics()
{
    PLHashTable* table = OrderTable;
    if (!table) return;

    
    
    PR_CSetOnMonitorRecycle(0);
    PR_DestroyLock(OrderTableLock);
    OrderTableLock = 0;
    PL_HashTableDestroy(table);
    OrderTable = 0;
}
#endif


static nsNamedVector* GetVector(PLHashTable* table, const void* key)
{
    PLHashNumber hash = _hash_pointer(key);
    PLHashEntry** hep = PL_HashTableRawLookup(table, hash, key);
    PLHashEntry* he = *hep;
    if (he)
        return (nsNamedVector*) he->value;
    nsNamedVector* vec = new nsNamedVector();
    if (vec)
        PL_HashTableRawAdd(table, hep, hash, key, vec);
    return vec;
}

static void OnResourceCreated(const void* key, const char* name )
{
    NS_PRECONDITION(key && OrderTable, "should be inited!");

    nsNamedVector* value = new nsNamedVector(name);
    if (value) {
        PR_Lock(OrderTableLock);
        PL_HashTableAdd(OrderTable, key, value);
        PR_Unlock(OrderTableLock);
    }
}


static PRBool Reachable(PLHashTable* table, const void* goal, const void* start)
{
    PR_ASSERT(goal);
    PR_ASSERT(start);
    nsNamedVector* vec = GetVector(table, start);
    for (PRUint32 i = 0, n = vec->Count(); i < n; i++) {
        void* aResource = vec->ElementAt(i);
        if (aResource == goal || Reachable(table, goal, aResource))
            return PR_TRUE;
    }
    return PR_FALSE;
}

static PRBool WellOrdered(const void* aResource1, const void* aResource2,
                          const void *aCallsite2, PRUint32* aIndex2p,
                          nsNamedVector** aVec1p, nsNamedVector** aVec2p)
{
    NS_ASSERTION(OrderTable && OrderTableLock, "supposed to be initialized!");

    PRBool rv = PR_TRUE;
    PLHashTable* table = OrderTable;

    PR_Lock(OrderTableLock);

    
    nsNamedVector* vec1 = GetVector(table, aResource1);
    if (vec1) {
        PRUint32 i, n;

        for (i = 0, n = vec1->Count(); i < n; i++)
            if (vec1->ElementAt(i) == aResource2)
                break;

        if (i == n) {
            
            nsNamedVector* vec2 = GetVector(table, aResource2);
            if (vec2) {
                for (i = 0, n = vec2->Count(); i < n; i++) {
                    void* aResourcei = vec2->ElementAt(i);
                    PR_ASSERT(aResourcei);
                    if (aResourcei == aResource1 
                        || Reachable(table, aResource1, aResourcei)) {
                        *aIndex2p = i;
                        *aVec1p = vec1;
                        *aVec2p = vec2;
                        rv = PR_FALSE;
                        break;
                    }
                }

                if (rv) {
                    
                    
                    vec1->AppendElement((void*) aResource2);
#ifdef NS_TRACE_MALLOC
                    vec1->mInnerSites.AppendElement((void*) aCallsite2);
#endif
                }
            }
        }
    }

    

    PR_Unlock(OrderTableLock);
    return rv;
}

















namespace mozilla {


void
BlockingResourceBase::Init(void* aResource, 
                           const char* aName, 
                           BlockingResourceBase::BlockingResourceType aType)
{
    if (NS_UNLIKELY(ResourceAcqnChainFrontTPI == PRUintn(-1)))
        InitAutoLockStatics();

    NS_PRECONDITION(aResource, "null resource");

    OnResourceCreated(aResource, aName);
    mResource = aResource;
    mChainPrev = 0;
    mType = aType;
}

BlockingResourceBase::~BlockingResourceBase()
{
    NS_PRECONDITION(mResource, "bad subclass impl, or double free");

    
    
    
    
    
    
    
    
    

    OnResourceRecycle(mResource);
    mResource = 0;
    mChainPrev = 0;
}

void BlockingResourceBase::Acquire()
{
    NS_PRECONDITION(mResource, "bad base class impl or double free");
    PR_ASSERT_CURRENT_THREAD_OWNS_LOCK(ResourceMutex);

    if (mType == eCondVar) {
        NS_NOTYETIMPLEMENTED("TODO: figure out how to Acquire() condvars");
        return;
    }

    BlockingResourceBase* chainFront = 
        (BlockingResourceBase*) 
            PR_GetThreadPrivate(ResourceAcqnChainFrontTPI);

    if (eMonitor == mType) {
        
        
        
        

        if (this == chainFront) {
            
            return;
        } 
        else if (chainFront) {
            BlockingResourceBase* br = chainFront->mChainPrev;
            while (br) {
                if (br == this) {
                    NS_ASSERTION(br != this, 
                                 "reentered monitor after acquiring "
                                 "other resources");
                    
                    
                    
                    
                    
                    return;
                }
                br = br->mChainPrev;
            }
        }
    }

    const void* callContext =
#ifdef NS_TRACE_MALLOC
        (const void*)NS_TraceMallocGetStackTrace();
#else
        nsnull
#endif
        ;

    if (eMutex == mType
        && chainFront == this
        && !(chainFront->mChainPrev)) {
        
        
        
        
        
        
        
        char buf[128];
        PR_snprintf(buf, sizeof buf,
                    "Imminent deadlock between Mutex@%p and itself!",
                    chainFront->mResource);

#ifdef NS_TRACE_MALLOC
        fputs("\nDeadlock will occur here:\n", stderr);
        NS_TraceMallocPrintStackTrace(
            stderr, (nsTMStackTraceIDStruct*)callContext);
        putc('\n', stderr);
#endif

        NS_ERROR(buf);

        return;                 
    }

    nsNamedVector* vec1;
    nsNamedVector* vec2;
    PRUint32 i2;

    if (!chainFront 
        || WellOrdered(chainFront->mResource, mResource, 
                       callContext, 
                       &i2, 
                       &vec1, &vec2)) {
        mChainPrev = chainFront;
        PR_SetThreadPrivate(ResourceAcqnChainFrontTPI, this);
    }
    else {
        char buf[128];
        PR_snprintf(buf, sizeof buf,
                    "Potential deadlock between %s%s@%p and %s%s@%p",
                    vec1->mName ? vec1->mName : "",
                    kResourceTypeNames[chainFront->mType],
                    chainFront->mResource,
                    vec2->mName ? vec2->mName : "",
                    kResourceTypeNames[mType],
                    mResource);

#ifdef NS_TRACE_MALLOC
        fprintf(stderr, "\n*** %s\n\nStack of current acquisition:\n", buf);
        NS_TraceMallocPrintStackTrace(
            stderr, NS_TraceMallocGetStackTrace());

        fputs("\nStack of conflicting acquisition:\n", stderr);
        NS_TraceMallocPrintStackTrace(
            stderr, (nsTMStackTraceIDStruct *)vec2->mInnerSites.ElementAt(i2));
        putc('\n', stderr);
#endif

        NS_ERROR(buf);

        
        
        
        
    }
}

void BlockingResourceBase::Release()
{
    NS_PRECONDITION(mResource, "bad base class impl or double free");
    PR_ASSERT_CURRENT_THREAD_OWNS_LOCK(ResourceMutex);

    if (mType == eCondVar) {
        NS_NOTYETIMPLEMENTED("TODO: figure out how to Release() condvars");
        return;
    }

    BlockingResourceBase* chainFront = 
        (BlockingResourceBase*) 
            PR_GetThreadPrivate(ResourceAcqnChainFrontTPI);
    NS_ASSERTION(chainFront, 
                 "Release()ing something that hasn't been Acquire()ed");

    if (NS_LIKELY(chainFront == this)) {
        PR_SetThreadPrivate(ResourceAcqnChainFrontTPI, mChainPrev);
    }
    else {
        
        NS_WARNING("you're releasing a resource in non-LIFO order; why?");

        
        
        
        
        BlockingResourceBase* curr = chainFront;
        BlockingResourceBase* prev = nsnull;
        while (curr && (prev = curr->mChainPrev) && (prev != this))
            curr = prev;
        if (prev == this)
            curr->mChainPrev = prev->mChainPrev;
    }
}



void
Mutex::Lock()
{
    PR_Lock(ResourceMutex);
    Acquire();
    PR_Unlock(ResourceMutex);

    PR_Lock(mLock);
}

void
Mutex::Unlock()
{
    PRStatus status = PR_Unlock(mLock);
    NS_ASSERTION(PR_SUCCESS == status, "problem Unlock()ing");

    PR_Lock(ResourceMutex);
    Release();
    PR_Unlock(ResourceMutex);
}



void 
Monitor::Enter() 
{
    PR_Lock(ResourceMutex);
    ++mEntryCount;
    Acquire();
    PR_Unlock(ResourceMutex);

    PR_EnterMonitor(mMonitor);
}

void
Monitor::Exit()
{
    PRStatus status = PR_ExitMonitor(mMonitor);
    NS_ASSERTION(PR_SUCCESS == status, "bad ExitMonitor()");

    PR_Lock(ResourceMutex);
    if (--mEntryCount == 0)
        Release();
    PR_Unlock(ResourceMutex);
}


} 


#endif 
