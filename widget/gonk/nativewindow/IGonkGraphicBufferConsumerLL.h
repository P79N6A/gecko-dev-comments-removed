















#ifndef NATIVEWINDOW_IGONKGRAPHICBUFFERCONSUMER_LL_H
#define NATIVEWINDOW_IGONKGRAPHICBUFFERCONSUMER_LL_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/Timers.h>

#include <binder/IInterface.h>
#include <ui/Rect.h>

#include "mozilla/RefPtr.h"

class ANativeWindowBuffer;

namespace mozilla {
namespace layers {
class TextureClient;
}
}

namespace android {


class Fence;
class GraphicBuffer;
class IConsumerListener;
class NativeHandle;

class IGonkGraphicBufferConsumer : public IInterface {
public:

    
    class BufferItem : public Flattenable<BufferItem> {
        friend class Flattenable<BufferItem>;
        size_t getPodSize() const;
        size_t getFlattenedSize() const;
        size_t getFdCount() const;
        status_t flatten(void*& buffer, size_t& size, int*& fds, size_t& count) const;
        status_t unflatten(void const*& buffer, size_t& size, int const*& fds, size_t& count);

    public:
        
        enum { INVALID_BUFFER_SLOT = -1 };
        BufferItem();

        
        
        
        sp<GraphicBuffer> mGraphicBuffer;

        
        sp<Fence> mFence;

        
        Rect mCrop;

        
        
        uint32_t mTransform;

        
        
        uint32_t mScalingMode;

        
        
        
        
        int64_t mTimestamp;

        
        
        bool mIsAutoTimestamp;

        
        uint64_t mFrameNumber;

        
        int mBuf;

        
        
        
        
        
        bool mIsDroppable;

        
        bool mAcquireCalled;

        
        
        bool mTransformToDisplayInverse;
    };

    enum {
        
        
        STALE_BUFFER_SLOT = 1,
        
        NO_BUFFER_AVAILABLE,
        
        PRESENT_LATER,
    };

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t acquireBuffer(BufferItem* buffer, nsecs_t presentWhen) = 0;

    
    
    
    
    
    
    
    
    
    
    
    virtual status_t detachBuffer(int slot) = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t attachBuffer(int *outSlot,
            const sp<GraphicBuffer>& buffer) = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t releaseBuffer(int buf, uint64_t frameNumber, const sp<Fence>& releaseFence) = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t consumerConnect(const sp<IConsumerListener>& consumer, bool controlledByApp) = 0;

    
    
    
    
    
    
    
    virtual status_t consumerDisconnect() = 0;

    
    
    
    
    
    
    
    
    
    virtual status_t getReleasedBuffers(uint64_t* slotMask) = 0;

    
    
    
    
    
    
    virtual status_t setDefaultBufferSize(uint32_t w, uint32_t h) = 0;

    
    
    
    
    
    
    
    
    
    virtual status_t setDefaultMaxBufferCount(int bufferCount) = 0;

    
    
    
    
    
    
    
    
    virtual status_t disableAsyncBuffer() = 0;

    
    
    
    
    
    
    
    
    
    virtual status_t setMaxAcquiredBufferCount(int maxAcquiredBuffers) = 0;

    
    virtual void setConsumerName(const String8& name) = 0;

    
    
    
    
    
    
    virtual status_t setDefaultBufferFormat(uint32_t defaultFormat) = 0;

    
    
    
    
    
    virtual status_t setConsumerUsageBits(uint32_t usage) = 0;

    
    
    
    
    
    virtual status_t setTransformHint(uint32_t hint) = 0;

    
    virtual sp<NativeHandle> getSidebandStream() const = 0;

    
    virtual void dumpToString(String8& result, const char* prefix) const = 0;

    
    virtual already_AddRefed<mozilla::layers::TextureClient>
        getTextureClientFromBuffer(ANativeWindowBuffer* buffer) = 0;

    virtual int getSlotFromTextureClientLocked(mozilla::layers::TextureClient* client) const = 0;

public:
    DECLARE_META_INTERFACE(GonkGraphicBufferConsumer);
};



class BnGonkGraphicBufferConsumer : public BnInterface<IGonkGraphicBufferConsumer>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};


}; 

#endif
