



#ifndef nsAHttpTransaction_h__
#define nsAHttpTransaction_h__

#include "nsISupports.h"
#include "nsTArray.h"

class nsIInterfaceRequestor;
class nsIEventTarget;
class nsITransport;
class nsILoadGroupConnectionInfo;

namespace mozilla { namespace net {

class nsAHttpConnection;
class nsAHttpSegmentReader;
class nsAHttpSegmentWriter;
class nsHttpTransaction;
class nsHttpPipeline;
class nsHttpRequestHead;










class nsAHttpTransaction : public nsISupports
{
public:
    
    virtual void SetConnection(nsAHttpConnection *) = 0;

    
    virtual nsAHttpConnection *Connection() = 0;

    
    
    virtual void GetSecurityCallbacks(nsIInterfaceRequestor **) = 0;

    
    virtual void OnTransportStatus(nsITransport* transport,
                                   nsresult status, uint64_t progress) = 0;

    
    virtual bool     IsDone() = 0;
    virtual nsresult Status() = 0;
    virtual uint32_t Caps() = 0;

    
    virtual void     SetDNSWasRefreshed() = 0;

    
    virtual uint64_t Available() = 0;

    
    virtual nsresult ReadSegments(nsAHttpSegmentReader *reader,
                                  uint32_t count, uint32_t *countRead) = 0;

    
    virtual nsresult WriteSegments(nsAHttpSegmentWriter *writer,
                                   uint32_t count, uint32_t *countWritten) = 0;

    
    virtual void Close(nsresult reason) = 0;

    
    virtual void SetProxyConnectFailed() = 0;

    
    virtual nsHttpRequestHead *RequestHead() = 0;

    
    
    
    virtual uint32_t Http1xTransactionCount() = 0;

    
    
    
    
    
    
    
    
    
    virtual nsresult TakeSubTransactions(
        nsTArray<nsRefPtr<nsAHttpTransaction> > &outTransactions) = 0;

    
    
    
    virtual nsresult AddTransaction(nsAHttpTransaction *transaction) = 0;

    
    
    virtual uint32_t PipelineDepth() = 0;

    
    
    
    virtual nsresult SetPipelinePosition(int32_t) = 0;
    virtual int32_t  PipelinePosition() = 0;

    
    
    
    
    

    
    
    
    virtual nsHttpPipeline *QueryPipeline() { return nullptr; }

    
    
    
    virtual bool IsNullTransaction() { return false; }

    
    virtual nsILoadGroupConnectionInfo *LoadGroupConnectionInfo() { return nullptr; }

    virtual bool ResponseTimeoutEnabled() const { return false; }

    
    
    
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
    void GetSecurityCallbacks(nsIInterfaceRequestor **);       \
    void OnTransportStatus(nsITransport* transport, \
                           nsresult status, uint64_t progress); \
    bool     IsDone(); \
    nsresult Status(); \
    uint32_t Caps();   \
    void     SetDNSWasRefreshed(); \
    uint64_t Available(); \
    nsresult ReadSegments(nsAHttpSegmentReader *, uint32_t, uint32_t *); \
    nsresult WriteSegments(nsAHttpSegmentWriter *, uint32_t, uint32_t *); \
    void     Close(nsresult reason);                                    \
    void     SetProxyConnectFailed();                                   \
    nsHttpRequestHead *RequestHead();                                   \
    uint32_t Http1xTransactionCount();                                  \
    nsresult TakeSubTransactions(nsTArray<nsRefPtr<nsAHttpTransaction> > &outTransactions); \
    nsresult AddTransaction(nsAHttpTransaction *);                      \
    uint32_t PipelineDepth();                                           \
    nsresult SetPipelinePosition(int32_t);                              \
    int32_t  PipelinePosition();





class nsAHttpSegmentReader
{
public:
    
    virtual nsresult OnReadSegment(const char *segment,
                                   uint32_t count,
                                   uint32_t *countRead) = 0;

    
    
    
    
    
    
    
    
    
    
    virtual nsresult CommitToSegmentSize(uint32_t size, bool forceCommitment)
    {
        return NS_ERROR_FAILURE;
    }
};

#define NS_DECL_NSAHTTPSEGMENTREADER \
    nsresult OnReadSegment(const char *, uint32_t, uint32_t *);





class nsAHttpSegmentWriter
{
public:
    
    virtual nsresult OnWriteSegment(char *segment,
                                    uint32_t count,
                                    uint32_t *countWritten) = 0;
};

#define NS_DECL_NSAHTTPSEGMENTWRITER \
    nsresult OnWriteSegment(char *, uint32_t, uint32_t *);

}} 

#endif 
