





#ifndef mozilla_net_ChildDNSService_h
#define mozilla_net_ChildDNSService_h


#include "nsPIDNSService.h"
#include "nsIObserver.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace net {

class ChildDNSService MOZ_FINAL
  : public nsPIDNSService
  , public nsIObserver
{
public:
  
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSPIDNSSERVICE
  NS_DECL_NSIDNSSERVICE
  NS_DECL_NSIOBSERVER

  ChildDNSService();

  static ChildDNSService* GetSingleton();

private:
  virtual ~ChildDNSService();

  bool mFirstTime;
  bool mOffline;
  bool mDisablePrefetch;
};

} 
} 
#endif 
