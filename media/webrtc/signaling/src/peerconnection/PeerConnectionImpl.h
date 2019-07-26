



#ifndef _PEER_CONNECTION_IMPL_H_
#define _PEER_CONNECTION_IMPL_H_

#include <string>
#include <vector>
#include <map>
#include <cmath>

#include "prlock.h"
#include "mozilla/RefPtr.h"
#include "nsWeakPtr.h"
#include "nsIWeakReferenceUtils.h" 
#include "IPeerConnection.h"
#include "nsComponentManagerUtils.h"
#include "nsPIDOMWindow.h"

#include "dtlsidentity.h"

#include "peer_connection_types.h"
#include "CallControlManager.h"
#include "CC_Device.h"
#include "CC_Call.h"
#include "CC_Observer.h"
#include "MediaPipeline.h"
#include "PeerConnectionMedia.h"

#ifdef MOZILLA_INTERNAL_API
#include "mozilla/TimeStamp.h"
#include "mozilla/net/DataChannel.h"
#include "VideoUtils.h"
#include "VideoSegment.h"
#include "nsNSSShutDown.h"
#else
namespace mozilla {
  class DataChannel;
}
#endif

#if defined(__cplusplus) && __cplusplus >= 201103L
typedef struct Timecard Timecard;
#else
#include "timecard.h"
#endif

using namespace mozilla;

namespace sipcc {

class PeerConnectionWrapper;

struct ConstraintInfo
{
  std::string  value;
  bool         mandatory;
};
typedef std::map<std::string, ConstraintInfo> constraints_map;

class MediaConstraints
{
public:
  void setBooleanConstraint(const std::string& constraint, bool enabled, bool mandatory);

  void buildArray(cc_media_constraints_t** constraintarray);

private:
  constraints_map  mConstraints;
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
    return true;
  }
  bool addTurnServer(const std::string& addr, uint16_t port,
                     const std::string& username,
                     const std::string& pwd)
  {
    
    
    std::vector<unsigned char> password(pwd.begin(), pwd.end());

    NrIceTurnServer* server(NrIceTurnServer::Create(addr, port, username, password));
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

class PeerConnectionWrapper;



#define PC_AUTO_ENTER_API_CALL(assert_ice_ready) \
    do { \
      /* do/while prevents res from conflicting with locals */    \
      nsresult res = CheckApiState(assert_ice_ready);             \
      if (NS_FAILED(res)) return res; \
    } while(0)
#define PC_AUTO_ENTER_API_CALL_NO_CHECK() CheckThread()


class PeerConnectionImpl MOZ_FINAL : public IPeerConnection,
#ifdef MOZILLA_INTERNAL_API
                                     public mozilla::DataChannelConnection::DataConnectionListener,
                                     public nsNSSShutDownObject,
#endif
                                     public sigslot::has_slots<>
{
public:
  PeerConnectionImpl();
  ~PeerConnectionImpl();

  enum ReadyState {
    kNew,
    kNegotiating,
    kActive,
    kClosing,
    kClosed
  };

  
  
  enum SignalingState {
    kSignalingInvalid            = 0,
    kSignalingStable             = 1,
    kSignalingHaveLocalOffer     = 2,
    kSignalingHaveRemoteOffer    = 3,
    kSignalingHaveLocalPranswer  = 4,
    kSignalingHaveRemotePranswer = 5,
    kSignalingClosed             = 6
  };

  enum SipccState {
    kIdle,
    kStarting,
    kStarted
  };

  
  enum IceState {
    kIceGathering,
    kIceWaiting,
    kIceChecking,
    kIceConnected,
    kIceFailed
  };

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
  NS_DECL_IPEERCONNECTION

  static PeerConnectionImpl* CreatePeerConnection();
  static nsresult ConvertRTCConfiguration(const JS::Value& aSrc,
    IceConfiguration *aDst, JSContext* aCx);
  static nsresult ConvertConstraints(
    const JS::Value& aConstraints, MediaConstraints* aObj, JSContext* aCx);
  static already_AddRefed<DOMMediaStream> MakeMediaStream(nsPIDOMWindow* aWindow,
                                                          uint32_t aHint);

  nsresult CreateRemoteSourceStreamInfo(nsRefPtr<RemoteSourceStreamInfo>* aInfo);

  
  virtual void onCallEvent(
    ccapi_call_event_e aCallEvent,
    CSF::CC_CallInfoPtr aInfo
  );

  
  void NotifyConnection();
  void NotifyClosedConnection();
  void NotifyDataChannel(already_AddRefed<mozilla::DataChannel> aChannel);

  
  const nsRefPtr<PeerConnectionMedia>& media() const {
    PC_AUTO_ENTER_API_CALL_NO_CHECK();
    return mMedia;
  }

  
  virtual const std::string& GetHandle();

  
  void IceGatheringCompleted(NrIceCtx *aCtx);
  void IceCompleted(NrIceCtx *aCtx);
  void IceFailed(NrIceCtx *aCtx);
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

  
  mozilla::RefPtr<DtlsIdentity> const GetIdentity() {
    PC_AUTO_ENTER_API_CALL_NO_CHECK();
    return mIdentity;
  }

  
  nsresult CreateFakeMediaStream(uint32_t hint, nsIDOMMediaStream** retval);

  nsPIDOMWindow* GetWindow() const {
    PC_AUTO_ENTER_API_CALL_NO_CHECK();
    return mWindow;
  }

  
  nsresult Initialize(IPeerConnectionObserver* aObserver,
                      nsIDOMWindow* aWindow,
                      const IceConfiguration& aConfiguration,
                      nsIThread* aThread) {
    return Initialize(aObserver, aWindow, &aConfiguration, nullptr, aThread, nullptr);
  }

  
  
  NS_IMETHODIMP CreateOffer(MediaConstraints& aConstraints);
  NS_IMETHODIMP CreateAnswer(MediaConstraints& aConstraints);

  nsresult InitializeDataChannel(int track_id, uint16_t aLocalport,
                                 uint16_t aRemoteport, uint16_t aNumstreams);

  
  
  
  void OnSdpParseError(const char* errorMessage);

  
  
  void ClearSdpParseErrorMessages();

  
  const std::vector<std::string> &GetSdpParseErrors();

  
  void SetSignalingState_m(SignalingState aSignalingState);

#ifdef MOZILLA_INTERNAL_API
  
  void startCallTelem();
#endif

private:
  PeerConnectionImpl(const PeerConnectionImpl&rhs);
  PeerConnectionImpl& operator=(PeerConnectionImpl);
  nsresult Initialize(IPeerConnectionObserver* aObserver,
                      nsIDOMWindow* aWindow,
                      const IceConfiguration* aConfiguration,
                      const JS::Value* aRTCConfiguration,
                      nsIThread* aThread,
                      JSContext* aCx);
  NS_IMETHODIMP CreateOfferInt(MediaConstraints& constraints);
  NS_IMETHODIMP CreateAnswerInt(MediaConstraints& constraints);
  NS_IMETHODIMP EnsureDataConnection(uint16_t aNumstreams);

  nsresult CloseInt();
  void ChangeReadyState(ReadyState aReadyState);
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
#endif

  
  void ShutdownMedia();

  
  nsresult IceStateChange_m(IceState aState);

  
  
  
  Timecard *mTimeCard;

  
  CSF::CC_CallPtr mCall;
  ReadyState mReadyState;
  SignalingState mSignalingState;

  
  IceState mIceState;

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

  
  std::string mHandle;

  
  nsCOMPtr<nsIEventTarget> mSTSThread;

#ifdef MOZILLA_INTERNAL_API
  
	nsRefPtr<mozilla::DataChannelConnection> mDataConnection;
#endif

  nsRefPtr<PeerConnectionMedia> mMedia;

#ifdef MOZILLA_INTERNAL_API
  
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

#endif  
