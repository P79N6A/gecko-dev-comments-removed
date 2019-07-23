





































#ifndef nsHttpPipeline_h__
#define nsHttpPipeline_h__

#include "nsHttp.h"
#include "nsAHttpConnection.h"
#include "nsAHttpTransaction.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsVoidArray.h"
#include "nsCOMPtr.h"

class nsHttpPipeline : public nsAHttpConnection
                     , public nsAHttpTransaction
                     , public nsAHttpSegmentReader
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSAHTTPCONNECTION
    NS_DECL_NSAHTTPTRANSACTION
    NS_DECL_NSAHTTPSEGMENTREADER

    nsHttpPipeline();
    virtual ~nsHttpPipeline();

    nsresult AddTransaction(nsAHttpTransaction *);

private:
    nsresult FillSendBuf();
    
    static NS_METHOD ReadFromPipe(nsIInputStream *, void *, const char *,
                                  PRUint32, PRUint32, PRUint32 *);

    
    nsAHttpTransaction *Request(PRInt32 i)
    {
        if (mRequestQ.Count() == 0)
            return nsnull;

        return (nsAHttpTransaction *) mRequestQ[i];
    }
    nsAHttpTransaction *Response(PRInt32 i)
    {
        if (mResponseQ.Count() == 0)
            return nsnull;

        return (nsAHttpTransaction *) mResponseQ[i];
    }

    nsAHttpConnection *mConnection;
    nsVoidArray        mRequestQ;  
    nsVoidArray        mResponseQ; 
    nsresult           mStatus;

    
    
    
    
    PRPackedBool mRequestIsPartial;
    PRPackedBool mResponseIsPartial;

    
    PRPackedBool mClosed;

    
    nsAHttpSegmentReader *mReader;
    nsAHttpSegmentWriter *mWriter;

    
    nsCOMPtr<nsIInputStream>  mSendBufIn;
    nsCOMPtr<nsIOutputStream> mSendBufOut;

    
    char     *mPushBackBuf;
    PRUint32  mPushBackLen;
    PRUint32  mPushBackMax;
};

#endif 
