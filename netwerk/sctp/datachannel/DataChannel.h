





#ifndef NETWERK_SCTP_DATACHANNEL_DATACHANNEL_H_
#define NETWERK_SCTP_DATACHANNEL_DATACHANNEL_H_

#ifdef MOZ_WEBRTC_SIGNALING
#define SCTP_DTLS_SUPPORTED 1
#endif

#include <string>
#include <errno.h>
#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "mozilla/WeakPtr.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsTArray.h"
#include "nsDeque.h"
#include "nsIInputStream.h"
#include "nsITimer.h"
#include "mozilla/Mutex.h"
#include "DataChannelProtocol.h"
#include "DataChannelListener.h"
#ifdef SCTP_DTLS_SUPPORTED
#include "mtransport/sigslot.h"
#include "mtransport/transportflow.h"
#include "mtransport/transportlayer.h"
#include "mtransport/transportlayerdtls.h"
#include "mtransport/transportlayerprsock.h"
#endif

#ifndef DATACHANNEL_LOG
#define DATACHANNEL_LOG(args)
#endif

#ifndef EALREADY
#define EALREADY  WSAEALREADY
#endif

extern "C" {
  struct socket;
  struct sctp_rcvinfo;
}

namespace mozilla {

class DTLSConnection;
class DataChannelConnection;
class DataChannel;
class DataChannelOnMessageAvailable;


class BufferedMsg
{
public:
  BufferedMsg(struct sctp_sendv_spa &spa,const char *data,
              uint32_t length);
  ~BufferedMsg();

  struct sctp_sendv_spa *mSpa;
  const char *mData;
  uint32_t mLength;
};



class QueuedDataMessage
{
public:
  QueuedDataMessage(uint16_t stream, uint32_t ppid,
             const void *data, size_t length)
    : mStream(stream)
    , mPpid(ppid)
    , mLength(length)
  {
    mData = static_cast<char *>(moz_xmalloc(length)); 
    memcpy(mData, data, length);
  }

  ~QueuedDataMessage()
  {
    free(mData);
  }

  uint16_t mStream;
  uint32_t mPpid;
  size_t   mLength;
  char     *mData;
};


class DataChannelConnection: public nsITimerCallback
#ifdef SCTP_DTLS_SUPPORTED
                             , public sigslot::has_slots<>
#endif
{
  virtual ~DataChannelConnection();

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

  class DataConnectionListener : public SupportsWeakPtr<DataConnectionListener>
  {
  public:
    MOZ_DECLARE_REFCOUNTED_TYPENAME(DataChannelConnection::DataConnectionListener)
    virtual ~DataConnectionListener() {}

    
    virtual void NotifyDataChannel(already_AddRefed<DataChannel> channel) = 0;
  };

  explicit DataChannelConnection(DataConnectionListener *listener);

  bool Init(unsigned short aPort, uint16_t aNumStreams, bool aUsingDtls);
  void Destroy(); 
  
  void DestroyOnSTS(struct socket *aMasterSocket,
                    struct socket *aSocket);

#ifdef ALLOW_DIRECT_SCTP_LISTEN_CONNECT
  
  
  
  bool Listen(unsigned short port);
  bool Connect(const char *addr, unsigned short port);
#endif

#ifdef SCTP_DTLS_SUPPORTED
  
  void SetEvenOdd();
  bool ConnectViaTransportFlow(TransportFlow *aFlow, uint16_t localport, uint16_t remoteport);
  void CompleteConnect(TransportFlow *flow, TransportLayer::State state);
  void SetSignals();
#endif

  typedef enum {
    RELIABLE=0,
    PARTIAL_RELIABLE_REXMIT = 1,
    PARTIAL_RELIABLE_TIMED = 2
  } Type;

  MOZ_WARN_UNUSED_RESULT
  already_AddRefed<DataChannel> Open(const nsACString& label,
                                     const nsACString& protocol,
                                     Type type, bool inOrder,
                                     uint32_t prValue,
                                     DataChannelListener *aListener,
                                     nsISupports *aContext,
                                     bool aExternalNegotiated,
                                     uint16_t aStream);

  void Close(DataChannel *aChannel);
  
  void CloseInt(DataChannel *aChannel);
  void CloseAll();

  int32_t SendMsg(uint16_t stream, const nsACString &aMsg)
    {
      return SendMsgCommon(stream, aMsg, false);
    }
  int32_t SendBinaryMsg(uint16_t stream, const nsACString &aMsg)
    {
      return SendMsgCommon(stream, aMsg, true);
    }
  int32_t SendBlob(uint16_t stream, nsIInputStream *aBlob);

  
  
  int ReceiveCallback(struct socket* sock, void *data, size_t datalen,
                      struct sctp_rcvinfo rcv, int32_t flags);

  
  enum {
    CONNECTING = 0U,
    OPEN = 1U,
    CLOSING = 2U,
    CLOSED = 3U
  };
  uint16_t GetReadyState() { MutexAutoLock lock(mLock); return mState; }

  friend class DataChannel;
  Mutex  mLock;

  void ReadBlob(already_AddRefed<DataChannelConnection> aThis, uint16_t aStream, nsIInputStream* aBlob);

  void GetStreamIds(std::vector<uint16_t>* aStreamList);

protected:
  friend class DataChannelOnMessageAvailable;
  
  
  WeakPtr<DataConnectionListener> mListener;

private:
  friend class DataChannelConnectRunnable;

#ifdef SCTP_DTLS_SUPPORTED
  static void DTLSConnectThread(void *data);
  int SendPacket(unsigned char data[], size_t len, bool release);
  void SctpDtlsInput(TransportFlow *flow, const unsigned char *data, size_t len);
  static int SctpDtlsOutput(void *addr, void *buffer, size_t length, uint8_t tos, uint8_t set_df);
#endif
  DataChannel* FindChannelByStream(uint16_t stream);
  uint16_t FindFreeStream();
  bool RequestMoreStreams(int32_t aNeeded = 16);
  int32_t SendControlMessage(void *msg, uint32_t len, uint16_t stream);
  int32_t SendOpenRequestMessage(const nsACString& label, const nsACString& protocol,
                                 uint16_t stream,
                                 bool unordered, uint16_t prPolicy, uint32_t prValue);
  int32_t SendOpenAckMessage(uint16_t stream);
  int32_t SendMsgInternal(DataChannel *channel, const char *data,
                          uint32_t length, uint32_t ppid);
  int32_t SendBinary(DataChannel *channel, const char *data,
                     uint32_t len, uint32_t ppid_partial, uint32_t ppid_final);
  int32_t SendMsgCommon(uint16_t stream, const nsACString &aMsg, bool isBinary);

  void DeliverQueuedData(uint16_t stream);

  already_AddRefed<DataChannel> OpenFinish(already_AddRefed<DataChannel>&& aChannel);

  void StartDefer();
  bool SendDeferredMessages();
  void ProcessQueuedOpens();
  void ClearResets();
  void SendOutgoingStreamReset();
  void ResetOutgoingStream(uint16_t stream);
  void HandleOpenRequestMessage(const struct rtcweb_datachannel_open_request *req,
                                size_t length,
                                uint16_t stream);
  void HandleOpenAckMessage(const struct rtcweb_datachannel_ack *ack,
                            size_t length, uint16_t stream);
  void HandleUnknownMessage(uint32_t ppid, size_t length, uint16_t stream);
  void HandleDataMessage(uint32_t ppid, const void *buffer, size_t length, uint16_t stream);
  void HandleMessage(const void *buffer, size_t length, uint32_t ppid, uint16_t stream);
  void HandleAssociationChangeEvent(const struct sctp_assoc_change *sac);
  void HandlePeerAddressChangeEvent(const struct sctp_paddr_change *spc);
  void HandleRemoteErrorEvent(const struct sctp_remote_error *sre);
  void HandleShutdownEvent(const struct sctp_shutdown_event *sse);
  void HandleAdaptationIndication(const struct sctp_adaptation_event *sai);
  void HandleSendFailedEvent(const struct sctp_send_failed_event *ssfe);
  void HandleStreamResetEvent(const struct sctp_stream_reset_event *strrst);
  void HandleStreamChangeEvent(const struct sctp_stream_change_event *strchg);
  void HandleNotification(const union sctp_notification *notif, size_t n);

#ifdef SCTP_DTLS_SUPPORTED
  bool IsSTSThread() {
    bool on = false;
    if (mSTS) {
      mSTS->IsOnCurrentThread(&on);
    }
    return on;
  }
#endif

  
  static void ReleaseTransportFlow(nsRefPtr<TransportFlow> aFlow) {}

  
  
  
  bool mAllocateEven;
  nsAutoTArray<nsRefPtr<DataChannel>,16> mStreams;
  nsDeque mPending; 
  
  nsTArray<nsAutoPtr<QueuedDataMessage> > mQueuedData;

  
  nsAutoTArray<uint16_t,4> mStreamsResetting;

  struct socket *mMasterSocket; 
  struct socket *mSocket; 
  uint16_t mState; 

#ifdef SCTP_DTLS_SUPPORTED
  nsRefPtr<TransportFlow> mTransportFlow;
  nsCOMPtr<nsIEventTarget> mSTS;
#endif
  uint16_t mLocalPort; 
  uint16_t mRemotePort;
  bool mUsingDtls;

  
  nsCOMPtr<nsITimer> mDeferredTimer;
  uint32_t mDeferTimeout; 
  bool mTimerRunning;
  nsCOMPtr<nsIThread> mInternalIOThread;
};

#define ENSURE_DATACONNECTION \
  do { if (!mConnection) { DATACHANNEL_LOG(("%s: %p no connection!",__FUNCTION__, this)); return; } } while (0)

#define ENSURE_DATACONNECTION_RET(x) \
  do { if (!mConnection) { DATACHANNEL_LOG(("%s: %p no connection!",__FUNCTION__, this)); return (x); } } while (0)

class DataChannel {
public:
  enum {
    CONNECTING = 0U,
    OPEN = 1U,
    CLOSING = 2U,
    CLOSED = 3U,
    WAITING_TO_OPEN = 4U
  };

  DataChannel(DataChannelConnection *connection,
              uint16_t stream,
              uint16_t state,
              const nsACString& label,
              const nsACString& protocol,
              uint16_t policy, uint32_t value,
              uint32_t flags,
              DataChannelListener *aListener,
              nsISupports *aContext)
    : mListenerLock("netwerk::sctp::DataChannel")
    , mListener(aListener)
    , mContext(aContext)
    , mConnection(connection)
    , mLabel(label)
    , mProtocol(protocol)
    , mState(state)
    , mReady(false)
    , mStream(stream)
    , mPrPolicy(policy)
    , mPrValue(value)
    , mFlags(flags)
    , mIsRecvBinary(false)
    {
      NS_ASSERTION(mConnection,"NULL connection");
    }

private:
  ~DataChannel();

public:
  void Destroy(); 

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DataChannel)

  
  
  void Close();

  
  void SetListener(DataChannelListener *aListener, nsISupports *aContext);

  
  bool SendMsg(const nsACString &aMsg)
    {
      ENSURE_DATACONNECTION_RET(false);

      if (mStream != INVALID_STREAM)
        return (mConnection->SendMsg(mStream, aMsg) > 0);
      else
        return false;
    }

  
  bool SendBinaryMsg(const nsACString &aMsg)
    {
      ENSURE_DATACONNECTION_RET(false);

      if (mStream != INVALID_STREAM)
        return (mConnection->SendBinaryMsg(mStream, aMsg) > 0);
      else
        return false;
    }

  
  bool SendBinaryStream(nsIInputStream *aBlob, uint32_t msgLen)
    {
      ENSURE_DATACONNECTION_RET(false);

      if (mStream != INVALID_STREAM)
        return (mConnection->SendBlob(mStream, aBlob) > 0);
      else
        return false;
    }

  uint16_t GetType() { return mPrPolicy; }

  bool GetOrdered() { return !(mFlags & DATA_CHANNEL_FLAGS_OUT_OF_ORDER_ALLOWED); }

  
  uint32_t GetBufferedAmount();

  
  uint16_t GetReadyState()
    {
      if (mConnection) {
        MutexAutoLock lock(mConnection->mLock);
        if (mState == WAITING_TO_OPEN)
          return CONNECTING;
        return mState;
      }
      return CLOSED;
    }

  void GetLabel(nsAString& aLabel) { CopyUTF8toUTF16(mLabel, aLabel); }
  void GetProtocol(nsAString& aProtocol) { CopyUTF8toUTF16(mProtocol, aProtocol); }
  uint16_t GetStream() { return mStream; }

  void AppReady();

  void SendOrQueue(DataChannelOnMessageAvailable *aMessage);

protected:
  Mutex mListenerLock; 
  DataChannelListener *mListener;
  nsCOMPtr<nsISupports> mContext;

private:
  friend class DataChannelOnMessageAvailable;
  friend class DataChannelConnection;

  nsresult AddDataToBinaryMsg(const char *data, uint32_t size);

  nsRefPtr<DataChannelConnection> mConnection;
  nsCString mLabel;
  nsCString mProtocol;
  uint16_t mState;
  bool     mReady;
  uint16_t mStream;
  uint16_t mPrPolicy;
  uint32_t mPrValue;
  uint32_t mFlags;
  uint32_t mId;
  bool mIsRecvBinary;
  nsCString mRecvBuffer;
  nsTArray<nsAutoPtr<BufferedMsg> > mBufferedData;
  nsTArray<nsCOMPtr<nsIRunnable> > mQueuedMessages;
};




class DataChannelOnMessageAvailable : public nsRunnable
{
public:
  enum {
    ON_CONNECTION,
    ON_DISCONNECTED,
    ON_CHANNEL_CREATED,
    ON_CHANNEL_OPEN,
    ON_CHANNEL_CLOSED,
    ON_DATA,
    START_DEFER,
  };  

  DataChannelOnMessageAvailable(int32_t     aType,
                                DataChannelConnection *aConnection,
                                DataChannel *aChannel,
                                nsCString   &aData,  
                                int32_t     aLen)
    : mType(aType),
      mChannel(aChannel),
      mConnection(aConnection),
      mData(aData),
      mLen(aLen) {}

  DataChannelOnMessageAvailable(int32_t     aType,
                                DataChannel *aChannel)
    : mType(aType),
      mChannel(aChannel) {}
  
  
  

  DataChannelOnMessageAvailable(int32_t     aType,
                                DataChannelConnection *aConnection,
                                DataChannel *aChannel)
    : mType(aType),
      mChannel(aChannel),
      mConnection(aConnection) {}

  
  DataChannelOnMessageAvailable(int32_t     aType,
                                DataChannelConnection *aConnection)
    : mType(aType),
      mConnection(aConnection) {}

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    switch (mType) {
      case ON_DATA:
      case ON_CHANNEL_OPEN:
      case ON_CHANNEL_CLOSED:
        {
          MutexAutoLock lock(mChannel->mListenerLock);
          if (!mChannel->mListener) {
            DATACHANNEL_LOG(("DataChannelOnMessageAvailable (%d) with null Listener!",mType));
            return NS_OK;
          }

          switch (mType) {
            case ON_DATA:
              if (mLen < 0) {
                mChannel->mListener->OnMessageAvailable(mChannel->mContext, mData);
              } else {
                mChannel->mListener->OnBinaryMessageAvailable(mChannel->mContext, mData);
              }
              break;
            case ON_CHANNEL_OPEN:
              mChannel->mListener->OnChannelConnected(mChannel->mContext);
              break;
            case ON_CHANNEL_CLOSED:
              mChannel->mListener->OnChannelClosed(mChannel->mContext);
              break;
          }
          break;
        }
      case ON_DISCONNECTED:
        
        mConnection->CloseAll();
        
      case ON_CHANNEL_CREATED:
      case ON_CONNECTION:
        
        if (!mConnection->mListener) {
          DATACHANNEL_LOG(("DataChannelOnMessageAvailable (%d) with null Listener",mType));
          return NS_OK;
        }
        switch (mType) {
          case ON_CHANNEL_CREATED:
            
            mConnection->mListener->NotifyDataChannel(mChannel.forget());
            break;
          default:
            break;
        }
        break;
      case START_DEFER:
        mConnection->StartDefer();
        break;
    }
    return NS_OK;
  }

private:
  ~DataChannelOnMessageAvailable() {}

  int32_t                           mType;
  
  nsRefPtr<DataChannel>             mChannel;
  nsRefPtr<DataChannelConnection>   mConnection;
  nsCString                         mData;
  int32_t                           mLen;
};

}

#endif  
