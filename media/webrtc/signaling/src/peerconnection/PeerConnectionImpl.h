



#ifndef _PEER_CONNECTION_IMPL_H_
#define _PEER_CONNECTION_IMPL_H_

#include <deque>
#include <string>
#include <vector>
#include <map>
#include <cmath>

#include "prlock.h"
#include "mozilla/RefPtr.h"
#include "nsWeakPtr.h"
#include "nsAutoPtr.h"
#include "nsIWeakReferenceUtils.h" 
#include "IPeerConnection.h"
#include "sigslot.h"
#include "nricectx.h"
#include "nricemediastream.h"
#include "nsComponentManagerUtils.h"
#include "nsPIDOMWindow.h"
#include "nsIThread.h"

#include "mozilla/ErrorResult.h"
#include "mozilla/dom/PeerConnectionImplEnumsBinding.h"
#include "StreamBuffer.h"
#include "LoadManagerFactory.h"

#ifdef MOZILLA_INTERNAL_API
#include "mozilla/TimeStamp.h"
#include "mozilla/net/DataChannel.h"
#include "VideoUtils.h"
#include "VideoSegment.h"
#include "nsNSSShutDown.h"
#include "mozilla/dom/RTCStatsReportBinding.h"
#include "nsIPrincipal.h"
#include "mozilla/PeerIdentity.h"
#ifndef USE_FAKE_MEDIA_STREAMS
#include "DOMMediaStream.h"
#endif
#endif

namespace test {
#ifdef USE_FAKE_PCOBSERVER
class AFakePCObserver;
#endif
}

#ifdef USE_FAKE_MEDIA_STREAMS
class Fake_DOMMediaStream;
#endif

class nsGlobalWindow;
class nsIDOMMediaStream;
class nsDOMDataChannel;

namespace mozilla {
class DataChannel;
class DtlsIdentity;
class NrIceCtx;
class NrIceMediaStream;
class NrIceStunServer;
class NrIceTurnServer;
class MediaPipeline;

#ifdef USE_FAKE_MEDIA_STREAMS
typedef Fake_DOMMediaStream DOMMediaStream;
#else
class DOMMediaStream;
#endif

namespace dom {
class RTCConfiguration;
class MediaConstraintsInternal;
class MediaStreamTrack;

#ifdef USE_FAKE_PCOBSERVER
typedef test::AFakePCObserver PeerConnectionObserver;
typedef const char *PCObserverString;
#else
class PeerConnectionObserver;
typedef NS_ConvertUTF8toUTF16 PCObserverString;
#endif
}
class MediaConstraintsExternal;
}

#if defined(__cplusplus) && __cplusplus >= 201103L
typedef struct Timecard Timecard;
#else
#include "timecard.h"
#endif




#define NS_IMETHODIMP_TO_ERRORRESULT(func, rv, ...) \
NS_IMETHODIMP func(__VA_ARGS__);                    \
void func (__VA_ARGS__, rv)

#define NS_IMETHODIMP_TO_ERRORRESULT_RETREF(resulttype, func, rv, ...) \
NS_IMETHODIMP func(__VA_ARGS__, resulttype **result);                  \
already_AddRefed<resulttype> func (__VA_ARGS__, rv)

namespace sipcc {

using mozilla::dom::PeerConnectionObserver;
using mozilla::dom::RTCConfiguration;
using mozilla::dom::MediaConstraintsInternal;
using mozilla::MediaConstraintsExternal;
using mozilla::DOMMediaStream;
using mozilla::NrIceCtx;
using mozilla::NrIceMediaStream;
using mozilla::DtlsIdentity;
using mozilla::ErrorResult;
using mozilla::NrIceStunServer;
using mozilla::NrIceTurnServer;
#ifdef MOZILLA_INTERNAL_API
using mozilla::PeerIdentity;
#endif

class PeerConnectionWrapper;
class PeerConnectionMedia;
class RemoteSourceStreamInfo;
class OnCallEventArgs;

class IceConfiguration
{
public:
  bool addStunServer(const std::string& addr, uint16_t port)
  {
    NrIceStunServer* server(NrIceStunServer::Create(addr, port));
    if (!server) {
      return false;
    }
    addStunServer(*server);
    return true;
  }
  bool addTurnServer(const std::string& addr, uint16_t port,
                     const std::string& username,
                     const std::string& pwd,
                     const char* transport)
  {
    
    
    std::vector<unsigned char> password(pwd.begin(), pwd.end());

    NrIceTurnServer* server(NrIceTurnServer::Create(addr, port, username, password,
                                                    transport));
    if (!server) {
      return false;
    }
    addTurnServer(*server);
    return true;
  }
  void addStunServer(const NrIceStunServer& server) { mStunServers.push_back (server); }
  void addTurnServer(const NrIceTurnServer& server) { mTurnServers.push_back (server); }
  const std::vector<NrIceStunServer>& getStunServers() const { return mStunServers; }
  const std::vector<NrIceTurnServer>& getTurnServers() const { return mTurnServers; }
private:
  std::vector<NrIceStunServer> mStunServers;
  std::vector<NrIceTurnServer> mTurnServers;
};

#ifdef MOZILLA_INTERNAL_API

class RTCStatsQuery {
  public:
    explicit RTCStatsQuery(bool internalStats);
    ~RTCStatsQuery();

    mozilla::dom::RTCStatsReportInternal report;
    std::string error;
    
    mozilla::TimeStamp iceStartTime;
    
    bool failed;

  private:
    friend class PeerConnectionImpl;
    std::string pcName;
    bool internalStats;
    nsTArray<mozilla::RefPtr<mozilla::MediaPipeline>> pipelines;
    mozilla::RefPtr<NrIceCtx> iceCtx;
    nsTArray<mozilla::RefPtr<NrIceMediaStream>> streams;
    DOMHighResTimeStamp now;
};
#endif 



#define PC_AUTO_ENTER_API_CALL(assert_ice_ready) \
    do { \
      /* do/while prevents res from conflicting with locals */    \
      nsresult res = CheckApiState(assert_ice_ready);             \
      if (NS_FAILED(res)) return res; \
    } while(0)
#define PC_AUTO_ENTER_API_CALL_NO_CHECK() CheckThread()

class PeerConnectionImpl MOZ_FINAL : public nsISupports,
#ifdef MOZILLA_INTERNAL_API
                                     public mozilla::DataChannelConnection::DataConnectionListener,
                                     public nsNSSShutDownObject,
                                     public DOMMediaStream::PrincipalChangeObserver,
#endif
                                     public sigslot::has_slots<>
{
  class Internal; 

public:
  PeerConnectionImpl(const mozilla::dom::GlobalObject* aGlobal = nullptr);
  virtual ~PeerConnectionImpl();

  enum Error {
    kNoError                          = 0,
    kInvalidConstraintsType           = 1,
    kInvalidCandidateType             = 2,
    kInvalidMediastreamTrack          = 3,
    kInvalidState                     = 4,
    kInvalidSessionDescription        = 5,
    kIncompatibleSessionDescription   = 6,
    kIncompatibleConstraints          = 7,
    kIncompatibleMediaStreamTrack     = 8,
    kInternalError                    = 9
  };

  NS_DECL_THREADSAFE_ISUPPORTS

#ifdef MOZILLA_INTERNAL_API
  virtual JSObject* WrapObject(JSContext* cx);
#endif

  static already_AddRefed<PeerConnectionImpl>
      Constructor(const mozilla::dom::GlobalObject& aGlobal, ErrorResult& rv);
  static PeerConnectionImpl* CreatePeerConnection();
  static nsresult ConvertRTCConfiguration(const RTCConfiguration& aSrc,
                                          IceConfiguration *aDst);
  already_AddRefed<DOMMediaStream> MakeMediaStream(uint32_t aHint);

  nsresult CreateRemoteSourceStreamInfo(nsRefPtr<RemoteSourceStreamInfo>* aInfo);

  
  void onCallEvent(const OnCallEventArgs &args);

  
  void NotifyConnection();
  void NotifyClosedConnection();
  void NotifyDataChannel(already_AddRefed<mozilla::DataChannel> aChannel);

  
  const nsRefPtr<PeerConnectionMedia>& media() const {
    PC_AUTO_ENTER_API_CALL_NO_CHECK();
    return mMedia;
  }

  mozilla::LoadManager* load_manager()  {
    return mLoadManager;
  }

  
  virtual const std::string& GetHandle();

  
  virtual const std::string& GetName();

  
  void IceConnectionStateChange(NrIceCtx* ctx,
                                NrIceCtx::ConnectionState state);
  void IceGatheringStateChange(NrIceCtx* ctx,
                               NrIceCtx::GatheringState state);
  void IceStreamReady(NrIceMediaStream *aStream);

  static void ListenThread(void *aData);
  static void ConnectThread(void *aData);

  
  nsCOMPtr<nsIThread> GetMainThread() {
    PC_AUTO_ENTER_API_CALL_NO_CHECK();
    return mThread;
  }

  
  nsCOMPtr<nsIEventTarget> GetSTSThread() {
    PC_AUTO_ENTER_API_CALL_NO_CHECK();
    return mSTSThread;
  }

  
  mozilla::RefPtr<DtlsIdentity> const GetIdentity() const;
  std::string GetFingerprint() const;
  std::string GetFingerprintAlgorithm() const;
  std::string GetFingerprintHexValue() const;

  
  nsresult CreateFakeMediaStream(uint32_t hint, nsIDOMMediaStream** retval);

  nsPIDOMWindow* GetWindow() const {
    PC_AUTO_ENTER_API_CALL_NO_CHECK();
    return mWindow;
  }

  
  nsresult Initialize(PeerConnectionObserver& aObserver,
                      nsGlobalWindow* aWindow,
                      const IceConfiguration& aConfiguration,
                      nsIThread* aThread) {
    return Initialize(aObserver, aWindow, &aConfiguration, nullptr, aThread);
  }

  
  void Initialize(PeerConnectionObserver& aObserver,
                  nsGlobalWindow& aWindow,
                  const RTCConfiguration& aConfiguration,
                  nsISupports* aThread,
                  ErrorResult &rv)
  {
    nsresult r = Initialize(aObserver, &aWindow, nullptr, &aConfiguration, aThread);
    if (NS_FAILED(r)) {
      rv.Throw(r);
    }
  }

  NS_IMETHODIMP_TO_ERRORRESULT(CreateOffer, ErrorResult &rv,
                               const MediaConstraintsInternal& aConstraints)
  {
    rv = CreateOffer(aConstraints);
  }

  NS_IMETHODIMP_TO_ERRORRESULT(CreateAnswer, ErrorResult &rv,
                               const MediaConstraintsInternal& aConstraints)
  {
    rv = CreateAnswer(aConstraints);
  }

  NS_IMETHODIMP CreateOffer(const MediaConstraintsExternal& aConstraints);
  NS_IMETHODIMP CreateAnswer(const MediaConstraintsExternal& aConstraints);

  NS_IMETHODIMP SetLocalDescription (int32_t aAction, const char* aSDP);

  void SetLocalDescription (int32_t aAction, const nsAString& aSDP, ErrorResult &rv)
  {
    rv = SetLocalDescription(aAction, NS_ConvertUTF16toUTF8(aSDP).get());
  }

  NS_IMETHODIMP SetRemoteDescription (int32_t aAction, const char* aSDP);

  void SetRemoteDescription (int32_t aAction, const nsAString& aSDP, ErrorResult &rv)
  {
    rv = SetRemoteDescription(aAction, NS_ConvertUTF16toUTF8(aSDP).get());
  }

  NS_IMETHODIMP_TO_ERRORRESULT(GetStats, ErrorResult &rv,
                               mozilla::dom::MediaStreamTrack *aSelector)
  {
    rv = GetStats(aSelector);
  }

  NS_IMETHODIMP AddIceCandidate(const char* aCandidate, const char* aMid,
                                unsigned short aLevel);

  void AddIceCandidate(const nsAString& aCandidate, const nsAString& aMid,
                       unsigned short aLevel, ErrorResult &rv)
  {
    rv = AddIceCandidate(NS_ConvertUTF16toUTF8(aCandidate).get(),
                         NS_ConvertUTF16toUTF8(aMid).get(), aLevel);
  }

  NS_IMETHODIMP CloseStreams();

  void CloseStreams(ErrorResult &rv)
  {
    rv = CloseStreams();
  }

  NS_IMETHODIMP_TO_ERRORRESULT(AddStream, ErrorResult &rv,
                               DOMMediaStream& aMediaStream,
                               const MediaConstraintsInternal& aConstraints)
  {
    rv = AddStream(aMediaStream, aConstraints);
  }

  nsresult AddStream(DOMMediaStream &aMediaStream,
                     const MediaConstraintsExternal& aConstraints);

  NS_IMETHODIMP_TO_ERRORRESULT(RemoveStream, ErrorResult &rv,
                               DOMMediaStream& aMediaStream)
  {
    rv = RemoveStream(aMediaStream);
  }


  nsresult GetPeerIdentity(nsAString& peerIdentity)
  {
#ifdef MOZILLA_INTERNAL_API
    if (mPeerIdentity) {
      peerIdentity = mPeerIdentity->ToString();
      return NS_OK;
    }
#endif

    peerIdentity.SetIsVoid(true);
    return NS_OK;
  }

#ifdef MOZILLA_INTERNAL_API
  const PeerIdentity* GetPeerIdentity() const { return mPeerIdentity; }
  nsresult SetPeerIdentity(const nsAString& peerIdentity);
#endif

  
  bool PrivacyRequested() const { return mPrivacyRequested; }

  NS_IMETHODIMP GetFingerprint(char** fingerprint);
  void GetFingerprint(nsAString& fingerprint)
  {
    char *tmp;
    GetFingerprint(&tmp);
    fingerprint.AssignASCII(tmp);
    delete[] tmp;
  }

  NS_IMETHODIMP GetLocalDescription(char** aSDP);

  void GetLocalDescription(nsAString& aSDP)
  {
    char *tmp;
    GetLocalDescription(&tmp);
    aSDP.AssignASCII(tmp);
    delete tmp;
  }

  NS_IMETHODIMP GetRemoteDescription(char** aSDP);

  void GetRemoteDescription(nsAString& aSDP)
  {
    char *tmp;
    GetRemoteDescription(&tmp);
    aSDP.AssignASCII(tmp);
    delete tmp;
  }

  NS_IMETHODIMP ReadyState(mozilla::dom::PCImplReadyState* aState);

  mozilla::dom::PCImplReadyState ReadyState()
  {
    mozilla::dom::PCImplReadyState state;
    ReadyState(&state);
    return state;
  }

  NS_IMETHODIMP SignalingState(mozilla::dom::PCImplSignalingState* aState);

  mozilla::dom::PCImplSignalingState SignalingState()
  {
    mozilla::dom::PCImplSignalingState state;
    SignalingState(&state);
    return state;
  }

  NS_IMETHODIMP SipccState(mozilla::dom::PCImplSipccState* aState);

  mozilla::dom::PCImplSipccState SipccState()
  {
    mozilla::dom::PCImplSipccState state;
    SipccState(&state);
    return state;
  }

  NS_IMETHODIMP IceConnectionState(
      mozilla::dom::PCImplIceConnectionState* aState);

  mozilla::dom::PCImplIceConnectionState IceConnectionState()
  {
    mozilla::dom::PCImplIceConnectionState state;
    IceConnectionState(&state);
    return state;
  }

  NS_IMETHODIMP IceGatheringState(
      mozilla::dom::PCImplIceGatheringState* aState);

  mozilla::dom::PCImplIceGatheringState IceGatheringState()
  {
    mozilla::dom::PCImplIceGatheringState state;
    IceGatheringState(&state);
    return state;
  }

  NS_IMETHODIMP Close();

  void Close(ErrorResult &rv)
  {
    rv = Close();
  }

  nsresult InitializeDataChannel(int track_id, uint16_t aLocalport,
                                 uint16_t aRemoteport, uint16_t aNumstreams);

  NS_IMETHODIMP_TO_ERRORRESULT(ConnectDataConnection, ErrorResult &rv,
                               uint16_t aLocalport,
                               uint16_t aRemoteport,
                               uint16_t aNumstreams)
  {
    rv = ConnectDataConnection(aLocalport, aRemoteport, aNumstreams);
  }

  NS_IMETHODIMP_TO_ERRORRESULT_RETREF(nsDOMDataChannel,
                                      CreateDataChannel, ErrorResult &rv,
                                      const nsAString& aLabel,
                                      const nsAString& aProtocol,
                                      uint16_t aType,
                                      bool outOfOrderAllowed,
                                      uint16_t aMaxTime,
                                      uint16_t aMaxNum,
                                      bool aExternalNegotiated,
                                      uint16_t aStream);

  NS_IMETHODIMP_TO_ERRORRESULT(GetLocalStreams, ErrorResult &rv,
                               nsTArray<nsRefPtr<DOMMediaStream > >& result)
  {
    rv = GetLocalStreams(result);
  }

  NS_IMETHODIMP_TO_ERRORRESULT(GetRemoteStreams, ErrorResult &rv,
                               nsTArray<nsRefPtr<DOMMediaStream > >& result)
  {
    rv = GetRemoteStreams(result);
  }

  
  
  
  void OnSdpParseError(const char* errorMessage);

  
  
  void ClearSdpParseErrorMessages();

  
  const std::vector<std::string> &GetSdpParseErrors();

  
  void SetSignalingState_m(mozilla::dom::PCImplSignalingState aSignalingState);

  bool IsClosed() const;
  
  nsresult SetDtlsConnected(bool aPrivacyRequested);

  bool HasMedia() const;

#ifdef MOZILLA_INTERNAL_API
  
  void startCallTelem();

  nsresult BuildStatsQuery_m(
      mozilla::dom::MediaStreamTrack *aSelector,
      RTCStatsQuery *query);

  static nsresult ExecuteStatsQuery_s(RTCStatsQuery *query);

  
  
  virtual void PrincipalChanged(DOMMediaStream* aMediaStream) MOZ_OVERRIDE;
#endif

private:
  PeerConnectionImpl(const PeerConnectionImpl&rhs);
  PeerConnectionImpl& operator=(PeerConnectionImpl);
  NS_IMETHODIMP Initialize(PeerConnectionObserver& aObserver,
                           nsGlobalWindow* aWindow,
                           const IceConfiguration* aConfiguration,
                           const RTCConfiguration* aRTCConfiguration,
                           nsISupports* aThread);

  NS_IMETHODIMP EnsureDataConnection(uint16_t aNumstreams);

  nsresult CloseInt();
  void ChangeReadyState(mozilla::dom::PCImplReadyState aReadyState);
  nsresult CheckApiState(bool assert_ice_ready) const;
  void CheckThread() const {
    NS_ABORT_IF_FALSE(CheckThreadInt(), "Wrong thread");
  }
  bool CheckThreadInt() const {
#ifdef MOZILLA_INTERNAL_API
    
    
    
    bool on;
    NS_ENSURE_SUCCESS(mThread->IsOnCurrentThread(&on), false);
    NS_ENSURE_TRUE(on, false);
#endif
    return true;
  }

#ifdef MOZILLA_INTERNAL_API
  void virtualDestroyNSSReference() MOZ_FINAL;
  void destructorSafeDestroyNSSReference();
  nsresult GetTimeSinceEpoch(DOMHighResTimeStamp *result);
#endif

  
  void ShutdownMedia();

  
  nsresult IceConnectionStateChange_m(
      mozilla::dom::PCImplIceConnectionState aState);
  nsresult IceGatheringStateChange_m(
      mozilla::dom::PCImplIceGatheringState aState);

  NS_IMETHOD FingerprintSplitHelper(
      std::string& fingerprint, size_t& spaceIdx) const;


#ifdef MOZILLA_INTERNAL_API
  static void GetStatsForPCObserver_s(
      const std::string& pcHandle,
      nsAutoPtr<RTCStatsQuery> query);

  
  static void DeliverStatsReportToPCObserver_m(
      const std::string& pcHandle,
      nsresult result,
      nsAutoPtr<RTCStatsQuery> query);
#endif

  
  
  
  
  
  void RecordLongtermICEStatistics();

  
  
  
  Timecard *mTimeCard;

  
  mozilla::ScopedDeletePtr<Internal> mInternal;
  mozilla::dom::PCImplReadyState mReadyState;
  mozilla::dom::PCImplSignalingState mSignalingState;

  
  mozilla::dom::PCImplIceConnectionState mIceConnectionState;
  mozilla::dom::PCImplIceGatheringState mIceGatheringState;

  
  
  bool mDtlsConnected;

  nsCOMPtr<nsIThread> mThread;
  
  nsWeakPtr mPCObserver;

  nsCOMPtr<nsPIDOMWindow> mWindow;

  
  std::string mLocalRequestedSDP;
  std::string mRemoteRequestedSDP;
  
  std::string mLocalSDP;
  std::string mRemoteSDP;

  
  std::string mFingerprint;
  std::string mRemoteFingerprint;

  
  mozilla::RefPtr<DtlsIdentity> mIdentity;
#ifdef MOZILLA_INTERNAL_API
  
  
  nsAutoPtr<PeerIdentity> mPeerIdentity;
#endif
  
  
  
  
  
  
  bool mPrivacyRequested;

  
  std::string mHandle;

  
  std::string mName;

  
  nsCOMPtr<nsIEventTarget> mSTSThread;

  
  mozilla::LoadManager* mLoadManager;

#ifdef MOZILLA_INTERNAL_API
  
  nsRefPtr<mozilla::DataChannelConnection> mDataConnection;
#endif

  nsRefPtr<PeerConnectionMedia> mMedia;

#ifdef MOZILLA_INTERNAL_API
  
  mozilla::TimeStamp mIceStartTime;
  
  mozilla::TimeStamp mStartTime;
#endif

  
  
  
  
  int mNumAudioStreams;
  int mNumVideoStreams;

  bool mHaveDataStream;

  
  std::vector<std::string> mSDPParseErrorMessages;

  bool mTrickle;

public:
  
  unsigned short listenPort;
  unsigned short connectPort;
  char *connectStr; 
};


class PeerConnectionWrapper
{
 public:
  PeerConnectionWrapper(const std::string& handle);

  PeerConnectionImpl *impl() { return impl_; }

 private:
  nsRefPtr<PeerConnectionImpl> impl_;
};

}  

#undef NS_IMETHODIMP_TO_ERRORRESULT
#undef NS_IMETHODIMP_TO_ERRORRESULT_RETREF
#endif  
