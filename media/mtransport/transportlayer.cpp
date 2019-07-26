






#include <prlog.h>

#include "logging.h"
#include "transportflow.h"
#include "transportlayer.h"


namespace mozilla {

MOZ_MTLOG_MODULE("mtransport");

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
  flow_ = flow;
  downward_ = downward;

  MOZ_MTLOG(PR_LOG_DEBUG, LAYER_INFO << "Inserted: downward='" <<
    (downward ? downward->id(): "none") << "'");

  WasInserted();
}

void TransportLayer::SetState(State state) {
  if (state != state_) {
    MOZ_MTLOG(PR_LOG_DEBUG, LAYER_INFO << "state " << state_ << "->" << state);
    state_ = state;
    SignalStateChange(this, state);
  }
}

const std::string& TransportLayer::flow_id() {
    static const std::string empty;

    return flow_ ? flow_->id() : empty;
  }
}  
