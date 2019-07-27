





#ifndef MEDIA_CODEC_READER_H
#define MEDIA_CODEC_READER_H

#include <utils/threads.h>

#include <mozilla/CheckedInt.h>

#include "MediaData.h"

#include "I420ColorConverterHelper.h"
#include "MediaCodecProxy.h"
#include "MediaOmxCommonReader.h"

namespace android {
struct ALooper;
struct AMessage;

class MOZ_EXPORT MediaExtractor;
class MOZ_EXPORT MediaBuffer;
struct MOZ_EXPORT MediaSource;
struct MediaCodec;
} 

namespace mozilla {

class MediaTaskQueue;

class MediaCodecReader : public MediaOmxCommonReader
{
public:
  MediaCodecReader(AbstractMediaDecoder* aDecoder);
  virtual ~MediaCodecReader();

  
  
  virtual nsresult Init(MediaDecoderReader* aCloneDonor);

  
  virtual bool IsWaitingMediaResources();

  
  virtual bool IsDormantNeeded();

  
  virtual void ReleaseMediaResources();

  
  
  
  virtual void Shutdown();

  
  virtual nsresult ResetDecode() MOZ_OVERRIDE;

  
  virtual void RequestVideoData(bool aSkipToNextKeyframe,
                                int64_t aTimeThreshold) MOZ_OVERRIDE;

  
  virtual void RequestAudioData() MOZ_OVERRIDE;

  virtual bool HasAudio();
  virtual bool HasVideo();

  
  
  
  
  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags);

  
  
  
  virtual nsresult Seek(int64_t aTime,
                        int64_t aStartTime,
                        int64_t aEndTime,
                        int64_t aCurrentTime);

  virtual bool IsMediaSeekable() MOZ_OVERRIDE;

  virtual android::sp<android::MediaSource> GetAudioOffloadTrack();

protected:
  struct TrackInputCopier
  {
    virtual bool Copy(android::MediaBuffer* aSourceBuffer,
                      android::sp<android::ABuffer> aCodecBuffer);
  };

  struct Track
  {
    Track();

    
    android::sp<android::MediaSource> mSource;
    bool mSourceIsStopped;
    android::sp<android::MediaCodecProxy> mCodec;
    android::Vector<android::sp<android::ABuffer> > mInputBuffers;
    android::Vector<android::sp<android::ABuffer> > mOutputBuffers;

    
    nsAutoPtr<TrackInputCopier> mInputCopier;

    
    int64_t mDurationUs;

    
    CheckedUint32 mInputIndex;
    
    
    
    bool mInputEndOfStream;
    bool mOutputEndOfStream;
    int64_t mSeekTimeUs;
    bool mFlushed; 
    bool mDiscontinuity;
    nsRefPtr<MediaTaskQueue> mTaskQueue;
  };

  
  
  void onMessageReceived(const android::sp<android::AMessage>& aMessage);

  
  
  virtual void codecReserved(Track& aTrack);
  virtual void codecCanceled(Track& aTrack);

  virtual bool CreateExtractor();

  android::sp<android::MediaExtractor> mExtractor;

private:
  
  
  class MessageHandler : public android::AHandler
  {
  public:
    MessageHandler(MediaCodecReader* aReader);
    ~MessageHandler();

    virtual void onMessageReceived(const android::sp<android::AMessage>& aMessage);

  private:
    
    MessageHandler() MOZ_DELETE;
    MessageHandler(const MessageHandler& rhs) MOZ_DELETE;
    const MessageHandler& operator=(const MessageHandler& rhs) MOZ_DELETE;

    MediaCodecReader *mReader;
  };
  friend class MessageHandler;

  
  
  class VideoResourceListener : public android::MediaCodecProxy::CodecResourceListener
  {
  public:
    VideoResourceListener(MediaCodecReader* aReader);
    ~VideoResourceListener();

    virtual void codecReserved();
    virtual void codecCanceled();

  private:
    
    VideoResourceListener() MOZ_DELETE;
    VideoResourceListener(const VideoResourceListener& rhs) MOZ_DELETE;
    const VideoResourceListener& operator=(const VideoResourceListener& rhs) MOZ_DELETE;

    MediaCodecReader* mReader;
  };
  friend class VideoResourceListener;

  class VorbisInputCopier : public TrackInputCopier
  {
    virtual bool Copy(android::MediaBuffer* aSourceBuffer,
                      android::sp<android::ABuffer> aCodecBuffer);
  };

  struct AudioTrack : public Track
  {
    AudioTrack();
  };

  struct VideoTrack : public Track
  {
    VideoTrack();

    int32_t mWidth;
    int32_t mHeight;
    int32_t mStride;
    int32_t mSliceHeight;
    int32_t mColorFormat;
    int32_t mRotation;
    nsIntSize mFrameSize;
    nsIntRect mPictureRect;
    gfx::IntRect mRelativePictureRect;
  };

  struct CodecBufferInfo
  {
    CodecBufferInfo();

    android::sp<android::ABuffer> mBuffer;
    size_t mIndex;
    size_t mOffset;
    size_t mSize;
    int64_t mTimeUs;
    uint32_t mFlags;
  };

  
  MediaCodecReader() MOZ_DELETE;
  const MediaCodecReader& operator=(const MediaCodecReader& rhs) MOZ_DELETE;

  bool ReallocateResources();
  void ReleaseCriticalResources();
  void ReleaseResources();

  bool CreateLooper();
  void DestroyLooper();

  void DestroyExtractor();

  bool CreateMediaSources();
  void DestroyMediaSources();

  bool CreateMediaCodecs();
  static bool CreateMediaCodec(android::sp<android::ALooper>& aLooper,
                               Track& aTrack,
                               bool aAsync,
                               android::wp<android::MediaCodecProxy::CodecResourceListener> aListener);
  static bool ConfigureMediaCodec(Track& aTrack);
  void DestroyMediaCodecs();
  static void DestroyMediaCodecs(Track& aTrack);

  bool CreateTaskQueues();
  void ShutdownTaskQueues();
  bool DecodeVideoFrameTask(int64_t aTimeThreshold);
  bool DecodeVideoFrameSync(int64_t aTimeThreshold);
  bool DecodeAudioDataTask();
  bool DecodeAudioDataSync();
  void DispatchVideoTask(int64_t aTimeThreshold);
  void DispatchAudioTask();
  inline bool CheckVideoResources() {
    return (HasVideo() && mVideoTrack.mSource != nullptr &&
            mVideoTrack.mTaskQueue);
  }

  inline bool CheckAudioResources() {
    return (HasAudio() && mAudioTrack.mSource != nullptr &&
            mAudioTrack.mTaskQueue);
  }

  bool UpdateDuration();
  bool UpdateAudioInfo();
  bool UpdateVideoInfo();

  static android::status_t FlushCodecData(Track& aTrack);
  static android::status_t FillCodecInputData(Track& aTrack);
  static android::status_t GetCodecOutputData(Track& aTrack,
                                              CodecBufferInfo& aBuffer,
                                              int64_t aThreshold,
                                              const TimeStamp& aTimeout);
  static bool EnsureCodecFormatParsed(Track& aTrack);

  uint8_t* GetColorConverterBuffer(int32_t aWidth, int32_t aHeight);
  void ClearColorConverterBuffer();

  android::sp<MessageHandler> mHandler;
  android::sp<VideoResourceListener> mVideoListener;

  android::sp<android::ALooper> mLooper;

  
  AudioTrack mAudioTrack;
  VideoTrack mVideoTrack;
  AudioTrack mAudioOffloadTrack; 

  
  android::I420ColorConverterHelper mColorConverter;
  nsAutoArrayPtr<uint8_t> mColorConverterBuffer;
  size_t mColorConverterBufferSize;
};

} 

#endif 
