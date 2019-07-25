



































#ifndef NSPROTECTEDAUTHTHREAD_H_
#define NSPROTECTEDAUTHTHREAD_H_

#include <nsCOMPtr.h>
#include "keyhi.h"
#include "nspr.h"

#include "mozilla/Mutex.h"
#include "nsIProtectedAuthThread.h"

class nsProtectedAuthThread : public nsIProtectedAuthThread
{
private:
    mozilla::Mutex mMutex;

    nsCOMPtr<nsIObserver> mStatusObserver;

    bool        mIAmRunning;
    bool        mStatusObserverNotified;
    bool        mLoginReady;

    PRThread    *mThreadHandle;

    
    PK11SlotInfo*   mSlot;

    
    SECStatus       mLoginResult;

public:

    nsProtectedAuthThread();
    virtual ~nsProtectedAuthThread();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTECTEDAUTHTHREAD

    
    void SetParams(PK11SlotInfo *slot);

    
    SECStatus GetResult();

    void Join(void);

    void Run(void);
};

#endif 
