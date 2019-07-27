



#ifndef _JSEPSESSION_H_
#define _JSEPSESSION_H_

#include <string>
#include <vector>
#include "mozilla/Maybe.h"
#include "mozilla/RefPtr.h"
#include "mozilla/UniquePtr.h"
#include "nsError.h"

#include "signaling/src/jsep/JsepTransport.h"
#include "signaling/src/sdp/Sdp.h"


namespace mozilla {


struct JsepCodecDescription;
class JsepTrack;
struct JsepTrackPair;

enum JsepSignalingState {
  kJsepStateStable,
  kJsepStateHaveLocalOffer,
  kJsepStateHaveRemoteOffer,
  kJsepStateHaveLocalPranswer,
  kJsepStateHaveRemotePranswer,
  kJsepStateClosed
};

enum JsepSdpType {
  kJsepSdpOffer,
  kJsepSdpAnswer,
  kJsepSdpPranswer,
};

struct JsepOAOptions {};
struct JsepOfferOptions : public JsepOAOptions {
  Maybe<size_t> mOfferToReceiveAudio;
  Maybe<size_t> mOfferToReceiveVideo;
  Maybe<bool> mDontOfferDataChannel;
};
struct JsepAnswerOptions : public JsepOAOptions {};

class JsepSession
{
public:
  explicit JsepSession(const std::string& name)
      : mName(name), mState(kJsepStateStable)
  {
  }
  virtual ~JsepSession() {}

  virtual nsresult Init() = 0;

  
  virtual const std::string&
  GetName() const
  {
    return mName;
  }
  virtual JsepSignalingState
  GetState() const
  {
    return mState;
  }

  
  virtual nsresult SetIceCredentials(const std::string& ufrag,
                                     const std::string& pwd) = 0;
  virtual bool RemoteIsIceLite() const = 0;
  virtual std::vector<std::string> GetIceOptions() const = 0;

  virtual nsresult AddDtlsFingerprint(const std::string& algorithm,
                                      const std::vector<uint8_t>& value) = 0;

  virtual nsresult AddAudioRtpExtension(const std::string& extensionName) = 0;
  virtual nsresult AddVideoRtpExtension(const std::string& extensionName) = 0;

  
  
  
  
  
  
  virtual std::vector<JsepCodecDescription*>& Codecs() = 0;

  
  virtual nsresult AddTrack(const RefPtr<JsepTrack>& track) = 0;
  virtual nsresult RemoveTrack(const std::string& streamId,
                               const std::string& trackId) = 0;
  virtual nsresult ReplaceTrack(size_t track_index,
                                const RefPtr<JsepTrack>& track) = 0;

  virtual std::vector<RefPtr<JsepTrack>> GetLocalTracks() const = 0;

  virtual std::vector<RefPtr<JsepTrack>> GetRemoteTracks() const = 0;

  virtual std::vector<RefPtr<JsepTrack>> GetRemoteTracksAdded() const = 0;

  virtual std::vector<RefPtr<JsepTrack>> GetRemoteTracksRemoved() const = 0;

  
  virtual std::vector<JsepTrackPair> GetNegotiatedTrackPairs() const = 0;

  
  virtual std::vector<RefPtr<JsepTransport>> GetTransports() const = 0;

  
  virtual nsresult CreateOffer(const JsepOfferOptions& options,
                               std::string* offer) = 0;
  virtual nsresult CreateAnswer(const JsepAnswerOptions& options,
                                std::string* answer) = 0;
  virtual std::string GetLocalDescription() const = 0;
  virtual std::string GetRemoteDescription() const = 0;
  virtual nsresult SetLocalDescription(JsepSdpType type,
                                       const std::string& sdp) = 0;
  virtual nsresult SetRemoteDescription(JsepSdpType type,
                                        const std::string& sdp) = 0;
  virtual nsresult AddRemoteIceCandidate(const std::string& candidate,
                                         const std::string& mid,
                                         uint16_t level) = 0;
  virtual nsresult AddLocalIceCandidate(const std::string& candidate,
                                        const std::string& mid,
                                        uint16_t level,
                                        bool* skipped) = 0;
  virtual nsresult EndOfLocalCandidates(const std::string& defaultCandidateAddr,
                                        uint16_t defaultCandidatePort,
                                        uint16_t level) = 0;
  virtual nsresult Close() = 0;

  
  virtual bool IsIceControlling() const = 0;

  virtual const std::string
  GetLastError() const
  {
    return "Error";
  }

  static const char*
  GetStateStr(JsepSignalingState state)
  {
    static const char* states[] = { "stable", "have-local-offer",
                                    "have-remote-offer", "have-local-pranswer",
                                    "have-remote-pranswer", "closed" };

    return states[state];
  }

  virtual bool AllLocalTracksAreAssigned() const = 0;

protected:
  const std::string mName;
  JsepSignalingState mState;
};

} 

#endif
