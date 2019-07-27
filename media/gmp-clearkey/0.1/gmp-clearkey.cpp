



#include <stdio.h>
#include <string.h>

#include "ClearKeyDecryptionManager.h"

#include "gmp-api/gmp-decryption.h"
#include "gmp-api/gmp-platform.h"
#include "mozilla/Attributes.h"

static GMPPlatformAPI* sPlatform = nullptr;
GMPPlatformAPI*
GetPlatform()
{
  return sPlatform;
}

extern "C" {

MOZ_EXPORT GMPErr
GMPInit(GMPPlatformAPI* aPlatformAPI)
{
  sPlatform = aPlatformAPI;
  return GMPNoErr;
}

MOZ_EXPORT GMPErr
GMPGetAPI(const char* aApiName, void* aHostAPI, void** aPluginAPI)
{
  if (strcmp(aApiName, GMP_API_DECRYPTOR)) {
    return GMPNotImplementedErr;
  }

  *aPluginAPI = new ClearKeyDecryptionManager();

  return GMPNoErr;
}

MOZ_EXPORT GMPErr
GMPShutdown(void)
{
  return GMPNoErr;
}

}
