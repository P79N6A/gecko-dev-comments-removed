




































#ifndef nsAHttpTransaction_h__
#define nsAHttpTransaction_h__

#include "nsISupports.h"

class nsAHttpConnection;
class nsAHttpSegmentReader;
class nsAHttpSegmentWriter;
class nsIInterfaceRequestor;
class nsIEventTarget;
class nsITransport;
class nsHttpRequestHead;










class nsAHttpTransaction : public nsISupports
{
public:
    
    virtual void SetConnection(nsAHttpConnection *) = 0;

    
    
    virtual void GetSecurityCallbacks(nsIInterfaceRequestor **,
                                      nsIEventTarget **) = 0;

    
    virtual void OnTransportStatus(nsITransport* transport,
                                   nsresult status, PRUint64 progress) = 0;

    
    virtual PRBool   IsDone() = 0;
    virtual nsresult Status() = 0;

    
    virtual PRUint32 Available() = 0;

    
    virtual nsresult ReadSegments(nsAHttpSegmentReader *reader,
                                  PRUint32 count, PRUint32 *countRead) = 0;

    
    virtual nsresult WriteSegments(nsAHttpSegmentWriter *writer,
                                   PRUint32 count, PRUint32 *countWritten) = 0;

    
    virtual void Close(nsresult reason) = 0;

    
    virtual void SetSSLConnectFailed() = 0;
    
    
    virtual nsHttpRequestHead *RequestHead() = 0;
};

#define NS_DECL_NSAHTTPTRANSACTION \
    void SetConnection(nsAHttpConnection *); \
    void GetSecurityCallbacks(nsIInterfaceRequestor **, \
                              nsIEventTarget **);       \
    void OnTransportStatus(nsITransport* transport, \
                           nsresult status, PRUint64 progress); \
    PRBool   IsDone(); \
    nsresult Status(); \
    PRUint32 Available(); \
    nsresult ReadSegments(nsAHttpSegmentReader *, PRUint32, PRUint32 *); \
    nsresult WriteSegments(nsAHttpSegmentWriter *, PRUint32, PRUint32 *); \
    void     Close(nsresult reason);                                    \
    void     SetSSLConnectFailed();                                     \
    nsHttpRequestHead *RequestHead();





class nsAHttpSegmentReader
{
public:
    
    virtual nsresult OnReadSegment(const char *segment,
                                   PRUint32 count,
                                   PRUint32 *countRead) = 0;
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
