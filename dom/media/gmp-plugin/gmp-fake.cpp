


































#include <stdint.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <memory>

#include "gmp-platform.h"
#include "gmp-video-decode.h"

#if defined(GMP_FAKE_SUPPORT_DECRYPT)
#include "gmp-decryption.h"
#include "gmp-test-decryptor.h"
#include "gmp-test-storage.h"
#endif

#if defined(_MSC_VER)
#define PUBLIC_FUNC __declspec(dllexport)
#else
#define PUBLIC_FUNC
#endif

GMPPlatformAPI* g_platform_api = NULL;

extern "C" {

  PUBLIC_FUNC GMPErr
  GMPInit (GMPPlatformAPI* aPlatformAPI) {
    g_platform_api = aPlatformAPI;
    return GMPNoErr;
  }

  PUBLIC_FUNC GMPErr
  GMPGetAPI (const char* aApiName, void* aHostAPI, void** aPluginApi) {
    if (!strcmp (aApiName, GMP_API_VIDEO_DECODER)) {
      
      
      
      return GMPGenericErr;
#if defined(GMP_FAKE_SUPPORT_DECRYPT)
    } else if (!strcmp (aApiName, GMP_API_DECRYPTOR)) {
      *aPluginApi = new FakeDecryptor(static_cast<GMPDecryptorHost*> (aHostAPI));
      return GMPNoErr;
    } else if (!strcmp (aApiName, GMP_API_ASYNC_SHUTDOWN)) {
      *aPluginApi = new TestAsyncShutdown(static_cast<GMPAsyncShutdownHost*> (aHostAPI));
      return GMPNoErr;
#endif
    }
    return GMPGenericErr;
  }

  PUBLIC_FUNC void
  GMPShutdown (void) {
    g_platform_api = NULL;
  }

} 
