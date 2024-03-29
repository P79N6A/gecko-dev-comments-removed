





#include "MFTDecoder.h"
#include "nsThreadUtils.h"
#include "WMFUtils.h"
#include "mozilla/Logging.h"

PRLogModuleInfo* GetDemuxerLog();
#define LOG(...) MOZ_LOG(GetDemuxerLog(), mozilla::LogLevel::Debug, (__VA_ARGS__))

namespace mozilla {

MFTDecoder::MFTDecoder()
  : mMFTProvidesOutputSamples(false)
  , mDiscontinuity(true)
{
  memset(&mInputStreamInfo, 0, sizeof(MFT_INPUT_STREAM_INFO));
  memset(&mOutputStreamInfo, 0, sizeof(MFT_OUTPUT_STREAM_INFO));
}

MFTDecoder::~MFTDecoder()
{
}

HRESULT
MFTDecoder::Create(const GUID& aMFTClsID)
{
  
  HRESULT hr;
  hr = CoCreateInstance(aMFTClsID,
                        nullptr,
                        CLSCTX_INPROC_SERVER,
                        IID_IMFTransform,
                        reinterpret_cast<void**>(static_cast<IMFTransform**>(byRef(mDecoder))));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  return S_OK;
}

HRESULT
MFTDecoder::SetMediaTypes(IMFMediaType* aInputType,
                          IMFMediaType* aOutputType,
                          ConfigureOutputCallback aCallback,
                          void* aData)
{
  mOutputType = aOutputType;

  
  HRESULT hr = mDecoder->SetInputType(0, aInputType, 0);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  hr = SetDecoderOutputType(aCallback, aData);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  hr = mDecoder->GetInputStreamInfo(0, &mInputStreamInfo);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  hr = SendMFTMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  hr = SendMFTMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  return S_OK;
}

already_AddRefed<IMFAttributes>
MFTDecoder::GetAttributes()
{
  RefPtr<IMFAttributes> attr;
  HRESULT hr = mDecoder->GetAttributes(byRef(attr));
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);
  return attr.forget();
}

HRESULT
MFTDecoder::SetDecoderOutputType(ConfigureOutputCallback aCallback, void* aData)
{
  NS_ENSURE_TRUE(mDecoder != nullptr, E_POINTER);

  
  
  HRESULT hr;
  RefPtr<IMFMediaType> outputType;
  UINT32 typeIndex = 0;
  while (SUCCEEDED(mDecoder->GetOutputAvailableType(0, typeIndex++, byRef(outputType)))) {
    BOOL resultMatch;
    hr = mOutputType->Compare(outputType, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &resultMatch);
    if (SUCCEEDED(hr) && resultMatch == TRUE) {
      if (aCallback) {
        hr = aCallback(outputType, aData);
        NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
      }
      hr = mDecoder->SetOutputType(0, outputType, 0);
      NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

      hr = mDecoder->GetOutputStreamInfo(0, &mOutputStreamInfo);
      NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

      mMFTProvidesOutputSamples = IsFlagSet(mOutputStreamInfo.dwFlags, MFT_OUTPUT_STREAM_PROVIDES_SAMPLES);

      return S_OK;
    }
    outputType = nullptr;
  }
  return E_FAIL;
}

HRESULT
MFTDecoder::SendMFTMessage(MFT_MESSAGE_TYPE aMsg, ULONG_PTR aData)
{
  NS_ENSURE_TRUE(mDecoder != nullptr, E_POINTER);
  HRESULT hr = mDecoder->ProcessMessage(aMsg, aData);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
  return S_OK;
}

HRESULT
MFTDecoder::CreateInputSample(const uint8_t* aData,
                              uint32_t aDataSize,
                              int64_t aTimestamp,
                              RefPtr<IMFSample>* aOutSample)
{
  NS_ENSURE_TRUE(mDecoder != nullptr, E_POINTER);

  HRESULT hr;
  RefPtr<IMFSample> sample;
  hr = wmf::MFCreateSample(byRef(sample));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  RefPtr<IMFMediaBuffer> buffer;
  int32_t bufferSize = std::max<uint32_t>(uint32_t(mInputStreamInfo.cbSize), aDataSize);
  UINT32 alignment = (mInputStreamInfo.cbAlignment > 1) ? mInputStreamInfo.cbAlignment - 1 : 0;
  hr = wmf::MFCreateAlignedMemoryBuffer(bufferSize, alignment, byRef(buffer));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  DWORD maxLength = 0;
  DWORD currentLength = 0;
  BYTE* dst = nullptr;
  hr = buffer->Lock(&dst, &maxLength, &currentLength);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  
  memcpy(dst, aData, aDataSize);

  hr = buffer->Unlock();
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  hr = buffer->SetCurrentLength(aDataSize);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  hr = sample->AddBuffer(buffer);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  hr = sample->SetSampleTime(UsecsToHNs(aTimestamp));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  *aOutSample = sample.forget();

  return S_OK;
}

HRESULT
MFTDecoder::CreateOutputSample(RefPtr<IMFSample>* aOutSample)
{
  NS_ENSURE_TRUE(mDecoder != nullptr, E_POINTER);

  HRESULT hr;
  RefPtr<IMFSample> sample;
  hr = wmf::MFCreateSample(byRef(sample));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  RefPtr<IMFMediaBuffer> buffer;
  int32_t bufferSize = mOutputStreamInfo.cbSize;
  UINT32 alignment = (mOutputStreamInfo.cbAlignment > 1) ? mOutputStreamInfo.cbAlignment - 1 : 0;
  hr = wmf::MFCreateAlignedMemoryBuffer(bufferSize, alignment, byRef(buffer));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  DWORD maxLength = 0;
  DWORD currentLength = 0;
  BYTE* dst = nullptr;

  hr = sample->AddBuffer(buffer);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  *aOutSample = sample.forget();

  return S_OK;
}

HRESULT
MFTDecoder::Output(RefPtr<IMFSample>* aOutput)
{
  NS_ENSURE_TRUE(mDecoder != nullptr, E_POINTER);

  HRESULT hr;

  MFT_OUTPUT_DATA_BUFFER output = {0};

  bool providedSample = false;
  RefPtr<IMFSample> sample;
  if (*aOutput) {
    output.pSample = *aOutput;
    providedSample = true;
  } else if (!mMFTProvidesOutputSamples) {
    hr = CreateOutputSample(&sample);
    NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
    output.pSample = sample;
  }

  DWORD status = 0;
  hr = mDecoder->ProcessOutput(0, 1, &output, &status);
  if (output.pEvents) {
    
    
    output.pEvents->Release();
    output.pEvents = nullptr;
  }

  if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
    
    
    
    
    hr = SetDecoderOutputType(nullptr, nullptr);
    NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
    
    return MF_E_TRANSFORM_STREAM_CHANGE;
  }

  if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
    
    
    return hr;
  }
  
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  MOZ_ASSERT(output.pSample);

  if (mDiscontinuity) {
    output.pSample->SetUINT32(MFSampleExtension_Discontinuity, TRUE);
    mDiscontinuity = false;
  }

  *aOutput = output.pSample; 
  if (mMFTProvidesOutputSamples && !providedSample) {
    
    
    
    
    output.pSample->Release();
    output.pSample = nullptr;
  }

  return S_OK;
}

HRESULT
MFTDecoder::Input(const uint8_t* aData,
                  uint32_t aDataSize,
                  int64_t aTimestamp)
{
  NS_ENSURE_TRUE(mDecoder != nullptr, E_POINTER);

  RefPtr<IMFSample> input;
  HRESULT hr = CreateInputSample(aData, aDataSize, aTimestamp, &input);
  NS_ENSURE_TRUE(SUCCEEDED(hr) && input != nullptr, hr);

  return Input(input);
}

HRESULT
MFTDecoder::Input(IMFSample* aSample)
{
  HRESULT hr = mDecoder->ProcessInput(0, aSample, 0);
  if (hr == MF_E_NOTACCEPTING) {
    
    return MF_E_NOTACCEPTING;
  }
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  return S_OK;
}

HRESULT
MFTDecoder::Flush()
{
  HRESULT hr = SendMFTMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  mDiscontinuity = true;

  return S_OK;
}

HRESULT
MFTDecoder::GetOutputMediaType(RefPtr<IMFMediaType>& aMediaType)
{
  NS_ENSURE_TRUE(mDecoder, E_POINTER);
  return mDecoder->GetOutputCurrentType(0, byRef(aMediaType));
}

} 
