




#include "nsCOMPtr.h"
#include "nsIGeolocationProvider.h"















class CoreLocationObjects;

class CoreLocationLocationProvider
  : public nsIGeolocationProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONPROVIDER

  CoreLocationLocationProvider();
  void NotifyError(uint16_t aErrorCode);
  void Update(nsIDOMGeoPosition* aSomewhere);
private:
  virtual ~CoreLocationLocationProvider() {};

  CoreLocationObjects* mCLObjects;
  nsCOMPtr<nsIGeolocationUpdate> mCallback;
};
