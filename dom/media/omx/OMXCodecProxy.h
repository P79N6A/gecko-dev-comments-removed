




#ifndef OMX_CODEC_PROXY_DECODER_H_
#define OMX_CODEC_PROXY_DECODER_H_


#include <android/native_window.h>
#include <media/IOMX.h>
#include <stagefright/MediaBuffer.h>
#include <stagefright/MediaSource.h>
#include <utils/threads.h>

#include "mozilla/media/MediaSystemResourceClient.h"
#include "nsRefPtr.h"

namespace android {

struct MetaData;

class OMXCodecProxy : public MediaSource
                    , public mozilla::MediaSystemResourceReservationListener
{
public:
  


  struct CodecResourceListener : public virtual RefBase {
    


    virtual void codecReserved() = 0;
    



    virtual void codecCanceled() = 0;
  };

  
  enum class ResourceState : int8_t {
    START,
    WAITING,
    ACQUIRED,
    NOT_ACQUIRED,
    END
  };

  static sp<OMXCodecProxy> Create(
          const sp<IOMX> &omx,
          const sp<MetaData> &meta, bool createEncoder,
          const sp<MediaSource> &source,
          const char *matchComponentName = nullptr,
          uint32_t flags = 0,
          const sp<ANativeWindow> &nativeWindow = nullptr);

    void setListener(const wp<CodecResourceListener>& listener);

    void requestResource();

    
    void ResourceReserved() override;
    void ResourceReserveFailed() override;

    
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

    void notifyResourceReserved();
    void notifyResourceCanceled();

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

    nsRefPtr<mozilla::MediaSystemResourceClient> mResourceClient;
    ResourceState mState;

    
    wp<CodecResourceListener> mListener;
};

}  

#endif
