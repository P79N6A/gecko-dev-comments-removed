





#include "WMFAudioMFTManager.h"
#include "MediaInfo.h"
#include "VideoUtils.h"
#include "WMFUtils.h"
#include "nsTArray.h"
#include "TimeUnits.h"

#include "mozilla/Logging.h"

PRLogModuleInfo* GetDemuxerLog();
#define LOG(...) MOZ_LOG(GetDemuxerLog(), mozilla::LogLevel::Debug, (__VA_ARGS__))

namespace mozilla {

static void
AACAudioSpecificConfigToUserData(uint8_t aAACProfileLevelIndication,
                                 const uint8_t* aAudioSpecConfig,
                                 uint32_t aConfigLength,
                                 nsTArray<BYTE>& aOutUserData)
{
  MOZ_ASSERT(aOutUserData.IsEmpty());

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  const UINT32 heeInfoLen = 4 * sizeof(WORD) + sizeof(DWORD);

  
  
  BYTE heeInfo[heeInfoLen] = {0};
  WORD* w = (WORD*)heeInfo;
  w[0] = 0x0; 
  w[1] = aAACProfileLevelIndication;

  aOutUserData.AppendElements(heeInfo, heeInfoLen);
  aOutUserData.AppendElements(aAudioSpecConfig, aConfigLength);
}

WMFAudioMFTManager::WMFAudioMFTManager(
  const AudioInfo& aConfig)
  : mAudioChannels(aConfig.mChannels)
  , mAudioRate(aConfig.mRate)
  , mAudioFrameOffset(0)
  , mAudioFrameSum(0)
  , mMustRecaptureAudioPosition(true)
{
  MOZ_COUNT_CTOR(WMFAudioMFTManager);

  if (aConfig.mMimeType.EqualsLiteral("audio/mpeg")) {
    mStreamType = MP3;
  } else if (aConfig.mMimeType.EqualsLiteral("audio/mp4a-latm")) {
    mStreamType = AAC;
    AACAudioSpecificConfigToUserData(aConfig.mProfile,
                                     aConfig.mCodecSpecificConfig->Elements(),
                                     aConfig.mCodecSpecificConfig->Length(),
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

already_AddRefed<MFTDecoder>
WMFAudioMFTManager::Init()
{
  NS_ENSURE_TRUE(mStreamType != Unknown, nullptr);

  RefPtr<MFTDecoder> decoder(new MFTDecoder());

  HRESULT hr = decoder->Create(GetMFTGUID());
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  
  RefPtr<IMFMediaType> inputType;

  hr = wmf::MFCreateMediaType(byRef(inputType));
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  hr = inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  hr = inputType->SetGUID(MF_MT_SUBTYPE, GetMediaSubtypeGUID());
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  hr = inputType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, mAudioRate);
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  hr = inputType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, mAudioChannels);
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  if (mStreamType == AAC) {
    hr = inputType->SetUINT32(MF_MT_AAC_PAYLOAD_TYPE, 0x0); 
    NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

    hr = inputType->SetBlob(MF_MT_USER_DATA,
                            mUserData.Elements(),
                            mUserData.Length());
    NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);
  }

  RefPtr<IMFMediaType> outputType;
  hr = wmf::MFCreateMediaType(byRef(outputType));
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  hr = outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  hr = outputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  hr = outputType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  hr = decoder->SetMediaTypes(inputType, outputType);
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  mDecoder = decoder;

  return decoder.forget();
}

HRESULT
WMFAudioMFTManager::Input(MediaRawData* aSample)
{
  return mDecoder->Input(aSample->mData,
                         uint32_t(aSample->mSize),
                         aSample->mTime);
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
                           nsRefPtr<MediaData>& aOutData)
{
  aOutData = nullptr;
  RefPtr<IMFSample> sample;
  HRESULT hr;
  int typeChangeCount = 0;
  while (true) {
    hr = mDecoder->Output(&sample);
    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
      return hr;
    }
    if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
      hr = UpdateOutputType();
      NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
      
      
      
      NS_ENSURE_TRUE(typeChangeCount < 100, MF_E_TRANSFORM_STREAM_CHANGE);
      ++typeChangeCount;
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

  
  
  
  
  
  
  
  
  

  
  
  

  
  
  
  UINT32 discontinuity = false;
  sample->GetUINT32(MFSampleExtension_Discontinuity, &discontinuity);
  if (mMustRecaptureAudioPosition || discontinuity) {
    
    
    
    
    hr = UpdateOutputType();
    NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

    mAudioFrameSum = 0;
    LONGLONG timestampHns = 0;
    hr = sample->GetSampleTime(&timestampHns);
    NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
    hr = HNsToFrames(timestampHns, mAudioRate, &mAudioFrameOffset);
    NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
    mMustRecaptureAudioPosition = false;
  }
  
  int32_t numSamples = currentLength / 2;
  int32_t numFrames = numSamples / mAudioChannels;
  MOZ_ASSERT(numFrames >= 0);
  MOZ_ASSERT(numSamples >= 0);
  if (numFrames == 0) {
    
    
    return S_OK;
  }

  nsAutoArrayPtr<AudioDataValue> audioData(new AudioDataValue[numSamples]);

  int16_t* pcm = (int16_t*)data;
  for (int32_t i = 0; i < numSamples; ++i) {
    audioData[i] = AudioSampleToFloat(pcm[i]);
  }

  buffer->Unlock();

  media::TimeUnit timestamp =
    FramesToTimeUnit(mAudioFrameOffset + mAudioFrameSum, mAudioRate);
  NS_ENSURE_TRUE(timestamp.IsValid(), E_FAIL);

  mAudioFrameSum += numFrames;

  media::TimeUnit duration = FramesToTimeUnit(numFrames, mAudioRate);
  NS_ENSURE_TRUE(duration.IsValid(), E_FAIL);

  aOutData = new AudioData(aStreamOffset,
                           timestamp.ToMicroseconds(),
                           duration.ToMicroseconds(),
                           numFrames,
                           audioData.forget(),
                           mAudioChannels,
                           mAudioRate);

  #ifdef LOG_SAMPLE_DECODE
  LOG("Decoded audio sample! timestamp=%lld duration=%lld currentLength=%u",
      timestamp.ToMicroseconds(), duration.ToMicroseconds(), currentLength);
  #endif

  return S_OK;
}

void
WMFAudioMFTManager::Shutdown()
{
  mDecoder = nullptr;
}

} 
