





#include "nsHttp.h"
#include "nsHttpHandler.h"

#include "ASpdySession.h"
#include "SpdySession2.h"
#include "SpdySession3.h"

#include "mozilla/Telemetry.h"

namespace mozilla {
namespace net {

ASpdySession *
ASpdySession::NewSpdySession(uint32_t version,
                             nsAHttpTransaction *aTransaction,
                             nsISocketTransport *aTransport,
                             int32_t aPriority)
{
  
  
  NS_ABORT_IF_FALSE(version == SpdyInformation::SPDY_VERSION_2 ||
                    version == SpdyInformation::SPDY_VERSION_3,
                    "Unsupported spdy version");

  
  
  
  

  Telemetry::Accumulate(Telemetry::SPDY_VERSION2, version);
    
  if (version == SpdyInformation::SPDY_VERSION_2)
    return new SpdySession2(aTransaction, aTransport, aPriority);

  return new SpdySession3(aTransaction, aTransport, aPriority);
}

SpdyInformation::SpdyInformation()
{
  
  Version[0] = SPDY_VERSION_3;
  VersionString[0] = NS_LITERAL_CSTRING("spdy/3");
  AlternateProtocolString[0] = NS_LITERAL_CSTRING("443:npn-spdy/3");

  Version[1] = SPDY_VERSION_2;
  VersionString[1] = NS_LITERAL_CSTRING("spdy/2");
  AlternateProtocolString[1] = NS_LITERAL_CSTRING("443:npn-spdy/2");
}

bool
SpdyInformation::ProtocolEnabled(uint32_t index)
{
  if (index == 0)
    return gHttpHandler->IsSpdyV3Enabled();

  if (index == 1)
    return gHttpHandler->IsSpdyV2Enabled();

  NS_ABORT_IF_FALSE(false, "index out of range");
  return false;
}

nsresult
SpdyInformation::GetNPNVersionIndex(const nsACString &npnString,
                                    uint8_t *result)
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
                                                  uint8_t *result)
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

