









































#ifndef _nsCacheSession_h_
#define _nsCacheSession_h_

#include "nspr.h"
#include "nsError.h"
#include "nsICacheSession.h"
#include "nsIOfflineCacheSession.h"
#include "nsString.h"

class nsCacheSession : public nsICacheSession
                     , public nsIOfflineCacheSession
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICACHESESSION
    NS_DECL_NSIOFFLINECACHESESSION
    
    nsCacheSession(const char * clientID, nsCacheStoragePolicy storagePolicy, PRBool streamBased);
    virtual ~nsCacheSession();
    
    nsCString *           ClientID()      { return &mClientID; }

    enum SessionInfo {
        eStoragePolicyMask        = 0x000000FF,
        eStreamBasedMask          = 0x00000100,
        eDoomEntriesIfExpiredMask = 0x00001000
    };

    void   MarkStreamBased()  { mInfo |=  eStreamBasedMask; }
    void   ClearStreamBased() { mInfo &= ~eStreamBasedMask; }
    PRBool IsStreamBased()    { return (mInfo & eStreamBasedMask) != 0; }

    void   MarkDoomEntriesIfExpired()  { mInfo |=  eDoomEntriesIfExpiredMask; }
    void   ClearDoomEntriesIfExpired() { mInfo &= ~eDoomEntriesIfExpiredMask; }
    PRBool WillDoomEntriesIfExpired()  { return (mInfo & eDoomEntriesIfExpiredMask); }

    nsCacheStoragePolicy  StoragePolicy()
    {
        return (nsCacheStoragePolicy)(mInfo & eStoragePolicyMask);
    }

    void SetStoragePolicy(nsCacheStoragePolicy policy)
    {
        NS_ASSERTION(policy <= 0xFF, "too many bits in nsCacheStoragePolicy");
        mInfo &= ~eStoragePolicyMask; 
        mInfo |= policy;
    }

private:
    nsCString               mClientID;
    PRUint32                mInfo;
};

#endif 
