







#include "logging.h"
#include "transportflow.h"
#include "transportlayerlog.h"

namespace mozilla {

MOZ_MTLOG_MODULE("mtransport")

void TransportLayerLogging::WasInserted() {
  if (downward_) {
    downward_->SignalStateChange.connect(
        this, &TransportLayerLogging::StateChange);
    downward_->SignalPacketReceived.connect(
        this, &TransportLayerLogging::PacketReceived);
    TL_SET_STATE(downward_->state());
  }
}

TransportResult
TransportLayerLogging::SendPacket(const unsigned char *data, size_t len) {
  MOZ_MTLOG(ML_DEBUG, LAYER_INFO << "SendPacket(" << len << ")");

  if (downward_) {
    return downward_->SendPacket(data, len);
  }
  else {
    return static_cast<TransportResult>(len);
  }
}

void TransportLayerLogging::StateChange(TransportLayer *layer, State state) {
  MOZ_MTLOG(ML_DEBUG, LAYER_INFO << "Received StateChange to " << state);

  TL_SET_STATE(state);
}

void TransportLayerLogging::PacketReceived(TransportLayer* layer,
                                           const unsigned char *data,
                                           size_t len) {
  MOZ_MTLOG(ML_DEBUG, LAYER_INFO << "PacketReceived(" << len << ")");

  SignalPacketReceived(this, data, len);
}



}  
