




#if !defined(MediaResourceServer_h_)
#define MediaResourceServer_h_

#include <map>
#include "nsIServerSocket.h"
#include "MediaResource.h"

namespace mozilla {

class MediaResource;
























class MediaResourceServer : public nsRunnable
{
private:
  
  
  
  mozilla::Mutex mMutex;

  
  nsCOMPtr<nsIServerSocket> mSocket;

  
  
  typedef std::map<nsCString,
                  nsRefPtr<mozilla::MediaResource> > ResourceMap;
  ResourceMap mResources;

  
  
  
  MediaResourceServer();
  NS_IMETHOD Run();

  
  
  
  
  nsresult AppendRandomPath(nsCString& aURL);

public:
  
  
  static already_AddRefed<MediaResourceServer> Start();

  
  void Stop();

  
  
  nsresult AddResource(mozilla::MediaResource* aResource, nsCString& aUrl);

  
  
  
  void RemoveResource(nsCString const& aUrl);

  
  
  nsCString GetURLPrefix();

  
  already_AddRefed<mozilla::MediaResource> GetResource(nsCString const& aUrl);
};

} 

#endif
