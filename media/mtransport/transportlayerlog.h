







#ifndef transportlayerlog_h__
#define transportlayerlog_h__

#include "m_cpp_utils.h"
#include "transportflow.h"
#include "transportlayer.h"

namespace mozilla {

class TransportLayerLogging : public TransportLayer {
public:
  TransportLayerLogging() {}

  
  virtual TransportResult SendPacket(const unsigned char *data, size_t len);

  
  void StateChange(TransportLayer *layer, State state);
  void PacketReceived(TransportLayer* layer, const unsigned char *data,
                      size_t len);

  TRANSPORT_LAYER_ID("log")

protected:
  virtual void WasInserted();

private:
  DISALLOW_COPY_ASSIGN(TransportLayerLogging);
};


}  
#endif
