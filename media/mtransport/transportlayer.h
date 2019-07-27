







#ifndef transportlayer_h__
#define transportlayer_h__

#include "sigslot.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/RefPtr.h"
#include "nsCOMPtr.h"
#include "nsIEventTarget.h"

#include "m_cpp_utils.h"

namespace mozilla {

class TransportFlow;

typedef int TransportResult;

enum {
  TE_WOULDBLOCK = -1, TE_ERROR = -2, TE_INTERNAL = -3
};

#define TRANSPORT_LAYER_ID(name) \
  virtual const std::string id() const { return name; } \
  static std::string ID() { return name; }


class TransportLayer : public sigslot::has_slots<> {
 public:
  
  
  enum State { TS_NONE, TS_INIT, TS_CONNECTING, TS_OPEN, TS_CLOSED, TS_ERROR };

  
  TransportLayer() :
    state_(TS_NONE),
    flow_id_(),
    downward_(nullptr) {}

  virtual ~TransportLayer() {}

  
  nsresult Init();  
  virtual nsresult InitInternal() { return NS_OK; } 

  
  virtual void Inserted(TransportFlow *flow, TransportLayer *downward);

  
  TransportLayer *downward() { return downward_; }

  
  
  nsresult RunOnThread(nsIRunnable *event);

  
  State state() const { return state_; }
  
  virtual TransportResult SendPacket(const unsigned char *data, size_t len) = 0;

  
  const nsCOMPtr<nsIEventTarget> GetThread() const {
    return target_;
  }

  
  
  sigslot::signal2<TransportLayer*, State> SignalStateChange;
  
  sigslot::signal3<TransportLayer*, const unsigned char *, size_t>
                         SignalPacketReceived;

  
  virtual const std::string id() const = 0;

  
  const std::string& flow_id() const {
    return flow_id_;
  }

 protected:
  virtual void WasInserted() {}
  virtual void SetState(State state, const char *file, unsigned line);
  
  void CheckThread() const {
    NS_ABORT_IF_FALSE(CheckThreadInt(), "Wrong thread");
  }

  State state_;
  std::string flow_id_;
  TransportLayer *downward_; 
  nsCOMPtr<nsIEventTarget> target_;

 private:
  DISALLOW_COPY_ASSIGN(TransportLayer);

  bool CheckThreadInt() const {
    bool on;

    if (!target_)  
      return true;

    NS_ENSURE_SUCCESS(target_->IsOnCurrentThread(&on), false);
    NS_ENSURE_TRUE(on, false);

    return true;
  }
};

#define LAYER_INFO "Flow[" << flow_id() << "(none)" << "]; Layer[" << id() << "]: "
#define TL_SET_STATE(x) SetState((x), __FILE__, __LINE__)

}  
#endif
