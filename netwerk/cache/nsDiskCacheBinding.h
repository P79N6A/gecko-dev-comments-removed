









































#ifndef _nsDiskCacheBinding_h_
#define _nsDiskCacheBinding_h_

#include "nspr.h"
#include "pldhash.h"

#include "nsISupports.h"
#include "nsCacheEntry.h"

#include "nsDiskCacheMap.h"
#include "nsDiskCacheStreams.h"











class nsDiskCacheDeviceDeactivateEntryEvent;

class nsDiskCacheBinding : public nsISupports, public PRCList {
public:
    NS_DECL_ISUPPORTS

    nsDiskCacheBinding(nsCacheEntry* entry, nsDiskCacheRecord * record);
    virtual ~nsDiskCacheBinding();

    nsresult EnsureStreamIO();
    PRBool   IsActive() { return mCacheEntry != nsnull;}


public:
    nsCacheEntry*           mCacheEntry;    
    nsDiskCacheRecord       mRecord;
    nsDiskCacheStreamIO*    mStreamIO;      
    PRBool                  mDoomed;        
    PRUint8                 mGeneration;    

    
    
    
    
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

    nsDiskCacheBinding *    FindActiveBinding(PRUint32  hashNumber);
    void                    RemoveBinding(nsDiskCacheBinding * binding);
    PRBool                  ActiveBindings();
    
private:
    nsresult                AddBinding(nsDiskCacheBinding * binding);

    
    static PLDHashTableOps ops;
    PLDHashTable           table;
    PRBool                 initialized;
};

#endif 
