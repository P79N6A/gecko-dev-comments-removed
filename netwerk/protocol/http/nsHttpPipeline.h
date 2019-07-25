




#ifndef nsHttpPipeline_h__
#define nsHttpPipeline_h__

#include "nsHttp.h"
#include "nsAHttpConnection.h"
#include "nsAHttpTransaction.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"

class nsHttpPipeline : public nsAHttpConnection
                     , public nsAHttpTransaction
                     , public nsAHttpSegmentReader
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSAHTTPCONNECTION(mConnection)
    NS_DECL_NSAHTTPTRANSACTION
    NS_DECL_NSAHTTPSEGMENTREADER

    nsHttpPipeline();
    virtual ~nsHttpPipeline();

private:
    nsresult FillSendBuf();
    
    static NS_METHOD ReadFromPipe(nsIInputStream *, void *, const char *,
                                  uint32_t, uint32_t, uint32_t *);

    
    nsAHttpTransaction *Request(int32_t i)
    {
        if (mRequestQ.Length() == 0)
            return nullptr;

        return mRequestQ[i];
    }
    nsAHttpTransaction *Response(int32_t i)
    {
        if (mResponseQ.Length() == 0)
            return nullptr;

        return mResponseQ[i];
    }

    
    nsHttpPipeline *QueryPipeline();

    nsAHttpConnection            *mConnection;
    nsTArray<nsAHttpTransaction*> mRequestQ;  
    nsTArray<nsAHttpTransaction*> mResponseQ; 
    nsresult                      mStatus;

    
    
    
    
    bool mRequestIsPartial;
    bool mResponseIsPartial;

    
    bool mClosed;

    
    
    bool mUtilizedPipeline;

    
    nsAHttpSegmentReader *mReader;

    
    nsCOMPtr<nsIInputStream>  mSendBufIn;
    nsCOMPtr<nsIOutputStream> mSendBufOut;

    
    char     *mPushBackBuf;
    uint32_t  mPushBackLen;
    uint32_t  mPushBackMax;

    
    uint32_t  mHttp1xTransactionCount;

    
    uint64_t  mReceivingFromProgress;
    uint64_t  mSendingToProgress;
    bool      mSuppressSendEvents;
};

#endif 
