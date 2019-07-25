









































#include "nsMemoryCacheDevice.h"
#include "nsCacheService.h"
#include "nsICacheService.h"
#include "nsIStorageStream.h"
#include "nsICacheVisitor.h"
#include "nsCRT.h"
#include "nsCache.h"
#include "nsReadableUtils.h"











const char *gMemoryDeviceID      = "memory";

nsMemoryCacheDevice::nsMemoryCacheDevice()
    : mInitialized(PR_FALSE),
      mEvictionThreshold(PR_INT32_MAX),
      mHardLimit(4 * 1024 * 1024),       
      mSoftLimit((mHardLimit * 9) / 10), 
      mTotalSize(0),
      mInactiveSize(0),
      mEntryCount(0),
      mMaxEntryCount(0)
{
    for (int i=0; i<kQueueCount; ++i)
        PR_INIT_CLIST(&mEvictionList[i]);
}


nsMemoryCacheDevice::~nsMemoryCacheDevice()
{    
    Shutdown();
}


nsresult
nsMemoryCacheDevice::Init()
{
    if (mInitialized)  return NS_ERROR_ALREADY_INITIALIZED;

    nsresult  rv = mMemCacheEntries.Init();
    mInitialized = NS_SUCCEEDED(rv);
    return rv;
}


nsresult
nsMemoryCacheDevice::Shutdown()
{
    NS_ASSERTION(mInitialized, "### attempting shutdown while not initialized");
    NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);
    
    mMemCacheEntries.Shutdown();

    
    nsCacheEntry * entry, * next;

    for (int i = kQueueCount - 1; i >= 0; --i) {
        entry = (nsCacheEntry *)PR_LIST_HEAD(&mEvictionList[i]);
        while (entry != &mEvictionList[i]) {
            NS_ASSERTION(!entry->IsInUse(), "### shutting down with active entries");
            next = (nsCacheEntry *)PR_NEXT_LINK(entry);
            PR_REMOVE_AND_INIT_LINK(entry);
        
            
            PRInt32 memoryRecovered = (PRInt32)entry->Size();
            mTotalSize    -= memoryRecovered;
            mInactiveSize -= memoryRecovered;
            --mEntryCount;

            delete entry;
            entry = next;
        }
    }





    NS_ASSERTION(mInactiveSize == 0, "### mem cache leaking entries?");
    NS_ASSERTION(mEntryCount == 0, "### mem cache leaking entries?");
    
    mInitialized = PR_FALSE;

    return NS_OK;
}


const char *
nsMemoryCacheDevice::GetDeviceID()
{
    return gMemoryDeviceID;
}


nsCacheEntry *
nsMemoryCacheDevice::FindEntry(nsCString * key, PRBool *collision)
{
    nsCacheEntry * entry = mMemCacheEntries.GetEntry(key);
    if (!entry)  return nsnull;

    
    PR_REMOVE_AND_INIT_LINK(entry);
    PR_APPEND_LINK(entry, &mEvictionList[EvictionList(entry, 0)]);
    
    mInactiveSize -= entry->Size();

    return entry;
}


nsresult
nsMemoryCacheDevice::DeactivateEntry(nsCacheEntry * entry)
{
    CACHE_LOG_DEBUG(("nsMemoryCacheDevice::DeactivateEntry for entry 0x%p\n",
                     entry));
    if (entry->IsDoomed()) {
#ifdef DEBUG
        
#endif
        delete entry;
        CACHE_LOG_DEBUG(("deleted doomed entry 0x%p\n", entry));
        return NS_OK;
    }

#ifdef DEBUG
    nsCacheEntry * ourEntry = mMemCacheEntries.GetEntry(entry->Key());
    NS_ASSERTION(ourEntry, "DeactivateEntry called for an entry we don't have!");
    NS_ASSERTION(entry == ourEntry, "entry doesn't match ourEntry");
    if (ourEntry != entry)
        return NS_ERROR_INVALID_POINTER;
#endif

    mInactiveSize += entry->Size();
    EvictEntriesIfNecessary();

    return NS_OK;
}


nsresult
nsMemoryCacheDevice::BindEntry(nsCacheEntry * entry)
{
    if (!entry->IsDoomed()) {
        NS_ASSERTION(PR_CLIST_IS_EMPTY(entry),"entry is already on a list!");

        
        PR_APPEND_LINK(entry, &mEvictionList[EvictionList(entry, 0)]);

        
        nsresult  rv = mMemCacheEntries.AddEntry(entry);
        if (NS_FAILED(rv)) {
            PR_REMOVE_AND_INIT_LINK(entry);
            return rv;
        }

        
        ++mEntryCount;
        if (mMaxEntryCount < mEntryCount) mMaxEntryCount = mEntryCount;

        mTotalSize += entry->Size();
        EvictEntriesIfNecessary();
    }

    return NS_OK;
}


void
nsMemoryCacheDevice::DoomEntry(nsCacheEntry * entry)
{
#ifdef DEBUG
    
    nsCacheEntry * hashEntry = mMemCacheEntries.GetEntry(entry->Key());
    if (!hashEntry)               NS_WARNING("no entry for key");
    else if (entry != hashEntry)  NS_WARNING("entry != hashEntry");
#endif
    CACHE_LOG_DEBUG(("Dooming entry 0x%p in memory cache\n", entry));
    EvictEntry(entry, DO_NOT_DELETE_ENTRY);
}


nsresult
nsMemoryCacheDevice::OpenInputStreamForEntry( nsCacheEntry *    entry,
                                              nsCacheAccessMode mode,
                                              PRUint32          offset,
                                              nsIInputStream ** result)
{
    NS_ENSURE_ARG_POINTER(entry);
    NS_ENSURE_ARG_POINTER(result);

    nsCOMPtr<nsIStorageStream> storage;
    nsresult rv;

    nsISupports *data = entry->Data();
    if (data) {
        storage = do_QueryInterface(data, &rv);
        if (NS_FAILED(rv))
            return rv;
    }
    else {
        rv = NS_NewStorageStream(4096, PRUint32(-1), getter_AddRefs(storage));
        if (NS_FAILED(rv))
            return rv;
        entry->SetData(storage);
    }

    return storage->NewInputStream(offset, result);
}


nsresult
nsMemoryCacheDevice::OpenOutputStreamForEntry( nsCacheEntry *     entry,
                                               nsCacheAccessMode  mode,
                                               PRUint32           offset,
                                               nsIOutputStream ** result)
{
    NS_ENSURE_ARG_POINTER(entry);
    NS_ENSURE_ARG_POINTER(result);

    nsCOMPtr<nsIStorageStream> storage;
    nsresult rv;

    nsISupports *data = entry->Data();
    if (data) {
        storage = do_QueryInterface(data, &rv);
        if (NS_FAILED(rv))
            return rv;
    }
    else {
        rv = NS_NewStorageStream(4096, PRUint32(-1), getter_AddRefs(storage));
        if (NS_FAILED(rv))
            return rv;
        entry->SetData(storage);
    }

    return storage->GetOutputStream(offset, result);
}


nsresult
nsMemoryCacheDevice::GetFileForEntry( nsCacheEntry *    entry,
                                      nsIFile **        result )
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

bool
nsMemoryCacheDevice::EntryIsTooBig(PRInt64 entrySize)
{
    return entrySize > mSoftLimit;
}


nsresult
nsMemoryCacheDevice::OnDataSizeChange( nsCacheEntry * entry, PRInt32 deltaSize)
{
    if (entry->IsStreamData()) {
        
        PRUint32  newSize = entry->DataSize() + deltaSize;
        if (EntryIsTooBig(newSize)) {
#ifdef DEBUG
            nsresult rv =
#endif
                nsCacheService::DoomEntry(entry);
            NS_ASSERTION(NS_SUCCEEDED(rv),"DoomEntry() failed.");
            return NS_ERROR_ABORT;
        }
    }

    
    mTotalSize    += deltaSize;
    
    if (!entry->IsDoomed()) {
        
        PR_REMOVE_AND_INIT_LINK(entry);
        PR_APPEND_LINK(entry, &mEvictionList[EvictionList(entry, deltaSize)]);
    }

    EvictEntriesIfNecessary();
    return NS_OK;
}


void
nsMemoryCacheDevice::AdjustMemoryLimits(PRInt32  softLimit, PRInt32  hardLimit)
{
    mSoftLimit = softLimit;
    mHardLimit = hardLimit;

    
    EvictEntriesIfNecessary();
}


void
nsMemoryCacheDevice::EvictEntry(nsCacheEntry * entry, PRBool deleteEntry)
{
    CACHE_LOG_DEBUG(("Evicting entry 0x%p from memory cache, deleting: %d\n",
                     entry, deleteEntry));
    
    mMemCacheEntries.RemoveEntry(entry);
    
    
    PR_REMOVE_AND_INIT_LINK(entry);
    
    
    PRInt32 memoryRecovered = (PRInt32)entry->Size();
    mTotalSize    -= memoryRecovered;
    if (!entry->IsDoomed())
        mInactiveSize -= memoryRecovered;
    --mEntryCount;
    
    if (deleteEntry)  delete entry;
}


void
nsMemoryCacheDevice::EvictEntriesIfNecessary(void)
{
    nsCacheEntry * entry, * next;

    CACHE_LOG_DEBUG(("EvictEntriesIfNecessary.  mTotalSize: %d, mHardLimit: %d,"
                     "mInactiveSize: %d, mSoftLimit: %d\n",
                     mTotalSize, mHardLimit, mInactiveSize, mSoftLimit));
    
    if ((mTotalSize < mHardLimit) && (mInactiveSize < mSoftLimit))
        return;

    for (int i = kQueueCount - 1; i >= 0; --i) {
        entry = (nsCacheEntry *)PR_LIST_HEAD(&mEvictionList[i]);
        while (entry != &mEvictionList[i]) {
            if (entry->IsInUse()) {
                entry = (nsCacheEntry *)PR_NEXT_LINK(entry);
                continue;
            }

            next = (nsCacheEntry *)PR_NEXT_LINK(entry);
            EvictEntry(entry, DELETE_ENTRY);
            entry = next;

            if ((mTotalSize < mHardLimit) && (mInactiveSize < mSoftLimit))
                return;
        }
    }
}


int
nsMemoryCacheDevice::EvictionList(nsCacheEntry * entry, PRInt32  deltaSize)
{
    
    if (entry->ExpirationTime() == nsICache::NO_EXPIRATION_TIME)
        return 0;

    
    
    PRInt32  size       = deltaSize + (PRInt32)entry->Size();
    PRInt32  fetchCount = NS_MAX(1, entry->FetchCount());

    return NS_MIN(PR_FloorLog2(size / fetchCount), kQueueCount - 1);
}


nsresult
nsMemoryCacheDevice::Visit(nsICacheVisitor * visitor)
{
    nsMemoryCacheDeviceInfo * deviceInfo = new nsMemoryCacheDeviceInfo(this);
    nsCOMPtr<nsICacheDeviceInfo> deviceRef(deviceInfo);
    if (!deviceInfo) return NS_ERROR_OUT_OF_MEMORY;

    PRBool keepGoing;
    nsresult rv = visitor->VisitDevice(gMemoryDeviceID, deviceInfo, &keepGoing);
    if (NS_FAILED(rv)) return rv;

    if (!keepGoing)
        return NS_OK;

    nsCacheEntry *              entry;
    nsCOMPtr<nsICacheEntryInfo> entryRef;

    for (int i = kQueueCount - 1; i >= 0; --i) {
        entry = (nsCacheEntry *)PR_LIST_HEAD(&mEvictionList[i]);
        while (entry != &mEvictionList[i]) {
            nsCacheEntryInfo * entryInfo = new nsCacheEntryInfo(entry);
            if (!entryInfo) return NS_ERROR_OUT_OF_MEMORY;
            entryRef = entryInfo;

            rv = visitor->VisitEntry(gMemoryDeviceID, entryInfo, &keepGoing);
            entryInfo->DetachEntry();
            if (NS_FAILED(rv)) return rv;
            if (!keepGoing) break;

            entry = (nsCacheEntry *)PR_NEXT_LINK(entry);
        }
    }
    return NS_OK;
}


nsresult
nsMemoryCacheDevice::EvictEntries(const char * clientID)
{
    nsCacheEntry * entry;
    PRUint32 prefixLength = (clientID ? strlen(clientID) : 0);

    for (int i = kQueueCount - 1; i >= 0; --i) {
        PRCList * elem = PR_LIST_HEAD(&mEvictionList[i]);
        while (elem != &mEvictionList[i]) {
            entry = (nsCacheEntry *)elem;
            elem = PR_NEXT_LINK(elem);
            
            const char * key = entry->Key()->get();
            if (clientID && nsCRT::strncmp(clientID, key, prefixLength) != 0)
                continue;
            
            if (entry->IsInUse()) {
                nsresult rv = nsCacheService::DoomEntry(entry);
                if (NS_FAILED(rv)) {
                    CACHE_LOG_WARNING(("memCache->EvictEntries() aborted: rv =%x", rv));
                    return rv;
                }
            } else {
                EvictEntry(entry, DELETE_ENTRY);
            }
        }
    }

    return NS_OK;
}



void
nsMemoryCacheDevice::SetCapacity(PRInt32  capacity)
{
    PRInt32 hardLimit = capacity * 1024;  
    PRInt32 softLimit = (hardLimit * 9) / 10;
    AdjustMemoryLimits(softLimit, hardLimit);
}


#ifdef DEBUG
static PLDHashOperator
CountEntry(PLDHashTable * table, PLDHashEntryHdr * hdr, PRUint32 number, void * arg)
{
    PRInt32 *entryCount = (PRInt32 *)arg;
    ++(*entryCount);
    return PL_DHASH_NEXT;
}

void
nsMemoryCacheDevice::CheckEntryCount()
{
    if (!mInitialized)  return;

    PRInt32 evictionListCount = 0;
    for (int i=0; i<kQueueCount; ++i) {
        PRCList * elem = PR_LIST_HEAD(&mEvictionList[i]);
        while (elem != &mEvictionList[i]) {
            elem = PR_NEXT_LINK(elem);
            ++evictionListCount;
        }
    }
    NS_ASSERTION(mEntryCount == evictionListCount, "### mem cache badness");

    PRInt32 entryCount = 0;
    mMemCacheEntries.VisitEntries(CountEntry, &entryCount);
    NS_ASSERTION(mEntryCount == entryCount, "### mem cache badness");    
}
#endif






NS_IMPL_ISUPPORTS1(nsMemoryCacheDeviceInfo, nsICacheDeviceInfo)


NS_IMETHODIMP
nsMemoryCacheDeviceInfo::GetDescription(char ** result)
{
    NS_ENSURE_ARG_POINTER(result);
    *result = NS_strdup("Memory cache device");
    if (!*result) return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}


NS_IMETHODIMP
nsMemoryCacheDeviceInfo::GetUsageReport(char ** result)
{
    NS_ENSURE_ARG_POINTER(result);
    nsCString  buffer;

    buffer.AssignLiteral("  <tr>\n"
                         "    <th>Inactive storage:</th>\n"
                         "    <td>");
    buffer.AppendInt(mDevice->mInactiveSize / 1024);
    buffer.AppendLiteral(" KiB</td>\n"
                         "  </tr>\n");

    *result = ToNewCString(buffer);
    if (!*result) return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}


NS_IMETHODIMP
nsMemoryCacheDeviceInfo::GetEntryCount(PRUint32 * result)
{
    NS_ENSURE_ARG_POINTER(result);
    
    *result = (PRUint32)mDevice->mEntryCount;
    return NS_OK;
}


NS_IMETHODIMP
nsMemoryCacheDeviceInfo::GetTotalSize(PRUint32 * result)
{
    NS_ENSURE_ARG_POINTER(result);
    *result = (PRUint32)mDevice->mTotalSize;
    return NS_OK;
}


NS_IMETHODIMP
nsMemoryCacheDeviceInfo::GetMaximumSize(PRUint32 * result)
{
    NS_ENSURE_ARG_POINTER(result);
    *result = (PRUint32)mDevice->mHardLimit;
    return NS_OK;
}
