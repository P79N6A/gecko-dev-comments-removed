





#ifndef MEDIA_CODEC_READER_H
#define MEDIA_CODEC_READER_H

#include <utils/threads.h>

#include <base/message_loop.h>

#include <mozilla/CheckedInt.h>
#include <mozilla/Mutex.h>
#include <mozilla/Monitor.h>

#include <nsDataHashtable.h>

#include "MediaData.h"

#include "I420ColorConverterHelper.h"
#include "MediaCodecProxy.h"
#include "MediaOmxCommonReader.h"
#include "mozilla/layers/FenceUtils.h"
#if MOZ_WIDGET_GONK && ANDROID_VERSION >= 17
#include <ui/Fence.h>
#endif

namespace android {
struct ALooper;
struct AMessage;

class MOZ_EXPORT MediaExtractor;
class MOZ_EXPORT MetaData;
class MOZ_EXPORT MediaBuffer;
struct MOZ_EXPORT MediaSource;

class GonkNativeWindow;
} 

namespace mozilla {

class FlushableMediaTaskQueue;
class MP3FrameParser;

namespace layers {
class TextureClient;
} 

class MediaCodecReader : public MediaOmxCommonReader
{
  typedef mozilla::layers::TextureClient TextureClient;
  typedef mozilla::layers::FenceHandle FenceHandle;

public:
  MediaCodecReader(AbstractMediaDecoder* aDecoder);
  virtual ~MediaCodecReader();

  
  
  virtual nsresult Init(MediaDecoderReader* aCloneDonor);

  
  virtual bool IsWaitingMediaResources();

  
  virtual bool IsDormantNeeded() { return true;}

  
  virtual void ReleaseMediaResources();

  
  
  
  virtual nsRefPtr<ShutdownPromise> Shutdown();

  
  
  
  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset);

  
  virtual nsresult ResetDecode() MOZ_OVERRIDE;

  
  virtual nsRefPtr<VideoDataPromise>
  RequestVideoData(bool aSkipToNextKeyframe,
                   int64_t aTimeThreshold) MOZ_OVERRIDE;

  
  virtual nsRefPtr<AudioDataPromise> RequestAudioData() MOZ_OVERRIDE;

  virtual bool HasAudio();
  virtual bool HasVideo();

  virtual void PreReadMetadata() MOZ_OVERRIDE;
  
  
  
  
  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags);

  
  
  
  virtual nsRefPtr<SeekPromise>
  Seek(int64_t aTime, int64_t aEndTime) MOZ_OVERRIDE;

  virtual bool IsMediaSeekable() MOZ_OVERRIDE;

  virtual android::sp<android::MediaSource> GetAudioOffloadTrack();

  virtual bool IsAsync() const MOZ_OVERRIDE { return true; }

protected:
  struct TrackInputCopier
  {
    virtual ~TrackInputCopier();

    virtual bool Copy(android::MediaBuffer* aSourceBuffer,
                      android::sp<android::ABuffer> aCodecBuffer);
  };

  struct Track
  {
    enum Type
    {
      kUnknown = 0,
      kAudio,
      kVideo,
    };

    Track(Type type=kUnknown);

    const Type mType;

    
    android::sp<android::MediaSource> mSource;
    bool mSourceIsStopped;
    android::sp<android::MediaCodecProxy> mCodec;
    android::Vector<android::sp<android::ABuffer> > mInputBuffers;
    android::Vector<android::sp<android::ABuffer> > mOutputBuffers;
    android::sp<android::GonkNativeWindow> mNativeWindow;
#if ANDROID_VERSION >= 21
    android::sp<android::IGraphicBufferProducer> mGraphicBufferProducer;
#endif

    
    nsAutoPtr<TrackInputCopier> mInputCopier;

    
    
    int64_t mDurationUs;

    
    CheckedUint32 mInputIndex;
    
    
    
    bool mInputEndOfStream;
    bool mOutputEndOfStream;
    int64_t mSeekTimeUs;
    bool mFlushed; 
    bool mDiscontinuity;
    nsRefPtr<FlushableMediaTaskQueue> mTaskQueue;
    Monitor mTrackMonitor;

  private:
    
    Track(const Track &rhs) = delete;
    const Track &operator=(const Track&) = delete;
  };

  
  
  void onMessageReceived(const android::sp<android::AMessage>& aMessage);

  
  
  virtual void VideoCodecReserved();
  virtual void VideoCodecCanceled();

  virtual bool CreateExtractor();

  
  
  void UpdateIsWaitingMediaResources();

  android::sp<android::MediaExtractor> mExtractor;
  
  
  bool mIsWaitingResources;

private:

  
  
  class VideoResourceListener : public android::MediaCodecProxy::CodecResourceListener
  {
  public:
    VideoResourceListener(MediaCodecReader* aReader);
    ~VideoResourceListener();

    virtual void codecReserved();
    virtual void codecCanceled();

  private:
    
    VideoResourceListener() = delete;
    VideoResourceListener(const VideoResourceListener& rhs) = delete;
    const VideoResourceListener& operator=(const VideoResourceListener& rhs) = delete;

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
    
    MediaPromiseHolder<AudioDataPromise> mAudioPromise;

  private:
    
    AudioTrack(const AudioTrack &rhs) = delete;
    const AudioTrack &operator=(const AudioTrack &rhs) = delete;
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
    
    MediaPromiseHolder<VideoDataPromise> mVideoPromise;

    nsRefPtr<MediaTaskQueue> mReleaseBufferTaskQueue;
  private:
    
    VideoTrack(const VideoTrack &rhs) = delete;
    const VideoTrack &operator=(const VideoTrack &rhs) = delete;
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

  class SignalObject
  {
  public:
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SignalObject)

    SignalObject(const char* aName);
    ~SignalObject();
    void Wait();
    void Signal();

  private:
    
    SignalObject() = delete;
    SignalObject(const SignalObject &rhs) = delete;
    const SignalObject &operator=(const SignalObject &rhs) = delete;

    Monitor mMonitor;
    bool mSignaled;
  };

  class ParseCachedDataRunnable : public nsRunnable
  {
  public:
    ParseCachedDataRunnable(nsRefPtr<MediaCodecReader> aReader,
                            const char* aBuffer,
                            uint32_t aLength,
                            int64_t aOffset,
                            nsRefPtr<SignalObject> aSignal);

    NS_IMETHOD Run() MOZ_OVERRIDE;

  private:
    
    ParseCachedDataRunnable() = delete;
    ParseCachedDataRunnable(const ParseCachedDataRunnable &rhs) = delete;
    const ParseCachedDataRunnable &operator=(const ParseCachedDataRunnable &rhs) = delete;

    nsRefPtr<MediaCodecReader> mReader;
    nsAutoArrayPtr<const char> mBuffer;
    uint32_t mLength;
    int64_t mOffset;
    nsRefPtr<SignalObject> mSignal;
  };
  friend class ParseCachedDataRunnable;

  class ProcessCachedDataTask : public Task
  {
  public:
    ProcessCachedDataTask(nsRefPtr<MediaCodecReader> aReader,
                          int64_t aOffset);

    void Run() MOZ_OVERRIDE;

  private:
    
    ProcessCachedDataTask() = delete;
    ProcessCachedDataTask(const ProcessCachedDataTask &rhs) = delete;
    const ProcessCachedDataTask &operator=(const ProcessCachedDataTask &rhs) = delete;

    nsRefPtr<MediaCodecReader> mReader;
    int64_t mOffset;
  };
  friend class ProcessCachedDataTask;

  
  MediaCodecReader() = delete;
  const MediaCodecReader& operator=(const MediaCodecReader& rhs) = delete;

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
  static void DestroyMediaCodec(Track& aTrack);

  bool CreateTaskQueues();
  void ShutdownTaskQueues();
  void DecodeVideoFrameTask(int64_t aTimeThreshold);
  void DecodeVideoFrameSync(int64_t aTimeThreshold);
  void DecodeAudioDataTask();
  void DecodeAudioDataSync();
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

  bool TriggerIncrementalParser();

  bool UpdateDuration();
  bool UpdateAudioInfo();
  bool UpdateVideoInfo();

  android::status_t FlushCodecData(Track& aTrack);
  android::status_t FillCodecInputData(Track& aTrack);
  android::status_t GetCodecOutputData(Track& aTrack,
                                       CodecBufferInfo& aBuffer,
                                       int64_t aThreshold,
                                       const TimeStamp& aTimeout);
  bool EnsureCodecFormatParsed(Track& aTrack);

  uint8_t* GetColorConverterBuffer(int32_t aWidth, int32_t aHeight);
  void ClearColorConverterBuffer();

  int64_t ProcessCachedData(int64_t aOffset,
                            nsRefPtr<SignalObject> aSignal);
  bool ParseDataSegment(const char* aBuffer,
                        uint32_t aLength,
                        int64_t aOffset);

  static void TextureClientRecycleCallback(TextureClient* aClient,
                                           void* aClosure);
  void TextureClientRecycleCallback(TextureClient* aClient);
  void WaitFenceAndReleaseOutputBuffer();

  void ReleaseRecycledTextureClients();
  static PLDHashOperator ReleaseTextureClient(TextureClient* aClient,
                                              size_t& aIndex,
                                              void* aUserArg);
  PLDHashOperator ReleaseTextureClient(TextureClient* aClient,
                                       size_t& aIndex);

  void ReleaseAllTextureClients();

  android::sp<VideoResourceListener> mVideoListener;

  android::sp<android::ALooper> mLooper;
  android::sp<android::MetaData> mMetaData;

  Mutex mTextureClientIndexesLock;
  nsDataHashtable<nsPtrHashKey<TextureClient>, size_t> mTextureClientIndexes;

  
  AudioTrack mAudioTrack;
  VideoTrack mVideoTrack;
  AudioTrack mAudioOffloadTrack; 

  
  android::I420ColorConverterHelper mColorConverter;
  nsAutoArrayPtr<uint8_t> mColorConverterBuffer;
  size_t mColorConverterBufferSize;

  
  Monitor mParserMonitor;
  bool mParseDataFromCache;
  int64_t mNextParserPosition;
  int64_t mParsedDataLength;
  nsAutoPtr<MP3FrameParser> mMP3FrameParser;
#if MOZ_WIDGET_GONK && ANDROID_VERSION >= 17
  
  
  
  
  struct ReleaseItem {
    ReleaseItem(size_t aIndex, const android::sp<android::Fence>& aFence)
    : mReleaseIndex(aIndex)
    , mReleaseFence(aFence) {}
    size_t mReleaseIndex;
    android::sp<android::Fence> mReleaseFence;
  };
#else
  struct ReleaseItem {
    ReleaseItem(size_t aIndex)
    : mReleaseIndex(aIndex) {}
    size_t mReleaseIndex;
  };
#endif
  nsTArray<ReleaseItem> mPendingReleaseItems;
};

} 

#endif 
