





#include "WMFAudioMFTManager.h"
#include "mp4_demuxer/DecoderData.h"
#include "VideoUtils.h"
#include "WMFUtils.h"
#include "nsTArray.h"

#include "prlog.h"

#ifdef PR_LOGGING
PRLogModuleInfo* GetDemuxerLog();
#define LOG(...) PR_LOG(GetDemuxerLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define LOG(...)
#endif

namespace mozilla {

static void
AACAudioSpecificConfigToUserData(const uint8_t* aAudioSpecConfig,
                                 uint32_t aConfigLength,
                                 nsTArray<BYTE>& aOutUserData)
{
  MOZ_ASSERT(aOutUserData.IsEmpty());

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  const UINT32 heeInfoLen = 4 * sizeof(WORD) + sizeof(DWORD);

  
  
  BYTE heeInfo[heeInfoLen] = {0};
  WORD* w = (WORD*)heeInfo;
  w[0] = 0x1; 
  w[1] = 0xFE; 

  aOutUserData.AppendElements(heeInfo, heeInfoLen);
  aOutUserData.AppendElements(aAudioSpecConfig, aConfigLength);
}

WMFAudioMFTManager::WMFAudioMFTManager(
  const mp4_demuxer::AudioDecoderConfig& aConfig)
  : mAudioChannels(aConfig.channel_count)
  , mAudioBytesPerSample(aConfig.bits_per_sample / 8)
  , mAudioRate(aConfig.samples_per_second)
  , mAudioFrameOffset(0)
  , mAudioFrameSum(0)
  , mMustRecaptureAudioPosition(true)
{
  MOZ_COUNT_CTOR(WMFAudioMFTManager);

  if (!strcmp(aConfig.mime_type, "audio/mpeg")) {
    mStreamType = MP3;
  } else if (!strcmp(aConfig.mime_type, "audio/mp4a-latm")) {
    mStreamType = AAC;
    AACAudioSpecificConfigToUserData(&aConfig.audio_specific_config[0],
                                     aConfig.audio_specific_config.length(),
                                     mUserData);
  } else {
    mStreamType = Unknown;
  }
}

WMFAudioMFTManager::~WMFAudioMFTManager()
{
  MOZ_COUNT_DTOR(WMFAudioMFTManager);
}

const GUID&
WMFAudioMFTManager::GetMFTGUID()
{
  MOZ_ASSERT(mStreamType != Unknown);
  switch (mStreamType) {
    case AAC: return CLSID_CMSAACDecMFT;
    case MP3: return CLSID_CMP3DecMediaObject;
    default: return GUID_NULL;
  };
}

const GUID&
WMFAudioMFTManager::GetMediaSubtypeGUID()
{
  MOZ_ASSERT(mStreamType != Unknown);
  switch (mStreamType) {
    case AAC: return MFAudioFormat_AAC;
    case MP3: return MFAudioFormat_MP3;
    default: return GUID_NULL;
  };
}

TemporaryRef<MFTDecoder>
WMFAudioMFTManager::Init()
{
  NS_ENSURE_TRUE(mStreamType != Unknown, nullptr);

  RefPtr<MFTDecoder> decoder(new MFTDecoder());

  HRESULT hr = decoder->Create(GetMFTGUID());
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  
  RefPtr<IMFMediaType> type;

  hr = wmf::MFCreateMediaType(byRef(type));
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  hr = type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  hr = type->SetGUID(MF_MT_SUBTYPE, GetMediaSubtypeGUID());
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  hr = type->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, mAudioRate);
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  hr = type->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, mAudioChannels);
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  if (mStreamType == AAC) {
    hr = type->SetUINT32(MF_MT_AAC_PAYLOAD_TYPE, 0x1); 
    NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

    hr = type->SetBlob(MF_MT_USER_DATA,
                       mUserData.Elements(),
                       mUserData.Length());
    NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);
  }

  hr = decoder->SetMediaTypes(type, MFAudioFormat_PCM);
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  mDecoder = decoder;

  return decoder.forget();
}

HRESULT
WMFAudioMFTManager::Input(mp4_demuxer::MP4Sample* aSample)
{
  const uint8_t* data = reinterpret_cast<const uint8_t*>(aSample->data);
  uint32_t length = aSample->size;
  return mDecoder->Input(data, length, aSample->composition_timestamp);
}

HRESULT
WMFAudioMFTManager::UpdateOutputType()
{
  HRESULT hr;

  RefPtr<IMFMediaType> type;
  hr = mDecoder->GetOutputMediaType(type);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  hr = type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &mAudioRate);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  hr = type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &mAudioChannels);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  return S_OK;
}

HRESULT
WMFAudioMFTManager::Output(int64_t aStreamOffset,
                           nsAutoPtr<MediaData>& aOutData)
{
  aOutData = nullptr;
  RefPtr<IMFSample> sample;
  HRESULT hr;
  while (true) {
    hr = mDecoder->Output(&sample);
    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
      return hr;
    }
    if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
      hr = UpdateOutputType();
      NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
      continue;
    }
    break;
  }

  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  RefPtr<IMFMediaBuffer> buffer;
  hr = sample->ConvertToContiguousBuffer(byRef(buffer));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  BYTE* data = nullptr; 
  DWORD maxLength = 0, currentLength = 0;
  hr = buffer->Lock(&data, &maxLength, &currentLength);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  int32_t numSamples = currentLength / mAudioBytesPerSample;
  int32_t numFrames = numSamples / mAudioChannels;

  
  
  
  
  
  
  
  
  

  
  
  

  
  
  
  UINT32 discontinuity = false;
  int32_t numFramesToStrip = 0;
  sample->GetUINT32(MFSampleExtension_Discontinuity, &discontinuity);
  if (mMustRecaptureAudioPosition || discontinuity) {
    mAudioFrameSum = 0;
    LONGLONG timestampHns = 0;
    hr = sample->GetSampleTime(&timestampHns);
    NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
    hr = HNsToFrames(timestampHns, mAudioRate, &mAudioFrameOffset);
    NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
    if (mAudioFrameOffset < 0) {
      
      
      numFramesToStrip = -mAudioFrameOffset;
      mAudioFrameOffset = 0;
    }
    mMustRecaptureAudioPosition = false;
  }
  MOZ_ASSERT(numFramesToStrip >= 0);
  int32_t offset = std::min<int32_t>(numFramesToStrip, numFrames);
  numFrames -= offset;
  numSamples -= offset * mAudioChannels;
  MOZ_ASSERT(numFrames >= 0);
  MOZ_ASSERT(numSamples >= 0);
  if (numFrames == 0) {
    
    
    return S_OK;
  }

  nsAutoArrayPtr<AudioDataValue> audioData(new AudioDataValue[numSamples]);

  
  MOZ_ASSERT(mAudioBytesPerSample == 2);
  int16_t* pcm = ((int16_t*)data) + (offset * mAudioChannels);
  MOZ_ASSERT(pcm >= (int16_t*)data);
  MOZ_ASSERT(pcm <= (int16_t*)(data + currentLength));
  MOZ_ASSERT(pcm+numSamples <= (int16_t*)(data + currentLength));
  for (int32_t i = 0; i < numSamples; ++i) {
    audioData[i] = AudioSampleToFloat(pcm[i]);
  }

  buffer->Unlock();
  int64_t timestamp;
  hr = FramesToUsecs(mAudioFrameOffset + mAudioFrameSum, mAudioRate, &timestamp);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  mAudioFrameSum += numFrames;

  int64_t duration;
  hr = FramesToUsecs(numFrames, mAudioRate, &duration);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  aOutData = new AudioData(aStreamOffset,
                           timestamp,
                           duration,
                           numFrames,
                           audioData.forget(),
                           mAudioChannels,
                           mAudioRate);

  #ifdef LOG_SAMPLE_DECODE
  LOG("Decoded audio sample! timestamp=%lld duration=%lld currentLength=%u",
      timestamp, duration, currentLength);
  #endif

  return S_OK;
}

void
WMFAudioMFTManager::Shutdown()
{
  mDecoder = nullptr;
}

} 
