



#ifndef mozilla_dom_WindowsLocationProvider_h__
#define mozilla_dom_WindowsLocationProvider_h__

#include "nsAutoPtr.h"
#include "nsIGeolocationProvider.h"

#include <locationapi.h>

namespace mozilla {
namespace dom {

class WindowsLocationProvider MOZ_FINAL : public nsIGeolocationProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONPROVIDER

  WindowsLocationProvider();

private:
  ~WindowsLocationProvider() {}

  nsRefPtr<ILocation> mLocation;
};

} 
} 

#endif 
