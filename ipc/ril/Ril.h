





#ifndef mozilla_ipc_Ril_h
#define mozilla_ipc_Ril_h 1

#include "nsError.h"

namespace mozilla {

namespace dom {
namespace workers {

class WorkerCrossThreadDispatcher;

} 
} 

namespace ipc {

class RilWorker final
{
public:
  static nsresult Register(
    unsigned int aClientId,
    mozilla::dom::workers::WorkerCrossThreadDispatcher* aDispatcher);

  static void Shutdown();
};

} 
} 

#endif 
