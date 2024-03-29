





#include "mozilla/MemoryReporting.h"
#include "nsCache.h"
#include <limits.h>

#include "nscore.h"
#include "nsDiskCacheBinding.h"
#include "nsCacheService.h"







struct HashTableEntry : PLDHashEntryHdr {
    nsDiskCacheBinding *  mBinding;
};


static PLDHashNumber
HashKey( PLDHashTable *table, const void *key)
{
    return (PLDHashNumber) NS_PTR_TO_INT32(key);
}


static bool
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
    ((HashTableEntry *)header)->mBinding = nullptr;
}





nsDiskCacheBinding *
GetCacheEntryBinding(nsCacheEntry * entry)
{
    return (nsDiskCacheBinding *) entry->Data();
}






NS_IMPL_ISUPPORTS0(nsDiskCacheBinding)

nsDiskCacheBinding::nsDiskCacheBinding(nsCacheEntry* entry, nsDiskCacheRecord * record)
    :   mCacheEntry(entry)
    ,   mStreamIO(nullptr)
    ,   mDeactivateEvent(nullptr)
{
    NS_ASSERTION(record->ValidRecord(), "bad record");
    PR_INIT_CLIST(this);
    mRecord     = *record;
    mDoomed     = entry->IsDoomed();
    mGeneration = record->Generation();    
}

nsDiskCacheBinding::~nsDiskCacheBinding()
{
    
    
    
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSDISKCACHEBINDING_DESTRUCTOR));

    NS_ASSERTION(PR_CLIST_IS_EMPTY(this), "binding deleted while still on list");
    if (!PR_CLIST_IS_EMPTY(this))
        PR_REMOVE_LINK(this);       
    
    
    if (mStreamIO) {
        if (NS_FAILED(mStreamIO->ClearBinding()))
            nsCacheService::DoomEntry(mCacheEntry);
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









const PLDHashTableOps nsDiskCacheBindery::ops =
{
    HashKey,
    MatchEntry,
    MoveEntry,
    ClearEntry
};


nsDiskCacheBindery::nsDiskCacheBindery()
    : table(&ops, sizeof(HashTableEntry), kInitialTableLength)
    , initialized(false)
{
}


nsDiskCacheBindery::~nsDiskCacheBindery()
{
    Reset();
}


void
nsDiskCacheBindery::Init()
{
    table.ClearAndPrepareForLength(kInitialTableLength);
    initialized = true;
}

void
nsDiskCacheBindery::Reset()
{
    if (initialized) {
        table.ClearAndPrepareForLength(kInitialTableLength);
        initialized = false;
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
        return nullptr;
    }
    
    nsDiskCacheBinding * binding = new nsDiskCacheBinding(entry, record);
    if (!binding)  return nullptr;
        
    
    entry->SetData(binding);
    
    
    nsresult rv = AddBinding(binding);
    if (NS_FAILED(rv)) {
        entry->SetData(nullptr);
        return nullptr;
    }

    return binding;
}





nsDiskCacheBinding *
nsDiskCacheBindery::FindActiveBinding(uint32_t  hashNumber)
{
    NS_ASSERTION(initialized, "nsDiskCacheBindery not initialized");
    
    HashTableEntry * hashEntry;
    hashEntry =
      (HashTableEntry *) PL_DHashTableSearch(&table,
                                             (void*)(uintptr_t) hashNumber);
    if (!hashEntry) return nullptr;

    
    NS_ASSERTION(hashEntry->mBinding, "hash entry left with no binding");
    nsDiskCacheBinding * binding = hashEntry->mBinding;
    while (binding->mCacheEntry->IsDoomed()) {
        binding = (nsDiskCacheBinding *)PR_NEXT_LINK(binding);
        if (binding == hashEntry->mBinding)  return nullptr;
    }
    return binding;
}












nsresult
nsDiskCacheBindery::AddBinding(nsDiskCacheBinding * binding)
{
    NS_ENSURE_ARG_POINTER(binding);
    NS_ASSERTION(initialized, "nsDiskCacheBindery not initialized");

    
    HashTableEntry * hashEntry;
    hashEntry = (HashTableEntry *)
      PL_DHashTableAdd(&table,
                       (void *)(uintptr_t) binding->mRecord.HashNumber(),
                       fallible);
    if (!hashEntry) return NS_ERROR_OUT_OF_MEMORY;
    
    if (hashEntry->mBinding == nullptr) {
        hashEntry->mBinding = binding;
        if (binding->mGeneration == 0)
            binding->mGeneration = 1;   
            
        return NS_OK;
    }
    
    
    
    nsDiskCacheBinding * p  = hashEntry->mBinding;
    bool     calcGeneration = (binding->mGeneration == 0);  
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
    void           * key = (void *)(uintptr_t)binding->mRecord.HashNumber();

    hashEntry = (HashTableEntry*) PL_DHashTableSearch(&table,
                                                      (void*)(uintptr_t) key);
    if (!hashEntry) {
        NS_WARNING("### disk cache: binding not in hashtable!");
        return;
    }
    
    if (binding == hashEntry->mBinding) {
        if (PR_CLIST_IS_EMPTY(binding)) {
            
            PL_DHashTableRemove(&table,
                                (void*)(uintptr_t) binding->mRecord.HashNumber());
            return;
            
        } else {
            
            hashEntry->mBinding = (nsDiskCacheBinding *)PR_NEXT_LINK(binding);
        }
    }
    PR_REMOVE_AND_INIT_LINK(binding);
}




bool
nsDiskCacheBindery::ActiveBindings()
{
    NS_ASSERTION(initialized, "nsDiskCacheBindery not initialized");
    if (!initialized) return false;

    for (auto iter = table.Iter(); !iter.Done(); iter.Next()) {
        auto entry = static_cast<HashTableEntry*>(iter.Get());
        nsDiskCacheBinding* binding = entry->mBinding;
        nsDiskCacheBinding* head = binding;
        do {
            if (binding->IsActive()) {
                return true;
            }
            binding = (nsDiskCacheBinding *)PR_NEXT_LINK(binding);
        } while (binding != head);
    }

    return false;
}





size_t
nsDiskCacheBindery::SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf)
{
    NS_ASSERTION(initialized, "nsDiskCacheBindery not initialized");
    if (!initialized) return 0;

    size_t size = 0;

    for (auto iter = table.Iter(); !iter.Done(); iter.Next()) {
        auto entry = static_cast<HashTableEntry*>(iter.Get());
        nsDiskCacheBinding* binding = entry->mBinding;

        nsDiskCacheBinding* head = binding;
        do {
            size += aMallocSizeOf(binding);
            if (binding->mStreamIO) {
                size += binding->mStreamIO->SizeOfIncludingThis(aMallocSizeOf);
            }

            
            
            if (binding->mDeactivateEvent) {
                size += aMallocSizeOf(binding->mDeactivateEvent);
            }
            binding = (nsDiskCacheBinding *)PR_NEXT_LINK(binding);
        } while (binding != head);
    }

    return size;
}
