



































#ifndef nsSyncStreamListener_h__
#define nsSyncStreamListener_h__

#include "nsISyncStreamListener.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsThreadUtils.h"
#include "nsCOMPtr.h"



class nsSyncStreamListener : public nsISyncStreamListener
                           , public nsIInputStream
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSISYNCSTREAMLISTENER
    NS_DECL_NSIINPUTSTREAM

    nsSyncStreamListener()
        : mStatus(NS_OK)
        , mKeepWaiting(PR_FALSE)
        , mDone(PR_FALSE) {}

    nsresult Init();

private:
    ~nsSyncStreamListener() {}

    nsresult WaitForData();

    nsCOMPtr<nsIInputStream>    mPipeIn;
    nsCOMPtr<nsIOutputStream>   mPipeOut;
    nsresult                    mStatus;
    PRPackedBool                mKeepWaiting;
    PRPackedBool                mDone;
};

#endif 
