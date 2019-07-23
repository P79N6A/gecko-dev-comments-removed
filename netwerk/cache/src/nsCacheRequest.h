







































#ifndef _nsCacheRequest_h_
#define _nsCacheRequest_h_

#include "nspr.h"
#include "nsCOMPtr.h"
#include "nsICache.h"
#include "nsICacheListener.h"
#include "nsCacheSession.h"
#include "nsCacheService.h"


class nsCacheRequest : public PRCList
{
private:
    friend class nsCacheService;
    friend class nsCacheEntry;

    nsCacheRequest( nsCString *           key, 
                    nsICacheListener *    listener,
                    nsCacheAccessMode     accessRequested,
                    PRBool                blockingMode,
                    nsCacheSession *      session)
        : mKey(key),
          mInfo(0),
          mListener(listener),
          mLock(nsnull),
          mCondVar(nsnull)
    {
        MOZ_COUNT_CTOR(nsCacheRequest);
        PR_INIT_CLIST(this);
        SetAccessRequested(accessRequested);
        SetStoragePolicy(session->StoragePolicy());
        if (session->IsStreamBased())             MarkStreamBased();
        if (session->WillDoomEntriesIfExpired())  MarkDoomEntriesIfExpired();
        if (blockingMode == nsICache::BLOCKING)    MarkBlockingMode();
        MarkWaitingForValidation();
        NS_IF_ADDREF(mListener);
    }
    
    ~nsCacheRequest()
    {
        MOZ_COUNT_DTOR(nsCacheRequest);
        delete mKey;
        if (mLock)    PR_DestroyLock(mLock);
        if (mCondVar) PR_DestroyCondVar(mCondVar);
        NS_ASSERTION(PR_CLIST_IS_EMPTY(this), "request still on a list");

        if (mListener)
            nsCacheService::ReleaseObject_Locked(mListener, mThread);
    }
    
    


    enum CacheRequestInfo {
        eStoragePolicyMask         = 0x000000FF,
        eStreamBasedMask           = 0x00000100,
        eDoomEntriesIfExpiredMask  = 0x00001000,
        eBlockingModeMask          = 0x00010000,
        eWaitingForValidationMask  = 0x00100000,
        eAccessRequestedMask       = 0xFF000000
    };

    void SetAccessRequested(nsCacheAccessMode mode)
    {
        NS_ASSERTION(mode <= 0xFF, "too many bits in nsCacheAccessMode");
        mInfo &= ~eAccessRequestedMask;
        mInfo |= mode << 24;
    }

    nsCacheAccessMode AccessRequested()
    {
        return (nsCacheAccessMode)((mInfo >> 24) & 0xFF);
    }

    void MarkStreamBased()      { mInfo |=  eStreamBasedMask; }
    PRBool IsStreamBased()      { return (mInfo & eStreamBasedMask) != 0; }


    void   MarkDoomEntriesIfExpired()   { mInfo |=  eDoomEntriesIfExpiredMask; }
    PRBool WillDoomEntriesIfExpired()   { return (mInfo & eDoomEntriesIfExpiredMask); }
    
    void   MarkBlockingMode()           { mInfo |= eBlockingModeMask; }
    PRBool IsBlocking()                 { return  (mInfo & eBlockingModeMask); }
    PRBool IsNonBlocking()              { return !(mInfo & eBlockingModeMask); }

    void SetStoragePolicy(nsCacheStoragePolicy policy)
    {
        NS_ASSERTION(policy <= 0xFF, "too many bits in nsCacheStoragePolicy");
        mInfo &= ~eStoragePolicyMask;  
        mInfo |= policy;         
    }

    nsCacheStoragePolicy StoragePolicy()
    {
        return (nsCacheStoragePolicy)(mInfo & 0xFF);
    }

    void   MarkWaitingForValidation() { mInfo |=  eWaitingForValidationMask; }
    void   DoneWaitingForValidation() { mInfo &= ~eWaitingForValidationMask; }
    PRBool WaitingForValidation()
    {
        return (mInfo & eWaitingForValidationMask) != 0;
    }

    nsresult
    WaitForValidation(void)
    {
        if (!WaitingForValidation()) {   
            MarkWaitingForValidation();  
            return NS_OK;                
        }

        if (!mLock) {
            mLock = PR_NewLock();
            if (!mLock) return NS_ERROR_OUT_OF_MEMORY;

            NS_ASSERTION(!mCondVar,"we have mCondVar, but didn't have mLock?");
            mCondVar = PR_NewCondVar(mLock);
            if (!mCondVar) {
                PR_DestroyLock(mLock);
                return NS_ERROR_OUT_OF_MEMORY;
            }
        }
        PRStatus status = PR_SUCCESS;
        PR_Lock(mLock);
        while (WaitingForValidation() && (status == PR_SUCCESS) ) {
            status = PR_WaitCondVar(mCondVar, PR_INTERVAL_NO_TIMEOUT);
        }
        MarkWaitingForValidation();  
        PR_Unlock(mLock);
        
        NS_ASSERTION(status == PR_SUCCESS, "PR_WaitCondVar() returned PR_FAILURE?");
        if (status == PR_FAILURE)
            return NS_ERROR_UNEXPECTED;

        return NS_OK;
    }

    void WakeUp(void) {
        DoneWaitingForValidation();
        if (mLock) {
        PR_Lock(mLock);
        PR_NotifyCondVar(mCondVar);
        PR_Unlock(mLock);
        }
    }

    


    nsCString *                mKey;
    PRUint32                   mInfo;
    nsICacheListener *         mListener;  
    nsCOMPtr<nsIThread>        mThread;
    PRLock *                   mLock;
    PRCondVar *                mCondVar;
};

#endif 
