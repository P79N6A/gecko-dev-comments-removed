




#ifndef OMX_CODEC_PROXY_DECODER_H_
#define OMX_CODEC_PROXY_DECODER_H_


#include <android/native_window.h>
#include <media/IOMX.h>
#include <stagefright/MediaBuffer.h>
#include <stagefright/MediaSource.h>
#include <utils/threads.h>

#include "MediaResourceManagerClient.h"

namespace android {

struct MetaData;

class OMXCodecProxy : public MediaSource,
                      public MediaResourceManagerClient::EventListener
{
public:
  struct EventListener : public virtual RefBase {
    virtual void statusChanged() = 0;
  };

  static sp<OMXCodecProxy> Create(
          const sp<IOMX> &omx,
          const sp<MetaData> &meta, bool createEncoder,
          const sp<MediaSource> &source,
          const char *matchComponentName = nullptr,
          uint32_t flags = 0,
          const sp<ANativeWindow> &nativeWindow = nullptr);

    MediaResourceManagerClient::State getState();

    void setEventListener(const wp<EventListener>& listener);

    void requestResource();
    bool IsWaitingResources();

    
    virtual void statusChanged(int event);

    
    virtual status_t start(MetaData *params = nullptr);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();

    virtual status_t read(
            MediaBuffer **buffer, const ReadOptions *options = nullptr);

    virtual status_t pause();

protected:
    OMXCodecProxy(
        const sp<IOMX> &omx,
        const sp<MetaData> &meta,
        bool createEncoder,
        const sp<MediaSource> &source,
        const char *matchComponentName,
        uint32_t flags,
        const sp<ANativeWindow> &nativeWindow);

    virtual ~OMXCodecProxy();

    void notifyStatusChangedLocked();

private:
    OMXCodecProxy(const OMXCodecProxy &);
    OMXCodecProxy &operator=(const OMXCodecProxy &);

    Mutex mLock;

    sp<IOMX> mOMX;
    sp<MetaData> mSrcMeta;
    char *mComponentName;
    bool mIsEncoder;
    
    uint32_t mFlags;
    sp<ANativeWindow> mNativeWindow;

    sp<MediaSource> mSource;

    sp<MediaSource> mOMXCodec;
    sp<MediaResourceManagerClient> mClient;
    MediaResourceManagerClient::State mState;

    sp<IMediaResourceManagerService> mManagerService;
    wp<OMXCodecProxy::EventListener> mEventListener;
};

}  

#endif  
