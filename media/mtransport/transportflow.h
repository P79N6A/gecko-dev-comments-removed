







#ifndef transportflow_h__
#define transportflow_h__

#include <deque>
#include <queue>
#include <string>

#include "nscore.h"
#include "nsISupportsImpl.h"
#include "mozilla/Scoped.h"
#include "transportlayer.h"
#include "m_cpp_utils.h"
#include "nsAutoPtr.h"




























namespace mozilla {

class TransportFlow final : public nsISupports,
                            public sigslot::has_slots<> {
 public:
  TransportFlow()
    : id_("(anonymous)"),
      state_(TransportLayer::TS_NONE),
      layers_(new std::deque<TransportLayer *>) {}
  explicit TransportFlow(const std::string id)
    : id_(id),
      state_(TransportLayer::TS_NONE),
      layers_(new std::deque<TransportLayer *>) {}

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

  bool Contains(TransportLayer *layer) const;

  NS_DECL_THREADSAFE_ISUPPORTS

 private:
  ~TransportFlow();

  DISALLOW_COPY_ASSIGN(TransportFlow);

  
  void CheckThread() const {
    if (!CheckThreadInt())
      MOZ_CRASH();
  }

  bool CheckThreadInt() const {
    bool on;

    if (!target_)  
      return true;
    if (NS_FAILED(target_->IsOnCurrentThread(&on)))
      return false;

    return on;
  }

  void EnsureSameThread(TransportLayer *layer);

  void StateChange(TransportLayer *layer, TransportLayer::State state);
  void StateChangeInt(TransportLayer::State state);
  void PacketReceived(TransportLayer* layer, const unsigned char *data,
      size_t len);
  static void DestroyFinal(nsAutoPtr<std::deque<TransportLayer *> > layers);

  
  static void ClearLayers(std::deque<TransportLayer *>* layers);
  static void ClearLayers(std::queue<TransportLayer *>* layers);

  std::string id_;
  TransportLayer::State state_;
  ScopedDeletePtr<std::deque<TransportLayer *> > layers_;
  nsCOMPtr<nsIEventTarget> target_;
};

}  
#endif
