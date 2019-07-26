



#include "OpusTrackEncoder.h"
#include "nsString.h"

#include <opus/opus.h>

#undef LOG
#ifdef MOZ_WIDGET_GONK
#include <android/log.h>
#define LOG(args...) __android_log_print(ANDROID_LOG_INFO, "MediaEncoder", ## args);
#else
#define LOG(args, ...)
#endif

namespace mozilla {



static const int MAX_CHANNELS = 2;


static const int MAX_DATA_BYTES = 4096;




static const int kOpusSamplingRate = 48000;


static const int kFrameDurationMs  = 20;

namespace {



template<typename T>
static void
SerializeToBuffer(T aValue, nsTArray<uint8_t>* aOutput)
{
  for (uint32_t i = 0; i < sizeof(T); i++) {
    aOutput->AppendElement((uint8_t)(0x000000ff & (aValue >> (i * 8))));
  }
}

static inline void
SerializeToBuffer(const nsCString& aComment, nsTArray<uint8_t>* aOutput)
{
  
  
  SerializeToBuffer((uint32_t)(aComment.Length()), aOutput);
  aOutput->AppendElements(aComment.get(), aComment.Length());
}


static void
SerializeOpusIdHeader(uint8_t aChannelCount, uint16_t aPreskip,
                      uint32_t aInputSampleRate, nsTArray<uint8_t>* aOutput)
{
  
  static const uint8_t magic[9] = "OpusHead";
  memcpy(aOutput->AppendElements(sizeof(magic) - 1), magic, sizeof(magic) - 1);

  
  aOutput->AppendElement(1);

  
  aOutput->AppendElement(aChannelCount);

  
  
  SerializeToBuffer(aPreskip, aOutput);

  
  SerializeToBuffer(aInputSampleRate, aOutput);

  
  
  SerializeToBuffer((int16_t)0, aOutput);

  
  
  aOutput->AppendElement(0);
}

static void
SerializeOpusCommentHeader(const nsCString& aVendor,
                           const nsTArray<nsCString>& aComments,
                           nsTArray<uint8_t>* aOutput)
{
  
  static const uint8_t magic[9] = "OpusTags";
  memcpy(aOutput->AppendElements(sizeof(magic) - 1), magic, sizeof(magic) - 1);

  
  
  
  SerializeToBuffer(aVendor, aOutput);

  
  
  
  
  
  
  SerializeToBuffer((uint32_t)aComments.Length(), aOutput);
  for (uint32_t i = 0; i < aComments.Length(); ++i) {
    SerializeToBuffer(aComments[i], aOutput);
  }
}

}  

OpusTrackEncoder::OpusTrackEncoder()
  : AudioTrackEncoder()
  , mEncoder(nullptr)
  , mLookahead(0)
  , mResampler(nullptr)
{
}

OpusTrackEncoder::~OpusTrackEncoder()
{
  if (mEncoder) {
    opus_encoder_destroy(mEncoder);
  }
  if (mResampler) {
    speex_resampler_destroy(mResampler);
    mResampler = nullptr;
  }
}

nsresult
OpusTrackEncoder::Init(int aChannels, int aSamplingRate)
{
  
  
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  
  
  
  mChannels = aChannels > MAX_CHANNELS ? MAX_CHANNELS : aChannels;

  if (aChannels <= 0) {
    return NS_ERROR_FAILURE;
  }
  
  
  
  
  if (!((aSamplingRate >= 8000) && (kOpusSamplingRate / aSamplingRate) *
         aSamplingRate == kOpusSamplingRate)) {
    int error;
    mResampler = speex_resampler_init(mChannels,
                                      aSamplingRate,
                                      kOpusSamplingRate,
                                      SPEEX_RESAMPLER_QUALITY_DEFAULT,
                                      &error);

    if (error != RESAMPLER_ERR_SUCCESS) {
      return NS_ERROR_FAILURE;
    }
  }
  mSamplingRate = aSamplingRate;

  int error = 0;
  mEncoder = opus_encoder_create(GetOutputSampleRate(), mChannels,
                                 OPUS_APPLICATION_AUDIO, &error);

  mInitialized = (error == OPUS_OK);

  mReentrantMonitor.NotifyAll();

  return error == OPUS_OK ? NS_OK : NS_ERROR_FAILURE;
}

int
OpusTrackEncoder::GetOutputSampleRate()
{
  return mResampler ? kOpusSamplingRate : mSamplingRate;
}

int
OpusTrackEncoder::GetPacketDuration()
{
  return GetOutputSampleRate() * kFrameDurationMs / 1000;
}

already_AddRefed<TrackMetadataBase>
OpusTrackEncoder::GetMetadata()
{
  {
    
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    while (!mCanceled && !mInitialized) {
      mReentrantMonitor.Wait();
    }
  }

  if (mCanceled || mEncodingComplete) {
    return nullptr;
  }

  nsRefPtr<OpusMetadata> meta = new OpusMetadata();

  mLookahead = 0;
  int error = opus_encoder_ctl(mEncoder, OPUS_GET_LOOKAHEAD(&mLookahead));
  if (error != OPUS_OK) {
    mLookahead = 0;
  }

  
  SerializeOpusIdHeader(mChannels, mLookahead*(kOpusSamplingRate/mSamplingRate),
                        mSamplingRate, &meta->mIdHeader);

  nsCString vendor;
  vendor.AppendASCII(opus_get_version_string());

  nsTArray<nsCString> comments;
  comments.AppendElement(NS_LITERAL_CSTRING("ENCODER=Mozilla" MOZ_APP_UA_VERSION));

  SerializeOpusCommentHeader(vendor, comments,
                             &meta->mCommentHeader);

  return meta.forget();
}

nsresult
OpusTrackEncoder::GetEncodedTrack(EncodedFrameContainer& aData)
{
  {
    
    
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    
    
    while (!mCanceled && (!mInitialized || (mRawSegment.GetDuration() +
           mSourceSegment.GetDuration() < GetPacketDuration() &&
           !mEndOfStream))) {
      mReentrantMonitor.Wait();
    }

    if (mCanceled || mEncodingComplete) {
      return NS_ERROR_FAILURE;
    }

    mSourceSegment.AppendFrom(&mRawSegment);

    
    
    if (mEndOfStream && !mEosSetInEncoder) {
      mEosSetInEncoder = true;
      mSourceSegment.AppendNullData(mLookahead);
    }
  }

  
  nsAutoTArray<AudioDataValue, 9600> pcm;
  pcm.SetLength(GetPacketDuration() * mChannels);
  AudioSegment::ChunkIterator iter(mSourceSegment);
  int frameCopied = 0;
  while (!iter.IsEnded() && frameCopied < GetPacketDuration()) {
    AudioChunk chunk = *iter;

    
    int frameToCopy = chunk.GetDuration();
    if (frameCopied + frameToCopy > GetPacketDuration()) {
      frameToCopy = GetPacketDuration() - frameCopied;
    }

    if (!chunk.IsNull()) {
      
      InterleaveTrackData(chunk, frameToCopy, mChannels,
                          pcm.Elements() + frameCopied * mChannels);
    } else {
      memset(pcm.Elements() + frameCopied * mChannels, 0,
             frameToCopy * mChannels * sizeof(AudioDataValue));
    }

    frameCopied += frameToCopy;
    iter.Next();
  }

  nsRefPtr<EncodedFrame> audiodata = new EncodedFrame();
  audiodata->SetFrameType(EncodedFrame::AUDIO_FRAME);
  if (mResampler) {
    nsAutoTArray<AudioDataValue, 9600> resamplingDest;
    
    
    
    uint32_t outframes = frameCopied * kOpusSamplingRate / mSamplingRate + 1;
    uint32_t inframes = frameCopied;

    resamplingDest.SetLength(outframes * mChannels);

#if MOZ_SAMPLE_TYPE_S16
    short* in = reinterpret_cast<short*>(pcm.Elements());
    short* out = reinterpret_cast<short*>(resamplingDest.Elements());
    speex_resampler_process_interleaved_int(mResampler, in, &inframes,
                                                        out, &outframes);
#else
    float* in = reinterpret_cast<float*>(pcm.Elements());
    float* out = reinterpret_cast<float*>(resamplingDest.Elements());
    speex_resampler_process_interleaved_float(mResampler, in, &inframes,
                                                          out, &outframes);
#endif

    pcm = resamplingDest;
    
    audiodata->SetDuration(outframes);
  } else {
    
    audiodata->SetDuration(frameCopied * (kOpusSamplingRate / mSamplingRate));
  }

  
  
  
  mSourceSegment.RemoveLeading(frameCopied);

  
  
  if (mSourceSegment.GetDuration() == 0 && mEndOfStream) {
    mEncodingComplete = true;
    LOG("[Opus] Done encoding.");
  }

  
  
  if (frameCopied < GetPacketDuration() && mEndOfStream) {
    memset(pcm.Elements() + frameCopied * mChannels, 0,
           (GetPacketDuration()-frameCopied)*mChannels*sizeof(AudioDataValue));
  }
  nsTArray<uint8_t> frameData;
  
  frameData.SetLength(MAX_DATA_BYTES);
  
  int result = 0;
#ifdef MOZ_SAMPLE_TYPE_S16
  const opus_int16* pcmBuf = static_cast<opus_int16*>(pcm.Elements());
  result = opus_encode(mEncoder, pcmBuf, GetPacketDuration(),
                       frameData.Elements(), MAX_DATA_BYTES);
#else
  const float* pcmBuf = static_cast<float*>(pcm.Elements());
  result = opus_encode_float(mEncoder, pcmBuf, GetPacketDuration(),
                             frameData.Elements(), MAX_DATA_BYTES);
#endif
  frameData.SetLength(result >= 0 ? result : 0);

  if (result < 0) {
    LOG("[Opus] Fail to encode data! Result: %s.", opus_strerror(result));
  }
  if (mEncodingComplete) {
    if (mResampler) {
      speex_resampler_destroy(mResampler);
      mResampler = nullptr;
    }
  }

  audiodata->SetFrameData(&frameData);
  aData.AppendEncodedFrame(audiodata);
  return result >= 0 ? NS_OK : NS_ERROR_FAILURE;
}

}
