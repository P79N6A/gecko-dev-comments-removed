






#include "nsCache.h"
#include "nsDiskCache.h"
#include "nsDiskCacheDevice.h"
#include "nsDiskCacheStreams.h"
#include "nsCacheService.h"
#include "mozilla/FileUtils.h"
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





class nsDiskCacheOutputStream : public nsIOutputStream
{
public:
    nsDiskCacheOutputStream( nsDiskCacheStreamIO * parent);
    virtual ~nsDiskCacheOutputStream();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOUTPUTSTREAM

    void ReleaseStreamIO() { NS_IF_RELEASE(mStreamIO); }

private:
    nsDiskCacheStreamIO *           mStreamIO;  
    bool                            mClosed;
};


NS_IMPL_THREADSAFE_ISUPPORTS1(nsDiskCacheOutputStream,
                              nsIOutputStream)

nsDiskCacheOutputStream::nsDiskCacheOutputStream( nsDiskCacheStreamIO * parent)
    : mStreamIO(parent)
    , mClosed(false)
{
    NS_ADDREF(mStreamIO);
}


nsDiskCacheOutputStream::~nsDiskCacheOutputStream()
{
    Close();
    ReleaseStreamIO();
}


NS_IMETHODIMP
nsDiskCacheOutputStream::Close()
{
    nsresult rv = NS_OK;
    mozilla::TimeStamp start = mozilla::TimeStamp::Now();

    if (!mClosed) {
        mClosed = true;
        
        rv = mStreamIO->CloseOutputStream(this);
    }

    mozilla::Telemetry::ID id;
    if (NS_IsMainThread())
        id = mozilla::Telemetry::NETWORK_DISK_CACHE_OUTPUT_STREAM_CLOSE_MAIN_THREAD;
    else
        id = mozilla::Telemetry::NETWORK_DISK_CACHE_OUTPUT_STREAM_CLOSE;

    mozilla::Telemetry::AccumulateTimeDelta(id, start);

    return rv;
}

NS_IMETHODIMP
nsDiskCacheOutputStream::Flush()
{
    if (mClosed)  return NS_BASE_STREAM_CLOSED;
    
    return NS_OK;
}


NS_IMETHODIMP
nsDiskCacheOutputStream::Write(const char *buf, uint32_t count, uint32_t *bytesWritten)
{
    if (mClosed)  return NS_BASE_STREAM_CLOSED;
    return mStreamIO->Write(buf, count, bytesWritten);
}


NS_IMETHODIMP
nsDiskCacheOutputStream::WriteFrom(nsIInputStream *inStream, uint32_t count, uint32_t *bytesWritten)
{
    NS_NOTREACHED("WriteFrom");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsDiskCacheOutputStream::WriteSegments( nsReadSegmentFun reader,
                                        void *           closure,
                                        uint32_t         count,
                                        uint32_t *       bytesWritten)
{
    NS_NOTREACHED("WriteSegments");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsDiskCacheOutputStream::IsNonBlocking(bool * nonBlocking)
{
    *nonBlocking = false;
    return NS_OK;
}






NS_IMPL_THREADSAFE_ISUPPORTS0(nsDiskCacheStreamIO)

nsDiskCacheStreamIO::nsDiskCacheStreamIO(nsDiskCacheBinding *   binding)
    : mBinding(binding)
    , mOutStream(nullptr)
    , mInStreamCount(0)
    , mFD(nullptr)
    , mStreamEnd(0)
    , mBufSize(0)
    , mBuffer(nullptr)
{
    mDevice = (nsDiskCacheDevice *)mBinding->mCacheEntry->CacheDevice();

    
    nsCacheService *service = nsCacheService::GlobalInstance();
    NS_ADDREF(service);
}


nsDiskCacheStreamIO::~nsDiskCacheStreamIO()
{
    Close();

    
    nsCacheService *service = nsCacheService::GlobalInstance();
    NS_RELEASE(service);
}


void
nsDiskCacheStreamIO::Close()
{
    
    
    
    
    NS_ASSERTION(!mOutStream, "output stream still open");
    NS_ASSERTION(mInStreamCount == 0, "input stream still open");
    NS_ASSERTION(!mFD, "file descriptor not closed");

    DeleteBuffer();
}



nsresult
nsDiskCacheStreamIO::GetInputStream(uint32_t offset, nsIInputStream ** inputStream)
{
    NS_ENSURE_ARG_POINTER(inputStream);
    NS_ENSURE_TRUE(offset == 0, NS_ERROR_NOT_IMPLEMENTED);

    *inputStream = nullptr;
    
    if (!mBinding)  return NS_ERROR_NOT_AVAILABLE;

    if (mOutStream) {
        NS_WARNING("already have an output stream open");
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
        
        rv = ReadCacheBlocks(mStreamEnd);
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
        
    NS_ASSERTION(!mOutStream, "already have an output stream open");
    NS_ASSERTION(mInStreamCount == 0, "we already have input streams open");
    if (mOutStream || mInStreamCount)  return NS_ERROR_NOT_AVAILABLE;
    
    mStreamEnd = mBinding->mCacheEntry->DataSize();

    
    nsresult rv = SeekAndTruncate(offset);
    if (NS_FAILED(rv)) return rv;

    
    mOutStream = new nsDiskCacheOutputStream(this);
    if (!mOutStream)  return NS_ERROR_OUT_OF_MEMORY;
    
    NS_ADDREF(*outputStream = mOutStream);
    return NS_OK;
}

nsresult
nsDiskCacheStreamIO::ClearBinding()
{
    nsresult rv = NS_OK;
    if (mBinding && mOutStream)
        rv = Flush();
    mBinding = nullptr;
    return rv;
}

nsresult
nsDiskCacheStreamIO::CloseOutputStream(nsDiskCacheOutputStream *  outputStream)
{
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSDISKCACHESTREAMIO_CLOSEOUTPUTSTREAM)); 

    if (outputStream != mOutStream) {
        NS_WARNING("mismatched output streams");
        return NS_ERROR_UNEXPECTED;
    }

    
    if (!mBinding) {    
        mOutStream = nullptr;
        outputStream->ReleaseStreamIO();
        return NS_ERROR_NOT_AVAILABLE;
    }

    nsresult rv = Flush();
    if (NS_FAILED(rv))
        NS_WARNING("Flush() failed");

    mOutStream = nullptr;
    return rv;
}

nsresult
nsDiskCacheStreamIO::Flush()
{
    NS_ASSERTION(mBinding, "oops");

    CACHE_LOG_DEBUG(("CACHE: Flush [%x doomed=%u]\n",
        mBinding->mRecord.HashNumber(), mBinding->mDoomed));

    
    if (mFD) {
        (void) PR_Close(mFD);
        mFD = nullptr;
        return NS_OK;
    }

    
    nsDiskCacheMap *cacheMap = mDevice->CacheMap();  
    nsresult rv;

    bool written = false;

    if (mStreamEnd <= kMaxBufferSize) {
        

        
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
            rv = cacheMap->WriteDataCacheBlocks(mBinding, mBuffer, mStreamEnd);
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







nsresult
nsDiskCacheStreamIO::Write( const char * buffer,
                            uint32_t     count,
                            uint32_t *   bytesWritten)
{
    
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSDISKCACHESTREAMIO_WRITE));
    if (!mBinding)  return NS_ERROR_NOT_AVAILABLE;

    if (mInStreamCount) {
        
        
        NS_WARNING("Attempting to write to cache entry with open input streams.\n");
        return NS_ERROR_NOT_AVAILABLE;
    }

    NS_ASSERTION(count, "Write called with count of zero");

    
    if (!mFD && (mStreamEnd + count <= kMaxBufferSize)) {

        
        if ((mStreamEnd + count > mBufSize) && (mBufSize < kMaxBufferSize)) {
            
            
            char *newbuf = (char *) realloc(mBuffer, kMaxBufferSize);
            if (newbuf) {
                
                mBuffer = newbuf;
                mBufSize = kMaxBufferSize;
            }
        }

        
        if ((count > 0) && (mStreamEnd + count <= mBufSize)) {
            memcpy(mBuffer + mStreamEnd, buffer, count);
            mStreamEnd += count;
            *bytesWritten = count;
            return NS_OK;
        }
    }

    
    if (!mFD) {
        
        nsresult rv = FlushBufferToFile();
        if (NS_FAILED(rv)) {
            *bytesWritten = 0;
            return rv;
        }
    }
    
    if (PR_Write(mFD, buffer, count) != (int32_t)count) {
        NS_WARNING("failed to write all data");
        *bytesWritten = 0;
        return NS_ERROR_UNEXPECTED;     
    }
    mStreamEnd += count;
    *bytesWritten = count;

    UpdateFileSize();
    NS_ASSERTION(mBinding->mCacheEntry->DataSize() == mStreamEnd, "bad stream");

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
    nsCOMPtr<nsIFile>           localFile;
    
    rv = cacheMap->GetLocalFileForDiskCacheRecord(&mBinding->mRecord,
                                                  nsDiskCache::kData,
                                                  !!(flags & PR_CREATE_FILE),
                                                  getter_AddRefs(localFile));
    if (NS_FAILED(rv))  return rv;
    
    
    return localFile->OpenNSPRFileDesc(flags, 00600, fd);
}


nsresult
nsDiskCacheStreamIO::ReadCacheBlocks(uint32_t bufferSize)
{
    NS_ASSERTION(mStreamEnd == mBinding->mCacheEntry->DataSize(), "bad stream");
    NS_ASSERTION(bufferSize <= kMaxBufferSize, "bufferSize too large for buffer");
    NS_ASSERTION(mStreamEnd <= bufferSize, "data too large for buffer");

    nsDiskCacheRecord * record = &mBinding->mRecord;
    if (!record->DataLocationInitialized()) return NS_OK;

    NS_ASSERTION(record->DataFile() != kSeparateFile, "attempt to read cache blocks on separate file");

    if (!mBuffer) {
        
        mBuffer = (char *) malloc(bufferSize);
        if (!mBuffer) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        mBufSize = bufferSize;
    }
    
    
    nsDiskCacheMap *map = mDevice->CacheMap();  
    return map->ReadDataCacheBlocks(mBinding, mBuffer, mStreamEnd);
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
        
        
        rv = OpenCacheFile(PR_RDWR | PR_CREATE_FILE, &mFD);
        if (NS_FAILED(rv))  return rv;

        int64_t dataSize = mBinding->mCacheEntry->PredictedDataSize();
        if (dataSize != -1)
            mozilla::fallocate(mFD, NS_MIN<int64_t>(dataSize, kPreallocateLimit));
    }
    
    
    if (PR_Write(mFD, mBuffer, mStreamEnd) != (int32_t)mStreamEnd) {
        NS_WARNING("failed to flush all data");
        return NS_ERROR_UNEXPECTED;     
    }

    
    DeleteBuffer();
   
    return NS_OK;
}


void
nsDiskCacheStreamIO::DeleteBuffer()
{
    if (mBuffer) {
        free(mBuffer);
        mBuffer = nullptr;
        mBufSize = 0;
    }
}

size_t
nsDiskCacheStreamIO::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf)
{
    size_t usage = aMallocSizeOf(this);

    usage += aMallocSizeOf(mFD);
    usage += aMallocSizeOf(mBuffer);

    return usage;
}

nsresult
nsDiskCacheStreamIO::SeekAndTruncate(uint32_t offset)
{
    if (!mBinding)  return NS_ERROR_NOT_AVAILABLE;
    
    if (uint32_t(offset) > mStreamEnd)  return NS_ERROR_FAILURE;
    
    
    mStreamEnd = offset;

    
    if (mBinding->mRecord.DataLocationInitialized() && 
        (mBinding->mRecord.DataFile() == 0)) {
        if (!mFD) {
            
            nsresult rv = OpenCacheFile(PR_RDWR | PR_CREATE_FILE, &mFD);
            if (NS_FAILED(rv))  return rv;
        }
        if (offset) {
            if (PR_Seek(mFD, offset, PR_SEEK_SET) == -1)
                return NS_ErrorAccordingToNSPR();
        }
        nsDiskCache::Truncate(mFD, offset);
        UpdateFileSize();

        
        
        
        
        if (offset == 0) {
            
            (void) PR_Close(mFD);
            mFD = nullptr;
        }
        return NS_OK;
    }
    
    
    if (offset && !mBuffer) {
        nsresult rv = ReadCacheBlocks(kMaxBufferSize);
        if (NS_FAILED(rv))  return rv;
    }

    
    NS_ASSERTION(mStreamEnd <= kMaxBufferSize, "bad stream");
    return NS_OK;
}
