





#ifndef mozilla_ipc_Ril_h
#define mozilla_ipc_Ril_h 1

#include "nsAutoPtr.h"
#include "nsError.h"
#include "nsTArray.h"

namespace mozilla {

namespace dom {
namespace workers {

class WorkerCrossThreadDispatcher;

} 
} 

namespace ipc {

class RilConsumer;

class RilWorker final
{
public:
  static nsresult Register(
    unsigned int aClientId,
    mozilla::dom::workers::WorkerCrossThreadDispatcher* aDispatcher);

  static void Shutdown();

private:
  class RegisterConsumerTask;
  class UnregisterConsumerTask;

  RilWorker(mozilla::dom::workers::WorkerCrossThreadDispatcher* aDispatcher);

  nsresult RegisterConsumer(unsigned int aClientId);
  void     UnregisterConsumer(unsigned int aClientId);

  static nsTArray<nsAutoPtr<RilWorker>> sRilWorkers;

  nsRefPtr<mozilla::dom::workers::WorkerCrossThreadDispatcher> mDispatcher;
};

} 
} 

#endif 
