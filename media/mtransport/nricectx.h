
















































#ifndef nricectx_h__
#define nricectx_h__

#include <string>
#include <vector>

#include "sigslot.h"

#include "prnetdb.h"

#include "mozilla/RefPtr.h"
#include "mozilla/Scoped.h"
#include "mozilla/TimeStamp.h"
#include "nsAutoPtr.h"
#include "nsIEventTarget.h"
#include "nsITimer.h"

#include "m_cpp_utils.h"

typedef struct nr_ice_ctx_ nr_ice_ctx;
typedef struct nr_ice_peer_ctx_ nr_ice_peer_ctx;
typedef struct nr_ice_media_stream_ nr_ice_media_stream;
typedef struct nr_ice_handler_ nr_ice_handler;
typedef struct nr_ice_handler_vtbl_ nr_ice_handler_vtbl;
typedef struct nr_ice_candidate_ nr_ice_candidate;
typedef struct nr_ice_cand_pair_ nr_ice_cand_pair;
typedef struct nr_ice_stun_server_ nr_ice_stun_server;
typedef struct nr_ice_turn_server_ nr_ice_turn_server;
typedef struct nr_resolver_ nr_resolver;

typedef void* NR_SOCKET;

namespace mozilla {



TimeStamp nr_socket_short_term_violation_time();
TimeStamp nr_socket_long_term_violation_time();

class NrIceMediaStream;

extern const char kNrIceTransportUdp[];
extern const char kNrIceTransportTcp[];

class NrIceStunServer {
 public:
  explicit NrIceStunServer(const PRNetAddr& addr) : has_addr_(true) {
    memcpy(&addr_, &addr, sizeof(addr));
  }

   
  static NrIceStunServer* Create(const std::string& addr, uint16_t port) {
    ScopedDeletePtr<NrIceStunServer> server(
        new NrIceStunServer());

    nsresult rv = server->Init(addr, port);
    if (NS_FAILED(rv))
      return nullptr;

    return server.forget();
  }

  nsresult ToNicerStunStruct(nr_ice_stun_server* server,
                             const std::string& transport =
                             kNrIceTransportUdp) const;

 protected:
  NrIceStunServer() : addr_() {}

  nsresult Init(const std::string& addr, uint16_t port) {
    PRStatus status = PR_StringToNetAddr(addr.c_str(), &addr_);
    if (status == PR_SUCCESS) {
      
      addr_.inet.port = PR_htons(port);
      port_ = port;
      has_addr_ = true;
      return NS_OK;
    }
    else if (addr.size() < 256) {
      
      host_ = addr;
      port_ = port;
      has_addr_ = false;
      return NS_OK;
    }

    return NS_ERROR_FAILURE;
  }

  bool has_addr_;
  std::string host_;
  uint16_t port_;
  PRNetAddr addr_;
};

class NrIceTurnServer : public NrIceStunServer {
 public:
  static NrIceTurnServer *Create(const std::string& addr, uint16_t port,
                                 const std::string& username,
                                 const std::vector<unsigned char>& password,
                                 const char *transport = kNrIceTransportUdp) {
    ScopedDeletePtr<NrIceTurnServer> server(
        new NrIceTurnServer(username, password, transport));

    nsresult rv = server->Init(addr, port);
    if (NS_FAILED(rv))
      return nullptr;

    return server.forget();
  }

  nsresult ToNicerTurnStruct(nr_ice_turn_server *server) const;

 private:
  NrIceTurnServer(const std::string& username,
                  const std::vector<unsigned char>& password,
                  const char *transport) :
      username_(username), password_(password), transport_(transport) {}

  std::string username_;
  std::vector<unsigned char> password_;
  std::string transport_;
};

class NrIceCtx {
 public:
  enum ConnectionState { ICE_CTX_INIT,
                         ICE_CTX_CHECKING,
                         ICE_CTX_OPEN,
                         ICE_CTX_FAILED
  };

  enum GatheringState { ICE_CTX_GATHER_INIT,
                        ICE_CTX_GATHER_STARTED,
                        ICE_CTX_GATHER_COMPLETE
  };

  enum Controlling { ICE_CONTROLLING,
                     ICE_CONTROLLED
  };

  static RefPtr<NrIceCtx> Create(const std::string& name,
                                 bool offerer,
                                 bool set_interface_priorities = true);
  nr_ice_ctx *ctx() { return ctx_; }
  nr_ice_peer_ctx *peer() { return peer_; }

  
  void destroy_peer_ctx();

  
  RefPtr<NrIceMediaStream> CreateStream(const std::string& name,
                                                 int components);

  
  const std::string& name() const { return name_; }

  
  ConnectionState connection_state() const {
    return connection_state_;
  }

  
  GatheringState gathering_state() const {
    return gathering_state_;
  }

  
  std::vector<std::string> GetGlobalAttributes();

  
  nsresult ParseGlobalAttributes(std::vector<std::string> attrs);

  
  nsresult SetControlling(Controlling controlling);

  Controlling GetControlling();

  
  
  nsresult SetStunServers(const std::vector<NrIceStunServer>& stun_servers);

  
  
  nsresult SetTurnServers(const std::vector<NrIceTurnServer>& turn_servers);

  
  
  nsresult SetResolver(nr_resolver *resolver);

  
  nsresult StartGathering();

  
  nsresult StartChecks();

  
  
  nsresult Finalize();

  
  bool generating_trickle() const { return trickle_; }

  
  
  sigslot::signal2<NrIceCtx*, NrIceCtx::GatheringState>
    SignalGatheringStateChange;
  sigslot::signal2<NrIceCtx*, NrIceCtx::ConnectionState>
    SignalConnectionStateChange;

  
  nsCOMPtr<nsIEventTarget> thread() { return sts_target_; }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(NrIceCtx)

 private:
  NrIceCtx(const std::string& name,
           bool offerer)
  : connection_state_(ICE_CTX_INIT),
    gathering_state_(ICE_CTX_GATHER_INIT),
    name_(name),
    offerer_(offerer),
    streams_(),
    ctx_(nullptr),
    peer_(nullptr),
    ice_handler_vtbl_(nullptr),
    ice_handler_(nullptr),
    trickle_(true) {
    
    (void)offerer_;
  }

  virtual ~NrIceCtx();

  DISALLOW_COPY_ASSIGN(NrIceCtx);

  
  static void initialized_cb(NR_SOCKET s, int h, void *arg);  

  
  static int select_pair(void *obj,nr_ice_media_stream *stream,
                         int component_id, nr_ice_cand_pair **potentials,
                         int potential_ct);
  static int stream_ready(void *obj, nr_ice_media_stream *stream);
  static int stream_failed(void *obj, nr_ice_media_stream *stream);
  static int ice_completed(void *obj, nr_ice_peer_ctx *pctx);
  static int msg_recvd(void *obj, nr_ice_peer_ctx *pctx,
                       nr_ice_media_stream *stream, int component_id,
                       unsigned char *msg, int len);
  static void trickle_cb(void *arg, nr_ice_ctx *ctx, nr_ice_media_stream *stream,
                         int component_id, nr_ice_candidate *candidate);

  
  RefPtr<NrIceMediaStream> FindStream(nr_ice_media_stream *stream);

  
  void SetConnectionState(ConnectionState state);

  
  void SetGatheringState(GatheringState state);

  ConnectionState connection_state_;
  GatheringState gathering_state_;
  const std::string name_;
  bool offerer_;
  std::vector<RefPtr<NrIceMediaStream> > streams_;
  nr_ice_ctx *ctx_;
  nr_ice_peer_ctx *peer_;
  nr_ice_handler_vtbl* ice_handler_vtbl_;  
  nr_ice_handler* ice_handler_;  
  bool trickle_;
  nsCOMPtr<nsIEventTarget> sts_target_; 
};


}  
#endif
