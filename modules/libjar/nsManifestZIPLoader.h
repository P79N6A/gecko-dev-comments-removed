






































#include "nsISupports.h"
#include "nsIManifestLoader.h"

#include "nsIZipReader.h"

class nsManifestZIPLoader
{
 public:
    nsManifestZIPLoader();
    ~nsManifestZIPLoader();

    already_AddRefed<nsIInputStream> LoadEntry(const char* name);

 private:
    nsCOMPtr<nsIZipReader> mZipReader;
};

