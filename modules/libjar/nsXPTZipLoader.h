






































#include "nsISupports.h"
#include "nsIXPTLoader.h"

#include "nsIZipReader.h"

class nsXPTZipLoader : public nsIXPTLoader
{
 public:
    nsXPTZipLoader();
    virtual ~nsXPTZipLoader() {}
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPTLOADER

 private:
    already_AddRefed<nsIZipReader> GetZipReader(nsILocalFile* aFile);
};

