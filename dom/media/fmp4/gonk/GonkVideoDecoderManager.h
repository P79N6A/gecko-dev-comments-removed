





#if !defined(GonkVideoDecoderManager_h_)
#define GonkVideoDecoderManager_h_

#include <set>
#include "MP4Reader.h"
#include "nsRect.h"
#include "GonkMediaDataDecoder.h"
#include "mozilla/RefPtr.h"
#include "I420ColorConverterHelper.h"
#include "MediaCodecProxy.h"
#include <stagefright/foundation/AHandler.h>
#include "GonkNativeWindow.h"
#include "GonkNativeWindowClient.h"

using namespace android;

namespace android {
struct ALooper;
class MediaBuffer;
struct MOZ_EXPORT AString;
class GonkNativeWindow;
} 

namespace mozilla {

namespace layers {
class TextureClient;
} 

class GonkVideoDecoderManager : public GonkDecoderManager {
typedef android::MediaCodecProxy MediaCodecProxy;
typedef mozilla::layers::TextureClient TextureClient;

public:
  GonkVideoDecoderManager(mozilla::layers::ImageContainer* aImageContainer,
		          const mp4_demuxer::VideoDecoderConfig& aConfig);

  ~GonkVideoDecoderManager();

  virtual android::sp<MediaCodecProxy> Init(MediaDataDecoderCallback* aCallback) MOZ_OVERRIDE;

  virtual nsresult Output(int64_t aStreamOffset,
                          nsRefPtr<MediaData>& aOutput) MOZ_OVERRIDE;

  virtual nsresult Flush() MOZ_OVERRIDE;

  virtual void AllocateMediaResources();

  virtual void ReleaseMediaResources();

  static void RecycleCallback(TextureClient* aClient, void* aClosure);

protected:
  virtual void PerformFormatSpecificProcess(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE;

  virtual android::status_t SendSampleToOMX(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE;

private:
  struct FrameInfo
  {
    int32_t mWidth = 0;
    int32_t mHeight = 0;
    int32_t mStride = 0;
    int32_t mSliceHeight = 0;
    int32_t mColorFormat = 0;
    int32_t mCropLeft = 0;
    int32_t mCropTop = 0;
    int32_t mCropRight = 0;
    int32_t mCropBottom = 0;
  };
  class MessageHandler : public android::AHandler
  {
  public:
    MessageHandler(GonkVideoDecoderManager *aManager);
    ~MessageHandler();

    virtual void onMessageReceived(const android::sp<android::AMessage> &aMessage);

  private:
    
    MessageHandler() = delete;
    MessageHandler(const MessageHandler &rhs) = delete;
    const MessageHandler &operator=(const MessageHandler &rhs) = delete;

    GonkVideoDecoderManager *mManager;
  };
  friend class MessageHandler;

  class VideoResourceListener : public android::MediaCodecProxy::CodecResourceListener
  {
  public:
    VideoResourceListener(GonkVideoDecoderManager *aManager);
    ~VideoResourceListener();

    virtual void codecReserved() MOZ_OVERRIDE;
    virtual void codecCanceled() MOZ_OVERRIDE;

  private:
    
    VideoResourceListener() = delete;
    VideoResourceListener(const VideoResourceListener &rhs) = delete;
    const VideoResourceListener &operator=(const VideoResourceListener &rhs) = delete;

    GonkVideoDecoderManager *mManager;
  };
  friend class VideoResourceListener;

  
  
  
  
  struct FrameTimeInfo {
    int64_t pts;       
    int64_t duration;  
  };

  bool SetVideoFormat();

  nsresult CreateVideoData(int64_t aStreamOffset, VideoData** aOutData);
  void ReleaseVideoBuffer();
  uint8_t* GetColorConverterBuffer(int32_t aWidth, int32_t aHeight);

  
  void codecReserved();
  void codecCanceled();
  void onMessageReceived(const sp<AMessage> &aMessage);

  void ReleaseAllPendingVideoBuffers();
  void PostReleaseVideoBuffer(android::MediaBuffer *aBuffer);

  void QueueFrameTimeIn(int64_t aPTS, int64_t aDuration);
  nsresult QueueFrameTimeOut(int64_t aPTS, int64_t& aDuration);

  uint32_t mVideoWidth;
  uint32_t mVideoHeight;
  uint32_t mDisplayWidth;
  uint32_t mDisplayHeight;
  nsIntRect mPicture;
  nsIntSize mInitialFrame;

  android::sp<MediaCodecProxy> mDecoder;
  nsRefPtr<layers::ImageContainer> mImageContainer;
  MediaDataDecoderCallback* mCallback;

  android::MediaBuffer* mVideoBuffer;

  MediaDataDecoderCallback*  mReaderCallback;
  MediaInfo mInfo;
  android::sp<VideoResourceListener> mVideoListener;
  android::sp<MessageHandler> mHandler;
  android::sp<ALooper> mLooper;
  android::sp<ALooper> mManagerLooper;
  FrameInfo mFrameInfo;

  
  
  
  
  
  nsTArray<FrameTimeInfo> mFrameTimeInfo;

  
  android::I420ColorConverterHelper mColorConverter;
  nsAutoArrayPtr<uint8_t> mColorConverterBuffer;
  size_t mColorConverterBufferSize;

  android::sp<android::GonkNativeWindow> mNativeWindow;
  enum {
    kNotifyPostReleaseBuffer = 'nprb',
  };

  
  
  Vector<android::MediaBuffer*> mPendingVideoBuffers;
  
  Mutex mPendingVideoBuffersLock;

};

} 

#endif 
