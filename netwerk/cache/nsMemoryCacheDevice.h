





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
    virtual nsCacheEntry *  FindEntry( nsCString * key, bool *collision );
    virtual void            DoomEntry( nsCacheEntry * entry );
    virtual nsresult        DeactivateEntry( nsCacheEntry * entry );

    virtual nsresult OpenInputStreamForEntry(nsCacheEntry *     entry,
                                             nsCacheAccessMode  mode,
                                             uint32_t           offset,
                                             nsIInputStream **  result);

    virtual nsresult OpenOutputStreamForEntry(nsCacheEntry *     entry,
                                              nsCacheAccessMode  mode,
                                              uint32_t           offset,
                                              nsIOutputStream ** result);

    virtual nsresult GetFileForEntry( nsCacheEntry *    entry,
                                      nsIFile **        result );

    virtual nsresult OnDataSizeChange( nsCacheEntry * entry, int32_t deltaSize );

    virtual nsresult Visit( nsICacheVisitor * visitor );

    virtual nsresult EvictEntries(const char * clientID);
    nsresult EvictPrivateEntries();
    
    void             SetCapacity(int32_t  capacity);
    void             SetMaxEntrySize(int32_t  maxSizeInKilobytes);

    bool             EntryIsTooBig(int64_t entrySize);

    size_t           TotalSize();

private:
    friend class nsMemoryCacheDeviceInfo;
    enum      { DELETE_ENTRY        = true,
                DO_NOT_DELETE_ENTRY = false };

    void      AdjustMemoryLimits( int32_t  softLimit, int32_t  hardLimit);
    void      EvictEntry( nsCacheEntry * entry , bool deleteEntry);
    void      EvictEntriesIfNecessary();
    int       EvictionList(nsCacheEntry * entry, int32_t  deltaSize);

    typedef bool (*EvictionMatcherFn)(nsCacheEntry* entry, void* args);
    nsresult DoEvictEntries(EvictionMatcherFn matchFn, void* args);

#ifdef DEBUG
    void      CheckEntryCount();
#endif
    


    enum {
        kQueueCount = 24   
    };

    nsCacheEntryHashTable  mMemCacheEntries;
    bool                   mInitialized;

    PRCList                mEvictionList[kQueueCount];

    int32_t                mHardLimit;
    int32_t                mSoftLimit;

    int32_t                mTotalSize;
    int32_t                mInactiveSize;

    int32_t                mEntryCount;
    int32_t                mMaxEntryCount;
    int32_t                mMaxEntrySize; 

    
};





class nsMemoryCacheDeviceInfo : public nsICacheDeviceInfo {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICACHEDEVICEINFO

    explicit nsMemoryCacheDeviceInfo(nsMemoryCacheDevice* device)
        :   mDevice(device)
    {
    }

private:
    virtual ~nsMemoryCacheDeviceInfo() {}
    nsMemoryCacheDevice* mDevice;
};


#endif 
