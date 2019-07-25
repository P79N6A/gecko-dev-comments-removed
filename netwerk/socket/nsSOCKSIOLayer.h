





#ifndef nsSOCKSIOLayer_h__
#define nsSOCKSIOLayer_h__

#include "prtypes.h"
#include "prio.h"
#include "nscore.h"

nsresult nsSOCKSIOLayerAddToSocket(int32_t       family,
                                   const char   *host, 
                                   int32_t       port,
                                   const char   *proxyHost,
                                   int32_t       proxyPort,
                                   int32_t       socksVersion,
                                   uint32_t      flags,
                                   PRFileDesc   *fd, 
                                   nsISupports **info);

#endif 
