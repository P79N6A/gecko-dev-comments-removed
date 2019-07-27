





#include "nsICache.h"
#include "nsCache.h"
#include "nsCacheService.h"
#include "nsCacheEntryDescriptor.h"
#include "nsCacheEntry.h"
#include "nsReadableUtils.h"
#include "nsIOutputStream.h"
#include "nsCRT.h"
#include "nsThreadUtils.h"
#include <algorithm>

#define kMinDecompressReadBufLen 1024
#define kMinCompressWriteBufLen  1024






class nsAsyncDoomEvent : public nsRunnable {
public:
    nsAsyncDoomEvent(nsCacheEntryDescriptor *descriptor,
                     nsICacheListener *listener)
    {
        mDescriptor = descriptor;
        mListener = listener;
        mThread = do_GetCurrentThread();
        
        
        
        
        NS_IF_ADDREF(mListener);
    }

    NS_IMETHOD Run()
    {
        nsresult status = NS_OK;

        {
            nsCacheServiceAutoLock lock(LOCK_TELEM(NSASYNCDOOMEVENT_RUN));

            if (mDescriptor->mCacheEntry) {
                status = nsCacheService::gService->DoomEntry_Internal(
                             mDescriptor->mCacheEntry, true);
            } else if (!mDescriptor->mDoomedOnClose) {
                status = NS_ERROR_NOT_AVAILABLE;
            }
        }

        if (mListener) {
            mThread->Dispatch(new nsNotifyDoomListener(mListener, status),
                              NS_DISPATCH_NORMAL);
            
            mListener = nullptr;
        }

        return NS_OK;
    }

private:
    nsRefPtr<nsCacheEntryDescriptor> mDescriptor;
    nsICacheListener                *mListener;
    nsCOMPtr<nsIThread>              mThread;
};


NS_IMPL_ISUPPORTS(nsCacheEntryDescriptor,
                  nsICacheEntryDescriptor,
                  nsICacheEntryInfo)

nsCacheEntryDescriptor::nsCacheEntryDescriptor(nsCacheEntry * entry,
                                               nsCacheAccessMode accessGranted)
    : mCacheEntry(entry),
      mAccessGranted(accessGranted),
      mOutputWrapper(nullptr),
      mLock("nsCacheEntryDescriptor.mLock"),
      mAsyncDoomPending(false),
      mDoomedOnClose(false),
      mClosingDescriptor(false)
{
    PR_INIT_CLIST(this);
    NS_ADDREF(nsCacheService::GlobalInstance());  
}


nsCacheEntryDescriptor::~nsCacheEntryDescriptor()
{
    
    
    
    
    
    if (mCacheEntry)
        Close();

    NS_ASSERTION(mInputWrappers.IsEmpty(),
                 "We have still some input wrapper!");
    NS_ASSERTION(!mOutputWrapper, "We have still an output wrapper!");

    nsCacheService * service = nsCacheService::GlobalInstance();
    NS_RELEASE(service);
}


NS_IMETHODIMP
nsCacheEntryDescriptor::GetClientID(char ** result)
{
    NS_ENSURE_ARG_POINTER(result);

    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_GETCLIENTID));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    return ClientIDFromCacheKey(*(mCacheEntry->Key()), result);
}


NS_IMETHODIMP
nsCacheEntryDescriptor::GetDeviceID(char ** aDeviceID)
{
    NS_ENSURE_ARG_POINTER(aDeviceID);
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_GETDEVICEID));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    const char* deviceID = mCacheEntry->GetDeviceID();
    if (!deviceID) {
        *aDeviceID = nullptr;
        return NS_OK;
    }

    *aDeviceID = NS_strdup(deviceID);
    return *aDeviceID ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::GetKey(nsACString &result)
{
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_GETKEY));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    return ClientKeyFromCacheKey(*(mCacheEntry->Key()), result);
}


NS_IMETHODIMP
nsCacheEntryDescriptor::GetFetchCount(int32_t *result)
{
    NS_ENSURE_ARG_POINTER(result);
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_GETFETCHCOUNT));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    *result = mCacheEntry->FetchCount();
    return NS_OK;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::GetLastFetched(uint32_t *result)
{
    NS_ENSURE_ARG_POINTER(result);
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_GETLASTFETCHED));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    *result = mCacheEntry->LastFetched();
    return NS_OK;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::GetLastModified(uint32_t *result)
{
    NS_ENSURE_ARG_POINTER(result);
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_GETLASTMODIFIED));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    *result = mCacheEntry->LastModified();
    return NS_OK;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::GetExpirationTime(uint32_t *result)
{
    NS_ENSURE_ARG_POINTER(result);
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_GETEXPIRATIONTIME));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    *result = mCacheEntry->ExpirationTime();
    return NS_OK;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::SetExpirationTime(uint32_t expirationTime)
{
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_SETEXPIRATIONTIME));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    mCacheEntry->SetExpirationTime(expirationTime);
    mCacheEntry->MarkEntryDirty();
    return NS_OK;
}


NS_IMETHODIMP nsCacheEntryDescriptor::IsStreamBased(bool *result)
{
    NS_ENSURE_ARG_POINTER(result);
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_ISSTREAMBASED));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    *result = mCacheEntry->IsStreamData();
    return NS_OK;
}

NS_IMETHODIMP nsCacheEntryDescriptor::GetPredictedDataSize(int64_t *result)
{
    NS_ENSURE_ARG_POINTER(result);
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_GETPREDICTEDDATASIZE));
    if (!mCacheEntry) return NS_ERROR_NOT_AVAILABLE;

    *result = mCacheEntry->PredictedDataSize();
    return NS_OK;
}

NS_IMETHODIMP nsCacheEntryDescriptor::SetPredictedDataSize(int64_t
                                                           predictedSize)
{
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_SETPREDICTEDDATASIZE));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    mCacheEntry->SetPredictedDataSize(predictedSize);
    return NS_OK;
}

NS_IMETHODIMP nsCacheEntryDescriptor::GetDataSize(uint32_t *result)
{
    NS_ENSURE_ARG_POINTER(result);
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_GETDATASIZE));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    const char* val = mCacheEntry->GetMetaDataElement("uncompressed-len");
    if (!val) {
        *result = mCacheEntry->DataSize();
    } else {
        *result = atol(val);
    }

    return NS_OK;
}


NS_IMETHODIMP nsCacheEntryDescriptor::GetStorageDataSize(uint32_t *result)
{
    NS_ENSURE_ARG_POINTER(result);
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_GETSTORAGEDATASIZE));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    *result = mCacheEntry->DataSize();

    return NS_OK;
}


nsresult
nsCacheEntryDescriptor::RequestDataSizeChange(int32_t deltaSize)
{
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_REQUESTDATASIZECHANGE));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    nsresult  rv;
    rv = nsCacheService::OnDataSizeChange(mCacheEntry, deltaSize);
    if (NS_SUCCEEDED(rv)) {
        
        uint32_t  newDataSize = mCacheEntry->DataSize() + deltaSize;
        mCacheEntry->SetDataSize(newDataSize);
        mCacheEntry->TouchData();
    }
    return rv;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::SetDataSize(uint32_t dataSize)
{
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_SETDATASIZE));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    
    int32_t  deltaSize = dataSize - mCacheEntry->DataSize();

    nsresult  rv;
    rv = nsCacheService::OnDataSizeChange(mCacheEntry, deltaSize);
    
    if (NS_SUCCEEDED(rv)) {
        
        uint32_t  newDataSize = mCacheEntry->DataSize() + deltaSize;
        mCacheEntry->SetDataSize(newDataSize);
        mCacheEntry->TouchData();
    } else {
        NS_WARNING("failed SetDataSize() on memory cache object!");
    }
    
    return rv;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::OpenInputStream(uint32_t offset, nsIInputStream ** result)
{
    NS_ENSURE_ARG_POINTER(result);

    nsInputStreamWrapper* cacheInput = nullptr;
    {
        nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_OPENINPUTSTREAM));
        if (!mCacheEntry)                  return NS_ERROR_NOT_AVAILABLE;
        if (!mCacheEntry->IsStreamData())  return NS_ERROR_CACHE_DATA_IS_NOT_STREAM;

        
        if (mClosingDescriptor || nsCacheService::GetClearingEntries())
            return NS_ERROR_NOT_AVAILABLE;

        
        if (!(mAccessGranted & nsICache::ACCESS_READ))
            return NS_ERROR_CACHE_READ_ACCESS_DENIED;

        const char *val;
        val = mCacheEntry->GetMetaDataElement("uncompressed-len");
        if (val) {
            cacheInput = new nsDecompressInputStreamWrapper(this, offset);
        } else {
            cacheInput = new nsInputStreamWrapper(this, offset);
        }
        if (!cacheInput) return NS_ERROR_OUT_OF_MEMORY;

        mInputWrappers.AppendElement(cacheInput);
    }

    NS_ADDREF(*result = cacheInput);
    return NS_OK;
}

NS_IMETHODIMP
nsCacheEntryDescriptor::OpenOutputStream(uint32_t offset, nsIOutputStream ** result)
{
    NS_ENSURE_ARG_POINTER(result);

    nsOutputStreamWrapper* cacheOutput = nullptr;
    {
        nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_OPENOUTPUTSTREAM));
        if (!mCacheEntry)                  return NS_ERROR_NOT_AVAILABLE;
        if (!mCacheEntry->IsStreamData())  return NS_ERROR_CACHE_DATA_IS_NOT_STREAM;

        
        if (mClosingDescriptor || nsCacheService::GetClearingEntries())
            return NS_ERROR_NOT_AVAILABLE;

        
        if (!(mAccessGranted & nsICache::ACCESS_WRITE))
            return NS_ERROR_CACHE_WRITE_ACCESS_DENIED;

        int32_t compressionLevel = nsCacheService::CacheCompressionLevel();
        const char *val;
        val = mCacheEntry->GetMetaDataElement("uncompressed-len");
        if ((compressionLevel > 0) && val) {
            cacheOutput = new nsCompressOutputStreamWrapper(this, offset);
        } else {
            
            if (val) {
                mCacheEntry->SetMetaDataElement("uncompressed-len", nullptr);
            }
            cacheOutput = new nsOutputStreamWrapper(this, offset);
        }
        if (!cacheOutput) return NS_ERROR_OUT_OF_MEMORY;

        mOutputWrapper = cacheOutput;
    }

    NS_ADDREF(*result = cacheOutput);
    return NS_OK;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::GetCacheElement(nsISupports ** result)
{
    NS_ENSURE_ARG_POINTER(result);
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_GETCACHEELEMENT));
    if (!mCacheEntry)                 return NS_ERROR_NOT_AVAILABLE;
    if (mCacheEntry->IsStreamData())  return NS_ERROR_CACHE_DATA_IS_STREAM;

    NS_IF_ADDREF(*result = mCacheEntry->Data());
    return NS_OK;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::SetCacheElement(nsISupports * cacheElement)
{
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_SETCACHEELEMENT));
    if (!mCacheEntry)                 return NS_ERROR_NOT_AVAILABLE;
    if (mCacheEntry->IsStreamData())  return NS_ERROR_CACHE_DATA_IS_STREAM;

    return nsCacheService::SetCacheElement(mCacheEntry, cacheElement);
}


NS_IMETHODIMP
nsCacheEntryDescriptor::GetAccessGranted(nsCacheAccessMode *result)
{
    NS_ENSURE_ARG_POINTER(result);
    *result = mAccessGranted;
    return NS_OK;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::GetStoragePolicy(nsCacheStoragePolicy *result)
{
    NS_ENSURE_ARG_POINTER(result);
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_GETSTORAGEPOLICY));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;
    
    *result = mCacheEntry->StoragePolicy();
    return NS_OK;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::SetStoragePolicy(nsCacheStoragePolicy policy)
{
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_SETSTORAGEPOLICY));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;
    
    
    bool        storageEnabled = false;
    storageEnabled = nsCacheService::IsStorageEnabledForPolicy_Locked(policy);
    if (!storageEnabled)    return NS_ERROR_FAILURE;

    
    if (!(mAccessGranted & nsICache::ACCESS_WRITE))
        return NS_ERROR_NOT_AVAILABLE;
    
    
    if (mCacheEntry->StoragePolicy() == nsICache::STORE_IN_MEMORY &&
        policy != nsICache::STORE_IN_MEMORY)
        return NS_ERROR_NOT_AVAILABLE;
        
    mCacheEntry->SetStoragePolicy(policy);
    mCacheEntry->MarkEntryDirty();
    return NS_OK;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::GetFile(nsIFile ** result)
{
    NS_ENSURE_ARG_POINTER(result);
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_GETFILE));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    return nsCacheService::GetFileForEntry(mCacheEntry, result);
}


NS_IMETHODIMP
nsCacheEntryDescriptor::GetSecurityInfo(nsISupports ** result)
{
    NS_ENSURE_ARG_POINTER(result);
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_GETSECURITYINFO));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    *result = mCacheEntry->SecurityInfo();
    NS_IF_ADDREF(*result);
    return NS_OK;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::SetSecurityInfo(nsISupports * securityInfo)
{
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_SETSECURITYINFO));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    mCacheEntry->SetSecurityInfo(securityInfo);
    mCacheEntry->MarkEntryDirty();
    return NS_OK;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::Doom()
{
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_DOOM));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    return nsCacheService::DoomEntry(mCacheEntry);
}


NS_IMETHODIMP
nsCacheEntryDescriptor::DoomAndFailPendingRequests(nsresult status)
{
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_DOOMANDFAILPENDINGREQUESTS));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::AsyncDoom(nsICacheListener *listener)
{
    bool asyncDoomPending;
    {
        mozilla::MutexAutoLock lock(mLock);
        asyncDoomPending = mAsyncDoomPending;
        mAsyncDoomPending = true;
    }

    if (asyncDoomPending) {
        
        
        if (listener) {
            nsresult rv = NS_DispatchToCurrentThread(
                new nsNotifyDoomListener(listener, NS_ERROR_NOT_AVAILABLE));
            if (NS_SUCCEEDED(rv))
                NS_IF_ADDREF(listener);
            return rv;
        }
        return NS_OK;
    }

    nsRefPtr<nsIRunnable> event = new nsAsyncDoomEvent(this, listener);
    return nsCacheService::DispatchToCacheIOThread(event);
}


NS_IMETHODIMP
nsCacheEntryDescriptor::MarkValid()
{
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_MARKVALID));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    nsresult  rv = nsCacheService::ValidateEntry(mCacheEntry);
    return rv;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::Close()
{
    nsRefPtr<nsOutputStreamWrapper> outputWrapper;
    nsTArray<nsRefPtr<nsInputStreamWrapper> > inputWrappers;

    {
        nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_CLOSE));
        if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

        
        mClosingDescriptor = true;
        outputWrapper = mOutputWrapper;
        for (size_t i = 0; i < mInputWrappers.Length(); i++)
            inputWrappers.AppendElement(mInputWrappers[i]);
    }

    
    
    
    if (outputWrapper) {
        if (NS_FAILED(outputWrapper->Close())) {
            NS_WARNING("Dooming entry because Close() failed!!!");
            Doom();
        }
        outputWrapper = nullptr;
    }

    for (uint32_t i = 0 ; i < inputWrappers.Length() ; i++)
        inputWrappers[i]->Close();

    inputWrappers.Clear();

    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_CLOSE));
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    

    
    nsCacheService::CloseDescriptor(this);
    NS_ASSERTION(mCacheEntry == nullptr, "mCacheEntry not null");

    return NS_OK;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::GetMetaDataElement(const char *key, char **result)
{
    NS_ENSURE_ARG_POINTER(key);
    *result = nullptr;

    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_GETMETADATAELEMENT));
    NS_ENSURE_TRUE(mCacheEntry, NS_ERROR_NOT_AVAILABLE);

    const char *value;

    value = mCacheEntry->GetMetaDataElement(key);
    if (!value) return NS_ERROR_NOT_AVAILABLE;

    *result = NS_strdup(value);
    if (!*result) return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::SetMetaDataElement(const char *key, const char *value)
{
    NS_ENSURE_ARG_POINTER(key);

    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_SETMETADATAELEMENT));
    NS_ENSURE_TRUE(mCacheEntry, NS_ERROR_NOT_AVAILABLE);

    

    nsresult rv = mCacheEntry->SetMetaDataElement(key, value);
    if (NS_SUCCEEDED(rv))
        mCacheEntry->TouchMetaData();
    return rv;
}


NS_IMETHODIMP
nsCacheEntryDescriptor::VisitMetaData(nsICacheMetaDataVisitor * visitor)
{
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHEENTRYDESCRIPTOR_VISITMETADATA)); 
    
    NS_ENSURE_ARG_POINTER(visitor);
    if (!mCacheEntry)  return NS_ERROR_NOT_AVAILABLE;

    return mCacheEntry->VisitMetaDataElements(visitor);
}







NS_IMPL_ADDREF(nsCacheEntryDescriptor::nsInputStreamWrapper)
NS_IMETHODIMP_(MozExternalRefCountType)
nsCacheEntryDescriptor::nsInputStreamWrapper::Release()
{
    
    
    nsRefPtr<nsCacheEntryDescriptor> desc;

    {
        mozilla::MutexAutoLock lock(mLock);
        desc = mDescriptor;
    }

    if (desc)
        nsCacheService::Lock(LOCK_TELEM(NSINPUTSTREAMWRAPPER_RELEASE));

    nsrefcnt count;
    NS_PRECONDITION(0 != mRefCnt, "dup release");
    count = --mRefCnt;
    NS_LOG_RELEASE(this, count, "nsCacheEntryDescriptor::nsInputStreamWrapper");

    if (0 == count) {
        
        if (mDescriptor) {
            NS_ASSERTION(mDescriptor->mInputWrappers.Contains(this),
                         "Wrapper not found in array!");
            mDescriptor->mInputWrappers.RemoveElement(this);
        }

        if (desc)
            nsCacheService::Unlock();

        mRefCnt = 1;
        delete (this);
        return 0;
    }

    if (desc)
        nsCacheService::Unlock();

    return count;
}

NS_INTERFACE_MAP_BEGIN(nsCacheEntryDescriptor::nsInputStreamWrapper)
  NS_INTERFACE_MAP_ENTRY(nsIInputStream)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END_THREADSAFE

nsresult nsCacheEntryDescriptor::
nsInputStreamWrapper::LazyInit()
{
    
    
    if (!mDescriptor)
        return NS_ERROR_NOT_AVAILABLE;

    nsCacheServiceAutoLock lock(LOCK_TELEM(NSINPUTSTREAMWRAPPER_LAZYINIT));

    nsCacheAccessMode mode;
    nsresult rv = mDescriptor->GetAccessGranted(&mode);
    if (NS_FAILED(rv)) return rv;

    NS_ENSURE_TRUE(mode & nsICache::ACCESS_READ, NS_ERROR_UNEXPECTED);

    nsCacheEntry* cacheEntry = mDescriptor->CacheEntry();
    if (!cacheEntry) return NS_ERROR_NOT_AVAILABLE;

    rv = nsCacheService::OpenInputStreamForEntry(cacheEntry, mode,
                                                 mStartOffset,
                                                 getter_AddRefs(mInput));

    CACHE_LOG_DEBUG(("nsInputStreamWrapper::LazyInit "
                      "[entry=%p, wrapper=%p, mInput=%p, rv=%d]",
                      mDescriptor, this, mInput.get(), int(rv)));

    if (NS_FAILED(rv)) return rv;

    mInitialized = true;
    return NS_OK;
}

nsresult nsCacheEntryDescriptor::
nsInputStreamWrapper::EnsureInit()
{
    if (mInitialized) {
        NS_ASSERTION(mDescriptor, "Bad state");
        return NS_OK;
    }

    return LazyInit();
}

void nsCacheEntryDescriptor::
nsInputStreamWrapper::CloseInternal()
{
    mLock.AssertCurrentThreadOwns();
    if (!mDescriptor) {
        NS_ASSERTION(!mInitialized, "Bad state");
        NS_ASSERTION(!mInput, "Bad state");
        return;
    }

    nsCacheServiceAutoLock lock(LOCK_TELEM(NSINPUTSTREAMWRAPPER_CLOSEINTERNAL));

    if (mDescriptor) {
        mDescriptor->mInputWrappers.RemoveElement(this);
        nsCacheService::ReleaseObject_Locked(mDescriptor);
        mDescriptor = nullptr;
    }
    mInitialized = false;
    mInput = nullptr;
}

nsresult nsCacheEntryDescriptor::
nsInputStreamWrapper::Close()
{
    mozilla::MutexAutoLock lock(mLock);

    return Close_Locked();
}

nsresult nsCacheEntryDescriptor::
nsInputStreamWrapper::Close_Locked()
{
    nsresult rv = EnsureInit();
    if (NS_SUCCEEDED(rv)) {
        rv = mInput->Close();
    } else {
        NS_ASSERTION(!mInput,
                     "Shouldn't have mInput when EnsureInit() failed");
    }

    
    
    CloseInternal();
    return rv;
}

nsresult nsCacheEntryDescriptor::
nsInputStreamWrapper::Available(uint64_t *avail)
{
    mozilla::MutexAutoLock lock(mLock);

    nsresult rv = EnsureInit();
    if (NS_FAILED(rv)) return rv;

    return mInput->Available(avail);
}

nsresult nsCacheEntryDescriptor::
nsInputStreamWrapper::Read(char *buf, uint32_t count, uint32_t *countRead)
{
    mozilla::MutexAutoLock lock(mLock);

    return Read_Locked(buf, count, countRead);
}

nsresult nsCacheEntryDescriptor::
nsInputStreamWrapper::Read_Locked(char *buf, uint32_t count, uint32_t *countRead)
{
    nsresult rv = EnsureInit();
    if (NS_SUCCEEDED(rv))
        rv = mInput->Read(buf, count, countRead);

    CACHE_LOG_DEBUG(("nsInputStreamWrapper::Read "
                      "[entry=%p, wrapper=%p, mInput=%p, rv=%d]",
                      mDescriptor, this, mInput.get(), rv));

    return rv;
}

nsresult nsCacheEntryDescriptor::
nsInputStreamWrapper::ReadSegments(nsWriteSegmentFun writer, void *closure,
                                   uint32_t count, uint32_t *countRead)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult nsCacheEntryDescriptor::
nsInputStreamWrapper::IsNonBlocking(bool *result)
{
    
    *result = false;
    return NS_OK;
}






NS_IMPL_ADDREF(nsCacheEntryDescriptor::nsDecompressInputStreamWrapper)
NS_IMETHODIMP_(MozExternalRefCountType)
nsCacheEntryDescriptor::nsDecompressInputStreamWrapper::Release()
{
    
    
    nsRefPtr<nsCacheEntryDescriptor> desc;

    {
        mozilla::MutexAutoLock lock(mLock);
        desc = mDescriptor;
    }

    if (desc)
        nsCacheService::Lock(LOCK_TELEM(
                             NSDECOMPRESSINPUTSTREAMWRAPPER_RELEASE));

    nsrefcnt count;
    NS_PRECONDITION(0 != mRefCnt, "dup release");
    count = --mRefCnt;
    NS_LOG_RELEASE(this, count,
                   "nsCacheEntryDescriptor::nsDecompressInputStreamWrapper");

    if (0 == count) {
        
        if (mDescriptor) {
            NS_ASSERTION(mDescriptor->mInputWrappers.Contains(this),
                         "Wrapper not found in array!");
            mDescriptor->mInputWrappers.RemoveElement(this);
        }

        if (desc)
            nsCacheService::Unlock();

        mRefCnt = 1;
        delete (this);
        return 0;
    }

    if (desc)
        nsCacheService::Unlock();

    return count;
}

NS_INTERFACE_MAP_BEGIN(nsCacheEntryDescriptor::nsDecompressInputStreamWrapper)
  NS_INTERFACE_MAP_ENTRY(nsIInputStream)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMETHODIMP nsCacheEntryDescriptor::
nsDecompressInputStreamWrapper::Read(char *    buf, 
                                     uint32_t  count, 
                                     uint32_t *countRead)
{
    mozilla::MutexAutoLock lock(mLock);

    int zerr = Z_OK;
    nsresult rv = NS_OK;

    if (!mStreamInitialized) {
        rv = InitZstream();
        if (NS_FAILED(rv)) {
            return rv;
        }
    }

    mZstream.next_out = (Bytef*)buf;
    mZstream.avail_out = count;

    if (mReadBufferLen < count) {
        
        
        
        
        
        uint32_t newBufLen = std::max(count, (uint32_t)kMinDecompressReadBufLen);
        unsigned char* newBuf;
        newBuf = (unsigned char*)nsMemory::Realloc(mReadBuffer, 
            newBufLen);
        if (newBuf) {
            mReadBuffer = newBuf;
            mReadBufferLen = newBufLen;
        }
        if (!mReadBuffer) {
            mReadBufferLen = 0;
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }

    
    
    while (NS_SUCCEEDED(rv) &&
           zerr == Z_OK && 
           mZstream.avail_out > 0 &&
           count > 0) {
        if (mZstream.avail_in == 0) {
            rv = nsInputStreamWrapper::Read_Locked((char*)mReadBuffer,
                                                   mReadBufferLen,
                                                   &mZstream.avail_in);
            if (NS_FAILED(rv) || !mZstream.avail_in) {
                break;
            }
            mZstream.next_in = mReadBuffer;
        }
        zerr = inflate(&mZstream, Z_NO_FLUSH);
        if (zerr == Z_STREAM_END) {
            
            
            
            
            Bytef * saveNextIn = mZstream.next_in;
            unsigned int saveAvailIn = mZstream.avail_in;
            Bytef * saveNextOut = mZstream.next_out;
            unsigned int saveAvailOut = mZstream.avail_out;
            inflateReset(&mZstream);
            mZstream.next_in = saveNextIn;
            mZstream.avail_in = saveAvailIn;
            mZstream.next_out = saveNextOut;
            mZstream.avail_out = saveAvailOut;
            zerr = Z_OK;
        } else if (zerr != Z_OK) {
            rv = NS_ERROR_INVALID_CONTENT_ENCODING;
        }
    }
    if (NS_SUCCEEDED(rv)) {
        *countRead = count - mZstream.avail_out;
    }
    return rv;
}

nsresult nsCacheEntryDescriptor::
nsDecompressInputStreamWrapper::Close()
{
    mozilla::MutexAutoLock lock(mLock);

    if (!mDescriptor)
        return NS_ERROR_NOT_AVAILABLE;

    EndZstream();
    if (mReadBuffer) {
        nsMemory::Free(mReadBuffer);
        mReadBuffer = 0;
        mReadBufferLen = 0;
    }
    return nsInputStreamWrapper::Close_Locked();
}

nsresult nsCacheEntryDescriptor::
nsDecompressInputStreamWrapper::InitZstream()
{
    if (!mDescriptor)
        return NS_ERROR_NOT_AVAILABLE;

    if (mStreamEnded)
        return NS_ERROR_FAILURE;

    
    mZstream.zalloc = Z_NULL;
    mZstream.zfree = Z_NULL;
    mZstream.opaque = Z_NULL;
    mZstream.next_out = Z_NULL;
    mZstream.avail_out = 0;
    mZstream.next_in = Z_NULL;
    mZstream.avail_in = 0;
    if (inflateInit(&mZstream) != Z_OK) {
        return NS_ERROR_FAILURE;
    }
    mStreamInitialized = true;
    return NS_OK;
}

nsresult nsCacheEntryDescriptor::
nsDecompressInputStreamWrapper::EndZstream()
{
    if (mStreamInitialized && !mStreamEnded) {
        inflateEnd(&mZstream);
        mStreamInitialized = false;
        mStreamEnded = true;
    }
    return NS_OK;
}








NS_IMPL_ADDREF(nsCacheEntryDescriptor::nsOutputStreamWrapper)
NS_IMETHODIMP_(MozExternalRefCountType)
nsCacheEntryDescriptor::nsOutputStreamWrapper::Release()
{
    
    
    nsRefPtr<nsCacheEntryDescriptor> desc;

    {
        mozilla::MutexAutoLock lock(mLock);
        desc = mDescriptor;
    }

    if (desc)
        nsCacheService::Lock(LOCK_TELEM(NSOUTPUTSTREAMWRAPPER_RELEASE));

    nsrefcnt count;
    NS_PRECONDITION(0 != mRefCnt, "dup release");
    count = --mRefCnt;
    NS_LOG_RELEASE(this, count,
                   "nsCacheEntryDescriptor::nsOutputStreamWrapper");

    if (0 == count) {
        
        if (mDescriptor)
            mDescriptor->mOutputWrapper = nullptr;

        if (desc)
            nsCacheService::Unlock();

        mRefCnt = 1;
        delete (this);
        return 0;
    }

    if (desc)
        nsCacheService::Unlock();

    return count;
}

NS_INTERFACE_MAP_BEGIN(nsCacheEntryDescriptor::nsOutputStreamWrapper)
  NS_INTERFACE_MAP_ENTRY(nsIOutputStream)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END_THREADSAFE

nsresult nsCacheEntryDescriptor::
nsOutputStreamWrapper::LazyInit()
{
    
    
    if (!mDescriptor)
        return NS_ERROR_NOT_AVAILABLE;

    nsCacheServiceAutoLock lock(LOCK_TELEM(NSOUTPUTSTREAMWRAPPER_LAZYINIT));

    nsCacheAccessMode mode;
    nsresult rv = mDescriptor->GetAccessGranted(&mode);
    if (NS_FAILED(rv)) return rv;

    NS_ENSURE_TRUE(mode & nsICache::ACCESS_WRITE, NS_ERROR_UNEXPECTED);

    nsCacheEntry* cacheEntry = mDescriptor->CacheEntry();
    if (!cacheEntry) return NS_ERROR_NOT_AVAILABLE;

    NS_ASSERTION(mOutput == nullptr, "mOutput set in LazyInit");

    nsCOMPtr<nsIOutputStream> stream;
    rv = nsCacheService::OpenOutputStreamForEntry(cacheEntry, mode, mStartOffset,
                                                  getter_AddRefs(stream));
    if (NS_FAILED(rv))
        return rv;

    nsCacheDevice* device = cacheEntry->CacheDevice();
    if (device) {
        
        int32_t size = cacheEntry->DataSize();
        rv = device->OnDataSizeChange(cacheEntry, mStartOffset - size);
        if (NS_SUCCEEDED(rv))
            cacheEntry->SetDataSize(mStartOffset);
    } else {
        rv = NS_ERROR_NOT_AVAILABLE;
    }

    
    
    if (NS_FAILED(rv)) {
        nsCacheService::ReleaseObject_Locked(stream.forget().take());
        mDescriptor->mOutputWrapper = nullptr;
        nsCacheService::ReleaseObject_Locked(mDescriptor);
        mDescriptor = nullptr;
        mInitialized = false;
        return rv;
    }

    mOutput = stream;
    mInitialized = true;
    return NS_OK;
}

nsresult nsCacheEntryDescriptor::
nsOutputStreamWrapper::EnsureInit()
{
    if (mInitialized) {
        NS_ASSERTION(mDescriptor, "Bad state");
        return NS_OK;
    }

    return LazyInit();
}

nsresult nsCacheEntryDescriptor::
nsOutputStreamWrapper::OnWrite(uint32_t count)
{
    if (count > INT32_MAX)  return NS_ERROR_UNEXPECTED;
    return mDescriptor->RequestDataSizeChange((int32_t)count);
}

void nsCacheEntryDescriptor::
nsOutputStreamWrapper::CloseInternal()
{
    mLock.AssertCurrentThreadOwns();
    if (!mDescriptor) {
        NS_ASSERTION(!mInitialized, "Bad state");
        NS_ASSERTION(!mOutput, "Bad state");
        return;
    }

    nsCacheServiceAutoLock lock(LOCK_TELEM(NSOUTPUTSTREAMWRAPPER_CLOSEINTERNAL));

    if (mDescriptor) {
        mDescriptor->mOutputWrapper = nullptr;
        nsCacheService::ReleaseObject_Locked(mDescriptor);
        mDescriptor = nullptr;
    }
    mInitialized = false;
    mOutput = nullptr;
}


NS_IMETHODIMP nsCacheEntryDescriptor::
nsOutputStreamWrapper::Close()
{
    mozilla::MutexAutoLock lock(mLock);

    return Close_Locked();
}

nsresult nsCacheEntryDescriptor::
nsOutputStreamWrapper::Close_Locked()
{
    nsresult rv = EnsureInit();
    if (NS_SUCCEEDED(rv)) {
        rv = mOutput->Close();
    } else {
        NS_ASSERTION(!mOutput,
                     "Shouldn't have mOutput when EnsureInit() failed");
    }

    
    
    CloseInternal();
    return rv;
}

NS_IMETHODIMP nsCacheEntryDescriptor::
nsOutputStreamWrapper::Flush()
{
    mozilla::MutexAutoLock lock(mLock);

    nsresult rv = EnsureInit();
    if (NS_FAILED(rv)) return rv;

    return mOutput->Flush();
}

NS_IMETHODIMP nsCacheEntryDescriptor::
nsOutputStreamWrapper::Write(const char * buf,
                             uint32_t     count,
                             uint32_t *   result)
{
    mozilla::MutexAutoLock lock(mLock);
    return Write_Locked(buf, count, result);
}

nsresult nsCacheEntryDescriptor::
nsOutputStreamWrapper::Write_Locked(const char * buf,
                                    uint32_t count,
                                    uint32_t * result)
{
    nsresult rv = EnsureInit();
    if (NS_FAILED(rv)) return rv;

    rv = OnWrite(count);
    if (NS_FAILED(rv)) return rv;

    return mOutput->Write(buf, count, result);
}

NS_IMETHODIMP nsCacheEntryDescriptor::
nsOutputStreamWrapper::WriteFrom(nsIInputStream * inStr,
                                 uint32_t         count,
                                 uint32_t *       result)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsCacheEntryDescriptor::
nsOutputStreamWrapper::WriteSegments(nsReadSegmentFun  reader,
                                     void *            closure,
                                     uint32_t          count,
                                     uint32_t *        result)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsCacheEntryDescriptor::
nsOutputStreamWrapper::IsNonBlocking(bool *result)
{
    
    *result = false;
    return NS_OK;
}







NS_IMPL_ADDREF(nsCacheEntryDescriptor::nsCompressOutputStreamWrapper)
NS_IMETHODIMP_(MozExternalRefCountType)
nsCacheEntryDescriptor::nsCompressOutputStreamWrapper::Release()
{
    
    
    nsRefPtr<nsCacheEntryDescriptor> desc;

    {
        mozilla::MutexAutoLock lock(mLock);
        desc = mDescriptor;
    }

    if (desc)
        nsCacheService::Lock(LOCK_TELEM(NSCOMPRESSOUTPUTSTREAMWRAPPER_RELEASE));

    nsrefcnt count;
    NS_PRECONDITION(0 != mRefCnt, "dup release");
    count = --mRefCnt;
    NS_LOG_RELEASE(this, count,
                   "nsCacheEntryDescriptor::nsCompressOutputStreamWrapper");

    if (0 == count) {
        
        if (mDescriptor)
            mDescriptor->mOutputWrapper = nullptr;

        if (desc)
            nsCacheService::Unlock();

        mRefCnt = 1;
        delete (this);
        return 0;
    }

    if (desc)
        nsCacheService::Unlock();

    return count;
}

NS_INTERFACE_MAP_BEGIN(nsCacheEntryDescriptor::nsCompressOutputStreamWrapper)
  NS_INTERFACE_MAP_ENTRY(nsIOutputStream)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMETHODIMP nsCacheEntryDescriptor::
nsCompressOutputStreamWrapper::Write(const char * buf,
                                     uint32_t     count,
                                     uint32_t *   result)
{
    mozilla::MutexAutoLock lock(mLock);

    int zerr = Z_OK;
    nsresult rv = NS_OK;

    if (!mStreamInitialized) {
        rv = InitZstream();
        if (NS_FAILED(rv)) {
            return rv;
        }
    }

    if (!mWriteBuffer) {
        
        
        
        mWriteBufferLen = std::max(count*2, (uint32_t)kMinCompressWriteBufLen);
        mWriteBuffer = (unsigned char*)nsMemory::Alloc(mWriteBufferLen);
        if (!mWriteBuffer) {
            mWriteBufferLen = 0;
            return NS_ERROR_OUT_OF_MEMORY;
        }
        mZstream.next_out = mWriteBuffer;
        mZstream.avail_out = mWriteBufferLen;
    }

    
    
    mZstream.avail_in = count;
    mZstream.next_in = (Bytef*)buf;
    while (mZstream.avail_in > 0) {
        zerr = deflate(&mZstream, Z_NO_FLUSH);
        if (zerr == Z_STREAM_ERROR) {
            deflateEnd(&mZstream);
            mStreamEnded = true;
            mStreamInitialized = false;
            return NS_ERROR_FAILURE;
        }
        

        
        
        if (mZstream.avail_out == 0) {
            rv = WriteBuffer();
            if (NS_FAILED(rv)) {
                deflateEnd(&mZstream);
                mStreamEnded = true;
                mStreamInitialized = false;
                return rv;
            }
        }
    }
    *result = count;
    mUncompressedCount += *result;
    return NS_OK;
}

NS_IMETHODIMP nsCacheEntryDescriptor::
nsCompressOutputStreamWrapper::Close()
{
    mozilla::MutexAutoLock lock(mLock);

    if (!mDescriptor)
        return NS_ERROR_NOT_AVAILABLE;

    nsresult retval = NS_OK;
    nsresult rv;
    int zerr = 0;

    if (mStreamInitialized) {
        
        do {
            zerr = deflate(&mZstream, Z_FINISH);
            rv = WriteBuffer();
            if (NS_FAILED(rv))
                retval = rv;
        } while (zerr == Z_OK && rv == NS_OK);
        deflateEnd(&mZstream);
        mStreamInitialized = false;
    }
    
    mStreamEnded = true;

    if (mDescriptor->CacheEntry()) {
        nsAutoCString uncompressedLenStr;
        rv = mDescriptor->GetMetaDataElement("uncompressed-len",
                                             getter_Copies(uncompressedLenStr));
        if (NS_SUCCEEDED(rv)) {
            int32_t oldCount = uncompressedLenStr.ToInteger(&rv);
            if (NS_SUCCEEDED(rv)) {
                mUncompressedCount += oldCount;
            }
        }
        uncompressedLenStr.Adopt(0);
        uncompressedLenStr.AppendInt(mUncompressedCount);
        rv = mDescriptor->SetMetaDataElement("uncompressed-len",
            uncompressedLenStr.get());
        if (NS_FAILED(rv))
            retval = rv;
    }

    if (mWriteBuffer) {
        nsMemory::Free(mWriteBuffer);
        mWriteBuffer = 0;
        mWriteBufferLen = 0;
    }

    rv = nsOutputStreamWrapper::Close_Locked();
    if (NS_FAILED(rv))
        retval = rv;

    return retval;
}

nsresult nsCacheEntryDescriptor::
nsCompressOutputStreamWrapper::InitZstream()
{
    if (!mDescriptor)
        return NS_ERROR_NOT_AVAILABLE;

    if (mStreamEnded)
        return NS_ERROR_FAILURE;

    
    
    
    
    int32_t compressionLevel = nsCacheService::CacheCompressionLevel();

    
    mZstream.zalloc = Z_NULL;
    mZstream.zfree = Z_NULL;
    mZstream.opaque = Z_NULL;
    if (deflateInit2(&mZstream, compressionLevel, Z_DEFLATED,
                     MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return NS_ERROR_FAILURE;
    }
    mZstream.next_in = Z_NULL;
    mZstream.avail_in = 0;

    mStreamInitialized = true;

    return NS_OK;
}

nsresult nsCacheEntryDescriptor::
nsCompressOutputStreamWrapper::WriteBuffer()
{
    uint32_t bytesToWrite = mWriteBufferLen - mZstream.avail_out;
    uint32_t result = 0;
    nsresult rv = nsCacheEntryDescriptor::nsOutputStreamWrapper::Write_Locked(
        (const char *)mWriteBuffer, bytesToWrite, &result);
    mZstream.next_out = mWriteBuffer;
    mZstream.avail_out = mWriteBufferLen;
    return rv;
}

