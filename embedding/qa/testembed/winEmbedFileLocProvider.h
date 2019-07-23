






































#include "nsIDirectoryService.h"
#include "nsILocalFile.h"

class nsIFile;





class winEmbedFileLocProvider : public nsIDirectoryServiceProvider
{
public:
                        
                        
                        winEmbedFileLocProvider(const char* productDirName);

   NS_DECL_ISUPPORTS
   NS_DECL_NSIDIRECTORYSERVICEPROVIDER

protected:
   virtual              ~winEmbedFileLocProvider();

   NS_METHOD            CloneMozBinDirectory(nsILocalFile **aLocalFile);   
   NS_METHOD            GetProductDirectory(nsILocalFile **aLocalFile);
   NS_METHOD            GetDefaultUserProfileRoot(nsILocalFile **aLocalFile);
   NS_METHOD            GetGreDirectory(nsILocalFile **aLocalFile); 

   char                 mProductDirName[256];
   nsCOMPtr<nsILocalFile> mMozBinDirectory;
};
