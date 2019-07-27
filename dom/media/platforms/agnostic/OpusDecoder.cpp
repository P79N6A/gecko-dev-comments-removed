





#include "OpusDecoder.h"
#include "TimeUnits.h"
#include "VorbisUtils.h"
#include "mozilla/Endian.h"

#include <stdint.h>
#include <inttypes.h>  

#define OPUS_DEBUG(arg, ...) MOZ_LOG(gMediaDecoderLog, mozilla::LogLevel::Debug, \
    ("OpusDataDecoder(%p)::%s: " arg, this, __func__, ##__VA_ARGS__))

namespace mozilla {

extern PRLogModuleInfo* gMediaDecoderLog;

OpusDataDecoder::OpusDataDecoder(const AudioInfo& aConfig,
                                 FlushableTaskQueue* aTaskQueue,
                                 MediaDataDecoderCallback* aCallback)
  : mInfo(aConfig)
  , mTaskQueue(aTaskQueue)
  , mCallback(aCallback)
  , mOpusDecoder(nullptr)
  , mSkip(0)
  , mDecodedHeader(false)
  , mPaddingDiscarded(false)
  , mFrames(0)
{
}

OpusDataDecoder::~OpusDataDecoder()
{
  if (mOpusDecoder) {
    opus_multistream_decoder_destroy(mOpusDecoder);
    mOpusDecoder = nullptr;
  }
}

nsresult
OpusDataDecoder::Shutdown()
{
  return NS_OK;
}

nsresult
OpusDataDecoder::Init()
{
  size_t length = mInfo.mCodecSpecificConfig->Length();
  uint8_t *p = mInfo.mCodecSpecificConfig->Elements();
  if (length < sizeof(uint64_t)) {
    return NS_ERROR_FAILURE;
  }
  int64_t codecDelay = BigEndian::readUint64(p);
  length -= sizeof(uint64_t);
  p += sizeof(uint64_t);
  if (NS_FAILED(DecodeHeader(p, length))) {
    return NS_ERROR_FAILURE;
  }

  int r;
  mOpusDecoder = opus_multistream_decoder_create(mOpusParser->mRate,
                                                 mOpusParser->mChannels,
                                                 mOpusParser->mStreams,
                                                 mOpusParser->mCoupledStreams,
                                                 mOpusParser->mMappingTable,
                                                 &r);
  mSkip = mOpusParser->mPreSkip;
  mPaddingDiscarded = false;

  if (codecDelay != FramesToUsecs(mOpusParser->mPreSkip,
                                  mOpusParser->mRate).value()) {
    NS_WARNING("Invalid Opus header: CodecDelay and pre-skip do not match!");
    return NS_ERROR_FAILURE;
  }

  if (mInfo.mRate != (uint32_t)mOpusParser->mRate) {
    NS_WARNING("Invalid Opus header: container and codec rate do not match!");
  }
  if (mInfo.mChannels != (uint32_t)mOpusParser->mChannels) {
    NS_WARNING("Invalid Opus header: container and codec channels do not match!");
  }

  return r == OPUS_OK ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
OpusDataDecoder::DecodeHeader(const unsigned char* aData, size_t aLength)
{
  MOZ_ASSERT(!mOpusParser);
  MOZ_ASSERT(!mOpusDecoder);
  MOZ_ASSERT(!mDecodedHeader);
  mDecodedHeader = true;

  mOpusParser = new OpusParser;
  if (!mOpusParser->DecodeHeader(const_cast<unsigned char*>(aData), aLength)) {
    return NS_ERROR_FAILURE;
  }
  
  if (mOpusParser->mChannels > 8) {
    OPUS_DEBUG("No channel mapping for more than 8 channels. Source is %d channels",
               mOpusParser->mChannels);
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
OpusDataDecoder::Input(MediaRawData* aSample)
{
  nsCOMPtr<nsIRunnable> runnable(
    NS_NewRunnableMethodWithArg<nsRefPtr<MediaRawData>>(
      this, &OpusDataDecoder::Decode,
      nsRefPtr<MediaRawData>(aSample)));
  mTaskQueue->Dispatch(runnable.forget());

  return NS_OK;
}

void
OpusDataDecoder::Decode(MediaRawData* aSample)
{
  if (DoDecode(aSample) == -1) {
    mCallback->Error();
  } else if(mTaskQueue->IsEmpty()) {
    mCallback->InputExhausted();
  }
}

int
OpusDataDecoder::DoDecode(MediaRawData* aSample)
{
  int64_t aDiscardPadding = 0;
  if (aSample->mExtraData) {
    aDiscardPadding = BigEndian::readInt64(aSample->mExtraData->Elements());
  }
  uint32_t channels = mOpusParser->mChannels;

  if (mPaddingDiscarded) {
    
    
    OPUS_DEBUG("Opus error, discard padding on interstitial packet");
    return -1;
  }

  
  int32_t frames_number = opus_packet_get_nb_frames(aSample->mData,
                                                    aSample->mSize);
  if (frames_number <= 0) {
    OPUS_DEBUG("Invalid packet header: r=%ld length=%ld",
               frames_number, aSample->mSize);
    return -1;
  }

  int32_t samples = opus_packet_get_samples_per_frame(aSample->mData,
                                           opus_int32(mOpusParser->mRate));


  
  int32_t frames = frames_number*samples;
  if (frames < 120 || frames > 5760) {
    OPUS_DEBUG("Invalid packet frames: %ld", frames);
    return -1;
  }

  nsAutoArrayPtr<AudioDataValue> buffer(new AudioDataValue[frames * channels]);

  
#ifdef MOZ_SAMPLE_TYPE_FLOAT32
  int ret = opus_multistream_decode_float(mOpusDecoder,
                                          aSample->mData, aSample->mSize,
                                          buffer, frames, false);
#else
  int ret = opus_multistream_decode(mOpusDecoder,
                                    aSample->mData, aSample->mSize,
                                    buffer, frames, false);
#endif
  if (ret < 0) {
    return -1;
  }
  NS_ASSERTION(ret == frames, "Opus decoded too few audio samples");
  CheckedInt64 startTime = aSample->mTime;

  
  if (mSkip > 0) {
    int32_t skipFrames = std::min<int32_t>(mSkip, frames);
    int32_t keepFrames = frames - skipFrames;
    OPUS_DEBUG("Opus decoder skipping %d of %d frames", skipFrames, frames);
    PodMove(buffer.get(),
            buffer.get() + skipFrames * channels,
            keepFrames * channels);
    startTime = startTime + FramesToUsecs(skipFrames, mOpusParser->mRate);
    frames = keepFrames;
    mSkip -= skipFrames;
  }

  if (aDiscardPadding < 0) {
    
    OPUS_DEBUG("Opus error, negative discard padding");
    return -1;
  }
  if (aDiscardPadding > 0) {
    OPUS_DEBUG("OpusDecoder discardpadding %" PRId64 "", aDiscardPadding);
    CheckedInt64 discardFrames =
      TimeUnitToFrames(media::TimeUnit::FromNanoseconds(aDiscardPadding),
                       mOpusParser->mRate);
    if (!discardFrames.isValid()) {
      NS_WARNING("Int overflow in DiscardPadding");
      return -1;
    }
    if (discardFrames.value() > frames) {
      
      OPUS_DEBUG("Opus error, discard padding larger than packet");
      return -1;
    }
    OPUS_DEBUG("Opus decoder discarding %d of %d frames",
        int32_t(discardFrames.value()), frames);
    
    
    
    mPaddingDiscarded = true;
    int32_t keepFrames = frames - discardFrames.value();
    frames = keepFrames;
  }

  
#ifdef MOZ_SAMPLE_TYPE_FLOAT32
  if (mOpusParser->mGain != 1.0f) {
    float gain = mOpusParser->mGain;
    int samples = frames * channels;
    for (int i = 0; i < samples; i++) {
      buffer[i] *= gain;
    }
  }
#else
  if (mOpusParser->mGain_Q16 != 65536) {
    int64_t gain_Q16 = mOpusParser->mGain_Q16;
    int samples = frames * channels;
    for (int i = 0; i < samples; i++) {
      int32_t val = static_cast<int32_t>((gain_Q16*buffer[i] + 32768)>>16);
      buffer[i] = static_cast<AudioDataValue>(MOZ_CLIP_TO_15(val));
    }
  }
#endif

  CheckedInt64 duration = FramesToUsecs(frames, mOpusParser->mRate);
  if (!duration.isValid()) {
    NS_WARNING("OpusDataDecoder: Int overflow converting WebM audio duration");
    return -1;
  }
  CheckedInt64 time = startTime - FramesToUsecs(mOpusParser->mPreSkip,
                                                mOpusParser->mRate);
  if (!time.isValid()) {
    NS_WARNING("OpusDataDecoder: Int overflow shifting tstamp by codec delay");
    return -1;
  };

  mCallback->Output(new AudioData(aSample->mOffset,
                                  time.value(),
                                  duration.value(),
                                  frames,
                                  buffer.forget(),
                                  mOpusParser->mChannels,
                                  mOpusParser->mRate));
  mFrames += frames;
  return frames;
}

void
OpusDataDecoder::DoDrain()
{
  mCallback->DrainComplete();
}

nsresult
OpusDataDecoder::Drain()
{
  RefPtr<nsIRunnable> runnable(
    NS_NewRunnableMethod(this, &OpusDataDecoder::DoDrain));
  mTaskQueue->Dispatch(runnable.forget());
  return NS_OK;
}

nsresult
OpusDataDecoder::Flush()
{
  mTaskQueue->Flush();
  if (mOpusDecoder) {
    
    opus_multistream_decoder_ctl(mOpusDecoder, OPUS_RESET_STATE);
    mSkip = mOpusParser->mPreSkip;
    mPaddingDiscarded = false;
    mFrames = 0;
  }
  return NS_OK;
}


bool
OpusDataDecoder::IsOpus(const nsACString& aMimeType)
{
  return aMimeType.EqualsLiteral("audio/ogg; codecs=opus");
}

} 
#undef OPUS_DEBUG
