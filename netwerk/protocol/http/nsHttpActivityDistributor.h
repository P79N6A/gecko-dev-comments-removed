



































#ifndef nsHttpActivityDistributor_h__
#define nsHttpActivityDistributor_h__

#include "nsIHttpActivityObserver.h"
#include "nsCOMArray.h"
#include "mozilla/Mutex.h"

class nsHttpActivityDistributor : public nsIHttpActivityDistributor
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHTTPACTIVITYOBSERVER
    NS_DECL_NSIHTTPACTIVITYDISTRIBUTOR

    nsHttpActivityDistributor();
    virtual ~nsHttpActivityDistributor();

protected:
    nsCOMArray<nsIHttpActivityObserver> mObservers;
    mozilla::Mutex mLock;
};

#endif 
