










































#include <string>
#include <vector>

#include "nsError.h"


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


#include "logging.h"
#include "nricectx.h"
#include "nricemediastream.h"

namespace mozilla {

MOZ_MTLOG_MODULE("mtransport")


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
    MOZ_MTLOG(PR_LOG_ERROR, "Couldn't create ICE media stream for '"
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
    MOZ_MTLOG(PR_LOG_ERROR, "Couldn't parse attributes for stream "
         << name_ << "'");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}


nsresult NrIceMediaStream::ParseTrickleCandidate(const std::string& candidate) {
  int r;

  MOZ_MTLOG(PR_LOG_DEBUG, "NrIceCtx(" << ctx_->name() << ")/STREAM(" <<
            name() << ") : parsing trickle candidate " << candidate);

  r = nr_ice_peer_ctx_parse_trickle_candidate(ctx_->peer(),
                                              stream_,
                                              const_cast<char *>(
                                                  candidate.c_str()));
  if (r) {
    if (r == R_ALREADY) {
      MOZ_MTLOG(PR_LOG_ERROR, "Trickle candidates are redundant for stream '"
         << name_ << "' because it is completed");

    } else {
      MOZ_MTLOG(PR_LOG_ERROR, "Couldn't parse trickle candidate for stream '"
         << name_ << "'");
      return NS_ERROR_FAILURE;
    }
  }

  return NS_OK;
}


void NrIceMediaStream::EmitAllCandidates() {
  char **attrs = 0;
  int attrct;
  int r;
  r = nr_ice_media_stream_get_attributes(stream_,
                                         &attrs, &attrct);
  if (r) {
    MOZ_MTLOG(PR_LOG_ERROR, "Couldn't get ICE candidates for '"
         << name_ << "'");
    return;
  }

  for (int i=0; i<attrct; i++) {
    SignalCandidate(this, attrs[i]);
    RFREE(attrs[i]);
  }

  RFREE(attrs);
}

nsresult NrIceMediaStream::GetDefaultCandidate(int component,
                                               std::string *addrp,
                                               int *portp) {
  nr_ice_candidate *cand;
  int r;

  r = nr_ice_media_stream_get_default_candidate(stream_,
                                                component, &cand);
  if (r) {
    MOZ_MTLOG(PR_LOG_ERROR, "Couldn't get default ICE candidate for '"
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
    MOZ_MTLOG(PR_LOG_ERROR, "Couldn't get ICE candidates for '"
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

nsresult NrIceMediaStream::SendPacket(int component_id,
                                      const unsigned char *data,
                                      size_t len) {
  if (!stream_)
    return NS_ERROR_FAILURE;

  int r = nr_ice_media_stream_send(ctx_->peer(), stream_,
                                   component_id,
                                   const_cast<unsigned char *>(data), len);
  if (r) {
    MOZ_MTLOG(PR_LOG_ERROR, "Couldn't send media on '" << name_ << "'");
    if (r == R_WOULDBLOCK) {
      return NS_BASE_STREAM_WOULD_BLOCK;
    }

    return NS_BASE_STREAM_OSERROR;
  }

  return NS_OK;
}


void NrIceMediaStream::Ready() {
  MOZ_MTLOG(PR_LOG_DEBUG, "Marking stream ready '" << name_ << "'");
  state_ = ICE_OPEN;
  SignalReady(this);
}

void NrIceMediaStream::Close() {
  MOZ_MTLOG(PR_LOG_DEBUG, "Marking stream closed '" << name_ << "'");
  state_ = ICE_CLOSED;
  stream_ = nullptr;
}
}  
