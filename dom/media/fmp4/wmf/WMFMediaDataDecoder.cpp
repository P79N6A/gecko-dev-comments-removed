





#include "WMFMediaDataDecoder.h"
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

WMFMediaDataDecoder::WMFMediaDataDecoder(MFTManager* aMFTManager,
                                         FlushableMediaTaskQueue* aTaskQueue,
                                         MediaDataDecoderCallback* aCallback)
  : mTaskQueue(aTaskQueue)
  , mCallback(aCallback)
  , mMFTManager(aMFTManager)
{
  MOZ_COUNT_CTOR(WMFMediaDataDecoder);
}

WMFMediaDataDecoder::~WMFMediaDataDecoder()
{
  MOZ_COUNT_DTOR(WMFMediaDataDecoder);
}

nsresult
WMFMediaDataDecoder::Init()
{
  mDecoder = mMFTManager->Init();
  NS_ENSURE_TRUE(mDecoder, NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult
WMFMediaDataDecoder::Shutdown()
{
  DebugOnly<nsresult> rv = mTaskQueue->FlushAndDispatch(
    NS_NewRunnableMethod(this, &WMFMediaDataDecoder::ProcessShutdown));
#ifdef DEBUG
  if (NS_FAILED(rv)) {
    NS_WARNING("WMFMediaDataDecoder::Shutdown() dispatch of task failed!");
  }
#endif
  return NS_OK;
}

void
WMFMediaDataDecoder::ProcessShutdown()
{
  mMFTManager->Shutdown();
  mMFTManager = nullptr;
  mDecoder = nullptr;
}


nsresult
WMFMediaDataDecoder::Input(MediaRawData* aSample)
{
  mTaskQueue->Dispatch(
    NS_NewRunnableMethodWithArg<nsRefPtr<MediaRawData>>(
      this,
      &WMFMediaDataDecoder::ProcessDecode,
      nsRefPtr<MediaRawData>(aSample)));
  return NS_OK;
}

void
WMFMediaDataDecoder::ProcessDecode(MediaRawData* aSample)
{
  HRESULT hr = mMFTManager->Input(aSample);
  if (FAILED(hr)) {
    NS_WARNING("MFTManager rejected sample");
    mCallback->Error();
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
    mCallback->Output(output);
  }
  if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
    if (mTaskQueue->IsEmpty()) {
      mCallback->InputExhausted();
    }
  } else if (FAILED(hr)) {
    NS_WARNING("WMFMediaDataDecoder failed to output data");
    mCallback->Error();
  }
}

nsresult
WMFMediaDataDecoder::Flush()
{
  
  
  
  
  mTaskQueue->Flush();

  
  NS_ENSURE_TRUE(mDecoder, NS_ERROR_FAILURE);
  HRESULT hr = mDecoder->Flush();
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  return NS_OK;
}

void
WMFMediaDataDecoder::ProcessDrain()
{
  if (mDecoder) {
    
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
  mTaskQueue->Dispatch(NS_NewRunnableMethod(this, &WMFMediaDataDecoder::ProcessDrain));
  return NS_OK;
}

bool
WMFMediaDataDecoder::IsHardwareAccelerated() const {
  return mMFTManager && mMFTManager->IsHardwareAccelerated();
}

} 
