





#if !defined(AudioSinkInputPin_h_)
#define AudioSinkInputPin_h_

#include "BaseInputPin.h"
#include "DirectShowUtils.h"
#include "mozilla/RefPtr.h"
#include "nsAutoPtr.h"

namespace mozilla {

namespace media {
  class MediaType;
}

class AudioSinkFilter;
class SampleSink;




class AudioSinkInputPin: public mozilla::media::BaseInputPin
{
public:
  AudioSinkInputPin(wchar_t* aObjectName,
                    AudioSinkFilter* aFilter,
                    mozilla::CriticalSection* aLock,
                    HRESULT* aOutResult);
  virtual ~AudioSinkInputPin();

  HRESULT GetMediaType (IN int iPos, OUT mozilla::media::MediaType * pmt);
  HRESULT CheckMediaType (IN const mozilla::media::MediaType * pmt);
  STDMETHODIMP Receive (IN IMediaSample *);
  STDMETHODIMP BeginFlush() override;
  STDMETHODIMP EndFlush() override;

  
  
  
  
  
  
  STDMETHODIMP NewSegment(REFERENCE_TIME tStart,
                          REFERENCE_TIME tStop,
                          double dRate) override;

  STDMETHODIMP EndOfStream() override;

  
  
  
  already_AddRefed<IMediaSeeking> GetConnectedPinSeeking();

  SampleSink* GetSampleSink();

private:
  AudioSinkFilter* GetAudioSinkFilter();

  
  
  HRESULT SetAbsoluteMediaTime(IMediaSample* aSample);

  nsAutoPtr<SampleSink> mSampleSink;

  
  REFERENCE_TIME mSegmentStartTime;
};

} 
#endif 
