



#include "mp4_demuxer/Adts.h"
#include "mp4_demuxer/DecoderData.h"
#include "media/stagefright/MetaData.h"
#include "media/stagefright/MediaBuffer.h"

using namespace android;

namespace mp4_demuxer
{

static int32_t
FindInt32(sp<MetaData>& mMetaData, uint32_t mKey)
{
  int32_t value;
  if (!mMetaData->findInt32(mKey, &value))
    return 0;
  return value;
}

static int64_t
FindInt64(sp<MetaData>& mMetaData, uint32_t mKey)
{
  int64_t value;
  if (!mMetaData->findInt64(mKey, &value))
    return 0;
  return value;
}

void
AudioDecoderConfig::Update(sp<MetaData>& aMetaData, const char* aMimeType)
{
  
  mime_type = aMimeType;
  duration = FindInt64(aMetaData, kKeyDuration);
  channel_count = FindInt32(aMetaData, kKeyChannelCount);
  bytes_per_sample = FindInt32(aMetaData, kKeySampleSize);
  samples_per_second = FindInt32(aMetaData, kKeySampleRate);
  frequency_index = Adts::GetFrequencyIndex(samples_per_second);
  aac_profile = FindInt32(aMetaData, kKeyAACProfile);

  uint32_t type;
  aMetaData->findData(kKeyAVCC, &type, &extra_data, &extra_data_size);
}

bool
AudioDecoderConfig::IsValid()
{
  return duration > 0 && channel_count > 0 && samples_per_second > 0;
}

void
VideoDecoderConfig::Update(sp<MetaData>& aMetaData, const char* aMimeType)
{
  
  mime_type = aMimeType;
  duration = FindInt64(aMetaData, kKeyDuration);
  display_width = FindInt32(aMetaData, kKeyDisplayWidth);
  display_height = FindInt32(aMetaData, kKeyDisplayHeight);

  
  const void* meta_extra_data;
  uint32_t type;
  aMetaData->findData(kKeyAVCC, &type, &meta_extra_data, &extra_data_size);
  extra_data = new uint8_t[extra_data_size];
  memcpy(extra_data, meta_extra_data, extra_data_size);
}

bool
VideoDecoderConfig::IsValid()
{
  return display_width > 0 && display_height > 0 && duration > 0;
}

MP4Sample::MP4Sample()
  : mMediaBuffer(nullptr)
  , composition_timestamp(0)
  , duration(0)
  , byte_offset(0)
  , is_sync_point(0)
  , data(nullptr)
  , size(0)
{
}

MP4Sample::~MP4Sample()
{
  if (mMediaBuffer) {
    mMediaBuffer->release();
  }
}

void
MP4Sample::Update()
{
  sp<MetaData> m = mMediaBuffer->meta_data();
  composition_timestamp = FindInt64(m, kKeyTime);
  duration = FindInt64(m, kKeyDuration);
  byte_offset = FindInt64(m, kKey64BitFileOffset);
  is_sync_point = FindInt32(m, kKeyIsSyncFrame);
  data = reinterpret_cast<uint8_t*>(mMediaBuffer->data());
  size = mMediaBuffer->range_length();
}
}
