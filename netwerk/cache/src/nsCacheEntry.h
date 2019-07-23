







































#ifndef _nsCacheEntry_h_
#define _nsCacheEntry_h_

#include "nsICache.h"
#include "nsICacheEntryDescriptor.h"
#include "nsIThread.h"
#include "nsCacheMetaData.h"

#include "nspr.h"
#include "pldhash.h"
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsAString.h"

class nsCacheDevice;
class nsCacheMetaData;
class nsCacheRequest;
class nsCacheEntryDescriptor;

#define NO_EXPIRATION_TIME  0xFFFFFFFF




class nsCacheEntry : public PRCList
{
public:

    nsCacheEntry(nsCString *          key,
                 PRBool               streamBased,
                 nsCacheStoragePolicy storagePolicy);
    ~nsCacheEntry();


    static nsresult  Create( const char *          key,
                             PRBool                streamBased,
                             nsCacheStoragePolicy  storagePolicy,
                             nsCacheDevice *       device,
                             nsCacheEntry **       result);
                                      
    nsCString *  Key()  { return mKey; }

    PRInt32  FetchCount()                              { return mFetchCount; }
    void     SetFetchCount( PRInt32   count)           { mFetchCount = count; }
    void     Fetched();

    PRUint32 LastFetched()                             { return mLastFetched; }
    void     SetLastFetched( PRUint32  lastFetched)    { mLastFetched = lastFetched; }

    PRUint32 LastModified()                            { return mLastModified; }
    void     SetLastModified( PRUint32 lastModified)   { mLastModified = lastModified; }

    PRUint32 ExpirationTime()                     { return mExpirationTime; }
    void     SetExpirationTime( PRUint32 expires) { mExpirationTime = expires; }

    PRUint32 Size()                               { return mDataSize + mMetaData.Size(); }

    nsCacheDevice * CacheDevice()                            { return mCacheDevice; }
    void            SetCacheDevice( nsCacheDevice * device)  { mCacheDevice = device; }
    const char *    GetDeviceID();

    


    nsISupports *Data()                           { return mData; }
    void         SetData( nsISupports * data);

    PRUint32 DataSize()                           { return mDataSize; }
    void     SetDataSize( PRUint32  size)         { mDataSize = size; }

    void     TouchData();
    
    


    const char * GetMetaDataElement( const char *  key) { return mMetaData.GetElement(key); }
    nsresult     SetMetaDataElement( const char *  key,
                                     const char *  value) { return mMetaData.SetElement(key, value); }
    nsresult VisitMetaDataElements( nsICacheMetaDataVisitor * visitor) { return mMetaData.VisitElements(visitor); }
    nsresult FlattenMetaData(char * buffer, PRUint32 bufSize) { return mMetaData.FlattenMetaData(buffer, bufSize); }
    nsresult UnflattenMetaData(const char * buffer, PRUint32 bufSize) { return mMetaData.UnflattenMetaData(buffer, bufSize); }
    PRUint32 MetaDataSize() { return mMetaData.Size(); }  

    void     TouchMetaData();


    


    nsresult GetSecurityInfo( nsISupports ** result);
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
        eBindingMask         = 0x00010000
    };
    
    void MarkBinding()         { mFlags |=  eBindingMask; }
    void ClearBinding()        { mFlags &= ~eBindingMask; }
    PRBool IsBinding()         { return (mFlags & eBindingMask) != 0; }

    void MarkEntryDirty()      { mFlags |=  eEntryDirtyMask; }
    void MarkEntryClean()      { mFlags &= ~eEntryDirtyMask; }
    void MarkDataDirty()       { mFlags |=  eDataDirtyMask; }
    void MarkDataClean()       { mFlags &= ~eDataDirtyMask; }
    void MarkMetaDataDirty()   { mFlags |=  eMetaDataDirtyMask; }
    void MarkMetaDataClean()   { mFlags &= ~eMetaDataDirtyMask; }
    void MarkStreamData()      { mFlags |=  eStreamDataMask; }
    void MarkValid()           { mFlags |=  eValidMask; }
    void MarkInvalid()         { mFlags &= ~eValidMask; }
    
    

    PRBool IsDoomed()          { return (mFlags & eDoomedMask) != 0; }
    PRBool IsEntryDirty()      { return (mFlags & eEntryDirtyMask) != 0; }
    PRBool IsDataDirty()       { return (mFlags & eDataDirtyMask) != 0; }
    PRBool IsMetaDataDirty()   { return (mFlags & eMetaDataDirtyMask) != 0; }
    PRBool IsStreamData()      { return (mFlags & eStreamDataMask) != 0; }
    PRBool IsActive()          { return (mFlags & eActiveMask) != 0; }
    PRBool IsInitialized()     { return (mFlags & eInitializedMask) != 0; }
    PRBool IsValid()           { return (mFlags & eValidMask) != 0; }
    PRBool IsInvalid()         { return (mFlags & eValidMask) == 0; }
    PRBool IsInUse()           { return IsBinding() ||
                                        !(PR_CLIST_IS_EMPTY(&mRequestQ) &&
                                          PR_CLIST_IS_EMPTY(&mDescriptorQ)); }
    PRBool IsNotInUse()        { return !IsInUse(); }


    PRBool IsAllowedInMemory()
    {
        return (StoragePolicy() ==  nsICache::STORE_ANYWHERE) ||
            (StoragePolicy() == nsICache::STORE_IN_MEMORY);
    }

    PRBool IsAllowedOnDisk()
    {
        return (StoragePolicy() == nsICache::STORE_ANYWHERE) ||
            (StoragePolicy() == nsICache::STORE_ON_DISK) ||
            (StoragePolicy() == nsICache::STORE_ON_DISK_AS_FILE);
    }

    PRBool IsAllowedOffline()
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

    
    
    PRBool   RemoveRequest( nsCacheRequest * request);
    PRBool   RemoveDescriptor( nsCacheEntryDescriptor * descriptor);
    
private:
    friend class nsCacheEntryHashTable;
    friend class nsCacheService;

    void     DetachDescriptors(void);

    
    void MarkDoomed()          { mFlags |=  eDoomedMask; }
    void MarkStreamBased()     { mFlags |=  eStreamDataMask; }
    void MarkInitialized()     { mFlags |=  eInitializedMask; }
    void MarkActive()          { mFlags |=  eActiveMask; }
    void MarkInactive()        { mFlags &= ~eActiveMask; }

    nsCString *             mKey;            
    PRUint32                mFetchCount;     
    PRUint32                mLastFetched;    
    PRUint32                mLastModified;   
    PRUint32                mLastValidated;  
    PRUint32                mExpirationTime; 
    PRUint32                mFlags;          
    PRUint32                mDataSize;       
    nsCacheDevice *         mCacheDevice;    
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

    nsCacheEntryInfo(nsCacheEntry* entry)
        :   mCacheEntry(entry)
    {
    }

    virtual ~nsCacheEntryInfo() {}
    void    DetachEntry() { mCacheEntry = nsnull; }
    
private:
    nsCacheEntry * mCacheEntry;
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
    
    
    class Visitor {
    public:
        virtual PRBool VisitEntry( nsCacheEntry *entry) = 0;
    };
    
    void          VisitEntries( Visitor *visitor);
    
private:
    friend class nsCacheService; 

    
    static PLDHashNumber  PR_CALLBACK HashKey( PLDHashTable *table, const void *key);

    static PRBool         PR_CALLBACK MatchEntry( PLDHashTable *           table,
                                                  const PLDHashEntryHdr *  entry,
                                                  const void *             key);

    static void           PR_CALLBACK MoveEntry( PLDHashTable *table,
                                                 const PLDHashEntryHdr *from,
                                                 PLDHashEntryHdr       *to);

    static void           PR_CALLBACK ClearEntry( PLDHashTable *table, PLDHashEntryHdr *entry);

    static void           PR_CALLBACK Finalize( PLDHashTable *table);

    static
    PLDHashOperator       PR_CALLBACK FreeCacheEntries(PLDHashTable *    table,
                                                       PLDHashEntryHdr * hdr,
                                                       PRUint32          number,
                                                       void *            arg);
    static
    PLDHashOperator       PR_CALLBACK VisitEntry(PLDHashTable *         table,
                                                 PLDHashEntryHdr *      hdr,
                                                 PRUint32               number,
                                                 void *                 arg);
                                     
    
    static PLDHashTableOps ops;
    PLDHashTable           table;
    PRBool                 initialized;
};

#endif 

