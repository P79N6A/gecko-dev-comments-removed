
























#ifndef mozilla_net_SpdyPush3_Public_h
#define mozilla_net_SpdyPush3_Public_h

#include "nsAutoPtr.h"
#include "nsDataHashtable.h"
#include "nsISupports.h"

class nsCString;

namespace mozilla {
namespace net {

class SpdyPushedStream3;


class SpdyPushCache3
{
public:
  SpdyPushCache3();
  virtual ~SpdyPushCache3();

  
  bool               RegisterPushedStream(nsCString key,
                                          SpdyPushedStream3 *stream);
  SpdyPushedStream3 *RemovePushedStream(nsCString key);
  SpdyPushedStream3 *GetPushedStream(nsCString key);

private:
  nsDataHashtable<nsCStringHashKey, SpdyPushedStream3 *> mHash;
};

} 
} 

#endif 
