



#ifndef nsHttpActivityDistributor_h__
#define nsHttpActivityDistributor_h__

#include "nsIHttpActivityObserver.h"
#include "nsTArray.h"
#include "nsProxyRelease.h"
#include "mozilla/Mutex.h"

namespace mozilla { namespace net {

class nsHttpActivityDistributor : public nsIHttpActivityDistributor
{
public:
    typedef nsTArray<nsMainThreadPtrHandle<nsIHttpActivityObserver> > ObserverArray;
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIHTTPACTIVITYOBSERVER
    NS_DECL_NSIHTTPACTIVITYDISTRIBUTOR

    nsHttpActivityDistributor();

protected:
    virtual ~nsHttpActivityDistributor();

    ObserverArray mObservers;
    Mutex mLock;
};

} 
} 

#endif 
