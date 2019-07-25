






































#include "nsISupports.h"
#include "nsIManifestLoader.h"

#include "nsIZipReader.h"

class nsManifestZIPLoader : public nsIManifestLoader
{
 public:
    nsManifestZIPLoader();
    virtual ~nsManifestZIPLoader() {}
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIMANIFESTLOADER

 private:
    already_AddRefed<nsIZipReader> GetZipReader(nsILocalFile* aFile);
};

