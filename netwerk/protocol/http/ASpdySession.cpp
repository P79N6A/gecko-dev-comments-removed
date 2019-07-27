






#include "HttpLog.h"





#include "nsHttp.h"
#include "nsHttpHandler.h"

#include "ASpdySession.h"
#include "PSpdyPush.h"
#include "SpdyPush31.h"
#include "Http2Push.h"
#include "SpdySession31.h"
#include "Http2Session.h"

#include "mozilla/Telemetry.h"

namespace mozilla {
namespace net {

ASpdySession::ASpdySession()
{
}

ASpdySession::~ASpdySession()
{
}

ASpdySession *
ASpdySession::NewSpdySession(uint32_t version,
                             nsISocketTransport *aTransport)
{
  
  
  MOZ_ASSERT(version == SPDY_VERSION_31 ||
             version == HTTP_VERSION_2 ||
             version == NS_HTTP2_DRAFT_VERSION,
             "Unsupported spdy version");

  
  
  
  

  Telemetry::Accumulate(Telemetry::SPDY_VERSION2, version);

  if (version == SPDY_VERSION_31) {
    return new SpdySession31(aTransport);
  } else if (version == NS_HTTP2_DRAFT_VERSION || version == HTTP_VERSION_2) {
    return new Http2Session(aTransport);
  }

  return nullptr;
}
static bool SpdySessionTrue(nsISupports *securityInfo)
{
  return true;
}

SpdyInformation::SpdyInformation()
{
  
  
  Version[0] = SPDY_VERSION_31;
  VersionString[0] = NS_LITERAL_CSTRING("spdy/3.1");
  ALPNCallbacks[0] = SpdySessionTrue;

  Version[1] = HTTP_VERSION_2;
  VersionString[1] = NS_LITERAL_CSTRING("h2");
  ALPNCallbacks[1] = Http2Session::ALPNCallback;

  Version[2] = NS_HTTP2_DRAFT_VERSION;
  VersionString[2] = NS_LITERAL_CSTRING("h2-14");
  ALPNCallbacks[2] = Http2Session::ALPNCallback;

  Version[3] = NS_HTTP2_DRAFT_VERSION;
  VersionString[3] = NS_LITERAL_CSTRING(NS_HTTP2_DRAFT_TOKEN);
  ALPNCallbacks[3] = Http2Session::ALPNCallback;
}

bool
SpdyInformation::ProtocolEnabled(uint32_t index) const
{
  MOZ_ASSERT(index < kCount, "index out of range");

  switch (index) {
  case 0:
    return gHttpHandler->IsSpdyV31Enabled();
  case 1:
    return gHttpHandler->IsHttp2Enabled();
  case 2:
  case 3:
    return gHttpHandler->IsHttp2DraftEnabled();
  }
  return false;
}

nsresult
SpdyInformation::GetNPNIndex(const nsACString &npnString,
                             uint32_t *result) const
{
  if (npnString.IsEmpty())
    return NS_ERROR_FAILURE;

  for (uint32_t index = 0; index < kCount; ++index) {
    if (npnString.Equals(VersionString[index])) {
      *result = index;
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}





SpdyPushCache::SpdyPushCache()
{
}

SpdyPushCache::~SpdyPushCache()
{
  mHashSpdy31.Clear();
  mHashHttp2.Clear();
}

bool
SpdyPushCache::RegisterPushedStreamSpdy31(nsCString key,
                                          SpdyPushedStream31 *stream)
{
  LOG3(("SpdyPushCache::RegisterPushedStreamSpdy31 %s 0x%X\n",
        key.get(), stream->StreamID()));
  if(mHashSpdy31.Get(key)) {
    LOG3(("SpdyPushCache::RegisterPushedStreamSpdy31 %s 0x%X duplicate key\n",
          key.get(), stream->StreamID()));
    return false;
  }
  mHashSpdy31.Put(key, stream);
  return true;
}

SpdyPushedStream31 *
SpdyPushCache::RemovePushedStreamSpdy31(nsCString key)
{
  SpdyPushedStream31 *rv = mHashSpdy31.Get(key);
  LOG3(("SpdyPushCache::RemovePushedStream %s 0x%X\n",
        key.get(), rv ? rv->StreamID() : 0));
  if (rv)
    mHashSpdy31.Remove(key);
  return rv;
}

bool
SpdyPushCache::RegisterPushedStreamHttp2(nsCString key,
                                         Http2PushedStream *stream)
{
  LOG3(("SpdyPushCache::RegisterPushedStreamHttp2 %s 0x%X\n",
        key.get(), stream->StreamID()));
  if(mHashHttp2.Get(key)) {
    LOG3(("SpdyPushCache::RegisterPushedStreamHttp2 %s 0x%X duplicate key\n",
          key.get(), stream->StreamID()));
    return false;
  }
  mHashHttp2.Put(key, stream);
  return true;
}

Http2PushedStream *
SpdyPushCache::RemovePushedStreamHttp2(nsCString key)
{
  Http2PushedStream *rv = mHashHttp2.Get(key);
  LOG3(("SpdyPushCache::RemovePushedStreamHttp2 %s 0x%X\n",
        key.get(), rv ? rv->StreamID() : 0));
  if (rv)
    mHashHttp2.Remove(key);
  return rv;
}
} 
} 

