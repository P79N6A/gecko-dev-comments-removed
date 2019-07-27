





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
  typedef AppendPromise RangeRemovalPromise;

  static already_AddRefed<SourceBufferContentManager>
  CreateManager(dom::SourceBuffer* aParent, MediaSourceDecoder* aParentDecoder,
                const nsACString& aType);

  
  
  virtual bool
  AppendData(MediaByteBuffer* aData, TimeUnit aTimestampOffset) = 0;

  
  
  
  virtual nsRefPtr<AppendPromise> BufferAppend() = 0;

  
  virtual void AbortAppendData() = 0;

  
  
  
  virtual void ResetParserState() = 0;

  
  
  virtual nsRefPtr<RangeRemovalPromise> RangeRemoval(TimeUnit aStart, TimeUnit aEnd) = 0;

  enum class EvictDataResult : int8_t
  {
    NO_DATA_EVICTED,
    DATA_EVICTED,
    CANT_EVICT,
    BUFFER_FULL,
  };

  
  
  
  
  virtual EvictDataResult
  EvictData(TimeUnit aPlaybackTime, uint32_t aThreshold, TimeUnit* aBufferStartTime) = 0;

  
  virtual void EvictBefore(TimeUnit aTime) = 0;

  
  
  
  virtual media::TimeIntervals Buffered() = 0;

  
  virtual int64_t GetSize() = 0;

  
  virtual void Ended() = 0;

  
  virtual void Detach() = 0;

  
  
  enum class AppendState : int32_t
  {
    WAITING_FOR_SEGMENT,
    PARSING_INIT_SEGMENT,
    PARSING_MEDIA_SEGMENT,
  };

  virtual AppendState GetAppendState()
  {
    return AppendState::WAITING_FOR_SEGMENT;
  }

  virtual void SetGroupStartTimestamp(const TimeUnit& aGroupStartTimestamp) {}
  virtual void RestartGroupStartTimestamp() {}

#if defined(DEBUG)
  virtual void Dump(const char* aPath) { }
#endif

protected:
  virtual ~SourceBufferContentManager() { }
};

} 
#endif
