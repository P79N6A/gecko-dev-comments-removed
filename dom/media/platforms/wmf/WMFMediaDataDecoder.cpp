





#include "WMFMediaDataDecoder.h"
#include "VideoUtils.h"
#include "WMFUtils.h"
#include "nsTArray.h"
#include "mozilla/Telemetry.h"

#include "mozilla/Logging.h"

PRLogModuleInfo* GetDemuxerLog();
#define LOG(...) MOZ_LOG(GetDemuxerLog(), mozilla::LogLevel::Debug, (__VA_ARGS__))

namespace mozilla {

WMFMediaDataDecoder::WMFMediaDataDecoder(MFTManager* aMFTManager,
                                         FlushableTaskQueue* aTaskQueue,
                                         MediaDataDecoderCallback* aCallback)
  : mTaskQueue(aTaskQueue)
  , mCallback(aCallback)
  , mMFTManager(aMFTManager)
  , mMonitor("WMFMediaDataDecoder")
  , mIsFlushing(false)
  , mIsShutDown(false)
{
}

WMFMediaDataDecoder::~WMFMediaDataDecoder()
{
}

nsresult
WMFMediaDataDecoder::Init()
{
  MOZ_ASSERT(!mDecoder);
  MOZ_ASSERT(!mIsShutDown);

  mDecoder = mMFTManager->Init();
  NS_ENSURE_TRUE(mDecoder, NS_ERROR_FAILURE);

  return NS_OK;
}



static void
SendTelemetry(HRESULT hr)
{
  
  
  
  
  uint32_t sample;
  if (SUCCEEDED(hr)) {
    sample = 0;
  } else if (hr < 0xc00d36b0) {
    sample = 1; 
  } else if (hr < 0xc00d3700) {
    sample = hr & 0xffU; 
  } else if (hr <= 0xc00d3705) {
    sample = 0x80 + (hr & 0xfU); 
  } else if (hr < 0xc00d6d60) {
    sample = 2; 
  } else if (hr <= 0xc00d6d78) {
    sample = hr & 0xffU; 
  } else {
    sample = 3; 
  }

  nsCOMPtr<nsIRunnable> runnable = NS_NewRunnableFunction(
    [sample] {
      Telemetry::Accumulate(Telemetry::MEDIA_WMF_DECODE_ERROR, sample);
    });
  NS_DispatchToMainThread(runnable);
}

nsresult
WMFMediaDataDecoder::Shutdown()
{
  MOZ_DIAGNOSTIC_ASSERT(!mIsShutDown);

  if (mTaskQueue) {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableMethod(this, &WMFMediaDataDecoder::ProcessShutdown);
    mTaskQueue->Dispatch(runnable.forget());
  } else {
    ProcessShutdown();
  }
  mIsShutDown = true;
  return NS_OK;
}

void
WMFMediaDataDecoder::ProcessShutdown()
{
  if (mMFTManager) {
    mMFTManager->Shutdown();
    mMFTManager = nullptr;
    if (!mRecordedError && mHasSuccessfulOutput) {
      SendTelemetry(S_OK);
    }
  }
  mDecoder = nullptr;
}


nsresult
WMFMediaDataDecoder::Input(MediaRawData* aSample)
{
  MOZ_ASSERT(mCallback->OnReaderTaskQueue());
  MOZ_DIAGNOSTIC_ASSERT(!mIsShutDown);

  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethodWithArg<nsRefPtr<MediaRawData>>(
      this,
      &WMFMediaDataDecoder::ProcessDecode,
      nsRefPtr<MediaRawData>(aSample));
  mTaskQueue->Dispatch(runnable.forget());
  return NS_OK;
}

void
WMFMediaDataDecoder::ProcessDecode(MediaRawData* aSample)
{
  {
    MonitorAutoLock mon(mMonitor);
    if (mIsFlushing) {
      
      return;
    }
  }

  HRESULT hr = mMFTManager->Input(aSample);
  if (FAILED(hr)) {
    NS_WARNING("MFTManager rejected sample");
    mCallback->Error();
    if (!mRecordedError) {
      SendTelemetry(hr);
      mRecordedError = true;
    }
    return;
  }

  mLastStreamOffset = aSample->mOffset;

  ProcessOutput();
}

void
WMFMediaDataDecoder::ProcessOutput()
{
  nsRefPtr<MediaData> output;
  HRESULT hr = S_OK;
  while (SUCCEEDED(hr = mMFTManager->Output(mLastStreamOffset, output)) &&
         output) {
    mHasSuccessfulOutput = true;
    mCallback->Output(output);
  }
  if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
    if (mTaskQueue->IsEmpty()) {
      mCallback->InputExhausted();
    }
  } else if (FAILED(hr)) {
    NS_WARNING("WMFMediaDataDecoder failed to output data");
    mCallback->Error();
    if (!mRecordedError) {
      SendTelemetry(hr);
      mRecordedError = true;
    }
  }
}

void
WMFMediaDataDecoder::ProcessFlush()
{
  if (mDecoder) {
    mDecoder->Flush();
  }
  MonitorAutoLock mon(mMonitor);
  mIsFlushing = false;
  mon.NotifyAll();
}

nsresult
WMFMediaDataDecoder::Flush()
{
  MOZ_ASSERT(mCallback->OnReaderTaskQueue());
  MOZ_DIAGNOSTIC_ASSERT(!mIsShutDown);

  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(this, &WMFMediaDataDecoder::ProcessFlush);
  MonitorAutoLock mon(mMonitor);
  mIsFlushing = true;
  mTaskQueue->Dispatch(runnable.forget());
  while (mIsFlushing) {
    mon.Wait();
  }
  return NS_OK;
}

void
WMFMediaDataDecoder::ProcessDrain()
{
  bool isFlushing;
  {
    MonitorAutoLock mon(mMonitor);
    isFlushing = mIsFlushing;
  }
  if (!isFlushing && mDecoder) {
    
    if (FAILED(mDecoder->SendMFTMessage(MFT_MESSAGE_COMMAND_DRAIN, 0))) {
      NS_WARNING("Failed to send DRAIN command to MFT");
    }
    
    ProcessOutput();
  }
  mCallback->DrainComplete();
}

nsresult
WMFMediaDataDecoder::Drain()
{
  MOZ_ASSERT(mCallback->OnReaderTaskQueue());
  MOZ_DIAGNOSTIC_ASSERT(!mIsShutDown);

  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(this, &WMFMediaDataDecoder::ProcessDrain);
  mTaskQueue->Dispatch(runnable.forget());
  return NS_OK;
}

bool
WMFMediaDataDecoder::IsHardwareAccelerated() const {
  MOZ_ASSERT(!mIsShutDown);

  return mMFTManager && mMFTManager->IsHardwareAccelerated();
}

} 
