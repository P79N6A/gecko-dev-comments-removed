





#if !defined(SampleSink_h_)
#define SampleSink_h_

#include "BaseFilter.h"
#include "DirectShowUtils.h"
#include "nsAutoPtr.h"
#include "mozilla/RefPtr.h"
#include "mozilla/ReentrantMonitor.h"

namespace mozilla {

class SampleSink {
public:
  SampleSink();
  virtual ~SampleSink();

  
  
  void SetAudioFormat(const WAVEFORMATEX* aInFormat);

  
  void GetAudioFormat(WAVEFORMATEX* aOutFormat);

  
  
  
  
  HRESULT Receive(IMediaSample* aSample);

  
  
  HRESULT Extract(RefPtr<IMediaSample>& aOutSample);

  
  
  void Flush();

  
  
  void Reset();

  
  void SetEOS();

  
  bool AtEOS();

private:
  
  ReentrantMonitor mMonitor;
  RefPtr<IMediaSample> mSample;

  
  WAVEFORMATEX mAudioFormat;

  bool mIsFlushing;
  bool mAtEOS;
};

} 
#endif 
