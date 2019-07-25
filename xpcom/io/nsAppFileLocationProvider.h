






































#include "nsIDirectoryService.h"
#include "nsILocalFile.h"

class nsIFile;





class nsAppFileLocationProvider : public nsIDirectoryServiceProvider2
{
public:
                        nsAppFileLocationProvider();

   NS_DECL_ISUPPORTS
   NS_DECL_NSIDIRECTORYSERVICEPROVIDER
   NS_DECL_NSIDIRECTORYSERVICEPROVIDER2

private:
                        ~nsAppFileLocationProvider() {}

protected:
   NS_METHOD            CloneMozBinDirectory(nsILocalFile **aLocalFile);
   






   NS_METHOD            GetProductDirectory(nsILocalFile **aLocalFile,
                                            bool aLocal = false);
   NS_METHOD            GetDefaultUserProfileRoot(nsILocalFile **aLocalFile,
                                                  bool aLocal = false);

#if defined(MOZ_WIDGET_COCOA)
   static bool          IsOSXLeopard();
#endif

   nsCOMPtr<nsILocalFile> mMozBinDirectory;
};
