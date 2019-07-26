





#ifndef GECKO_TASK_TRACER_IMPL_H
#define GECKO_TASK_TRACER_IMPL_H

#include "GeckoTaskTracer.h"

namespace mozilla {
namespace tasktracer {

struct TraceInfo
{
  TraceInfo(uint32_t aThreadId) : mCurTraceSourceId(0)
                                , mCurTaskId(0)
                                , mSavedCurTraceSourceId(0)
                                , mSavedCurTaskId(0)
                                , mCurTraceSourceType(UNKNOWN)
                                , mSavedCurTraceSourceType(UNKNOWN)
                                , mThreadId(aThreadId)
                                , mLastUniqueTaskId(0)
  {
    MOZ_COUNT_CTOR(TraceInfo);
  }

  ~TraceInfo() { MOZ_COUNT_DTOR(TraceInfo); }

  uint64_t mCurTraceSourceId;
  uint64_t mCurTaskId;
  uint64_t mSavedCurTraceSourceId;
  uint64_t mSavedCurTaskId;
  SourceEventType mCurTraceSourceType;
  SourceEventType mSavedCurTraceSourceType;
  uint32_t mThreadId;
  uint32_t mLastUniqueTaskId;
};

void InitTaskTracer();
void ShutdownTaskTracer();


TraceInfo* GetOrCreateTraceInfo();

uint64_t GenNewUniqueTaskId();

class AutoSaveCurTraceInfo
{
public:
  AutoSaveCurTraceInfo();
  ~AutoSaveCurTraceInfo();
};

void SetCurTraceInfo(uint64_t aSourceEventId, uint64_t aParentTaskId,
                     SourceEventType aSourceEventType);

void GetCurTraceInfo(uint64_t* aOutSourceEventId, uint64_t* aOutParentTaskId,
                     SourceEventType* aOutSourceEventType);




enum ActionType {
  ACTION_DISPATCH = 0,
  ACTION_BEGIN,
  ACTION_END,
  ACTION_ADD_LABEL,
  ACTION_GET_VTABLE
};

void LogDispatch(uint64_t aTaskId, uint64_t aParentTaskId,
                 uint64_t aSourceEventId, SourceEventType aSourceEventType);

void LogBegin(uint64_t aTaskId, uint64_t aSourceEventId);

void LogEnd(uint64_t aTaskId, uint64_t aSourceEventId);

void LogVirtualTablePtr(uint64_t aTaskId, uint64_t aSourceEventId, int* aVptr);

} 
} 

#endif
