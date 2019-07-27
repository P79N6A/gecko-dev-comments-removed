




#include "nsCOMPtr.h"
#include "nsIGeolocationProvider.h"
















class CoreLocationObjects;
class MLSFallback;

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
  virtual ~CoreLocationLocationProvider();

  CoreLocationObjects* mCLObjects;
  nsCOMPtr<nsIGeolocationUpdate> mCallback;
  nsRefPtr<MLSFallback> mMLSFallbackProvider;

  class MLSUpdate : public nsIGeolocationUpdate
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIGEOLOCATIONUPDATE

    explicit MLSUpdate(CoreLocationLocationProvider& parentProvider);

  private:
    CoreLocationLocationProvider& mParentLocationProvider;
    virtual ~MLSUpdate();
  };
};
