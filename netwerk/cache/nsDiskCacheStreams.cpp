








































#include "nsDiskCache.h"
#include "nsDiskCacheDevice.h"
#include "nsDiskCacheStreams.h"
#include "nsCacheService.h"
#include "mozilla/FileUtils.h"
#include "nsIDiskCacheStreamInternal.h"













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
    PRBool                          mClosed;
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
    , mClosed(PR_FALSE)
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
            mFD = nsnull;
        }
        mClosed = PR_TRUE;
    }
    return NS_OK;
}


NS_IMETHODIMP
nsDiskCacheInputStream::Available(PRUint32 * bytesAvailable)
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

    if (mClosed)
        return NS_OK;
    
    if (mPos == mStreamEnd)  return NS_OK;
    if (mPos > mStreamEnd)   return NS_ERROR_UNEXPECTED;
    
    if (count > mStreamEnd - mPos)
        count = mStreamEnd - mPos;

    if (mFD) {
        
        PRInt32  result = PR_Read(mFD, buffer, count);
        if (result < 0)  return  NS_ErrorAccordingToNSPR();
        
        mPos += (PRUint32)result;
        *bytesRead = (PRUint32)result;
        
    } else if (mBuffer) {
        
        memcpy(buffer, mBuffer + mPos, count);
        mPos += count;
        *bytesRead = count;
    } else {
        
    }

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
nsDiskCacheInputStream::IsNonBlocking(PRBool * nonBlocking)
{
    *nonBlocking = PR_FALSE;
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
    PRBool                          mClosed;
};


NS_IMPL_THREADSAFE_ISUPPORTS2(nsDiskCacheOutputStream,
                              nsIOutputStream,
                              nsIDiskCacheStreamInternal)

nsDiskCacheOutputStream::nsDiskCacheOutputStream( nsDiskCacheStreamIO * parent)
    : mStreamIO(parent)
    , mClosed(PR_FALSE)
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
    if (!mClosed) {
        mClosed = PR_TRUE;
        
        mStreamIO->CloseOutputStream(this);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsDiskCacheOutputStream::CloseInternal()
{
    if (!mClosed) {
        mClosed = PR_TRUE;
        
        mStreamIO->CloseOutputStreamInternal(this);
    }
    return NS_OK;
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
nsDiskCacheOutputStream::IsNonBlocking(PRBool * nonBlocking)
{
    *nonBlocking = PR_FALSE;
    return NS_OK;
}






NS_IMPL_THREADSAFE_ISUPPORTS0(nsDiskCacheStreamIO)




#define kMaxBufferSize      (16 * 1024)

nsDiskCacheStreamIO::nsDiskCacheStreamIO(nsDiskCacheBinding *   binding)
    : mBinding(binding)
    , mOutStream(nsnull)
    , mInStreamCount(0)
    , mFD(nsnull)
    , mStreamPos(0)
    , mStreamEnd(0)
    , mBufPos(0)
    , mBufEnd(0)
    , mBufSize(0)
    , mBufDirty(PR_FALSE)
    , mBuffer(nsnull)
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

    *inputStream = nsnull;
    
    if (!mBinding)  return NS_ERROR_NOT_AVAILABLE;

    if (mOutStream) {
        NS_WARNING("already have an output stream open");
        return NS_ERROR_NOT_AVAILABLE;
    }

    nsresult            rv;
    PRFileDesc *        fd = nsnull;

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
    *outputStream = nsnull;

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

void
nsDiskCacheStreamIO::ClearBinding()
{
    if (mBinding && mOutStream)
        Flush();
    mBinding = nsnull;
}

nsresult
nsDiskCacheStreamIO::CloseOutputStream(nsDiskCacheOutputStream *  outputStream)
{
    nsCacheServiceAutoLock lock; 
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
        mOutStream = nsnull;
        outputStream->ReleaseStreamIO();
        return NS_ERROR_NOT_AVAILABLE;
    }

    rv = Flush();
    if (NS_FAILED(rv))
        NS_WARNING("Flush() failed");

    mOutStream = nsnull;
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
            mFD = nsnull;
        }
        return NS_OK;
    }

    
    nsDiskCacheMap *cacheMap = mDevice->CacheMap();  
    nsresult rv;

    PRBool written = PR_FALSE;

    if ((mStreamEnd <= kMaxBufferSize) &&
        (mBinding->mCacheEntry->StoragePolicy() != nsICache::STORE_ON_DISK_AS_FILE)) {
        

        mBufDirty = PR_FALSE;

        
        nsDiskCacheRecord * record = &mBinding->mRecord;
        if (record->DataLocationInitialized()) {
            rv = cacheMap->DeleteStorage(record, nsDiskCache::kData);
            if (NS_FAILED(rv)) {
                NS_WARNING("cacheMap->DeleteStorage() failed.");
                cacheMap->DeleteRecord(record);
                return rv;
            }
        }

        
        written = PR_TRUE;
        if (mStreamEnd > 0) {
            rv = cacheMap->WriteDataCacheBlocks(mBinding, mBuffer, mBufEnd);
            if (NS_FAILED(rv)) {
                NS_WARNING("WriteDataCacheBlocks() failed.");
                written = PR_FALSE;
            }
        }
    }

    if (!written) {
        
        rv = FlushBufferToFile(); 

        if (mFD) {
          
          UpdateFileSize();

          
          (void) PR_Close(mFD);
          mFD = nsnull;
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
    nsCacheServiceAutoLock lock; 
    if (!mBinding)  return NS_ERROR_NOT_AVAILABLE;

    if (mInStreamCount) {
        
        
        NS_WARNING("Attempting to write to cache entry with open input streams.\n");
        return NS_ERROR_NOT_AVAILABLE;
    }

    NS_ASSERTION(count, "Write called with count of zero");
    NS_ASSERTION(mBufPos <= mBufEnd, "streamIO buffer corrupted");

    PRUint32 bytesLeft = count;
    PRBool   flushed = PR_FALSE;
    
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
                flushed = PR_TRUE;
            }
        }
        
        PRUint32 chunkSize = bytesLeft;
        if (chunkSize > (mBufSize - mBufPos))
            chunkSize =  mBufSize - mBufPos;
        
        memcpy(mBuffer + mBufPos, buffer, chunkSize);
        mBufDirty = PR_TRUE;
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
nsDiskCacheStreamIO::OpenCacheFile(PRIntn flags, PRFileDesc ** fd)
{
    NS_ENSURE_ARG_POINTER(fd);
    
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
            mozilla::fallocate(mFD, PR_MIN(dataSize, kPreallocateLimit));
    }
    
    
    PRInt32 bytesWritten = PR_Write(mFD, mBuffer, mBufEnd);
    if (PRUint32(bytesWritten) != mBufEnd) {
        NS_WARNING("failed to flush all data");
        return NS_ERROR_UNEXPECTED;     
    }
    mBufDirty = PR_FALSE;
    
    
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
        mBuffer = nsnull;
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
    PRBool      needToCloseFD = PR_FALSE;

    NS_ASSERTION(mStreamPos <= mStreamEnd, "bad stream");
    if (!mBinding)  return NS_ERROR_NOT_AVAILABLE;
    
    if (mBinding->mRecord.DataLocationInitialized()) {
        if (mBinding->mRecord.DataFile() == 0) {
            if (!mFD) {
                
                rv = OpenCacheFile(PR_RDWR | PR_CREATE_FILE, &mFD);
                if (NS_FAILED(rv))  return rv;
                needToCloseFD = PR_TRUE;
            }
        } else {
            
            if ((mStreamPos != 0) && (mStreamPos != mBufPos)) {
                
                rv = ReadCacheBlocks();
                if (NS_FAILED(rv))  return rv;
            }

            
            
            
            mBufDirty = PR_TRUE;
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
        NS_ASSERTION(mBuffer ? mBufEnd == mStreamEnd : PR_TRUE, "bad stream");
#endif
    }

    NS_ASSERTION(mStreamEnd == mBinding->mCacheEntry->DataSize(), "cache entry not updated");
    
    

    mStreamEnd  = mStreamPos;
    mBufEnd     = mBufPos;
    
    if (mFD) {
        UpdateFileSize();
        if (needToCloseFD) {
            (void) PR_Close(mFD);
            mFD = nsnull;
        } 
    }

    return  NS_OK;
}
