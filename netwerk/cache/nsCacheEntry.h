





#ifndef _nsCacheEntry_h_
#define _nsCacheEntry_h_

#include "nsICache.h"
#include "nsICacheEntryDescriptor.h"
#include "nsIThread.h"
#include "nsCacheMetaData.h"

#include "nspr.h"
#include "pldhash.h"
#include "nsAutoPtr.h"
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsAString.h"

class nsCacheDevice;
class nsCacheMetaData;
class nsCacheRequest;
class nsCacheEntryDescriptor;




class nsCacheEntry : public PRCList
{
public:

    nsCacheEntry(const nsACString &   key,
                 bool                 streamBased,
                 nsCacheStoragePolicy storagePolicy);
    ~nsCacheEntry();


    static nsresult  Create( const char *          key,
                             bool                  streamBased,
                             nsCacheStoragePolicy  storagePolicy,
                             nsCacheDevice *       device,
                             nsCacheEntry **       result);
                                      
    nsCString *  Key()  { return &mKey; }

    int32_t  FetchCount()                              { return mFetchCount; }
    void     SetFetchCount( int32_t   count)           { mFetchCount = count; }
    void     Fetched();

    uint32_t LastFetched()                             { return mLastFetched; }
    void     SetLastFetched( uint32_t  lastFetched)    { mLastFetched = lastFetched; }

    uint32_t LastModified()                            { return mLastModified; }
    void     SetLastModified( uint32_t lastModified)   { mLastModified = lastModified; }

    uint32_t ExpirationTime()                     { return mExpirationTime; }
    void     SetExpirationTime( uint32_t expires) { mExpirationTime = expires; }

    uint32_t Size()                               
        { return mDataSize + mMetaData.Size() + mKey.Length() ; }

    nsCacheDevice * CacheDevice()                            { return mCacheDevice; }
    void            SetCacheDevice( nsCacheDevice * device)  { mCacheDevice = device; }
    void            SetCustomCacheDevice( nsCacheDevice * device )
                                                             { mCustomDevice = device; }
    nsCacheDevice * CustomCacheDevice()                      { return mCustomDevice; }
    const char *    GetDeviceID();

    


    nsISupports *Data()                           { return mData; }
    void         SetData( nsISupports * data);

    int64_t  PredictedDataSize()                  { return mPredictedDataSize; }
    void     SetPredictedDataSize(int64_t size)   { mPredictedDataSize = size; }

    uint32_t DataSize()                           { return mDataSize; }
    void     SetDataSize( uint32_t  size)         { mDataSize = size; }

    void     TouchData();
    
    


    const char * GetMetaDataElement( const char *  key) { return mMetaData.GetElement(key); }
    nsresult     SetMetaDataElement( const char *  key,
                                     const char *  value) { return mMetaData.SetElement(key, value); }
    nsresult VisitMetaDataElements( nsICacheMetaDataVisitor * visitor) { return mMetaData.VisitElements(visitor); }
    nsresult FlattenMetaData(char * buffer, uint32_t bufSize) { return mMetaData.FlattenMetaData(buffer, bufSize); }
    nsresult UnflattenMetaData(const char * buffer, uint32_t bufSize) { return mMetaData.UnflattenMetaData(buffer, bufSize); }
    uint32_t MetaDataSize() { return mMetaData.Size(); }  

    void     TouchMetaData();


    


    nsISupports* SecurityInfo() { return mSecurityInfo; }
    void     SetSecurityInfo( nsISupports *  info) { mSecurityInfo = info; }


    


    enum CacheEntryFlags {
        eStoragePolicyMask   = 0x000000FF,
        eDoomedMask          = 0x00000100,
        eEntryDirtyMask      = 0x00000200,
        eDataDirtyMask       = 0x00000400,
        eMetaDataDirtyMask   = 0x00000800,
        eStreamDataMask      = 0x00001000,
        eActiveMask          = 0x00002000,
        eInitializedMask     = 0x00004000,
        eValidMask           = 0x00008000,
        eBindingMask         = 0x00010000,
        ePrivateMask         = 0x00020000
    };
    
    void MarkBinding()         { mFlags |=  eBindingMask; }
    void ClearBinding()        { mFlags &= ~eBindingMask; }
    bool IsBinding()         { return (mFlags & eBindingMask) != 0; }

    void MarkEntryDirty()      { mFlags |=  eEntryDirtyMask; }
    void MarkEntryClean()      { mFlags &= ~eEntryDirtyMask; }
    void MarkDataDirty()       { mFlags |=  eDataDirtyMask; }
    void MarkDataClean()       { mFlags &= ~eDataDirtyMask; }
    void MarkMetaDataDirty()   { mFlags |=  eMetaDataDirtyMask; }
    void MarkMetaDataClean()   { mFlags &= ~eMetaDataDirtyMask; }
    void MarkStreamData()      { mFlags |=  eStreamDataMask; }
    void MarkValid()           { mFlags |=  eValidMask; }
    void MarkInvalid()         { mFlags &= ~eValidMask; }
    void MarkPrivate()         { mFlags |=  ePrivateMask; }
    void MarkPublic()          { mFlags &= ~ePrivateMask; }
    
    

    bool IsDoomed()          { return (mFlags & eDoomedMask) != 0; }
    bool IsEntryDirty()      { return (mFlags & eEntryDirtyMask) != 0; }
    bool IsDataDirty()       { return (mFlags & eDataDirtyMask) != 0; }
    bool IsMetaDataDirty()   { return (mFlags & eMetaDataDirtyMask) != 0; }
    bool IsStreamData()      { return (mFlags & eStreamDataMask) != 0; }
    bool IsActive()          { return (mFlags & eActiveMask) != 0; }
    bool IsInitialized()     { return (mFlags & eInitializedMask) != 0; }
    bool IsValid()           { return (mFlags & eValidMask) != 0; }
    bool IsInvalid()         { return (mFlags & eValidMask) == 0; }
    bool IsInUse()           { return IsBinding() ||
                                        !(PR_CLIST_IS_EMPTY(&mRequestQ) &&
                                          PR_CLIST_IS_EMPTY(&mDescriptorQ)); }
    bool IsNotInUse()        { return !IsInUse(); }
    bool IsPrivate()         { return (mFlags & ePrivateMask) != 0; }


    bool IsAllowedInMemory()
    {
        return (StoragePolicy() ==  nsICache::STORE_ANYWHERE) ||
            (StoragePolicy() == nsICache::STORE_IN_MEMORY);
    }

    bool IsAllowedOnDisk()
    {
        return !IsPrivate() && ((StoragePolicy() == nsICache::STORE_ANYWHERE) ||
            (StoragePolicy() == nsICache::STORE_ON_DISK));
    }

    bool IsAllowedOffline()
    {
        return (StoragePolicy() == nsICache::STORE_OFFLINE);
    }

    nsCacheStoragePolicy  StoragePolicy()
    {
        return (nsCacheStoragePolicy)(mFlags & eStoragePolicyMask);
    }

    void SetStoragePolicy(nsCacheStoragePolicy policy)
    {
        NS_ASSERTION(policy <= 0xFF, "too many bits in nsCacheStoragePolicy");
        mFlags &= ~eStoragePolicyMask; 
        mFlags |= policy;
    }


    
    nsresult RequestAccess( nsCacheRequest * request, nsCacheAccessMode *accessGranted);
    nsresult CreateDescriptor( nsCacheRequest *           request,
                               nsCacheAccessMode          accessGranted,
                               nsICacheEntryDescriptor ** result);

    bool     RemoveRequest( nsCacheRequest * request);
    bool     RemoveDescriptor( nsCacheEntryDescriptor * descriptor,
                               bool                   * doomEntry);

    void     GetDescriptors(nsTArray<nsRefPtr<nsCacheEntryDescriptor> > &outDescriptors);

private:
    friend class nsCacheEntryHashTable;
    friend class nsCacheService;

    void     DetachDescriptors();

    
    void MarkDoomed()          { mFlags |=  eDoomedMask; }
    void MarkStreamBased()     { mFlags |=  eStreamDataMask; }
    void MarkInitialized()     { mFlags |=  eInitializedMask; }
    void MarkActive()          { mFlags |=  eActiveMask; }
    void MarkInactive()        { mFlags &= ~eActiveMask; }

    nsCString               mKey;
    uint32_t                mFetchCount;     
    uint32_t                mLastFetched;    
    uint32_t                mLastModified;   
    uint32_t                mLastValidated;  
    uint32_t                mExpirationTime; 
    uint32_t                mFlags;          
    int64_t                 mPredictedDataSize;  
    uint32_t                mDataSize;       
    nsCacheDevice *         mCacheDevice;    
    nsCacheDevice *         mCustomDevice;   
    nsCOMPtr<nsISupports>   mSecurityInfo;   
    nsISupports *           mData;           
    nsCOMPtr<nsIThread>     mThread;
    nsCacheMetaData         mMetaData;       
    PRCList                 mRequestQ;       
    PRCList                 mDescriptorQ;    
};





class nsCacheEntryInfo : public nsICacheEntryInfo {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICACHEENTRYINFO

    explicit nsCacheEntryInfo(nsCacheEntry* entry)
        :   mCacheEntry(entry)
    {
    }

    void    DetachEntry() { mCacheEntry = nullptr; }

private:
    nsCacheEntry * mCacheEntry;

    virtual ~nsCacheEntryInfo() {}
};





typedef struct {
    PLDHashNumber  keyHash;
    nsCacheEntry  *cacheEntry;
} nsCacheEntryHashTableEntry;


class nsCacheEntryHashTable
{
public:
    nsCacheEntryHashTable();
    ~nsCacheEntryHashTable();

    nsresult      Init();
    void          Shutdown();

    nsCacheEntry *GetEntry( const nsCString * key);
    nsresult      AddEntry( nsCacheEntry *entry);
    void          RemoveEntry( nsCacheEntry *entry);
    
    void          VisitEntries( PLDHashEnumerator etor, void *arg);

private:
    
    static PLDHashNumber  HashKey( PLDHashTable *table, const void *key);

    static bool           MatchEntry( PLDHashTable *           table,
                                      const PLDHashEntryHdr *  entry,
                                      const void *             key);

    static void           MoveEntry( PLDHashTable *table,
                                     const PLDHashEntryHdr *from,
                                     PLDHashEntryHdr       *to);

    static void           ClearEntry( PLDHashTable *table, PLDHashEntryHdr *entry);

    static void           Finalize( PLDHashTable *table);

    static
    PLDHashOperator       FreeCacheEntries(PLDHashTable *    table,
                                           PLDHashEntryHdr * hdr,
                                           uint32_t          number,
                                           void *            arg);
    static
    PLDHashOperator       VisitEntry(PLDHashTable *         table,
                                     PLDHashEntryHdr *      hdr,
                                     uint32_t               number,
                                     void *                 arg);
                                     
    
    static const PLDHashTableOps ops;
    PLDHashTable                 table;
    bool                         initialized;
};

#endif 
