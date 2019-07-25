






































#include "nsCOMPtr.h"

#include "nsIZipReader.h"
#include "nsIInputStream.h"

class nsManifestZIPLoader
{
 public:
    nsManifestZIPLoader();
    ~nsManifestZIPLoader();

    already_AddRefed<nsIInputStream> LoadEntry(const char* name);

 private:
    nsCOMPtr<nsIZipReader> mZipReader;
};

