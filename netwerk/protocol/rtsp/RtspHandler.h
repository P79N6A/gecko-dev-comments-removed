





#ifndef RtspHandler_h
#define RtspHandler_h

#include "nsIProtocolHandler.h"
#include "nsCOMPtr.h"
#include "nsString.h"

namespace mozilla {
namespace net {


class RtspHandler MOZ_FINAL : public nsIProtocolHandler
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROTOCOLHANDLER

  RtspHandler() { }
  const static int32_t kDefaultRtspPort = 554;

protected:
  ~RtspHandler() { }
};

} 
} 

#endif 
