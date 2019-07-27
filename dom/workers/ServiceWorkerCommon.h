





#ifndef mozilla_dom_ServiceWorkerCommon_h
#define mozilla_dom_ServiceWorkerCommon_h

namespace mozilla {
namespace dom {



MOZ_BEGIN_ENUM_CLASS(WhichServiceWorker)
  INSTALLING_WORKER = 1,
  WAITING_WORKER    = 2,
  ACTIVE_WORKER     = 4,
MOZ_END_ENUM_CLASS(WhichServiceWorker)
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(WhichServiceWorker)

} 
} 

#endif 
