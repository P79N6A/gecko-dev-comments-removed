





#include "MP4Reader.h"
#include "MediaResource.h"
#include "mp4_demuxer/mp4_demuxer.h"
#include "mp4_demuxer/Streams.h"
#include "nsSize.h"
#include "VideoUtils.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "ImageContainer.h"
#include "Layers.h"

using mozilla::layers::Image;
using mozilla::layers::LayerManager;
using mozilla::layers::LayersBackend;

#ifdef PR_LOGGING
PRLogModuleInfo* GetDemuxerLog() {
  static PRLogModuleInfo* log = nullptr;
  if (!log) {
    log = PR_NewLogModule("MP4Demuxer");
  }
  return log;
}
#define LOG(...) PR_LOG(GetDemuxerLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define LOG(...)
#endif

using namespace mp4_demuxer;

namespace mozilla {




class MP4Stream : public mp4_demuxer::Stream {
public:

  MP4Stream(MediaResource* aResource)
    : mResource(aResource)
  {
    MOZ_COUNT_CTOR(MP4Stream);
    MOZ_ASSERT(aResource);
  }
  ~MP4Stream() {
    MOZ_COUNT_DTOR(MP4Stream);
  }

  virtual bool ReadAt(int64_t aOffset,
                      uint8_t* aBuffer,
                      uint32_t aCount,
                      uint32_t* aBytesRead) MOZ_OVERRIDE {
    uint32_t sum = 0;
    do {
      uint32_t offset = aOffset + sum;
      char* buffer = reinterpret_cast<char*>(aBuffer + sum);
      uint32_t toRead = aCount - sum;
      uint32_t bytesRead = 0;
      nsresult rv = mResource->ReadAt(offset, buffer, toRead, &bytesRead);
      if (NS_FAILED(rv)) {
        return false;
      }
      sum += bytesRead;
    } while (sum < aCount);
    *aBytesRead = sum;
    return true;
  }

  virtual int64_t Length() const MOZ_OVERRIDE {
    return mResource->GetLength();
  }

private:
  RefPtr<MediaResource> mResource;
};

MP4Reader::MP4Reader(AbstractMediaDecoder* aDecoder)
  : MediaDecoderReader(aDecoder),
    mLayersBackendType(layers::LAYERS_NONE),
    mHasAudio(false),
    mHasVideo(false)
{
  MOZ_COUNT_CTOR(MP4Reader);
}

MP4Reader::~MP4Reader()
{
  MOZ_COUNT_DTOR(MP4Reader);
}

nsresult
MP4Reader::Init(MediaDecoderReader* aCloneDonor)
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
  mMP4Stream = new MP4Stream(mDecoder->GetResource());
  mDemuxer = new MP4Demuxer(mMP4Stream);

  mPlatform = PlatformDecoderModule::Create();
  NS_ENSURE_TRUE(mPlatform, NS_ERROR_FAILURE);

  if (IsVideoContentType(mDecoder->GetResource()->GetContentType())) {
    
    
    
    MediaDecoderOwner* owner = mDecoder->GetOwner();
    NS_ENSURE_TRUE(owner, NS_ERROR_FAILURE);

    dom::HTMLMediaElement* element = owner->GetMediaElement();
    NS_ENSURE_TRUE(element, NS_ERROR_FAILURE);

    nsRefPtr<LayerManager> layerManager =
      nsContentUtils::LayerManagerForDocument(element->OwnerDoc());
    NS_ENSURE_TRUE(layerManager, NS_ERROR_FAILURE);

    mLayersBackendType = layerManager->GetBackendType();
  }

  return NS_OK;
}

nsresult
MP4Reader::ReadMetadata(MediaInfo* aInfo,
                        MetadataTags** aTags)
{
  bool ok = mDemuxer->Init();
  NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);

  mInfo.mAudio.mHasAudio = mHasAudio = mDemuxer->HasAudio();
  if (mHasAudio) {
    const AudioDecoderConfig& config = mDemuxer->AudioConfig();
    mInfo.mAudio.mRate = config.samples_per_second();
    mInfo.mAudio.mChannels = ChannelLayoutToChannelCount(config.channel_layout());
    mAudioDecoder = mPlatform->CreateAudioDecoder(mInfo.mAudio.mChannels,
                                                  mInfo.mAudio.mRate,
                                                  config.bits_per_channel(),
                                                  config.extra_data(),
                                                  config.extra_data_size());
    NS_ENSURE_TRUE(mAudioDecoder != nullptr, NS_ERROR_FAILURE);
  }

  mInfo.mVideo.mHasVideo = mHasVideo = mDemuxer->HasVideo();
  if (mHasVideo) {
    const VideoDecoderConfig& config = mDemuxer->VideoConfig();
    IntSize sz = config.natural_size();
    mInfo.mVideo.mDisplay = nsIntSize(sz.width(), sz.height());

    mVideoDecoder = mPlatform->CreateVideoDecoder(mLayersBackendType,
                                                  mDecoder->GetImageContainer());
    NS_ENSURE_TRUE(mVideoDecoder != nullptr, NS_ERROR_FAILURE);
  }

  
  Microseconds duration = mDemuxer->Duration();
  if (duration != -1) {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDecoder->SetMediaDuration(duration);
  }
  
  
  if (!mDemuxer->CanSeek()) {
    mDecoder->SetMediaSeekable(false);
  }

  *aInfo = mInfo;
  *aTags = nullptr;

  return NS_OK;
}

bool
MP4Reader::HasAudio()
{
  return mHasAudio;
}

bool
MP4Reader::HasVideo()
{
  return mHasVideo;
}

MP4SampleQueue&
MP4Reader::SampleQueue(TrackType aTrack)
{
  MOZ_ASSERT(aTrack == kAudio || aTrack == kVideo);
  return (aTrack == kAudio) ? mCompressedAudioQueue
                            : mCompressedVideoQueue;
}

MediaDataDecoder*
MP4Reader::Decoder(mp4_demuxer::TrackType aTrack)
{
  MOZ_ASSERT(aTrack == kAudio || aTrack == kVideo);
  return (aTrack == kAudio) ? mAudioDecoder
                            : mVideoDecoder;
}

MP4Sample*
MP4Reader::PopSample(TrackType aTrack)
{
  
  
  
  MP4SampleQueue& sampleQueue = SampleQueue(aTrack);
  while (sampleQueue.empty()) {
    nsAutoPtr<MP4Sample> sample;
    bool eos = false;
    bool ok = mDemuxer->Demux(&sample, &eos);
    if (!ok || eos) {
      MOZ_ASSERT(!sample);
      return nullptr;
    }
    MOZ_ASSERT(sample);
    MP4Sample* s = sample.forget();
    SampleQueue(s->type).push_back(s);
  }
  MOZ_ASSERT(!sampleQueue.empty());
  MP4Sample* sample = sampleQueue.front();
  sampleQueue.pop_front();
  return sample;
}

bool
MP4Reader::Decode(TrackType aTrack, nsAutoPtr<MediaData>& aOutData)
{
  MP4SampleQueue& sampleQueue = SampleQueue(aTrack);
  MediaDataDecoder* decoder = Decoder(aTrack);

  MOZ_ASSERT(decoder);

  
  while (true) {
    DecoderStatus status = decoder->Output(aOutData);
    if (status == DECODE_STATUS_OK) {
      MOZ_ASSERT(aOutData);
      return true;
    }
    
    MOZ_ASSERT(!aOutData);

    if (status == DECODE_STATUS_ERROR) {
      return false;
    }

    if (status == DECODE_STATUS_NEED_MORE_INPUT) {
      
      
      nsAutoPtr<MP4Sample> compressed;
      do {
        compressed = PopSample(aTrack);
        if (!compressed) {
          
          
          return false;
        }
        const std::vector<uint8_t>* data = compressed->data;
        status = decoder->Input(&data->front(),
                                data->size(),
                                compressed->decode_timestamp,
                                compressed->composition_timestamp,
                                compressed->byte_offset);
      } while (status == DECODE_STATUS_OK);
      if (status == DECODE_STATUS_NOT_ACCEPTING) {
        
        SampleQueue(aTrack).push_front(compressed.forget());
        continue;
      }
      LOG("MP4Reader decode failure. track=%d status=%d\n", aTrack, status);
      return false;
    } else {
      LOG("MP4Reader unexpected error. track=%d status=%d\n", aTrack, status);
      return false;
    }
  }
}

bool
MP4Reader::DecodeAudioData()
{
  MOZ_ASSERT(mHasAudio && mPlatform && mAudioDecoder);
  nsAutoPtr<MediaData> audio;
  bool ok = Decode(kAudio, audio);
  if (ok && audio && audio->mType == MediaData::AUDIO_SAMPLES) {
#ifdef LOG_SAMPLE_DECODE
    LOG("DecodeAudioData time=%lld dur=%lld", audio->mTime, audio->mDuration);
#endif
    mAudioQueue.Push(static_cast<AudioData*>(audio.forget()));
  }
  return ok;
}

bool
MP4Reader::SkipVideoDemuxToNextKeyFrame(int64_t aTimeThreshold, uint32_t& parsed)
{
  MOZ_ASSERT(mVideoDecoder);

  
  mVideoDecoder->Flush();

  
  while (true) {
    nsAutoPtr<MP4Sample> compressed(PopSample(kVideo));
    if (!compressed) {
      
      return false;
    }
    parsed++;
    if (!compressed->is_sync_point ||
        compressed->composition_timestamp < aTimeThreshold) {
      continue;
    }
    mCompressedVideoQueue.push_front(compressed.forget());
    break;
  }

  return true;
}

bool
MP4Reader::DecodeVideoFrame(bool &aKeyframeSkip,
                            int64_t aTimeThreshold)
{
  
  
  uint32_t parsed = 0, decoded = 0;
  AbstractMediaDecoder::AutoNotifyDecoded autoNotify(mDecoder, parsed, decoded);

  MOZ_ASSERT(mHasVideo && mPlatform && mVideoDecoder);

  if (aKeyframeSkip) {
    bool ok = SkipVideoDemuxToNextKeyFrame(aTimeThreshold, parsed);
    if (!ok) {
      NS_WARNING("Failed to skip demux up to next keyframe");
      return false;
    }
    aKeyframeSkip = false;
  }

  nsAutoPtr<MediaData> data;
  bool ok = Decode(kVideo, data);
  MOZ_ASSERT(!data || data->mType == MediaData::VIDEO_FRAME);
  if (ok && data) {
    parsed++;
    if (data->mTime < aTimeThreshold) {
      
      return true;
    }
    decoded++;
    VideoData* video = static_cast<VideoData*>(data.forget());
#ifdef LOG_SAMPLE_DECODE
    LOG("DecodeVideoData time=%lld dur=%lld", video->mTime, video->mDuration);
#endif
    mVideoQueue.Push(video);
  }
  return ok;
}

nsresult
MP4Reader::Seek(int64_t aTime,
                int64_t aStartTime,
                int64_t aEndTime,
                int64_t aCurrentTime)
{
  if (!mDemuxer->CanSeek()) {
    return NS_ERROR_FAILURE;
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}

void
MP4Reader::OnDecodeThreadStart()
{
  MOZ_ASSERT(!NS_IsMainThread(), "Must not be on main thread.");
  MOZ_ASSERT(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  MOZ_ASSERT(mPlatform);
  mPlatform->OnDecodeThreadStart();
}

void
MP4Reader::OnDecodeThreadFinish()
{
  MOZ_ASSERT(!NS_IsMainThread(), "Must not be on main thread.");
  MOZ_ASSERT(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  MOZ_ASSERT(mPlatform);
  mPlatform->OnDecodeThreadFinish();
}

} 
