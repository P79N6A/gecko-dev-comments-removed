







































#include "nsXPTZipLoader.h"
#include "nsIZipReader.h"
#include "nsXPIDLString.h"
#include "nsString.h"
#include "nsStringEnumerator.h"

static const char gCacheContractID[] =
  "@mozilla.org/libjar/zip-reader-cache;1";
static const PRUint32 gCacheSize = 1;

nsXPTZipLoader::nsXPTZipLoader() {
}

NS_IMPL_ISUPPORTS1(nsXPTZipLoader, nsIXPTLoader)

nsresult
nsXPTZipLoader::LoadEntry(nsILocalFile* aFile,
                          const char* aName,
                          nsIInputStream** aResult)
{
    nsCOMPtr<nsIZipReader> zip = dont_AddRef(GetZipReader(aFile));

    if (!zip)
        return NS_OK;

    return zip->GetInputStream(aName, aResult);
}
    
nsresult
nsXPTZipLoader::EnumerateEntries(nsILocalFile* aFile,
                                 nsIXPTLoaderSink* aSink)
{
    nsCOMPtr<nsIZipReader> zip = dont_AddRef(GetZipReader(aFile));

    if (!zip) {
        NS_WARNING("Could not get Zip Reader");
        return NS_OK;
    }

    nsCOMPtr<nsIUTF8StringEnumerator> entries;
    if (NS_FAILED(zip->FindEntries("*.xpt", getter_AddRefs(entries))) ||
        !entries) {
        
        return NS_OK;
    }

    PRBool hasMore;
    int index = 0;
    while (NS_SUCCEEDED(entries->HasMore(&hasMore)) && hasMore) {
        nsCAutoString itemName;
        if (NS_FAILED(entries->GetNext(itemName)))
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsIInputStream> stream;
        if (NS_FAILED(zip->GetInputStream(itemName.get(), getter_AddRefs(stream))))
            return NS_ERROR_FAILURE;

        
        aSink->FoundEntry(itemName.get(), index++, stream);
    }

    return NS_OK;
}

nsIZipReader*
nsXPTZipLoader::GetZipReader(nsILocalFile* file)
{
    NS_ASSERTION(file, "bad file");
    
    if(!mCache)
    {
        mCache = do_CreateInstance(gCacheContractID);
        if(!mCache || NS_FAILED(mCache->Init(gCacheSize)))
            return nsnull;
    }

    nsIZipReader* reader = nsnull;

    if(NS_FAILED(mCache->GetZip(file, &reader)))
        return nsnull;

    return reader;
}
