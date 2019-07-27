















#include "nsEscape.h"
#include "nsDirectoryIndexStream.h"
#include "prlog.h"
#include "prtime.h"
#ifdef PR_LOGGING
static PRLogModuleInfo* gLog;
#endif

#include "nsISimpleEnumerator.h"
#ifdef THREADSAFE_I18N
#include "nsCollationCID.h"
#include "nsICollation.h"
#include "nsILocale.h"
#include "nsILocaleService.h"
#endif
#include "nsIFile.h"
#include "nsURLHelper.h"
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

static int compare(nsIFile* aElement1, nsIFile* aElement2, void* aData)
{
    if (!NS_IsNativeUTF8()) {
        
        nsAutoString name1, name2;
        aElement1->GetLeafName(name1);
        aElement2->GetLeafName(name2);

        
        
        
        
        
        
        
        
        
        
        
        
        return Compare(name1, name2);
    }

    nsAutoCString name1, name2;
    aElement1->GetNativeLeafName(name1);
    aElement2->GetNativeLeafName(name2);

    return Compare(name1, name2);
}

nsresult
nsDirectoryIndexStream::Init(nsIFile* aDir)
{
    nsresult rv;
    bool isDir;
    rv = aDir->IsDirectory(&isDir);
    if (NS_FAILED(rv)) return rv;
    NS_PRECONDITION(isDir, "not a directory");
    if (!isDir)
        return NS_ERROR_ILLEGAL_VALUE;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gLog, PR_LOG_DEBUG)) {
        nsAutoCString path;
        aDir->GetNativePath(path);
        PR_LOG(gLog, PR_LOG_DEBUG,
               ("nsDirectoryIndexStream[%p]: initialized on %s",
                this, path.get()));
    }
#endif

    
    
    nsCOMPtr<nsISimpleEnumerator> iter;
    rv = aDir->GetDirectoryEntries(getter_AddRefs(iter));
    if (NS_FAILED(rv)) return rv;

    
    
    

    bool more;
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
    mArray.Sort(compare, nullptr);
#endif

    mBuf.AppendLiteral("300: ");
    nsAutoCString url;
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
    nsRefPtr<nsDirectoryIndexStream> result = new nsDirectoryIndexStream();
    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = result->Init(aDir);
    if (NS_FAILED(rv)) {
        return rv;
    }

    result.forget(aResult);
    return NS_OK;
}

NS_IMPL_ISUPPORTS(nsDirectoryIndexStream, nsIInputStream)


NS_IMETHODIMP
nsDirectoryIndexStream::Close()
{
    mStatus = NS_BASE_STREAM_CLOSED;
    return NS_OK;
}

NS_IMETHODIMP
nsDirectoryIndexStream::Available(uint64_t* aLength)
{
    if (NS_FAILED(mStatus))
        return mStatus;

    
    if (mOffset < (int32_t)mBuf.Length()) {
        *aLength = mBuf.Length() - mOffset;
        return NS_OK;
    }

    
    *aLength = (mPos < mArray.Count()) ? 1 : 0;
    return NS_OK;
}

NS_IMETHODIMP
nsDirectoryIndexStream::Read(char* aBuf, uint32_t aCount, uint32_t* aReadCount)
{
    if (mStatus == NS_BASE_STREAM_CLOSED) {
        *aReadCount = 0;
        return NS_OK;
    }
    if (NS_FAILED(mStatus))
        return mStatus;

    uint32_t nread = 0;

    
    
    while (mOffset < (int32_t)mBuf.Length() && aCount != 0) {
        *(aBuf++) = char(mBuf.CharAt(mOffset++));
        --aCount;
        ++nread;
    }

    
    if (aCount > 0) {
        mOffset = 0;
        mBuf.Truncate();

        
        while (uint32_t(mBuf.Length()) < aCount) {
            bool more = mPos < mArray.Count();
            if (!more) break;

            
            
            nsIFile* current = mArray.ObjectAt(mPos);
            ++mPos;

#ifdef PR_LOGGING
            if (PR_LOG_TEST(gLog, PR_LOG_DEBUG)) {
                nsAutoCString path;
                current->GetNativePath(path);
                PR_LOG(gLog, PR_LOG_DEBUG,
                       ("nsDirectoryIndexStream[%p]: iterated %s",
                        this, path.get()));
            }
#endif

            
            
            nsresult rv;
#ifndef XP_UNIX
            bool hidden = false;
            current->IsHidden(&hidden);
            if (hidden) {
                PR_LOG(gLog, PR_LOG_DEBUG,
                       ("nsDirectoryIndexStream[%p]: skipping hidden file/directory",
                        this));
                continue;
            }
#endif

            int64_t fileSize = 0;
            current->GetFileSize( &fileSize );

            PRTime fileInfoModifyTime = 0;
            current->GetLastModifiedTime( &fileInfoModifyTime );
            fileInfoModifyTime *= PR_USEC_PER_MSEC;

            mBuf.AppendLiteral("201: ");

            
            char* escaped = nullptr;
            if (!NS_IsNativeUTF8()) {
                nsAutoString leafname;
                rv = current->GetLeafName(leafname);
                if (NS_FAILED(rv)) return rv;
                if (!leafname.IsEmpty())
                    escaped = nsEscape(NS_ConvertUTF16toUTF8(leafname).get(), url_Path);
            } else {
                nsAutoCString leafname;
                rv = current->GetNativeLeafName(leafname);
                if (NS_FAILED(rv)) return rv;
                if (!leafname.IsEmpty())
                    escaped = nsEscape(leafname.get(), url_Path);
            }
            if (escaped) {
                mBuf += escaped;
                mBuf.Append(' ');
                free(escaped);
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

            
            bool isFile = true;
            current->IsFile(&isFile);
            if (isFile) {
                mBuf.AppendLiteral("FILE ");
            }
            else {
                bool isDir;
                rv = current->IsDirectory(&isDir);
                if (NS_FAILED(rv)) return rv; 
                if (isDir) {
                    mBuf.AppendLiteral("DIRECTORY ");
                }
                else {
                    bool isLink;
                    rv = current->IsSymlink(&isLink);
                    if (NS_FAILED(rv)) return rv; 
                    if (isLink) {
                        mBuf.AppendLiteral("SYMBOLIC-LINK ");
                    }
                }
            }

            mBuf.Append('\n');
        }

        
        
        while (mOffset < (int32_t)mBuf.Length() && aCount != 0) {
            *(aBuf++) = char(mBuf.CharAt(mOffset++));
            --aCount;
            ++nread;
        }
    }

    *aReadCount = nread;
    return NS_OK;
}

NS_IMETHODIMP
nsDirectoryIndexStream::ReadSegments(nsWriteSegmentFun writer, void * closure, uint32_t count, uint32_t *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDirectoryIndexStream::IsNonBlocking(bool *aNonBlocking)
{
    *aNonBlocking = false;
    return NS_OK;
}
