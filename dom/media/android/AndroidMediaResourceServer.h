




#if !defined(AndroidMediaResourceServer_h_)
#define AndroidMediaResourceServer_h_

#include <map>
#include "nsIServerSocket.h"
#include "MediaResource.h"

namespace mozilla {

class MediaResource;
























class AndroidMediaResourceServer : public nsRunnable
{
private:
  
  
  
  mozilla::Mutex mMutex;

  
  nsCOMPtr<nsIServerSocket> mSocket;

  
  
  typedef std::map<nsCString,
                  nsRefPtr<mozilla::MediaResource> > ResourceMap;
  ResourceMap mResources;

  
  
  
  AndroidMediaResourceServer();
  NS_IMETHOD Run();

  
  
  
  
  nsresult AppendRandomPath(nsCString& aURL);

public:
  
  
  static already_AddRefed<AndroidMediaResourceServer> Start();

  
  void Stop();

  
  
  nsresult AddResource(mozilla::MediaResource* aResource, nsCString& aUrl);

  
  
  
  void RemoveResource(nsCString const& aUrl);

  
  
  nsCString GetURLPrefix();

  
  already_AddRefed<mozilla::MediaResource> GetResource(nsCString const& aUrl);
};

} 

#endif
