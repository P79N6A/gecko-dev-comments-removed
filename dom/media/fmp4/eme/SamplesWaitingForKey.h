





#ifndef SamplesWaitingForKey_h_
#define SamplesWaitingForKey_h_

#include "mp4_demuxer/DecoderData.h"
#include "MediaTaskQueue.h"
#include "PlatformDecoderModule.h"

namespace mozilla {

typedef nsTArray<uint8_t> CencKeyId;

class CDMProxy;



class SamplesWaitingForKey {
  typedef mp4_demuxer::MP4Sample MP4Sample;
public:

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SamplesWaitingForKey)

  explicit SamplesWaitingForKey(MediaDataDecoder* aDecoder,
                                MediaTaskQueue* aTaskQueue,
                                CDMProxy* aProxy);

  
  
  
  
  bool WaitIfKeyNotUsable(MP4Sample* aSample);

  void NotifyUsable(const CencKeyId& aKeyId);

  void Flush();

  void BreakCycles();

protected:
  ~SamplesWaitingForKey();

private:
  Mutex mMutex;
  nsRefPtr<MediaDataDecoder> mDecoder;
  nsRefPtr<MediaTaskQueue> mTaskQueue;
  nsRefPtr<CDMProxy> mProxy;
  nsTArray<nsAutoPtr<MP4Sample>> mSamples;
};

} 

#endif 
