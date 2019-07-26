



#ifndef mozilla_net_StreamingProtocolControllerService_h
#define mozilla_net_StreamingProtocolControllerService_h

#include "mozilla/StaticPtr.h"
#include "nsIStreamingProtocolService.h"
#include "nsIStreamingProtocolController.h"
#include "nsCOMPtr.h"
#include "nsIChannel.h"

namespace mozilla {
namespace net {




class StreamingProtocolControllerService : public nsIStreamingProtocolControllerService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMINGPROTOCOLCONTROLLERSERVICE

  StreamingProtocolControllerService() {};
  virtual ~StreamingProtocolControllerService() {};
  static already_AddRefed<StreamingProtocolControllerService> GetInstance();
};
} 
} 

#endif 
