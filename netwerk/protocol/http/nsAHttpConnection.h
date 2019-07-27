



#ifndef nsAHttpConnection_h__
#define nsAHttpConnection_h__

#include "nsISupports.h"
#include "nsAHttpTransaction.h"

class nsISocketTransport;
class nsIAsyncInputStream;
class nsIAsyncOutputStream;

namespace mozilla { namespace net {

class nsHttpConnectionInfo;
class nsHttpConnection;






#define NS_AHTTPCONNECTION_IID \
{ 0x5a66aed7, 0xeede, 0x468b, {0xac, 0x2b, 0xe5, 0xfb, 0x43, 0x1f, 0xcc, 0x5c }}

class nsAHttpConnection : public nsISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_AHTTPCONNECTION_IID)

    
    
    

    
    
    
    
    
    
    
    virtual nsresult OnHeadersAvailable(nsAHttpTransaction *,
                                        nsHttpRequestHead *,
                                        nsHttpResponseHead *,
                                        bool *reset) = 0;

    
    
    
    
    
    virtual nsresult ResumeSend() = 0;
    virtual nsresult ResumeRecv() = 0;

    
    
    virtual nsresult ForceSend() = 0;
    virtual nsresult ForceRecv() = 0;

    
    
    
    
    
    
    virtual void TransactionHasDataToWrite(nsAHttpTransaction *)
    {
        
        return;
    }
    
    
    
    
    
    
    
    
    
    
    virtual void CloseTransaction(nsAHttpTransaction *transaction,
                                  nsresult reason) = 0;

    
    virtual void GetConnectionInfo(nsHttpConnectionInfo **) = 0;

    
    
    virtual nsresult TakeTransport(nsISocketTransport **,
                                   nsIAsyncInputStream **,
                                   nsIAsyncOutputStream **) = 0;

    
    virtual void GetSecurityInfo(nsISupports **) = 0;

    
    
    virtual bool IsPersistent() = 0;

    
    virtual bool IsReused() = 0;
    virtual void DontReuse() = 0;

    
    
    virtual nsresult PushBack(const char *data, uint32_t length) = 0;

    
    
    
    virtual bool IsProxyConnectInProgress() = 0;

    
    
    virtual bool LastTransactionExpectedNoContent() = 0;
    virtual void   SetLastTransactionExpectedNoContent(bool) = 0;

    
    
    virtual nsHttpConnection *TakeHttpConnection() = 0;

    
    
    virtual nsISocketTransport *Transport() = 0;

    
    
    virtual uint32_t CancelPipeline(nsresult originalReason) = 0;

    
    virtual nsAHttpTransaction::Classifier Classification() = 0;
    virtual void Classify(nsAHttpTransaction::Classifier newclass) = 0;

    
    
    virtual int64_t BytesWritten() = 0;

    
    
    virtual void SetSecurityCallbacks(nsIInterfaceRequestor* aCallbacks) = 0;

    
    virtual uint32_t Version() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsAHttpConnection, NS_AHTTPCONNECTION_IID)

#define NS_DECL_NSAHTTPCONNECTION(fwdObject)                    \
    nsresult OnHeadersAvailable(nsAHttpTransaction *, nsHttpRequestHead *, nsHttpResponseHead *, bool *reset) override; \
    void CloseTransaction(nsAHttpTransaction *, nsresult) override; \
    nsresult TakeTransport(nsISocketTransport **,    \
                           nsIAsyncInputStream **,   \
                           nsIAsyncOutputStream **) override; \
    bool IsPersistent() override;                         \
    bool IsReused() override;                             \
    void DontReuse() override;                            \
    nsresult PushBack(const char *, uint32_t) override;   \
    nsHttpConnection *TakeHttpConnection() override;      \
    uint32_t CancelPipeline(nsresult originalReason) override; \
    nsAHttpTransaction::Classifier Classification() override; \
    /*                                                    \
       Thes methods below have automatic definitions that just forward the \
       function to a lower level connection object        \
    */                                                    \
    void GetConnectionInfo(nsHttpConnectionInfo **result) \
      override                                            \
    {                                                     \
      if (!(fwdObject)) {                                 \
          *result = nullptr;                              \
          return;                                         \
      }                                                   \
        return (fwdObject)->GetConnectionInfo(result);    \
    }                                                     \
    void GetSecurityInfo(nsISupports **result) override   \
    {                                                     \
      if (!(fwdObject)) {                                 \
          *result = nullptr;                              \
          return;                                         \
      }                                                   \
      return (fwdObject)->GetSecurityInfo(result);        \
    }                                                     \
    nsresult ResumeSend() override     \
    {                                      \
        if (!(fwdObject))                  \
            return NS_ERROR_FAILURE;       \
        return (fwdObject)->ResumeSend();  \
    }                                      \
    nsresult ResumeRecv() override     \
    {                                      \
        if (!(fwdObject))                  \
            return NS_ERROR_FAILURE;       \
        return (fwdObject)->ResumeRecv();  \
    }                                      \
    nsresult ForceSend() override      \
    {                                      \
        if (!(fwdObject))                  \
            return NS_ERROR_FAILURE;       \
        return (fwdObject)->ForceSend();   \
    }                                      \
    nsresult ForceRecv() override      \
    {                                      \
        if (!(fwdObject))                  \
            return NS_ERROR_FAILURE;       \
        return (fwdObject)->ForceRecv();   \
    }                                      \
    nsISocketTransport *Transport()        \
      override                         \
    {                                      \
        if (!(fwdObject))                  \
            return nullptr;                 \
        return (fwdObject)->Transport();   \
    }                                      \
    uint32_t Version() override        \
    {                                      \
        return (fwdObject) ?               \
            (fwdObject)->Version() :       \
            NS_HTTP_VERSION_UNKNOWN;       \
    }                                      \
    bool IsProxyConnectInProgress() override                \
    {                                                       \
        return (fwdObject)->IsProxyConnectInProgress();     \
    }                                                       \
    bool LastTransactionExpectedNoContent() override        \
    {                                                       \
        return (fwdObject)->LastTransactionExpectedNoContent(); \
    }                                                       \
    void SetLastTransactionExpectedNoContent(bool val)      \
      override                                              \
    {                                                       \
        return (fwdObject)->SetLastTransactionExpectedNoContent(val); \
    }                                                       \
    void Classify(nsAHttpTransaction::Classifier newclass)  \
      override                                              \
    {                                                       \
    if (fwdObject)                                          \
        return (fwdObject)->Classify(newclass);             \
    }                                                       \
    int64_t BytesWritten() override                         \
    {     return fwdObject ? (fwdObject)->BytesWritten() : 0; } \
    void SetSecurityCallbacks(nsIInterfaceRequestor* aCallbacks) \
      override                                              \
    {                                                       \
        if (fwdObject)                                      \
            (fwdObject)->SetSecurityCallbacks(aCallbacks);  \
    }

} 
} 

#endif 
