




#include "MediaCodecProxy.h"
#include <OMX_IVCommon.h>
#include <gui/Surface.h>
#include <ICrypto.h>
#include "GonkAudioDecoderManager.h"
#include "MediaDecoderReader.h"
#include "VideoUtils.h"
#include "nsTArray.h"
#include "prlog.h"
#include "stagefright/MediaBuffer.h"
#include "stagefright/MetaData.h"
#include "stagefright/MediaErrors.h"
#include <stagefright/foundation/AMessage.h>
#include <stagefright/foundation/ALooper.h>
#include "media/openmax/OMX_Audio.h"

#define LOG_TAG "GonkAudioDecoderManager"
#include <android/log.h>
#define ALOG(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#ifdef PR_LOGGING
PRLogModuleInfo* GetDemuxerLog();
#define LOG(...) PR_LOG(GetDemuxerLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define LOG(...)
#endif
#define READ_OUTPUT_BUFFER_TIMEOUT_US  3000

using namespace android;
typedef android::MediaCodecProxy MediaCodecProxy;

namespace mozilla {

GonkAudioDecoderManager::GonkAudioDecoderManager(
  const mp4_demuxer::AudioDecoderConfig& aConfig)
  : mAudioChannels(aConfig.channel_count)
  , mAudioRate(aConfig.samples_per_second)
  , mAudioProfile(aConfig.aac_profile)
  , mAudioBuffer(nullptr)
{
  MOZ_COUNT_CTOR(GonkAudioDecoderManager);
  MOZ_ASSERT(mAudioChannels);
  mUserData.AppendElements(&aConfig.audio_specific_config[0],
                           aConfig.audio_specific_config.length());
}

GonkAudioDecoderManager::~GonkAudioDecoderManager()
{
  MOZ_COUNT_DTOR(GonkAudioDecoderManager);
}

android::sp<MediaCodecProxy>
GonkAudioDecoderManager::Init(MediaDataDecoderCallback* aCallback)
{
  if (mLooper != nullptr) {
    return nullptr;
  }
  
  mLooper = new ALooper;
  mLooper->setName("GonkAudioDecoderManager");
  mLooper->start();

  mDecoder = MediaCodecProxy::CreateByType(mLooper, "audio/mp4a-latm", false, false, nullptr);
  if (!mDecoder.get()) {
    return nullptr;
  }
  sp<AMessage> format = new AMessage;
  
  ALOG("Init Audio channel no:%d, sample-rate:%d", mAudioChannels, mAudioRate);
  format->setString("mime", "audio/mp4a-latm");
  format->setInt32("channel-count", mAudioChannels);
  format->setInt32("sample-rate", mAudioRate);
  format->setInt32("aac-profile", mAudioProfile);
  format->setInt32("is-adts", true);
  status_t err = mDecoder->configure(format, nullptr, nullptr, 0);
  if (err != OK || !mDecoder->Prepare()) {
    return nullptr;
  }
  status_t rv = mDecoder->Input(mUserData.Elements(), mUserData.Length(), 0,
                                android::MediaCodec::BUFFER_FLAG_CODECCONFIG);

  if (rv == OK) {
    return mDecoder;
  } else {
    ALOG("Failed to input codec specific data!");
    return nullptr;
  }
}

nsresult
GonkAudioDecoderManager::CreateAudioData(int64_t aStreamOffset, AudioData **v) {

  void *data;
  size_t dataOffset;
  size_t size;
  int64_t timeUs;

  if (!mAudioBuffer->meta_data()->findInt64(kKeyTime, &timeUs)) {
    return NS_ERROR_UNEXPECTED;
  }
  data = mAudioBuffer->data();
  dataOffset = mAudioBuffer->range_offset();
  size = mAudioBuffer->range_length();

  nsAutoArrayPtr<AudioDataValue> buffer(new AudioDataValue[size/2] );
  memcpy(buffer.get(), data+dataOffset, size);
  uint32_t frames = size / (2 * mAudioChannels);

  CheckedInt64 duration = FramesToUsecs(frames, mAudioRate);
  if (!duration.isValid()) {
    return NS_ERROR_UNEXPECTED;
  }
  *v = new AudioData(aStreamOffset, timeUs, duration.value(), frames, buffer.forget(),
		     mAudioChannels);
  ReleaseAudioBuffer();
  return NS_OK;
}

nsresult
GonkAudioDecoderManager::Output(int64_t aStreamOffset,
                                nsAutoPtr<MediaData>& aOutData)
{
  aOutData = nullptr;
  status_t err;
  err = mDecoder->Output(&mAudioBuffer, READ_OUTPUT_BUFFER_TIMEOUT_US);

  switch (err) {
    case OK:
    {
      if (mAudioBuffer && mAudioBuffer->range_length() != 0) {
        int64_t timeUs;
        if (!mAudioBuffer->meta_data()->findInt64(kKeyTime, &timeUs)) {
          return NS_ERROR_UNEXPECTED;
	}
      }
      AudioData* data = nullptr;
      nsresult rv = CreateAudioData(aStreamOffset, &data);
      
      if (rv != NS_OK) {
        return NS_ERROR_UNEXPECTED;
      }
      aOutData = data;
      return NS_OK;
    }
    case android::INFO_FORMAT_CHANGED:
    case android::INFO_OUTPUT_BUFFERS_CHANGED:
    {
      
      ALOG("Decoder format changed");
      return Output(aStreamOffset, aOutData);
    }
    case -EAGAIN:
    {
      return NS_ERROR_NOT_AVAILABLE;
    }
    case android::ERROR_END_OF_STREAM:
    {
      ALOG("End of Stream");
      return NS_ERROR_ABORT;
    }
    case -ETIMEDOUT:
    {
      ALOG("Timeout. can try again next time");
      return NS_ERROR_UNEXPECTED;
    }
    default:
    {
      ALOG("Decoder failed, err=%d", err);
      return NS_ERROR_UNEXPECTED;
    }
  }

  return NS_OK;
}

void GonkAudioDecoderManager::ReleaseAudioBuffer() {
  if (mAudioBuffer) {
    sp<MetaData> metaData = mAudioBuffer->meta_data();
    int32_t index;
    metaData->findInt32(android::MediaCodecProxy::kKeyBufferIndex, &index);
    mAudioBuffer->release();
    mAudioBuffer = nullptr;
    mDecoder->releaseOutputBuffer(index);
  }
}

nsresult
GonkAudioDecoderManager::Input(mp4_demuxer::MP4Sample* aSample)
{
  const uint8_t* data = reinterpret_cast<const uint8_t*>(aSample->data);
  uint32_t length = aSample->size;
  status_t rv = mDecoder->Input(data, length, aSample->composition_timestamp, 0);
  return rv == OK ? NS_OK : NS_ERROR_UNEXPECTED;
}

} 
