




































#ifndef nsSocketTransport2_h__
#define nsSocketTransport2_h__

#ifdef DEBUG_darinf
#define ENABLE_SOCKET_TRACING
#endif

#include "nsSocketTransportService2.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsInt64.h"

#include "nsISocketTransport.h"
#include "nsIInterfaceRequestor.h"
#include "nsIAsyncInputStream.h"
#include "nsIAsyncOutputStream.h"
#include "nsIDNSListener.h"
#include "nsIDNSRecord.h"
#include "nsICancelable.h"
#include "nsIClassInfo.h"

class nsSocketTransport;




#define NS_SOCKET_CONNECT_TIMEOUT PR_MillisecondsToInterval(20)



class nsSocketInputStream : public nsIAsyncInputStream
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIINPUTSTREAM
    NS_DECL_NSIASYNCINPUTSTREAM

    nsSocketInputStream(nsSocketTransport *);
    virtual ~nsSocketInputStream();

    PRBool   IsReferenced() { return mReaderRefCnt > 0; }
    nsresult Condition()    { return mCondition; }
    PRUint64 ByteCount()    { return mByteCount; }

    
    void OnSocketReady(nsresult condition);

private:
    nsSocketTransport               *mTransport;
    nsrefcnt                         mReaderRefCnt;

    
    nsresult                         mCondition;
    nsCOMPtr<nsIInputStreamCallback> mCallback;
    PRUint32                         mCallbackFlags;
    PRUint64                         mByteCount;
};



class nsSocketOutputStream : public nsIAsyncOutputStream
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIOUTPUTSTREAM
    NS_DECL_NSIASYNCOUTPUTSTREAM

    nsSocketOutputStream(nsSocketTransport *);
    virtual ~nsSocketOutputStream();

    PRBool   IsReferenced() { return mWriterRefCnt > 0; }
    nsresult Condition()    { return mCondition; }
    PRUint64 ByteCount()    { return mByteCount; }

    
    void OnSocketReady(nsresult condition); 

private:
    static NS_METHOD WriteFromSegments(nsIInputStream *, void *,
                                       const char *, PRUint32 offset,
                                       PRUint32 count, PRUint32 *countRead);

    nsSocketTransport                *mTransport;
    nsrefcnt                          mWriterRefCnt;

    
    nsresult                          mCondition;
    nsCOMPtr<nsIOutputStreamCallback> mCallback;
    PRUint32                          mCallbackFlags;
    PRUint64                          mByteCount;
};



class nsSocketTransport : public nsASocketHandler
                        , public nsISocketTransport
                        , public nsIDNSListener
                        , public nsIClassInfo
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSITRANSPORT
    NS_DECL_NSISOCKETTRANSPORT
    NS_DECL_NSIDNSLISTENER
    NS_DECL_NSICLASSINFO

    nsSocketTransport();

    
    
    nsresult Init(const char **socketTypes, PRUint32 typeCount,
                  const nsACString &host, PRUint16 port,
                  nsIProxyInfo *proxyInfo);

    
    
    nsresult InitWithConnectedSocket(PRFileDesc *socketFD,
                                     const PRNetAddr *addr);

    
    void OnSocketReady(PRFileDesc *, PRInt16 outFlags); 
    void OnSocketDetached(PRFileDesc *);

    
    void OnSocketEvent(PRUint32 type, nsresult status, nsISupports *param);

protected:

    virtual ~nsSocketTransport();

private:

    
    enum {
        MSG_ENSURE_CONNECT,
        MSG_DNS_LOOKUP_COMPLETE,
        MSG_RETRY_INIT_SOCKET,
        MSG_TIMEOUT_CHANGED,
        MSG_INPUT_CLOSED,
        MSG_INPUT_PENDING,
        MSG_OUTPUT_CLOSED,
        MSG_OUTPUT_PENDING
    };
    nsresult PostEvent(PRUint32 type, nsresult status = NS_OK, nsISupports *param = nsnull);

    enum {
        STATE_CLOSED,
        STATE_IDLE,
        STATE_RESOLVING,
        STATE_CONNECTING,
        STATE_TRANSFERRING
    };

    
    
    
    

    
    char       **mTypes;
    PRUint32     mTypeCount;
    nsCString    mHost;
    nsCString    mProxyHost;
    PRUint16     mPort;
    PRUint16     mProxyPort;
    PRPackedBool mProxyTransparent;
    PRPackedBool mProxyTransparentResolvesHost;
    PRUint32     mConnectionFlags;
    
    PRUint16         SocketPort() { return (!mProxyHost.IsEmpty() && !mProxyTransparent) ? mProxyPort : mPort; }
    const nsCString &SocketHost() { return (!mProxyHost.IsEmpty() && !mProxyTransparent) ? mProxyHost : mHost; }

    
    
    
    

    
    PRUint32     mState;     
    PRPackedBool mAttached;
    PRPackedBool mInputClosed;
    PRPackedBool mOutputClosed;

    
    
    PRPackedBool mResolving;

    nsCOMPtr<nsICancelable> mDNSRequest;
    nsCOMPtr<nsIDNSRecord>  mDNSRecord;
    PRNetAddr               mNetAddr;

    

    void     SendStatus(nsresult status);
    nsresult ResolveHost();
    nsresult BuildSocket(PRFileDesc *&, PRBool &, PRBool &); 
    nsresult InitiateSocket();
    PRBool   RecoverFromError();

    void OnMsgInputPending()
    {
        if (mState == STATE_TRANSFERRING)
            mPollFlags |= (PR_POLL_READ | PR_POLL_EXCEPT);
    }
    void OnMsgOutputPending()
    {
        if (mState == STATE_TRANSFERRING)
            mPollFlags |= (PR_POLL_WRITE | PR_POLL_EXCEPT);
    }
    void OnMsgInputClosed(nsresult reason);
    void OnMsgOutputClosed(nsresult reason);

    
    void OnSocketConnected();

    
    
    

    PRLock     *mLock;  
    PRFileDesc *mFD;
    nsrefcnt    mFDref;       
    PRBool      mFDconnected; 

    nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
    nsCOMPtr<nsITransportEventSink> mEventSink;
    nsCOMPtr<nsISupports>           mSecInfo;

    nsSocketInputStream  mInput;
    nsSocketOutputStream mOutput;

    friend class nsSocketInputStream;
    friend class nsSocketOutputStream;

    
    PRUint16 mTimeouts[2];

    
    
    
    PRFileDesc *GetFD_Locked();
    void        ReleaseFD_Locked(PRFileDesc *fd);

    
    
    
    void OnInputClosed(nsresult reason)
    {
        
        if (PR_GetCurrentThread() == gSocketThread)
            OnMsgInputClosed(reason);
        else
            PostEvent(MSG_INPUT_CLOSED, reason);
    }
    void OnInputPending()
    {
        
        if (PR_GetCurrentThread() == gSocketThread)
            OnMsgInputPending();
        else
            PostEvent(MSG_INPUT_PENDING);
    }
    void OnOutputClosed(nsresult reason)
    {
        
        if (PR_GetCurrentThread() == gSocketThread)
            OnMsgOutputClosed(reason); 
        else
            PostEvent(MSG_OUTPUT_CLOSED, reason);
    }
    void OnOutputPending()
    {
        
        if (PR_GetCurrentThread() == gSocketThread)
            OnMsgOutputPending();
        else
            PostEvent(MSG_OUTPUT_PENDING);
    }

#ifdef ENABLE_SOCKET_TRACING
    void TraceInBuf(const char *buf, PRInt32 n);
    void TraceOutBuf(const char *buf, PRInt32 n);
#endif
};

#endif 
