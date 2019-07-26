





#ifndef GECKO_TASK_TRACER_H
#define GECKO_TASK_TRACER_H

#include "nsCOMPtr.h"















class Task;
class nsIRunnable;

namespace mozilla {
namespace tasktracer {

class FakeTracedTask;

enum SourceEventType {
  UNKNOWN = 0,
  TOUCH,
  MOUSE,
  KEY,
  BLUETOOTH,
  UNIXSOCKET,
  WIFI
};

class AutoSourceEvent
{
public:
  AutoSourceEvent(SourceEventType aType);
  ~AutoSourceEvent();
};



void AddLabel(const char* aFormat, ...);





Task* CreateTracedTask(Task* aTask);

already_AddRefed<nsIRunnable> CreateTracedRunnable(nsIRunnable* aRunnable);

FakeTracedTask* CreateFakeTracedTask(int* aVptr);




void FreeTraceInfo();

} 
} 

#endif
