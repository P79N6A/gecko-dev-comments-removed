




































#include "nsDownloader.h"
#include "nsICachingChannel.h"
#include "nsIInputStream.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsNetUtil.h"




#include <stdlib.h>
#define TABLE_SIZE 36
static const char table[] =
    { 'a','b','c','d','e','f','g','h','i','j',
      'k','l','m','n','o','p','q','r','s','t',
      'u','v','w','x','y','z','0','1','2','3',
      '4','5','6','7','8','9' };
static void
MakeRandomString(char *buf, PRInt32 bufLen)
{
    
    
    double fpTime;
    LL_L2D(fpTime, PR_Now());
    srand((uint)(fpTime * 1e-6 + 0.5));   

    PRInt32 i;
    for (i=0;i<bufLen;i++) {
        *buf++ = table[rand()%TABLE_SIZE];
    }
    *buf = 0;
}


nsDownloader::~nsDownloader()
{
    if (mLocation && mLocationIsTemp) {
        
        
        
        mSink = 0;

        nsresult rv = mLocation->Remove(PR_FALSE);
        if (NS_FAILED(rv))
            NS_ERROR("unable to remove temp file");
    }
}

NS_IMPL_ISUPPORTS3(nsDownloader,
                   nsIDownloader,
                   nsIStreamListener,
                   nsIRequestObserver)

NS_IMETHODIMP
nsDownloader::Init(nsIDownloadObserver *observer, nsIFile *location)
{
    mObserver = observer;
    mLocation = location;
    return NS_OK;
}

NS_IMETHODIMP 
nsDownloader::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
    nsresult rv = NS_ERROR_FAILURE;
    if (!mLocation) {
        nsCOMPtr<nsICachingChannel> caching = do_QueryInterface(request, &rv);
        if (NS_SUCCEEDED(rv))
            rv = caching->SetCacheAsFile(PR_TRUE);
    }
    if (NS_FAILED(rv)) {
        
        
        if (!mLocation) {
            rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(mLocation));
            if (NS_FAILED(rv)) return rv;

            char buf[13];
            MakeRandomString(buf, 8);
            memcpy(buf+8, ".tmp", 5);
            rv = mLocation->AppendNative(nsDependentCString(buf, 12));
            if (NS_FAILED(rv)) return rv;

            rv = mLocation->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
            if (NS_FAILED(rv)) return rv;

            mLocationIsTemp = PR_TRUE;
        }
        
        rv = NS_NewLocalFileOutputStream(getter_AddRefs(mSink), mLocation);
        if (NS_FAILED(rv)) return rv;

        
        
        
    }
    return rv;
}

NS_IMETHODIMP 
nsDownloader::OnStopRequest(nsIRequest  *request,
                            nsISupports *ctxt,
                            nsresult     status)
{
    if (!mSink && NS_SUCCEEDED(status)) {
        nsCOMPtr<nsICachingChannel> caching = do_QueryInterface(request, &status);
        if (NS_SUCCEEDED(status)) {
            status = caching->GetCacheFile(getter_AddRefs(mLocation));
            if (NS_SUCCEEDED(status)) {
                NS_ASSERTION(mLocation, "success without a cache file");
                
                
                
                caching->GetCacheToken(getter_AddRefs(mCacheToken));
            }
        }
    }

    mObserver->OnDownloadComplete(this, request, ctxt, status, mLocation);
    mObserver = nsnull;

    return NS_OK;
}

NS_METHOD
nsDownloader::ConsumeData(nsIInputStream* in,
                          void* closure,
                          const char* fromRawSegment,
                          PRUint32 toOffset,
                          PRUint32 count,
                          PRUint32 *writeCount)
{
    nsDownloader *self = (nsDownloader *) closure;
    if (self->mSink)
        return self->mSink->Write(fromRawSegment, count, writeCount);

    *writeCount = count;
    return NS_OK;
}

NS_IMETHODIMP 
nsDownloader::OnDataAvailable(nsIRequest *request, nsISupports *ctxt, 
                              nsIInputStream *inStr, 
                              PRUint32 sourceOffset, PRUint32 count)
{
    PRUint32 n;  
    return inStr->ReadSegments(ConsumeData, this, count, &n);
}
