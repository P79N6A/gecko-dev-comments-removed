















#ifndef ANDROID_GUI_IGRAPHICBUFFERPRODUCER_H
#define ANDROID_GUI_IGRAPHICBUFFERPRODUCER_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/Errors.h>
#include <utils/RefBase.h>

#include <binder/IInterface.h>

#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>
#include <ui/Rect.h>

namespace android {


class Surface;















class IGraphicBufferProducer : public IInterface
{
public:
    DECLARE_META_INTERFACE(GraphicBufferProducer);

    enum {
        BUFFER_NEEDS_REALLOCATION = 0x1,
        RELEASE_ALL_BUFFERS       = 0x2,
    };

    
    
    
    
    
    virtual status_t requestBuffer(int slot, sp<GraphicBuffer>* buf) = 0;

    
    
    
    virtual status_t setBufferCount(int bufferCount) = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t dequeueBuffer(int *slot, sp<Fence>* fence,
            uint32_t w, uint32_t h, uint32_t format, uint32_t usage) = 0;

    
    
    
    
    
    
    
    
    
    
    
    

    struct QueueBufferInput : public Flattenable {
        inline QueueBufferInput(const Parcel& parcel);
        inline QueueBufferInput(int64_t timestamp,
                const Rect& crop, int scalingMode, uint32_t transform,
                sp<Fence> fence)
        : timestamp(timestamp), crop(crop), scalingMode(scalingMode),
          transform(transform), fence(fence) { }
        inline void deflate(int64_t* outTimestamp, Rect* outCrop,
                int* outScalingMode, uint32_t* outTransform,
                sp<Fence>* outFence) const {
            *outTimestamp = timestamp;
            *outCrop = crop;
            *outScalingMode = scalingMode;
            *outTransform = transform;
            *outFence = fence;
        }

        
        virtual size_t getFlattenedSize() const;
        virtual size_t getFdCount() const;
        virtual status_t flatten(void* buffer, size_t size,
                int fds[], size_t count) const;
        virtual status_t unflatten(void const* buffer, size_t size,
                int fds[], size_t count);

    private:
        int64_t timestamp;
        Rect crop;
        int scalingMode;
        uint32_t transform;
        sp<Fence> fence;
    };

    
    struct QueueBufferOutput {
        inline QueueBufferOutput() { }
        inline void deflate(uint32_t* outWidth,
                uint32_t* outHeight,
                uint32_t* outTransformHint,
                uint32_t* outNumPendingBuffers) const {
            *outWidth = width;
            *outHeight = height;
            *outTransformHint = transformHint;
            *outNumPendingBuffers = numPendingBuffers;
        }
        inline void inflate(uint32_t inWidth, uint32_t inHeight,
                uint32_t inTransformHint, uint32_t inNumPendingBuffers) {
            width = inWidth;
            height = inHeight;
            transformHint = inTransformHint;
            numPendingBuffers = inNumPendingBuffers;
        }
    private:
        uint32_t width;
        uint32_t height;
        uint32_t transformHint;
        uint32_t numPendingBuffers;
    };

    virtual status_t queueBuffer(int slot,
            const QueueBufferInput& input, QueueBufferOutput* output) = 0;

    
    
    
    virtual void cancelBuffer(int slot, const sp<Fence>& fence) = 0;

    
    
    virtual int query(int what, int* value) = 0;

    
    
    
    
    
    virtual status_t setSynchronousMode(bool enabled) = 0;

    
    
    
    
    
    
    
    
    
    
    virtual status_t connect(int api, QueueBufferOutput* output) = 0;

    
    
    
    
    
    
    
    
    virtual status_t disconnect(int api) = 0;
};



class BnGraphicBufferProducer : public BnInterface<IGraphicBufferProducer>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};


}; 

#endif 
