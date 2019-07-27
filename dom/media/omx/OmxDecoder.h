#include <set>
#include <stagefright/foundation/ABase.h>
#include <stagefright/foundation/AHandlerReflector.h>
#include <stagefright/foundation/ALooper.h>
#include <utils/RefBase.h>
#include <stagefright/MediaExtractor.h>

#include "GonkNativeWindow.h"
#include "GonkNativeWindowClient.h"
#include "mozilla/layers/FenceUtils.h"
#include "MP3FrameParser.h"
#include "MPAPI.h"
#include "MediaResource.h"
#include "AbstractMediaDecoder.h"
#include "OMXCodecProxy.h"

namespace android {
class OmxDecoder;
};

namespace android {

class OmxDecoder : public OMXCodecProxy::EventListener {
  typedef MPAPI::AudioFrame AudioFrame;
  typedef MPAPI::VideoFrame VideoFrame;
  typedef mozilla::MP3FrameParser MP3FrameParser;
  typedef mozilla::MediaResource MediaResource;
  typedef mozilla::AbstractMediaDecoder AbstractMediaDecoder;
  typedef mozilla::layers::FenceHandle FenceHandle;
  typedef mozilla::layers::TextureClient TextureClient;

  enum {
    kPreferSoftwareCodecs = 1,
    kSoftwareCodecsOnly = 8,
    kHardwareCodecsOnly = 16,
  };

  enum {
    kNotifyPostReleaseVideoBuffer = 'noti',
    kNotifyStatusChanged = 'stat'
  };

  AbstractMediaDecoder *mDecoder;
  nsRefPtr<MediaResource> mResource;
  sp<GonkNativeWindow> mNativeWindow;
  sp<GonkNativeWindowClient> mNativeWindowClient;
  sp<MediaSource> mVideoTrack;
  sp<OMXCodecProxy> mVideoSource;
  sp<MediaSource> mAudioOffloadTrack;
  sp<MediaSource> mAudioTrack;
  sp<MediaSource> mAudioSource;
  int32_t mDisplayWidth;
  int32_t mDisplayHeight;
  int32_t mVideoWidth;
  int32_t mVideoHeight;
  int32_t mVideoColorFormat;
  int32_t mVideoStride;
  int32_t mVideoSliceHeight;
  int32_t mVideoRotation;
  int32_t mAudioChannels;
  int32_t mAudioSampleRate;
  int64_t mDurationUs;
  VideoFrame mVideoFrame;
  AudioFrame mAudioFrame;
  MP3FrameParser mMP3FrameParser;
  bool mIsMp3;

  
  
  MediaBuffer *mVideoBuffer;
  MediaBuffer *mAudioBuffer;

  struct BufferItem {
    BufferItem()
     : mMediaBuffer(nullptr)
    {
    }
    BufferItem(MediaBuffer* aMediaBuffer, const FenceHandle& aReleaseFenceHandle)
     : mMediaBuffer(aMediaBuffer)
     , mReleaseFenceHandle(aReleaseFenceHandle) {
    }

    MediaBuffer* mMediaBuffer;
    
    FenceHandle mReleaseFenceHandle;
  };

  
  
  
  
  Vector<BufferItem> mPendingVideoBuffers;

  
  std::set<TextureClient*> mPendingRecycleTexutreClients;

  
  Mutex mPendingVideoBuffersLock;

  
  bool mIsVideoSeeking;
  
  
  
  
  Mutex mSeekLock;

  
  
  
  
  sp<ALooper> mLooper;
  
  
  
  sp<AHandlerReflector<OmxDecoder> > mReflector;

  
  bool mAudioMetadataRead;

  void ReleaseVideoBuffer();
  void ReleaseAudioBuffer();
  
  void ReleaseAllPendingVideoBuffersLocked();

  void PlanarYUV420Frame(VideoFrame *aFrame, int64_t aTimeUs, void *aData, size_t aSize, bool aKeyFrame);
  void CbYCrYFrame(VideoFrame *aFrame, int64_t aTimeUs, void *aData, size_t aSize, bool aKeyFrame);
  void SemiPlanarYUV420Frame(VideoFrame *aFrame, int64_t aTimeUs, void *aData, size_t aSize, bool aKeyFrame);
  void SemiPlanarYVU420Frame(VideoFrame *aFrame, int64_t aTimeUs, void *aData, size_t aSize, bool aKeyFrame);
  bool ToVideoFrame(VideoFrame *aFrame, int64_t aTimeUs, void *aData, size_t aSize, bool aKeyFrame);
  bool ToAudioFrame(AudioFrame *aFrame, int64_t aTimeUs, void *aData, size_t aDataOffset, size_t aSize,
                    int32_t aAudioChannels, int32_t aAudioSampleRate);

  
  bool mAudioPaused;
  bool mVideoPaused;

public:
  OmxDecoder(MediaResource *aResource, AbstractMediaDecoder *aDecoder);
  ~OmxDecoder();

  
  virtual void statusChanged();

  
  
  
  
  
  
  
  
  bool Init(sp<MediaExtractor>& extractor);

  bool IsDormantNeeded();

  
  
  bool EnsureMetadata();

  
  
  bool IsWaitingMediaResources();

  bool AllocateMediaResources();
  void ReleaseMediaResources();
  bool SetVideoFormat();
  bool SetAudioFormat();

  void ReleaseDecoder();

  void GetDuration(int64_t *durationUs) {
    *durationUs = mDurationUs;
  }

  void GetVideoParameters(int32_t* aDisplayWidth, int32_t* aDisplayHeight,
                          int32_t* aWidth, int32_t* aHeight) {
    *aDisplayWidth = mDisplayWidth;
    *aDisplayHeight = mDisplayHeight;
    *aWidth = mVideoWidth;
    *aHeight = mVideoHeight;
  }

  void GetAudioParameters(int32_t *numChannels, int32_t *sampleRate) {
    *numChannels = mAudioChannels;
    *sampleRate = mAudioSampleRate;
  }

  bool HasVideo() {
    return mVideoSource != nullptr;
  }

  bool HasAudio() {
    return mAudioSource != nullptr;
  }

  bool ReadVideo(VideoFrame *aFrame, int64_t aSeekTimeUs,
                 bool aKeyframeSkip = false,
                 bool aDoSeek = false);
  bool ReadAudio(AudioFrame *aFrame, int64_t aSeekTimeUs);

  MediaResource *GetResource() {
    return mResource;
  }

  
  nsresult Play();

  
  void Pause();

  
  void PostReleaseVideoBuffer(MediaBuffer *aBuffer, const FenceHandle& aReleaseFenceHandle);
  
  
  void onMessageReceived(const sp<AMessage> &msg);

  int64_t ProcessCachedData(int64_t aOffset, bool aWaitForCompletion);

  sp<MediaSource> GetAudioOffloadTrack() { return mAudioOffloadTrack; }

  void RecycleCallbackImp(TextureClient* aClient);

  static void RecycleCallback(TextureClient* aClient, void* aClosure);
};

}

