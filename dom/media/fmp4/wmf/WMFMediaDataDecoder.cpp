





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
                                         MediaTaskQueue* aTaskQueue,
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
  mTaskQueue->FlushAndDispatch(NS_NewRunnableMethod(this, &WMFMediaDataDecoder::ProcessShutdown));
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
WMFMediaDataDecoder::Input(mp4_demuxer::MP4Sample* aSample)
{
  mTaskQueue->Dispatch(
    NS_NewRunnableMethodWithArg<nsAutoPtr<mp4_demuxer::MP4Sample>>(
      this,
      &WMFMediaDataDecoder::ProcessDecode,
      nsAutoPtr<mp4_demuxer::MP4Sample>(aSample)));
  return NS_OK;
}

void
WMFMediaDataDecoder::ProcessDecode(mp4_demuxer::MP4Sample* aSample)
{
  HRESULT hr = mMFTManager->Input(aSample);
  if (FAILED(hr)) {
    NS_WARNING("MFTManager rejected sample");
    mCallback->Error();
    return;
  }

  mLastStreamOffset = aSample->byte_offset;

  ProcessOutput();
}

void
WMFMediaDataDecoder::ProcessOutput()
{
  nsAutoPtr<MediaData> output;
  HRESULT hr = S_OK;
  while (SUCCEEDED(hr = mMFTManager->Output(mLastStreamOffset, output)) &&
         output) {
    mCallback->Output(output.forget());
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
  
  if (FAILED(mDecoder->SendMFTMessage(MFT_MESSAGE_COMMAND_DRAIN, 0))) {
    NS_WARNING("Failed to send DRAIN command to MFT");
  }
  
  ProcessOutput();
  mCallback->DrainComplete();
}

nsresult
WMFMediaDataDecoder::Drain()
{
  mTaskQueue->Dispatch(NS_NewRunnableMethod(this, &WMFMediaDataDecoder::ProcessDrain));
  return NS_OK;
}

} 
