



#ifndef nsAHttpTransaction_h__
#define nsAHttpTransaction_h__

#include "nsISupports.h"
#include "nsTArray.h"
#include "nsWeakReference.h"

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
class nsHttpConnectionInfo;
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

    
    
    
    virtual nsHttpTransaction *QueryHttpTransaction() { return nullptr; }

    
    
    
    virtual SpdyConnectTransaction *QuerySpdyConnectTransaction() { return nullptr; }

    
    virtual nsILoadGroupConnectionInfo *LoadGroupConnectionInfo() { return nullptr; }

    
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
    void SetConnection(nsAHttpConnection *) MOZ_OVERRIDE; \
    nsAHttpConnection *Connection() MOZ_OVERRIDE; \
    void GetSecurityCallbacks(nsIInterfaceRequestor **) MOZ_OVERRIDE;       \
    void OnTransportStatus(nsITransport* transport, \
                           nsresult status, uint64_t progress) MOZ_OVERRIDE; \
    bool     IsDone() MOZ_OVERRIDE; \
    nsresult Status() MOZ_OVERRIDE; \
    uint32_t Caps() MOZ_OVERRIDE;   \
    void     SetDNSWasRefreshed() MOZ_OVERRIDE; \
    uint64_t Available() MOZ_OVERRIDE; \
    virtual nsresult ReadSegments(nsAHttpSegmentReader *, uint32_t, uint32_t *) MOZ_OVERRIDE; \
    virtual nsresult WriteSegments(nsAHttpSegmentWriter *, uint32_t, uint32_t *) MOZ_OVERRIDE; \
    virtual void Close(nsresult reason) MOZ_OVERRIDE;                                \
    nsHttpConnectionInfo *ConnectionInfo() MOZ_OVERRIDE;                             \
    void     SetProxyConnectFailed() MOZ_OVERRIDE;                                   \
    virtual nsHttpRequestHead *RequestHead() MOZ_OVERRIDE;                                   \
    uint32_t Http1xTransactionCount() MOZ_OVERRIDE;                                  \
    nsresult TakeSubTransactions(nsTArray<nsRefPtr<nsAHttpTransaction> > &outTransactions) MOZ_OVERRIDE; \
    nsresult AddTransaction(nsAHttpTransaction *) MOZ_OVERRIDE;                      \
    uint32_t PipelineDepth() MOZ_OVERRIDE;                                           \
    nsresult SetPipelinePosition(int32_t) MOZ_OVERRIDE;                              \
    int32_t  PipelinePosition() MOZ_OVERRIDE;





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
    nsresult OnReadSegment(const char *, uint32_t, uint32_t *) MOZ_OVERRIDE;





class nsAHttpSegmentWriter
{
public:
    
    virtual nsresult OnWriteSegment(char *segment,
                                    uint32_t count,
                                    uint32_t *countWritten) = 0;
};

#define NS_DECL_NSAHTTPSEGMENTWRITER \
    nsresult OnWriteSegment(char *, uint32_t, uint32_t *) MOZ_OVERRIDE;

}} 

#endif 
