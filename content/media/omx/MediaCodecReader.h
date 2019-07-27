





#ifndef MEDIA_CODEC_READER_H
#define MEDIA_CODEC_READER_H

#include <utils/threads.h>

#include <base/message_loop.h>

#include <mozilla/CheckedInt.h>
#include <mozilla/Mutex.h>
#include <mozilla/Monitor.h>

#include "MediaData.h"

#include "I420ColorConverterHelper.h"
#include "MediaCodecProxy.h"
#include "MediaOmxCommonReader.h"

namespace android {
struct ALooper;
struct AMessage;

class MOZ_EXPORT MediaExtractor;
class MOZ_EXPORT MetaData;
class MOZ_EXPORT MediaBuffer;
struct MOZ_EXPORT MediaSource;
} 

namespace mozilla {

class MediaTaskQueue;
class MP3FrameParser;

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

  
  
  
  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset);

  
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

    
    Mutex mDurationLock; 
                         
    int64_t mDurationUs;

    
    CheckedUint32 mInputIndex;
    
    
    
    bool mInputEndOfStream;
    bool mOutputEndOfStream;
    int64_t mSeekTimeUs;
    bool mFlushed; 
    bool mDiscontinuity;
    nsRefPtr<MediaTaskQueue> mTaskQueue;

  private:
    
    Track(const Track &rhs) MOZ_DELETE;
    const Track &operator=(const Track&) MOZ_DELETE;
  };

  
  
  void onMessageReceived(const android::sp<android::AMessage>& aMessage);

  
  
  virtual void codecReserved(Track& aTrack);
  virtual void codecCanceled(Track& aTrack);

  virtual bool CreateExtractor();

  
  
  void UpdateIsWaitingMediaResources();

  android::sp<android::MediaExtractor> mExtractor;
  
  
  bool mIsWaitingResources;

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

  private:
    
    AudioTrack(const AudioTrack &rhs) MOZ_DELETE;
    const AudioTrack &operator=(const AudioTrack &rhs) MOZ_DELETE;
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

  private:
    
    VideoTrack(const VideoTrack &rhs) MOZ_DELETE;
    const VideoTrack &operator=(const VideoTrack &rhs) MOZ_DELETE;
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
    
    SignalObject() MOZ_DELETE;
    SignalObject(const SignalObject &rhs) MOZ_DELETE;
    const SignalObject &operator=(const SignalObject &rhs) MOZ_DELETE;

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
    
    ParseCachedDataRunnable() MOZ_DELETE;
    ParseCachedDataRunnable(const ParseCachedDataRunnable &rhs) MOZ_DELETE;
    const ParseCachedDataRunnable &operator=(const ParseCachedDataRunnable &rhs) MOZ_DELETE;

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
    
    ProcessCachedDataTask() MOZ_DELETE;
    ProcessCachedDataTask(const ProcessCachedDataTask &rhs) MOZ_DELETE;
    const ProcessCachedDataTask &operator=(const ProcessCachedDataTask &rhs) MOZ_DELETE;

    nsRefPtr<MediaCodecReader> mReader;
    int64_t mOffset;
  };
  friend class ProcessCachedDataTask;

  
  
  
  
  
  
  
  template<class T>
  class ReferenceKeeperRunnable : public nsRunnable
  {
  public:
    ReferenceKeeperRunnable(nsRefPtr<T> aPointer)
      : mPointer(aPointer)
    {
    }

    NS_IMETHOD Run() MOZ_OVERRIDE
    {
      mPointer = nullptr;
      return NS_OK;
    }

  private:
    
    ReferenceKeeperRunnable() MOZ_DELETE;
    ReferenceKeeperRunnable(const ReferenceKeeperRunnable &rhs) MOZ_DELETE;
    const ReferenceKeeperRunnable &operator=(const ReferenceKeeperRunnable &rhs) MOZ_DELETE;

    nsRefPtr<T> mPointer;
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

  bool TriggerIncrementalParser();

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

  int64_t ProcessCachedData(int64_t aOffset,
                            nsRefPtr<SignalObject> aSignal);
  bool ParseDataSegment(const char* aBuffer,
                        uint32_t aLength,
                        int64_t aOffset);

  android::sp<MessageHandler> mHandler;
  android::sp<VideoResourceListener> mVideoListener;

  android::sp<android::ALooper> mLooper;
  android::sp<android::MetaData> mMetaData;

  
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
};

} 

#endif 
