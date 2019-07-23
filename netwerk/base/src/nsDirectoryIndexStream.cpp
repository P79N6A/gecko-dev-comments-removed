
















































#include "nsEscape.h"
#include "nsDirectoryIndexStream.h"
#include "nsXPIDLString.h"
#include "prio.h"
#include "prlog.h"
#include "prlong.h"
#ifdef PR_LOGGING
static PRLogModuleInfo* gLog;
#endif

#include "nsISimpleEnumerator.h"
#include "nsICollation.h"
#include "nsILocale.h"
#include "nsILocaleService.h"
#include "nsCollationCID.h"
#include "nsIPlatformCharset.h"
#include "nsReadableUtils.h"
#include "nsURLHelper.h"
#include "nsNetUtil.h"
#include "nsCRT.h"
#include "nsNativeCharsetUtils.h"











nsDirectoryIndexStream::nsDirectoryIndexStream()
    : mOffset(0), mStatus(NS_OK), mPos(0)
{
#ifdef PR_LOGGING
    if (! gLog)
        gLog = PR_NewLogModule("nsDirectoryIndexStream");
#endif

    PR_LOG(gLog, PR_LOG_DEBUG,
           ("nsDirectoryIndexStream[%p]: created", this));
}

static int PR_CALLBACK compare(nsIFile* aElement1,
                               nsIFile* aElement2,
                               void* aData)
{
    if (!NS_IsNativeUTF8()) {
        
        nsAutoString name1, name2;
        aElement1->GetLeafName(name1);
        aElement2->GetLeafName(name2);

        
        
        
        
        
        
        
        
        
        
        
        
        return Compare(name1, name2);
    }

    nsCAutoString name1, name2;
    aElement1->GetNativeLeafName(name1);
    aElement2->GetNativeLeafName(name2);

    return Compare(name1, name2);
}

nsresult
nsDirectoryIndexStream::Init(nsIFile* aDir)
{
    nsresult rv;
    PRBool isDir;
    rv = aDir->IsDirectory(&isDir);
    if (NS_FAILED(rv)) return rv;
    NS_PRECONDITION(isDir, "not a directory");
    if (!isDir)
        return NS_ERROR_ILLEGAL_VALUE;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gLog, PR_LOG_DEBUG)) {
        nsCAutoString path;
        aDir->GetNativePath(path);
        PR_LOG(gLog, PR_LOG_DEBUG,
               ("nsDirectoryIndexStream[%p]: initialized on %s",
                this, path.get()));
    }
#endif

    
    
    nsCOMPtr<nsISimpleEnumerator> iter;
    rv = aDir->GetDirectoryEntries(getter_AddRefs(iter));
    if (NS_FAILED(rv)) return rv;

    
    
    

    PRBool more;
    nsCOMPtr<nsISupports> elem;
    while (NS_SUCCEEDED(iter->HasMoreElements(&more)) && more) {
        rv = iter->GetNext(getter_AddRefs(elem));
        if (NS_SUCCEEDED(rv)) {
            nsCOMPtr<nsIFile> file = do_QueryInterface(elem);
            if (file)
                mArray.AppendObject(file); 
        }
    }

#ifdef THREADSAFE_I18N
    nsCOMPtr<nsILocaleService> ls = do_GetService(NS_LOCALESERVICE_CONTRACTID,
                                                  &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsILocale> locale;
    rv = ls->GetApplicationLocale(getter_AddRefs(locale));
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsICollationFactory> cf = do_CreateInstance(NS_COLLATIONFACTORY_CONTRACTID,
                                                         &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsICollation> coll;
    rv = cf->CreateCollation(locale, getter_AddRefs(coll));
    if (NS_FAILED(rv)) return rv;

    mArray.Sort(compare, coll);
#else
    mArray.Sort(compare, nsnull);
#endif

    mBuf.AppendLiteral("300: ");
    nsCAutoString url;
    rv = net_GetURLSpecFromFile(aDir, url);
    if (NS_FAILED(rv)) return rv;
    mBuf.Append(url);
    mBuf.Append('\n');

    mBuf.AppendLiteral("200: filename content-length last-modified file-type\n");

    return NS_OK;
}

nsDirectoryIndexStream::~nsDirectoryIndexStream()
{
    PR_LOG(gLog, PR_LOG_DEBUG,
           ("nsDirectoryIndexStream[%p]: destroyed", this));
}

nsresult
nsDirectoryIndexStream::Create(nsIFile* aDir, nsIInputStream** aResult)
{
    nsDirectoryIndexStream* result = new nsDirectoryIndexStream();
    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv;
    rv = result->Init(aDir);
    if (NS_FAILED(rv)) {
        delete result;
        return rv;
    }

    *aResult = result;
    NS_ADDREF(*aResult);
    return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDirectoryIndexStream, nsIInputStream)


NS_IMETHODIMP
nsDirectoryIndexStream::Close()
{
    mStatus = NS_BASE_STREAM_CLOSED;
    return NS_OK;
}

NS_IMETHODIMP
nsDirectoryIndexStream::Available(PRUint32* aLength)
{
    if (NS_FAILED(mStatus))
        return mStatus;

    
    if (mOffset < (PRInt32)mBuf.Length()) {
        *aLength = mBuf.Length() - mOffset;
        return NS_OK;
    }

    
    *aLength = (mPos < mArray.Count()) ? 1 : 0;
    return NS_OK;
}

NS_IMETHODIMP
nsDirectoryIndexStream::Read(char* aBuf, PRUint32 aCount, PRUint32* aReadCount)
{
    if (mStatus == NS_BASE_STREAM_CLOSED) {
        *aReadCount = 0;
        return NS_OK;
    }
    if (NS_FAILED(mStatus))
        return mStatus;

    PRUint32 nread = 0;

    
    
    while (mOffset < (PRInt32)mBuf.Length() && aCount != 0) {
        *(aBuf++) = char(mBuf.CharAt(mOffset++));
        --aCount;
        ++nread;
    }

    
    if (aCount > 0) {
        mOffset = 0;
        mBuf.Truncate();

        
        while (PRUint32(mBuf.Length()) < aCount) {
            PRBool more = mPos < mArray.Count();
            if (!more) break;

            
            
            nsIFile* current = mArray.ObjectAt(mPos);
            ++mPos;

#ifdef PR_LOGGING
            if (PR_LOG_TEST(gLog, PR_LOG_DEBUG)) {
                nsCAutoString path;
                current->GetNativePath(path);
                PR_LOG(gLog, PR_LOG_DEBUG,
                       ("nsDirectoryIndexStream[%p]: iterated %s",
                        this, path.get()));
            }
#endif

            
            
            nsresult rv;
#ifndef XP_UNIX
            PRBool hidden = PR_FALSE;
            current->IsHidden(&hidden);
            if (hidden) {
                PR_LOG(gLog, PR_LOG_DEBUG,
                       ("nsDirectoryIndexStream[%p]: skipping hidden file/directory",
                        this));
                continue;
            }
#endif

            PRInt64 fileSize = 0;
            current->GetFileSize( &fileSize );

            PRInt64 fileInfoModifyTime = 0;
            current->GetLastModifiedTime( &fileInfoModifyTime );
            fileInfoModifyTime *= PR_USEC_PER_MSEC;

            mBuf.AppendLiteral("201: ");

            
            char* escaped = nsnull;
            if (!NS_IsNativeUTF8()) {
                nsAutoString leafname;
                rv = current->GetLeafName(leafname);
                if (NS_FAILED(rv)) return rv;
                if (!leafname.IsEmpty())
                    escaped = nsEscape(NS_ConvertUTF16toUTF8(leafname).get(), url_Path);
            } else {
                nsCAutoString leafname;
                rv = current->GetNativeLeafName(leafname);
                if (NS_FAILED(rv)) return rv;
                if (!leafname.IsEmpty())
                    escaped = nsEscape(leafname.get(), url_Path);
            }
            if (escaped) {
                mBuf += escaped;
                mBuf.Append(' ');
                nsMemory::Free(escaped);
            }

            
            mBuf.AppendInt(fileSize, 10);
            mBuf.Append(' ');

            
            PRExplodedTime tm;
            PR_ExplodeTime(fileInfoModifyTime, PR_GMTParameters, &tm);
            {
                char buf[64];
                PR_FormatTimeUSEnglish(buf, sizeof(buf), "%a,%%20%d%%20%b%%20%Y%%20%H:%M:%S%%20GMT ", &tm);
                mBuf.Append(buf);
            }

            
            PRBool isFile = PR_TRUE;
            current->IsFile(&isFile);
            if (isFile) {
                mBuf.AppendLiteral("FILE ");
            }
            else {
                PRBool isDir;
                rv = current->IsDirectory(&isDir);
                if (NS_FAILED(rv)) return rv; 
                if (isDir) {
                    mBuf.AppendLiteral("DIRECTORY ");
                }
                else {
                    PRBool isLink;
                    rv = current->IsSymlink(&isLink);
                    if (NS_FAILED(rv)) return rv; 
                    if (isLink) {
                        mBuf.AppendLiteral("SYMBOLIC-LINK ");
                    }
                }
            }

            mBuf.Append('\n');
        }

        
        
        while (mOffset < (PRInt32)mBuf.Length() && aCount != 0) {
            *(aBuf++) = char(mBuf.CharAt(mOffset++));
            --aCount;
            ++nread;
        }
    }

    *aReadCount = nread;
    return NS_OK;
}

NS_IMETHODIMP
nsDirectoryIndexStream::ReadSegments(nsWriteSegmentFun writer, void * closure, PRUint32 count, PRUint32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDirectoryIndexStream::IsNonBlocking(PRBool *aNonBlocking)
{
    *aNonBlocking = PR_FALSE;
    return NS_OK;
}
