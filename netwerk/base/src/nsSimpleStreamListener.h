





































#include "nsISimpleStreamListener.h"
#include "nsIOutputStream.h"
#include "nsCOMPtr.h"

class nsSimpleStreamListener : public nsISimpleStreamListener
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSISIMPLESTREAMLISTENER

    nsSimpleStreamListener() { }
    virtual ~nsSimpleStreamListener() {}

protected:
    nsCOMPtr<nsIOutputStream>    mSink;
    nsCOMPtr<nsIRequestObserver> mObserver;
};
