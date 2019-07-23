









































#ifndef _nsDiskCache_h_
#define _nsDiskCache_h_

#include "nsCacheEntry.h"

#ifdef XP_WIN
#include <winsock.h>  
#endif


class nsDiskCache {
public:
    enum {
            kCurrentVersion = 0x0001000B      
    };

    enum { kData, kMetaData };

    static PLDHashNumber    Hash(const char* key);
    static nsresult         Truncate(PRFileDesc *  fd, PRUint32  newEOF);
};

#endif
