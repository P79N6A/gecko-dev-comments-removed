






#ifndef _nsDiskCacheBinding_h_
#define _nsDiskCacheBinding_h_

#include "mozilla/MemoryReporting.h"
#include "nspr.h"
#include "pldhash.h"

#include "nsISupports.h"
#include "nsCacheEntry.h"

#include "nsDiskCacheMap.h"
#include "nsDiskCacheStreams.h"











class nsDiskCacheDeviceDeactivateEntryEvent;

class nsDiskCacheBinding : public nsISupports, public PRCList {
    virtual ~nsDiskCacheBinding();

public:
    NS_DECL_THREADSAFE_ISUPPORTS

    nsDiskCacheBinding(nsCacheEntry* entry, nsDiskCacheRecord * record);

    nsresult EnsureStreamIO();
    bool     IsActive() { return mCacheEntry != nullptr;}


public:
    nsCacheEntry*           mCacheEntry;    
    nsDiskCacheRecord       mRecord;
    nsDiskCacheStreamIO*    mStreamIO;      
    bool                    mDoomed;        
    uint8_t                 mGeneration;    

    
    
    
    
    nsDiskCacheDeviceDeactivateEntryEvent *mDeactivateEvent;
};






nsDiskCacheBinding *   GetCacheEntryBinding(nsCacheEntry * entry);






























class nsDiskCacheBindery {
public:
    nsDiskCacheBindery();
    ~nsDiskCacheBindery();

    nsresult                Init();
    void                    Reset();

    nsDiskCacheBinding *    CreateBinding(nsCacheEntry *       entry,
                                          nsDiskCacheRecord *  record);

    nsDiskCacheBinding *    FindActiveBinding(uint32_t  hashNumber);
    void                    RemoveBinding(nsDiskCacheBinding * binding);
    bool                    ActiveBindings();

    size_t                 SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf);

private:
    nsresult                AddBinding(nsDiskCacheBinding * binding);

    
    static const PLDHashTableOps ops;
    PLDHashTable           table;
    bool                   initialized;
};

#endif 
