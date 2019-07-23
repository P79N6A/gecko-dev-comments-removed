







































#include "nsJARInputStream.h"
#include "zipstruct.h"         
#include "nsZipArchive.h"

#include "nsNetUtil.h"
#include "nsEscape.h"
#include "nsIFile.h"





NS_IMPL_THREADSAFE_ISUPPORTS1(nsJARInputStream, nsIInputStream)





nsresult
nsJARInputStream::InitFile(nsZipArchive* aZip, nsZipItem *item, PRFileDesc *fd)
{
    nsresult rv;

    
    mFd = fd;
      
    NS_ENSURE_ARG_POINTER(aZip);
    NS_ENSURE_ARG_POINTER(item);
    NS_ENSURE_ARG_POINTER(fd);

    
    mClosed = PR_TRUE;

    
    mInSize = item->size;
 
    
    switch (item->compression) {
       case STORED: 
           break;

       case DEFLATED:
           mInflate = (InflateStruct *) PR_Malloc(sizeof(InflateStruct));
           NS_ENSURE_TRUE(mInflate, NS_ERROR_OUT_OF_MEMORY);
    
           rv = gZlibInit(&(mInflate->mZs));
           NS_ENSURE_SUCCESS(rv, NS_ERROR_OUT_OF_MEMORY);
    
           mInflate->mOutSize = item->realsize;
           mInflate->mInCrc = item->crc32;
           mInflate->mOutCrc = crc32(0L, Z_NULL, 0);
           break;

       default:
           return NS_ERROR_NOT_IMPLEMENTED;
    }
   
    
    rv = aZip->SeekToItem(item, mFd);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FILE_CORRUPTED);
        
    
    mClosed = PR_FALSE;
    mCurPos = 0;
    return NS_OK;
}

nsresult
nsJARInputStream::InitDirectory(nsZipArchive* aZip,
                                const nsACString& aJarDirSpec,
                                const char* aDir)
{
    NS_ENSURE_ARG_POINTER(aZip);
    NS_ENSURE_ARG_POINTER(aDir);

    
    mClosed = PR_TRUE;
    mDirectory = PR_TRUE;
    
    
    mZip = aZip;
    nsZipFind *find;
    nsresult rv;
    
    
    
    
    
    nsDependentCString dirName(aDir);
    mNameLen = dirName.Length();

    
    
    
    nsCAutoString escDirName;
    const char* curr = dirName.BeginReading();
    const char* end  = dirName.EndReading();
    while (curr != end) {
        switch (*curr) {
            case '*':
            case '?':
            case '$':
            case '[':
            case ']':
            case '^':
            case '~':
            case '(':
            case ')':
            case '\\':
                escDirName.Append('\\');
                
            default:
                escDirName.Append(*curr);
        }
        ++curr;
    }
    nsCAutoString pattern = escDirName + NS_LITERAL_CSTRING("?*~") +
                            escDirName + NS_LITERAL_CSTRING("?*/?*");
    rv = aZip->FindInit(pattern.get(), &find);
    if (NS_FAILED(rv)) return rv;

    const char *name;
    while ((rv = find->FindNext( &name )) == NS_OK) {
        
        mArray.AppendElement(nsDependentCString(name));
    }
    delete find;

    if (rv != NS_ERROR_FILE_TARGET_DOES_NOT_EXIST && NS_FAILED(rv)) {
        return NS_ERROR_FAILURE;    
    }

    
    mArray.Sort();

    mBuffer.AssignLiteral("300: ");
    mBuffer.Append(aJarDirSpec);
    mBuffer.AppendLiteral("\n200: filename content-length last-modified file-type\n");

    
    mClosed = PR_FALSE;
    mCurPos = 0;
    mArrPos = 0;
    return NS_OK;
}

NS_IMETHODIMP 
nsJARInputStream::Available(PRUint32 *_retval)
{
    if (mClosed)
        return NS_BASE_STREAM_CLOSED;

    if (mDirectory)
        *_retval = mBuffer.Length();
    else if (mInflate) 
        *_retval = mInflate->mOutSize - mInflate->mZs.total_out;
    else 
        *_retval = mInSize - mCurPos;
    return NS_OK;
}

NS_IMETHODIMP
nsJARInputStream::Read(char* aBuffer, PRUint32 aCount, PRUint32 *aBytesRead)
{
    NS_ENSURE_ARG_POINTER(aBuffer);
    NS_ENSURE_ARG_POINTER(aBytesRead);

    *aBytesRead = 0;

    nsresult rv = NS_OK;
    if (mClosed)
        return rv;

    if (mDirectory) {
        rv = ReadDirectory(aBuffer, aCount, aBytesRead);
    } else {
        if (mInflate) {
            rv = ContinueInflate(aBuffer, aCount, aBytesRead);
        } else {
            PRInt32 bytesRead = 0;
            aCount = PR_MIN(aCount, mInSize - mCurPos);
            if (aCount) {
                bytesRead = PR_Read(mFd, aBuffer, aCount);
                if (bytesRead < 0)
                    return NS_ERROR_FILE_CORRUPTED;
                mCurPos += bytesRead;
                if (bytesRead != aCount) {
                    
                    PR_Close(mFd);
                    mFd = nsnull;
                    return NS_ERROR_FILE_CORRUPTED;
                }
            }
            *aBytesRead = bytesRead;
        }

        
        
        
        
        if (mCurPos >= mInSize && mFd) {
            PR_Close(mFd);
            mFd = nsnull;
        }
    }
    return rv;
}

NS_IMETHODIMP
nsJARInputStream::ReadSegments(nsWriteSegmentFun writer, void * closure, PRUint32 count, PRUint32 *_retval)
{
    
    NS_NOTREACHED("Consumers should be using Read()!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsJARInputStream::IsNonBlocking(PRBool *aNonBlocking)
{
    *aNonBlocking = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsJARInputStream::Close()
{
    PR_FREEIF(mInflate);
    if (mFd) {
        PR_Close(mFd);
        mFd = nsnull;
    }
    mClosed = PR_TRUE;
    return NS_OK;
}

nsresult 
nsJARInputStream::ContinueInflate(char* aBuffer, PRUint32 aCount,
                                  PRUint32* aBytesRead)
{
    
    NS_ASSERTION(mInflate,"inflate data structure missing");
    NS_ASSERTION(aBuffer,"aBuffer parameter must not be null");
    NS_ASSERTION(aBytesRead,"aBytesRead parameter must not be null");

    
    const PRUint32 oldTotalOut = mInflate->mZs.total_out;
    
    
    mInflate->mZs.avail_out = (mInflate->mOutSize-oldTotalOut > aCount) ? aCount : mInflate->mOutSize-oldTotalOut;
    mInflate->mZs.next_out = (unsigned char*)aBuffer;

    int zerr = Z_OK;
    
    while (mInflate->mZs.avail_out > 0 && zerr == Z_OK) {

        if (mInflate->mZs.avail_in == 0 && mCurPos < mInSize) {
            
            PRUint32 bytesToRead = PR_MIN(mInSize - mCurPos, ZIP_BUFLEN);

            NS_ASSERTION(mFd, "File handle missing");
            PRInt32 bytesRead = PR_Read(mFd, mInflate->mReadBuf, bytesToRead);
            if (bytesRead < 0) {
                zerr = Z_ERRNO;
                break;
            }
            mCurPos += bytesRead;

            
            mInflate->mZs.next_in = mInflate->mReadBuf;
            mInflate->mZs.avail_in = bytesRead;
        }

        
        zerr = inflate(&(mInflate->mZs), Z_SYNC_FLUSH);
    }

    if ((zerr != Z_OK) && (zerr != Z_STREAM_END))
        return NS_ERROR_FILE_CORRUPTED;

    *aBytesRead = (mInflate->mZs.total_out - oldTotalOut);

    
    mInflate->mOutCrc = crc32(mInflate->mOutCrc, (unsigned char*)aBuffer, *aBytesRead);

    
    
    if (zerr == Z_STREAM_END || mInflate->mZs.total_out == mInflate->mOutSize) {
        inflateEnd(&(mInflate->mZs));

        
        if (mInflate->mOutCrc != mInflate->mInCrc) {
            
            
            NS_NOTREACHED(0);
            return NS_ERROR_FILE_CORRUPTED;
        }
    }

    return NS_OK;
}

nsresult
nsJARInputStream::ReadDirectory(char* aBuffer, PRUint32 aCount, PRUint32 *aBytesRead)
{
    
    NS_ASSERTION(aBuffer,"aBuffer parameter must not be null");
    NS_ASSERTION(aBytesRead,"aBytesRead parameter must not be null");

    
    PRUint32 numRead = CopyDataToBuffer(aBuffer, aCount);

    if (aCount > 0) {
        
        mBuffer.Truncate();
        mCurPos = 0;
        const PRUint32 arrayLen = mArray.Length();

        for ( ;aCount > mBuffer.Length(); mArrPos++) {
            
            if (arrayLen <= mArrPos)
                break;

            const char * entryName = mArray[mArrPos].get();
            PRUint32 entryNameLen = mArray[mArrPos].Length();
            nsZipItem* ze = mZip->GetItem(entryName);
            NS_ENSURE_TRUE(ze, NS_ERROR_FILE_TARGET_DOES_NOT_EXIST);

            
            PRExplodedTime tm;
            PR_ExplodeTime(GetModTime(ze->date, ze->time), PR_GMTParameters, &tm);
            char itemLastModTime[65];
            PR_FormatTimeUSEnglish(itemLastModTime,
                                   sizeof(itemLastModTime),
                                   " %a,%%20%d%%20%b%%20%Y%%20%H:%M:%S%%20GMT ",
                                   &tm);

            
            
            mBuffer.AppendLiteral("201: ");

            
            
            
            NS_EscapeURL(entryName + mNameLen,
                         entryNameLen - mNameLen, 
                         esc_Minimal | esc_AlwaysCopy,
                         mBuffer);

            mBuffer.Append(' ');
            mBuffer.AppendInt(ze->realsize, 10);
            mBuffer.Append(itemLastModTime); 
            if (ze->isDirectory) 
                mBuffer.AppendLiteral("DIRECTORY\n");
            else
                mBuffer.AppendLiteral("FILE\n");
        }

        
        numRead += CopyDataToBuffer(aBuffer, aCount);
    }

    *aBytesRead = numRead;
    return NS_OK;
}

PRUint32
nsJARInputStream::CopyDataToBuffer(char* &aBuffer, PRUint32 &aCount)
{
    const PRUint32 writeLength = PR_MIN(aCount, mBuffer.Length() - mCurPos);

    if (writeLength > 0) {
        memcpy(aBuffer, mBuffer.get() + mCurPos, writeLength);
        mCurPos += writeLength;
        aCount  -= writeLength;
        aBuffer += writeLength;
    }

    
    
    return writeLength;
}
