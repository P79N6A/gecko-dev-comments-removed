







































#include "nsManifestZIPLoader.h"
#include "nsJAR.h"
#include "nsString.h"
#include "nsStringEnumerator.h"

nsManifestZIPLoader::nsManifestZIPLoader() {
}

NS_IMPL_ISUPPORTS1(nsManifestZIPLoader, nsIManifestLoader)

nsresult
nsManifestZIPLoader::LoadEntry(nsILocalFile* aFile,
                               const char* aName,
                               nsIInputStream** aResult)
{
    nsCOMPtr<nsIZipReader> zip = dont_AddRef(GetZipReader(aFile));

    if (!zip)
        return NS_OK;

    return zip->GetInputStream(aName, aResult);
}

static void
EnumerateEntriesForPattern(nsIZipReader* zip, const char* pattern,
                           nsIManifestLoaderSink* aSink)
{
    nsCOMPtr<nsIUTF8StringEnumerator> entries;
    if (NS_FAILED(zip->FindEntries(pattern, getter_AddRefs(entries))) ||
        !entries) {
        return;
    }

    PRBool hasMore;
    int index = 0;
    while (NS_SUCCEEDED(entries->HasMore(&hasMore)) && hasMore) {
        nsCAutoString itemName;
        if (NS_FAILED(entries->GetNext(itemName)))
            return;

        nsCOMPtr<nsIInputStream> stream;
        if (NS_FAILED(zip->GetInputStream(itemName.get(), getter_AddRefs(stream))))
            continue;

        
        aSink->FoundEntry(itemName.get(), index++, stream);
    }
}
    
nsresult
nsManifestZIPLoader::EnumerateEntries(nsILocalFile* aFile,
                                      nsIManifestLoaderSink* aSink)
{
    nsCOMPtr<nsIZipReader> zip = dont_AddRef(GetZipReader(aFile));

    if (!zip) {
        NS_WARNING("Could not get Zip Reader");
        return NS_OK;
    }

    EnumerateEntriesForPattern(zip, "components/*.manifest$", aSink);
    EnumerateEntriesForPattern(zip, "chrome/*.manifest$", aSink);

    return NS_OK;
}

already_AddRefed<nsIZipReader>
nsManifestZIPLoader::GetZipReader(nsILocalFile* file)
{
    NS_ASSERTION(file, "bad file");
    
    nsCOMPtr<nsIZipReader> reader = new nsJAR();
    nsresult rv = reader->Open(file);
    if (NS_FAILED(rv))
        return NULL;

    return reader.forget();
}
