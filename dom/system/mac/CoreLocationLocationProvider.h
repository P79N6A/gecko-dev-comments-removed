




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
  void CreateMLSFallbackProvider();
  void CancelMLSFallbackProvider();

private:
  virtual ~CoreLocationLocationProvider() {};

  CoreLocationObjects* mCLObjects;
  nsCOMPtr<nsIGeolocationUpdate> mCallback;
  nsCOMPtr<nsIGeolocationProvider> mMLSFallbackProvider;

  class MLSUpdate : public nsIGeolocationUpdate
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIGEOLOCATIONUPDATE

    MLSUpdate(CoreLocationLocationProvider& parentProvider);

  private:
    CoreLocationLocationProvider& mParentLocationProvider;
    virtual ~MLSUpdate() {}
  };
};
