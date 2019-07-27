



#ifndef nsAHttpTransaction_h__
#define nsAHttpTransaction_h__

#include "nsISupports.h"
#include "nsTArray.h"
#include "nsWeakReference.h"

class nsIInterfaceRequestor;
class nsIEventTarget;
class nsITransport;
class nsISchedulingContext;

namespace mozilla { namespace net {

class nsAHttpConnection;
class nsAHttpSegmentReader;
class nsAHttpSegmentWriter;
class nsHttpTransaction;
class nsHttpPipeline;
class nsHttpRequestHead;
class nsHttpConnectionInfo;
class NullHttpTransaction;
class SpdyConnectTransaction;











#define NS_AHTTPTRANSACTION_IID \
{ 0x2af6d634, 0x13e3, 0x494c, {0x89, 0x03, 0xc9, 0xdc, 0xe5, 0xc2, 0x2f, 0xc0 }}

class nsAHttpTransaction : public nsSupportsWeakReference
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_AHTTPTRANSACTION_IID)

    
    virtual void SetConnection(nsAHttpConnection *) = 0;

    
    virtual nsAHttpConnection *Connection() = 0;

    
    
    virtual void GetSecurityCallbacks(nsIInterfaceRequestor **) = 0;

    
    virtual void OnTransportStatus(nsITransport* transport,
                                   nsresult status, int64_t progress) = 0;

    
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
    virtual NullHttpTransaction *QueryNullTransaction() { return nullptr; }

    
    
    
    virtual nsHttpTransaction *QueryHttpTransaction() { return nullptr; }

    
    
    
    virtual SpdyConnectTransaction *QuerySpdyConnectTransaction() { return nullptr; }

    
    virtual nsISchedulingContext *SchedulingContext() { return nullptr; }

    
    virtual nsHttpConnectionInfo *ConnectionInfo() = 0;

    
    virtual bool ResponseTimeoutEnabled() const;
    virtual PRIntervalTime ResponseTimeout();

    
    
    
    enum Classifier  {
        
        CLASS_REVALIDATION,

        
        CLASS_SCRIPT,

        
        CLASS_IMAGE,

        
        CLASS_SOLO,

        
        
        CLASS_GENERAL,

        CLASS_MAX
    };

    
    
    
    
    
    
    virtual nsresult GetTransactionSecurityInfo(nsISupports **)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    virtual void DisableSpdy() { }
    virtual void ReuseConnectionOnRestartOK(bool) { }
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsAHttpTransaction, NS_AHTTPTRANSACTION_IID)

#define NS_DECL_NSAHTTPTRANSACTION \
    void SetConnection(nsAHttpConnection *) override; \
    nsAHttpConnection *Connection() override; \
    void GetSecurityCallbacks(nsIInterfaceRequestor **) override;       \
    void OnTransportStatus(nsITransport* transport, \
                           nsresult status, int64_t progress) override; \
    bool     IsDone() override; \
    nsresult Status() override; \
    uint32_t Caps() override;   \
    void     SetDNSWasRefreshed() override; \
    uint64_t Available() override; \
    virtual nsresult ReadSegments(nsAHttpSegmentReader *, uint32_t, uint32_t *) override; \
    virtual nsresult WriteSegments(nsAHttpSegmentWriter *, uint32_t, uint32_t *) override; \
    virtual void Close(nsresult reason) override;                                \
    nsHttpConnectionInfo *ConnectionInfo() override;                             \
    void     SetProxyConnectFailed() override;                                   \
    virtual nsHttpRequestHead *RequestHead() override;                                   \
    uint32_t Http1xTransactionCount() override;                                  \
    nsresult TakeSubTransactions(nsTArray<nsRefPtr<nsAHttpTransaction> > &outTransactions) override; \
    nsresult AddTransaction(nsAHttpTransaction *) override;                      \
    uint32_t PipelineDepth() override;                                           \
    nsresult SetPipelinePosition(int32_t) override;                              \
    int32_t  PipelinePosition() override;





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
    nsresult OnReadSegment(const char *, uint32_t, uint32_t *) override;





class nsAHttpSegmentWriter
{
public:
    
    virtual nsresult OnWriteSegment(char *segment,
                                    uint32_t count,
                                    uint32_t *countWritten) = 0;
};

#define NS_DECL_NSAHTTPSEGMENTWRITER \
    nsresult OnWriteSegment(char *, uint32_t, uint32_t *) override;

}} 

#endif 
