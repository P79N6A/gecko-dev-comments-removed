








































#include "nsDiskCache.h"
#include "nsDiskCacheEntry.h"
#include "nsDiskCacheBinding.h"
#include "nsCRT.h"

#include "nsCache.h"











nsCacheEntry *
nsDiskCacheEntry::CreateCacheEntry(nsCacheDevice *  device)
{
    nsCacheEntry * entry = nsnull;
    nsresult       rv = nsCacheEntry::Create(Key(),
                                             nsICache::STREAM_BASED,
                                             nsICache::STORE_ON_DISK,
                                             device,
                                             &entry);
    if (NS_FAILED(rv) || !entry) return nsnull;
    
    entry->SetFetchCount(mFetchCount);
    entry->SetLastFetched(mLastFetched);
    entry->SetLastModified(mLastModified);
    entry->SetExpirationTime(mExpirationTime);
    entry->SetCacheDevice(device);
    
    entry->SetDataSize(mDataSize);
    
    rv = entry->UnflattenMetaData(MetaData(), mMetaDataSize);
    if (NS_FAILED(rv)) {
        delete entry;
        return nsnull;
    }
    
    return entry;                      
}






nsDiskCacheEntry *
CreateDiskCacheEntry(nsDiskCacheBinding *  binding,
                     PRUint32 * aSize)
{
    nsCacheEntry * entry = binding->mCacheEntry;
    if (!entry)  return nsnull;
    
    PRUint32  keySize  = entry->Key()->Length() + 1;
    PRUint32  metaSize = entry->MetaDataSize();
    PRUint32  size     = sizeof(nsDiskCacheEntry) + keySize + metaSize;
    
    if (aSize) *aSize = size;
    
    nsDiskCacheEntry * diskEntry = (nsDiskCacheEntry *)new char[size];
    if (!diskEntry)  return nsnull;
    
    diskEntry->mHeaderVersion   = nsDiskCache::kCurrentVersion;
    diskEntry->mMetaLocation    = binding->mRecord.MetaLocation();
    diskEntry->mFetchCount      = entry->FetchCount();
    diskEntry->mLastFetched     = entry->LastFetched();
    diskEntry->mLastModified    = entry->LastModified();
    diskEntry->mExpirationTime  = entry->ExpirationTime();
    diskEntry->mDataSize        = entry->DataSize();
    diskEntry->mKeySize         = keySize;
    diskEntry->mMetaDataSize    = metaSize;
    
    memcpy(diskEntry->Key(), entry->Key()->get(),keySize);
    
    nsresult rv = entry->FlattenMetaData(diskEntry->MetaData(), metaSize);
    if (NS_FAILED(rv)) {
        delete [] (char *)diskEntry;
        return nsnull;
    }
        
    return  diskEntry;
}






NS_IMPL_ISUPPORTS1(nsDiskCacheEntryInfo, nsICacheEntryInfo)

NS_IMETHODIMP nsDiskCacheEntryInfo::GetClientID(char ** clientID)
{
    NS_ENSURE_ARG_POINTER(clientID);
    return ClientIDFromCacheKey(nsDependentCString(mDiskEntry->Key()), clientID);
}

extern const char DISK_CACHE_DEVICE_ID[];
NS_IMETHODIMP nsDiskCacheEntryInfo::GetDeviceID(char ** deviceID)
{
    NS_ENSURE_ARG_POINTER(deviceID);
    *deviceID = nsCRT::strdup(mDeviceID);
    return *deviceID ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP nsDiskCacheEntryInfo::GetKey(nsACString &clientKey)
{
    return ClientKeyFromCacheKey(nsDependentCString(mDiskEntry->Key()), clientKey);
}

NS_IMETHODIMP nsDiskCacheEntryInfo::GetFetchCount(PRInt32 *aFetchCount)
{
    NS_ENSURE_ARG_POINTER(aFetchCount);
    *aFetchCount = mDiskEntry->mFetchCount;
    return NS_OK;
}

NS_IMETHODIMP nsDiskCacheEntryInfo::GetLastFetched(PRUint32 *aLastFetched)
{
    NS_ENSURE_ARG_POINTER(aLastFetched);
    *aLastFetched = mDiskEntry->mLastFetched;
    return NS_OK;
}

NS_IMETHODIMP nsDiskCacheEntryInfo::GetLastModified(PRUint32 *aLastModified)
{
    NS_ENSURE_ARG_POINTER(aLastModified);
    *aLastModified = mDiskEntry->mLastModified;
    return NS_OK;
}

NS_IMETHODIMP nsDiskCacheEntryInfo::GetExpirationTime(PRUint32 *aExpirationTime)
{
    NS_ENSURE_ARG_POINTER(aExpirationTime);
    *aExpirationTime = mDiskEntry->mExpirationTime;
    return NS_OK;
}

NS_IMETHODIMP nsDiskCacheEntryInfo::IsStreamBased(PRBool *aStreamBased)
{
    NS_ENSURE_ARG_POINTER(aStreamBased);
    *aStreamBased = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP nsDiskCacheEntryInfo::GetDataSize(PRUint32 *aDataSize)
{
    NS_ENSURE_ARG_POINTER(aDataSize);
    *aDataSize = mDiskEntry->mDataSize;
    return NS_OK;
}
