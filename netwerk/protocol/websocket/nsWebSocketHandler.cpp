






































#include "nsWebSocketHandler.h"

#include "nsISocketTransportService.h"
#include "nsIURI.h"
#include "nsIChannel.h"
#include "nsICryptoHash.h"
#include "nsIRunnable.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsICancelable.h"
#include "nsIDNSRecord.h"
#include "nsIDNSService.h"
#include "nsIStreamConverterService.h"
#include "nsIIOService2.h"
#include "nsIProtocolProxyService.h"

#include "nsAutoPtr.h"
#include "nsStandardURL.h"
#include "nsNetCID.h"
#include "nsServiceManagerUtils.h"
#include "nsXPIDLString.h"
#include "nsCRT.h"
#include "nsThreadUtils.h"
#include "nsNetError.h"
#include "nsStringStream.h"
#include "nsAlgorithm.h"
#include "nsProxyRelease.h"

#include "plbase64.h"
#include "prmem.h"
#include "prnetdb.h"
#include "prbit.h"
#include "prlog.h"
#include "zlib.h"

extern PRThread *gSocketThread;

namespace mozilla {
namespace net {

NS_IMPL_THREADSAFE_ISUPPORTS11(nsWebSocketHandler,
                               nsIWebSocketProtocol,
                               nsIHttpUpgradeListener,
                               nsIRequestObserver,
                               nsIStreamListener,
                               nsIProtocolHandler,
                               nsIInputStreamCallback,
                               nsIOutputStreamCallback,
                               nsITimerCallback,
                               nsIDNSListener,
                               nsIInterfaceRequestor,
                               nsIChannelEventSink)

#if defined(PR_LOGGING)
static PRLogModuleInfo *webSocketLog = nsnull;
#endif
#define LOG(args) PR_LOG(webSocketLog, PR_LOG_DEBUG, args)



#define kFinMessage (reinterpret_cast<nsCString *>(0x01))


#define SEC_WEBSOCKET_VERSION "8"
















class CallOnMessageAvailable : public nsIRunnable
{
public:
    NS_DECL_ISUPPORTS
        
    CallOnMessageAvailable(nsIWebSocketListener *aListener,
                           nsISupports          *aContext,
                           nsCString            &aData,
                           PRInt32               aLen)
      : mListener(aListener),
        mContext(aContext),
        mData(aData),
        mLen(aLen) {}
    
    NS_SCRIPTABLE NS_IMETHOD Run()
    {
        if (mLen < 0)
            mListener->OnMessageAvailable(mContext, mData);
        else
            mListener->OnBinaryMessageAvailable(mContext, mData);
        return NS_OK;
    }

private:
    ~CallOnMessageAvailable() {}

    nsCOMPtr<nsIWebSocketListener>    mListener;
    nsCOMPtr<nsISupports>             mContext;
    nsCString                         mData;
    PRInt32                           mLen;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(CallOnMessageAvailable, nsIRunnable)

class CallOnStop : public nsIRunnable
{
public:
    NS_DECL_ISUPPORTS
        
    CallOnStop(nsIWebSocketListener *aListener,
               nsISupports          *aContext,
               nsresult              aData)
    : mListener(aListener),
      mContext(aContext),
      mData(aData) {}
    
    NS_SCRIPTABLE NS_IMETHOD Run()
    {
        mListener->OnStop(mContext, mData);
        return NS_OK;
    }

private:
    ~CallOnStop() {}

    nsCOMPtr<nsIWebSocketListener>    mListener;
    nsCOMPtr<nsISupports>             mContext;
    nsresult                          mData;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(CallOnStop, nsIRunnable)

class CallOnServerClose : public nsIRunnable
{
public:
    NS_DECL_ISUPPORTS
        
    CallOnServerClose(nsIWebSocketListener *aListener,
                      nsISupports          *aContext)
    : mListener(aListener),
      mContext(aContext) {}
    
    NS_SCRIPTABLE NS_IMETHOD Run()
    {
        mListener->OnServerClose(mContext);
        return NS_OK;
    }

private:
    ~CallOnServerClose() {}

    nsCOMPtr<nsIWebSocketListener>    mListener;
    nsCOMPtr<nsISupports>             mContext;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(CallOnServerClose, nsIRunnable)

class CallAcknowledge : public nsIRunnable
{
public:
    NS_DECL_ISUPPORTS
        
    CallAcknowledge(nsIWebSocketListener *aListener,
                    nsISupports          *aContext,
                    PRUint32              aSize)
    : mListener(aListener),
      mContext(aContext),
      mSize(aSize) {}

    NS_SCRIPTABLE NS_IMETHOD Run()
    {
        LOG(("WebSocketHandler::CallAcknowledge Size %u\n", mSize));
        mListener->OnAcknowledge(mContext, mSize);
        return NS_OK;
    }
    
private:
    ~CallAcknowledge() {}

    nsCOMPtr<nsIWebSocketListener>    mListener;
    nsCOMPtr<nsISupports>             mContext;
    PRUint32                          mSize;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(CallAcknowledge, nsIRunnable)

class nsPostMessage : public nsIRunnable
{
public:
    NS_DECL_ISUPPORTS
        
    nsPostMessage(nsWebSocketHandler *handler,
                  nsCString          *aData,
                  PRInt32             aDataLen)
        : mHandler(handler),
        mData(aData),
        mDataLen(aDataLen) {}
    
    NS_SCRIPTABLE NS_IMETHOD Run()
    {
        if (mData)
            mHandler->SendMsgInternal(mData, mDataLen);
        return NS_OK;
    }

private:
    ~nsPostMessage() {}
    
    nsRefPtr<nsWebSocketHandler>    mHandler;
    nsCString                      *mData;
    PRInt32                         mDataLen;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(nsPostMessage, nsIRunnable)






class nsWSAdmissionManager
{
public:
    nsWSAdmissionManager()
        : mConnectedCount(0)
    {
        MOZ_COUNT_CTOR(nsWSAdmissionManager);
    }

    class nsOpenConn
    {
    public:
        nsOpenConn(nsCString &addr, nsWebSocketHandler *handler)
            : mAddress(addr), mHandler(handler)
        { MOZ_COUNT_CTOR(nsOpenConn); }
        ~nsOpenConn() {MOZ_COUNT_DTOR(nsOpenConn); }
        
        nsCString mAddress;
        nsRefPtr<nsWebSocketHandler> mHandler;
    };
    
    ~nsWSAdmissionManager()
    {
        MOZ_COUNT_DTOR(nsWSAdmissionManager);
        for (PRUint32 i = 0; i < mData.Length(); i++)
            delete mData[i];
    }

    PRBool ConditionallyConnect(nsCString &aStr, nsWebSocketHandler *ws)
    {
        NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

        
        
        
        

        
        
        
        PRBool found = (IndexOf(aStr) >= 0);
        nsOpenConn *newdata = new nsOpenConn(aStr, ws);
        mData.AppendElement(newdata);

        if (!found)
            ws->BeginOpen();
        return !found;
    }

    PRBool Complete(nsCString &aStr)
    {
        NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");
        PRInt32 index = IndexOf(aStr);
        NS_ABORT_IF_FALSE(index >= 0, "completed connection not in open list");
        
        nsOpenConn *olddata = mData[index];
        mData.RemoveElementAt(index);
        delete olddata;
        
        
        index = IndexOf(aStr);
        if (index >= 0) {
            (mData[index])->mHandler->BeginOpen();
            return PR_TRUE;
        }
        return PR_FALSE;
    }

    void IncrementConnectedCount()
    {
        PR_ATOMIC_INCREMENT(&mConnectedCount);
    }

    void DecrementConnectedCount()
    {
        PR_ATOMIC_DECREMENT(&mConnectedCount);
    }

    PRInt32 ConnectedCount()
    {
        return mConnectedCount;
    }
    
private:
    nsTArray<nsOpenConn *> mData;

    PRInt32 IndexOf(nsCString &aStr)
    {
        for (PRUint32 i = 0; i < mData.Length(); i++)
            if (aStr == (mData[i])->mAddress)
                return i;
        return -1;
    }
    
    
    
    PRInt32 mConnectedCount;
};






class nsWSCompression
{
public:
  nsWSCompression(nsIStreamListener *aListener,
                  nsISupports *aContext)
      : mActive(PR_FALSE),
        mContext(aContext),
        mListener(aListener)
    {
        MOZ_COUNT_CTOR(nsWSCompression);

        mZlib.zalloc = allocator;
        mZlib.zfree = destructor;
        mZlib.opaque = Z_NULL;
        
        
        
        
        
        
        
        nsresult rv;
        mStream = do_CreateInstance(NS_STRINGINPUTSTREAM_CONTRACTID, &rv);
        if (NS_SUCCEEDED(rv) && aContext && aListener &&
            deflateInit2(&mZlib, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                         -15, 8, Z_DEFAULT_STRATEGY) == Z_OK) {
            mActive = PR_TRUE;
        }
    }

    ~nsWSCompression()
    {
        MOZ_COUNT_DTOR(nsWSCompression);

        if (mActive)
            deflateEnd(&mZlib);
    }

    PRBool Active()
    {
        return mActive;
    }

    nsresult Deflate(PRUint8 *buf1, PRUint32 buf1Len,
                     PRUint8 *buf2, PRUint32 buf2Len)
    {
        NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread,
                          "not socket thread");
        NS_ABORT_IF_FALSE(mActive, "not active");
        
        mZlib.avail_out = kBufferLen;
        mZlib.next_out = mBuffer;
        mZlib.avail_in = buf1Len;
        mZlib.next_in = buf1;
        
        nsresult rv;
        
        while (mZlib.avail_in > 0) {
            deflate(&mZlib, (buf2Len > 0) ? Z_NO_FLUSH : Z_SYNC_FLUSH);
            rv = PushData();
            if (NS_FAILED(rv))
                return rv;
            mZlib.avail_out = kBufferLen;
            mZlib.next_out = mBuffer;
        }
        
        mZlib.avail_in = buf2Len;
        mZlib.next_in = buf2;

        while (mZlib.avail_in > 0) {
            deflate(&mZlib, Z_SYNC_FLUSH);
            rv = PushData();
            if (NS_FAILED(rv))
                return rv;
            mZlib.avail_out = kBufferLen;
            mZlib.next_out = mBuffer;
        }
        
        return NS_OK;
    }
    
private:
    
    
    static void *allocator(void *opaque, uInt items, uInt size)
    {
        return moz_xmalloc(items * size);
    }

    static void destructor(void *opaque, void *addr)
    {
        moz_free(addr);
    }

    nsresult PushData()
    {
        PRUint32 bytesToWrite = kBufferLen - mZlib.avail_out;
        if (bytesToWrite > 0) {
            mStream->ShareData(reinterpret_cast<char *>(mBuffer), bytesToWrite);
            nsresult rv;
            rv = mListener->OnDataAvailable(nsnull, mContext,
                                            mStream, 0, bytesToWrite);
            if (NS_FAILED(rv))
                return rv;
        }
        return NS_OK;
    }

    PRBool    mActive;
    z_stream  mZlib;
    nsCOMPtr<nsIStringInputStream>  mStream;

    nsISupports *mContext;                        
    nsIStreamListener *mListener;                 
    
    const static PRInt32 kBufferLen = 4096;
    PRUint8   mBuffer[kBufferLen];
};

static nsWSAdmissionManager *sWebSocketAdmissions = nsnull;



nsWebSocketHandler::nsWebSocketHandler() :
    mEncrypted(PR_FALSE),
    mCloseTimeout(20000),
    mOpenTimeout(20000),
    mPingTimeout(0),
    mPingResponseTimeout(10000),
    mMaxConcurrentConnections(200),
    mRecvdHttpOnStartRequest(0),
    mRecvdHttpUpgradeTransport(0),
    mRequestedClose(0),
    mClientClosed(0),
    mServerClosed(0),
    mStopped(0),
    mCalledOnStop(0),
    mPingOutstanding(0),
    mAllowCompression(1),
    mAutoFollowRedirects(0),
    mReleaseOnTransmit(0),
    mTCPClosed(0),
    mMaxMessageSize(16000000),
    mStopOnClose(NS_OK),
    mCloseCode(kCloseAbnormal),
    mFragmentOpcode(0),
    mFragmentAccumulator(0),
    mBuffered(0),
    mBufferSize(16384),
    mCurrentOut(nsnull),
    mCurrentOutSent(0),
    mCompressor(nsnull),
    mDynamicOutputSize(0),
    mDynamicOutput(nsnull)
{
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");
#if defined(PR_LOGGING)
    if (!webSocketLog)
        webSocketLog = PR_NewLogModule("nsWebSocket");
#endif

    LOG(("WebSocketHandler::nsWebSocketHandler() %p\n", this));
    
    if (!sWebSocketAdmissions)
        sWebSocketAdmissions = new nsWSAdmissionManager();

    mFramePtr = mBuffer = static_cast<PRUint8 *>(moz_xmalloc(mBufferSize));
}

nsWebSocketHandler::~nsWebSocketHandler()
{
    LOG(("WebSocketHandler::~nsWebSocketHandler() %p\n", this));

    
    mStopped = 1;
    StopSession(NS_ERROR_UNEXPECTED);
    
    moz_free(mBuffer);
    moz_free(mDynamicOutput);
    delete mCompressor;
    delete mCurrentOut;

    while ((mCurrentOut = (OutboundMessage *) mOutgoingPingMessages.PopFront()))
        delete mCurrentOut;
    while ((mCurrentOut = (OutboundMessage *) mOutgoingPongMessages.PopFront()))
        delete mCurrentOut;
    while ((mCurrentOut = (OutboundMessage *) mOutgoingMessages.PopFront()))
        delete mCurrentOut;

    nsCOMPtr<nsIThread> mainThread;
    nsIURI *forgettable;
    NS_GetMainThread(getter_AddRefs(mainThread));
    
    if (mURI) {
        mURI.forget(&forgettable);
        NS_ProxyRelease(mainThread, forgettable, PR_FALSE);
    }
    
    if (mOriginalURI) {
        mOriginalURI.forget(&forgettable);
        NS_ProxyRelease(mainThread, forgettable, PR_FALSE);
    }

    if (mListener) {
        nsIWebSocketListener *forgettableListener;
        mListener.forget(&forgettableListener);
        NS_ProxyRelease(mainThread, forgettableListener, PR_FALSE);
    }

    if (mContext) {
        nsISupports *forgettableContext;
        mContext.forget(&forgettableContext);
        NS_ProxyRelease(mainThread, forgettableContext, PR_FALSE);
    }
}

void
nsWebSocketHandler::Shutdown()
{
    delete sWebSocketAdmissions;
    sWebSocketAdmissions = nsnull;
}

nsresult
nsWebSocketHandler::BeginOpen()
{
    LOG(("WebSocketHandler::BeginOpen() %p\n", this));
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

    nsresult rv;

    if (mRedirectCallback) {
        LOG(("WebSocketHandler::BeginOpen Resuming Redirect\n"));
        rv = mRedirectCallback->OnRedirectVerifyCallback(NS_OK);
        mRedirectCallback = nsnull;
        return rv;
    }

    nsCOMPtr<nsIChannel> localChannel = do_QueryInterface(mChannel, &rv);
    if (NS_FAILED(rv)) {
        LOG(("WebSocketHandler::BeginOpen cannot async open\n"));
        AbortSession(NS_ERROR_CONNECTION_REFUSED);
        return rv;
    }

    rv = localChannel->AsyncOpen(this, mHttpChannel);
    if (NS_FAILED(rv)) {
        LOG(("WebSocketHandler::BeginOpen cannot async open\n"));
        AbortSession(NS_ERROR_CONNECTION_REFUSED);
        return rv;
    }

    mOpenTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    if (NS_SUCCEEDED(rv))
        mOpenTimer->InitWithCallback(this, mOpenTimeout,
                                     nsITimer::TYPE_ONE_SHOT);

    return rv;
}

PRBool
nsWebSocketHandler::IsPersistentFramePtr()
{
    return (mFramePtr >= mBuffer && mFramePtr < mBuffer + mBufferSize);
}



PRUint32
nsWebSocketHandler::UpdateReadBuffer(PRUint8 *buffer, PRUint32 count)
{
    LOG(("WebSocketHandler::UpdateReadBuffer() %p [%p %u]\n",
         this, buffer, count));

    if (!mBuffered)
        mFramePtr = mBuffer;
    
    NS_ABORT_IF_FALSE(IsPersistentFramePtr(),
                      "update read buffer bad mFramePtr");

    if (mBuffered + count <= mBufferSize) {
        
        LOG(("WebSocketHandler:: update read buffer absorbed %u\n", count));
    }
    else if (mBuffered + count - (mFramePtr - mBuffer) <= mBufferSize) {
        
        mBuffered -= (mFramePtr - mBuffer);
        LOG(("WebSocketHandler:: update read buffer shifted %u\n",
             mBuffered));
        ::memmove(mBuffer, mFramePtr, mBuffered);
        mFramePtr = mBuffer;
    }
    else {
        
        mBufferSize += count + 8192;
        LOG(("WebSocketHandler:: update read buffer extended to %u\n",
             mBufferSize));
        PRUint8 *old = mBuffer;
        mBuffer = (PRUint8 *)moz_xrealloc(mBuffer, mBufferSize);
        mFramePtr = mBuffer + (mFramePtr - old);
    }
    
    ::memcpy(mBuffer + mBuffered, buffer, count);
    mBuffered += count;
    
    return mBuffered - (mFramePtr - mBuffer);
}

nsresult
nsWebSocketHandler::ProcessInput(PRUint8 *buffer, PRUint32 count)
{
    LOG(("WebSocketHandler::ProcessInput %p [%d %d]\n",
         this, count, mBuffered));
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread,
                      "not socket thread");
    
    
    if (mPingTimer) {
        
        
        
        
        
        mPingOutstanding = 0;
        mPingTimer->SetDelay(mPingTimeout);
    }

    PRUint32 avail;

    if (!mBuffered) {
        
        

        mFramePtr = buffer;
        avail = count;
    }
    else {
        avail = UpdateReadBuffer(buffer, count);
    }

    PRUint8 *payload;
    PRUint32 totalAvail = avail;

    while (avail >= 2) {

        PRInt64 payloadLength = mFramePtr[1] & 0x7F;
        PRUint8 finBit        = mFramePtr[0] & kFinalFragBit;
        PRUint8 rsvBits       = mFramePtr[0] & 0x70;
        PRUint8 maskBit       = mFramePtr[1] & kMaskBit;
        PRUint8 opcode        = mFramePtr[0] & 0x0F;

        PRUint32 framingLength = 2; 
        if (maskBit)
            framingLength += 4;

        if (payloadLength < 126) {
            if (avail < framingLength)
                break;
        }
        else if (payloadLength == 126) {
            
            framingLength += 2;
            if (avail < framingLength)
                break;

            payloadLength = mFramePtr[2] << 8 | mFramePtr[3];
        }
        else {
            
            framingLength += 8;
            if (avail < framingLength)
                break;

            if (mFramePtr[2] & 0x80) {
                
                
                LOG(("WebSocketHandler:: high bit of 64 bit length set"));
                AbortSession(NS_ERROR_ILLEGAL_VALUE);
                return NS_ERROR_ILLEGAL_VALUE;
            }

            
            PRUint64 tempLen;
            memcpy(&tempLen, mFramePtr + 2, 8);
            payloadLength = PR_ntohll(tempLen);
        }

        payload = mFramePtr + framingLength;
        avail -= framingLength;
        
        LOG(("WebSocketHandler:: ProcessInput payload %lld avail %lu\n",
             payloadLength, avail));

        
        
        if (payloadLength + mFragmentAccumulator > mMaxMessageSize) {
            AbortSession(NS_ERROR_FILE_TOO_BIG);
            return NS_ERROR_FILE_TOO_BIG;
        }
        
        if (avail < payloadLength)
            break;

        LOG(("WebSocketHandler::ProcessInput Frame accumulated - opcode %d\n",
             opcode));

        if (maskBit) {
            
            
            LOG(("WebSocketHandler:: Client RECEIVING masked frame."));

            PRUint32 mask;
            memcpy(&mask, payload - 4, 4);
            mask = PR_ntohl(mask);
            ApplyMask(mask, payload, payloadLength);
        }
        
        
        if (!finBit && (opcode & kControlFrameMask)) {
            LOG(("WebSocketHandler:: fragmented control frame code %d\n",
                 opcode));
            AbortSession(NS_ERROR_ILLEGAL_VALUE);
            return NS_ERROR_ILLEGAL_VALUE;
        }

        if (rsvBits) {
            LOG(("WebSocketHandler:: unexpected reserved bits %x\n", rsvBits));
            AbortSession(NS_ERROR_ILLEGAL_VALUE);
            return NS_ERROR_ILLEGAL_VALUE;
        }

        if (!finBit || opcode == kContinuation) {
            

            
            
            
            if ((mFragmentAccumulator != 0) && (opcode != kContinuation)) {
                LOG(("WebSocketHeandler:: nested fragments\n"));
                AbortSession(NS_ERROR_ILLEGAL_VALUE);
                return NS_ERROR_ILLEGAL_VALUE;
            }

            LOG(("WebSocketHandler:: Accumulating Fragment %lld\n",
                 payloadLength));
            
            if (opcode == kContinuation) {
                
                
                NS_ABORT_IF_FALSE(mFramePtr + framingLength == payload,
                                  "payload offset from frameptr wrong");
                ::memmove (mFramePtr, payload, avail);
                payload = mFramePtr;
                if (mBuffered)
                    mBuffered -= framingLength;
            }
            else {
                mFragmentOpcode = opcode;
            }
            
            if (finBit) {
                LOG(("WebSocketHandler:: Finalizing Fragment\n"));
                payload -= mFragmentAccumulator;
                payloadLength += mFragmentAccumulator;
                avail += mFragmentAccumulator;
                mFragmentAccumulator = 0;
                opcode = mFragmentOpcode;
            } else {
                opcode = kContinuation;
                mFragmentAccumulator += payloadLength;
            }
        }
        else if (mFragmentAccumulator != 0 && !(opcode & kControlFrameMask)) {
            
            
            
            LOG(("WebSocketHeandler:: illegal fragment sequence\n"));
            AbortSession(NS_ERROR_ILLEGAL_VALUE);
            return NS_ERROR_ILLEGAL_VALUE;
        }

        if (mServerClosed) {
            LOG(("WebSocketHandler:: ignoring read frame code %d after close\n",
                 opcode));
            
        }
        else if (mStopped) {
            LOG(("WebSocketHandler:: "
                 "ignoring read frame code %d after completion\n",
                 opcode));
        }
        else if (opcode == kText) {
            LOG(("WebSocketHandler:: text frame received\n"));
            if (mListener) {
                nsCString utf8Data((const char *)payload, payloadLength);

                
                
                
                
                if (!IsUTF8(utf8Data)) {
                    LOG(("WebSocketHandler:: text frame invalid utf-8\n"));
                    AbortSession(NS_ERROR_ILLEGAL_VALUE);
                    return NS_ERROR_ILLEGAL_VALUE;
                }

                nsCOMPtr<nsIRunnable> event =
                    new CallOnMessageAvailable(mListener, mContext,
                                               utf8Data, -1);
                NS_DispatchToMainThread(event);
            }
        }
        else if (opcode & kControlFrameMask) {
            
            if (payloadLength > 125) {
                LOG(("WebSocketHandler:: bad control frame code %d length %d\n",
                     opcode, payloadLength));
                AbortSession(NS_ERROR_ILLEGAL_VALUE);
                return NS_ERROR_ILLEGAL_VALUE;
            }
            
            if (opcode == kClose) {
                LOG(("WebSocketHandler:: close received\n"));
                mServerClosed = 1;
                
                mCloseCode = kCloseNoStatus;
                if (payloadLength >= 2) {
                    memcpy(&mCloseCode, payload, 2);
                    mCloseCode = PR_ntohs(mCloseCode);
                    LOG(("WebSocketHandler:: close recvd code %u\n", mCloseCode));
                    PRUint16 msglen = payloadLength - 2;
                    if (msglen > 0) {
                        nsCString utf8Data((const char *)payload + 2, msglen);

                        
                        
                        
                        
                        if (!IsUTF8(utf8Data)) {
                            LOG(("WebSocketHandler:: close frame invalid utf-8\n"));
                            AbortSession(NS_ERROR_ILLEGAL_VALUE);
                            return NS_ERROR_ILLEGAL_VALUE;
                        }

                        LOG(("WebSocketHandler:: close msg  %s\n",
                             utf8Data.get()));
                    }
                }

                if (mCloseTimer) {
                    mCloseTimer->Cancel();
                    mCloseTimer = nsnull;
                }
                if (mListener) {
                    nsCOMPtr<nsIRunnable> event =
                            new CallOnServerClose(mListener, mContext);
                    NS_DispatchToMainThread(event);
                }

                if (mClientClosed)
                    ReleaseSession();
            }
            else if (opcode == kPing) {
                LOG(("WebSocketHandler:: ping received\n"));
                GeneratePong(payload, payloadLength);
            }
            else {
                
                
                
                LOG(("WebSocketHandler:: pong received\n"));
            }

            if (mFragmentAccumulator) {
                
                
                LOG(("WebSocketHandler:: Removing Control From Read buffer\n"));
                NS_ABORT_IF_FALSE(mFramePtr + framingLength == payload,
                                  "payload offset from frameptr wrong");
                ::memmove (mFramePtr, payload + payloadLength,
                           avail - payloadLength);
                payload = mFramePtr;
                avail -= payloadLength;
                payloadLength = 0;
                if (mBuffered)
                    mBuffered -= framingLength + payloadLength;
            }
        }
        else if (opcode == kBinary) {
            LOG(("WebSocketHandler:: binary frame received\n"));
            if (mListener) {
                nsCString binaryData((const char *)payload, payloadLength);
                nsCOMPtr<nsIRunnable> event =
                    new CallOnMessageAvailable(mListener, mContext,
                                               binaryData, payloadLength);
                NS_DispatchToMainThread(event);
            }
        }
        else if (opcode != kContinuation) {
            
            LOG(("WebSocketHandler:: unknown op code %d\n", opcode));
            AbortSession(NS_ERROR_ILLEGAL_VALUE);
            return NS_ERROR_ILLEGAL_VALUE;
        }
            
        mFramePtr = payload + payloadLength;
        avail -= payloadLength;
        totalAvail = avail;
    }

    
    
    
    
    if (!IsPersistentFramePtr()) {
        mBuffered = 0;
        
        if (mFragmentAccumulator) {
            LOG(("WebSocketHandler:: Setup Buffer due to fragment"));
            
            UpdateReadBuffer(mFramePtr - mFragmentAccumulator,
                             totalAvail + mFragmentAccumulator);

            
            
            mFramePtr += mFragmentAccumulator;
        }
        else if (totalAvail) {
            LOG(("WebSocketHandler:: Setup Buffer due to partial frame"));
            UpdateReadBuffer(mFramePtr, totalAvail);
        }
    }
    else if (!mFragmentAccumulator && !totalAvail) {
        
        
        
        LOG(("WebSocketHandler:: Internal buffering not needed anymore"));
        mBuffered = 0;
    }
    return NS_OK;
}

void
nsWebSocketHandler::ApplyMask(PRUint32 mask, PRUint8 *data, PRUint64 len)
{
    
    
    

    while (len && (reinterpret_cast<PRUptrdiff>(data) & 3)) {
        *data ^= mask >> 24;
        mask = PR_ROTATE_LEFT32(mask, 8);
        data++;
        len--;
    }
    
    

    PRUint32 *iData = (PRUint32 *) data;
    PRUint32 *end = iData + (len / 4);
    mask = PR_htonl(mask);
    for (; iData < end; iData++)
        *iData ^= mask;
    mask = PR_ntohl(mask);
    data = (PRUint8 *)iData;
    len  = len % 4;
    
    
    
    
    while (len) {
        *data ^= mask >> 24;
        mask = PR_ROTATE_LEFT32(mask, 8);
        data++;
        len--;
    }
}

void
nsWebSocketHandler::GeneratePing()
{
    LOG(("WebSocketHandler::GeneratePing() %p\n", this));

    nsCString *buf = new nsCString();
    buf->Assign("PING");
    mOutgoingPingMessages.Push(new OutboundMessage(buf));
    OnOutputStreamReady(mSocketOut);
}

void
nsWebSocketHandler::GeneratePong(PRUint8 *payload, PRUint32 len)
{
    LOG(("WebSocketHandler::GeneratePong() %p [%p %u]\n", this, payload, len));

    nsCString *buf = new nsCString();
    buf->SetLength(len);
    if (buf->Length() < len) {
        LOG(("WebSocketHandler::GeneratePong Allocation Failure\n"));
        delete buf;
        return;
    }
    
    memcpy(buf->BeginWriting(), payload, len);
    mOutgoingPongMessages.Push(new OutboundMessage(buf));
    OnOutputStreamReady(mSocketOut);
}

void
nsWebSocketHandler::SendMsgInternal(nsCString *aMsg,
                                    PRInt32 aDataLen)
{
    LOG(("WebSocketHandler::SendMsgInternal %p [%p len=%d]\n",
         this, aMsg, aDataLen));
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread,
                      "not socket thread");
    if (aMsg == kFinMessage)
        mOutgoingMessages.Push(new OutboundMessage());
    else if (aDataLen < 0)
        mOutgoingMessages.Push(new OutboundMessage(aMsg));
    else
        mOutgoingMessages.Push(new OutboundMessage(aMsg, aDataLen));
    OnOutputStreamReady(mSocketOut);
}

PRUint16
nsWebSocketHandler::ResultToCloseCode(nsresult resultCode)
{
    if (NS_SUCCEEDED(resultCode))
        return kCloseNormal;
    if (resultCode == NS_ERROR_FILE_TOO_BIG)
        return kCloseTooLarge;
    if (resultCode == NS_BASE_STREAM_CLOSED ||
        resultCode == NS_ERROR_NET_TIMEOUT ||
        resultCode == NS_ERROR_CONNECTION_REFUSED)
        return kCloseAbnormal;
    
    return kCloseProtocolError;
}

void
nsWebSocketHandler::PrimeNewOutgoingMessage()
{
    LOG(("WebSocketHandler::PrimeNewOutgoingMessage() %p\n", this));
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread,
                      "not socket thread");
    NS_ABORT_IF_FALSE(!mCurrentOut, "Current message in progress");

    PRBool isPong = PR_FALSE;
    PRBool isPing = PR_FALSE;
    
    mCurrentOut = (OutboundMessage *)mOutgoingPongMessages.PopFront();
    if (mCurrentOut) {
        isPong = PR_TRUE;
    } else {
        mCurrentOut = (OutboundMessage *)mOutgoingPingMessages.PopFront();
        if (mCurrentOut)
            isPing = PR_TRUE;
        else
            mCurrentOut = (OutboundMessage *)mOutgoingMessages.PopFront();
    }

    if (!mCurrentOut)
        return;
    mCurrentOutSent = 0;
    mHdrOut = mOutHeader;

    PRUint8 *payload = nsnull;
    if (mCurrentOut->IsControl() && !isPing && !isPong) {
        
        if (mClientClosed) {
            PrimeNewOutgoingMessage();
            return;
        }
            
        LOG(("WebSocketHandler:: PrimeNewOutgoingMessage() "
             "found close request\n"));
        mClientClosed = 1;
        mOutHeader[0] = kFinalFragBit | kClose;
        mOutHeader[1] = 0x02; 
        mOutHeader[1] |= kMaskBit;

        
        payload = mOutHeader + 6;
        
        
        *((PRUint16 *)payload) = PR_htons(ResultToCloseCode(mStopOnClose));

        mHdrOutToSend = 8;
        if (mServerClosed) {
            
            mReleaseOnTransmit = 1;
        }
        else if (NS_FAILED(mStopOnClose)) {
            
            StopSession(mStopOnClose);
        }
        else {
            
            nsresult rv;
            mCloseTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
            if (NS_SUCCEEDED(rv)) {
                mCloseTimer->InitWithCallback(this, mCloseTimeout,
                                              nsITimer::TYPE_ONE_SHOT);
            }
            else {
                StopSession(rv);
            }
        }
    }
    else {
        if (isPong) {
            LOG(("WebSocketHandler:: PrimeNewOutgoingMessage() "
                 "found pong request\n"));
            mOutHeader[0] = kFinalFragBit | kPong;
        }
        else if (isPing) {
            LOG(("WebSocketHandler:: PrimeNewOutgoingMessage() "
                 "found ping request\n"));
            mOutHeader[0] = kFinalFragBit | kPing;
        }
        else if (mCurrentOut->BinaryLen() < 0) {
            LOG(("WebSocketHandler:: PrimeNewOutgoing Message() "
                 "found queued text message len %d\n",
                 mCurrentOut->Length()));
            mOutHeader[0] = kFinalFragBit | kText;
        }
        else
        {
            LOG(("WebSocketHandler:: PrimeNewOutgoing Message() "
                 "found queued binary message len %d\n",
                 mCurrentOut->Length()));
            mOutHeader[0] = kFinalFragBit | kBinary;
        }

        if (mCurrentOut->Length() < 126) {
            mOutHeader[1] = mCurrentOut->Length() | kMaskBit;
            mHdrOutToSend = 6;
        }
        else if (mCurrentOut->Length() < 0xffff) {
            mOutHeader[1] = 126 | kMaskBit;
            ((PRUint16 *)mOutHeader)[1] =
                PR_htons(mCurrentOut->Length());
            mHdrOutToSend = 8;
        }
        else {
            mOutHeader[1] = 127 | kMaskBit;
            PRUint64 tempLen = mCurrentOut->Length();
            tempLen = PR_htonll(tempLen);
            memcpy(mOutHeader + 2, &tempLen, 8);
            mHdrOutToSend = 14;
        }
        payload = mOutHeader + mHdrOutToSend;
    }
    
    NS_ABORT_IF_FALSE(payload, "payload offset not found");
    
    
    PRUint32 mask;
    do {
        PRUint8 *buffer;
        nsresult rv = mRandomGenerator->GenerateRandomBytes(4, &buffer);
        if (NS_FAILED(rv)) {
            LOG(("WebSocketHandler:: PrimeNewOutgoingMessage() "
                 "GenerateRandomBytes failure %x\n", rv));
            StopSession(rv);
            return;
        }
        mask = * reinterpret_cast<PRUint32 *>(buffer);
        NS_Free(buffer);
    } while (!mask);
    *(((PRUint32 *)payload) - 1) = PR_htonl(mask);

    LOG(("WebSocketHandler:: PrimeNewOutgoingMessage() "
         "using mask %08x\n", mask));

    
    
    
    
    
    

    while (payload < (mOutHeader + mHdrOutToSend)) {
        *payload ^= mask >> 24;
        mask = PR_ROTATE_LEFT32(mask, 8);
        payload++;
    }

    

    ApplyMask(mask,
              mCurrentOut->BeginWriting(),
              mCurrentOut->Length());
    
    
    if (mCurrentOut->Length() <= kCopyBreak) {
        memcpy(mOutHeader + mHdrOutToSend,
               mCurrentOut->BeginWriting(),
               mCurrentOut->Length());
        mHdrOutToSend += mCurrentOut->Length();
        mCurrentOutSent = mCurrentOut->Length();
    }

    if (mCompressor) {
        
        
        PRUint32 currentHeaderSize = mHdrOutToSend;
        mHdrOutToSend = 0;
        
        EnsureHdrOut(32 +
                     (currentHeaderSize +
                      mCurrentOut->Length() - mCurrentOutSent) / 2 * 3);
        mCompressor->
            Deflate(mOutHeader, currentHeaderSize,
                    mCurrentOut->BeginReading() + mCurrentOutSent,
                    mCurrentOut->Length() - mCurrentOutSent);
        
        
        
        mCurrentOutSent = mCurrentOut->Length();
    }

    
    
    
    
}

void
nsWebSocketHandler::EnsureHdrOut(PRUint32 size)
{
    LOG(("WebSocketHandler::EnsureHdrOut() %p [%d]\n", this, size));

    if (mDynamicOutputSize < size) {
        mDynamicOutputSize = size;
        mDynamicOutput =
            (PRUint8 *) moz_xrealloc(mDynamicOutput, mDynamicOutputSize);
    }
    
    mHdrOut = mDynamicOutput;
}

void
nsWebSocketHandler::CleanupConnection()
{
    LOG(("WebSocketHandler::CleanupConnection() %p", this));

    if (mLingeringCloseTimer) {
        mLingeringCloseTimer->Cancel();
        mLingeringCloseTimer = nsnull;
    }

    if (mSocketIn) {
        if (sWebSocketAdmissions)
            sWebSocketAdmissions->DecrementConnectedCount();
        mSocketIn->AsyncWait(nsnull, 0, 0, nsnull);
        mSocketIn = nsnull;
    }
    
    if (mSocketOut) {
        mSocketOut->AsyncWait(nsnull, 0, 0, nsnull);
        mSocketOut = nsnull;
    }
    
    if (mTransport) {
        mTransport->SetSecurityCallbacks(nsnull);
        mTransport->SetEventSink(nsnull, nsnull);
        mTransport->Close(NS_BASE_STREAM_CLOSED);
        mTransport = nsnull;
    }
}

void
nsWebSocketHandler::StopSession(nsresult reason)
{
    LOG(("WebSocketHandler::StopSession() %p [%x]\n", this, reason));

    
    

    NS_ABORT_IF_FALSE(mStopped, "stopsession() has not transitioned "
                      "through abort or close");

    if (mCloseTimer) {
        mCloseTimer->Cancel();
        mCloseTimer = nsnull;
    }

    if (mOpenTimer) {
        mOpenTimer->Cancel();
        mOpenTimer = nsnull;
    }

    if (mPingTimer) {
        mPingTimer->Cancel();
        mPingTimer = nsnull;
    }

    if (mSocketIn && !mTCPClosed) {
        
        
        
        
        
        

        char     buffer[512];
        PRUint32 count = 0;
        PRUint32 total = 0;
        nsresult rv;
        do {
            total += count;
            rv = mSocketIn->Read(buffer, 512, &count);
            if (rv != NS_BASE_STREAM_WOULD_BLOCK &&
                (NS_FAILED(rv) || count == 0))
                mTCPClosed = PR_TRUE;
        } while (NS_SUCCEEDED(rv) && count > 0 && total < 32000);
    }

    if (!mTCPClosed && mTransport && sWebSocketAdmissions &&
        sWebSocketAdmissions->ConnectedCount() < kLingeringCloseThreshold) {

        
        
        
        
        
        
        
        
        
        
        

        LOG(("nsWebSocketHandler::StopSession - Wait for Server TCP close"));

        nsresult rv;
        mLingeringCloseTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
        if (NS_SUCCEEDED(rv))
            mLingeringCloseTimer->InitWithCallback(this, kLingeringCloseTimeout,
                                                   nsITimer::TYPE_ONE_SHOT);
        else
            CleanupConnection();
    }
    else {
        CleanupConnection();
    }

    if (mDNSRequest) {
        mDNSRequest->Cancel(NS_ERROR_UNEXPECTED);
        mDNSRequest = nsnull;
    }

    mInflateReader = nsnull;
    mInflateStream = nsnull;
    
    delete mCompressor;
    mCompressor = nsnull;

    if (!mCalledOnStop) {
        mCalledOnStop = 1;
        if (mListener) {
            nsCOMPtr<nsIRunnable> event =
                    new CallOnStop(mListener, mContext, reason);
            NS_DispatchToMainThread(event);
        }
    }

    return;
}

void
nsWebSocketHandler::AbortSession(nsresult reason)
{
    LOG(("WebSocketHandler::AbortSession() %p [reason %x] stopped = %d\n",
         this, reason, mStopped));

    
    

    
    
    mTCPClosed = PR_TRUE;

    if (mLingeringCloseTimer) {
        NS_ABORT_IF_FALSE(mStopped, "Lingering without Stop");
        LOG(("Cleanup Connection based on TCP Close"));
        CleanupConnection();
        return;
    }

    if (mStopped)
        return;
    mStopped = 1;

    if (mTransport && reason != NS_BASE_STREAM_CLOSED &&
        !mRequestedClose && !mClientClosed && !mServerClosed) {
        mRequestedClose = 1;
        nsCOMPtr<nsIRunnable> event =
            new nsPostMessage(this, kFinMessage, -1);
        mSocketThread->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);
        mStopOnClose = reason;
    }
    else {
        StopSession(reason);
    }
}


void
nsWebSocketHandler::ReleaseSession()
{
    LOG(("WebSocketHandler::ReleaseSession() %p stopped = %d\n",
         this, mStopped));
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread,
                      "not socket thread");
    
    if (mStopped)
        return;
    mStopped = 1;
    StopSession(NS_OK);
}

nsresult
nsWebSocketHandler::HandleExtensions()
{
    LOG(("WebSocketHandler::HandleExtensions() %p\n", this));

    nsresult rv;
    nsCAutoString extensions;

    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

    rv = mHttpChannel->GetResponseHeader(
        NS_LITERAL_CSTRING("Sec-WebSocket-Extensions"), extensions);
    if (NS_SUCCEEDED(rv)) {
        if (!extensions.IsEmpty()) {
            if (!extensions.Equals(NS_LITERAL_CSTRING("deflate-stream"))) {
                LOG(("WebSocketHandler::OnStartRequest "
                     "HTTP Sec-WebSocket-Exensions negotiated "
                     "unknown value %s\n",
                     extensions.get()));
                AbortSession(NS_ERROR_ILLEGAL_VALUE);
                return NS_ERROR_ILLEGAL_VALUE;
            }

            if (!mAllowCompression) {
                LOG(("WebSocketHandler::HandleExtensions "
                     "Recvd Compression Extension that wasn't offered\n"));
                AbortSession(NS_ERROR_ILLEGAL_VALUE);
                return NS_ERROR_ILLEGAL_VALUE;
            }
            
            nsCOMPtr<nsIStreamConverterService> serv =
                do_GetService(NS_STREAMCONVERTERSERVICE_CONTRACTID, &rv);
            if (NS_FAILED(rv)) {
                LOG(("WebSocketHandler:: Cannot find compression service\n"));
                AbortSession(NS_ERROR_UNEXPECTED);
                return NS_ERROR_UNEXPECTED;
            }
            
            rv = serv->AsyncConvertData("deflate",
                                        "uncompressed",
                                        this,
                                        nsnull,
                                        getter_AddRefs(mInflateReader));
            
            if (NS_FAILED(rv)) {
                LOG(("WebSocketHandler:: Cannot find inflate listener\n"));
                AbortSession(NS_ERROR_UNEXPECTED);
                return NS_ERROR_UNEXPECTED;
            }
            
            mInflateStream =
                do_CreateInstance(NS_STRINGINPUTSTREAM_CONTRACTID, &rv);
            
            if (NS_FAILED(rv)) {
                LOG(("WebSocketHandler:: Cannot find inflate stream\n"));
                AbortSession(NS_ERROR_UNEXPECTED);
                return NS_ERROR_UNEXPECTED;
            }
            
            mCompressor = new nsWSCompression(this, mSocketOut);
            if (!mCompressor->Active()) {
                LOG(("WebSocketHandler:: Cannot init deflate object\n"));
                delete mCompressor;
                mCompressor = nsnull;
                AbortSession(NS_ERROR_UNEXPECTED);
                return NS_ERROR_UNEXPECTED;
            }
        }
    }
    
    return NS_OK;
}

nsresult
nsWebSocketHandler::SetupRequest()
{
    LOG(("WebSocketHandler::SetupRequest() %p\n", this));

    nsresult rv;
    
    if (mLoadGroup) {
        rv = mHttpChannel->SetLoadGroup(mLoadGroup);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = mHttpChannel->SetLoadFlags(nsIRequest::LOAD_BACKGROUND |
                                    nsIRequest::INHIBIT_CACHING |
                                    nsIRequest::LOAD_BYPASS_CACHE);
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    
    
    rv = mChannel->HTTPUpgrade(NS_LITERAL_CSTRING("websocket"), this);
    NS_ENSURE_SUCCESS(rv, rv);

    mHttpChannel->SetRequestHeader(
        NS_LITERAL_CSTRING("Sec-WebSocket-Version"),
        NS_LITERAL_CSTRING(SEC_WEBSOCKET_VERSION), PR_FALSE);

    if (!mOrigin.IsEmpty())
        mHttpChannel->SetRequestHeader(
            NS_LITERAL_CSTRING("Sec-WebSocket-Origin"),
            mOrigin, PR_FALSE);

    if (!mProtocol.IsEmpty())
        mHttpChannel->SetRequestHeader(
            NS_LITERAL_CSTRING("Sec-WebSocket-Protocol"),
            mProtocol, PR_TRUE);

    if (mAllowCompression)
        mHttpChannel->SetRequestHeader(
            NS_LITERAL_CSTRING("Sec-WebSocket-Extensions"),
            NS_LITERAL_CSTRING("deflate-stream"), PR_FALSE);

    PRUint8      *secKey;
    nsCAutoString secKeyString;
    
    rv = mRandomGenerator->GenerateRandomBytes(16, &secKey);
    NS_ENSURE_SUCCESS(rv, rv);
    char* b64 = PL_Base64Encode((const char *)secKey, 16, nsnull);
    NS_Free(secKey);
    if (!b64) return NS_ERROR_OUT_OF_MEMORY;
    secKeyString.Assign(b64);
    PR_Free(b64);
    mHttpChannel->SetRequestHeader(
        NS_LITERAL_CSTRING("Sec-WebSocket-Key"), secKeyString, PR_FALSE);
    LOG(("WebSocketHandler::AsyncOpen() client key %s\n", secKeyString.get()));

    
    
    secKeyString.AppendLiteral("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    nsCOMPtr<nsICryptoHash> hasher =
        do_CreateInstance(NS_CRYPTO_HASH_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = hasher->Init(nsICryptoHash::SHA1);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = hasher->Update((const PRUint8 *) secKeyString.BeginWriting(),
                        secKeyString.Length());
    NS_ENSURE_SUCCESS(rv, rv);
    rv = hasher->Finish(PR_TRUE, mHashedSecret);
    NS_ENSURE_SUCCESS(rv, rv);
    LOG(("WebSocketHandler::AsyncOpen() expected server key %s\n",
         mHashedSecret.get()));
    
    return NS_OK;
}

nsresult
nsWebSocketHandler::ApplyForAdmission()
{
    LOG(("WebSocketHandler::ApplyForAdmission() %p\n", this));

    
    

    nsresult rv;
    nsCOMPtr<nsIDNSService> dns = do_GetService(NS_DNSSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCString hostName;
    rv = mURI->GetHost(hostName);
    NS_ENSURE_SUCCESS(rv, rv);
    mAddress = hostName;
    
    
    LOG(("WebSocketHandler::AsyncOpen() checking for concurrent open\n"));
    nsCOMPtr<nsIThread> mainThread;
    NS_GetMainThread(getter_AddRefs(mainThread));
    dns->AsyncResolve(hostName,
                      0,
                      this,
                      mainThread,
                      getter_AddRefs(mDNSRequest));
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}




nsresult
nsWebSocketHandler::StartWebsocketData()
{
    LOG(("WebSocketHandler::StartWebsocketData() %p", this));

    if (sWebSocketAdmissions &&
        sWebSocketAdmissions->ConnectedCount() > mMaxConcurrentConnections) {
        LOG(("nsWebSocketHandler max concurrency %d exceeded "
             "in OnTransportAvailable()",
             mMaxConcurrentConnections));
        
        AbortSession(NS_ERROR_SOCKET_CREATE_FAILED);
        return NS_OK;
    }

    return mSocketIn->AsyncWait(this, 0, 0, mSocketThread);
}



NS_IMETHODIMP
nsWebSocketHandler::OnLookupComplete(nsICancelable *aRequest,
                                     nsIDNSRecord *aRecord,
                                     nsresult aStatus)
{
    LOG(("WebSocketHandler::OnLookupComplete() %p [%p %p %x]\n",
         this, aRequest, aRecord, aStatus));

    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");
    NS_ABORT_IF_FALSE(aRequest == mDNSRequest, "wrong dns request");

    mDNSRequest = nsnull;

    
    if (NS_FAILED(aStatus)) {
        LOG(("WebSocketHandler::OnLookupComplete No DNS Response\n"));
    }
    else {
        nsresult rv = aRecord->GetNextAddrAsString(mAddress);
        if (NS_FAILED(rv))
            LOG(("WebSocketHandler::OnLookupComplete Failed GetNextAddr\n"));
    }
    
    if (sWebSocketAdmissions->ConditionallyConnect(mAddress, this)) {
        LOG(("WebSocketHandler::OnLookupComplete Proceeding with Open\n"));
    }
    else {
        LOG(("WebSocketHandler::OnLookupComplete Deferring Open\n"));
    }
    
    return NS_OK;
}



NS_IMETHODIMP
nsWebSocketHandler::GetInterface(const nsIID & iid, void **result NS_OUTPARAM)
{
    LOG(("WebSocketHandler::GetInterface() %p\n", this));

    if (iid.Equals(NS_GET_IID(nsIChannelEventSink)))
        return QueryInterface(iid, result);
    
    if (mCallbacks)
        return mCallbacks->GetInterface(iid, result);

    return NS_ERROR_FAILURE;
}



NS_IMETHODIMP
nsWebSocketHandler::AsyncOnChannelRedirect(
    nsIChannel *oldChannel,
    nsIChannel *newChannel,
    PRUint32 flags,
    nsIAsyncVerifyRedirectCallback *callback)
{
    LOG(("WebSocketHandler::AsyncOnChannelRedirect() %p\n", this));
    nsresult rv;
    
    nsCOMPtr<nsIURI> newuri;
    rv = newChannel->GetURI(getter_AddRefs(newuri));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mAutoFollowRedirects) {
        nsCAutoString spec;
        if (NS_SUCCEEDED(newuri->GetSpec(spec)))
            LOG(("nsWebSocketHandler Redirect to %s denied by configuration\n",
                 spec.get()));
        callback->OnRedirectVerifyCallback(NS_ERROR_FAILURE);
        return NS_OK;
    }

    PRBool isHttps = PR_FALSE;
    rv = newuri->SchemeIs("https", &isHttps);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (mEncrypted && !isHttps) {
        nsCAutoString spec;
        if (NS_SUCCEEDED(newuri->GetSpec(spec)))
            LOG(("nsWebSocketHandler Redirect to %s violates encryption rule\n",
                 spec.get()));
        callback->OnRedirectVerifyCallback(NS_ERROR_FAILURE);
        return NS_OK;
    }
    
    nsCOMPtr<nsIHttpChannel> newHttpChannel =
        do_QueryInterface(newChannel, &rv);
    
    if (NS_FAILED(rv)) {
        LOG(("nsWebSocketHandler Redirect could not QI to HTTP\n"));
        callback->OnRedirectVerifyCallback(rv);
        return NS_OK;
    }

    nsCOMPtr<nsIHttpChannelInternal> newUpgradeChannel =
        do_QueryInterface(newChannel, &rv);
    
    if (NS_FAILED(rv)) {
        LOG(("nsWebSocketHandler Redirect could not QI to HTTP Upgrade\n"));
        callback->OnRedirectVerifyCallback(rv);
        return NS_OK;
    }
    
    

    newChannel->SetNotificationCallbacks(this);
    mURI = newuri;
    mHttpChannel = newHttpChannel;
    mChannel = newUpgradeChannel;
    rv = SetupRequest();
    if (NS_FAILED(rv)) {
        LOG(("nsWebSocketHandler Redirect could not SetupRequest()\n"));
        callback->OnRedirectVerifyCallback(rv);
        return NS_OK;
    }
    
    
    
    
    
    

    sWebSocketAdmissions->Complete(mAddress);
    mAddress.Truncate();
    mRedirectCallback = callback;

    rv = ApplyForAdmission();
    if (NS_FAILED(rv)) {
        LOG(("nsWebSocketHandler Redirect failed due to DNS failure\n"));
        callback->OnRedirectVerifyCallback(rv);
        mRedirectCallback = nsnull;
    }
    
    return NS_OK;
}



NS_IMETHODIMP
nsWebSocketHandler::Notify(nsITimer *timer)
{
    LOG(("WebSocketHandler::Notify() %p [%p]\n", this, timer));

    if (timer == mCloseTimer) {
        NS_ABORT_IF_FALSE(mClientClosed, "Close Timeout without local close");
        NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread,
                      "not socket thread");

        mCloseTimer = nsnull;
        if (mStopped || mServerClosed)                
            return NS_OK;
    
        LOG(("nsWebSocketHandler:: Expecting Server Close - Timed Out\n"));
        AbortSession(NS_ERROR_NET_TIMEOUT);
    }
    else if (timer == mOpenTimer) {
        NS_ABORT_IF_FALSE(!mRecvdHttpOnStartRequest,
                          "Open Timer after open complete");
        NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

        mOpenTimer = nsnull;
        LOG(("nsWebSocketHandler:: Connection Timed Out\n"));
        if (mStopped || mServerClosed)                
            return NS_OK;
    
        AbortSession(NS_ERROR_NET_TIMEOUT);
    }
    else if (timer == mPingTimer) {
        NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread,
                      "not socket thread");

        if (mClientClosed || mServerClosed || mRequestedClose) {
            
            mPingTimer = nsnull;
            return NS_OK;
        }

        if (!mPingOutstanding) {
            LOG(("nsWebSockethandler:: Generating Ping\n"));
            mPingOutstanding = 1;
            GeneratePing();
            mPingTimer->InitWithCallback(this, mPingResponseTimeout,
                                         nsITimer::TYPE_ONE_SHOT);
        }
        else {
            LOG(("nsWebSockethandler:: Timed out Ping\n"));
            mPingTimer = nsnull;
            AbortSession(NS_ERROR_NET_TIMEOUT);
        }
    }
    else if (timer == mLingeringCloseTimer) {
        LOG(("nsWebSocketHandler:: Lingering Close Timer"));
        CleanupConnection();
    }
    else {
        NS_ABORT_IF_FALSE(0, "Unknown Timer");
    }

    return NS_OK;
}



NS_IMETHODIMP
nsWebSocketHandler::GetOriginalURI(nsIURI **aOriginalURI)
{
    LOG(("WebSocketHandler::GetOriginalURI() %p\n", this));

    if (!mOriginalURI)
        return NS_ERROR_NOT_INITIALIZED;
    NS_ADDREF(*aOriginalURI = mOriginalURI);
    return NS_OK;
}

NS_IMETHODIMP
nsWebSocketHandler::GetURI(nsIURI **aURI)
{
    LOG(("WebSocketHandler::GetURI() %p\n", this));

    if (!mOriginalURI)
        return NS_ERROR_NOT_INITIALIZED;
    if (mURI)
        NS_ADDREF(*aURI = mURI);
    else
        NS_ADDREF(*aURI = mOriginalURI);
    return NS_OK;
}

NS_IMETHODIMP
nsWebSocketHandler::
GetNotificationCallbacks(nsIInterfaceRequestor **aNotificationCallbacks)
{
    LOG(("WebSocketHandler::GetNotificationCallbacks() %p\n", this));
    NS_IF_ADDREF(*aNotificationCallbacks = mCallbacks);
    return NS_OK;
}

NS_IMETHODIMP
nsWebSocketHandler::
SetNotificationCallbacks(nsIInterfaceRequestor *aNotificationCallbacks)
{
    LOG(("WebSocketHandler::SetNotificationCallbacks() %p\n", this));
    mCallbacks = aNotificationCallbacks;
    return NS_OK;
}

NS_IMETHODIMP
nsWebSocketHandler::GetSecurityInfo(nsISupports **aSecurityInfo)
{
    LOG(("WebSocketHandler::GetSecurityInfo() %p\n", this));
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

    if (mTransport) {
        if (NS_FAILED(mTransport->GetSecurityInfo(aSecurityInfo)))
            *aSecurityInfo = nsnull;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsWebSocketHandler::GetLoadGroup(nsILoadGroup **aLoadGroup)
{
    LOG(("WebSocketHandler::GetLoadGroup() %p\n", this));
    NS_IF_ADDREF(*aLoadGroup = mLoadGroup);
    return NS_OK;
}

NS_IMETHODIMP
nsWebSocketHandler::SetLoadGroup(nsILoadGroup *aLoadGroup)
{
    LOG(("WebSocketHandler::SetLoadGroup() %p\n", this));
    mLoadGroup = aLoadGroup;
    return NS_OK;
}

NS_IMETHODIMP
nsWebSocketHandler::GetProtocol(nsACString &aProtocol)
{
    LOG(("WebSocketHandler::GetProtocol() %p\n", this));
    aProtocol = mProtocol;
    return NS_OK;
}

NS_IMETHODIMP
nsWebSocketHandler::SetProtocol(const nsACString &aProtocol)
{
    LOG(("WebSocketHandler::SetProtocol() %p\n", this));
    mProtocol = aProtocol;                        
    return NS_OK;
}

NS_IMETHODIMP
nsWebSocketHandler::AsyncOpen(nsIURI *aURI,
                              const nsACString &aOrigin,
                              nsIWebSocketListener *aListener,
                              nsISupports *aContext)
{
    LOG(("WebSocketHandler::AsyncOpen() %p\n", this));

    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");
    
    if (!aURI || !aListener) {
        LOG(("WebSocketHandler::AsyncOpen() Uri or Listener null"));
        return NS_ERROR_UNEXPECTED;
    }

    if (mListener)
        return NS_ERROR_ALREADY_OPENED;

    nsresult rv;

    mSocketThread = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) {
        NS_WARNING("unable to continue without socket transport service");
        return rv;
    }

    mRandomGenerator = do_GetService("@mozilla.org/security/random-generator;1",
                                     &rv);
    if (NS_FAILED(rv)) {
        NS_WARNING("unable to continue without random number generator");
        return rv;
    }

    nsCOMPtr<nsIPrefBranch> prefService;
    prefService = do_GetService(NS_PREFSERVICE_CONTRACTID);

    if (prefService) {
        PRInt32 intpref;
        PRBool boolpref;
        rv = prefService->
            GetIntPref("network.websocket.max-message-size", &intpref);
        if (NS_SUCCEEDED(rv)) {
            mMaxMessageSize = NS_CLAMP(intpref, 1024, 1 << 30);
        }
        rv = prefService->GetIntPref
            ("network.websocket.timeout.close", &intpref);
        if (NS_SUCCEEDED(rv)) {
            mCloseTimeout = NS_CLAMP(intpref, 1, 1800) * 1000;
        }
        rv = prefService->GetIntPref
            ("network.websocket.timeout.open", &intpref);
        if (NS_SUCCEEDED(rv)) {
            mOpenTimeout = NS_CLAMP(intpref, 1, 1800) * 1000;
        }
        rv = prefService->GetIntPref
            ("network.websocket.timeout.ping.request", &intpref);
        if (NS_SUCCEEDED(rv)) {
            mPingTimeout = NS_CLAMP(intpref, 0, 86400) * 1000;
        }
        rv = prefService->GetIntPref
            ("network.websocket.timeout.ping.response", &intpref);
        if (NS_SUCCEEDED(rv)) {
            mPingResponseTimeout = NS_CLAMP(intpref, 1, 3600) * 1000;
        }
        rv = prefService->GetBoolPref
            ("network.websocket.extensions.stream-deflate", &boolpref);
        if (NS_SUCCEEDED(rv)) {
            mAllowCompression = boolpref ? 1 : 0;
        }
        rv = prefService->GetBoolPref
            ("network.websocket.auto-follow-http-redirects", &boolpref);
        if (NS_SUCCEEDED(rv)) {
            mAutoFollowRedirects = boolpref ? 1 : 0;
        }
        rv = prefService->GetIntPref
            ("network.websocket.max-connections", &intpref);
        if (NS_SUCCEEDED(rv)) {
            mMaxConcurrentConnections = NS_CLAMP(intpref, 1, 0xffff);
        }
    }
    
    if (sWebSocketAdmissions &&
        sWebSocketAdmissions->ConnectedCount() >= mMaxConcurrentConnections) {
        
        
        
        
        LOG(("nsWebSocketHandler max concurrency %d exceeded",
             mMaxConcurrentConnections));
        
        
        
        return NS_ERROR_SOCKET_CREATE_FAILED;
    }

    if (mPingTimeout) {
        mPingTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
        if (NS_FAILED(rv)) {
            NS_WARNING("unable to create ping timer. Carrying on.");
        }
        else {
            LOG(("nsWebSocketHandler will generate ping after %d ms "
                 "of receive silence\n", mPingTimeout));
            mPingTimer->SetTarget(mSocketThread);
            mPingTimer->InitWithCallback(this, mPingTimeout,
                                         nsITimer::TYPE_ONE_SHOT);
        }
    }

    mOriginalURI = aURI;
    mURI = mOriginalURI;
    mListener = aListener;
    mContext = aContext;
    mOrigin = aOrigin;

    nsCOMPtr<nsIURI> localURI;
    nsCOMPtr<nsIChannel> localChannel;
    
    mURI->Clone(getter_AddRefs(localURI));
    if (mEncrypted)
        rv = localURI->SetScheme(NS_LITERAL_CSTRING("https"));
    else
        rv = localURI->SetScheme(NS_LITERAL_CSTRING("http"));
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsIIOService> ioService;
    ioService = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) {
        NS_WARNING("unable to continue without io service");
        return rv;
    }

    nsCOMPtr<nsIIOService2> io2 = do_QueryInterface(ioService, &rv);
    if (NS_FAILED(rv)) {
        NS_WARNING("unable to continue without ioservice2 interface");
        return rv;
    }

    rv = io2->NewChannelFromURIWithProxyFlags(
        localURI,
        mURI,
        nsIProtocolProxyService::RESOLVE_PREFER_SOCKS_PROXY |
        nsIProtocolProxyService::RESOLVE_PREFER_HTTPS_PROXY |
        nsIProtocolProxyService::RESOLVE_ALWAYS_TUNNEL,
        getter_AddRefs(localChannel));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    localChannel->SetNotificationCallbacks(this);

    mChannel = do_QueryInterface(localChannel, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    mHttpChannel = do_QueryInterface(localChannel, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = SetupRequest();
    if (NS_FAILED(rv))
        return rv;

    return ApplyForAdmission();
}

NS_IMETHODIMP
nsWebSocketHandler::Close()
{
    LOG(("WebSocketHandler::Close() %p\n", this));
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");
    if (mRequestedClose) {
        LOG(("WebSocketHandler:: Double close error\n"));
        return NS_ERROR_UNEXPECTED;
    }

    mRequestedClose = 1;
    
    nsCOMPtr<nsIRunnable> event =
        new nsPostMessage(this, kFinMessage, -1);
    return mSocketThread->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);

    return NS_OK;
}

NS_IMETHODIMP
nsWebSocketHandler::SendMsg(const nsACString &aMsg)
{
    LOG(("WebSocketHandler::SendMsg() %p\n", this));
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

    if (mRequestedClose) {
        LOG(("WebSocketHandler:: SendMsg when closed error\n"));
        return NS_ERROR_UNEXPECTED;
    }

    if (mStopped) {
        LOG(("WebSocketHandler:: SendMsg when stopped error\n"));
        return NS_ERROR_NOT_CONNECTED;
    }
    
    nsCOMPtr<nsIRunnable> event =
        new nsPostMessage(this, new nsCString(aMsg), -1);
    return mSocketThread->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);
}

NS_IMETHODIMP
nsWebSocketHandler::SendBinaryMsg(const nsACString &aMsg)
{
    LOG(("WebSocketHandler::SendBinaryMsg() %p len=%d\n", this, aMsg.Length()));
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

    if (mRequestedClose) {
        LOG(("WebSocketHandler:: SendBinaryMsg when closed error\n"));
        return NS_ERROR_UNEXPECTED;
    }

    if (mStopped) {
        LOG(("WebSocketHandler:: SendBinaryMsg when stopped error\n"));
        return NS_ERROR_NOT_CONNECTED;
    }
    
    nsCOMPtr<nsIRunnable> event =
        new nsPostMessage(this, new nsCString(aMsg), aMsg.Length());
    return mSocketThread->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);
}

NS_IMETHODIMP
nsWebSocketHandler::OnTransportAvailable(nsISocketTransport *aTransport,
                                         nsIAsyncInputStream *aSocketIn,
                                         nsIAsyncOutputStream *aSocketOut)
{
    LOG(("WebSocketHandler::OnTransportAvailable "
         "%p [%p %p %p] rcvdonstart=%d\n",
         this, aTransport, aSocketIn, aSocketOut, mRecvdHttpOnStartRequest));

    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");
    NS_ABORT_IF_FALSE(!mRecvdHttpUpgradeTransport, "OTA duplicated");
    NS_ABORT_IF_FALSE(aSocketIn, "OTA with invalid socketIn");
    
    mTransport = aTransport;
    mSocketIn = aSocketIn;
    mSocketOut = aSocketOut;
    if (sWebSocketAdmissions)
        sWebSocketAdmissions->IncrementConnectedCount();

    nsresult rv;
    rv = mTransport->SetEventSink(nsnull, nsnull);
    if (NS_FAILED(rv)) return rv;
    rv = mTransport->SetSecurityCallbacks(mCallbacks);
    if (NS_FAILED(rv)) return rv;

    mRecvdHttpUpgradeTransport = 1;
    if (mRecvdHttpOnStartRequest)
        return StartWebsocketData();
    return NS_OK;
}



NS_IMETHODIMP
nsWebSocketHandler::OnStartRequest(nsIRequest *aRequest,
                                   nsISupports *aContext)
{
    LOG(("WebSocketHandler::OnStartRequest() %p [%p %p] recvdhttpupgrade=%d\n",
         this, aRequest, aContext, mRecvdHttpUpgradeTransport));
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");
    NS_ABORT_IF_FALSE(!mRecvdHttpOnStartRequest, "OTA duplicated");

    
    
    
    

    if (sWebSocketAdmissions->Complete(mAddress))
        LOG(("nsWebSocketHandler::OnStartRequest Starting Pending Open\n"));
    else
        LOG(("nsWebSocketHandler::OnStartRequest No More Pending Opens\n"));

    if (mOpenTimer) {
        mOpenTimer->Cancel();
        mOpenTimer = nsnull;
    }

    if (mStopped) {
        LOG(("WebSocketHandler::OnStartRequest Handler Already Done\n"));
        AbortSession(NS_ERROR_CONNECTION_REFUSED);
        return NS_ERROR_CONNECTION_REFUSED;
    }

    nsresult rv;
    PRUint32 status;
    char *val, *token;

    rv = mHttpChannel->GetResponseStatus(&status);
    if (NS_FAILED(rv)) {
        LOG(("WebSocketHandler::OnStartRequest No HTTP Response\n"));
        AbortSession(NS_ERROR_CONNECTION_REFUSED);
        return NS_ERROR_CONNECTION_REFUSED;
    }

    LOG(("WebSocketHandler::OnStartRequest HTTP status %d\n", status));
    if (status != 101) {
        AbortSession(NS_ERROR_CONNECTION_REFUSED);
        return NS_ERROR_CONNECTION_REFUSED;
    }
    
    nsCAutoString respUpgrade;
    rv = mHttpChannel->GetResponseHeader(
        NS_LITERAL_CSTRING("Upgrade"), respUpgrade);

    if (NS_SUCCEEDED(rv)) {
        rv = NS_ERROR_ILLEGAL_VALUE;
        if (!respUpgrade.IsEmpty()) {
            val = respUpgrade.BeginWriting();
            while ((token = nsCRT::strtok(val, ", \t", &val))) {
                if (PL_strcasecmp(token, "Websocket") == 0) {
                    rv = NS_OK;
                    break;
                }
            }
        }
    }
    
    if (NS_FAILED(rv)) {
        LOG(("WebSocketHandler::OnStartRequest "
             "HTTP response header Upgrade: websocket not found\n"));
        AbortSession(rv);
        return rv;
    }

    nsCAutoString respConnection;
    rv = mHttpChannel->GetResponseHeader(
        NS_LITERAL_CSTRING("Connection"), respConnection);

    if (NS_SUCCEEDED(rv)) {
        rv = NS_ERROR_ILLEGAL_VALUE;
        if (!respConnection.IsEmpty()) {
            val = respConnection.BeginWriting();
            while ((token = nsCRT::strtok(val, ", \t", &val))) {
                if (PL_strcasecmp(token, "Upgrade") == 0) {
                    rv = NS_OK;
                    break;
                }
            }
        }
    }
    
    if (NS_FAILED(rv)) {
        LOG(("WebSocketHandler::OnStartRequest "
             "HTTP response header Connection: Upgrade not found\n"));
        AbortSession(rv);
        return rv;
    }

    nsCAutoString respAccept;
    rv = mHttpChannel->GetResponseHeader(
        NS_LITERAL_CSTRING("Sec-WebSocket-Accept"), respAccept);

    if (NS_FAILED(rv) ||
        respAccept.IsEmpty() || !respAccept.Equals(mHashedSecret)) {
        LOG(("WebSocketHandler::OnStartRequest "
             "HTTP response header Sec-WebSocket-Accept check failed\n"));
        LOG(("WebSocketHandler::OnStartRequest "
             "Expected %s recevied %s\n",
             mHashedSecret.get(), respAccept.get()));
        AbortSession(NS_ERROR_ILLEGAL_VALUE);
        return NS_ERROR_ILLEGAL_VALUE;
    }

    
    
    
    if (!mProtocol.IsEmpty()) {
        nsCAutoString respProtocol;
        rv = mHttpChannel->GetResponseHeader(
            NS_LITERAL_CSTRING("Sec-WebSocket-Protocol"), respProtocol);
        if (NS_SUCCEEDED(rv)) {
            rv = NS_ERROR_ILLEGAL_VALUE;
            val = mProtocol.BeginWriting();
            while ((token = nsCRT::strtok(val, ", \t", &val))) {
                if (PL_strcasecmp(token, respProtocol.get()) == 0) {
                    rv = NS_OK;
                    break;
                }
            }

            if (NS_SUCCEEDED(rv)) {
                LOG(("WebsocketHandler::OnStartRequest "
                     "subprotocol %s confirmed", respProtocol.get()));
                mProtocol = respProtocol;
            }
            else {
                LOG(("WebsocketHandler::OnStartRequest "
                     "subprotocol [%s] not found - %s returned",
                     mProtocol.get(), respProtocol.get()));
                mProtocol.Truncate();
            }
        }
        else {
            LOG(("WebsocketHandler::OnStartRequest "
                 "subprotocol [%s] not found - none returned",
                 mProtocol.get()));
            mProtocol.Truncate();
        }
    }

    rv = HandleExtensions();
    if (NS_FAILED(rv))
        return rv;
    
    LOG(("WebSocketHandler::OnStartRequest Notifying Listener %p\n",
         mListener.get()));
    
    if (mListener)
        mListener->OnStart(mContext);

    mRecvdHttpOnStartRequest = 1;
    if (mRecvdHttpUpgradeTransport)
        return StartWebsocketData();

    return NS_OK;
}

NS_IMETHODIMP
nsWebSocketHandler::OnStopRequest(nsIRequest *aRequest,
                                  nsISupports *aContext,
                                  nsresult aStatusCode)
{
    LOG(("WebSocketHandler::OnStopRequest() %p [%p %p %x]\n",
         this, aRequest, aContext, aStatusCode));
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");
    
    
    

    mChannel = nsnull;
    mHttpChannel = nsnull;
    mLoadGroup = nsnull;
    mCallbacks = nsnull;
    
    return NS_OK;
}



NS_IMETHODIMP
nsWebSocketHandler::OnInputStreamReady(nsIAsyncInputStream *aStream)
{
    LOG(("WebSocketHandler::OnInputStreamReady() %p\n", this));
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread,
                      "not socket thread");
    
    nsRefPtr<nsIStreamListener>    deleteProtector1(mInflateReader);
    nsRefPtr<nsIStringInputStream> deleteProtector2(mInflateStream);

    
    char  buffer[2048];
    PRUint32 count;
    nsresult rv;

    do {
        rv = mSocketIn->Read((char *)buffer, 2048, &count);
        LOG(("WebSocketHandler::OnInputStreamReady read %u rv %x\n",
             count, rv));

        if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
            mSocketIn->AsyncWait(this, 0, 0, mSocketThread);
            return NS_OK;
        }
        
        if (NS_FAILED(rv)) {
            mTCPClosed = PR_TRUE;
            AbortSession(rv);
            return rv;
        }
        
        if (count == 0) {
            mTCPClosed = PR_TRUE;
            AbortSession(NS_BASE_STREAM_CLOSED);
            return NS_OK;
        }
        
        if (mStopped) {
            NS_ABORT_IF_FALSE(mLingeringCloseTimer,
                              "OnInputReady after stop without linger");
            continue;
        }
        
        if (mInflateReader) {
            mInflateStream->ShareData(buffer, count);
            rv = mInflateReader->OnDataAvailable(nsnull, mSocketIn,
                                                 mInflateStream, 0, count);
        }
        else {
            rv = ProcessInput((PRUint8 *)buffer, count);
        }

        if (NS_FAILED(rv)) {
            AbortSession(rv);
            return rv;
        }

    } while (NS_SUCCEEDED(rv) && mSocketIn);

    return NS_OK;
}




NS_IMETHODIMP
nsWebSocketHandler::OnOutputStreamReady(nsIAsyncOutputStream *aStream)
{
    LOG(("WebSocketHandler::OnOutputStreamReady() %p\n", this));
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread,
                      "not socket thread");
    nsresult rv;

    if (!mCurrentOut)
        PrimeNewOutgoingMessage();

    while (mCurrentOut && mSocketOut) {
        const char *sndBuf;
        PRUint32 toSend;
        PRUint32 amtSent;
        
        if (mHdrOut) {
            sndBuf = (const char *)mHdrOut;
            toSend = mHdrOutToSend;
            LOG(("WebSocketHandler::OnOutputStreamReady "
                 "Try to send %u of hdr/copybreak\n",
                 toSend));
        }
        else {
            sndBuf = (char *) mCurrentOut->BeginReading() + mCurrentOutSent;
            toSend = mCurrentOut->Length() - mCurrentOutSent;
            if (toSend > 0) {
                LOG(("WebSocketHandler::OnOutputStreamReady "
                     "Try to send %u of data\n",
                     toSend));
            }
        }
        
        if (toSend == 0) {
            amtSent = 0;
        }
        else {
            rv = mSocketOut->Write(sndBuf, toSend, &amtSent);
            LOG(("WebSocketHandler::OnOutputStreamReady write %u rv %x\n",
                 amtSent, rv));
        
            if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
                mSocketOut->AsyncWait(this, 0, 0, nsnull);
                return NS_OK;
            }

            if (NS_FAILED(rv)) {
                AbortSession(rv);
                return NS_OK;
            }
        }
        
        if (mHdrOut) {
            if (amtSent == toSend) {
                mHdrOut = nsnull;
                mHdrOutToSend = 0;
            }
            else {
                mHdrOut += amtSent;
                mHdrOutToSend -= amtSent;
            }
        }
        else {
            if (amtSent == toSend) {
                if (!mStopped) {
                    nsCOMPtr<nsIRunnable> event =
                        new CallAcknowledge(mListener, mContext,
                                            mCurrentOut->Length());
                    NS_DispatchToMainThread(event);
                }
                delete mCurrentOut;
                mCurrentOut = nsnull;
                mCurrentOutSent = 0;
                PrimeNewOutgoingMessage();
            }
            else {
                mCurrentOutSent += amtSent;
            }
        }
    }

    if (mReleaseOnTransmit)
        ReleaseSession();
    return NS_OK;
}



NS_IMETHODIMP
nsWebSocketHandler::OnDataAvailable(nsIRequest *aRequest,
                                    nsISupports *aContext,
                                    nsIInputStream *aInputStream,
                                    PRUint32 aOffset,
                                    PRUint32 aCount)
{
    LOG(("WebSocketHandler::OnDataAvailable() %p [%p %p %p %u %u]\n",
         this, aRequest, aContext, aInputStream, aOffset, aCount));

    if (aContext == mSocketIn) {
        

        LOG(("WebSocketHandler::OnDataAvailable Deflate Data %u\n",
             aCount));

        PRUint8  buffer[2048];
        PRUint32 maxRead;
        PRUint32 count;
        nsresult rv;

        while (aCount > 0) {
            if (mStopped)
                return NS_BASE_STREAM_CLOSED;
            
            maxRead = NS_MIN(2048U, aCount);
            rv = aInputStream->Read((char *)buffer, maxRead, &count);
            LOG(("WebSocketHandler::OnDataAvailable "
                 "InflateRead read %u rv %x\n",
                 count, rv));
            if (NS_FAILED(rv) || count == 0) {
                AbortSession(rv);
                break;
            }
            
            aCount -= count;
            rv = ProcessInput(buffer, count);
        }
        return NS_OK;
    }

    if (aContext == mSocketOut) {
        
        
        PRUint32 maxRead;
        PRUint32 count;
        nsresult rv;

        while (aCount > 0) {
            if (mStopped)
                return NS_BASE_STREAM_CLOSED;

            maxRead = NS_MIN(2048U, aCount);
            EnsureHdrOut(mHdrOutToSend + aCount);
            rv = aInputStream->Read((char *)mHdrOut + mHdrOutToSend,
                                    maxRead, &count);
            LOG(("WebSocketHandler::OnDataAvailable "
                 "DeflateWrite read %u rv %x\n", count, rv));
            if (NS_FAILED(rv) || count == 0) {
                AbortSession(rv);
                break;
            }

            mHdrOutToSend += count;
            aCount -= count;
        }
        return NS_OK;
    }
    

    
    
    

    
    

    LOG(("WebSocketHandler::OnDataAvailable HTTP data unexpected len>=%u\n",
         aCount));

    return NS_OK;
}



NS_IMETHODIMP
nsWebSocketHandler::GetScheme(nsACString &aScheme)
{
    LOG(("WebSocketHandler::GetScheme() %p\n", this));

    if (mEncrypted)
        aScheme.AssignLiteral("wss");
    else
        aScheme.AssignLiteral("ws");
    return NS_OK;
}

NS_IMETHODIMP
nsWebSocketHandler::GetDefaultPort(PRInt32 *aDefaultPort)
{
    LOG(("WebSocketHandler::GetDefaultPort() %p\n", this));

    if (mEncrypted)
        *aDefaultPort = kDefaultWSSPort;
    else
        *aDefaultPort = kDefaultWSPort;
    return NS_OK;
}

NS_IMETHODIMP
nsWebSocketHandler::GetProtocolFlags(PRUint32 *aProtocolFlags)
{
    LOG(("WebSocketHandler::GetProtocolFlags() %p\n", this));

    *aProtocolFlags = URI_NORELATIVE | URI_NON_PERSISTABLE | ALLOWS_PROXY | 
        ALLOWS_PROXY_HTTP | URI_DOES_NOT_RETURN_DATA | URI_DANGEROUS_TO_LOAD;
    return NS_OK;
}

NS_IMETHODIMP
nsWebSocketHandler::NewURI(const nsACString & aSpec, const char *aOriginCharset,
                           nsIURI *aBaseURI, nsIURI **_retval NS_OUTPARAM)
{
    LOG(("WebSocketHandler::NewURI() %p\n", this));

    PRInt32 port;
    nsresult rv = GetDefaultPort(&port);
    if (NS_FAILED(rv))
        return rv;

    nsRefPtr<nsStandardURL> url = new nsStandardURL();
    rv = url->Init(nsIStandardURL::URLTYPE_AUTHORITY, port, aSpec,
                   aOriginCharset, aBaseURI);
    if (NS_FAILED(rv))
        return rv;
    NS_ADDREF(*_retval = url);
    return NS_OK;
}

NS_IMETHODIMP
nsWebSocketHandler::NewChannel(nsIURI *aURI, nsIChannel **_retval NS_OUTPARAM)
{
    LOG(("WebSocketHandler::NewChannel() %p\n", this));
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWebSocketHandler::AllowPort(PRInt32 port, const char *scheme,
                              PRBool *_retval NS_OUTPARAM)
{
    LOG(("WebSocketHandler::AllowPort() %p\n", this));

    
    *_retval = PR_FALSE;
    return NS_OK;
}

} 
} 
