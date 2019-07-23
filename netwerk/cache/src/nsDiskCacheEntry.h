








































#ifndef _nsDiskCacheEntry_h_
#define _nsDiskCacheEntry_h_

#include "nsDiskCacheMap.h"

#include "nsCacheEntry.h"





struct nsDiskCacheEntry {
    PRUint32        mHeaderVersion; 
    PRUint32        mMetaLocation;  
    PRInt32         mFetchCount;
    PRUint32        mLastFetched;
    PRUint32        mLastModified;
    PRUint32        mExpirationTime;
    PRUint32        mDataSize;
    PRUint32        mKeySize;       
    PRUint32        mMetaDataSize;  
    
    

    PRUint32        Size()    { return sizeof(nsDiskCacheEntry) + 
                                    mKeySize + mMetaDataSize;
                              }

    char*           Key()     { return NS_REINTERPRET_CAST(char*const, this) + 
                                    sizeof(nsDiskCacheEntry);
                              }

    char*           MetaData()
                              { return Key() + mKeySize; }

    nsCacheEntry *  CreateCacheEntry(nsCacheDevice *  device);

    void Swap()         
    {
#if defined(IS_LITTLE_ENDIAN)   
        mHeaderVersion      = htonl(mHeaderVersion);
        mMetaLocation       = htonl(mMetaLocation);
        mFetchCount         = htonl(mFetchCount);
        mLastFetched        = htonl(mLastFetched);
        mLastModified       = htonl(mLastModified);
        mExpirationTime     = htonl(mExpirationTime);
        mDataSize           = htonl(mDataSize);
        mKeySize            = htonl(mKeySize);
        mMetaDataSize       = htonl(mMetaDataSize);
#endif
    }
    
    void Unswap()       
    {
#if defined(IS_LITTLE_ENDIAN)
        mHeaderVersion      = ntohl(mHeaderVersion);
        mMetaLocation       = ntohl(mMetaLocation);
        mFetchCount         = ntohl(mFetchCount);
        mLastFetched        = ntohl(mLastFetched);
        mLastModified       = ntohl(mLastModified);
        mExpirationTime     = ntohl(mExpirationTime);
        mDataSize           = ntohl(mDataSize);
        mKeySize            = ntohl(mKeySize);
        mMetaDataSize       = ntohl(mMetaDataSize);
#endif
    }
};

nsDiskCacheEntry *  CreateDiskCacheEntry(nsDiskCacheBinding *  binding,
                                         PRUint32 * size);






class nsDiskCacheEntryInfo : public nsICacheEntryInfo {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICACHEENTRYINFO

    nsDiskCacheEntryInfo(const char * deviceID, nsDiskCacheEntry * diskEntry)
        : mDeviceID(deviceID)
        , mDiskEntry(diskEntry)
    {
    }

    virtual ~nsDiskCacheEntryInfo() {}
    
    const char* Key() { return mDiskEntry->Key(); }
    
private:
    const char *        mDeviceID;
    nsDiskCacheEntry *  mDiskEntry;
};


#endif 
