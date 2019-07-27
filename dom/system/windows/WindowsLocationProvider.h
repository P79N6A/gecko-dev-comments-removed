



#ifndef mozilla_dom_WindowsLocationProvider_h__
#define mozilla_dom_WindowsLocationProvider_h__

#include "nsAutoPtr.h"
#include "nsIGeolocationProvider.h"

#include <locationapi.h>

class MLSFallback;

namespace mozilla {
namespace dom {

class WindowsLocationProvider MOZ_FINAL : public nsIGeolocationProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONPROVIDER

  WindowsLocationProvider();

  nsresult CreateAndWatchMLSProvider(nsIGeolocationUpdate* aCallback);
  void CancelMLSProvider();

  class MLSUpdate : public nsIGeolocationUpdate
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIGEOLOCATIONUPDATE
    explicit MLSUpdate(nsIGeolocationUpdate* aCallback);

  private:
    nsCOMPtr<nsIGeolocationUpdate> mCallback;
    virtual ~MLSUpdate() {}
  };
private:
  ~WindowsLocationProvider();

  nsRefPtr<ILocation> mLocation;
  nsRefPtr<MLSFallback> mMLSProvider;
};

} 
} 

#endif 
