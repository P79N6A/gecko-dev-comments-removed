























#ifndef mozilla_net_SpdyPush_Public_h
#define mozilla_net_SpdyPush_Public_h

#include "nsAutoPtr.h"
#include "nsDataHashtable.h"
#include "nsISupports.h"

class nsCString;

namespace mozilla {
namespace net {

class SpdyPushedStream31;
class Http2PushedStream;


class SpdyPushCache
{
public:
  
  SpdyPushCache();
  virtual ~SpdyPushCache();


public:
  bool               RegisterPushedStreamSpdy31(nsCString key,
                                                SpdyPushedStream31 *stream);
  SpdyPushedStream31 *RemovePushedStreamSpdy31(nsCString key);
private:
  nsDataHashtable<nsCStringHashKey, SpdyPushedStream31 *> mHashSpdy31;


public:
  bool               RegisterPushedStreamHttp2(nsCString key,
                                               Http2PushedStream *stream);
  Http2PushedStream *RemovePushedStreamHttp2(nsCString key);
private:
  nsDataHashtable<nsCStringHashKey, Http2PushedStream *> mHashHttp2;
};

} 
} 

#endif 
