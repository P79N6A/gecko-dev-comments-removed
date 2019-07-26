





#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#if !defined(__Userspace_os_Windows)
#include <arpa/inet.h>
#endif

#define SCTP_DEBUG 1
#define SCTP_STDINT_INCLUDE "mozilla/StandardInteger.h"
#include "usrsctp.h"

#include "DataChannelLog.h"

#include "nsServiceManagerUtils.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "mozilla/Services.h"
#include "nsThreadUtils.h"
#include "nsAutoPtr.h"
#include "nsNetUtil.h"
#ifdef MOZ_PEERCONNECTION
#include "mtransport/runnable_utils.h"
#endif

#define DATACHANNEL_LOG(args) LOG(args)
#include "DataChannel.h"
#include "DataChannelProtocol.h"

#ifdef PR_LOGGING
PRLogModuleInfo*
GetDataChannelLog()
{
  static PRLogModuleInfo* sLog;
  if (!sLog)
    sLog = PR_NewLogModule("DataChannel");
  return sLog;
}

PRLogModuleInfo*
GetSCTPLog()
{
  static PRLogModuleInfo* sLog;
  if (!sLog)
    sLog = PR_NewLogModule("SCTP");
  return sLog;
}
#endif


#ifdef DEBUG
#define ASSERT_WEBRTC(x) MOZ_ASSERT((x))
#elif defined(MOZ_WEBRTC_ASSERT_ALWAYS)
#define ASSERT_WEBRTC(x) do { if (!(x)) { MOZ_CRASH(); } } while (0)
#endif

static bool sctp_initialized;

namespace mozilla {

class DataChannelShutdown;
nsRefPtr<DataChannelShutdown> gDataChannelShutdown;

class DataChannelShutdown : public nsIObserver
{
public:
  
  
  
  
  

  NS_DECL_ISUPPORTS

  DataChannelShutdown() {}

  void Init()
    {
      nsCOMPtr<nsIObserverService> observerService =
        mozilla::services::GetObserverService();
      if (!observerService)
        return;

      nsresult rv = observerService->AddObserver(this,
                                                 "profile-change-net-teardown",
                                                 false);
      MOZ_ASSERT(rv == NS_OK);
      (void) rv;
    }

  virtual ~DataChannelShutdown()
    {
      nsCOMPtr<nsIObserverService> observerService =
        mozilla::services::GetObserverService();
      if (observerService)
        observerService->RemoveObserver(this, "profile-change-net-teardown");
    }

  NS_IMETHODIMP Observe(nsISupports* aSubject, const char* aTopic,
                        const PRUnichar* aData) {
    if (strcmp(aTopic, "profile-change-net-teardown") == 0) {
      LOG(("Shutting down SCTP"));
      if (sctp_initialized) {
        usrsctp_finish();
        sctp_initialized = false;
      }
      nsCOMPtr<nsIObserverService> observerService =
        mozilla::services::GetObserverService();
      if (!observerService)
        return NS_ERROR_FAILURE;

      nsresult rv = observerService->RemoveObserver(this,
                                                    "profile-change-net-teardown");
      MOZ_ASSERT(rv == NS_OK);
      (void) rv;

      nsRefPtr<DataChannelShutdown> kungFuDeathGrip(this);
      gDataChannelShutdown = nullptr;
    }
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS1(DataChannelShutdown, nsIObserver);


BufferedMsg::BufferedMsg(struct sctp_sendv_spa &spa, const char *data,
                         uint32_t length) : mLength(length)
{
  mSpa = new sctp_sendv_spa;
  *mSpa = spa;
  char *tmp = new char[length]; 
  memcpy(tmp, data, length);
  mData = tmp;
}

BufferedMsg::~BufferedMsg()
{
  delete mSpa;
  delete mData;
}

static int
receive_cb(struct socket* sock, union sctp_sockstore addr,
           void *data, size_t datalen,
           struct sctp_rcvinfo rcv, int flags, void *ulp_info)
{
  DataChannelConnection *connection = static_cast<DataChannelConnection*>(ulp_info);
  return connection->ReceiveCallback(sock, data, datalen, rcv, flags);
}

#ifdef PR_LOGGING
static void
debug_printf(const char *format, ...)
{
  va_list ap;
  char buffer[1024];

  if (PR_LOG_TEST(GetSCTPLog(), PR_LOG_ALWAYS)) {
    va_start(ap, format);
#ifdef _WIN32
    if (vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, ap) > 0) {
#else
    if (vsnprintf(buffer, sizeof(buffer), format, ap) > 0) {
#endif
      PR_LogPrint("%s", buffer);
    }
    va_end(ap);
  }
}
#endif

DataChannelConnection::DataChannelConnection(DataConnectionListener *listener) :
   mLock("netwerk::sctp::DataChannelConnection")
{
  mState = CLOSED;
  mSocket = nullptr;
  mMasterSocket = nullptr;
  mListener = listener->asWeakPtr();
  mLocalPort = 0;
  mRemotePort = 0;
  mDeferTimeout = 10;
  mTimerRunning = false;
  LOG(("Constructor DataChannelConnection=%p, listener=%p", this, mListener.get()));
}

DataChannelConnection::~DataChannelConnection()
{
  LOG(("Deleting DataChannelConnection %p", (void *) this));
  
  ASSERT_WEBRTC(mState == CLOSED);
  MOZ_ASSERT(!mMasterSocket);
  MOZ_ASSERT(mPending.GetSize() == 0);

  
  
  if (mTransportFlow && !IsSTSThread()) {
    ASSERT_WEBRTC(mSTS);
    RUN_ON_THREAD(mSTS, WrapRunnableNM(ReleaseTransportFlow, mTransportFlow.forget()),
                  NS_DISPATCH_NORMAL);
  }
}

void
DataChannelConnection::Destroy()
{
  
  
  
  
  LOG(("Destroying DataChannelConnection %p", (void *) this));
  ASSERT_WEBRTC(NS_IsMainThread());
  CloseAll();

  if (mSocket && mSocket != mMasterSocket)
    usrsctp_close(mSocket);
  if (mMasterSocket)
    usrsctp_close(mMasterSocket);

  mSocket = nullptr;
  mMasterSocket = nullptr;

  
  

  

  if (mTransportFlow) {
    MOZ_ASSERT(mSTS);
    ASSERT_WEBRTC(NS_IsMainThread());
    RUN_ON_THREAD(mSTS, WrapRunnable(nsRefPtr<DataChannelConnection>(this),
                                     &DataChannelConnection::disconnect_all),
                  NS_DISPATCH_NORMAL);
    
    
    
  }
}

NS_IMPL_THREADSAFE_ISUPPORTS1(DataChannelConnection,
                              nsITimerCallback)

bool
DataChannelConnection::Init(unsigned short aPort, uint16_t aNumStreams, bool aUsingDtls)
{
  struct sctp_initmsg initmsg;
  struct sctp_udpencaps encaps;
  struct sctp_assoc_value av;
  struct sctp_event event;
  socklen_t len;

  uint16_t event_types[] = {SCTP_ASSOC_CHANGE,
                            SCTP_PEER_ADDR_CHANGE,
                            SCTP_REMOTE_ERROR,
                            SCTP_SHUTDOWN_EVENT,
                            SCTP_ADAPTATION_INDICATION,
                            SCTP_SEND_FAILED_EVENT,
                            SCTP_STREAM_RESET_EVENT,
                            SCTP_STREAM_CHANGE_EVENT};
  {
    ASSERT_WEBRTC(NS_IsMainThread());

    
    if (!sctp_initialized) {
      if (aUsingDtls) {
        LOG(("sctp_init(DTLS)"));
#ifdef MOZ_PEERCONNECTION
        usrsctp_init(0,
                     DataChannelConnection::SctpDtlsOutput,
#ifdef PR_LOGGING
                     debug_printf
#else
                     nullptr
#endif
                    );
#else
        NS_ASSERTION(!aUsingDtls, "Trying to use SCTP/DTLS without mtransport");
#endif
      } else {
        LOG(("sctp_init(%d)", aPort));
        usrsctp_init(aPort,
                     nullptr,
#ifdef PR_LOGGING
                     debug_printf
#else
                     nullptr
#endif
                    );
      }

#ifdef PR_LOGGING
      
      if (PR_LOG_TEST(GetSCTPLog(), PR_LOG_ALWAYS)) {
        usrsctp_sysctl_set_sctp_debug_on(SCTP_DEBUG_ALL);
      }
#endif
      usrsctp_sysctl_set_sctp_blackhole(2);
      sctp_initialized = true;

      gDataChannelShutdown = new DataChannelShutdown();
      gDataChannelShutdown->Init();
    }
  }

  
  
  nsresult rv;
  mSTS = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
  MOZ_ASSERT(NS_SUCCEEDED(rv));

  
  if ((mMasterSocket = usrsctp_socket(
         aUsingDtls ? AF_CONN : AF_INET,
         SOCK_STREAM, IPPROTO_SCTP, receive_cb, nullptr, 0, this)) == nullptr) {
    return false;
  }

  
  
  if (usrsctp_set_non_blocking(mMasterSocket, 1) < 0) {
    LOG(("Couldn't set non_blocking on SCTP socket"));
    
    
    goto error_cleanup;
  }

  
  
  struct linger l;
  l.l_onoff = 1;
  l.l_linger = 0;
  if (usrsctp_setsockopt(mMasterSocket, SOL_SOCKET, SO_LINGER,
                         (const void *)&l, (socklen_t)sizeof(struct linger)) < 0) {
    LOG(("Couldn't set SO_LINGER on SCTP socket"));
    
    goto error_cleanup;
  }

  
  
  
  {
    uint32_t on = 1;
    if (usrsctp_setsockopt(mMasterSocket, IPPROTO_SCTP, SCTP_REUSE_PORT,
                           (const void *)&on, (socklen_t)sizeof(on)) < 0) {
      LOG(("Couldn't set SCTP_REUSE_PORT on SCTP socket"));
    }
  }

  if (!aUsingDtls) {
    memset(&encaps, 0, sizeof(encaps));
    encaps.sue_address.ss_family = AF_INET;
    encaps.sue_port = htons(aPort);
    if (usrsctp_setsockopt(mMasterSocket, IPPROTO_SCTP, SCTP_REMOTE_UDP_ENCAPS_PORT,
                           (const void*)&encaps,
                           (socklen_t)sizeof(struct sctp_udpencaps)) < 0) {
      LOG(("*** failed encaps errno %d", errno));
      goto error_cleanup;
    }
    LOG(("SCTP encapsulation local port %d", aPort));
  }

  av.assoc_id = SCTP_ALL_ASSOC;
  av.assoc_value = SCTP_ENABLE_RESET_STREAM_REQ | SCTP_ENABLE_CHANGE_ASSOC_REQ;
  if (usrsctp_setsockopt(mMasterSocket, IPPROTO_SCTP, SCTP_ENABLE_STREAM_RESET, &av,
                         (socklen_t)sizeof(struct sctp_assoc_value)) < 0) {
    LOG(("*** failed enable stream reset errno %d", errno));
    goto error_cleanup;
  }

  
  memset(&event, 0, sizeof(event));
  event.se_assoc_id = SCTP_ALL_ASSOC;
  event.se_on = 1;
  for (uint32_t i = 0; i < sizeof(event_types)/sizeof(event_types[0]); ++i) {
    event.se_type = event_types[i];
    if (usrsctp_setsockopt(mMasterSocket, IPPROTO_SCTP, SCTP_EVENT, &event, sizeof(event)) < 0) {
      LOG(("*** failed setsockopt SCTP_EVENT errno %d", errno));
      goto error_cleanup;
    }
  }

  
  mStreamsOut.AppendElements(aNumStreams);
  mStreamsIn.AppendElements(aNumStreams); 
  for (uint32_t i = 0; i < aNumStreams; ++i) {
    mStreamsOut[i] = nullptr;
    mStreamsIn[i]  = nullptr;
  }
  memset(&initmsg, 0, sizeof(initmsg));
  len = sizeof(initmsg);
  if (usrsctp_getsockopt(mMasterSocket, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, &len) < 0) {
    LOG(("*** failed getsockopt SCTP_INITMSG"));
    goto error_cleanup;
  }
  LOG(("Setting number of SCTP streams to %u, was %u/%u", aNumStreams,
       initmsg.sinit_num_ostreams, initmsg.sinit_max_instreams));
  initmsg.sinit_num_ostreams  = aNumStreams;
  initmsg.sinit_max_instreams = MAX_NUM_STREAMS;
  if (usrsctp_setsockopt(mMasterSocket, IPPROTO_SCTP, SCTP_INITMSG, &initmsg,
                         (socklen_t)sizeof(initmsg)) < 0) {
    LOG(("*** failed setsockopt SCTP_INITMSG, errno %d", errno));
    goto error_cleanup;
  }

  mSocket = nullptr;
  return true;

error_cleanup:
  usrsctp_close(mMasterSocket);
  mMasterSocket = nullptr;
  return false;
}

void
DataChannelConnection::StartDefer()
{
  nsresult rv;
  if (!NS_IsMainThread()) {
    NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                            DataChannelOnMessageAvailable::START_DEFER,
                            this, (DataChannel *) nullptr));
    return;
  }

  ASSERT_WEBRTC(NS_IsMainThread());
  if (!mDeferredTimer) {
    mDeferredTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    MOZ_ASSERT(mDeferredTimer);
  }

  if (!mTimerRunning) {
    rv = mDeferredTimer->InitWithCallback(this, mDeferTimeout,
                                          nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_TRUE_VOID(rv == NS_OK);

    mTimerRunning = true;
  }
}



NS_IMETHODIMP
DataChannelConnection::Notify(nsITimer *timer)
{
  ASSERT_WEBRTC(NS_IsMainThread());
  LOG(("%s: %p [%p] (%dms), sending deferred messages", __FUNCTION__, this, timer, mDeferTimeout));

  if (timer == mDeferredTimer) {
    if (SendDeferredMessages()) {
      
      
      nsresult rv = mDeferredTimer->InitWithCallback(this, mDeferTimeout,
                                                     nsITimer::TYPE_ONE_SHOT);
      if (NS_FAILED(rv)) {
        LOG(("%s: cannot initialize open timer", __FUNCTION__));
        
        return rv;
      }
      mTimerRunning = true;
    } else {
      LOG(("Turned off deferred send timer"));
      mTimerRunning = false;
    }
  }
  return NS_OK;
}

#ifdef MOZ_PEERCONNECTION
bool
DataChannelConnection::ConnectDTLS(TransportFlow *aFlow, uint16_t localport, uint16_t remoteport)
{
  LOG(("Connect DTLS local %d, remote %d", localport, remoteport));

  NS_PRECONDITION(mMasterSocket, "SCTP wasn't initialized before ConnectDTLS!");
  NS_ENSURE_TRUE(aFlow, false);

  mTransportFlow = aFlow;
  mTransportFlow->SignalPacketReceived.connect(this, &DataChannelConnection::SctpDtlsInput);
  mLocalPort = localport;
  mRemotePort = remoteport;
  mState = CONNECTING;

  struct sockaddr_conn addr;
  memset(&addr, 0, sizeof(addr));
  addr.sconn_family = AF_CONN;
#if defined(__Userspace_os_Darwin)
  addr.sconn_len = sizeof(addr);
#endif
  addr.sconn_port = htons(mLocalPort);

  LOG(("Calling usrsctp_bind"));
  int r = usrsctp_bind(mMasterSocket, reinterpret_cast<struct sockaddr *>(&addr),
                       sizeof(addr));
  if (r < 0) {
    LOG(("usrsctp_bind failed: %d", r));
  } else {
    
    addr.sconn_port = htons(mRemotePort);
    addr.sconn_addr = static_cast<void *>(this);
    LOG(("Calling usrsctp_connect"));
    r = usrsctp_connect(mMasterSocket, reinterpret_cast<struct sockaddr *>(&addr),
                        sizeof(addr));
    if (r < 0) {
      if (errno == EINPROGRESS) {
        
        return true;
      } else {
        LOG(("usrsctp_connect failed: %d", errno));
        mState = CLOSED;
      }
    } else {
      
      LOG(("%s: sending ON_CONNECTION for %p", __FUNCTION__, this));
      mSocket = mMasterSocket;
      mState = OPEN;
      LOG(("DTLS connect() succeeded!  Entering connected mode"));

      NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                                DataChannelOnMessageAvailable::ON_CONNECTION,
                                this, true));
      return true;
    }
  }
  
  NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                            DataChannelOnMessageAvailable::ON_CONNECTION,
                            this, false));
  return false;
}

void
DataChannelConnection::SctpDtlsInput(TransportFlow *flow,
                                     const unsigned char *data, size_t len)
{
#ifdef PR_LOGGING
  if (PR_LOG_TEST(GetSCTPLog(), PR_LOG_DEBUG)) {
    char *buf;

    if ((buf = usrsctp_dumppacket((void *)data, len, SCTP_DUMP_INBOUND)) != NULL) {
      PR_LogPrint("%s", buf);
      usrsctp_freedumpbuffer(buf);
    }
  }
#endif
  
  usrsctp_conninput(static_cast<void *>(this), data, len, 0);
}

int
DataChannelConnection::SendPacket(const unsigned char *data, size_t len, bool release)
{
  
  int res = mTransportFlow->SendPacket(data, len) < 0 ? 1 : 0;
  if (release)
    delete data;
  return res;
}


int
DataChannelConnection::SctpDtlsOutput(void *addr, void *buffer, size_t length,
                                      uint8_t tos, uint8_t set_df)
{
  DataChannelConnection *peer = static_cast<DataChannelConnection *>(addr);
  int res;

#ifdef PR_LOGGING
  if (PR_LOG_TEST(GetSCTPLog(), PR_LOG_DEBUG)) {
    char *buf;

    if ((buf = usrsctp_dumppacket(buffer, length, SCTP_DUMP_OUTBOUND)) != NULL) {
      PR_LogPrint("%s", buf);
      usrsctp_freedumpbuffer(buf);
    }
  }
#endif
  
  
  
  
  
  if (0 ) {
    res = peer->SendPacket(static_cast<unsigned char *>(buffer), length, false);
  } else {
    unsigned char *data = new unsigned char[length];
    memcpy(data, buffer, length);
    res = -1;
    
    
    
    
    peer->mSTS->Dispatch(WrapRunnable(
                           nsRefPtr<DataChannelConnection>(peer),
                           &DataChannelConnection::SendPacket, data, length, true),
                         NS_DISPATCH_NORMAL);
    res = 0; 
  }
  return res;
}
#endif



bool
DataChannelConnection::Listen(unsigned short port)
{
  struct sockaddr_in addr;
  socklen_t addr_len;

  NS_WARN_IF_FALSE(!NS_IsMainThread(), "Blocks, do not call from main thread!!!");

  
  memset((void *)&addr, 0, sizeof(addr));
#ifdef HAVE_SIN_LEN
  addr.sin_len = sizeof(struct sockaddr_in);
#endif
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  LOG(("Waiting for connections on port %d", ntohs(addr.sin_port)));
  mState = CONNECTING;
  if (usrsctp_bind(mMasterSocket, reinterpret_cast<struct sockaddr *>(&addr), sizeof(struct sockaddr_in)) < 0) {
    LOG(("***Failed userspace_bind"));
    return false;
  }
  if (usrsctp_listen(mMasterSocket, 1) < 0) {
    LOG(("***Failed userspace_listen"));
    return false;
  }

  LOG(("Accepting connection"));
  addr_len = 0;
  if ((mSocket = usrsctp_accept(mMasterSocket, nullptr, &addr_len)) == nullptr) {
    LOG(("***Failed accept"));
    return false;
  }
  mState = OPEN;

  struct linger l;
  l.l_onoff = 1;
  l.l_linger = 0;
  if (usrsctp_setsockopt(mSocket, SOL_SOCKET, SO_LINGER,
                         (const void *)&l, (socklen_t)sizeof(struct linger)) < 0) {
    LOG(("Couldn't set SO_LINGER on SCTP socket"));
  }

  
  
  LOG(("%s: sending ON_CONNECTION for %p", __FUNCTION__, this));
  NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                            DataChannelOnMessageAvailable::ON_CONNECTION,
                            this, (DataChannel *) nullptr));
  return true;
}


bool
DataChannelConnection::Connect(const char *addr, unsigned short port)
{
  struct sockaddr_in addr4;
  struct sockaddr_in6 addr6;

  NS_WARN_IF_FALSE(!NS_IsMainThread(), "Blocks, do not call from main thread!!!");

  
  LOG(("Connecting to %s, port %u", addr, port));
  memset((void *)&addr4, 0, sizeof(struct sockaddr_in));
  memset((void *)&addr6, 0, sizeof(struct sockaddr_in6));
#ifdef HAVE_SIN_LEN
  addr4.sin_len = sizeof(struct sockaddr_in);
#endif
#ifdef HAVE_SIN6_LEN
  addr6.sin6_len = sizeof(struct sockaddr_in6);
#endif
  addr4.sin_family = AF_INET;
  addr6.sin6_family = AF_INET6;
  addr4.sin_port = htons(port);
  addr6.sin6_port = htons(port);
  mState = CONNECTING;

#if !defined(__Userspace_os_Windows)
  if (inet_pton(AF_INET6, addr, &addr6.sin6_addr) == 1) {
    if (usrsctp_connect(mMasterSocket, reinterpret_cast<struct sockaddr *>(&addr6), sizeof(struct sockaddr_in6)) < 0) {
      LOG(("*** Failed userspace_connect"));
      return false;
    }
  } else if (inet_pton(AF_INET, addr, &addr4.sin_addr) == 1) {
    if (usrsctp_connect(mMasterSocket, reinterpret_cast<struct sockaddr *>(&addr4), sizeof(struct sockaddr_in)) < 0) {
      LOG(("*** Failed userspace_connect"));
      return false;
    }
  } else {
    LOG(("*** Illegal destination address."));
  }
#else
  {
    struct sockaddr_storage ss;
    int sslen = sizeof(ss);

    if (!WSAStringToAddressA(const_cast<char *>(addr), AF_INET6, nullptr, (struct sockaddr*)&ss, &sslen)) {
      addr6.sin6_addr = (reinterpret_cast<struct sockaddr_in6 *>(&ss))->sin6_addr;
      if (usrsctp_connect(mMasterSocket, reinterpret_cast<struct sockaddr *>(&addr6), sizeof(struct sockaddr_in6)) < 0) {
        LOG(("*** Failed userspace_connect"));
        return false;
      }
    } else if (!WSAStringToAddressA(const_cast<char *>(addr), AF_INET, nullptr, (struct sockaddr*)&ss, &sslen)) {
      addr4.sin_addr = (reinterpret_cast<struct sockaddr_in *>(&ss))->sin_addr;
      if (usrsctp_connect(mMasterSocket, reinterpret_cast<struct sockaddr *>(&addr4), sizeof(struct sockaddr_in)) < 0) {
        LOG(("*** Failed userspace_connect"));
        return false;
      }
    } else {
      LOG(("*** Illegal destination address."));
    }
  }
#endif

  mSocket = mMasterSocket;

  LOG(("connect() succeeded!  Entering connected mode"));
  mState = OPEN;

  
  
  LOG(("%s: sending ON_CONNECTION for %p", __FUNCTION__, this));
  NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                            DataChannelOnMessageAvailable::ON_CONNECTION,
                            this, (DataChannel *) nullptr));
  return true;
}

DataChannel *
DataChannelConnection::FindChannelByStreamIn(uint16_t streamIn)
{
  
  if (((uint32_t) streamIn) + 1 > mStreamsIn.Length()) {
    uint32_t old_len = mStreamsIn.Length();
    LOG(("Extending mStreamsIn[] to %d elements", ((int32_t) streamIn)+1));
    mStreamsIn.AppendElements((streamIn+1) - mStreamsIn.Length());
    for (uint32_t i = old_len; i < mStreamsIn.Length(); ++i)
      mStreamsIn[i] = nullptr;
  }
  
  return mStreamsIn.SafeElementAt(streamIn);
}

DataChannel *
DataChannelConnection::FindChannelByStreamOut(uint16_t streamOut)
{
  return mStreamsOut.SafeElementAt(streamOut);
}

uint16_t
DataChannelConnection::FindFreeStreamOut()
{
  uint32_t i, limit;

  limit = mStreamsOut.Length();
  if (limit > MAX_NUM_STREAMS)
    limit = MAX_NUM_STREAMS;
  for (i = 0; i < limit; ++i) {
    if (!mStreamsOut[i]) {
      
      for (uint32_t j = 0; j < mStreamsResetting.Length(); ++j) {
        if (mStreamsResetting[j] == i) {
          continue;
        }
      }
      break;
    }
  }
  if (i == limit) {
    return INVALID_STREAM;
  }
  return i;
}

bool
DataChannelConnection::RequestMoreStreamsOut(int32_t aNeeded)
{
  struct sctp_status status;
  struct sctp_add_streams sas;
  uint32_t outStreamsNeeded;
  socklen_t len;

  if (aNeeded + mStreamsOut.Length() > MAX_NUM_STREAMS)
    aNeeded = MAX_NUM_STREAMS - mStreamsOut.Length();
  if (aNeeded <= 0)
    return false;

  len = (socklen_t)sizeof(struct sctp_status);
  if (usrsctp_getsockopt(mMasterSocket, IPPROTO_SCTP, SCTP_STATUS, &status, &len) < 0) {
    LOG(("***failed: getsockopt SCTP_STATUS"));
    return false;
  }
  outStreamsNeeded = aNeeded; 

  memset(&sas, 0, sizeof(struct sctp_add_streams));
  sas.sas_instrms = 0;
  sas.sas_outstrms = (uint16_t)outStreamsNeeded; 
  
  if (usrsctp_setsockopt(mMasterSocket, IPPROTO_SCTP, SCTP_ADD_STREAMS, &sas,
                         (socklen_t) sizeof(struct sctp_add_streams)) < 0) {
    if (errno == EALREADY)
      return true;

    LOG(("***failed: setsockopt ADD errno=%d", errno));
    return false;
  }
  LOG(("Requested %u more streams", outStreamsNeeded));
  return true;
}

int32_t
DataChannelConnection::SendControlMessage(void *msg, uint32_t len, uint16_t streamOut)
{
  struct sctp_sndinfo sndinfo;

  
  memset(&sndinfo, 0, sizeof(struct sctp_sndinfo));
  sndinfo.snd_sid = streamOut;
  sndinfo.snd_ppid = htonl(DATA_CHANNEL_PPID_CONTROL);
  if (usrsctp_sendv(mSocket, msg, len, nullptr, 0,
                    &sndinfo, (socklen_t)sizeof(struct sctp_sndinfo),
                    SCTP_SENDV_SNDINFO, 0) < 0) {
    
    return (0);
  }
  return (1);
}

int32_t
DataChannelConnection::SendOpenResponseMessage(uint16_t streamOut, uint16_t streamIn)
{
  struct rtcweb_datachannel_open_response rsp;

  memset(&rsp, 0, sizeof(struct rtcweb_datachannel_open_response));
  rsp.msg_type = DATA_CHANNEL_OPEN_RESPONSE;
  rsp.reverse_stream = htons(streamIn);

  return SendControlMessage(&rsp, sizeof(rsp), streamOut);
}


int32_t
DataChannelConnection::SendOpenAckMessage(uint16_t streamOut)
{
  struct rtcweb_datachannel_ack ack;

  memset(&ack, 0, sizeof(struct rtcweb_datachannel_ack));
  ack.msg_type = DATA_CHANNEL_ACK;

  return SendControlMessage(&ack, sizeof(ack), streamOut);
}

int32_t
DataChannelConnection::SendOpenRequestMessage(const nsACString& label,
                                              uint16_t streamOut, bool unordered,
                                              uint16_t prPolicy, uint32_t prValue)
{
  int len = label.Length(); 
  struct rtcweb_datachannel_open_request *req =
    (struct rtcweb_datachannel_open_request*) moz_xmalloc(sizeof(*req)+len);
   

  memset(req, 0, sizeof(struct rtcweb_datachannel_open_request));
  req->msg_type = DATA_CHANNEL_OPEN_REQUEST;
  switch (prPolicy) {
  case SCTP_PR_SCTP_NONE:
    req->channel_type = DATA_CHANNEL_RELIABLE;
    break;
  case SCTP_PR_SCTP_TTL:
    req->channel_type = DATA_CHANNEL_PARTIAL_RELIABLE_TIMED;
    break;
  case SCTP_PR_SCTP_RTX:
    req->channel_type = DATA_CHANNEL_PARTIAL_RELIABLE_REXMIT;
    break;
  default:
    
    moz_free(req);
    return (0);
  }
  req->flags = htons(0);
  if (unordered) {
    req->flags |= htons(DATA_CHANNEL_FLAG_OUT_OF_ORDER_ALLOWED);
  }
  req->reliability_params = htons((uint16_t)prValue); 
  req->priority = htons(0); 
  strcpy(&req->label[0], PromiseFlatCString(label).get());

  int32_t result = SendControlMessage(req, sizeof(*req)+len, streamOut);

  moz_free(req);
  return result;
}











bool
DataChannelConnection::SendDeferredMessages()
{
  uint32_t i;
  nsRefPtr<DataChannel> channel; 
  bool still_blocked = false;
  bool sent = false;

  
  MutexAutoLock lock(mLock);

  
  
  for (i = 0; i < mStreamsOut.Length(); ++i) {
    channel = mStreamsOut[i];
    if (!channel)
      continue;

    
    if (channel->mFlags & DATA_CHANNEL_FLAGS_SEND_REQ) {
      if (SendOpenRequestMessage(channel->mLabel, channel->mStreamOut,
                                 channel->mFlags & DATA_CHANNEL_FLAG_OUT_OF_ORDER_ALLOWED,
                                 channel->mPrPolicy, channel->mPrValue)) {
        channel->mFlags &= ~DATA_CHANNEL_FLAGS_SEND_REQ;
        sent = true;
      } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          still_blocked = true;
        } else {
          
          mStreamsOut[channel->mStreamOut] = nullptr;
          channel->mState = CLOSED;
          
          NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                                    DataChannelOnMessageAvailable::ON_CHANNEL_CLOSED, this,
                                    channel));
        }
      }
    }
    if (still_blocked)
      break;

    if (channel->mFlags & DATA_CHANNEL_FLAGS_SEND_RSP) {
      if (SendOpenResponseMessage(channel->mStreamOut, channel->mStreamIn)) {
        channel->mFlags &= ~DATA_CHANNEL_FLAGS_SEND_RSP;
        sent = true;
      } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          still_blocked = true;
        } else {
          
          
          
          
          
          
          mStreamsIn[channel->mStreamIn]   = nullptr;
          mStreamsOut[channel->mStreamOut] = nullptr;
          channel->mState = CLOSED;
        }
      }
    }
    if (still_blocked)
      break;

    if (channel->mFlags & DATA_CHANNEL_FLAGS_SEND_ACK) {
      if (SendOpenAckMessage(channel->mStreamOut)) {
        channel->mFlags &= ~DATA_CHANNEL_FLAGS_SEND_ACK;
        sent = true;
      } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          still_blocked = true;
        } else {
          
          CloseInt(channel);
          
        }
      }
    }
    if (still_blocked)
      break;

    if (channel->mFlags & DATA_CHANNEL_FLAGS_SEND_DATA) {
      bool failed_send = false;
      int32_t result;

      if (channel->mState == CLOSED || channel->mState == CLOSING) {
        channel->mBufferedData.Clear();
      }
      while (!channel->mBufferedData.IsEmpty() &&
             !failed_send) {
        struct sctp_sendv_spa *spa = channel->mBufferedData[0]->mSpa;
        const char *data           = channel->mBufferedData[0]->mData;
        uint32_t len               = channel->mBufferedData[0]->mLength;

        
        
        if ((result = usrsctp_sendv(mSocket, data, len,
                                    nullptr, 0,
                                    (void *)spa, (socklen_t)sizeof(struct sctp_sendv_spa),
                                    SCTP_SENDV_SPA,
                                    0) < 0)) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            
            failed_send = true;
            LOG(("queue full again when resending %d bytes (%d)", len, result));
          } else {
            LOG(("error %d re-sending string", errno));
            failed_send = true;
          }
        } else {
          LOG(("Resent buffer of %d bytes (%d)", len, result));
          sent = true;
          channel->mBufferedData.RemoveElementAt(0);
        }
      }
      if (channel->mBufferedData.IsEmpty())
        channel->mFlags &= ~DATA_CHANNEL_FLAGS_SEND_DATA;
      else
        still_blocked = true;
    }
    if (still_blocked)
      break;
  }

  if (!still_blocked) {
    
    return false;
  }
  
  
  if (!sent && mDeferTimeout < 50)
    mDeferTimeout++;
  else if (sent && mDeferTimeout > 10)
    mDeferTimeout--;

  return true;
}

void
DataChannelConnection::HandleOpenRequestMessage(const struct rtcweb_datachannel_open_request *req,
                                                size_t length,
                                                uint16_t streamIn)
{
  nsRefPtr<DataChannel> channel;
  uint32_t prValue;
  uint16_t prPolicy;
  uint32_t flags;
  nsCString label(nsDependentCString(req->label));

  mLock.AssertCurrentThreadOwns();

  if ((channel = FindChannelByStreamIn(streamIn))) {
    LOG(("ERROR: HandleOpenRequestMessage: channel for stream %d is in state %d instead of CLOSED.",
         streamIn, channel->mState));
    
    return;
  }
  switch (req->channel_type) {
    case DATA_CHANNEL_RELIABLE:
      prPolicy = SCTP_PR_SCTP_NONE;
      break;
    case DATA_CHANNEL_PARTIAL_RELIABLE_REXMIT:
      prPolicy = SCTP_PR_SCTP_RTX;
      break;
    case DATA_CHANNEL_PARTIAL_RELIABLE_TIMED:
      prPolicy = SCTP_PR_SCTP_TTL;
      break;
    default:
      
      return;
  }
  prValue = ntohs(req->reliability_params);
  flags = ntohs(req->flags) & DATA_CHANNEL_FLAG_OUT_OF_ORDER_ALLOWED;
  channel = new DataChannel(this,
                            INVALID_STREAM, streamIn,
                            DataChannel::CONNECTING,
                            label,
                            prPolicy, prValue,
                            flags,
                            nullptr, nullptr);
  mStreamsIn[streamIn] = channel;

  OpenResponseFinish(channel.forget());
}

void
DataChannelConnection::OpenResponseFinish(already_AddRefed<DataChannel> aChannel)
{
  nsRefPtr<DataChannel> channel(aChannel);
  uint16_t streamOut = FindFreeStreamOut(); 

  mLock.AssertCurrentThreadOwns();

  LOG(("Finished response: channel %p, streamOut = %u", channel.get(), streamOut));

  if (streamOut == INVALID_STREAM) {
    if (!RequestMoreStreamsOut()) {
      channel->mState = CLOSED;
      if (channel->mFlags & DATA_CHANNEL_FLAGS_FINISH_RSP) {
        
        NS_ERROR("Failed to request more streams");
        NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                                  DataChannelOnMessageAvailable::ON_CHANNEL_CLOSED, this,
                                  channel));
      }
      
      
      
      
      mStreamsIn[channel->mStreamIn] = nullptr;
      
      return;
    }
    LOG(("Queuing channel %d to finish response", channel->mStreamIn));
    channel->mFlags |= DATA_CHANNEL_FLAGS_FINISH_RSP;
    DataChannel *temp = channel.get(); 
    channel.forget();
    mPending.Push(temp);
    
  } else {
    channel->mStreamOut = streamOut;
    mStreamsOut[streamOut] = channel;
    if (SendOpenResponseMessage(streamOut, channel->mStreamIn)) {
      
      
      LOG(("%s: sending ON_CHANNEL_CREATED for %s: %d/%d", __FUNCTION__,
           channel->mLabel.get(), streamOut, channel->mStreamIn));
      NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                                DataChannelOnMessageAvailable::ON_CHANNEL_CREATED,
                                this, channel));
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        channel->mFlags |= DATA_CHANNEL_FLAGS_SEND_RSP;
        StartDefer();
      } else {
        if (channel->mFlags & DATA_CHANNEL_FLAGS_FINISH_RSP) {
          
          NS_ERROR("Failed to send open response");
          NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                                    DataChannelOnMessageAvailable::ON_CHANNEL_CLOSED, this,
                                    channel));
        }

        
        mStreamsIn[channel->mStreamIn] = nullptr;
        mStreamsOut[streamOut] = nullptr;
        channel->mStreamOut = INVALID_STREAM;
        
        channel->mState = CLOSED;
        return;
      }
    }
  }
}


void
DataChannelConnection::HandleOpenResponseMessage(const struct rtcweb_datachannel_open_response *rsp,
                                                 size_t length, uint16_t streamIn)
{
  uint16_t streamOut;
  DataChannel *channel;

  mLock.AssertCurrentThreadOwns();

  streamOut = ntohs(rsp->reverse_stream);
  channel = FindChannelByStreamOut(streamOut);

  NS_ENSURE_TRUE_VOID(channel);
  NS_ENSURE_TRUE_VOID(channel->mState == CONNECTING);

  if (rsp->error) {
    LOG(("%s: error in response to open of channel %d (%s)",
         __FUNCTION__, streamOut, channel->mLabel.get()));

  } else {
    NS_ENSURE_TRUE_VOID(!FindChannelByStreamIn(streamIn));

    channel->mStreamIn = streamIn;
    channel->mState = OPEN;
    channel->mReady = true;
    mStreamsIn[streamIn] = channel;
    if (SendOpenAckMessage(streamOut)) {
      channel->mFlags = 0;
    } else {
      
      channel->mFlags |= DATA_CHANNEL_FLAGS_SEND_ACK;
      StartDefer();
    }
    LOG(("%s: sending ON_CHANNEL_OPEN for %p", __FUNCTION__, channel));
    NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                              DataChannelOnMessageAvailable::ON_CHANNEL_OPEN, this,
                              channel));
  }
}

void
DataChannelConnection::HandleOpenAckMessage(const struct rtcweb_datachannel_ack *ack,
                                            size_t length, uint16_t streamIn)
{
  DataChannel *channel;

  mLock.AssertCurrentThreadOwns();

  channel = FindChannelByStreamIn(streamIn);

  NS_ENSURE_TRUE_VOID(channel);
  NS_ENSURE_TRUE_VOID(channel->mState == CONNECTING);

  channel->mState = channel->mReady ? DataChannel::OPEN : DataChannel::WAITING_TO_OPEN;
  if (channel->mState == OPEN) {
    LOG(("%s: sending ON_CHANNEL_OPEN for %p", __FUNCTION__, channel));
    NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                              DataChannelOnMessageAvailable::ON_CHANNEL_OPEN, this,
                              channel));
  } else {
    LOG(("%s: deferring sending ON_CHANNEL_OPEN for %p", __FUNCTION__, channel));
  }
}

void
DataChannelConnection::HandleUnknownMessage(uint32_t ppid, size_t length, uint16_t streamIn)
{
  
  LOG(("unknown DataChannel message received: %u, len %ld on stream %lu", ppid, length, streamIn));
  
}

void
DataChannelConnection::HandleDataMessage(uint32_t ppid,
                                         const void *data, size_t length,
                                         uint16_t streamIn)
{
  DataChannel *channel;
  const char *buffer = (const char *) data;

  mLock.AssertCurrentThreadOwns();

  channel = FindChannelByStreamIn(streamIn);

  
  NS_ENSURE_TRUE_VOID(channel);
  NS_ENSURE_TRUE_VOID(channel->mState != CONNECTING);

  
  NS_ENSURE_TRUE_VOID(channel->mState != CLOSED);

  {
    nsAutoCString recvData(buffer, length);

    switch (ppid) {
      case DATA_CHANNEL_PPID_DOMSTRING:
        LOG(("DataChannel: String message received of length %lu on channel %d: %.*s",
             length, channel->mStreamOut, (int)PR_MIN(length, 80), buffer));
        length = -1; 

        

        NS_WARN_IF_FALSE(channel->mBinaryBuffer.IsEmpty(), "Binary message aborted by text message!");
        if (!channel->mBinaryBuffer.IsEmpty())
          channel->mBinaryBuffer.Truncate(0);
        break;

      case DATA_CHANNEL_PPID_BINARY:
        channel->mBinaryBuffer += recvData;
        LOG(("DataChannel: Received binary message of length %lu (total %u) on channel id %d",
             length, channel->mBinaryBuffer.Length(), channel->mStreamOut));
        return; 

      case DATA_CHANNEL_PPID_BINARY_LAST:
        LOG(("DataChannel: Received binary message of length %lu on channel id %d",
             length, channel->mStreamOut));
        if (!channel->mBinaryBuffer.IsEmpty()) {
          channel->mBinaryBuffer += recvData;
          LOG(("%s: sending ON_DATA (binary fragmented) for %p", __FUNCTION__, channel));
          channel->SendOrQueue(new DataChannelOnMessageAvailable(
                                 DataChannelOnMessageAvailable::ON_DATA, this,
                                 channel, channel->mBinaryBuffer,
                                 channel->mBinaryBuffer.Length()));
          channel->mBinaryBuffer.Truncate(0);
          return;
        }
        
        break;

      default:
        NS_ERROR("Unknown data PPID");
        return;
    }
    
    LOG(("%s: sending ON_DATA for %p", __FUNCTION__, channel));
    channel->SendOrQueue(new DataChannelOnMessageAvailable(
                           DataChannelOnMessageAvailable::ON_DATA, this,
                           channel, recvData, length));
  }
}


void
DataChannelConnection::HandleMessage(const void *buffer, size_t length, uint32_t ppid, uint16_t streamIn)
{
  const struct rtcweb_datachannel_open_request *req;
  const struct rtcweb_datachannel_open_response *rsp;
  const struct rtcweb_datachannel_ack *ack, *msg;

  mLock.AssertCurrentThreadOwns();

  switch (ppid) {
    case DATA_CHANNEL_PPID_CONTROL:
      NS_ENSURE_TRUE_VOID(length >= sizeof(*ack)); 

      msg = static_cast<const struct rtcweb_datachannel_ack *>(buffer);
      switch (msg->msg_type) {
        case DATA_CHANNEL_OPEN_REQUEST:
          LOG(("length %u, sizeof(*req) = %u", length, sizeof(*req)));
          NS_ENSURE_TRUE_VOID(length >= sizeof(*req));

          req = static_cast<const struct rtcweb_datachannel_open_request *>(buffer);
          HandleOpenRequestMessage(req, length, streamIn);
          break;
        case DATA_CHANNEL_OPEN_RESPONSE:
          NS_ENSURE_TRUE_VOID(length >= sizeof(*rsp));

          rsp = static_cast<const struct rtcweb_datachannel_open_response *>(buffer);
          HandleOpenResponseMessage(rsp, length, streamIn);
          break;
        case DATA_CHANNEL_ACK:
          

          ack = static_cast<const struct rtcweb_datachannel_ack *>(buffer);
          HandleOpenAckMessage(ack, length, streamIn);
          break;
        default:
          HandleUnknownMessage(ppid, length, streamIn);
          break;
      }
      break;
    case DATA_CHANNEL_PPID_DOMSTRING:
    case DATA_CHANNEL_PPID_BINARY:
    case DATA_CHANNEL_PPID_BINARY_LAST:
      HandleDataMessage(ppid, buffer, length, streamIn);
      break;
    default:
      LOG(("Message of length %lu, PPID %u on stream %u received.",
           length, ppid, streamIn));
      break;
  }
}

void
DataChannelConnection::HandleAssociationChangeEvent(const struct sctp_assoc_change *sac)
{
  uint32_t i, n;

  switch (sac->sac_state) {
  case SCTP_COMM_UP:
    LOG(("Association change: SCTP_COMM_UP"));
    if (mState == CONNECTING) {
      mSocket = mMasterSocket;
      mState = OPEN;

      NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                                DataChannelOnMessageAvailable::ON_CONNECTION,
                                this, true));
      LOG(("DTLS connect() succeeded!  Entering connected mode"));
    } else if (mState == OPEN) {
      LOG(("DataConnection Already OPEN"));
    } else {
      LOG(("Unexpected state: %d", mState));
    }
    break;
  case SCTP_COMM_LOST:
    LOG(("Association change: SCTP_COMM_LOST"));
    NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                              DataChannelOnMessageAvailable::ON_DISCONNECTED,
                              this));
    break;
  case SCTP_RESTART:
    LOG(("Association change: SCTP_RESTART"));
    break;
  case SCTP_SHUTDOWN_COMP:
    LOG(("Association change: SCTP_SHUTDOWN_COMP"));
    NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                              DataChannelOnMessageAvailable::ON_DISCONNECTED,
                              this));
    break;
  case SCTP_CANT_STR_ASSOC:
    LOG(("Association change: SCTP_CANT_STR_ASSOC"));
    break;
  default:
    LOG(("Association change: UNKNOWN"));
    break;
  }
  LOG(("Association change: streams (in/out) = (%u/%u)",
       sac->sac_inbound_streams, sac->sac_outbound_streams));

  NS_ENSURE_TRUE_VOID(sac);
  n = sac->sac_length - sizeof(*sac);
  if (((sac->sac_state == SCTP_COMM_UP) ||
        (sac->sac_state == SCTP_RESTART)) && (n > 0)) {
    for (i = 0; i < n; ++i) {
      switch (sac->sac_info[i]) {
      case SCTP_ASSOC_SUPPORTS_PR:
        LOG(("Supports: PR"));
        break;
      case SCTP_ASSOC_SUPPORTS_AUTH:
        LOG(("Supports: AUTH"));
        break;
      case SCTP_ASSOC_SUPPORTS_ASCONF:
        LOG(("Supports: ASCONF"));
        break;
      case SCTP_ASSOC_SUPPORTS_MULTIBUF:
        LOG(("Supports: MULTIBUF"));
        break;
      case SCTP_ASSOC_SUPPORTS_RE_CONFIG:
        LOG(("Supports: RE-CONFIG"));
        break;
      default:
        LOG(("Supports: UNKNOWN(0x%02x)", sac->sac_info[i]));
        break;
      }
    }
  } else if (((sac->sac_state == SCTP_COMM_LOST) ||
              (sac->sac_state == SCTP_CANT_STR_ASSOC)) && (n > 0)) {
    LOG(("Association: ABORT ="));
    for (i = 0; i < n; ++i) {
      LOG((" 0x%02x", sac->sac_info[i]));
    }
  }
  if ((sac->sac_state == SCTP_CANT_STR_ASSOC) ||
      (sac->sac_state == SCTP_SHUTDOWN_COMP) ||
      (sac->sac_state == SCTP_COMM_LOST)) {
    return;
  }
}

void
DataChannelConnection::HandlePeerAddressChangeEvent(const struct sctp_paddr_change *spc)
{
  char addr_buf[INET6_ADDRSTRLEN];
  const char *addr = "";
  struct sockaddr_in *sin;
  struct sockaddr_in6 *sin6;
#if defined(__Userspace_os_Windows)
  DWORD addr_len = INET6_ADDRSTRLEN;
#endif

  switch (spc->spc_aaddr.ss_family) {
  case AF_INET:
    sin = (struct sockaddr_in *)&spc->spc_aaddr;
#if !defined(__Userspace_os_Windows)
    addr = inet_ntop(AF_INET, &sin->sin_addr, addr_buf, INET6_ADDRSTRLEN);
#else
    if (WSAAddressToStringA((LPSOCKADDR)sin, sizeof(sin->sin_addr), nullptr,
                            addr_buf, &addr_len)) {
      return;
    }
#endif
    break;
  case AF_INET6:
    sin6 = (struct sockaddr_in6 *)&spc->spc_aaddr;
#if !defined(__Userspace_os_Windows)
    addr = inet_ntop(AF_INET6, &sin6->sin6_addr, addr_buf, INET6_ADDRSTRLEN);
#else
    if (WSAAddressToStringA((LPSOCKADDR)sin6, sizeof(sin6), nullptr,
                            addr_buf, &addr_len)) {
      return;
    }
#endif
  case AF_CONN:
    addr = "DTLS connection";
    break;
  default:
    break;
  }
  LOG(("Peer address %s is now ", addr));
  switch (spc->spc_state) {
  case SCTP_ADDR_AVAILABLE:
    LOG(("SCTP_ADDR_AVAILABLE"));
    break;
  case SCTP_ADDR_UNREACHABLE:
    LOG(("SCTP_ADDR_UNREACHABLE"));
    break;
  case SCTP_ADDR_REMOVED:
    LOG(("SCTP_ADDR_REMOVED"));
    break;
  case SCTP_ADDR_ADDED:
    LOG(("SCTP_ADDR_ADDED"));
    break;
  case SCTP_ADDR_MADE_PRIM:
    LOG(("SCTP_ADDR_MADE_PRIM"));
    break;
  case SCTP_ADDR_CONFIRMED:
    LOG(("SCTP_ADDR_CONFIRMED"));
    break;
  default:
    LOG(("UNKNOWN"));
    break;
  }
  LOG((" (error = 0x%08x).\n", spc->spc_error));
}

void
DataChannelConnection::HandleRemoteErrorEvent(const struct sctp_remote_error *sre)
{
  size_t i, n;

  n = sre->sre_length - sizeof(struct sctp_remote_error);
  LOG(("Remote Error (error = 0x%04x): ", sre->sre_error));
  for (i = 0; i < n; ++i) {
    LOG((" 0x%02x", sre-> sre_data[i]));
  }
}

void
DataChannelConnection::HandleShutdownEvent(const struct sctp_shutdown_event *sse)
{
  LOG(("Shutdown event."));
  
  
}

void
DataChannelConnection::HandleAdaptationIndication(const struct sctp_adaptation_event *sai)
{
  LOG(("Adaptation indication: %x.", sai-> sai_adaptation_ind));
}

void
DataChannelConnection::HandleSendFailedEvent(const struct sctp_send_failed_event *ssfe)
{
  size_t i, n;

  if (ssfe->ssfe_flags & SCTP_DATA_UNSENT) {
    LOG(("Unsent "));
  }
   if (ssfe->ssfe_flags & SCTP_DATA_SENT) {
    LOG(("Sent "));
  }
  if (ssfe->ssfe_flags & ~(SCTP_DATA_SENT | SCTP_DATA_UNSENT)) {
    LOG(("(flags = %x) ", ssfe->ssfe_flags));
  }
  LOG(("message with PPID = %d, SID = %d, flags: 0x%04x due to error = 0x%08x",
       ntohl(ssfe->ssfe_info.snd_ppid), ssfe->ssfe_info.snd_sid,
       ssfe->ssfe_info.snd_flags, ssfe->ssfe_error));
  n = ssfe->ssfe_length - sizeof(struct sctp_send_failed_event);
  for (i = 0; i < n; ++i) {
    LOG((" 0x%02x", ssfe->ssfe_data[i]));
  }
}

void
DataChannelConnection::ResetOutgoingStream(uint16_t streamOut)
{
  uint32_t i;

  mLock.AssertCurrentThreadOwns();
  LOG(("Connection %p: Resetting outgoing stream %d",
       (void *) this, streamOut));
  
  for (i = 0; i < mStreamsResetting.Length(); ++i) {
    if (mStreamsResetting[i] == streamOut) {
      return;
    }
  }
  mStreamsResetting.AppendElement(streamOut);
}

void
DataChannelConnection::SendOutgoingStreamReset()
{
  struct sctp_reset_streams *srs;
  uint32_t i;
  size_t len;

  LOG(("Connection %p: Sending outgoing stream reset for %d streams",
       (void *) this, mStreamsResetting.Length()));
  mLock.AssertCurrentThreadOwns();
  if (mStreamsResetting.IsEmpty()) {
    LOG(("No streams to reset"));
    return;
  }
  len = sizeof(sctp_assoc_t) + (2 + mStreamsResetting.Length()) * sizeof(uint16_t);
  srs = static_cast<struct sctp_reset_streams *> (moz_xmalloc(len)); 
  memset(srs, 0, len);
  srs->srs_flags = SCTP_STREAM_RESET_OUTGOING;
  srs->srs_number_streams = mStreamsResetting.Length();
  for (i = 0; i < mStreamsResetting.Length(); ++i) {
    srs->srs_stream_list[i] = mStreamsResetting[i];
  }
  if (usrsctp_setsockopt(mMasterSocket, IPPROTO_SCTP, SCTP_RESET_STREAMS, srs, (socklen_t)len) < 0) {
    LOG(("***failed: setsockopt RESET, errno %d", errno));
  } else {
    mStreamsResetting.Clear();
  }
  moz_free(srs);
}

void
DataChannelConnection::HandleStreamResetEvent(const struct sctp_stream_reset_event *strrst)
{
  uint32_t n, i;
  nsRefPtr<DataChannel> channel; 

  if (!(strrst->strreset_flags & SCTP_STREAM_RESET_DENIED) &&
      !(strrst->strreset_flags & SCTP_STREAM_RESET_FAILED)) {
    n = (strrst->strreset_length - sizeof(struct sctp_stream_reset_event)) / sizeof(uint16_t);
    for (i = 0; i < n; ++i) {
      if (strrst->strreset_flags & SCTP_STREAM_RESET_INCOMING_SSN) {
        channel = FindChannelByStreamIn(strrst->strreset_stream_list[i]);
        if (channel) {
          
          
          
          
          
          
          
          
          
          

          LOG(("Incoming: Channel %d outgoing/%d incoming closed, state %d",
               channel->mStreamOut, channel->mStreamIn, channel->mState));
          ASSERT_WEBRTC(channel->mState == DataChannel::OPEN ||
                        channel->mState == DataChannel::CLOSING ||
                        channel->mState == DataChannel::WAITING_TO_OPEN);
          if (channel->mState == DataChannel::OPEN ||
              channel->mState == DataChannel::WAITING_TO_OPEN) {
            ResetOutgoingStream(channel->mStreamOut);
            SendOutgoingStreamReset();
            NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                                      DataChannelOnMessageAvailable::ON_CHANNEL_CLOSED, this,
                                      channel));
            mStreamsOut[channel->mStreamOut] = nullptr;
          }
          mStreamsIn[channel->mStreamIn] = nullptr;

          LOG(("Disconnected DataChannel %p from connection %p",
               (void *) channel.get(), (void *) channel->mConnection.get()));
          channel->Destroy();
          
        } else {
          LOG(("Can't find incoming channel %d",i));
        }
      }

      if (strrst->strreset_flags & SCTP_STREAM_RESET_OUTGOING_SSN) {
        channel = FindChannelByStreamOut(strrst->strreset_stream_list[i]);
        if (channel) {
          LOG(("Outgoing: Connection %p channel %p  streams: %d outgoing/%d incoming closed",
               (void *) this, (void *) channel.get(), channel->mStreamOut, channel->mStreamIn));

          ASSERT_WEBRTC(channel->mState == CLOSING);
          if (channel->mState == CLOSING) {
            mStreamsOut[channel->mStreamOut] = nullptr;
            if (channel->mStreamIn != INVALID_STREAM)
              mStreamsIn[channel->mStreamIn] = nullptr;
            LOG(("Disconnected DataChannel %p from connection %p (refcnt will be %u)",
                 (void *) channel.get(), (void *) channel->mConnection.get(),
                 (uint32_t) channel->mConnection->mRefCnt-1));
            channel->Destroy();
            
          }
        } else {
          LOG(("Can't find outgoing channel %d",i));
        }
      }
    }
  }
}

void
DataChannelConnection::HandleStreamChangeEvent(const struct sctp_stream_change_event *strchg)
{
  uint16_t streamOut;
  uint32_t i;
  nsRefPtr<DataChannel> channel;

  if (strchg->strchange_flags == SCTP_STREAM_CHANGE_DENIED) {
    LOG(("*** Failed increasing number of streams from %u (%u/%u)",
         mStreamsOut.Length(),
         strchg->strchange_instrms,
         strchg->strchange_outstrms));
    
    return;
  } else {
    if (strchg->strchange_instrms > mStreamsIn.Length()) {
      LOG(("Other side increased streamds from %u to %u",
           mStreamsIn.Length(), strchg->strchange_instrms));
    }
    if (strchg->strchange_outstrms > mStreamsOut.Length()) {
      uint16_t old_len = mStreamsOut.Length();
      LOG(("Increasing number of streams from %u to %u - adding %u (in: %u)",
           old_len,
           strchg->strchange_outstrms,
           strchg->strchange_outstrms - old_len,
           strchg->strchange_instrms));
      
      mStreamsOut.AppendElements(strchg->strchange_outstrms - old_len);
      LOG(("New length = %d (was %d)", mStreamsOut.Length(), old_len));
      for (uint32_t i = old_len; i < mStreamsOut.Length(); ++i) {
        mStreamsOut[i] = nullptr;
      }
      
      
      

      
      
      int32_t num_needed = mPending.GetSize();
      LOG(("%d of %d new streams already needed", num_needed,
           strchg->strchange_outstrms - old_len));
      num_needed -= (strchg->strchange_outstrms - old_len); 
      if (num_needed > 0) {
        if (num_needed < 16)
          num_needed = 16;
        LOG(("Not enough new streams, asking for %d more", num_needed));
        RequestMoreStreamsOut(num_needed);
      }

      
      
      nsDeque temp;
      DataChannel *temp_channel; 
      while (nullptr != (temp_channel = static_cast<DataChannel *>(mPending.PopFront()))) {
        temp.Push(static_cast<void *>(temp_channel));
      }

      
      while (nullptr != (channel = dont_AddRef(static_cast<DataChannel *>(temp.PopFront())))) {
        if (channel->mFlags & DATA_CHANNEL_FLAGS_FINISH_RSP) {
          channel->mFlags &= ~DATA_CHANNEL_FLAGS_FINISH_RSP;
          OpenResponseFinish(channel.forget()); 
        } else if (channel->mFlags & DATA_CHANNEL_FLAGS_FINISH_OPEN) {
          channel->mFlags &= ~DATA_CHANNEL_FLAGS_FINISH_OPEN;
          OpenFinish(channel.forget()); 
        }
      }
    }
    
  }

  for (i = 0; i < mStreamsOut.Length(); ++i) {
    channel = mStreamsOut[i];
    if (!channel)
      continue;

    if ((channel->mState == CONNECTING) &&
        (channel->mStreamOut == INVALID_STREAM)) {
      if ((strchg->strchange_flags & SCTP_STREAM_CHANGE_DENIED) ||
          (strchg->strchange_flags & SCTP_STREAM_CHANGE_FAILED)) {
        
        if (channel->mStreamIn != INVALID_STREAM) {
          mStreamsIn[channel->mStreamIn] = nullptr;
        }
        channel->mState = CLOSED;
        NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                                  DataChannelOnMessageAvailable::ON_CHANNEL_CLOSED, this,
                                  channel));
        
      } else {
        streamOut = FindFreeStreamOut();
        if (streamOut != INVALID_STREAM) {
          channel->mStreamOut = streamOut;
          mStreamsOut[streamOut] = channel;
          if (channel->mStreamIn == INVALID_STREAM) {
            channel->mFlags |= DATA_CHANNEL_FLAGS_SEND_REQ;
          } else {
            channel->mFlags |= DATA_CHANNEL_FLAGS_SEND_RSP;
          }
          StartDefer();
        } else {
          
          break;
        }
      }
    }
  }
}



void
DataChannelConnection::HandleNotification(const union sctp_notification *notif, size_t n)
{
  mLock.AssertCurrentThreadOwns();
  if (notif->sn_header.sn_length != (uint32_t)n) {
    return;
  }
  switch (notif->sn_header.sn_type) {
  case SCTP_ASSOC_CHANGE:
    HandleAssociationChangeEvent(&(notif->sn_assoc_change));
    break;
  case SCTP_PEER_ADDR_CHANGE:
    HandlePeerAddressChangeEvent(&(notif->sn_paddr_change));
    break;
  case SCTP_REMOTE_ERROR:
    HandleRemoteErrorEvent(&(notif->sn_remote_error));
    break;
  case SCTP_SHUTDOWN_EVENT:
    HandleShutdownEvent(&(notif->sn_shutdown_event));
    break;
  case SCTP_ADAPTATION_INDICATION:
    HandleAdaptationIndication(&(notif->sn_adaptation_event));
    break;
  case SCTP_PARTIAL_DELIVERY_EVENT:
    LOG(("SCTP_PARTIAL_DELIVERY_EVENT"));
    break;
  case SCTP_AUTHENTICATION_EVENT:
    LOG(("SCTP_AUTHENTICATION_EVENT"));
    break;
  case SCTP_SENDER_DRY_EVENT:
    
    break;
  case SCTP_NOTIFICATIONS_STOPPED_EVENT:
    LOG(("SCTP_NOTIFICATIONS_STOPPED_EVENT"));
    break;
  case SCTP_SEND_FAILED_EVENT:
    HandleSendFailedEvent(&(notif->sn_send_failed_event));
    break;
  case SCTP_STREAM_RESET_EVENT:
    HandleStreamResetEvent(&(notif->sn_strreset_event));
    break;
  case SCTP_ASSOC_RESET_EVENT:
    LOG(("SCTP_ASSOC_RESET_EVENT"));
    break;
  case SCTP_STREAM_CHANGE_EVENT:
    HandleStreamChangeEvent(&(notif->sn_strchange_event));
    break;
  default:
    LOG(("unknown SCTP event: %u", (uint32_t)notif->sn_header.sn_type));
    break;
   }
 }

int
DataChannelConnection::ReceiveCallback(struct socket* sock, void *data, size_t datalen,
                                       struct sctp_rcvinfo rcv, int32_t flags)
{
  ASSERT_WEBRTC(!NS_IsMainThread());

  if (!data) {
    usrsctp_close(sock); 
  } else {
    MutexAutoLock lock(mLock);
    if (flags & MSG_NOTIFICATION) {
      HandleNotification(static_cast<union sctp_notification *>(data), datalen);
    } else {
      HandleMessage(data, datalen, ntohl(rcv.rcv_ppid), rcv.rcv_sid);
    }
  }
  
  return 1;
}

already_AddRefed<DataChannel>
DataChannelConnection::Open(const nsACString& label, Type type, bool inOrder,
                            uint32_t prValue, DataChannelListener *aListener,
                            nsISupports *aContext)
{
  uint16_t prPolicy = SCTP_PR_SCTP_NONE;
  uint32_t flags;

  LOG(("DC Open: label %s, type %u, inorder %d, prValue %u, listener %p, context %p",
       PromiseFlatCString(label).get(), type, inOrder, prValue, aListener, aContext));
  switch (type) {
    case DATA_CHANNEL_RELIABLE:
      prPolicy = SCTP_PR_SCTP_NONE;
      break;
    case DATA_CHANNEL_PARTIAL_RELIABLE_REXMIT:
      prPolicy = SCTP_PR_SCTP_RTX;
      break;
    case DATA_CHANNEL_PARTIAL_RELIABLE_TIMED:
      prPolicy = SCTP_PR_SCTP_TTL;
      break;
  }
  if ((prPolicy == SCTP_PR_SCTP_NONE) && (prValue != 0)) {
    return nullptr;
  }

  flags = !inOrder ? DATA_CHANNEL_FLAG_OUT_OF_ORDER_ALLOWED : 0;
  nsRefPtr<DataChannel> channel(new DataChannel(this,
                                                INVALID_STREAM, INVALID_STREAM,
                                                DataChannel::CONNECTING,
                                                label, type, prValue,
                                                flags,
                                                aListener, aContext));

  MutexAutoLock lock(mLock); 
  return OpenFinish(channel.forget());
}


already_AddRefed<DataChannel>
DataChannelConnection::OpenFinish(already_AddRefed<DataChannel> aChannel)
{
  uint16_t streamOut = FindFreeStreamOut(); 
  nsRefPtr<DataChannel> channel(aChannel);

  mLock.AssertCurrentThreadOwns();

  LOG(("Finishing open: channel %p, streamOut = %u", channel.get(), streamOut));

  if (streamOut == INVALID_STREAM) {
    if (!RequestMoreStreamsOut()) {
      channel->mState = CLOSED;
      if (channel->mFlags & DATA_CHANNEL_FLAGS_FINISH_OPEN) {
        
        NS_ERROR("Failed to request more streams");
        NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                                  DataChannelOnMessageAvailable::ON_CHANNEL_CLOSED, this,
                                  channel));
        return channel.forget();
      }
      
      
      
      return nullptr;
    }
    LOG(("Queuing channel %p to finish open", channel.get()));
    
    channel->mFlags |= DATA_CHANNEL_FLAGS_FINISH_OPEN;
    channel->AddRef(); 
    mPending.Push(channel);
    return channel.forget();
  }
  mStreamsOut[streamOut] = channel;
  channel->mStreamOut = streamOut;

  if (!SendOpenRequestMessage(channel->mLabel, streamOut,
                              !!(channel->mFlags & DATA_CHANNEL_FLAG_OUT_OF_ORDER_ALLOWED),
                              channel->mPrPolicy, channel->mPrValue)) {
    LOG(("SendOpenRequest failed, errno = %d", errno));
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      channel->mFlags |= DATA_CHANNEL_FLAGS_SEND_REQ;
      StartDefer();
    } else {
      if (channel->mFlags & DATA_CHANNEL_FLAGS_FINISH_OPEN) {
        
        NS_ERROR("Failed to send open request");
        NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                                  DataChannelOnMessageAvailable::ON_CHANNEL_CLOSED, this,
                                  channel));
      }
      
      
      mStreamsOut[streamOut] = nullptr;
      channel->mStreamOut = INVALID_STREAM;
      
      channel->mState = CLOSED;
      return nullptr;
    }
  }
  return channel.forget();
}

int32_t
DataChannelConnection::SendMsgInternal(DataChannel *channel, const char *data,
                                       uint32_t length, uint32_t ppid)
{
  uint16_t flags;
  struct sctp_sendv_spa spa;
  int32_t result;

  NS_ENSURE_TRUE(channel->mState == OPEN || channel->mState == CONNECTING, 0);
  NS_WARN_IF_FALSE(length > 0, "Length is 0?!");

  flags = (channel->mFlags & DATA_CHANNEL_FLAG_OUT_OF_ORDER_ALLOWED) ? SCTP_UNORDERED : 0;

  
  
  
  if (channel->mState == CONNECTING) {
    flags &= ~SCTP_UNORDERED;
  }
  spa.sendv_sndinfo.snd_ppid = htonl(ppid);
  spa.sendv_sndinfo.snd_sid = channel->mStreamOut;
  spa.sendv_sndinfo.snd_flags = flags;
  spa.sendv_sndinfo.snd_context = 0;
  spa.sendv_sndinfo.snd_assoc_id = 0;
  spa.sendv_flags = SCTP_SEND_SNDINFO_VALID;

  if (channel->mPrPolicy != SCTP_PR_SCTP_NONE) {
    spa.sendv_prinfo.pr_policy = channel->mPrPolicy;
    spa.sendv_prinfo.pr_value = channel->mPrValue;
    spa.sendv_flags |= SCTP_SEND_PRINFO_VALID;
  }

  
  
  
  

  
  
  if (channel->mBufferedData.IsEmpty()) {
    result = usrsctp_sendv(mSocket, data, length,
                           nullptr, 0,
                           (void *)&spa, (socklen_t)sizeof(struct sctp_sendv_spa),
                           SCTP_SENDV_SPA, 0);
    LOG(("Sent buffer (len=%u), result=%d", length, result));
  } else {
    
    result = -1;
    errno = EAGAIN;
  }
  if (result < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      
      BufferedMsg *buffered = new BufferedMsg(spa, data, length); 
      channel->mBufferedData.AppendElement(buffered); 
      channel->mFlags |= DATA_CHANNEL_FLAGS_SEND_DATA;
      LOG(("Queued %u buffers (len=%u)", channel->mBufferedData.Length(), length));
      StartDefer();
      return 0;
    }
    LOG(("error %d sending string", errno));
  }
  return result;
}


int32_t
DataChannelConnection::SendBinary(DataChannel *channel, const char *data,
                                  uint32_t len)
{
  
  
  
  
  
  
  
  
  
  

  
  
  if (len > DATA_CHANNEL_MAX_BINARY_FRAGMENT &&
      channel->mPrPolicy == DATA_CHANNEL_RELIABLE) {
    int32_t sent=0;
    uint32_t origlen = len;
    LOG(("Sending binary message length %u in chunks", len));
    
    while (len > 0) {
      uint32_t sendlen = PR_MIN(len, DATA_CHANNEL_MAX_BINARY_FRAGMENT);
      uint32_t ppid;
      len -= sendlen;
      ppid = len > 0 ? DATA_CHANNEL_PPID_BINARY : DATA_CHANNEL_PPID_BINARY_LAST;
      LOG(("Send chunk of %d bytes, ppid %d", sendlen, ppid));
      
      sent += SendMsgInternal(channel, data, sendlen, ppid);
      data += sendlen;
    }
    LOG(("Sent %d buffers for %u bytes, %d sent immediately, %d buffers queued",
         (origlen+DATA_CHANNEL_MAX_BINARY_FRAGMENT-1)/DATA_CHANNEL_MAX_BINARY_FRAGMENT,
         origlen, sent,
         channel->mBufferedData.Length()));
    return sent;
  }
  NS_WARN_IF_FALSE(len <= DATA_CHANNEL_MAX_BINARY_FRAGMENT,
                   "Sending too-large data on unreliable channel!");

  
  return SendMsgInternal(channel, data, len, DATA_CHANNEL_PPID_BINARY_LAST);
}

int32_t
DataChannelConnection::SendBlob(uint16_t stream, nsIInputStream *aBlob)
{
  DataChannel *channel = mStreamsOut[stream];
  NS_ENSURE_TRUE(channel, 0);
  

  LOG(("Sending blob to stream %u", stream));

  
  
  
  
  

  
  
  nsAutoPtr<nsCString> temp(new nsCString());
  uint64_t len;
  aBlob->Available(&len);
  nsresult rv = NS_ReadInputStreamToString(aBlob, *temp, len);

  NS_ENSURE_SUCCESS(rv, 0);

  aBlob->Close();
  

  
  
  
  
  

  
  
  const char *data = temp.get()->BeginReading();
  len              = temp.get()->Length();

  return SendBinary(channel, data, len);
}

int32_t
DataChannelConnection::SendMsgCommon(uint16_t stream, const nsACString &aMsg,
                                     bool isBinary)
{
  ASSERT_WEBRTC(NS_IsMainThread());
  
  
  

  const char *data = aMsg.BeginReading();
  uint32_t len     = aMsg.Length();
  DataChannel *channel;

  LOG(("Sending %sto stream %u: %u bytes", isBinary ? "binary " : "", stream, len));
  
  channel = mStreamsOut[stream];
  NS_ENSURE_TRUE(channel, 0);

  if (isBinary)
    return SendBinary(channel, data, len);
  return SendMsgInternal(channel, data, len, DATA_CHANNEL_PPID_DOMSTRING);
}

void
DataChannelConnection::Close(DataChannel *aChannel)
{
  MutexAutoLock lock(mLock);
  CloseInt(aChannel);
}



void
DataChannelConnection::CloseInt(DataChannel *aChannel)
{
  MOZ_ASSERT(aChannel);
  nsRefPtr<DataChannel> channel(aChannel); 

  mLock.AssertCurrentThreadOwns();
  LOG(("Connection %p/Channel %p: Closing stream %d",
       aChannel->mConnection.get(), aChannel, aChannel->mStreamOut));
  
  if (aChannel->mState == CLOSED || aChannel->mState == CLOSING) {
    LOG(("Channel already closing/closed (%d)", aChannel->mState));
    return;
  }
  aChannel->mBufferedData.Clear();
  if (aChannel->mStreamOut != INVALID_STREAM) {
    ResetOutgoingStream(aChannel->mStreamOut);
    if (mState == CLOSED) { 
      
      
      mStreamsOut[aChannel->mStreamOut] = nullptr;
    } else {
      SendOutgoingStreamReset();
    }
  }
  aChannel->mState = CLOSING;
  if (mState == CLOSED) {
    
    if (channel->mStreamOut != INVALID_STREAM) {
      mStreamsIn[channel->mStreamIn] = nullptr;
    }
    channel->Destroy();
  }
  
}

void DataChannelConnection::CloseAll()
{
  LOG(("Closing all channels"));
  

  
  mState = CLOSED;

  
  
  
  bool closed_some = false;
  for (uint32_t i = 0; i < mStreamsOut.Length(); ++i) {
    if (mStreamsOut[i]) {
      mStreamsOut[i]->Close();
      closed_some = true;
    }
  }

  
  nsRefPtr<DataChannel> channel;
  while (nullptr != (channel = dont_AddRef(static_cast<DataChannel *>(mPending.PopFront())))) {
    LOG(("closing pending channel %p, stream %d", channel.get(), channel->mStreamOut));
    channel->Close(); 
    closed_some = true;
  }
  
  
  if (closed_some) {
    MutexAutoLock lock(mLock);
    SendOutgoingStreamReset();
  }
}

DataChannel::~DataChannel()
{
  
  
  
  NS_ASSERTION(mState == CLOSED || mState == CLOSING, "unexpected state in ~DataChannel");
}

void
DataChannel::Close()
{
  ENSURE_DATACONNECTION;
  mConnection->Close(this);
}


void
DataChannel::Destroy()
{
  ENSURE_DATACONNECTION;

  LOG(("Destroying Data channel %d/%d", mStreamOut, mStreamIn));
  MOZ_ASSERT_IF(mStreamOut != INVALID_STREAM,
                !mConnection->FindChannelByStreamOut(mStreamOut));
  MOZ_ASSERT_IF(mStreamIn != INVALID_STREAM,
                !mConnection->FindChannelByStreamIn(mStreamIn));
  mStreamIn  = INVALID_STREAM;
  mStreamOut = INVALID_STREAM;
  mState = CLOSED;
  mConnection = nullptr;
}

void
DataChannel::SetListener(DataChannelListener *aListener, nsISupports *aContext)
{
  MutexAutoLock mLock(mListenerLock);
  mContext = aContext;
  mListener = aListener;
}


void
DataChannel::AppReady()
{
  ENSURE_DATACONNECTION;

  MutexAutoLock lock(mConnection->mLock);

  mReady = true;
  if (mState == WAITING_TO_OPEN) {
    mState = OPEN;
    NS_DispatchToMainThread(new DataChannelOnMessageAvailable(
                              DataChannelOnMessageAvailable::ON_CHANNEL_OPEN, mConnection,
                              this));
    for (uint32_t i = 0; i < mQueuedMessages.Length(); ++i) {
      nsCOMPtr<nsIRunnable> runnable = mQueuedMessages[i];
      MOZ_ASSERT(runnable);
      NS_DispatchToMainThread(runnable);
    }
  } else {
    NS_ASSERTION(mQueuedMessages.IsEmpty(), "Shouldn't have queued messages if not WAITING_TO_OPEN");
  }
  mQueuedMessages.Clear();
  mQueuedMessages.Compact();
  
  
}

uint32_t
DataChannel::GetBufferedAmount()
{
  uint32_t buffered = 0;
  for (uint32_t i = 0; i < mBufferedData.Length(); ++i) {
    buffered += mBufferedData[i]->mLength;
  }
  return buffered;
}


void
DataChannel::SendOrQueue(DataChannelOnMessageAvailable *aMessage)
{
  if (!mReady &&
      (mState == CONNECTING || mState == WAITING_TO_OPEN)) {
    mQueuedMessages.AppendElement(aMessage);
  } else {
    NS_DispatchToMainThread(aMessage);
  }
}

} 
