



#ifndef nsHttpActivityDistributor_h__
#define nsHttpActivityDistributor_h__

#include "nsIHttpActivityObserver.h"
#include "nsTArray.h"
#include "nsProxyRelease.h"
#include "mozilla/Mutex.h"


class nsHttpActivityDistributor : public nsIHttpActivityDistributor
{
public:
    typedef nsTArray<nsMainThreadPtrHandle<nsIHttpActivityObserver> > ObserverArray;
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHTTPACTIVITYOBSERVER
    NS_DECL_NSIHTTPACTIVITYDISTRIBUTOR

    nsHttpActivityDistributor();
    virtual ~nsHttpActivityDistributor();

protected:
    ObserverArray mObservers;
    mozilla::Mutex mLock;
};

#endif 
