



#include <iostream>
#include <map>
#include <algorithm>
#include <string>
#include <unistd.h>

#include "base/basictypes.h"
#include "logging.h"

#define GTEST_HAS_RTTI 0
#include "gtest/gtest.h"
#include "gtest_utils.h"

#include "nspr.h"
#include "nss.h"
#include "ssl.h"
#include "prthread.h"

#include "FakeMediaStreams.h"
#include "FakeMediaStreamsImpl.h"
#include "PeerConnectionImpl.h"
#include "PeerConnectionCtx.h"
#include "runnable_utils.h"
#include "nsServiceManagerUtils.h"
#include "nsNetUtil.h"
#include "nsIIOService.h"
#include "nsIDNSService.h"
#include "nsWeakReference.h"
#include "nricectx.h"
#include "mozilla/SyncRunnable.h"
#include "logging.h"
#include "stunserver.h"
#include "stunserver.cpp"

#include "mtransport_test_utils.h"
MtransportTestUtils *test_utils;
nsCOMPtr<nsIThread> gThread;

static int kDefaultTimeout = 5000;
static bool fRtcpMux = true;

static std::string callerName = "caller";
static std::string calleeName = "callee";

#define ARRAY_TO_STL(container, type, array) \
        (container<type>((array), (array) + PR_ARRAY_SIZE(array)))

#define ARRAY_TO_SET(type, array) ARRAY_TO_STL(std::set, type, array)

std::string g_stun_server_address((char *)"23.21.150.121");
uint16_t g_stun_server_port(3478);
std::string kBogusSrflxAddress((char *)"192.0.2.1");
uint16_t kBogusSrflxPort(1001);

namespace test {

std::string indent(const std::string &s, int width = 4) {
  std::string prefix;
  std::string out;
  char previous = '\n';
  prefix.assign(width, ' ');
  for (std::string::const_iterator i = s.begin(); i != s.end(); i++) {
    if (previous == '\n') {
      out += prefix;
    }
    out += *i;
    previous = *i;
  }
  return out;
}

static const std::string strSampleSdpAudioVideoNoIce =
  "v=0\r\n"
  "o=Mozilla-SIPUA 4949 0 IN IP4 10.86.255.143\r\n"
  "s=SIP Call\r\n"
  "t=0 0\r\n"
  "a=ice-ufrag:qkEP\r\n"
  "a=ice-pwd:ed6f9GuHjLcoCN6sC/Eh7fVl\r\n"
  "m=audio 16384 RTP/AVP 0 8 9 101\r\n"
  "c=IN IP4 10.86.255.143\r\n"
  "a=rtpmap:0 PCMU/8000\r\n"
  "a=rtpmap:8 PCMA/8000\r\n"
  "a=rtpmap:9 G722/8000\r\n"
  "a=rtpmap:101 telephone-event/8000\r\n"
  "a=fmtp:101 0-15\r\n"
  "a=sendrecv\r\n"
  "a=candidate:1 1 UDP 2130706431 192.168.2.1 50005 typ host\r\n"
  "a=candidate:2 2 UDP 2130706431 192.168.2.2 50006 typ host\r\n"
  "m=video 1024 RTP/AVP 97\r\n"
  "c=IN IP4 10.86.255.143\r\n"
  "a=rtpmap:120 VP8/90000\r\n"
  "a=fmtp:97 profile-level-id=42E00C\r\n"
  "a=sendrecv\r\n"
  "a=candidate:1 1 UDP 2130706431 192.168.2.3 50007 typ host\r\n"
  "a=candidate:2 2 UDP 2130706431 192.168.2.4 50008 typ host\r\n";


static const std::string strSampleCandidate =
  "a=candidate:1 1 UDP 2130706431 192.168.2.1 50005 typ host\r\n";

static const std::string strSampleMid = "";

static const unsigned short nSamplelevel = 2;

enum sdpTestFlags
{
  SHOULD_SEND_AUDIO     = (1<<0),
  SHOULD_RECV_AUDIO     = (1<<1),
  SHOULD_INACTIVE_AUDIO = (1<<2),
  SHOULD_REJECT_AUDIO   = (1<<3),
  SHOULD_OMIT_AUDIO     = (1<<4),
  DONT_CHECK_AUDIO      = (1<<5),

  SHOULD_SEND_VIDEO     = (1<<8),
  SHOULD_RECV_VIDEO     = (1<<9),
  SHOULD_INACTIVE_VIDEO = (1<<10),
  SHOULD_REJECT_VIDEO   = (1<<11),
  SHOULD_OMIT_VIDEO     = (1<<12),
  DONT_CHECK_VIDEO      = (1<<13),

  SHOULD_INCLUDE_DATA   = (1 << 16),
  DONT_CHECK_DATA       = (1 << 17),

  SHOULD_SENDRECV_AUDIO = SHOULD_SEND_AUDIO | SHOULD_RECV_AUDIO,
  SHOULD_SENDRECV_VIDEO = SHOULD_SEND_VIDEO | SHOULD_RECV_VIDEO,
  SHOULD_SENDRECV_AV = SHOULD_SENDRECV_AUDIO | SHOULD_SENDRECV_VIDEO,

  AUDIO_FLAGS = SHOULD_SEND_AUDIO | SHOULD_RECV_AUDIO
                | SHOULD_INACTIVE_AUDIO | SHOULD_REJECT_AUDIO
                | DONT_CHECK_AUDIO | SHOULD_OMIT_AUDIO,

  VIDEO_FLAGS = SHOULD_SEND_VIDEO | SHOULD_RECV_VIDEO
                | SHOULD_INACTIVE_VIDEO | SHOULD_REJECT_VIDEO
                | DONT_CHECK_VIDEO | SHOULD_OMIT_VIDEO
};

enum offerAnswerFlags
{
  OFFER_NONE  = 0, 
  OFFER_AUDIO = (1<<0),
  OFFER_VIDEO = (1<<1),
  
  ANSWER_NONE  = 0, 
  ANSWER_AUDIO = (1<<8),
  ANSWER_VIDEO = (1<<9),

  OFFER_AV = OFFER_AUDIO | OFFER_VIDEO,
  ANSWER_AV = ANSWER_AUDIO | ANSWER_VIDEO
};

enum mediaPipelineFlags
{
  PIPELINE_LOCAL = (1<<0),
  PIPELINE_RTCP_MUX = (1<<1),
  PIPELINE_SEND = (1<<2),
  PIPELINE_VIDEO = (1<<3),
  PIPELINE_RTCP_NACK = (1<<4)
};


static bool SetupGlobalThread() {
  if (!gThread) {
    nsIThread *thread;

    nsresult rv = NS_NewNamedThread("pseudo-main",&thread);
    if (NS_FAILED(rv))
      return false;

    gThread = thread;
  }
  return true;
}

class TestObserver : public IPeerConnectionObserver,
                     public nsSupportsWeakReference
{
public:
  enum Action {
    OFFER,
    ANSWER
  };

  enum ResponseState {
    stateNoResponse,
    stateSuccess,
    stateError
  };

  TestObserver(sipcc::PeerConnectionImpl *peerConnection,
               const std::string &aName) :
    state(stateNoResponse), addIceSuccessCount(0),
    onAddStreamCalled(false),
    name(aName),
    candidates(),
    pc(peerConnection) {
  }

  virtual ~TestObserver() {}

  std::vector<DOMMediaStream *> GetStreams() { return streams; }

  size_t MatchingCandidates(const std::string& cand) {
    size_t count = 0;

    for (size_t i=0; i<candidates.size(); ++i) {
      if (candidates[i].find(cand) != std::string::npos)
        ++count;
    }

    return count;
  }

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_IPEERCONNECTIONOBSERVER

  ResponseState state;
  char *lastString;
  sipcc::PeerConnectionImpl::Error lastStatusCode;
  uint32_t lastStateType;
  int addIceSuccessCount;
  bool onAddStreamCalled;
  std::string name;
  std::vector<std::string> candidates;

private:
  sipcc::PeerConnectionImpl *pc;
  std::vector<DOMMediaStream *> streams;
};

NS_IMPL_ISUPPORTS2(TestObserver,
                   IPeerConnectionObserver,
                   nsISupportsWeakReference)

NS_IMETHODIMP
TestObserver::OnCreateOfferSuccess(const char* offer)
{
  lastString = strdup(offer);
  state = stateSuccess;
  std::cout << name << ": onCreateOfferSuccess = " << std::endl << indent(offer)
            << std::endl;
  return NS_OK;
}

NS_IMETHODIMP
TestObserver::OnCreateOfferError(uint32_t code, const char *message)
{
  lastStatusCode = static_cast<sipcc::PeerConnectionImpl::Error>(code);
  state = stateError;
  std::cout << name << ": onCreateOfferError = " << code
            << " (" << message << ")" << std::endl;
  return NS_OK;
}

NS_IMETHODIMP
TestObserver::OnCreateAnswerSuccess(const char* answer)
{
  lastString = strdup(answer);
  state = stateSuccess;
  std::cout << name << ": onCreateAnswerSuccess =" << std::endl
            << indent(answer) << std::endl;
  return NS_OK;
}

NS_IMETHODIMP
TestObserver::OnCreateAnswerError(uint32_t code, const char *message)
{
  lastStatusCode = static_cast<sipcc::PeerConnectionImpl::Error>(code);
  std::cout << name << ": onCreateAnswerError = " << code
            << " (" << message << ")" << std::endl;
  state = stateError;
  return NS_OK;
}

NS_IMETHODIMP
TestObserver::OnSetLocalDescriptionSuccess()
{
  lastStatusCode = sipcc::PeerConnectionImpl::kNoError;
  state = stateSuccess;
  std::cout << name << ": onSetLocalDescriptionSuccess" << std::endl;
  return NS_OK;
}

NS_IMETHODIMP
TestObserver::OnSetRemoteDescriptionSuccess()
{
  lastStatusCode = sipcc::PeerConnectionImpl::kNoError;
  state = stateSuccess;
  std::cout << name << ": onSetRemoteDescriptionSuccess" << std::endl;
  return NS_OK;
}

NS_IMETHODIMP
TestObserver::OnSetLocalDescriptionError(uint32_t code, const char *message)
{
  lastStatusCode = static_cast<sipcc::PeerConnectionImpl::Error>(code);
  state = stateError;
  std::cout << name << ": onSetLocalDescriptionError = " << code
            << " (" << message << ")" << std::endl;
  return NS_OK;
}

NS_IMETHODIMP
TestObserver::OnSetRemoteDescriptionError(uint32_t code, const char *message)
{
  lastStatusCode = static_cast<sipcc::PeerConnectionImpl::Error>(code);
  state = stateError;
  std::cout << name << ": onSetRemoteDescriptionError = " << code
            << " (" << message << ")" << std::endl;
  return NS_OK;
}

NS_IMETHODIMP
TestObserver::NotifyConnection()
{
  std::cout << name << ": NotifyConnection" << std::endl;
  return NS_OK;
}

NS_IMETHODIMP
TestObserver::NotifyClosedConnection()
{
  std::cout << name << ": NotifyClosedConnection" << std::endl;
  return NS_OK;
}

NS_IMETHODIMP
TestObserver::NotifyDataChannel(nsIDOMDataChannel *channel)
{
  std::cout << name << ": NotifyDataChannel" << std::endl;
  return NS_OK;
}

NS_IMETHODIMP
TestObserver::OnStateChange(uint32_t state_type)
{
  nsresult rv;
  uint32_t gotstate;

  std::cout << name << ": ";

  switch (state_type)
  {
  case IPeerConnectionObserver::kReadyState:
    rv = pc->GetReadyState(&gotstate);
    NS_ENSURE_SUCCESS(rv, rv);
    std::cout << "Ready State: " << gotstate << std::endl;
    break;
  case IPeerConnectionObserver::kIceState:
    rv = pc->GetIceState(&gotstate);
    NS_ENSURE_SUCCESS(rv, rv);
    std::cout << "ICE State: " << gotstate << std::endl;
    break;
  case IPeerConnectionObserver::kSdpState:
    std::cout << "SDP State: " << std::endl;
    
    break;
  case IPeerConnectionObserver::kSipccState:
    rv = pc->GetSipccState(&gotstate);
    NS_ENSURE_SUCCESS(rv, rv);
    std::cout << "SIPCC State: " << gotstate << std::endl;
    break;
  case IPeerConnectionObserver::kSignalingState:
    rv = pc->GetSignalingState(&gotstate);
    NS_ENSURE_SUCCESS(rv, rv);
    std::cout << "Signaling State: " << gotstate << " (";
    switch (gotstate) {
      case sipcc::PeerConnectionImpl::kSignalingInvalid:
        std::cout << "INVALID";
        break;
      case sipcc::PeerConnectionImpl::kSignalingStable:
        std::cout << "stable";
        break;
      case sipcc::PeerConnectionImpl::kSignalingHaveLocalOffer:
        std::cout << "have-local-offer";
        break;
      case sipcc::PeerConnectionImpl::kSignalingHaveRemoteOffer:
        std::cout << "have-remote-offer";
        break;
      case sipcc::PeerConnectionImpl::kSignalingHaveLocalPranswer:
        std::cout << "have-local-pranswer";
        break;
      case sipcc::PeerConnectionImpl::kSignalingHaveRemotePranswer:
        std::cout << "have-remote-pranswer";
        break;
      case sipcc::PeerConnectionImpl::kSignalingClosed:
        std::cout << "closed";
        break;
      default:
        std::cout << "UNKNOWN";
    }
    std::cout << ")" << std::endl;
    break;
  default:
    
    break;
  }

  lastStateType = state_type;
  return NS_OK;
}


NS_IMETHODIMP
TestObserver::OnAddStream(nsIDOMMediaStream *stream)
{
  PR_ASSERT(stream);

  DOMMediaStream *ms = static_cast<DOMMediaStream *>(stream);

  std::cout << name << ": OnAddStream called hints=" << ms->GetHintContents()
            << " thread=" << PR_GetCurrentThread() << std::endl ;

  onAddStreamCalled = true;

  streams.push_back(ms);

  
  
  Fake_SourceMediaStream *fs = static_cast<Fake_SourceMediaStream *>(ms->GetStream());

  test_utils->sts_target()->Dispatch(
    WrapRunnable(fs, &Fake_SourceMediaStream::Start),
    NS_DISPATCH_NORMAL);

  return NS_OK;
}

NS_IMETHODIMP
TestObserver::OnRemoveStream()
{
  state = stateSuccess;
  return NS_OK;
}

NS_IMETHODIMP
TestObserver::OnAddTrack()
{
  state = stateSuccess;
  return NS_OK;
}

NS_IMETHODIMP
TestObserver::OnRemoveTrack()
{
  state = stateSuccess;
  return NS_OK;
}

NS_IMETHODIMP
TestObserver::OnIceCandidate(uint16_t level,
                             const char * mid,
                             const char * candidate)
{
  std::cout << name << ": onIceCandidate [" << level << "/"
            << mid << "] " << candidate << std::endl;
  candidates.push_back(candidate);
  return NS_OK;
}

NS_IMETHODIMP
TestObserver::OnAddIceCandidateSuccess()
{
  lastStatusCode = sipcc::PeerConnectionImpl::kNoError;
  state = stateSuccess;
  std::cout << name << ": onAddIceCandidateSuccess" << std::endl;
  addIceSuccessCount++;
  return NS_OK;
}

NS_IMETHODIMP
TestObserver::OnAddIceCandidateError(uint32_t code, const char *message)
{
  lastStatusCode = static_cast<sipcc::PeerConnectionImpl::Error>(code);
  state = stateError;
  std::cout << name << ": onAddIceCandidateError = " << code
            << " (" << message << ")" << std::endl;
  return NS_OK;
}

class ParsedSDP {
 public:
  
  typedef std::pair<int, std::string> SdpLine;

  ParsedSDP(std::string sdp):
    sdp_(),
    sdp_without_ice_(),
    ice_candidates_(),
    levels_(0),
    num_lines(0)
  {
    sdp_ = sdp;
    Parse();
  }

  void DeleteAllLines(std::string objType)
  {
    int count = sdp_map_.count(objType);
    std::cout << "Removing " << count << " lines from SDP (" << objType
              << ")" << std::endl;

    for (int i = 0; i < count; i++) {
      DeleteLine(objType);
    }
  }

  void DeleteLine(std::string objType)
  {
    ReplaceLine(objType, "");
  }

  
  
  
  void ReplaceLine(std::string objType, std::string content)
  {
    std::multimap<std::string, SdpLine>::iterator it;
    it = sdp_map_.find(objType);
    if(it != sdp_map_.end()) {
      SdpLine sdp_line_pair = (*it).second;
      int line_no = sdp_line_pair.first;
      sdp_map_.erase(it);
      if(content.empty()) {
        return;
      }
      std::string value = content.substr(objType.length() + 1);
      sdp_map_.insert(std::pair<std::string, SdpLine>(objType,
        std::make_pair(line_no,value)));
    }
  }

  void AddLine(std::string content)
  {
    size_t whiteSpace = content.find(' ');
    std::string key;
    std::string value;
    if(whiteSpace == std::string::npos) {
      key = content.substr(0,  content.size() - 2);
      value = "";
    } else {
      key = content.substr(0, whiteSpace);
      value = content.substr(whiteSpace+1);
    }
    sdp_map_.insert(std::pair<std::string, SdpLine>(key,
      std::make_pair(num_lines,value)));
    num_lines++;
  }

  
  
  std::vector<std::string> GetLines(std::string objType) const
  {
    std::multimap<std::string, SdpLine>::const_iterator it, end;
    std::vector<std::string> values;
    it = sdp_map_.lower_bound(objType);
    end = sdp_map_.upper_bound(objType);
    while (it != end) {
      std::string value = it->second.second;
      if (value.find("\r") != std::string::npos) {
        value = value.substr(0, value.find("\r"));
      } else {
        ADD_FAILURE();
      }
      values.push_back(value);
      ++it;
    }
    return values;
  }

  
  
  
  void Parse()
  {
    size_t prev = 0;
    size_t found = 0;
    num_lines = 0;
    for(;;) {
      found = sdp_.find('\n', found + 1);
      if (found == std::string::npos)
        break;
      std::string line = sdp_.substr(prev, (found - prev) + 1);
      size_t whiteSpace = line.find(' ');
      std::string key;
      std::string value;
      if(whiteSpace == std::string::npos) {
        
        
        key = line.substr(0, line.size() - 2);
        
        value = "";
      } else {
        key = line.substr(0, whiteSpace);
        
        value = line.substr(whiteSpace+1);
      }
      SdpLine sdp_line_pair = std::make_pair(num_lines,value);
      sdp_map_.insert(std::pair<std::string, SdpLine>(key, sdp_line_pair));
      num_lines++;
      
      
      if (line.find("a=candidate") == 0) {
        
        std::string cand = line.substr(2, line.size() - 4);
        ice_candidates_.insert(std::pair<int, std::string>(levels_, cand));
       } else {
        sdp_without_ice_ += line;
      }
      if (line.find("m=") == 0) {
        
        ++levels_;
      }
      prev = found + 1;
    }
  }

  
  std::string getSdp()
  {
     std::vector<std::string> sdp_lines(num_lines);
     for (std::multimap<std::string, SdpLine>::iterator it = sdp_map_.begin();
         it != sdp_map_.end(); ++it) {

      SdpLine sdp_line_pair = (*it).second;
      std::string value;
      if(sdp_line_pair.second.length() == 0) {
        value = (*it).first + "\r\n";
        sdp_lines[sdp_line_pair.first] = value;
      } else {
        value = (*it).first + ' ' + sdp_line_pair.second;
        sdp_lines[sdp_line_pair.first] = value;
      }
   }

    
    std::string sdp;
    for (size_t i = 0; i < sdp_lines.size(); i++)
    {
      sdp += sdp_lines[i];
    }

    return sdp;
  }



  std::string sdp_;
  std::string sdp_without_ice_;
  std::multimap<int, std::string> ice_candidates_;
  std::multimap<std::string, SdpLine> sdp_map_;
  int levels_;
  int num_lines;
};

class SignalingAgent {
 public:
  SignalingAgent(const std::string &aName) : pc(nullptr), name(aName) {
    cfg_.addStunServer(g_stun_server_address, g_stun_server_port);

    pc = sipcc::PeerConnectionImpl::CreatePeerConnection();
    EXPECT_TRUE(pc);
  }

  SignalingAgent(const std::string &aName, const std::string stun_addr,
                 uint16_t stun_port) : pc(nullptr), name(aName) {
    cfg_.addStunServer(stun_addr, stun_port);

    pc = sipcc::PeerConnectionImpl::CreatePeerConnection();
    EXPECT_TRUE(pc);
  }


  ~SignalingAgent() {
    mozilla::SyncRunnable::DispatchToThread(gThread,
      WrapRunnable(this, &SignalingAgent::Close));
  }

  void Init_m(nsCOMPtr<nsIThread> thread)
  {
    pObserver = new TestObserver(pc, name);
    ASSERT_TRUE(pObserver);

    ASSERT_EQ(pc->Initialize(pObserver, nullptr, cfg_, thread), NS_OK);
  }

  void Init(nsCOMPtr<nsIThread> thread)
  {
    mozilla::SyncRunnable::DispatchToThread(thread,
      WrapRunnable(this, &SignalingAgent::Init_m, thread));

    ASSERT_TRUE_WAIT(sipcc_state() == sipcc::PeerConnectionImpl::kStarted,
                     kDefaultTimeout);
  }

  void WaitForGather() {
    ASSERT_TRUE_WAIT(ice_state() == sipcc::PeerConnectionImpl::kIceWaiting, 5000);

    std::cout << name << ": Init Complete" << std::endl;
  }

  bool WaitForGatherAllowFail() {
    EXPECT_TRUE_WAIT(ice_state() == sipcc::PeerConnectionImpl::kIceWaiting ||
                     ice_state() == sipcc::PeerConnectionImpl::kIceFailed, 5000);

    if (ice_state() == sipcc::PeerConnectionImpl::kIceFailed) {
      std::cout << name << ": Init Failed" << std::endl;
      return false;
    }

    std::cout << name << "Init Complete" << std::endl;
    return true;
  }

  uint32_t sipcc_state()
  {
    uint32_t res;

    pc->GetSipccState(&res);
    return res;
  }

  uint32_t ice_state()
  {
    uint32_t res;

    pc->GetIceState(&res);
    return res;
  }

  sipcc::PeerConnectionImpl::SignalingState signaling_state()
  {
    uint32_t res;

   pc->GetSignalingState(&res);
    return static_cast<sipcc::PeerConnectionImpl::SignalingState>(res);
  }

  void Close()
  {
    if (pc) {
      std::cout << name << ": Close" << std::endl;

      pc->Close();
      pc = nullptr;
    }

    
    
    
  }

  bool OfferContains(const std::string& str) {
    std::string o(offer());

    return o.find(str) != std::string::npos;
  }

  bool AnswerContains(const std::string& str) {
    std::string o(answer());

    return o.find(str) != std::string::npos;
  }

  size_t MatchingCandidates(const std::string& cand) {
    return pObserver->MatchingCandidates(cand);
  }

  char* offer() const { return offer_; }
  char* answer() const { return answer_; }

  std::string getLocalDescription() const {
    char *sdp = nullptr;
    pc->GetLocalDescription(&sdp);
    if (!sdp) {
      return "";
    }
    return sdp;
  }

  std::string getRemoteDescription() const {
    char *sdp = 0;
    pc->GetRemoteDescription(&sdp);
    if (!sdp) {
      return "";
    }
    return sdp;
  }

  
  void AddStream(uint32_t hint =
         DOMMediaStream::HINT_CONTENTS_AUDIO |
         DOMMediaStream::HINT_CONTENTS_VIDEO,
       MediaStream *stream = nullptr) {

    nsRefPtr<DOMMediaStream> domMediaStream;
    if (stream) {
      domMediaStream = new DOMMediaStream(stream);
    } else {
      domMediaStream = new DOMMediaStream();
    }

    domMediaStream->SetHintContents(hint);
    ASSERT_EQ(pc->AddStream(domMediaStream), NS_OK);
    domMediaStream_ = domMediaStream;
  }


  
  
  
  void RemoveLastStreamAdded() {
    ASSERT_EQ(pc->RemoveStream(domMediaStream_), NS_OK);
  }

  void CreateOffer(sipcc::MediaConstraints& constraints,
                   uint32_t offerFlags, uint32_t sdpCheck,
                   sipcc::PeerConnectionImpl::SignalingState endState =
                     sipcc::PeerConnectionImpl::kSignalingStable) {

    
    Fake_AudioStreamSource *audio_stream =
      new Fake_AudioStreamSource();

    nsresult ret;
    mozilla::SyncRunnable::DispatchToThread(
      test_utils->sts_target(),
      WrapRunnableRet(audio_stream, &Fake_MediaStream::Start, &ret));

    ASSERT_TRUE(NS_SUCCEEDED(ret));

    uint32_t aHintContents = 0;
    if (offerFlags & OFFER_AUDIO) {
      aHintContents |= DOMMediaStream::HINT_CONTENTS_AUDIO;
    }
    if (offerFlags & OFFER_VIDEO) {
      aHintContents |= DOMMediaStream::HINT_CONTENTS_VIDEO;
    }
    AddStream(aHintContents, audio_stream);

    
    pObserver->state = TestObserver::stateNoResponse;
    ASSERT_EQ(pc->CreateOffer(constraints), NS_OK);
    ASSERT_TRUE_WAIT(pObserver->state != TestObserver::stateNoResponse,
                     kDefaultTimeout);
    ASSERT_EQ(pObserver->state, TestObserver::stateSuccess);
    SDPSanityCheck(pObserver->lastString, sdpCheck, true);
    ASSERT_EQ(signaling_state(), endState);
    offer_ = pObserver->lastString;
  }

void CreateAnswer(sipcc::MediaConstraints& constraints, std::string offer,
                    uint32_t offerAnswerFlags,
                    uint32_t sdpCheck = DONT_CHECK_AUDIO|
                                        DONT_CHECK_VIDEO|
                                        DONT_CHECK_DATA,
                    sipcc::PeerConnectionImpl::SignalingState endState =
                    sipcc::PeerConnectionImpl::kSignalingHaveRemoteOffer) {

    uint32_t aHintContents = 0;
    if (offerAnswerFlags & ANSWER_AUDIO) {
      aHintContents |= DOMMediaStream::HINT_CONTENTS_AUDIO;
    }
    if (offerAnswerFlags & ANSWER_VIDEO) {
      aHintContents |= DOMMediaStream::HINT_CONTENTS_VIDEO;
    }
    AddStream(aHintContents);

    
    
    pObserver->state = TestObserver::stateNoResponse;
    ASSERT_EQ(pc->CreateAnswer(constraints), NS_OK);
    ASSERT_TRUE_WAIT(pObserver->state != TestObserver::stateNoResponse,
                     kDefaultTimeout);
    ASSERT_EQ(pObserver->state, TestObserver::stateSuccess);
    SDPSanityCheck(pObserver->lastString, sdpCheck, false);
    ASSERT_EQ(signaling_state(), endState);

    answer_ = pObserver->lastString;
  }

  
  
  
  
  
  void CreateOfferRemoveStream(sipcc::MediaConstraints& constraints,
                               uint32_t hints, uint32_t sdpCheck) {

    domMediaStream_->SetHintContents(hints);

    
    
    
    
    ASSERT_EQ(pc->RemoveStream(domMediaStream_), NS_OK);

    
    pObserver->state = TestObserver::stateNoResponse;
    ASSERT_EQ(pc->CreateOffer(constraints), NS_OK);
    ASSERT_TRUE_WAIT(pObserver->state != TestObserver::stateNoResponse,
                     kDefaultTimeout);
    ASSERT_TRUE(pObserver->state == TestObserver::stateSuccess);
    SDPSanityCheck(pObserver->lastString, sdpCheck, true);
    offer_ = pObserver->lastString;
  }

  void SetRemote(TestObserver::Action action, std::string remote,
                 bool ignoreError = false,
                 sipcc::PeerConnectionImpl::SignalingState endState =
                   sipcc::PeerConnectionImpl::kSignalingInvalid) {

    if (endState == sipcc::PeerConnectionImpl::kSignalingInvalid) {
      endState = (action == TestObserver::OFFER ?
                  sipcc::PeerConnectionImpl::kSignalingHaveRemoteOffer :
                  sipcc::PeerConnectionImpl::kSignalingStable);
    }

    pObserver->state = TestObserver::stateNoResponse;
    ASSERT_EQ(pc->SetRemoteDescription(action, remote.c_str()), NS_OK);
    ASSERT_TRUE_WAIT(pObserver->state != TestObserver::stateNoResponse,
                     kDefaultTimeout);
    ASSERT_EQ(signaling_state(), endState);
    if (!ignoreError) {
      ASSERT_EQ(pObserver->state, TestObserver::stateSuccess);
    }
  }

  void SetLocal(TestObserver::Action action, std::string local,
                bool ignoreError = false,
                sipcc::PeerConnectionImpl::SignalingState endState =
                  sipcc::PeerConnectionImpl::kSignalingInvalid) {

    if (endState == sipcc::PeerConnectionImpl::kSignalingInvalid) {
      endState = (action == TestObserver::OFFER ?
                  sipcc::PeerConnectionImpl::kSignalingHaveLocalOffer :
                  sipcc::PeerConnectionImpl::kSignalingStable);
    }

    pObserver->state = TestObserver::stateNoResponse;
    ASSERT_EQ(pc->SetLocalDescription(action, local.c_str()), NS_OK);
    ASSERT_TRUE_WAIT(pObserver->state != TestObserver::stateNoResponse,
                     kDefaultTimeout);
    ASSERT_EQ(signaling_state(), endState);
    if (!ignoreError) {
      ASSERT_EQ(pObserver->state, TestObserver::stateSuccess);
    }
  }

  void DoTrickleIce(ParsedSDP &sdp) {
    int expectAddIce = 0;
    pObserver->addIceSuccessCount = 0;
    for (std::multimap<int, std::string>::iterator it =
           sdp.ice_candidates_.begin();
         it != sdp.ice_candidates_.end(); ++it) {
      if ((*it).first != 0) {
        std::cerr << "Adding trickle ICE candidate " << (*it).second
                  << std::endl;
        ASSERT_TRUE(NS_SUCCEEDED(pc->AddIceCandidate((*it).second.c_str(), "", (*it).first)));
        expectAddIce++;
      }
    }
    ASSERT_TRUE_WAIT(pObserver->addIceSuccessCount == expectAddIce,
                     kDefaultTimeout);
  }


  void DoTrickleIceChrome(ParsedSDP &sdp) {
    int expectAddIce = 0;
    pObserver->addIceSuccessCount = 0;
    for (std::multimap<int, std::string>::iterator it =
           sdp.ice_candidates_.begin();
         it != sdp.ice_candidates_.end(); ++it) {
      if ((*it).first != 0) {
        std::string candidate = "a=" + (*it).second + "\r\n";
        std::cerr << "Adding trickle ICE candidate " << candidate << std::endl;

        ASSERT_TRUE(NS_SUCCEEDED(pc->AddIceCandidate(candidate.c_str(), "", (*it).first)));
        expectAddIce++;
      }
    }
    ASSERT_TRUE_WAIT(pObserver->addIceSuccessCount == expectAddIce,
                     kDefaultTimeout);
  }


  bool IceCompleted() {
    uint32_t state;
    pc->GetIceState(&state);
    return state == sipcc::PeerConnectionImpl::kIceConnected;
  }

  void AddIceCandidate(const char* candidate, const char* mid, unsigned short level,
                       bool expectSuccess) {
    sipcc::PeerConnectionImpl::SignalingState endState = signaling_state();
    pObserver->state = TestObserver::stateNoResponse;
    pc->AddIceCandidate(candidate, mid, level);
    ASSERT_TRUE_WAIT(pObserver->state != TestObserver::stateNoResponse,
                     kDefaultTimeout);
    ASSERT_TRUE(pObserver->state ==
                expectSuccess ? TestObserver::stateSuccess :
                                TestObserver::stateError
               );

    
    ASSERT_EQ(signaling_state(), endState);
  }

  int GetPacketsReceived(int stream) {
    std::vector<DOMMediaStream *> streams = pObserver->GetStreams();

    if ((int) streams.size() <= stream) {
      return 0;
    }

    return streams[stream]->GetStream()->AsSourceStream()->GetSegmentsAdded();
  }

  int GetPacketsSent(int stream) {
    return static_cast<Fake_MediaStreamBase *>(
        domMediaStream_->GetStream())->GetSegmentsAdded();
  }

  
  
  void CloseSendStreams() {
    static_cast<Fake_AudioStreamSource*>(
        domMediaStream_->GetStream())->StopStream();
  }

  
  
  void CloseReceiveStreams() {
    std::vector<DOMMediaStream *> streams =
                            pObserver->GetStreams();
    for (size_t i = 0; i < streams.size(); i++) {
      streams[i]->GetStream()->AsSourceStream()->StopStream();
    }
  }

  mozilla::RefPtr<mozilla::MediaPipeline> GetMediaPipeline(
    bool local, int stream, int track) {
    sipcc::SourceStreamInfo *streamInfo;

    if (local) {
      streamInfo = pc->media()->GetLocalStream(stream);
    } else {
      streamInfo = pc->media()->GetRemoteStream(stream);
    }

    if (!streamInfo) {
      return nullptr;
    }

    return streamInfo->GetPipeline(track);
  }

  void CheckMediaPipeline(int stream, int track, uint32_t flags,
    VideoSessionConduit::FrameRequestType frameRequestMethod =
      VideoSessionConduit::FrameRequestNone) {

    std::cout << name << ": Checking media pipeline settings for "
              << ((flags & PIPELINE_LOCAL) ? "local " : "remote ")
              << ((flags & PIPELINE_SEND) ? "sending " : "receiving ")
              << ((flags & PIPELINE_VIDEO) ? "video" : "audio")
              << " pipeline (stream " << stream
              << ", track " << track << "); expect "
              << ((flags & PIPELINE_RTCP_MUX) ? "MUX, " : "no MUX, ")
              << ((flags & PIPELINE_RTCP_NACK) ? "NACK." : "no NACK.")
              << std::endl;

    mozilla::RefPtr<mozilla::MediaPipeline> pipeline =
      GetMediaPipeline((flags & PIPELINE_LOCAL), stream, track);
    ASSERT_TRUE(pipeline);
    ASSERT_EQ(pipeline->IsDoingRtcpMux(), !!(flags & PIPELINE_RTCP_MUX));
    
    if (!(flags & PIPELINE_VIDEO)) {
      if (flags & PIPELINE_SEND) {
        ASSERT_TRUE_WAIT(pipeline->rtp_packets_sent() >= 40 &&
                         pipeline->rtcp_packets_received() >= 1,
                         kDefaultTimeout);
        ASSERT_GE(pipeline->rtp_packets_sent(), 40);
        ASSERT_GE(pipeline->rtcp_packets_received(), 1);
      } else {
        ASSERT_TRUE_WAIT(pipeline->rtp_packets_received() >= 40 &&
                         pipeline->rtcp_packets_sent() >= 1,
                         kDefaultTimeout);
        ASSERT_GE(pipeline->rtp_packets_received(), 40);
        ASSERT_GE(pipeline->rtcp_packets_sent(), 1);
      }
    }


    
    if (flags & PIPELINE_VIDEO) {
        mozilla::MediaSessionConduit *conduit = pipeline->Conduit();
        ASSERT_TRUE(conduit);
        ASSERT_EQ(conduit->type(), mozilla::MediaSessionConduit::VIDEO);
        mozilla::VideoSessionConduit *video_conduit =
          static_cast<mozilla::VideoSessionConduit*>(conduit);
        ASSERT_EQ(!!(flags & PIPELINE_RTCP_NACK),
                  video_conduit->UsingNackBasic());
        ASSERT_EQ(frameRequestMethod, video_conduit->FrameRequestMethod());
    }
  }

public:
  mozilla::RefPtr<sipcc::PeerConnectionImpl> pc;
  nsRefPtr<TestObserver> pObserver;
  char* offer_;
  char* answer_;
  nsRefPtr<DOMMediaStream> domMediaStream_;
  sipcc::IceConfiguration cfg_;
  const std::string name;

private:
  void SDPSanityCheck(std::string sdp, uint32_t flags, bool offer)
  {
    ASSERT_TRUE(pObserver->state == TestObserver::stateSuccess);
    ASSERT_NE(sdp.find("v=0"), std::string::npos);
    ASSERT_NE(sdp.find("c=IN IP4"), std::string::npos);
    ASSERT_NE(sdp.find("a=fingerprint:sha-256"), std::string::npos);

    std::cout << name << ": SDPSanityCheck flags for "
              << (offer ? "offer" : "answer")
              << " = " << std::hex << std::showbase
              << flags << std::dec

              << ((flags & SHOULD_SEND_AUDIO)?" SHOULD_SEND_AUDIO":"")
              << ((flags & SHOULD_RECV_AUDIO)?" SHOULD_RECV_AUDIO":"")
              << ((flags & SHOULD_INACTIVE_AUDIO)?" SHOULD_INACTIVE_AUDIO":"")
              << ((flags & SHOULD_REJECT_AUDIO)?" SHOULD_REJECT_AUDIO":"")
              << ((flags & SHOULD_OMIT_AUDIO)?" SHOULD_OMIT_AUDIO":"")
              << ((flags & DONT_CHECK_AUDIO)?" DONT_CHECK_AUDIO":"")

              << ((flags & SHOULD_SEND_VIDEO)?" SHOULD_SEND_VIDEO":"")
              << ((flags & SHOULD_RECV_VIDEO)?" SHOULD_RECV_VIDEO":"")
              << ((flags & SHOULD_INACTIVE_VIDEO)?" SHOULD_INACTIVE_VIDEO":"")
              << ((flags & SHOULD_REJECT_VIDEO)?" SHOULD_REJECT_VIDEO":"")
              << ((flags & SHOULD_OMIT_VIDEO)?" SHOULD_OMIT_VIDEO":"")
              << ((flags & DONT_CHECK_VIDEO)?" DONT_CHECK_VIDEO":"")

              << ((flags & SHOULD_INCLUDE_DATA)?" SHOULD_INCLUDE_DATA":"")
              << ((flags & DONT_CHECK_DATA)?" DONT_CHECK_DATA":"")
              << std::endl;

    switch(flags & AUDIO_FLAGS) {
      case 0:
            ASSERT_EQ(sdp.find("a=rtpmap:109 opus/48000"), std::string::npos);
        break;
      case SHOULD_SEND_AUDIO:
            ASSERT_NE(sdp.find("a=rtpmap:109 opus/48000"), std::string::npos);
            ASSERT_NE(sdp.find(" 0-15\r\na=sendonly"), std::string::npos);
            if (offer) {
              ASSERT_NE(sdp.find("a=rtpmap:0 PCMU/8000"), std::string::npos);
            }
        break;
      case SHOULD_RECV_AUDIO:
            ASSERT_NE(sdp.find("a=rtpmap:109 opus/48000"), std::string::npos);
            ASSERT_NE(sdp.find(" 0-15\r\na=recvonly"), std::string::npos);
            if (offer) {
              ASSERT_NE(sdp.find("a=rtpmap:0 PCMU/8000"), std::string::npos);
            }
        break;
      case SHOULD_SENDRECV_AUDIO:
            ASSERT_NE(sdp.find("a=rtpmap:109 opus/48000"), std::string::npos);
            ASSERT_NE(sdp.find(" 0-15\r\na=sendrecv"), std::string::npos);
            if (offer) {
              ASSERT_NE(sdp.find("a=rtpmap:0 PCMU/8000"), std::string::npos);
            }
        break;
      case SHOULD_INACTIVE_AUDIO:
            ASSERT_NE(sdp.find("a=rtpmap:109 opus/48000"), std::string::npos);
            ASSERT_NE(sdp.find(" 0-15\r\na=inactive"), std::string::npos);
        break;
      case SHOULD_REJECT_AUDIO:
            ASSERT_EQ(sdp.find("a=rtpmap:109 opus/48000"), std::string::npos);
            ASSERT_NE(sdp.find("m=audio 0 "), std::string::npos);
        break;
      case SHOULD_OMIT_AUDIO:
            ASSERT_EQ(sdp.find("m=audio"), std::string::npos);
        break;
      case DONT_CHECK_AUDIO:
        break;
      default:
            ASSERT_FALSE("Missing case in switch statement");
    }

    switch(flags & VIDEO_FLAGS) {
      case 0:
            ASSERT_EQ(sdp.find("a=rtpmap:120 VP8/90000"), std::string::npos);
        break;
      case SHOULD_SEND_VIDEO:
            ASSERT_NE(sdp.find("a=rtpmap:120 VP8/90000\r\na=sendonly"),
                  std::string::npos);
        break;
      case SHOULD_RECV_VIDEO:
            ASSERT_NE(sdp.find("a=rtpmap:120 VP8/90000\r\na=recvonly"),
                  std::string::npos);
        break;
      case SHOULD_SENDRECV_VIDEO:
            ASSERT_NE(sdp.find("a=rtpmap:120 VP8/90000\r\na=sendrecv"),
                  std::string::npos);
        break;
      case SHOULD_INACTIVE_VIDEO:
            ASSERT_NE(sdp.find("a=rtpmap:120 VP8/90000\r\na=inactive"),
                      std::string::npos);
        break;
      case SHOULD_REJECT_VIDEO:
            ASSERT_NE(sdp.find("m=video 0 "), std::string::npos);
        break;
      case SHOULD_OMIT_VIDEO:
            ASSERT_EQ(sdp.find("m=video"), std::string::npos);
        break;
      case DONT_CHECK_VIDEO:
        break;
      default:
            ASSERT_FALSE("Missing case in switch statement");
    }

    if (flags & SHOULD_INCLUDE_DATA) {
      ASSERT_NE(sdp.find("m=application"), std::string::npos);
    } else if (!(flags & DONT_CHECK_DATA)) {
      ASSERT_EQ(sdp.find("m=application"), std::string::npos);
    }
  }
};

class SignalingEnvironment : public ::testing::Environment {
 public:
  void TearDown() {
    
  }
};

class SignalingAgentTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    ASSERT_TRUE(SetupGlobalThread());
  }

  void TearDown() {
    
    for (size_t i=0; i < agents_.size(); i++) {
      delete agents_[i];
    }
  }

  bool CreateAgent() {
    return CreateAgent(g_stun_server_address, g_stun_server_port);
  }

  bool CreateAgent(const std::string stun_addr, uint16_t stun_port,
                   bool wait_for_gather = true) {
    ScopedDeletePtr<SignalingAgent> agent(
        new SignalingAgent("agent", stun_addr, stun_port));

    agent->Init(gThread);

    if (wait_for_gather) {
      if (!agent->WaitForGatherAllowFail())
        return false;
    }

    agents_.push_back(agent.forget());

    return true;
  }

  void CreateAgentNoInit() {
    ScopedDeletePtr<SignalingAgent> agent(new SignalingAgent("agent"));
    agents_.push_back(agent.forget());
  }

  SignalingAgent *agent(size_t i) {
    return agents_[i];
  }

 private:
  std::vector<SignalingAgent *> agents_;
};


class SignalingTest : public ::testing::Test {
public:
  SignalingTest()
      : init_(false),
        a1_(nullptr),
        a2_(nullptr),
        wait_for_gather_(true),
        stun_addr_(g_stun_server_address),
        stun_port_(g_stun_server_port) {}

  SignalingTest(const std::string& stun_addr, uint16_t stun_port)
      : a1_(nullptr),
        a2_(nullptr),
        wait_for_gather_(true),
        stun_addr_(stun_addr),
        stun_port_(stun_port) {}

  static void SetUpTestCase() {
    ASSERT_TRUE(SetupGlobalThread());
  }

  void EnsureInit() {

    if (init_)
      return;

    a1_ = new SignalingAgent(callerName, stun_addr_, stun_port_);
    a2_ = new SignalingAgent(calleeName, stun_addr_, stun_port_);

    a1_->Init(gThread);
    a2_->Init(gThread);

    if (wait_for_gather_) {
      WaitForGather();
    }
  }

  void WaitForGather() {
    a1_->WaitForGather();
    a2_->WaitForGather();
  }

  static void TearDownTestCase() {
    gThread = nullptr;
  }

  void CreateOffer(sipcc::MediaConstraints& constraints,
                   uint32_t offerFlags, uint32_t sdpCheck) {
    EnsureInit();
    a1_->CreateOffer(constraints, offerFlags, sdpCheck);
  }

  void CreateSetOffer(sipcc::MediaConstraints& constraints, uint32_t sdpCheck) {
    EnsureInit();
    a1_->CreateOffer(constraints, OFFER_AV, sdpCheck);
    a1_->SetLocal(TestObserver::OFFER, a1_->offer());
  }

  void OfferAnswer(sipcc::MediaConstraints& aconstraints,
                   sipcc::MediaConstraints& bconstraints,
                   uint32_t offerAnswerFlags,
                   bool finishAfterAnswer, uint32_t offerSdpCheck,
                   uint32_t answerSdpCheck) {
    EnsureInit();
    a1_->CreateOffer(aconstraints, offerAnswerFlags, offerSdpCheck);
    a1_->SetLocal(TestObserver::OFFER, a1_->offer());
    a2_->SetRemote(TestObserver::OFFER, a1_->offer());
    a2_->CreateAnswer(bconstraints, a1_->offer(),
                     offerAnswerFlags, answerSdpCheck);
    if(true == finishAfterAnswer) {
        a2_->SetLocal(TestObserver::ANSWER, a2_->answer());
        a1_->SetRemote(TestObserver::ANSWER, a2_->answer());

        ASSERT_TRUE_WAIT(a1_->IceCompleted() == true, kDefaultTimeout);
        ASSERT_TRUE_WAIT(a2_->IceCompleted() == true, kDefaultTimeout);
    }
  }

  void OfferModifiedAnswer(sipcc::MediaConstraints& aconstraints,
                           sipcc::MediaConstraints& bconstraints,
                           uint32_t offerSdpCheck, uint32_t answerSdpCheck) {
    EnsureInit();
    a1_->CreateOffer(aconstraints, OFFER_AUDIO, offerSdpCheck);
    a1_->SetLocal(TestObserver::OFFER, a1_->offer());
    a2_->SetRemote(TestObserver::OFFER, a1_->offer());
    a2_->CreateAnswer(bconstraints, a1_->offer(), OFFER_AUDIO | ANSWER_AUDIO,
                     answerSdpCheck);
    a2_->SetLocal(TestObserver::ANSWER, a2_->answer());
    ParsedSDP sdpWrapper(a2_->answer());
    sdpWrapper.ReplaceLine("m=audio", "m=audio 65375 RTP/SAVPF 109 8 101\r\n");
    sdpWrapper.AddLine("a=rtpmap:8 PCMA/8000\r\n");
    std::cout << "Modified SDP " << std::endl
              << indent(sdpWrapper.getSdp()) << std::endl;
    a1_->SetRemote(TestObserver::ANSWER, sdpWrapper.getSdp());
    ASSERT_TRUE_WAIT(a1_->IceCompleted() == true, kDefaultTimeout);
    ASSERT_TRUE_WAIT(a2_->IceCompleted() == true, kDefaultTimeout);
  }

  void OfferAnswerTrickle(sipcc::MediaConstraints& aconstraints,
                          sipcc::MediaConstraints& bconstraints,
                          uint32_t offerSdpCheck, uint32_t answerSdpCheck) {
    EnsureInit();
    a1_->CreateOffer(aconstraints, OFFER_AV, offerSdpCheck);
    a1_->SetLocal(TestObserver::OFFER, a1_->offer());
    ParsedSDP a1_offer(a1_->offer());
    a2_->SetRemote(TestObserver::OFFER, a1_offer.sdp_without_ice_);
    a2_->CreateAnswer(bconstraints, a1_offer.sdp_without_ice_,
                     OFFER_AV|ANSWER_AV, answerSdpCheck);
    a2_->SetLocal(TestObserver::ANSWER, a2_->answer());
    ParsedSDP a2_answer(a2_->answer());
    a1_->SetRemote(TestObserver::ANSWER, a2_answer.sdp_without_ice_);
    
    a1_->DoTrickleIce(a2_answer);
    a2_->DoTrickleIce(a1_offer);
    ASSERT_TRUE_WAIT(a1_->IceCompleted() == true, kDefaultTimeout);
    ASSERT_TRUE_WAIT(a2_->IceCompleted() == true, kDefaultTimeout);
  }


  void OfferAnswerTrickleChrome(sipcc::MediaConstraints& aconstraints,
                          sipcc::MediaConstraints& bconstraints,
                          uint32_t offerSdpCheck, uint32_t answerSdpCheck) {
    EnsureInit();
    a1_->CreateOffer(aconstraints, OFFER_AV, offerSdpCheck);
    a1_->SetLocal(TestObserver::OFFER, a1_->offer());
    ParsedSDP a1_offer(a1_->offer());
    a2_->SetRemote(TestObserver::OFFER, a1_offer.sdp_without_ice_);
    a2_->CreateAnswer(bconstraints, a1_offer.sdp_without_ice_,
                     OFFER_AV|ANSWER_AV, answerSdpCheck);
    a2_->SetLocal(TestObserver::ANSWER, a2_->answer());
    ParsedSDP a2_answer(a2_->answer());
    a1_->SetRemote(TestObserver::ANSWER, a2_answer.sdp_without_ice_);
    
    a1_->DoTrickleIceChrome(a2_answer);
    a2_->DoTrickleIceChrome(a1_offer);
    ASSERT_TRUE_WAIT(a1_->IceCompleted() == true, kDefaultTimeout);
    ASSERT_TRUE_WAIT(a2_->IceCompleted() == true, kDefaultTimeout);
  }

  void CreateOfferRemoveStream(sipcc::MediaConstraints& constraints,
                               uint32_t hints, uint32_t sdpCheck) {
    EnsureInit();
    sipcc::MediaConstraints aconstraints;
    aconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
    aconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
    a1_->CreateOffer(aconstraints, OFFER_AV, SHOULD_SENDRECV_AV );
    a1_->CreateOfferRemoveStream(constraints, hints, sdpCheck);
  }

  void CreateOfferAudioOnly(sipcc::MediaConstraints& constraints,
                            uint32_t sdpCheck) {
    EnsureInit();
    a1_->CreateOffer(constraints, OFFER_AUDIO, sdpCheck);
  }

  void CreateOfferAddCandidate(sipcc::MediaConstraints& constraints,
                               const char * candidate, const char * mid,
                               unsigned short level, uint32_t sdpCheck) {
    EnsureInit();
    a1_->CreateOffer(constraints, OFFER_AV, sdpCheck);
    a1_->AddIceCandidate(candidate, mid, level, true);
  }

  void AddIceCandidateEarly(const char * candidate, const char * mid,
                            unsigned short level) {
    EnsureInit();
    a1_->AddIceCandidate(candidate, mid, level, false);
  }

  void CheckRtcpFbSdp(const std::string &sdp,
                      const std::set<std::string>& expected) {

    std::set<std::string>::const_iterator it;

    
    
    for (it = expected.begin(); it != expected.end(); ++it) {
      std::string attr = std::string("\r\na=rtcp-fb:120 ") + (*it) + "\r\n";
      std::cout << " - Checking for a=rtcp-fb: '" << *it << "'" << std::endl;
      ASSERT_NE(sdp.find(attr), std::string::npos);
    }

    
    
    ParsedSDP sdpWrapper(sdp);
    std::vector<std::string> values = sdpWrapper.GetLines("a=rtcp-fb:120");
    std::vector<std::string>::iterator it2;
    for (it2 = values.begin(); it2 != values.end(); ++it2) {
      std::cout << " - Verifying that rtcp-fb is okay: '" << *it2
                << "'" << std::endl;
      ASSERT_NE(0U, expected.count(*it2));
    }
  }

  void TestRtcpFb(const std::set<std::string>& feedback,
                  uint32_t rtcpFbFlags,
                  VideoSessionConduit::FrameRequestType frameRequestMethod) {
    EnsureInit();
    sipcc::MediaConstraints constraints;

    a1_->CreateOffer(constraints, OFFER_AV, SHOULD_SENDRECV_AV);
    a1_->SetLocal(TestObserver::OFFER, a1_->offer());

    ParsedSDP sdpWrapper(a1_->offer());

    
    sdpWrapper.DeleteAllLines("a=rtcp-fb:120");

    
    
    
    std::set<std::string>::const_iterator it;
    for (it = feedback.begin(); it != feedback.end(); ++it) {
      sdpWrapper.AddLine(std::string("a=rtcp-fb:120 ") + (*it) + "\r\n");
    }

    std::cout << "Modified SDP " << std::endl
              << indent(sdpWrapper.getSdp()) << std::endl;

    
    CheckRtcpFbSdp(sdpWrapper.getSdp(), feedback);

    a2_->SetRemote(TestObserver::OFFER, sdpWrapper.getSdp());
    a2_->CreateAnswer(constraints, sdpWrapper.getSdp(), OFFER_AV | ANSWER_AV);

    CheckRtcpFbSdp(a2_->answer(), feedback);

    a2_->SetLocal(TestObserver::ANSWER, a2_->answer());
    a1_->SetRemote(TestObserver::ANSWER, a2_->answer());

    ASSERT_TRUE_WAIT(a1_->IceCompleted() == true, kDefaultTimeout);
    ASSERT_TRUE_WAIT(a2_->IceCompleted() == true, kDefaultTimeout);

    a1_->CloseSendStreams();
    a1_->CloseReceiveStreams();
    a2_->CloseSendStreams();
    a2_->CloseReceiveStreams();

    
    a1_->CheckMediaPipeline(0, 2, (fRtcpMux ? PIPELINE_RTCP_MUX : 0) |
      PIPELINE_SEND | PIPELINE_VIDEO | rtcpFbFlags, frameRequestMethod);

    
    a2_->CheckMediaPipeline(0, 2, (fRtcpMux ? PIPELINE_RTCP_MUX : 0) |
      PIPELINE_VIDEO | rtcpFbFlags, frameRequestMethod);
  }

  void SetTestStunServer() {
    stun_addr_ = TestStunServer::GetInstance()->addr();
    stun_port_ = TestStunServer::GetInstance()->port();

    TestStunServer::GetInstance()->SetActive(false);
    TestStunServer::GetInstance()->SetResponseAddr(
        kBogusSrflxAddress, kBogusSrflxPort);
  }

 protected:
  bool init_;
  ScopedDeletePtr<SignalingAgent> a1_;  
  ScopedDeletePtr<SignalingAgent> a2_;  
  bool wait_for_gather_;
  std::string stun_addr_;
  uint16_t stun_port_;
};

TEST_F(SignalingTest, JustInit)
{
}

TEST_F(SignalingTest, CreateSetOffer)
{
  sipcc::MediaConstraints constraints;
  CreateSetOffer(constraints, SHOULD_SENDRECV_AV);
}

TEST_F(SignalingTest, CreateOfferAudioVideoConstraintUndefined)
{
  sipcc::MediaConstraints constraints;
  CreateOffer(constraints, OFFER_AV, SHOULD_SENDRECV_AV);
}

TEST_F(SignalingTest, CreateOfferNoVideoStreamRecvVideo)
{
  sipcc::MediaConstraints constraints;
  constraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  constraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  CreateOffer(constraints, OFFER_AUDIO,
              SHOULD_SENDRECV_AUDIO | SHOULD_RECV_VIDEO);
}

TEST_F(SignalingTest, CreateOfferNoAudioStreamRecvAudio)
{
  sipcc::MediaConstraints constraints;
  constraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  constraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  CreateOffer(constraints, OFFER_VIDEO,
              SHOULD_RECV_AUDIO | SHOULD_SENDRECV_VIDEO);
}

TEST_F(SignalingTest, CreateOfferNoVideoStream)
{
  sipcc::MediaConstraints constraints;
  constraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  constraints.setBooleanConstraint("OfferToReceiveVideo", false, false);
  CreateOffer(constraints, OFFER_AUDIO,
              SHOULD_SENDRECV_AUDIO | SHOULD_OMIT_VIDEO);
}

TEST_F(SignalingTest, CreateOfferNoAudioStream)
{
  sipcc::MediaConstraints constraints;
  constraints.setBooleanConstraint("OfferToReceiveAudio", false, false);
  constraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  CreateOffer(constraints, OFFER_VIDEO,
              SHOULD_OMIT_AUDIO | SHOULD_SENDRECV_VIDEO);
}

TEST_F(SignalingTest, CreateOfferDontReceiveAudio)
{
  sipcc::MediaConstraints constraints;
  constraints.setBooleanConstraint("OfferToReceiveAudio", false, false);
  constraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  constraints.setBooleanConstraint("VoiceActivityDetection", true, true);
  CreateOffer(constraints, OFFER_AV,
              SHOULD_SEND_AUDIO | SHOULD_SENDRECV_VIDEO);
}

TEST_F(SignalingTest, CreateOfferDontReceiveVideo)
{
  sipcc::MediaConstraints constraints;
  constraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  constraints.setBooleanConstraint("OfferToReceiveVideo", false, false);
  CreateOffer(constraints, OFFER_AV,
              SHOULD_SENDRECV_AUDIO | SHOULD_SEND_VIDEO);
}


TEST_F(SignalingTest, DISABLED_CreateOfferRemoveAudioStream)
{
  sipcc::MediaConstraints constraints;
  constraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  constraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  CreateOfferRemoveStream(constraints, DOMMediaStream::HINT_CONTENTS_AUDIO,
              SHOULD_RECV_AUDIO | SHOULD_SENDRECV_VIDEO);
}


TEST_F(SignalingTest, DISABLED_CreateOfferDontReceiveAudioRemoveAudioStream)
{
  sipcc::MediaConstraints constraints;
  constraints.setBooleanConstraint("OfferToReceiveAudio", false, false);
  constraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  CreateOfferRemoveStream(constraints, DOMMediaStream::HINT_CONTENTS_AUDIO,
              SHOULD_SENDRECV_VIDEO);
}


TEST_F(SignalingTest, DISABLED_CreateOfferDontReceiveVideoRemoveVideoStream)
{
  sipcc::MediaConstraints constraints;
  constraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  constraints.setBooleanConstraint("OfferToReceiveVideo", false, false);
  CreateOfferRemoveStream(constraints, DOMMediaStream::HINT_CONTENTS_VIDEO,
              SHOULD_SENDRECV_AUDIO);
}

TEST_F(SignalingTest, OfferAnswerNothingDisabled)
{
  sipcc::MediaConstraints constraints;
  OfferAnswer(constraints, constraints, OFFER_AV | ANSWER_AV, false,
              SHOULD_SENDRECV_AV, SHOULD_SENDRECV_AV);
}

TEST_F(SignalingTest, OfferAnswerDontReceiveAudioOnOffer)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", false, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  sipcc::MediaConstraints answerconstraints;
  answerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  answerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  OfferAnswer(offerconstraints, answerconstraints, OFFER_AV | ANSWER_AV,
              false, SHOULD_SEND_AUDIO | SHOULD_SENDRECV_VIDEO,
              SHOULD_RECV_AUDIO | SHOULD_SENDRECV_VIDEO);
}

TEST_F(SignalingTest, OfferAnswerDontReceiveVideoOnOffer)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", false, false);
  sipcc::MediaConstraints answerconstraints;
  answerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  answerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  OfferAnswer(offerconstraints, answerconstraints, OFFER_AV | ANSWER_AV,
              false, SHOULD_SENDRECV_AUDIO | SHOULD_SEND_VIDEO,
              SHOULD_SENDRECV_AUDIO | SHOULD_RECV_VIDEO);
}

TEST_F(SignalingTest, OfferAnswerDontReceiveAudioOnAnswer)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  sipcc::MediaConstraints answerconstraints;
  answerconstraints.setBooleanConstraint("OfferToReceiveAudio", false, false);
  answerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  OfferAnswer(offerconstraints, answerconstraints, OFFER_AV | ANSWER_AV,
              false, SHOULD_SENDRECV_AV,
              SHOULD_SEND_AUDIO | SHOULD_SENDRECV_VIDEO);
}

TEST_F(SignalingTest, OfferAnswerDontReceiveVideoOnAnswer)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  sipcc::MediaConstraints answerconstraints;
  answerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  answerconstraints.setBooleanConstraint("OfferToReceiveVideo", false, false);
  OfferAnswer(offerconstraints, answerconstraints, OFFER_AV | ANSWER_AV,
              false, SHOULD_SENDRECV_AV,
              SHOULD_SENDRECV_AUDIO | SHOULD_SEND_VIDEO);
}

TEST_F(SignalingTest, OfferAnswerDontAddAudioStreamOnOfferRecvAudio)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  sipcc::MediaConstraints answerconstraints;
  answerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  answerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  OfferAnswer(offerconstraints, answerconstraints, OFFER_VIDEO | ANSWER_AV,
              false, SHOULD_RECV_AUDIO | SHOULD_SENDRECV_VIDEO,
              SHOULD_SEND_AUDIO | SHOULD_SENDRECV_VIDEO);
}

TEST_F(SignalingTest, OfferAnswerDontAddAudioStreamOnOffer)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", false, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  sipcc::MediaConstraints answerconstraints;
  answerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  answerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  OfferAnswer(offerconstraints, answerconstraints, OFFER_VIDEO | ANSWER_AV,
              false, SHOULD_OMIT_AUDIO | SHOULD_SENDRECV_VIDEO,
              SHOULD_OMIT_AUDIO | SHOULD_SENDRECV_VIDEO);
}

TEST_F(SignalingTest, OfferAnswerDontAddVideoStreamOnOfferRecvVideo)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  sipcc::MediaConstraints answerconstraints;
  answerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  answerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  OfferAnswer(offerconstraints, answerconstraints, OFFER_AUDIO | ANSWER_AV,
              false, SHOULD_SENDRECV_AUDIO | SHOULD_RECV_VIDEO,
              SHOULD_SENDRECV_AUDIO | SHOULD_SEND_VIDEO);
}

TEST_F(SignalingTest, OfferAnswerDontAddVideoStreamOnOffer)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", false, false);
  sipcc::MediaConstraints answerconstraints;
  answerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  answerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  OfferAnswer(offerconstraints, answerconstraints, OFFER_AUDIO | ANSWER_AV,
              false, SHOULD_SENDRECV_AUDIO | SHOULD_OMIT_VIDEO,
              SHOULD_SENDRECV_AUDIO | SHOULD_OMIT_VIDEO);
}

TEST_F(SignalingTest, OfferAnswerDontAddAudioStreamOnAnswer)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  sipcc::MediaConstraints answerconstraints;
  answerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  answerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  OfferAnswer(offerconstraints, answerconstraints, OFFER_AV | ANSWER_VIDEO,
              false, SHOULD_SENDRECV_AV,
              SHOULD_RECV_AUDIO | SHOULD_SENDRECV_VIDEO);
}

TEST_F(SignalingTest, OfferAnswerDontAddVideoStreamOnAnswer)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  sipcc::MediaConstraints answerconstraints;
  answerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  answerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  OfferAnswer(offerconstraints, answerconstraints, OFFER_AV | ANSWER_AUDIO,
              false, SHOULD_SENDRECV_AV,
              SHOULD_SENDRECV_AUDIO | SHOULD_RECV_VIDEO);
}

TEST_F(SignalingTest, OfferAnswerDontAddVideoStreamOnAnswerDontReceiveVideoOnAnswer)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  sipcc::MediaConstraints answerconstraints;
  answerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  answerconstraints.setBooleanConstraint("OfferToReceiveVideo", false, false);
  OfferAnswer(offerconstraints, answerconstraints, OFFER_AV | ANSWER_AUDIO,
              false, SHOULD_SENDRECV_AV, SHOULD_SENDRECV_AUDIO );
}

TEST_F(SignalingTest, OfferAnswerDontAddAudioStreamOnAnswerDontReceiveAudioOnAnswer)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  sipcc::MediaConstraints answerconstraints;
  answerconstraints.setBooleanConstraint("OfferToReceiveAudio", false, false);
  answerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  OfferAnswer(offerconstraints, answerconstraints, OFFER_AV | ANSWER_VIDEO,
              false, SHOULD_SENDRECV_AV,
              SHOULD_REJECT_AUDIO | SHOULD_SENDRECV_VIDEO);
}

TEST_F(SignalingTest, OfferAnswerDontAddAudioStreamOnOfferDontReceiveAudioOnOffer)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", false, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  sipcc::MediaConstraints answerconstraints;
  answerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  answerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  OfferAnswer(offerconstraints, answerconstraints, OFFER_VIDEO | ANSWER_AV,
              false, SHOULD_SENDRECV_VIDEO, SHOULD_SENDRECV_VIDEO);
}

TEST_F(SignalingTest, OfferAnswerDontAddVideoStreamOnOfferDontReceiveVideoOnOffer)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", false, false);
  sipcc::MediaConstraints answerconstraints;
  answerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  answerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  OfferAnswer(offerconstraints, answerconstraints, OFFER_AUDIO | ANSWER_AV,
              false, SHOULD_SENDRECV_AUDIO | SHOULD_OMIT_VIDEO,
              SHOULD_SENDRECV_AUDIO | SHOULD_OMIT_VIDEO);
}

TEST_F(SignalingTest, OfferAnswerDontReceiveAudioNoAudioStreamOnOfferDontReceiveVideoOnAnswer)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", false, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  sipcc::MediaConstraints answerconstraints;
  answerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  answerconstraints.setBooleanConstraint("OfferToReceiveVideo", false, false);
  OfferAnswer(offerconstraints, answerconstraints, OFFER_VIDEO | ANSWER_AV,
              false, SHOULD_SENDRECV_VIDEO, SHOULD_SEND_VIDEO);
}

TEST_F(SignalingTest, CreateOfferAddCandidate)
{
  sipcc::MediaConstraints constraints;
  CreateOfferAddCandidate(constraints, strSampleCandidate.c_str(),
                          strSampleMid.c_str(), nSamplelevel,
                          SHOULD_SENDRECV_AV);
}

TEST_F(SignalingTest, AddIceCandidateEarly)
{
  sipcc::MediaConstraints constraints;
  AddIceCandidateEarly(strSampleCandidate.c_str(),
                       strSampleMid.c_str(), nSamplelevel);
}



TEST_F(SignalingTest, DISABLED_OfferAnswerReNegotiateOfferAnswerDontReceiveVideoNoVideoStream)
{
  sipcc::MediaConstraints aconstraints;
  aconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  aconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);

  sipcc::MediaConstraints bconstraints;
  bconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  bconstraints.setBooleanConstraint("OfferToReceiveVideo", false, false);

  OfferAnswer(aconstraints, aconstraints, OFFER_AV | ANSWER_AV,
              false, SHOULD_SENDRECV_AV, SHOULD_SENDRECV_AV);
  OfferAnswer(bconstraints, bconstraints, OFFER_AUDIO | ANSWER_AV,
              false, SHOULD_SENDRECV_AUDIO | SHOULD_SEND_VIDEO,
              SHOULD_SENDRECV_AUDIO | SHOULD_INACTIVE_VIDEO);
}

TEST_F(SignalingTest, OfferAnswerDontAddAudioStreamOnAnswerNoConstraints)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  sipcc::MediaConstraints answerconstraints;
  OfferAnswer(offerconstraints, answerconstraints, OFFER_AV | ANSWER_VIDEO,
              false, SHOULD_SENDRECV_AV,
              SHOULD_RECV_AUDIO | SHOULD_SENDRECV_VIDEO);
}

TEST_F(SignalingTest, OfferAnswerDontAddVideoStreamOnAnswerNoConstraints)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  sipcc::MediaConstraints answerconstraints;
  OfferAnswer(offerconstraints, answerconstraints, OFFER_AV | ANSWER_AUDIO,
              false, SHOULD_SENDRECV_AV,
              SHOULD_SENDRECV_AUDIO | SHOULD_RECV_VIDEO);
}

TEST_F(SignalingTest, OfferAnswerDontAddAudioVideoStreamsOnAnswerNoConstraints)
{
  sipcc::MediaConstraints offerconstraints;
  offerconstraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  offerconstraints.setBooleanConstraint("OfferToReceiveVideo", true, false);
  sipcc::MediaConstraints answerconstraints;
  OfferAnswer(offerconstraints, answerconstraints, OFFER_AV | ANSWER_NONE,
              false, SHOULD_SENDRECV_AV,
              SHOULD_RECV_AUDIO | SHOULD_RECV_VIDEO);
}

TEST_F(SignalingTest, FullCall)
{
  sipcc::MediaConstraints constraints;
  OfferAnswer(constraints, constraints, OFFER_AV | ANSWER_AV,
              true, SHOULD_SENDRECV_AV, SHOULD_SENDRECV_AV);

  
  ASSERT_TRUE_WAIT(a1_->GetPacketsSent(0) >= 40 &&
                   a2_->GetPacketsReceived(0) >= 40, kDefaultTimeout * 2);

  a1_->CloseSendStreams();
  a2_->CloseReceiveStreams();
  
  ASSERT_GE(a1_->GetPacketsSent(0), 40);
  
  
  ASSERT_GE(a2_->GetPacketsReceived(0), 40);

  
  
  
  a1_->CheckMediaPipeline(0, 0, fRtcpMux ?
    PIPELINE_LOCAL | PIPELINE_RTCP_MUX | PIPELINE_SEND :
    PIPELINE_LOCAL | PIPELINE_SEND);

  
  a2_->CheckMediaPipeline(0, 1, (fRtcpMux ?  PIPELINE_RTCP_MUX : 0));
}

TEST_F(SignalingTest, FullCallAudioOnly)
{
  sipcc::MediaConstraints constraints;
  OfferAnswer(constraints, constraints, OFFER_AUDIO | ANSWER_AUDIO,
              true, SHOULD_SENDRECV_AUDIO, SHOULD_SENDRECV_AUDIO);

  
  ASSERT_TRUE_WAIT(a1_->GetPacketsSent(0) >= 40 &&
                   a2_->GetPacketsReceived(0) >= 40, kDefaultTimeout * 2);

  a1_->CloseSendStreams();
  a2_->CloseReceiveStreams();
  
  ASSERT_GE(a1_->GetPacketsSent(0), 40);
  
  
  ASSERT_GE(a2_->GetPacketsReceived(0), 40);
}

TEST_F(SignalingTest, FullCallVideoOnly)
{
  sipcc::MediaConstraints constraints;
  OfferAnswer(constraints, constraints, OFFER_VIDEO | ANSWER_VIDEO,
              true, SHOULD_SENDRECV_VIDEO, SHOULD_SENDRECV_VIDEO);

  
  
  
  

  a1_->CloseSendStreams();
  a2_->CloseReceiveStreams();

  
  
  
  
  
  
  
  
  
}

TEST_F(SignalingTest, OfferModifiedAnswer)
{
  sipcc::MediaConstraints constraints;
  OfferModifiedAnswer(constraints, constraints, SHOULD_SENDRECV_AUDIO,
                      SHOULD_SENDRECV_AUDIO);
  a1_->CloseSendStreams();
  a2_->CloseReceiveStreams();
}

TEST_F(SignalingTest, FullCallTrickle)
{
  sipcc::MediaConstraints constraints;
  OfferAnswerTrickle(constraints, constraints,
                     SHOULD_SENDRECV_AV, SHOULD_SENDRECV_AV);

  std::cerr << "ICE handshake completed" << std::endl;

  
  ASSERT_TRUE_WAIT(a1_->GetPacketsSent(0) >= 40 &&
                   a2_->GetPacketsReceived(0) >= 40, kDefaultTimeout * 2);

  a1_->CloseSendStreams();
  a2_->CloseReceiveStreams();
  ASSERT_GE(a1_->GetPacketsSent(0), 40);
  ASSERT_GE(a2_->GetPacketsReceived(0), 40);
}


TEST_F(SignalingTest, FullCallTrickleChrome)
{
  sipcc::MediaConstraints constraints;
  OfferAnswerTrickleChrome(constraints, constraints,
                           SHOULD_SENDRECV_AV, SHOULD_SENDRECV_AV);

  std::cerr << "ICE handshake completed" << std::endl;

  
  ASSERT_TRUE_WAIT(a1_->GetPacketsSent(0) >= 40 &&
                   a2_->GetPacketsReceived(0) >= 40, kDefaultTimeout * 2);

  a1_->CloseSendStreams();
  a2_->CloseReceiveStreams();
  ASSERT_GE(a1_->GetPacketsSent(0), 40);
  ASSERT_GE(a2_->GetPacketsReceived(0), 40);
}


TEST_F(SignalingTest, AudioOnlyG711Call)
{
  EnsureInit();

  sipcc::MediaConstraints constraints;
  std::string offer =
    "v=0\r\n"
    "o=- 1 1 IN IP4 148.147.200.251\r\n"
    "s=-\r\n"
    "b=AS:64\r\n"
    "t=0 0\r\n"
    "a=fingerprint:sha-256 F3:FA:20:C0:CD:48:C4:5F:02:5F:A5:D3:21:D0:2D:48:"
      "7B:31:60:5C:5A:D8:0D:CD:78:78:6C:6D:CE:CC:0C:67\r\n"
    "m=audio 9000 RTP/AVP 0 8 126\r\n"
    "c=IN IP4 148.147.200.251\r\n"
    "b=TIAS:64000\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "a=rtpmap:8 PCMA/8000\r\n"
    "a=rtpmap:126 telephone-event/8000\r\n"
    "a=candidate:0 1 udp 2130706432 148.147.200.251 9000 typ host\r\n"
    "a=candidate:0 2 udp 2130706432 148.147.200.251 9005 typ host\r\n"
    "a=ice-ufrag:cYuakxkEKH+RApYE\r\n"
    "a=ice-pwd:bwtpzLZD+3jbu8vQHvEa6Xuq\r\n"
    "a=sendrecv\r\n";

  std::cout << "Setting offer to:" << std::endl << indent(offer) << std::endl;
  a2_->SetRemote(TestObserver::OFFER, offer);

  std::cout << "Creating answer:" << std::endl;
  a2_->CreateAnswer(constraints, offer, OFFER_AUDIO | ANSWER_AUDIO,
                   DONT_CHECK_AUDIO | DONT_CHECK_VIDEO | DONT_CHECK_DATA);

  std::string answer = a2_->answer();

  
  ASSERT_EQ(answer.find(" opus/"), std::string::npos);

  
  ASSERT_EQ(answer.find("video"), std::string::npos);
  ASSERT_EQ(answer.find("application"), std::string::npos);

  
  ASSERT_NE(answer.find(" PCMU/8000"), std::string::npos);
  ASSERT_NE(answer.find(" telephone-event/8000"), std::string::npos);

  
  ASSERT_NE(answer.find("\r\na=sendrecv"), std::string::npos);

}


TEST_F(SignalingTest, ChromeOfferAnswer)
{
  EnsureInit();

  sipcc::MediaConstraints constraints;

  
  std::string offer =
    "v=0\r\n"
    "o=- 1713781661 2 IN IP4 127.0.0.1\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=group:BUNDLE audio video\r\n"

    "m=audio 1 RTP/SAVPF 103 104 111 0 8 107 106 105 13 126\r\n"
    "a=fingerprint:sha-1 4A:AD:B9:B1:3F:82:18:3B:54:02:12:DF:3E:"
      "5D:49:6B:19:E5:7C:AB\r\n"
    "c=IN IP4 0.0.0.0\r\n"
    "a=rtcp:1 IN IP4 0.0.0.0\r\n"
    "a=ice-ufrag:lBrbdDfrVBH1cldN\r\n"
    "a=ice-pwd:rzh23jet4QpCaEoj9Sl75pL3\r\n"
    "a=ice-options:google-ice\r\n"
    "a=sendrecv\r\n"
    "a=mid:audio\r\n"
    "a=rtcp-mux\r\n"
    "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:"
      "RzrYlzpkTsvgYFD1hQqNCzQ7y4emNLKI1tODsjim\r\n"
    "a=rtpmap:103 ISAC/16000\r\n"
    "a=rtpmap:104 ISAC/32000\r\n"
    
    
    
    
    
    "a=rtpmap:111 opus/48000/2\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "a=rtpmap:8 PCMA/8000\r\n"
    "a=rtpmap:107 CN/48000\r\n"
    "a=rtpmap:106 CN/32000\r\n"
    "a=rtpmap:105 CN/16000\r\n"
    "a=rtpmap:13 CN/8000\r\n"
    "a=rtpmap:126 telephone-event/8000\r\n"
    "a=ssrc:661333377 cname:KIXaNxUlU5DP3fVS\r\n"
    "a=ssrc:661333377 msid:A5UL339RyGxT7zwgyF12BFqesxkmbUsaycp5 a0\r\n"
    "a=ssrc:661333377 mslabel:A5UL339RyGxT7zwgyF12BFqesxkmbUsaycp5\r\n"
    "a=ssrc:661333377 label:A5UL339RyGxT7zwgyF12BFqesxkmbUsaycp5a0\r\n"

    "m=video 1 RTP/SAVPF 100 101 102\r\n"
    "a=fingerprint:sha-1 4A:AD:B9:B1:3F:82:18:3B:54:02:12:DF:3E:5D:49:"
      "6B:19:E5:7C:AB\r\n"
    "c=IN IP4 0.0.0.0\r\n"
    "a=rtcp:1 IN IP4 0.0.0.0\r\n"
    "a=ice-ufrag:lBrbdDfrVBH1cldN\r\n"
    "a=ice-pwd:rzh23jet4QpCaEoj9Sl75pL3\r\n"
    "a=ice-options:google-ice\r\n"
    "a=sendrecv\r\n"
    "a=mid:video\r\n"
    "a=rtcp-mux\r\n"
    "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:"
      "RzrYlzpkTsvgYFD1hQqNCzQ7y4emNLKI1tODsjim\r\n"
    "a=rtpmap:100 VP8/90000\r\n"
    "a=rtpmap:101 red/90000\r\n"
    "a=rtpmap:102 ulpfec/90000\r\n"
    "a=rtcp-fb:100 nack\r\n"
    "a=rtcp-fb:100 ccm fir\r\n"
    "a=ssrc:3012607008 cname:KIXaNxUlU5DP3fVS\r\n"
    "a=ssrc:3012607008 msid:A5UL339RyGxT7zwgyF12BFqesxkmbUsaycp5 v0\r\n"
    "a=ssrc:3012607008 mslabel:A5UL339RyGxT7zwgyF12BFqesxkmbUsaycp5\r\n"
    "a=ssrc:3012607008 label:A5UL339RyGxT7zwgyF12BFqesxkmbUsaycp5v0\r\n";


  std::cout << "Setting offer to:" << std::endl << indent(offer) << std::endl;
  a2_->SetRemote(TestObserver::OFFER, offer);

  std::cout << "Creating answer:" << std::endl;
  a2_->CreateAnswer(constraints, offer, OFFER_AUDIO | ANSWER_AUDIO);

  std::string answer = a2_->answer();
}


TEST_F(SignalingTest, FullChromeHandshake)
{
  EnsureInit();

  sipcc::MediaConstraints constraints;
  constraints.setBooleanConstraint("OfferToReceiveAudio", true, false);
  constraints.setBooleanConstraint("OfferToReceiveVideo", true, false);

  std::string offer = "v=0\r\n"
      "o=- 3835809413 2 IN IP4 127.0.0.1\r\n"
      "s=-\r\n"
      "t=0 0\r\n"
      "a=group:BUNDLE audio video\r\n"
      "a=msid-semantic: WMS ahheYQXHFU52slYMrWNtKUyHCtWZsOJgjlOH\r\n"
      "m=audio 1 RTP/SAVPF 103 104 111 0 8 107 106 105 13 126\r\n"
      "c=IN IP4 1.1.1.1\r\n"
      "a=rtcp:1 IN IP4 1.1.1.1\r\n"
      "a=ice-ufrag:jz9UBk9RT8eCQXiL\r\n"
      "a=ice-pwd:iscXxsdU+0gracg0g5D45orx\r\n"
      "a=ice-options:google-ice\r\n"
      "a=fingerprint:sha-256 A8:76:8C:4C:FA:2E:67:D7:F8:1D:28:4E:90:24:04:"
        "12:EB:B4:A6:69:3D:05:92:E4:91:C3:EA:F9:B7:54:D3:09\r\n"
      "a=sendrecv\r\n"
      "a=mid:audio\r\n"
      "a=rtcp-mux\r\n"
      "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:/he/v44FKu/QvEhex86zV0pdn2V"
        "4Y7wB2xaZ8eUy\r\n"
      "a=rtpmap:103 ISAC/16000\r\n"
      "a=rtpmap:104 ISAC/32000\r\n"
      "a=rtpmap:111 opus/48000/2\r\n"
      "a=rtpmap:0 PCMU/8000\r\n"
      "a=rtpmap:8 PCMA/8000\r\n"
      "a=rtpmap:107 CN/48000\r\n"
      "a=rtpmap:106 CN/32000\r\n"
      "a=rtpmap:105 CN/16000\r\n"
      "a=rtpmap:13 CN/8000\r\n"
      "a=rtpmap:126 telephone-event/8000\r\n"
      "a=ssrc:3389377748 cname:G5I+Jxz4rcaq8IIK\r\n"
      "a=ssrc:3389377748 msid:ahheYQXHFU52slYMrWNtKUyHCtWZsOJgjlOH a0\r\n"
      "a=ssrc:3389377748 mslabel:ahheYQXHFU52slYMrWNtKUyHCtWZsOJgjlOH\r\n"
      "a=ssrc:3389377748 label:ahheYQXHFU52slYMrWNtKUyHCtWZsOJgjlOHa0\r\n"
      "m=video 1 RTP/SAVPF 100 116 117\r\n"
      "c=IN IP4 1.1.1.1\r\n"
      "a=rtcp:1 IN IP4 1.1.1.1\r\n"
      "a=ice-ufrag:jz9UBk9RT8eCQXiL\r\n"
      "a=ice-pwd:iscXxsdU+0gracg0g5D45orx\r\n"
      "a=ice-options:google-ice\r\n"
      "a=fingerprint:sha-256 A8:76:8C:4C:FA:2E:67:D7:F8:1D:28:4E:90:24:04:"
        "12:EB:B4:A6:69:3D:05:92:E4:91:C3:EA:F9:B7:54:D3:09\r\n"
      "a=sendrecv\r\n"
      "a=mid:video\r\n"
      "a=rtcp-mux\r\n"
      "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:/he/v44FKu/QvEhex86zV0pdn2V"
        "4Y7wB2xaZ8eUy\r\n"
      "a=rtpmap:100 VP8/90000\r\n"
      "a=rtpmap:116 red/90000\r\n"
      "a=rtpmap:117 ulpfec/90000\r\n"
      "a=ssrc:3613537198 cname:G5I+Jxz4rcaq8IIK\r\n"
      "a=ssrc:3613537198 msid:ahheYQXHFU52slYMrWNtKUyHCtWZsOJgjlOH v0\r\n"
      "a=ssrc:3613537198 mslabel:ahheYQXHFU52slYMrWNtKUyHCtWZsOJgjlOH\r\n"
      "a=ssrc:3613537198 label:ahheYQXHFU52slYMrWNtKUyHCtWZsOJgjlOHv0\r\n";

  std::cout << "Setting offer to:" << std::endl << indent(offer) << std::endl;
  a2_->SetRemote(TestObserver::OFFER, offer);

  std::cout << "Creating answer:" << std::endl;
  a2_->CreateAnswer(constraints, offer, OFFER_AUDIO | ANSWER_AUDIO);

  std::cout << "Setting answer" << std::endl;
  a2_->SetLocal(TestObserver::ANSWER, a2_->answer());

  std::string answer = a2_->answer();
  ASSERT_NE(answer.find("111 opus/"), std::string::npos);
}


TEST_F(SignalingTest, DISABLED_OfferAllDynamicTypes)
{
  EnsureInit();

  sipcc::MediaConstraints constraints;
  std::string offer;
  for (int i = 96; i < 128; i++)
  {
    std::stringstream ss;
    ss << i;
    std::cout << "Trying dynamic pt = " << i << std::endl;
    offer =
      "v=0\r\n"
      "o=- 1 1 IN IP4 148.147.200.251\r\n"
      "s=-\r\n"
      "b=AS:64\r\n"
      "t=0 0\r\n"
      "a=fingerprint:sha-256 F3:FA:20:C0:CD:48:C4:5F:02:5F:A5:D3:21:D0:2D:48:"
        "7B:31:60:5C:5A:D8:0D:CD:78:78:6C:6D:CE:CC:0C:67\r\n"
      "m=audio 9000 RTP/AVP " + ss.str() + "\r\n"
      "c=IN IP4 148.147.200.251\r\n"
      "b=TIAS:64000\r\n"
      "a=rtpmap:" + ss.str() +" opus/48000/2\r\n"
      "a=candidate:0 1 udp 2130706432 148.147.200.251 9000 typ host\r\n"
      "a=candidate:0 2 udp 2130706432 148.147.200.251 9005 typ host\r\n"
      "a=ice-ufrag:cYuakxkEKH+RApYE\r\n"
      "a=ice-pwd:bwtpzLZD+3jbu8vQHvEa6Xuq\r\n"
      "a=sendrecv\r\n";

      



      a2_->SetRemote(TestObserver::OFFER, offer);

      
      a2_->CreateAnswer(constraints, offer, OFFER_AUDIO | ANSWER_AUDIO);

      std::string answer = a2_->answer();

      ASSERT_NE(answer.find(ss.str() + " opus/"), std::string::npos);
  }

}

TEST_F(SignalingTest, OfferAnswerCheckDescriptions)
{
  sipcc::MediaConstraints constraints;
  OfferAnswer(constraints, constraints, OFFER_AV | ANSWER_AV, true,
              SHOULD_SENDRECV_AV, SHOULD_SENDRECV_AV);

  std::cout << "Caller's Local Description: " << std::endl
            << indent(a1_->getLocalDescription()) << std::endl << std::endl;

  std::cout << "Caller's Remote Description: " << std::endl
            << indent(a1_->getRemoteDescription()) << std::endl << std::endl;

  std::cout << "Callee's Local Description: " << std::endl
            << indent(a2_->getLocalDescription()) << std::endl << std::endl;

  std::cout << "Callee's Remote Description: " << std::endl
            << indent(a2_->getRemoteDescription()) << std::endl << std::endl;

  ASSERT_EQ(a1_->getLocalDescription(),a2_->getRemoteDescription());
  ASSERT_EQ(a2_->getLocalDescription(),a1_->getRemoteDescription());
}

TEST_F(SignalingTest, CheckTrickleSdpChange)
{
  sipcc::MediaConstraints constraints;
  OfferAnswerTrickle(constraints, constraints,
                     SHOULD_SENDRECV_AV, SHOULD_SENDRECV_AV);
  std::cerr << "ICE handshake completed" << std::endl;

  a1_->CloseSendStreams();
  a2_->CloseReceiveStreams();

  std::cout << "Caller's Local Description: " << std::endl
            << indent(a1_->getLocalDescription()) << std::endl << std::endl;

  std::cout << "Caller's Remote Description: " << std::endl
            << indent(a1_->getRemoteDescription()) << std::endl << std::endl;

  std::cout << "Callee's Local Description: " << std::endl
            << indent(a2_->getLocalDescription()) << std::endl << std::endl;

  std::cout << "Callee's Remote Description: " << std::endl
            << indent(a2_->getRemoteDescription()) << std::endl << std::endl;

  ASSERT_NE(a1_->getLocalDescription().find("\r\na=candidate"),
            std::string::npos);
  ASSERT_NE(a1_->getRemoteDescription().find("\r\na=candidate"),
            std::string::npos);
  ASSERT_NE(a2_->getLocalDescription().find("\r\na=candidate"),
            std::string::npos);
  ASSERT_NE(a2_->getRemoteDescription().find("\r\na=candidate"),
            std::string::npos);
  









}

TEST_F(SignalingTest, ipAddrAnyOffer)
{
  EnsureInit();

  sipcc::MediaConstraints constraints;
  std::string offer =
    "v=0\r\n"
    "o=- 1 1 IN IP4 127.0.0.1\r\n"
    "s=-\r\n"
    "b=AS:64\r\n"
    "t=0 0\r\n"
    "a=fingerprint:sha-256 F3:FA:20:C0:CD:48:C4:5F:02:5F:A5:D3:21:D0:2D:48:"
      "7B:31:60:5C:5A:D8:0D:CD:78:78:6C:6D:CE:CC:0C:67\r\n"
    "m=audio 9000 RTP/AVP 99\r\n"
    "c=IN IP4 0.0.0.0\r\n"
    "a=rtpmap:99 opus/48000/2\r\n"
    "a=ice-ufrag:cYuakxkEKH+RApYE\r\n"
    "a=ice-pwd:bwtpzLZD+3jbu8vQHvEa6Xuq\r\n"
    "a=sendrecv\r\n";

    a2_->SetRemote(TestObserver::OFFER, offer);
    ASSERT_TRUE(a2_->pObserver->state == TestObserver::stateSuccess);
    a2_->CreateAnswer(constraints, offer, OFFER_AUDIO | ANSWER_AUDIO);
    ASSERT_TRUE(a2_->pObserver->state == TestObserver::stateSuccess);
    std::string answer = a2_->answer();
    ASSERT_NE(answer.find("a=sendrecv"), std::string::npos);
}

static void CreateSDPForBigOTests(std::string& offer, const char *number) {
  offer =
    "v=0\r\n"
    "o=- ";
  offer += number;
  offer += " ";
  offer += number;
  offer += " IN IP4 127.0.0.1\r\n"
    "s=-\r\n"
    "b=AS:64\r\n"
    "t=0 0\r\n"
    "a=fingerprint:sha-256 F3:FA:20:C0:CD:48:C4:5F:02:5F:A5:D3:21:D0:2D:48:"
      "7B:31:60:5C:5A:D8:0D:CD:78:78:6C:6D:CE:CC:0C:67\r\n"
    "m=audio 9000 RTP/AVP 99\r\n"
    "c=IN IP4 0.0.0.0\r\n"
    "a=rtpmap:99 opus/48000/2\r\n"
    "a=ice-ufrag:cYuakxkEKH+RApYE\r\n"
    "a=ice-pwd:bwtpzLZD+3jbu8vQHvEa6Xuq\r\n"
    "a=sendrecv\r\n";
}

TEST_F(SignalingTest, BigOValues)
{
  EnsureInit();

  std::string offer;

  CreateSDPForBigOTests(offer, "12345678901234567");

  a2_->SetRemote(TestObserver::OFFER, offer);
  ASSERT_EQ(a2_->pObserver->state, TestObserver::stateSuccess);
}

TEST_F(SignalingTest, BigOValuesExtraChars)
{
  EnsureInit();

  std::string offer;

  CreateSDPForBigOTests(offer, "12345678901234567FOOBAR");

  
  
  a2_->SetRemote(TestObserver::OFFER, offer, true,
                sipcc::PeerConnectionImpl::kSignalingStable);
  ASSERT_TRUE(a2_->pObserver->state == TestObserver::stateError);
}

TEST_F(SignalingTest, BigOValuesTooBig)
{
  EnsureInit();

  std::string offer;

  CreateSDPForBigOTests(offer, "18446744073709551615");

  
  
  a2_->SetRemote(TestObserver::OFFER, offer, true,
                sipcc::PeerConnectionImpl::kSignalingStable);
  ASSERT_TRUE(a2_->pObserver->state == TestObserver::stateError);
}

TEST_F(SignalingTest, SetLocalAnswerInStable)
{
  EnsureInit();

  sipcc::MediaConstraints constraints;
  CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);

  
  
  a1_->SetLocal(TestObserver::ANSWER, a1_->offer(), true,
               sipcc::PeerConnectionImpl::kSignalingStable);
  ASSERT_EQ(a1_->pObserver->lastStatusCode,
            sipcc::PeerConnectionImpl::kInvalidState);
}

TEST_F(SignalingTest, SetRemoteAnswerInStable) {
  EnsureInit();

  
  
  a1_->SetRemote(TestObserver::ANSWER, strSampleSdpAudioVideoNoIce, true,
                sipcc::PeerConnectionImpl::kSignalingStable);
  ASSERT_EQ(a1_->pObserver->lastStatusCode,
            sipcc::PeerConnectionImpl::kInvalidState);
}

TEST_F(SignalingTest, SetLocalAnswerInHaveLocalOffer) {
  sipcc::MediaConstraints constraints;
  CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);
  a1_->SetLocal(TestObserver::OFFER, a1_->offer());
  ASSERT_EQ(a1_->pObserver->lastStatusCode,
            sipcc::PeerConnectionImpl::kNoError);

  
  
  a1_->SetLocal(TestObserver::ANSWER, a1_->offer(), true,
               sipcc::PeerConnectionImpl::kSignalingHaveLocalOffer);
  ASSERT_EQ(a1_->pObserver->lastStatusCode,
            sipcc::PeerConnectionImpl::kInvalidState);
}

TEST_F(SignalingTest, SetRemoteOfferInHaveLocalOffer) {
  sipcc::MediaConstraints constraints;
  CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);
  a1_->SetLocal(TestObserver::OFFER, a1_->offer());
  ASSERT_EQ(a1_->pObserver->lastStatusCode,
            sipcc::PeerConnectionImpl::kNoError);

  
  
  a1_->SetRemote(TestObserver::OFFER, a1_->offer(), true,
                sipcc::PeerConnectionImpl::kSignalingHaveLocalOffer);
  ASSERT_EQ(a1_->pObserver->lastStatusCode,
            sipcc::PeerConnectionImpl::kInvalidState);
}

TEST_F(SignalingTest, SetLocalOfferInHaveRemoteOffer) {
  sipcc::MediaConstraints constraints;
  CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);
  a2_->SetRemote(TestObserver::OFFER, a1_->offer());
  ASSERT_EQ(a2_->pObserver->lastStatusCode,
            sipcc::PeerConnectionImpl::kNoError);

  
  
  a2_->SetLocal(TestObserver::OFFER, a1_->offer(), true,
               sipcc::PeerConnectionImpl::kSignalingHaveRemoteOffer);
  ASSERT_EQ(a2_->pObserver->lastStatusCode,
            sipcc::PeerConnectionImpl::kInvalidState);
}

TEST_F(SignalingTest, SetRemoteAnswerInHaveRemoteOffer) {
  sipcc::MediaConstraints constraints;
  CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);
  a2_->SetRemote(TestObserver::OFFER, a1_->offer());
  ASSERT_EQ(a2_->pObserver->lastStatusCode,
            sipcc::PeerConnectionImpl::kNoError);

  
  
  a2_->SetRemote(TestObserver::ANSWER, a1_->offer(), true,
               sipcc::PeerConnectionImpl::kSignalingHaveRemoteOffer);
  ASSERT_EQ(a2_->pObserver->lastStatusCode,
            sipcc::PeerConnectionImpl::kInvalidState);
}


TEST_F(SignalingTest, DISABLED_AddStreamInHaveLocalOffer) {
  sipcc::MediaConstraints constraints;
  CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);
  a1_->SetLocal(TestObserver::OFFER, a1_->offer());
  ASSERT_EQ(a1_->pObserver->lastStatusCode,
            sipcc::PeerConnectionImpl::kNoError);
  a1_->AddStream();
  ASSERT_EQ(a1_->pObserver->lastStatusCode,
            sipcc::PeerConnectionImpl::kInvalidState);
}


TEST_F(SignalingTest, DISABLED_RemoveStreamInHaveLocalOffer) {
  sipcc::MediaConstraints constraints;
  CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);
  a1_->SetLocal(TestObserver::OFFER, a1_->offer());
  ASSERT_EQ(a1_->pObserver->lastStatusCode,
            sipcc::PeerConnectionImpl::kNoError);
  a1_->RemoveLastStreamAdded();
  ASSERT_EQ(a1_->pObserver->lastStatusCode,
            sipcc::PeerConnectionImpl::kInvalidState);
}

TEST_F(SignalingTest, AddCandidateInHaveLocalOffer) {
  sipcc::MediaConstraints constraints;
  CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);
  a1_->SetLocal(TestObserver::OFFER, a1_->offer());
  ASSERT_EQ(a1_->pObserver->lastStatusCode,
            sipcc::PeerConnectionImpl::kNoError);
  a1_->AddIceCandidate(strSampleCandidate.c_str(),
                      strSampleMid.c_str(), nSamplelevel, false);
  ASSERT_EQ(a1_->pObserver->lastStatusCode,
            sipcc::PeerConnectionImpl::kInvalidState);
}

TEST_F(SignalingAgentTest, CreateOffer) {
  CreateAgent();
  sipcc::MediaConstraints constraints;
  agent(0)->CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);
  PR_Sleep(20000);
}

TEST_F(SignalingAgentTest, CreateOfferTrickleTestServer) {
  TestStunServer::GetInstance()->SetActive(false);
  TestStunServer::GetInstance()->SetResponseAddr(
      kBogusSrflxAddress, kBogusSrflxPort);

  CreateAgent(
      TestStunServer::GetInstance()->addr(),
      TestStunServer::GetInstance()->port(),
      false);

  sipcc::MediaConstraints constraints;
  agent(0)->CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);

  
  ASSERT_FALSE(agent(0)->OfferContains(kBogusSrflxAddress));

  
  TestStunServer::GetInstance()->SetActive(true);
  agent(0)->WaitForGather();

  
  ASSERT_EQ(0U, agent(0)->MatchingCandidates(kBogusSrflxAddress));

  
  size_t match;
  match = agent(0)->getLocalDescription().find(kBogusSrflxAddress);
  ASSERT_LT(0U, match);
}

TEST_F(SignalingAgentTest, CreateOfferSetLocalTrickleTestServer) {
  TestStunServer::GetInstance()->SetActive(false);
  TestStunServer::GetInstance()->SetResponseAddr(
      kBogusSrflxAddress, kBogusSrflxPort);

  CreateAgent(
      TestStunServer::GetInstance()->addr(),
      TestStunServer::GetInstance()->port(),
      false);

  sipcc::MediaConstraints constraints;
  agent(0)->CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);

  
  ASSERT_FALSE(agent(0)->OfferContains(kBogusSrflxAddress));

  
  TestStunServer::GetInstance()->SetActive(true);
  agent(0)->WaitForGather();

  
  ASSERT_EQ(0U, agent(0)->MatchingCandidates(kBogusSrflxAddress));

  agent(0)->SetLocal(TestObserver::OFFER, agent(0)->offer());
  PR_Sleep(1000); 

  
  ASSERT_LE(2U, agent(0)->MatchingCandidates(kBogusSrflxAddress));

  
  size_t match;
  match = agent(0)->getLocalDescription().find(kBogusSrflxAddress);
  ASSERT_LT(0U, match);
}



TEST_F(SignalingAgentTest, CreateUntilFailThenWait) {
  int i;

  for (i=0; ; i++) {
    if (!CreateAgent())
      break;
    std::cerr << "Created agent " << i << std::endl;
  }
  std::cerr << "Failed after creating " << i << " PCs " << std::endl;
  PR_Sleep(10000);  
}


TEST_F(SignalingAgentTest, CreateNoInit) {
  CreateAgentNoInit();
}





TEST_F(SignalingTest, missingUfrag)
{
  EnsureInit();

  sipcc::MediaConstraints constraints;
  std::string offer =
    "v=0\r\n"
    "o=Mozilla-SIPUA 2208 0 IN IP4 0.0.0.0\r\n"
    "s=SIP Call\r\n"
    "t=0 0\r\n"
    "a=ice-pwd:4450d5a4a5f097855c16fa079893be18\r\n"
    "a=fingerprint:sha-256 23:9A:2E:43:94:42:CF:46:68:FC:62:F9:F4:48:61:DB:"
      "2F:8C:C9:FF:6B:25:54:9D:41:09:EF:83:A8:19:FC:B6\r\n"
    "m=audio 56187 RTP/SAVPF 109 0 8 101\r\n"
    "c=IN IP4 77.9.79.167\r\n"
    "a=rtpmap:109 opus/48000/2\r\n"
    "a=ptime:20\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "a=rtpmap:8 PCMA/8000\r\n"
    "a=rtpmap:101 telephone-event/8000\r\n"
    "a=fmtp:101 0-15\r\n"
    "a=sendrecv\r\n"
    "a=candidate:0 1 UDP 2113601791 192.168.178.20 56187 typ host\r\n"
    "a=candidate:1 1 UDP 1694236671 77.9.79.167 56187 typ srflx raddr "
      "192.168.178.20 rport 56187\r\n"
    "a=candidate:0 2 UDP 2113601790 192.168.178.20 52955 typ host\r\n"
    "a=candidate:1 2 UDP 1694236670 77.9.79.167 52955 typ srflx raddr "
      "192.168.178.20 rport 52955\r\n"
    "m=video 49929 RTP/SAVPF 120\r\n"
    "c=IN IP4 77.9.79.167\r\n"
    "a=rtpmap:120 VP8/90000\r\n"
    "a=recvonly\r\n"
    "a=candidate:0 1 UDP 2113601791 192.168.178.20 49929 typ host\r\n"
    "a=candidate:1 1 UDP 1694236671 77.9.79.167 49929 typ srflx raddr "
      "192.168.178.20 rport 49929\r\n"
    "a=candidate:0 2 UDP 2113601790 192.168.178.20 50769 typ host\r\n"
    "a=candidate:1 2 UDP 1694236670 77.9.79.167 50769 typ srflx raddr "
      "192.168.178.20 rport 50769\r\n"
    "m=application 54054 DTLS/SCTP 5000 \r\n"
    "c=IN IP4 77.9.79.167\r\n"
    "a=fmtp:HuRUu]Dtcl\\zM,7(OmEU%O$gU]x/z\tD protocol=webrtc-datachannel;"
      "streams=16\r\n"
    "a=sendrecv\r\n";

  
  
  a1_->CreateOffer(constraints, OFFER_AV, SHOULD_SENDRECV_AV);
  a1_->SetLocal(TestObserver::OFFER, offer, true);
  
  a2_->SetRemote(TestObserver::OFFER, offer, true,
    sipcc::PeerConnectionImpl::kSignalingStable);
  ASSERT_TRUE(a2_->pObserver->state == TestObserver::stateError);
}

TEST_F(SignalingTest, AudioOnlyCalleeNoRtcpMux)
{
  EnsureInit();

  sipcc::MediaConstraints constraints;

  a1_->CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);
  a1_->SetLocal(TestObserver::OFFER, a1_->offer(), false);
  ParsedSDP sdpWrapper(a1_->offer());
  sdpWrapper.DeleteLine("a=rtcp-mux");
  std::cout << "Modified SDP " << std::endl
            << indent(sdpWrapper.getSdp()) << std::endl;
  a2_->SetRemote(TestObserver::OFFER, sdpWrapper.getSdp(), false);
  a2_->CreateAnswer(constraints, sdpWrapper.getSdp(),
    OFFER_AUDIO | ANSWER_AUDIO);
  a2_->SetLocal(TestObserver::ANSWER, a2_->answer(), false);
  a1_->SetRemote(TestObserver::ANSWER, a2_->answer(), false);

  
  ASSERT_EQ(a2_->getLocalDescription().find("\r\na=rtcp-mux"),
            std::string::npos);

  ASSERT_TRUE_WAIT(a1_->IceCompleted() == true, kDefaultTimeout);
  ASSERT_TRUE_WAIT(a2_->IceCompleted() == true, kDefaultTimeout);

  
  ASSERT_TRUE_WAIT(a1_->GetPacketsSent(0) >= 40 &&
                   a2_->GetPacketsReceived(0) >= 40, kDefaultTimeout * 2);

  a1_->CloseSendStreams();
  a2_->CloseReceiveStreams();

  ASSERT_GE(a1_->GetPacketsSent(0), 40);
  ASSERT_GE(a2_->GetPacketsReceived(0), 40);

  
  
  
  a1_->CheckMediaPipeline(0, 0, PIPELINE_LOCAL | PIPELINE_SEND);

  
  a2_->CheckMediaPipeline(0, 1, 0);
}

TEST_F(SignalingTest, FullCallAudioNoMuxVideoMux)
{
  EnsureInit();

  sipcc::MediaConstraints constraints;

  a1_->CreateOffer(constraints, OFFER_AV, SHOULD_SENDRECV_AV);
  a1_->SetLocal(TestObserver::OFFER, a1_->offer(), false);
  ParsedSDP sdpWrapper(a1_->offer());
  sdpWrapper.DeleteLine("a=rtcp-mux");
  std::cout << "Modified SDP " << std::endl
            << indent(sdpWrapper.getSdp()) << std::endl;
  a2_->SetRemote(TestObserver::OFFER, sdpWrapper.getSdp(), false);
  a2_->CreateAnswer(constraints, sdpWrapper.getSdp(), OFFER_AV | ANSWER_AV);
  a2_->SetLocal(TestObserver::ANSWER, a2_->answer(), false);
  a1_->SetRemote(TestObserver::ANSWER, a2_->answer(), false);

  
  size_t match = a2_->getLocalDescription().find("\r\na=rtcp-mux");
  if (fRtcpMux) {
    ASSERT_NE(match, std::string::npos);
    match = a2_->getLocalDescription().find("\r\na=rtcp-mux", match + 1);
  }
  ASSERT_EQ(match, std::string::npos);

  ASSERT_TRUE_WAIT(a1_->IceCompleted() == true, kDefaultTimeout);
  ASSERT_TRUE_WAIT(a2_->IceCompleted() == true, kDefaultTimeout);

  
  ASSERT_TRUE_WAIT(a1_->GetPacketsSent(0) >= 40 &&
                   a2_->GetPacketsReceived(0) >= 40, kDefaultTimeout * 2);

  a1_->CloseSendStreams();
  a2_->CloseReceiveStreams();

  ASSERT_GE(a1_->GetPacketsSent(0), 40);
  ASSERT_GE(a2_->GetPacketsReceived(0), 40);

  
  
  
  a1_->CheckMediaPipeline(0, 0, PIPELINE_LOCAL | PIPELINE_SEND);

  
  a1_->CheckMediaPipeline(0, 1,
    PIPELINE_LOCAL | (fRtcpMux ? PIPELINE_RTCP_MUX : 0) | PIPELINE_SEND |
    PIPELINE_VIDEO);

  
  a2_->CheckMediaPipeline(0, 1, 0);

  
  a2_->CheckMediaPipeline(0, 2, (fRtcpMux ?  PIPELINE_RTCP_MUX : 0) |
    PIPELINE_VIDEO | PIPELINE_RTCP_NACK, VideoSessionConduit::FrameRequestPli);
}

TEST_F(SignalingTest, RtcpFbInOffer)
{
  EnsureInit();
  sipcc::MediaConstraints constraints;
  a1_->CreateOffer(constraints, OFFER_AV, SHOULD_SENDRECV_AV);
  const char *expected[] = { "nack", "nack pli", "ccm fir" };
  CheckRtcpFbSdp(a1_->offer(), ARRAY_TO_SET(std::string, expected));
}

TEST_F(SignalingTest, RtcpFbInAnswer)
{
  const char *feedbackTypes[] = { "nack", "nack pli", "ccm fir" };
  TestRtcpFb(ARRAY_TO_SET(std::string, feedbackTypes),
             PIPELINE_RTCP_NACK,
             VideoSessionConduit::FrameRequestPli);
}

TEST_F(SignalingTest, RtcpFbNoNackBasic)
{
  const char *feedbackTypes[] = { "nack pli", "ccm fir" };
  TestRtcpFb(ARRAY_TO_SET(std::string, feedbackTypes),
             0,
             VideoSessionConduit::FrameRequestPli);
}

TEST_F(SignalingTest, RtcpFbNoNackPli)
{
  const char *feedbackTypes[] = { "nack", "ccm fir" };
  TestRtcpFb(ARRAY_TO_SET(std::string, feedbackTypes),
             PIPELINE_RTCP_NACK,
             VideoSessionConduit::FrameRequestFir);
}

TEST_F(SignalingTest, RtcpFbNoCcmFir)
{
  const char *feedbackTypes[] = { "nack", "nack pli" };
  TestRtcpFb(ARRAY_TO_SET(std::string, feedbackTypes),
             PIPELINE_RTCP_NACK,
             VideoSessionConduit::FrameRequestPli);
}

TEST_F(SignalingTest, RtcpFbNoNack)
{
  const char *feedbackTypes[] = { "ccm fir" };
  TestRtcpFb(ARRAY_TO_SET(std::string, feedbackTypes),
             0,
             VideoSessionConduit::FrameRequestFir);
}

TEST_F(SignalingTest, RtcpFbNoFrameRequest)
{
  const char *feedbackTypes[] = { "nack" };
  TestRtcpFb(ARRAY_TO_SET(std::string, feedbackTypes),
             PIPELINE_RTCP_NACK,
             VideoSessionConduit::FrameRequestNone);
}

TEST_F(SignalingTest, RtcpFbPliOnly)
{
  const char *feedbackTypes[] = { "nack pli" };
  TestRtcpFb(ARRAY_TO_SET(std::string, feedbackTypes),
             0,
             VideoSessionConduit::FrameRequestPli);
}

TEST_F(SignalingTest, RtcpFbNoFeedback)
{
  const char *feedbackTypes[] = { };
  TestRtcpFb(ARRAY_TO_SET(std::string, feedbackTypes),
             0,
             VideoSessionConduit::FrameRequestNone);
}



TEST_F(SignalingTest, AudioCallForceDtlsRoles)
{
  EnsureInit();

  sipcc::MediaConstraints constraints;
  size_t match;

  a1_->CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);

  
  std::string offer(a1_->offer());
  match = offer.find("\r\na=setup:actpass");
  ASSERT_NE(match, std::string::npos);
  
  
  offer.replace(match, strlen("\r\na=setup:actpass"),
    "\r\na=setup:passive");
  std::cout << "Modified SDP " << std::endl
            << indent(offer) << std::endl;

  a1_->SetLocal(TestObserver::OFFER, offer.c_str(), false);
  a2_->SetRemote(TestObserver::OFFER, offer.c_str(), false);
  a2_->CreateAnswer(constraints, offer.c_str(), OFFER_AUDIO | ANSWER_AUDIO);

  
  std::string answer(a2_->answer());
  match = answer.find("\r\na=setup:active");
  ASSERT_NE(match, std::string::npos);

  
  
  a2_->SetLocal(TestObserver::ANSWER, a2_->answer(), false);
  a1_->SetRemote(TestObserver::ANSWER, a2_->answer(), false);

  ASSERT_TRUE_WAIT(a1_->IceCompleted() == true, kDefaultTimeout);
  ASSERT_TRUE_WAIT(a2_->IceCompleted() == true, kDefaultTimeout);

  
  ASSERT_TRUE_WAIT(a1_->GetPacketsSent(0) >= 40 &&
                   a2_->GetPacketsReceived(0) >= 40, kDefaultTimeout * 2);

  a1_->CloseSendStreams();
  a2_->CloseReceiveStreams();

  ASSERT_GE(a1_->GetPacketsSent(0), 40);
  ASSERT_GE(a2_->GetPacketsReceived(0), 40);
}



TEST_F(SignalingTest, AudioCallReverseDtlsRoles)
{
  EnsureInit();

  sipcc::MediaConstraints constraints;
  size_t match;

  a1_->CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);

  
  std::string offer(a1_->offer());
  match = offer.find("\r\na=setup:actpass");
  ASSERT_NE(match, std::string::npos);
  
  
  offer.replace(match, strlen("\r\na=setup:actpass"),
    "\r\na=setup:active");
  std::cout << "Modified SDP " << std::endl
            << indent(offer) << std::endl;

  a1_->SetLocal(TestObserver::OFFER, offer.c_str(), false);
  a2_->SetRemote(TestObserver::OFFER, offer.c_str(), false);
  a2_->CreateAnswer(constraints, offer.c_str(), OFFER_AUDIO | ANSWER_AUDIO);

  
  std::string answer(a2_->answer());
  match = answer.find("\r\na=setup:passive");
  ASSERT_NE(match, std::string::npos);

  
  
  a2_->SetLocal(TestObserver::ANSWER, a2_->answer(), false);
  a1_->SetRemote(TestObserver::ANSWER, a2_->answer(), false);

  ASSERT_TRUE_WAIT(a1_->IceCompleted() == true, kDefaultTimeout);
  ASSERT_TRUE_WAIT(a2_->IceCompleted() == true, kDefaultTimeout);

  
  ASSERT_TRUE_WAIT(a1_->GetPacketsSent(0) >= 40 &&
                   a2_->GetPacketsReceived(0) >= 40, kDefaultTimeout * 2);

  a1_->CloseSendStreams();
  a2_->CloseReceiveStreams();

  ASSERT_GE(a1_->GetPacketsSent(0), 40);
  ASSERT_GE(a2_->GetPacketsReceived(0), 40);
}




TEST_F(SignalingTest, AudioCallMismatchDtlsRoles)
{
  EnsureInit();

  sipcc::MediaConstraints constraints;
  size_t match;

  a1_->CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);

  
  std::string offer(a1_->offer());
  match = offer.find("\r\na=setup:actpass");
  ASSERT_NE(match, std::string::npos);
  a1_->SetLocal(TestObserver::OFFER, offer.c_str(), false);
  a2_->SetRemote(TestObserver::OFFER, offer.c_str(), false);
  a2_->CreateAnswer(constraints, offer.c_str(), OFFER_AUDIO | ANSWER_AUDIO);

  
  std::string answer(a2_->answer());
  match = answer.find("\r\na=setup:active");
  ASSERT_NE(match, std::string::npos);

  
  
  answer.replace(match, strlen("\r\na=setup:active"),
    "\r\na=setup:passive");
  std::cout << "Modified SDP " << std::endl
            << indent(answer) << std::endl;

  
  a2_->SetLocal(TestObserver::ANSWER, answer.c_str(), false);
  a1_->SetRemote(TestObserver::ANSWER, answer.c_str(), false);

  ASSERT_TRUE_WAIT(a1_->IceCompleted() == true, kDefaultTimeout);
  ASSERT_TRUE_WAIT(a2_->IceCompleted() == true, kDefaultTimeout);

  
  PR_Sleep(kDefaultTimeout * 2); 

  a1_->CloseSendStreams();
  a2_->CloseReceiveStreams();

  ASSERT_GE(a1_->GetPacketsSent(0), 40);
  
  ASSERT_EQ(a2_->GetPacketsReceived(0), 0);
}




TEST_F(SignalingTest, AudioCallGarbageSetup)
{
  EnsureInit();

  sipcc::MediaConstraints constraints;
  size_t match;

  a1_->CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);

  
  std::string offer(a1_->offer());
  match = offer.find("\r\na=setup:actpass");
  ASSERT_NE(match, std::string::npos);
  
  offer.replace(match, strlen("\r\na=setup:actpass"),
    "\r\na=setup:G4rb4g3V4lu3");
  std::cout << "Modified SDP " << std::endl
            << indent(offer) << std::endl;

  a1_->SetLocal(TestObserver::OFFER, offer.c_str(), false);
  a2_->SetRemote(TestObserver::OFFER, offer.c_str(), false);
  a2_->CreateAnswer(constraints, offer.c_str(), OFFER_AUDIO | ANSWER_AUDIO);

  
  std::string answer(a2_->answer());
  match = answer.find("\r\na=setup:active");
  ASSERT_NE(match, std::string::npos);

  
  
  a2_->SetLocal(TestObserver::ANSWER, a2_->answer(), false);
  a1_->SetRemote(TestObserver::ANSWER, a2_->answer(), false);

  ASSERT_TRUE_WAIT(a1_->IceCompleted() == true, kDefaultTimeout);
  ASSERT_TRUE_WAIT(a2_->IceCompleted() == true, kDefaultTimeout);

  
  ASSERT_TRUE_WAIT(a1_->GetPacketsSent(0) >= 40 &&
                   a2_->GetPacketsReceived(0) >= 40, kDefaultTimeout * 2);

  a1_->CloseSendStreams();
  a2_->CloseReceiveStreams();

  ASSERT_GE(a1_->GetPacketsSent(0), 40);
  ASSERT_GE(a2_->GetPacketsReceived(0), 40);
}



TEST_F(SignalingTest, AudioCallOfferNoSetupOrConnection)
{
  EnsureInit();

  sipcc::MediaConstraints constraints;
  size_t match;

  a1_->CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);

  
  std::string offer(a1_->offer());
  match = offer.find("\r\na=setup:actpass");
  ASSERT_NE(match, std::string::npos);
  
  offer.replace(match, strlen("\r\na=setup:actpass"), "");
  std::cout << "Modified SDP " << std::endl
            << indent(offer) << std::endl;

  a1_->SetLocal(TestObserver::OFFER, offer.c_str(), false);
  a2_->SetRemote(TestObserver::OFFER, offer.c_str(), false);
  a2_->CreateAnswer(constraints, offer.c_str(), OFFER_AUDIO | ANSWER_AUDIO);

  
  std::string answer(a2_->answer());
  match = answer.find("\r\na=setup:active");
  ASSERT_NE(match, std::string::npos);

  
  
  a2_->SetLocal(TestObserver::ANSWER, a2_->answer(), false);
  a1_->SetRemote(TestObserver::ANSWER, a2_->answer(), false);

  ASSERT_TRUE_WAIT(a1_->IceCompleted() == true, kDefaultTimeout);
  ASSERT_TRUE_WAIT(a2_->IceCompleted() == true, kDefaultTimeout);

  
  ASSERT_TRUE_WAIT(a1_->GetPacketsSent(0) >= 40 &&
                   a2_->GetPacketsReceived(0) >= 40, kDefaultTimeout * 2);

  a1_->CloseSendStreams();
  a2_->CloseReceiveStreams();

  ASSERT_GE(a1_->GetPacketsSent(0), 40);
  ASSERT_GE(a2_->GetPacketsReceived(0), 40);
}




TEST_F(SignalingTest, AudioCallAnswerNoSetupOrConnection)
{
  EnsureInit();

  sipcc::MediaConstraints constraints;
  size_t match;

  a1_->CreateOffer(constraints, OFFER_AUDIO, SHOULD_SENDRECV_AUDIO);

  
  std::string offer(a1_->offer());
  match = offer.find("\r\na=setup:actpass");
  ASSERT_NE(match, std::string::npos);

  a1_->SetLocal(TestObserver::OFFER, offer.c_str(), false);
  a2_->SetRemote(TestObserver::OFFER, offer.c_str(), false);
  a2_->CreateAnswer(constraints, offer.c_str(), OFFER_AUDIO | ANSWER_AUDIO);

  
  std::string answer(a2_->answer());
  match = answer.find("\r\na=setup:active");
  ASSERT_NE(match, std::string::npos);
  
  answer.replace(match, strlen("\r\na=setup:active"), "");
  std::cout << "Modified SDP " << std::endl
            << indent(answer) << std::endl;

  
  
  a2_->SetLocal(TestObserver::ANSWER, answer, false);
  a1_->SetRemote(TestObserver::ANSWER, answer, false);

  ASSERT_TRUE_WAIT(a1_->IceCompleted() == true, kDefaultTimeout);
  ASSERT_TRUE_WAIT(a2_->IceCompleted() == true, kDefaultTimeout);

  
  ASSERT_TRUE_WAIT(a1_->GetPacketsSent(0) >= 40 &&
                   a2_->GetPacketsReceived(0) >= 40, kDefaultTimeout * 2);

  a1_->CloseSendStreams();
  a2_->CloseReceiveStreams();

  ASSERT_GE(a1_->GetPacketsSent(0), 40);
  ASSERT_GE(a2_->GetPacketsReceived(0), 40);
}


TEST_F(SignalingTest, FullCallRealTrickle)
{
  wait_for_gather_ = false;

  sipcc::MediaConstraints constraints;
  OfferAnswer(constraints, constraints, OFFER_AV | ANSWER_AV,
              true, SHOULD_SENDRECV_AV, SHOULD_SENDRECV_AV);

  
  ASSERT_TRUE_WAIT(a1_->GetPacketsSent(0) >= 40 &&
                   a2_->GetPacketsReceived(0) >= 40, kDefaultTimeout * 2);

  a1_->CloseSendStreams();
  a2_->CloseReceiveStreams();
  ASSERT_GE(a1_->GetPacketsSent(0), 40);
  ASSERT_GE(a2_->GetPacketsReceived(0), 40);
}

TEST_F(SignalingTest, FullCallRealTrickleTestServer)
{
  wait_for_gather_ = false;
  SetTestStunServer();

  sipcc::MediaConstraints constraints;
  OfferAnswer(constraints, constraints, OFFER_AV | ANSWER_AV,
              true, SHOULD_SENDRECV_AV, SHOULD_SENDRECV_AV);

  TestStunServer::GetInstance()->SetActive(true);

  
  ASSERT_TRUE_WAIT(a1_->GetPacketsSent(0) >= 40 &&
                   a2_->GetPacketsReceived(0) >= 40, kDefaultTimeout * 2);

  a1_->CloseSendStreams();
  a2_->CloseReceiveStreams();
  ASSERT_GE(a1_->GetPacketsSent(0), 40);
  ASSERT_GE(a2_->GetPacketsReceived(0), 40);
}

TEST_F(SignalingTest, hugeSdp)
{
  EnsureInit();

  sipcc::MediaConstraints constraints;
  std::string offer =
    "v=0\r\n"
    "o=- 1109973417102828257 2 IN IP4 127.0.0.1\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=group:BUNDLE audio video\r\n"
    "a=msid-semantic: WMS 1PBxet5BYh0oYodwsvNM4k6KiO2eWCX40VIP\r\n"
    "m=audio 32952 RTP/SAVPF 111 103 104 0 8 107 106 105 13 126\r\n"
    "c=IN IP4 128.64.32.16\r\n"
    "a=rtcp:32952 IN IP4 128.64.32.16\r\n"
    "a=candidate:77142221 1 udp 2113937151 192.168.137.1 54081 typ host generation 0\r\n"
    "a=candidate:77142221 2 udp 2113937151 192.168.137.1 54081 typ host generation 0\r\n"
    "a=candidate:983072742 1 udp 2113937151 172.22.0.56 54082 typ host generation 0\r\n"
    "a=candidate:983072742 2 udp 2113937151 172.22.0.56 54082 typ host generation 0\r\n"
    "a=candidate:2245074553 1 udp 1845501695 32.64.128.1 62397 typ srflx raddr 192.168.137.1 rport 54081 generation 0\r\n"
    "a=candidate:2245074553 2 udp 1845501695 32.64.128.1 62397 typ srflx raddr 192.168.137.1 rport 54081 generation 0\r\n"
    "a=candidate:2479353907 1 udp 1845501695 32.64.128.1 54082 typ srflx raddr 172.22.0.56 rport 54082 generation 0\r\n"
    "a=candidate:2479353907 2 udp 1845501695 32.64.128.1 54082 typ srflx raddr 172.22.0.56 rport 54082 generation 0\r\n"
    "a=candidate:1243276349 1 tcp 1509957375 192.168.137.1 0 typ host generation 0\r\n"
    "a=candidate:1243276349 2 tcp 1509957375 192.168.137.1 0 typ host generation 0\r\n"
    "a=candidate:1947960086 1 tcp 1509957375 172.22.0.56 0 typ host generation 0\r\n"
    "a=candidate:1947960086 2 tcp 1509957375 172.22.0.56 0 typ host generation 0\r\n"
    "a=candidate:1808221584 1 udp 33562367 128.64.32.16 32952 typ relay raddr 32.64.128.1 rport 62398 generation 0\r\n"
    "a=candidate:1808221584 2 udp 33562367 128.64.32.16 32952 typ relay raddr 32.64.128.1 rport 62398 generation 0\r\n"
    "a=candidate:507872740 1 udp 33562367 128.64.32.16 40975 typ relay raddr 32.64.128.1 rport 54085 generation 0\r\n"
    "a=candidate:507872740 2 udp 33562367 128.64.32.16 40975 typ relay raddr 32.64.128.1 rport 54085 generation 0\r\n"
    "a=ice-ufrag:xQuJwjX3V3eMA81k\r\n"
    "a=ice-pwd:ZUiRmjS2GDhG140p73dAsSVP\r\n"
    "a=ice-options:google-ice\r\n"
    "a=fingerprint:sha-256 59:4A:8B:73:A7:73:53:71:88:D7:4D:58:28:0C:79:72:31:29:9B:05:37:DD:58:43:C2:D4:85:A2:B3:66:38:7A\r\n"
    "a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level\r\n"
    "a=sendrecv\r\n"
    "a=mid:audio\r\n"
    "a=rtcp-mux\r\n"
    "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:/U44g3ULdtapeiSg+T3n6dDLBKIjpOhb/NXAL/2b\r\n"
    "a=rtpmap:111 opus/48000/2\r\n"
    "a=fmtp:111 minptime=10\r\n"
    "a=rtpmap:103 ISAC/16000\r\n"
    "a=rtpmap:104 ISAC/32000\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "a=rtpmap:8 PCMA/8000\r\n"
    "a=rtpmap:107 CN/48000\r\n"
    "a=rtpmap:106 CN/32000\r\n"
    "a=rtpmap:105 CN/16000\r\n"
    "a=rtpmap:13 CN/8000\r\n"
    "a=rtpmap:126 telephone-event/8000\r\n"
    "a=maxptime:60\r\n"
    "a=ssrc:2271517329 cname:mKDNt7SQf6pwDlIn\r\n"
    "a=ssrc:2271517329 msid:1PBxet5BYh0oYodwsvNM4k6KiO2eWCX40VIP 1PBxet5BYh0oYodwsvNM4k6KiO2eWCX40VIPa0\r\n"
    "a=ssrc:2271517329 mslabel:1PBxet5BYh0oYodwsvNM4k6KiO2eWCX40VIP\r\n"
    "a=ssrc:2271517329 label:1PBxet5BYh0oYodwsvNM4k6KiO2eWCX40VIPa0\r\n"
    "m=video 32952 RTP/SAVPF 100 116 117\r\n"
    "c=IN IP4 128.64.32.16\r\n"
    "a=rtcp:32952 IN IP4 128.64.32.16\r\n"
    "a=candidate:77142221 1 udp 2113937151 192.168.137.1 54081 typ host generation 0\r\n"
    "a=candidate:77142221 2 udp 2113937151 192.168.137.1 54081 typ host generation 0\r\n"
    "a=candidate:983072742 1 udp 2113937151 172.22.0.56 54082 typ host generation 0\r\n"
    "a=candidate:983072742 2 udp 2113937151 172.22.0.56 54082 typ host generation 0\r\n"
    "a=candidate:2245074553 1 udp 1845501695 32.64.128.1 62397 typ srflx raddr 192.168.137.1 rport 54081 generation 0\r\n"
    "a=candidate:2245074553 2 udp 1845501695 32.64.128.1 62397 typ srflx raddr 192.168.137.1 rport 54081 generation 0\r\n"
    "a=candidate:2479353907 1 udp 1845501695 32.64.128.1 54082 typ srflx raddr 172.22.0.56 rport 54082 generation 0\r\n"
    "a=candidate:2479353907 2 udp 1845501695 32.64.128.1 54082 typ srflx raddr 172.22.0.56 rport 54082 generation 0\r\n"
    "a=candidate:1243276349 1 tcp 1509957375 192.168.137.1 0 typ host generation 0\r\n"
    "a=candidate:1243276349 2 tcp 1509957375 192.168.137.1 0 typ host generation 0\r\n"
    "a=candidate:1947960086 1 tcp 1509957375 172.22.0.56 0 typ host generation 0\r\n"
    "a=candidate:1947960086 2 tcp 1509957375 172.22.0.56 0 typ host generation 0\r\n"
    "a=candidate:1808221584 1 udp 33562367 128.64.32.16 32952 typ relay raddr 32.64.128.1 rport 62398 generation 0\r\n"
    "a=candidate:1808221584 2 udp 33562367 128.64.32.16 32952 typ relay raddr 32.64.128.1 rport 62398 generation 0\r\n"
    "a=candidate:507872740 1 udp 33562367 128.64.32.16 40975 typ relay raddr 32.64.128.1 rport 54085 generation 0\r\n"
    "a=candidate:507872740 2 udp 33562367 128.64.32.16 40975 typ relay raddr 32.64.128.1 rport 54085 generation 0\r\n"
    "a=ice-ufrag:xQuJwjX3V3eMA81k\r\n"
    "a=ice-pwd:ZUiRmjS2GDhG140p73dAsSVP\r\n"
    "a=ice-options:google-ice\r\n"
    "a=fingerprint:sha-256 59:4A:8B:73:A7:73:53:71:88:D7:4D:58:28:0C:79:72:31:29:9B:05:37:DD:58:43:C2:D4:85:A2:B3:66:38:7A\r\n"
    "a=extmap:2 urn:ietf:params:rtp-hdrext:toffset\r\n"
    "a=extmap:3 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\r\n"
    "a=sendrecv\r\n"
    "a=mid:video\r\n"
    "a=rtcp-mux\r\n"
    "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:/U44g3ULdtapeiSg+T3n6dDLBKIjpOhb/NXAL/2b\r\n"
    "a=rtpmap:100 VP8/90000\r\n"
    "a=rtcp-fb:100 ccm fir\r\n"
    "a=rtcp-fb:100 nack\r\n"
    "a=rtcp-fb:100 goog-remb\r\n"
    "a=rtpmap:116 red/90000\r\n"
    "a=rtpmap:117 ulpfec/90000\r\n"
    "a=ssrc:54724160 cname:mKDNt7SQf6pwDlIn\r\n"
    "a=ssrc:54724160 msid:1PBxet5BYh0oYodwsvNM4k6KiO2eWCX40VIP 1PBxet5BYh0oYodwsvNM4k6KiO2eWCX40VIPv0\r\n"
    "a=ssrc:54724160 mslabel:1PBxet5BYh0oYodwsvNM4k6KiO2eWCX40VIP\r\n"
    "a=ssrc:54724160 label:1PBxet5BYh0oYodwsvNM4k6KiO2eWCX40VIPv0\r\n";

  a1_->CreateOffer(constraints, OFFER_AV, SHOULD_SENDRECV_AV);
  a1_->SetLocal(TestObserver::OFFER, offer, true);

  a2_->SetRemote(TestObserver::OFFER, offer, true);
  ASSERT_GE(a2_->getRemoteDescription().length(), 4096U);
  a2_->CreateAnswer(constraints, offer, OFFER_AV);
}

} 

bool is_color_terminal(const char *terminal) {
  if (!terminal) {
    return false;
  }
  const char *color_terms[] = {
    "xterm",
    "xterm-color",
    "xterm-256color",
    "screen",
    "linux",
    "cygwin",
    0
  };
  const char **p = color_terms;
  while (*p) {
    if (!strcmp(terminal, *p)) {
      return true;
    }
    p++;
  }
  return false;
}

static std::string get_environment(const char *name) {
  char *value = getenv(name);

  if (!value)
    return "";

  return value;
}

int main(int argc, char **argv) {

  
  CHECK_ENVIRONMENT_FLAG("MOZ_WEBRTC_TESTS")

  if (isatty(STDOUT_FILENO) && is_color_terminal(getenv("TERM"))) {
    std::string ansiMagenta = "\x1b[35m";
    std::string ansiCyan = "\x1b[36m";
    std::string ansiColorOff = "\x1b[0m";
    callerName = ansiCyan + callerName + ansiColorOff;
    calleeName = ansiMagenta + calleeName + ansiColorOff;
  }

  std::string tmp = get_environment("STUN_SERVER_ADDRESS");
  if (tmp != "")
    g_stun_server_address = tmp;

  tmp = get_environment("STUN_SERVER_PORT");
  if (tmp != "")
      g_stun_server_port = atoi(tmp.c_str());

  test_utils = new MtransportTestUtils();
  NSS_NoDB_Init(NULL);
  NSS_SetDomesticPolicy();

  ::testing::InitGoogleTest(&argc, argv);

  for(int i=0; i<argc; i++) {
    if (!strcmp(argv[i],"-t")) {
      kDefaultTimeout = 20000;
    }
  }

  test_utils->sts_target()->Dispatch(
    WrapRunnableNM(&TestStunServer::GetInstance), NS_DISPATCH_SYNC);

  ::testing::AddGlobalTestEnvironment(new test::SignalingEnvironment);
  int result = RUN_ALL_TESTS();

  test_utils->sts_target()->Dispatch(
    WrapRunnableNM(&TestStunServer::ShutdownInstance), NS_DISPATCH_SYNC);

  
  
  
  sipcc::PeerConnectionCtx::Destroy();
  delete test_utils;

  return result;
}
