







































#include "nsManifestZIPLoader.h"
#include "nsJAR.h"
#include "mozilla/Omnijar.h"

nsManifestZIPLoader::nsManifestZIPLoader() 
    : mZipReader(new nsJAR())
{
    nsresult rv = mZipReader->Open(mozilla::OmnijarPath());
    if (NS_FAILED(rv))
        mZipReader = NULL;
}

nsManifestZIPLoader::~nsManifestZIPLoader()
{
}

already_AddRefed<nsIInputStream>
nsManifestZIPLoader::LoadEntry(const char* aName)
{
    if (!mZipReader)
        return NULL;

    nsCOMPtr<nsIInputStream> is;
    nsresult rv = mZipReader->GetInputStream(aName, getter_AddRefs(is));
    if (NS_FAILED(rv))
        return NULL;

    return is.forget();
}
