










































#include <string>
#include <vector>

#include "logging.h"
#include "nsError.h"
#include "mozilla/Scoped.h"


extern "C" {
#include "nr_api.h"
#include "registry.h"
#include "async_timer.h"
#include "ice_util.h"
#include "transport_addr.h"
#include "nr_crypto.h"
#include "nr_socket.h"
#include "nr_socket_local.h"
#include "stun_client_ctx.h"
#include "stun_server_ctx.h"
#include "ice_ctx.h"
#include "ice_candidate.h"
#include "ice_handler.h"
}


#include "nricectx.h"
#include "nricemediastream.h"

namespace mozilla {

MOZ_MTLOG_MODULE("mtransport")

static bool ToNrIceCandidate(const nr_ice_candidate& candc,
                             NrIceCandidate* out) {
  MOZ_ASSERT(out);
  int r;
  
  nr_ice_candidate *cand = const_cast<nr_ice_candidate *>(&candc);
  char addr[INET6_ADDRSTRLEN + 1];

  r = nr_transport_addr_get_addrstring(&cand->addr, addr, sizeof(addr));
  if (r)
    return false;

  int port;
  r = nr_transport_addr_get_port(&cand->addr, &port);
  if (r)
    return false;

  NrIceCandidate::Type type;

  switch (cand->type) {
    case HOST:
      type = NrIceCandidate::ICE_HOST;
      break;
    case SERVER_REFLEXIVE:
      type = NrIceCandidate::ICE_SERVER_REFLEXIVE;
      break;
    case PEER_REFLEXIVE:
      type = NrIceCandidate::ICE_PEER_REFLEXIVE;
      break;
    case RELAYED:
      type = NrIceCandidate::ICE_RELAYED;
      break;
    default:
      return false;
  }

  out->host = addr;
  out->port = port;
  out->type = type;
  return true;
}




static NrIceCandidate* MakeNrIceCandidate(const nr_ice_candidate& candc) {
  ScopedDeletePtr<NrIceCandidate> out(new NrIceCandidate());

  if (!ToNrIceCandidate(candc, out)) {
    return nullptr;
  }
  return out.forget();
}


RefPtr<NrIceMediaStream>
NrIceMediaStream::Create(NrIceCtx *ctx,
                         const std::string& name,
                         int components) {
  RefPtr<NrIceMediaStream> stream =
    new NrIceMediaStream(ctx, name, components);

  int r = nr_ice_add_media_stream(ctx->ctx(),
                                  const_cast<char *>(name.c_str()),
                                  components, &stream->stream_);
  if (r) {
    MOZ_MTLOG(ML_ERROR, "Couldn't create ICE media stream for '"
              << name << "'");
    return nullptr;
  }

  return stream;
}

NrIceMediaStream::~NrIceMediaStream() {
  
  
}

nsresult NrIceMediaStream::ParseAttributes(std::vector<std::string>&
                                           attributes) {
  if (!stream_)
    return NS_ERROR_FAILURE;

  std::vector<char *> attributes_in;

  for (size_t i=0; i<attributes.size(); ++i) {
    attributes_in.push_back(const_cast<char *>(attributes[i].c_str()));
  }

  
  int r = nr_ice_peer_ctx_parse_stream_attributes(ctx_->peer(),
                                                  stream_,
                                                  attributes_in.size() ?
                                                  &attributes_in[0] : nullptr,
                                                  attributes_in.size());
  if (r) {
    MOZ_MTLOG(ML_ERROR, "Couldn't parse attributes for stream "
              << name_ << "'");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}


nsresult NrIceMediaStream::ParseTrickleCandidate(const std::string& candidate) {
  int r;

  MOZ_MTLOG(ML_DEBUG, "NrIceCtx(" << ctx_->name() << ")/STREAM(" <<
            name() << ") : parsing trickle candidate " << candidate);

  r = nr_ice_peer_ctx_parse_trickle_candidate(ctx_->peer(),
                                              stream_,
                                              const_cast<char *>(
                                                candidate.c_str())
                                              );
  if (r) {
    if (r == R_ALREADY) {
      MOZ_MTLOG(ML_ERROR, "Trickle candidates are redundant for stream '"
                << name_ << "' because it is completed");

    } else {
      MOZ_MTLOG(ML_ERROR, "Couldn't parse trickle candidate for stream '"
                << name_ << "'");
      return NS_ERROR_FAILURE;
    }
  }

  return NS_OK;
}


nsresult NrIceMediaStream::GetActivePair(int component,
                                         NrIceCandidate **localp,
                                         NrIceCandidate **remotep) {
  int r;
  nr_ice_candidate *local_int;
  nr_ice_candidate *remote_int;

  r = nr_ice_media_stream_get_active(ctx_->peer(),
                                     stream_,
                                     component,
                                     &local_int, &remote_int);
  
  if (r == R_REJECTED)
    return NS_ERROR_NOT_AVAILABLE;

  if (r)
    return NS_ERROR_FAILURE;

  ScopedDeletePtr<NrIceCandidate> local(
      MakeNrIceCandidate(*local_int));
  if (!local)
    return NS_ERROR_FAILURE;

  ScopedDeletePtr<NrIceCandidate> remote(
      MakeNrIceCandidate(*remote_int));
  if (!remote)
    return NS_ERROR_FAILURE;

  if (localp)
    *localp = local.forget();
  if (remotep)
    *remotep = remote.forget();

  return NS_OK;
}


void NrIceMediaStream::EmitAllCandidates() {
  char **attrs = 0;
  int attrct;
  int r;
  r = nr_ice_media_stream_get_attributes(stream_,
                                         &attrs, &attrct);
  if (r) {
    MOZ_MTLOG(ML_ERROR, "Couldn't get ICE candidates for '"
         << name_ << "'");
    return;
  }

  for (int i=0; i<attrct; i++) {
    SignalCandidate(this, attrs[i]);
    RFREE(attrs[i]);
  }

  RFREE(attrs);
}

nsresult NrIceMediaStream::GetCandidatePairs(std::vector<NrIceCandidatePair>*
                                             out_pairs) const {
  MOZ_ASSERT(out_pairs);

  
  
  nr_ice_media_stream* peer_stream;
  int r = nr_ice_peer_ctx_find_pstream(ctx_->peer(), stream_, &peer_stream);
  if (r != 0) {
    return NS_ERROR_FAILURE;
  }

  nr_ice_cand_pair *p1;
  out_pairs->clear();

  TAILQ_FOREACH(p1, &peer_stream->check_list, entry) {
    MOZ_ASSERT(p1);
    MOZ_ASSERT(p1->local);
    MOZ_ASSERT(p1->remote);
    NrIceCandidatePair pair;

    switch (p1->state) {
      case NR_ICE_PAIR_STATE_FROZEN:
        pair.state = NrIceCandidatePair::State::STATE_FROZEN;
        break;
      case NR_ICE_PAIR_STATE_WAITING:
        pair.state = NrIceCandidatePair::State::STATE_WAITING;
        break;
      case NR_ICE_PAIR_STATE_IN_PROGRESS:
        pair.state = NrIceCandidatePair::State::STATE_IN_PROGRESS;
        break;
      case NR_ICE_PAIR_STATE_FAILED:
        pair.state = NrIceCandidatePair::State::STATE_FAILED;
        break;
      case NR_ICE_PAIR_STATE_SUCCEEDED:
        pair.state = NrIceCandidatePair::State::STATE_SUCCEEDED;
        break;
      case NR_ICE_PAIR_STATE_CANCELLED:
        pair.state = NrIceCandidatePair::State::STATE_CANCELLED;
        break;
      default:
        MOZ_ASSERT(0);
    }

    pair.priority = p1->priority;
    pair.nominated = p1->peer_nominated || p1->nominated;
    pair.selected = p1->local->component &&
                    p1->local->component->active == p1;

    if (!ToNrIceCandidate(*(p1->local), &pair.local) ||
        !ToNrIceCandidate(*(p1->remote), &pair.remote)) {
      return NS_ERROR_FAILURE;
    }

    out_pairs->push_back(pair);
  }

  return NS_OK;
}

nsresult NrIceMediaStream::GetDefaultCandidate(int component,
                                               std::string *addrp,
                                               int *portp) {
  nr_ice_candidate *cand;
  int r;

  r = nr_ice_media_stream_get_default_candidate(stream_,
                                                component, &cand);
  if (r) {
    MOZ_MTLOG(ML_ERROR, "Couldn't get default ICE candidate for '"
              << name_ << "'");
    return NS_ERROR_NOT_AVAILABLE;
  }

  char addr[64];  
  r = nr_transport_addr_get_addrstring(&cand->addr,addr,sizeof(addr));
  if (r)
    return NS_ERROR_FAILURE;

  int port;
  r=nr_transport_addr_get_port(&cand->addr,&port);
  if (r)
    return NS_ERROR_FAILURE;

  *addrp = addr;
  *portp = port;

  return NS_OK;
}

std::vector<std::string> NrIceMediaStream::GetCandidates() const {
  char **attrs = 0;
  int attrct;
  int r;
  std::vector<std::string> ret;

  r = nr_ice_media_stream_get_attributes(stream_,
                                         &attrs, &attrct);
  if (r) {
    MOZ_MTLOG(ML_ERROR, "Couldn't get ICE candidates for '"
              << name_ << "'");
    return ret;
  }

  for (int i=0; i<attrct; i++) {
    ret.push_back(attrs[i]);
    RFREE(attrs[i]);
  }

  RFREE(attrs);

  return ret;
}

nsresult NrIceMediaStream::DisableComponent(int component_id) {
  if (!stream_)
    return NS_ERROR_FAILURE;

  int r = nr_ice_media_stream_disable_component(stream_,
                                                component_id);
  if (r) {
    MOZ_MTLOG(ML_ERROR, "Couldn't disable '" << name_ << "':" <<
              component_id);
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult NrIceMediaStream::SendPacket(int component_id,
                                      const unsigned char *data,
                                      size_t len) {
  if (!stream_)
    return NS_ERROR_FAILURE;

  int r = nr_ice_media_stream_send(ctx_->peer(), stream_,
                                   component_id,
                                   const_cast<unsigned char *>(data), len);
  if (r) {
    MOZ_MTLOG(ML_ERROR, "Couldn't send media on '" << name_ << "'");
    if (r == R_WOULDBLOCK) {
      return NS_BASE_STREAM_WOULD_BLOCK;
    }

    return NS_BASE_STREAM_OSERROR;
  }

  return NS_OK;
}


void NrIceMediaStream::Ready() {
  
  
  if (state_ != ICE_OPEN) {
    MOZ_MTLOG(ML_DEBUG, "Marking stream ready '" << name_ << "'");
    state_ = ICE_OPEN;
    SignalReady(this);
  }
  else {
    MOZ_MTLOG(ML_DEBUG, "Stream ready callback fired again for '" << name_ << "'");
  }
}

void NrIceMediaStream::Close() {
  MOZ_MTLOG(ML_DEBUG, "Marking stream closed '" << name_ << "'");
  state_ = ICE_CLOSED;
  stream_ = nullptr;
}
}  
