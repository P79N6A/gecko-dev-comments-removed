





#include "WMFAudioDecoder.h"
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

WMFAudioDecoder::WMFAudioDecoder()
  : mAudioChannels(0),
    mAudioBytesPerSample(0),
    mAudioRate(0),
    mLastStreamOffset(0),
    mAudioFrameOffset(0),
    mAudioFrameSum(0),
    mMustRecaptureAudioPosition(true)
{
}

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

nsresult
WMFAudioDecoder::Init(uint32_t aChannelCount,
                      uint32_t aSampleRate,
                      uint16_t aBitsPerSample,
                      const uint8_t* aAudioSpecConfig,
                      uint32_t aConfigLength)
{
  mDecoder = new MFTDecoder();

  HRESULT hr = mDecoder->Create(CLSID_CMSAACDecMFT);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  
  RefPtr<IMFMediaType> type;

  hr = wmf::MFCreateMediaType(byRef(type));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = type->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, aSampleRate);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = type->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, aChannelCount);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = type->SetUINT32(MF_MT_AAC_PAYLOAD_TYPE, 0x1); 
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  nsTArray<BYTE> userData;
  AACAudioSpecificConfigToUserData(aAudioSpecConfig,
                                   aConfigLength,
                                   userData);

  hr = type->SetBlob(MF_MT_USER_DATA,
                     userData.Elements(),
                     userData.Length());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = mDecoder->SetMediaTypes(type, MFAudioFormat_PCM);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  mAudioChannels = aChannelCount;
  mAudioBytesPerSample = aBitsPerSample / 8;
  mAudioRate = aSampleRate;
  return NS_OK;
}

nsresult
WMFAudioDecoder::Shutdown()
{
  return NS_OK;
}

DecoderStatus
WMFAudioDecoder::Input(const uint8_t* aData,
                       uint32_t aLength,
                       Microseconds aDTS,
                       Microseconds aPTS,
                       int64_t aOffsetInStream)
{
  mLastStreamOffset = aOffsetInStream;
  HRESULT hr = mDecoder->Input(aData, aLength, aPTS);
  if (hr == MF_E_NOTACCEPTING) {
    return DECODE_STATUS_NOT_ACCEPTING;
  }
  NS_ENSURE_TRUE(SUCCEEDED(hr), DECODE_STATUS_ERROR);

  return DECODE_STATUS_OK;
}

DecoderStatus
WMFAudioDecoder::Output(nsAutoPtr<MediaData>& aOutData)
{
  DecoderStatus status;
  do {
    status = OutputNonNegativeTimeSamples(aOutData);
  } while (status == DECODE_STATUS_OK && !aOutData);
  return status;
}

DecoderStatus
WMFAudioDecoder::OutputNonNegativeTimeSamples(nsAutoPtr<MediaData>& aOutData)
{

  aOutData = nullptr;
  RefPtr<IMFSample> sample;
  HRESULT hr = mDecoder->Output(&sample);
  if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
    return DECODE_STATUS_NEED_MORE_INPUT;
  }
  NS_ENSURE_TRUE(SUCCEEDED(hr), DECODE_STATUS_ERROR);

  RefPtr<IMFMediaBuffer> buffer;
  hr = sample->ConvertToContiguousBuffer(byRef(buffer));
  NS_ENSURE_TRUE(SUCCEEDED(hr), DECODE_STATUS_ERROR);

  BYTE* data = nullptr; 
  DWORD maxLength = 0, currentLength = 0;
  hr = buffer->Lock(&data, &maxLength, &currentLength);
  NS_ENSURE_TRUE(SUCCEEDED(hr), DECODE_STATUS_ERROR);

  int32_t numSamples = currentLength / mAudioBytesPerSample;
  int32_t numFrames = numSamples / mAudioChannels;

  
  
  
  
  
  
  
  
  

  
  
  

  
  
  
  UINT32 discontinuity = false;
  int32_t numFramesToStrip = 0;
  sample->GetUINT32(MFSampleExtension_Discontinuity, &discontinuity);
  if (mMustRecaptureAudioPosition || discontinuity) {
    mAudioFrameSum = 0;
    LONGLONG timestampHns = 0;
    hr = sample->GetSampleTime(&timestampHns);
    NS_ENSURE_TRUE(SUCCEEDED(hr), DECODE_STATUS_ERROR);
    hr = HNsToFrames(timestampHns, mAudioRate, &mAudioFrameOffset);
    NS_ENSURE_TRUE(SUCCEEDED(hr), DECODE_STATUS_ERROR);
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
    
    
    return DECODE_STATUS_OK;
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
  NS_ENSURE_TRUE(SUCCEEDED(hr), DECODE_STATUS_ERROR);

  mAudioFrameSum += numFrames;

  int64_t duration;
  hr = FramesToUsecs(numFrames, mAudioRate, &duration);
  NS_ENSURE_TRUE(SUCCEEDED(hr), DECODE_STATUS_ERROR);

  aOutData = new AudioData(mLastStreamOffset,
                            timestamp,
                            duration,
                            numFrames,
                            audioData.forget(),
                            mAudioChannels);

  #ifdef LOG_SAMPLE_DECODE
  LOG("Decoded audio sample! timestamp=%lld duration=%lld currentLength=%u",
      timestamp, duration, currentLength);
  #endif

  return DECODE_STATUS_OK;
}

DecoderStatus
WMFAudioDecoder::Flush()
{
  NS_ENSURE_TRUE(mDecoder, DECODE_STATUS_ERROR);
  HRESULT hr = mDecoder->Flush();
  NS_ENSURE_TRUE(SUCCEEDED(hr), DECODE_STATUS_ERROR);
  return DECODE_STATUS_OK;
}

} 
