















#include "WMFAACDecoder.h"
#include <algorithm>
#include <stdint.h>
#include <vector>

using std::vector;

namespace wmf {

WMFAACDecoder::WMFAACDecoder()
  : mDecoder(nullptr)
  , mChannels(0)
  , mRate(0)
{
  memset(&mInputStreamInfo, 0, sizeof(MFT_INPUT_STREAM_INFO));
  memset(&mOutputStreamInfo, 0, sizeof(MFT_OUTPUT_STREAM_INFO));
}

WMFAACDecoder::~WMFAACDecoder()
{
  Reset();
}

HRESULT
AACAudioSpecificConfigToUserData(BYTE* aAudioSpecConfig, UINT32 aConfigLength,
                                 BYTE** aOutUserData, UINT32* aOutUserDataLength)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  const UINT32 heeInfoLen = 4 * sizeof(WORD) + sizeof(DWORD);
  BYTE heeInfo[heeInfoLen] = {0};
  WORD* w = (WORD*)heeInfo;
  w[0] = 0x0; 
  w[1] = 0; 

  const UINT32 len =  heeInfoLen + aConfigLength;
  BYTE* data = new BYTE[len];
  memcpy(data, heeInfo, heeInfoLen);
  memcpy(data+heeInfoLen, aAudioSpecConfig, aConfigLength);
  *aOutUserData = data;
  *aOutUserDataLength = len;
  return S_OK;
}

HRESULT
WMFAACDecoder::Init(int32_t aChannelCount,
                    int32_t aSampleRate,
                    BYTE* aAACAudioSpecificConfig,
                    UINT32 aAudioConfigLength)
{
  HRESULT hr;

  
  hr = CreateMFT(CLSID_CMSAACDecMFT,
                 WMFDecoderDllNameFor(AAC),
                 mDecoder);
  if (FAILED(hr)) {
    hr = CreateMFT(CLSID_CMSAACDecMFT,
                   WMFDecoderDllNameFor(AAC),
                   mDecoder);
    if (FAILED(hr)) {
      LOG("Failed to create AAC decoder\n");
      return E_FAIL;
    }
  }

  BYTE* userData = nullptr;
  UINT32 userDataLength;
  hr = AACAudioSpecificConfigToUserData(aAACAudioSpecificConfig,
                                        aAudioConfigLength,
                                        &userData,
                                        &userDataLength);
  ENSURE(SUCCEEDED(hr), hr);
  hr = SetDecoderInputType(aChannelCount, aSampleRate, userData, userDataLength);
  delete userData;
  ENSURE(SUCCEEDED(hr), hr);

  hr = SetDecoderOutputType();
  ENSURE(SUCCEEDED(hr), hr);

  hr = SendMFTMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
  ENSURE(SUCCEEDED(hr), hr);

  hr = SendMFTMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
  ENSURE(SUCCEEDED(hr), hr);

  hr = mDecoder->GetInputStreamInfo(0, &mInputStreamInfo);
  ENSURE(SUCCEEDED(hr), hr);

  hr = mDecoder->GetOutputStreamInfo(0, &mOutputStreamInfo);
  ENSURE(SUCCEEDED(hr), hr);

  return S_OK;
}

HRESULT
WMFAACDecoder::SetDecoderInputType(int32_t aChannelCount,
                                   int32_t aSampleRate,
                                   BYTE* aUserData,
                                   UINT32 aUserDataLength)
{
  HRESULT hr;

  CComPtr<IMFMediaType> type;
  hr = MFCreateMediaType(&type);
  ENSURE(SUCCEEDED(hr), hr);

  hr = type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
  ENSURE(SUCCEEDED(hr), hr);

  hr = type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC);
  ENSURE(SUCCEEDED(hr), hr);

  mRate = aSampleRate;
  hr = type->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, mRate);
  ENSURE(SUCCEEDED(hr), hr);

  mChannels = aChannelCount;
  hr = type->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, mChannels);
  ENSURE(SUCCEEDED(hr), hr);

  hr = type->SetBlob(MF_MT_USER_DATA, aUserData, aUserDataLength);
  ENSURE(SUCCEEDED(hr), hr);

  hr = mDecoder->SetInputType(0, type, 0);
  ENSURE(SUCCEEDED(hr), hr);

  return S_OK;
}

HRESULT
WMFAACDecoder::SetDecoderOutputType()
{
  HRESULT hr;

  CComPtr<IMFMediaType> type;

  UINT32 typeIndex = 0;
  while (type = nullptr, SUCCEEDED(mDecoder->GetOutputAvailableType(0, typeIndex++, &type))) {
    GUID subtype;
    hr = type->GetGUID(MF_MT_SUBTYPE, &subtype);
    if (FAILED(hr)) {
      continue;
    }
    if (subtype == MFAudioFormat_PCM) {
      hr = mDecoder->SetOutputType(0, type, 0);
      ENSURE(SUCCEEDED(hr), hr);

      hr = type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &mChannels);
      ENSURE(SUCCEEDED(hr), hr);

      hr = type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &mRate);
      ENSURE(SUCCEEDED(hr), hr);

      return S_OK;
    }
  }

  return E_FAIL;
}

HRESULT
WMFAACDecoder::SendMFTMessage(MFT_MESSAGE_TYPE aMsg, UINT32 aData)
{
  ENSURE(mDecoder != nullptr, E_POINTER);
  HRESULT hr = mDecoder->ProcessMessage(aMsg, aData);
  ENSURE(SUCCEEDED(hr), hr);
  return S_OK;
}

HRESULT
WMFAACDecoder::CreateInputSample(const uint8_t* aData,
                                 uint32_t aDataSize,
                                 Microseconds aTimestamp,
                                 IMFSample** aOutSample)
{
  HRESULT hr;
  CComPtr<IMFSample> sample = nullptr;
  hr = MFCreateSample(&sample);
  ENSURE(SUCCEEDED(hr), hr);

  CComPtr<IMFMediaBuffer> buffer = nullptr;
  int32_t bufferSize = std::max<uint32_t>(uint32_t(mInputStreamInfo.cbSize), aDataSize);
  UINT32 alignment = (mInputStreamInfo.cbAlignment > 1) ? mInputStreamInfo.cbAlignment - 1 : 0;
  hr = MFCreateAlignedMemoryBuffer(bufferSize, alignment, &buffer);
  ENSURE(SUCCEEDED(hr), hr);

  DWORD maxLength = 0;
  DWORD currentLength = 0;
  BYTE* dst = nullptr;
  hr = buffer->Lock(&dst, &maxLength, &currentLength);
  ENSURE(SUCCEEDED(hr), hr);

  
  memcpy(dst, aData, aDataSize);

  hr = buffer->Unlock();
  ENSURE(SUCCEEDED(hr), hr);

  hr = buffer->SetCurrentLength(aDataSize);
  ENSURE(SUCCEEDED(hr), hr);

  hr = sample->AddBuffer(buffer);
  ENSURE(SUCCEEDED(hr), hr);

  hr = sample->SetSampleTime(UsecsToHNs(aTimestamp));
  ENSURE(SUCCEEDED(hr), hr);

  *aOutSample = sample.Detach();

  return S_OK;
}

HRESULT
WMFAACDecoder::CreateOutputSample(IMFSample** aOutSample)
{
  HRESULT hr;
  CComPtr<IMFSample> sample = nullptr;
  hr = MFCreateSample(&sample);
  ENSURE(SUCCEEDED(hr), hr);

  CComPtr<IMFMediaBuffer> buffer = nullptr;
  int32_t bufferSize = mOutputStreamInfo.cbSize;
  UINT32 alignment = (mOutputStreamInfo.cbAlignment > 1) ? mOutputStreamInfo.cbAlignment - 1 : 0;
  hr = MFCreateAlignedMemoryBuffer(bufferSize, alignment, &buffer);
  ENSURE(SUCCEEDED(hr), hr);

  hr = sample->AddBuffer(buffer);
  ENSURE(SUCCEEDED(hr), hr);

  *aOutSample = sample.Detach();

  return S_OK;
}


HRESULT
WMFAACDecoder::GetOutputSample(IMFSample** aOutSample)
{
  HRESULT hr;
  
  MFT_OUTPUT_DATA_BUFFER output = {0};

  CComPtr<IMFSample> sample = nullptr;
  hr = CreateOutputSample(&sample);
  ENSURE(SUCCEEDED(hr), hr);

  output.pSample = sample;

  DWORD status = 0;
  hr = mDecoder->ProcessOutput(0, 1, &output, &status);
  CComPtr<IMFCollection> events = output.pEvents; 

  if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
    
    hr = SetDecoderOutputType();
    ENSURE(SUCCEEDED(hr), hr);

    return GetOutputSample(aOutSample);
  } else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT || !sample) {
    return MF_E_TRANSFORM_NEED_MORE_INPUT;
  }
  
  ENSURE(SUCCEEDED(hr), hr);

  assert(sample);

  *aOutSample = sample.Detach();
  return S_OK;
}

HRESULT
WMFAACDecoder::Input(const uint8_t* aData,
                     uint32_t aDataSize,
                     Microseconds aTimestamp)
{
  CComPtr<IMFSample> input = nullptr;
  HRESULT hr = CreateInputSample(aData, aDataSize, aTimestamp, &input);
  ENSURE(SUCCEEDED(hr) && input!=nullptr, hr);

  hr = mDecoder->ProcessInput(0, input, 0);
  if (hr == MF_E_NOTACCEPTING) {
    
    LOG("ProcessInput returned MF_E_NOTACCEPTING\n");
    return MF_E_NOTACCEPTING;
  }
  ENSURE(SUCCEEDED(hr), hr);

  return S_OK;
}

HRESULT
WMFAACDecoder::Output(IMFSample** aOutput)
{
  CComPtr<IMFSample> outputSample = nullptr;
  HRESULT hr = GetOutputSample(&outputSample);
  if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
    return MF_E_TRANSFORM_NEED_MORE_INPUT;
  }
  
  ENSURE(SUCCEEDED(hr) && outputSample, hr);

  *aOutput = outputSample.Detach();

  return S_OK;
}

HRESULT
WMFAACDecoder::Reset()
{
  HRESULT hr = SendMFTMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
  ENSURE(SUCCEEDED(hr), hr);

  return S_OK;
}

HRESULT
WMFAACDecoder::Drain()
{
  HRESULT hr = SendMFTMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);
  ENSURE(SUCCEEDED(hr), hr);

  return S_OK;
}

}
