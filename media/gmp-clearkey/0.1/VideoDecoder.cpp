















#include "stdafx.h"

#ifdef TEST_DECODING

VideoDecoder::VideoDecoder(GMPVideoHost *aHostAPI)
  : mHostAPI(aHostAPI)
  , mCallback(nullptr)
  , mWorkerThread(nullptr)
  , mMutex(nullptr)
  , mNumInputTasks(0)
  , mSentExtraData(false)
{
}

VideoDecoder::~VideoDecoder()
{
  mMutex->Destroy();
}

void
VideoDecoder::InitDecode(const GMPVideoCodec& aCodecSettings,
                         const uint8_t* aCodecSpecific,
                         uint32_t aCodecSpecificLength,
                         GMPVideoDecoderCallback* aCallback,
                         int32_t aCoreCount)
{
  mCallback = aCallback;
  assert(mCallback);
  mDecoder = new WMFH264Decoder();
  HRESULT hr = mDecoder->Init();
  if (FAILED(hr)) {
    mCallback->Error(GMPGenericErr);
    return;
  }

  auto err = GMPCreateMutex(&mMutex);
  if (GMP_FAILED(err)) {
    mCallback->Error(GMPGenericErr);
    return;
  }

  
  
  const uint8_t* avcc = aCodecSpecific + 1;
  const uint8_t* avccEnd = aCodecSpecific + aCodecSpecificLength;
  mExtraData.insert(mExtraData.end(), avcc, avccEnd);

  if (!mAVCC.Parse(mExtraData) ||
      !AVC::ConvertConfigToAnnexB(mAVCC, &mAnnexB)) {
    mCallback->Error(GMPGenericErr);
    return;
  }
}

void
VideoDecoder::Decode(GMPVideoEncodedFrame* aInputFrame,
                     bool aMissingFrames,
                     const uint8_t* aCodecSpecificInfo,
                     uint32_t aCodecSpecificInfoLength,
                     int64_t aRenderTimeMs)
{
  if (aInputFrame->BufferType() != GMP_BufferLength32) {
    
    mCallback->Error(GMPGenericErr);
    return;
  }

  if (!mWorkerThread) {
    GMPCreateThread(&mWorkerThread);
    if (!mWorkerThread) {
      mCallback->Error(GMPAllocErr);
      return;
    }
  }
  {
    AutoLock lock(mMutex);
    mNumInputTasks++;
  }

  
  

  mWorkerThread->Post(WrapTask(this,
                               &VideoDecoder::DecodeTask,
                               aInputFrame));
}

static const uint8_t kAnnexBDelimiter[] = { 0, 0, 0, 1 };

void
VideoDecoder::DecodeTask(GMPVideoEncodedFrame* aInput)
{
  HRESULT hr;

  {
    AutoLock lock(mMutex);
    mNumInputTasks--;
    assert(mNumInputTasks >= 0);
  }

  if (!aInput || !mHostAPI || !mDecoder) {
    LOG(L"Decode job not set up correctly!");
    return;
  }

  const uint8_t* inBuffer = aInput->Buffer();
  if (!inBuffer) {
    LOG(L"No buffer for encoded frame!\n");
    return;
  }

  const GMPEncryptedBufferMetadata* crypto = aInput->GetDecryptionData();
  std::vector<uint8_t> buffer;
  if (crypto) {
    
    
    auto decryptor = Decryptor::Get(crypto);
    if (!decryptor ||
        !decryptor->Decrypt(inBuffer, aInput->Size(), crypto, buffer)) {
      LOG(L"Video decryption error!");
      mCallback->Error(GMPNoKeyErr);
      return;
    }
    buffer.data();
  } else {
    buffer.insert(buffer.end(), inBuffer, inBuffer+aInput->Size());
  }

  AVC::ConvertFrameToAnnexB(4, &buffer);
  if (aInput->FrameType() == kGMPKeyFrame) {
    
    
    
    buffer.insert(buffer.begin(), mAnnexB.begin(), mAnnexB.end());
  }

  hr = mDecoder->Input(buffer.data(),
                       buffer.size(),
                       aInput->TimeStamp(),
                       aInput->Duration());

  
  GMPRunOnMainThread(WrapTask(aInput, &GMPVideoEncodedFrame::Destroy));

  SAMPLE_LOG(L"VideoDecoder::DecodeTask() Input ret hr=0x%x\n", hr);
  if (FAILED(hr)) {
    LOG(L"VideoDecoder::DecodeTask() decode failed ret=0x%x%s\n",
        hr,
        ((hr == MF_E_NOTACCEPTING) ? " (MF_E_NOTACCEPTING)" : ""));
    return;
  }

  while (hr == S_OK) {
    CComPtr<IMFSample> output;
    hr = mDecoder->Output(&output);
    SAMPLE_LOG(L"VideoDecoder::DecodeTask() output ret=0x%x\n", hr);
    if (hr == S_OK) {
      ReturnOutput(output);
    }
    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
      AutoLock lock(mMutex);
      if (mNumInputTasks == 0) {
        
        
        GMPRunOnMainThread(WrapTask(mCallback, &GMPVideoDecoderCallback::InputDataExhausted));
      }
    }
    if (FAILED(hr)) {
      LOG(L"VideoDecoder::DecodeTask() output failed hr=0x%x\n", hr);
    }
  }
}

void
VideoDecoder::ReturnOutput(IMFSample* aSample)
{
  SAMPLE_LOG(L"[%p] WMFDecodingModule::OutputVideoFrame()\n", this);
  assert(aSample);

  HRESULT hr;

  GMPVideoFrame* f = nullptr;
  mHostAPI->CreateFrame(kGMPI420VideoFrame, &f);
  if (!f) {
    LOG(L"Failed to create i420 frame!\n");
    return;
  }
  auto vf = static_cast<GMPVideoi420Frame*>(f);

  hr = SampleToVideoFrame(aSample, vf);
  ENSURE(SUCCEEDED(hr), );

  GMPRunOnMainThread(WrapTask(mCallback, &GMPVideoDecoderCallback::Decoded, vf));

}

HRESULT
VideoDecoder::SampleToVideoFrame(IMFSample* aSample,
                                 GMPVideoi420Frame* aVideoFrame)
{
  ENSURE(aSample != nullptr, E_POINTER);
  ENSURE(aVideoFrame != nullptr, E_POINTER);

  HRESULT hr;
  CComPtr<IMFMediaBuffer> mediaBuffer;

  
  hr = aSample->ConvertToContiguousBuffer(&mediaBuffer);
  ENSURE(SUCCEEDED(hr), hr);

  
  
  
  BYTE* data = nullptr;
  LONG stride = 0;
  CComPtr<IMF2DBuffer> twoDBuffer;
  hr = mediaBuffer->QueryInterface(static_cast<IMF2DBuffer**>(&twoDBuffer));
  if (SUCCEEDED(hr)) {
    hr = twoDBuffer->Lock2D(&data, &stride);
    ENSURE(SUCCEEDED(hr), hr);
  } else {
    hr = mediaBuffer->Lock(&data, NULL, NULL);
    ENSURE(SUCCEEDED(hr), hr);
    stride = mDecoder->GetStride();
  }
  int32_t width = mDecoder->GetFrameWidth();
  int32_t height = mDecoder->GetFrameHeight();

  
  
  
  
  uint32_t padding = 0;
  if (height % 16 != 0) {
    padding = 16 - (height % 16);
  }
  int32_t y_size = stride * (height + padding);
  int32_t v_size = stride * (height + padding) / 4;
  int32_t halfStride = (stride + 1) / 2;
  int32_t halfHeight = (height + 1) / 2;
  int32_t halfWidth = (width + 1) / 2;
  int32_t totalSize = y_size + 2 * v_size;

  GMPSyncRunOnMainThread(WrapTask(aVideoFrame,
                                  &GMPVideoi420Frame::CreateEmptyFrame,
                                  stride, height, stride, halfStride, halfStride));

  auto err = aVideoFrame->SetWidth(width);
  ENSURE(GMP_SUCCEEDED(err), E_FAIL);
  err = aVideoFrame->SetHeight(height);
  ENSURE(GMP_SUCCEEDED(err), E_FAIL);

  uint8_t* outBuffer = aVideoFrame->Buffer(kGMPYPlane);
  ENSURE(outBuffer != nullptr, E_FAIL);
  assert(aVideoFrame->AllocatedSize(kGMPYPlane) >= stride*height);
  memcpy(outBuffer, data, stride*height);

  outBuffer = aVideoFrame->Buffer(kGMPUPlane);
  ENSURE(outBuffer != nullptr, E_FAIL);
  assert(aVideoFrame->AllocatedSize(kGMPUPlane) >= halfStride*halfHeight);
  memcpy(outBuffer, data+y_size, halfStride*halfHeight);

  outBuffer = aVideoFrame->Buffer(kGMPVPlane);
  ENSURE(outBuffer != nullptr, E_FAIL);
  assert(aVideoFrame->AllocatedSize(kGMPVPlane) >= halfStride*halfHeight);
  memcpy(outBuffer, data + y_size + v_size, halfStride*halfHeight);

  if (twoDBuffer) {
    twoDBuffer->Unlock2D();
  } else {
    mediaBuffer->Unlock();
  }

  LONGLONG hns = 0;
  hr = aSample->GetSampleTime(&hns);
  ENSURE(SUCCEEDED(hr), hr);
  aVideoFrame->SetTimestamp(HNsToUsecs(hns));

  hr = aSample->GetSampleDuration(&hns);
  ENSURE(SUCCEEDED(hr), hr);
  aVideoFrame->SetDuration(HNsToUsecs(hns));

  return S_OK;
}

void
VideoDecoder::Reset()
{
  mDecoder->Reset();
  mCallback->ResetComplete();
}

void
VideoDecoder::DrainTask()
{
  if (FAILED(mDecoder->Drain())) {
    GMPSyncRunOnMainThread(WrapTask(mCallback, &GMPVideoDecoderCallback::DrainComplete));
  }

  
  HRESULT hr = S_OK;
  while (hr == S_OK) {
    CComPtr<IMFSample> output;
    hr = mDecoder->Output(&output);
    SAMPLE_LOG(L"VideoDecoder::DrainTask() output ret=0x%x\n", hr);
    if (hr == S_OK) {
      ReturnOutput(output);
    }
  }
  GMPSyncRunOnMainThread(WrapTask(mCallback, &GMPVideoDecoderCallback::DrainComplete));
}

void
VideoDecoder::Drain()
{
  mWorkerThread->Post(WrapTask(this,
                               &VideoDecoder::DrainTask));
}

void
VideoDecoder::DecodingComplete()
{
  if (mWorkerThread) {
    mWorkerThread->Join();
  }
  delete this;
}

#endif
