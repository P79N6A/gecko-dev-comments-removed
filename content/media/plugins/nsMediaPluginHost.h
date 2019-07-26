




#if !defined(nsMediaPluginHost_h_)
#define nsMediaPluginHost_h_

#include "nsTArray.h"
#include "MediaResource.h"
#include "MPAPI.h"

namespace mozilla {

class nsMediaPluginReader;

class nsMediaPluginHost {
  nsTArray<MPAPI::Manifest *> mPlugins;
  MPAPI::Manifest *FindPlugin(const nsACString& aMimeType);
  void TryLoad(const char *name);
public:
  nsMediaPluginHost();
  ~nsMediaPluginHost();

  static void Shutdown();

  bool FindDecoder(const nsACString& aMimeType, const char* const** aCodecs);
  MPAPI::Decoder *CreateDecoder(mozilla::MediaResource *aResource, const nsACString& aMimeType);
  void DestroyDecoder(MPAPI::Decoder *aDecoder);
};

nsMediaPluginHost *GetMediaPluginHost();

} 

#endif
