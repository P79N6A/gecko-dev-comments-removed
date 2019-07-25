







































#include "nsXPTZipLoader.h"
#include "nsJAR.h"
#include "nsString.h"
#include "nsStringEnumerator.h"

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

already_AddRefed<nsIZipReader>
nsXPTZipLoader::GetZipReader(nsILocalFile* file)
{
    NS_ASSERTION(file, "bad file");
    
    nsCOMPtr<nsIZipReader> reader = new nsJAR();
    nsresult rv = reader->Open(file);
    if (NS_FAILED(rv))
        return NULL;

    return reader.forget();
}
