




#include "DirectShowReader.h"
#include "MediaDecoderReader.h"
#include "mozilla/RefPtr.h"
#include "dshow.h"
#include "AudioSinkFilter.h"
#include "SourceFilter.h"
#include "DirectShowUtils.h"
#include "SampleSink.h"
#include "MediaResource.h"
#include "VideoUtils.h"

namespace mozilla {


#ifdef PR_LOGGING

PRLogModuleInfo*
GetDirectShowLog() {
  static PRLogModuleInfo* log = nullptr;
  if (!log) {
    log = PR_NewLogModule("DirectShowDecoder");
  }
  return log;
}

#define LOG(...) PR_LOG(GetDirectShowLog(), PR_LOG_DEBUG, (__VA_ARGS__))

#else
#define LOG(...)
#endif

DirectShowReader::DirectShowReader(AbstractMediaDecoder* aDecoder)
  : MediaDecoderReader(aDecoder),
    mMP3FrameParser(aDecoder->GetResource()->GetLength()),
#ifdef DEBUG
    mRotRegister(0),
#endif
    mNumChannels(0),
    mAudioRate(0),
    mBytesPerSample(0),
    mDuration(0)
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
  MOZ_COUNT_CTOR(DirectShowReader);
}

DirectShowReader::~DirectShowReader()
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
  MOZ_COUNT_DTOR(DirectShowReader);
#ifdef DEBUG
  if (mRotRegister) {
    RemoveGraphFromRunningObjectTable(mRotRegister);
  }
#endif
}

nsresult
DirectShowReader::Init(MediaDecoderReader* aCloneDonor)
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
  return NS_OK;
}



static const GUID CLSID_MPEG_LAYER_3_DECODER_FILTER =
{ 0x38BE3000, 0xDBF4, 0x11D0, 0x86, 0x0E, 0x00, 0xA0, 0x24, 0xCF, 0xEF, 0x6D };

nsresult
DirectShowReader::ReadMetadata(VideoInfo* aInfo,
                               MetadataTags** aTags)
{
  MOZ_ASSERT(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  HRESULT hr;
  nsresult rv;

  
  
  hr = CoCreateInstance(CLSID_FilterGraph,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IGraphBuilder,
                        reinterpret_cast<void**>(static_cast<IGraphBuilder**>(byRef(mGraph))));
  NS_ENSURE_TRUE(SUCCEEDED(hr) && mGraph, NS_ERROR_FAILURE);

  #ifdef DEBUG
  
  
  
  
  hr = AddGraphToRunningObjectTable(mGraph, &mRotRegister);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  #endif

  
  hr = mGraph->QueryInterface(static_cast<IMediaControl**>(byRef(mControl)));
  NS_ENSURE_TRUE(SUCCEEDED(hr) && mControl, NS_ERROR_FAILURE);

  hr = mGraph->QueryInterface(static_cast<IMediaSeeking**>(byRef(mMediaSeeking)));
  NS_ENSURE_TRUE(SUCCEEDED(hr) && mMediaSeeking, NS_ERROR_FAILURE);

  
  
  

  
  mSourceFilter = new SourceFilter(MEDIATYPE_Stream, MEDIASUBTYPE_MPEG1Audio);
  NS_ENSURE_TRUE(mSourceFilter, NS_ERROR_FAILURE);

  rv = mSourceFilter->Init(mDecoder->GetResource());
  NS_ENSURE_SUCCESS(rv, rv);

  hr = mGraph->AddFilter(mSourceFilter, L"MozillaDirectShowSource");
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  
  RefPtr<IBaseFilter> demuxer;
  hr = CreateAndAddFilter(mGraph,
                          CLSID_MPEG1Splitter,
                          L"MPEG1Splitter",
                          byRef(demuxer));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  
  RefPtr<IBaseFilter> decoder;
  
  
  
  hr = CreateAndAddFilter(mGraph,
                          CLSID_MPEG_LAYER_3_DECODER_FILTER,
                          L"MPEG Layer 3 Decoder",
                          byRef(decoder));
  if (FAILED(hr)) {
    
    
    hr = AddMP3DMOWrapperFilter(mGraph, byRef(decoder));
    NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  }

  
  static const wchar_t* AudioSinkFilterName = L"MozAudioSinkFilter";
  mAudioSinkFilter = new AudioSinkFilter(AudioSinkFilterName, &hr);
  NS_ENSURE_TRUE(mAudioSinkFilter && SUCCEEDED(hr), NS_ERROR_FAILURE);
  hr = mGraph->AddFilter(mAudioSinkFilter, AudioSinkFilterName);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  
  hr = ConnectFilters(mGraph, mSourceFilter, demuxer);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = ConnectFilters(mGraph, demuxer, decoder);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = ConnectFilters(mGraph, decoder, mAudioSinkFilter);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  WAVEFORMATEX format;
  mAudioSinkFilter->GetSampleSink()->GetAudioFormat(&format);
  NS_ENSURE_TRUE(format.wFormatTag == WAVE_FORMAT_PCM, NS_ERROR_FAILURE);

  mInfo.mAudioChannels = mNumChannels = format.nChannels;
  mInfo.mAudioRate = mAudioRate = format.nSamplesPerSec;
  mBytesPerSample = format.wBitsPerSample / 8;
  mInfo.mHasAudio = true;
  mInfo.mHasVideo = false;

  *aInfo = mInfo;
  
  *aTags = nullptr;

  
  hr = mControl->Run();
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  DWORD seekCaps = 0;
  hr = mMediaSeeking->GetCapabilities(&seekCaps);
  bool canSeek = ((AM_SEEKING_CanSeekAbsolute & seekCaps) == AM_SEEKING_CanSeekAbsolute);
  if (!canSeek) {
    mDecoder->SetMediaSeekable(false);
  }

  int64_t duration = 0;
  hr = mMediaSeeking->GetDuration(&duration);
  if (SUCCEEDED(hr)) {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDecoder->SetMediaDuration(RefTimeToUsecs(duration));
  }

  LOG("Successfully initialized DirectShow MP3 decoder.");
  LOG("Channels=%u Hz=%u duration=%lld bytesPerSample=%d",
      mInfo.mAudioChannels,
      mInfo.mAudioRate,
      RefTimeToUsecs(duration),
      mBytesPerSample);

  return NS_OK;
}

inline float
UnsignedByteToAudioSample(uint8_t aValue)
{
  return aValue * (2.0f / UINT8_MAX) - 1.0f;
}

bool
DirectShowReader::Finish(HRESULT aStatus)
{
  MOZ_ASSERT(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  LOG("DirectShowReader::Finish(0x%x)", aStatus);
  mAudioQueue.Finish();
  
  RefPtr<IMediaEventSink> eventSink;
  HRESULT hr = mGraph->QueryInterface(static_cast<IMediaEventSink**>(byRef(eventSink)));
  if (SUCCEEDED(hr) && eventSink) {
    eventSink->Notify(EC_COMPLETE, aStatus, NULL);
  }
  return false;
}

bool
DirectShowReader::DecodeAudioData()
{
  MOZ_ASSERT(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  HRESULT hr;

  SampleSink* sink = mAudioSinkFilter->GetSampleSink();
  if (sink->AtEOS()) {
    
    return Finish(S_OK);
  }

  
  
  RefPtr<IMediaSample> sample;
  hr = sink->Extract(sample);
  if (FAILED(hr) || hr == S_FALSE) {
    return Finish(hr);
  }

  int64_t start = 0, end = 0;
  sample->GetMediaTime(&start, &end);
  LOG("DirectShowReader::DecodeAudioData [%4.2lf-%4.2lf]",
      RefTimeToSeconds(start),
      RefTimeToSeconds(end));

  LONG length = sample->GetActualDataLength();
  LONG numSamples = length / mBytesPerSample;
  LONG numFrames = length / mBytesPerSample / mNumChannels;

  BYTE* data = nullptr;
  hr = sample->GetPointer(&data);
  NS_ENSURE_TRUE(SUCCEEDED(hr), Finish(hr));

  nsAutoArrayPtr<AudioDataValue> buffer(new AudioDataValue[numSamples]);
  AudioDataValue* dst = buffer.get();
  if (mBytesPerSample == 1) {
    uint8_t* src = reinterpret_cast<uint8_t*>(data);
    for (int32_t i = 0; i < numSamples; ++i) {
      dst[i] = UnsignedByteToAudioSample(src[i]);
    }
  } else if (mBytesPerSample == 2) {
    int16_t* src = reinterpret_cast<int16_t*>(data);
    for (int32_t i = 0; i < numSamples; ++i) {
      dst[i] = AudioSampleToFloat(src[i]);
    }
  }

  mAudioQueue.Push(new AudioData(mDecoder->GetResource()->Tell(),
                                 RefTimeToUsecs(start),
                                 RefTimeToUsecs(end - start),
                                 numFrames,
                                 buffer.forget(),
                                 mNumChannels));

  uint32_t bytesConsumed = mSourceFilter->GetAndResetBytesConsumedCount();
  if (bytesConsumed > 0) {
    mDecoder->NotifyBytesConsumed(bytesConsumed);
  }

  return true;
}

bool
DirectShowReader::DecodeVideoFrame(bool &aKeyframeSkip,
                                   int64_t aTimeThreshold)
{
  MOZ_ASSERT(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  return false;
}

bool
DirectShowReader::HasAudio()
{
  MOZ_ASSERT(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  return true;
}

bool
DirectShowReader::HasVideo()
{
  MOZ_ASSERT(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  return false;
}

nsresult
DirectShowReader::Seek(int64_t aTargetUs,
                       int64_t aStartTime,
                       int64_t aEndTime,
                       int64_t aCurrentTime)
{
  HRESULT hr;
  MOZ_ASSERT(mDecoder->OnDecodeThread(), "Should be on decode thread.");\

  LOG("DirectShowReader::Seek() target=%lld", aTargetUs);

  hr = mControl->Pause();
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  nsresult rv = ResetDecode();
  NS_ENSURE_SUCCESS(rv, rv);

  LONGLONG seekPosition = UsecsToRefTime(aTargetUs);
  hr = mMediaSeeking->SetPositions(&seekPosition,
                                   AM_SEEKING_AbsolutePositioning,
                                   nullptr,
                                   AM_SEEKING_NoPositioning);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = mControl->Run();
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  return DecodeToTarget(aTargetUs);
}

nsresult
DirectShowReader::GetBuffered(mozilla::dom::TimeRanges* aBuffered,
                              int64_t aStartTime)
{
  MediaResource* stream = mDecoder->GetResource();
  int64_t durationUs = 0;
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    durationUs = mDecoder->GetMediaDuration();
  }
  GetEstimatedBufferedTimeRanges(stream, durationUs, aBuffered);
  return NS_OK;
}

void
DirectShowReader::OnDecodeThreadStart()
{
  MOZ_ASSERT(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
  NS_ENSURE_TRUE_VOID(SUCCEEDED(hr));
}

void
DirectShowReader::OnDecodeThreadFinish()
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  CoUninitialize();
}

void
DirectShowReader::NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mMP3FrameParser.IsMP3()) {
    return;
  }
  mMP3FrameParser.Parse(aBuffer, aLength, aOffset);
  int64_t duration = mMP3FrameParser.GetDuration();
  if (duration != mDuration) {
    mDuration = duration;
    MOZ_ASSERT(mDecoder);
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDecoder->UpdateEstimatedMediaDuration(mDuration);
  }
}

} 
