






#include <deque>

#include <prlog.h>

#include "logging.h"
#include "transportflow.h"
#include "transportlayer.h"

namespace mozilla {

MOZ_MTLOG_MODULE("mtransport")

TransportFlow::~TransportFlow() {
  for (std::deque<TransportLayer *>::iterator it = layers_.begin();
       it != layers_.end(); ++it) {
    delete *it;
  }
}

nsresult TransportFlow::PushLayer(TransportLayer *layer) {
  
  if (state_ == TransportLayer::TS_ERROR) {
    MOZ_MTLOG(PR_LOG_ERROR, id_ + ": Can't call PushLayer in error state for flow ");
    return NS_ERROR_FAILURE;
  }

  nsresult rv = layer->Init();
  if (!NS_SUCCEEDED(rv)) {
    
    MOZ_MTLOG(PR_LOG_ERROR, id_ << ": Layer initialization failed; invalidating");
    StateChangeInt(TransportLayer::TS_ERROR);
    return rv;
  }

  TransportLayer *old_layer = layers_.empty() ? nullptr : layers_.front();

  
  if (old_layer) {
    old_layer->SignalStateChange.disconnect(this);
    old_layer->SignalPacketReceived.disconnect(this);
  }
  layers_.push_front(layer);
  layer->Inserted(this, old_layer);

  layer->SignalStateChange.connect(this, &TransportFlow::StateChange);
  layer->SignalPacketReceived.connect(this, &TransportFlow::PacketReceived);
  StateChangeInt(layer->state());

  return NS_OK;
}


nsresult TransportFlow::PushLayers(nsAutoPtr<std::queue<TransportLayer *> > layers) {
  MOZ_ASSERT(!layers->empty());
  if (layers->empty()) {
    MOZ_MTLOG(PR_LOG_ERROR, id_ << ": Can't call PushLayers with empty layers");
    return NS_ERROR_INVALID_ARG;
  }

  
  if (state_ == TransportLayer::TS_ERROR) {
    MOZ_MTLOG(PR_LOG_ERROR, id_ << ": Can't call PushLayers in error state for flow ");
    return NS_ERROR_FAILURE;
  }

  nsresult rv = NS_OK;

  
  disconnect_all();

  TransportLayer *layer;

  while (!layers->empty()) {
    TransportLayer *old_layer = layers_.empty() ? nullptr : layers_.front();
    layer = layers->front();

    rv = layer->Init();
    if (NS_FAILED(rv)) {
      MOZ_MTLOG(PR_LOG_ERROR, id_ << ": Layer initialization failed; invalidating flow ");
      break;
    }

    
    layers_.push_front(layer);
    layers->pop();
    layer->Inserted(this, old_layer);
  }

  if (NS_FAILED(rv)) {
    
    while (!layers->empty()) {
      delete layers->front();
      layers->pop();
    }

    
    
    while (!layers_.empty()) {
      delete layers_.front();
      layers_.pop_front();
    }

    
    StateChangeInt(TransportLayer::TS_ERROR);

    
    return rv;
  }

  
  layer->SignalStateChange.connect(this, &TransportFlow::StateChange);
  layer->SignalPacketReceived.connect(this, &TransportFlow::PacketReceived);
  StateChangeInt(layer->state());  

  return NS_OK;
}

TransportLayer *TransportFlow::top() const {
  return layers_.empty() ? nullptr : layers_.front();
}

TransportLayer *TransportFlow::GetLayer(const std::string& id) const {
  for (std::deque<TransportLayer *>::const_iterator it = layers_.begin();
       it != layers_.end(); ++it) {
    if ((*it)->id() == id)
      return *it;
  }

  return nullptr;
}

TransportLayer::State TransportFlow::state() {
  return state_;
}

TransportResult TransportFlow::SendPacket(const unsigned char *data,
                                          size_t len) {
  if (state_ != TransportLayer::TS_OPEN) {
    return TE_ERROR;
  }
  return top() ? top()->SendPacket(data, len) : TE_ERROR;
}

void TransportFlow::StateChangeInt(TransportLayer::State state) {
  if (state == state_) {
    return;
  }

  state_ = state;
  SignalStateChange(this, state_);
}

void TransportFlow::StateChange(TransportLayer *layer,
                                TransportLayer::State state) {
  StateChangeInt(state);
}

void TransportFlow::PacketReceived(TransportLayer* layer,
                                   const unsigned char *data,
                                   size_t len) {
  SignalPacketReceived(this, data, len);
}

}  
