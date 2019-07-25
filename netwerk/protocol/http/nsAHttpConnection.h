



#ifndef nsAHttpConnection_h__
#define nsAHttpConnection_h__

#include "nsISupports.h"
#include "nsAHttpTransaction.h"

class nsHttpRequestHead;
class nsHttpResponseHead;
class nsHttpConnectionInfo;
class nsHttpConnection;
class nsISocketTransport;
class nsIAsyncInputStream;
class nsIAsyncOutputStream;





class nsAHttpConnection : public nsISupports
{
public:
    
    
    

    
    
    
    
    
    
    
    virtual nsresult OnHeadersAvailable(nsAHttpTransaction *,
                                        nsHttpRequestHead *,
                                        nsHttpResponseHead *,
                                        bool *reset) = 0;

    
    
    
    
    
    virtual nsresult ResumeSend() = 0;
    virtual nsresult ResumeRecv() = 0;

    
    
    
    
    
    
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

    
    
    virtual nsresult PushBack(const char *data, PRUint32 length) = 0;

    
    
    
    virtual bool IsProxyConnectInProgress() = 0;

    
    
    virtual bool LastTransactionExpectedNoContent() = 0;
    virtual void   SetLastTransactionExpectedNoContent(bool) = 0;

    
    
    virtual nsHttpConnection *TakeHttpConnection() = 0;

    
    
    virtual nsISocketTransport *Transport() = 0;

    
    
    virtual PRUint32 CancelPipeline(nsresult originalReason) = 0;

    
    virtual nsAHttpTransaction::Classifier Classification() = 0;
    virtual void Classify(nsAHttpTransaction::Classifier newclass) = 0;

    
    
    virtual PRInt64 BytesWritten() = 0;
};

#define NS_DECL_NSAHTTPCONNECTION(fwdObject)                    \
    nsresult OnHeadersAvailable(nsAHttpTransaction *, nsHttpRequestHead *, nsHttpResponseHead *, bool *reset); \
    void CloseTransaction(nsAHttpTransaction *, nsresult); \
    nsresult TakeTransport(nsISocketTransport **,    \
                           nsIAsyncInputStream **,   \
                           nsIAsyncOutputStream **); \
    bool IsPersistent(); \
    bool IsReused(); \
    void DontReuse();  \
    nsresult PushBack(const char *, PRUint32); \
    nsHttpConnection *TakeHttpConnection(); \
    PRUint32 CancelPipeline(nsresult originalReason);   \
    nsAHttpTransaction::Classifier Classification();      \
    /*                                                    \
       Thes methods below have automatic definitions that just forward the \
       function to a lower level connection object        \
    */                                                    \
    void GetConnectionInfo(nsHttpConnectionInfo **result) \
    {                                                     \
      if (!(fwdObject)) {                                 \
          *result = nsnull;                               \
          return;                                         \
      }                                                   \
        return (fwdObject)->GetConnectionInfo(result);    \
    }                                                     \
    void GetSecurityInfo(nsISupports **result)            \
    {                                                     \
      if (!(fwdObject)) {                                 \
          *result = nsnull;                               \
          return;                                         \
      }                                                   \
      return (fwdObject)->GetSecurityInfo(result);        \
    }                                                     \
    nsresult ResumeSend()                  \
    {                                      \
        if (!(fwdObject))                  \
            return NS_ERROR_FAILURE;       \
        return (fwdObject)->ResumeSend();  \
    }                                      \
    nsresult ResumeRecv()                  \
    {                                      \
        if (!(fwdObject))                  \
            return NS_ERROR_FAILURE;       \
        return (fwdObject)->ResumeRecv();  \
    }                                      \
    nsISocketTransport *Transport()        \
    {                                      \
        if (!(fwdObject))                  \
            return nsnull;                 \
        return (fwdObject)->Transport();   \
    }                                      \
    bool IsProxyConnectInProgress()                         \
    {                                                       \
        return (fwdObject)->IsProxyConnectInProgress();     \
    }                                                       \
    bool LastTransactionExpectedNoContent()                 \
    {                                                       \
        return (fwdObject)->LastTransactionExpectedNoContent(); \
    }                                                       \
    void SetLastTransactionExpectedNoContent(bool val)      \
    {                                                       \
        return (fwdObject)->SetLastTransactionExpectedNoContent(val); \
    }                                                       \
    void Classify(nsAHttpTransaction::Classifier newclass)  \
    {                                                       \
    if (fwdObject)                                          \
        return (fwdObject)->Classify(newclass);             \
    }                                                       \
    PRInt64 BytesWritten()                                  \
    {     return fwdObject ? (fwdObject)->BytesWritten() : 0; }

#endif 
