





#include "MediaCodecReader.h"

#include <OMX_IVCommon.h>

#include <gui/Surface.h>
#include <ICrypto.h>

#include <stagefright/foundation/ABuffer.h>
#include <stagefright/foundation/ADebug.h>
#include <stagefright/foundation/ALooper.h>
#include <stagefright/foundation/AMessage.h>
#include <stagefright/MediaBuffer.h>
#include <stagefright/MediaCodec.h>
#include <stagefright/MediaDefs.h>
#include <stagefright/MediaExtractor.h>
#include <stagefright/MediaSource.h>
#include <stagefright/MetaData.h>
#include <stagefright/Utils.h>

#include "mozilla/TimeStamp.h"

#include "gfx2DGlue.h"

#include "MediaStreamSource.h"
#include "MediaTaskQueue.h"
#include "nsThreadUtils.h"
#include "ImageContainer.h"
#include "SharedThreadPool.h"
#include "VideoFrameContainer.h"

using namespace android;

namespace mozilla {

enum {
  kNotifyCodecReserved = 'core',
  kNotifyCodecCanceled = 'coca',
};

static const int64_t sInvalidDurationUs = INT64_C(-1);
static const int64_t sInvalidTimestampUs = INT64_C(-1);



static const double sMaxAudioDecodeDurationS = 0.1;
static const double sMaxVideoDecodeDurationS = 0.1;

static CheckedUint32 sInvalidInputIndex = INT32_C(-1);

inline bool
IsValidDurationUs(int64_t aDuration)
{
  return aDuration >= INT64_C(0);
}

inline bool
IsValidTimestampUs(int64_t aTimestamp)
{
  return aTimestamp >= INT64_C(0);
}

MediaCodecReader::MessageHandler::MessageHandler(MediaCodecReader* aReader)
  : mReader(aReader)
{
}

MediaCodecReader::MessageHandler::~MessageHandler()
{
  mReader = nullptr;
}

void
MediaCodecReader::MessageHandler::onMessageReceived(
  const sp<AMessage>& aMessage)
{
  if (mReader) {
    mReader->onMessageReceived(aMessage);
  }
}

MediaCodecReader::VideoResourceListener::VideoResourceListener(
  MediaCodecReader* aReader)
  : mReader(aReader)
{
}

MediaCodecReader::VideoResourceListener::~VideoResourceListener()
{
  mReader = nullptr;
}

void
MediaCodecReader::VideoResourceListener::codecReserved()
{
  if (mReader) {
    mReader->codecReserved(mReader->mVideoTrack);
  }
}

void
MediaCodecReader::VideoResourceListener::codecCanceled()
{
  if (mReader) {
    mReader->codecCanceled(mReader->mVideoTrack);
  }
}

bool
MediaCodecReader::TrackInputCopier::Copy(MediaBuffer* aSourceBuffer,
                                         sp<ABuffer> aCodecBuffer)
{
  if (aSourceBuffer == nullptr ||
      aCodecBuffer == nullptr ||
      aSourceBuffer->range_length() > aCodecBuffer->capacity()) {
    return false;
  }

  aCodecBuffer->setRange(0, aSourceBuffer->range_length());
  memcpy(aCodecBuffer->data(),
         (uint8_t*)aSourceBuffer->data() + aSourceBuffer->range_offset(),
         aSourceBuffer->range_length());

  return true;
}

MediaCodecReader::Track::Track()
  : mSourceIsStopped(true)
  , mDurationUs(INT64_C(0))
  , mInputIndex(sInvalidInputIndex)
  , mInputEndOfStream(false)
  , mOutputEndOfStream(false)
  , mSeekTimeUs(sInvalidTimestampUs)
  , mFlushed(false)
  , mDiscontinuity(false)
  , mTaskQueue(nullptr)
{
}




bool
MediaCodecReader::VorbisInputCopier::Copy(MediaBuffer* aSourceBuffer,
                                          sp<ABuffer> aCodecBuffer)
{
  if (aSourceBuffer == nullptr ||
      aCodecBuffer == nullptr ||
      aSourceBuffer->range_length() + sizeof(int32_t) > aCodecBuffer->capacity()) {
    return false;
  }

  int32_t numPageSamples = 0;
  if (!aSourceBuffer->meta_data()->findInt32(kKeyValidSamples, &numPageSamples)) {
    numPageSamples = -1;
  }

  aCodecBuffer->setRange(0, aSourceBuffer->range_length() + sizeof(int32_t));
  memcpy(aCodecBuffer->data(),
         (uint8_t*)aSourceBuffer->data() + aSourceBuffer->range_offset(),
         aSourceBuffer->range_length());
  memcpy(aCodecBuffer->data() + aSourceBuffer->range_length(),
         &numPageSamples, sizeof(numPageSamples));

  return true;
}

MediaCodecReader::AudioTrack::AudioTrack()
{
}

MediaCodecReader::VideoTrack::VideoTrack()
  : mWidth(0)
  , mHeight(0)
  , mStride(0)
  , mSliceHeight(0)
  , mColorFormat(0)
  , mRotation(0)
{
}

MediaCodecReader::CodecBufferInfo::CodecBufferInfo()
  : mIndex(0)
  , mOffset(0)
  , mSize(0)
  , mTimeUs(0)
  , mFlags(0)
{
}

MediaCodecReader::MediaCodecReader(AbstractMediaDecoder* aDecoder)
  : MediaOmxCommonReader(aDecoder)
  , mColorConverterBufferSize(0)
  , mExtractor(nullptr)
{
  mHandler = new MessageHandler(this);
  mVideoListener = new VideoResourceListener(this);
}

MediaCodecReader::~MediaCodecReader()
{
  MOZ_ASSERT(NS_IsMainThread(), "Should be on main thread.");
}

nsresult
MediaCodecReader::Init(MediaDecoderReader* aCloneDonor)
{
  return NS_OK;
}

bool
MediaCodecReader::IsWaitingMediaResources()
{
  return mVideoTrack.mCodec != nullptr && !mVideoTrack.mCodec->allocated();
}

bool
MediaCodecReader::IsDormantNeeded()
{
  return mVideoTrack.mSource != nullptr;
}

void
MediaCodecReader::ReleaseMediaResources()
{
  
  
  if (mVideoTrack.mSource != nullptr) {
    mVideoTrack.mSource->stop();
    mVideoTrack.mSourceIsStopped = true;
  }
  if (mAudioTrack.mSource != nullptr) {
    mAudioTrack.mSource->stop();
    mAudioTrack.mSourceIsStopped = true;
  }
  ReleaseCriticalResources();
}

void
MediaCodecReader::Shutdown()
{
  ReleaseResources();
}

void
MediaCodecReader::DispatchAudioTask()
{
  if (mAudioTrack.mTaskQueue && mAudioTrack.mTaskQueue->IsEmpty()) {
    RefPtr<nsIRunnable> task =
      NS_NewRunnableMethod(this,
                           &MediaCodecReader::DecodeAudioDataTask);
    mAudioTrack.mTaskQueue->Dispatch(task);
  }
}

void
MediaCodecReader::DispatchVideoTask(int64_t aTimeThreshold)
{
  if (mVideoTrack.mTaskQueue && mVideoTrack.mTaskQueue->IsEmpty()) {
    RefPtr<nsIRunnable> task =
      NS_NewRunnableMethodWithArg<int64_t>(this,
                                           &MediaCodecReader::DecodeVideoFrameTask,
                                           aTimeThreshold);
    mVideoTrack.mTaskQueue->Dispatch(task);
  }
}

void
MediaCodecReader::RequestAudioData()
{
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());
  MOZ_ASSERT(HasAudio());
  if (CheckAudioResources()) {
    DispatchAudioTask();
  }
}

void
MediaCodecReader::RequestVideoData(bool aSkipToNextKeyframe,
                                   int64_t aTimeThreshold)
{
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());
  MOZ_ASSERT(HasVideo());

  int64_t threshold = sInvalidTimestampUs;
  if (aSkipToNextKeyframe && IsValidTimestampUs(aTimeThreshold)) {
    mVideoTrack.mTaskQueue->Flush();
    threshold = aTimeThreshold;
  }
  if (CheckVideoResources()) {
    DispatchVideoTask(threshold);
  }
}

bool
MediaCodecReader::DecodeAudioDataSync()
{
  if (mAudioTrack.mCodec == nullptr || !mAudioTrack.mCodec->allocated() ||
      mAudioTrack.mOutputEndOfStream) {
    return false;
  }

  
  CodecBufferInfo bufferInfo;
  status_t status;
  TimeStamp timeout = TimeStamp::Now() +
                      TimeDuration::FromSeconds(sMaxAudioDecodeDurationS);
  while (true) {
    
    
    FillCodecInputData(mAudioTrack);

    status = GetCodecOutputData(mAudioTrack, bufferInfo, sInvalidTimestampUs,
                                timeout);
    if (status == OK || status == ERROR_END_OF_STREAM) {
      break;
    } else if (status == -EAGAIN) {
      if (TimeStamp::Now() > timeout) {
        
        if (CheckAudioResources()) {
          DispatchAudioTask();
        }
        return true;
      }
      continue; 
    } else if (status == INFO_FORMAT_CHANGED) {
      if (UpdateAudioInfo()) {
        continue; 
      } else {
        return false;
      }
    } else {
      return false;
    }
  }

  bool result = false;
  if (bufferInfo.mBuffer != nullptr && bufferInfo.mSize > 0 &&
      bufferInfo.mBuffer->data() != nullptr) {
    
    int64_t pos = mDecoder->GetResource()->Tell();

    uint32_t frames = bufferInfo.mSize /
                      (mInfo.mAudio.mChannels * sizeof(AudioDataValue));

    result = mAudioCompactor.Push(
      pos,
      bufferInfo.mTimeUs,
      mInfo.mAudio.mRate,
      frames,
      mInfo.mAudio.mChannels,
      AudioCompactor::NativeCopy(
        bufferInfo.mBuffer->data() + bufferInfo.mOffset,
        bufferInfo.mSize,
        mInfo.mAudio.mChannels));
  }

  if ((bufferInfo.mFlags & MediaCodec::BUFFER_FLAG_EOS) ||
      (status == ERROR_END_OF_STREAM)) {
    AudioQueue().Finish();
  }
  mAudioTrack.mCodec->releaseOutputBuffer(bufferInfo.mIndex);

  return result;
}

bool
MediaCodecReader::DecodeAudioDataTask()
{
  bool result = DecodeAudioDataSync();
  if (AudioQueue().GetSize() > 0) {
    AudioData* a = AudioQueue().PopFront();
    if (a) {
      if (mAudioTrack.mDiscontinuity) {
        a->mDiscontinuity = true;
        mAudioTrack.mDiscontinuity = false;
      }
      GetCallback()->OnAudioDecoded(a);
    }
  }
  if (AudioQueue().AtEndOfStream()) {
    GetCallback()->OnAudioEOS();
  }
  return result;
}

bool
MediaCodecReader::DecodeVideoFrameTask(int64_t aTimeThreshold)
{
  bool result = DecodeVideoFrameSync(aTimeThreshold);
  if (VideoQueue().GetSize() > 0) {
    VideoData* v = VideoQueue().PopFront();
    if (v) {
      if (mVideoTrack.mDiscontinuity) {
        v->mDiscontinuity = true;
        mVideoTrack.mDiscontinuity = false;
      }
      GetCallback()->OnVideoDecoded(v);
    }
  }
  if (VideoQueue().AtEndOfStream()) {
    GetCallback()->OnVideoEOS();
  }
  return result;
}

bool
MediaCodecReader::HasAudio()
{
  return mInfo.mAudio.mHasAudio;
}

bool
MediaCodecReader::HasVideo()
{
  return mInfo.mVideo.mHasVideo;
}

nsresult
MediaCodecReader::ReadMetadata(MediaInfo* aInfo,
                               MetadataTags** aTags)
{
  MOZ_ASSERT(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  if (!ReallocateResources()) {
    return NS_ERROR_FAILURE;
  }

#ifdef MOZ_AUDIO_OFFLOAD
  CheckAudioOffload();
#endif

  if (IsWaitingMediaResources()) {
    return NS_OK;
  }

  

  if (!UpdateDuration()) {
    return NS_ERROR_FAILURE;
  }

  if (!UpdateAudioInfo()) {
    return NS_ERROR_FAILURE;
  }

  if (!UpdateVideoInfo()) {
    return NS_ERROR_FAILURE;
  }

  
  int64_t duration = mAudioTrack.mDurationUs > mVideoTrack.mDurationUs ?
    mAudioTrack.mDurationUs : mVideoTrack.mDurationUs;
  if (duration >= 0LL) {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDecoder->SetMediaDuration(duration);
  }

  
  VideoFrameContainer* container = mDecoder->GetVideoFrameContainer();
  if (container) {
    container->SetCurrentFrame(
      gfxIntSize(mInfo.mVideo.mDisplay.width, mInfo.mVideo.mDisplay.height),
      nullptr,
      mozilla::TimeStamp::Now());
  }

  *aInfo = mInfo;
  *aTags = nullptr;

  return NS_OK;
}

nsresult
MediaCodecReader::ResetDecode()
{
  if (CheckAudioResources()) {
    mAudioTrack.mTaskQueue->Flush();
    FlushCodecData(mAudioTrack);
    mAudioTrack.mDiscontinuity = true;
  }
  if (CheckVideoResources()) {
    mVideoTrack.mTaskQueue->Flush();
    FlushCodecData(mVideoTrack);
    mVideoTrack.mDiscontinuity = true;
  }

  return MediaDecoderReader::ResetDecode();
}

bool
MediaCodecReader::DecodeVideoFrameSync(int64_t aTimeThreshold)
{
  if (mVideoTrack.mCodec == nullptr || !mVideoTrack.mCodec->allocated() ||
      mVideoTrack.mOutputEndOfStream) {
    return false;
  }

  
  CodecBufferInfo bufferInfo;
  status_t status;
  TimeStamp timeout = TimeStamp::Now() +
                      TimeDuration::FromSeconds(sMaxVideoDecodeDurationS);
  while (true) {
    
    
    FillCodecInputData(mVideoTrack);

    status = GetCodecOutputData(mVideoTrack, bufferInfo, aTimeThreshold,
                                timeout);
    if (status == OK || status == ERROR_END_OF_STREAM) {
      break;
    } else if (status == -EAGAIN) {
      if (TimeStamp::Now() > timeout) {
        
        if (CheckVideoResources()) {
          DispatchVideoTask(aTimeThreshold);
        }
        return true;
      }
      continue; 
    } else if (status == INFO_FORMAT_CHANGED) {
      if (UpdateVideoInfo()) {
        continue; 
      } else {
        return false;
      }
    } else {
      return false;
    }
  }

  bool result = false;
  if (bufferInfo.mBuffer != nullptr && bufferInfo.mSize > 0 &&
      bufferInfo.mBuffer->data() != nullptr) {
    uint8_t* yuv420p_buffer = bufferInfo.mBuffer->data();
    int32_t stride = mVideoTrack.mStride;
    int32_t slice_height = mVideoTrack.mSliceHeight;

    
    if (mVideoTrack.mColorFormat != OMX_COLOR_FormatYUV420Planar) {
      ARect crop;
      crop.top = 0;
      crop.bottom = mVideoTrack.mHeight;
      crop.left = 0;
      crop.right = mVideoTrack.mWidth;

      yuv420p_buffer = GetColorConverterBuffer(mVideoTrack.mWidth,
                                               mVideoTrack.mHeight);
      if (mColorConverter.convertDecoderOutputToI420(
            bufferInfo.mBuffer->data(), mVideoTrack.mWidth, mVideoTrack.mHeight,
            crop, yuv420p_buffer) != OK) {
        mVideoTrack.mCodec->releaseOutputBuffer(bufferInfo.mIndex);
        NS_WARNING("Unable to convert color format");
        return false;
      }

      stride = mVideoTrack.mWidth;
      slice_height = mVideoTrack.mHeight;
    }

    size_t yuv420p_y_size = stride * slice_height;
    size_t yuv420p_u_size = ((stride + 1) / 2) * ((slice_height + 1) / 2);
    uint8_t* yuv420p_y = yuv420p_buffer;
    uint8_t* yuv420p_u = yuv420p_y + yuv420p_y_size;
    uint8_t* yuv420p_v = yuv420p_u + yuv420p_u_size;

    
    int64_t pos = mDecoder->GetResource()->Tell();

    VideoData::YCbCrBuffer b;
    b.mPlanes[0].mData = yuv420p_y;
    b.mPlanes[0].mWidth = mVideoTrack.mWidth;
    b.mPlanes[0].mHeight = mVideoTrack.mHeight;
    b.mPlanes[0].mStride = stride;
    b.mPlanes[0].mOffset = 0;
    b.mPlanes[0].mSkip = 0;

    b.mPlanes[1].mData = yuv420p_u;
    b.mPlanes[1].mWidth = (mVideoTrack.mWidth + 1) / 2;
    b.mPlanes[1].mHeight = (mVideoTrack.mHeight + 1) / 2;
    b.mPlanes[1].mStride = (stride + 1) / 2;
    b.mPlanes[1].mOffset = 0;
    b.mPlanes[1].mSkip = 0;

    b.mPlanes[2].mData = yuv420p_v;
    b.mPlanes[2].mWidth =(mVideoTrack.mWidth + 1) / 2;
    b.mPlanes[2].mHeight = (mVideoTrack.mHeight + 1) / 2;
    b.mPlanes[2].mStride = (stride + 1) / 2;
    b.mPlanes[2].mOffset = 0;
    b.mPlanes[2].mSkip = 0;

    VideoData *v = VideoData::Create(
      mInfo.mVideo,
      mDecoder->GetImageContainer(),
      pos,
      bufferInfo.mTimeUs,
      1, 
      b,
      bufferInfo.mFlags & MediaCodec::BUFFER_FLAG_SYNCFRAME,
      -1,
      mVideoTrack.mRelativePictureRect);

    if (v) {
      result = true;
      VideoQueue().Push(v);
    } else {
      NS_WARNING("Unable to create VideoData");
    }
  }

  if ((bufferInfo.mFlags & MediaCodec::BUFFER_FLAG_EOS) ||
      (status == ERROR_END_OF_STREAM)) {
    VideoQueue().Finish();
  }
  mVideoTrack.mCodec->releaseOutputBuffer(bufferInfo.mIndex);

  return result;
}

nsresult
MediaCodecReader::Seek(int64_t aTime,
                       int64_t aStartTime,
                       int64_t aEndTime,
                       int64_t aCurrentTime)
{
  MOZ_ASSERT(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  mVideoTrack.mSeekTimeUs = aTime;
  mAudioTrack.mSeekTimeUs = aTime;
  mVideoTrack.mInputEndOfStream = false;
  mVideoTrack.mOutputEndOfStream = false;
  mAudioTrack.mInputEndOfStream = false;
  mAudioTrack.mOutputEndOfStream = false;
  mAudioTrack.mFlushed = false;
  mVideoTrack.mFlushed = false;

  if (CheckVideoResources()) {
    VideoFrameContainer* videoframe = mDecoder->GetVideoFrameContainer();
    if (videoframe) {
      layers::ImageContainer* image = videoframe->GetImageContainer();
      if (image) {
        image->ClearAllImagesExceptFront();
      }
    }

    MediaBuffer* source_buffer = nullptr;
    MediaSource::ReadOptions options;
    int64_t timestamp = sInvalidTimestampUs;
    options.setSeekTo(aTime, MediaSource::ReadOptions::SEEK_PREVIOUS_SYNC);
    if (mVideoTrack.mSource->read(&source_buffer, &options) != OK ||
        source_buffer == nullptr) {
      return NS_ERROR_FAILURE;
    }
    sp<MetaData> format = source_buffer->meta_data();
    if (format != nullptr) {
      if (format->findInt64(kKeyTime, &timestamp) &&
          IsValidTimestampUs(timestamp)) {
        mVideoTrack.mSeekTimeUs = timestamp;
        mAudioTrack.mSeekTimeUs = timestamp;
      }
      format = nullptr;
    }
    source_buffer->release();

    MOZ_ASSERT(mVideoTrack.mTaskQueue->IsEmpty());
    DispatchVideoTask(mVideoTrack.mSeekTimeUs);

    if (CheckAudioResources()) {
      MOZ_ASSERT(mAudioTrack.mTaskQueue->IsEmpty());
      DispatchAudioTask();
    }
  } else if (CheckAudioResources()) {
    MOZ_ASSERT(mAudioTrack.mTaskQueue->IsEmpty());
    DispatchAudioTask();
  }
  return NS_OK;
}

bool
MediaCodecReader::IsMediaSeekable()
{
  
  return (mExtractor != nullptr) &&
         (mExtractor->flags() & MediaExtractor::CAN_SEEK);
}

sp<MediaSource>
MediaCodecReader::GetAudioOffloadTrack()
{
  return mAudioOffloadTrack.mSource;
}

bool
MediaCodecReader::ReallocateResources()
{
  if (CreateLooper() &&
      CreateExtractor() &&
      CreateMediaSources() &&
      CreateMediaCodecs() &&
      CreateTaskQueues()) {
    return true;
  }

  ReleaseResources();
  return false;
}

void
MediaCodecReader::ReleaseCriticalResources()
{
  ResetDecode();
  
  
  VideoFrameContainer* videoframe = mDecoder->GetVideoFrameContainer();
  if (videoframe) {
    videoframe->ClearCurrentFrame();
  }

  DestroyMediaCodecs();

  ClearColorConverterBuffer();
}

void
MediaCodecReader::ReleaseResources()
{
  ReleaseCriticalResources();
  DestroyMediaSources();
  DestroyExtractor();
  DestroyLooper();
  ShutdownTaskQueues();
}

bool
MediaCodecReader::CreateLooper()
{
  if (mLooper != nullptr) {
    return true;
  }

  
  mLooper = new ALooper;
  mLooper->setName("MediaCodecReader");

  
  mLooper->registerHandler(mHandler);

  
  if (mLooper->start() != OK) {
    return false;
  }

  return true;
}

void
MediaCodecReader::DestroyLooper()
{
  if (mLooper == nullptr) {
    return;
  }

  
  if (mHandler != nullptr) {
    mLooper->unregisterHandler(mHandler->id());
  }

  
  mLooper->stop();

  
  mLooper = nullptr;
}

bool
MediaCodecReader::CreateExtractor()
{
  if (mExtractor != nullptr) {
    return true;
  }

  
  DataSource::RegisterDefaultSniffers();

  if (mExtractor == nullptr) {
    sp<DataSource> dataSource = new MediaStreamSource(mDecoder->GetResource());

    if (dataSource->initCheck() != OK) {
      return false;
    }

    mExtractor = MediaExtractor::Create(dataSource);
  }

  return mExtractor != nullptr;
}

void
MediaCodecReader::DestroyExtractor()
{
  mExtractor = nullptr;
}

bool
MediaCodecReader::CreateMediaSources()
{
  if (mExtractor == nullptr) {
    return false;
  }

  sp<MetaData> extractorMetaData = mExtractor->getMetaData();
  

  const ssize_t invalidTrackIndex = -1;
  ssize_t audioTrackIndex = invalidTrackIndex;
  ssize_t videoTrackIndex = invalidTrackIndex;

  for (size_t i = 0; i < mExtractor->countTracks(); ++i) {
    sp<MetaData> trackFormat = mExtractor->getTrackMetaData(i);

    const char* mime;
    if (!trackFormat->findCString(kKeyMIMEType, &mime)) {
      continue;
    }

    if (audioTrackIndex == invalidTrackIndex &&
        !strncasecmp(mime, "audio/", 6)) {
      audioTrackIndex = i;
    } else if (videoTrackIndex == invalidTrackIndex &&
               !strncasecmp(mime, "video/", 6)) {
      videoTrackIndex = i;
    }
  }

  if (audioTrackIndex == invalidTrackIndex &&
      videoTrackIndex == invalidTrackIndex) {
    NS_WARNING("OMX decoder could not find audio or video tracks");
    return false;
  }

  if (audioTrackIndex != invalidTrackIndex && mAudioTrack.mSource == nullptr) {
    sp<MediaSource> audioSource = mExtractor->getTrack(audioTrackIndex);
    if (audioSource != nullptr && audioSource->start() == OK) {
      mAudioTrack.mSource = audioSource;
      mAudioTrack.mSourceIsStopped = false;
    }
    
    mAudioOffloadTrack.mSource = mExtractor->getTrack(audioTrackIndex);
  }

  if (videoTrackIndex != invalidTrackIndex && mVideoTrack.mSource == nullptr) {
    sp<MediaSource> videoSource = mExtractor->getTrack(videoTrackIndex);
    if (videoSource != nullptr && videoSource->start() == OK) {
      mVideoTrack.mSource = videoSource;
      mVideoTrack.mSourceIsStopped = false;
    }
  }

  return
    (audioTrackIndex == invalidTrackIndex || mAudioTrack.mSource != nullptr) &&
    (videoTrackIndex == invalidTrackIndex || mVideoTrack.mSource != nullptr);
}

void
MediaCodecReader::DestroyMediaSources()
{
  mAudioTrack.mSource = nullptr;
  mVideoTrack.mSource = nullptr;
  mAudioOffloadTrack.mSource = nullptr;
}

void
MediaCodecReader::ShutdownTaskQueues()
{
  if(mAudioTrack.mTaskQueue) {
    mAudioTrack.mTaskQueue->Shutdown();
    mAudioTrack.mTaskQueue = nullptr;
  }
  if(mVideoTrack.mTaskQueue) {
    mVideoTrack.mTaskQueue->Shutdown();
    mVideoTrack.mTaskQueue = nullptr;
  }
}

bool
MediaCodecReader::CreateTaskQueues()
{
  if (mAudioTrack.mSource != nullptr && mAudioTrack.mCodec != nullptr &&
      !mAudioTrack.mTaskQueue) {
    mAudioTrack.mTaskQueue = new MediaTaskQueue(
      SharedThreadPool::Get(NS_LITERAL_CSTRING("MediaCodecReader Audio"), 1));
    NS_ENSURE_TRUE(mAudioTrack.mTaskQueue, false);
  }
 if (mVideoTrack.mSource != nullptr && mVideoTrack.mCodec != nullptr &&
     !mVideoTrack.mTaskQueue) {
    mVideoTrack.mTaskQueue = new MediaTaskQueue(
      SharedThreadPool::Get(NS_LITERAL_CSTRING("MediaCodecReader Video"), 1));
    NS_ENSURE_TRUE(mVideoTrack.mTaskQueue, false);
  }

  return true;
}

bool
MediaCodecReader::CreateMediaCodecs()
{
  if (CreateMediaCodec(mLooper, mAudioTrack, false, nullptr) &&
      CreateMediaCodec(mLooper, mVideoTrack, true, mVideoListener)) {
    return true;
  }

  return false;
}

bool
MediaCodecReader::CreateMediaCodec(sp<ALooper>& aLooper,
                                   Track& aTrack,
                                   bool aAsync,
                                   wp<MediaCodecProxy::CodecResourceListener> aListener)
{
  if (aTrack.mSource != nullptr && aTrack.mCodec == nullptr) {
    sp<MetaData> sourceFormat = aTrack.mSource->getFormat();

    const char* mime;
    if (sourceFormat->findCString(kKeyMIMEType, &mime)) {
      aTrack.mCodec = MediaCodecProxy::CreateByType(aLooper, mime, false, aAsync, aListener);
    }

    if (aTrack.mCodec == nullptr) {
      NS_WARNING("Couldn't create MediaCodecProxy");
      return false;
    }

    if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_VORBIS)) {
      aTrack.mInputCopier = new VorbisInputCopier;
    } else {
      aTrack.mInputCopier = new TrackInputCopier;
    }

    if (!aAsync) {
      
      
      if (!aTrack.mCodec->allocated() || !ConfigureMediaCodec(aTrack)){
        NS_WARNING("Couldn't create and configure MediaCodec synchronously");
        aTrack.mCodec = nullptr;
        return false;
      }
    }
  }

  return true;
}

bool
MediaCodecReader::ConfigureMediaCodec(Track& aTrack)
{

  if (aTrack.mSource != nullptr && aTrack.mCodec != nullptr) {
    if (!aTrack.mCodec->allocated()) {
      return false;
    }

    sp<MetaData> sourceFormat = aTrack.mSource->getFormat();
    sp<AMessage> codecFormat;
    convertMetaDataToMessage(sourceFormat, &codecFormat);

    bool allpass = true;
    if (allpass && aTrack.mCodec->configure(codecFormat, nullptr, nullptr, 0) != OK) {
      NS_WARNING("Couldn't configure MediaCodec");
      allpass = false;
    }
    if (allpass && aTrack.mCodec->start() != OK) {
      NS_WARNING("Couldn't start MediaCodec");
      allpass = false;
    }
    if (allpass && aTrack.mCodec->getInputBuffers(&aTrack.mInputBuffers) != OK) {
      NS_WARNING("Couldn't get input buffers from MediaCodec");
      allpass = false;
    }
    if (allpass && aTrack.mCodec->getOutputBuffers(&aTrack.mOutputBuffers) != OK) {
      NS_WARNING("Couldn't get output buffers from MediaCodec");
      allpass = false;
    }
    if (!allpass) {
      aTrack.mCodec = nullptr;
      return false;
    }
  }

  return true;
}

void
MediaCodecReader::DestroyMediaCodecs()
{
  DestroyMediaCodecs(mAudioTrack);
  DestroyMediaCodecs(mVideoTrack);
}

void
MediaCodecReader::DestroyMediaCodecs(Track& aTrack)
{
  aTrack.mCodec = nullptr;
}

bool
MediaCodecReader::UpdateDuration()
{
  
  if (mAudioTrack.mSource != nullptr) {
    sp<MetaData> audioFormat = mAudioTrack.mSource->getFormat();
    if (audioFormat != nullptr) {
      int64_t audioDurationUs = 0LL;
      if (audioFormat->findInt64(kKeyDuration, &audioDurationUs) &&
          audioDurationUs > mAudioTrack.mDurationUs) {
        mAudioTrack.mDurationUs = audioDurationUs;
      }
    }
  }
  

  
  if (mVideoTrack.mSource != nullptr) {
    sp<MetaData> videoFormat = mVideoTrack.mSource->getFormat();
    if (videoFormat != nullptr) {
      int64_t videoDurationUs = 0LL;
      if (videoFormat->findInt64(kKeyDuration, &videoDurationUs) &&
          videoDurationUs > mVideoTrack.mDurationUs) {
        mVideoTrack.mDurationUs = videoDurationUs;
      }
    }
  }

  return true;
}

bool
MediaCodecReader::UpdateAudioInfo()
{
  if (mAudioTrack.mSource == nullptr && mAudioTrack.mCodec == nullptr) {
    
    return true;
  }

  if (mAudioTrack.mSource == nullptr || mAudioTrack.mCodec == nullptr ||
      !mAudioTrack.mCodec->allocated()) {
    
    MOZ_ASSERT(mAudioTrack.mSource != nullptr, "mAudioTrack.mSource should not be nullptr");
    MOZ_ASSERT(mAudioTrack.mCodec != nullptr, "mAudioTrack.mCodec should not be nullptr");
    MOZ_ASSERT(mAudioTrack.mCodec->allocated(), "mAudioTrack.mCodec->allocated() should not be false");
    return false;
  }

  
  sp<MetaData> audioSourceFormat = mAudioTrack.mSource->getFormat();
  if (audioSourceFormat == nullptr) {
    return false;
  }

  
  if (!EnsureCodecFormatParsed(mAudioTrack)){
    return false;
  }

  
  sp<AMessage> audioCodecFormat;
  if (mAudioTrack.mCodec->getOutputFormat(&audioCodecFormat) != OK ||
      audioCodecFormat == nullptr) {
    return false;
  }

  AString codec_mime;
  int32_t codec_channel_count = 0;
  int32_t codec_sample_rate = 0;
  if (!audioCodecFormat->findString("mime", &codec_mime) ||
      !audioCodecFormat->findInt32("channel-count", &codec_channel_count) ||
      !audioCodecFormat->findInt32("sample-rate", &codec_sample_rate)) {
    return false;
  }

  
  mInfo.mAudio.mHasAudio = true;
  mInfo.mAudio.mChannels = codec_channel_count;
  mInfo.mAudio.mRate = codec_sample_rate;

  return true;
}

bool
MediaCodecReader::UpdateVideoInfo()
{
  if (mVideoTrack.mSource == nullptr && mVideoTrack.mCodec == nullptr) {
    
    return true;
  }

  if (mVideoTrack.mSource == nullptr || mVideoTrack.mCodec == nullptr ||
      !mVideoTrack.mCodec->allocated()) {
    
    MOZ_ASSERT(mVideoTrack.mSource != nullptr, "mVideoTrack.mSource should not be nullptr");
    MOZ_ASSERT(mVideoTrack.mCodec != nullptr, "mVideoTrack.mCodec should not be nullptr");
    MOZ_ASSERT(mVideoTrack.mCodec->allocated(), "mVideoTrack.mCodec->allocated() should not be false");
    return false;
  }

  
  sp<MetaData> videoSourceFormat = mVideoTrack.mSource->getFormat();
  if (videoSourceFormat == nullptr) {
    return false;
  }
  int32_t container_width = 0;
  int32_t container_height = 0;
  int32_t container_rotation = 0;
  if (!videoSourceFormat->findInt32(kKeyWidth, &container_width) ||
      !videoSourceFormat->findInt32(kKeyHeight, &container_height)) {
    return false;
  }
  mVideoTrack.mFrameSize = nsIntSize(container_width, container_height);
  if (videoSourceFormat->findInt32(kKeyRotation, &container_rotation)) {
    mVideoTrack.mRotation = container_rotation;
  }

  
  if (!EnsureCodecFormatParsed(mVideoTrack)){
    return false;
  }

  
  sp<AMessage> videoCodecFormat;
  if (mVideoTrack.mCodec->getOutputFormat(&videoCodecFormat) != OK ||
      videoCodecFormat == nullptr) {
    return false;
  }
  AString codec_mime;
  int32_t codec_width = 0;
  int32_t codec_height = 0;
  int32_t codec_stride = 0;
  int32_t codec_slice_height = 0;
  int32_t codec_color_format = 0;
  int32_t codec_crop_left = 0;
  int32_t codec_crop_top = 0;
  int32_t codec_crop_right = 0;
  int32_t codec_crop_bottom = 0;
  if (!videoCodecFormat->findString("mime", &codec_mime) ||
      !videoCodecFormat->findInt32("width", &codec_width) ||
      !videoCodecFormat->findInt32("height", &codec_height) ||
      !videoCodecFormat->findInt32("stride", &codec_stride) ||
      !videoCodecFormat->findInt32("slice-height", &codec_slice_height) ||
      !videoCodecFormat->findInt32("color-format", &codec_color_format) ||
      !videoCodecFormat->findRect("crop", &codec_crop_left, &codec_crop_top,
                                  &codec_crop_right, &codec_crop_bottom)) {
    return false;
  }

  mVideoTrack.mWidth = codec_width;
  mVideoTrack.mHeight = codec_height;
  mVideoTrack.mStride = codec_stride;
  mVideoTrack.mSliceHeight = codec_slice_height;
  mVideoTrack.mColorFormat = codec_color_format;

  
  
  int32_t display_width = codec_crop_right - codec_crop_left + 1;
  int32_t display_height = codec_crop_bottom - codec_crop_top + 1;
  nsIntRect picture_rect(0, 0, mVideoTrack.mWidth, mVideoTrack.mHeight);
  nsIntSize display_size(display_width, display_height);
  if (!IsValidVideoRegion(mVideoTrack.mFrameSize, picture_rect, display_size)) {
    return false;
  }

  
  gfx::IntRect relative_picture_rect = gfx::ToIntRect(picture_rect);
  if (mVideoTrack.mWidth != mVideoTrack.mFrameSize.width ||
      mVideoTrack.mHeight != mVideoTrack.mFrameSize.height) {
    
    
    
    relative_picture_rect.x = (picture_rect.x * mVideoTrack.mWidth) /
                              mVideoTrack.mFrameSize.width;
    relative_picture_rect.y = (picture_rect.y * mVideoTrack.mHeight) /
                              mVideoTrack.mFrameSize.height;
    relative_picture_rect.width = (picture_rect.width * mVideoTrack.mWidth) /
                                  mVideoTrack.mFrameSize.width;
    relative_picture_rect.height = (picture_rect.height * mVideoTrack.mHeight) /
                                   mVideoTrack.mFrameSize.height;
  }

  
  mInfo.mVideo.mHasVideo = true;
  mVideoTrack.mPictureRect = picture_rect;
  mInfo.mVideo.mDisplay = display_size;
  mVideoTrack.mRelativePictureRect = relative_picture_rect;

  return true;
}

status_t
MediaCodecReader::FlushCodecData(Track& aTrack)
{
  if (aTrack.mSource == nullptr || aTrack.mCodec == nullptr ||
      !aTrack.mCodec->allocated()) {
    return UNKNOWN_ERROR;
  }

  status_t status = aTrack.mCodec->flush();
  aTrack.mFlushed = (status == OK);
  if (aTrack.mFlushed) {
    aTrack.mInputIndex = sInvalidInputIndex;
  }

  return status;
}



status_t
MediaCodecReader::FillCodecInputData(Track& aTrack)
{
  if (aTrack.mSource == nullptr || aTrack.mCodec == nullptr ||
      !aTrack.mCodec->allocated()) {
    return UNKNOWN_ERROR;
  }

  if (aTrack.mInputEndOfStream) {
    return ERROR_END_OF_STREAM;
  }

  if (IsValidTimestampUs(aTrack.mSeekTimeUs) && !aTrack.mFlushed) {
    FlushCodecData(aTrack);
  }

  size_t index = 0;
  while (aTrack.mInputIndex.isValid() ||
         aTrack.mCodec->dequeueInputBuffer(&index) == OK) {
    if (!aTrack.mInputIndex.isValid()) {
      aTrack.mInputIndex = index;
    }
    MOZ_ASSERT(aTrack.mInputIndex.isValid(), "aElement.mInputIndex should be valid");

    
    if (aTrack.mSourceIsStopped) {
      if (aTrack.mSource->start() == OK) {
        aTrack.mSourceIsStopped = false;
      } else {
        return UNKNOWN_ERROR;
      }
    }
    MediaBuffer* source_buffer = nullptr;
    status_t status = OK;
    if (IsValidTimestampUs(aTrack.mSeekTimeUs)) {
      MediaSource::ReadOptions options;
      options.setSeekTo(aTrack.mSeekTimeUs);
      status = aTrack.mSource->read(&source_buffer, &options);
    } else {
      status = aTrack.mSource->read(&source_buffer);
    }

    
    if (status == INFO_FORMAT_CHANGED) {
      return INFO_FORMAT_CHANGED;
    } else if (status == ERROR_END_OF_STREAM) {
      aTrack.mInputEndOfStream = true;
      aTrack.mCodec->queueInputBuffer(aTrack.mInputIndex.value(),
                                      0, 0, 0,
                                      MediaCodec::BUFFER_FLAG_EOS);
      return ERROR_END_OF_STREAM;
    } else if (status == -ETIMEDOUT) {
      return OK; 
    } else if (status != OK) {
      return status;
    } else if (source_buffer == nullptr) {
      return UNKNOWN_ERROR;
    }

    
    aTrack.mInputEndOfStream = false;
    aTrack.mSeekTimeUs = sInvalidTimestampUs;

    sp<ABuffer> input_buffer = nullptr;
    if (aTrack.mInputIndex.value() < aTrack.mInputBuffers.size()) {
      input_buffer = aTrack.mInputBuffers[aTrack.mInputIndex.value()];
    }
    if (input_buffer != nullptr &&
        aTrack.mInputCopier != nullptr &&
        aTrack.mInputCopier->Copy(source_buffer, input_buffer)) {
      int64_t timestamp = sInvalidTimestampUs;
      sp<MetaData> codec_format = source_buffer->meta_data();
      if (codec_format != nullptr) {
        codec_format->findInt64(kKeyTime, &timestamp);
      }

      status = aTrack.mCodec->queueInputBuffer(
        aTrack.mInputIndex.value(), input_buffer->offset(),
        input_buffer->size(), timestamp, 0);
      if (status == OK) {
        aTrack.mInputIndex = sInvalidInputIndex;
      }
    }
    source_buffer->release();

    if (status != OK) {
      return status;
    }
  }

  return OK;
}

status_t
MediaCodecReader::GetCodecOutputData(Track& aTrack,
                                     CodecBufferInfo& aBuffer,
                                     int64_t aThreshold,
                                     const TimeStamp& aTimeout)
{
  
  CodecBufferInfo info;
  status_t status = OK;
  while (status == OK || status == INFO_OUTPUT_BUFFERS_CHANGED ||
         status == -EAGAIN) {

    int64_t duration = (int64_t)(aTimeout - TimeStamp::Now()).ToMicroseconds();
    if (!IsValidDurationUs(duration)) {
      return -EAGAIN;
    }

    status = aTrack.mCodec->dequeueOutputBuffer(&info.mIndex, &info.mOffset,
      &info.mSize, &info.mTimeUs, &info.mFlags, duration);
    
    if (status == ERROR_END_OF_STREAM ||
        (info.mFlags & MediaCodec::BUFFER_FLAG_EOS)) {
      aBuffer = info;
      aBuffer.mBuffer = aTrack.mOutputBuffers[info.mIndex];
      aTrack.mOutputEndOfStream = true;
      return ERROR_END_OF_STREAM;
    }

    if (status == OK) {
      if (!IsValidTimestampUs(aThreshold) || info.mTimeUs >= aThreshold) {
        
        break;
      } else {
        aTrack.mCodec->releaseOutputBuffer(info.mIndex);
      }
    } else if (status == INFO_OUTPUT_BUFFERS_CHANGED) {
      
      if (aTrack.mCodec->getOutputBuffers(&aTrack.mOutputBuffers) != OK) {
        NS_WARNING("Couldn't get output buffers from MediaCodec");
        aTrack.mCodec = nullptr;
        return UNKNOWN_ERROR;
      }
    }

    if (TimeStamp::Now() > aTimeout) {
      
      return -EAGAIN;
    }
  }

  if (status != OK) {
    
    return status;
  }

  if (info.mIndex >= aTrack.mOutputBuffers.size()) {
    NS_WARNING("Couldn't get proper index of output buffers from MediaCodec");
    aTrack.mCodec->releaseOutputBuffer(info.mIndex);
    return UNKNOWN_ERROR;
  }

  aBuffer = info;
  aBuffer.mBuffer = aTrack.mOutputBuffers[info.mIndex];

  return OK;
}

bool
MediaCodecReader::EnsureCodecFormatParsed(Track& aTrack)
{
  if (aTrack.mSource == nullptr || aTrack.mCodec == nullptr ||
      !aTrack.mCodec->allocated()) {
    return false;
  }

  sp<AMessage> format;
  if (aTrack.mCodec->getOutputFormat(&format) == OK) {
    return true;
  }

  status_t status = OK;
  size_t index = 0;
  size_t offset = 0;
  size_t size = 0;
  int64_t timeUs = 0LL;
  uint32_t flags = 0;
  while ((status = aTrack.mCodec->dequeueOutputBuffer(&index, &offset, &size,
                     &timeUs, &flags)) != INFO_FORMAT_CHANGED) {
    if (status == OK) {
      aTrack.mCodec->releaseOutputBuffer(index);
    }
    status = FillCodecInputData(aTrack);
    if (status == INFO_FORMAT_CHANGED) {
      break;
    } else if (status != OK) {
      return false;
    }
  }
  return aTrack.mCodec->getOutputFormat(&format) == OK;
}

uint8_t*
MediaCodecReader::GetColorConverterBuffer(int32_t aWidth, int32_t aHeight)
{
  
  size_t yuv420p_y_size = aWidth * aHeight;
  size_t yuv420p_u_size = ((aWidth + 1) / 2) * ((aHeight + 1) / 2);
  size_t yuv420p_v_size = yuv420p_u_size;
  size_t yuv420p_size = yuv420p_y_size + yuv420p_u_size + yuv420p_v_size;
  if (mColorConverterBufferSize != yuv420p_size) {
    mColorConverterBuffer = nullptr; 
    mColorConverterBuffer = new uint8_t[yuv420p_size];
    mColorConverterBufferSize = yuv420p_size;
  }
  return mColorConverterBuffer.get();
}

void
MediaCodecReader::ClearColorConverterBuffer()
{
  mColorConverterBuffer = nullptr;
  mColorConverterBufferSize = 0;
}


void
MediaCodecReader::onMessageReceived(const sp<AMessage>& aMessage)
{
  switch (aMessage->what()) {

    case kNotifyCodecReserved:
    {
      
      
      mDecoder->NotifyWaitingForResourcesStatusChanged();
      break;
    }

    case kNotifyCodecCanceled:
    {
      ReleaseCriticalResources();
      break;
    }

    default:
      TRESPASS();
      break;
  }
}


void
MediaCodecReader::codecReserved(Track& aTrack)
{
  if (!ConfigureMediaCodec(aTrack)) {
    DestroyMediaCodecs(aTrack);
    return;
  }

  if (mHandler != nullptr) {
    
    sp<AMessage> notify = new AMessage(kNotifyCodecReserved, mHandler->id());
    notify->post();
  }
}


void
MediaCodecReader::codecCanceled(Track& aTrack)
{
  DestroyMediaCodecs(aTrack);

  if (mHandler != nullptr) {
    
    sp<AMessage> notify = new AMessage(kNotifyCodecCanceled, mHandler->id());
    notify->post();
  }
}

} 
