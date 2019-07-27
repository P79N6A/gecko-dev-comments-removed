















#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ClearKeySessionManager.h"
#include "gmp-api/gmp-decryption.h"
#include "gmp-api/gmp-platform.h"

#if defined(ENABLE_WMF)
#include "WMFUtils.h"
#include "AudioDecoder.h"
#include "VideoDecoder.h"
#endif

#if defined(WIN32)
#define GMP_EXPORT __declspec(dllexport)
#else
#define GMP_EXPORT __attribute__((visibility("default")))
#endif

static GMPPlatformAPI* sPlatform = nullptr;
GMPPlatformAPI*
GetPlatform()
{
  return sPlatform;
}

extern "C" {

GMP_EXPORT GMPErr
GMPInit(GMPPlatformAPI* aPlatformAPI)
{
  sPlatform = aPlatformAPI;
  return GMPNoErr;
}

GMP_EXPORT GMPErr
GMPGetAPI(const char* aApiName, void* aHostAPI, void** aPluginAPI)
{
  CK_LOGD("ClearKey GMPGetAPI |%s|", aApiName);
  assert(!*aPluginAPI);

  if (!strcmp(aApiName, GMP_API_DECRYPTOR)) {
    *aPluginAPI = new ClearKeySessionManager();
  }
#if defined(ENABLE_WMF)
  else if (wmf::EnsureLibs()) {
    if (!strcmp(aApiName, GMP_API_AUDIO_DECODER)) {
      *aPluginAPI = new AudioDecoder(static_cast<GMPAudioHost*>(aHostAPI));
    } else if (!strcmp(aApiName, GMP_API_VIDEO_DECODER)) {
      *aPluginAPI = new VideoDecoder(static_cast<GMPVideoHost*>(aHostAPI));
    }
  }
#endif
  else {
    CK_LOGE("GMPGetAPI couldn't resolve API name |%s|\n", aApiName);
  }

  return *aPluginAPI ? GMPNoErr : GMPNotImplementedErr;
}

GMP_EXPORT GMPErr
GMPShutdown(void)
{
  CK_LOGD("ClearKey GMPShutdown");
  return GMPNoErr;
}

}
