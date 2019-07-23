






































#include "nsISupports.h"
#include "nsIXPTLoader.h"

#include "nsIZipReader.h"


#define NS_XPTZIPREADER_CID \
  { 0x320e073, 0x79c7, 0x4dae, \
      { 0x80, 0x55, 0x81, 0xbe, 0xd8, 0xb8, 0xdb, 0x96 } }


class nsXPTZipLoader : public nsIXPTLoader
{
 public:
    nsXPTZipLoader();
    virtual ~nsXPTZipLoader() {}
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPTLOADER

 private:
    nsIZipReader* GetZipReader(nsILocalFile* aFile);
    nsCOMPtr<nsIZipReaderCache> mCache;
};

