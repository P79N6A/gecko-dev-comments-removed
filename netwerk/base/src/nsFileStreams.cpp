




































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
#include "nsILocalFile.h"
#include "nsXPIDLString.h"
#include "prerror.h"
#include "nsCRT.h"
#include "nsInt64.h"
#include "nsIFile.h"
#include "nsDirectoryIndexStream.h"
#include "nsMimeTypes.h"
#include "nsReadLine.h"
#include "nsNetUtil.h"


#define NS_NO_INPUT_BUFFERING 1 // see http://bugzilla.mozilla.org/show_bug.cgi?id=41067




nsFileStream::nsFileStream()
    : mFD(nsnull)
    , mCloseFD(PR_TRUE)
{
}

nsFileStream::~nsFileStream()
{
    if (mCloseFD)
        Close();
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsFileStream, nsISeekableStream)

nsresult
nsFileStream::InitWithFileDescriptor(PRFileDesc* fd, nsISupports* parent)
{
    NS_ENSURE_TRUE(mFD == nsnull, NS_ERROR_ALREADY_INITIALIZED);
    
    
    
    
    
    mFD = fd;
    mCloseFD = PR_FALSE;
    mParent = parent;
    return NS_OK;
}

nsresult
nsFileStream::Close()
{
    nsresult rv = NS_OK;
    if (mFD) {
        if (mCloseFD)
            if (PR_Close(mFD) == PR_FAILURE)
                rv = NS_BASE_STREAM_OSERROR;
        mFD = nsnull;
    }
    return rv;
}

NS_IMETHODIMP
nsFileStream::Seek(PRInt32 whence, PRInt64 offset)
{
    if (mFD == nsnull)
        return NS_BASE_STREAM_CLOSED;

    nsInt64 cnt = PR_Seek64(mFD, offset, (PRSeekWhence)whence);
    if (cnt == nsInt64(-1)) {
        return NS_ErrorAccordingToNSPR();
    }
    return NS_OK;
}

NS_IMETHODIMP
nsFileStream::Tell(PRInt64 *result)
{
    if (mFD == nsnull)
        return NS_BASE_STREAM_CLOSED;

    nsInt64 cnt = PR_Seek64(mFD, 0, PR_SEEK_CUR);
    if (cnt == nsInt64(-1)) {
        return NS_ErrorAccordingToNSPR();
    }
    *result = cnt;
    return NS_OK;
}

NS_IMETHODIMP
nsFileStream::SetEOF()
{
    if (mFD == nsnull)
        return NS_BASE_STREAM_CLOSED;

#if defined(XP_UNIX) || defined(XP_OS2) || defined(XP_BEOS)
    
    PRInt64 offset;
    nsresult rv = Tell(&offset);
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




NS_IMPL_ISUPPORTS_INHERITED3(nsFileInputStream, 
                             nsFileStream,
                             nsIInputStream,
                             nsIFileInputStream,
                             nsILineInputStream)

NS_METHOD
nsFileInputStream::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    NS_ENSURE_NO_AGGREGATION(aOuter);

    nsFileInputStream* stream = new nsFileInputStream();
    if (stream == nsnull)
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

    
    nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(aFile, &rv);
    if (NS_FAILED(rv)) return rv;
    if (aIOFlags == -1)
        aIOFlags = PR_RDONLY;
    if (aPerm == -1)
        aPerm = 0;

    PRFileDesc* fd;
    rv = localFile->OpenNSPRFileDesc(aIOFlags, aPerm, &fd);
    if (NS_FAILED(rv)) return rv;

    mFD = fd;

    if (mBehaviorFlags & DELETE_ON_CLOSE) {
        
        
        
        
        
        rv = aFile->Remove(PR_FALSE);
        if (NS_FAILED(rv) && !(mBehaviorFlags & REOPEN_ON_REWIND)) {
            
            mFile = aFile;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsFileInputStream::Init(nsIFile* aFile, PRInt32 aIOFlags, PRInt32 aPerm,
                        PRInt32 aBehaviorFlags)
{
    NS_ENSURE_TRUE(!mFD, NS_ERROR_ALREADY_INITIALIZED);
    NS_ENSURE_TRUE(!mParent, NS_ERROR_ALREADY_INITIALIZED);

    mBehaviorFlags = aBehaviorFlags;

    
    if (mBehaviorFlags & REOPEN_ON_REWIND) {
        mFile = aFile;
        mIOFlags = aIOFlags;
        mPerm = aPerm;
    }

    return Open(aFile, aIOFlags, aPerm);
}

NS_IMETHODIMP
nsFileInputStream::Close()
{
    
    PR_FREEIF(mLineBuffer);
    nsresult rv = nsFileStream::Close();
    if (NS_FAILED(rv)) return rv;
    if (mFile && (mBehaviorFlags & DELETE_ON_CLOSE)) {
        rv = mFile->Remove(PR_FALSE);
        NS_ASSERTION(NS_SUCCEEDED(rv), "failed to delete file");
        
        if (!(mBehaviorFlags & REOPEN_ON_REWIND)) {
          mFile = nsnull;
        }
    }
    return rv;
}

NS_IMETHODIMP
nsFileInputStream::Available(PRUint32* aResult)
{
    if (!mFD) {
        return NS_BASE_STREAM_CLOSED;
    }

    PRInt32 avail = PR_Available(mFD);
    if (avail == -1) {
        return NS_ErrorAccordingToNSPR();
    }
    *aResult = avail;
    return NS_OK;
}

NS_IMETHODIMP
nsFileInputStream::Read(char* aBuf, PRUint32 aCount, PRUint32* aResult)
{
    if (!mFD) {
        *aResult = 0;
        return NS_OK;
    }

    PRInt32 bytesRead = PR_Read(mFD, aBuf, aCount);
    if (bytesRead == -1) {
        return NS_ErrorAccordingToNSPR();
    }
    
    if (mBehaviorFlags & CLOSE_ON_EOF) {
        if (bytesRead == 0) {
            Close();
        }
    }

    *aResult = bytesRead;
    return NS_OK;
}

NS_IMETHODIMP
nsFileInputStream::ReadLine(nsACString& aLine, PRBool* aResult)
{
    if (!mLineBuffer) {
        nsresult rv = NS_InitLineBuffer(&mLineBuffer);
        if (NS_FAILED(rv)) return rv;
    }
    return NS_ReadLine(this, mLineBuffer, aLine, aResult);
}

NS_IMETHODIMP
nsFileInputStream::ReadSegments(nsWriteSegmentFun aWriter, void* aClosure,
                                PRUint32 aCount, PRUint32* aResult)
{
    
    
    
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileInputStream::IsNonBlocking(PRBool *aNonBlocking)
{
    *aNonBlocking = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsFileInputStream::Seek(PRInt32 aWhence, PRInt64 aOffset)
{
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

    return nsFileStream::Seek(aWhence, aOffset);
}




NS_IMPL_ISUPPORTS_INHERITED2(nsFileOutputStream, 
                             nsFileStream,
                             nsIOutputStream,
                             nsIFileOutputStream)
 
NS_METHOD
nsFileOutputStream::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    NS_ENSURE_NO_AGGREGATION(aOuter);

    nsFileOutputStream* stream = new nsFileOutputStream();
    if (stream == nsnull)
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
    NS_ENSURE_TRUE(mFD == nsnull, NS_ERROR_ALREADY_INITIALIZED);

    nsresult rv;
    nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(file, &rv);
    if (NS_FAILED(rv)) return rv;
    if (ioFlags == -1)
        ioFlags = PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE;
    if (perm <= 0)
        perm = 0664;

    PRFileDesc* fd;
    rv = localFile->OpenNSPRFileDesc(ioFlags, perm, &fd);
    if (NS_FAILED(rv)) return rv;

    mFD = fd;
    return NS_OK;
}

NS_IMETHODIMP
nsFileOutputStream::Close()
{
    return nsFileStream::Close();
}

NS_IMETHODIMP
nsFileOutputStream::Write(const char *buf, PRUint32 count, PRUint32 *result)
{
    if (mFD == nsnull)
        return NS_BASE_STREAM_CLOSED;

    PRInt32 cnt = PR_Write(mFD, buf, count);
    if (cnt == -1) {
        return NS_ErrorAccordingToNSPR();
    }
    *result = cnt;
    return NS_OK;
}

NS_IMETHODIMP
nsFileOutputStream::Flush(void)
{
    if (mFD == nsnull)
        return NS_BASE_STREAM_CLOSED;

    PRInt32 cnt = PR_Sync(mFD);
    if (cnt == -1) {
        return NS_ErrorAccordingToNSPR();
    }
    return NS_OK;
}
    
NS_IMETHODIMP
nsFileOutputStream::WriteFrom(nsIInputStream *inStr, PRUint32 count, PRUint32 *_retval)
{
    NS_NOTREACHED("WriteFrom (see source comment)");
    return NS_ERROR_NOT_IMPLEMENTED;
    
    
    
}

NS_IMETHODIMP
nsFileOutputStream::WriteSegments(nsReadSegmentFun reader, void * closure, PRUint32 count, PRUint32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
    
    
    
}

NS_IMETHODIMP
nsFileOutputStream::IsNonBlocking(PRBool *aNonBlocking)
{
    *aNonBlocking = PR_FALSE;
    return NS_OK;
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
    NS_ENSURE_ARG(file);

    nsresult rv = file->Exists(&mTargetFileExists);
    if (NS_FAILED(rv)) {
        NS_ERROR("Can't tell if target file exists");
        mTargetFileExists = PR_TRUE; 
    }

    
    
    
    
    nsCOMPtr<nsIFile> tempResult;
    rv = file->Clone(getter_AddRefs(tempResult));
    if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsILocalFile> tempLocal = do_QueryInterface(tempResult);
        if (tempLocal)
            tempLocal->SetFollowLinks(PR_TRUE);

        
        tempResult->Normalize();
    }

    if (NS_SUCCEEDED(rv) && mTargetFileExists) {
        PRUint32 origPerm;
        if (NS_FAILED(file->GetPermissions(&origPerm))) {
            NS_ERROR("Can't get permissions of target file");
            origPerm = perm;
        }
        
        
        rv = tempResult->CreateUnique(nsIFile::NORMAL_FILE_TYPE, origPerm);
    }
    if (NS_SUCCEEDED(rv)) {
        mTempFile = tempResult;
        mTargetFile = file;
        rv = nsFileOutputStream::Init(mTempFile, ioFlags, perm, behaviorFlags);
    }
    return rv;
}

NS_IMETHODIMP
nsSafeFileOutputStream::Close()
{
    nsresult rv = nsFileOutputStream::Close();

    
    
    if (mTempFile) {
        mTempFile->Remove(PR_FALSE);
        mTempFile = nsnull;
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
            PRBool equal;
            if (NS_FAILED(mTargetFile->Equals(mTempFile, &equal)) || !equal)
                NS_ERROR("mTempFile not equal to mTargetFile");
#endif
        }
        else {
            nsCAutoString targetFilename;
            rv = mTargetFile->GetNativeLeafName(targetFilename);
            if (NS_SUCCEEDED(rv)) {
                
                rv = mTempFile->MoveToNative(nsnull, targetFilename);
                if (NS_FAILED(rv))
                    mTempFile->Remove(PR_FALSE);
            }
        }
    }
    else {
        mTempFile->Remove(PR_FALSE);

        
        if (NS_FAILED(mWriteResult))
            rv = mWriteResult;
    }
    mTempFile = nsnull;
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


