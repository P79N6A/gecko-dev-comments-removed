





#ifndef mozilla_net_ASpdySession_h
#define mozilla_net_ASpdySession_h

#include "nsAHttpTransaction.h"
#include "prinrval.h"
#include "nsString.h"

class nsISocketTransport;

namespace mozilla { namespace net {

class nsHttpConnectionInfo;

class ASpdySession : public nsAHttpTransaction
{
public:
  ASpdySession();
  virtual ~ASpdySession();

  virtual bool AddStream(nsAHttpTransaction *, int32_t,
                         bool, nsIInterfaceRequestor *) = 0;
  virtual bool CanReuse() = 0;
  virtual bool RoomForMoreStreams() = 0;
  virtual PRIntervalTime IdleTime() = 0;
  virtual uint32_t ReadTimeoutTick(PRIntervalTime now) = 0;
  virtual void DontReuse() = 0;

  static ASpdySession *NewSpdySession(uint32_t version, nsISocketTransport *);

  virtual void PrintDiagnostics (nsCString &log) = 0;

  bool ResponseTimeoutEnabled() const MOZ_OVERRIDE MOZ_FINAL {
    return true;
  }

  virtual void SendPing() = 0;

  const static uint32_t kSendingChunkSize = 4095;
  const static uint32_t kTCPSendBufferSize = 131072;

  
  
  
  
  const static uint32_t kInitialRwin = 256 * 1024 * 1024;

  bool SoftStreamError(nsresult code)
  {
    if (NS_SUCCEEDED(code)) {
      return false;
    }

    return (code == NS_BASE_STREAM_CLOSED || code == NS_BINDING_FAILED ||
            code == NS_BINDING_ABORTED || code == NS_BINDING_REDIRECTED ||
            code == NS_ERROR_INVALID_CONTENT_ENCODING ||
            code == NS_BINDING_RETARGETED || code == NS_ERROR_CORRUPTED_CONTENT);
  }
};

typedef bool (*ALPNCallback) (nsISupports *); 




class SpdyInformation
{
public:
  SpdyInformation();
  ~SpdyInformation() {}

  static const uint32_t kCount = 4;

  
  
  nsresult GetNPNIndex(const nsACString &npnString, uint32_t *result) const;

  
  bool ProtocolEnabled(uint32_t index) const;

  uint8_t   Version[kCount]; 
  nsCString VersionString[kCount]; 

  
  
  
  
  
  ALPNCallback ALPNCallbacks[kCount];
};

}} 

#endif 
