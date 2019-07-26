





#ifndef RtspHandler_h
#define RtspHandler_h

#include "nsIProtocolHandler.h"
#include "nsCOMPtr.h"
#include "nsString.h"

namespace mozilla {
namespace net {


class RtspHandler : public nsIProtocolHandler
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROTOCOLHANDLER

  RtspHandler() { }
  ~RtspHandler() { }
  const static int32_t kDefaultRtspPort = 554;
};

} 
} 

#endif 
