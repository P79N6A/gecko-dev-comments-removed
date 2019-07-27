




#include "GMPAudioDecoderParent.h"
#include "GMPParent.h"
#include <stdio.h>
#include "mozilla/unused.h"
#include "GMPMessageUtils.h"
#include "nsThreadUtils.h"
#include "prlog.h"

namespace mozilla {

#ifdef LOG
#undef LOG
#endif

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetGMPLog();

#define LOGD(msg) PR_LOG(GetGMPLog(), PR_LOG_DEBUG, msg)
#define LOG(level, msg) PR_LOG(GetGMPLog(), (level), msg)
#else
#define LOGD(msg)
#define LOG(level, msg)
#endif

namespace gmp {

GMPAudioDecoderParent::GMPAudioDecoderParent(GMPParent* aPlugin)
  : mIsOpen(false)
  , mPlugin(aPlugin)
  , mCallback(nullptr)
{
  MOZ_ASSERT(mPlugin);
}

GMPAudioDecoderParent::~GMPAudioDecoderParent()
{
}

nsresult
GMPAudioDecoderParent::InitDecode(GMPAudioCodecType aCodecType,
                                  uint32_t aChannelCount,
                                  uint32_t aBitsPerChannel,
                                  uint32_t aSamplesPerSecond,
                                  nsTArray<uint8_t>& aExtraData,
                                  GMPAudioDecoderProxyCallback* aCallback)
{
  if (mIsOpen) {
    NS_WARNING("Trying to re-init an in-use GMP audio decoder!");
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  if (!aCallback) {
    return NS_ERROR_FAILURE;
  }
  mCallback = aCallback;

  GMPAudioCodecData data;
  data.mCodecType() = aCodecType;
  data.mChannelCount() = aChannelCount;
  data.mBitsPerChannel() = aBitsPerChannel;
  data.mSamplesPerSecond() = aSamplesPerSecond;
  data.mExtraData() = aExtraData;
  if (!SendInitDecode(data)) {
    return NS_ERROR_FAILURE;
  }
  mIsOpen = true;

  
  return NS_OK;
}

nsresult
GMPAudioDecoderParent::Decode(GMPAudioSamplesImpl& aEncodedSamples)
{

  if (!mIsOpen) {
    NS_WARNING("Trying to use a dead GMP Audio decoder!");
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  GMPAudioEncodedSampleData samples;
  aEncodedSamples.RelinquishData(samples);

  if (!SendDecode(samples)) {
    return NS_ERROR_FAILURE;
  }

  
  return NS_OK;
}

nsresult
GMPAudioDecoderParent::Reset()
{
  if (!mIsOpen) {
    NS_WARNING("Trying to use a dead GMP Audio decoder!");
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  if (!SendReset()) {
    return NS_ERROR_FAILURE;
  }

  
  return NS_OK;
}

nsresult
GMPAudioDecoderParent::Drain()
{
  if (!mIsOpen) {
    NS_WARNING("Trying to use a dead GMP Audio decoder!");
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  if (!SendDrain()) {
    return NS_ERROR_FAILURE;
  }

  
  return NS_OK;
}


nsresult
GMPAudioDecoderParent::Close()
{
  LOGD(("%s: %p", __FUNCTION__, this));
  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  
  
  mCallback = nullptr;
  

  
  nsRefPtr<GMPAudioDecoderParent> kungfudeathgrip(this);
  NS_RELEASE(kungfudeathgrip);
  Shutdown();

  return NS_OK;
}


nsresult
GMPAudioDecoderParent::Shutdown()
{
  LOGD(("%s: %p", __FUNCTION__, this));
  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  
  if (mCallback) {
    mCallback->Terminated();
    mCallback = nullptr;
  }

  if (mIsOpen) {
    
    mIsOpen = false;
    unused << SendDecodingComplete();
  }

  return NS_OK;
}


void
GMPAudioDecoderParent::ActorDestroy(ActorDestroyReason aWhy)
{
  mIsOpen = false;
  if (mCallback) {
    
    mCallback->Terminated();
    mCallback = nullptr;
  }
  if (mPlugin) {
    
    mPlugin->AudioDecoderDestroyed(this);
    mPlugin = nullptr;
  }
}

bool
GMPAudioDecoderParent::RecvDecoded(const GMPAudioDecodedSampleData& aDecoded)
{
  if (!mCallback) {
    return false;
  }

  mCallback->Decoded(aDecoded.mData(), aDecoded.mTimeStamp());

  return true;
}

bool
GMPAudioDecoderParent::RecvInputDataExhausted()
{
  if (!mCallback) {
    return false;
  }

  
  mCallback->InputDataExhausted();

  return true;
}

bool
GMPAudioDecoderParent::RecvDrainComplete()
{
  if (!mCallback) {
    return false;
  }

  
  mCallback->DrainComplete();

  return true;
}

bool
GMPAudioDecoderParent::RecvResetComplete()
{
  if (!mCallback) {
    return false;
  }

  
  mCallback->ResetComplete();

  return true;
}

bool
GMPAudioDecoderParent::RecvError(const GMPErr& aError)
{
  if (!mCallback) {
    return false;
  }

  
  mCallback->Error(aError);

  return true;
}

bool
GMPAudioDecoderParent::Recv__delete__()
{
  if (mPlugin) {
    
    mPlugin->AudioDecoderDestroyed(this);
    mPlugin = nullptr;
  }

  return true;
}

} 
} 
