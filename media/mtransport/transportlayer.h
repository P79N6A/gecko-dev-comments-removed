







#ifndef transportlayer_h__
#define transportlayer_h__

#include "sigslot.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/RefPtr.h"
#include "nsCOMPtr.h"
#include "nsIEventTarget.h"
#include "nsThreadUtils.h"

#include "m_cpp_utils.h"

namespace mozilla {

class TransportFlow;

typedef int TransportResult;

enum {
  TE_WOULDBLOCK = -1, TE_ERROR = -2, TE_INTERNAL = -3
};

#define TRANSPORT_LAYER_ID(name) \
  virtual const std::string id() { return name; } \
  static std::string ID() { return name; }


class TransportLayer : public sigslot::has_slots<> {
 public:
  
  
  enum State { TS_NONE, TS_INIT, TS_CONNECTING, TS_OPEN, TS_CLOSED, TS_ERROR };
  enum Mode { STREAM, DGRAM };

  
  TransportLayer(Mode mode = STREAM) :
    mode_(mode),
    state_(TS_NONE),
    flow_(nullptr),
    downward_(nullptr) {}

  virtual ~TransportLayer() {}

  
  nsresult Init();  
  virtual nsresult InitInternal() { return NS_OK; } 

  
  virtual void Inserted(TransportFlow *flow, TransportLayer *downward);

  
  TransportLayer *downward() { return downward_; }

  
  
  nsresult RunOnThread(nsIRunnable *event) {
    if (target_) {
      nsIThread *thr;

      DebugOnly<nsresult> rv = NS_GetCurrentThread(&thr);
      MOZ_ASSERT(NS_SUCCEEDED(rv));

      if (target_ != thr) {
        return target_->Dispatch(event, NS_DISPATCH_SYNC);
      }
    }

    return event->Run();
  }

  
  State state() const { return state_; }
  
  virtual TransportResult SendPacket(const unsigned char *data, size_t len) = 0;

  
  
  sigslot::signal2<TransportLayer*, State> SignalStateChange;
  
  sigslot::signal3<TransportLayer*, const unsigned char *, size_t>
                         SignalPacketReceived;

  
  virtual const std::string id() = 0;

  
  virtual const std::string& flow_id();

 protected:
  virtual void WasInserted() {}
  virtual void SetState(State state);

  void CheckThread() {
    NS_ABORT_IF_FALSE(CheckThreadInt(), "Wrong thread");
  }

  Mode mode_;
  State state_;
  TransportFlow *flow_;  
  TransportLayer *downward_; 
  nsCOMPtr<nsIEventTarget> target_;

 private:
  DISALLOW_COPY_ASSIGN(TransportLayer);

  bool CheckThreadInt() {
    bool on;
    NS_ENSURE_TRUE(target_, false);
    NS_ENSURE_SUCCESS(target_->IsOnCurrentThread(&on), false);
    NS_ENSURE_TRUE(on, false);

    return true;
  }
};

#define LAYER_INFO "Flow[" << flow_id() << "(none)" << "]; Layer[" << id() << "]: "

}  
#endif
