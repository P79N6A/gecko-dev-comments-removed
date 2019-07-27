






#include "nsJARInputStream.h"
#include "zipstruct.h"         
#include "nsZipArchive.h"

#include "nsNetUtil.h"
#include "nsEscape.h"
#include "nsIFile.h"
#include "nsDebug.h"
#include <algorithm>
#if defined(XP_WIN)
#include <windows.h>
#endif





NS_IMPL_ISUPPORTS(nsJARInputStream, nsIInputStream)





nsresult
nsJARInputStream::InitFile(nsJAR *aJar, nsZipItem *item)
{
    nsresult rv = NS_OK;
    NS_ABORT_IF_FALSE(aJar, "Argument may not be null");
    NS_ABORT_IF_FALSE(item, "Argument may not be null");

    
    mMode = MODE_CLOSED;
    
    switch (item->Compression()) {
       case STORED: 
           mMode = MODE_COPY;
           break;

       case DEFLATED:
           rv = gZlibInit(&mZs);
           NS_ENSURE_SUCCESS(rv, rv);
    
           mMode = MODE_INFLATE;
           mInCrc = item->CRC32();
           mOutCrc = crc32(0L, Z_NULL, 0);
           break;

       default:
           return NS_ERROR_NOT_IMPLEMENTED;
    }
   
    
    mFd = aJar->mZip->GetFD();
    mZs.next_in = (Bytef *)aJar->mZip->GetData(item);
    if (!mZs.next_in)
        return NS_ERROR_FILE_CORRUPTED;
    mZs.avail_in = item->Size();
    mOutSize = item->RealSize();
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

    
    
    
    nsAutoCString escDirName;
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
    nsAutoCString pattern = escDirName + NS_LITERAL_CSTRING("?*~") +
                            escDirName + NS_LITERAL_CSTRING("?*/?*");
    rv = mJar->mZip->FindInit(pattern.get(), &find);
    if (NS_FAILED(rv)) return rv;

    const char *name;
    uint16_t nameLen;
    while ((rv = find->FindNext( &name, &nameLen )) == NS_OK) {
        
        mArray.AppendElement(nsCString(name,nameLen));
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
nsJARInputStream::Available(uint64_t *_retval)
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
nsJARInputStream::Read(char* aBuffer, uint32_t aCount, uint32_t *aBytesRead)
{
    NS_ENSURE_ARG_POINTER(aBuffer);
    NS_ENSURE_ARG_POINTER(aBytesRead);

    *aBytesRead = 0;

    nsresult rv = NS_OK;
MOZ_WIN_MEM_TRY_BEGIN
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
            mFd = nullptr;
        }
        break;

      case MODE_COPY:
        if (mFd) {
          uint32_t count = std::min(aCount, mOutSize - uint32_t(mZs.total_out));
          if (count) {
              memcpy(aBuffer, mZs.next_in + mZs.total_out, count);
              mZs.total_out += count;
          }
          *aBytesRead = count;
        }
        
        
        if (mZs.total_out >= mOutSize) {
            mFd = nullptr;
        }
        break;
    }
MOZ_WIN_MEM_TRY_CATCH(rv = NS_ERROR_FAILURE)
    return rv;
}

NS_IMETHODIMP
nsJARInputStream::ReadSegments(nsWriteSegmentFun writer, void * closure, uint32_t count, uint32_t *_retval)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsJARInputStream::IsNonBlocking(bool *aNonBlocking)
{
    *aNonBlocking = false;
    return NS_OK;
}

NS_IMETHODIMP
nsJARInputStream::Close()
{
    if (mMode == MODE_INFLATE) {
        inflateEnd(&mZs);
    }
    mMode = MODE_CLOSED;
    mFd = nullptr;
    return NS_OK;
}

nsresult 
nsJARInputStream::ContinueInflate(char* aBuffer, uint32_t aCount,
                                  uint32_t* aBytesRead)
{
    
    NS_ASSERTION(aBuffer,"aBuffer parameter must not be null");
    NS_ASSERTION(aBytesRead,"aBytesRead parameter must not be null");

    
    const uint32_t oldTotalOut = mZs.total_out;
    
    
    mZs.avail_out = std::min(aCount, (mOutSize-oldTotalOut));
    mZs.next_out = (unsigned char*)aBuffer;

    
    int zerr = inflate(&mZs, Z_SYNC_FLUSH);
    if ((zerr != Z_OK) && (zerr != Z_STREAM_END))
        return NS_ERROR_FILE_CORRUPTED;

    *aBytesRead = (mZs.total_out - oldTotalOut);

    
    mOutCrc = crc32(mOutCrc, (unsigned char*)aBuffer, *aBytesRead);

    
    
    if (zerr == Z_STREAM_END || mZs.total_out == mOutSize) {
        inflateEnd(&mZs);

        
        if (mOutCrc != mInCrc)
            return NS_ERROR_FILE_CORRUPTED;
    }

    return NS_OK;
}

nsresult
nsJARInputStream::ReadDirectory(char* aBuffer, uint32_t aCount, uint32_t *aBytesRead)
{
    
    NS_ASSERTION(aBuffer,"aBuffer parameter must not be null");
    NS_ASSERTION(aBytesRead,"aBytesRead parameter must not be null");

    
    uint32_t numRead = CopyDataToBuffer(aBuffer, aCount);

    if (aCount > 0) {
        
        mBuffer.Truncate();
        mCurPos = 0;
        const uint32_t arrayLen = mArray.Length();

        for ( ;aCount > mBuffer.Length(); mArrPos++) {
            
            if (arrayLen <= mArrPos)
                break;

            const char * entryName = mArray[mArrPos].get();
            uint32_t entryNameLen = mArray[mArrPos].Length();
            nsZipItem* ze = mJar->mZip->GetItem(entryName);
            NS_ENSURE_TRUE(ze, NS_ERROR_FILE_TARGET_DOES_NOT_EXIST);

            
            PRExplodedTime tm;
            PR_ExplodeTime(ze->LastModTime(), PR_GMTParameters, &tm);
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
            mBuffer.AppendInt(ze->RealSize(), 10);
            mBuffer.Append(itemLastModTime); 
            if (ze->IsDirectory()) 
                mBuffer.AppendLiteral("DIRECTORY\n");
            else
                mBuffer.AppendLiteral("FILE\n");
        }

        
        numRead += CopyDataToBuffer(aBuffer, aCount);
    }

    *aBytesRead = numRead;
    return NS_OK;
}

uint32_t
nsJARInputStream::CopyDataToBuffer(char* &aBuffer, uint32_t &aCount)
{
    const uint32_t writeLength = std::min(aCount, mBuffer.Length() - mCurPos);

    if (writeLength > 0) {
        memcpy(aBuffer, mBuffer.get() + mCurPos, writeLength);
        mCurPos += writeLength;
        aCount  -= writeLength;
        aBuffer += writeLength;
    }

    
    
    return writeLength;
}
