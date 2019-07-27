





#include "AudioSinkInputPin.h"
#include "AudioSinkFilter.h"
#include "SampleSink.h"
#include "mozilla/Logging.h"

#include <wmsdkidl.h>

using namespace mozilla::media;

namespace mozilla {

PRLogModuleInfo* GetDirectShowLog();
#define LOG(...) MOZ_LOG(GetDirectShowLog(), mozilla::LogLevel::Debug, (__VA_ARGS__))

AudioSinkInputPin::AudioSinkInputPin(wchar_t* aObjectName,
                                     AudioSinkFilter* aFilter,
                                     mozilla::CriticalSection* aLock,
                                     HRESULT* aOutResult)
  : BaseInputPin(aObjectName, aFilter, aLock, aOutResult, aObjectName),
    mSegmentStartTime(0)
{
  MOZ_COUNT_CTOR(AudioSinkInputPin);
  mSampleSink = new SampleSink();
}

AudioSinkInputPin::~AudioSinkInputPin()
{
  MOZ_COUNT_DTOR(AudioSinkInputPin);
}

HRESULT
AudioSinkInputPin::GetMediaType(int aPosition, MediaType* aOutMediaType)
{
  NS_ENSURE_TRUE(aPosition >= 0, E_INVALIDARG);
  NS_ENSURE_TRUE(aOutMediaType, E_POINTER);

  if (aPosition > 0) {
    return S_FALSE;
  }

  
  
  aOutMediaType->SetType(&MEDIATYPE_Audio);
  aOutMediaType->SetSubtype(&MEDIASUBTYPE_PCM);
  aOutMediaType->SetType(&FORMAT_WaveFormatEx);
  aOutMediaType->SetTemporalCompression(FALSE);

  return S_OK;
}

HRESULT
AudioSinkInputPin::CheckMediaType(const MediaType* aMediaType)
{
  if (!aMediaType) {
    return E_INVALIDARG;
  }

  GUID majorType = *aMediaType->Type();
  if (majorType != MEDIATYPE_Audio && majorType != WMMEDIATYPE_Audio) {
    return E_INVALIDARG;
  }

  if (*aMediaType->Subtype() != MEDIASUBTYPE_PCM) {
    return E_INVALIDARG;
  }

  if (*aMediaType->FormatType() != FORMAT_WaveFormatEx) {
    return E_INVALIDARG;
  }

  
  WAVEFORMATEX* wfx = (WAVEFORMATEX*)(aMediaType->pbFormat);
  GetSampleSink()->SetAudioFormat(wfx);

  return S_OK;
}

AudioSinkFilter*
AudioSinkInputPin::GetAudioSinkFilter()
{
  return reinterpret_cast<AudioSinkFilter*>(mFilter);
}

SampleSink*
AudioSinkInputPin::GetSampleSink()
{
  return mSampleSink;
}

HRESULT
AudioSinkInputPin::SetAbsoluteMediaTime(IMediaSample* aSample)
{
  HRESULT hr;
  REFERENCE_TIME start = 0, end = 0;
  hr = aSample->GetTime(&start, &end);
  NS_ENSURE_TRUE(SUCCEEDED(hr), E_FAIL);
  {
    CriticalSectionAutoEnter lock(*mLock);
    start += mSegmentStartTime;
    end += mSegmentStartTime;
  }
  hr = aSample->SetMediaTime(&start, &end);
  NS_ENSURE_TRUE(SUCCEEDED(hr), E_FAIL);
  return S_OK;
}

HRESULT
AudioSinkInputPin::Receive(IMediaSample* aSample )
{
  HRESULT hr;
  NS_ENSURE_TRUE(aSample, E_POINTER);

  hr = BaseInputPin::Receive(aSample);
  if (SUCCEEDED(hr) && hr != S_FALSE) { 
    
    
    
    
    
    hr = SetAbsoluteMediaTime(aSample);
    NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
    hr = GetSampleSink()->Receive(aSample);
    NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
  }
  return S_OK;
}

already_AddRefed<IMediaSeeking>
AudioSinkInputPin::GetConnectedPinSeeking()
{
  RefPtr<IPin> peer = GetConnected();
  if (!peer)
    return nullptr;
  RefPtr<IMediaSeeking> seeking;
  peer->QueryInterface(static_cast<IMediaSeeking**>(byRef(seeking)));
  return seeking.forget();
}

HRESULT
AudioSinkInputPin::BeginFlush()
{
  HRESULT hr = media::BaseInputPin::BeginFlush();
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  GetSampleSink()->Flush();

  return S_OK;
}

HRESULT
AudioSinkInputPin::EndFlush()
{
  HRESULT hr = media::BaseInputPin::EndFlush();
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  
  GetSampleSink()->Reset();

  return S_OK;
}

HRESULT
AudioSinkInputPin::EndOfStream(void)
{
  HRESULT hr = media::BaseInputPin::EndOfStream();
  if (FAILED(hr) || hr == S_FALSE) {
    
    return hr;
  }
  GetSampleSink()->SetEOS();

  return S_OK;
}


HRESULT
AudioSinkInputPin::NewSegment(REFERENCE_TIME tStart,
                              REFERENCE_TIME tStop,
                              double dRate)
{
  CriticalSectionAutoEnter lock(*mLock);
  
  
  mSegmentStartTime = tStart;
  return S_OK;
}

} 

