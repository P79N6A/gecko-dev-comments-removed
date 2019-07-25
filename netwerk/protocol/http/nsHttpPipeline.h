




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
                                  PRUint32, PRUint32, PRUint32 *);

    
    nsAHttpTransaction *Request(PRInt32 i)
    {
        if (mRequestQ.Length() == 0)
            return nsnull;

        return mRequestQ[i];
    }
    nsAHttpTransaction *Response(PRInt32 i)
    {
        if (mResponseQ.Length() == 0)
            return nsnull;

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
    nsAHttpSegmentWriter *mWriter;

    
    nsCOMPtr<nsIInputStream>  mSendBufIn;
    nsCOMPtr<nsIOutputStream> mSendBufOut;

    
    char     *mPushBackBuf;
    PRUint32  mPushBackLen;
    PRUint32  mPushBackMax;

    
    PRUint32  mHttp1xTransactionCount;

    
    PRUint64  mReceivingFromProgress;
    PRUint64  mSendingToProgress;
    bool      mSuppressSendEvents;
};

#endif 
