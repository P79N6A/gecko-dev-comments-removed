



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
#include "nsIUUIDGenerator.h"
#include "nsIThread.h"

#include "signaling/src/jsep/JsepSession.h"
#include "signaling/src/jsep/JsepSessionImpl.h"
#include "signaling/src/sdp/SdpMediaSection.h"

#include "mozilla/ErrorResult.h"
#include "mozilla/dom/PeerConnectionImplEnumsBinding.h"
#include "StreamBuffer.h"

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
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
class Fake_MediaStreamTrack;
#endif

class nsGlobalWindow;
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
typedef Fake_MediaStreamTrack MediaStreamTrack;
#else
class DOMMediaStream;
#endif

namespace dom {
struct RTCConfiguration;
struct RTCIceServer;
struct RTCOfferOptions;
#ifdef USE_FAKE_MEDIA_STREAMS
typedef Fake_MediaStreamTrack MediaStreamTrack;
#else
class MediaStreamTrack;
#endif

#ifdef USE_FAKE_PCOBSERVER
typedef test::AFakePCObserver PeerConnectionObserver;
typedef const char *PCObserverString;
#else
class PeerConnectionObserver;
typedef NS_ConvertUTF8toUTF16 PCObserverString;
#endif
}
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

struct MediaStreamTable;

namespace mozilla {

using mozilla::dom::PeerConnectionObserver;
using mozilla::dom::RTCConfiguration;
using mozilla::dom::RTCIceServer;
using mozilla::dom::RTCOfferOptions;
using mozilla::DOMMediaStream;
using mozilla::NrIceCtx;
using mozilla::NrIceMediaStream;
using mozilla::DtlsIdentity;
using mozilla::ErrorResult;
using mozilla::NrIceStunServer;
using mozilla::NrIceTurnServer;
#if !defined(MOZILLA_EXTERNAL_LINKAGE)
using mozilla::PeerIdentity;
#endif

class PeerConnectionWrapper;
class PeerConnectionMedia;
class RemoteSourceStreamInfo;


class PCUuidGenerator : public mozilla::JsepUuidGenerator {
 public:
  virtual bool Generate(std::string* idp) override;

 private:
  nsCOMPtr<nsIUUIDGenerator> mGenerator;
};

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
    delete server;
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
    delete server;
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

#if !defined(MOZILLA_EXTERNAL_LINKAGE)

class RTCStatsQuery {
  public:
    explicit RTCStatsQuery(bool internalStats);
    ~RTCStatsQuery();

    nsAutoPtr<mozilla::dom::RTCStatsReportInternal> report;
    std::string error;
    
    mozilla::TimeStamp iceStartTime;
    
    bool failed;

  private:
    friend class PeerConnectionImpl;
    std::string pcName;
    bool internalStats;
    nsTArray<mozilla::RefPtr<mozilla::MediaPipeline>> pipelines;
    mozilla::RefPtr<NrIceCtx> iceCtx;
    bool grabAllLevels;
    DOMHighResTimeStamp now;
};
#endif 



#define PC_AUTO_ENTER_API_CALL(assert_ice_ready) \
    do { \
      /* do/while prevents res from conflicting with locals */    \
      nsresult res = CheckApiState(assert_ice_ready);             \
      if (NS_FAILED(res)) return res; \
    } while(0)
#define PC_AUTO_ENTER_API_CALL_VOID_RETURN(assert_ice_ready) \
    do { \
      /* do/while prevents res from conflicting with locals */    \
      nsresult res = CheckApiState(assert_ice_ready);             \
      if (NS_FAILED(res)) return; \
    } while(0)
#define PC_AUTO_ENTER_API_CALL_NO_CHECK() CheckThread()

class PeerConnectionImpl final : public nsISupports,
#if !defined(MOZILLA_EXTERNAL_LINKAGE)
                                 public mozilla::DataChannelConnection::DataConnectionListener,
                                 public nsNSSShutDownObject,
                                 public DOMMediaStream::PrincipalChangeObserver,
#endif
                                 public sigslot::has_slots<>
{
  struct Internal; 

public:
  explicit PeerConnectionImpl(const mozilla::dom::GlobalObject* aGlobal = nullptr);

  enum Error {
    kNoError                          = 0,
    kInvalidCandidate                 = 2,
    kInvalidMediastreamTrack          = 3,
    kInvalidState                     = 4,
    kInvalidSessionDescription        = 5,
    kIncompatibleSessionDescription   = 6,
    kIncompatibleMediaStreamTrack     = 8,
    kInternalError                    = 9
  };

  NS_DECL_THREADSAFE_ISUPPORTS

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
  bool WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto, JS::MutableHandle<JSObject*> aReflector);
#endif

  static already_AddRefed<PeerConnectionImpl>
      Constructor(const mozilla::dom::GlobalObject& aGlobal, ErrorResult& rv);
  static PeerConnectionImpl* CreatePeerConnection();
  static nsresult ConvertRTCConfiguration(const RTCConfiguration& aSrc,
                                          IceConfiguration *aDst);
  static nsresult AddIceServer(const RTCIceServer& aServer,
                               IceConfiguration* aDst);
  already_AddRefed<DOMMediaStream> MakeMediaStream();

  nsresult CreateRemoteSourceStreamInfo(nsRefPtr<RemoteSourceStreamInfo>* aInfo,
                                        const std::string& aId);

  
  void NotifyDataChannel(already_AddRefed<mozilla::DataChannel> aChannel)
#if !defined(MOZILLA_EXTERNAL_LINKAGE)
    
    
    override
#endif
    ;

  
  const nsRefPtr<PeerConnectionMedia>& media() const {
    PC_AUTO_ENTER_API_CALL_NO_CHECK();
    return mMedia;
  }

  
  void SetAllowIceLoopback(bool val) { mAllowIceLoopback = val; }

  
  virtual const std::string& GetHandle();

  
  virtual const std::string& GetName();

  
  void IceConnectionStateChange(NrIceCtx* ctx,
                                NrIceCtx::ConnectionState state);
  void IceGatheringStateChange(NrIceCtx* ctx,
                               NrIceCtx::GatheringState state);
  
  void EndOfLocalCandidates(const std::string& defaultAddr,
                            uint16_t defaultPort,
                            uint16_t level);
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
                               const RTCOfferOptions& aOptions)
  {
    rv = CreateOffer(aOptions);
  }

  NS_IMETHODIMP CreateAnswer();
  void CreateAnswer(ErrorResult &rv)
  {
    rv = CreateAnswer();
  }

  NS_IMETHODIMP CreateOffer(
      const mozilla::JsepOfferOptions& aConstraints);

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

  NS_IMETHODIMP_TO_ERRORRESULT(AddTrack, ErrorResult &rv,
      mozilla::dom::MediaStreamTrack& aTrack,
      const mozilla::dom::Sequence<mozilla::dom::OwningNonNull<DOMMediaStream>>& aStreams)
  {
    rv = AddTrack(aTrack, aStreams);
  }

  NS_IMETHODIMP_TO_ERRORRESULT(RemoveTrack, ErrorResult &rv,
                               mozilla::dom::MediaStreamTrack& aTrack)
  {
    rv = RemoveTrack(aTrack);
  }

  nsresult
  AddTrack(mozilla::dom::MediaStreamTrack& aTrack, DOMMediaStream& aStream);

  NS_IMETHODIMP_TO_ERRORRESULT(ReplaceTrack, ErrorResult &rv,
                               mozilla::dom::MediaStreamTrack& aThisTrack,
                               mozilla::dom::MediaStreamTrack& aWithTrack)
  {
    rv = ReplaceTrack(aThisTrack, aWithTrack);
  }

  nsresult GetPeerIdentity(nsAString& peerIdentity)
  {
#if !defined(MOZILLA_EXTERNAL_LINKAGE)
    if (mPeerIdentity) {
      peerIdentity = mPeerIdentity->ToString();
      return NS_OK;
    }
#endif

    peerIdentity.SetIsVoid(true);
    return NS_OK;
  }

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
  const PeerIdentity* GetPeerIdentity() const { return mPeerIdentity; }
  nsresult SetPeerIdentity(const nsAString& peerIdentity);

  const std::string& GetIdAsAscii() const
  {
    return mName;
  }

  nsresult GetId(nsAString& id)
  {
    id = NS_ConvertASCIItoUTF16(mName.c_str());
    return NS_OK;
  }

  nsresult SetId(const nsAString& id)
  {
    mName = NS_ConvertUTF16toUTF8(id).get();
    return NS_OK;
  }
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
    delete[] tmp;
  }

  NS_IMETHODIMP GetRemoteDescription(char** aSDP);

  void GetRemoteDescription(nsAString& aSDP)
  {
    char *tmp;
    GetRemoteDescription(&tmp);
    aSDP.AssignASCII(tmp);
    delete[] tmp;
  }

  NS_IMETHODIMP SignalingState(mozilla::dom::PCImplSignalingState* aState);

  mozilla::dom::PCImplSignalingState SignalingState()
  {
    mozilla::dom::PCImplSignalingState state;
    SignalingState(&state);
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

  bool PluginCrash(uint64_t aPluginID,
                   const nsAString& aPluginName,
                   const nsAString& aPluginDumpID);

  nsresult InitializeDataChannel();

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

  void OnAddIceCandidateError() {
    ++mAddCandidateErrorCount;
  }

  
  const std::vector<std::string> &GetSdpParseErrors();

  
  void SetSignalingState_m(mozilla::dom::PCImplSignalingState aSignalingState);

  
  void UpdateSignalingState();

  bool IsClosed() const;
  
  nsresult SetDtlsConnected(bool aPrivacyRequested);

  bool HasMedia() const;

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
  
  void startCallTelem();

  nsresult BuildStatsQuery_m(
      mozilla::dom::MediaStreamTrack *aSelector,
      RTCStatsQuery *query);

  static nsresult ExecuteStatsQuery_s(RTCStatsQuery *query);

  
  
  virtual void PrincipalChanged(DOMMediaStream* aMediaStream) override;

  nsresult GetRemoteTrackId(const std::string streamId,
                            TrackID numericTrackId,
                            std::string* trackId) const;
#endif

  static std::string GetStreamId(const DOMMediaStream& aStream);
  static std::string GetTrackId(const dom::MediaStreamTrack& track);

private:
  virtual ~PeerConnectionImpl();
  PeerConnectionImpl(const PeerConnectionImpl&rhs);
  PeerConnectionImpl& operator=(PeerConnectionImpl);
  NS_IMETHODIMP Initialize(PeerConnectionObserver& aObserver,
                           nsGlobalWindow* aWindow,
                           const IceConfiguration* aConfiguration,
                           const RTCConfiguration* aRTCConfiguration,
                           nsISupports* aThread);
  nsresult CalculateFingerprint(const std::string& algorithm,
                                std::vector<uint8_t>& fingerprint) const;
  nsresult ConfigureJsepSessionCodecs();

  NS_IMETHODIMP EnsureDataConnection(uint16_t aNumstreams);

  nsresult CloseInt();
  nsresult CheckApiState(bool assert_ice_ready) const;
  void CheckThread() const {
    MOZ_ASSERT(CheckThreadInt(), "Wrong thread");
  }
  bool CheckThreadInt() const {
#if !defined(MOZILLA_EXTERNAL_LINKAGE)
    
    
    
    
    
    bool on;
    NS_ENSURE_SUCCESS(mThread->IsOnCurrentThread(&on), false);
    NS_ENSURE_TRUE(on, false);
#endif
    return true;
  }

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
  void virtualDestroyNSSReference() final;
  void destructorSafeDestroyNSSReference();
  nsresult GetTimeSinceEpoch(DOMHighResTimeStamp *result);
#endif

  
  void ShutdownMedia();

  void CandidateReady(const std::string& candidate, uint16_t level);
  void SendLocalIceCandidateToContent(uint16_t level,
                                      const std::string& mid,
                                      const std::string& candidate);

  nsresult GetDatachannelParameters(
      const mozilla::JsepApplicationCodecDescription** codec,
      uint16_t* level) const;

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
  static void GetStatsForPCObserver_s(
      const std::string& pcHandle,
      nsAutoPtr<RTCStatsQuery> query);

  
  static void DeliverStatsReportToPCObserver_m(
      const std::string& pcHandle,
      nsresult result,
      nsAutoPtr<RTCStatsQuery> query);
#endif

  
  
  
  
  
  void RecordLongtermICEStatistics();

  void OnNegotiationNeeded();

  
  
  
  Timecard *mTimeCard;

  mozilla::dom::PCImplSignalingState mSignalingState;

  
  mozilla::dom::PCImplIceConnectionState mIceConnectionState;
  mozilla::dom::PCImplIceGatheringState mIceGatheringState;

  
  
  bool mDtlsConnected;

  nsCOMPtr<nsIThread> mThread;
  
  nsWeakPtr mPCObserver;

  nsCOMPtr<nsPIDOMWindow> mWindow;

  
  std::string mLocalRequestedSDP;
  std::string mRemoteRequestedSDP;

  
  std::string mFingerprint;
  std::string mRemoteFingerprint;

  
  mozilla::RefPtr<DtlsIdentity> mIdentity;
#if !defined(MOZILLA_EXTERNAL_LINKAGE)
  
  
  nsAutoPtr<PeerIdentity> mPeerIdentity;
#endif
  
  
  
  
  
  
  bool mPrivacyRequested;

  
  std::string mHandle;

  
  std::string mName;

  
  nsCOMPtr<nsIEventTarget> mSTSThread;

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
  
  nsRefPtr<mozilla::DataChannelConnection> mDataConnection;
#endif

  bool mAllowIceLoopback;
  nsRefPtr<PeerConnectionMedia> mMedia;

  
  mozilla::UniquePtr<PCUuidGenerator> mUuidGen;
  mozilla::UniquePtr<mozilla::JsepSession> mJsepSession;

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
  
  mozilla::TimeStamp mIceStartTime;
  
  mozilla::TimeStamp mStartTime;
#endif

  
  
  
  
  int mNumAudioStreams;
  int mNumVideoStreams;

  bool mHaveDataStream;

  unsigned int mAddCandidateErrorCount;

  bool mTrickle;

  bool mShouldSuppressNegotiationNeeded;

public:
  
  unsigned short listenPort;
  unsigned short connectPort;
  char *connectStr; 
};


class PeerConnectionWrapper
{
 public:
  explicit PeerConnectionWrapper(const std::string& handle);

  PeerConnectionImpl *impl() { return impl_; }

 private:
  nsRefPtr<PeerConnectionImpl> impl_;
};

}  

#undef NS_IMETHODIMP_TO_ERRORRESULT
#undef NS_IMETHODIMP_TO_ERRORRESULT_RETREF
#endif
