















#ifndef ANDROID_GUI_IGONKGRAPHICBUFFERCONSUMER_H
#define ANDROID_GUI_IGONKGRAPHICBUFFERCONSUMER_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/Timers.h>

#include <binder/IInterface.h>
#include <ui/Rect.h>

#include "mozilla/layers/LayersSurfaces.h"

namespace mozilla {

namespace layers {
class TextureClient;
}
}

namespace android {


class IConsumerListener;
class GraphicBuffer;
class Fence;

class IGonkGraphicBufferConsumer : public IInterface {
    typedef mozilla::layers::TextureClient TextureClient;
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


    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t acquireBuffer(BufferItem *buffer, nsecs_t presentWhen) = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t releaseBuffer(int buf, uint64_t frameNumber, const sp<Fence>& releaseFence) = 0;

    
    
    
    
    
    
    
    
    virtual status_t consumerConnect(const sp<IConsumerListener>& consumer, bool controlledByApp) = 0;

    
    
    
    
    virtual status_t consumerDisconnect() = 0;

    
    
    
    
    
    virtual status_t getReleasedBuffers(uint32_t* slotMask) = 0;

    
    
    
    virtual status_t setDefaultBufferSize(uint32_t w, uint32_t h) = 0;

    
    
    
    
    
    
    virtual status_t setDefaultMaxBufferCount(int bufferCount) = 0;

    
    
    
    
    
    virtual status_t disableAsyncBuffer() = 0;

    
    
    
    virtual status_t setMaxAcquiredBufferCount(int maxAcquiredBuffers) = 0;

    
    virtual void setConsumerName(const String8& name) = 0;

    
    
    
    
    virtual status_t setDefaultBufferFormat(uint32_t defaultFormat) = 0;

    
    
    
    virtual status_t setConsumerUsageBits(uint32_t usage) = 0;

    
    
    
    virtual status_t setTransformHint(uint32_t hint) = 0;

    
    virtual void dump(String8& result, const char* prefix) const = 0;

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
