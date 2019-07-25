



#ifndef nsAHttpTransaction_h__
#define nsAHttpTransaction_h__

#include "nsISupports.h"
#include "nsTArray.h"

class nsAHttpConnection;
class nsAHttpSegmentReader;
class nsAHttpSegmentWriter;
class nsIInterfaceRequestor;
class nsIEventTarget;
class nsITransport;
class nsHttpRequestHead;
class nsHttpPipeline;










class nsAHttpTransaction : public nsISupports
{
public:
    
    virtual void SetConnection(nsAHttpConnection *) = 0;

    
    virtual nsAHttpConnection *Connection() = 0;

    
    
    virtual void GetSecurityCallbacks(nsIInterfaceRequestor **,
                                      nsIEventTarget **) = 0;

    
    virtual void OnTransportStatus(nsITransport* transport,
                                   nsresult status, PRUint64 progress) = 0;

    
    virtual bool     IsDone() = 0;
    virtual nsresult Status() = 0;
    virtual PRUint8  Caps() = 0;

    
    virtual PRUint32 Available() = 0;

    
    virtual nsresult ReadSegments(nsAHttpSegmentReader *reader,
                                  PRUint32 count, PRUint32 *countRead) = 0;

    
    virtual nsresult WriteSegments(nsAHttpSegmentWriter *writer,
                                   PRUint32 count, PRUint32 *countWritten) = 0;

    
    virtual void Close(nsresult reason) = 0;

    
    virtual void SetSSLConnectFailed() = 0;
    
    
    virtual nsHttpRequestHead *RequestHead() = 0;

    
    
    
    virtual PRUint32 Http1xTransactionCount() = 0;

    
    
    
    
    
    
    
    
    
    virtual nsresult TakeSubTransactions(
        nsTArray<nsRefPtr<nsAHttpTransaction> > &outTransactions) = 0;

    
    
    
    virtual nsresult AddTransaction(nsAHttpTransaction *transaction) = 0;
    
    
    
    virtual PRUint32 PipelineDepth() = 0;

    
    
    
    virtual nsresult SetPipelinePosition(PRInt32) = 0;
    virtual PRInt32  PipelinePosition() = 0;

    
    
    
    virtual nsHttpPipeline *QueryPipeline() { return nsnull; }

    
    
    
    virtual bool IsNullTransaction() { return false; }
    
    
    
    
    enum Classifier  {
        
        CLASS_REVALIDATION,

        
        CLASS_SCRIPT,

        
        CLASS_IMAGE,

        
        CLASS_SOLO,

        
        
        CLASS_GENERAL,

        CLASS_MAX
    };
};

#define NS_DECL_NSAHTTPTRANSACTION \
    void SetConnection(nsAHttpConnection *); \
    nsAHttpConnection *Connection(); \
    void GetSecurityCallbacks(nsIInterfaceRequestor **, \
                              nsIEventTarget **);       \
    void OnTransportStatus(nsITransport* transport, \
                           nsresult status, PRUint64 progress); \
    bool     IsDone(); \
    nsresult Status(); \
    PRUint8  Caps();   \
    PRUint32 Available(); \
    nsresult ReadSegments(nsAHttpSegmentReader *, PRUint32, PRUint32 *); \
    nsresult WriteSegments(nsAHttpSegmentWriter *, PRUint32, PRUint32 *); \
    void     Close(nsresult reason);                                    \
    void     SetSSLConnectFailed();                                     \
    nsHttpRequestHead *RequestHead();                                   \
    PRUint32 Http1xTransactionCount();                                  \
    nsresult TakeSubTransactions(nsTArray<nsRefPtr<nsAHttpTransaction> > &outTransactions); \
    nsresult AddTransaction(nsAHttpTransaction *);                      \
    PRUint32 PipelineDepth();                                           \
    nsresult SetPipelinePosition(PRInt32);                              \
    PRInt32  PipelinePosition();





class nsAHttpSegmentReader
{
public:
    
    virtual nsresult OnReadSegment(const char *segment,
                                   PRUint32 count,
                                   PRUint32 *countRead) = 0;

    
    
    
    
    
    
    
    
    
    virtual nsresult CommitToSegmentSize(PRUint32 size)
    {
        return NS_ERROR_FAILURE;
    }
};

#define NS_DECL_NSAHTTPSEGMENTREADER \
    nsresult OnReadSegment(const char *, PRUint32, PRUint32 *);





class nsAHttpSegmentWriter
{
public:
    
    virtual nsresult OnWriteSegment(char *segment,
                                    PRUint32 count,
                                    PRUint32 *countWritten) = 0;
};

#define NS_DECL_NSAHTTPSEGMENTWRITER \
    nsresult OnWriteSegment(char *, PRUint32, PRUint32 *);

#endif 
