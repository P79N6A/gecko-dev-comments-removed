





#include "WebSocketLog.h"
#include "WebSocketChannel.h"

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
#include "nsIProxyInfo.h"
#include "nsIProxiedChannel.h"

#include "nsAutoPtr.h"
#include "nsStandardURL.h"
#include "nsNetCID.h"
#include "nsServiceManagerUtils.h"
#include "nsXPIDLString.h"
#include "nsCRT.h"
#include "nsThreadUtils.h"
#include "nsError.h"
#include "nsStringStream.h"
#include "nsAlgorithm.h"
#include "nsProxyRelease.h"
#include "nsNetUtil.h"
#include "mozilla/Attributes.h"
#include "TimeStamp.h"
#include "mozilla/Telemetry.h"

#include "plbase64.h"
#include "prmem.h"
#include "prnetdb.h"
#include "prbit.h"
#include "zlib.h"
#include <algorithm>



#define CLOSE_GOING_AWAY 1001

extern PRThread *gSocketThread;

using namespace mozilla;

namespace mozilla {
namespace net {

NS_IMPL_THREADSAFE_ISUPPORTS11(WebSocketChannel,
                               nsIWebSocketChannel,
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


#define SEC_WEBSOCKET_VERSION "13"


























const uint32_t kWSReconnectInitialBaseDelay     = 200;
const uint32_t kWSReconnectInitialRandomDelay   = 200;


const uint32_t kWSReconnectBaseLifeTime         = 60 * 1000;

const uint32_t kWSReconnectMaxDelay             = 60 * 1000;



class FailDelay
{
public:
  FailDelay(nsCString address, int32_t port)
    : mAddress(address), mPort(port)
  {
    mLastFailure = TimeStamp::Now();
    mNextDelay = kWSReconnectInitialBaseDelay +
                 (rand() % kWSReconnectInitialRandomDelay);
  }

  
  void FailedAgain()
  {
    mLastFailure = TimeStamp::Now();
    
    
    mNextDelay = static_cast<uint32_t>(
      std::min<double>(kWSReconnectMaxDelay, mNextDelay * 1.5));
    LOG(("WebSocket: FailedAgain: host=%s, port=%d: incremented delay to %lu",
         mAddress.get(), mPort, mNextDelay));
  }

  
  uint32_t RemainingDelay(TimeStamp rightNow)
  {
    TimeDuration dur = rightNow - mLastFailure;
    uint32_t sinceFail = (uint32_t) dur.ToMilliseconds();
    if (sinceFail > mNextDelay)
      return 0;

    return mNextDelay - sinceFail;
  }

  bool IsExpired(TimeStamp rightNow)
  {
    return (mLastFailure +
            TimeDuration::FromMilliseconds(kWSReconnectBaseLifeTime + mNextDelay))
            <= rightNow;
  }

  nsCString  mAddress;     
  int32_t    mPort;

private:
  TimeStamp  mLastFailure; 
  
  uint32_t   mNextDelay;   
};

class FailDelayManager
{
public:
  FailDelayManager()
  {
    MOZ_COUNT_CTOR(FailDelayManager);

    mDelaysDisabled = false;

    nsCOMPtr<nsIPrefBranch> prefService =
      do_GetService(NS_PREFSERVICE_CONTRACTID);
    bool boolpref = true;
    nsresult rv;
    rv = prefService->GetBoolPref("network.websocket.delay-failed-reconnects",
                                  &boolpref);
    if (NS_SUCCEEDED(rv) && !boolpref) {
      mDelaysDisabled = true;
    }
  }

  ~FailDelayManager()
  {
    MOZ_COUNT_DTOR(FailDelayManager);
    for (uint32_t i = 0; i < mEntries.Length(); i++) {
      delete mEntries[i];
    }
  }

  void Add(nsCString &address, int32_t port)
  {
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

    if (mDelaysDisabled)
      return;

    FailDelay *record = new FailDelay(address, port);
    mEntries.AppendElement(record);
  }

  
  
  FailDelay* Lookup(nsCString &address, int32_t port,
                    uint32_t *outIndex = nullptr)
  {
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

    if (mDelaysDisabled)
      return nullptr;

    FailDelay *result = nullptr;
    TimeStamp rightNow = TimeStamp::Now();

    
    
    for (int32_t i = mEntries.Length() - 1; i >= 0; --i) {
      FailDelay *fail = mEntries[i];
      if (fail->mAddress.Equals(address) && fail->mPort == port) {
        if (outIndex)
          *outIndex = i;
        result = fail;
        
        
        
        break;
      } else if (fail->IsExpired(rightNow)) {
        mEntries.RemoveElementAt(i);
        delete fail;
      }
    }
    return result;
  }

  
  void DelayOrBegin(WebSocketChannel *ws)
  {
    if (!mDelaysDisabled) {
      uint32_t failIndex = 0;
      FailDelay *fail = Lookup(ws->mAddress, ws->mPort, &failIndex);

      if (fail) {
        TimeStamp rightNow = TimeStamp::Now();

        uint32_t remainingDelay = fail->RemainingDelay(rightNow);
        if (remainingDelay) {
          
          nsresult rv;
          ws->mReconnectDelayTimer =
            do_CreateInstance("@mozilla.org/timer;1", &rv);
          if (NS_SUCCEEDED(rv)) {
            rv = ws->mReconnectDelayTimer->InitWithCallback(
                          ws, remainingDelay, nsITimer::TYPE_ONE_SHOT);
            if (NS_SUCCEEDED(rv)) {
              LOG(("WebSocket: delaying websocket [this=%p] by %lu ms",
                   ws, (unsigned long)remainingDelay));
              ws->mConnecting = CONNECTING_DELAYED;
              return;
            }
          }
          
        } else if (fail->IsExpired(rightNow)) {
          mEntries.RemoveElementAt(failIndex);
          delete fail;
        }
      }
    }

    
    
    ws->BeginOpen();
  }

  
  
  void Remove(nsCString &address, int32_t port)
  {
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

    TimeStamp rightNow = TimeStamp::Now();

    
    for (int32_t i = mEntries.Length() - 1; i >= 0; --i) {
      FailDelay *entry = mEntries[i];
      if ((entry->mAddress.Equals(address) && entry->mPort == port) ||
          entry->IsExpired(rightNow)) {
        mEntries.RemoveElementAt(i);
        delete entry;
      }
    }
  }

private:
  nsTArray<FailDelay *> mEntries;
  bool                  mDelaysDisabled;
};









class nsWSAdmissionManager
{
public:
  nsWSAdmissionManager() : mSessionCount(0)
  {
    MOZ_COUNT_CTOR(nsWSAdmissionManager);
  }

  class nsOpenConn
  {
  public:
    nsOpenConn(nsCString &addr, WebSocketChannel *channel)
      : mAddress(addr), mChannel(channel) { MOZ_COUNT_CTOR(nsOpenConn); }
    ~nsOpenConn() { MOZ_COUNT_DTOR(nsOpenConn); }

    nsCString mAddress;
    WebSocketChannel *mChannel;
  };

  ~nsWSAdmissionManager()
  {
    MOZ_COUNT_DTOR(nsWSAdmissionManager);
    for (uint32_t i = 0; i < mQueue.Length(); i++)
      delete mQueue[i];
  }

  
  
  void ConditionallyConnect(WebSocketChannel *ws)
  {
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");
    NS_ABORT_IF_FALSE(ws->mConnecting == NOT_CONNECTING, "opening state");

    
    
    bool found = (IndexOf(ws->mAddress) >= 0);

    
    nsOpenConn *newdata = new nsOpenConn(ws->mAddress, ws);
    mQueue.AppendElement(newdata);

    if (found) {
      ws->mConnecting = CONNECTING_QUEUED;
    } else {
      mFailures.DelayOrBegin(ws);
    }
  }

  void OnConnected(WebSocketChannel *aChannel)
  {
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");
    NS_ABORT_IF_FALSE(aChannel->mConnecting == CONNECTING_IN_PROGRESS,
                      "Channel completed connect, but not connecting?");

    aChannel->mConnecting = NOT_CONNECTING;

    
    RemoveFromQueue(aChannel);

    
    mFailures.Remove(aChannel->mAddress, aChannel->mPort);

    
    
    
    ConnectNext(aChannel->mAddress);
  }

  
  
  void OnStopSession(WebSocketChannel *aChannel, nsresult aReason)
  {
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

    if (NS_FAILED(aReason)) {
      
      FailDelay *knownFailure = mFailures.Lookup(aChannel->mAddress,
                                                 aChannel->mPort);
      if (knownFailure) {
        
        knownFailure->FailedAgain();
      } else {
        
        LOG(("WebSocket: connection to %s, %d failed: [this=%p]",
              aChannel->mAddress.get(), (int)aChannel->mPort, aChannel));
        mFailures.Add(aChannel->mAddress, aChannel->mPort);
      }
    }

    if (aChannel->mConnecting) {
      
      
      NS_ABORT_IF_FALSE(NS_FAILED(aReason) ||
                        aChannel->mScriptCloseCode == CLOSE_GOING_AWAY,
                        "websocket closed while connecting w/o failing?");

      RemoveFromQueue(aChannel);

      bool wasNotQueued = (aChannel->mConnecting != CONNECTING_QUEUED);
      aChannel->mConnecting = NOT_CONNECTING;
      if (wasNotQueued) {
        ConnectNext(aChannel->mAddress);
      }
    }
  }

  void ConnectNext(nsCString &hostName)
  {
    int32_t index = IndexOf(hostName);
    if (index >= 0) {
      WebSocketChannel *chan = mQueue[index]->mChannel;

      NS_ABORT_IF_FALSE(chan->mConnecting == CONNECTING_QUEUED,
                        "transaction not queued but in queue");
      LOG(("WebSocket: ConnectNext: found channel [this=%p] in queue", chan));

      mFailures.DelayOrBegin(chan);
    }
  }

  void IncrementSessionCount()
  {
    PR_ATOMIC_INCREMENT(&mSessionCount);
  }

  void DecrementSessionCount()
  {
    PR_ATOMIC_DECREMENT(&mSessionCount);
  }

  int32_t SessionCount()
  {
    return mSessionCount;
  }

private:

  void RemoveFromQueue(WebSocketChannel *aChannel)
  {
    int32_t index = IndexOf(aChannel);
    NS_ABORT_IF_FALSE(index >= 0, "connection to remove not in queue");
    if (index >= 0) {
      nsOpenConn *olddata = mQueue[index];
      mQueue.RemoveElementAt(index);
      delete olddata;
    }
  }

  int32_t IndexOf(nsCString &aStr)
  {
    for (uint32_t i = 0; i < mQueue.Length(); i++)
      if (aStr == (mQueue[i])->mAddress)
        return i;
    return -1;
  }

  int32_t IndexOf(WebSocketChannel *aChannel)
  {
    for (uint32_t i = 0; i < mQueue.Length(); i++)
      if (aChannel == (mQueue[i])->mChannel)
        return i;
    return -1;
  }

  
  
  int32_t               mSessionCount;

  
  
  
  
  
  
  
  nsTArray<nsOpenConn *> mQueue;

  FailDelayManager       mFailures;
};

static nsWSAdmissionManager *sWebSocketAdmissions = nullptr;





class CallOnMessageAvailable MOZ_FINAL : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  CallOnMessageAvailable(WebSocketChannel *aChannel,
                         nsCString        &aData,
                         int32_t           aLen)
    : mChannel(aChannel),
      mData(aData),
      mLen(aLen) {}

  NS_IMETHOD Run()
  {
    if (mLen < 0)
      mChannel->mListener->OnMessageAvailable(mChannel->mContext, mData);
    else
      mChannel->mListener->OnBinaryMessageAvailable(mChannel->mContext, mData);
    return NS_OK;
  }

private:
  ~CallOnMessageAvailable() {}

  nsRefPtr<WebSocketChannel>        mChannel;
  nsCString                         mData;
  int32_t                           mLen;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(CallOnMessageAvailable, nsIRunnable)





class CallOnStop MOZ_FINAL : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  CallOnStop(WebSocketChannel *aChannel,
             nsresult          aReason)
    : mChannel(aChannel),
      mReason(aReason) {}

  NS_IMETHOD Run()
  {
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

    sWebSocketAdmissions->OnStopSession(mChannel, mReason);

    if (mChannel->mListener) {
      mChannel->mListener->OnStop(mChannel->mContext, mReason);
      mChannel->mListener = nullptr;
      mChannel->mContext = nullptr;
    }
    return NS_OK;
  }

private:
  ~CallOnStop() {}

  nsRefPtr<WebSocketChannel>        mChannel;
  nsresult                          mReason;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(CallOnStop, nsIRunnable)





class CallOnServerClose MOZ_FINAL : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  CallOnServerClose(WebSocketChannel *aChannel,
                    uint16_t          aCode,
                    nsCString        &aReason)
    : mChannel(aChannel),
      mCode(aCode),
      mReason(aReason) {}

  NS_IMETHOD Run()
  {
    mChannel->mListener->OnServerClose(mChannel->mContext, mCode, mReason);
    return NS_OK;
  }

private:
  ~CallOnServerClose() {}

  nsRefPtr<WebSocketChannel>        mChannel;
  uint16_t                          mCode;
  nsCString                         mReason;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(CallOnServerClose, nsIRunnable)





class CallAcknowledge MOZ_FINAL : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  CallAcknowledge(WebSocketChannel *aChannel,
                  uint32_t          aSize)
    : mChannel(aChannel),
      mSize(aSize) {}

  NS_IMETHOD Run()
  {
    LOG(("WebSocketChannel::CallAcknowledge: Size %u\n", mSize));
    mChannel->mListener->OnAcknowledge(mChannel->mContext, mSize);
    return NS_OK;
  }

private:
  ~CallAcknowledge() {}

  nsRefPtr<WebSocketChannel>        mChannel;
  uint32_t                          mSize;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(CallAcknowledge, nsIRunnable)





class CallOnTransportAvailable MOZ_FINAL : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  CallOnTransportAvailable(WebSocketChannel *aChannel,
                           nsISocketTransport *aTransport,
                           nsIAsyncInputStream *aSocketIn,
                           nsIAsyncOutputStream *aSocketOut)
    : mChannel(aChannel),
      mTransport(aTransport),
      mSocketIn(aSocketIn),
      mSocketOut(aSocketOut) {}

  NS_IMETHOD Run()
  {
    LOG(("WebSocketChannel::CallOnTransportAvailable %p\n", this));
    return mChannel->OnTransportAvailable(mTransport, mSocketIn, mSocketOut);
  }

private:
  ~CallOnTransportAvailable() {}

  nsRefPtr<WebSocketChannel>     mChannel;
  nsCOMPtr<nsISocketTransport>   mTransport;
  nsCOMPtr<nsIAsyncInputStream>  mSocketIn;
  nsCOMPtr<nsIAsyncOutputStream> mSocketOut;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(CallOnTransportAvailable, nsIRunnable)





enum WsMsgType {
  kMsgTypeString = 0,
  kMsgTypeBinaryString,
  kMsgTypeStream,
  kMsgTypePing,
  kMsgTypePong,
  kMsgTypeFin
};

static const char* msgNames[] = {
  "text",
  "binaryString",
  "binaryStream",
  "ping",
  "pong",
  "close"
};

class OutboundMessage
{
public:
  OutboundMessage(WsMsgType type, nsCString *str)
    : mMsgType(type)
  {
    MOZ_COUNT_CTOR(OutboundMessage);
    mMsg.pString = str;
    mLength = str ? str->Length() : 0;
  }

  OutboundMessage(nsIInputStream *stream, uint32_t length)
    : mMsgType(kMsgTypeStream), mLength(length)
  {
    MOZ_COUNT_CTOR(OutboundMessage);
    mMsg.pStream = stream;
    mMsg.pStream->AddRef();
  }

 ~OutboundMessage() {
    MOZ_COUNT_DTOR(OutboundMessage);
    switch (mMsgType) {
      case kMsgTypeString:
      case kMsgTypeBinaryString:
      case kMsgTypePing:
      case kMsgTypePong:
        delete mMsg.pString;
        break;
      case kMsgTypeStream:
        
        if (mMsg.pStream) {
          mMsg.pStream->Close();
          mMsg.pStream->Release();
        }
        break;
      case kMsgTypeFin:
        break;    
    }
  }

  WsMsgType GetMsgType() const { return mMsgType; }
  int32_t Length() const { return mLength; }

  uint8_t* BeginWriting() {
    NS_ABORT_IF_FALSE(mMsgType != kMsgTypeStream,
                      "Stream should have been converted to string by now");
    return (uint8_t *)(mMsg.pString ? mMsg.pString->BeginWriting() : nullptr);
  }

  uint8_t* BeginReading() {
    NS_ABORT_IF_FALSE(mMsgType != kMsgTypeStream,
                      "Stream should have been converted to string by now");
    return (uint8_t *)(mMsg.pString ? mMsg.pString->BeginReading() : nullptr);
  }

  nsresult ConvertStreamToString()
  {
    NS_ABORT_IF_FALSE(mMsgType == kMsgTypeStream, "Not a stream!");

#ifdef DEBUG
    
    uint64_t bytes;
    mMsg.pStream->Available(&bytes);
    NS_ASSERTION(bytes == mLength, "Stream length != blob length!");
#endif

    nsAutoPtr<nsCString> temp(new nsCString());
    nsresult rv = NS_ReadInputStreamToString(mMsg.pStream, *temp, mLength);

    NS_ENSURE_SUCCESS(rv, rv);

    mMsg.pStream->Close();
    mMsg.pStream->Release();
    mMsg.pString = temp.forget();
    mMsgType = kMsgTypeBinaryString;

    return NS_OK;
  }

private:
  union {
    nsCString      *pString;
    nsIInputStream *pStream;
  }                           mMsg;
  WsMsgType                   mMsgType;
  uint32_t                    mLength;
};





class OutboundEnqueuer MOZ_FINAL : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  OutboundEnqueuer(WebSocketChannel *aChannel, OutboundMessage *aMsg)
    : mChannel(aChannel), mMessage(aMsg) {}

  NS_IMETHOD Run()
  {
    mChannel->EnqueueOutgoingMessage(mChannel->mOutgoingMessages, mMessage);
    return NS_OK;
  }

private:
  ~OutboundEnqueuer() {}

  nsRefPtr<WebSocketChannel>  mChannel;
  OutboundMessage            *mMessage;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(OutboundEnqueuer, nsIRunnable)










class nsWSCompression
{
public:
  nsWSCompression(nsIStreamListener *aListener,
                  nsISupports *aContext)
    : mActive(false),
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
      deflateInit2(&mZlib, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8,
                   Z_DEFAULT_STRATEGY) == Z_OK) {
      mActive = true;
    }
  }

  ~nsWSCompression()
  {
    MOZ_COUNT_DTOR(nsWSCompression);

    if (mActive)
      deflateEnd(&mZlib);
  }

  bool Active()
  {
    return mActive;
  }

  nsresult Deflate(uint8_t *buf1, uint32_t buf1Len,
                   uint8_t *buf2, uint32_t buf2Len)
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
    uint32_t bytesToWrite = kBufferLen - mZlib.avail_out;
    if (bytesToWrite > 0) {
      mStream->ShareData(reinterpret_cast<char *>(mBuffer), bytesToWrite);
      nsresult rv =
        mListener->OnDataAvailable(nullptr, mContext, mStream, 0, bytesToWrite);
      if (NS_FAILED(rv))
        return rv;
    }
    return NS_OK;
  }

  bool                            mActive;
  z_stream                        mZlib;
  nsCOMPtr<nsIStringInputStream>  mStream;

  nsISupports                    *mContext;     
  nsIStreamListener              *mListener;    

  const static int32_t            kBufferLen = 4096;
  uint8_t                         mBuffer[kBufferLen];
};





uint32_t WebSocketChannel::sSerialSeed = 0;

WebSocketChannel::WebSocketChannel() :
  mPort(0),
  mCloseTimeout(20000),
  mOpenTimeout(20000),
  mConnecting(NOT_CONNECTING),
  mMaxConcurrentConnections(200),
  mGotUpgradeOK(0),
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
  mOpenedHttpChannel(0),
  mDataStarted(0),
  mIncrementedSessionCount(0),
  mDecrementedSessionCount(0),
  mMaxMessageSize(INT32_MAX),
  mStopOnClose(NS_OK),
  mServerCloseCode(CLOSE_ABNORMAL),
  mScriptCloseCode(0),
  mFragmentOpcode(kContinuation),
  mFragmentAccumulator(0),
  mBuffered(0),
  mBufferSize(kIncomingBufferInitialSize),
  mCurrentOut(nullptr),
  mCurrentOutSent(0),
  mCompressor(nullptr),
  mDynamicOutputSize(0),
  mDynamicOutput(nullptr),
  mConnectionLogService(nullptr)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

  LOG(("WebSocketChannel::WebSocketChannel() %p\n", this));

  if (!sWebSocketAdmissions)
    sWebSocketAdmissions = new nsWSAdmissionManager();

  mFramePtr = mBuffer = static_cast<uint8_t *>(moz_xmalloc(mBufferSize));

  nsresult rv;
  mConnectionLogService = do_GetService("@mozilla.org/network/dashboard;1",&rv);
  if (NS_FAILED(rv))
    LOG(("Failed to initiate dashboard service."));

  mSerial = sSerialSeed++;
}

WebSocketChannel::~WebSocketChannel()
{
  LOG(("WebSocketChannel::~WebSocketChannel() %p\n", this));

  if (mWasOpened) {
    MOZ_ASSERT(mCalledOnStop, "WebSocket was opened but OnStop was not called");
    MOZ_ASSERT(mStopped, "WebSocket was opened but never stopped");
  }
  MOZ_ASSERT(!mDNSRequest, "DNS Request still alive at destruction");
  MOZ_ASSERT(!mConnecting, "Should not be connecting in destructor");

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
    NS_ProxyRelease(mainThread, forgettable, false);
  }

  if (mOriginalURI) {
    mOriginalURI.forget(&forgettable);
    NS_ProxyRelease(mainThread, forgettable, false);
  }

  if (mListener) {
    nsIWebSocketListener *forgettableListener;
    mListener.forget(&forgettableListener);
    NS_ProxyRelease(mainThread, forgettableListener, false);
  }

  if (mContext) {
    nsISupports *forgettableContext;
    mContext.forget(&forgettableContext);
    NS_ProxyRelease(mainThread, forgettableContext, false);
  }

  if (mLoadGroup) {
    nsILoadGroup *forgettableGroup;
    mLoadGroup.forget(&forgettableGroup);
    NS_ProxyRelease(mainThread, forgettableGroup, false);
  }
}

void
WebSocketChannel::Shutdown()
{
  delete sWebSocketAdmissions;
  sWebSocketAdmissions = nullptr;
}

void
WebSocketChannel::BeginOpen()
{
  LOG(("WebSocketChannel::BeginOpen() %p\n", this));
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

  nsresult rv;

  
  
  
  mConnecting = CONNECTING_IN_PROGRESS;

  if (mRedirectCallback) {
    LOG(("WebSocketChannel::BeginOpen: Resuming Redirect\n"));
    rv = mRedirectCallback->OnRedirectVerifyCallback(NS_OK);
    mRedirectCallback = nullptr;
    return;
  }

  nsCOMPtr<nsIChannel> localChannel = do_QueryInterface(mChannel, &rv);
  if (NS_FAILED(rv)) {
    LOG(("WebSocketChannel::BeginOpen: cannot async open\n"));
    AbortSession(NS_ERROR_UNEXPECTED);
    return;
  }

  rv = localChannel->AsyncOpen(this, mHttpChannel);
  if (NS_FAILED(rv)) {
    LOG(("WebSocketChannel::BeginOpen: cannot async open\n"));
    AbortSession(NS_ERROR_CONNECTION_REFUSED);
    return;
  }
  mOpenedHttpChannel = 1;

  mOpenTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  if (NS_FAILED(rv)) {
    LOG(("WebSocketChannel::BeginOpen: cannot create open timer\n"));
    AbortSession(NS_ERROR_UNEXPECTED);
    return;
  }

  rv = mOpenTimer->InitWithCallback(this, mOpenTimeout,
                                    nsITimer::TYPE_ONE_SHOT);
  if (NS_FAILED(rv)) {
    LOG(("WebSocketChannel::BeginOpen: cannot initialize open timer\n"));
    AbortSession(NS_ERROR_UNEXPECTED);
    return;
  }
}

bool
WebSocketChannel::IsPersistentFramePtr()
{
  return (mFramePtr >= mBuffer && mFramePtr < mBuffer + mBufferSize);
}








bool
WebSocketChannel::UpdateReadBuffer(uint8_t *buffer, uint32_t count,
                                   uint32_t accumulatedFragments,
                                   uint32_t *available)
{
  LOG(("WebSocketChannel::UpdateReadBuffer() %p [%p %u]\n",
         this, buffer, count));

  if (!mBuffered)
    mFramePtr = mBuffer;

  NS_ABORT_IF_FALSE(IsPersistentFramePtr(), "update read buffer bad mFramePtr");
  NS_ABORT_IF_FALSE(mFramePtr - accumulatedFragments >= mBuffer,
                    "reserved FramePtr bad");

  if (mBuffered + count <= mBufferSize) {
    
    LOG(("WebSocketChannel: update read buffer absorbed %u\n", count));
  } else if (mBuffered + count - 
             (mFramePtr - accumulatedFragments - mBuffer) <= mBufferSize) {
    
    mBuffered -= (mFramePtr - mBuffer - accumulatedFragments);
    LOG(("WebSocketChannel: update read buffer shifted %u\n", mBuffered));
    ::memmove(mBuffer, mFramePtr - accumulatedFragments, mBuffered);
    mFramePtr = mBuffer + accumulatedFragments;
  } else {
    
    mBufferSize += count + 8192 + mBufferSize/3;
    LOG(("WebSocketChannel: update read buffer extended to %u\n", mBufferSize));
    uint8_t *old = mBuffer;
    mBuffer = (uint8_t *)moz_realloc(mBuffer, mBufferSize);
    if (!mBuffer) {
      mBuffer = old;
      return false;
    }
    mFramePtr = mBuffer + (mFramePtr - old);
  }

  ::memcpy(mBuffer + mBuffered, buffer, count);
  mBuffered += count;

  if (available)
    *available = mBuffered - (mFramePtr - mBuffer);

  return true;
}

nsresult
WebSocketChannel::ProcessInput(uint8_t *buffer, uint32_t count)
{
  LOG(("WebSocketChannel::ProcessInput %p [%d %d]\n", this, count, mBuffered));
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "not socket thread");

  
  
  
  
  ResetPingTimer();

  uint32_t avail;

  if (!mBuffered) {
    
    
    mFramePtr = buffer;
    avail = count;
  } else {
    if (!UpdateReadBuffer(buffer, count, mFragmentAccumulator, &avail)) {
      return NS_ERROR_FILE_TOO_BIG;
    }
  }

  uint8_t *payload;
  uint32_t totalAvail = avail;

  while (avail >= 2) {
    int64_t payloadLength64 = mFramePtr[1] & 0x7F;
    uint8_t finBit  = mFramePtr[0] & kFinalFragBit;
    uint8_t rsvBits = mFramePtr[0] & 0x70;
    uint8_t maskBit = mFramePtr[1] & kMaskBit;
    uint8_t opcode  = mFramePtr[0] & 0x0F;

    uint32_t framingLength = 2;
    if (maskBit)
      framingLength += 4;

    if (payloadLength64 < 126) {
      if (avail < framingLength)
        break;
    } else if (payloadLength64 == 126) {
      
      framingLength += 2;
      if (avail < framingLength)
        break;

      payloadLength64 = mFramePtr[2] << 8 | mFramePtr[3];
    } else {
      
      framingLength += 8;
      if (avail < framingLength)
        break;

      if (mFramePtr[2] & 0x80) {
        
        
        LOG(("WebSocketChannel:: high bit of 64 bit length set"));
        return NS_ERROR_ILLEGAL_VALUE;
      }

      
      uint64_t tempLen;
      memcpy(&tempLen, mFramePtr + 2, 8);
      payloadLength64 = PR_ntohll(tempLen);
    }

    payload = mFramePtr + framingLength;
    avail -= framingLength;

    LOG(("WebSocketChannel::ProcessInput: payload %lld avail %lu\n",
         payloadLength64, avail));

    if (payloadLength64 + mFragmentAccumulator > mMaxMessageSize) {
      return NS_ERROR_FILE_TOO_BIG;
    }
    uint32_t payloadLength = static_cast<uint32_t>(payloadLength64);

    if (avail < payloadLength)
      break;

    LOG(("WebSocketChannel::ProcessInput: Frame accumulated - opcode %d\n",
         opcode));

    if (maskBit) {
      
      
      LOG(("WebSocketChannel:: Client RECEIVING masked frame."));

      uint32_t mask;
      memcpy(&mask, payload - 4, 4);
      mask = PR_ntohl(mask);
      ApplyMask(mask, payload, payloadLength);
    }

    
    if (!finBit && (opcode & kControlFrameMask)) {
      LOG(("WebSocketChannel:: fragmented control frame code %d\n", opcode));
      return NS_ERROR_ILLEGAL_VALUE;
    }

    if (rsvBits) {
      LOG(("WebSocketChannel:: unexpected reserved bits %x\n", rsvBits));
      return NS_ERROR_ILLEGAL_VALUE;
    }

    if (!finBit || opcode == kContinuation) {
      

      
      
      if ((mFragmentAccumulator != 0) && (opcode != kContinuation)) {
        LOG(("WebSocketChannel:: nested fragments\n"));
        return NS_ERROR_ILLEGAL_VALUE;
      }

      LOG(("WebSocketChannel:: Accumulating Fragment %ld\n", payloadLength));

      if (opcode == kContinuation) {

        
        if (mFragmentOpcode == kContinuation) {
          LOG(("WebSocketHeandler:: continuation code in first fragment\n"));
          return NS_ERROR_ILLEGAL_VALUE;
        }

        
        
        NS_ABORT_IF_FALSE(mFramePtr + framingLength == payload,
                          "payload offset from frameptr wrong");
        ::memmove(mFramePtr, payload, avail);
        payload = mFramePtr;
        if (mBuffered)
          mBuffered -= framingLength;
      } else {
        mFragmentOpcode = opcode;
      }

      if (finBit) {
        LOG(("WebSocketChannel:: Finalizing Fragment\n"));
        payload -= mFragmentAccumulator;
        payloadLength += mFragmentAccumulator;
        avail += mFragmentAccumulator;
        mFragmentAccumulator = 0;
        opcode = mFragmentOpcode;
        
        mFragmentOpcode = kContinuation;
      } else {
        opcode = kContinuation;
        mFragmentAccumulator += payloadLength;
      }
    } else if (mFragmentAccumulator != 0 && !(opcode & kControlFrameMask)) {
      
      
      
      LOG(("WebSocketChannel:: illegal fragment sequence\n"));
      return NS_ERROR_ILLEGAL_VALUE;
    }

    if (mServerClosed) {
      LOG(("WebSocketChannel:: ignoring read frame code %d after close\n",
                 opcode));
      
    } else if (mStopped) {
      LOG(("WebSocketChannel:: ignoring read frame code %d after completion\n",
           opcode));
    } else if (opcode == kText) {
      LOG(("WebSocketChannel:: text frame received\n"));
      if (mListener) {
        nsCString utf8Data;
        if (!utf8Data.Assign((const char *)payload, payloadLength,
                             mozilla::fallible_t()))
          return NS_ERROR_OUT_OF_MEMORY;

        
        if (!IsUTF8(utf8Data, false)) {
          LOG(("WebSocketChannel:: text frame invalid utf-8\n"));
          return NS_ERROR_CANNOT_CONVERT_DATA;
        }

        NS_DispatchToMainThread(new CallOnMessageAvailable(this, utf8Data, -1));
        nsresult rv;
        if (mConnectionLogService) {
          nsAutoCString host;
          rv = mURI->GetHostPort(host);
          if (NS_SUCCEEDED(rv)) {
            mConnectionLogService->NewMsgReceived(host, mSerial, count);
            LOG(("Added new msg received for %s",host.get()));
          }
        }
      }
    } else if (opcode & kControlFrameMask) {
      
      if (payloadLength > 125) {
        LOG(("WebSocketChannel:: bad control frame code %d length %d\n",
             opcode, payloadLength));
        return NS_ERROR_ILLEGAL_VALUE;
      }

      if (opcode == kClose) {
        LOG(("WebSocketChannel:: close received\n"));
        mServerClosed = 1;

        mServerCloseCode = CLOSE_NO_STATUS;
        if (payloadLength >= 2) {
          memcpy(&mServerCloseCode, payload, 2);
          mServerCloseCode = PR_ntohs(mServerCloseCode);
          LOG(("WebSocketChannel:: close recvd code %u\n", mServerCloseCode));
          uint16_t msglen = static_cast<uint16_t>(payloadLength - 2);
          if (msglen > 0) {
            mServerCloseReason.SetLength(msglen);
            memcpy(mServerCloseReason.BeginWriting(),
                   (const char *)payload + 2, msglen);

            
            
            
            
            if (!IsUTF8(mServerCloseReason, false)) {
              LOG(("WebSocketChannel:: close frame invalid utf-8\n"));
              return NS_ERROR_CANNOT_CONVERT_DATA;
            }

            LOG(("WebSocketChannel:: close msg %s\n",
                 mServerCloseReason.get()));
          }
        }

        if (mCloseTimer) {
          mCloseTimer->Cancel();
          mCloseTimer = nullptr;
        }
        if (mListener) {
          NS_DispatchToMainThread(new CallOnServerClose(this, mServerCloseCode,
                                                        mServerCloseReason));
        }

        if (mClientClosed)
          ReleaseSession();
      } else if (opcode == kPing) {
        LOG(("WebSocketChannel:: ping received\n"));
        GeneratePong(payload, payloadLength);
      } else if (opcode == kPong) {
        
        
        LOG(("WebSocketChannel:: pong received\n"));
      } else {
        
        LOG(("WebSocketChannel:: unknown control op code %d\n", opcode));
        return NS_ERROR_ILLEGAL_VALUE;
      }

      if (mFragmentAccumulator) {
        
        
        LOG(("WebSocketChannel:: Removing Control From Read buffer\n"));
        NS_ABORT_IF_FALSE(mFramePtr + framingLength == payload,
                          "payload offset from frameptr wrong");
        ::memmove(mFramePtr, payload + payloadLength, avail - payloadLength);
        payload = mFramePtr;
        avail -= payloadLength;
        if (mBuffered)
          mBuffered -= framingLength + payloadLength;
        payloadLength = 0;
      }
    } else if (opcode == kBinary) {
      LOG(("WebSocketChannel:: binary frame received\n"));
      if (mListener) {
        nsCString binaryData((const char *)payload, payloadLength);
        NS_DispatchToMainThread(new CallOnMessageAvailable(this, binaryData,
                                                           payloadLength));
        
        nsresult rv;
        if (mConnectionLogService) {
          nsAutoCString host;
          rv = mURI->GetHostPort(host);
          if (NS_SUCCEEDED(rv)) {
            mConnectionLogService->NewMsgReceived(host, mSerial, count);
            LOG(("Added new received msg for %s",host.get()));
          }
        }
      }
    } else if (opcode != kContinuation) {
      
      LOG(("WebSocketChannel:: unknown op code %d\n", opcode));
      return NS_ERROR_ILLEGAL_VALUE;
    }

    mFramePtr = payload + payloadLength;
    avail -= payloadLength;
    totalAvail = avail;
  }

  
  
  
  
  if (!IsPersistentFramePtr()) {
    mBuffered = 0;

    if (mFragmentAccumulator) {
      LOG(("WebSocketChannel:: Setup Buffer due to fragment"));

      if (!UpdateReadBuffer(mFramePtr - mFragmentAccumulator,
                            totalAvail + mFragmentAccumulator, 0, nullptr)) {
        return NS_ERROR_FILE_TOO_BIG;
      }

      
      
      mFramePtr += mFragmentAccumulator;
    } else if (totalAvail) {
      LOG(("WebSocketChannel:: Setup Buffer due to partial frame"));
      if (!UpdateReadBuffer(mFramePtr, totalAvail, 0, nullptr)) {
        return NS_ERROR_FILE_TOO_BIG;
      }
    }
  } else if (!mFragmentAccumulator && !totalAvail) {
    
    
    LOG(("WebSocketChannel:: Internal buffering not needed anymore"));
    mBuffered = 0;

    
    if (mBufferSize > kIncomingBufferStableSize) {
      mBufferSize = kIncomingBufferStableSize;
      moz_free(mBuffer);
      mBuffer = (uint8_t *)moz_xmalloc(mBufferSize);
    }
  }
  return NS_OK;
}

void
WebSocketChannel::ApplyMask(uint32_t mask, uint8_t *data, uint64_t len)
{
  if (!data || len == 0)
    return;

  
  
  

  while (len && (reinterpret_cast<uintptr_t>(data) & 3)) {
    *data ^= mask >> 24;
    mask = PR_ROTATE_LEFT32(mask, 8);
    data++;
    len--;
  }

  

  uint32_t *iData = (uint32_t *) data;
  uint32_t *end = iData + (len / 4);
  mask = PR_htonl(mask);
  for (; iData < end; iData++)
    *iData ^= mask;
  mask = PR_ntohl(mask);
  data = (uint8_t *)iData;
  len  = len % 4;

  
  

  while (len) {
    *data ^= mask >> 24;
    mask = PR_ROTATE_LEFT32(mask, 8);
    data++;
    len--;
  }
}

void
WebSocketChannel::GeneratePing()
{
  nsCString *buf = new nsCString();
  buf->Assign("PING");
  EnqueueOutgoingMessage(mOutgoingPingMessages,
                         new OutboundMessage(kMsgTypePing, buf));
}

void
WebSocketChannel::GeneratePong(uint8_t *payload, uint32_t len)
{
  nsCString *buf = new nsCString();
  buf->SetLength(len);
  if (buf->Length() < len) {
    LOG(("WebSocketChannel::GeneratePong Allocation Failure\n"));
    delete buf;
    return;
  }

  memcpy(buf->BeginWriting(), payload, len);
  EnqueueOutgoingMessage(mOutgoingPongMessages,
                         new OutboundMessage(kMsgTypePong, buf));
}

void
WebSocketChannel::EnqueueOutgoingMessage(nsDeque &aQueue,
                                         OutboundMessage *aMsg)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "not socket thread");

  LOG(("WebSocketChannel::EnqueueOutgoingMessage %p "
       "queueing msg %p [type=%s len=%d]\n",
       this, aMsg, msgNames[aMsg->GetMsgType()], aMsg->Length()));

  aQueue.Push(aMsg);
  OnOutputStreamReady(mSocketOut);
}


uint16_t
WebSocketChannel::ResultToCloseCode(nsresult resultCode)
{
  if (NS_SUCCEEDED(resultCode))
    return CLOSE_NORMAL;

  switch (resultCode) {
    case NS_ERROR_FILE_TOO_BIG:
    case NS_ERROR_OUT_OF_MEMORY:
      return CLOSE_TOO_LARGE;
    case NS_ERROR_CANNOT_CONVERT_DATA:
      return CLOSE_INVALID_PAYLOAD;
    case NS_ERROR_UNEXPECTED:
      return CLOSE_INTERNAL_ERROR;
    default:
      return CLOSE_PROTOCOL_ERROR;
  }
}

void
WebSocketChannel::PrimeNewOutgoingMessage()
{
  LOG(("WebSocketChannel::PrimeNewOutgoingMessage() %p\n", this));
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "not socket thread");
  NS_ABORT_IF_FALSE(!mCurrentOut, "Current message in progress");

  nsresult rv = NS_OK;

  mCurrentOut = (OutboundMessage *)mOutgoingPongMessages.PopFront();
  if (mCurrentOut) {
    NS_ABORT_IF_FALSE(mCurrentOut->GetMsgType() == kMsgTypePong,
                     "Not pong message!");
  } else {
    mCurrentOut = (OutboundMessage *)mOutgoingPingMessages.PopFront();
    if (mCurrentOut)
      NS_ABORT_IF_FALSE(mCurrentOut->GetMsgType() == kMsgTypePing,
                        "Not ping message!");
    else
      mCurrentOut = (OutboundMessage *)mOutgoingMessages.PopFront();
  }

  if (!mCurrentOut)
    return;

  WsMsgType msgType = mCurrentOut->GetMsgType();

  LOG(("WebSocketChannel::PrimeNewOutgoingMessage "
       "%p found queued msg %p [type=%s len=%d]\n",
       this, mCurrentOut, msgNames[msgType], mCurrentOut->Length()));

  mCurrentOutSent = 0;
  mHdrOut = mOutHeader;

  uint8_t *payload = nullptr;

  if (msgType == kMsgTypeFin) {
    
    if (mClientClosed) {
      DeleteCurrentOutGoingMessage();
      PrimeNewOutgoingMessage();
      return;
    }

    mClientClosed = 1;
    mOutHeader[0] = kFinalFragBit | kClose;
    mOutHeader[1] = kMaskBit;

    
    payload = mOutHeader + 6;

    
    
    
    if (NS_SUCCEEDED(mStopOnClose)) {
      if (mScriptCloseCode) {
        *((uint16_t *)payload) = PR_htons(mScriptCloseCode);
        mOutHeader[1] += 2;
        mHdrOutToSend = 8;
        if (!mScriptCloseReason.IsEmpty()) {
          NS_ABORT_IF_FALSE(mScriptCloseReason.Length() <= 123,
                            "Close Reason Too Long");
          mOutHeader[1] += mScriptCloseReason.Length();
          mHdrOutToSend += mScriptCloseReason.Length();
          memcpy (payload + 2,
                  mScriptCloseReason.BeginReading(),
                  mScriptCloseReason.Length());
        }
      } else {
        
        
        
        mHdrOutToSend = 6;
      }
    } else {
      *((uint16_t *)payload) = PR_htons(ResultToCloseCode(mStopOnClose));
      mOutHeader[1] += 2;
      mHdrOutToSend = 8;
    }

    if (mServerClosed) {
      
      mReleaseOnTransmit = 1;
    } else if (NS_FAILED(mStopOnClose)) {
      
      StopSession(mStopOnClose);
    } else {
      
      mCloseTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
      if (NS_SUCCEEDED(rv)) {
        mCloseTimer->InitWithCallback(this, mCloseTimeout,
                                      nsITimer::TYPE_ONE_SHOT);
      } else {
        StopSession(rv);
      }
    }
  } else {
    switch (msgType) {
    case kMsgTypePong:
      mOutHeader[0] = kFinalFragBit | kPong;
      break;
    case kMsgTypePing:
      mOutHeader[0] = kFinalFragBit | kPing;
      break;
    case kMsgTypeString:
      mOutHeader[0] = kFinalFragBit | kText;
      break;
    case kMsgTypeStream:
      
      
      
      rv = mCurrentOut->ConvertStreamToString();
      if (NS_FAILED(rv)) {
        AbortSession(NS_ERROR_FILE_TOO_BIG);
        return;
      }
      
      msgType = kMsgTypeBinaryString;

      

    case kMsgTypeBinaryString:
      mOutHeader[0] = kFinalFragBit | kBinary;
      break;
    case kMsgTypeFin:
      NS_ABORT_IF_FALSE(false, "unreachable");  
      break;
    }

    if (mCurrentOut->Length() < 126) {
      mOutHeader[1] = mCurrentOut->Length() | kMaskBit;
      mHdrOutToSend = 6;
    } else if (mCurrentOut->Length() <= 0xffff) {
      mOutHeader[1] = 126 | kMaskBit;
      ((uint16_t *)mOutHeader)[1] =
        PR_htons(mCurrentOut->Length());
      mHdrOutToSend = 8;
    } else {
      mOutHeader[1] = 127 | kMaskBit;
      uint64_t tempLen = mCurrentOut->Length();
      tempLen = PR_htonll(tempLen);
      memcpy(mOutHeader + 2, &tempLen, 8);
      mHdrOutToSend = 14;
    }
    payload = mOutHeader + mHdrOutToSend;
  }

  NS_ABORT_IF_FALSE(payload, "payload offset not found");

  
  uint32_t mask;
  do {
    uint8_t *buffer;
    nsresult rv = mRandomGenerator->GenerateRandomBytes(4, &buffer);
    if (NS_FAILED(rv)) {
      LOG(("WebSocketChannel::PrimeNewOutgoingMessage(): "
           "GenerateRandomBytes failure %x\n", rv));
      StopSession(rv);
      return;
    }
    mask = * reinterpret_cast<uint32_t *>(buffer);
    NS_Free(buffer);
  } while (!mask);
  *(((uint32_t *)payload) - 1) = PR_htonl(mask);

  LOG(("WebSocketChannel::PrimeNewOutgoingMessage() using mask %08x\n", mask));

  
  
  
  
  

  while (payload < (mOutHeader + mHdrOutToSend)) {
    *payload ^= mask >> 24;
    mask = PR_ROTATE_LEFT32(mask, 8);
    payload++;
  }

  

  ApplyMask(mask, mCurrentOut->BeginWriting(), mCurrentOut->Length());

  int32_t len = mCurrentOut->Length();

  
  if (len && len <= kCopyBreak) {
    memcpy(mOutHeader + mHdrOutToSend, mCurrentOut->BeginWriting(), len);
    mHdrOutToSend += len;
    mCurrentOutSent = len;
  }

  if (len && mCompressor) {
    
    
    uint32_t currentHeaderSize = mHdrOutToSend;
    mHdrOutToSend = 0;

    EnsureHdrOut(32 + (currentHeaderSize + len - mCurrentOutSent) / 2 * 3);
    mCompressor->Deflate(mOutHeader, currentHeaderSize,
                         mCurrentOut->BeginReading() + mCurrentOutSent,
                         len - mCurrentOutSent);

    
    
    mCurrentOutSent = len;
  }

  
  
  
  
}

void
WebSocketChannel::DeleteCurrentOutGoingMessage()
{
  delete mCurrentOut;
  mCurrentOut = nullptr;
  mCurrentOutSent = 0;
}

void
WebSocketChannel::EnsureHdrOut(uint32_t size)
{
  LOG(("WebSocketChannel::EnsureHdrOut() %p [%d]\n", this, size));

  if (mDynamicOutputSize < size) {
    mDynamicOutputSize = size;
    mDynamicOutput =
      (uint8_t *) moz_xrealloc(mDynamicOutput, mDynamicOutputSize);
  }

  mHdrOut = mDynamicOutput;
}

void
WebSocketChannel::CleanupConnection()
{
  LOG(("WebSocketChannel::CleanupConnection() %p", this));

  if (mLingeringCloseTimer) {
    mLingeringCloseTimer->Cancel();
    mLingeringCloseTimer = nullptr;
  }

  if (mSocketIn) {
    mSocketIn->AsyncWait(nullptr, 0, 0, nullptr);
    mSocketIn = nullptr;
  }

  if (mSocketOut) {
    mSocketOut->AsyncWait(nullptr, 0, 0, nullptr);
    mSocketOut = nullptr;
  }

  if (mTransport) {
    mTransport->SetSecurityCallbacks(nullptr);
    mTransport->SetEventSink(nullptr, nullptr);
    mTransport->Close(NS_BASE_STREAM_CLOSED);
    mTransport = nullptr;
  }

  nsresult rv;
  if (mConnectionLogService) {
    nsAutoCString host;
    rv = mURI->GetHostPort(host);
    if (NS_SUCCEEDED(rv))
      mConnectionLogService->RemoveHost(host, mSerial);
  }

  DecrementSessionCount();
}

void
WebSocketChannel::StopSession(nsresult reason)
{
  LOG(("WebSocketChannel::StopSession() %p [%x]\n", this, reason));

  
  

  mStopped = 1;

  if (!mOpenedHttpChannel) {
    
    mChannel = nullptr;
    mHttpChannel = nullptr;
    mLoadGroup = nullptr;
    mCallbacks = nullptr;
  }

  if (mCloseTimer) {
    mCloseTimer->Cancel();
    mCloseTimer = nullptr;
  }

  if (mOpenTimer) {
    mOpenTimer->Cancel();
    mOpenTimer = nullptr;
  }

  if (mReconnectDelayTimer) {
    mReconnectDelayTimer->Cancel();
    mReconnectDelayTimer = nullptr;
  }

  if (mPingTimer) {
    mPingTimer->Cancel();
    mPingTimer = nullptr;
  }

  if (mSocketIn && !mTCPClosed) {
    
    
    
    
    
    

    char     buffer[512];
    uint32_t count = 0;
    uint32_t total = 0;
    nsresult rv;
    do {
      total += count;
      rv = mSocketIn->Read(buffer, 512, &count);
      if (rv != NS_BASE_STREAM_WOULD_BLOCK &&
        (NS_FAILED(rv) || count == 0))
        mTCPClosed = true;
    } while (NS_SUCCEEDED(rv) && count > 0 && total < 32000);
  }

  if (!mTCPClosed && mTransport && sWebSocketAdmissions &&
      sWebSocketAdmissions->SessionCount() < kLingeringCloseThreshold) {

    
    
    
    
    
    
    
    
    
    

    LOG(("WebSocketChannel::StopSession: Wait for Server TCP close"));

    nsresult rv;
    mLingeringCloseTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    if (NS_SUCCEEDED(rv))
      mLingeringCloseTimer->InitWithCallback(this, kLingeringCloseTimeout,
                                             nsITimer::TYPE_ONE_SHOT);
    else
      CleanupConnection();
  } else {
    CleanupConnection();
  }

  if (mDNSRequest) {
    mDNSRequest->Cancel(NS_ERROR_UNEXPECTED);
    mDNSRequest = nullptr;
  }

  mInflateReader = nullptr;
  mInflateStream = nullptr;

  delete mCompressor;
  mCompressor = nullptr;

  if (!mCalledOnStop) {
    mCalledOnStop = 1;
    NS_DispatchToMainThread(new CallOnStop(this, reason));
  }

  return;
}

void
WebSocketChannel::AbortSession(nsresult reason)
{
  LOG(("WebSocketChannel::AbortSession() %p [reason %x] stopped = %d\n",
       this, reason, mStopped));

  
  

  
  
  mTCPClosed = true;

  if (mLingeringCloseTimer) {
    NS_ABORT_IF_FALSE(mStopped, "Lingering without Stop");
    LOG(("WebSocketChannel:: Cleanup connection based on TCP Close"));
    CleanupConnection();
    return;
  }

  if (mStopped)
    return;
  mStopped = 1;

  if (mTransport && reason != NS_BASE_STREAM_CLOSED &&
      !mRequestedClose && !mClientClosed && !mServerClosed) {
    mRequestedClose = 1;
    mStopOnClose = reason;
    mSocketThread->Dispatch(
      new OutboundEnqueuer(this, new OutboundMessage(kMsgTypeFin, nullptr)),
                           nsIEventTarget::DISPATCH_NORMAL);
  } else {
    StopSession(reason);
  }
}


void
WebSocketChannel::ReleaseSession()
{
  LOG(("WebSocketChannel::ReleaseSession() %p stopped = %d\n",
       this, mStopped));
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "not socket thread");

  if (mStopped)
    return;
  StopSession(NS_OK);
}

void
WebSocketChannel::IncrementSessionCount()
{
  if (!mIncrementedSessionCount) {
    sWebSocketAdmissions->IncrementSessionCount();
    mIncrementedSessionCount = 1;
  }
}

void
WebSocketChannel::DecrementSessionCount()
{
  
  
  
  
  if (mIncrementedSessionCount && !mDecrementedSessionCount) {
    sWebSocketAdmissions->DecrementSessionCount();
    mDecrementedSessionCount = 1;
  }
}

nsresult
WebSocketChannel::HandleExtensions()
{
  LOG(("WebSocketChannel::HandleExtensions() %p\n", this));

  nsresult rv;
  nsAutoCString extensions;

  NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

  rv = mHttpChannel->GetResponseHeader(
    NS_LITERAL_CSTRING("Sec-WebSocket-Extensions"), extensions);
  if (NS_SUCCEEDED(rv)) {
    if (!extensions.IsEmpty()) {
      if (!extensions.Equals(NS_LITERAL_CSTRING("deflate-stream"))) {
        LOG(("WebSocketChannel::OnStartRequest: "
             "HTTP Sec-WebSocket-Exensions negotiated unknown value %s\n",
             extensions.get()));
        AbortSession(NS_ERROR_ILLEGAL_VALUE);
        return NS_ERROR_ILLEGAL_VALUE;
      }

      if (!mAllowCompression) {
        LOG(("WebSocketChannel::HandleExtensions: "
             "Recvd Compression Extension that wasn't offered\n"));
        AbortSession(NS_ERROR_ILLEGAL_VALUE);
        return NS_ERROR_ILLEGAL_VALUE;
      }

      nsCOMPtr<nsIStreamConverterService> serv =
        do_GetService(NS_STREAMCONVERTERSERVICE_CONTRACTID, &rv);
      if (NS_FAILED(rv)) {
        LOG(("WebSocketChannel:: Cannot find compression service\n"));
        AbortSession(NS_ERROR_UNEXPECTED);
        return NS_ERROR_UNEXPECTED;
      }

      rv = serv->AsyncConvertData("deflate", "uncompressed", this, nullptr,
                                  getter_AddRefs(mInflateReader));

      if (NS_FAILED(rv)) {
        LOG(("WebSocketChannel:: Cannot find inflate listener\n"));
        AbortSession(NS_ERROR_UNEXPECTED);
        return NS_ERROR_UNEXPECTED;
      }

      mInflateStream = do_CreateInstance(NS_STRINGINPUTSTREAM_CONTRACTID, &rv);

      if (NS_FAILED(rv)) {
        LOG(("WebSocketChannel:: Cannot find inflate stream\n"));
        AbortSession(NS_ERROR_UNEXPECTED);
        return NS_ERROR_UNEXPECTED;
      }

      mCompressor = new nsWSCompression(this, mSocketOut);
      if (!mCompressor->Active()) {
        LOG(("WebSocketChannel:: Cannot init deflate object\n"));
        delete mCompressor;
        mCompressor = nullptr;
        AbortSession(NS_ERROR_UNEXPECTED);
        return NS_ERROR_UNEXPECTED;
      }
      mNegotiatedExtensions = extensions;
    }
  }

  return NS_OK;
}

nsresult
WebSocketChannel::SetupRequest()
{
  LOG(("WebSocketChannel::SetupRequest() %p\n", this));

  nsresult rv;

  if (mLoadGroup) {
    rv = mHttpChannel->SetLoadGroup(mLoadGroup);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mHttpChannel->SetLoadFlags(nsIRequest::LOAD_BACKGROUND |
                                  nsIRequest::INHIBIT_CACHING |
                                  nsIRequest::LOAD_BYPASS_CACHE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  rv = mChannel->SetLoadUnblocked(true);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mChannel->HTTPUpgrade(NS_LITERAL_CSTRING("websocket"), this);
  NS_ENSURE_SUCCESS(rv, rv);

  mHttpChannel->SetRequestHeader(
    NS_LITERAL_CSTRING("Sec-WebSocket-Version"),
    NS_LITERAL_CSTRING(SEC_WEBSOCKET_VERSION), false);

  if (!mOrigin.IsEmpty())
    mHttpChannel->SetRequestHeader(NS_LITERAL_CSTRING("Origin"), mOrigin,
                                   false);

  if (!mProtocol.IsEmpty())
    mHttpChannel->SetRequestHeader(NS_LITERAL_CSTRING("Sec-WebSocket-Protocol"),
                                   mProtocol, true);

  if (mAllowCompression)
    mHttpChannel->SetRequestHeader(NS_LITERAL_CSTRING("Sec-WebSocket-Extensions"),
                                   NS_LITERAL_CSTRING("deflate-stream"),
                                   false);

  uint8_t      *secKey;
  nsAutoCString secKeyString;

  rv = mRandomGenerator->GenerateRandomBytes(16, &secKey);
  NS_ENSURE_SUCCESS(rv, rv);
  char* b64 = PL_Base64Encode((const char *)secKey, 16, nullptr);
  NS_Free(secKey);
  if (!b64)
    return NS_ERROR_OUT_OF_MEMORY;
  secKeyString.Assign(b64);
  PR_Free(b64);
  mHttpChannel->SetRequestHeader(NS_LITERAL_CSTRING("Sec-WebSocket-Key"),
                                 secKeyString, false);
  LOG(("WebSocketChannel::SetupRequest: client key %s\n", secKeyString.get()));

  
  
  secKeyString.AppendLiteral("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
  nsCOMPtr<nsICryptoHash> hasher =
    do_CreateInstance(NS_CRYPTO_HASH_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = hasher->Init(nsICryptoHash::SHA1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = hasher->Update((const uint8_t *) secKeyString.BeginWriting(),
                      secKeyString.Length());
  NS_ENSURE_SUCCESS(rv, rv);
  rv = hasher->Finish(true, mHashedSecret);
  NS_ENSURE_SUCCESS(rv, rv);
  LOG(("WebSocketChannel::SetupRequest: expected server key %s\n",
       mHashedSecret.get()));

  return NS_OK;
}

nsresult
WebSocketChannel::ApplyForAdmission()
{
  LOG(("WebSocketChannel::ApplyForAdmission() %p\n", this));

  
  

  nsresult rv;
  nsCOMPtr<nsIDNSService> dns = do_GetService(NS_DNSSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString hostName;
  rv = mURI->GetHost(hostName);
  NS_ENSURE_SUCCESS(rv, rv);
  mAddress = hostName;
  rv = mURI->GetPort(&mPort);
  NS_ENSURE_SUCCESS(rv, rv);
  if (mPort == -1)
    mPort = (mEncrypted ? kDefaultWSSPort : kDefaultWSPort);

  
  LOG(("WebSocketChannel::ApplyForAdmission: checking for concurrent open\n"));
  nsCOMPtr<nsIThread> mainThread;
  NS_GetMainThread(getter_AddRefs(mainThread));
  dns->AsyncResolve(hostName, 0, this, mainThread, getter_AddRefs(mDNSRequest));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}




nsresult
WebSocketChannel::StartWebsocketData()
{
  LOG(("WebSocketChannel::StartWebsocketData() %p", this));
  NS_ABORT_IF_FALSE(!mDataStarted, "StartWebsocketData twice");
  mDataStarted = 1;

  
  
  
  sWebSocketAdmissions->OnConnected(this);

  LOG(("WebSocketChannel::StartWebsocketData Notifying Listener %p\n",
       mListener.get()));

  if (mListener)
    mListener->OnStart(mContext);

  
  if (mPingInterval) {
    nsresult rv;
    mPingTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    if (NS_FAILED(rv)) {
      NS_WARNING("unable to create ping timer. Carrying on.");
    } else {
      LOG(("WebSocketChannel will generate ping after %d ms of receive silence\n",
           mPingInterval));
      mPingTimer->SetTarget(mSocketThread);
      mPingTimer->InitWithCallback(this, mPingInterval, nsITimer::TYPE_ONE_SHOT);
    }
  }

  return mSocketIn->AsyncWait(this, 0, 0, mSocketThread);
}

void
WebSocketChannel::ReportConnectionTelemetry()
{ 
  
  
  
  

  bool didProxy = false;

  nsCOMPtr<nsIProxyInfo> pi;
  nsCOMPtr<nsIProxiedChannel> pc = do_QueryInterface(mChannel);
  if (pc)
    pc->GetProxyInfo(getter_AddRefs(pi));
  if (pi) {
    nsAutoCString proxyType;
    pi->GetType(proxyType);
    if (!proxyType.IsEmpty() &&
        !proxyType.Equals(NS_LITERAL_CSTRING("direct")))
      didProxy = true;
  }

  uint8_t value = (mEncrypted ? (1 << 2) : 0) | 
    (!mGotUpgradeOK ? (1 << 1) : 0) |
    (didProxy ? (1 << 0) : 0);

  LOG(("WebSocketChannel::ReportConnectionTelemetry() %p %d", this, value));
  Telemetry::Accumulate(Telemetry::WEBSOCKETS_HANDSHAKE_TYPE, value);
}



NS_IMETHODIMP
WebSocketChannel::OnLookupComplete(nsICancelable *aRequest,
                                     nsIDNSRecord *aRecord,
                                     nsresult aStatus)
{
  LOG(("WebSocketChannel::OnLookupComplete() %p [%p %p %x]\n",
       this, aRequest, aRecord, aStatus));

  NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");
  NS_ABORT_IF_FALSE(aRequest == mDNSRequest || mStopped,
                    "wrong dns request");

  if (mStopped) {
    LOG(("WebSocketChannel::OnLookupComplete: Request Already Stopped\n"));
    return NS_OK;
  }

  mDNSRequest = nullptr;

  
  if (NS_FAILED(aStatus)) {
    LOG(("WebSocketChannel::OnLookupComplete: No DNS Response\n"));
  } else {
    nsresult rv = aRecord->GetNextAddrAsString(mAddress);
    if (NS_FAILED(rv))
      LOG(("WebSocketChannel::OnLookupComplete: Failed GetNextAddr\n"));
  }

  LOG(("WebSocket OnLookupComplete: Proceeding to ConditionallyConnect\n"));
  sWebSocketAdmissions->ConditionallyConnect(this);

  return NS_OK;
}



NS_IMETHODIMP
WebSocketChannel::GetInterface(const nsIID & iid, void **result)
{
  LOG(("WebSocketChannel::GetInterface() %p\n", this));

  if (iid.Equals(NS_GET_IID(nsIChannelEventSink)))
    return QueryInterface(iid, result);

  if (mCallbacks)
    return mCallbacks->GetInterface(iid, result);

  return NS_ERROR_FAILURE;
}



NS_IMETHODIMP
WebSocketChannel::AsyncOnChannelRedirect(
                    nsIChannel *oldChannel,
                    nsIChannel *newChannel,
                    uint32_t flags,
                    nsIAsyncVerifyRedirectCallback *callback)
{
  LOG(("WebSocketChannel::AsyncOnChannelRedirect() %p\n", this));
  nsresult rv;

  nsCOMPtr<nsIURI> newuri;
  rv = newChannel->GetURI(getter_AddRefs(newuri));
  NS_ENSURE_SUCCESS(rv, rv);

  
  bool newuriIsHttps = false;
  rv = newuri->SchemeIs("https", &newuriIsHttps);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mAutoFollowRedirects) {
    
    

    nsCOMPtr<nsIURI> clonedNewURI;
    rv = newuri->Clone(getter_AddRefs(clonedNewURI));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = clonedNewURI->SetScheme(NS_LITERAL_CSTRING("ws"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIURI> currentURI;
    rv = GetURI(getter_AddRefs(currentURI));
    NS_ENSURE_SUCCESS(rv, rv);

    
    bool currentIsHttps = false;
    rv = currentURI->SchemeIs("wss", &currentIsHttps);
    NS_ENSURE_SUCCESS(rv, rv);

    bool uriEqual = false;
    rv = clonedNewURI->Equals(currentURI, &uriEqual);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (!(!currentIsHttps && newuriIsHttps && uriEqual)) {
      nsAutoCString newSpec;
      rv = newuri->GetSpec(newSpec);
      NS_ENSURE_SUCCESS(rv, rv);

      LOG(("WebSocketChannel: Redirect to %s denied by configuration\n",
           newSpec.get()));
      return NS_ERROR_FAILURE;
    }
  }

  if (mEncrypted && !newuriIsHttps) {
    nsAutoCString spec;
    if (NS_SUCCEEDED(newuri->GetSpec(spec)))
      LOG(("WebSocketChannel: Redirect to %s violates encryption rule\n",
           spec.get()));
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIHttpChannel> newHttpChannel = do_QueryInterface(newChannel, &rv);
  if (NS_FAILED(rv)) {
    LOG(("WebSocketChannel: Redirect could not QI to HTTP\n"));
    return rv;
  }

  nsCOMPtr<nsIHttpChannelInternal> newUpgradeChannel =
    do_QueryInterface(newChannel, &rv);

  if (NS_FAILED(rv)) {
    LOG(("WebSocketChannel: Redirect could not QI to HTTP Upgrade\n"));
    return rv;
  }

  

  newChannel->SetNotificationCallbacks(this);

  mEncrypted = newuriIsHttps;
  newuri->Clone(getter_AddRefs(mURI));
  if (mEncrypted)
    rv = mURI->SetScheme(NS_LITERAL_CSTRING("wss"));
  else
    rv = mURI->SetScheme(NS_LITERAL_CSTRING("ws"));

  mHttpChannel = newHttpChannel;
  mChannel = newUpgradeChannel;
  rv = SetupRequest();
  if (NS_FAILED(rv)) {
    LOG(("WebSocketChannel: Redirect could not SetupRequest()\n"));
    return rv;
  }

  
  
  
  mRedirectCallback = callback;

  
  
  
  sWebSocketAdmissions->OnConnected(this);

  
  mAddress.Truncate();
  mOpenedHttpChannel = 0;
  rv = ApplyForAdmission();
  if (NS_FAILED(rv)) {
    LOG(("WebSocketChannel: Redirect failed due to DNS failure\n"));
    mRedirectCallback = nullptr;
    return rv;
  }

  return NS_OK;
}



NS_IMETHODIMP
WebSocketChannel::Notify(nsITimer *timer)
{
  LOG(("WebSocketChannel::Notify() %p [%p]\n", this, timer));

  if (timer == mCloseTimer) {
    NS_ABORT_IF_FALSE(mClientClosed, "Close Timeout without local close");
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread,
                      "not socket thread");

    mCloseTimer = nullptr;
    if (mStopped || mServerClosed)                
      return NS_OK;

    LOG(("WebSocketChannel:: Expecting Server Close - Timed Out\n"));
    AbortSession(NS_ERROR_NET_TIMEOUT);
  } else if (timer == mOpenTimer) {
    NS_ABORT_IF_FALSE(!mGotUpgradeOK,
                      "Open Timer after open complete");
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

    mOpenTimer = nullptr;
    LOG(("WebSocketChannel:: Connection Timed Out\n"));
    if (mStopped || mServerClosed)                
      return NS_OK;

    AbortSession(NS_ERROR_NET_TIMEOUT);
  } else if (timer == mReconnectDelayTimer) {
    NS_ABORT_IF_FALSE(mConnecting == CONNECTING_DELAYED,
                      "woke up from delay w/o being delayed?");

    mReconnectDelayTimer = nullptr;
    LOG(("WebSocketChannel: connecting [this=%p] after reconnect delay", this));
    BeginOpen();
  } else if (timer == mPingTimer) {
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread,
                      "not socket thread");

    if (mClientClosed || mServerClosed || mRequestedClose) {
      
      mPingTimer = nullptr;
      return NS_OK;
    }

    if (!mPingOutstanding) {
      LOG(("nsWebSocketChannel:: Generating Ping\n"));
      mPingOutstanding = 1;
      GeneratePing();
      mPingTimer->InitWithCallback(this, mPingResponseTimeout,
                                   nsITimer::TYPE_ONE_SHOT);
    } else {
      LOG(("nsWebSocketChannel:: Timed out Ping\n"));
      mPingTimer = nullptr;
      AbortSession(NS_ERROR_NET_TIMEOUT);
    }
  } else if (timer == mLingeringCloseTimer) {
    LOG(("WebSocketChannel:: Lingering Close Timer"));
    CleanupConnection();
  } else {
    NS_ABORT_IF_FALSE(0, "Unknown Timer");
  }

  return NS_OK;
}



NS_IMETHODIMP
WebSocketChannel::GetSecurityInfo(nsISupports **aSecurityInfo)
{
  LOG(("WebSocketChannel::GetSecurityInfo() %p\n", this));
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

  if (mTransport) {
    if (NS_FAILED(mTransport->GetSecurityInfo(aSecurityInfo)))
      *aSecurityInfo = nullptr;
  }
  return NS_OK;
}


NS_IMETHODIMP
WebSocketChannel::AsyncOpen(nsIURI *aURI,
                            const nsACString &aOrigin,
                            nsIWebSocketListener *aListener,
                            nsISupports *aContext)
{
  LOG(("WebSocketChannel::AsyncOpen() %p\n", this));

  NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

  if (!aURI || !aListener) {
    LOG(("WebSocketChannel::AsyncOpen() Uri or Listener null"));
    return NS_ERROR_UNEXPECTED;
  }

  if (mListener || mWasOpened)
    return NS_ERROR_ALREADY_OPENED;

  nsresult rv;

  mSocketThread = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    NS_WARNING("unable to continue without socket transport service");
    return rv;
  }

  mRandomGenerator =
    do_GetService("@mozilla.org/security/random-generator;1", &rv);
  if (NS_FAILED(rv)) {
    NS_WARNING("unable to continue without random number generator");
    return rv;
  }

  nsCOMPtr<nsIPrefBranch> prefService;
  prefService = do_GetService(NS_PREFSERVICE_CONTRACTID);

  if (prefService) {
    int32_t intpref;
    bool boolpref;
    rv = prefService->GetIntPref("network.websocket.max-message-size", 
                                 &intpref);
    if (NS_SUCCEEDED(rv)) {
      mMaxMessageSize = clamped(intpref, 1024, INT32_MAX);
    }
    rv = prefService->GetIntPref("network.websocket.timeout.close", &intpref);
    if (NS_SUCCEEDED(rv)) {
      mCloseTimeout = clamped(intpref, 1, 1800) * 1000;
    }
    rv = prefService->GetIntPref("network.websocket.timeout.open", &intpref);
    if (NS_SUCCEEDED(rv)) {
      mOpenTimeout = clamped(intpref, 1, 1800) * 1000;
    }
    rv = prefService->GetIntPref("network.websocket.timeout.ping.request",
                                 &intpref);
    if (NS_SUCCEEDED(rv) && !mClientSetPingInterval) {
      mPingInterval = clamped(intpref, 0, 86400) * 1000;
    }
    rv = prefService->GetIntPref("network.websocket.timeout.ping.response",
                                 &intpref);
    if (NS_SUCCEEDED(rv) && !mClientSetPingTimeout) {
      mPingResponseTimeout = clamped(intpref, 1, 3600) * 1000;
    }
    rv = prefService->GetBoolPref("network.websocket.extensions.stream-deflate",
                                  &boolpref);
    if (NS_SUCCEEDED(rv)) {
      mAllowCompression = boolpref ? 1 : 0;
    }
    rv = prefService->GetBoolPref("network.websocket.auto-follow-http-redirects",
                                  &boolpref);
    if (NS_SUCCEEDED(rv)) {
      mAutoFollowRedirects = boolpref ? 1 : 0;
    }
    rv = prefService->GetIntPref
      ("network.websocket.max-connections", &intpref);
    if (NS_SUCCEEDED(rv)) {
      mMaxConcurrentConnections = clamped(intpref, 1, 0xffff);
    }
  }

  if (sWebSocketAdmissions)
    LOG(("WebSocketChannel::AsyncOpen %p sessionCount=%d max=%d\n", this,
         sWebSocketAdmissions->SessionCount(), mMaxConcurrentConnections));

  if (sWebSocketAdmissions &&
      sWebSocketAdmissions->SessionCount() >= mMaxConcurrentConnections)
  {
    LOG(("WebSocketChannel: max concurrency %d exceeded (%d)",
         mMaxConcurrentConnections,
         sWebSocketAdmissions->SessionCount()));

    
    
    return NS_ERROR_SOCKET_CREATE_FAILED;
  }

  mOriginalURI = aURI;
  mURI = mOriginalURI;
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
    NS_WARNING("WebSocketChannel: unable to continue without ioservice2");
    return rv;
  }

  rv = io2->NewChannelFromURIWithProxyFlags(
              localURI,
              mURI,
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

  if (mConnectionLogService) {
    nsAutoCString host;
    rv = mURI->GetHostPort(host);
    if (NS_SUCCEEDED(rv)) {
      mConnectionLogService->AddHost(host, mSerial, BaseWebSocketChannel::mEncrypted);
    }
  }

  rv = ApplyForAdmission();
  if (NS_FAILED(rv))
    return rv;

  
  
  mWasOpened = 1;
  mListener = aListener;
  mContext = aContext;
  IncrementSessionCount();

  return rv;
}

NS_IMETHODIMP
WebSocketChannel::Close(uint16_t code, const nsACString & reason)
{
  LOG(("WebSocketChannel::Close() %p\n", this));
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

  if (mRequestedClose) {
    return NS_OK;
  }

  
  if (reason.Length() > 123)
    return NS_ERROR_ILLEGAL_VALUE;

  mRequestedClose = 1;
  mScriptCloseReason = reason;
  mScriptCloseCode = code;

  if (!mTransport) {
    nsresult rv;
    if (code == CLOSE_GOING_AWAY) {
      
      LOG(("WebSocketChannel::Close() GOING_AWAY without transport."));
      rv = NS_OK;
    } else {
      LOG(("WebSocketChannel::Close() without transport - error."));
      rv = NS_ERROR_NOT_CONNECTED;
    }
    StopSession(rv);
    return rv;
  }

  return mSocketThread->Dispatch(
      new OutboundEnqueuer(this, new OutboundMessage(kMsgTypeFin, nullptr)),
                           nsIEventTarget::DISPATCH_NORMAL);
}

NS_IMETHODIMP
WebSocketChannel::SendMsg(const nsACString &aMsg)
{
  LOG(("WebSocketChannel::SendMsg() %p\n", this));

  return SendMsgCommon(&aMsg, false, aMsg.Length());
}

NS_IMETHODIMP
WebSocketChannel::SendBinaryMsg(const nsACString &aMsg)
{
  LOG(("WebSocketChannel::SendBinaryMsg() %p len=%d\n", this, aMsg.Length()));
  return SendMsgCommon(&aMsg, true, aMsg.Length());
}

NS_IMETHODIMP
WebSocketChannel::SendBinaryStream(nsIInputStream *aStream, uint32_t aLength)
{
  LOG(("WebSocketChannel::SendBinaryStream() %p\n", this));

  return SendMsgCommon(nullptr, true, aLength, aStream);
}

nsresult
WebSocketChannel::SendMsgCommon(const nsACString *aMsg, bool aIsBinary,
                                uint32_t aLength, nsIInputStream *aStream)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

  if (mRequestedClose) {
    LOG(("WebSocketChannel:: Error: send when closed\n"));
    return NS_ERROR_UNEXPECTED;
  }

  if (mStopped) {
    LOG(("WebSocketChannel:: Error: send when stopped\n"));
    return NS_ERROR_NOT_CONNECTED;
  }

  NS_ABORT_IF_FALSE(mMaxMessageSize >= 0, "max message size negative");
  if (aLength > static_cast<uint32_t>(mMaxMessageSize)) {
    LOG(("WebSocketChannel:: Error: message too big\n"));
    return NS_ERROR_FILE_TOO_BIG;
  }

  nsresult rv;
  if (mConnectionLogService) {
    nsAutoCString host;
    rv = mURI->GetHostPort(host);
    if (NS_SUCCEEDED(rv)) {
      mConnectionLogService->NewMsgSent(host, mSerial, aLength);
      LOG(("Added new msg sent for %s",host.get()));
    }
  }

  return mSocketThread->Dispatch(
    aStream ? new OutboundEnqueuer(this, new OutboundMessage(aStream, aLength))
            : new OutboundEnqueuer(this,
                     new OutboundMessage(aIsBinary ? kMsgTypeBinaryString
                                                   : kMsgTypeString,
                                         new nsCString(*aMsg))),
    nsIEventTarget::DISPATCH_NORMAL);
}



NS_IMETHODIMP
WebSocketChannel::OnTransportAvailable(nsISocketTransport *aTransport,
                                       nsIAsyncInputStream *aSocketIn,
                                       nsIAsyncOutputStream *aSocketOut)
{
  if (!NS_IsMainThread()) {
    return NS_DispatchToMainThread(new CallOnTransportAvailable(this,
                                                                aTransport,
                                                                aSocketIn,
                                                                aSocketOut));
  }

  LOG(("WebSocketChannel::OnTransportAvailable %p [%p %p %p] rcvdonstart=%d\n",
       this, aTransport, aSocketIn, aSocketOut, mGotUpgradeOK));

  NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");
  NS_ABORT_IF_FALSE(!mRecvdHttpUpgradeTransport, "OTA duplicated");
  NS_ABORT_IF_FALSE(aSocketIn, "OTA with invalid socketIn");

  mTransport = aTransport;
  mSocketIn = aSocketIn;
  mSocketOut = aSocketOut;

  nsresult rv;
  rv = mTransport->SetEventSink(nullptr, nullptr);
  if (NS_FAILED(rv)) return rv;
  rv = mTransport->SetSecurityCallbacks(this);
  if (NS_FAILED(rv)) return rv;

  mRecvdHttpUpgradeTransport = 1;
  if (mGotUpgradeOK)
    return StartWebsocketData();
  return NS_OK;
}



NS_IMETHODIMP
WebSocketChannel::OnStartRequest(nsIRequest *aRequest,
                                 nsISupports *aContext)
{
  LOG(("WebSocketChannel::OnStartRequest(): %p [%p %p] recvdhttpupgrade=%d\n",
       this, aRequest, aContext, mRecvdHttpUpgradeTransport));
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");
  NS_ABORT_IF_FALSE(!mGotUpgradeOK, "OTA duplicated");

  if (mOpenTimer) {
    mOpenTimer->Cancel();
    mOpenTimer = nullptr;
  }

  if (mStopped) {
    LOG(("WebSocketChannel::OnStartRequest: Channel Already Done\n"));
    AbortSession(NS_ERROR_CONNECTION_REFUSED);
    return NS_ERROR_CONNECTION_REFUSED;
  }

  nsresult rv;
  uint32_t status;
  char *val, *token;

  rv = mHttpChannel->GetResponseStatus(&status);
  if (NS_FAILED(rv)) {
    LOG(("WebSocketChannel::OnStartRequest: No HTTP Response\n"));
    AbortSession(NS_ERROR_CONNECTION_REFUSED);
    return NS_ERROR_CONNECTION_REFUSED;
  }

  LOG(("WebSocketChannel::OnStartRequest: HTTP status %d\n", status));
  if (status != 101) {
    AbortSession(NS_ERROR_CONNECTION_REFUSED);
    return NS_ERROR_CONNECTION_REFUSED;
  }

  nsAutoCString respUpgrade;
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
    LOG(("WebSocketChannel::OnStartRequest: "
         "HTTP response header Upgrade: websocket not found\n"));
    AbortSession(NS_ERROR_ILLEGAL_VALUE);
    return rv;
  }

  nsAutoCString respConnection;
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
    LOG(("WebSocketChannel::OnStartRequest: "
         "HTTP response header 'Connection: Upgrade' not found\n"));
    AbortSession(NS_ERROR_ILLEGAL_VALUE);
    return rv;
  }

  nsAutoCString respAccept;
  rv = mHttpChannel->GetResponseHeader(
                       NS_LITERAL_CSTRING("Sec-WebSocket-Accept"),
                       respAccept);

  if (NS_FAILED(rv) ||
    respAccept.IsEmpty() || !respAccept.Equals(mHashedSecret)) {
    LOG(("WebSocketChannel::OnStartRequest: "
         "HTTP response header Sec-WebSocket-Accept check failed\n"));
    LOG(("WebSocketChannel::OnStartRequest: Expected %s received %s\n",
         mHashedSecret.get(), respAccept.get()));
    AbortSession(NS_ERROR_ILLEGAL_VALUE);
    return NS_ERROR_ILLEGAL_VALUE;
  }

  
  
  
  if (!mProtocol.IsEmpty()) {
    nsAutoCString respProtocol;
    rv = mHttpChannel->GetResponseHeader(
                         NS_LITERAL_CSTRING("Sec-WebSocket-Protocol"), 
                         respProtocol);
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
        LOG(("WebsocketChannel::OnStartRequest: subprotocol %s confirmed",
             respProtocol.get()));
        mProtocol = respProtocol;
      } else {
        LOG(("WebsocketChannel::OnStartRequest: "
             "subprotocol [%s] not found - %s returned",
             mProtocol.get(), respProtocol.get()));
        mProtocol.Truncate();
      }
    } else {
      LOG(("WebsocketChannel::OnStartRequest "
                 "subprotocol [%s] not found - none returned",
                 mProtocol.get()));
      mProtocol.Truncate();
    }
  }

  rv = HandleExtensions();
  if (NS_FAILED(rv))
    return rv;

  mGotUpgradeOK = 1;
  if (mRecvdHttpUpgradeTransport)
    return StartWebsocketData();

  return NS_OK;
}

NS_IMETHODIMP
WebSocketChannel::OnStopRequest(nsIRequest *aRequest,
                                  nsISupports *aContext,
                                  nsresult aStatusCode)
{
  LOG(("WebSocketChannel::OnStopRequest() %p [%p %p %x]\n",
       this, aRequest, aContext, aStatusCode));
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

  ReportConnectionTelemetry();

  
  

  mChannel = nullptr;
  mHttpChannel = nullptr;
  mLoadGroup = nullptr;
  mCallbacks = nullptr;

  return NS_OK;
}



NS_IMETHODIMP
WebSocketChannel::OnInputStreamReady(nsIAsyncInputStream *aStream)
{
  LOG(("WebSocketChannel::OnInputStreamReady() %p\n", this));
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "not socket thread");

  if (!mSocketIn) 
    return NS_OK;
  
  nsRefPtr<nsIStreamListener>    deleteProtector1(mInflateReader);
  nsRefPtr<nsIStringInputStream> deleteProtector2(mInflateStream);

  
  char  buffer[2048];
  uint32_t count;
  nsresult rv;

  do {
    rv = mSocketIn->Read((char *)buffer, 2048, &count);
    LOG(("WebSocketChannel::OnInputStreamReady: read %u rv %x\n", count, rv));

    if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
      mSocketIn->AsyncWait(this, 0, 0, mSocketThread);
      return NS_OK;
    }

    if (NS_FAILED(rv)) {
      mTCPClosed = true;
      AbortSession(rv);
      return rv;
    }

    if (count == 0) {
      mTCPClosed = true;
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
      rv = mInflateReader->OnDataAvailable(nullptr, mSocketIn, mInflateStream, 
                                           0, count);
    } else {
      rv = ProcessInput((uint8_t *)buffer, count);
    }

    if (NS_FAILED(rv)) {
      AbortSession(rv);
      return rv;
    }
  } while (NS_SUCCEEDED(rv) && mSocketIn);

  return NS_OK;
}




NS_IMETHODIMP
WebSocketChannel::OnOutputStreamReady(nsIAsyncOutputStream *aStream)
{
  LOG(("WebSocketChannel::OnOutputStreamReady() %p\n", this));
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "not socket thread");
  nsresult rv;

  if (!mCurrentOut)
    PrimeNewOutgoingMessage();

  while (mCurrentOut && mSocketOut) {
    const char *sndBuf;
    uint32_t toSend;
    uint32_t amtSent;

    if (mHdrOut) {
      sndBuf = (const char *)mHdrOut;
      toSend = mHdrOutToSend;
      LOG(("WebSocketChannel::OnOutputStreamReady: "
           "Try to send %u of hdr/copybreak\n", toSend));
    } else {
      sndBuf = (char *) mCurrentOut->BeginReading() + mCurrentOutSent;
      toSend = mCurrentOut->Length() - mCurrentOutSent;
      if (toSend > 0) {
        LOG(("WebSocketChannel::OnOutputStreamReady: "
             "Try to send %u of data\n", toSend));
      }
    }

    if (toSend == 0) {
      amtSent = 0;
    } else {
      rv = mSocketOut->Write(sndBuf, toSend, &amtSent);
      LOG(("WebSocketChannel::OnOutputStreamReady: write %u rv %x\n",
           amtSent, rv));

      if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
        mSocketOut->AsyncWait(this, 0, 0, nullptr);
        return NS_OK;
      }

      if (NS_FAILED(rv)) {
        AbortSession(rv);
        return NS_OK;
      }
    }

    if (mHdrOut) {
      if (amtSent == toSend) {
        mHdrOut = nullptr;
        mHdrOutToSend = 0;
      } else {
        mHdrOut += amtSent;
        mHdrOutToSend -= amtSent;
      }
    } else {
      if (amtSent == toSend) {
        if (!mStopped) {
          NS_DispatchToMainThread(new CallAcknowledge(this,
                                                      mCurrentOut->Length()));
        }
        DeleteCurrentOutGoingMessage();
        PrimeNewOutgoingMessage();
      } else {
        mCurrentOutSent += amtSent;
      }
    }
  }

  if (mReleaseOnTransmit)
    ReleaseSession();
  return NS_OK;
}



NS_IMETHODIMP
WebSocketChannel::OnDataAvailable(nsIRequest *aRequest,
                                    nsISupports *aContext,
                                    nsIInputStream *aInputStream,
                                    uint64_t aOffset,
                                    uint32_t aCount)
{
  LOG(("WebSocketChannel::OnDataAvailable() %p [%p %p %p %llu %u]\n",
         this, aRequest, aContext, aInputStream, aOffset, aCount));

  if (aContext == mSocketIn) {
    

    LOG(("WebSocketChannel::OnDataAvailable: Deflate Data %u\n",
             aCount));

    uint8_t  buffer[2048];
    uint32_t maxRead;
    uint32_t count;
    nsresult rv = NS_OK;  

    while (aCount > 0) {
      if (mStopped)
        return NS_BASE_STREAM_CLOSED;

      maxRead = std::min(2048U, aCount);
      rv = aInputStream->Read((char *)buffer, maxRead, &count);
      LOG(("WebSocketChannel::OnDataAvailable: InflateRead read %u rv %x\n",
           count, rv));
      if (NS_FAILED(rv) || count == 0) {
        AbortSession(NS_ERROR_UNEXPECTED);
        break;
      }

      aCount -= count;
      rv = ProcessInput(buffer, count);
      if (NS_FAILED(rv)) {
        AbortSession(rv);
        break;
      }
    }
    return rv;
  }

  if (aContext == mSocketOut) {
    

    uint32_t maxRead;
    uint32_t count;
    nsresult rv;

    while (aCount > 0) {
      if (mStopped)
        return NS_BASE_STREAM_CLOSED;

      maxRead = std::min(2048U, aCount);
      EnsureHdrOut(mHdrOutToSend + aCount);
      rv = aInputStream->Read((char *)mHdrOut + mHdrOutToSend, maxRead, &count);
      LOG(("WebSocketChannel::OnDataAvailable: DeflateWrite read %u rv %x\n", 
           count, rv));
      if (NS_FAILED(rv) || count == 0) {
        AbortSession(rv);
        break;
      }

      mHdrOutToSend += count;
      aCount -= count;
    }
    return NS_OK;
  }


  
  
  

  
  

  LOG(("WebSocketChannel::OnDataAvailable: HTTP data unexpected len>=%u\n",
         aCount));

  return NS_OK;
}

} 
} 
