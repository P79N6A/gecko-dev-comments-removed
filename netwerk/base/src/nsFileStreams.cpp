




#include "IPC/IPCMessageUtils.h"

#if defined(XP_UNIX) || defined(XP_BEOS)
#include <unistd.h>
#elif defined(XP_WIN)
#include <windows.h>
#elif defined(XP_OS2)
#define INCL_DOSERRORS
#include <os2.h>
#else

#endif

#include "private/pprio.h"

#include "nsFileStreams.h"
#include "nsXPIDLString.h"
#include "prerror.h"
#include "nsCRT.h"
#include "nsIFile.h"
#include "nsDirectoryIndexStream.h"
#include "nsMimeTypes.h"
#include "nsReadLine.h"
#include "nsNetUtil.h"
#include "nsIClassInfoImpl.h"

#define NS_NO_INPUT_BUFFERING 1 // see http://bugzilla.mozilla.org/show_bug.cgi?id=41067




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

NS_IMPL_THREADSAFE_ISUPPORTS1(nsFileStreamBase, nsISeekableStream)

NS_IMETHODIMP
nsFileStreamBase::Seek(PRInt32 whence, PRInt64 offset)
{
    nsresult rv = DoPendingOpen();
    NS_ENSURE_SUCCESS(rv, rv);

    if (mFD == nullptr)
        return NS_BASE_STREAM_CLOSED;

    PRInt64 cnt = PR_Seek64(mFD, offset, (PRSeekWhence)whence);
    if (cnt == PRInt64(-1)) {
        return NS_ErrorAccordingToNSPR();
    }
    return NS_OK;
}

NS_IMETHODIMP
nsFileStreamBase::Tell(PRInt64 *result)
{
    nsresult rv = DoPendingOpen();
    NS_ENSURE_SUCCESS(rv, rv);

    if (mFD == nullptr)
        return NS_BASE_STREAM_CLOSED;

    PRInt64 cnt = PR_Seek64(mFD, 0, PR_SEEK_CUR);
    if (cnt == PRInt64(-1)) {
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

#if defined(XP_UNIX) || defined(XP_OS2) || defined(XP_BEOS)
    
    PRInt64 offset;
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
#elif defined(XP_OS2)
    if (DosSetFileSize((HFILE) PR_FileDesc2NativeHandle(mFD), offset) != NO_ERROR) {
        NS_ERROR("DosSetFileSize failed");
        return NS_ERROR_FAILURE;
    }
#else
    
#endif

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
nsFileStreamBase::Available(PRUint64* aResult)
{
    nsresult rv = DoPendingOpen();
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mFD) {
        return NS_BASE_STREAM_CLOSED;
    }

    
    
    PRInt64 avail = PR_Available64(mFD);
    if (avail == -1) {
        return NS_ErrorAccordingToNSPR();
    }

    
    *aResult = (PRUint64)avail;
    return NS_OK;
}

nsresult
nsFileStreamBase::Read(char* aBuf, PRUint32 aCount, PRUint32* aResult)
{
    nsresult rv = DoPendingOpen();
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mFD) {
        *aResult = 0;
        return NS_OK;
    }

    PRInt32 bytesRead = PR_Read(mFD, aBuf, aCount);
    if (bytesRead == -1) {
        return NS_ErrorAccordingToNSPR();
    }

    *aResult = bytesRead;
    return NS_OK;
}

nsresult
nsFileStreamBase::ReadSegments(nsWriteSegmentFun aWriter, void* aClosure,
                               PRUint32 aCount, PRUint32* aResult)
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

    PRInt32 cnt = PR_Sync(mFD);
    if (cnt == -1) {
        return NS_ErrorAccordingToNSPR();
    }
    return NS_OK;
}

nsresult
nsFileStreamBase::Write(const char *buf, PRUint32 count, PRUint32 *result)
{
    nsresult rv = DoPendingOpen();
    NS_ENSURE_SUCCESS(rv, rv);

    if (mFD == nullptr)
        return NS_BASE_STREAM_CLOSED;

    PRInt32 cnt = PR_Write(mFD, buf, count);
    if (cnt == -1) {
        return NS_ErrorAccordingToNSPR();
    }
    *result = cnt;
    return NS_OK;
}
    
nsresult
nsFileStreamBase::WriteFrom(nsIInputStream *inStr, PRUint32 count, PRUint32 *_retval)
{
    NS_NOTREACHED("WriteFrom (see source comment)");
    return NS_ERROR_NOT_IMPLEMENTED;
    
    
    
}

nsresult
nsFileStreamBase::WriteSegments(nsReadSegmentFun reader, void * closure, PRUint32 count, PRUint32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
    
    
    
}

nsresult
nsFileStreamBase::MaybeOpen(nsIFile* aFile, PRInt32 aIoFlags,
                            PRInt32 aPerm, bool aDeferred)
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
    NS_PRECONDITION(mOpenParams.localFile, "Must have a file to open");

    PRFileDesc* fd;
    nsresult rv = mOpenParams.localFile->OpenNSPRFileDesc(mOpenParams.ioFlags, mOpenParams.perm, &fd);
    CleanUpOpen();
    if (NS_FAILED(rv)) return rv;
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

NS_IMPL_CLASSINFO(nsFileInputStream, NULL, nsIClassInfo::THREADSAFE,
                  NS_LOCALFILEINPUTSTREAM_CID)

NS_INTERFACE_MAP_BEGIN(nsFileInputStream)
    NS_INTERFACE_MAP_ENTRY(nsIInputStream)
    NS_INTERFACE_MAP_ENTRY(nsIFileInputStream)
    NS_INTERFACE_MAP_ENTRY(nsILineInputStream)
    NS_INTERFACE_MAP_ENTRY(nsIIPCSerializable)
    NS_IMPL_QUERY_CLASSINFO(nsFileInputStream)
NS_INTERFACE_MAP_END_INHERITING(nsFileStreamBase)

NS_IMPL_CI_INTERFACE_GETTER5(nsFileInputStream,
                             nsIInputStream,
                             nsIFileInputStream,
                             nsISeekableStream,
                             nsILineInputStream,
                             nsIIPCSerializable)

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
nsFileInputStream::Open(nsIFile* aFile, PRInt32 aIOFlags, PRInt32 aPerm)
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
nsFileInputStream::Init(nsIFile* aFile, PRInt32 aIOFlags, PRInt32 aPerm,
                        PRInt32 aBehaviorFlags)
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
    
    PR_FREEIF(mLineBuffer);
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
nsFileInputStream::Read(char* aBuf, PRUint32 aCount, PRUint32* _retval)
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
        nsresult rv = NS_InitLineBuffer(&mLineBuffer);
        if (NS_FAILED(rv)) return rv;
    }
    return NS_ReadLine(this, mLineBuffer, aLine, aResult);
}

NS_IMETHODIMP
nsFileInputStream::Seek(PRInt32 aWhence, PRInt64 aOffset)
{
    nsresult rv = DoPendingOpen();
    NS_ENSURE_SUCCESS(rv, rv);

    PR_FREEIF(mLineBuffer); 
    if (!mFD) {
        if (mBehaviorFlags & REOPEN_ON_REWIND) {
            nsresult rv = Reopen();
            if (NS_FAILED(rv)) {
                return rv;
            }
        } else {
            return NS_BASE_STREAM_CLOSED;
        }
    }

    return nsFileStreamBase::Seek(aWhence, aOffset);
}

bool
nsFileInputStream::Read(const IPC::Message *aMsg, void **aIter)
{
    using IPC::ReadParam;

    nsCString path;
    bool followLinks;
    PRInt32 flags;
    if (!ReadParam(aMsg, aIter, &path) ||
        !ReadParam(aMsg, aIter, &followLinks) ||
        !ReadParam(aMsg, aIter, &flags))
        return false;

    nsCOMPtr<nsIFile> file;
    nsresult rv = NS_NewNativeLocalFile(path, followLinks, getter_AddRefs(file));
    if (NS_FAILED(rv))
        return false;

    
    
    rv = Init(file, -1, -1, flags);
    if (NS_FAILED(rv))
        return false;

    return true;
}

void
nsFileInputStream::Write(IPC::Message *aMsg)
{
    using IPC::WriteParam;

    nsCString path;
    mFile->GetNativePath(path);
    WriteParam(aMsg, path);
    bool followLinks;
    mFile->GetFollowLinks(&followLinks);
    WriteParam(aMsg, followLinks);
    WriteParam(aMsg, mBehaviorFlags);
}




NS_IMPL_ADDREF_INHERITED(nsPartialFileInputStream, nsFileStreamBase)
NS_IMPL_RELEASE_INHERITED(nsPartialFileInputStream, nsFileStreamBase)

NS_IMPL_CLASSINFO(nsPartialFileInputStream, NULL, nsIClassInfo::THREADSAFE,
                  NS_PARTIALLOCALFILEINPUTSTREAM_CID)



NS_INTERFACE_MAP_BEGIN(nsPartialFileInputStream)
    NS_INTERFACE_MAP_ENTRY(nsIInputStream)
    NS_INTERFACE_MAP_ENTRY(nsIPartialFileInputStream)
    NS_INTERFACE_MAP_ENTRY(nsILineInputStream)
    NS_INTERFACE_MAP_ENTRY(nsIIPCSerializable)
    NS_IMPL_QUERY_CLASSINFO(nsPartialFileInputStream)
NS_INTERFACE_MAP_END_INHERITING(nsFileStreamBase)

NS_IMPL_CI_INTERFACE_GETTER5(nsPartialFileInputStream,
                             nsIInputStream,
                             nsIPartialFileInputStream,
                             nsISeekableStream,
                             nsILineInputStream,
                             nsIIPCSerializable)

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
nsPartialFileInputStream::Init(nsIFile* aFile, PRUint64 aStart,
                               PRUint64 aLength, PRInt32 aIOFlags,
                               PRInt32 aPerm, PRInt32 aBehaviorFlags)
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
nsPartialFileInputStream::Tell(PRInt64 *aResult)
{
    PRInt64 tell;
    nsresult rv = nsFileInputStream::Tell(&tell);
    if (NS_SUCCEEDED(rv)) {
        *aResult = tell - mStart;
    }
    return rv;
}

NS_IMETHODIMP
nsPartialFileInputStream::Available(PRUint64* aResult)
{
    PRUint64 available;
    nsresult rv = nsFileInputStream::Available(&available);
    if (NS_SUCCEEDED(rv)) {
        *aResult = TruncateSize(available);
    }
    return rv;
}

NS_IMETHODIMP
nsPartialFileInputStream::Read(char* aBuf, PRUint32 aCount, PRUint32* aResult)
{
    PRUint32 readsize = (PRUint32) TruncateSize(aCount);
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
nsPartialFileInputStream::Seek(PRInt32 aWhence, PRInt64 aOffset)
{
    PRInt64 offset;
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

    if (offset < (PRInt64)mStart || offset > (PRInt64)(mStart + mLength)) {
        return NS_ERROR_INVALID_ARG;
    }

    nsresult rv = nsFileInputStream::Seek(NS_SEEK_SET, offset);
    if (NS_SUCCEEDED(rv)) {
        mPosition = offset - mStart;
    }
    return rv;
}

bool
nsPartialFileInputStream::Read(const IPC::Message *aMsg, void **aIter)
{
    using IPC::ReadParam;

    
    PRUint64 start;
    PRUint64 length;
    if (!ReadParam(aMsg, aIter, &start) ||
        !ReadParam(aMsg, aIter, &length))
        return false;

    
    if (!nsFileInputStream::Read(aMsg, aIter))
        return false;

    
    mStart = start;
    mLength = length;

    
    
    
    mPosition = 0;

    
    
    
    return NS_SUCCEEDED(nsFileInputStream::Seek(NS_SEEK_SET, start));
}

void
nsPartialFileInputStream::Write(IPC::Message *aMsg)
{
    using IPC::WriteParam;

    
    WriteParam(aMsg, mStart);
    WriteParam(aMsg, mLength);

    
    
    
    if (mPosition) {
      NS_WARNING("No support for sending a partially-consumed input stream!");
    }

    
    nsFileInputStream::Write(aMsg);
}




NS_IMPL_ISUPPORTS_INHERITED2(nsFileOutputStream,
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
nsFileOutputStream::Init(nsIFile* file, PRInt32 ioFlags, PRInt32 perm,
                         PRInt32 behaviorFlags)
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




NS_IMPL_ISUPPORTS_INHERITED3(nsSafeFileOutputStream,
                             nsFileOutputStream,
                             nsISafeOutputStream,
                             nsIOutputStream,
                             nsIFileOutputStream)

NS_IMETHODIMP
nsSafeFileOutputStream::Init(nsIFile* file, PRInt32 ioFlags, PRInt32 perm,
                             PRInt32 behaviorFlags)
{
    return nsFileOutputStream::Init(file, ioFlags, perm, behaviorFlags);
}

nsresult
nsSafeFileOutputStream::DoOpen()
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
        PRUint32 origPerm;
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
nsSafeFileOutputStream::Close()
{
    nsresult rv = nsFileOutputStream::Close();

    
    
    if (mTempFile) {
        mTempFile->Remove(false);
        mTempFile = nullptr;
    }

    return rv;
}

NS_IMETHODIMP
nsSafeFileOutputStream::Finish()
{
    Flush();
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
nsSafeFileOutputStream::Write(const char *buf, PRUint32 count, PRUint32 *result)
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




NS_IMPL_ISUPPORTS_INHERITED4(nsFileStream, 
                             nsFileStreamBase,
                             nsIInputStream,
                             nsIOutputStream,
                             nsIFileStream,
                             nsIFileMetadata)
 
NS_IMETHODIMP
nsFileStream::Init(nsIFile* file, PRInt32 ioFlags, PRInt32 perm,
                   PRInt32 behaviorFlags)
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

NS_IMETHODIMP
nsFileStream::GetSize(PRInt64* _retval)
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

    *_retval = PRInt64(info.size);

    return NS_OK;
}

NS_IMETHODIMP
nsFileStream::GetLastModified(PRInt64* _retval)
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

    PRInt64 modTime = PRInt64(info.modifyTime);
    if (modTime == 0) {
        *_retval = 0;
    }
    else {
        *_retval = modTime / PRInt64(PR_USEC_PER_MSEC);
    }

    return NS_OK;
}


