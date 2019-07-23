






























#include "nsIDirectoryService.h"
#include "nsILocalFile.h"
#include "nsEmbedString.h"
#include "nsCOMPtr.h"

class nsILocalFile;





class winEmbedFileLocProvider : public nsIDirectoryServiceProvider
{
public:
    
    
    winEmbedFileLocProvider(const nsACString& aProductDirName);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIDIRECTORYSERVICEPROVIDER

protected:
    virtual              ~winEmbedFileLocProvider();

    NS_METHOD            CloneMozBinDirectory(nsILocalFile **aLocalFile);   
    NS_METHOD            GetProductDirectory(nsILocalFile **aLocalFile);
    NS_METHOD            GetDefaultUserProfileRoot(nsILocalFile **aLocalFile);


    nsEmbedCString         mProductDirName;
    nsCOMPtr<nsILocalFile> mMozBinDirectory;
};
