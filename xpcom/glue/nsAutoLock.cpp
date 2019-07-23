




































#include "nsAutoLock.h"

#ifdef DEBUG

#include "plhash.h"
#include "prprf.h"
#include "prlock.h"
#include "prthread.h"
#include "nsDebug.h"
#include "nsVoidArray.h"

#ifdef NS_TRACE_MALLOC_XXX
# include <stdio.h>
# include "nsTraceMalloc.h"
#endif

static PRUintn      LockStackTPI = (PRUintn)-1;
static PLHashTable* OrderTable = 0;
static PRLock*      OrderTableLock = 0;

static const char* const LockTypeNames[] = {"Lock", "Monitor", "CMonitor"};

struct nsNamedVector : public nsVoidArray {
    const char* mName;

#ifdef NS_TRACE_MALLOC_XXX
    
    
    nsVoidArray mInnerSites;
#endif

    nsNamedVector(const char* name = 0, PRUint32 initialSize = 0)
        : nsVoidArray(initialSize),
          mName(name)
    {
    }
};

static void * PR_CALLBACK
_hash_alloc_table(void *pool, PRSize size)
{
    return operator new(size);
}

static void  PR_CALLBACK
_hash_free_table(void *pool, void *item)
{
    operator delete(item);
}

static PLHashEntry * PR_CALLBACK
_hash_alloc_entry(void *pool, const void *key)
{
    return new PLHashEntry;
}



















static void  PR_CALLBACK
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

PR_STATIC_CALLBACK(PRIntn)
_purge_one(PLHashEntry* he, PRIntn cnt, void* arg)
{
    nsNamedVector* vec = (nsNamedVector*) he->value;

    if (he->key == arg)
        return HT_ENUMERATE_REMOVE;
    vec->RemoveElement(arg);
    return HT_ENUMERATE_NEXT;
}

PR_STATIC_CALLBACK(void)
OnSemaphoreRecycle(void* addr)
{
    if (OrderTable) { 
        PR_Lock(OrderTableLock);
        PL_HashTableEnumerateEntries(OrderTable, _purge_one, addr);
        PR_Unlock(OrderTableLock);
    }
}

PR_STATIC_CALLBACK(PLHashNumber)
_hash_pointer(const void* key)
{
    return PLHashNumber(NS_PTR_TO_INT32(key)) >> 2;
}


static void InitAutoLockStatics()
{
    (void) PR_NewThreadPrivateIndex(&LockStackTPI, 0);
    OrderTable = PL_NewHashTable(64, _hash_pointer,
                                 PL_CompareValues, PL_CompareValues,
                                 &_hash_alloc_ops, 0);
    if (OrderTable && !(OrderTableLock = PR_NewLock())) {
        PL_HashTableDestroy(OrderTable);
        OrderTable = 0;
    }
    PR_CSetOnMonitorRecycle(OnSemaphoreRecycle);
}

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

static void OnSemaphoreCreated(const void* key, const char* name )
{
    if (key && OrderTable) {
        nsNamedVector* value = new nsNamedVector(name);
        if (value) {
            PR_Lock(OrderTableLock);
            PL_HashTableAdd(OrderTable, key, value);
            PR_Unlock(OrderTableLock);
        }
    }
}


static PRBool Reachable(PLHashTable* table, const void* goal, const void* start)
{
    PR_ASSERT(goal);
    PR_ASSERT(start);
    nsNamedVector* vec = GetVector(table, start);
    for (PRUint32 i = 0, n = vec->Count(); i < n; i++) {
        void* addr = vec->ElementAt(i);
        if (addr == goal || Reachable(table, goal, addr))
            return PR_TRUE;
    }
    return PR_FALSE;
}

static PRBool WellOrdered(const void* addr1, const void* addr2,
                          const void *callsite2, PRUint32* index2p,
                          nsNamedVector** vec1p, nsNamedVector** vec2p)
{
    PRBool rv = PR_TRUE;
    PLHashTable* table = OrderTable;
    if (!table) return rv;
    PR_Lock(OrderTableLock);

    
    nsNamedVector* vec1 = GetVector(table, addr1);
    if (vec1) {
        PRUint32 i, n;

        for (i = 0, n = vec1->Count(); i < n; i++)
            if (vec1->ElementAt(i) == addr2)
                break;

        if (i == n) {
            
            nsNamedVector* vec2 = GetVector(table, addr2);
            if (vec2) {
                for (i = 0, n = vec2->Count(); i < n; i++) {
                    void* addri = vec2->ElementAt(i);
                    PR_ASSERT(addri);
                    if (addri == addr1 || Reachable(table, addr1, addri)) {
                        *index2p = i;
                        *vec1p = vec1;
                        *vec2p = vec2;
                        rv = PR_FALSE;
                        break;
                    }
                }

                if (rv) {
                    
                    
                    vec1->AppendElement((void*) addr2);
#ifdef NS_TRACE_MALLOC_XXX
                    vec1->mInnerSites.AppendElement((void*) callsite2);
#endif
                }
            }
        }
    }

    PR_Unlock(OrderTableLock);
    return rv;
}

nsAutoLockBase::nsAutoLockBase(void* addr, nsAutoLockType type)
{
    if (LockStackTPI == PRUintn(-1))
        InitAutoLockStatics();

    nsAutoLockBase* stackTop =
        (nsAutoLockBase*) PR_GetThreadPrivate(LockStackTPI);
    if (stackTop) {
        if (stackTop->mAddr == addr) {
            
            
        } else if (!addr) {
            
            
        } else {
            const void* node =
#ifdef NS_TRACE_MALLOC_XXX
                NS_GetStackTrace(1)
#else
                nsnull
#endif
                ;
            nsNamedVector* vec1;
            nsNamedVector* vec2;
            PRUint32 i2;

            if (!WellOrdered(stackTop->mAddr, addr, node, &i2, &vec1, &vec2)) {
                char buf[128];
                PR_snprintf(buf, sizeof buf,
                            "Potential deadlock between %s%s@%p and %s%s@%p",
                            vec1->mName ? vec1->mName : "",
                            LockTypeNames[stackTop->mType],
                            stackTop->mAddr,
                            vec2->mName ? vec2->mName : "",
                            LockTypeNames[type],
                            addr);
#ifdef NS_TRACE_MALLOC_XXX
                fprintf(stderr, "\n*** %s\n\nCurrent stack:\n", buf);
                NS_DumpStackTrace(node, stderr);

                fputs("\nPrevious stack:\n", stderr);
                NS_DumpStackTrace(vec2->mInnerSites.ElementAt(i2), stderr);
                putc('\n', stderr);
#endif
                NS_ERROR(buf);
            }
        }
    }

    mAddr = addr;
    mDown = stackTop;
    mType = type;
    if (mAddr)
        (void) PR_SetThreadPrivate(LockStackTPI, this);
}

nsAutoLockBase::~nsAutoLockBase()
{
    if (mAddr)
        (void) PR_SetThreadPrivate(LockStackTPI, mDown);
}

void nsAutoLockBase::Show()
{
    if (!mAddr)
        return;
    nsAutoLockBase* curr = (nsAutoLockBase*) PR_GetThreadPrivate(LockStackTPI);
    nsAutoLockBase* prev = nsnull;
    while (curr != mDown) {
        prev = curr;
        curr = prev->mDown;
    }
    if (!prev)
        PR_SetThreadPrivate(LockStackTPI, this);
    else
        prev->mDown = this;
}

void nsAutoLockBase::Hide()
{
    if (!mAddr)
        return;
    nsAutoLockBase* curr = (nsAutoLockBase*) PR_GetThreadPrivate(LockStackTPI);
    nsAutoLockBase* prev = nsnull;
    while (curr != this) {
        prev = curr;
        curr = prev->mDown;
    }
    if (!prev)
        PR_SetThreadPrivate(LockStackTPI, mDown);
    else
        prev->mDown = mDown;
}

nsAutoUnlockBase::nsAutoUnlockBase(void* addr)
{
    if (addr)
    {
        nsAutoLockBase* curr = (nsAutoLockBase*) PR_GetThreadPrivate(LockStackTPI);
        while (curr && curr->mAddr != addr)
            curr = curr->mDown;

        mLock = curr;
    }
    else
        mLock = nsnull;

    if (mLock)
        mLock->Hide();
}

nsAutoUnlockBase::~nsAutoUnlockBase()
{
    if (mLock)
        mLock->Show();
}

#endif 

PRLock* nsAutoLock::NewLock(const char* name)
{
    PRLock* lock = PR_NewLock();
#ifdef DEBUG
    OnSemaphoreCreated(lock, name);
#endif
    return lock;
}

void nsAutoLock::DestroyLock(PRLock* lock)
{
#ifdef DEBUG
    OnSemaphoreRecycle(lock);
#endif
    PR_DestroyLock(lock);
}

PRMonitor* nsAutoMonitor::NewMonitor(const char* name)
{
    PRMonitor* mon = PR_NewMonitor();
#ifdef DEBUG
    OnSemaphoreCreated(mon, name);
#endif
    return mon;
}

void nsAutoMonitor::DestroyMonitor(PRMonitor* mon)
{
#ifdef DEBUG
    OnSemaphoreRecycle(mon);
#endif
    PR_DestroyMonitor(mon);
}

void nsAutoMonitor::Enter()
{
#ifdef DEBUG
    if (!mAddr) {
        NS_ERROR("It is not legal to enter a null monitor");
        return;
    }
    nsAutoLockBase* stackTop =
        (nsAutoLockBase*) PR_GetThreadPrivate(LockStackTPI);
    NS_ASSERTION(stackTop == mDown, "non-LIFO nsAutoMonitor::Enter");
    mDown = stackTop;
    (void) PR_SetThreadPrivate(LockStackTPI, this);
#endif
    PR_EnterMonitor(mMonitor);
    mLockCount += 1;
}

void nsAutoMonitor::Exit()
{
#ifdef DEBUG
    if (!mAddr) {
        NS_ERROR("It is not legal to exit a null monitor");
        return;
    }
    (void) PR_SetThreadPrivate(LockStackTPI, mDown);
#endif
    PRStatus status = PR_ExitMonitor(mMonitor);
    NS_ASSERTION(status == PR_SUCCESS, "PR_ExitMonitor failed");
    mLockCount -= 1;
}





void nsAutoCMonitor::Enter()
{
#ifdef DEBUG
    nsAutoLockBase* stackTop =
        (nsAutoLockBase*) PR_GetThreadPrivate(LockStackTPI);
    NS_ASSERTION(stackTop == mDown, "non-LIFO nsAutoCMonitor::Enter");
    mDown = stackTop;
    (void) PR_SetThreadPrivate(LockStackTPI, this);
#endif
    PR_CEnterMonitor(mLockObject);
    mLockCount += 1;
}

void nsAutoCMonitor::Exit()
{
#ifdef DEBUG
    (void) PR_SetThreadPrivate(LockStackTPI, mDown);
#endif
    PRStatus status = PR_CExitMonitor(mLockObject);
    NS_ASSERTION(status == PR_SUCCESS, "PR_CExitMonitor failed");
    mLockCount -= 1;
}
