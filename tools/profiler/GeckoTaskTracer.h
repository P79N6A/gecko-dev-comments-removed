





#ifndef GECKO_TASK_TRACER_H
#define GECKO_TASK_TRACER_H

#include "nsCOMPtr.h"















class Task;
class nsIRunnable;
class nsCString;
template <class> class nsTArray;

namespace mozilla {

class TimeStamp;

namespace tasktracer {

enum {
  FORKED_AFTER_NUWA = 1 << 0
};
void InitTaskTracer(uint32_t aFlags = 0);
void ShutdownTaskTracer();

class FakeTracedTask;

enum SourceEventType {
  Unknown = 0,
  Touch,
  Mouse,
  Key,
  Bluetooth,
  Unixsocket,
  Wifi
};

class AutoSourceEvent
{
public:
  AutoSourceEvent(SourceEventType aType);
  ~AutoSourceEvent();
};



void AddLabel(const char* aFormat, ...);

void StartLogging();
void StopLogging();
nsTArray<nsCString>* GetLoggedData(TimeStamp aStartTime);


const PRTime GetStartTime();





Task* CreateTracedTask(Task* aTask);

already_AddRefed<nsIRunnable> CreateTracedRunnable(nsIRunnable* aRunnable);

already_AddRefed<FakeTracedTask> CreateFakeTracedTask(int* aVptr);




void FreeTraceInfo();

const char* GetJSLabelPrefix();

} 
} 

#endif
