
































#ifndef GMP_ENTRYPOINTS_h_
#define GMP_ENTRYPOINTS_h_

#include "gmp-errors.h"
#include "gmp-platform.h"








typedef GMPErr (*GMPInitFunc)(const GMPPlatformAPI* aPlatformAPI);











typedef GMPErr (*GMPGetAPIFunc)(const char* aAPIName, void* aHostAPI, void** aPluginAPI);




typedef void   (*GMPShutdownFunc)(void);





typedef void   (*GMPSetNodeIdFunc)(const char* aNodeId, uint32_t aLength);

#endif 
