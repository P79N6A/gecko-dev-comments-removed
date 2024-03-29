





#ifndef _nsCacheDevice_h_
#define _nsCacheDevice_h_

#include "nspr.h"
#include "nsError.h"
#include "nsICache.h"

class nsIFile;
class nsCString;
class nsCacheEntry;
class nsICacheVisitor;
class nsIInputStream;
class nsIOutputStream;




class nsCacheDevice {
public:
    nsCacheDevice() { MOZ_COUNT_CTOR(nsCacheDevice); }
    virtual ~nsCacheDevice() { MOZ_COUNT_DTOR(nsCacheDevice); }

    virtual nsresult Init() = 0;
    virtual nsresult Shutdown() = 0;

    virtual const char *   GetDeviceID(void) = 0;
    virtual nsCacheEntry * FindEntry( nsCString * key, bool *collision ) = 0;

    virtual nsresult DeactivateEntry( nsCacheEntry * entry ) = 0;
    virtual nsresult BindEntry( nsCacheEntry * entry ) = 0;
    virtual void     DoomEntry( nsCacheEntry * entry ) = 0;

    virtual nsresult OpenInputStreamForEntry(nsCacheEntry *     entry,
                                             nsCacheAccessMode  mode,
                                             uint32_t           offset,
                                             nsIInputStream **  result) = 0;

    virtual nsresult OpenOutputStreamForEntry(nsCacheEntry *     entry,
                                              nsCacheAccessMode  mode,
                                              uint32_t           offset,
                                              nsIOutputStream ** result) = 0;

    virtual nsresult GetFileForEntry( nsCacheEntry *    entry,
                                      nsIFile **        result ) = 0;

    virtual nsresult OnDataSizeChange( nsCacheEntry * entry, int32_t deltaSize ) = 0;

    virtual nsresult Visit(nsICacheVisitor * visitor) = 0;

    



    virtual nsresult EvictEntries(const char * clientID) = 0;
};

#endif 
