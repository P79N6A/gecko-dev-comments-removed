





#ifndef mozilla_net_ASpdySession_h
#define mozilla_net_ASpdySession_h

#include "nsAHttpTransaction.h"
#include "nsAHttpConnection.h"
#include "prinrval.h"
#include "nsString.h"

class nsISocketTransport;

namespace mozilla { namespace net {








class ASpdySession : public nsAHttpTransaction
{
public:
  virtual bool AddStream(nsAHttpTransaction *, PRInt32) = 0;
  virtual bool CanReuse() = 0;
  virtual bool RoomForMoreStreams() = 0;
  virtual PRIntervalTime IdleTime() = 0;
  virtual void ReadTimeoutTick(PRIntervalTime now) = 0;
  virtual void DontReuse() = 0;

  static ASpdySession *NewSpdySession(PRUint32 version,
                                      nsAHttpTransaction *,
                                      nsISocketTransport *,
                                      PRInt32);

  virtual void PrintDiagnostics (nsCString &log) = 0;

  const static PRUint32 kSendingChunkSize = 4096;
};




class SpdyInformation
{
public:
  SpdyInformation();
  ~SpdyInformation() {}

  
  
  bool ProtocolEnabled(PRUint32 index);

  
  
  nsresult GetNPNVersionIndex(const nsACString &npnString, PRUint8 *result);

  
  
  nsresult GetAlternateProtocolVersionIndex(const char *val,
                                            PRUint8 *result);

  enum {
    SPDY_VERSION_2 = 2,
    SPDY_VERSION_3 = 3
  };

  PRUint8   Version[2];
  nsCString VersionString[2];
  nsCString AlternateProtocolString[2];
};

}} 

#endif 
