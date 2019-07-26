







#ifndef transportflow_h__
#define transportflow_h__

#include <deque>
#include <queue>
#include <string>

#include "nscore.h"
#include "nsISupportsImpl.h"
#include "transportlayer.h"
#include "m_cpp_utils.h"



namespace mozilla {

class TransportFlow : public sigslot::has_slots<> {
 public:
  TransportFlow()
    : id_("(anonymous)"),
      state_(TransportLayer::TS_NONE) {}
  TransportFlow(const std::string id)
    : id_(id),
      state_(TransportLayer::TS_NONE) {}

  ~TransportFlow();

  const std::string& id() const { return id_; }

  
  
  
  
  
  
  
  nsresult PushLayer(TransportLayer *layer);

  
  
  
  
  
  nsresult PushLayers(nsAutoPtr<std::queue<TransportLayer *> > layers);

  TransportLayer *top() const;
  TransportLayer *GetLayer(const std::string& id) const;

  
  
  TransportLayer::State state(); 
  TransportResult SendPacket(const unsigned char *data, size_t len);

  
  sigslot::signal2<TransportFlow *, TransportLayer::State>
    SignalStateChange;

  
  sigslot::signal3<TransportFlow*, const unsigned char *, size_t>
    SignalPacketReceived;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TransportFlow)

 private:
  DISALLOW_COPY_ASSIGN(TransportFlow);

  void StateChange(TransportLayer *layer, TransportLayer::State state);
  void StateChangeInt(TransportLayer::State state);
  void PacketReceived(TransportLayer* layer, const unsigned char *data,
      size_t len);

  std::string id_;
  std::deque<TransportLayer *> layers_;
  TransportLayer::State state_;
};

}  
#endif
