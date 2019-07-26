
















































#ifndef nricectx_h__
#define nricectx_h__

#include <vector>

#include "sigslot.h"

#include "mozilla/RefPtr.h"
#include "mozilla/Scoped.h"
#include "nsAutoPtr.h"
#include "nsIEventTarget.h"
#include "nsITimer.h"

#include "m_cpp_utils.h"

namespace mozilla {

typedef void* NR_SOCKET;
typedef struct nr_ice_ctx_ nr_ice_ctx;
typedef struct nr_ice_peer_ctx_ nr_ice_peer_ctx;
typedef struct nr_ice_media_stream_ nr_ice_media_stream;
typedef struct nr_ice_handler_ nr_ice_handler;
typedef struct nr_ice_handler_vtbl_ nr_ice_handler_vtbl;
typedef struct nr_ice_cand_pair_ nr_ice_cand_pair;

class NrIceMediaStream;

class NrIceCtx {
 public:
  enum State { ICE_CTX_INIT,
               ICE_CTX_GATHERING,
               ICE_CTX_GATHERED,
               ICE_CTX_CHECKING,
               ICE_CTX_OPEN,
               ICE_CTX_FAILED
  };

  static RefPtr<NrIceCtx> Create(const std::string& name,
                                          bool offerer,
                                          bool set_interface_priorities = true);
  virtual ~NrIceCtx();

  nr_ice_ctx *ctx() { return ctx_; }
  nr_ice_peer_ctx *peer() { return peer_; }

  
  RefPtr<NrIceMediaStream> CreateStream(const std::string& name,
                                                 int components);

  
  const std::string& name() const { return name_; }

  
  State state() const { return state_; }

  
  std::vector<std::string> GetGlobalAttributes();

  
  nsresult ParseGlobalAttributes(std::vector<std::string> attrs);

  
  nsresult StartGathering();

  
  nsresult StartChecks();

  
  
  nsresult Finalize();

  
  
  sigslot::signal1<NrIceCtx *> SignalGatheringCompleted;  
  sigslot::signal1<NrIceCtx *> SignalCompleted;  

  
  nsCOMPtr<nsIEventTarget> thread() { return sts_target_; }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(NrIceCtx);

 private:
  NrIceCtx(const std::string& name, bool offerer)
      : state_(ICE_CTX_INIT),
      name_(name),
      offerer_(offerer),
      streams_(),
      ctx_(nullptr),
      peer_(nullptr),
      ice_handler_vtbl_(nullptr),
      ice_handler_(nullptr) {}

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

  
  
  void EmitAllCandidates();

  
  RefPtr<NrIceMediaStream> FindStream(nr_ice_media_stream *stream);

  
  void SetState(State state);

  State state_;
  const std::string name_;
  bool offerer_;
  std::vector<RefPtr<NrIceMediaStream> > streams_;
  nr_ice_ctx *ctx_;
  nr_ice_peer_ctx *peer_;
  nr_ice_handler_vtbl* ice_handler_vtbl_;  
  nr_ice_handler* ice_handler_;  
  nsCOMPtr<nsIEventTarget> sts_target_; 
};


}  
#endif
