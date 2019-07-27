










































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

struct NrIceAddr {
  std::string host;
  uint16_t port;
  std::string transport;
};



struct NrIceCandidate {
  enum Type {
    ICE_HOST,
    ICE_SERVER_REFLEXIVE,
    ICE_PEER_REFLEXIVE,
    ICE_RELAYED
  };

  enum TcpType {
    ICE_NONE,
    ICE_ACTIVE,
    ICE_PASSIVE,
    ICE_SO
  };

  NrIceAddr cand_addr;
  NrIceAddr local_addr;
  Type type;
  TcpType tcp_type;
  std::string codeword;
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
  
  std::string codeword;
};

class NrIceMediaStream {
 public:
  static RefPtr<NrIceMediaStream> Create(NrIceCtx *ctx,
                                         const std::string& name,
                                         int components);
  enum State { ICE_CONNECTING, ICE_OPEN, ICE_CLOSED};

  State state() const { return state_; }

  
  const std::string& name() const { return name_; }

  
  std::vector<std::string> GetCandidates() const;

  nsresult GetLocalCandidates(std::vector<NrIceCandidate>* candidates) const;
  nsresult GetRemoteCandidates(std::vector<NrIceCandidate>* candidates) const;

  
  
  nsresult GetCandidatePairs(std::vector<NrIceCandidatePair>* out_pairs) const;

  nsresult GetDefaultCandidate(int component, NrIceCandidate* candidate) const;

  
  nsresult ParseAttributes(std::vector<std::string>& candidates);
  bool HasParsedAttributes() const { return has_parsed_attrs_; }

  
  nsresult ParseTrickleCandidate(const std::string& candidate);

  
  nsresult DisableComponent(int component);

  
  
  nsresult GetActivePair(int component,
                         NrIceCandidate** local, NrIceCandidate** remote);

  
  size_t components() const { return components_; }

  
  nr_ice_media_stream *stream() { return stream_; }
  
  

  
  nsresult SendPacket(int component_id, const unsigned char *data, size_t len);

  
  void Ready();

  
  
  
  
  void Close();

  
  
  void SetLevel(uint16_t level) { level_ = level; }

  uint16_t GetLevel() const { return level_; }

  sigslot::signal2<NrIceMediaStream *, const std::string& >
  SignalCandidate;  

  sigslot::signal1<NrIceMediaStream *> SignalReady;  
  sigslot::signal1<NrIceMediaStream *> SignalFailed;  
  sigslot::signal4<NrIceMediaStream *, int, const unsigned char *, int>
  SignalPacketReceived;  

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(NrIceMediaStream)

 private:
  NrIceMediaStream(NrIceCtx *ctx,  const std::string& name,
                   size_t components) :
      state_(ICE_CONNECTING),
      ctx_(ctx),
      name_(name),
      components_(components),
      stream_(nullptr),
      level_(0),
      has_parsed_attrs_(false) {}

  ~NrIceMediaStream();

  DISALLOW_COPY_ASSIGN(NrIceMediaStream);

  State state_;
  NrIceCtx *ctx_;
  const std::string name_;
  const size_t components_;
  nr_ice_media_stream *stream_;
  uint16_t level_;
  bool has_parsed_attrs_;
};


}  
#endif
