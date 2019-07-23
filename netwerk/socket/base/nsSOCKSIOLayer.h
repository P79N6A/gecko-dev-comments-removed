







































#ifndef nsSOCKSIOLayer_h__
#define NSSOCKSIOLayer_h__

#include "prtypes.h"
#include "prio.h"
#include "nscore.h"

nsresult nsSOCKSIOLayerAddToSocket(PRInt32       family,
                                   const char   *host, 
                                   PRInt32       port,
                                   const char   *proxyHost,
                                   PRInt32       proxyPort,
                                   PRInt32       socksVersion,
                                   PRUint32      flags,
                                   PRFileDesc   *fd, 
                                   nsISupports **info);

#endif 
