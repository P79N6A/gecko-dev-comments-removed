




#include "ipc/IPCMessageUtils.h"

#if defined(XP_UNIX) || defined(XP_BEOS)
#include <unistd.h>
#elif defined(XP_WIN)
#include <windows.h>
#include "nsILocalFileWin.h"
#else

#endif

#include "private/pprio.h"

#include "nsFileStreams.h"
#include "nsIFile.h"
#include "nsReadLine.h"
#include "nsIClassInfoImpl.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include "nsNetCID.h"

#define NS_NO_INPUT_BUFFERING 1 // see http://bugzilla.mozilla.org/show_bug.cgi?id=41067

typedef mozilla::ipc::FileDescriptor::PlatformHandleType FileHandleType;

using namespace mozilla::ipc;
using mozilla::DebugOnly;




nsFileStreamBase::nsFileStreamBase()
    : mFD(nullptr)
    , mBehaviorFlags(0)
    , mDeferredOpen(false)
{
}

nsFileStreamBase::~nsFileStreamBase()
{
    Close();
}

NS_IMPL_ISUPPORTS(nsFileStreamBase,
                  nsISeekableStream,
                  nsIFileMetadata)

NS_IMETHODIMP
nsFileStreamBase::Seek(int32_t whence, int64_t offset)
{
    nsresult rv = DoPendingOpen();
    NS_ENSURE_SUCCESS(rv, rv);

    if (mFD == nullptr)
        return NS_BASE_STREAM_CLOSED;

    int64_t cnt = PR_Seek64(mFD, offset, (PRSeekWhence)whence);
    if (cnt == int64_t(-1)) {
        return NS_ErrorAccordingToNSPR();
    }
    return NS_OK;
}

NS_IMETHODIMP
nsFileStreamBase::Tell(int64_t *result)
{
    nsresult rv = DoPendingOpen();
    NS_ENSURE_SUCCESS(rv, rv);

    if (mFD == nullptr)
        return NS_BASE_STREAM_CLOSED;

    int64_t cnt = PR_Seek64(mFD, 0, PR_SEEK_CUR);
    if (cnt == int64_t(-1)) {
        return NS_ErrorAccordingToNSPR();
    }
    *result = cnt;
    return NS_OK;
}

NS_IMETHODIMP
nsFileStreamBase::SetEOF()
{
    nsresult rv = DoPendingOpen();
    NS_ENSURE_SUCCESS(rv, rv);

    if (mFD == nullptr)
        return NS_BASE_STREAM_CLOSED;

#if defined(XP_UNIX) || defined(XP_BEOS)
    
    int64_t offset;
    rv = Tell(&offset);
    if (NS_FAILED(rv)) return rv;
#endif

#if defined(XP_UNIX) || defined(XP_BEOS)
    if (ftruncate(PR_FileDesc2NativeHandle(mFD), offset) != 0) {
        NS_ERROR("ftruncate failed");
        return NS_ERROR_FAILURE;
    }
#elif defined(XP_WIN)
    if (!SetEndOfFile((HANDLE) PR_FileDesc2NativeHandle(mFD))) {
        NS_ERROR("SetEndOfFile failed");
        return NS_ERROR_FAILURE;
    }
#else
    
#endif

    return NS_OK;
}

NS_IMETHODIMP
nsFileStreamBase::GetSize(int64_t* _retval)
{
    nsresult rv = DoPendingOpen();
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mFD) {
        return NS_BASE_STREAM_CLOSED;
    }

    PRFileInfo64 info;
    if (PR_GetOpenFileInfo64(mFD, &info) == PR_FAILURE) {
        return NS_BASE_STREAM_OSERROR;
    }

    *_retval = int64_t(info.size);

    return NS_OK;
}

NS_IMETHODIMP
nsFileStreamBase::GetLastModified(int64_t* _retval)
{
    nsresult rv = DoPendingOpen();
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mFD) {
        return NS_BASE_STREAM_CLOSED;
    }

    PRFileInfo64 info;
    if (PR_GetOpenFileInfo64(mFD, &info) == PR_FAILURE) {
        return NS_BASE_STREAM_OSERROR;
    }

    int64_t modTime = int64_t(info.modifyTime);
    if (modTime == 0) {
        *_retval = 0;
    }
    else {
        *_retval = modTime / int64_t(PR_USEC_PER_MSEC);
    }

    return NS_OK;
}

nsresult
nsFileStreamBase::Close()
{
    CleanUpOpen();

    nsresult rv = NS_OK;
    if (mFD) {
        if (PR_Close(mFD) == PR_FAILURE)
            rv = NS_BASE_STREAM_OSERROR;
        mFD = nullptr;
    }
    return rv;
}

nsresult
nsFileStreamBase::Available(uint64_t* aResult)
{
    nsresult rv = DoPendingOpen();
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mFD) {
        return NS_BASE_STREAM_CLOSED;
    }

    
    
    int64_t avail = PR_Available64(mFD);
    if (avail == -1) {
        return NS_ErrorAccordingToNSPR();
    }

    
    *aResult = (uint64_t)avail;
    return NS_OK;
}

nsresult
nsFileStreamBase::Read(char* aBuf, uint32_t aCount, uint32_t* aResult)
{
    nsresult rv = DoPendingOpen();
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mFD) {
        *aResult = 0;
        return NS_OK;
    }

    int32_t bytesRead = PR_Read(mFD, aBuf, aCount);
    if (bytesRead == -1) {
        return NS_ErrorAccordingToNSPR();
    }

    *aResult = bytesRead;
    return NS_OK;
}

nsresult
nsFileStreamBase::ReadSegments(nsWriteSegmentFun aWriter, void* aClosure,
                               uint32_t aCount, uint32_t* aResult)
{
    
    
    
    

    
    

    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFileStreamBase::IsNonBlocking(bool *aNonBlocking)
{
    *aNonBlocking = false;
    return NS_OK;
}

nsresult
nsFileStreamBase::Flush(void)
{
    nsresult rv = DoPendingOpen();
    NS_ENSURE_SUCCESS(rv, rv);

    if (mFD == nullptr)
        return NS_BASE_STREAM_CLOSED;

    int32_t cnt = PR_Sync(mFD);
    if (cnt == -1) {
        return NS_ErrorAccordingToNSPR();
    }
    return NS_OK;
}

nsresult
nsFileStreamBase::Write(const char *buf, uint32_t count, uint32_t *result)
{
    nsresult rv = DoPendingOpen();
    NS_ENSURE_SUCCESS(rv, rv);

    if (mFD == nullptr)
        return NS_BASE_STREAM_CLOSED;

    int32_t cnt = PR_Write(mFD, buf, count);
    if (cnt == -1) {
        return NS_ErrorAccordingToNSPR();
    }
    *result = cnt;
    return NS_OK;
}
    
nsresult
nsFileStreamBase::WriteFrom(nsIInputStream *inStr, uint32_t count, uint32_t *_retval)
{
    NS_NOTREACHED("WriteFrom (see source comment)");
    return NS_ERROR_NOT_IMPLEMENTED;
    
    
    
}

nsresult
nsFileStreamBase::WriteSegments(nsReadSegmentFun reader, void * closure, uint32_t count, uint32_t *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
    
    
    
}

nsresult
nsFileStreamBase::MaybeOpen(nsIFile* aFile, int32_t aIoFlags,
                            int32_t aPerm, bool aDeferred)
{
    NS_ENSURE_STATE(aFile);

    mOpenParams.ioFlags = aIoFlags;
    mOpenParams.perm = aPerm;

    if (aDeferred) {
        
        nsCOMPtr<nsIFile> file;
        nsresult rv = aFile->Clone(getter_AddRefs(file));
        NS_ENSURE_SUCCESS(rv, rv);

        mOpenParams.localFile = do_QueryInterface(file);
        NS_ENSURE_TRUE(mOpenParams.localFile, NS_ERROR_UNEXPECTED);

        mDeferredOpen = true;
        return NS_OK;
    }

    mOpenParams.localFile = aFile;

    return DoOpen();
}

void
nsFileStreamBase::CleanUpOpen()
{
    mOpenParams.localFile = nullptr;
    mDeferredOpen = false;
}

nsresult
nsFileStreamBase::DoOpen()
{
    NS_ASSERTION(!mFD, "Already have a file descriptor!");
    NS_ASSERTION(mOpenParams.localFile, "Must have a file to open");

    PRFileDesc* fd;
    nsresult rv;

#ifdef XP_WIN
    if (mBehaviorFlags & nsIFileInputStream::SHARE_DELETE) {
      nsCOMPtr<nsILocalFileWin> file = do_QueryInterface(mOpenParams.localFile);
      MOZ_ASSERT(file);

      rv = file->OpenNSPRFileDescShareDelete(mOpenParams.ioFlags,
                                             mOpenParams.perm,
                                             &fd);
    } else
#endif 
    {
      rv = mOpenParams.localFile->OpenNSPRFileDesc(mOpenParams.ioFlags,
                                                   mOpenParams.perm,
                                                   &fd);
    }

    CleanUpOpen();
    if (NS_FAILED(rv))
        return rv;
    mFD = fd;

    return NS_OK;
}

nsresult
nsFileStreamBase::DoPendingOpen()
{
    if (!mDeferredOpen) {
        return NS_OK;
    }

    return DoOpen();
}




NS_IMPL_ADDREF_INHERITED(nsFileInputStream, nsFileStreamBase)
NS_IMPL_RELEASE_INHERITED(nsFileInputStream, nsFileStreamBase)

NS_IMPL_CLASSINFO(nsFileInputStream, nullptr, nsIClassInfo::THREADSAFE,
                  NS_LOCALFILEINPUTSTREAM_CID)

NS_INTERFACE_MAP_BEGIN(nsFileInputStream)
    NS_INTERFACE_MAP_ENTRY(nsIInputStream)
    NS_INTERFACE_MAP_ENTRY(nsIFileInputStream)
    NS_INTERFACE_MAP_ENTRY(nsILineInputStream)
    NS_INTERFACE_MAP_ENTRY(nsIIPCSerializableInputStream)
    NS_IMPL_QUERY_CLASSINFO(nsFileInputStream)
NS_INTERFACE_MAP_END_INHERITING(nsFileStreamBase)

NS_IMPL_CI_INTERFACE_GETTER(nsFileInputStream,
                            nsIInputStream,
                            nsIFileInputStream,
                            nsISeekableStream,
                            nsILineInputStream)

nsresult
nsFileInputStream::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    NS_ENSURE_NO_AGGREGATION(aOuter);

    nsFileInputStream* stream = new nsFileInputStream();
    if (stream == nullptr)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(stream);
    nsresult rv = stream->QueryInterface(aIID, aResult);
    NS_RELEASE(stream);
    return rv;
}

nsresult
nsFileInputStream::Open(nsIFile* aFile, int32_t aIOFlags, int32_t aPerm)
{   
    nsresult rv = NS_OK;

    
    if (mFD) {
        rv = Close();
        if (NS_FAILED(rv)) return rv;
    }

    
    if (aIOFlags == -1)
        aIOFlags = PR_RDONLY;
    if (aPerm == -1)
        aPerm = 0;

    rv = MaybeOpen(aFile, aIOFlags, aPerm,
                   mBehaviorFlags & nsIFileInputStream::DEFER_OPEN);
    if (NS_FAILED(rv)) return rv;

    if (mBehaviorFlags & DELETE_ON_CLOSE) {
        
        
        
        
        
        rv = aFile->Remove(false);
        if (NS_SUCCEEDED(rv)) {
          
          mBehaviorFlags &= ~DELETE_ON_CLOSE;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsFileInputStream::Init(nsIFile* aFile, int32_t aIOFlags, int32_t aPerm,
                        int32_t aBehaviorFlags)
{
    NS_ENSURE_TRUE(!mFD, NS_ERROR_ALREADY_INITIALIZED);
    NS_ENSURE_TRUE(!mDeferredOpen, NS_ERROR_ALREADY_INITIALIZED);

    mBehaviorFlags = aBehaviorFlags;

    mFile = aFile;
    mIOFlags = aIOFlags;
    mPerm = aPerm;

    return Open(aFile, aIOFlags, aPerm);
}

NS_IMETHODIMP
nsFileInputStream::Close()
{
    
    
    
    if (mBehaviorFlags & REOPEN_ON_REWIND) {
        
        nsFileStreamBase::Tell(&mCachedPosition);
    }

    
    mLineBuffer = nullptr;
    nsresult rv = nsFileStreamBase::Close();
    if (NS_FAILED(rv)) return rv;
    if (mFile && (mBehaviorFlags & DELETE_ON_CLOSE)) {
        rv = mFile->Remove(false);
        NS_ASSERTION(NS_SUCCEEDED(rv), "failed to delete file");
        
        if (!(mBehaviorFlags & REOPEN_ON_REWIND)) {
          mFile = nullptr;
        }
    }
    return rv;
}

NS_IMETHODIMP
nsFileInputStream::Read(char* aBuf, uint32_t aCount, uint32_t* _retval)
{
    nsresult rv = nsFileStreamBase::Read(aBuf, aCount, _retval);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (mBehaviorFlags & CLOSE_ON_EOF && *_retval == 0) {
        Close();
    }

    return NS_OK;
}

NS_IMETHODIMP
nsFileInputStream::ReadLine(nsACString& aLine, bool* aResult)
{
    nsresult rv = DoPendingOpen();
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mLineBuffer) {
      mLineBuffer = new nsLineBuffer<char>;
    }
    return NS_ReadLine(this, mLineBuffer.get(), aLine, aResult);
}

NS_IMETHODIMP
nsFileInputStream::Seek(int32_t aWhence, int64_t aOffset)
{
    nsresult rv = DoPendingOpen();
    NS_ENSURE_SUCCESS(rv, rv);

    mLineBuffer = nullptr;
    if (!mFD) {
        if (mBehaviorFlags & REOPEN_ON_REWIND) {
            rv = Open(mFile, mIOFlags, mPerm);
            NS_ENSURE_SUCCESS(rv, rv);

            
            
            
            if (aWhence == NS_SEEK_CUR) {
                aWhence = NS_SEEK_SET;
                aOffset += mCachedPosition;
            }
        } else {
            return NS_BASE_STREAM_CLOSED;
        }
    }

    return nsFileStreamBase::Seek(aWhence, aOffset);
}

NS_IMETHODIMP
nsFileInputStream::Tell(int64_t *aResult)
{
    return nsFileStreamBase::Tell(aResult);
}

NS_IMETHODIMP
nsFileInputStream::Available(uint64_t *aResult)
{
    return nsFileStreamBase::Available(aResult);
}

void
nsFileInputStream::Serialize(InputStreamParams& aParams,
                             FileDescriptorArray& aFileDescriptors)
{
    FileInputStreamParams params;

    if (mFD) {
        FileHandleType fd = FileHandleType(PR_FileDesc2NativeHandle(mFD));
        NS_ASSERTION(fd, "This should never be null!");

        DebugOnly<FileDescriptor*> dbgFD = aFileDescriptors.AppendElement(fd);
        NS_ASSERTION(dbgFD->IsValid(), "Sending an invalid file descriptor!");

        params.fileDescriptorIndex() = aFileDescriptors.Length() - 1;

        Close();
    } else {
        NS_WARNING("This file has not been opened (or could not be opened). "
                   "Sending an invalid file descriptor to the other process!");

        params.fileDescriptorIndex() = UINT32_MAX;
    }

    int32_t behaviorFlags = mBehaviorFlags;

    
    
    behaviorFlags &= ~nsIFileInputStream::CLOSE_ON_EOF;

    
    
    behaviorFlags &= ~nsIFileInputStream::REOPEN_ON_REWIND;

    
    
    behaviorFlags &= ~nsIFileInputStream::DEFER_OPEN;

    params.behaviorFlags() = behaviorFlags;
    params.ioFlags() = mIOFlags;

    aParams = params;
}

bool
nsFileInputStream::Deserialize(const InputStreamParams& aParams,
                               const FileDescriptorArray& aFileDescriptors)
{
    NS_ASSERTION(!mFD, "Already have a file descriptor?!");
    NS_ASSERTION(!mDeferredOpen, "Deferring open?!");
    NS_ASSERTION(!mFile, "Should never have a file here!");
    NS_ASSERTION(!mPerm, "This should always be 0!");

    if (aParams.type() != InputStreamParams::TFileInputStreamParams) {
        NS_WARNING("Received unknown parameters from the other process!");
        return false;
    }

    const FileInputStreamParams& params = aParams.get_FileInputStreamParams();

    uint32_t fileDescriptorIndex = params.fileDescriptorIndex();

    FileDescriptor fd;
    if (fileDescriptorIndex < aFileDescriptors.Length()) {
        fd = aFileDescriptors[fileDescriptorIndex];
        NS_WARN_IF_FALSE(fd.IsValid(), "Received an invalid file descriptor!");
    } else {
        NS_WARNING("Received a bad file descriptor index!");
    }

    if (fd.IsValid()) {
        PRFileDesc* fileDesc = PR_ImportFile(PROsfd(fd.PlatformHandle()));
        if (!fileDesc) {
            NS_WARNING("Failed to import file handle!");
            return false;
        }
        mFD = fileDesc;
    }

    mBehaviorFlags = params.behaviorFlags();
    mIOFlags = params.ioFlags();

    return true;
}




NS_IMPL_ADDREF_INHERITED(nsPartialFileInputStream, nsFileStreamBase)
NS_IMPL_RELEASE_INHERITED(nsPartialFileInputStream, nsFileStreamBase)

NS_IMPL_CLASSINFO(nsPartialFileInputStream, nullptr, nsIClassInfo::THREADSAFE,
                  NS_PARTIALLOCALFILEINPUTSTREAM_CID)



NS_INTERFACE_MAP_BEGIN(nsPartialFileInputStream)
    NS_INTERFACE_MAP_ENTRY(nsIInputStream)
    NS_INTERFACE_MAP_ENTRY(nsIPartialFileInputStream)
    NS_INTERFACE_MAP_ENTRY(nsILineInputStream)
    NS_INTERFACE_MAP_ENTRY(nsIIPCSerializableInputStream)
    NS_IMPL_QUERY_CLASSINFO(nsPartialFileInputStream)
NS_INTERFACE_MAP_END_INHERITING(nsFileStreamBase)

NS_IMPL_CI_INTERFACE_GETTER(nsPartialFileInputStream,
                            nsIInputStream,
                            nsIPartialFileInputStream,
                            nsISeekableStream,
                            nsILineInputStream)

nsresult
nsPartialFileInputStream::Create(nsISupports *aOuter, REFNSIID aIID,
                                 void **aResult)
{
    NS_ENSURE_NO_AGGREGATION(aOuter);

    nsPartialFileInputStream* stream = new nsPartialFileInputStream();

    NS_ADDREF(stream);
    nsresult rv = stream->QueryInterface(aIID, aResult);
    NS_RELEASE(stream);
    return rv;
}

NS_IMETHODIMP
nsPartialFileInputStream::Init(nsIFile* aFile, uint64_t aStart,
                               uint64_t aLength, int32_t aIOFlags,
                               int32_t aPerm, int32_t aBehaviorFlags)
{
    mStart = aStart;
    mLength = aLength;
    mPosition = 0;

    nsresult rv = nsFileInputStream::Init(aFile, aIOFlags, aPerm,
                                          aBehaviorFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    
    return nsFileInputStream::Seek(NS_SEEK_SET, mStart);
}

NS_IMETHODIMP
nsPartialFileInputStream::Tell(int64_t *aResult)
{
    int64_t tell = 0;
    nsresult rv = nsFileInputStream::Tell(&tell);
    if (NS_SUCCEEDED(rv)) {
        *aResult = tell - mStart;
    }
    return rv;
}

NS_IMETHODIMP
nsPartialFileInputStream::Available(uint64_t* aResult)
{
    uint64_t available = 0;
    nsresult rv = nsFileInputStream::Available(&available);
    if (NS_SUCCEEDED(rv)) {
        *aResult = TruncateSize(available);
    }
    return rv;
}

NS_IMETHODIMP
nsPartialFileInputStream::Read(char* aBuf, uint32_t aCount, uint32_t* aResult)
{
    uint32_t readsize = (uint32_t) TruncateSize(aCount);
    if (readsize == 0 && mBehaviorFlags & CLOSE_ON_EOF) {
        Close();
        *aResult = 0;
        return NS_OK;
    }

    nsresult rv = nsFileInputStream::Read(aBuf, readsize, aResult);
    if (NS_SUCCEEDED(rv)) {
        mPosition += readsize;
    }
    return rv;
}

NS_IMETHODIMP
nsPartialFileInputStream::Seek(int32_t aWhence, int64_t aOffset)
{
    int64_t offset;
    switch (aWhence) {
        case NS_SEEK_SET:
            offset = mStart + aOffset;
            break;
        case NS_SEEK_CUR:
            offset = mStart + mPosition + aOffset;
            break;
        case NS_SEEK_END:
            offset = mStart + mLength + aOffset;
            break;
        default:
            return NS_ERROR_ILLEGAL_VALUE;
    }

    if (offset < (int64_t)mStart || offset > (int64_t)(mStart + mLength)) {
        return NS_ERROR_INVALID_ARG;
    }

    nsresult rv = nsFileInputStream::Seek(NS_SEEK_SET, offset);
    if (NS_SUCCEEDED(rv)) {
        mPosition = offset - mStart;
    }
    return rv;
}

void
nsPartialFileInputStream::Serialize(InputStreamParams& aParams,
                                    FileDescriptorArray& aFileDescriptors)
{
    
    InputStreamParams fileParams;
    nsFileInputStream::Serialize(fileParams, aFileDescriptors);

    PartialFileInputStreamParams params;

    params.fileStreamParams() = fileParams.get_FileInputStreamParams();
    params.begin() = mStart;
    params.length() = mLength;

    aParams = params;
}

bool
nsPartialFileInputStream::Deserialize(
                                    const InputStreamParams& aParams,
                                    const FileDescriptorArray& aFileDescriptors)
{
    NS_ASSERTION(!mFD, "Already have a file descriptor?!");
    NS_ASSERTION(!mStart, "Already have a start?!");
    NS_ASSERTION(!mLength, "Already have a length?!");
    NS_ASSERTION(!mPosition, "Already have a position?!");

    if (aParams.type() != InputStreamParams::TPartialFileInputStreamParams) {
        NS_WARNING("Received unknown parameters from the other process!");
        return false;
    }

    const PartialFileInputStreamParams& params =
        aParams.get_PartialFileInputStreamParams();

    
    InputStreamParams fileParams(params.fileStreamParams());
    if (!nsFileInputStream::Deserialize(fileParams, aFileDescriptors)) {
        NS_WARNING("Base class deserialize failed!");
        return false;
    }

    NS_ASSERTION(mFD, "Must have a file descriptor now!");

    mStart = params.begin();
    mLength = params.length();
    mPosition = 0;

    if (!mStart) {
      return true;
    }

    
    return NS_SUCCEEDED(nsFileInputStream::Seek(NS_SEEK_SET, mStart));
}




NS_IMPL_ISUPPORTS_INHERITED(nsFileOutputStream,
                            nsFileStreamBase,
                            nsIOutputStream,
                            nsIFileOutputStream)
 
nsresult
nsFileOutputStream::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    NS_ENSURE_NO_AGGREGATION(aOuter);

    nsFileOutputStream* stream = new nsFileOutputStream();
    if (stream == nullptr)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(stream);
    nsresult rv = stream->QueryInterface(aIID, aResult);
    NS_RELEASE(stream);
    return rv;
}

NS_IMETHODIMP
nsFileOutputStream::Init(nsIFile* file, int32_t ioFlags, int32_t perm,
                         int32_t behaviorFlags)
{
    NS_ENSURE_TRUE(mFD == nullptr, NS_ERROR_ALREADY_INITIALIZED);
    NS_ENSURE_TRUE(!mDeferredOpen, NS_ERROR_ALREADY_INITIALIZED);

    mBehaviorFlags = behaviorFlags;

    if (ioFlags == -1)
        ioFlags = PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE;
    if (perm <= 0)
        perm = 0664;

    return MaybeOpen(file, ioFlags, perm,
                     mBehaviorFlags & nsIFileOutputStream::DEFER_OPEN);
}




NS_IMPL_ISUPPORTS_INHERITED(nsAtomicFileOutputStream,
                            nsFileOutputStream,
                            nsISafeOutputStream,
                            nsIOutputStream,
                            nsIFileOutputStream)

NS_IMETHODIMP
nsAtomicFileOutputStream::Init(nsIFile* file, int32_t ioFlags, int32_t perm,
                             int32_t behaviorFlags)
{
    return nsFileOutputStream::Init(file, ioFlags, perm, behaviorFlags);
}

nsresult
nsAtomicFileOutputStream::DoOpen()
{
    
    
    nsCOMPtr<nsIFile> file;
    file.swap(mOpenParams.localFile);

    nsresult rv = file->Exists(&mTargetFileExists);
    if (NS_FAILED(rv)) {
        NS_ERROR("Can't tell if target file exists");
        mTargetFileExists = true; 
    }

    
    
    
    
    nsCOMPtr<nsIFile> tempResult;
    rv = file->Clone(getter_AddRefs(tempResult));
    if (NS_SUCCEEDED(rv)) {
        tempResult->SetFollowLinks(true);

        
        tempResult->Normalize();
    }

    if (NS_SUCCEEDED(rv) && mTargetFileExists) {
        uint32_t origPerm;
        if (NS_FAILED(file->GetPermissions(&origPerm))) {
            NS_ERROR("Can't get permissions of target file");
            origPerm = mOpenParams.perm;
        }
        
        
        rv = tempResult->CreateUnique(nsIFile::NORMAL_FILE_TYPE, origPerm);
    }
    if (NS_SUCCEEDED(rv)) {
        
        
        mOpenParams.localFile = tempResult;
        mTempFile = tempResult;
        mTargetFile = file;
        rv = nsFileOutputStream::DoOpen();
    }
    return rv;
}

NS_IMETHODIMP
nsAtomicFileOutputStream::Close()
{
    nsresult rv = nsFileOutputStream::Close();

    
    
    if (mTempFile) {
        mTempFile->Remove(false);
        mTempFile = nullptr;
    }

    return rv;
}

NS_IMETHODIMP
nsAtomicFileOutputStream::Finish()
{
    nsresult rv = nsFileOutputStream::Close();

    
    
    if (!mTempFile)
        return rv;

    
    if (NS_SUCCEEDED(mWriteResult) && NS_SUCCEEDED(rv)) {
        NS_ENSURE_STATE(mTargetFile);

        if (!mTargetFileExists) {
            
            
            
            
#ifdef DEBUG
            bool equal;
            if (NS_FAILED(mTargetFile->Equals(mTempFile, &equal)) || !equal)
                NS_ERROR("mTempFile not equal to mTargetFile");
#endif
        }
        else {
            nsAutoString targetFilename;
            rv = mTargetFile->GetLeafName(targetFilename);
            if (NS_SUCCEEDED(rv)) {
                
                rv = mTempFile->MoveTo(nullptr, targetFilename);
                if (NS_FAILED(rv))
                    mTempFile->Remove(false);
            }
        }
    }
    else {
        mTempFile->Remove(false);

        
        if (NS_FAILED(mWriteResult))
            rv = mWriteResult;
    }
    mTempFile = nullptr;
    return rv;
}

NS_IMETHODIMP
nsAtomicFileOutputStream::Write(const char *buf, uint32_t count, uint32_t *result)
{
    nsresult rv = nsFileOutputStream::Write(buf, count, result);
    if (NS_SUCCEEDED(mWriteResult)) {
        if (NS_FAILED(rv))
            mWriteResult = rv;
        else if (count != *result)
            mWriteResult = NS_ERROR_LOSS_OF_SIGNIFICANT_DATA;

        if (NS_FAILED(mWriteResult) && count > 0)
            NS_WARNING("writing to output stream failed! data may be lost");
    }
    return rv;
}




NS_IMETHODIMP
nsSafeFileOutputStream::Finish()
{
    (void) Flush();
    return nsAtomicFileOutputStream::Finish();
}




NS_IMPL_ISUPPORTS_INHERITED(nsFileStream,
                            nsFileStreamBase,
                            nsIInputStream,
                            nsIOutputStream,
                            nsIFileStream)

NS_IMETHODIMP
nsFileStream::Init(nsIFile* file, int32_t ioFlags, int32_t perm,
                   int32_t behaviorFlags)
{
    NS_ENSURE_TRUE(mFD == nullptr, NS_ERROR_ALREADY_INITIALIZED);
    NS_ENSURE_TRUE(!mDeferredOpen, NS_ERROR_ALREADY_INITIALIZED);

    mBehaviorFlags = behaviorFlags;

    if (ioFlags == -1)
        ioFlags = PR_RDWR;
    if (perm <= 0)
        perm = 0;

    return MaybeOpen(file, ioFlags, perm,
                     mBehaviorFlags & nsIFileStream::DEFER_OPEN);
}


