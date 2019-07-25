








































#include <limits.h>

#include "nscore.h"
#include "nsDiskCacheBinding.h"







struct HashTableEntry : PLDHashEntryHdr {
    nsDiskCacheBinding *  mBinding;
};


static PLDHashNumber
HashKey( PLDHashTable *table, const void *key)
{
    return (PLDHashNumber) NS_PTR_TO_INT32(key);
}


static PRBool
MatchEntry(PLDHashTable *              ,
            const PLDHashEntryHdr *       header,
            const void *                  key)
{
    HashTableEntry * hashEntry = (HashTableEntry *) header;
    return (hashEntry->mBinding->mRecord.HashNumber() == (PLDHashNumber) NS_PTR_TO_INT32(key));
}

static void
MoveEntry(PLDHashTable *           ,
          const PLDHashEntryHdr *     src,
          PLDHashEntryHdr       *     dst)
{
    ((HashTableEntry *)dst)->mBinding = ((HashTableEntry *)src)->mBinding;
}


static void
ClearEntry(PLDHashTable *      ,
           PLDHashEntryHdr *      header)
{
    ((HashTableEntry *)header)->mBinding = nsnull;
}





nsDiskCacheBinding *
GetCacheEntryBinding(nsCacheEntry * entry)
{
    return (nsDiskCacheBinding *) entry->Data();
}






NS_IMPL_THREADSAFE_ISUPPORTS0(nsDiskCacheBinding)

nsDiskCacheBinding::nsDiskCacheBinding(nsCacheEntry* entry, nsDiskCacheRecord * record)
    :   mCacheEntry(entry)
    ,   mStreamIO(nsnull)
    ,   mDeactivateEvent(nsnull)
{
    NS_ASSERTION(record->ValidRecord(), "bad record");
    PR_INIT_CLIST(this);
    mRecord     = *record;
    mDoomed     = entry->IsDoomed();
    mGeneration = record->Generation();    
}

nsDiskCacheBinding::~nsDiskCacheBinding()
{
    NS_ASSERTION(PR_CLIST_IS_EMPTY(this), "binding deleted while still on list");
    if (!PR_CLIST_IS_EMPTY(this))
        PR_REMOVE_LINK(this);       
    
    
    if (mStreamIO) {
        mStreamIO->ClearBinding();
        NS_RELEASE(mStreamIO);
    }
}

nsresult
nsDiskCacheBinding::EnsureStreamIO()
{
    if (!mStreamIO) {
        mStreamIO = new nsDiskCacheStreamIO(this);
        if (!mStreamIO)  return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(mStreamIO);
    }
    return NS_OK;
}









PLDHashTableOps nsDiskCacheBindery::ops =
{
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    HashKey,
    MatchEntry,
    MoveEntry,
    ClearEntry,
    PL_DHashFinalizeStub
};


nsDiskCacheBindery::nsDiskCacheBindery()
    : initialized(PR_FALSE)
{
}


nsDiskCacheBindery::~nsDiskCacheBindery()
{
    Reset();
}


nsresult
nsDiskCacheBindery::Init()
{
    nsresult rv = NS_OK;
    initialized = PL_DHashTableInit(&table, &ops, nsnull, sizeof(HashTableEntry), 0);

    if (!initialized) rv = NS_ERROR_OUT_OF_MEMORY;
    
    return rv;
}

void
nsDiskCacheBindery::Reset()
{
    if (initialized) {
        PL_DHashTableFinish(&table);
        initialized = PR_FALSE;
    }
}


nsDiskCacheBinding *
nsDiskCacheBindery::CreateBinding(nsCacheEntry *       entry,
                                  nsDiskCacheRecord *  record)
{
    NS_ASSERTION(initialized, "nsDiskCacheBindery not initialized");
    nsCOMPtr<nsISupports> data = entry->Data();
    if (data) {
        NS_ERROR("cache entry already has bind data");
        return nsnull;
    }
    
    nsDiskCacheBinding * binding = new nsDiskCacheBinding(entry, record);
    if (!binding)  return nsnull;
        
    
    entry->SetData(binding);
    
    
    nsresult rv = AddBinding(binding);
    if (NS_FAILED(rv)) {
        entry->SetData(nsnull);
        return nsnull;
    }

    return binding;
}





nsDiskCacheBinding *
nsDiskCacheBindery::FindActiveBinding(PRUint32  hashNumber)
{
    NS_ASSERTION(initialized, "nsDiskCacheBindery not initialized");
    
    HashTableEntry * hashEntry;
    hashEntry = (HashTableEntry *) PL_DHashTableOperate(&table, (void*) hashNumber, PL_DHASH_LOOKUP);
    if (PL_DHASH_ENTRY_IS_FREE(hashEntry)) return nsnull;
    
    
    NS_ASSERTION(hashEntry->mBinding, "hash entry left with no binding");
    nsDiskCacheBinding * binding = hashEntry->mBinding;    
    while (binding->mCacheEntry->IsDoomed()) {
        binding = (nsDiskCacheBinding *)PR_NEXT_LINK(binding);
        if (binding == hashEntry->mBinding)  return nsnull;
    }
    return binding;
}












nsresult
nsDiskCacheBindery::AddBinding(nsDiskCacheBinding * binding)
{
    NS_ENSURE_ARG_POINTER(binding);
    NS_ASSERTION(initialized, "nsDiskCacheBindery not initialized");

    
    HashTableEntry * hashEntry;
    hashEntry = (HashTableEntry *) PL_DHashTableOperate(&table,
                                                        (void*) binding->mRecord.HashNumber(),
                                                        PL_DHASH_ADD);
    if (!hashEntry) return NS_ERROR_OUT_OF_MEMORY;
    
    if (hashEntry->mBinding == nsnull) {
        hashEntry->mBinding = binding;
        if (binding->mGeneration == 0)
            binding->mGeneration = 1;   
            
        return NS_OK;
    }
    
    
    
    nsDiskCacheBinding * p  = hashEntry->mBinding;
    PRBool   calcGeneration = (binding->mGeneration == 0);  
    if (calcGeneration)  binding->mGeneration = 1;          
    while (1) {
    
        if (binding->mGeneration < p->mGeneration) {
            
            PR_INSERT_BEFORE(binding, p);
            if (hashEntry->mBinding == p)
                hashEntry->mBinding = binding;
            break;
        }
        
        if (binding->mGeneration == p->mGeneration) {
            if (calcGeneration)  ++binding->mGeneration;    
            else {
                NS_ERROR("### disk cache: generations collide!");
                return NS_ERROR_UNEXPECTED;
            }
        }

        p = (nsDiskCacheBinding *)PR_NEXT_LINK(p);
        if (p == hashEntry->mBinding) {
            
            p = (nsDiskCacheBinding *)PR_PREV_LINK(p);  
            if (p->mGeneration == 255) {
                NS_WARNING("### disk cache: generation capacity at full");
                return NS_ERROR_UNEXPECTED;
            }
            PR_INSERT_BEFORE(binding, hashEntry->mBinding);
            break;
        }
    }
    return NS_OK;
}





void
nsDiskCacheBindery::RemoveBinding(nsDiskCacheBinding * binding)
{
    NS_ASSERTION(initialized, "nsDiskCacheBindery not initialized");
    if (!initialized)   return;
    
    HashTableEntry * hashEntry;
    void *           key = (void *)binding->mRecord.HashNumber();

    hashEntry = (HashTableEntry*) PL_DHashTableOperate(&table,
                                                       (void*) key,
                                                       PL_DHASH_LOOKUP);
    if (!PL_DHASH_ENTRY_IS_BUSY(hashEntry)) {
        NS_WARNING("### disk cache: binding not in hashtable!");
        return;
    }
    
    if (binding == hashEntry->mBinding) {
        if (PR_CLIST_IS_EMPTY(binding)) {
            
            (void) PL_DHashTableOperate(&table,
                                        (void*) binding->mRecord.HashNumber(),
                                        PL_DHASH_REMOVE);
            return;
            
        } else {
            
            hashEntry->mBinding = (nsDiskCacheBinding *)PR_NEXT_LINK(binding);
        }
    }
    PR_REMOVE_AND_INIT_LINK(binding);
}






PLDHashOperator
ActiveBinding(PLDHashTable *    table,
              PLDHashEntryHdr * hdr,
              PRUint32          number,
              void *            arg)
{
    nsDiskCacheBinding * binding = ((HashTableEntry *)hdr)->mBinding;
    NS_ASSERTION(binding, "### disk cache binding = nsnull!");
    
    nsDiskCacheBinding * head = binding;
    do {   
        if (binding->IsActive()) {
           *((PRBool *)arg) = PR_TRUE;
            return PL_DHASH_STOP;
        }

        binding = (nsDiskCacheBinding *)PR_NEXT_LINK(binding);
    } while (binding != head);

    return PL_DHASH_NEXT;
}





PRBool
nsDiskCacheBindery::ActiveBindings()
{
    NS_ASSERTION(initialized, "nsDiskCacheBindery not initialized");
    if (!initialized) return PR_FALSE;

    PRBool  activeBinding = PR_FALSE;
    PL_DHashTableEnumerate(&table, ActiveBinding, &activeBinding);

    return activeBinding;
}
