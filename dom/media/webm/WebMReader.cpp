




#include "nsError.h"
#include "MediaDecoderStateMachine.h"
#include "AbstractMediaDecoder.h"
#include "MediaResource.h"
#include "SoftwareWebMVideoDecoder.h"
#include "WebMReader.h"
#include "WebMBufferedParser.h"
#include "mozilla/dom/TimeRanges.h"
#include "VorbisUtils.h"
#include "gfx2DGlue.h"
#include "Layers.h"
#include "mozilla/Preferences.h"
#include "SharedThreadPool.h"

#include <algorithm>

#define VPX_DONT_DEFINE_STDINT_TYPES
#include "vpx/vp8dx.h"
#include "vpx/vpx_decoder.h"

#include "OggReader.h"


#if defined(MOZ_PDM_VPX)
#include "IntelWebMVideoDecoder.h"
#endif




#undef LOG

#ifdef PR_LOGGING
#include "prprf.h"
#define LOG(type, msg) PR_LOG(gMediaDecoderLog, type, msg)
#ifdef SEEK_LOGGING
#define SEEK_LOG(type, msg) PR_LOG(gMediaDecoderLog, type, msg)
#else
#define SEEK_LOG(type, msg)
#endif
#else
#define LOG(type, msg)
#define SEEK_LOG(type, msg)
#endif

namespace mozilla {

using namespace gfx;
using namespace layers;

extern PRLogModuleInfo* gMediaDecoderLog;
PRLogModuleInfo* gNesteggLog;




static int webm_read(void *aBuffer, size_t aLength, void *aUserData)
{
  MOZ_ASSERT(aUserData);
  AbstractMediaDecoder* decoder =
    reinterpret_cast<AbstractMediaDecoder*>(aUserData);
  MediaResource* resource = decoder->GetResource();
  NS_ASSERTION(resource, "Decoder has no media resource");

  nsresult rv = NS_OK;
  bool eof = false;

  char *p = static_cast<char *>(aBuffer);
  while (NS_SUCCEEDED(rv) && aLength > 0) {
    uint32_t bytes = 0;
    rv = resource->Read(p, aLength, &bytes);
    if (bytes == 0) {
      eof = true;
      break;
    }
    aLength -= bytes;
    p += bytes;
  }

  return NS_FAILED(rv) ? -1 : eof ? 0 : 1;
}

static int webm_seek(int64_t aOffset, int aWhence, void *aUserData)
{
  MOZ_ASSERT(aUserData);
  AbstractMediaDecoder* decoder =
    reinterpret_cast<AbstractMediaDecoder*>(aUserData);
  MediaResource* resource = decoder->GetResource();
  NS_ASSERTION(resource, "Decoder has no media resource");
  nsresult rv = resource->Seek(aWhence, aOffset);
  return NS_SUCCEEDED(rv) ? 0 : -1;
}

static int64_t webm_tell(void *aUserData)
{
  MOZ_ASSERT(aUserData);
  AbstractMediaDecoder* decoder =
    reinterpret_cast<AbstractMediaDecoder*>(aUserData);
  MediaResource* resource = decoder->GetResource();
  NS_ASSERTION(resource, "Decoder has no media resource");
  return resource->Tell();
}

static void webm_log(nestegg * context,
                     unsigned int severity,
                     char const * format, ...)
{
#ifdef PR_LOGGING
  va_list args;
  char msg[256];
  const char * sevStr;

  switch(severity) {
    case NESTEGG_LOG_DEBUG:
      sevStr = "DBG";
      break;
    case NESTEGG_LOG_INFO:
      sevStr = "INF";
      break;
    case NESTEGG_LOG_WARNING:
      sevStr = "WRN";
      break;
    case NESTEGG_LOG_ERROR:
      sevStr = "ERR";
      break;
    case NESTEGG_LOG_CRITICAL:
      sevStr = "CRT";
      break;
    default:
      sevStr = "UNK";
      break;
  }

  va_start(args, format);

  PR_snprintf(msg, sizeof(msg), "%p [Nestegg-%s] ", context, sevStr);
  PR_vsnprintf(msg+strlen(msg), sizeof(msg)-strlen(msg), format, args);
  PR_LOG(gNesteggLog, PR_LOG_DEBUG, (msg));

  va_end(args);
#endif
}

ogg_packet InitOggPacket(const unsigned char* aData, size_t aLength,
                         bool aBOS, bool aEOS,
                         int64_t aGranulepos, int64_t aPacketNo)
{
  ogg_packet packet;
  packet.packet = const_cast<unsigned char*>(aData);
  packet.bytes = aLength;
  packet.b_o_s = aBOS;
  packet.e_o_s = aEOS;
  packet.granulepos = aGranulepos;
  packet.packetno = aPacketNo;
  return packet;
}

#if defined(MOZ_PDM_VPX)
static bool sIsIntelDecoderEnabled = false;
#endif

WebMReader::WebMReader(AbstractMediaDecoder* aDecoder)
  : MediaDecoderReader(aDecoder)
  , mContext(nullptr)
  , mPacketCount(0)
  , mOpusDecoder(nullptr)
  , mSkip(0)
  , mSeekPreroll(0)
  , mVideoTrack(0)
  , mAudioTrack(0)
  , mAudioStartUsec(-1)
  , mAudioFrames(0)
  , mLastVideoFrameTime(0)
  , mAudioCodec(-1)
  , mVideoCodec(-1)
  , mLayersBackendType(layers::LayersBackend::LAYERS_NONE)
  , mHasVideo(false)
  , mHasAudio(false)
  , mPaddingDiscarded(false)
{
  MOZ_COUNT_CTOR(WebMReader);
#ifdef PR_LOGGING
  if (!gNesteggLog) {
    gNesteggLog = PR_NewLogModule("Nestegg");
  }
#endif
  
  
  memset(&mVorbisBlock, 0, sizeof(vorbis_block));
  memset(&mVorbisDsp, 0, sizeof(vorbis_dsp_state));
  memset(&mVorbisInfo, 0, sizeof(vorbis_info));
  memset(&mVorbisComment, 0, sizeof(vorbis_comment));

#if defined(MOZ_PDM_VPX)
  sIsIntelDecoderEnabled = Preferences::GetBool("media.webm.intel_decoder.enabled", false);
#endif
}

WebMReader::~WebMReader()
{
  Cleanup();
  mVideoPackets.Reset();
  mAudioPackets.Reset();
  vorbis_block_clear(&mVorbisBlock);
  vorbis_dsp_clear(&mVorbisDsp);
  vorbis_info_clear(&mVorbisInfo);
  vorbis_comment_clear(&mVorbisComment);
  if (mOpusDecoder) {
    opus_multistream_decoder_destroy(mOpusDecoder);
    mOpusDecoder = nullptr;
  }
  MOZ_ASSERT(!mVideoDecoder);
  MOZ_COUNT_DTOR(WebMReader);
}

void WebMReader::Shutdown()
{
#if defined(MOZ_PDM_VPX)
  if (mVideoTaskQueue) {
    mVideoTaskQueue->BeginShutdown();
    mVideoTaskQueue->AwaitShutdownAndIdle();
  }
#endif

  if (mVideoDecoder) {
    mVideoDecoder->Shutdown();
    mVideoDecoder = nullptr;
  }

  MediaDecoderReader::Shutdown();
}

nsresult WebMReader::Init(MediaDecoderReader* aCloneDonor)
{
  vorbis_info_init(&mVorbisInfo);
  vorbis_comment_init(&mVorbisComment);
  memset(&mVorbisDsp, 0, sizeof(vorbis_dsp_state));
  memset(&mVorbisBlock, 0, sizeof(vorbis_block));

#if defined(MOZ_PDM_VPX)
  if (sIsIntelDecoderEnabled) {
    PlatformDecoderModule::Init();

    InitLayersBackendType();

    mVideoTaskQueue = new MediaTaskQueue(
      SharedThreadPool::Get(NS_LITERAL_CSTRING("IntelVP8 Video Decode")));
    NS_ENSURE_TRUE(mVideoTaskQueue, NS_ERROR_FAILURE);
  }
#endif

  if (aCloneDonor) {
    mBufferedState = static_cast<WebMReader*>(aCloneDonor)->mBufferedState;
  } else {
    mBufferedState = new WebMBufferedState;
  }

  return NS_OK;
}

void WebMReader::InitLayersBackendType()
{
  if (!IsVideoContentType(GetDecoder()->GetResource()->GetContentType())) {
    
    return;
  }
  
  
  
  MediaDecoderOwner* owner = mDecoder->GetOwner();
  if (!owner) {
    NS_WARNING("WebMReader without a decoder owner, can't get HWAccel");
    return;
  }

  dom::HTMLMediaElement* element = owner->GetMediaElement();
  NS_ENSURE_TRUE_VOID(element);

  nsRefPtr<LayerManager> layerManager =
    nsContentUtils::LayerManagerForDocument(element->OwnerDoc());
  NS_ENSURE_TRUE_VOID(layerManager);

  mLayersBackendType = layerManager->GetCompositorBackendType();
}

nsresult WebMReader::ResetDecode()
{
  mAudioFrames = 0;
  mAudioStartUsec = -1;
  nsresult res = NS_OK;
  if (NS_FAILED(MediaDecoderReader::ResetDecode())) {
    res = NS_ERROR_FAILURE;
  }

  if (mAudioCodec == NESTEGG_CODEC_VORBIS) {
    
    
    
    vorbis_synthesis_restart(&mVorbisDsp);
  } else if (mAudioCodec == NESTEGG_CODEC_OPUS) {
    if (mOpusDecoder) {
      
      opus_multistream_decoder_ctl(mOpusDecoder, OPUS_RESET_STATE);
      mSkip = mOpusParser->mPreSkip;
      mPaddingDiscarded = false;
    }
  }

  mVideoPackets.Reset();
  mAudioPackets.Reset();

  return res;
}

void WebMReader::Cleanup()
{
  if (mContext) {
    nestegg_destroy(mContext);
    mContext = nullptr;
  }
}

nsresult WebMReader::ReadMetadata(MediaInfo* aInfo,
                                  MetadataTags** aTags)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  nestegg_io io;
  io.read = webm_read;
  io.seek = webm_seek;
  io.tell = webm_tell;
  io.userdata = mDecoder;
  int64_t maxOffset = mDecoder->HasInitializationData() ?
    mDecoder->GetResource()->GetLength() : -1;
  int r = nestegg_init(&mContext, io, &webm_log, maxOffset);
  if (r == -1) {
    return NS_ERROR_FAILURE;
  }

  uint64_t duration = 0;
  r = nestegg_duration(mContext, &duration);
  if (r == 0) {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDecoder->SetMediaDuration(duration / NS_PER_USEC);
  }

  unsigned int ntracks = 0;
  r = nestegg_track_count(mContext, &ntracks);
  if (r == -1) {
    Cleanup();
    return NS_ERROR_FAILURE;
  }

  for (uint32_t track = 0; track < ntracks; ++track) {
    int id = nestegg_track_codec_id(mContext, track);
    if (id == -1) {
      Cleanup();
      return NS_ERROR_FAILURE;
    }
    int type = nestegg_track_type(mContext, track);
    if (!mHasVideo && type == NESTEGG_TRACK_VIDEO &&
        mDecoder->GetImageContainer()) {
      nestegg_video_params params;
      r = nestegg_track_video_params(mContext, track, &params);
      if (r == -1) {
        Cleanup();
        return NS_ERROR_FAILURE;
      }

      mVideoCodec = nestegg_track_codec_id(mContext, track);

#if defined(MOZ_PDM_VPX)
      if (sIsIntelDecoderEnabled) {
        mVideoDecoder = IntelWebMVideoDecoder::Create(this);
        if (mVideoDecoder &&
            NS_FAILED(mVideoDecoder->Init(params.display_width, params.display_height))) {
          mVideoDecoder = nullptr;
        }
      }
#endif

      
      if (!mVideoDecoder) {
        mVideoDecoder = SoftwareWebMVideoDecoder::Create(this);
        if (mVideoDecoder &&
            NS_FAILED(mVideoDecoder->Init(params.display_width, params.display_height))) {
          mVideoDecoder = nullptr;
        }
      }

      if (!mVideoDecoder) {
        Cleanup();
        return NS_ERROR_FAILURE;
      }

      
      
      unsigned int cropH = params.crop_right + params.crop_left;
      unsigned int cropV = params.crop_bottom + params.crop_top;
      nsIntRect pictureRect(params.crop_left,
                            params.crop_top,
                            params.width - cropH,
                            params.height - cropV);

      
      if (pictureRect.width <= 0 ||
          pictureRect.height <= 0 ||
          pictureRect.x < 0 ||
          pictureRect.y < 0) {
        pictureRect.x = 0;
        pictureRect.y = 0;
        pictureRect.width = params.width;
        pictureRect.height = params.height;
      }

      
      
      nsIntSize displaySize(params.display_width, params.display_height);
      nsIntSize frameSize(params.width, params.height);
      if (!IsValidVideoRegion(frameSize, pictureRect, displaySize)) {
        
        continue;
      }

      mVideoTrack = track;
      mHasVideo = true;
      mInfo.mVideo.mHasVideo = true;

      mInfo.mVideo.mDisplay = displaySize;
      mPicture = pictureRect;
      mInitialFrame = frameSize;

      switch (params.stereo_mode) {
      case NESTEGG_VIDEO_MONO:
        mInfo.mVideo.mStereoMode = StereoMode::MONO;
        break;
      case NESTEGG_VIDEO_STEREO_LEFT_RIGHT:
        mInfo.mVideo.mStereoMode = StereoMode::LEFT_RIGHT;
        break;
      case NESTEGG_VIDEO_STEREO_BOTTOM_TOP:
        mInfo.mVideo.mStereoMode = StereoMode::BOTTOM_TOP;
        break;
      case NESTEGG_VIDEO_STEREO_TOP_BOTTOM:
        mInfo.mVideo.mStereoMode = StereoMode::TOP_BOTTOM;
        break;
      case NESTEGG_VIDEO_STEREO_RIGHT_LEFT:
        mInfo.mVideo.mStereoMode = StereoMode::RIGHT_LEFT;
        break;
      }
    } else if (!mHasAudio && type == NESTEGG_TRACK_AUDIO) {
      nestegg_audio_params params;
      r = nestegg_track_audio_params(mContext, track, &params);
      if (r == -1) {
        Cleanup();
        return NS_ERROR_FAILURE;
      }

      mAudioTrack = track;
      mHasAudio = true;
      mInfo.mAudio.mHasAudio = true;
      mAudioCodec = nestegg_track_codec_id(mContext, track);
      mCodecDelay = params.codec_delay / NS_PER_USEC;

      if (mAudioCodec == NESTEGG_CODEC_VORBIS) {
        
        unsigned int nheaders = 0;
        r = nestegg_track_codec_data_count(mContext, track, &nheaders);
        if (r == -1 || nheaders != 3) {
          Cleanup();
          return NS_ERROR_FAILURE;
        }

        for (uint32_t header = 0; header < nheaders; ++header) {
          unsigned char* data = 0;
          size_t length = 0;

          r = nestegg_track_codec_data(mContext, track, header, &data, &length);
          if (r == -1) {
            Cleanup();
            return NS_ERROR_FAILURE;
          }
          ogg_packet opacket = InitOggPacket(data, length, header == 0, false,
                                             0, mPacketCount++);

          r = vorbis_synthesis_headerin(&mVorbisInfo,
                                        &mVorbisComment,
                                        &opacket);
          if (r != 0) {
            Cleanup();
            return NS_ERROR_FAILURE;
          }
        }

        r = vorbis_synthesis_init(&mVorbisDsp, &mVorbisInfo);
        if (r != 0) {
          Cleanup();
          return NS_ERROR_FAILURE;
        }

        r = vorbis_block_init(&mVorbisDsp, &mVorbisBlock);
        if (r != 0) {
          Cleanup();
          return NS_ERROR_FAILURE;
        }

        mInfo.mAudio.mRate = mVorbisDsp.vi->rate;
        mInfo.mAudio.mChannels = mVorbisDsp.vi->channels;
      } else if (mAudioCodec == NESTEGG_CODEC_OPUS) {
        unsigned char* data = 0;
        size_t length = 0;
        r = nestegg_track_codec_data(mContext, track, 0, &data, &length);
        if (r == -1) {
          Cleanup();
          return NS_ERROR_FAILURE;
        }

        mOpusParser = new OpusParser;
        if (!mOpusParser->DecodeHeader(data, length)) {
          Cleanup();
          return NS_ERROR_FAILURE;
        }

        if (!InitOpusDecoder()) {
          Cleanup();
          return NS_ERROR_FAILURE;
        }

        if (int64_t(mCodecDelay) != FramesToUsecs(mOpusParser->mPreSkip,
                                                  mOpusParser->mRate).value()) {
          LOG(PR_LOG_WARNING,
              ("Invalid Opus header: CodecDelay and pre-skip do not match!"));
          Cleanup();
          return NS_ERROR_FAILURE;
        }

        mInfo.mAudio.mRate = mOpusParser->mRate;

        mInfo.mAudio.mChannels = mOpusParser->mChannels;
        mSeekPreroll = params.seek_preroll;
      } else {
        Cleanup();
        return NS_ERROR_FAILURE;
      }
    }
  }

  *aInfo = mInfo;

  *aTags = nullptr;

  return NS_OK;
}

bool
WebMReader::IsMediaSeekable()
{
  return mContext && nestegg_has_cues(mContext);
}

bool WebMReader::InitOpusDecoder()
{
  int r;

  NS_ASSERTION(mOpusDecoder == nullptr, "leaking OpusDecoder");

  mOpusDecoder = opus_multistream_decoder_create(mOpusParser->mRate,
                                                 mOpusParser->mChannels,
                                                 mOpusParser->mStreams,
                                                 mOpusParser->mCoupledStreams,
                                                 mOpusParser->mMappingTable,
                                                 &r);
  mSkip = mOpusParser->mPreSkip;
  mPaddingDiscarded = false;

  return r == OPUS_OK;
}

bool WebMReader::DecodeAudioPacket(nestegg_packet* aPacket, int64_t aOffset)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  int r = 0;
  unsigned int count = 0;
  r = nestegg_packet_count(aPacket, &count);
  if (r == -1) {
    return false;
  }

  uint64_t tstamp = 0;
  r = nestegg_packet_tstamp(aPacket, &tstamp);
  if (r == -1) {
    return false;
  }

  uint64_t tstamp_usecs = tstamp / NS_PER_USEC;
  if (mAudioStartUsec == -1) {
    
    
    mAudioStartUsec = tstamp_usecs;
  }
  
  
  
  
  CheckedInt64 tstamp_frames = UsecsToFrames(tstamp_usecs, mInfo.mAudio.mRate);
  CheckedInt64 decoded_frames = UsecsToFrames(mAudioStartUsec,
                                              mInfo.mAudio.mRate);
  if (!tstamp_frames.isValid() || !decoded_frames.isValid()) {
    NS_WARNING("Int overflow converting WebM times to frames");
    return false;
  }
  decoded_frames += mAudioFrames;
  if (!decoded_frames.isValid()) {
    NS_WARNING("Int overflow adding decoded_frames");
    return false;
  }
  if (tstamp_frames.value() > decoded_frames.value()) {
#ifdef DEBUG
    int64_t gap_frames = tstamp_frames.value() - decoded_frames.value();
    CheckedInt64 usecs = FramesToUsecs(gap_frames, mInfo.mAudio.mRate);
    LOG(PR_LOG_DEBUG, ("WebMReader detected gap of %lld, %lld frames, in audio",
                       usecs.isValid() ? usecs.value() : -1,
                       gap_frames));
#endif
    mPacketCount++;
    mAudioStartUsec = tstamp_usecs;
    mAudioFrames = 0;
  }

  int32_t total_frames = 0;
  for (uint32_t i = 0; i < count; ++i) {
    unsigned char* data;
    size_t length;
    r = nestegg_packet_data(aPacket, i, &data, &length);
    if (r == -1) {
      return false;
    }
    if (mAudioCodec == NESTEGG_CODEC_VORBIS) {
      if (!DecodeVorbis(data, length, aOffset, tstamp_usecs, &total_frames)) {
        return false;
      }
    } else if (mAudioCodec == NESTEGG_CODEC_OPUS) {
      if (!DecodeOpus(data, length, aOffset, tstamp_usecs, aPacket)) {
        return false;
      }
    }
  }

  return true;
}

bool WebMReader::DecodeVorbis(const unsigned char* aData, size_t aLength,
                              int64_t aOffset, uint64_t aTstampUsecs,
                              int32_t* aTotalFrames)
{
  ogg_packet opacket = InitOggPacket(aData, aLength, false, false, -1,
                                     mPacketCount++);

  if (vorbis_synthesis(&mVorbisBlock, &opacket) != 0) {
    return false;
  }

  if (vorbis_synthesis_blockin(&mVorbisDsp,
                               &mVorbisBlock) != 0) {
    return false;
  }

  VorbisPCMValue** pcm = 0;
  int32_t frames = vorbis_synthesis_pcmout(&mVorbisDsp, &pcm);
  
  
  
  
  
  if (frames == 0 && mAudioFrames == 0) {
    AudioQueue().Push(new AudioData(aOffset, aTstampUsecs, 0, 0, nullptr,
                                    mInfo.mAudio.mChannels,
                                    mInfo.mAudio.mRate));
  }
  while (frames > 0) {
    uint32_t channels = mInfo.mAudio.mChannels;
    nsAutoArrayPtr<AudioDataValue> buffer(new AudioDataValue[frames*channels]);
    for (uint32_t j = 0; j < channels; ++j) {
      VorbisPCMValue* channel = pcm[j];
      for (uint32_t i = 0; i < uint32_t(frames); ++i) {
        buffer[i*channels + j] = MOZ_CONVERT_VORBIS_SAMPLE(channel[i]);
      }
    }

    CheckedInt64 duration = FramesToUsecs(frames, mInfo.mAudio.mRate);
    if (!duration.isValid()) {
      NS_WARNING("Int overflow converting WebM audio duration");
      return false;
    }
    CheckedInt64 total_duration = FramesToUsecs(*aTotalFrames,
                                                mInfo.mAudio.mRate);
    if (!total_duration.isValid()) {
      NS_WARNING("Int overflow converting WebM audio total_duration");
      return false;
    }

    CheckedInt64 time = total_duration + aTstampUsecs;
    if (!time.isValid()) {
      NS_WARNING("Int overflow adding total_duration and aTstampUsecs");
      return false;
    };

    *aTotalFrames += frames;
    AudioQueue().Push(new AudioData(aOffset,
                                    time.value(),
                                    duration.value(),
                                    frames,
                                    buffer.forget(),
                                    mInfo.mAudio.mChannels,
                                    mInfo.mAudio.mRate));
    mAudioFrames += frames;
    if (vorbis_synthesis_read(&mVorbisDsp, frames) != 0) {
      return false;
    }

    frames = vorbis_synthesis_pcmout(&mVorbisDsp, &pcm);
  }

  return true;
}

bool WebMReader::DecodeOpus(const unsigned char* aData, size_t aLength,
                            int64_t aOffset, uint64_t aTstampUsecs,
                            nestegg_packet* aPacket)
{
  uint32_t channels = mOpusParser->mChannels;
  
  if (channels > 8) {
    return false;
  }

  if (mPaddingDiscarded) {
    
    
    LOG(PR_LOG_DEBUG, ("Opus error, discard padding on interstitial packet"));
    GetCallback()->OnNotDecoded(MediaData::AUDIO_DATA, DECODE_ERROR);
    return false;
  }

  
  int32_t frames_number = opus_packet_get_nb_frames(aData, aLength);
  if (frames_number <= 0) {
    return false; 
  }

  int32_t samples =
    opus_packet_get_samples_per_frame(aData, opus_int32(mInfo.mAudio.mRate));

  
  int32_t frames = frames_number*samples;
  if (frames < 120 || frames > 5760)
    return false;

  nsAutoArrayPtr<AudioDataValue> buffer(new AudioDataValue[frames * channels]);

  
#ifdef MOZ_SAMPLE_TYPE_FLOAT32
  int ret = opus_multistream_decode_float(mOpusDecoder,
                                          aData, aLength,
                                          buffer, frames, false);
#else
  int ret = opus_multistream_decode(mOpusDecoder,
                                    aData, aLength,
                                    buffer, frames, false);
#endif
  if (ret < 0)
    return false;
  NS_ASSERTION(ret == frames, "Opus decoded too few audio samples");
  CheckedInt64 startTime = aTstampUsecs;

  
  if (mSkip > 0) {
    int32_t skipFrames = std::min<int32_t>(mSkip, frames);
    int32_t keepFrames = frames - skipFrames;
    LOG(PR_LOG_DEBUG, ("Opus decoder skipping %d of %d frames",
                       skipFrames, frames));
    PodMove(buffer.get(),
            buffer.get() + skipFrames * channels,
            keepFrames * channels);
    startTime = startTime + FramesToUsecs(skipFrames, mInfo.mAudio.mRate);
    frames = keepFrames;
    mSkip -= skipFrames;
  }

  int64_t discardPadding = 0;
  (void) nestegg_packet_discard_padding(aPacket, &discardPadding);
  if (discardPadding < 0) {
    
    LOG(PR_LOG_DEBUG, ("Opus error, negative discard padding"));
    GetCallback()->OnNotDecoded(MediaData::AUDIO_DATA, DECODE_ERROR);
    return false;
  }
  if (discardPadding > 0) {
    CheckedInt64 discardFrames = UsecsToFrames(discardPadding / NS_PER_USEC,
                                               mInfo.mAudio.mRate);
    if (!discardFrames.isValid()) {
      NS_WARNING("Int overflow in DiscardPadding");
      return false;
    }
    if (discardFrames.value() > frames) {
      
      LOG(PR_LOG_DEBUG, ("Opus error, discard padding larger than packet"));
      GetCallback()->OnNotDecoded(MediaData::AUDIO_DATA, DECODE_ERROR);
      return false;
    }
    LOG(PR_LOG_DEBUG, ("Opus decoder discarding %d of %d frames",
                       int32_t(discardFrames.value()), frames));
    
    
    
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

  CheckedInt64 duration = FramesToUsecs(frames, mInfo.mAudio.mRate);
  if (!duration.isValid()) {
    NS_WARNING("Int overflow converting WebM audio duration");
    return false;
  }
  CheckedInt64 time = startTime - mCodecDelay;
  if (!time.isValid()) {
    NS_WARNING("Int overflow shifting tstamp by codec delay");
    return false;
  };
  AudioQueue().Push(new AudioData(aOffset,
                                  time.value(),
                                  duration.value(),
                                  frames,
                                  buffer.forget(),
                                  mInfo.mAudio.mChannels,
                                  mInfo.mAudio.mRate));

  mAudioFrames += frames;

  return true;
}

nsReturnRef<NesteggPacketHolder> WebMReader::NextPacket(TrackType aTrackType)
{
  
  
  WebMPacketQueue& otherPackets =
    aTrackType == VIDEO ? mAudioPackets : mVideoPackets;

  
  WebMPacketQueue &packets =
    aTrackType == VIDEO ? mVideoPackets : mAudioPackets;

  
  
  bool hasType = aTrackType == VIDEO ? mHasVideo : mHasAudio;

  
  
  bool hasOtherType = aTrackType == VIDEO ? mHasAudio : mHasVideo;

  
  uint32_t ourTrack = aTrackType == VIDEO ? mVideoTrack : mAudioTrack;

  
  uint32_t otherTrack = aTrackType == VIDEO ? mAudioTrack : mVideoTrack;

  nsAutoRef<NesteggPacketHolder> holder;

  if (packets.GetSize() > 0) {
    holder.own(packets.PopFront());
  } else {
    
    
    do {
      nestegg_packet* packet;
      int r = nestegg_read_packet(mContext, &packet);
      if (r <= 0) {
        return nsReturnRef<NesteggPacketHolder>();
      }
      int64_t offset = mDecoder->GetResource()->Tell();
      holder.own(new NesteggPacketHolder(packet, offset));

      unsigned int track = 0;
      r = nestegg_packet_track(packet, &track);
      if (r == -1) {
        return nsReturnRef<NesteggPacketHolder>();
      }

      if (hasOtherType && otherTrack == track) {
        
        otherPackets.Push(holder.disown());
        continue;
      }

      
      if (hasType && ourTrack == track) {
        break;
      }
    } while (true);
  }

  return holder.out();
}

bool WebMReader::DecodeAudioData()
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  nsAutoRef<NesteggPacketHolder> holder(NextPacket(AUDIO));
  if (!holder) {
    return false;
  }

  return DecodeAudioPacket(holder->mPacket, holder->mOffset);
}

bool WebMReader::DecodeVideoFrame(bool &aKeyframeSkip, int64_t aTimeThreshold)
{
  return mVideoDecoder->DecodeVideoFrame(aKeyframeSkip, aTimeThreshold);
}

void WebMReader::PushVideoPacket(NesteggPacketHolder* aItem)
{
    mVideoPackets.PushFront(aItem);
}

void WebMReader::Seek(int64_t aTarget, int64_t aStartTime, int64_t aEndTime,
                      int64_t aCurrentTime)
{
  nsresult res = SeekInternal(aTarget, aStartTime);
  GetCallback()->OnSeekCompleted(res);
}

nsresult WebMReader::SeekInternal(int64_t aTarget, int64_t aStartTime)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  if (mVideoDecoder) {
    nsresult rv = mVideoDecoder->Flush();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  LOG(PR_LOG_DEBUG, ("Reader [%p] for Decoder [%p]: About to seek to %fs",
                     this, mDecoder, double(aTarget) / USECS_PER_S));
  if (NS_FAILED(ResetDecode())) {
    return NS_ERROR_FAILURE;
  }
  uint32_t trackToSeek = mHasVideo ? mVideoTrack : mAudioTrack;
  uint64_t target = aTarget * NS_PER_USEC;

  if (mSeekPreroll) {
    target = std::max(uint64_t(aStartTime * NS_PER_USEC),
                      target - mSeekPreroll);
  }
  int r = nestegg_track_seek(mContext, trackToSeek, target);
  if (r != 0) {
    LOG(PR_LOG_DEBUG, ("Reader [%p]: track_seek for track %u failed, r=%d",
                       this, trackToSeek, r));

    
    int64_t offset = 0;
    bool rv = mBufferedState->GetOffsetForTime(target, &offset);
    if (!rv) {
      return NS_ERROR_FAILURE;
    }

    r = nestegg_offset_seek(mContext, offset);
    LOG(PR_LOG_DEBUG, ("Reader [%p]: attempted offset_seek to %lld r=%d",
                       this, offset, r));
    if (r != 0) {
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}

nsresult WebMReader::GetBuffered(dom::TimeRanges* aBuffered)
{
  MOZ_ASSERT(mStartTime != -1, "Need to finish metadata decode first");
  if (aBuffered->Length() != 0) {
    return NS_ERROR_FAILURE;
  }

  AutoPinned<MediaResource> resource(mDecoder->GetResource());

  
  if (mContext && resource->IsDataCachedToEndOfResource(0)) {
    uint64_t duration = 0;
    if (nestegg_duration(mContext, &duration) == 0) {
      aBuffered->Add(0, duration / NS_PER_S);
      return NS_OK;
    }
  }

  
  
  nsTArray<MediaByteRange> ranges;
  nsresult res = resource->GetCachedRanges(ranges);
  NS_ENSURE_SUCCESS(res, res);

  for (uint32_t index = 0; index < ranges.Length(); index++) {
    uint64_t start, end;
    bool rv = mBufferedState->CalculateBufferedForRange(ranges[index].mStart,
                                                        ranges[index].mEnd,
                                                        &start, &end);
    if (rv) {
      int64_t startOffset = mStartTime * NS_PER_USEC;
      NS_ASSERTION(startOffset >= 0 && uint64_t(startOffset) <= start,
                   "startOffset negative or larger than start time");
      if (!(startOffset >= 0 && uint64_t(startOffset) <= start)) {
        startOffset = 0;
      }
      double startTime = (start - startOffset) / NS_PER_S;
      double endTime = (end - startOffset) / NS_PER_S;
      
      
      if (mContext &&
          resource->IsDataCachedToEndOfResource(ranges[index].mStart)) {
        uint64_t duration = 0;
        if (nestegg_duration(mContext, &duration) == 0) {
          endTime = duration / NS_PER_S;
        }
      }

      aBuffered->Add(startTime, endTime);
    }
  }

  return NS_OK;
}

void WebMReader::NotifyDataArrived(const char* aBuffer, uint32_t aLength,
                                   int64_t aOffset)
{
  mBufferedState->NotifyDataArrived(aBuffer, aLength, aOffset);
}

int64_t WebMReader::GetEvictionOffset(double aTime)
{
  int64_t offset;
  if (!mBufferedState->GetOffsetForTime(aTime * NS_PER_S, &offset)) {
    return -1;
  }

  return offset;
}

int WebMReader::GetVideoCodec()
{
  return mVideoCodec;
}

nsIntRect WebMReader::GetPicture()
{
  return mPicture;
}

nsIntSize WebMReader::GetInitialFrame()
{
  return mInitialFrame;
}

uint64_t WebMReader::GetLastVideoFrameTime()
{
  return mLastVideoFrameTime;
}

void WebMReader::SetLastVideoFrameTime(uint64_t aFrameTime)
{
  mLastVideoFrameTime = aFrameTime;
}

} 
