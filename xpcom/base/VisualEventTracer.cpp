



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

const uint32_t kBatchSize = 256;
const char kTypeChars[eventtracer::eLast] = {' ','N','S','W','E','D'};


mozilla::Monitor * gMonitor = nullptr;



bool volatile gInitialized = false;


bool volatile gCapture = false;


mozilla::TimeStamp * gProfilerStart;


mozilla::TimeDuration * gMaxBacklogTime;



class Record {
public:
  Record() 
    : mType(::mozilla::eventtracer::eNone)
    , mItem(nullptr)
    , mText(nullptr)
    , mText2(nullptr) 
  {
    MOZ_COUNT_CTOR(Record);
  } 

  Record& operator=(const Record & aOther)
  {
    mType = aOther.mType;
    mTime = aOther.mTime;
    mItem = aOther.mItem;
    mText = PL_strdup(aOther.mText);
    mText2 = aOther.mText2 ? PL_strdup(aOther.mText2) : nullptr;
    return *this;
  }

  ~Record() 
  {
    PL_strfree(mText2);
    PL_strfree(mText); 
    MOZ_COUNT_DTOR(Record);
  }

  uint32_t mType;
  TimeStamp mTime;
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
  RecordBatch(size_t aLength = kBatchSize,
              char * aThreadName = DupCurrentThreadName())
    : mRecordsHead(new Record[aLength])
    , mRecordsTail(mRecordsHead + aLength)
    , mNextRecord(mRecordsHead)
    , mNextBatch(nullptr)
    , mThreadNameCopy(aThreadName)
    , mClosed(false)
  {
    MOZ_COUNT_CTOR(RecordBatch);
  }

  ~RecordBatch()
  {
    delete [] mRecordsHead;
    PL_strfree(mThreadNameCopy);
    MOZ_COUNT_DTOR(RecordBatch);
  }

  void Close() { mClosed = true; }

  size_t Length() const { return mNextRecord - mRecordsHead; }
  bool CanBeDeleted(const TimeStamp& aUntil) const;

  static RecordBatch * Register();
  static void Close(void * data); 
  static RecordBatch * Clone(RecordBatch * aLog, const TimeStamp& aSince);
  static void Delete(RecordBatch * aLog);

  static RecordBatch * CloneLog();
  static void GCLog(const TimeStamp& aUntil);
  static void DeleteLog();

  Record * mRecordsHead;
  Record * mRecordsTail;
  Record * volatile mNextRecord;

  RecordBatch * mNextBatch;
  char * mThreadNameCopy;
  bool mClosed;
};



RecordBatch * volatile gLogHead = nullptr;
RecordBatch * volatile gLogTail = nullptr;



RecordBatch *
RecordBatch::Register()
{
  MonitorAutoLock mon(*gMonitor);

  if (!gInitialized)
    return nullptr;

  if (gLogHead)
    RecordBatch::GCLog(TimeStamp::Now() - *gMaxBacklogTime);

  RecordBatch * batch = new RecordBatch();
  if (!gLogHead)
    gLogHead = batch;
  else 
    gLogTail->mNextBatch = batch;
  gLogTail = batch;

  mon.Notify();
  return batch;
}

void
RecordBatch::Close(void * data)
{
  RecordBatch * batch = static_cast<RecordBatch*>(data);
  batch->Close();
}


RecordBatch *
RecordBatch::Clone(RecordBatch * aOther, const TimeStamp& aSince)
{
  if (!aOther)
    return nullptr;

  size_t length = aOther->Length();
  size_t min = 0;
  size_t max = length;
  Record * record = nullptr;

  
  size_t i;
  while (min < max) {
    i = (max + min) / 2;

    record = aOther->mRecordsHead + i;
    if (record->mTime >= aSince)
      max = i;
    else
      min = i+1;
  }
  i = (max + min) / 2;

  
  size_t toCopy = length - i;
  if (!toCopy)
    return RecordBatch::Clone(aOther->mNextBatch, aSince);

  
  RecordBatch * clone = new RecordBatch(toCopy, PL_strdup(aOther->mThreadNameCopy));
  for (; i < length; ++i) {
    record = aOther->mRecordsHead + i;
    *clone->mNextRecord = *record;
    ++clone->mNextRecord;
  }
  clone->mNextBatch = RecordBatch::Clone(aOther->mNextBatch, aSince);

  return clone;
}


void
RecordBatch::Delete(RecordBatch * aLog)
{
  while (aLog) {
    RecordBatch * batch = aLog;
    aLog = aLog->mNextBatch;
    delete batch;
  }
}


RecordBatch *
RecordBatch::CloneLog()
{
  TimeStamp startEpoch = *gProfilerStart;
  TimeStamp backlogEpoch = TimeStamp::Now() - *gMaxBacklogTime;

  TimeStamp since = (startEpoch > backlogEpoch) ? startEpoch : backlogEpoch;

  MonitorAutoLock mon(*gMonitor);

  return RecordBatch::Clone(gLogHead, since);
}


void
RecordBatch::GCLog(const TimeStamp& aUntil)
{
  
  gMonitor->AssertCurrentThreadOwns();

  RecordBatch *volatile * referer = &gLogHead;
  gLogTail = nullptr;

  RecordBatch * batch = *referer;
  while (batch) {
    if (batch->CanBeDeleted(aUntil)) {
      
      
      
      *referer = batch->mNextBatch;
      delete batch;
      batch = *referer;
    }
    else {
      
      
      gLogTail = batch;
      
      batch = batch->mNextBatch;
      
      
      referer = &((*referer)->mNextBatch);
    }
  }
}


void
RecordBatch::DeleteLog()
{
  RecordBatch * batch;
  {
    MonitorAutoLock mon(*gMonitor);
    batch = gLogHead;
    gLogHead = nullptr;
    gLogTail = nullptr;
  }

  RecordBatch::Delete(batch);
}

bool
RecordBatch::CanBeDeleted(const TimeStamp& aUntil) const
{
  if (mClosed) {
    
    
    
    

    if (!Length()) {
      
      return true;
    }

    if ((mNextRecord-1)->mTime <= aUntil) {
      
      
      return true;
    }
  }

  
  return false;
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
unsigned gThreadPrivateIndex;


bool CheckEventFilters(uint32_t aType, void * aItem, const char * aText)
{
  if (!gEventFilter)
    return true;

  if (aType == eName)
    return true;

  return gEventFilter->EventPasses(aText);
}

} 

#endif 


void Init()
{
#ifdef MOZ_VISUAL_EVENT_TRACER
  const char * logEvents = PR_GetEnv("MOZ_PROFILING_EVENTS");
  if (logEvents && *logEvents)
    gEventFilter = EventFilter::Build(logEvents);

  PRStatus status = PR_NewThreadPrivateIndex(&gThreadPrivateIndex, &RecordBatch::Close);
  if (status != PR_SUCCESS)
    return;

  gMonitor = new mozilla::Monitor("Profiler");
  if (!gMonitor)
    return;

  gProfilerStart = new mozilla::TimeStamp();
  gMaxBacklogTime = new mozilla::TimeDuration();

  gInitialized = true;
#endif
}


void Shutdown()
{
#ifdef MOZ_VISUAL_EVENT_TRACER
  gCapture = false;
  gInitialized = false;

  RecordBatch::DeleteLog();

  if (gMonitor) {
    delete gMonitor;
    gMonitor = nullptr;
  }

  if (gEventFilter) {
    delete gEventFilter;
    gEventFilter = nullptr;
  }

  if (gProfilerStart) {
    delete gProfilerStart;
    gProfilerStart = nullptr;
  }

  if (gMaxBacklogTime) {
    delete gMaxBacklogTime;
    gMaxBacklogTime = nullptr;
  }
#endif
}


void Mark(uint32_t aType, void * aItem, const char * aText, const char * aText2)
{
#ifdef MOZ_VISUAL_EVENT_TRACER
  if (!gInitialized || !gCapture)
    return;

  if (aType == eNone)
    return;

  if (!CheckEventFilters(aType, aItem, aText)) 
    return;

  RecordBatch * threadLogPrivate = static_cast<RecordBatch *>(
      PR_GetThreadPrivate(gThreadPrivateIndex));
  if (!threadLogPrivate) {
    threadLogPrivate = RecordBatch::Register();
    if (!threadLogPrivate)
      return;

    PR_SetThreadPrivate(gThreadPrivateIndex, threadLogPrivate);
  }

  Record * record = threadLogPrivate->mNextRecord;
  record->mType = aType;
  record->mTime = mozilla::TimeStamp::Now();
  record->mItem = aItem;
  record->mText = PL_strdup(aText);
  record->mText2 = aText2 ? PL_strdup(aText2) : nullptr;

  ++threadLogPrivate->mNextRecord;
  if (threadLogPrivate->mNextRecord == threadLogPrivate->mRecordsTail) {
    
    
    PR_SetThreadPrivate(gThreadPrivateIndex, nullptr);
  }
#endif
}


#ifdef MOZ_VISUAL_EVENT_TRACER



class VisualEventTracerLog : public nsIVisualEventTracerLog
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVISUALEVENTTRACERLOG

  VisualEventTracerLog(RecordBatch* aBatch)
    : mBatch(aBatch)
    , mProfilerStart(*gProfilerStart)
  {}

  virtual ~VisualEventTracerLog();

protected:
  RecordBatch * mBatch;
  TimeStamp mProfilerStart;
};

NS_IMPL_ISUPPORTS1(VisualEventTracerLog, nsIVisualEventTracerLog)

VisualEventTracerLog::~VisualEventTracerLog()
{
  RecordBatch::Delete(mBatch);
}

NS_IMETHODIMP
VisualEventTracerLog::GetJSONString(nsACString & _retval)
{
  nsCString buffer;

  buffer.Assign(NS_LITERAL_CSTRING("{\n\"version\": 1,\n\"records\":[\n"));

  RecordBatch * batch = mBatch;
  while (batch) {
    if (batch != mBatch) {
      
      buffer.Append(NS_LITERAL_CSTRING(",\n"));
    }

    buffer.Append(NS_LITERAL_CSTRING("{\"thread\":\""));
    buffer.Append(batch->mThreadNameCopy);
    buffer.Append(NS_LITERAL_CSTRING("\",\"log\":[\n"));

    static const int kBufferSize = 2048;
    char buf[kBufferSize];

    for (Record * record = batch->mRecordsHead;
         record < batch->mNextRecord;
         ++record) {

      
      
      
      uint32_t type = record->mType & 0xffffUL;
      uint32_t flags = record->mType >> 16;
      PR_snprintf(buf, kBufferSize,
        "{\"e\":\"%c\",\"t\":%f,\"f\":%d,\"i\":\"%p\",\"n\":\"%s%s\"}%s\n",
        kTypeChars[type],
        (record->mTime - mProfilerStart).ToMilliseconds(),
        flags,
        record->mItem,
        record->mText,
        record->mText2 ? record->mText2 : "",
        (record == batch->mNextRecord - 1) ? "" : ",");

      buffer.Append(buf);
    }

    buffer.Append(NS_LITERAL_CSTRING("]}\n"));

    RecordBatch * next = batch->mNextBatch;
    batch = next;
  }

  buffer.Append(NS_LITERAL_CSTRING("]}\n"));
  _retval.Assign(buffer);

  return NS_OK;
}

NS_IMPL_ISUPPORTS1(VisualEventTracer, nsIVisualEventTracer)

NS_IMETHODIMP
VisualEventTracer::Start(const uint32_t aMaxBacklogSeconds)
{
  if (!gInitialized)
    return NS_ERROR_UNEXPECTED;

  if (gCapture) {
    NS_WARNING("VisualEventTracer has already been started");
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  *gMaxBacklogTime = TimeDuration::FromMilliseconds(aMaxBacklogSeconds * 1000);

  *gProfilerStart = mozilla::TimeStamp::Now();
  {
    MonitorAutoLock mon(*gMonitor);
    RecordBatch::GCLog(*gProfilerStart);
  }
  gCapture = true;

  MOZ_EVENT_TRACER_MARK(this, "trace::start");

  return NS_OK;
}

NS_IMETHODIMP
VisualEventTracer::Stop()
{
  if (!gInitialized)
    return NS_ERROR_UNEXPECTED;

  if (!gCapture) {
    NS_WARNING("VisualEventTracer is not runing");
    return NS_ERROR_NOT_INITIALIZED;
  }

  MOZ_EVENT_TRACER_MARK(this, "trace::stop");

  gCapture = false;

  return NS_OK;
}

NS_IMETHODIMP
VisualEventTracer::Snapshot(nsIVisualEventTracerLog ** _result)
{
  if (!gInitialized)
    return NS_ERROR_UNEXPECTED;

  RecordBatch * batch = RecordBatch::CloneLog();

  nsRefPtr<VisualEventTracerLog> log = new VisualEventTracerLog(batch);
  log.forget(_result);

  return NS_OK;
}

#endif

} 
} 
