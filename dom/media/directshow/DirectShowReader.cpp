





#include "DirectShowReader.h"
#include "MediaDecoderReader.h"
#include "mozilla/RefPtr.h"
#include "DirectShowUtils.h"
#include "AudioSinkFilter.h"
#include "SourceFilter.h"
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




static nsresult
ParseMP3Headers(MP3FrameParser *aParser, MediaResource *aResource)
{
  const uint32_t MAX_READ_SIZE = 4096;

  uint64_t offset = 0;
  while (aParser->NeedsData() && !aParser->ParsedHeaders()) {
    uint32_t bytesRead;
    char buffer[MAX_READ_SIZE];
    nsresult rv = aResource->ReadAt(offset, buffer,
                                    MAX_READ_SIZE, &bytesRead);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!bytesRead) {
      
      return NS_ERROR_FAILURE;
    }

    aParser->Parse(buffer, bytesRead, offset);
    offset += bytesRead;
  }

  return aParser->IsMP3() ? NS_OK : NS_ERROR_FAILURE;
}



static const GUID CLSID_MPEG_LAYER_3_DECODER_FILTER =
{ 0x38BE3000, 0xDBF4, 0x11D0, {0x86, 0x0E, 0x00, 0xA0, 0x24, 0xCF, 0xEF, 0x6D} };

nsresult
DirectShowReader::ReadMetadata(MediaInfo* aInfo,
                               MetadataTags** aTags)
{
  MOZ_ASSERT(OnTaskQueue());
  HRESULT hr;
  nsresult rv;

  
  
  hr = CoCreateInstance(CLSID_FilterGraph,
                        nullptr,
                        CLSCTX_INPROC_SERVER,
                        IID_IGraphBuilder,
                        reinterpret_cast<void**>(static_cast<IGraphBuilder**>(byRef(mGraph))));
  NS_ENSURE_TRUE(SUCCEEDED(hr) && mGraph, NS_ERROR_FAILURE);

  rv = ParseMP3Headers(&mMP3FrameParser, mDecoder->GetResource());
  NS_ENSURE_SUCCESS(rv, rv);

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

  rv = mSourceFilter->Init(mDecoder->GetResource(), mMP3FrameParser.GetMP3Offset());
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

  mInfo.mAudio.mChannels = mNumChannels = format.nChannels;
  mInfo.mAudio.mRate = mAudioRate = format.nSamplesPerSec;
  mInfo.mAudio.mBitDepth = format.wBitsPerSample;
  mBytesPerSample = format.wBitsPerSample / 8;

  *aInfo = mInfo;
  
  *aTags = nullptr;

  
  hr = mControl->Run();
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  DWORD seekCaps = 0;
  hr = mMediaSeeking->GetCapabilities(&seekCaps);

  int64_t duration = mMP3FrameParser.GetDuration();
  if (SUCCEEDED(hr)) {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDecoder->SetMediaDuration(duration);
  }

  LOG("Successfully initialized DirectShow MP3 decoder.");
  LOG("Channels=%u Hz=%u duration=%lld bytesPerSample=%d",
      mInfo.mAudio.mChannels,
      mInfo.mAudio.mRate,
      RefTimeToUsecs(duration),
      mBytesPerSample);

  return NS_OK;
}

bool
DirectShowReader::IsMediaSeekable()
{
  DWORD seekCaps = 0;
  HRESULT hr = mMediaSeeking->GetCapabilities(&seekCaps);
  return ((AM_SEEKING_CanSeekAbsolute & seekCaps) ==
          AM_SEEKING_CanSeekAbsolute);
}

inline float
UnsignedByteToAudioSample(uint8_t aValue)
{
  return aValue * (2.0f / UINT8_MAX) - 1.0f;
}

bool
DirectShowReader::Finish(HRESULT aStatus)
{
  MOZ_ASSERT(OnTaskQueue());

  LOG("DirectShowReader::Finish(0x%x)", aStatus);
  
  RefPtr<IMediaEventSink> eventSink;
  HRESULT hr = mGraph->QueryInterface(static_cast<IMediaEventSink**>(byRef(eventSink)));
  if (SUCCEEDED(hr) && eventSink) {
    eventSink->Notify(EC_COMPLETE, aStatus, 0);
  }
  return false;
}

class DirectShowCopy
{
public:
  DirectShowCopy(uint8_t *aSource, uint32_t aBytesPerSample,
                 uint32_t aSamples, uint32_t aChannels)
    : mSource(aSource)
    , mBytesPerSample(aBytesPerSample)
    , mSamples(aSamples)
    , mChannels(aChannels)
    , mNextSample(0)
  { }

  uint32_t operator()(AudioDataValue *aBuffer, uint32_t aSamples)
  {
    uint32_t maxSamples = std::min(aSamples, mSamples - mNextSample);
    uint32_t frames = maxSamples / mChannels;
    size_t byteOffset = mNextSample * mBytesPerSample;
    if (mBytesPerSample == 1) {
      for (uint32_t i = 0; i < maxSamples; ++i) {
        uint8_t *sample = mSource + byteOffset;
        aBuffer[i] = UnsignedByteToAudioSample(*sample);
        byteOffset += mBytesPerSample;
      }
    } else if (mBytesPerSample == 2) {
      for (uint32_t i = 0; i < maxSamples; ++i) {
        int16_t *sample = reinterpret_cast<int16_t *>(mSource + byteOffset);
        aBuffer[i] = AudioSampleToFloat(*sample);
        byteOffset += mBytesPerSample;
      }
    }
    mNextSample += maxSamples;
    return frames;
  }

private:
  uint8_t * const mSource;
  const uint32_t mBytesPerSample;
  const uint32_t mSamples;
  const uint32_t mChannels;
  uint32_t mNextSample;
};

bool
DirectShowReader::DecodeAudioData()
{
  MOZ_ASSERT(OnTaskQueue());
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

  mAudioCompactor.Push(mDecoder->GetResource()->Tell(),
                       RefTimeToUsecs(start),
                       mInfo.mAudio.mRate,
                       numFrames,
                       mNumChannels,
                       DirectShowCopy(reinterpret_cast<uint8_t *>(data),
                                      mBytesPerSample,
                                      numSamples,
                                      mNumChannels));
  return true;
}

bool
DirectShowReader::DecodeVideoFrame(bool &aKeyframeSkip,
                                   int64_t aTimeThreshold)
{
  MOZ_ASSERT(OnTaskQueue());
  return false;
}

bool
DirectShowReader::HasAudio()
{
  MOZ_ASSERT(OnTaskQueue());
  return true;
}

bool
DirectShowReader::HasVideo()
{
  MOZ_ASSERT(OnTaskQueue());
  return false;
}

nsRefPtr<MediaDecoderReader::SeekPromise>
DirectShowReader::Seek(int64_t aTargetUs, int64_t aEndTime)
{
  nsresult res = SeekInternal(aTargetUs);
  if (NS_FAILED(res)) {
    return SeekPromise::CreateAndReject(res, __func__);
  } else {
    return SeekPromise::CreateAndResolve(aTargetUs, __func__);
  }
}

nsresult
DirectShowReader::SeekInternal(int64_t aTargetUs)
{
  HRESULT hr;
  MOZ_ASSERT(OnTaskQueue());

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

  return NS_OK;
}

void
DirectShowReader::NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mMP3FrameParser.NeedsData()) {
    return;
  }

  mMP3FrameParser.Parse(aBuffer, aLength, aOffset);
  if (!mMP3FrameParser.IsMP3()) {
    return;
  }

  int64_t duration = mMP3FrameParser.GetDuration();
  if (duration != mDuration) {
    MOZ_ASSERT(mDecoder);
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDuration = duration;
    mDecoder->UpdateEstimatedMediaDuration(mDuration);
  }
}

} 
