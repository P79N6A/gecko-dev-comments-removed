




#if !defined(AndroidMediaPluginHost_h_)
#define AndroidMediaPluginHost_h_

#include "nsTArray.h"
#include "MediaResource.h"
#include "MPAPI.h"
#include "AndroidMediaResourceServer.h"

namespace mozilla {

class AndroidMediaPluginHost {
  nsRefPtr<AndroidMediaResourceServer> mResourceServer;
  nsTArray<MPAPI::Manifest *> mPlugins;

  MPAPI::Manifest *FindPlugin(const nsACString& aMimeType);
public:
  AndroidMediaPluginHost();
  ~AndroidMediaPluginHost();

  static void Shutdown();

  bool FindDecoder(const nsACString& aMimeType, const char* const** aCodecs);
  MPAPI::Decoder *CreateDecoder(mozilla::MediaResource *aResource, const nsACString& aMimeType);
  void DestroyDecoder(MPAPI::Decoder *aDecoder);
};



AndroidMediaPluginHost *EnsureAndroidMediaPluginHost();


AndroidMediaPluginHost *GetAndroidMediaPluginHost();

} 

#endif
