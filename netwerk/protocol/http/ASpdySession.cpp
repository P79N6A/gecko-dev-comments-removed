






































#include "nsHttp.h"
#include "ASpdySession.h"
#include "SpdySession2.h"

#include "mozilla/Telemetry.h"

namespace mozilla {
namespace net {

ASpdySession *
ASpdySession::NewSpdySession(PRUint32 version,
                             nsAHttpTransaction *aTransaction,
                             nsISocketTransport *aTransport,
                             PRInt32 aPriority)
{
  
  
  NS_ABORT_IF_FALSE(version == SpdyInformation::SPDY_VERSION_2,
                    "Only version 2 implemented");

  Telemetry::Accumulate(Telemetry::SPDY_VERSION, version);
    
  return new SpdySession2(aTransaction,
                          aTransport,
                          aPriority);
}

SpdyInformation::SpdyInformation()
{
  Version[0] = SPDY_VERSION_2;
  VersionString[0] = NS_LITERAL_CSTRING("spdy/2");
  AlternateProtocolString[0] = NS_LITERAL_CSTRING("443:npn-spdy/2");

  Version[1] = 0;
  VersionString[1] = EmptyCString();
  AlternateProtocolString[1] = EmptyCString();
}

bool
SpdyInformation::ProtocolEnabled(PRUint32 index)
{
  
  if (index == 0)
    return true;

  
  if (index == 1)
    return false;
  
  NS_ABORT_IF_FALSE(false, "index out of range");
  return false;
}

nsresult
SpdyInformation::GetNPNVersionIndex(const nsACString &npnString,
                                    PRUint8 *result)
{
  if (npnString.IsEmpty())
    return NS_ERROR_FAILURE;

  if (npnString.Equals(VersionString[0]))
    *result = Version[0];
  else if (npnString.Equals(VersionString[1]))
    *result = Version[1];
  else
    return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult
SpdyInformation::GetAlternateProtocolVersionIndex(const char *val,
                                                  PRUint8 *result)
{
  if (!val || !val[0])
    return NS_ERROR_FAILURE;

  if (ProtocolEnabled(0) && nsHttp::FindToken(val,
                                              AlternateProtocolString[0].get(),
                                              HTTP_HEADER_VALUE_SEPS))
    *result = Version[0];
  else if (ProtocolEnabled(1) && nsHttp::FindToken(val,
                                                   AlternateProtocolString[1].get(),
                                                   HTTP_HEADER_VALUE_SEPS))
    *result = Version[1];
  else
    return NS_ERROR_FAILURE;

  return NS_OK;
}

} 
} 

