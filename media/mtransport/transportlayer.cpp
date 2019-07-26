






#include "logging.h"
#include "transportflow.h"
#include "transportlayer.h"


namespace mozilla {

MOZ_MTLOG_MODULE("mtransport")

nsresult TransportLayer::Init() {
  if (state_ != TS_NONE)
    return state_ == TS_ERROR ? NS_ERROR_FAILURE : NS_OK;

  nsresult rv = InitInternal();

  if (!NS_SUCCEEDED(rv)) {
    state_ = TS_ERROR;
    return rv;
  }
  state_ = TS_INIT;

  return NS_OK;
}

void TransportLayer::Inserted(TransportFlow *flow, TransportLayer *downward) {
  downward_ = downward;
  flow_id_ = flow->id();
  MOZ_MTLOG(ML_DEBUG, LAYER_INFO << "Inserted: downward='" <<
    (downward ? downward->id(): "none") << "'");

  WasInserted();
}

void TransportLayer::SetState(State state) {
  if (state != state_) {
    MOZ_MTLOG(ML_DEBUG, LAYER_INFO << "state " << state_ << "->" << state);
    state_ = state;
    SignalStateChange(this, state);
  }
}

}  
