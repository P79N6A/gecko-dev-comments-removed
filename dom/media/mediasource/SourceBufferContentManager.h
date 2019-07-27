





#ifndef MOZILLA_SOURCEBUFFERCONTENTMANAGER_H_
#define MOZILLA_SOURCEBUFFERCONTENTMANAGER_H_

#include "MediaData.h"
#include "MediaPromise.h"
#include "MediaSourceDecoder.h"
#include "SourceBuffer.h"
#include "TimeUnits.h"
#include "nsString.h"

namespace mozilla {

using media::TimeUnit;
using media::TimeIntervals;

class SourceBufferContentManager {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SourceBufferContentManager);

  typedef MediaPromise<bool, nsresult,  true> AppendPromise;

  static already_AddRefed<SourceBufferContentManager>
  CreateManager(MediaSourceDecoder* aParentDecoder, const nsACString& aType);

  
  
  virtual bool
  AppendData(MediaLargeByteBuffer* aData, TimeUnit aTimestampOffset) = 0;

  
  
  
  virtual nsRefPtr<AppendPromise> BufferAppend() = 0;

  
  virtual void AbortAppendData() = 0;

  
  
  
  virtual void ResetParserState() = 0;

  
  
  virtual bool RangeRemoval(TimeUnit aStart, TimeUnit aEnd) = 0;

  enum class EvictDataResult : int8_t
  {
    NO_DATA_EVICTED,
    DATA_EVICTED,
    CANT_EVICT,
  };

  
  
  
  
  virtual EvictDataResult
  EvictData(TimeUnit aPlaybackTime, uint32_t aThreshold, TimeUnit* aBufferStartTime) = 0;

  
  virtual void EvictBefore(TimeUnit aTime) = 0;

  
  
  
  virtual media::TimeIntervals Buffered() = 0;

  
  virtual int64_t GetSize() = 0;

  
  virtual void Ended() = 0;

  
  virtual void Detach() = 0;

#if defined(DEBUG)
  virtual void Dump(const char* aPath) { }
#endif

protected:
  virtual ~SourceBufferContentManager() { }
};

} 
#endif
