







































#ifndef _nsCacheRequest_h_
#define _nsCacheRequest_h_

#include "nspr.h"
#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"
#include "nsCOMPtr.h"
#include "nsICache.h"
#include "nsICacheListener.h"
#include "nsCacheSession.h"
#include "nsCacheService.h"


class nsCacheRequest : public PRCList
{
    typedef mozilla::CondVar CondVar;
    typedef mozilla::MutexAutoLock MutexAutoLock;
    typedef mozilla::Mutex Mutex;

private:
    friend class nsCacheService;
    friend class nsCacheEntry;
    friend class nsProcessRequestEvent;

    nsCacheRequest( nsCString *           key, 
                    nsICacheListener *    listener,
                    nsCacheAccessMode     accessRequested,
                    bool                  blockingMode,
                    nsCacheSession *      session)
        : mKey(key),
          mInfo(0),
          mListener(listener),
          mLock("nsCacheRequest.mLock"),
          mCondVar(mLock, "nsCacheRequest.mCondVar")
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
    bool IsStreamBased()      { return (mInfo & eStreamBasedMask) != 0; }


    void   MarkDoomEntriesIfExpired()   { mInfo |=  eDoomEntriesIfExpiredMask; }
    bool WillDoomEntriesIfExpired()   { return (0 != (mInfo & eDoomEntriesIfExpiredMask)); }
    
    void   MarkBlockingMode()           { mInfo |= eBlockingModeMask; }
    bool IsBlocking()                 { return (0 != (mInfo & eBlockingModeMask)); }
    bool IsNonBlocking()              { return !(mInfo & eBlockingModeMask); }

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
    bool WaitingForValidation()
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
        {
            MutexAutoLock lock(mLock);
            while (WaitingForValidation()) {
                mCondVar.Wait();
            }
            MarkWaitingForValidation();  
        }       
        return NS_OK;
    }

    void WakeUp(void) {
        DoneWaitingForValidation();
        MutexAutoLock lock(mLock);
        mCondVar.Notify();
    }

    


    nsCString *                mKey;
    PRUint32                   mInfo;
    nsICacheListener *         mListener;  
    nsCOMPtr<nsIThread>        mThread;
    Mutex                      mLock;
    CondVar                    mCondVar;
};

#endif 
