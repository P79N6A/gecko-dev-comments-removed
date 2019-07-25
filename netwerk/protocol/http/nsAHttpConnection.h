




































#ifndef nsAHttpConnection_h__
#define nsAHttpConnection_h__

#include "nsISupports.h"

class nsAHttpTransaction;
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

    
    
    
    
    
    
    
    
    
    
    virtual void CloseTransaction(nsAHttpTransaction *transaction,
                                  nsresult reason) = 0;

    
    virtual void GetConnectionInfo(nsHttpConnectionInfo **) = 0;

    
    
    virtual nsresult TakeTransport(nsISocketTransport **,
                                   nsIAsyncInputStream **,
                                   nsIAsyncOutputStream **) = 0;

    
    virtual void GetSecurityInfo(nsISupports **) = 0;

    
    
    virtual bool IsPersistent() = 0;

    
    virtual bool IsReused() = 0;
    
    
    
    virtual nsresult PushBack(const char *data, PRUint32 length) = 0;

    
    
    virtual bool LastTransactionExpectedNoContent() = 0;
    virtual void   SetLastTransactionExpectedNoContent(bool) = 0;

    
    
    virtual nsHttpConnection *TakeHttpConnection() = 0;

    
    
    virtual nsISocketTransport *Transport() = 0;
};

#define NS_DECL_NSAHTTPCONNECTION \
    nsresult OnHeadersAvailable(nsAHttpTransaction *, nsHttpRequestHead *, nsHttpResponseHead *, bool *reset); \
    nsresult ResumeSend(); \
    nsresult ResumeRecv(); \
    void CloseTransaction(nsAHttpTransaction *, nsresult); \
    void GetConnectionInfo(nsHttpConnectionInfo **); \
    nsresult TakeTransport(nsISocketTransport **,    \
                           nsIAsyncInputStream **,   \
                           nsIAsyncOutputStream **); \
    void GetSecurityInfo(nsISupports **); \
    bool IsPersistent(); \
    bool IsReused(); \
    nsresult PushBack(const char *, PRUint32); \
    bool LastTransactionExpectedNoContent(); \
    void   SetLastTransactionExpectedNoContent(bool); \
    nsHttpConnection *TakeHttpConnection(); \
    nsISocketTransport *Transport();

#endif 
