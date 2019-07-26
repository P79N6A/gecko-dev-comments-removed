





#ifndef mozilla_net_ASpdySession_h
#define mozilla_net_ASpdySession_h

#include "nsAHttpTransaction.h"
#include "prinrval.h"
#include "nsString.h"

class nsISocketTransport;

namespace mozilla { namespace net {

class ASpdySession : public nsAHttpTransaction
{
public:
  virtual bool AddStream(nsAHttpTransaction *, int32_t) = 0;
  virtual bool CanReuse() = 0;
  virtual bool RoomForMoreStreams() = 0;
  virtual PRIntervalTime IdleTime() = 0;
  virtual uint32_t ReadTimeoutTick(PRIntervalTime now) = 0;
  virtual void DontReuse() = 0;

  static ASpdySession *NewSpdySession(uint32_t version,
                                      nsAHttpTransaction *,
                                      nsISocketTransport *,
                                      int32_t);

  virtual void PrintDiagnostics (nsCString &log) = 0;

  bool ResponseTimeoutEnabled() const MOZ_OVERRIDE MOZ_FINAL {
    return true;
  }

  const static uint32_t kSendingChunkSize = 4095;
  const static uint32_t kTCPSendBufferSize = 131072;

  
  
  
  
  const static uint32_t kInitialRwin = 256 * 1024 * 1024;

  bool SoftStreamError(nsresult code)
  {
    return (code == NS_BASE_STREAM_CLOSED || code == NS_BINDING_FAILED ||
            code == NS_BINDING_ABORTED || code == NS_BINDING_REDIRECTED ||
            code == NS_BINDING_RETARGETED);
  }
};




class SpdyInformation
{
public:
  SpdyInformation();
  ~SpdyInformation() {}

  static const uint32_t kCount = 3;

  
  bool ProtocolEnabled(uint32_t index);

  
  
  nsresult GetNPNVersionIndex(const nsACString &npnString, uint8_t *result);

  uint8_t   Version[kCount];
  nsCString VersionString[kCount];
};

}} 

#endif 
