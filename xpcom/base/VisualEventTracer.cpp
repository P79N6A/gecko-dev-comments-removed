



#include "mozilla/VisualEventTracer.h"
#include "mozilla/Monitor.h"
#include "mozilla/TimeStamp.h"
#include "nscore.h"
#include "prthread.h"
#include "prprf.h"
#include "prio.h"
#include "prenv.h"
#include "plstr.h"
#include "nsThreadUtils.h"

namespace mozilla { namespace eventtracer {

#ifdef MOZ_VISUAL_EVENT_TRACER

namespace {

const uint32_t kBatchSize = 0x1000;
const char kTypeChars[eventtracer::eLast] = {' ','N','S','W','E','D'};


mozilla::Monitor * gMonitor = nullptr;



bool gInitialized = false;


class Record {
public:
  Record() 
    : mType(::mozilla::eventtracer::eNone)
    , mTime(0)
    , mItem(nullptr)
    , mText(nullptr)
    , mText2(nullptr) 
  {
    MOZ_COUNT_CTOR(Record);
  } 
  ~Record() 
  {
    PL_strfree(mText); 
    PL_strfree(mText2);
    MOZ_COUNT_DTOR(Record);
  }

  uint32_t mType;
  double mTime;
  void * mItem;
  char * mText;
  char * mText2;
};

char * DupCurrentThreadName()
{
  if (NS_IsMainThread())
    return PL_strdup("Main Thread");

  PRThread * currentThread = PR_GetCurrentThread();
  const char * name = PR_GetThreadName(currentThread);
  if (name)
    return PL_strdup(name);

  char buffer[128];
  PR_snprintf(buffer, 127, "Nameless %p", currentThread);

  return PL_strdup(buffer);
}


class RecordBatch {
public:
  RecordBatch()
    : mRecordsHead(new Record[kBatchSize])
    , mRecordsTail(mRecordsHead + kBatchSize)
    , mNextRecord(mRecordsHead)
    , mNextBatch(nullptr)
    , mThreadNameCopy(DupCurrentThreadName())
  {
    MOZ_COUNT_CTOR(RecordBatch);
  }

  ~RecordBatch()
  {
    delete [] mRecordsHead;
    PL_strfree(mThreadNameCopy);
    MOZ_COUNT_DTOR(RecordBatch);
  }

  static void FlushBatch(void * aData);

  Record * mRecordsHead;
  Record * mRecordsTail;
  Record * mNextRecord;

  RecordBatch * mNextBatch;
  char * mThreadNameCopy;
};



RecordBatch * gLogHead = nullptr;
RecordBatch * gLogTail = nullptr;


void
RecordBatch::FlushBatch(void * aData)
{
  RecordBatch * threadLogPrivate = static_cast<RecordBatch *>(aData);

  MonitorAutoLock mon(*gMonitor);

  if (!gInitialized) {
    delete threadLogPrivate;
    return;
  }

  if (!gLogHead)
    gLogHead = threadLogPrivate;
  else 
    gLogTail->mNextBatch = threadLogPrivate;
  gLogTail = threadLogPrivate;

  mon.Notify();  
}


class EventFilter
{
public:
  static EventFilter * Build(const char * filterVar);
  bool EventPasses(const char * eventName);

  ~EventFilter()
  {
    delete mNext;
    PL_strfree(mFilter);
    MOZ_COUNT_DTOR(EventFilter);
  }

private:
  EventFilter(const char * eventName, EventFilter * next)
    : mFilter(PL_strdup(eventName))
    , mNext(next)
  {
    MOZ_COUNT_CTOR(EventFilter);
  }

  char * mFilter;
  EventFilter * mNext;
};


EventFilter *
EventFilter::Build(const char * filterVar)
{
  if (!filterVar || !*filterVar)
    return nullptr;

  

  
  char eventName[64];
  int evlen = strlen(filterVar), pos = 0, count, delta = 0;

  
  count = sscanf(filterVar, "%63[^,]%n", eventName, &delta);
  if (count == 0) 
    return nullptr;

  pos = delta;

  
  count = sscanf(filterVar + pos, " , %n", &delta);
  if (count != EOF)
    pos += delta;

  
  
  
  return new EventFilter(eventName, Build(filterVar + pos));
}

bool
EventFilter::EventPasses(const char * eventName)
{
  if (!strcmp(eventName, mFilter))
    return true;

  if (mNext)
    return mNext->EventPasses(eventName);

  return false;
}


bool gStopFlushingThread = false;



EventFilter * gEventFilter = nullptr;
const char * gLogFilePath = nullptr;
PRThread * gFlushingThread = nullptr;
unsigned gThreadPrivateIndex;
mozilla::TimeStamp gProfilerStart;






void FlushingThread(void * aArg)
{
  PRFileDesc * logFile = PR_Open(gLogFilePath, 
                         PR_WRONLY | PR_TRUNCATE | PR_CREATE_FILE,
                         0644);

  MonitorAutoLock mon(*gMonitor);

  if (!logFile) {
    gInitialized = false;
    return;
  }

  int32_t rv;
  bool ioError = false;

  const char logHead[] = "{\n\"version\": 1,\n\"records\":[\n";
  rv = PR_Write(logFile, logHead, sizeof(logHead) - 1);
  ioError |= (rv < 0);

  bool firstBatch = true;
  while (!gStopFlushingThread || gLogHead) {
    if (ioError) {
      gInitialized = false;
      break;
    }

    mon.Wait();

    
    RecordBatch * batch = gLogHead;
    gLogHead = nullptr;
    gLogTail = nullptr;

    MonitorAutoUnlock unlock(*gMonitor); 

    while (batch) {
      if (!firstBatch) {
        const char threadDelimiter[] = ",\n";
        rv = PR_Write(logFile, threadDelimiter, sizeof(threadDelimiter) - 1);
        ioError |= (rv < 0);
      }
      firstBatch = false;

      static const int kBufferSize = 2048;
      char buf[kBufferSize];

      PR_snprintf(buf, kBufferSize, "{\"thread\":\"%s\",\"log\":[\n", 
                  batch->mThreadNameCopy);

      rv = PR_Write(logFile, buf, strlen(buf));
      ioError |= (rv < 0);

      for (Record * record = batch->mRecordsHead;
           record < batch->mNextRecord && !ioError;
           ++record) {

        
        
        
        uint32_t type = record->mType & 0xffffUL;
        uint32_t flags = record->mType >> 16;
        PR_snprintf(buf, kBufferSize, 
          "{\"e\":\"%c\",\"t\":%f,\"f\":%d,\"i\":\"%p\",\"n\":\"%s%s\"}%s\n",
          kTypeChars[type],
          record->mTime,
          flags,
          record->mItem,
          record->mText,
          record->mText2 ? record->mText2 : "",
          (record == batch->mNextRecord - 1) ? "" : ",");

        rv = PR_Write(logFile, buf, strlen(buf));
        ioError |= (rv < 0);
      }

      const char threadTail[] = "]}\n";
      rv = PR_Write(logFile, threadTail, sizeof(threadTail) - 1);
      ioError |= (rv < 0);

      RecordBatch * next = batch->mNextBatch;
      delete batch;
      batch = next;
    }
  }

  const char logTail[] = "]}\n";
  rv = PR_Write(logFile, logTail, sizeof(logTail) - 1);
  ioError |= (rv < 0);

  PR_Close(logFile);

  if (ioError)
    PR_Delete(gLogFilePath);
}


bool CheckEventFilters(uint32_t aType, void * aItem, const char * aText)
{
  if (!gEventFilter)
    return true;

  if (aType == eName)
    return true;

  if (aItem == gFlushingThread) 
    return true;

  return gEventFilter->EventPasses(aText);
}

} 

#endif 


void Init()
{
#ifdef MOZ_VISUAL_EVENT_TRACER
  const char * logFile = PR_GetEnv("MOZ_PROFILING_FILE");
  if (!logFile || !*logFile)
    return;

  gLogFilePath = logFile;

  const char * logEvents = PR_GetEnv("MOZ_PROFILING_EVENTS");
  if (logEvents && *logEvents)
    gEventFilter = EventFilter::Build(logEvents);

  gProfilerStart = mozilla::TimeStamp::Now();

  PRStatus status = PR_NewThreadPrivateIndex(&gThreadPrivateIndex, 
                                             &RecordBatch::FlushBatch);
  if (status != PR_SUCCESS)
    return;

  gMonitor = new mozilla::Monitor("Profiler");
  if (!gMonitor)
    return;

  gFlushingThread = PR_CreateThread(PR_USER_THREAD, 
                                    &FlushingThread,
                                    nullptr,
                                    PR_PRIORITY_LOW,
                                    PR_LOCAL_THREAD,
                                    PR_JOINABLE_THREAD,
                                    32768);
  if (!gFlushingThread)
    return;
    
  gInitialized = true;

  MOZ_EVENT_TRACER_NAME_OBJECT(gFlushingThread, "Profiler");
  MOZ_EVENT_TRACER_MARK(gFlushingThread, "Profiling Start");
#endif
}


void Shutdown()
{
#ifdef MOZ_VISUAL_EVENT_TRACER
  MOZ_EVENT_TRACER_MARK(gFlushingThread, "Profiling End");

  
  

  
  PR_SetThreadPrivate(gThreadPrivateIndex, nullptr);

  if (gFlushingThread) {
    {
      MonitorAutoLock mon(*gMonitor);
      gInitialized = false;
      gStopFlushingThread = true;
      mon.Notify();
    }

    PR_JoinThread(gFlushingThread);
    gFlushingThread = nullptr;
  }

  if (gMonitor) {
    delete gMonitor;
    gMonitor = nullptr;
  }

  if (gEventFilter) {
    delete gEventFilter;
    gEventFilter = nullptr;
  }
#endif
}


void Mark(uint32_t aType, void * aItem, const char * aText, const char * aText2)
{
#ifdef MOZ_VISUAL_EVENT_TRACER
  if (!gInitialized)
    return;

  if (aType == eNone)
    return;

  if (!CheckEventFilters(aType, aItem, aText)) 
    return;

  RecordBatch * threadLogPrivate = static_cast<RecordBatch *>(
      PR_GetThreadPrivate(gThreadPrivateIndex));
  if (!threadLogPrivate) {
    
    threadLogPrivate = new RecordBatch();
    PR_SetThreadPrivate(gThreadPrivateIndex, threadLogPrivate);
  }

  Record * record = threadLogPrivate->mNextRecord;
  record->mType = aType;
  record->mTime = (mozilla::TimeStamp::Now() - gProfilerStart).ToMilliseconds();
  record->mItem = aItem;
  record->mText = PL_strdup(aText);
  record->mText2 = aText2 ? PL_strdup(aText2) : nullptr;

  ++threadLogPrivate->mNextRecord;
  if (threadLogPrivate->mNextRecord == threadLogPrivate->mRecordsTail) {
    
    PR_SetThreadPrivate(gThreadPrivateIndex, nullptr);
  }
#endif
}

} 
} 
