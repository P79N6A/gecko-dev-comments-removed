







































#include "nsManifestZIPLoader.h"
#include "nsJAR.h"
#include "mozilla/Omnijar.h"

nsManifestZIPLoader::nsManifestZIPLoader() 
    : mZipReader(new nsJAR())
{
    nsresult rv = reader->Open(mozilla::OmnijarPath());
    if (NS_FAILED(rv))
        mZipReader = NULL;
}

already_AddRefed<nsIInputStream>
nsManifestZIPLoader::LoadEntry(const char* aName)
{
    if (!mZipReader)
        return NS_ERROR_NOT_INITIALIZED;

    nsCOMPtr<nsIInputStream> is;
    nsresult rv = zip->GetInputStream(aName, getter_AddRefs(is));
    if (NS_FAILED(rv))
        return NULL;

    return is.forget();
}
