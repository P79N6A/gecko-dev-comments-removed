






#include "nsDiskCache.h"
#include "nsDiskCacheDevice.h"
#include "nsDiskCacheStreams.h"
#include "nsCacheService.h"
#include "mozilla/FileUtils.h"
#include "nsIDiskCacheStreamInternal.h"
#include "nsThreadUtils.h"
#include "mozilla/Telemetry.h"
#include "mozilla/TimeStamp.h"













class nsDiskCacheInputStream : public nsIInputStream {

public:

    nsDiskCacheInputStream( nsDiskCacheStreamIO * parent,
                            PRFileDesc *          fileDesc,
                            const char *          buffer,
                            PRUint32              endOfStream);

    virtual ~nsDiskCacheInputStream();
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM

private:
    nsDiskCacheStreamIO *           mStreamIO;  
    PRFileDesc *                    mFD;
    const char *                    mBuffer;
    PRUint32                        mStreamEnd;
    PRUint32                        mPos;       
    bool                            mClosed;
};


NS_IMPL_THREADSAFE_ISUPPORTS1(nsDiskCacheInputStream, nsIInputStream)


nsDiskCacheInputStream::nsDiskCacheInputStream( nsDiskCacheStreamIO * parent,
                                                PRFileDesc *          fileDesc,
                                                const char *          buffer,
                                                PRUint32              endOfStream)
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
nsDiskCacheInputStream::Available(PRUint64 * bytesAvailable)
{
    if (mClosed)  return NS_BASE_STREAM_CLOSED;
    if (mStreamEnd < mPos)  return NS_ERROR_UNEXPECTED;
    
    *bytesAvailable = mStreamEnd - mPos;
    return NS_OK;
}


NS_IMETHODIMP
nsDiskCacheInputStream::Read(char * buffer, PRUint32 count, PRUint32 * bytesRead)
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
        
        PRInt32  result = PR_Read(mFD, buffer, count);
        if (result < 0) {
            PRErrorCode error = PR_GetError();
            nsresult rv = NS_ErrorAccordingToNSPR();
            CACHE_LOG_DEBUG(("CACHE: nsDiskCacheInputStream::Read PR_Read failed"
                             "[stream=%p, rv=%d, NSPR error %s",
                             this, int(rv), PR_ErrorToName(error)));
            return rv;
        }
        
        mPos += (PRUint32)result;
        *bytesRead = (PRUint32)result;
        
    } else if (mBuffer) {
        
        memcpy(buffer, mBuffer + mPos, count);
        mPos += count;
        *bytesRead = count;
    } else {
        
    }

    CACHE_LOG_DEBUG(("CACHE: nsDiskCacheInputStream::Read "
                     "[stream=%p, count=%ud, byteRead=%ud] ",
                     this, PRUintn(count), PRUintn(*bytesRead)));
    return NS_OK;
}


NS_IMETHODIMP
nsDiskCacheInputStream::ReadSegments(nsWriteSegmentFun writer,
                                     void *            closure,
                                     PRUint32          count,
                                     PRUint32 *        bytesRead)
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
                              , public nsIDiskCacheStreamInternal
{
public:
    nsDiskCacheOutputStream( nsDiskCacheStreamIO * parent);
    virtual ~nsDiskCacheOutputStream();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOUTPUTSTREAM
    NS_DECL_NSIDISKCACHESTREAMINTERNAL

    void ReleaseStreamIO() { NS_IF_RELEASE(mStreamIO); }

private:
    nsDiskCacheStreamIO *           mStreamIO;  
    bool                            mClosed;
};


NS_IMPL_THREADSAFE_ISUPPORTS2(nsDiskCacheOutputStream,
                              nsIOutputStream,
                              nsIDiskCacheStreamInternal)

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
nsDiskCacheOutputStream::CloseInternal()
{
    nsresult rv = NS_OK;
    mozilla::TimeStamp start = mozilla::TimeStamp::Now();

    if (!mClosed) {
        mClosed = true;
        
        rv = mStreamIO->CloseOutputStreamInternal(this);
    }

    mozilla::Telemetry::ID id;
    if (NS_IsMainThread())
        id = mozilla::Telemetry::NETWORK_DISK_CACHE_OUTPUT_STREAM_CLOSE_INTERNAL_MAIN_THREAD;
    else
        id = mozilla::Telemetry::NETWORK_DISK_CACHE_OUTPUT_STREAM_CLOSE_INTERNAL;

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
nsDiskCacheOutputStream::Write(const char *buf, PRUint32 count, PRUint32 *bytesWritten)
{
    if (mClosed)  return NS_BASE_STREAM_CLOSED;
    return mStreamIO->Write(buf, count, bytesWritten);
}


NS_IMETHODIMP
nsDiskCacheOutputStream::WriteFrom(nsIInputStream *inStream, PRUint32 count, PRUint32 *bytesWritten)
{
    NS_NOTREACHED("WriteFrom");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsDiskCacheOutputStream::WriteSegments( nsReadSegmentFun reader,
                                        void *           closure,
                                        PRUint32         count,
                                        PRUint32 *       bytesWritten)
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




#define kMaxBufferSize      (16 * 1024)

nsDiskCacheStreamIO::nsDiskCacheStreamIO(nsDiskCacheBinding *   binding)
    : mBinding(binding)
    , mOutStream(nullptr)
    , mInStreamCount(0)
    , mFD(nullptr)
    , mStreamPos(0)
    , mStreamEnd(0)
    , mBufPos(0)
    , mBufEnd(0)
    , mBufSize(0)
    , mBufDirty(false)
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
nsDiskCacheStreamIO::GetInputStream(PRUint32 offset, nsIInputStream ** inputStream)
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
nsDiskCacheStreamIO::GetOutputStream(PRUint32 offset, nsIOutputStream ** outputStream)
{
    NS_ENSURE_ARG_POINTER(outputStream);
    *outputStream = nullptr;

    if (!mBinding)  return NS_ERROR_NOT_AVAILABLE;
        
    NS_ASSERTION(!mOutStream, "already have an output stream open");
    NS_ASSERTION(mInStreamCount == 0, "we already have input streams open");
    if (mOutStream || mInStreamCount)  return NS_ERROR_NOT_AVAILABLE;
    
    
    
    mBufPos    = 0;
    mStreamPos = 0;
    mStreamEnd = mBinding->mCacheEntry->DataSize();

    nsresult rv;
    if (offset) {
        rv = Seek(PR_SEEK_SET, offset);
        if (NS_FAILED(rv)) return rv;
    }
    rv = SetEOF();
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
    return CloseOutputStreamInternal(outputStream);
}

nsresult
nsDiskCacheStreamIO::CloseOutputStreamInternal(
    nsDiskCacheOutputStream * outputStream)
{
    nsresult   rv;

    if (outputStream != mOutStream) {
        NS_WARNING("mismatched output streams");
        return NS_ERROR_UNEXPECTED;
    }
    
    
    if (!mBinding) {    
        NS_ASSERTION(!mBufDirty, "oops");
        mOutStream = nullptr;
        outputStream->ReleaseStreamIO();
        return NS_ERROR_NOT_AVAILABLE;
    }

    rv = Flush();
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







nsresult
nsDiskCacheStreamIO::Write( const char * buffer,
                            PRUint32     count,
                            PRUint32 *   bytesWritten)
{
    nsresult    rv = NS_OK;
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSDISKCACHESTREAMIO_WRITE)); 
    if (!mBinding)  return NS_ERROR_NOT_AVAILABLE;

    if (mInStreamCount) {
        
        
        NS_WARNING("Attempting to write to cache entry with open input streams.\n");
        return NS_ERROR_NOT_AVAILABLE;
    }

    NS_ASSERTION(count, "Write called with count of zero");
    NS_ASSERTION(mBufPos <= mBufEnd, "streamIO buffer corrupted");

    PRUint32 bytesLeft = count;
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
        
        PRUint32 chunkSize = bytesLeft;
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
    
    return rv;
}


void
nsDiskCacheStreamIO::UpdateFileSize()
{
    NS_ASSERTION(mFD, "nsDiskCacheStreamIO::UpdateFileSize should not have been called");
    
    nsDiskCacheRecord * record = &mBinding->mRecord;
    const PRUint32      oldSizeK  = record->DataFileSize();
    PRUint32            newSizeK  = (mStreamEnd + 0x03FF) >> 10;

    
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
        
        
        rv = OpenCacheFile(PR_RDWR | PR_CREATE_FILE, &mFD);
        if (NS_FAILED(rv))  return rv;

        PRInt64 dataSize = mBinding->mCacheEntry->PredictedDataSize();
        if (dataSize != -1)
            mozilla::fallocate(mFD, NS_MIN<PRInt64>(dataSize, kPreallocateLimit));
    }
    
    
    PRInt32 bytesWritten = PR_Write(mFD, mBuffer, mBufEnd);
    if (PRUint32(bytesWritten) != mBufEnd) {
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



nsresult
nsDiskCacheStreamIO::Seek(PRInt32 whence, PRInt32 offset)
{
    PRInt32  newPos;
    if (!mBinding)  return NS_ERROR_NOT_AVAILABLE;

    if (PRUint32(offset) > mStreamEnd)  return NS_ERROR_FAILURE;
 
    if (mBinding->mRecord.DataLocationInitialized()) {
        if (mBinding->mRecord.DataFile() == 0) {
            if (!mFD) {
                
                nsresult rv = OpenCacheFile(PR_RDWR | PR_CREATE_FILE, &mFD);
                if (NS_FAILED(rv))  return rv;
            }
        }
    }

    if (mFD) {
        
        if (mBufDirty) {
            
            nsresult rv = FlushBufferToFile();
            if (NS_FAILED(rv))  return rv;
        }
    
        newPos = PR_Seek(mFD, offset, (PRSeekWhence)whence);
        if (newPos == -1)
            return NS_ErrorAccordingToNSPR();
        
        mStreamPos = (PRUint32) newPos;
        mBufPos = 0;
        mBufEnd = 0;
        return NS_OK;
    }
    
    
    
    switch(whence) {
        case PR_SEEK_SET:
            newPos = offset;
            break;
        
        case PR_SEEK_CUR:   
            newPos = offset + (PRUint32)mStreamPos;
            break;
            
        case PR_SEEK_END:   
            newPos = offset + (PRUint32)mBufEnd;
            break;
        
        default:
            return NS_ERROR_INVALID_ARG;
    }

    
    if (mStreamEnd && !mBufEnd) {
        if (newPos > 0) {
            nsresult rv = ReadCacheBlocks();
            if (NS_FAILED(rv))  return rv;
        }
    }

    
    NS_ASSERTION(mBufEnd <= kMaxBufferSize, "bad stream");
    NS_ASSERTION(mBufPos <= mBufEnd,     "bad stream");
    NS_ASSERTION(mStreamPos == mBufPos,  "bad stream");
    NS_ASSERTION(mStreamEnd == mBufEnd,  "bad stream");
    
    if ((newPos < 0) || (PRUint32(newPos) > mBufEnd)) {
        NS_WARNING("seek offset out of range");
        return NS_ERROR_INVALID_ARG;
    }

    mStreamPos = newPos;
    mBufPos    = newPos;
    return NS_OK;
}



nsresult
nsDiskCacheStreamIO::Tell(PRUint32 * result)
{
    NS_ENSURE_ARG_POINTER(result);
    *result = mStreamPos;
    return NS_OK;
}



nsresult
nsDiskCacheStreamIO::SetEOF()
{
    nsresult    rv;
    bool        needToCloseFD = false;

    NS_ASSERTION(mStreamPos <= mStreamEnd, "bad stream");
    if (!mBinding)  return NS_ERROR_NOT_AVAILABLE;
    
    if (mBinding->mRecord.DataLocationInitialized()) {
        if (mBinding->mRecord.DataFile() == 0) {
            if (!mFD) {
                
                rv = OpenCacheFile(PR_RDWR | PR_CREATE_FILE, &mFD);
                if (NS_FAILED(rv))  return rv;
                needToCloseFD = true;
            }
        } else {
            
            if ((mStreamPos != 0) && (mStreamPos != mBufPos)) {
                
                rv = ReadCacheBlocks();
                if (NS_FAILED(rv))  return rv;
            }

            
            
            
            mBufDirty = true;
        }
    }
    
    if (mFD) {
        rv = nsDiskCache::Truncate(mFD, mStreamPos);
#ifdef DEBUG
        PRUint32 oldSizeK = (mStreamEnd + 0x03FF) >> 10;
        NS_ASSERTION(mBinding->mRecord.DataFileSize() == oldSizeK, "bad disk cache entry size");
    } else {
        
        NS_ASSERTION(mStreamEnd <= kMaxBufferSize, "buffer truncation inadequate");
        NS_ASSERTION(mBufPos == mStreamPos, "bad stream");
        NS_ASSERTION(mBuffer ? mBufEnd == mStreamEnd : true, "bad stream");
#endif
    }

    NS_ASSERTION(mStreamEnd == mBinding->mCacheEntry->DataSize(), "cache entry not updated");
    
    

    mStreamEnd  = mStreamPos;
    mBufEnd     = mBufPos;
    
    if (mFD) {
        UpdateFileSize();
        if (needToCloseFD) {
            (void) PR_Close(mFD);
            mFD = nullptr;
        } 
    }

    return  NS_OK;
}
