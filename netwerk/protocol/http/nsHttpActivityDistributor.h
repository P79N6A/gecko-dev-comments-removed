



































#ifndef nsHttpActivityDistributor_h__
#define nsHttpActivityDistributor_h__

#include "nsIHttpActivityObserver.h"
#include "nsCOMArray.h"
#include "prlock.h"

class nsHttpActivityDistributor : public nsIHttpActivityDistributor
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHTTPACTIVITYOBSERVER
    NS_DECL_NSIHTTPACTIVITYDISTRIBUTOR

    nsHttpActivityDistributor();
    virtual ~nsHttpActivityDistributor();
    nsresult Init();

protected:
    nsCOMArray<nsIHttpActivityObserver> mObservers;
    PRLock       *mLock;
};

#endif 
