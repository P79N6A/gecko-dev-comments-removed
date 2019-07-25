





#ifndef _nsDiskCacheDevice_h_
#define _nsDiskCacheDevice_h_

#include "nsCacheDevice.h"
#include "nsDiskCacheBinding.h"
#include "nsDiskCacheBlockFile.h"
#include "nsDiskCacheEntry.h"

#include "nsIFile.h"
#include "nsIObserver.h"
#include "nsCOMArray.h"

class nsDiskCacheMap;


class nsDiskCacheDevice : public nsCacheDevice {
public:
    nsDiskCacheDevice();
    virtual ~nsDiskCacheDevice();

    virtual nsresult        Init();
    virtual nsresult        Shutdown();

    virtual const char *    GetDeviceID(void);
    virtual nsCacheEntry *  FindEntry(nsCString * key, bool *collision);
    virtual nsresult        DeactivateEntry(nsCacheEntry * entry);
    virtual nsresult        BindEntry(nsCacheEntry * entry);
    virtual void            DoomEntry( nsCacheEntry * entry );

    virtual nsresult OpenInputStreamForEntry(nsCacheEntry *    entry,
                                             nsCacheAccessMode mode,
                                             uint32_t          offset,
                                             nsIInputStream ** result);

    virtual nsresult OpenOutputStreamForEntry(nsCacheEntry *     entry,
                                              nsCacheAccessMode  mode,
                                              uint32_t           offset,
                                              nsIOutputStream ** result);

    virtual nsresult        GetFileForEntry(nsCacheEntry *    entry,
                                            nsIFile **        result);

    virtual nsresult        OnDataSizeChange(nsCacheEntry * entry, int32_t deltaSize);
    
    virtual nsresult        Visit(nsICacheVisitor * visitor);

    virtual nsresult        EvictEntries(const char * clientID);

    bool                    EntryIsTooBig(int64_t entrySize);

    


    void                    SetCacheParentDirectory(nsIFile * parentDir);
    void                    SetCapacity(uint32_t  capacity);
    void                    SetMaxEntrySize(int32_t  maxSizeInKilobytes);



    void                    getCacheDirectory(nsIFile ** result);
    uint32_t                getCacheCapacity();
    uint32_t                getCacheSize();
    uint32_t                getEntryCount();
    
    nsDiskCacheMap *        CacheMap()    { return &mCacheMap; }
    
private:    
    friend class nsDiskCacheDeviceDeactivateEntryEvent;
    friend class nsEvictDiskCacheEntriesEvent;
    



    inline bool IsValidBinding(nsDiskCacheBinding *binding)
    {
        NS_ASSERTION(binding, "  binding == nullptr");
        NS_ASSERTION(binding->mDeactivateEvent == nullptr,
                     "  entry in process of deactivation");
        return (binding && !binding->mDeactivateEvent);
    }

    bool                    Initialized() { return mInitialized; }

    nsresult                Shutdown_Private(bool flush);
    nsresult                DeactivateEntry_Private(nsCacheEntry * entry,
                                                    nsDiskCacheBinding * binding);

    nsresult                OpenDiskCache();
    nsresult                ClearDiskCache();

    nsresult                EvictDiskCacheEntries(uint32_t  targetCapacity);
    
    


    nsCOMPtr<nsIFile>       mCacheDirectory;
    nsDiskCacheBindery      mBindery;
    uint32_t                mCacheCapacity;     
    int32_t                 mMaxEntrySize;      
    
    nsDiskCacheMap          mCacheMap;
    bool                    mInitialized;
    bool                    mClearingDiskCache;
};

#endif 
