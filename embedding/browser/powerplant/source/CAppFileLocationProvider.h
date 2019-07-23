






































#include "nsIDirectoryService.h"
#include "nsILocalFile.h"
#include "nsString.h"

class nsIFile;





class CAppFileLocationProvider : public nsIDirectoryServiceProvider
{
public:
                        CAppFileLocationProvider(const nsAString& aAppDataDirName);

   NS_DECL_ISUPPORTS
   NS_DECL_NSIDIRECTORYSERVICEPROVIDER

protected:
   virtual              ~CAppFileLocationProvider();

   NS_METHOD            GetAppDataDirectory(nsILocalFile **aLocalFile);
     
   nsString             mAppDataDirName;
};
