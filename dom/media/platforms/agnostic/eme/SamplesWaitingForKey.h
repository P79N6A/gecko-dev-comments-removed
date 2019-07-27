





#ifndef SamplesWaitingForKey_h_
#define SamplesWaitingForKey_h_

#include "TaskQueue.h"
#include "PlatformDecoderModule.h"

namespace mozilla {

typedef nsTArray<uint8_t> CencKeyId;

class CDMProxy;



class SamplesWaitingForKey {
public:

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SamplesWaitingForKey)

  explicit SamplesWaitingForKey(MediaDataDecoder* aDecoder,
                                TaskQueue* aTaskQueue,
                                CDMProxy* aProxy);

  
  
  
  
  bool WaitIfKeyNotUsable(MediaRawData* aSample);

  void NotifyUsable(const CencKeyId& aKeyId);

  void Flush();

  void BreakCycles();

protected:
  ~SamplesWaitingForKey();

private:
  Mutex mMutex;
  nsRefPtr<MediaDataDecoder> mDecoder;
  nsRefPtr<TaskQueue> mTaskQueue;
  nsRefPtr<CDMProxy> mProxy;
  nsTArray<nsRefPtr<MediaRawData>> mSamples;
};

} 

#endif 
