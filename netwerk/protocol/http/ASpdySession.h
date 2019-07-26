





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
  virtual void ReadTimeoutTick(PRIntervalTime now) = 0;
  virtual void DontReuse() = 0;

  static ASpdySession *NewSpdySession(uint32_t version,
                                      nsAHttpTransaction *,
                                      nsISocketTransport *,
                                      int32_t);

  virtual void PrintDiagnostics (nsCString &log) = 0;

  const static uint32_t kSendingChunkSize = 4096;
  const static uint32_t kTCPSendBufferSize = 131072;

  
  
  
  
  const static uint32_t kInitialRwin = 256 * 1024 * 1024;
};




class SpdyInformation
{
public:
  SpdyInformation();
  ~SpdyInformation() {}

  
  
  bool ProtocolEnabled(uint32_t index);

  
  
  nsresult GetNPNVersionIndex(const nsACString &npnString, uint8_t *result);

  uint8_t   Version[2];
  nsCString VersionString[2];
};

}} 

#endif 
