





#ifndef GECKO_TASK_TRACER_H
#define GECKO_TASK_TRACER_H

#include "nsCOMPtr.h"
#include "mozilla/TimeStamp.h"















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

void StartLogging(mozilla::TimeStamp aStartTime);
void StopLogging();
nsTArray<nsCString>* GetLoggedData(TimeStamp aStartTime);





Task* CreateTracedTask(Task* aTask);

already_AddRefed<nsIRunnable> CreateTracedRunnable(nsIRunnable* aRunnable);

already_AddRefed<FakeTracedTask> CreateFakeTracedTask(int* aVptr);




void FreeTraceInfo();

} 
} 

#endif
