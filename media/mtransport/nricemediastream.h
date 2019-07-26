










































#ifndef nricemediastream_h__
#define nricemediastream_h__

#include <string>
#include <vector>

#include "sigslot.h"

#include "mozilla/RefPtr.h"
#include "mozilla/Scoped.h"
#include "nsCOMPtr.h"
#include "nsIEventTarget.h"
#include "nsITimer.h"

#include "m_cpp_utils.h"


namespace mozilla {

typedef struct nr_ice_media_stream_ nr_ice_media_stream;

class NrIceCtx;



struct NrIceCandidate {
  enum Type {
    ICE_HOST,
    ICE_SERVER_REFLEXIVE,
    ICE_PEER_REFLEXIVE,
    ICE_RELAYED
  };

  std::string host;
  uint16_t port;
  Type type;
};

struct NrIceCandidatePair {

  enum State {
    STATE_FROZEN,
    STATE_WAITING,
    STATE_IN_PROGRESS,
    STATE_FAILED,
    STATE_SUCCEEDED,
    STATE_CANCELLED
  };

  State state;
  uint64_t priority;
  
  
  
  
  
  bool nominated;
  
  
  bool selected;
  NrIceCandidate local;
  NrIceCandidate remote;
  
};

class NrIceMediaStream {
 public:
  static RefPtr<NrIceMediaStream> Create(NrIceCtx *ctx,
                                         const std::string& name,
                                         int components);
  ~NrIceMediaStream();

  enum State { ICE_CONNECTING, ICE_OPEN, ICE_CLOSED};

  State state() const { return state_; }

  
  const std::string& name() const { return name_; }

  
  std::vector<std::string> GetCandidates() const;

  
  
  nsresult GetCandidatePairs(std::vector<NrIceCandidatePair>* out_pairs) const;

  
  nsresult GetDefaultCandidate(int component, std::string *host, int *port);

  
  nsresult ParseAttributes(std::vector<std::string>& candidates);

  
  nsresult ParseTrickleCandidate(const std::string& candidate);

  
  nsresult DisableComponent(int component);

  
  
  nsresult GetActivePair(int component,
                         NrIceCandidate** local, NrIceCandidate** remote);

  
  int components() const { return components_; }

  
  nr_ice_media_stream *stream() { return stream_; }
  
  

  
  nsresult SendPacket(int component_id, const unsigned char *data, size_t len);

  
  void Ready();

  
  
  
  
  void Close();

  sigslot::signal2<NrIceMediaStream *, const std::string& >
  SignalCandidate;  
  sigslot::signal1<NrIceMediaStream *> SignalReady;  
  sigslot::signal1<NrIceMediaStream *> SignalFailed;  
  sigslot::signal4<NrIceMediaStream *, int, const unsigned char *, int>
  SignalPacketReceived;  

  
  
  void EmitAllCandidates();

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(NrIceMediaStream)

 private:
  NrIceMediaStream(NrIceCtx *ctx,  const std::string& name,
                   int components) :
      state_(ICE_CONNECTING),
      ctx_(ctx),
      name_(name),
      components_(components),
      stream_(nullptr) {}

  DISALLOW_COPY_ASSIGN(NrIceMediaStream);

  State state_;
  NrIceCtx *ctx_;
  const std::string name_;
  const int components_;
  nr_ice_media_stream *stream_;
};


}  
#endif
