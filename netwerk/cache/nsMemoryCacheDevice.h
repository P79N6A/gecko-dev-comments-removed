








































#ifndef _nsMemoryCacheDevice_h_
#define _nsMemoryCacheDevice_h_

#include "nsCacheDevice.h"
#include "pldhash.h"
#include "nsCacheEntry.h"


class nsMemoryCacheDeviceInfo;




class nsMemoryCacheDevice : public nsCacheDevice
{
public:
    nsMemoryCacheDevice();
    virtual ~nsMemoryCacheDevice();

    virtual nsresult        Init();
    virtual nsresult        Shutdown();

    virtual const char *    GetDeviceID(void);

    virtual nsresult        BindEntry( nsCacheEntry * entry );
    virtual nsCacheEntry *  FindEntry( nsCString * key, PRBool *collision );
    virtual void            DoomEntry( nsCacheEntry * entry );
    virtual nsresult        DeactivateEntry( nsCacheEntry * entry );

    virtual nsresult OpenInputStreamForEntry(nsCacheEntry *     entry,
                                             nsCacheAccessMode  mode,
                                             PRUint32           offset,
                                             nsIInputStream **  result);

    virtual nsresult OpenOutputStreamForEntry(nsCacheEntry *     entry,
                                              nsCacheAccessMode  mode,
                                              PRUint32           offset,
                                              nsIOutputStream ** result);

    virtual nsresult GetFileForEntry( nsCacheEntry *    entry,
                                      nsIFile **        result );

    virtual nsresult OnDataSizeChange( nsCacheEntry * entry, PRInt32 deltaSize );

    virtual nsresult Visit( nsICacheVisitor * visitor );

    virtual nsresult EvictEntries(const char * clientID);
    
    void             SetCapacity(PRInt32  capacity);
 
private:
    friend class nsMemoryCacheDeviceInfo;
    enum      { DELETE_ENTRY        = PR_TRUE,
                DO_NOT_DELETE_ENTRY = PR_FALSE };

    void      AdjustMemoryLimits( PRInt32  softLimit, PRInt32  hardLimit);
    void      EvictEntry( nsCacheEntry * entry , PRBool deleteEntry);
    void      EvictEntriesIfNecessary();
    int       EvictionList(nsCacheEntry * entry, PRInt32  deltaSize);

#ifdef DEBUG
    void      CheckEntryCount();
#endif
    


    enum {
        kQueueCount = 24   
    };

    nsCacheEntryHashTable  mMemCacheEntries;
    PRBool                 mInitialized;

    PRCList                mEvictionList[kQueueCount];
    PRInt32                mEvictionThreshold;

    PRInt32                mHardLimit;
    PRInt32                mSoftLimit;

    PRInt32                mTotalSize;
    PRInt32                mInactiveSize;

    PRInt32                mEntryCount;
    PRInt32                mMaxEntryCount;

    
};





class nsMemoryCacheDeviceInfo : public nsICacheDeviceInfo {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICACHEDEVICEINFO

    nsMemoryCacheDeviceInfo(nsMemoryCacheDevice* device)
        :   mDevice(device)
    {
    }

    virtual ~nsMemoryCacheDeviceInfo() {}
    
private:
    nsMemoryCacheDevice* mDevice;
};


#endif 
