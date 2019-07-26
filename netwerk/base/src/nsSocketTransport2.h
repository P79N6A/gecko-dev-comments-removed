



#ifndef nsSocketTransport2_h__
#define nsSocketTransport2_h__

#ifdef DEBUG_darinf
#define ENABLE_SOCKET_TRACING
#endif

#include "mozilla/Mutex.h"
#include "nsSocketTransportService2.h"
#include "nsString.h"
#include "nsCOMPtr.h"

#include "nsISocketTransport.h"
#include "nsIInterfaceRequestor.h"
#include "nsIAsyncInputStream.h"
#include "nsIAsyncOutputStream.h"
#include "nsIDNSListener.h"
#include "nsIDNSRecord.h"
#include "nsICancelable.h"
#include "nsIClassInfo.h"
#include "mozilla/net/DNS.h"

#include "prerror.h"

class nsSocketTransport;

nsresult
ErrorAccordingToNSPR(PRErrorCode errorCode);




#define NS_SOCKET_CONNECT_TIMEOUT PR_MillisecondsToInterval(20)



class nsSocketInputStream : public nsIAsyncInputStream
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIINPUTSTREAM
    NS_DECL_NSIASYNCINPUTSTREAM

    nsSocketInputStream(nsSocketTransport *);
    virtual ~nsSocketInputStream();

    bool     IsReferenced() { return mReaderRefCnt > 0; }
    nsresult Condition()    { return mCondition; }
    uint64_t ByteCount()    { return mByteCount; }

    
    void OnSocketReady(nsresult condition);

private:
    nsSocketTransport               *mTransport;
    nsrefcnt                         mReaderRefCnt;

    
    nsresult                         mCondition;
    nsCOMPtr<nsIInputStreamCallback> mCallback;
    uint32_t                         mCallbackFlags;
    uint64_t                         mByteCount;
};



class nsSocketOutputStream : public nsIAsyncOutputStream
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIOUTPUTSTREAM
    NS_DECL_NSIASYNCOUTPUTSTREAM

    nsSocketOutputStream(nsSocketTransport *);
    virtual ~nsSocketOutputStream();

    bool     IsReferenced() { return mWriterRefCnt > 0; }
    nsresult Condition()    { return mCondition; }
    uint64_t ByteCount()    { return mByteCount; }

    
    void OnSocketReady(nsresult condition); 

private:
    static NS_METHOD WriteFromSegments(nsIInputStream *, void *,
                                       const char *, uint32_t offset,
                                       uint32_t count, uint32_t *countRead);

    nsSocketTransport                *mTransport;
    nsrefcnt                          mWriterRefCnt;

    
    nsresult                          mCondition;
    nsCOMPtr<nsIOutputStreamCallback> mCallback;
    uint32_t                          mCallbackFlags;
    uint64_t                          mByteCount;
};



class nsSocketTransport : public nsASocketHandler
                        , public nsISocketTransport
                        , public nsIDNSListener
                        , public nsIClassInfo
{
    typedef mozilla::Mutex Mutex;

public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSITRANSPORT
    NS_DECL_NSISOCKETTRANSPORT
    NS_DECL_NSIDNSLISTENER
    NS_DECL_NSICLASSINFO

    nsSocketTransport();

    
    
    nsresult Init(const char **socketTypes, uint32_t typeCount,
                  const nsACString &host, uint16_t port,
                  nsIProxyInfo *proxyInfo);

    
    
    nsresult InitWithConnectedSocket(PRFileDesc *socketFD,
                                     const mozilla::net::NetAddr *addr);

    
    void OnSocketReady(PRFileDesc *, int16_t outFlags); 
    void OnSocketDetached(PRFileDesc *);
    void IsLocal(bool *aIsLocal);

    
    void OnSocketEvent(uint32_t type, nsresult status, nsISupports *param);

    uint64_t ByteCountReceived() { return mInput.ByteCount(); }
    uint64_t ByteCountSent() { return mOutput.ByteCount(); }
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
    nsresult PostEvent(uint32_t type, nsresult status = NS_OK, nsISupports *param = nullptr);

    enum {
        STATE_CLOSED,
        STATE_IDLE,
        STATE_RESOLVING,
        STATE_CONNECTING,
        STATE_TRANSFERRING
    };

    
    
    
    

    
    char       **mTypes;
    uint32_t     mTypeCount;
    nsCString    mHost;
    nsCString    mProxyHost;
    uint16_t     mPort;
    uint16_t     mProxyPort;
    bool mProxyTransparent;
    bool mProxyTransparentResolvesHost;
    uint32_t     mConnectionFlags;
    
    uint16_t         SocketPort() { return (!mProxyHost.IsEmpty() && !mProxyTransparent) ? mProxyPort : mPort; }
    const nsCString &SocketHost() { return (!mProxyHost.IsEmpty() && !mProxyTransparent) ? mProxyHost : mHost; }

    
    
    
    

    
    uint32_t     mState;     
    bool mAttached;
    bool mInputClosed;
    bool mOutputClosed;

    
    
    bool mResolving;

    nsCOMPtr<nsICancelable> mDNSRequest;
    nsCOMPtr<nsIDNSRecord>  mDNSRecord;

    
    
    mozilla::net::NetAddr   mNetAddr;
    bool                    mNetAddrIsSet;

    

    void     SendStatus(nsresult status);
    nsresult ResolveHost();
    nsresult BuildSocket(PRFileDesc *&, bool &, bool &); 
    nsresult InitiateSocket();
    bool     RecoverFromError();

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

    
    
    

    Mutex       mLock;  
    PRFileDesc *mFD;
    nsrefcnt    mFDref;       
    bool        mFDconnected; 

    nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
    nsCOMPtr<nsITransportEventSink> mEventSink;
    nsCOMPtr<nsISupports>           mSecInfo;

    nsSocketInputStream  mInput;
    nsSocketOutputStream mOutput;

    friend class nsSocketInputStream;
    friend class nsSocketOutputStream;

    
    uint16_t mTimeouts[2];

    
    uint8_t mQoSBits;

    
    
    
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
    void TraceInBuf(const char *buf, int32_t n);
    void TraceOutBuf(const char *buf, int32_t n);
#endif
};

#endif 
