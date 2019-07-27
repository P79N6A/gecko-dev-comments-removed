





#ifndef MOZILLA_SOURCEBUFFERCONTENTMANAGER_H_
#define MOZILLA_SOURCEBUFFERCONTENTMANAGER_H_

#include "MediaData.h"
#include "MediaPromise.h"
#include "MediaSourceDecoder.h"
#include "SourceBuffer.h"
#include "TimeUnits.h"
#include "nsString.h"

namespace mozilla {

class SourceBufferContentManager {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SourceBufferContentManager);

  typedef MediaPromise<bool, nsresult,  true> AppendPromise;

  static already_AddRefed<SourceBufferContentManager>
  CreateManager(MediaSourceDecoder* aParentDecoder, const nsACString& aType);

  
  
  
  virtual nsRefPtr<AppendPromise>
  AppendData(MediaLargeByteBuffer* aData, int64_t aTimestampOffset ) = 0;

  
  virtual void AbortAppendData() = 0;

  
  
  
  virtual void ResetParserState() = 0;

  
  
  virtual bool RangeRemoval(mozilla::media::TimeUnit aStart,
                            mozilla::media::TimeUnit aEnd) = 0;

  enum class EvictDataResult : int8_t
  {
    NO_DATA_EVICTED,
    DATA_EVICTED,
    CANT_EVICT,
  };

  
  
  
  
  virtual EvictDataResult
  EvictData(double aPlaybackTime, uint32_t aThreshold, double* aBufferStartTime) = 0;

  
  virtual void EvictBefore(double aTime) = 0;

  
  
  
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
