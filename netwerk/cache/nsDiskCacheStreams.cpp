






#include "nsDiskCache.h"
#include "nsDiskCacheDevice.h"
#include "nsDiskCacheStreams.h"
#include "nsCacheService.h"
#include "mozilla/FileUtils.h"
#include "nsIDiskCacheStreamInternal.h"
#include "nsThreadUtils.h"
#include "mozilla/Telemetry.h"
#include "mozilla/TimeStamp.h"










#define kMaxBufferSize      (16 * 1024)




class nsDiskCacheInputStream : public nsIInputStream {

public:

    nsDiskCacheInputStream( nsDiskCacheStreamIO * parent,
                            PRFileDesc *          fileDesc,
                            const char *          buffer,
                            uint32_t              endOfStream);

    virtual ~nsDiskCacheInputStream();
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM

private:
    nsDiskCacheStreamIO *           mStreamIO;  
    PRFileDesc *                    mFD;
    const char *                    mBuffer;
    uint32_t                        mStreamEnd;
    uint32_t                        mPos;       
    bool                            mClosed;
};


NS_IMPL_THREADSAFE_ISUPPORTS1(nsDiskCacheInputStream, nsIInputStream)


nsDiskCacheInputStream::nsDiskCacheInputStream( nsDiskCacheStreamIO * parent,
                                                PRFileDesc *          fileDesc,
                                                const char *          buffer,
                                                uint32_t              endOfStream)
    : mStreamIO(parent)
    , mFD(fileDesc)
    , mBuffer(buffer)
    , mStreamEnd(endOfStream)
    , mPos(0)
    , mClosed(false)
{
    NS_ADDREF(mStreamIO);
    mStreamIO->IncrementInputStreamCount();
}


nsDiskCacheInputStream::~nsDiskCacheInputStream()
{
    Close();
    mStreamIO->DecrementInputStreamCount();
    NS_RELEASE(mStreamIO);
}


NS_IMETHODIMP
nsDiskCacheInputStream::Close()
{
    if (!mClosed) {
        if (mFD) {
            (void) PR_Close(mFD);
            mFD = nullptr;
        }
        mClosed = true;
    }
    return NS_OK;
}


NS_IMETHODIMP
nsDiskCacheInputStream::Available(uint64_t * bytesAvailable)
{
    if (mClosed)  return NS_BASE_STREAM_CLOSED;
    if (mStreamEnd < mPos)  return NS_ERROR_UNEXPECTED;
    
    *bytesAvailable = mStreamEnd - mPos;
    return NS_OK;
}


NS_IMETHODIMP
nsDiskCacheInputStream::Read(char * buffer, uint32_t count, uint32_t * bytesRead)
{
    *bytesRead = 0;

    if (mClosed) {
        CACHE_LOG_DEBUG(("CACHE: nsDiskCacheInputStream::Read "
                         "[stream=%p] stream was closed",
                         this, buffer, count));
        return NS_OK;
    }
    
    if (mPos == mStreamEnd) {
        CACHE_LOG_DEBUG(("CACHE: nsDiskCacheInputStream::Read "
                         "[stream=%p] stream at end of file",
                         this, buffer, count));
        return NS_OK;
    }
    if (mPos > mStreamEnd) {
        CACHE_LOG_DEBUG(("CACHE: nsDiskCacheInputStream::Read "
                         "[stream=%p] stream past end of file (!)",
                         this, buffer, count));
        return NS_ERROR_UNEXPECTED;
    }
    
    if (count > mStreamEnd - mPos)
        count = mStreamEnd - mPos;

    if (mFD) {
        
        int32_t  result = PR_Read(mFD, buffer, count);
        if (result < 0) {
            nsresult rv = NS_ErrorAccordingToNSPR();
            CACHE_LOG_DEBUG(("CACHE: nsDiskCacheInputStream::Read PR_Read failed"
                             "[stream=%p, rv=%d, NSPR error %s",
                             this, int(rv), PR_ErrorToName(PR_GetError())));
            return rv;
        }
        
        mPos += (uint32_t)result;
        *bytesRead = (uint32_t)result;
        
    } else if (mBuffer) {
        
        memcpy(buffer, mBuffer + mPos, count);
        mPos += count;
        *bytesRead = count;
    } else {
        
    }

    CACHE_LOG_DEBUG(("CACHE: nsDiskCacheInputStream::Read "
                     "[stream=%p, count=%ud, byteRead=%ud] ",
                     this, unsigned(count), unsigned(*bytesRead)));
    return NS_OK;
}


NS_IMETHODIMP
nsDiskCacheInputStream::ReadSegments(nsWriteSegmentFun writer,
                                     void *            closure,
                                     uint32_t          count,
                                     uint32_t *        bytesRead)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsDiskCacheInputStream::IsNonBlocking(bool * nonBlocking)
{
    *nonBlocking = false;
    return NS_OK;
}




NS_IMPL_THREADSAFE_ISUPPORTS2(nsDiskCacheStreamIO, nsIOutputStream, nsIDiskCacheStreamInternal)

nsDiskCacheStreamIO::nsDiskCacheStreamIO(nsDiskCacheBinding *   binding)
    : mBinding(binding)
    , mInStreamCount(0)
    , mFD(nullptr)
    , mStreamPos(0)
    , mStreamEnd(0)
    , mBufPos(0)
    , mBufEnd(0)
    , mBufSize(0)
    , mBufDirty(false)
    , mOutputStreamIsOpen(false)
    , mBuffer(nullptr)
{
    mDevice = (nsDiskCacheDevice *)mBinding->mCacheEntry->CacheDevice();

    
    nsCacheService *service = nsCacheService::GlobalInstance();
    NS_ADDREF(service);
}


nsDiskCacheStreamIO::~nsDiskCacheStreamIO()
{
    if (mOutputStreamIsOpen) {
        nsCacheService::AssertOwnsLock();
        CloseInternal();
    }

    NS_ASSERTION(!mOutputStreamIsOpen, "output stream still open");
    NS_ASSERTION(mInStreamCount == 0, "input stream still open");
    NS_ASSERTION(!mFD, "file descriptor not closed");

    DeleteBuffer();

    
    nsCacheService *service = nsCacheService::GlobalInstance();
    NS_RELEASE(service);
}


NS_IMETHODIMP
nsDiskCacheStreamIO::WriteFrom(nsIInputStream *inStream, uint32_t count, uint32_t *bytesWritten)
{
    NS_NOTREACHED("WriteFrom");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsDiskCacheStreamIO::WriteSegments(nsReadSegmentFun reader,
                                       void *           closure,
                                       uint32_t         count,
                                       uint32_t *       bytesWritten)
{
    NS_NOTREACHED("WriteSegments");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsDiskCacheStreamIO::IsNonBlocking(bool * nonBlocking)
{
  *nonBlocking = false;
  return NS_OK;
}


NS_IMETHODIMP
nsDiskCacheStreamIO::Close()
{
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSDISKCACHESTREAMIO_CLOSEOUTPUTSTREAM));
    return CloseInternal();
}



nsresult
nsDiskCacheStreamIO::GetInputStream(uint32_t offset, nsIInputStream ** inputStream)
{
    NS_ENSURE_ARG_POINTER(inputStream);
    NS_ENSURE_TRUE(offset == 0, NS_ERROR_NOT_IMPLEMENTED);

    *inputStream = nullptr;
    
    if (!mBinding)  return NS_ERROR_NOT_AVAILABLE;

    if (mOutputStreamIsOpen) {
        NS_WARNING("already have the output stream open");
        return NS_ERROR_NOT_AVAILABLE;
    }

    nsresult            rv;
    PRFileDesc *        fd = nullptr;

    mStreamEnd = mBinding->mCacheEntry->DataSize();
    if (mStreamEnd == 0) {
        
        NS_ASSERTION(!mBinding->mRecord.DataLocationInitialized(), "storage allocated for zero data size");
    } else if (mBinding->mRecord.DataFile() == 0) {
        
        rv = OpenCacheFile(PR_RDONLY, &fd);
        if (NS_FAILED(rv))  return rv;  
        NS_ASSERTION(fd, "cache stream lacking open file.");
            
    } else if (!mBuffer) {
        
        rv = ReadCacheBlocks();
        if (NS_FAILED(rv))  return rv;
    }
    
    

    NS_ASSERTION(!(fd && mBuffer), "ambiguous data sources for input stream");

    
    nsDiskCacheInputStream * inStream = new nsDiskCacheInputStream(this, fd, mBuffer, mStreamEnd);
    if (!inStream)  return NS_ERROR_OUT_OF_MEMORY;
    
    NS_ADDREF(*inputStream = inStream);
    return NS_OK;
}



nsresult
nsDiskCacheStreamIO::GetOutputStream(uint32_t offset, nsIOutputStream ** outputStream)
{
    NS_ENSURE_ARG_POINTER(outputStream);
    *outputStream = nullptr;

    if (!mBinding)  return NS_ERROR_NOT_AVAILABLE;

    NS_ASSERTION(!mOutputStreamIsOpen, "already have the output stream open");
    NS_ASSERTION(mInStreamCount == 0, "we already have input streams open");
    if (mOutputStreamIsOpen || mInStreamCount)  return NS_ERROR_NOT_AVAILABLE;
    
    
    
    mBufPos    = 0;
    mStreamPos = 0;
    mStreamEnd = mBinding->mCacheEntry->DataSize();

    if (offset > mStreamEnd) {
        NS_WARNING("seek offset out of range");
        return NS_ERROR_INVALID_ARG;
    }

    nsresult rv;
    
    if (mBinding->mRecord.DataLocationInitialized() &&
        (mBinding->mRecord.DataFile() == 0)) {
        
        rv = OpenCacheFile(PR_WRONLY | PR_CREATE_FILE, &mFD);
        NS_ENSURE_SUCCESS(rv, rv);
        if (offset) {
            int32_t newPos = PR_Seek(mFD, offset, PR_SEEK_SET);
            if (newPos == -1) {
                return NS_ErrorAccordingToNSPR();
            }
        }

        
        rv = nsDiskCache::Truncate(mFD, offset);
        NS_ENSURE_SUCCESS(rv, rv);

        mStreamPos = mStreamEnd = offset;
        UpdateFileSize();
    } else if (offset) {
        
        rv = ReadCacheBlocks();
        NS_ENSURE_SUCCESS(rv, rv);

        
        mBufEnd = mBufPos = offset;
        mStreamPos = mStreamEnd = offset;
    }

    mOutputStreamIsOpen = true;
    
    NS_ADDREF(*outputStream = this);
    return NS_OK;
}


nsresult
nsDiskCacheStreamIO::ClearBinding()
{
    nsresult rv = NS_OK;
    if (mBinding && mOutputStreamIsOpen)
        rv = Flush();
    mBinding = nullptr;
    return rv;
}


NS_IMETHODIMP
nsDiskCacheStreamIO::CloseInternal()
{
    mozilla::TimeStamp start = mozilla::TimeStamp::Now();

    if (mOutputStreamIsOpen) {
        if (!mBinding) {    
            NS_ASSERTION(!mBufDirty, "oops");
        } else {
            nsresult rv = Flush();
            NS_ENSURE_SUCCESS(rv, rv);
        }
        mOutputStreamIsOpen = PR_FALSE;
    }

    
    if (mFD) {
        (void) PR_Close(mFD);
        mFD = nullptr;
    }

    mozilla::Telemetry::ID id;
    if (NS_IsMainThread())
        id = mozilla::Telemetry::NETWORK_DISK_CACHE_OUTPUT_STREAM_CLOSE_INTERNAL_MAIN_THREAD;
    else
        id = mozilla::Telemetry::NETWORK_DISK_CACHE_OUTPUT_STREAM_CLOSE_INTERNAL;

    mozilla::Telemetry::AccumulateTimeDelta(id, start);

    return NS_OK;
}


NS_IMETHODIMP
nsDiskCacheStreamIO::Flush()
{
    if (!mOutputStreamIsOpen) return NS_BASE_STREAM_CLOSED;

    NS_ASSERTION(mBinding, "oops");

    CACHE_LOG_DEBUG(("CACHE: Flush [%x doomed=%u]\n",
        mBinding->mRecord.HashNumber(), mBinding->mDoomed));

    if (!mBufDirty) {
        if (mFD) {
            (void) PR_Close(mFD);
            mFD = nullptr;
        }
        return NS_OK;
    }

    
    nsDiskCacheMap *cacheMap = mDevice->CacheMap();  
    nsresult rv;

    bool written = false;

    if ((mStreamEnd <= kMaxBufferSize) &&
        (mBinding->mCacheEntry->StoragePolicy() != nsICache::STORE_ON_DISK_AS_FILE)) {
        

        mBufDirty = false;

        
        nsDiskCacheRecord * record = &mBinding->mRecord;
        if (record->DataLocationInitialized()) {
            rv = cacheMap->DeleteStorage(record, nsDiskCache::kData);
            if (NS_FAILED(rv)) {
                NS_WARNING("cacheMap->DeleteStorage() failed.");
                return rv;
            }
        }

        
        written = true;
        if (mStreamEnd > 0) {
            rv = cacheMap->WriteDataCacheBlocks(mBinding, mBuffer, mBufEnd);
            if (NS_FAILED(rv)) {
                NS_WARNING("WriteDataCacheBlocks() failed.");
                written = false;
            }
        }
    }

    if (!written) {
        
        rv = FlushBufferToFile(); 

        if (mFD) {
          
          UpdateFileSize();

          
          (void) PR_Close(mFD);
          mFD = nullptr;
        }
        else
          NS_WARNING("no file descriptor");

        
        
        NS_ENSURE_SUCCESS(rv, rv);

        
        
        
        
        
        DeleteBuffer();
    }
    
    
    
    if (!mBinding->mDoomed) {
        rv = cacheMap->UpdateRecord(&mBinding->mRecord);
        if (NS_FAILED(rv)) {
            NS_WARNING("cacheMap->UpdateRecord() failed.");
            return rv;   
        }
    }
    
    return NS_OK;
}







NS_IMETHODIMP
nsDiskCacheStreamIO::Write( const char * buffer,
                            uint32_t     count,
                            uint32_t *   bytesWritten)
{
    if (!mOutputStreamIsOpen) {
        return NS_BASE_STREAM_CLOSED;
    }

    nsCacheServiceAutoLock lock(LOCK_TELEM(NSDISKCACHESTREAMIO_WRITE)); 
    if (!mBinding)  return NS_ERROR_NOT_AVAILABLE;

    if (mInStreamCount) {
        
        
        NS_WARNING("Attempting to write to cache entry with open input streams.\n");
        return NS_ERROR_NOT_AVAILABLE;
    }

    NS_ASSERTION(count, "Write called with count of zero");
    NS_ASSERTION(mBufPos <= mBufEnd, "streamIO buffer corrupted");

    uint32_t bytesLeft = count;
    bool     flushed = false;
    
    while (bytesLeft) {
        if (mBufPos == mBufSize) {
            if (mBufSize < kMaxBufferSize) {
                mBufSize = kMaxBufferSize;
                char *buffer = mBuffer;

                mBuffer  = (char *) realloc(mBuffer, mBufSize);
                if (!mBuffer) {
                    free(buffer);
                    mBufSize = 0;
                    break;
                }
            } else {
                nsresult rv = FlushBufferToFile();
                if (NS_FAILED(rv))  break;
                flushed = true;
            }
        }
        
        uint32_t chunkSize = bytesLeft;
        if (chunkSize > (mBufSize - mBufPos))
            chunkSize =  mBufSize - mBufPos;
        
        memcpy(mBuffer + mBufPos, buffer, chunkSize);
        mBufDirty = true;
        mBufPos += chunkSize;
        bytesLeft -= chunkSize;
        buffer += chunkSize;
        
        if (mBufEnd < mBufPos)
            mBufEnd = mBufPos;
    }
    if (bytesLeft) {
        *bytesWritten = 0;
        return NS_ERROR_FAILURE;
    }
    *bytesWritten = count;

    
    mStreamPos += count;
    if (mStreamEnd < mStreamPos) {
        mStreamEnd = mStreamPos;
        NS_ASSERTION(mBinding->mCacheEntry->DataSize() == mStreamEnd, "bad stream");

        
        if (flushed && mFD) {
            UpdateFileSize();
        }
    }
    
    return NS_OK;
}


void
nsDiskCacheStreamIO::UpdateFileSize()
{
    NS_ASSERTION(mFD, "nsDiskCacheStreamIO::UpdateFileSize should not have been called");
    
    nsDiskCacheRecord * record = &mBinding->mRecord;
    const uint32_t      oldSizeK  = record->DataFileSize();
    uint32_t            newSizeK  = (mStreamEnd + 0x03FF) >> 10;

    
    if (newSizeK > kMaxDataSizeK)
        newSizeK = kMaxDataSizeK;

    if (newSizeK == oldSizeK)  return;
    
    record->SetDataFileSize(newSizeK);

    
    nsDiskCacheMap * cacheMap = mDevice->CacheMap();
    cacheMap->DecrementTotalSize(oldSizeK);       
    cacheMap->IncrementTotalSize(newSizeK);       
    
    if (!mBinding->mDoomed) {
        nsresult rv = cacheMap->UpdateRecord(record);
        if (NS_FAILED(rv)) {
            NS_WARNING("cacheMap->UpdateRecord() failed.");
            
        }
    }
}


nsresult
nsDiskCacheStreamIO::OpenCacheFile(int flags, PRFileDesc ** fd)
{
    NS_ENSURE_ARG_POINTER(fd);
    
    CACHE_LOG_DEBUG(("nsDiskCacheStreamIO::OpenCacheFile"));

    nsresult         rv;
    nsDiskCacheMap * cacheMap = mDevice->CacheMap();
    
    rv = cacheMap->GetLocalFileForDiskCacheRecord(&mBinding->mRecord,
                                                  nsDiskCache::kData,
                                                  !!(flags & PR_CREATE_FILE),
                                                  getter_AddRefs(mLocalFile));
    if (NS_FAILED(rv))  return rv;
    
    
    rv = mLocalFile->OpenNSPRFileDesc(flags, 00600, fd);
    if (NS_FAILED(rv))  return rv;  

    return NS_OK;
}


nsresult
nsDiskCacheStreamIO::ReadCacheBlocks()
{
    NS_ASSERTION(mStreamEnd == mBinding->mCacheEntry->DataSize(), "bad stream");
    NS_ASSERTION(mStreamEnd <= kMaxBufferSize, "data too large for buffer");

    nsDiskCacheRecord * record = &mBinding->mRecord;
    if (!record->DataLocationInitialized()) return NS_OK;

    NS_ASSERTION(record->DataFile() != kSeparateFile, "attempt to read cache blocks on separate file");

    if (!mBuffer) {
        
        mBuffer = (char *) malloc(mStreamEnd);
        if (!mBuffer) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        mBufSize = mStreamEnd;
    }
    
    
    nsDiskCacheMap *map = mDevice->CacheMap();  
    nsresult rv = map->ReadDataCacheBlocks(mBinding, mBuffer, mStreamEnd);
    if (NS_FAILED(rv)) return rv;

    
    mBufPos = 0;
    mBufEnd = mStreamEnd;
    
    return NS_OK;
}


nsresult
nsDiskCacheStreamIO::FlushBufferToFile()
{
    nsresult  rv;
    nsDiskCacheRecord * record = &mBinding->mRecord;
    
    if (!mFD) {
        if (record->DataLocationInitialized() && (record->DataFile() > 0)) {
            
            nsDiskCacheMap * cacheMap = mDevice->CacheMap();
            rv = cacheMap->DeleteStorage(record, nsDiskCache::kData);
            if (NS_FAILED(rv))  return rv;
        }
        record->SetDataFileGeneration(mBinding->mGeneration);
        
        
        rv = OpenCacheFile(PR_WRONLY | PR_CREATE_FILE, &mFD);
        if (NS_FAILED(rv))  return rv;

        int64_t dataSize = mBinding->mCacheEntry->PredictedDataSize();
        if (dataSize != -1)
            mozilla::fallocate(mFD, NS_MIN<int64_t>(dataSize, kPreallocateLimit));
    }
    
    
    int32_t bytesWritten = PR_Write(mFD, mBuffer, mBufEnd);
    if (uint32_t(bytesWritten) != mBufEnd) {
        NS_WARNING("failed to flush all data");
        return NS_ERROR_UNEXPECTED;     
    }
    mBufDirty = false;
    
    
    mBufPos = 0;
    mBufEnd = 0;
    
    return NS_OK;
}


void
nsDiskCacheStreamIO::DeleteBuffer()
{
    if (mBuffer) {
        NS_ASSERTION(!mBufDirty, "deleting dirty buffer");
        free(mBuffer);
        mBuffer = nullptr;
        mBufPos = 0;
        mBufEnd = 0;
        mBufSize = 0;
    }
}
