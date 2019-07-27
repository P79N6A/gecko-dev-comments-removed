





#ifndef mozilla_dom_ServiceWorkerCommon_h
#define mozilla_dom_ServiceWorkerCommon_h

namespace mozilla {
namespace dom {



enum class WhichServiceWorker {
  INSTALLING_WORKER = 1,
  WAITING_WORKER    = 2,
  ACTIVE_WORKER     = 4,
};
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(WhichServiceWorker)

} 
} 

#endif
