








#ifndef transportlayerice_h__
#define transportlayerice_h__

#include <vector>

#include "sigslot.h"

#include "mozilla/RefPtr.h"
#include "mozilla/Scoped.h"
#include "nsCOMPtr.h"
#include "nsIEventTarget.h"
#include "nsITimer.h"

#include "m_cpp_utils.h"

#include "nricemediastream.h"
#include "transportflow.h"
#include "transportlayer.h"


namespace mozilla {

class TransportLayerIce : public TransportLayer {
 public:
  explicit TransportLayerIce(const std::string& name);

  virtual ~TransportLayerIce();

  void SetParameters(RefPtr<NrIceCtx> ctx,
                     RefPtr<NrIceMediaStream> stream,
                     int component);

  
  virtual TransportResult SendPacket(const unsigned char *data, size_t len);

  
  void IceCandidate(NrIceMediaStream *stream, const std::string&);
  void IceReady(NrIceMediaStream *stream);
  void IceFailed(NrIceMediaStream *stream);
  void IcePacketReceived(NrIceMediaStream *stream, int component,
                         const unsigned char *data, int len);

  TRANSPORT_LAYER_ID("ice")

 private:
  DISALLOW_COPY_ASSIGN(TransportLayerIce);
  void PostSetup();

  const std::string name_;
  RefPtr<NrIceCtx> ctx_;
  RefPtr<NrIceMediaStream> stream_;
  int component_;
};

}  
#endif
