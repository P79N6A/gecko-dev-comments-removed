




#if !defined(MediaPluginHost_h_)
#define MediaPluginHost_h_

#include "nsTArray.h"
#include "MediaResource.h"
#include "MPAPI.h"
#include "MediaResourceServer.h"

namespace mozilla {

class MediaPluginReader;

class MediaPluginHost {
  nsRefPtr<MediaResourceServer> mResourceServer;
  nsTArray<MPAPI::Manifest *> mPlugins;

  MPAPI::Manifest *FindPlugin(const nsACString& aMimeType);
public:
  MediaPluginHost();
  ~MediaPluginHost();

  static void Shutdown();

  bool FindDecoder(const nsACString& aMimeType, const char* const** aCodecs);
  MPAPI::Decoder *CreateDecoder(mozilla::MediaResource *aResource, const nsACString& aMimeType);
  void DestroyDecoder(MPAPI::Decoder *aDecoder);
};

MediaPluginHost *GetMediaPluginHost();

} 

#endif
