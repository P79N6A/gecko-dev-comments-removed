








































#include "nsJARInputStream.h"
#include "zipstruct.h"         
#include "nsZipArchive.h"

#include "nsNetUtil.h"
#include "nsEscape.h"
#include "nsIFile.h"
#include "nsDebug.h"





NS_IMPL_THREADSAFE_ISUPPORTS1(nsJARInputStream, nsIInputStream)





nsresult
nsJARInputStream::InitFile(nsJAR *aJar, nsZipItem *item)
{
    nsresult rv = NS_OK;
    NS_ABORT_IF_FALSE(aJar, "Argument may not be null");
    NS_ABORT_IF_FALSE(item, "Argument may not be null");

    
    mMode = MODE_CLOSED;
    
    switch (item->compression) {
       case STORED: 
           mMode = MODE_COPY;
           break;

       case DEFLATED:
           rv = gZlibInit(&mZs);
           NS_ENSURE_SUCCESS(rv, rv);
    
           mMode = MODE_INFLATE;
           mOutSize = item->realsize;
           mInCrc = item->crc32;
           mOutCrc = crc32(0L, Z_NULL, 0);
           break;

       default:
           return NS_ERROR_NOT_IMPLEMENTED;
    }
   
    
    mFd = aJar->mZip.GetFD();
    mZs.next_in = aJar->mZip.GetData(item);
    mZs.avail_in = item->size;
    mOutSize = item->realsize;
    mZs.total_out = 0;
    return NS_OK;
}

nsresult
nsJARInputStream::InitDirectory(nsJAR* aJar,
                                const nsACString& aJarDirSpec,
                                const char* aDir)
{
    NS_ABORT_IF_FALSE(aJar, "Argument may not be null");
    NS_ABORT_IF_FALSE(aDir, "Argument may not be null");

    
    mMode = MODE_CLOSED;
    
    
    mJar = aJar;
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
    rv = mJar->mZip.FindInit(pattern.get(), &find);
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

    
    mMode = MODE_DIRECTORY;
    mZs.total_out = 0;
    mArrPos = 0;
    return NS_OK;
}

NS_IMETHODIMP 
nsJARInputStream::Available(PRUint32 *_retval)
{
    
    
    *_retval = 0;

    switch (mMode) {
      case MODE_NOTINITED:
        break;

      case MODE_CLOSED:
        return NS_BASE_STREAM_CLOSED;

      case MODE_DIRECTORY:
        *_retval = mBuffer.Length();
        break;

      case MODE_INFLATE:
      case MODE_COPY:
        *_retval = mOutSize - mZs.total_out;
        break;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsJARInputStream::Read(char* aBuffer, PRUint32 aCount, PRUint32 *aBytesRead)
{
    NS_ENSURE_ARG_POINTER(aBuffer);
    NS_ENSURE_ARG_POINTER(aBytesRead);

    *aBytesRead = 0;

    nsresult rv = NS_OK;
    switch (mMode) {
      case MODE_NOTINITED:
        return NS_OK;

      case MODE_CLOSED:
        return NS_BASE_STREAM_CLOSED;

      case MODE_DIRECTORY:
        return ReadDirectory(aBuffer, aCount, aBytesRead);

      case MODE_INFLATE:
        if (mFd) {
          rv = ContinueInflate(aBuffer, aCount, aBytesRead);
        }
        
        
        
        if (mZs.avail_in == 0) {
            mFd = nsnull;
        }
        break;

      case MODE_COPY:
        if (mFd) {
          PRUint32 count = PR_MIN(aCount, mOutSize - mZs.total_out);
          if (count) {
              memcpy(aBuffer, mZs.next_in + mZs.total_out, count);
              mZs.total_out += count;
          }
          *aBytesRead = count;
        }
        
        
        if (mZs.total_out >= mOutSize) {
            mFd = nsnull;
        }
        break;
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
    mMode = MODE_CLOSED;
    mFd = nsnull;
    return NS_OK;
}

nsresult 
nsJARInputStream::ContinueInflate(char* aBuffer, PRUint32 aCount,
                                  PRUint32* aBytesRead)
{
    
    NS_ASSERTION(aBuffer,"aBuffer parameter must not be null");
    NS_ASSERTION(aBytesRead,"aBytesRead parameter must not be null");

    
    const PRUint32 oldTotalOut = mZs.total_out;
    
    
    mZs.avail_out = PR_MIN(aCount, (mOutSize-oldTotalOut));
    mZs.next_out = (unsigned char*)aBuffer;

    
    int zerr = inflate(&mZs, Z_SYNC_FLUSH);
    if ((zerr != Z_OK) && (zerr != Z_STREAM_END))
        return NS_ERROR_FILE_CORRUPTED;

    *aBytesRead = (mZs.total_out - oldTotalOut);

    
    mOutCrc = crc32(mOutCrc, (unsigned char*)aBuffer, *aBytesRead);

    
    
    if (zerr == Z_STREAM_END || mZs.total_out == mOutSize) {
        inflateEnd(&mZs);

        
        if (mOutCrc != mInCrc) {
            
            
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
            nsZipItem* ze = mJar->mZip.GetItem(entryName);
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
