




#include "nsCOMPtr.h"
#include "nsIGeolocationProvider.h"















class CoreLocationObjects;

class CoreLocationLocationProvider
  : public nsIGeolocationProvider
  , public nsIGeolocationUpdate
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONUPDATE
  NS_DECL_NSIGEOLOCATIONPROVIDER

  CoreLocationLocationProvider();
private:
  virtual ~CoreLocationLocationProvider() {};

  CoreLocationObjects* mCLObjects;
  nsCOMPtr<nsIGeolocationUpdate> mCallback;
};
