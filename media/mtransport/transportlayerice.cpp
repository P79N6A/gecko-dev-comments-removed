










































#include <string>
#include <vector>

#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsError.h"
#include "nsIEventTarget.h"
#include "nsNetCID.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"


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
#include "transportflow.h"
#include "transportlayerice.h"

namespace mozilla {

#ifdef ERROR
#undef ERROR
#endif

MOZ_MTLOG_MODULE("mtransport");

TransportLayerIce::TransportLayerIce(const std::string& name,
    RefPtr<NrIceCtx> ctx, RefPtr<NrIceMediaStream> stream,
                                     int component)
    : name_(name), ctx_(ctx), stream_(stream), component_(component) {
  stream_->SignalReady.connect(this, &TransportLayerIce::IceReady);
  stream_->SignalFailed.connect(this, &TransportLayerIce::IceFailed);
  stream_->SignalPacketReceived.connect(this,
                                        &TransportLayerIce::IcePacketReceived);
  if (stream_->state() == NrIceMediaStream::ICE_OPEN) {
    SetState(TS_OPEN);
  }
}

TransportLayerIce::~TransportLayerIce() {
  
}

TransportResult TransportLayerIce::SendPacket(const unsigned char *data,
                                              size_t len) {
  nsresult res = stream_->SendPacket(component_, data, len);

  if (!NS_SUCCEEDED(res)) {
    return (res == NS_BASE_STREAM_WOULD_BLOCK) ?
        TE_WOULDBLOCK : TE_ERROR;
  }

  MOZ_MTLOG(PR_LOG_DEBUG, LAYER_INFO << " SendPacket(" << len << ") succeeded");

  return len;
}


void TransportLayerIce::IceCandidate(NrIceMediaStream *stream,
                                     const std::string&) {
  
}

void TransportLayerIce::IceReady(NrIceMediaStream *stream) {
  SetState(TS_OPEN);
}

void TransportLayerIce::IceFailed(NrIceMediaStream *stream) {
  SetState(TS_ERROR);
}

void TransportLayerIce::IcePacketReceived(NrIceMediaStream *stream, int component,
                       const unsigned char *data, int len) {
  
  
  if (component_ != component)
    return;

  MOZ_MTLOG(PR_LOG_DEBUG, LAYER_INFO << "PacketReceived(" << stream->name() << ","
    << component << "," << len << ")");
  SignalPacketReceived(this, data, len);
}

}  
