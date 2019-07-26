





#ifndef nsAppFileLocationProvider_h
#define nsAppFileLocationProvider_h

#include "nsIDirectoryService.h"
#include "nsIFile.h"
#include "mozilla/Attributes.h"

class nsIFile;





class nsAppFileLocationProvider MOZ_FINAL : public nsIDirectoryServiceProvider2
{
public:
  nsAppFileLocationProvider();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER2

private:
  ~nsAppFileLocationProvider()
  {
  }

protected:
  NS_METHOD CloneMozBinDirectory(nsIFile** aLocalFile);
  






  NS_METHOD GetProductDirectory(nsIFile** aLocalFile,
                                bool aLocal = false);
  NS_METHOD GetDefaultUserProfileRoot(nsIFile** aLocalFile,
                                      bool aLocal = false);

#if defined(MOZ_WIDGET_COCOA)
  static bool IsOSXLeopard();
#endif

  nsCOMPtr<nsIFile> mMozBinDirectory;
};

#endif
