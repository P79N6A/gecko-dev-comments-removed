























#ifndef mozilla_net_SpdyPush_Public_h
#define mozilla_net_SpdyPush_Public_h

#include "nsAutoPtr.h"
#include "nsDataHashtable.h"
#include "nsISupports.h"

class nsCString;

namespace mozilla {
namespace net {

class SpdyPushedStream3;
class SpdyPushedStream31;


class SpdyPushCache
{
public:
  
  SpdyPushCache();
  virtual ~SpdyPushCache();


public:
  bool               RegisterPushedStreamSpdy3(nsCString key,
                                               SpdyPushedStream3 *stream);
  SpdyPushedStream3 *RemovePushedStreamSpdy3(nsCString key);

private:
  nsDataHashtable<nsCStringHashKey, SpdyPushedStream3 *> mHashSpdy3;


public:
  bool               RegisterPushedStreamSpdy31(nsCString key,
                                                SpdyPushedStream31 *stream);
  SpdyPushedStream31 *RemovePushedStreamSpdy31(nsCString key);
private:
  nsDataHashtable<nsCStringHashKey, SpdyPushedStream31 *> mHashSpdy31;
};

} 
} 

#endif 
